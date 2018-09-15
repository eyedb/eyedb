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


//#define protected public
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <time.h>
#include "eyedb_p.h"
#include <eyedblib/rpc_be.h>
#include "DBM_Database.h"
//#include "ipattern.cc"
#include "make_pattern.cc"
#include "odl.h"
#include "Attribute_p.h"
#include "kernel.h"
#include "oqlctb.h"
#include <compile_builtin.h>

using namespace std;

namespace eyedb {

  extern Bool odl_smartptr;

  // 22/08/01: defined if one wants the same API when using dynamic-attr
  // option or not:
#define UNIFIED_API

  //#define DEF_PREFIX "idb"
#define DEF_PREFIX ""

#ifdef UNIFIED_API
  static const char dyn_call_error[] = "can be called only when the dynamic attribute fonctionnality is on (-dynamic-attr option of eyedbodl)";
#endif

  Database *default_db;
  struct SchemaLink {
    Oid oid;
    char *name;
    Class *cls;
    Class *cl;

    SchemaLink *next;

    SchemaLink(const Oid &_oid, Class *_class) {
      oid = _oid;
      cls = _class;
      name = 0;
      cl = 0;
      next = 0;
    }

    SchemaLink(const char *_name, Class *_class) {
      name = strdup(_name);
      cls = _class;
      next = 0;
    }

    SchemaLink(Class *_class) {
      cls = _class;
      name = 0;
      next = 0;
    }

    ~SchemaLink() {
      free(name);
    }
  };

  int check_pointer(void *);
  struct SchemaHashList {

    SchemaLink *first, *last;

    SchemaHashList() {first = last = 0;}

    inline void insert(SchemaLink *l) {
      if (last)
	last->next = l;
      else
	first = l;

      last = l;
    }

    ~SchemaHashList() {

      SchemaLink *l = first;
      while (l)
	{
	  SchemaLink *next = l->next;
	  delete l;
	  l = next;
	}
      first = last = 0;
    }
  };

  static void stop_1() {}
  struct SchemaHashTable {

    int hash_cnt;
    unsigned int mask;
    SchemaHashList **names_lists;
    SchemaHashList **oids_lists;
    SchemaHashList **pts_lists;

    inline int get_key(const Oid &oid) {
      return oid.getNX() & mask;
    }

    inline int get_key(const Class *cls) {
      return (((unsigned long)cls)>>2) & mask;
    }

    inline int get_key(const char *name) {
      int len = strlen(name);
      int k = 0;

      for (int i = 0; i < len; i++)
	k += *name++;

      return k & mask;
    }

    inline SchemaHashTable() {
      hash_cnt = 256;
      mask = hash_cnt - 1;
      names_lists = new SchemaHashList*[hash_cnt];
      memset(names_lists, 0, sizeof(SchemaHashList *)*hash_cnt);

      oids_lists = new SchemaHashList*[hash_cnt];
      memset(oids_lists, 0, sizeof(SchemaHashList *)*hash_cnt);

      pts_lists = new SchemaHashList*[hash_cnt];
      memset(pts_lists, 0, sizeof(SchemaHashList *)*hash_cnt);
    }

    ~SchemaHashTable() {

      for (int k = 0; k < hash_cnt; k++)
	{
	  delete oids_lists[k];
	  delete names_lists[k];
	  delete pts_lists[k];
	}

      delete [] oids_lists;
      delete [] names_lists;
      delete [] pts_lists;
    }

    void insert(const Oid &oid, Class *cls) {
#ifdef OPTOPEN_TRACE
      printf("inserting %s %s %p\n", oid.getString(), cls->getName(), cls);
      if (!strcmp(cls->getName(), "set<object*>") ||
	  !strcmp(cls->getName(), "set<Division*>"))
	stop_1();
#endif
      int key = get_key(oid);

      if (!oids_lists[key])
	oids_lists[key] = new SchemaHashList();
      SchemaLink *l = new SchemaLink(oid, cls);

      oids_lists[key]->insert(l);
    }

    void insert(const char *name, Class *cls) {
      int key = get_key(name);

      if (!names_lists[key])
	names_lists[key] = new SchemaHashList();
      SchemaLink *l = new SchemaLink(name, cls);

      names_lists[key]->insert(l);
    }

    void insert(Class *cls) {
      int key = get_key(cls);

      if (!pts_lists[key])
	pts_lists[key] = new SchemaHashList();
      SchemaLink *l = new SchemaLink(cls);

      pts_lists[key]->insert(l);
    }

    Class *get(const Oid &oid) {
      int key = get_key(oid);

      if (!oids_lists[key])
	return 0;
      SchemaLink *l = oids_lists[key]->first;
      while (l)
	{
	  if (l->oid.compare(oid))
	    return l->cls;
	  l = l->next;
	}
      return 0;
    }

    Class *get(const char *name) {
      int key = get_key(name);

      if (!names_lists[key])
	return 0;

      SchemaLink *l = names_lists[key]->first;
      while (l)
	{
	  if (!strcmp(l->name, name))
	    return l->cls;
	  l = l->next;
	}

      return 0;
    }

    Class *get(const Class *cls) {
      int key = get_key(cls);

      if (!pts_lists[key])
	return 0;

      SchemaLink *l = pts_lists[key]->first;
      while (l)
	{
	  if (l->cls == cls)
	    return l->cls;
	  l = l->next;
	}
      return 0;
    }

    Bool suppress(const char *name) {
      int key = get_key(name);

      if (!names_lists[key])
	return False;

      SchemaLink *prev = 0;
      SchemaLink *l = names_lists[key]->first;

      while (l)
	{
	  if (!strcmp(l->name, name))
	    {
	      if (prev)
		prev->next = l->next;
	      else
		names_lists[key]->first = l->next;

	      if (names_lists[key]->last == l)
		names_lists[key]->last = prev;

	      delete l;
	      return True;
	    }
	  prev = l;
	  l = l->next;
	}

      return False;
    }

    Bool suppress(const Oid &oid) {
      int key = get_key(oid);

      if (!oids_lists[key])
	return False;

      SchemaLink *prev = 0;
      SchemaLink *l = oids_lists[key]->first;

      while (l)
	{
	  if (l->oid.compare(oid))
	    {
	      if (prev)
		prev->next = l->next;
	      else
		oids_lists[key]->first = l->next;

	      if (oids_lists[key]->last == l)
		oids_lists[key]->last = prev;

	      delete l;
	      return True;
	    }
	  prev = l;
	  l = l->next;
	}

      return False;
    }

    Bool suppress(const Class *cls) {
      int key = get_key(cls);

      if (!pts_lists[key])
	return False;

      SchemaLink *prev = 0;
      SchemaLink *l = pts_lists[key]->first;

      while (l)
	{
	  if (l->cls == cls)
	    {
	      if (prev)
		prev->next = l->next;
	      else
		pts_lists[key]->first = l->next;

	      if (pts_lists[key]->last == l)
		pts_lists[key]->last = prev;

	      delete l;
	      return True;
	    }
	  prev = l;
	  l = l->next;
	}

      return False;
    }
  };

  struct deferredLink {
    char *clname;
    eyedbsm::Oid oid;
    deferredLink(const char *_clname, const eyedbsm::Oid *_oid) {
      clname = strdup(_clname);
      oid = *_oid;
    }
    ~deferredLink() {
      free(clname);
    }
  };

  Schema::Schema() : Instance()
  {
    _class = new LinkedList();
    hash = new SchemaHashTable();
    name = strdup("");
    deferred_list = 0;
    reversal = False;
    class_cnt = 0;
    classes = 0;
    dont_delete_comps = False;
  }

  Schema::Schema(const Schema &sch) : Instance(sch)
  {
    _class = 0;
    hash = 0;
    name = 0;
    class_cnt = 0;
    classes = 0;
    *this = sch;
  }

  Schema& Schema::operator=(const Schema &sch)
  {
    //garbage(); // automatically called by the copy operator of Object

    *(Instance *)this = (const Instance &)sch;
    _class = Object::copyList(sch._class, True);
    name = strdup(sch.name);
    deferred_list = 0;
    reversal = False;
    hash = 0;
    dont_delete_comps = sch.dont_delete_comps;
    computeHashTable();

    Object_Class = sch.Object_Class;
    Class_Class = sch.Class_Class;
    BasicClass_Class = sch.BasicClass_Class;
    EnumClass_Class = sch.EnumClass_Class;
    AgregatClass_Class = sch.AgregatClass_Class;
    StructClass_Class = sch.StructClass_Class;
    UnionClass_Class = sch.UnionClass_Class;
    Instance_Class = sch.Instance_Class;
    Basic_Class = sch.Basic_Class;
    Enum_Class = sch.Enum_Class;
    Agregat_Class = sch.Agregat_Class;
    Struct_Class = sch.Struct_Class;
    Union_Class = sch.Union_Class;
    Schema_Class = sch.Schema_Class;
    Bool_Class = sch.Bool_Class;
    CollectionClass_Class = sch.CollectionClass_Class;
    CollSetClass_Class = sch.CollSetClass_Class;
    CollBagClass_Class = sch.CollBagClass_Class;
    CollListClass_Class = sch.CollListClass_Class;
    CollArrayClass_Class = sch.CollArrayClass_Class;
    Collection_Class = sch.Collection_Class;
    CollSet_Class = sch.CollSet_Class;
    CollBag_Class = sch.CollBag_Class;
    CollList_Class = sch.CollList_Class;
    CollArray_Class = sch.CollArray_Class;
    Char_Class = sch.Char_Class;
    Byte_Class = sch.Byte_Class;
    OidP_Class = sch.OidP_Class;
    Int16_Class = sch.Int16_Class;
    Int32_Class = sch.Int32_Class;
    Int64_Class = sch.Int64_Class;
    Float_Class = sch.Float_Class;
    return *this;
  }

  void Schema::computeHashTable()
  {
    //  printf("\nSchema::computeHashTable(this=%p)\n\n", this);
    delete hash;
    hash = new SchemaHashTable();
    free(classes);

    LinkedListCursor c(_class);
    class_cnt = _class->getCount();
    classes = (Class **)malloc(sizeof(Class *)*class_cnt);

    Class *cl;

    for (int n = 0; c.getNext((void *&)cl); n++)
      {
	assert(!cl->isRemoved());
	hash->insert(cl->getOid(), cl);
	hash->insert(cl->getName(), cl);
	hash->insert(cl);
	classes[n] = cl;
	cl->setNum(n);
      }
  }

  void Schema::hashTableInvalidate()
  {
    //  delete hash;
    //  hash = (SchemaHashTable *)0;
  }

  //#define BUG_TRACE

  Status Schema::addClass_nocheck(Class *mc, Bool atall)
  {
    if (mc->isRemoved()) {
#ifdef BUG_TRACE
      printf("%d adding class %s %s %p\n", getpid(), mc->getName(), mc->getOid().toString(), mc);
#endif
      assert(!mc->isRemoved());
    }
    /*
      printf("adding class %s %s %p\n", mc->getName(), mc->getOid().toString(),
      mc);
    */
    if (!atall)
      {
	if (mc->getOid().isValid())
	  {
	    if (hash->get(mc->getOid()))
	      return Success;
	  }
	else if (_class->getPos(mc) >= 0)
	  return Success;
      }

    _class->insertObjectLast(mc);
    mc->gbx_locked = gbxTrue;

    if (mc->getOid().isValid())
      {
	hash->insert(mc->getOid(), mc);
	hash->insert(mc->getName(), mc);
      }

    touch();
    mc->sch = this;
    // added the 30/03/00
    mc->db = db;
    // ...

    return Success;
  }

  Status Schema::addClass(Class *mc)
  {
    assert(!mc->isRemoved());
    /*
      printf("Schema::addingClass(%s, %s)\n", mc->getName(),
      mc->getOid().toString());
    */
    if (mc->getOid().isValid())
      {
	if (hash->get(mc->getOid()))
	  return Success;
      }
    else if (_class->getPos(mc) >= 0)
      return Success;

    Class *omc;
    if ((omc = getClass(mc->name))) // was &&db : why??
      return Exception::make(IDB_SCHEMA_ERROR,
			     "duplicate class names in schema: '%s'",
			     mc->name);

    _class->insertObjectLast(mc);
    mc->gbx_locked = gbxTrue;
    if (mc->getOid().isValid())
      {
	hash->insert(mc->getOid(), mc);
	hash->insert(mc->getName(), mc);
      }

    touch();
    mc->sch = this;
    // added the 10/12/99
    //#ifndef OPTOPEN
    mc->attrsComplete();
    //#endif
    // ..

    // added the 30/03/00
    mc->db = db;
    // ...

    return Success;
  }

  Status Schema::suppressClass(Class *mc)
  {
    if (!mc)
      return Success;

    /*
      printf("schema(this=%p) suppress %p: \"%s\" [%s]\n", this, mc, mc->getName(),
      mc->getOid().getString());
    */

    if (_class->deleteObject(mc) >= 0)
      mc->gbx_locked = gbxFalse;

    if (mc->getOid().isValid())
      {
	hash->suppress(mc->getOid());
	hash->suppress(mc->getName());
      }

    touch();
    if (mc->sch == this)
      mc->sch = NULL;
    return Success;
  }

  void Schema::setReversal(Bool on_off)
  {
    reversal = on_off;
  }

  Bool Schema::isReversalSet() const
  {
    return reversal;
  }

  void Schema::revert(Bool rev)
  {
    if (!reversal)
      return;

    LinkedListCursor c(_class);
    Class *cl;

    while (c.getNext((void *&)cl))
      cl->revert(rev);

    reversal = False;
  }

  void Schema::purge()
  {
    Class **tosup = new Class *[_class->getCount()];
    int tosup_cnt = 0;
    LinkedListCursor *c = _class->startScan();
    Class *cl;

    while (_class->getNextObject(c, (void *&)cl))
      {
	Bool found;
	const Oid &xoid = cl->getOid();
	if (xoid.isValid())
	  {
	    Status status = db->containsObject(xoid, found);
	    if ((status && status->getStatus() == eyedbsm::INVALID_OID) || !found)
	      tosup[tosup_cnt++] = cl;
	  }
      }

    for (int i = 0; i < tosup_cnt; i++)
      suppressClass(tosup[i]);
  
    delete[] tosup;
    modify = False;
  }

  // added the 21/05/99

  void
  Schema::postComplete()
  {
    Class *cl;
    LinkedListCursor c(_class);
    while (c.getNext((void *&)cl))
      {
	ObjectPeer::setClass(cl, getClass(cl->getClass()->getName()));

	if (cl->getParent())
	  ClassPeer::setParent(cl, getClass(cl->getParent()->getName()));
      }
  }

  Status
  Schema::complete(Bool _setup, Bool force)
  {
    Status status;
    Class *cl;

    computeHashTable();

    LinkedListCursor c(_class);

    while (c.getNext((void *&)cl)) {
      assert(!cl->isRemoved());
      if (status = cl->attrsComplete())
	return status;
    }

    postComplete();

    if (!_setup)
      return Success;

    status = setup(force);
    if (status) return status;

    /*
      if (db->isBackEnd() && (db->getOpenFlag() & _DBRW))
      return completeIndexes();
    */

    return Success;
  }

  Status Schema::setup(Bool force)
  {
    Status status;
    Class *cl;
    LinkedListCursor c(_class);

    while (c.getNext((void *&)cl)) {
      assert(!cl->isRemoved());
      if (status = cl->setup(force))
	return status;
    }

    return Success;
  }

  Status Schema::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
  {
    fprintf(fd, "%s schema = ", oid.getString());
    return trace_realize(fd, INDENT_INC, flags, rcm);
  }

  Status Schema::setValue(Data)
  {
    return Success;
  }

  Status Schema::getValue(Data*) const
  {
    return Success;
  }

  Status
  Schema::trace_realize(FILE *fd, int indent,
			unsigned int flags,
			const RecMode *rcm) const
  {
    LinkedListCursor *c = _class->startScan();
    char *indent_str = make_indent(indent);
    void *o;

    if (state & Tracing)
      {
	fprintf(fd, "%s%s;\n", indent_str, oid.getString());
	delete_indent(indent_str);
	return Success;
      }

    Status s = Success;
    char *lastindent_str = make_indent(indent-INDENT_INC);
    const_cast<Schema *>(this)->state |= Tracing;

    fprintf(fd, "{\n");

    fprintf(fd, "%sname = \"%s\";\n", indent_str, name);
    while (_class->getNextObject(c, o))
      {
	Class *cl = (Class *)o;
	//      fprintf(fd, "%s%s = ", indent_str, cl->getOid().getString());
	s = ObjectPeer::trace_realize(cl, fd, indent + INDENT_INC, flags, rcm);
      }

    _class->endScan(c);

    fprintf(fd, "%s};\n", lastindent_str);
    delete_indent(lastindent_str);
    delete_indent(indent_str);
    const_cast<Schema *>(this)->state &= ~Tracing;

    return s;
  }

  Status Schema::create(void)
  {
    return Success;
  }

  Status
  Schema::deferredCollRegisterRealize(DbHandle *dbh)
  {
    Status status = Success;

    if (!deferred_list)
      return Success;

    LinkedListCursor *c = deferred_list->startScan();

    deferredLink *d;

    while (deferred_list->getNextObject(c, (void *&)d))
      {
	Class *cl = getClass(d->clname);

	if (!cl)
	  {
	    status = Exception::make(IDB_ERROR, "class '%s' not found\n", d->clname);
	    goto out;
	  }

	Collection *extent;
	if (cl->isPartiallyLoaded())
	  {
	    status = cl->wholeComplete();
	    if (status) return status;
	  }

	status = cl->getExtent(extent);
	if (status) return status;

	if (!extent)
	  {
	    status = Exception::make(IDB_ERROR, "extent not found for class '%s'", d->clname);
	    goto out;
	  }

	const eyedbsm::Oid *colloid = extent->getOid().getOid();
	IDB_collClassRegister(dbh, colloid, &d->oid, True);
	delete d;
      }

  out:
    deferred_list->endScan(c);
    delete deferred_list;
    deferred_list = 0;
    return status;
  }

  void
  Schema::deferredCollRegister(const char *clname,
			       const eyedbsm::Oid *_oid)
  {
    if (!deferred_list)
      deferred_list = new LinkedList;

    deferredLink *d = new deferredLink(clname, _oid);
    deferred_list->insertObject(d);
  }

#define builtin_make(cls, type) \
cls = new type; \
addClass_nocheck(cls); \
cls->setAttributes((Attribute **)class_info[Basic_Type].items, \
		      class_info[Basic_Type].items_cnt); \
		      ClassPeer::setMType(cls, Class::System)

  void
  Schema::class_make(Class **cl, int _type,
		     Class *_parent)
  {
    *cl = new Class(class_info[_type].name, _parent);

    (*cl)->setAttributes((Attribute **)class_info[_type].items, class_info[_type].items_cnt);

    ClassPeer::setMType(*cl, Class::System);

    addClass_nocheck(*cl);
  }

  void
  Schema::bool_class_make(Class **cl)
  {
    *cl = Class::makeBoolClass();
    addClass_nocheck(*cl);
  }

  Status Schema::init(Database *database, Bool _create)
  {
    Status status = Success;

    db = database;

    Class_Class = 0;

    class_make(&Object_Class, Object_Type, (Class *)0);

    class_make(&Class_Class, Class_Type, Object_Class);
    Object_Class->setClass(Class_Class);
    Object_Class->parent = (Class *)0;

    class_make(&BasicClass_Class, BasicClass_Type, Class_Class);

    class_make(&EnumClass_Class, EnumClass_Type, Class_Class);

    class_make(&AgregatClass_Class, AgregatClass_Type, Class_Class);

    class_make(&StructClass_Class, StructClass_Type, AgregatClass_Class);

    class_make(&UnionClass_Class, UnionClass_Type, AgregatClass_Class);


    class_make(&Instance_Class, Instance_Type, Object_Class);

    class_make(&Basic_Class, Basic_Type, Instance_Class);

    class_make(&Enum_Class, Enum_Type, Instance_Class);

    class_make(&Agregat_Class, Agregat_Type, Instance_Class);

    class_make(&Struct_Class, Struct_Type, Agregat_Class);

    class_make(&Union_Class, Union_Type, Agregat_Class);

    class_make(&Schema_Class, Schema_Type, Instance_Class);

    bool_class_make(&Bool_Class);

    class_make(&CollectionClass_Class, CollectionClass_Type, Class_Class);

    class_make(&CollBagClass_Class, CollBagClass_Type, CollectionClass_Class);

    class_make(&CollSetClass_Class, CollSetClass_Type, CollectionClass_Class);

    class_make(&CollListClass_Class, CollListClass_Type, CollectionClass_Class);

    class_make(&CollArrayClass_Class, CollArrayClass_Type, CollectionClass_Class);


    class_make(&Collection_Class, Collection_Type, Instance_Class);

    class_make(&CollBag_Class, CollBag_Type, Collection_Class);

    class_make(&CollSet_Class, CollSet_Type, Collection_Class);

    class_make(&CollList_Class, CollList_Type, Collection_Class);

    class_make(&CollArray_Class, CollArray_Type, Collection_Class);

    builtin_make(Char_Class, CharClass);
    builtin_make(Byte_Class, ByteClass);
    builtin_make(OidP_Class, OidClass);
    builtin_make(Int16_Class, Int16Class);
    builtin_make(Int32_Class, Int32Class);
    builtin_make(Int64_Class, Int64Class);
    builtin_make(Float_Class, FloatClass);

    if (db && _create)
      {
	Database *odb = default_db;
	default_db = db;
	status = syscls::updateSchema(db);
	if (!status) {
	  db->transactionBegin();
	  status = post_etc_update(db);
	  db->transactionCommit();
	  if (status)
	    {
	      default_db = odb;
	      return status;
	    }

	  if (db->getName() &&
	      !strcmp(db->getName(), DBM_Database::getDbName()))
	    status = DBM_Database::updateSchema(db);

	  if (!status && !db->isBackEnd())
	    {
	      db->transactionBegin();
	      status = realize();
	      db->transactionCommit();
	    }

	  if (!status)
	    status = oqlctb::updateSchema(db);

	  if (!status)
	    status = utils::updateSchema(db);
	}

	default_db = odb;
	return status;
      }

    return status;
  }

  Status Schema::remove(const RecMode*)
  {
    return Success;
  }

  Status Schema::realize(const RecMode*)
  {
    //printf("Schema::realize(%s, state %p)\n", oid.toString(), state);

    if (state & Realizing)
      return Success;

    state |= Realizing;

    LinkedListCursor *c = _class->startScan();
    Status status;

    Class *cl;

    while (_class->getNextObject(c, (void *&)cl)) {
      assert(!cl->isRemoved());
      if (status = cl->setDatabase(db))
	{
	  _class->endScan(c);
	  state &= ~Realizing;
	  return status;
	}
    }

    _class->endScan(c);

    ClassPeer::setMType(CollSet_Class, Class::System);
    status = CollSet_Class->realize();

    if (status)
      {
	state &= ~Realizing;
	return status;
      }

    status = Class_Class->realize();

    if (status)
      {
	state &= ~Realizing;
	return status;
      }

    Class **mtmp = (Class **)malloc(_class->getCount() *
				    sizeof(Class *));

    int n = 0;
    c = _class->startScan();
    while (_class->getNextObject(c, (void* &)cl))
      mtmp[n++] = cl;
    _class->endScan(c);

    int mod_cnt = 0;
    int i;
    for (i = 0; i < n ; i++)
      {
	cl = mtmp[i];
	mod_cnt += (cl->isModify() ? 1 : 0);

	status = cl->realize();

	if (status)
	  {
	    free(mtmp);
	    state &= ~Realizing;
	    return status;
	  }
      }

    if ((status = complete(mod_cnt ? True : False)) ||
	(status = StatusMake(schemaComplete(db->getDbHandle(), name))))
      {
	free(mtmp);
	state &= ~Realizing;
	return status;
      }

    for (i = 0; i < n ; i++) {
      status = mtmp[i]->postCreate();
      if (status) {
	free(mtmp);
	state &= ~Realizing;
	return status;
      }
    }

    for (i = 0; i < n ; i++) {
      status = mtmp[i]->createComps();
      if (status) {
	free(mtmp);
	state &= ~Realizing;
	return status;
      }
    }

    free(mtmp);

    state &= ~Realizing;

    return status;
  }

  Status Schema::storeName()
  {
    return StatusMake(schemaComplete(db->getDbHandle(), name));
  }

  Status Schema::update(void)
  {
    return realize();
  }

  Status
  schemaClassMake(Database *db, const Oid *oid, Object **o,
		      const RecMode *rcm, const ObjectHeader *hdr,
		      Data idr, LockMode lockmode, const Class*)
  {
    Schema *sch = new Schema();
    RPCStatus rpc_status;
    Data temp;

    temp = (unsigned char *)malloc(hdr->size);

    if (!idr)
      {
	object_header_code_head(temp, hdr);
	rpc_status = objectRead(db->getDbHandle(), (Data)temp, 0, 0,
				    oid->getOid(), 0, lockmode, 0);
      }
    else
      {
	memcpy(temp, idr, hdr->size);
	rpc_status = RPCSuccess;
      }

    if (rpc_status == RPCSuccess)
      {
	eyedblib::int32 cnt;
	Offset offset = IDB_SCH_CNT_INDEX;
	int32_decode(temp, &offset, &cnt);

	sch->init(db, False);

	Schema *o_sch = db->getSchema();
	db->setSchema(sch);

	char *s;
	offset = IDB_SCH_NAME_INDEX;
	string_decode(temp, &offset, &s);

	sch->setName(s);

	for (int i = 0; i < cnt; i++) {
	  Oid toid;
	  Status status;
	  Class *cl;
	  Offset offset = IDB_SCH_OID_INDEX(i);
	
	  oid_decode(temp, &offset, toid.getOid());
	
	  if (toid.isValid()) {
	    char *name = 0;
	    eyedblib::int32 hdr_type;
	    int32_decode(temp, &offset, &hdr_type);
	    status = class_name_decode(db->getDbHandle(), temp,
					   &offset, &name);
	    if (status) return status;
	    Bool newClass;
	    status = Class::makeClass(db, toid, hdr_type, name,
				      newClass, cl);
	    free(name);
	    if (status) return status;
	    if (newClass)
	      sch->addClass_nocheck(cl, True);
	  }
	}

	ObjectPeer::setModify(sch, False);
	db->setSchema(o_sch);
	*o = sch;
	ObjectPeer::setClass(sch, sch->getClass("schema"));
      }
    else
      sch->release();

    free(temp);
    return StatusMake(rpc_status);
  }

  void
  Schema::setName(const char *_name)
  {
    if (name == _name)
      return;

    free(name);
    name = strdup(_name);
  }

  void Schema::garbage()
  {
    //printf("Schema::garbage(this=%p, count=%d)\n", this, _class->getCount());
    dont_delete_comps = True;

    Class *cl;
    LinkedListCursor c_cls(_class);

    ClassComponent *comp;
    LinkedList tmpcomplist;

    while (c_cls.getNext((void *&)cl))
      {
	cl->lock(); // prevents deleting while recursive component deletion
	LinkedList *complist = cl->complist;
	if (complist)
	  {
	    LinkedListCursor cx(complist);
	    while (cx.getNext((void *&)comp))
	      {
		if (tmpcomplist.getPos(comp) < 0)
		  tmpcomplist.insertObjectLast(comp);
	      }

	    delete complist;
	  }

	cl->complist = NULL;
      }

    LinkedListCursor c_comp(tmpcomplist);
  
    while (c_comp.getNext((void *&)comp)) {
      comp->unlock_refcnt();
      comp->release();
    }

    c_cls.restart();

    while (c_cls.getNext((void *&)cl)) {
      cl->pre_release();
    }

    c_cls.restart();

    while (c_cls.getNext((void *&)cl)) {
      cl->unlock_refcnt();
      cl->release();
    }

    delete _class;
    delete hash;

    free(classes);
    free(name);
    Instance::garbage();
  }

  Schema::~Schema()
  {
    garbageRealize();
  }

  Status
  Schema::manageClassDeferred(Class *cl)
  {
#ifdef OPTOPEN_TRACE
    printf("class %s is deferred %s %p\n", cl->getName(), cl->getOid().getString(),
	   cl);
#endif
#ifdef BUG_TRACE
    printf("isRemoved %d %p %s\n", cl->isRemoved(), cl, cl->getName());
#endif
    cl->setPartiallyLoaded(False);

    Class *clx;
    Status s = db->loadObject(cl->getOid(), (Object *&)clx);
    if (s) return s;

#ifdef BUG_TRACE
    printf("isRemoved2 %d %p %s\n", clx->isRemoved(), clx, clx->getName());
#endif
#ifdef OPTOPEN_TRACE
    printf("clx %s -> %d [clx => %p == %p?\n",
	   clx->getName(), clx->getAttributesCount(), clx, cl);
#endif
    if (cl != clx) {
      s = cl->loadComplete(clx);
      if (s) return s;
      s = cl->attrsComplete();
      if (s) return s;
      clx->release();
    }

#ifdef OPTOPEN_TRACE
    printf("Returning %p %s cnt=%d\n",
	   cl, cl->getName(), cl->getAttributesCount());
#endif
    return Success;
  }

  Status
  Schema::checkDuplicates()
  {
    std::string s;

    LinkedListCursor c(_class);
    Class *cls;
    while (c.getNext((void *&)cls)) {
      //printf("class=%p\n", cls);
      LinkedListCursor xc(_class);
      Class *xcls;
      while (xc.getNext((void *&)xcls)) {
	//printf("%p vs. %p '%s' vs '%s'\n", xcls, cls, cls->getName(), xcls->getName());
	if (xcls != cls && !strcmp(cls->getName(), xcls->getName())) {
	  s += std::string("duplicate: ") + str_convert((long)cls, "%p") +
	    " " + str_convert((long)xcls, "%p") + " " + cls->getName() + "\n";
	}
      }
    }

    printf("checking schema duplicates -> %s\n", s.c_str());
    if (s.size())
      return Exception::make(IDB_ERROR, s.c_str());

    return Success;
  }

  Bool
  Schema::checkClass(const Class *cl)
  {
    LinkedListCursor c(_class);
    Class *cls;
    while (c.getNext((void *&)cls)) {
      if (cls == cl)
	return True;
    }

    return False;
  }

  Status
  Schema::clean(Database *db)
  {
    LinkedListCursor c(_class);
    Class *cls;
    while (c.getNext((void *&)cls)) {
      Status s = cls->clean(db);
      if (s) return s;
    }

    return Success;
  }

  Class *Schema::getClass(const Oid &poid, Bool perform_load)
  {
    if (!poid.isValid())
      return (Class *)0;

    Class *cl;

    assert(hash);
    if (hash)
      {
	cl = hash->get(poid);
	if (cl)
	  {
	    if (cl->isPartiallyLoaded())
	      {
		Status s = manageClassDeferred(cl);
		if (s) throw *s;
	      }
	    return cl;
	  }
      }

    if (!hash || !cl)
      {
	LinkedListCursor *c = _class->startScan();

	while (_class->getNextObject(c, (void* &)cl))
	  {
	    Oid moid = cl->getOid();
	  
	    if (moid.compare(poid))
	      {
		_class->endScan(c);
		return cl;
	      }
	  }

	_class->endScan(c);
      }


    if (perform_load)
      {
	Status status;
	Object *o;
	status = db->loadObject(&poid, &o);
	if (status == Success)
	  {
	    if (!o->asClass())
	      {
		o->release();
		return 0;
	      }

	    cl = (Class *)o;

	    Class *tcl;
	    if (tcl = getClass(cl->getName()))
	      {
		if (cl != tcl)
		  cl->release();
		return tcl;
	      }

	    addClass_nocheck(cl);
	    cl->attrsComplete();

	    return cl;
	  }
	else
	  throw *status;
      }

    return 0;
  }

  Class *Schema::getClass(const char *n)
  {
    Class *cl;

    assert(hash);
    if (hash)
      {
	cl = hash->get(n);
	if (cl)
	  {
	    if (cl->isPartiallyLoaded())
	      {
		Status s = manageClassDeferred(cl);
		if (s) throw *s;
	      }
	    return cl;
	  }
      }

    if (!hash || !cl)
      {
	LinkedListCursor c(_class);

	while (c.getNext((void *&)cl))
	  if (!strcmp(cl->getName(), n) || !strcmp(cl->getAliasName(), n))
	    return cl;
      }

    return 0;
  }

  const LinkedList *Schema::getClassList(void) const
  {
    Class *cl;
    LinkedListCursor c(_class);
    while (c.getNext((void *&)cl)) {
      if (cl->isPartiallyLoaded()) {
	Status s = const_cast<Schema *>(this)->manageClassDeferred(cl);
	if (s) throw *s;
      }
    }
    return _class;
  }

  static void
  head_gen_C(FILE *fd, const char *files, const char *ext, const char *package,
	     const GenCodeHints &hints,
	     Bool donot_edit, const char *suffix = "",
	     Bool inclPack = False, Bool use_namespace = False,
	     const char *c_namespace = 0)
  {
    char file[256];

    sprintf(file, "%s%s%s", files, ext, suffix);
    fprintf(fd ,"\n/*\n");
    fprintf(fd, " * EyeDB Version %s Copyright (c) 1995-2006 SYSRA\n",
	    eyedb::getVersion());
    fprintf(fd, " *\n");
    fprintf(fd, " * File '%s'\n", file);
    fprintf(fd, " *\n");
    fprintf(fd, " * Package Name '%s'\n", package);
    fprintf(fd, " *\n");
    if (hints.gen_date) {
      time_t t;
      time(&t);
      fprintf(fd, " * Generated by eyedbodl at %s", ctime(&t));
    }
    else
      fprintf(fd, " * Generated by eyedbodl\n");

    fprintf(fd ," *\n");

    if (donot_edit)
      {
	fprintf(fd, " * ---------------------------------------------------\n");
	fprintf(fd, " * -------------- DO NOT EDIT THIS CODE --------------\n");
	fprintf(fd, " * ---------------------------------------------------\n");
	fprintf(fd ," *\n");
      }

    fprintf(fd, " */\n\n");

    fprintf(fd, "#include <eyedb/eyedb.h>\n\n");

    if (inclPack)
      fprintf(fd, "#include \"%s.h\"\n\n", files); // 22/08/01: was `package'
    // instead of files
    if (use_namespace && c_namespace)
      fprintf(fd, "using namespace %s;\n\n", c_namespace);
  }

  /*
  static void
  head_gen_Java(FILE *fd, const char *files, const char *ext,
		const char *package, Bool donot_edit = True)
  {
    char file[256];
    time_t t;
    time(&t);

    sprintf(file, "%s%s", files, ext);
    fprintf(fd ,"\n*\n");
    fprintf(fd, " * File '%s'\n", file);
    fprintf(fd, " *\n");
    fprintf(fd, " * Package Name '%s'\n", package);
    fprintf(fd, " *\n");
    fprintf(fd, " * Generated by eyedbodl at %s", ctime(&t));
    fprintf(fd ," *\n");

    if (donot_edit)
      {
	fprintf(fd, " * Do not edit this code\n");
	fprintf(fd ," *\n");
      }

    fprintf(fd, " *\n\n");

    fprintf(fd, "#include <eyedb/eyedb.h>\n\n");
  }
  */

  inline static void
  make_file(const char *dirname, const char *files, const char *ext, char file[],
	    const char *ext1)
  {
    sprintf(file, "%s/%s%s%s", dirname, files, ext, ext1);
  }

  static Status
  make_file(const char *dirname, const char *files, const char *ext, FILE **fd,
	    const char *ext1 = "", Bool check_if_exists = False)
  {
    static FILE *fdnull;

    char file[256];

    make_file(dirname, files, ext, file, ext1);

    if (check_if_exists) {
	struct stat s;
	if (stat(file, &s) >= 0) {
	  /*
	  fprintf(stderr, "Notice: file %s exists, not overwrite it\n",
		  file);
	  */
	  /*if (!fdnull)
	    fdnull = fopen("/dev/null", "rw");
	  *fd = fdnull;
	  */
	  *fd = 0;
	  return Success;
	}
    }

    *fd = fopen(file, "w");

    if (!*fd)
      return Exception::make(IDB_GENERATION_CODE_ERROR, "cannot open file '%s' for writing", file);

    return Success;
  }

  static Status
  remove_file(const char *dirname, const char *files, const char *ext,
	      const char *ext1 = "")
  {
    char file[256];

    make_file(dirname, files, ext, file, ext1);
    unlink(file);
    return Success;
  }

  static void
  skel_comment(FILE *fd, const char *files, const char *febe, Bool backend)
  {
    fprintf(fd,
	    "// To implement and use user methods, perform the following operations\n"
	    "/*\n"
	    "\n"
	    "#1. Copy the skeleton file\n"
	    "cp %s%s-skel.cc %s%s.cc\n"
	    "\n"
	    "#2. Implement the user methods in %s%s.cc using a text editor\n"
	    "\n"
	    //	    "#3. Compile the shared library using GNU make\n"
	    //	    "make -f Makefile.%s all\n",
	    "#3. Compile the shared library\n",
	    files, febe, files, febe,
	    files, febe,
	    files);

    const char *s = Executable::getSOTag();
    fprintf(fd,
	    "\n"
	    "#4. Copy the shared library to the eyedb loadable library directory\n"
	    "cp %s%s%s.so <eyedbinstalldir>/lib/eyedb\n"
	    "\n"
	    "#5. Change the file access mode\n"
	    "chmod a+r <eyedbinstalldir>/lib/eyedb/%s%s%s.so\n",
	    files, febe, s,
	    files, febe, s);

    fprintf(fd, "\n*/\n\n");
  }

  static Status
  make_files(const char *dirname, const char *files, const char *package,
	     const char *c_suffix, const char *h_suffix,
	     FILE **pfdh, FILE **pfdc,
	     FILE **pfdstubsfe, FILE **pfdstubsbe,
	     FILE **pfdmthfe, FILE **pfdmthbe, FILE **pfdmk,
	     FILE **pfdtempl)
  {
    Status status;

    if (status = make_file(dirname, files, h_suffix, pfdh))
      return status;
  
    if (status = make_file(dirname, files, c_suffix, pfdc))
      return status;

    if (status = make_file(dirname, files, "stubsfe", pfdstubsfe, c_suffix))
      return status;

    if (status = make_file(dirname, files, "stubsbe", pfdstubsbe, c_suffix))
      return status;

    if (status = make_file(dirname, files, "mthfe-skel", pfdmthfe, c_suffix))
      return status;

    if (status = make_file(dirname, files, "mthbe-skel", pfdmthbe, c_suffix))
      return status;

    if (status = make_file(dirname, "Makefile.", package, pfdmk,
			   "", True))
      return status;

    if (status = make_file(dirname, "template_", package, pfdtempl,
			   c_suffix, True))
      return status;

    return Success;
  }

  static Status
  remove_files(const char *dirname, const char *files, const char *package,
	       const char *c_suffix, const char *h_suffix)
  {
    Status status;

    if (status = remove_file(dirname, files, h_suffix))
      return status;
  
    if (status = remove_file(dirname, files, "stubsbe", c_suffix))
      return status;
  
    if (status = remove_file(dirname, files, "stubsfe", c_suffix))
      return status;
  
    if (status = remove_file(dirname, files, c_suffix, c_suffix))
      return status;

    if (status = remove_file(dirname, files, "init"))
      return status;

    if (status = remove_file(dirname, "Makefile.", package))
      return status;

    if (status = remove_file(dirname, "template_", package, c_suffix))
      return status;

    return Success;
  }

  void
  Schema::genODL(FILE *fd, unsigned int flags) const
  {
    ((Schema *)this)->sort_classes();

    if (name && *name)
      {
	fprintf(fd ,"\n//\n");
	fprintf(fd, "// EyeDB Version %s Copyright (c) 1995-2007 SYSRA\n",
		eyedb::getVersion());
	fprintf(fd, "//\n");
	fprintf(fd, "// %s Schema\n", name);
	fprintf(fd, "//\n");
	time_t t;
	time(&t);
	fprintf(fd, "// Generated by eyedbodl at %s", ctime(&t));
	fprintf(fd, "//\n\n");
	/*
	fprintf(fd, "#if defined(EYEDBNUMVERSION) && EYEDBNUMVERSION != %d\n", eyedb::getVersionNumber());
	fprintf(fd, "#error \"This file is being compiled with a version of eyedb different from that used to create it (%s)\"\n", eyedb::getVersion());
	fprintf(fd, "#endif\n\n");
	*/
      }

    LinkedListCursor c(_class);
    Class *cl;
    int r = 0;

    if (db && db->isOpened()) db->transactionBegin();
    while(c.getNext((void *&)cl)) {
      if (r) fprintf(fd, "\n");
      r = cl->genODL(fd, (Schema *)this);
    }

    if (db && db->isOpened()) db->transactionAbort();
  }

  const char *
  Schema::generateStubs_C(Bool genstubs, Class *cl,
			  const char *dirname, const char *package,
			  const GenCodeHints &hints)
  {
    if (genstubs && cl->asAgregatClass()) {
      static char file[256];
      sprintf(file, "%s/%s_stubs.h", dirname, cl->getName());
      const char *pfile = strchr(file, '/') + 1;
      FILE *fd = fopen(file, "r");
      if (fd) {
	fclose(fd);
	return pfile;
      }
      
      fd = fopen(file, "w");

      if (fd) {
	head_gen_C(fd, pfile, "", package, hints, False);
	fclose(fd);
      }

      return pfile;
    }
    
    return (char *)0;
  }

  static const char *
  cap(const char *s, const char *prefix)
  {
    static char *str;
    static int len;
    int l;
    int ll = strlen(prefix);
    char c;

    l = strlen(s) - ll;

    if (l >= len)
      {
	str = (char *)realloc(str, l+1);
	len = l;
      }

    c = s[ll];
    strcpy(str, &s[ll]);
    str[0] = ((c >= 'a' && c <= 'z') ? (c + 'A' - 'a') : c);
    return str;
  }

  static void *InitSort = (void *)0;
  static void *InsertedSort = (void *)1;
  static void *ProcessingSort = (void *)2;

  void
  Schema::sort_realize(const Class *cl, LinkedList *l)
  {
    if (cl->getUserData() != InitSort)
      return;

    const_cast<Class *>(cl)->setUserData(ProcessingSort);

    const Class **tmp = new const Class*[_class->getCount()];

    int i, j;
    for (i = 0; cl; i++)
      {
	tmp[i] = cl;
	cl = cl->getParent();
      }

    for (j = i-1; j >= 0; j--)
      if (tmp[j]->getUserData() != InsertedSort)
	{
	  unsigned int attr_cnt;
	  const Attribute **attr = tmp[j]->getAttributes(attr_cnt);
	  for (int k = 0; k < attr_cnt; k++)
	    if (!attr[k]->isIndirect() &&
		attr[k]->getClass()->asAgregatClass())
	      sort_realize(attr[k]->getClass(), l);

	  l->insertObjectLast((void *)tmp[j]);
	  const_cast<Class *>(tmp[j])->setUserData(InsertedSort);
	}

    delete[] tmp;
  }

  void
  Schema::sort_classes()
  {
    (void)getClassList(); // to force schema completion
    LinkedList *l = new LinkedList();

    LinkedListCursor c(_class);
    Class *cl;
    int n;
    for (n = 0; c.getNext((void *&)cl); n++)
      {
	cl->setUserData(InitSort);
	if (cl->asEnumClass())
	  {
	    l->insertObjectLast(cl);
	    cl->setUserData(InsertedSort);
	  }
      }

    LinkedListCursor cx(_class);
    for (n = 0; cx.getNext((void *&)cl); n++)
      if (!cl->asEnumClass())
	sort_realize(cl, l);

    delete _class;
    _class = l;
  }

  Status
  Schema::generateCode(ProgLang lang,
		       const char *package, const char *schname,
		       const char *c_namespace,
		       const char *prefix,
		       const char *db_prefix,
		       const GenCodeHints &hints,
		       Bool _export,
		       Class *superclass,
		       LinkedList *qseq_list)
  {
    sort_classes();

    if (lang == ProgLang_C)
      return generateCode_C(package, schname, c_namespace, prefix, db_prefix,
			    hints, _export, superclass, qseq_list);

    if (lang == ProgLang_Java)
      return generateCode_Java(package, schname, prefix, db_prefix,
			       hints, _export, superclass,
			       qseq_list);

    return Exception::make(IDB_ERROR, "unknown language: %d", lang);
  }

  static Status
  make_java_file(const char *dirname, const char *package,
		 const char *prefix, const char *name, 
		 const GenCodeHints &hints, FILE *&fd)
  {
    char file[256];
    sprintf(file, "%s/%s%s.java", dirname, prefix, name);

    fd = fopen(file, "w");

    if (!fd)
      return Exception::make(IDB_ERROR, "cannot create file '%s'", file);

    fprintf(fd, "\n");
    fprintf(fd, "//\n");
    fprintf(fd, "// class %s%s\n", prefix, name);
    fprintf(fd, "//\n");
    fprintf(fd, "// package %s\n", package);
    fprintf(fd, "//\n");
    if (hints.gen_date) {
      time_t t;
      time(&t);
      fprintf(fd, "// Generated by eyedbodl at %s", ctime(&t));
    }
    else
      fprintf(fd, "// Generated by eyedbodl\n");

    fprintf(fd, "//\n\n");

    fprintf(fd, "package %s;\n\n", package);
    // (fd): commented out next line for compilation with gcj (org.eyedb.Object conflicts with java.lang.Object)
    //    fprintf(fd, "import org.eyedb.*;\n"); 
    fprintf(fd, "import org.eyedb.utils.*;\n");
    fprintf(fd, "import org.eyedb.syscls.*;\n\n");

    return Success;
  }

  static Bool
  check_class(const Class *cl, Bool pass)
  {
    if (cl->getUserData(odlGENCODE))
      return True;

    if (pass && cl->getUserData(odlGENCOMP))
      return True;

    return False;
  }

  static Status
  check_dir(const char *dirname)
  {
    struct stat st;
    if (stat(dirname, &st) < 0)
      {
	if (mkdir(dirname, 0777) < 0)
	  return Exception::make(IDB_ERROR, "cannot create directory %s",
				 dirname);
      }

    return Success;
  }


  Status
  Schema::generateCode_Java(const char *package, const char *schname,
			    const char *prefix,
			    const char *db_prefix,
			    const GenCodeHints &hints,
			    Bool _export,
			    Class *superclass,
			    LinkedList *)
  {
    Status status = Success;
    FILE *fd;

    if (status = check_dir(hints.dirname))
      return status;

    if (status = make_java_file(hints.dirname, package, prefix, "Database", hints, fd))
      return status;

    GenContext ctx(fd, package, odl_rootclass);
    ctx.push();

    fprintf(fd, "public class %sDatabase extends org.eyedb.Database {\n\n", prefix);

    fprintf(fd, "  public %sDatabase(String name) {super(name);}\n\n", prefix);

    fprintf(fd, "  public %sDatabase(String name, String dbmfile) {super(name, dbmfile);}\n\n", prefix);

    fprintf(fd, "  public %sDatabase(int dbid) {super(dbid);}\n\n", prefix);

    fprintf(fd, "  public %sDatabase(int dbid, String dbmfile) {super(dbid, dbmfile);}\n\n", prefix);

    fprintf(fd,
	    "  public void open(org.eyedb.Connection conn, int flags, String userauth, "
	    "String passwdauth) throws org.eyedb.Exception\n");
    fprintf(fd, "  {\n");
    fprintf(fd, "    super.open(conn, flags, userauth, passwdauth);\n\n");
    fprintf(fd, "    checkSchema(getSchema());\n");
    fprintf(fd, "  }\n\n");

    fprintf(fd, "  public org.eyedb.Object loadObjectRealize(org.eyedb.Oid oid, "
	    "int lockmode, org.eyedb.RecMode rcm)\n  throws org.eyedb.Exception\n");

    fprintf(fd, "  {\n");
    fprintf(fd, "    org.eyedb.Object o = super.loadObjectRealize(oid, lockmode, rcm);\n");
    fprintf(fd, "    org.eyedb.Object ro = makeObject(o, true);\n");
    fprintf(fd, "    if (ro != null) o = ro;\n");
    fprintf(fd, "    return o;\n");
    fprintf(fd, "  }\n\n");

    fprintf(fd, "  private void checkSchema(org.eyedb.Schema m) throws org.eyedb.Exception {\n");
    // TBD
    LinkedListCursor c(_class);
    fprintf(fd, "    org.eyedb.Class cl;\n");
    fprintf(fd, "    String msg = \"\";\n\n");
    Class *cl;

    while (c.getNext((void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass()) {
	if (cl->isRootClass())
	  continue;
	fprintf(fd, "    if ((cl = m.getClass(\"%s\")) == null)\n",
		cl->getAliasName());
	fprintf(fd, "      msg += \"class '%s' does not exist\\n\";\n",
		cl->getAliasName());
	fprintf(fd, "    else if (!%s.idbclass.compare(cl))\n", cl->getCName());
	fprintf(fd, "      msg += \"class '%s' differs in database and in runtime environment\\n\";\n",
		cl->getAliasName());
      }

    fprintf(fd, "    if (!msg.equals(\"\")) throw new org.eyedb.Exception(new org.eyedb.Status(org.eyedb.Status.IDB_ERROR, msg));\n");
    fprintf(fd, "  }\n\n");

    fprintf(fd, "  static public org.eyedb.Object makeObject(org.eyedb.Object o, boolean share)\n");
    fprintf(fd, "  throws org.eyedb.Exception {\n\n");
    fprintf(fd, "    if (o == null || o.getClass(true) == null) return o;\n\n");
    fprintf(fd, "    if (o.isGRTObject()) return o;\n\n");
    //#define Java_DEBUG

    LinkedListCursor *curs;
#ifdef Java_DEBUG
    curs = _class->startScan();

    ctx.push();
    fprintf(fd, "%sString xname = o.getClass(true).getName();\n",
	    ctx.get());
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && cl->asAgregatClass()) {
	fprintf(fd, "%sif (xname.equals(\"%s\"))\n", ctx.get(),
		cl->getAliasName());
	fprintf(fd, "%s  return new %s((org.eyedb.Struct)o, share);\n",
		ctx.get(), cl->getCName());
      }

    fprintf(fd, "%sreturn o;\n", ctx.get());
    ctx.pop();

#else
    fprintf(fd, "    try {\n");
    fprintf(fd, "      java.lang.reflect.Constructor cons = (java.lang.reflect.Constructor)hash.get(o.getClass(true).getName());\n");
    fprintf(fd, "      if (cons == null) return o;\n\n");
    fprintf(fd, "      java.lang.Object[] tmp = new java.lang.Object[2]; tmp[0] = o; tmp[1] = new java.lang.Boolean(share);\n");
    fprintf(fd, "      return (org.eyedb.Object)cons.newInstance(tmp);\n");
    fprintf(fd, "    } catch(java.lang.Exception e) {\n");
    fprintf(fd, "      System.err.println(\"caught \" + e + \" in database\");\n");
    fprintf(fd, "      System.exit(2);\n");
    fprintf(fd, "      return null;\n");
    fprintf(fd, "    }\n");

#endif

    fprintf(fd, "  }\n\n");

    fprintf(fd, "  static java.util.Hashtable hash = new java.util.Hashtable(256);\n");
    fprintf(fd, "  static protected java.lang.Class[] clazz;\n");
    fprintf(fd, "  static {\n");
    fprintf(fd, "    clazz = new java.lang.Class[2];\n");
    fprintf(fd, "    clazz[0] = org.eyedb.Struct.class;\n");
    fprintf(fd, "    clazz[1] = boolean.class;\n");
    fprintf(fd, "  }\n\n");

    fprintf(fd, "  public static void init()\n throws org.eyedb.Exception {\n");

    ctx.push();

    curs = _class->startScan();

    while (_class->getNextObject(curs, (void *&)cl))
      if (check_class(cl, False))
	fprintf(fd, "%s%s.init_p();\n", ctx.get(), cl->getCName());

    ctx.pop();
    _class->endScan(curs);

    ctx.push();
    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass())
	fprintf(fd, "%s%s.init();\n", ctx.get(), cl->getCName());

    ctx.pop();
    _class->endScan(curs);

    fprintf(fd, "  }\n");

    fprintf(fd, "}\n\n");

    fclose(fd);

    curs = _class->startScan();

    while (_class->getNextObject(curs, (void *&)cl))
      {
	if (check_class(cl, False)) {
	  if (status = make_java_file(hints.dirname, package, "",
				      cl->getCName(), hints, fd))
	    return status;
	
	  status = cl->generateCode_Java(this, prefix, hints, fd);
	  if (status) {
	    _class->endScan(curs);
	    goto out;
	  }
	
	  fclose(fd);
	}
      }

    _class->endScan(curs);

  out:
    return status;
  }

  static void
  pack_init(FILE *fd, const char *package)
  {
    fprintf(fd, "static Bool __%s_init = False;\n\n", package);
    //  fprintf(fd, "static Database *__%s_db = 0;\n\n", package);
    fprintf(fd, "#define _packageInit(DB) \\\n \\\n");
    fprintf(fd, "  if (!__%s_init) { \\\n", package);
    fprintf(fd, "    %s::init(); \\\n", package);
    fprintf(fd, "    __%s_init = True; \\\n", package);
    fprintf(fd, "  } \\\n \\\n");
    fprintf(fd, "  if (!(DB)->getUserData(\"eyedb:%s\")) { \\\n", package);
    fprintf(fd, "     Status s = %sDatabase::checkSchema((DB)->getSchema()); \\\n", package);
    fprintf(fd, "     if (s) return s; \\\n");
    fprintf(fd, "     %sDatabase::setConsApp(DB); \\\n", package);
    fprintf(fd, "     (DB)->setUserData(\"eyedb:%s\", (void *)1); \\\n", package);
    fprintf(fd, "  }\n\n");
  }

  Status
  Schema::generateCode_C(const char *package, const char *schname,
			 const char *c_namespace,
			 const char *prefix,
			 const char *db_prefix,
			 const GenCodeHints &hints,
			 Bool _export,
			 Class *superclass,
			 LinkedList *qseq_list)
  {
    FILE *fdh, *fdc, *fdstubsfe, *fdstubsbe, *fdmthfe, *fdmthbe, *fdmk,
      *fdtempl;
    Status status;

    //Exception::Mode MM = Exception::setMode(Exception::StatusMode);
    //status = checkDuplicates();
    //if (status) return status;
    //Exception::setMode(MM);

    if (status = check_dir(hints.dirname))
      return status;

    if (status = make_files(hints.dirname, hints.fileprefix, package,
			    hints.c_suffix, hints.h_suffix,
			    &fdh, &fdc, &fdstubsfe, &fdstubsbe,
			    &fdmthfe, &fdmthbe, &fdmk, &fdtempl))
      return status;

    head_gen_C(fdh, hints.fileprefix, hints.h_suffix, package, hints, True);

    fprintf(fdh, "#ifndef _eyedb_%s_\n", package);
    fprintf(fdh, "#define _eyedb_%s_\n\n", package);

    // 6/09/05: should be replaced by correct naming for classes
    //fprintf(fdh, "using namespace eyedb;\n\n");

    if (c_namespace)
      fprintf(fdh, "namespace %s {\n\n", c_namespace);

    if (qseq_list)
      {
	LinkedListCursor *curs = qseq_list->startScan();

	char *qseq;

	while (qseq_list->getNextObject(curs, (void *&)qseq))
	  fprintf(fdh, "%s", qseq);

	qseq_list->endScan(curs);
      }

    //  fprintf(fdc, "#ifndef _inlining_\n\n");
    head_gen_C(fdc, hints.fileprefix, hints.c_suffix, package, hints, True);

    fprintf(fdc, "#include <eyedb/internals/ObjectPeer.h>\n");
    fprintf(fdc, "#include <eyedb/internals/ClassPeer.h>\n");
    fprintf(fdc, "#include <eyedb/internals/kern_const.h>\n\n");

    fprintf(fdc, "#include \"%s%s\"\n\n", hints.fileprefix, hints.h_suffix);

    fprintf(fdc, "#define min(x,y)((x)<(y)?(x):(y))\n\n");

    if (c_namespace)
      fprintf(fdc, "namespace %s {\n\n", c_namespace);

    /*
      fprintf(fdc, "static inline unsigned char *rawdup(const unsigned char *data, unsigned int len)\n{\n");
      fprintf(fdc, "  unsigned char *x = (unsigned char *)malloc(len);\n");
      fprintf(fdc, "  memcpy(x, data, len);\n");
      fprintf(fdc, "  return x;\n");
      fprintf(fdc, "}\n\n");
    */

#ifndef UNIFIED_API
    if (odl_dynamic_attr) {
#endif
      fprintf(fdc, "static eyedb::Bool dynget_error_policy = eyedb::False;\n");
      fprintf(fdc, "static eyedb::Bool dynset_error_policy = eyedb::True;\n");
#ifndef UNIFIED_API
    }
#endif

    fprintf(fdc, "static eyedb::Oid nulloid;\n");
    fprintf(fdc, "static unsigned char nulldata[1];\n");
    fprintf(fdc, "static eyedb::Bool oid_check = eyedb::True;\n");
    fprintf(fdc, "static int class_ind;\n");

    fprintf(fdc, "static eyedb::Database::consapp_t *constructors_x = new eyedb::Database::consapp_t[%d];\n", _class->getCount());

    fprintf(fdc, "static eyedb::Object *(*constructors[%d])(const eyedb::Object *, eyedb::Bool);\n", _class->getCount());
    fprintf(fdc, "static eyedb::GenHashTable *hash;\n");
    //fprintf(fdc, "extern StructClass *OString_Class;\n");
    fprintf(fdc, "#define make_object %sMakeObject\n", package);

    fprintf(fdc, "extern void %sInit(void);\n", package);
    fprintf(fdc, "extern void %sRelease(void);\n", package);
    fprintf(fdc, "extern eyedb::Status %sSchemaUpdate(eyedb::Database *);\n", package);
    fprintf(fdc, "extern eyedb::Status %sSchemaUpdate(eyedb::Schema *);\n\n", package);

    // kludge
    fprintf(fdc, "static eyedb::Class *index_Class = new eyedb::Class(\"index\");\n\n");

    fprintf(fdc, "void %s::init()\n{\n  %sInit();\n}\n\n", package, package);
    fprintf(fdc, "void %s::release()\n{\n  %sRelease();\n}\n\n", package, package);
    fprintf(fdc, "eyedb::Status %s::updateSchema(eyedb::Database *db)\n{\n  return %sSchemaUpdate(db);\n}\n\n", package, package);

    fprintf(fdc, "eyedb::Status %s::updateSchema(eyedb::Schema *m)\n{\n  return %sSchemaUpdate(m);\n}\n\n", package, package);

    /*
      head_gen_C(fdinit, hints.fileprefix, "init", package, False,
      hints.c_suffix);
    */
    head_gen_C(fdstubsfe, hints.fileprefix, "stubsfe", package, hints, True,
	       hints.c_suffix, True, True, c_namespace);
    head_gen_C(fdstubsbe, hints.fileprefix, "stubsbe", package, hints, True,
	       hints.c_suffix, True, True, c_namespace);
    head_gen_C(fdmthfe, hints.fileprefix, "mthfe-skel", package, hints, False,
	       hints.c_suffix, True, True, c_namespace);
    skel_comment(fdmthfe, hints.fileprefix, "mthfe", False);
    pack_init(fdmthfe, package);
    head_gen_C(fdmthbe, hints.fileprefix, "mthbe-skel", package, hints, False,
	       hints.c_suffix, True, True, c_namespace);
    skel_comment(fdmthbe, hints.fileprefix, "mthbe", True);
    pack_init(fdmthbe, package);
    /*
      fprintf(fdinit, ipattern, package, package, package, package, package,
      package, package, package);
    */
    time_t t;
    time(&t);
    const char *sopath = eyedb::ServerConfig::getSValue("sopath");
    if (!sopath)
      return Exception::make("Configuration variable sopath is not set");

    char *xsopath = strdup(sopath);
    char *r = strchr(xsopath, ':');
    if (r) *r = 0;
    if (fdmk)
      fprintf(fdmk, make_pattern, package, package,
	      "Generated by eyedbodl at",
	      ctime(&t),
	      eyedb::CompileBuiltin::getPkgdatadir(),
	      package, package, package,
	      package);

    if (fdtempl)
      fprintf(fdtempl, template_pattern, package, package,
	      "Generated by eyedbodl at",
	      ctime(&t), package,
	      (c_namespace ?
	       (std::string(c_namespace) + "::" + package).c_str() :
	       package), package, package,
	      package, package, package, package);

    free(xsopath);
    LinkedListCursor *curs = _class->startScan();

    Class *cl;

    char *true_prefix = NULL;
    const char *p = strchr(prefix, ':');
    if (p)
      {
	int len = p - prefix;
	true_prefix = new char[len+1];
	strcpy(true_prefix, prefix);
	true_prefix[len] = 0;
      }

    GenContext ctxH(fdh, package, odl_rootclass);

    if (true_prefix){
      fprintf(fdh, "class %s {\n", true_prefix);
      ctxH.push();
    }

    while (_class->getNextObject(curs, (void *&)cl)) {
      if (check_class(cl, False)) {
	const char *suffix;
	string s;
	if (cl->asEnumClass())
	  suffix = "eyedb::Enum";
	else if (cl->asAgregatClass())
	  suffix = cl->asStructClass() ? "eyedb::Struct" : "eyedb::Union";
	else if (cl->asCollectionClass()) {
	  s = string("eyedb::") + cl->asCollectionClass()->getCSuffix();
	  suffix = s.c_str();
	}
	else
	  suffix = "";

	if (!cl->asEnumClass() && !cl->asCollectionClass()) // added !cl->asCollectionClass the 2/11/06
	  fprintf(fdh, "%sclass %s;\n", ctxH.get(), cl->getCName());

	fprintf(fdc, "%s" DEF_PREFIX "%sClass *%s_Class;\n",
		((!_export || cl->asCollectionClass()) ? "static " : ""),
		suffix, cl->asBasicClass() ? cl->getName() : cl->getCName());

	cl->setCanonicalName(cap(cl->getName(), prefix));
      }
    }

    fprintf(fdc, "\n");
    fprintf(fdh, "\n");
    _class->endScan(curs);

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl)) {
      if (check_class(cl, False)) {
	if (!cl->asEnumClass() && !cl->asCollectionClass()) {
	  if (odl_smartptr) {
	    fprintf(fdh, "%sclass %sPtr : public %sPtr {\n",
		    ctxH.get(), cl->getCName(), cl->getParent()->getCName());

	    fprintf(fdh, "\npublic:\n");
	    fprintf(fdh, "%s  %sPtr(%s *o = 0);\n\n",
		    ctxH.get(), cl->getCName(), cl->getCName(),
		    cl->getParent()->getCName());
	  
	    fprintf(fdh, "%s  %s *get%s();\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName());
	    fprintf(fdh, "%s  const %s *get%s() const;\n\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName());
    
	    fprintf(fdh, "%s  %s *operator->();\n",
		    ctxH.get(), cl->getCName(), cl->getCName());
	    fprintf(fdh, "%s  const %s *operator->() const;\n",
		    ctxH.get(), cl->getCName(), cl->getCName());
	    fprintf(fdh, "%s};\n\n", ctxH.get());
	  }
	  /*
	  else
	    fprintf(fdh, "%stypedef %s * %sPtr;\n", ctxH.get(), cl->getCName(), cl->getName());
	  */
	}
      }
    }

    _class->endScan(curs);

    fprintf(fdh, "\n%sclass %s {\n\n", ctxH.get(), package);
    fprintf(fdh, "%s public:\n", ctxH.get());
    fprintf(fdh, "%s  %s(int &argc, char *argv[]) {\n", ctxH.get(), package);
    fprintf(fdh, "%s    eyedb::init(argc, argv);\n", ctxH.get());
    fprintf(fdh, "%s    init();\n", ctxH.get());
    fprintf(fdh, "%s  }\n\n", ctxH.get());
    fprintf(fdh, "%s  ~%s() {\n", ctxH.get(), package);
    fprintf(fdh, "%s    release();\n", ctxH.get());
    fprintf(fdh, "%s    eyedb::release();\n", ctxH.get());
    fprintf(fdh, "%s  }\n\n", ctxH.get());
    fprintf(fdh, "%s  static void init();\n", ctxH.get());
    fprintf(fdh, "%s  static void release();\n", ctxH.get());
    fprintf(fdh, "%s  static eyedb::Status updateSchema(eyedb::Database *db);\n", ctxH.get());
    fprintf(fdh, "%s  static eyedb::Status updateSchema(eyedb::Schema *m);\n", ctxH.get());
    fprintf(fdh, "%s};\n", ctxH.get());

    fprintf(fdh, "%s\nclass %sDatabase : public eyedb::Database {\n", ctxH.get(), package);

#define NO_COMMENTS

#ifndef NO_COMMENTS
    fprintf(fdh, "%s\n  // ----------------------------------------------------------------------\n", ctxH.get());
    fprintf(fdh, "%s  // %sDatabase Interface\n", ctxH.get(), package);
    fprintf(fdh, "%s  // ----------------------------------------------------------------------\n", ctxH.get());
#else
    fprintf(fdh, "%s\n", ctxH.get());
#endif
    fprintf(fdh, "%s public:\n", ctxH.get());

    fprintf(fdh, "%s  %sDatabase(const char *dbname, const char *_dbmdb_str = 0) : eyedb::Database(dbname, _dbmdb_str) {}\n", ctxH.get(), package);

    fprintf(fdh, "%s  %sDatabase(eyedb::Connection *conn, const char *dbname, const char *_dbmdb_str, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);\n", ctxH.get(), package);

    fprintf(fdh, "%s  %sDatabase(eyedb::Connection *conn, const char *dbname, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);\n", ctxH.get(), package);

    fprintf(fdh, "%s  %sDatabase(const char *dbname, int _dbid, const char *_dbmdb_str = 0) : eyedb::Database(dbname, _dbid, _dbmdb_str) {}\n", ctxH.get(), package);
    fprintf(fdh, "%s  %sDatabase(int _dbid, const char *_dbmdb_str = 0) : eyedb::Database(_dbid, _dbmdb_str) {}\n", ctxH.get(), package);

    fprintf(fdh, "%s  eyedb::Status open(eyedb::Connection *, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);\n", ctxH.get());

    fprintf(fdh, "%s  eyedb::Status open(eyedb::Connection *, eyedb::Database::OpenFlag, const eyedb::OpenHints *hints, const char *user = 0, const char *passwd = 0);\n", ctxH.get());

    fprintf(fdh, "%s  static void setConsApp(eyedb::Database *);\n", ctxH.get());

    if (superclass)
      {
	const char *sname = superclass->getName();
	fprintf(fdh, "%s\n  static %s *as%s(eyedb::Object *);\n", ctxH.get(), sname,
		superclass->getCanonicalName());
	fprintf(fdh, "%s  static const %s *as%s(const eyedb::Object *);\n", ctxH.get(), sname,
		superclass->getCanonicalName());
      }

    fprintf(fdh, "%s  static eyedb::Status checkSchema(eyedb::Schema *);\n", ctxH.get());
#ifndef UNIFIED_API
    if (odl_dynamic_attr) {
#endif
      fprintf(fdh, "%s  static eyedb::Bool getDynamicGetErrorPolicy();\n",
	      ctxH.get());
      fprintf(fdh, "%s  static eyedb::Bool getDynamicSetErrorPolicy();\n",
	      ctxH.get());
      fprintf(fdh, "%s  static void setDynamicGetErrorPolicy(eyedb::Bool policy);\n",
	      ctxH.get());
      fprintf(fdh, "%s  static void setDynamicSetErrorPolicy(eyedb::Bool policy);\n",
	      ctxH.get());
#ifndef UNIFIED_API
    }
#endif

    fprintf(fdh, "%s};\n\n", ctxH.get());

    GenContext ctx(fdc, package, odl_rootclass);

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (check_class(cl, True))
	{
	  const char *stubs = generateStubs_C(hints.gen_class_stubs,
					      cl, hints.dirname, package, hints);
	  status = cl->generateCode_C(this, prefix, hints, stubs, fdh, fdc,
				      fdstubsfe, fdstubsbe, fdmthfe, fdmthbe);
	  if (status)
	    {
	      _class->endScan(curs);
	      goto out;
	    }
	}

    _class->endScan(curs);

    fprintf(fdc, "static const char not_exit_msg[] = \"class does not exist\";\n");
    fprintf(fdc, "static const char differ_msg[] = \"class differs in database and in runtime environment\";\n\n");

    fprintf(fdc, "void %sInit(void)\n{\n", package);

    ctx.push();
    curs = _class->startScan();

    fprintf(fdc, "%sif (hash) return;\n\n", ctx.get());
    fprintf(fdc, "%shash = new eyedb::GenHashTable(%d, %d);\n\n", ctx.get(),
	    strlen(db_prefix), _class->getCount());

    while (_class->getNextObject(curs, (void *&)cl))
      if (check_class(cl, False))
	fprintf(fdc, "%s%s_init_p();\n", ctx.get(), cl->getCName());

    ctx.pop();
    _class->endScan(curs);

    ctx.push();
    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass())
	fprintf(fdc, "%s%s_init();\n", ctx.get(), cl->getCName());

    ctx.pop();
    _class->endScan(curs);

    fprintf(fdc, "}\n\n");

    fprintf(fdc, "void %sRelease(void)\n{\n", package);

    ctx.push();
    curs = _class->startScan();

    fprintf(fdc, "%sif (!hash) return;\n\n", ctx.get());
    fprintf(fdc, "%sdelete hash;\n", ctx.get());
    fprintf(fdc, "%shash = 0;\n\n", ctx.get());

    while (_class->getNextObject(curs, (void *&)cl))
      if (check_class(cl, False))
	fprintf(fdc, "%s%s_Class->release();\n", ctx.get(), cl->getCName());

    ctx.pop();
    _class->endScan(curs);

    fprintf(fdc, "}\n\n");

    fprintf(fdc, "static eyedb::Status\n%sSchemaUpdate(eyedb::Schema *m, eyedb::Database *db)\n{\n", package);

    ctx.push();
    fprintf(fdc, "%sm->setName(\"%s\");\n", ctx.get(),
	    (schname ? schname : package));
    fprintf(fdc, "%seyedb::Status status;\n", ctx.get());

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (check_class(cl, False))
	{
	  const char *suffix;
	  string s;
	  if (cl->asEnumClass())
	    suffix = "eyedb::Enum";
	  else if (cl->asAgregatClass())
	    suffix = cl->asStructClass() ? "eyedb::Struct" : "eyedb::Union";
	  else if (cl->asCollectionClass()) {
	    s = string("eyedb::") + cl->asCollectionClass()->getCSuffix();
	    suffix = s.c_str();
	  }
	  else
	    suffix = "";

	  fprintf(fdc, "%s" DEF_PREFIX "%sClass *%s_class = %s_make(0, m);\n", ctx.get(),
		  suffix, cl->getCName(), cl->getCName());

	  if (!strcmp(cl->getName(), "set<object*>")) // kludge??
	    continue;

	  if (cl->isRootClass())
	    continue;

	  fprintf(fdc, "%sif (!m->getClass(\"%s\"))\n",
		  ctx.get(), cl->getAliasName());
	  fprintf(fdc, "%s  {\n", ctx.get());
	  fprintf(fdc, "%s    status = m->addClass(%s_class);\n", ctx.get(), cl->getCName());
	  fprintf(fdc, "%s    if (status)\n", ctx.get());
	  fprintf(fdc, "%s%s    return status;\n", ctx.get(), ctx.get());
	  fprintf(fdc, "%s  }\n", ctx.get());
	}

    ctx.pop();
    _class->endScan(curs);

    fprintf(fdc, "\n");

    ctx.push();
    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass())
	fprintf(fdc, "%s%s_make(%s_class, m);\n", ctx.get(), cl->getCName(), cl->getName());
    _class->endScan(curs);

    fprintf(fdc, "\n%sif (!db) return eyedb::Success;\n\n", ctx.get());

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass()) {
	if (cl->isRootClass())
	  continue;

	fprintf(fdc, "%sif (!%s_class->compare(m->getClass(\"%s\")))\n",
		ctx.get(), cl->getCName(), cl->getAliasName());
	fprintf(fdc, "%s  return eyedb::Exception::make(eyedb::IDB_ERROR, \"'%s' %%s\", differ_msg);\n", ctx.get(), cl->getName());
      }

    _class->endScan(curs);
    fprintf(fdc, "\n");
  
    //  fprintf(fdc, "%sdb->transactionBegin(DatabaseExclusiveTRMode);\n", ctx.get());
    fprintf(fdc, "%sdb->transactionBegin();\n", ctx.get());
#define NEW_COMP_POLICY
#ifndef NEW_COMP_POLICY
    fprintf(fdc, "%sstatus = m->realize();\n", ctx.get());
    fprintf(fdc, "%sif (status) return status;\n", ctx.get());
#endif

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if ((cl->getUserData(odlGENCODE) || cl->getUserData(odlGENCOMP)) &&
	  cl->getCompList() && cl->getCompList()->getCount()) {
	if (cl->asAgregatClass()) 
	  fprintf(fdc, "\n%sif ((status = %s_comp_realize(db, m->getClass(\"%s\")))) "
		  "return status;\n", ctx.get(), cl->getName(), cl->getAliasName());
	else if (!cl->asCollectionClass())
	  fprintf(fdc, "\n%sif ((status = %s_comp_realize(db, m->getClass(\"%s\")))) "
		  "return status;\n", ctx.get(), cl->getName(),
		  cl->getName());
      }

    _class->endScan(curs);

    curs = _class->startScan();
    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCOMP) ||
	  (cl->getUserData(odlGENCODE) && cl->asAgregatClass()))
	fprintf(fdc, "\n%sif ((status = %s_attrcomp_realize(db, m->getClass(\"%s\")))) "
		"return status;\n", ctx.get(), cl->getName(), cl->getAliasName());

    _class->endScan(curs);
#ifdef NEW_COMP_POLICY
    fprintf(fdc, "%sstatus = m->realize();\n", ctx.get());
    fprintf(fdc, "%sif (status) return status;\n", ctx.get());
#endif
    fprintf(fdc, "%sdb->transactionCommit();\n", ctx.get());
    fprintf(fdc, "%sreturn eyedb::Success;\n", ctx.get());

    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Status %sSchemaUpdate(eyedb::Database *db)\n{\n",
	    package);

    fprintf(fdc, "%sreturn %sSchemaUpdate(db->getSchema(), db);\n",
	    ctx.get(), package);

    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Status %sSchemaUpdate(eyedb::Schema *m)\n{\n",
	    package);
    fprintf(fdc, "%sreturn %sSchemaUpdate(m, NULL);\n", ctx.get(), package);
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Object *%sMakeObject(eyedb::Object *o, eyedb::Bool remove)\n{\n", package);
    fprintf(fdc, "  if (!o->getClass()) return (eyedb::Object *)0;\n");
    // these 2 lines: REALLY?
    fprintf(fdc, "%sif (eyedb::ObjectPeer::isGRTObject(o))\n", ctx.get());
    fprintf(fdc, "%s  return o;\n", ctx.get());
    fprintf(fdc, "%sint ind = hash->get(o->getClass()->getName());\n", ctx.get());
    // BUG_ETC_UPDATE
    // 5/3/00 replaced:
    // fprintf(fdc, "%sif (ind < 0) return 0;\n", ctx.get());
    // by:
    fprintf(fdc, "%sif (ind < 0 && (!o->getClass()->getStrictAliasName() || (ind = hash->get(o->getClass()->getStrictAliasName())) < 0)) return 0;\n", ctx.get());

    fprintf(fdc, "%seyedb::Object *co = constructors[ind](o, (remove ? eyedb::True : eyedb::False));\n", ctx.get());
    fprintf(fdc, "%seyedb::ObjectPeer::setClass(co, o->getClass());\n", ctx.get());
    fprintf(fdc, "%sif (remove) o->release();\n", ctx.get());
    fprintf(fdc, "%sif (co->getDatabase())\n", ctx.get());
    fprintf(fdc, "%s  co->getDatabase()->cacheObject(co);\n", ctx.get());
    fprintf(fdc, "%sreturn co;\n", ctx.get());
    fprintf(fdc, "}\n\n");

    ctx.pop();

    fprintf(fdc, "%sDatabase::%sDatabase(eyedb::Connection *conn, const char *dbname, eyedb::Database::OpenFlag flag, const char *userauth, const char *passwdauth) : eyedb::Database(dbname)\n{\n", package, package);
    fprintf(fdc, "  eyedb::Status status = open(conn, flag, 0, userauth, passwdauth);\n");
    fprintf(fdc, "  if (status) throw *status;\n");
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "%sDatabase::%sDatabase(eyedb::Connection *conn, const char *dbname, const char *dbmdb_str, eyedb::Database::OpenFlag flag, const char *userauth, const char *passwdauth) : eyedb::Database(dbname, dbmdb_str)\n{\n", package, package);
    fprintf(fdc, "  eyedb::Status status = open(conn, flag, 0, userauth, passwdauth);\n");
    fprintf(fdc, "  if (status) throw *status;\n");
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Status %sDatabase::open(eyedb::Connection *conn, eyedb::Database::OpenFlag flag, const char *userauth, const char *passwdauth)\n{\n", package);
    fprintf(fdc, "  return open(conn, flag, 0, userauth, passwdauth);\n");
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Status %sDatabase::open(eyedb::Connection *conn, eyedb::Database::OpenFlag flag, const eyedb::OpenHints *hints, const char *userauth, const char *passwdauth)\n{\n", package);

    fprintf(fdc, "  eyedb::Status status = eyedb::Database::open(conn, flag, hints, userauth, passwdauth);\n");

    fprintf(fdc, "  if (status) return status;\n");
    if (!odl_dynamic_attr) {
      fprintf(fdc, "  transactionBegin();\n");
      fprintf(fdc, "  status = %sDatabase::checkSchema(getSchema());\n", package);
      fprintf(fdc, "  transactionCommit();\n");
    }


    fprintf(fdc, "\n  if (!status) add(hash, constructors_x);\n");
    fprintf(fdc, "\n  return status;\n");
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "void %sDatabase::setConsApp(eyedb::Database *_db)\n{\n", package);

    /*
      fprintf(fdc, "  if (!_db->getUserData(\"eyedb:%s\") {\n", package);
      fprintf(fdc, "     _db->setUserData(\"eyedb:%s\", (void *)1);\n", package);
      fprintf(fdc, "     _db->add(hash, constructors_x);\n");
      fprintf(fdc, "  }\n");
    */
    fprintf(fdc, "  _db->add(hash, constructors_x);\n");
    fprintf(fdc, "}\n\n");

    if (superclass)
      {
	const char *sname = superclass->getName();
	fprintf(fdc, "%s *%sDatabase::as%s(eyedb::Object *o)\n{\n", sname,
		package, superclass->getCanonicalName());
#if 0
	fprintf(fdc, "  Bool is;\n");
	fprintf(fdc, "  if (o && !%s_Class->isSuperClassOf(o->getClass(), &is) && is)\n", sname);
	fprintf(fdc, "     return (%s *)o;\n", sname);
	fprintf(fdc, "  return (%s *)0;\n", sname);
#else
	fprintf(fdc, "  if (!eyedb::ObjectPeer::isGRTObject(o))\n");
	fprintf(fdc, "     return (%s *)make_object(o, eyedb::False);\n\n",
		sname);
	fprintf(fdc, "  if (hash->get(o->getClass()->getName()) < 0)\n");
	fprintf(fdc, "     return (%s *)0;\n\n", sname);

	fprintf(fdc, "  return (%s *)o;\n", sname);
#endif
	fprintf(fdc, "}\n\n");

	fprintf(fdc, "const %s *%sDatabase::as%s(const eyedb::Object *o)\n{\n", sname,
		package,  superclass->getCanonicalName());
#if 0
	fprintf(fdc, "  eyedb::Bool is;\n");
	fprintf(fdc, "  if (o && !%s_Class->isSuperClassOf(o->getClass(), &is) && is)\n", sname);
	fprintf(fdc, "    return (const %s *)o;\n", sname);
	fprintf(fdc, "  return (const %s *)0;\n", sname);
#else
	fprintf(fdc, "  if (!eyedb::ObjectPeer::isGRTObject((eyedb::Object *)o))\n");
	fprintf(fdc, "    return (const %s *)make_object((eyedb::Object *)o, eyedb::False);\n\n",
		sname);
	fprintf(fdc, "  if (hash->get(o->getClass()->getName()) < 0)\n");
	fprintf(fdc, "    return (const %s *)0;\n\n", sname);

	fprintf(fdc, "  return (const %s *)o;\n", sname);
#endif
	fprintf(fdc, "}\n\n");
      }


    fprintf(fdc, "static void append(char *&s, const char *m1, const char *m2)\n{\n");
    fprintf(fdc, "  if (!s) {s = (char *)malloc(strlen(m1)+strlen(m2)+2); *s = 0;}\n");
    fprintf(fdc, "  else s = (char *)realloc(s, strlen(s)+strlen(m1)+strlen(m2)+2);\n");
    fprintf(fdc, "  strcat(s, m1);\n");
    fprintf(fdc, "  strcat(s, m2);\n");
    fprintf(fdc, "  strcat(s, \"\\n\");\n");
    fprintf(fdc, "}\n\n");

#ifndef UNIFIED_API
    if (odl_dynamic_attr) {
#endif
      fprintf(fdc, "eyedb::Bool %sDatabase::getDynamicGetErrorPolicy() {\n", package);
#ifdef UNIFIED_API    
      if (!odl_dynamic_attr)
	fprintf(fdc, "   throw *eyedb::Exception::make(eyedb::IDB_ERROR, \"getDynamicGetErrorPolicy() %s\");\n}\n\n", dyn_call_error);
      else
#endif
	fprintf(fdc, "   return dynget_error_policy;\n}\n\n");

      fprintf(fdc, "eyedb::Bool %sDatabase::getDynamicSetErrorPolicy() {\n",
	      package);

#ifdef UNIFIED_API
      if (!odl_dynamic_attr)
	fprintf(fdc, "   throw *eyedb::Exception::make(eyedb::IDB_ERROR, \"getDynamicSetErrorPolicy() %s\");\n}\n\n", dyn_call_error);
      else
#endif
	fprintf(fdc, "   return dynget_error_policy;\n}\n\n");

      fprintf(fdc, "void %sDatabase::setDynamicGetErrorPolicy(eyedb::Bool policy) {\n",
	      package);
#ifdef UNIFIED_API
      if (!odl_dynamic_attr)
	fprintf(fdc, "   throw *eyedb::Exception::make(eyedb::IDB_ERROR, \"setDynamicGetErrorPolicy() %s\");\n}\n\n", dyn_call_error);
      else
#endif
	fprintf(fdc, "   dynget_error_policy = policy;\n}\n\n");
      fprintf(fdc, "void %sDatabase::setDynamicSetErrorPolicy(eyedb::Bool policy) {\n",
	      package);
#ifdef UNIFIED_API
      if (!odl_dynamic_attr)
	fprintf(fdc, "   throw *eyedb::Exception::make(eyedb::IDB_ERROR, \"setDynamicSetErrorPolicy() %s\");\n}\n\n", dyn_call_error);
      else
#endif
	fprintf(fdc, "   dynset_error_policy = policy;\n}\n\n");
#ifndef UNIFIED_API
    }
#endif

    fprintf(fdc, "eyedb::Status %sDatabase::checkSchema(eyedb::Schema *m)\n{\n", package);
    //  fprintf(fdc, "  Schema *m = getSchema();\n");
    fprintf(fdc, "  eyedb::Class *cl;\n");
    fprintf(fdc, "  char *s = 0;\n\n");

    curs = _class->startScan();

    while (_class->getNextObject(curs, (void *&)cl))
      if (cl->getUserData(odlGENCODE) && !cl->asCollectionClass()) {
	if (cl->isRootClass())
	  continue;
	fprintf(fdc, "  if (!(cl = m->getClass(\"%s\")))\n",
		cl->getAliasName());
	fprintf(fdc, "    append(s, \"'%s' \", not_exit_msg);\n",
		cl->getAliasName());
	fprintf(fdc, "  else if (!%s_Class->compare(cl))\n", cl->getCName());
	fprintf(fdc, "    append(s, \"'%s' \", differ_msg);\n",
		cl->getName());
      }

    _class->endScan(curs);

    fprintf(fdc, "  if (s) {eyedb::Status status = eyedb::Exception::make(s); free(s); return status;}\n");
    fprintf(fdc, "  return eyedb::Success;\n}\n\n");


    fprintf(fdc, "eyedb::Bool %s_set_oid_check(eyedb::Bool _oid_check)\n", package);
    fprintf(fdc, "{\n");
    fprintf(fdc, "  eyedb::Bool old = oid_check;\n");
    fprintf(fdc, "  oid_check = _oid_check;\n");
    fprintf(fdc, "  return old;\n");
    fprintf(fdc, "}\n\n");

    fprintf(fdc, "eyedb::Bool %s_get_oid_check()\n", package);
    fprintf(fdc, "{\n");
    fprintf(fdc, "  return oid_check;\n");
    fprintf(fdc, "}\n");

    if (c_namespace)
      fprintf(fdc, "\n}\n", c_namespace);

    if (true_prefix) {
      fprintf(fdh, "};\n\n");
      delete [] true_prefix;
    }

    if (1) // because of a previous `goto'
      {
	LinkedListCursor c = LinkedListCursor(_class);
	fprintf(fdh, "\n");
	while (c.getNext((void *&)cl))
	  if (cl->getUserData(odlGENCODE) && cl->asAgregatClass())
	    {
	      /*
	      fprintf(fdh, "#define %s(X) ((%s *)(X))\n\n",
		      hints.style->getString(GenCodeHints::tCast,
					     cl->getCanonicalName(), prefix),
		      cl->getName());
	      */

	      if (!hints.gen_down_casting || !superclass)
		continue;

	      fprintf(fdh, "inline %s *%s(eyedb::Object *o)\n{\n",
		      cl->getName(),
		      hints.style->getString(GenCodeHints::tSafeCast,
					     cl->getCanonicalName(), prefix));
	      fprintf(fdh, "  %s *x = %sDatabase::as%s(o);\n",
		      superclass->getName(),
		      package,
		      superclass->getCanonicalName());
	      fprintf(fdh, "  if (!x) return (%s *)0;\n", cl->getName());
	      fprintf(fdh, "  return x->as%s();\n", cl->getCanonicalName());
	      fprintf(fdh, "}\n\n");

	      fprintf(fdh, "inline const %s *%s(const eyedb::Object *o)\n{\n",
		      cl->getName(),
		      hints.style->getString(GenCodeHints::tSafeCast,
					     cl->getCanonicalName(), prefix));
	      fprintf(fdh, "  const %s *x = %sDatabase::as%s(o);\n",
		      superclass->getName(),
		      package,
		      superclass->getCanonicalName());
	      fprintf(fdh, "  if (!x) return (const %s *)0;\n", cl->getName());
	      fprintf(fdh, "  return x->as%s();\n", cl->getCanonicalName());
	      fprintf(fdh, "}\n\n");
	    }
      }

    fprintf(fdh, "%sextern eyedb::Object *%sMakeObject(eyedb::Object *, eyedb::Bool=eyedb::True);\n", ctxH.get(), package);

    fprintf(fdh, "%sextern eyedb::Bool %s_set_oid_check(eyedb::Bool);\n", ctxH.get(), package);
    fprintf(fdh, "%sextern eyedb::Bool %s_get_oid_check();\n", ctxH.get(), package);

    // added the 30/05/01
    if (_export) {
      fprintf(fdh, "\n");
      curs = _class->startScan();

      while (_class->getNextObject(curs, (void *&)cl))
	if (check_class(cl, False))
	  {
	    if (cl->asCollectionClass()) continue;
	    const char *suffix;
	    if (cl->asEnumClass())
	      suffix = "eyedb::Enum";
	    else if (cl->asAgregatClass())
	      suffix = cl->asStructClass() ? "eyedb::Struct" : "eyedb::Union";
	    else if (cl->asCollectionClass())
	      suffix = cl->asCollectionClass()->getCSuffix();
	    else
	      suffix = "";
	  
	    fprintf(fdh, "extern " DEF_PREFIX "%sClass *%s_Class;\n", suffix, cl->getCName());
	  }
    
      _class->endScan(curs);
    }

    // end of add

    fprintf(fdh, "\n");
    if (odl_smartptr) {
      curs = _class->startScan();
      while (_class->getNextObject(curs, (void *&)cl)) {
	if (check_class(cl, False)) {
	  if (!cl->asEnumClass() && !cl->asCollectionClass()) {
	    fprintf(fdh, "%sinline %sPtr::%sPtr(%s *o) : %sPtr(o) { }\n\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName(),
		    cl->getParent()->getCName());
	  
	    fprintf(fdh, "%sinline %s *%sPtr::get%s() {return dynamic_cast<%s *>(o);}\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName(), cl->getCName());
	    fprintf(fdh, "%sinline const %s *%sPtr::get%s() const "
		    "{return dynamic_cast<%s *>(o);}\n\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName(), cl->getCName());
    
	    fprintf(fdh, "%sinline %s *%sPtr::operator->() {return dynamic_cast<%s *>(o);}\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName());
	    fprintf(fdh, "%sinline const %s *%sPtr::operator->() const {return dynamic_cast<%s *>(o);}\n\n",
		    ctxH.get(), cl->getCName(), cl->getCName(), cl->getCName());
	  }
	}
      }

      _class->endScan(curs);
    }

    if (c_namespace)
      fprintf(fdh, "\n}\n", c_namespace);

    fprintf(fdh, "\n#endif\n");

  out:

    fclose(fdh);
    fclose(fdc);
    if (fdmk)
      fclose(fdmk);
    if (fdtempl)
      fclose(fdtempl);
    fclose(fdstubsfe);
    fclose(fdstubsbe);
    fclose(fdmthbe);
    fclose(fdmthfe);

    if (status)
      remove_files(hints.dirname, hints.fileprefix, hints.c_suffix,
		   hints.h_suffix, package);

    return status;
  }
}
