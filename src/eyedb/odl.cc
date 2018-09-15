/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
   EyeDB is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   EyeDB is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA 
*/

/*
   Author: Eric Viara <viara@sysra.com>
*/


#include "odl.h"
#include "misc.h"
#include "eyedb/internals/ClassPeer.h"
#include "eyedb/internals/ObjectPeer.h"
#include "oql_p.h"
#include "CollectionBE.h"
#include <sstream>
#include <pthread.h>
#include "eyedblib/butils.h"
#include "oqlctb.h"

using std::ostringstream;

#define NO_DIRECT_SET

#define NEW_REORDER
#define NEW_DIFF_RELSHIP

namespace eyedb {

  LinkedList *odl_decl_list;
  ProgLang odl_lang = (ProgLang)0;
  int odl_error;
  int odl_diff;
  const char *odl_rootclass;
  LinkedList qseq_list;
  odlAgregatClass *odlAgregatClass::superclass;
  unsigned int odlAgregatClass::class_count;
  static const char *odl_prefix;
  static std::string odl_db_prefix;
  static Bool odl_gencode = False;
  Bool odl_system_class = False;
  Bool odl_rmv_undef_attrcomp = False;
  Bool odl_update_index = False;
  Bool odl_dynamic_attr = False;
  Bool odl_class_enums = False;
  Bool odl_sch_rm = False;
  Bool odl_smartptr = False;

  LinkedList odl_cls_rm;
  static Bool odl_not_check_missing = IDBBOOL(getenv("EYEDBNOCHECKMISSING"));
  FILE *odl_fd = stdout;
  static FILE *odl_fdnull = fopen("/dev/null", "rw");
  LinkedList odlAgregatClass::declared_list;

  static const char odlUPDLIST[] = "eyedb:odl:update:list";
  static const char odlSELF[] = "eyedb:odl:self";
  char odlMTHLIST[] = "eyedb:odl:method";
  char odlGENCOMP[] = "eyedb:odl:gencomp";
  char odlGENCODE[] = "eyedb:odl:gencode";

#define odlUPDLIST(M) ((LinkedList *)(M)->getUserData(odlUPDLIST))
#define odlMTHLIST(C) ((LinkedList *)(C)->getUserData(odlMTHLIST))

  FILE *
  run_cpp(FILE *fd, const char *cpp_cmd, const char *cpp_flags,
	  const char *file);

  static
  Database *odl_get_dummy_db(Schema *m)
  {
    static Database *dumdb;
    if (!dumdb) {
      dumdb = new Database("dummy");
      dumdb->setSchema(m);
    }

    return dumdb;
  }

  static int
  magorder_error(const Class *cls, const char *attrname, const char *type)
  {
    if (attrname)
      odl_add_error
	(std::string("attribute: ") + cls->getName() + "::" +
	 attrname + " cannot have several " + type + " specifications\n");
    else
      odl_add_error
	(std::string("class: ") + cls->getName() +
	 " cannot have several " + type + " specifications\n");

    return 0;
  }

  static std::string odl_str_error;

  void
  odl_add_error(Status s)
  {
    ostringstream ostr;
    ostr << s;
    odl_str_error += ostr.str();
    if (odl_str_error[odl_str_error.size()-1] != '\n')
      odl_str_error += "\n";
    odl_error++;
  }

  void
  odl_add_error(const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    char *s = eyedblib::getFBuffer(fmt, ap);
    vsprintf(s, fmt, aq);
    va_end(ap);
    odl_str_error += s;
    odl_error++;
  }

  void
  odl_add_error(const std::string &s)
  {
    odl_str_error += s;
    odl_error++;
  }

  static Status
  odl_status_error(int r = 0)
  {
    if (odl_error)
      return Exception::make(IDB_ERROR, odl_str_error.c_str());

    if (odl_str_error != std::string(""))
      fprintf(stderr, "%s\n", odl_str_error.c_str());
    else if (r)
      return Exception::make(IDB_ERROR, "");

    return Success;
  }

  static Class *
  getClass(Schema *m, const char *name, const char *prefix)
  {
    Class *cls;

    cls = m->getClass(name);

    if (cls) return cls;

    return m->getClass(makeName(name, prefix));
  }

  static void
  odl_copy_attr_comp_sets(Class *ocls, Class *cls)
  {
    unsigned int attr_cnt;
    Attribute **attrs = (Attribute **)cls->getAttributes(attr_cnt);

    for (int i = 0; i < attr_cnt; i++)
      {
	const Attribute *oattr = ocls->getAttribute(attrs[i]->getName());
	if (oattr) {
	  /*
	    printf("%s::%s attr_comp_set_oid %s -> %s\n",
	    oattr->getAttrCompSetOid().toString(),
	    attrs[i]->getAttrCompSetOid().toString(),
	    attrs[i]->getDynClassOwner()->getName(),
	    attrs[i]->getName());
	  */
	  attrs[i]->setAttrCompSetOid(oattr->getAttrCompSetOid());
	}
      }
  }

  static bool is_in(const std::vector<Class *> &rm_v, Class *cls) {
    std::vector<Class *>::const_iterator begin = rm_v.begin();
    std::vector<Class *>::const_iterator end = rm_v.end();
    while (begin != end) {
      if (cls == *begin)
	return true;
      ++begin;
    }
    return false;
  }

  static void
  odl_check_removed(Schema *m, LinkedList *&list)
  {
    if (odl_sch_rm) {
      if (!list)
	list = new LinkedList();
      const LinkedList *cls_list = m->getClassList();
      odlUpdateHint *rmHints = new odlUpdateHint(odlUpdateHint::Remove);
      Class *cls;
      // bug corrected 21/01/06
#if 1
      std::vector<Class *> rm_v;

      unsigned int cnt = 0;

      do {
	LinkedListCursor c(cls_list);
	cnt = 0;
	while (c.getNext((void *&)cls)) {
	  if (cls->isSystem() || is_in(rm_v, cls))
	    continue;

	  Class **subclasses;
	  unsigned int subclass_cnt;
	  cls->getSubClasses(subclasses, subclass_cnt);
	  bool subclass_notin = false;
	  for (int n = 0; n < subclass_cnt; n++) {
	    if (subclasses[n] != cls && !is_in(rm_v, subclasses[n])) {
	      subclass_notin = true;
	      break;
	    }
	  }

	  if (!subclass_notin) {
	    rm_v.push_back(cls);
	    cnt++;
	  }
	}

      } while(cnt);

      std::vector<Class *>::iterator begin = rm_v.begin();
      std::vector<Class *>::iterator end = rm_v.end();
      while (begin != end) {
	cls = *begin;
	odlClassSpec *clsspec = new odlClassSpec(cls->getName(), 0,
						 cls->getName(), 0);
	new odlAgregatClass(rmHints, odl_Struct,  clsspec,
			    new odlDeclRootList());
	++begin;
      }
#else
      LinkedListCursor c(cls_list);
      while (c.getNext((void *&)cls)) {
	if (cls->isSystem())
	  continue;
	odlClassSpec *clsspec = new odlClassSpec(cls->getName(), 0,
						 cls->getName(), 0);
	new odlAgregatClass(rmHints, odl_Struct,  clsspec,
			    new odlDeclRootList());
      }
#endif

      return;
    }
    
    if (odl_cls_rm.getCount() == 0)
      return;

    if (!list)
      list = new LinkedList();

    char *name;
    LinkedListCursor c(odl_cls_rm);
    odlUpdateHint *rmHints = new odlUpdateHint(odlUpdateHint::Remove);
    while (c.getNext((void * &)name)) {
      //printf("adding declaration %s for removal\n", name);
      odlClassSpec *clsspec = new odlClassSpec(name, 0, name, 0);
      new odlAgregatClass(rmHints, odl_Struct,
			  clsspec,
			  new odlDeclRootList());
    }
  }

  static Bool
  odl_check_renamed_from(Schema *m, LinkedList *list, Class *cls)
  {
    LinkedListCursor clx(list);
    odlDeclaration *decl;

    while (clx.getNext((void *&)decl))
      if (decl->asAgregatClass()) {
	odlUpdateHint *upd_hints = decl->asAgregatClass()->upd_hints;
	if (upd_hints && upd_hints->type == odlUpdateHint::RenameFrom &&
	    !strcmp(upd_hints->detail, cls->getName()))
	  return True;
      }

    return False;
  }

  static void
  odl_check_missing(Schema *m, LinkedList *list)
  {
    LinkedListCursor cm(m->getClassList());
    Class *cls;

    while (cm.getNext((void *&)cls))
      {
	if (cls->isPartiallyLoaded())
	  {
	    Status s = m->manageClassDeferred(cls);
	    if (s)
	      {
		odl_add_error(s);
		break;
	      }
	  }

	if (cls->isSystem() || cls->asCollectionClass())
	  continue;

	LinkedListCursor cl(list);
	odlDeclaration *decl;
	Bool found = False;
	while (cl.getNext((void *&)decl))
	  if (!strcmp(decl->getName(), cls->getName()) ||
	      (decl->getAliasName() &&
	       !strcmp(decl->getAliasName(), cls->getName())))
	    {
	      found = True;
	      break;
	    }

	if (!found)
	  {
#define MANAGE_MISSING_CLASSES
	    // disconnected the 21/05/2001
	    // because this put the bordel in the components of missing
	    // classes (index, methods, constraints)!
#ifdef MANAGE_MISSING_CLASSES

	    if (!odl_gencode) { // add not declared class on the fly
	      if (!odl_check_renamed_from(m, list, cls)) {
#if 0
		list->insertObject
		  (new odlAgregatClass(0, odl_Declare,
				       new odlClassSpec(cls->getName(), 0, 0, 0),
				       new odlDeclRootList()));
#else
		new odlAgregatClass(0, odl_Declare,
				    new odlClassSpec(cls->getName(), 0, 0, 0),
				    new odlDeclRootList());
#endif
		//printf("declaring %p %s\n", cls, cls->getName());
	      }
	    }
	    else if (!odl_not_check_missing)
#endif
	      odl_add_error("class %s is missing in ODL\n", cls->getName());
	  }
      }
  }

  static Bool
  odl_has_attribute(const Class *origcls, const Class *cls,
		    const char *attrname)
  {
#if 1
    return True;
#endif
    if (!cls)
      return False;

    if (cls->getAttribute(attrname))
      {
	odl_add_error("attribute '%s' exists both in class '%s' and its "
		      "superclass '%s'\n",
		      attrname,  origcls->getName(), cls->getName());
	return True;
      }

    return odl_has_attribute(origcls, cls->getParent(), attrname);
  }


  static void
  odl_check_attributes(const Class *cls)
  {
    unsigned int attr_cnt;
    Attribute **attrs = (Attribute **)cls->getAttributes(attr_cnt);
  
    for (int i = 0; i < attr_cnt; i++)
      {
	const Attribute *attr = attrs[i];
	if (!attr->isNative() && attr->getClassOwner()->compare(cls))
	  odl_has_attribute(cls, cls->getParent(), attr->getName());
      }
  }

  int
  odl_realize(Database *db, Schema *m, LinkedList *list,
	      const char *prefix, const char *db_prefix,
	      const char *package, Bool diff)
  {
    if (!prefix)    prefix = "";
    if (!db_prefix) db_prefix = "";
    if (!package)   package = "";

    odl_prefix = prefix;
    odl_db_prefix = db_prefix;

    odl_check_removed(m, list);

    if (list && list->getCount()) {
      odlDeclaration *decl;
      
      m->setUserData(odlUPDLIST, new LinkedList());
      
      odl_check_missing(m, list);
      if (odl_error)
	return 1;

      LinkedListCursor curs(list);

      while (curs.getNext((void* &)decl))
	decl->record(db, m, prefix, db_prefix);

      if (odl_error)
	return 1;

      curs.restart();
      while (curs.getNext((void* &)decl))
	decl->realize(db, m, prefix, package, diff);

      if (odl_error)
	return 1;

#ifdef NEW_REORDER
      curs.restart();
      while (curs.getNext((void* &)decl))
	if (decl->asAgregatClass())
	  decl->asAgregatClass()->preManage(m);
#endif

      if (odl_error)
	return 1;

      curs.restart();
      while (curs.getNext((void* &)decl))
	if (decl->asAgregatClass())
	  decl->asAgregatClass()->postRealize(db, m, prefix);

      if (odl_error)
	return 1;

      curs.restart();
      while (curs.getNext((void* &)decl))
	if (decl->asAgregatClass())
	  decl->asAgregatClass()->manageDifferences(db, m, diff);
    }

    const LinkedList *_class = m->getClassList();
    LinkedListCursor curs(_class);

    Class *cl;

    if (!odl_error && !odl_diff)
      while (curs.getNext((void *&)cl)) {
	Status status = cl->checkInverse(m);
	if (status)
	  odl_add_error(status);
      
	odl_check_attributes(cl);
      
	if (cl->getUserData())
	  odl_copy_attr_comp_sets((Class *)cl->getUserData(), cl);
      }

    if (odl_error) {
      odl_add_error("%d error%s found, compilation aborted\n",
		    odl_error, (odl_error > 1 ? "s" : ""));
      return 1;
    }

    return 0;
  }

  static char *
  get_superclass_name(const char *prefix = "")
  {
    static char *spname;

    if (!odlAgregatClass::superclass || spname)
      return spname;

    const char *name = odlAgregatClass::superclass->getName();
    if (prefix && *prefix)
      {
	spname = (char *)malloc(strlen(prefix)+strlen(name)+1);
	strcpy(spname, prefix);
	strcat(spname, name);
      }
    else
      spname = strdup(name);

    return spname;
  }

  int
  odl_generate_code(Database *db, Schema *m, ProgLang lang,
		    LinkedList *list,
		    const char *package, const char *schname,
		    const char *c_namespace,
		    const char *prefix, const char *db_prefix, Bool _export,
		    const GenCodeHints &gc_hints)
  {
    odl_gencode = True;
    odl_class_enums = gc_hints.class_enums;

    if (odl_realize(db, m, list, prefix, db_prefix, package))
      return 1;

    Class *superclass = NULL;
    if (odlAgregatClass::superclass) {
      superclass = m->getClass(get_superclass_name(odl_prefix));
      if (odlAgregatClass::superclass->getAgregSpec() == odl_RootClass)
	superclass->setIsRootClass();
    }

    Status status = m->generateCode
      (lang, package, schname,
       c_namespace, prefix, db_prefix, gc_hints,
       _export,
       superclass,
       &qseq_list);

    if (status) {
      odl_add_error(status);
      return 1;
    }

    return 0;
  }

#define SWAP_CLSNAMES

  odlClassSpec::odlClassSpec(const char *_aliasname, const char *_parentname,
			     const char *_classname, odlCollImplSpec *_coll_impl_spec)
  {
    if (getenv("SYSTEM_UPDATE"))
      {
	classname = (_aliasname ? strdup(_aliasname) : strdup(_classname));
	aliasname = 0;
      }
    else
      {
#ifdef SWAP_CLSNAMES
	classname = (_classname ? strdup(_classname) : strdup(_aliasname));
	aliasname = strdup(_aliasname);
#else
	classname = strdup(_classname);
	aliasname = (_aliasname ? strdup(_aliasname) : 0);
#endif
      }

    parentname = (_parentname ? strdup(_parentname) : 0);
    coll_impl_spec = _coll_impl_spec;
  }

  static void
  rename_classes(Schema *m)
  {
    Class *cl;
    LinkedListCursor c(m->getClassList());

    while (c.getNext((void *&)cl))
      if (cl->asAgregatClass())
	cl->asAgregatClass()->completeInverse(m);

    LinkedListCursor cx(m->getClassList());

    while (cx.getNext((void *&)cl)) {
      if (strcmp(cl->getAliasName(), cl->getName()))
	cl->setName(cl->getAliasName());
    }
  }

  void
  odl_skip_volatiles(Database *db, Schema *m)
  {
    if (!odlAgregatClass::superclass ||
	odlAgregatClass::superclass->getAgregSpec() != odl_RootClass)
      return;

    db->transactionBegin();
    LinkedListCursor c(m->getClassList());
    Class *cl, *toSuppress = 0;

    char name[64];
    sprintf(name, "%s%s", odl_prefix, get_superclass_name());
    while (c.getNext((void *&)cl))
      {
	if (!strcmp(cl->getName(), name))
	  toSuppress = cl;
	else if (cl->getParent() && !strcmp(cl->getParent()->getName(), name))
	  ClassPeer::setParent(cl, cl->getParent()->getParent());
      }

    if (toSuppress)
      m->suppressClass(toSuppress);

    rename_classes(m);

    //  db->transactionAbort();
    db->transactionCommit();
  }

  int odlDeclaration::check(Schema *m, const char *prefix)
  {
    /*
      Class *cl;
      if (cl = getClass(m, name, prefix))
      {
      if (!cls->compare(cl))
      odl_add_error("class '%s' already exists in schema and differs from input class\n", name);
      return 1;
      }
    */

    return 0;
  }

  static void
  odl_rename_class(Schema *m, const Class *cls, const char *name)
  {
    odlUPDLIST(m)->insertObjectLast(new odlRenameClass(cls, name));
  }

  static void
  odl_migrate_attributes(Schema *m, const Class *cls);

  static void
  odl_remove_class(Database *db, Schema *m, const Class *cls);

  int
  odlAgregatClass::record(Database *db, Schema *m,
			  const char *prefix, const char *db_prefix)
  {
    if (parentname) {
      parent = eyedb::getClass(m, parentname, prefix);
      if (!parent)
	odl_add_error("cannot find parent '%s' for agregat_class '%s'\n",
		      parentname, name);
    }
    else
      parent = 0;

    if (agrspec == odl_Struct || agrspec == odl_SuperClass ||
	agrspec == odl_RootClass) {
      cls = new StructClass(makeName(name, prefix), parent);
      cls->setUserData(odlGENCODE, AnyUserData);
    }
    else if (agrspec == odl_Declare) {
      cls = m->getClass(name);
      if (!cls)
	odl_add_error("cannot find declared class '%s'\n", name);
      
      if (!odl_error)
	cls = new Class(cls->getName(), cls->getParent());
    }
    else if (agrspec == odl_NativeClass) {
      cls = m->getClass(name);
      if (!cls)
	odl_add_error("cannot find native class '%s'\n", name);
      else if (!cls->isSystem())
	odl_add_error("class '%s' is not native\n",
		      name);
      else if (aliasname && strcmp(aliasname, name))
	odl_add_error("cannot set an alias name on the native"
		      " class '%s'\n", name);
      else {
	/*
	  odlDeclRootLink *l = decl_list->first;
	  while (l) {
	  if (!l->x->asExecSpec()) {
	  odl_add_error("native class '%s' can include "
	  " only methods or triggers.\n", name);
	  break;
	  }
	  l = l->next;
	  }
	*/
      }

#if 0
      if (!odl_error) {
	printf("NATIVE CLASS %s %s -> %p [system=%d] ", cls->getName(),
	       cls->isSystem() ? "system" : "user", cls, system);
      }
#endif
      if (!odl_error) {
	cls = new Class(cls->getName(), cls->getParent());
	cls->setUserData(odlGENCOMP, AnyUserData);
	ClassPeer::setMType(cls, Class::System);
	if (db)
	  cls->setDatabase(db);
      }
    }
    else {
      cls = new UnionClass(makeName(name, prefix), parent);
      cls->setUserData(odlGENCODE, AnyUserData);
    }

    if (!odl_lang && upd_hints && upd_hints->type == odlUpdateHint::RenameFrom)
      {
	const char *xname = upd_hints->detail;
	ocls = eyedb::getClass(m, xname, prefix);
	if (!ocls)
	  odl_add_error("class %s: does not exist in database\n", xname);
	else
	  {
	    odl_rename_class(m, cls, xname);
	    ocls->setName(name);
	  }
      }
    else
      ocls = eyedb::getClass(m, (aliasname ? aliasname : name), prefix);

    if (!odl_lang && upd_hints && upd_hints->type == odlUpdateHint::Remove &&
	!ocls)
      odl_add_error("cannot remove class '%s'\n", name);
    else if (agrspec != odl_NativeClass && agrspec != odl_Declare)
      {
	if (aliasname)
	  cls->setAliasName(aliasname);
	else if (db_prefix)
	  cls->setAliasName(makeName(name, db_prefix));

	assert(ocls != cls);
	if (ocls && ocls != cls)
	  {
	    m->suppressClass(ocls);
	    cls->setUserData(ocls);
	    cls->setExtentImplementation(ocls->getExtentImplementation(),
					 True);
	    ObjectPeer::setOid(cls, ocls->getOid());
	  }
      
	if (upd_hints && upd_hints->type == odlUpdateHint::Remove)
	  {
	    if (ocls->isSystem()) {
	      odl_add_error("cannot remove the system class '%s'\n",
			    ocls->getName());
	      return 1;
	    } else {
	      odl_remove_class(db, m, cls);
	      cls = 0;
	    }
	  }
	else
	  m->addClass(cls);
      }
    else if (odl_gencode && ocls)
      {
	m->suppressClass(ocls);
	if (upd_hints && upd_hints->type == odlUpdateHint::Remove)
	  {
	    if (ocls->isSystem()) {
	      odl_add_error("cannot remove the system class '%s'\n",
			    ocls->getName());
	      return 1;
	    } else {
	      odl_remove_class(db, m, cls);
	      cls = 0;
	    }
	  }
	else
	  m->addClass(cls);

	if (!odl_error && agrspec == odl_NativeClass) {
	  /*
	    printf("should report attributes from ocls %d to cls %d\n",
	    ocls->getAttributesCount(), cls->getAttributesCount());
	  */
	  unsigned int attr_cnt;
	  const Attribute **attrs = ocls->getAttributes(attr_cnt);
	  Status status = cls->setAttributes((Attribute **)attrs, attr_cnt);
	  if (status)
	    odl_add_error(status);
	}
      }
  
    if (odl_system_class && !odl_error)
      ClassPeer::setMType(cls, Class::System);

    if (cls)
      cls->setUserData(odlSELF, this);

    if (cls && coll_impl_spec) {
      // TBD: Index -> Coll
      odlCollImplSpecItem::Type type;
      char *hints;
      if (!coll_impl_spec->make_class_prologue(cls->getName(), type, hints))
	return 1;

      if (type != odlCollImplSpecItem::HashIndex &&
	  type != odlCollImplSpecItem::BTreeIndex) {
	odl_add_error("class %s: extent implementation '%s' must be an hashindex or a btreeindex", cls->getName());
	return 1;

      }
      IndexImpl *idximpl;
      Status s = IndexImpl::make
	((db ? db : odl_get_dummy_db(m)),
	 (type == odlCollImplSpecItem::HashIndex ? IndexImpl::Hash :
	  IndexImpl::BTree), hints, idximpl);
      if (!s) {
	if (cls->getOid().isValid()) {
	  IndexImpl *oidximpl = cls->getExtentImplementation();
	  if (!oidximpl->compare(idximpl)) {
	    odl_add_error("class %s: extent implementation '%s' cannot be "
			  "dynamically changed to '%s' using eyedbodl\n",
			  cls->getName(),
			  oidximpl->getHintsString().c_str(),
			  idximpl->getHintsString().c_str());
	    return 1;
	  }
	}
	else
	  s = cls->setExtentImplementation(idximpl);
      }

      if (s) {
	odl_add_error(s);
	return 1;
      }
    }

    return 0;
  }
      
  static void
  array_make(odlDeclItem *item, int* &dims, int &ndims)
  {
    if (item->array_list)
      {
	int count = item->array_list->count;
	odlArrayItemLink *l = item->array_list->first;
	dims = new int[count];

	for (ndims = 0; ndims < count; ndims++, l = l->next)
	  {
	    int v = l->x;
	    if (v>0)
	      dims[ndims] = v;
	    else
	      dims[ndims] = idbVarDim(-v);
	  }
      }
    else
      {
	dims = 0;
	ndims = 0;
      }
  }

  static void
  sign_error(const char *method_name, const Class *cls)
  {
    odl_add_error("invalid hash method signature '%s' in "
		  "class '%s' in index declaration: must be "
		  "`classmethod int %s(in rawdata, in int)'.\n",
		  method_name, cls->getName(), method_name);
  }

#define FIND_IDX_OPTIM

  Status
  odlIndex::findIndex(Schema *m, Index *&idx_obj)
  {
    if (!m->getDatabase())
      return Success;

#ifdef FIND_IDX_OPTIM
    Status s;
    static ObjectArray *index_arr;
    static int index_arr_cnt;
    if (!index_arr)
      {
	index_arr = new ObjectArray();
	OQL q(m->getDatabase(), "select index");
	s = q.execute(*index_arr);
	if (s) return s;
	index_arr_cnt = index_arr->getCount();
      }

    // a hash table should be better!!
    const char *idxname = idx_obj->getName().c_str();
    for (int i = 0; i < index_arr_cnt; i++)
      if (!strcmp(idxname, ((Index *)(*index_arr)[i])->getName().c_str()))
	{
	  idx_obj->release();
	  idx_obj = (Index *)(*index_arr)[i];
	  return Success;
	}

    return Success;
#else      
    OQL q(m->getDatabase(), "select index.name = \"%s\"", idx_obj->getName());

    Object *o;
    Bool found;
    Status s;

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s) return s;

    if (obj_arr.getCount())
      {
	idx_obj->release();
	idx_obj = (Index *)obj_arr[0];
      }

    return Success;
#endif
  }

  static Bool
  odlIsType(odlDeclItem *item, const char *type)
  {
    if (strcmp(item->typname, type) || item->isref ||
	!item->array_list || item->array_list->count != 1)
      return False;

    return True;
  }

  static Bool
  odlIsString(odlDeclItem *item)
  {
    return odlIsType(item, char_class_name);
  }

  static Bool
  odlIsRawData(odlDeclItem *item)
  {
    return odlIsType(item, "byte");
  }

  static Signature *
  makeSign(Schema *m)
  {
    Signature *sign = new Signature();
    ArgType *type;

#ifdef NO_DIRECT_SET
    type = sign->getRettype();
    *type = *ArgType::make(m, int32_class_name);
#else
    type = ArgType::make(m, int32_class_name);
#endif

    type->setType((ArgType_Type)(type->getType() | OUT_ARG_TYPE), False);

#ifndef NO_DIRECT_SET
    sign->setRettype(type);
#endif
    sign->setNargs(2);

#ifdef NO_DIRECT_SET
    sign->setTypesCount(2);
    type = sign->getTypes(0);
    *type = *ArgType::make(m, "rawdata");
#else
    type = ArgType::make(m, "rawdata");
#endif

    type->setType((ArgType_Type)(type->getType() | IN_ARG_TYPE), False);

#ifndef NO_DIRECT_SET
    sign->setTypes(0, type);
#endif

#ifdef NO_DIRECT_SET
    sign->setTypesCount(2);
    type = sign->getTypes(1);
    *type = *ArgType::make(m, int32_class_name);
#else
    type = ArgType::make(m, int32_class_name);
#endif
    type->setType((ArgType_Type)(type->getType() | IN_ARG_TYPE), False);

#ifndef NO_DIRECT_SET
    sign->setTypes(1, type);
#endif

    return sign;
  }

  static Signature *
  getHashSignature(Schema *m)
  {
    static Signature *sign;

    if (!sign)
      sign = makeSign(m);
 
    return sign;
  }

  odlBool
  odlDeclItem::hasInverseAttr() const
  {
    return inverse ? odlTrue : odlFalse;
#if 0
    if (!attr_list)
      return odlFalse;

    odlAttrItemLink *l = attr_list->first;

    while (l)
      {
	odlAttrItem *attr = l->x;
	if (attr->asInverse())
	  return odlTrue;
	l = l ->next;
      }

    return odlFalse;
#endif
  }

#if 0
  void
  odlDeclItem::attr_list_make(Schema *m, Class *cls,
			      int& index_mode, Index *&idx_item,
			      Index *&idx_comp,
			      char **invcname, char **invfname,
			      ClassComponent *comp[], int &comp_cnt)
  {
    index_mode = 0;
    idx_item = idx_comp = 0;
    *invcname = *invfname = 0;
  
    int ocomp_cnt = comp_cnt;

    if (attr_list)
      {
	odlAttrItemLink *l = attr_list->first;

	while (l)
	  {
	    odlAttrItem *attr = l->x;
	    if (attr->asInverse())
	      {
		odlInverse *inv = attr->asInverse();
		*invcname = inv->classname;
		*invfname = inv->attrname;
	      }
	    if (attr->asCardinality())
	      {
		if (!coll_spec)
		  {
		    odl_add_error("cannot set cardinality_constraint on a not collection item '%s'\n", attrname);
		  }
		else
		  {
		    odlCardinality *card = attr->asCardinality();
		    comp[comp_cnt++] =
		      new CardinalityConstraint(0, cls, attrname,
						card->bottom, (int)card->bottom_excl,
						card->top, (int)card->top_excl);
		  }
	      }
	    /*
	      else
	      abort();
	    */

	    l = l->next;
	  }
      }
  }
#endif

  static int key_w(const char *s, const char *key)
  {
    if (!s)
      return 0;

    if (!strcmp(s, key))
      {
	odl_add_error("use of invalid C++ keyword '%s' for attribute name or type name\n", s);
	return 1;
      }
    return 0;
  }

#define KEY_W(K) if (key_w(typname, K) || key_w(attrname, K)) return 1

  static int
  check_name(Schema *m, const char *classname,
	     const char *typname, const char *attrname,
	     const char *prefix)
  {
    KEY_W("delete");
    KEY_W("operator");

    if (getClass(m, attrname, prefix))
      {
	// why??
	// because may be ambiguous in uses such as OQL, C++ or Java
	odl_add_error("cannot use a type name for an attribute name: '%s %s' in class '%s'\n", (typname ? typname : "identifier"), attrname, classname);
	return 1;
      }

    const LinkedList *_class = m->getClassList();
    LinkedListCursor *c = _class->startScan();

    Class *cl;

    char tmp[128];
    sprintf(tmp, "%s%s", prefix, classname);
    while (_class->getNextObject(c, (void *&)cl))
      if (cl->asEnumClass())
	{
	  if (!strcmp(cl->getName(), tmp))
	    continue;

	  int count;
	  const EnumItem **items = cl->asEnumClass()->getEnumItems(count);
	  for (int i = 0; i < count; i++)
	    if (!strcmp(attrname, items[i]->getName()))
	      {
		if (!typname)
		  odl_add_error("enum item name '%s' found in '%s' and '%s'\n",
				attrname, cl->getName(), tmp);
		else
		  odl_add_error("cannot use a enum item name for "
				"an attribute name: '%s'\n", attrname);
		_class->endScan(c);
		return 1;
	      }
	}

    _class->endScan(c);
  
    return 0;
  }

  static void
  add_quoted_seq(Class *cls, const char *quoted_seq)
  {
    char *tied_code = cls->getTiedCode();
    int len = strlen(quoted_seq)+1;

    if (!tied_code)
      tied_code = (char *)calloc(len, sizeof(char));
    else
      tied_code = (char *)realloc(tied_code, len+strlen(tied_code));

    strcat(tied_code, quoted_seq);

    cls->setTiedCode(tied_code);
  }


  struct ClassNotFound {

    static LinkedList list;

    static Bool get(const char *name) {
      LinkedListCursor c(list);
      char *s;

      while (c.getNext((void *&)s))
	if (!strcmp(s, name))
	  return True;

      return False;
    }

    static void error(const char *name, const char *prefix = 0) {
      char *x = strdup(name);
      if (x[strlen(x)-1] == '*')
	x[strlen(x)-1] = 0;

      if (get(x))
	{
	  free(x);
	  return;
	}

      if (prefix) {
	std::string str = std::string(prefix) + " : cannot find class '%s'\n";
	odl_add_error(str.c_str(), x);
      }
      else
	odl_add_error("cannot find class '%s'\n", x);
      put(x);
    }

    static void put(const char *name) {
      list.insertObjectLast((void *)name);
    }

  };

  LinkedList ClassNotFound::list;

  // debug function
  std::string
  odlCollSpecToString(odlCollSpec *coll_spec)
  {
#define STR(X) ((X) ? (X) : "<null>")
    std::string s = "collname=";
    s += STR(coll_spec->collname);
    s += " typename=";

    if (coll_spec->typname)
      s += coll_spec->typname;
    else if (coll_spec->coll_spec)
      s += std::string("{") + odlCollSpecToString(coll_spec->coll_spec) + "}";
    else
      s += "<null>";

    s += " isref=";
    s += str_convert((long)coll_spec->isref);
    s += " dim=";
    s += str_convert((long)coll_spec->dim);
    return s;
  }

  AttributeComponent *
  odlAttrComponent::make(Database *db, Schema *m, Class *cls,
			 const Attribute *&attr)
  {
    if (isCloned) {
      //printf("component %p is cloned\n", this);
      return 0;
    }

    std::string sattrpath;
    const char *clsname = cls->getAliasName();
    int len = strlen(clsname);
    std::string s = std::string(clsname) + ".";
    const char *p = strchr(attrpath, ':');
    if (p) {
      if (strncmp(attrpath, clsname, p - attrpath)) {
	odl_add_error("invalid syntax '%s' for attribute path: class name "
		      "expected before scope operator\n",
		      attrpath);
	return 0;
      }
      sattrpath = s + std::string(p + 2);
    }
    else if (!strncmp(attrpath, s.c_str(), len + 1))
      sattrpath = attrpath;
    else
      sattrpath = s + attrpath;

    const Class *xcls;
    Status st = Attribute::checkAttrPath(m, xcls, attr, sattrpath.c_str());
    if (st) {
      odl_add_error(st);
      return 0;
    }

    free(attrpath);
    attrpath = strdup(sattrpath.c_str());
    return make_realize(db, m, const_cast<Class *>(cls), attr);
  }

  static odlBool
  is_update_attribute(odlUpdateItem *ci, const Attribute *attr)
  {
    if (!ci->asUpdateAttribute())
      return odlFalse;

    const Attribute *cattr = ci->asUpdateAttribute()->item;
    if (cattr->getClassOwner() == attr->getClassOwner()) { // reconnected the 27/05/02
      return odlTrue;
    }

    /*
      if (cattr == attr)
      return odlTrue;
    */


    /*
      if (attr->getClassOwner() == cattr->getClassOwner()) {
      printf("update attribute %s returns TRUE becaus of %s\n", attr->getName(),
      attr->getClassOwner()->getName());
      return odlTrue;
      }
    */

    return odlFalse;
  }

  static odlBool
  is_remove_attribute(odlUpdateItem *ci, const Attribute *attr)
  {
    return ci->asRemoveAttribute() &&
      ci->asRemoveAttribute()->item == attr ? odlTrue : odlFalse;
  }

  //static Bool is_pred_bypass = IDBBOOL(getenv("IS_PRED_BYPASS"));

  static const Attribute *
  odl_is_pred_attribute(Schema *m, const Attribute *attr,
			odlBool (*pred)(odlUpdateItem *,
					const Attribute *),
			const char *predname)
  {
    LinkedListCursor c(odlUPDLIST(m));
    odlUpdateItem *ci;

    //printf("XCOMP: is_%s_attribute -> %s\n", predname, attr->getName());
    while (c.getNext((void *&)ci)) {
      if (pred(ci, attr))
	return attr;

      //if (is_pred_bypass) return 0;

      if (!attr->isIndirect() && attr->getClass()->asAgregatClass()) {
	unsigned int attr_cnt;
	const Attribute **attrs = attr->getClass()->getAttributes(attr_cnt);
	/*
	  printf("XCOMP: is_%s_attribute recursion in %s\n", predname,
	  attr->getClass()->getName());
	*/
	for (int i = 0; i < attr_cnt; i++) {
	  if (!attrs[i]->isNative() &&
	      odl_is_pred_attribute(m, attrs[i], pred, predname))
	    return attrs[i];
	}
      }
    }
  
    return 0;
  }

  static const Attribute *
  odl_is_update_attribute(Schema *m, const Attribute *attr)
  {
    return odl_is_pred_attribute(m, attr, is_update_attribute, "update");
  }

  static const Attribute *
  odl_is_remove_attribute(Schema *m, const Attribute *attr)
  {
    return odl_is_pred_attribute(m, attr, is_remove_attribute, "remove");
  }

  void odlAgregatClass::realize(Database *db, odlAttrComponent *odl_comp,
				Schema *m, const char *prefix)
  {
    if (!cls) {
      odl_add_error(std::string("class ") + ocls->getName() + " is removed : "
		    "cannot add components");
      return ;
    }

    const Attribute *attr;
    AttributeComponent *attr_comp = odl_comp->make
      (db, m, const_cast<Class *>(cls), attr);
    /*
      printf("realize::AttrComponent %p '%s' %s attr_comp=%p\n",
      odl_comp, odl_comp->attrpath, cls->getName(), attr_comp);
    */

    if (attr_comp) {
      if (db && db->isOpened() && ocls) {
	const LinkedList *dlist;
	// to force attrcomplist loading
	Status s = const_cast<Class *>(ocls)->getAttrCompList(dlist);

	if (s) {
	  odl_add_error(s);
	  return;
	}

	AttributeComponent *oattr_comp = 0;
	s = ocls->getAttrComp(attr_comp->getName().c_str(), oattr_comp);

	if (s) {
	  odl_add_error(s);
	  return;
	}

	if (oattr_comp) {
	  if (!attr_comp->getClass()->compare(oattr_comp->getClass())) {
	    if (attr_comp->asIndex()) {
	      if (odl_update_index)
		odl_add_error("index on %s has not the same implementation type "
			      "in database : use idxupdate to change manually "
			      "its implementation type\n",
			      attr_comp->getAttrpath().c_str());
	    }
	    else
	      odl_add_error("internal error in "
			    "attribute component management\n");
	    return;
	  }

	  ObjectPeer::setOid(attr_comp, oattr_comp->getOid());
	}
      }

      cls->add(attr_comp->getInd(), attr_comp);
    }
  }

  int odlCollImplSpec::make_class_prologue(const char *clsname,
					   odlCollImplSpecItem::Type &type,
					   char *&hints) const
  {
    return make_prologue(True, clsname, type, hints, 0);
  }

  int odlCollImplSpec::make_attr_prologue(const char *attrpath,
					  odlCollImplSpecItem::Type &type,
					  char *&hints, const Attribute *attr) const {
    return make_prologue(False, attrpath, type, hints, attr);
  }

  int odlCollImplSpec::make_prologue(Bool isclass, const char *name,
				     odlCollImplSpecItem::Type &type,
				     char *&hints, const Attribute *attr) const {
    odlCollImplSpecItem::Type undefType = odlCollImplSpecItem::UndefType;

    type = undefType;
    hints = 0;

    for (int i = 0; i < item_cnt; i++) {
      odlCollImplSpecItem *item = &items[i];
      if (item->type != undefType) {
	if (type != undefType) {
	  if (isclass)
	    odl_add_error("class implementation'%s': collection type is "
			  "defined twice", name);
	  else
	    odl_add_error("attribute '%s': collection type is defined twice",
			  name);
	  return 0;
	}
	type = item->type;
      }
      else if (item->hints) {
	if (hints) {
	  if (isclass)
	    odl_add_error("class implementation '%s': collection hints are "
			  "defined twice", name);
	  else
	    odl_add_error("attribute '%s': collection hints are defined twice",
			  name);
	  return 0;
	}
	hints = item->hints;
      }
    }

    if (type == undefType) {
      if (isclass) {
	type = odlCollImplSpecItem::HashIndex;
      }
      else {
	type = odlCollImplSpecItem::NoIndex;
      }
      /*
      else if (attr &&
	       (attr->isString() || attr->isIndirect() ||
		attr->getClass()->asCollectionClass())) // what about enums
	type = odlCollImplSpecItem::HashIndex;
      else
	type = odlCollImplSpecItem::BTreeIndex;
      */
    }

    return 1;
  }

  int
  odlIndexImplSpec::make_prologue(const char *name,
				  odlIndexImplSpecItem::Type &type,
				  char *&hints, 
				  const Attribute *attr) const
  {
    odlIndexImplSpecItem::Type undefType = odlIndexImplSpecItem::UndefType;

    type = undefType;
    hints = 0;

    for (int i = 0; i < item_cnt; i++) {
      odlIndexImplSpecItem *item = &items[i];
      if (item->type != undefType) {
	if (type != undefType) {
	  odl_add_error("attribute '%s': index type is defined twice",
			name);
	  return 0;
	}
	type = item->type;
      }
      else if (item->hints) {
	if (hints) {
	  odl_add_error("attribute '%s': index hints are defined twice", name);
	  return 0;
	}
	hints = item->hints;
      }
    }

    if (type == undefType) {
      if (attr && (attr->isString() || attr->isIndirect() ||
		   attr->getClass()->asCollectionClass())) // what about enums
	type = odlIndexImplSpecItem::Hash;
      else
	type = odlIndexImplSpecItem::BTree;
    }

    return 1;
  }

  odlBool
  odlAttrComponent::similar(odlAttrComponent *comp,
			    const Class *cls1, const Class *cls2)
  {
    /*
      printf("MUST CHECK BETTER %s %s/%s %s\n",
      cls1->getName(), attrpath, cls2->getName(), comp->attrpath);
    */

    if (!strcmp(comp->attrpath, attrpath))
      return odlTrue;

    std::string s1 = std::string(cls1->getName()) + ".";
    std::string s2 = std::string(cls2->getName()) + ".";
    int len1 = strlen(s1.c_str());
    int len2 = strlen(s2.c_str());
    if (!strncmp(attrpath, s1.c_str(), len1)) {
      if (!strncmp(comp->attrpath, s2.c_str(), len2)) {
	return !strcmp(&attrpath[len1], &comp->attrpath[len2]) ?
	  odlTrue : odlFalse;
      }
      else
	return !strcmp(&attrpath[len1], comp->attrpath) ?
	  odlTrue : odlFalse;
    }
    else {
      if (!strncmp(comp->attrpath, s2.c_str(), len2)) {
	return !strcmp(attrpath, &comp->attrpath[len2]) ?
	  odlTrue : odlFalse;
      }
    }

    return odlFalse;
  }

  odlBool
  odlIndex::similar(odlAttrComponent *comp,
		    const Class *cls1, const Class *cls2)
  {
    if (!comp->asIndex())
      return odlFalse;
    return odlAttrComponent::similar(comp, cls1, cls2);
  }

  odlBool
  odlCardinality::similar(odlAttrComponent *comp,
			  const Class *cls1, const Class *cls2)
  {
    if (!comp->asCardinality())
      return odlFalse;
    return odlAttrComponent::similar(comp, cls1, cls2);
  }

  odlBool
  odlImplementation::similar(odlAttrComponent *comp,
			     const Class *cls1, const Class *cls2)
  {
    if (!comp->asImplementation())
      return odlFalse;
    return odlAttrComponent::similar(comp, cls1, cls2);
  }

  odlBool
  odlNotnull::similar(odlAttrComponent *comp,
		      const Class *cls1, const Class *cls2)
  {
    if (!comp->asNotnull())
      return odlFalse;
    return odlAttrComponent::similar(comp, cls1, cls2);
  }

  odlBool
  odlUnique::similar(odlAttrComponent *comp,
		     const Class *cls1, const Class *cls2)
  {
    if (!comp->asUnique())
      return odlFalse;
    return odlAttrComponent::similar(comp, cls1, cls2);
  }

  AttributeComponent *
  odlIndex::make_realize(Database *db, Schema *m, Class *cls,
			 const Attribute *attr)
  {
    odlIndexImplSpecItem::Type type;
    char *hints;

    if (index_impl_spec) {
      if (!index_impl_spec->make_prologue(attrpath, type, hints, attr))
	return 0;
    }
    else {
      hints = 0;
      if (attr &&
	  (attr->isString() || attr->isIndirect() ||
	   attr->getClass()->asCollectionClass())) // what about enums
	type = odlIndexImplSpecItem::Hash;
      else
	type = odlIndexImplSpecItem::BTree;
    }

    Bool hasDB = IDBBOOL(db);

    if (!db)
      db = odl_get_dummy_db(m);

    Index *idx;
    Status s;
    if (type == odlIndexImplSpecItem::Hash)
      s = HashIndex::make(db,
			  const_cast<Class *>(cls), attrpath,
			  IDBBOOL(propagate), attr->isString(), hints,
			  (HashIndex *&)idx);
    else
      s = BTreeIndex::make(db,
			   const_cast<Class *>(cls), attrpath,
			   IDBBOOL(propagate), attr->isString(), hints,
			   (BTreeIndex *&)idx);
  
    if (s) { 
      odl_add_error(s);
      return 0;
    }

    return idx;
  }

  AttributeComponent *
  odlImplementation::make_realize(Database *db, Schema *m, Class *cls,
				  const Attribute *attr)
  {
    // TBD: Index -> Coll
    odlCollImplSpecItem::Type type;
    char *hints;

    if (coll_impl_spec) {
      if (!coll_impl_spec->make_attr_prologue(attrpath, type, hints, attr))
	return 0;
    }
    else {
      hints = 0;
      //type = odlCollImplSpecItem::HashIndex;
      type = odlCollImplSpecItem::NoIndex;
    }

    Bool hasDB = IDBBOOL(db);
    if (!db)
      db = odl_get_dummy_db(m);

    CollAttrImpl *impl;
    CollImpl::Type impltype;

    if (type == odlCollImplSpecItem::HashIndex) {
      impltype = CollImpl::HashIndex;
    }
    else if (type == odlCollImplSpecItem::BTreeIndex) {
      impltype = CollImpl::BTreeIndex;
    }
    else if (type == odlCollImplSpecItem::NoIndex) {
      impltype = CollImpl::NoIndex;
    }
    else {
      odl_add_error("unknown implementation type for attribute %s", attrpath);
      return 0;
    }

    Status s = CollAttrImpl::make
      (db, const_cast<Class *>(cls), attrpath, IDBBOOL(propagate),
       impltype, hints, (CollAttrImpl *&)impl);
  
    if (s) { 
      odl_add_error(s);
      return 0;
    }

    return impl;
  }

  AttributeComponent *
  odlNotnull::make_realize(Database *db, Schema *m, Class *cls,
			   const Attribute *)
  {
    return new NotNullConstraint(db, cls, attrpath, IDBBOOL(propagate));
  }

  AttributeComponent *
  odlUnique::make_realize(Database *db, Schema *m, Class *cls,
			  const Attribute *)
  {
    return new UniqueConstraint(db, cls, attrpath, IDBBOOL(propagate));
  }

  AttributeComponent *
  odlCardinality::make_realize(Database *db, Schema *m, Class *cls,
			       const Attribute *)
  {
    return 0;
  }

  void odlAgregatClass::realize(odlDeclItem *item,
				Schema *m, const char *prefix,
				int n, ClassComponent **comp,
				int &comp_cnt, Attribute **agr)
  {
    Class *cl = NULL;
    int *dims, ndims, index_mode;
    Status status;

    if (item->typname)
      {
	/*
	  if (!strcmp(item->typname, "int"))
	  cl = m->Int32_Class;
	  else */
	cl = eyedb::getClass(m, item->typname, prefix);
      }

    if (item->typname && !cl)
      {
	ClassNotFound::error(item->typname, 
			     (std::string("class ") + name +
			      ", attribute #" + str_convert((long)n+1)).c_str());
	return;
      }

    if (check_name(m, name, item->typname, item->attrname, prefix))
      {
	odl_error++;
	return;
      }

    array_make(item, dims, ndims);

    char *invcname, *invfname;
    if (item->inverse) {
      invcname = item->inverse->classname;
      invfname = item->inverse->attrname;
    }
    else {
      invcname = invfname = 0;
    }
    
#if 0
    Index *idx_item, *idx_comp;
    item->attr_list_make(m, cls, index_mode, idx_item, idx_comp,
			 &invcname, &invfname, comp, comp_cnt);
#endif

    if (invcname)
      invcname = strdup(makeName(invcname, prefix));

    if (item->coll_spec)
      {
	odlCollSpec *coll_spec = item->coll_spec;
	odlCollSpec *coll_spec_arr[64];
	int coll_spec_cnt = 0;

	while (coll_spec)
	  {
	    //printf("coll_spec #%d: %s\n", coll_spec_cnt, collSpecToString(coll_spec));
	    coll_spec_arr[coll_spec_cnt++] = coll_spec;
	    coll_spec = coll_spec->coll_spec;
	  }

	const char *coll_typname = coll_spec_arr[coll_spec_cnt-1]->typname;
	Class *mcoll = NULL;

	for (int i = coll_spec_cnt-1; i >= 0; i--)
	  {
	    coll_spec = coll_spec_arr[i];
	    cl = eyedb::getClass(m, coll_typname, prefix);

	    if (!cl) {
	      ClassNotFound::error(coll_typname,
				   (std::string("class ") + name).c_str());
	      return;
	    }

	    Exception::Mode mode = Exception::setMode(Exception::StatusMode);
	    if (!strcmp(coll_spec->collname, "set")) {
	      if (coll_spec->dim)
		mcoll = new CollSetClass(cl, coll_spec->dim);
	      else
		mcoll = new CollSetClass(cl, (Bool)coll_spec->isref);
	    }
	    else if (!strcmp(coll_spec->collname, "bag")) {
	      if (coll_spec->dim)
		mcoll = new CollBagClass(cl, coll_spec->dim);
	      else
		mcoll = new CollBagClass(cl, (Bool)coll_spec->isref);
	    }
	    else if (!strcmp(coll_spec->collname, "array")) {
	      if (coll_spec->dim)
		mcoll = new CollArrayClass(cl, coll_spec->dim);
	      else
		mcoll = new CollArrayClass(cl, (Bool)coll_spec->isref);
	    }
	    else if (!strcmp(coll_spec->collname, "list")) {
	      if (coll_spec->dim)
		mcoll = new CollListClass(cl, coll_spec->dim);
	      else
		mcoll = new CollListClass(cl, (Bool)coll_spec->isref);
	    }
	    else {
	      odl_add_error("invalid collection type '%s'\n",
			    coll_spec->collname);
	      return;
	    }

	    mcoll->setUserData(odlGENCODE, AnyUserData);
	    Exception::setMode(mode);
	  
	    /*
	      if (!coll_spec->isref) {
	      odl_add_error("'%s<%s> %s%s': only collection of objects are "
	      "supported\n",
	      coll_spec->collname, cl->getName(),
	      (item->isref ? "*" : ""), item->attrname);
	      return;
	      }
	    */

	    status = mcoll->asCollectionClass()->getStatus();
	    if (status) {
	      odl_add_error("attribute error '%s::%s': ", name, item->attrname);
	      odl_add_error(status);
	      odl_error--;
	      return;
	    }

	    Class *mcoll_old = m->getClass(mcoll->getName());

	    if (mcoll_old) {
	      mcoll->release();
	      mcoll = mcoll_old;
	    }
	    else
	      m->addClass(mcoll);

	    coll_typname = mcoll->getName();
	  }

	if (invfname)
	  {
	    if (invcname)
	      {
		if (strcmp(invcname, cl->getName()) &&
		    strcmp(invcname, cl->getAliasName()))
		  {
		    odl_add_error("inverse class incoherency: got '%s', expected '%s'\n", invcname, mcoll->getName());
		    return;
		  }
	      }
	    else
	      invcname = strdup(cl->getAliasName());
	  }

	agr[n] =
	  new Attribute(mcoll, item->attrname, (Bool)item->isref, ndims, dims);

	//agr[n]->setMagOrder(getMagOrder(cls, item->attr_list, agr[n]->getName()));
	agr[n]->setUserData(item->upd_hints);

	status = agr[n]->check();
	if (status)
	  odl_add_error(status);
      }
    else
      {
	if (invfname)
	  {
	    if (invcname)
	      {
		if (strcmp(invcname, cl->getName()) &&
		    strcmp(invcname, cl->getAliasName()))
		  {
		    odl_add_error("inverse class incoherency: got '%s', expected '%s'\n", invcname, cl->getName());
		    return;
		  }
	      }
	    else
	      invcname = strdup(cl->getAliasName());
	  }

	agr[n] = new Attribute(cl, item->attrname, (Bool)item->isref,
			       ndims, dims);
	//agr[n]->setMagOrder(getMagOrder(cls, item->attr_list, agr[n]->getName()));
	agr[n]->setUserData(item->upd_hints);
	status = agr[n]->check();
	if (status)
	  odl_add_error(status);
      }

    if (invcname)
      {
	status = agr[n]->setInverse(invcname, invfname);
      
	if (status)
	  odl_add_error(status);
      }
  }

  static TriggerType
  makeTrigType(const char *loc)
  {
    if (!strcmp(loc, "create_before"))
      return TriggerCREATE_BEFORE;
    if (!strcmp(loc, "create_after"))
      return TriggerCREATE_AFTER;
    if (!strcmp(loc, "update_before"))
      return TriggerUPDATE_BEFORE;
    if (!strcmp(loc, "update_after"))
      return TriggerUPDATE_AFTER;
    if (!strcmp(loc, "load_before"))
      return TriggerLOAD_BEFORE;
    if (!strcmp(loc, "load_after"))
      return TriggerLOAD_AFTER;
    if (!strcmp(loc, "remove_before"))
      return TriggerREMOVE_BEFORE;
    if (!strcmp(loc, "remove_after"))
      return TriggerREMOVE_AFTER;

    odl_add_error("invalid trigger localisation '%s'\n", loc);
    return (TriggerType)0;
  }

  static Signature *
  makeSignature(Schema *m, const char *clsname, const char *mthname,
		const char *rettype, odlArgSpecList *arglist,
		int &missNames)
  {
    ArgType *type = ArgType::make(m, rettype);

    if (!type)
      {
	type = ArgType::make(m, (odl_db_prefix + rettype).c_str());
	if (!type) {
	  ClassNotFound::error(rettype,
			       (std::string("method ") + clsname + "::" +
				mthname).c_str());
	  return 0;
	}
      }

    missNames = 0;
    type->setType((ArgType_Type)(type->getType() | OUT_ARG_TYPE), False);

    Signature *sign = new Signature();
#ifdef NO_DIRECT_SET
    *sign->getRettype() = *type;
#endif

#ifndef NO_DIRECT_SET
    sign->setRettype(type);
#endif
    sign->setNargs(arglist->count);
    odlArgSpecLink *l = arglist->first;

#ifdef NO_DIRECT_SET
    sign->setTypesCount(arglist->count);
#endif
    char **names = new char *[arglist->count];

    int n = 0;
    while (l)
      {
	type = ArgType::make(m, l->x->typname);
	if (!type)
	  {
	    type = ArgType::make(m, (odl_db_prefix + l->x->typname).c_str());
	    if (!type) {
	      ClassNotFound::error(l->x->typname,
				   (std::string("method ") + clsname + "::" +
				    mthname + ", argument #" +
				    str_convert((long)n+1)).c_str());
	      if (sign)
		sign->release();
	      sign = 0;
	      //return 0;
	    }
	  }

	if (sign) {
	  type->setType((ArgType_Type)(type->getType() | l->x->inout), False);
#ifdef NO_DIRECT_SET
	  *sign->getTypes(n) = *type;
#else
	  sign->setTypes(n, type);
#endif
	  names[n] = (l->x->varname ? strdup(l->x->varname) : 0);
	  if (!names[n])
	    missNames++;
	}

	l = l->next;
	n++;
      }

    if (!sign) return 0;
    odlSignUserData *sud = new odlSignUserData(names);
    sign->setUserData(sud);
    return sign;
  }

  int
  odlMethodSpec::getParamNames(char **&typnames, char **&varnames)
  {
    int param_cnt = arglist->count;
    typnames = new char*[param_cnt];
    varnames = new char*[param_cnt];

    odlArgSpecLink *l = arglist->first;

    for (int n = 0; l; n++)
      {
	const char *s;
	if (l->x->inout == INOUT_ARG_TYPE)
	  s = "inout";
	else if (l->x->inout == IN_ARG_TYPE)
	  s = "in";
	else
	  s = "out";
	typnames[n] = strdup((std::string(s) + "$" + l->x->typname).c_str());
	varnames[n] = l->x->varname;
	l = l->next;
      }

    return param_cnt;
  }

  std::string
  odlTriggerSpec::makeOQLBody(const Class *cls) const
  {
    return std::string("oql$") + cls->getAliasName() + "$" + name + "(this) ";
  }

  static void
  compileMethod(Schema *m, const Class *cls, odlMethodSpec *mth)
  {
    if (!m->getDatabase() || !m->getDatabase()->isOpened())
      return;

    char **typnames;
    char **varnames;
    int param_cnt = mth->getParamNames(typnames, varnames);
    std::string oqlConstruct;

    std::string body = BEMethod_OQL::makeExtrefBody(cls, mth->oqlSpec,
						    mth->fname,
						    typnames, varnames,
						    param_cnt,
						    oqlConstruct);

    oqmlStatus *s = oqml_realize(m->getDatabase(),
				 (char *)oqlConstruct.c_str(), 0,
				 oqml_True);
    if (s)
      odl_add_error("compiling '%s::%s': %s\n",
		    cls->getAliasName(), mth->fname, s->msg);

    free(mth->oqlSpec);
    mth->oqlSpec = strdup(body.c_str());
  }

  static void
  compileTrigger(Schema *m, const Class *cls, odlTriggerSpec *trig)
  {
    std::string oqlConstruct;

    std::string body = Trigger::makeExtrefBody(cls, trig->oqlSpec,
					       trig->name,
					       oqlConstruct);

    oqmlStatus *s = oqml_realize(m->getDatabase(),
				 (char *)oqlConstruct.c_str(), 0,
				 oqml_True);
    if (s)
      odl_add_error("compiling trigger '%s::%s': %s\n",
		    cls->getAliasName(), trig->name, s->msg);

    /*
      char *str = strdup(std::string("function ") + trig->makeOQLBody(cls) +
      trig->oqlSpec);

      oqmlStatus *s = oqml_realize(m->getDatabase(), (char *)str, 0, oqml_True);
      if (s)
      odl_add_error("compiling trigger '%s::%s': %s\n",
      cls->getAliasName(), trig->name, s->msg);
    */

    free(trig->oqlSpec);
    trig->oqlSpec = strdup(body.c_str());
  }

  void odlAgregatClass::realize(Database *db, Schema *m,
				odlExecSpec *ex, const char *def_extref)
  {
    ClassComponent *comp = 0;
    odlMethodSpec *mth;
    odlTriggerSpec *trig;

    if (mth = ex->asMethodSpec()) {
      int missNames;
      Signature *sign = makeSignature(m, name, mth->fname,
				      mth->rettype, mth->arglist,
				      missNames);

      if (!sign)
	return;

      if (mth->oqlSpec) {
	if (missNames) {
	  odl_add_error("method '%s::%s' : signatures for OQL"
			" methods must contains parameter names\n",
			cls->getName(), mth->fname);
	  return;
	}

	compileMethod(m, cls, mth);

	comp = new BEMethod_OQL(0, cls, mth->fname, sign,
				(Bool)mth->isClassMethod,
				odl_system_class,
				mth->oqlSpec);
      }
      else if (mth->mth_hints.isClient)
	comp = new FEMethod_C(0, cls, mth->fname, sign,
			      (Bool)mth->isClassMethod,
			      odl_system_class,
			      mth->extref ? mth->extref : def_extref);
      else
	comp = new BEMethod_C(0, cls, mth->fname, sign,
			      (Bool)mth->isClassMethod,
			      odl_system_class,
			      mth->extref ? mth->extref : def_extref);
      ((odlSignUserData *)sign->getUserData())->mth_hints = &mth->mth_hints;
      ((odlSignUserData *)sign->getUserData())->upd_hints = mth->upd_hints;
      comp->setUserData(sign->getUserData());
      comp->asMethod()->getEx()->getSign()->setUserData(sign->getUserData());
    }
    else if (trig = ex->asTriggerSpec()) {
      TriggerType ttype = makeTrigType(trig->localisation);
      if (ttype) {
	if (trig->oqlSpec)
	  compileTrigger(m, cls, trig);
	const char *extref;
	if (trig->oqlSpec)
	  extref = trig->oqlSpec;
	else if (trig->extref)
	  extref = trig->extref;
	else
	  extref = def_extref;

	comp = new Trigger(0, cls, ttype,
			   (trig->oqlSpec ? OQL_LANG : C_LANG),
			   odl_system_class,
			   trig->name, trig->light ? True : False,
			   extref);

	odlSignUserData *sud = new odlSignUserData(0);
	sud->upd_hints = trig->upd_hints;
	comp->setUserData(sud);
      }
    }
    else
      abort();

    if (!odl_error){
      if (ex->asMethodSpec()) {
	if (!cls->getUserData(odlMTHLIST))
	  cls->setUserData(odlMTHLIST, new LinkedList());
	odlMTHLIST(cls)->insertObject(comp);
      }
      cls->add(comp->getInd(), comp);
    }
  }

  void
  odl_add_component(Schema *m, ClassComponent *comp)
  {
    if (odl_error)
      return;

    //printf("odlAddComponent('%s')\n", comp->getName());
    odlUPDLIST(m)->insertObjectLast(new odlAddComponent(comp));
    comp->getClassOwner()->touch();
  }

  void
  odl_add_component(Schema *m, AttributeComponent *comp)
  {
    if (odl_error)
      return;

    //printf("odlAddComponent('%s') -> %p\n", comp->getName(), comp);
    odlUPDLIST(m)->insertObjectLast(new odlAddComponent(comp));
    //odlUPDLIST(m)->insertObjectFirst(new odlAddComponent(comp));
  }

  void
  odl_remove_component(Schema *m, ClassComponent *comp)
  {
    if (odl_error)
      return;

    //printf("odlRemoveComponent('%s')\n", comp->getName());
    odlUPDLIST(m)->insertObjectLast(new odlRemoveComponent(comp));
    comp->getClassOwner()->touch();
  }

  void
  odl_remove_component(Schema *m, AttributeComponent *comp)
  {
    if (odl_error)
      return;

    //printf("odlRemoveComponent('%s') -> %p\n", comp->getName(), comp);
    odlUPDLIST(m)->insertObjectLast(new odlRemoveComponent(comp));
  }

  static void
  odl_add_relationship(Schema *m, Class *cls, const char *name,
		       const Attribute *item,
		       const Attribute *invitem)
  {
    cls->touch();
    odlUPDLIST(m)->insertObjectLast(new odlAddRelationship(item, invitem));
  }

  static void
  odl_remove_relationship(Schema *m, Class *cls, const char *name,
			  const Attribute *item,
			  const Attribute *invitem)
  {
    cls->touch();
    odlUPDLIST(m)->insertObjectLast(new odlRemoveRelationship(item, invitem));
  }

  static void
  odl_add_class(Schema *m, const Class *cls)
  {
    if (odl_error)
      return;

    if (!odl_rootclass || strcmp(cls->getName(), odl_rootclass))
      odlUPDLIST(m)->insertObjectLast(new odlAddClass(cls));
  }

  static void
  odl_remove_class(Database *db, Schema *m, const Class *cls)
  {
    if (odl_error)
      return;

    odlUPDLIST(m)->insertObjectLast(new odlRemoveClass(db, cls,
						       odlUPDLIST(m)));
  }

  static void
  odl_migrate_attributes(Schema *m, const Class *cls)
  {
    unsigned int attr_cnt;
    const Attribute **attrs  = cls->getAttributes(attr_cnt);

    for (int i = 0; i < attr_cnt; i++)
      {
	const Attribute *attr = attrs[i];
	odlUpdateHint *upd_hints = (odlUpdateHint *)attr->getUserData();
	if (upd_hints && upd_hints->type == odlUpdateHint::MigrateFrom)
	  {
	    const Class *ocls = eyedb::getClass(m, upd_hints->detail, odl_db_prefix.c_str());
	    if (!ocls)
	      {
		ClassNotFound::error(upd_hints->detail);
		continue;
	      }

	    ocls = (const Class *)ocls->getUserData();
	    assert(ocls);

	    const Attribute *oattr = ocls->getAttribute(upd_hints->detail2);
	    if (!oattr)
	      {
		odl_add_error("attribute %s not found in class %s\n",
			      upd_hints->detail2, upd_hints->detail);
		continue;
	      }

	    const_cast<Attribute *>(oattr)->setUserData
	      (new odlUpdateHint
	       (odlUpdateHint::MigrateTo,
		cls->getName(), attr->getName(), upd_hints->detail3));
	  }
      }
  }

  static void
  odl_reparent_class(Schema *m, const Class *cls)
  {
    odlUPDLIST(m)->insertObjectLast(new odlReparentClass(cls));
  }

  static void
  odl_add_attribute(Schema *m, const Class *cls,
		    const Attribute *attr)
  {
    //printf("XCOMP: add_attribute(%s, %p)\n", attr->getName(), cls);
    odlUPDLIST(m)->insertObjectLast(new odlAddAttribute(cls, attr));
  }

  static void
  odl_remove_attribute(Schema *m, const Class *cls,
		       const Attribute *attr)
  {
    //printf("XCOMP: remove_attribute(%s)\n", attr->getName());
    odlUPDLIST(m)->insertObjectLast(new odlRemoveAttribute(cls, attr));
  }

  static void
  odl_rename_attribute(Schema *m, const Class *cls,
		       const Attribute *attr,
		       odlUpdateHint *upd_hints)
  {
    odlUPDLIST(m)->insertObjectLast(new odlRenameAttribute(cls, attr, upd_hints));
  }

  static void
  odl_migrate_attribute(Schema *m, const Class *cls,
			const Attribute *attr, odlUpdateHint *upd_hints)
  {
    odlUPDLIST(m)->insertObjectLast(new odlMigrateAttribute(cls, attr, upd_hints));
  }
  static void
  odl_convert_attribute(Schema *m, const Class *cls,
			const Attribute *oattr, const Attribute *attr,
			odlUpdateHint *upd_hints = 0)
  {
    odlUPDLIST(m)->insertObjectLast(new odlConvertAttribute(cls, oattr, attr, upd_hints));
  }

  static Bool
  odl_reorder_attr(Schema *m, const Class *cls,
		   const Attribute *attr, int newnum)
  {
    if (attr->getNum() != newnum)
      {
	odlUPDLIST(m)->insertObjectLast
	  (new odlReorderAttribute(cls, attr, newnum, attr->getNum()));
	const_cast<Attribute *>(attr)->setNum(newnum);
	return True;
      }

    return False;
  }

  static int
  cmp_attr(const void *x1, const void *x2)
  {
    return (*(Attribute **)x1)->getNum() - (*(Attribute **)x2)->getNum();
  }

  static Attribute *
  odl_get_renamed_attr(Attribute *attrs[], int attr_cnt,
		       const Attribute *oattr)
  {
    // check if not renamed!
    for (int j = 0; j < attr_cnt; j++)
      {
	const Attribute *attr = attrs[j];
	odlUpdateHint *upd_hints = (odlUpdateHint *)attr->getUserData();
	if (upd_hints && upd_hints->type == odlUpdateHint::RenameFrom &&
	    !strcmp(upd_hints->detail, oattr->getName()))
	  return const_cast<Attribute *>(attr);
      }

    return 0;
  }

#define COMPLETE(CLS)					\
  if ((CLS)->isPartiallyLoaded())			\
    {							\
      Status s = m->manageClassDeferred((Class *)CLS);	\
      if (s)						\
	{						\
	  odl_add_error(s);				\
	  return;					\
	}						\
    }

#ifdef NEW_REORDER
  static Bool
  odl_class_need_reorder(const Class *ocls,
			 Attribute **attrs, int attr_cnt)
  {
    int onum = 0;
    for (int i = 0; i < attr_cnt; i++)
      {
	const Attribute *attr = attrs[i];
	const Attribute *oattr;

	if (oattr = ocls->getAttribute(attr->getName())) {
	  if (oattr->getNum() < onum) {
	    //printf("%s NEEDING REORDER!\n", ocls->getName());
	    return True;
	  }
	  onum = oattr->getNum();
	}
      }

    //printf("%s DOES NOT NEED REORDER!\n", ocls->getName());
    return False;
  }

  static void
  odl_class_premanage(Schema *m, const Class *ocls, const Class *cls)
  {
    static const int invalid_num = -1;
    unsigned int attr_cnt;
    Attribute **attrs = (Attribute **)cls->getAttributes(attr_cnt);
    int i;

    COMPLETE(cls);
    COMPLETE(ocls);

    if (!odl_class_need_reorder(ocls, attrs, attr_cnt))
      return;

    //
    // ignore attribute reordering
    //

    Bool is_renum = False;
    int attr_num = 0;
    for (i = 0; i < attr_cnt; i++)
      {
	const Attribute *attr = attrs[i];
	const Attribute *oattr;

	if (oattr = ocls->getAttribute(attr->getName())) {
	  if (odl_reorder_attr(m, cls, attr, oattr->getNum()))
	    is_renum = True;
	  attr_num++;
	}
	else
	  {
	    odlUpdateHint *upd_hints = (odlUpdateHint *)attr->getUserData();
	    if (upd_hints && upd_hints->type == odlUpdateHint::RenameFrom)
	      {
		oattr = ocls->getAttribute(upd_hints->detail);
		if (oattr) {
		  if (odl_reorder_attr(m, cls, attr, oattr->getNum()))
		    is_renum = True;
		  attr_num++;
		}
		else
		  const_cast<Attribute *>(attr)->setNum(invalid_num);
	      }
	    else
	      const_cast<Attribute *>(attr)->setNum(invalid_num);
	  }
      }

    for (i = 0; i < attr_cnt; i++) {
      const Attribute *attr = attrs[i];
      if (attr->getNum() == invalid_num) {
	const_cast<Attribute *>(attr)->setNum(attr_num++);
	is_renum = True;
      }
    }

    qsort(attrs, attr_cnt, sizeof(Attribute *), cmp_attr);
    if (is_renum) const_cast<Class *>(cls)->compile();
  }
#endif

  static void
  odl_class_compare(Schema *m, const Class *ocls, const Class *cls,
		    odlUpdateHint *cls_hints)
  {
    unsigned int oattr_cnt, attr_cnt;
    Attribute **oattrs = (Attribute **)ocls->getAttributes(oattr_cnt);
    Attribute **attrs = (Attribute **)cls->getAttributes(attr_cnt);
    int i;

    COMPLETE(cls);
    COMPLETE(ocls);

    // check removed and converted attributes
    for (i = 0; i < oattr_cnt; i++) {
      const Attribute *oattr = oattrs[i];
      const Attribute *attr;
      Bool migrate = False;

      odlUpdateHint *upd_hints = (odlUpdateHint *)oattr->getUserData();
      if (upd_hints && upd_hints->type == odlUpdateHint::MigrateTo) {
	migrate = True;
	odl_migrate_attribute(m, cls, oattr, upd_hints);
      }

      if (!(attr = cls->getAttribute(oattr->getName()))) {
	attr = odl_get_renamed_attr(attrs, attr_cnt, oattr);
	if (!attr) {
	  if (!migrate) {
	    if (cls_hints && cls_hints->type == odlUpdateHint::Extend) {
	      if (upd_hints && upd_hints->type == odlUpdateHint::Remove)
		odl_remove_attribute(m, cls, oattr);
	    }
	    else
	      odl_remove_attribute(m, cls, oattr);
	  }
	}
	else if (upd_hints && upd_hints->type == odlUpdateHint::Convert)
	  odl_convert_attribute(m, cls, oattr, attr, upd_hints);
	else if (!oattr->compare(m->getDatabase(), attr,
				 False,  // compClassOwner
				 False,  // compNum
				 False,  // compName
				 True))  // inDepth
	  odl_convert_attribute(m, cls, oattr, attr);
      }
      else if (upd_hints && upd_hints->type == odlUpdateHint::Convert)
	odl_convert_attribute(m, cls, oattr, attr, upd_hints);
      else if (!oattr->compare(m->getDatabase(), attr,
			       False,  // compClassOwner
			       False,  // compNum
			       False,  // compName
			       True))  // inDepth
	odl_convert_attribute(m, cls, oattr, attr);
    }

    // check added attributes
    for (i = 0; i < attr_cnt; i++) {
      const Attribute *attr = attrs[i];
      const Attribute *oattr;
      
      const char *name = attr->getName();
      odlUpdateHint *upd_hints = (odlUpdateHint *)attr->getUserData();
      if (upd_hints && upd_hints->type == odlUpdateHint::RenameFrom) {
	name = upd_hints->detail;
	if (oattr = ocls->getAttribute(name))
	  odl_rename_attribute(m, cls, attr, upd_hints);
	else
	  odl_add_error("class %s: no attribute named %s\n",
			cls->getName(), name);
      }

      if (!(oattr = ocls->getAttribute(name))) {
	if (!upd_hints || upd_hints->type != odlUpdateHint::MigrateFrom)
	  odl_add_attribute(m, cls, attr);
      }
    }

    if (odl_error) return;

#ifndef NEW_REORDER
    // check attribute order
    int cnt = 0;
    for (i = 0; i < attr_cnt; i++) {
      const Attribute *attr = attrs[i];
      const Attribute *oattr;

      if (oattr = ocls->getAttribute(attr->getName()))
	odl_reorder_attr(m, cls, attr, oattr->getNum() + cnt);
      else {
	odlUpdateHint *upd_hints = (odlUpdateHint *)attr->getUserData();
	if (upd_hints && upd_hints->type == odlUpdateHint::RenameFrom) {
	  oattr = ocls->getAttribute(upd_hints->detail);
	  if (oattr)
	    odl_reorder_attr(m, cls, attr, oattr->getNum() + cnt);
	  else
	    cnt++;
	}
	else
	  cnt++;
      }
    }

    // ignore attribute reordering!
    qsort(attrs, attr_cnt, sizeof(Attribute *), cmp_attr);
#endif
  }

  static void
  odl_class_parent_compare(Schema *m, const Class *ocls, const Class *cls)
  {
    const Class *parent = cls->getParent();
    if (odl_rootclass && !strcmp(parent->getName(), odl_rootclass))
      parent = parent->getParent();
    const Class *oparent = ocls->getParent();
    if (odl_rootclass && !strcmp(oparent->getName(), odl_rootclass))
      oparent = oparent->getParent();

    /*
      if (!parent->compare(oparent))
      odl_reparent_class(m, cls);
    */

    if (strcmp(parent->getName(), oparent->getName()))
      odl_reparent_class(m, cls);
  }

  Bool
  odl_find_component(ClassComponent *&comp, const LinkedList *complist,
		     Bool strictCheck, ClassComponent *&fclcomp)
  {
    //printf("LOOKING for component '%s'\n", comp->getName());
    if (strchr(comp->getName().c_str(), '.')) {
      fclcomp = comp;
      return True;
    }

    fclcomp = 0;
    LinkedListCursor c(complist);
    ClassComponent *tmpcomp;
    while (c.getNext((void *&)tmpcomp))
      {
	if (!strcmp(comp->getName().c_str(), tmpcomp->getName().c_str()))
	  {
	    if (!comp->asAgregatClassExecutable()) {
	      fclcomp = tmpcomp;
	      return True;
	    }

	    Executable *ex = comp->asAgregatClassExecutable()->getEx();
	    Executable *tmpex = tmpcomp->asAgregatClassExecutable()->getEx();

	    if (strictCheck &&
		strcmp(ex->getExtrefBody().c_str(), tmpex->getExtrefBody().c_str()))
	      {
		tmpex->setExtrefBody(ex->getExtrefBody());
		if ((ex->getLang() & SYSTEM_EXEC) && !odl_system_class)
		  {
		    odl_add_error("cannot modify system method "
				  "'%s'\n", comp->getName().c_str());
		    fclcomp = tmpcomp;
		    return True;
		  }
	    
		if (ex->getLoc() != tmpex->getLoc()) {
		  return False;
		}
		  
		comp = tmpcomp;
		return False;
	      }
	    else if (ex->getLoc() != tmpex->getLoc()) {
	      return False;
	    }

	    fclcomp = tmpcomp;
	    return True;
	  }
      }

    if (comp->asAgregatClassExecutable() &&
	((comp->asAgregatClassExecutable()->getEx()->getLang() &
	  SYSTEM_EXEC) && !odl_system_class)) {
      fclcomp = comp;
      return True;
    }

    return False;
  }

  Bool
  odl_compare_index_hints(const Index *idx1, const Index *idx2)
  {
    if (idx1->getImplHintsCount() != idx2->getImplHintsCount())
      return False;

    int cnt = idx1->getImplHintsCount();
    for (int i = 0; i < cnt; i++) {
      if (idx1->getImplHints(i) && idx2->getImplHints(i) &&
	  idx1->getImplHints(i) != idx2->getImplHints(i))
	return False;
    }

    return True;
  }

  Bool
  odl_compare_index(const HashIndex *idx1, const HashIndex *idx2)
  {
    if (idx1->getKeyCount() && idx2->getKeyCount() &&
	idx1->getKeyCount() != idx2->getKeyCount())
      return False;

    if (!const_cast<HashIndex *>(idx1)->compareHashMethod(const_cast<HashIndex *>(idx2)))
      return False;

    return odl_compare_index_hints(idx1, idx2);
  }

  Bool
  odl_compare_index(const BTreeIndex *idx1, const BTreeIndex *idx2)
  {
    if (idx1->getDegree() && idx2->getDegree() &&
	idx1->getDegree() != idx2->getDegree())
      return False;

    return odl_compare_index_hints(idx1, idx2);
  }

  AttributeComponent *
  odl_report_index_hints(const Index *idx1, Index *idx2)
  {
    int cnt1 = idx1->getImplHintsCount();
    int cnt2 = idx2->getImplHintsCount();
    for (int i = 0; i < cnt1; i++)
      idx2->setImplHints(i, idx1->getImplHints(i));

    for (int i = cnt1; i < cnt2; i++)
      idx2->setImplHints(i, 0);

    return idx2;
  }

  AttributeComponent *
  odl_report_index(Index *idx1, Index *idx2)
  {
    ObjectPeer::setOid(idx1, idx2->getOid());
    idx1->setIdxOid(idx2->getIdxOid());
    return idx1;
  }

  Bool
  odl_find_component(AttributeComponent *&comp, const LinkedList *complist,
		     Bool strictCheck, AttributeComponent *&fattr_comp)
  {
    fattr_comp = 0;
    LinkedListCursor c(complist);
    AttributeComponent *tmpcomp;
    while (c.getNext((void *&)tmpcomp)) {
      if (!strcmp(comp->getName().c_str(), tmpcomp->getName().c_str())) {
	fattr_comp = tmpcomp;
	if (strictCheck) {
	  if (comp->asIndex()) {
	    BTreeIndex *bidx = comp->asBTreeIndex();
	    BTreeIndex *tmpbidx = tmpcomp->asBTreeIndex();
	    HashIndex *hidx = comp->asHashIndex();
	    HashIndex *tmphidx = tmpcomp->asHashIndex();

	    if (hidx && tmphidx) {
	      if (odl_update_index && !odl_compare_index(hidx, tmphidx)) {
		comp = odl_report_index(hidx, tmphidx);
		return False;
	      }
	    }
	    else if (bidx && tmpbidx) {
	      if (odl_update_index && !odl_compare_index(bidx, tmpbidx)) {
		comp = odl_report_index(bidx, tmpbidx);
		return False;
	      }
	    }
	    else
	      return False;
	  }

	  if (odl_update_index && comp->getPropagate() != tmpcomp->getPropagate()) {
	    tmpcomp->setPropagate(comp->getPropagate());
	    comp = tmpcomp;
	    return False;
	  }
	}

	return True;
      }
    }

    return False;
  }

  static int
  completeInverse(Schema *m, const char *cname,
		  const char *fname, const Attribute *&invitem)
  {
    if (invitem)
      return 0;

    if (cname && fname)
      {
	const Class *cls = m->getClass(cname);
	if (!cls)
	  {
	    odl_error++;
	    return 1;
	  }

	invitem = cls->getAttribute(fname);
      }

    return 0;
  }

  //
  // 17/09/01:
  // THIS FUNCTION IS WRONG IF THE SCHEMA HAS EVOLVED (MORE OR LESS ATTRIBUTES
  // OR RENAMED ATTRIBUTES)!!
  // 

#ifdef NEW_DIFF_RELSHIP
  int
  odl_remove_relationships(Database *, Schema *m, Class *ocls)
  {
    // function not really pertinent
    if (true)
      return 0;

    if (!ocls)
      return 0;

    unsigned int oattr_cnt;
    const Attribute **oattrs  = ocls->getAttributes(oattr_cnt);

    for (int i = 0; i < oattr_cnt; i++) {
      const Attribute *oattr = oattrs[i];

      const char *ocname, *ofname;
      const Attribute *oinvitem;

      oattr->getInverse(&ocname, &ofname, &oinvitem);
      completeInverse(m, ocname, ofname, oinvitem);
      
      if (oinvitem)
	odl_remove_relationship(m, ocls, ocls->getName(), oattr, oinvitem);
    }

    return 0;
  }


  int
  odlAgregatClass::manageDiffRelationShips(Database *, Schema *m, Bool diff)
  {
    if (!ocls)
      return 0;

    unsigned int oattr_cnt, attr_cnt;
    const Attribute **attrs  = cls->getAttributes(attr_cnt);
    const Attribute **oattrs  = ocls->getAttributes(oattr_cnt);

    for (int i = 0; i < oattr_cnt; i++)
      {
	const Attribute *oattr = oattrs[i];
	const Attribute *attr;

	if (!(attr = cls->getAttribute(oattr->getName()))) {
	  const char *ocname, *ofname;
	  const Attribute *oinvitem;

	  oattr->getInverse(&ocname, &ofname, &oinvitem);
	  completeInverse(m, ocname, ofname, oinvitem);

	  if (oinvitem)
	    odl_remove_relationship(m, cls, name, oattr, oinvitem);
	}
      }

    for (int i = 0; i < attr_cnt; i++)
      {
	const Attribute *attr = attrs[i];
	const Attribute *oattr;
	const Attribute *invitem;
	const char *cname, *fname;

	attr->getInverse(&cname, &fname, &invitem);
	completeInverse(m, cname, fname, invitem);

	if (oattr = ocls->getAttribute(attr->getName())) {
	  const char *ocname, *ofname;
	  const Attribute *oinvitem;

	  oattr->getInverse(&ocname, &ofname, &oinvitem);
	  completeInverse(m, ocname, ofname, oinvitem);

	  if (oinvitem && !invitem)
	    odl_remove_relationship(m, cls, name, attr, oinvitem);
	  else if (invitem && !oinvitem)
	    odl_add_relationship(m, cls, name, attr, invitem);
	  else if (invitem && oinvitem &&
		   strcmp(invitem->getName(), oinvitem->getName())) {
	    odl_remove_relationship(m, cls, name, attr, oinvitem);
	    odl_add_relationship(m, cls, name, attr, invitem);
	  }
	}
	else if (invitem)
	  odl_add_relationship(m, cls, name, attr, invitem);
      }

    return 0;
  }
#else
  int
  odlAgregatClass::manageDiffRelationShips(Schema *m, Bool diff)
  {
    int attr_cnt, oattr_cnt;
    const Attribute **attrs  = cls->getAttributes(attr_cnt);
    const Attribute **oattrs = ocls->getAttributes(oattr_cnt);

    //assert(attr_cnt == oattr_cnt);
    int cnt = (attr_cnt < oattr_cnt ? attr_cnt : oattr_cnt);

    for (int i = 0; i < cnt; i++)
      {
	const char *cname, *fname, *ocname, *ofname;
	const Attribute *invitem, *oinvitem;

	attrs[i]->getInverse(&cname, &fname, &invitem);
	oattrs[i]->getInverse(&ocname, &ofname, &oinvitem);

	completeInverse(m, ocname, ofname, oinvitem);
	completeInverse(m, cname, fname, invitem);

	if (oinvitem && !invitem)
	  {
	    cls->touch();
	    odl_remove_relationship(m, name, attrs[i], oinvitem);
	  }
	else if (invitem && !oinvitem)
	  {
	    cls->touch();
	    odl_add_relationship(m, name, oattrs[i], invitem);
	  }
      }

    return 0;
  }
#endif

  Bool
  odl_exec_removed(ClassComponent *comp)
  {
    if (!comp)
      return False;
  
    if (!comp->asTrigger() && !comp->asMethod())
      return False;

    odlSignUserData *sud = (odlSignUserData *)comp->getUserData();
    return (sud && sud->upd_hints &&
	    sud->upd_hints->type == odlUpdateHint::Remove ?
	    True : False);
  }

#ifdef NEW_REORDER
  int
  odlAgregatClass::preManage(Schema *m)
  {
    if (odl_gencode || !m->getDatabase() || !cls)
      return 0;

    if (!ocls)
      {
	odl_add_class(m, cls);
	return 0;
      }

    // added 31/05/01
    if (agrspec == odl_Declare)
      return 0;

    if (agrspec != odl_NativeClass) {
      odl_class_premanage(m, ocls, cls);
      if (odl_error)
	return 0;
    }

    return 0;
  }
#endif

  const Attribute *
  odl_getattr(const Class *cls, const AttributeComponent *attr_comp)
  {
    char *s = strdup(attr_comp->getAttrpath().c_str());
    char *q = strchr(s, '.');

    if (!q) {
      odl_add_error("unexpected attrpath '%s'\n", s);
      free(s);
      return 0;
    }

    *q = 0;
    char *p = strchr(q+1, '.');
    if (p) *p = 0;
    const Attribute *attr = cls->getAttribute(q+1);

    if (!attr) {
      odl_add_error("attribute %s not found in class %s\n", q+1, cls->getName());
      free(s);
      return 0;
    }

    free(s);
    return attr;
  }

  static int
  odl_remove_components(Schema *m, Class *ocls, Class *cls)
  {
    m->getDatabase()->transactionBegin();

    const char *name = ocls->getName();
    const LinkedList *ocomplist = ocls->getCompList();
    LinkedListCursor oc(ocomplist);
    ClassComponent *clcomp;
    while (oc.getNext((void *&)clcomp)) {
      if (!strcmp(clcomp->getClassOwner()->getName(), name))
	odl_remove_component(m, clcomp);
    }

    const LinkedList *attr_ocomplist;
    Status s = ocls->getAttrCompList(attr_ocomplist);
    if (s) {odl_add_error(s); return 1;}

    AttributeComponent *attr_comp;
    LinkedListCursor attr_oc(attr_ocomplist);

    while (attr_oc.getNext((void *&)attr_comp)) {
      //if (!strcmp(attrcomp->getClassOwner()->getName(), name))
      AttributeComponent *cattr_comp = 0;
      if (ocls && ocls->getDatabase() &&
	  attr_comp->find(ocls->getDatabase(), ocls->getParent(),
			  cattr_comp) == Success) {
	if (!cattr_comp || !cattr_comp->getPropagate())
	  odl_remove_component(m, attr_comp);
      }
      else
	odl_remove_component(m, attr_comp);
    }

    m->getDatabase()->transactionAbort();
    return 0;
  }

  int
  odlAgregatClass::manageDifferences(Database *db, Schema *m, Bool diff)
  {
    if (upd_hints && upd_hints->type == odlUpdateHint::Remove &&
	!odl_gencode && m->getDatabase()) {
      int r = odl_remove_components(m, ocls, cls);
      if (r)
	return r;
      return odl_remove_relationships(db, m, ocls);
    }

    if (odl_gencode || !m->getDatabase() || !cls)
      return 0;

    odl_migrate_attributes(m, cls);
    if (!ocls) {
#ifndef NEW_REORDER
      odl_add_class(m, cls);
#endif
      return 0;
    }
    
    // added 31/05/01
    if (agrspec == odl_Declare)
      return 0;

    if (agrspec != odl_NativeClass) {
      odl_class_compare(m, ocls, cls, upd_hints);
      if (odl_error)
	return 0;
      odl_class_parent_compare(m, ocls, cls);
    }

    const LinkedList *complist = cls->getCompList();
    const LinkedList *ocomplist = ocls->getCompList();

    LinkedListCursor oc(ocomplist);
    LinkedListCursor c(complist);

    //printf("NEW COMLIST = %d %s\n", complist->getCount(), cls->getName());
    ClassComponent *clcomp = 0, *fclcomp;

    m->getDatabase()->transactionBegin();

    while (oc.getNext((void *&)clcomp)) {
      if (clcomp->getClassOwner()->compare(ocls)) {
	if (!odl_find_component(clcomp, complist, False, fclcomp)) {
	  if ((!upd_hints || upd_hints->type != odlUpdateHint::Extend) &&
	      agrspec != odl_NativeClass)
	    odl_remove_component(m, clcomp);
	}
	else if (odl_exec_removed(fclcomp))
	  odl_remove_component(m, clcomp);
      }
    }
  
    while (c.getNext((void *&)clcomp)) {
      ClassComponent *oclcomp = clcomp;
      if (clcomp->getClassOwner()->compare(cls)) {
	if (!odl_find_component(clcomp, ocomplist, True, fclcomp)) {
	  if (!odl_exec_removed(oclcomp))
	    odl_add_component(m, clcomp);
	  else {
	    fprintf(stderr, "warning: cannot remove non existing '");
	    clcomp->m_trace(stderr, 0, 0, NoRecurs);
	    fprintf(stderr, "'\n");
	  }
	}
      }
    }

    const LinkedList *attr_ocomplist, *attr_complist;
    Status s = cls->getAttrCompList(attr_complist);
    if (s) {odl_add_error(s); return 1;}
    s = ocls->getAttrCompList(attr_ocomplist);
    if (s) {odl_add_error(s); return 1;}

    /*
      printf("\nClass %s Attribute Component List Count=%d, Old Count=%d\n",
      cls->getName(),
      (attr_complist ? attr_complist->getCount() : -1),
      (attr_ocomplist ? attr_ocomplist->getCount() : -1));
    */

    LinkedListCursor attr_oc(attr_ocomplist);
    LinkedListCursor attr_c(attr_complist);

    AttributeComponent *attr_comp, *fattr_comp;
    int err = 0;
    while (attr_oc.getNext((void *&)attr_comp)) {
      if (!odl_find_component(attr_comp, attr_complist, False,
			      fattr_comp)) {
	const Attribute *attr = odl_getattr(ocls, attr_comp);
	if (!attr) return 1;
	if (attr = odl_is_remove_attribute(m, attr)) {
	  std::string s = std::string("removed attribute '") +
	    attr->getClassOwner()->getName () + "::" + attr->getName() +
	    "' : must remove " +
	    (attr_comp->asIndex() ? "index" : "constraint") +
	    " '" +  attr_comp->getAttrpath() + "' manually using '" +
	    (attr_comp->asIndex() ? "idxdelete" : "consdelete") +
	    "' before updating the schema\n";
	  odl_add_error(s);
	  err++;
	}
	// warning: odl_rmv_undef_attrcomp is not well implemented !
	if (odl_rmv_undef_attrcomp &&
	    (!upd_hints || upd_hints->type != odlUpdateHint::Extend))
	  odl_remove_component(m, attr_comp);
      }
    }
    if (err) return 1;

    LinkedList torm_list;

    while (attr_c.getNext((void *&)attr_comp)) {
      AttributeComponent *oattr_comp = attr_comp;
      if (!odl_find_component(attr_comp, attr_ocomplist, True, fattr_comp)) {
	//printf("component %s not found\n", attr_comp->getName());
	odl_add_component(m, attr_comp);
	const Attribute *attr = odl_getattr(cls, attr_comp);
	if (!attr) return 1;
	if (odl_is_update_attribute(m, attr))
	  torm_list.insertObject(attr_comp);
      }
    }

    LinkedListCursor ctorm(torm_list);
    while (ctorm.getNext((void *&)attr_comp)) {
      //printf("XCOMP: SUPPRESSING %s\n", attr_comp->getName());
      cls->suppress(attr_comp->getInd(), attr_comp);
    }

    if (agrspec != odl_NativeClass && !odl_error)
      manageDiffRelationShips(db, m, diff);

    m->getDatabase()->transactionAbort();
    return 0;
  }

  // WARNING: this should centralized in a .h
#define HASH_COEF 64
#define DEFAULT_MAG_ORDER 60000
  // .....

#if 0
  unsigned int
  odlImplementation::getMagOrder() const
  {
    unsigned magorder;
    if (!strcasecmp(ident, "btree"))
      return IDB_COLL_BIDX_MASK;

    if (!strcasecmp(ident, "hash"))
      return (nentries ? nentries * HASH_COEF : DEFAULT_MAG_ORDER);

    odl_add_error("expected keyword 'btree' or 'hash', got '%s'", ident);
    return 0;
  }
#endif

  odlBool
  odlAgregatClass::hasSimilarComp(odlAttrComponent *comp,
				  const Class *cls2)
  {
    //printf("ODL AGREGAT CLASS %s %s\n", cls->getName(), cls2->getName());
    odlDeclRootLink *l = decl_list->first;
    while(l) {
      odlAttrComponent *xcomp = l->x->asAttrComponent();
      if (xcomp && xcomp->similar(comp, cls, cls2))
	return odlTrue;
      l = l->next;
    }

    return odlFalse;
  }

  void
  odlAgregatClass::addComp(odlAttrComponent *comp)
  {
    decl_list->add(comp->clone());
  }

  int
  odlAgregatClass::propagateComponents(Database *db, Schema *m)
  {
    odlDeclRootLink *l = decl_list->first;
    if (!l)
      return 0;

    //printf("\npropagate components %s\n", cls->getAliasName());
    Class **subclasses;
    unsigned int subclass_cnt;
    Status s = cls->getSubClasses(subclasses, subclass_cnt);
    if (s) {
      odl_add_error(s);
      return 1;
    }

    while(l) {
      odlAttrComponent *comp = l->x->asAttrComponent();
      if (comp && comp->propagate) {
	//printf("checking comp %s\n", comp->attrpath);
	for (int i = 0; i < subclass_cnt; i++) {
	  Class *clsx = subclasses[i];
	  if (clsx == cls) continue;
	  odlAgregatClass *x = (odlAgregatClass *)clsx->getUserData(odlSELF);
	  if (x && !x->hasSimilarComp(comp, cls)) {
	    // warning: odl_rmv_undef_attrcomp is not well implemented !
	    if (odl_rmv_undef_attrcomp) {
	      //printf("%s -> %s %s\n", cls->getName(), clsx->getName(), comp->attrpath);
	      odl_add_error("attribute component %s: "
			    "when using the -rmv-undef-attrcomp option, all the "
			    "attribute components must be defined in the ODL",
			    comp->attrpath);
	      return 1;
	    }
	    //printf("adding component %s to %s\n", comp->attrpath, clsx->getAliasName());
	    x->addComp(comp);
	  }
	  /*
	    else
	    printf("%s has a similar component %s\n", clsx->getAliasName(),
	    comp->attrpath);
	  */
	}
      }
      l = l->next;
    }

    return 0;
  }

  int odlAgregatClass::postRealize(Database *db, Schema *m,
				   const char *prefix)
  {
    odlDeclRootLink *l = decl_list->first;

    while(l) {
      if (l->x->asAttrComponent())
	realize(db, l->x->asAttrComponent(), m, prefix);
      l = l->next;
    }

    return 0;
  }

  int odlAgregatClass::realize(Database *db, Schema *m, const char *prefix,
			       const char *package, Bool diff)
  {
    if (!cls)
      return 0;

#if 0
    unsigned int magorder = getMagOrder(cls, attr_list);

    if (magorder)
      cls->setMagOrder(magorder);
#endif

    Attribute **agr;

    agr = new Attribute*[decl_list->count];
    memset(agr, 0, sizeof(Attribute *) * decl_list->count);

    ClassComponent **comp = new ClassComponent*[decl_list->count*8];

    int comp_cnt = 0, agr_cnt = 0;

    char *def_extref_fe = new char[strlen(package)+strlen("mthfe")+1];
    sprintf(def_extref_fe, "%smthfe", package);

    char *def_extref_be = new char[strlen(package)+strlen("mthbe")+1];
    sprintf(def_extref_be, "%smthbe", package);

    odlDeclRootLink *l = decl_list->first;

    while(l) {
      if (l->x->asExecSpec()) {
	odlBool isClient =
	  (l->x->asExecSpec()->asMethodSpec() ?
	   l->x->asExecSpec()->asMethodSpec()->mth_hints.isClient : odlFalse);
	realize(db, m, l->x->asExecSpec(),
		(isClient ? def_extref_fe : def_extref_be));
      }
    
      l = l->next;
    }

    l = decl_list->first;

    while(l) {
      if (l->x->asDeclItem()) {
	realize(l->x->asDeclItem(), m, prefix, agr_cnt, comp, comp_cnt, agr);
	agr_cnt++;
      }
      else if (l->x->asQuotedSeq())
	add_quoted_seq(cls, l->x->asQuotedSeq()->quoted_seq);

      l = l->next;
    }

    if (odl_error) {
      if (agrspec != odl_NativeClass && agrspec != odl_Declare)
	m->suppressClass(cls);
      return odl_error;
    }

    if (agrspec != odl_NativeClass && agrspec != odl_Declare) {
      Status status;
      status = cls->setAttributes(agr, agr_cnt);

      if (status) {
	odl_add_error(status);
	return 0;
      }
    }
  
    for (int i = 0; i < comp_cnt; i++)
      cls->add(comp[i]->getInd(), comp[i]);

    return propagateComponents(db, m);
  }

  int
  odlEnumClass::record(Database *, Schema *m,
		       const char *prefix, const char *db_prefix)
  {
    if (check(m, prefix))
      odl_error++;

    cls = new EnumClass(makeName(name, prefix));
    cls->setUserData(odlGENCODE, AnyUserData);

    if (aliasname)
      cls->setAliasName(aliasname);
    else if (db_prefix)
      cls->setAliasName(makeName(name, db_prefix));

    // added the 17/01/01
    ocls = eyedb::getClass(m, (aliasname ? aliasname : name), prefix);
    if (ocls && ocls != cls)
      {
	m->suppressClass(ocls);
	cls->setUserData(ocls);
	ObjectPeer::setOid(cls, ocls->getOid());
      }
    // ...

    m->addClass(cls);
    if (odl_system_class)
      ClassPeer::setMType(cls, Class::System);
    return 0;
  }

  int odlEnumClass::realize(Database *db, Schema *m, const char *prefix,
			    const char *, Bool)
  {
    EnumItem **en = new EnumItem*[enum_list->count];

    memset(en, 0, sizeof(EnumItem *) * enum_list->count);

    odlEnumItemLink *l = enum_list->first;
  
    int n = 0;
    int lastvalue, value;
    lastvalue = 0;

    while(l)
      {
	odlEnumItem *item = l->x;

	if (item->novalue)
	  value = lastvalue;
	else
	  value = item->value;

	//      if (check_name(m, name, 0, item->name, prefix))
	if (check_name(m, (aliasname ? aliasname : name), 0, item->name, prefix))
	  odl_error++;

	en[n++] = new EnumItem(item->name, item->aliasname, value);

	lastvalue = value + 1;
	l = l->next;
      }

    Status status;
    status = ((EnumClass *)cls)->setEnumItems(en, n);

    if (status)
      odl_add_error(status);

    return 0;
  }

  const char *odl_get_typname(const char *typname)
  {
    if (!strcmp(typname, "int"))
      return int32_class_name;

    if (!strcmp(typname, "short"))
      return int16_class_name;

    if (!strcmp(typname, "long"))
      return int64_class_name;

    if (!strcmp(typname, "octet"))
      return "byte";

    if (!strcmp(typname, "boolean"))
      return int32_class_name;

    if (!strcmp(typname, "double"))
      return "float";

    if (!strcmp(typname, "sequence"))
      return "array";

    return typname;
  }

  int
  odl_generate(Schema *m, const char *ofile)
  {
    FILE *fd;

    Class *superclass = NULL;
    if (odlAgregatClass::superclass)
      {
	superclass = m->getClass(get_superclass_name());
	if (odlAgregatClass::superclass->getAgregSpec() == odl_RootClass)
	  superclass->setIsRootClass();
      }

    if (!ofile)
      fd = odl_fd;
    else
      {
	fd = fopen(ofile, "w");
	if (!fd)
	  {
	    odl_add_error("cannot open file '%s' for reading.\n",
			  ofile);
	    return 1;
	  }
      }

    m->genODL(fd);

    return 0;
  }

  static int odl_echo;

  void
  odl_prompt_init(FILE *fd)
  {
    odl_echo = isatty(fileno(fd));
  }

  void
  odl_prompt(const char *prompt)
  {
    if (odl_echo)
      {
	fprintf(odl_fd, prompt);
	fflush(odl_fd);
      }
  }

  // ----------------------------------------------------------------------------
  //
  // Database API
  //
  // ----------------------------------------------------------------------------

  static Status
  odl_post_update_non_components(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci)) {
      if (ci->asUpdateComponent() ||
	  ci->asUpdateRelationship() ||
	  ci->asRemoveClass())
	continue;

      s = ci->postPerform(db, m);
      if (s) return s;
    }

    return Success;
  }

  static Status
  odl_post_update_components_1(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci)) {
      if (!ci->asUpdateComponent())
	continue;

      ClassComponent *comp = ci->asUpdateComponent()->cls_comp;

      if (comp && comp->asMethod()) {
	s = ci->postPerform(db, m);
	if (s) return s;
      }
    }

    return Success;
  }

  static Status
  odl_post_update_components_2(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci)) {
      if (!ci->asUpdateComponent())
	continue;

      ClassComponent *cls_comp = ci->asUpdateComponent()->cls_comp;
      AttributeComponent *attr_comp = ci->asUpdateComponent()->attr_comp;
    
      if ((cls_comp && cls_comp->asMethod()) ||
	  (attr_comp && attr_comp->asIndex()))
	continue;
    
      s = ci->postPerform(db, m);
      if (s) return s;
    }

    return Success;
  }

  static Status
  odl_post_update_indexes(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    odlUpdateItem *ci;
    Status s;

    while (c.getNext((void *&)ci)) {
      if (!ci->asUpdateComponent())
	continue;

      AttributeComponent *comp = ci->asUpdateComponent()->attr_comp;
    
      if (comp && comp->asIndex()) {
	s = ci->postPerform(db, m);
	if (s) return s;
      }
    }

    return Success;
  }

  static Status
  odl_post_update_relships(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci)) {
      if (!ci->asUpdateRelationship())
	continue;

      const Attribute *invitem = ci->asUpdateRelationship()->invitem;

      if (!invitem)
	continue;
    
      s = ci->postPerform(db, m);
      if (s) return s;
    }

    return Success;
  }

  static Status
  odl_post_update_remove_classes(Database *db, Schema *m)
  {
    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci)) {
      if (!ci->asRemoveClass())
	continue;

      s = ci->postPerform(db, m);
      if (s) return s;
    }

    return Success;
  }

  static Status
  odl_post_update_components(Database *db, Schema *m)
  {
    Status s;
    s = odl_post_update_components_1(db, m);
    if (s) return s;

    s = odl_post_update_indexes(db, m);
    if (s) return s;

    s = odl_post_update_components_2(db, m);
    if (s) return s;

    return odl_post_update_relships(db, m);
  }

  static Status
  odl_post_update_items(Database *db, Schema *m)
  {
    if (odlUPDLIST(m)) {
      Status s;
      s = odl_post_update_non_components(db, m);
      if (s) return s;

      s = odl_post_update_components(db, m);
      if (s) return s;

      s = odl_post_update_remove_classes(db, m);
      if (s) return s;
    }

    return odl_post_update(db);
  }

  static Status
  odl_pre_update_items(Database *db, Schema *m)
  {
    if (!odlUPDLIST(m))
      return Success;

    LinkedList *list = odlUPDLIST(m);
    LinkedListCursor c(list);
    Status s;
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci))
      {
	s = ci->prePerform(db, m);
	if (s) return s;
      }

    return Success;
  }

  static void
  odl_diff_realize(Database *db, Schema *m, const char *odlfile)
  {
    LinkedList *list = odlUPDLIST(m);

    if (!list)
      return;

    //m->trace();

    LinkedListCursor c(list);
    odlUpdateItem *ci;
    while (c.getNext((void *&)ci))
      {
	odl_diff++;
	ci->displayDiff(db, odlfile);
      }

    db->transactionAbort();
  }


  static int odl_finished;

  static void  *
  patient_thr(void *arg)
  {
    if (getenv("EYEDBNOPATIENT")) // for debugging only
      return 0;

    sleep(4);
    while (!odl_finished)
      {
	fprintf(odl_fd, ".");
	fflush(odl_fd);
	sleep(1);
      }

    return (void *)0;
  }

  static void
  run_patient()
  {
    pthread_t tid;
    pthread_create(&tid, NULL, patient_thr, NULL);
  }

  static int
  odl_update_realize(Database *db, Schema *m, const char *dbname,
		     const char *schname)
  {
    odl_skip_volatiles(db, m);

    fprintf(odl_fd, "Updating '%s' schema in database %s...",  schname, dbname);
    fflush(odl_fd);

    Status s;

    if (db->getUserData() && (s = m->storeName()))
      {
	odl_add_error(s);
	return 1;
      }

    s = odl_pre_update_items(db, m);

    if (s)
      {
	odl_add_error(s);
	return 1;
      }

    run_patient();
    s = m->realize();

    if (s)
      {
	odl_add_error(s);
	return 1;
      }

    s = odl_post_update_items(db, m);

    if (s)
      {
	odl_add_error(s);
	return 1;
      }

    if (!getenv("EYEDBABORT"))
      s = db->transactionCommit();
    odl_finished = 1;

    if (s)
      {
	odl_add_error(s);
	return 1;
      }

    fprintf(odl_fd, "\nDone\n");

    return 0;
  }

  static int
  check_odlfile(const char *file, const char *cpp_cmd, const char *cpp_flags)
  {
    odl_decl_list = new LinkedList();

    if (file) {
      FILE *fd;
      
      if (!strcmp(file, "-"))
	odlin = stdin;
      else {
	fd = fopen(file, "r");
	if (!fd) {
	  odl_add_error("cannot open file '%s' for reading\n",
			file);
	  return 1;
	}

	if (odlin && odlin != stdin)
	  fclose(odlin);
	
	odlin = run_cpp(fd, cpp_cmd, cpp_flags, file);
	if (!odlin)
	  return 1;
      }
    }

    if (file) {
      odl_prompt_init(odlin);
      odl_prompt("ODL construct expected on standard input (end input by a ^D) :\n");
      odl_prompt();
    }
  
    return 0;
  }

  /*
    static Status
    x_transaction_begin(Database *db)
    {
    TransactionParams params = Database::getGlobalDefaultTransactionParams();
    params.lockmode = DatabaseX;
    params.wait_timeout = 1;

    Status s = db->transactionBegin(params);
    if (s) {
    if (s->getStatus() == SE_LOCK_TIMEOUT) {
    fprintf(stderr,
    "cannot acquire exclusive lock on database %s\n",
    db->getName());
    exit(1);
    }
    }

    return s;
    }
  */

  static int
  odl_check_db(Database *db, const char *&schname, const char *package,
	       Schema *&m, Bool update = False)
  {
    if (!schname)
      schname = package;

    if (db) {
      Status s;
      Exception::Mode mode =
	Exception::setMode(Exception::StatusMode);

      if (update)
	s = db->transactionBeginExclusive();
      else
	s = db->transactionBegin();

      if (s) {
	odl_add_error(s);
	return 1;
      }

      Exception::setMode(mode);
      m = db->getSchema();
      if (strcmp(m->getName(), schname))
	db->setUserData(AnyUserData);

      LinkedListCursor c(m->getClassList());
      Class *cls;
      while (c.getNext((void *&)cls))
	if (!cls->isSystem())
	  cls->setUserData(odlGENCODE, AnyUserData);
    }
    else {
      m = new Schema();
      m->init();
      syscls::updateSchema(m);
      oqlctb::updateSchema(m);
      utils::updateSchema(m);
    }
    
    m->setName(schname);
    return 0;
  }

  Status
  odl_prelim(Database *db, const char *odlfile, const char *package,
	     const char *&schname, const char *prefix, const char *db_prefix,
	     Bool diff, const char *cpp_cmd,  const char *cpp_flags,
	     Schema *&m, Bool update = False)
  {
    odl_error = 0;
    odl_str_error = "";

    int r;
    r = odl_check_db(db, schname, package, m, update);
    if (r || odl_error) return odl_status_error(r);

    r = check_odlfile(odlfile, cpp_cmd, cpp_flags);

    if (r || odl_error) return odl_status_error(r);

    if (odlfile) {
      r = odlparse();
      if (r || odl_error) return odl_status_error(r);
    }

    odl_realize(db, m, odl_decl_list, prefix, db_prefix, package, diff);

    return odl_status_error();
  }

  Status
  Database::updateSchema(const char *odlfile, const char *package,
			 const char *schname, const char *db_prefix,
			 FILE *fd, const char *cpp_cmd,
			 const char *cpp_flags)
  {
    odl_fd = (fd ? fd : odl_fdnull);
    Schema *m;
    Status s;
    s = odl_prelim(this, odlfile, package, schname, db_prefix, db_prefix,
		   False, cpp_cmd, cpp_flags, m, True);
    if (s) return s;

    odl_update_realize(this, sch, name, schname);
    return odl_status_error();
  }

  Status
  Schema::displaySchemaDiff(Database *db, const char *odlfile,
			    const char *package,
			    const char *db_prefix,
			    FILE *fd, const char *cpp_cmd,
			    const char *cpp_flags)
  {
    odl_fd = (fd ? fd : odl_fdnull);
    Schema *m;
    const char *schname = "";
    Status s;
    s =  odl_prelim(db, odlfile, package, schname, db_prefix, db_prefix,
		    True, cpp_cmd, cpp_flags, m);
    if (s) return s;
    if (odl_error) return odl_status_error();
    odl_diff_realize(db, m, odlfile);
    return odl_status_error();
  }

#ifdef SUPPORT_CORBA
  Status
  Schema::genIDL(Database *db, const char *odlfile,
		 const char *package,
		 const char *module,
		 const char *schname,
		 const char *prefix,
		 const char *db_prefix,
		 const char *idltarget,
		 const char *imdlfile,
		 Bool no_generic_idl,
		 const char *generic_idl,
		 Bool no_factory,
		 Bool imdl_synchro,
		 GenCodeHints *gc_hints,
		 Bool comments,
		 const char *cpp_cmd, const char *cpp_flags,
		 const char *ofile)
  {
    if (!gc_hints) gc_hints = new GenCodeHints();
    Schema *m;
    Status s;
    s = odl_prelim(db, odlfile, package, schname, prefix, db_prefix,
		   False, cpp_cmd, cpp_flags, m);
    if (s) return s;

    imdl_realize(db, m, package, module, False, False,
		 schname, prefix,
		 db_prefix, cpp_cmd, cpp_flags,
		 idltarget, imdlfile,
		 no_generic_idl, generic_idl,
		 no_factory, imdl_synchro, gc_hints, comments,
		 ofile);

    return odl_status_error();
  }

  Status
  Schema::genORB(const char *orb,
		 Database *db, const char *odlfile,
		 const char *package,
		 const char *module,
		 const char *schname,
		 const char *prefix,
		 const char *db_prefix,
		 const char *idltarget,
		 const char *imdlfile,
		 Bool no_generic_idl,
		 const char *generic_idl,
		 Bool no_factory,
		 Bool imdl_synchro,
		 GenCodeHints *gc_hints,
		 Bool comments,
		 const char *cpp_cmd, const char *cpp_flags)
  {
    if (!gc_hints) gc_hints = new GenCodeHints();
    Schema *m;
    Status s;
    s = odl_prelim(db, odlfile, package, schname, prefix, db_prefix,
		   False, cpp_cmd, cpp_flags, m);
    if (s) return s;

    Bool orbix_gen, orbacus_gen;
    if (!strcasecmp(orb, "orbix"))
      {
	orbix_gen = True;
	orbacus_gen = False;
      }
    else
      {
	orbix_gen = False;
	orbacus_gen = True;
      }

    imdl_realize(db, m, package, module, orbix_gen, orbacus_gen,
		 schname, prefix,
		 db_prefix, cpp_cmd, cpp_flags,
		 idltarget, imdlfile,
		 no_generic_idl, generic_idl,
		 no_factory, imdl_synchro, gc_hints, comments, 0);

    return odl_status_error();
  }
#endif

  Status
  Schema::genC_API(Database *db, const char *odlfile,
		   const char *package,
		   const char *schname,
		   const char *c_namespace,
		   const char *prefix,
		   const char *db_prefix,
		   Bool _export,
		   GenCodeHints *gc_hints,
		   const char *cpp_cmd, const char *cpp_flags)
  {
    odl_error = 0;
    odl_str_error = "";

    if (!gc_hints) gc_hints = new GenCodeHints();
    Schema *m;
    odl_check_db(db, schname, package, m);

    int r = check_odlfile(odlfile, cpp_cmd, cpp_flags);

    if (r || odl_error) return odl_status_error(r);

    if (odlfile)
      {
	r = odlparse();
	if (r || odl_error) return odl_status_error(r);
      }

    odl_generate_code(db, m, ProgLang_C,
		      odl_decl_list,
		      package,
		      schname,
		      c_namespace,
		      prefix,
		      db_prefix,
		      _export,
		      *gc_hints);

    return odl_status_error();
  }

  Status
  Schema::genJava_API(Database *db, const char *odlfile,
		      const char *package,
		      const char *schname,
		      const char *prefix,
		      const char *db_prefix,
		      Bool _export,
		      GenCodeHints *gc_hints,
		      const char *cpp_cmd, const char *cpp_flags)
  {
    if (!gc_hints) gc_hints = new GenCodeHints();
    Schema *m;
    Status s;

    odl_check_db(db, schname, package, m);

    int r = check_odlfile(odlfile, cpp_cmd, cpp_flags);

    if (r || odl_error) return odl_status_error(r);

    if (odlfile)
      {
	r = odlparse();
	if (r || odl_error) return odl_status_error(r);
      }

    odl_generate_code(db, m, ProgLang_Java,
		      odl_decl_list,
		      package,
		      schname,
		      NULL,
		      prefix,
		      db_prefix,
		      _export,
		      *gc_hints);

    return odl_status_error();
  }

  Status
  Schema::checkODL(const char *odlfile, const char *package,
		   const char *cpp_cmd, const char *cpp_flags)
  {
    odl_error = 0;
    odl_str_error = "";

    Schema *m;
    const char *schname = "";
    return odl_prelim(0, odlfile, package, schname, "", "",
		      False, cpp_cmd, cpp_flags, m);
  }

  Status
  Schema::genODL(Database *db,
		 const char *odlfile,
		 const char *package,
		 const char *schname,
		 const char *prefix,
		 const char *db_prefix,
		 const char *ofile,
		 const char *cpp_cmd, const char *cpp_flags)
  {
    odl_error = 0;
    odl_str_error = "";

    Schema *m;

    if (odlfile)
      {
	Status s;
	s = odl_prelim(db, odlfile, package, schname, prefix, db_prefix,
		       False, cpp_cmd, cpp_flags, m);
	if (s) return s;
      }
    else
      {
	int r = odl_check_db(db, schname, package, m);
	if (r || odl_error) return odl_status_error(r);
      }

    odl_generate(m, ofile);

    return odl_status_error();
  }
}
