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


#define protected public
#include "odl.h"
#include "misc.h"
#include "eyedb/internals/ClassPeer.h"
#include "eyedb/internals/ObjectPeer.h"
//#include "imdl.h"
#include "oql_p.h"
#include "CollectionBE.h"
#include <sstream>
using std::ostringstream;

//#define TRACE

using namespace std;

namespace eyedb {

extern FILE *odl_fd;
#define SHBE " must "

class odlPostUpdate {
  static LinkedList list;
  Oid oclsoid;
  Class *cls;

  odlPostUpdate(const Oid &_oclsoid, Class *_cls) {
    oclsoid = _oclsoid;
    cls = _cls;
  }

  Status perform(Database *db);

public:
  static void add(const Oid &oclsoid, Class *cls);
  static Status realize(Database *db);
};

LinkedList odlPostUpdate::list;

//
// odlUpdateItem methods
//

void
odlUpdateItem::initDisplay()
{
  static int init;

  if (!init)
    {
      fprintf(odl_fd, "\n");
      init = 1;
    }
}

void
odlUpdateItem::initDisplayDiff(Database *_db, const char *_odlfile)
{
  static Database *db;
  static const char *odlfile;
  static int init;

  if (_db)
    {
      db = _db;
      odlfile = _odlfile;
      return;
    }

  if (!init)
    {
      fprintf(odl_fd, "\nDifferences between database '%s' and '%s':\n\n",
	      db->getName(), odlfile);
      init = 1;
    }
}

//
// update component method family
//

void
odlUpdateComponent::realize(Database *db, Schema *m)
{
  if (cls_comp) {
    cls_comp->setDatabase(db);
    cls_comp->setClassOwner(m->getClass(cls_comp->getClassOwner()->getName()));

    Class *clown = cls_comp->getClassOwner();
    if (!clown->getIDR() && clown->getUserData())
      ObjectPeer::setIDR(clown,
			    ((Class *)clown->getUserData())->getIDR());
    cls_comp->setClassOwnerOid(cls_comp->getClassOwner()->getOid());
  }
  else {
    attr_comp->setDatabase(db);
  }
}

Status
odlRemoveComponent::prePerform(Database *db, Schema *m)
{
  return Success;
}

odlAddComponent::odlAddComponent(ClassComponent *_comp) :
  odlUpdateComponent(_comp)
{
#ifdef TRACE
  printf("AddComponent(%p)\n", this);
  printf("AddComponent(%p)\n", this);
  printf("%s '", cls_comp->getClass()->getCanonicalName());
  cls_comp->m_trace(stdout, 0, 0, NoRecurs);
  printf("\n");
#endif
}

odlAddComponent::odlAddComponent(AttributeComponent *_attr_comp) :
  odlUpdateComponent(_attr_comp)
{
#ifdef TRACE
  printf("AddComponent(%p)\n", this);
  printf("%s '", attr_comp->getClassOwner()->getCanonicalName());
  comp->m_trace(stdout, 0, AttrCompDetailTrace, NoRecurs);
  printf("\n");
#endif
}

Status
odlAddComponent::postPerform(Database *db, Schema *m)
{
  realize(db, m);
  display();

#ifdef TRACE
  printf("realizing component %p %s %s addcomponent::postPerform\n", comp,
	 cls_comp->getName(), cls_comp->getOid().toString());
#endif
  if (cls_comp)
    return cls_comp->realize_for_update();
  return attr_comp->realize();
}

odlRemoveComponent::odlRemoveComponent(ClassComponent *_cls_comp) :
  odlUpdateComponent(_cls_comp)
{
#ifdef TRACE
  printf("RemoveComponent(%p)\n", this);
  printf("%s '", cls_comp->getClass()->getCanonicalName());
  comp->m_trace(stdout, 0, 0, NoRecurs);
  printf("\n");
#endif
}

odlRemoveComponent::odlRemoveComponent(AttributeComponent *_attr_comp) :
  odlUpdateComponent(_attr_comp)
{
#ifdef TRACE
  printf("RemoveComponent(%p)\n", this);
  printf("%s '", attr_comp->getClassOwner()->getCanonicalName());
  comp->m_trace(stdout, 0, AttrCompDetailTrace, NoRecurs);
  printf("\n");
#endif
}

Status
odlRemoveComponent::postPerform(Database *db, Schema *m)
{
  if (cls_comp && cls_comp->isRemoved())
    return Success;

  if (attr_comp && attr_comp->isRemoved())
    return Success;

  realize(db, m);
  display();

  if (cls_comp)
    return cls_comp->remove_for_update();

  return attr_comp->remove();
}

void
odlUpdateComponent::display()
{
  odlUpdateItem::initDisplay();

  //printf("attr_comp = %p\n", attr_comp);

  if (asRemoveComponent())
    fprintf(odl_fd, "Removing ");
  /*
  else if ((cls_comp && cls_comp->getOid().isValid()) ||
	   (attr_comp && attr_comp->getOid().isValid()))
  */
  else if (updating)
    fprintf(odl_fd, "Updating ");
  else
    fprintf(odl_fd, "Creating ");

  if (cls_comp) {
    //fprintf(odl_fd, "[%s] ", cls_comp->getOid().toString());
    fprintf(odl_fd, "%s '", cls_comp->getClass()->getCanonicalName());
    cls_comp->m_trace(odl_fd, 0, 0, NoRecurs);
  }
  else {
    //fprintf(odl_fd, "[%s] ", attr_comp->getOid().toString());
    fprintf(odl_fd, "%s '", attr_comp->getClass()->getCanonicalName());
    attr_comp->m_trace(odl_fd, 0, AttrCompDetailTrace, NoRecurs);
  }

  if (asRemoveComponent())
    fprintf(odl_fd, "' from ");
  else
    fprintf(odl_fd, "' on ");
  
  fprintf(odl_fd, "class '%s'...\n", (cls_comp ? cls_comp->getClassOwner()->getName() : attr_comp->getClassOwner()->getName()));
}

void
odlUpdateComponent::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  if (cls_comp) {
    fprintf(odl_fd, "  class %s: ", cls_comp->getClassOwner()->getName());
    cls_comp->m_trace(odl_fd, 0, 0, NoRecurs);
  } else {
    fprintf(odl_fd, "  class %s: ", attr_comp->getClassOwner()->getName());
    attr_comp->m_trace(odl_fd, 0, AttrCompDetailTrace, NoRecurs);
  }

  fprintf(odl_fd, SHBE);
  if (asAddComponent())
    fprintf(odl_fd, "be added to database");
  else
    fprintf(odl_fd, "be removed from database");

  fprintf(odl_fd, "\n");
}

//
// update class method family
//

Status
odlAddClass::prePerform(Database *db, Schema *m)
{
  display();
  return Success;
}

bool must_remove(const Class *cls, const Oid &cls_oid, bool check_self)
{
  if (check_self && cls->getOid() == cls_oid)
    return true;

  if (cls->asCollectionClass()) {
    const Class *coll_class = cls->asCollectionClass()->getCollClass();
    if (coll_class->getOid() == cls_oid)
      return true;

    return false;
  }

  if (cls->getParent() && cls->getParent()->getOid() == cls_oid)
    return true;

  return false;
}

odlRemoveClass::odlRemoveClass(Database *db, const Class *_cls,
			       LinkedList *list) :
  odlUpdateClass(_cls), list(list)
{
  Oid cls_oid = cls->getOid();
}

Status
odlRemoveClass::prePerform(Database *db, Schema *_m)
{
  Oid cls_oid = cls->getOid();

  const Schema *m = db->getSchema();
  const LinkedList *cls_lst = m->getClassList();
  LinkedListCursor c(cls_lst);
  Class *xcls;

  string err;

  while (c.getNext((void *&)xcls)) {
    if (must_remove(xcls, cls_oid, false))
      err += string("\n  must remove class ") + xcls->getName();
    else {
      unsigned int attr_cnt;
      const Attribute **attrs = xcls->getAttributes(attr_cnt);
      for (int n = 0; n < attr_cnt; n++) {
	if (must_remove(attrs[n]->getClass(), cls_oid, true))
	  err += string("\n  must remove class ") + xcls->getName() +
	    " or remove attribute " + attrs[n]->getClassOwner()->getName() +
	    "::" + attrs[n]->getName();
      }
    }
  }

  if (err.size())
    return Exception::make((string("while removing class ") + cls->getName() + ": " +
			    err).c_str());

  return Success;
}

Status
odlRemoveClass::postPerform(Database *db, Schema *m)
{
  display();

  const Class *ocls = (const Class *)cls->getUserData();

  if (!ocls->isRemoved() && ocls->getOid().isValid()) {
    Status s = const_cast<Class *>(ocls)->remove_r
      (RecMode::NoRecurs, Class::RemoveInstances);
    if (s) return s;
  }

  return Success;
}

Status
odlRenameClass::prePerform(Database *db, Schema *m)
{
  return Success;
}

Status
odlReparentClass::prePerform(Database *db, Schema *m)
{
  return Success;
}

Status
odlConvertClass::prePerform(Database *db, Schema *m)
{
  return Success;
}

void
odlUpdateClass::display()
{
  odlUpdateItem::initDisplay();

  if (asRemoveClass())
    fprintf(odl_fd, "Removing");
  else if (asAddClass())
    fprintf(odl_fd, "Adding");
  else if (asConvertClass())
    fprintf(odl_fd, "Converting");
  else if (asReparentClass())
    fprintf(odl_fd, "Reparenting");
  else if (asRenameClass())
    fprintf(odl_fd, "Renaming");

  fprintf(odl_fd, " class %s", cls->getName());
  if (asRenameClass())
    fprintf(odl_fd, " from %s", asRenameClass()->name);
  fprintf(odl_fd, "\n");
}

void
odlUpdateClass::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s:" SHBE, cls->getName());

  if (asRemoveClass())
    fprintf(odl_fd, "be removed from database");
  else if (asAddClass())
    fprintf(odl_fd, "be added to database");
  else if (asConvertClass())
    fprintf(odl_fd, "be converted");
  else if (asReparentClass())
    fprintf(odl_fd, "be reparented to %s", cls->getParent()->getName());
  else if (asRenameClass())
    fprintf(odl_fd, "be renamed from %s", asRenameClass()->name);

  fprintf(odl_fd, "\n");
}

Status
odlUpdateClass::postPerform(Database *db, Schema *m)
{
  if (!clsconv)
    return Success;

  clsconv->setOidN(cls->getOid());
  return clsconv->realize();
}

//
// update relationship method family
//

Status
odlAddRelationship::postPerform(Database *db, Schema *m)
{
  display();
  return Success;
}

Status
odlRemoveRelationship::postPerform(Database *db, Schema *m)
{
  display();
  return Success;
}

static const char *
get_relship_card(const Attribute *item, const Attribute *invitem)
{
  if (item->getClass()->asCollectionClass())
    {
      if (invitem->getClass()->asCollectionClass())
	return "many-to-many";
      return "many-to-one";
    }

  if (invitem->getClass()->asCollectionClass())
    return "one-to-many";
  return "one-to-one";
}

void
odlUpdateRelationship::display()
{
  odlUpdateItem::initDisplay();

  //fprintf(odl_fd, "class %s: ", item->getClassOwner()->getName());

  if (asRemoveRelationship())
    fprintf(odl_fd, "Removing ");
  else
    fprintf(odl_fd, "Creating ");

  fprintf(odl_fd, "%s relationship %s::%s <-> %s::%s\n",
	  get_relship_card(item, invitem),
	  item->getClassOwner()->getName(), item->getName(),
	  invitem->getClassOwner()->getName(),
	  invitem->getName());
}

void
odlUpdateRelationship::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: ", item->getClassOwner()->getName());
  fprintf(odl_fd, "%s relationship %s::%s <-> %s::%s",
	  get_relship_card(item, invitem),
	  item->getClassOwner()->getName(), item->getName(),
	  invitem->getClassOwner()->getName(),
	  invitem->getName());
  
  fprintf(odl_fd, SHBE);
  if (asAddRelationship())
    fprintf(odl_fd, "be added to database");
  else
    fprintf(odl_fd, "be removed from database");

  fprintf(odl_fd, "\n");
}

//
// update attribute method family
//

void
odlUpdateAttribute::display()
{
  odlUpdateItem::initDisplay();

  if (asAddAttribute())
    fprintf(odl_fd, "Adding");
  else if (asRemoveAttribute())
    fprintf(odl_fd, "Removing");

  fprintf(odl_fd, " attribute %s::%s", cls->getName(), item->getName());

  fprintf(odl_fd, "\n");
}

void
odlUpdateAttribute::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: attribute %s", cls->getName(), item->getName());
  
  fprintf(odl_fd, SHBE);
  if (asAddAttribute())
    fprintf(odl_fd, "be added to database");
  else if (asRemoveAttribute())
    fprintf(odl_fd, "be removed from database");

  fprintf(odl_fd, "\n");
}

Status
odlUpdateAttribute::invalidateCollClassOid(Database *db,
					   const Class *ocls)
{
  const LinkedList *list = db->getSchema()->getClassList();
  LinkedListCursor c(list);
  Class *xcls;
  while (c.getNext((void *&)xcls))
    {
      CollectionClass *cls_c = xcls->asCollectionClass();
      if (!cls_c)
	continue;
      if (cls_c->getCollClass()->getOid() == ocls->getOid())
	cls_c->invalidateCollClassOid();
    }

  return Success;
}

Status
odlUpdateAttribute::invalidateInverseOid(Database *db,
					 const Class *ocls)
{
  const LinkedList *list = db->getSchema()->getClassList();
  LinkedListCursor c(list);
  Class *xcls;
  while (c.getNext((void *&)xcls))
    {
      AgregatClass *cls_c = xcls->asAgregatClass();
      if (!cls_c || cls_c->isSystem())
	continue;
      unsigned int attr_cnt;
      const Attribute **attrs = cls_c->getAttributes(attr_cnt);
      for (int i = 0; i < attr_cnt; i++)
	{
	  const char *clsname = 0;
	  attrs[i]->getInverse(&clsname, 0, 0);
	  if (clsname && !strcmp(clsname, ocls->getName()))
	    {
	      xcls->touch();
	      break;
	    }
	}
    }

  return Success;
}

Status
odlUpdateAttribute::reportExtentOid(Database *db, const Class *ocls)
{
  Collection *extent = 0, *cls_comp;
  Status s = ocls->getExtent(extent);
  if (s) return s;
  s = ocls->getComponents(cls_comp);
  if (s) return s;
  const_cast<Class *>(cls)->setExtentCompOid(extent->getOid(),
						cls_comp->getOid());
  return Success;
}

static Bool
odlIsInSchema(Database *db, const Class *scls)
{
  Schema *m = db->getSchema();
  const LinkedList *list = m->getClassList();
  LinkedListCursor c(list);
  
  Class *cls;
  while (c.getNext((void *&)cls))
    if (cls == scls)
      return True;

  return False;
}

Status
odlUpdateAttribute::check()
{
  if (!odlAgregatClass::getDeclaredCount())
    return Success;

  std::string s = "when the schema is evolving all database user classes "
    "must be defined in the odl file:\nmissing ";
  LinkedList &list = odlAgregatClass::getDeclaredList();
  LinkedListCursor c(list);
  char *name;
  for (int i = 0; c.getNext((void *&)name); i++)
    s += std::string(i ? ", " : "") + name;
  s += std::string(" class definition") + (list.getCount() > 1 ? "s" : "");
  return Exception::make(IDB_ERROR, s);
}

static Bool
is_defined(const Class *cls)
{
  LinkedList &list = odlAgregatClass::getDeclaredList();
  LinkedListCursor c(list);
  char *name;

  while (c.getNext((void *&)name))
    if (!strcmp(name, cls->getName()))
      return False;

  return True;
}

static Bool
refer_to(const Attribute *attr, const Class *cls);

static Bool
refer_to(const Class *xcls, const Class *cls)
{
  if (xcls->state)
    return False;

  const_cast<Class *>(xcls)->state |= 1;

  unsigned int attr_cnt;
  const Attribute **attrs = xcls->getAttributes(attr_cnt);
  for (int i = 0; i < attr_cnt; i++)
    if (refer_to(attrs[i], cls)) {
      const_cast<Class *>(xcls)->state &= ~1;
      return True;
    }

  const_cast<Class *>(xcls)->state &= ~1;
  return False;
}

static Bool
refer_to(const Attribute *attr, const Class *cls)
{
  const Class *attr_cls = attr->getClass();
  assert(attr_cls);

  if (attr_cls->asBasicClass() ||
      attr_cls->asEnumClass() || attr_cls->asCollectionClass())
    return False;

  //  if (attr_cls->compare(cls))
  if (!strcmp(attr_cls->getName(), cls->getName()))
    return True;

  return refer_to(attr_cls, cls);
}

Status
odlUpdateAttribute::check(Database *db, const Class *cls)
{
  if (!odlAgregatClass::getDeclaredCount())
    return Success;

  LinkedListCursor c(db->getSchema()->getClassList());
  Class *xcls;
  std::string s = "when the schema is evolving all database user classes "
    "referring to an evolving classes "
    "must be defined in the odl file:\n";

  int n = 0;
  while (c.getNext((void *&)xcls))
    if (refer_to(xcls, cls) && !is_defined(xcls)) {
      s += std::string("class ") + xcls->getName() + " refers to class " +
	cls->getName() + " and definition is missing\n";
      n++;
    }

  if (!n)
    return Success;

  return Exception::make(IDB_ERROR, s);
}

//#undef TRACE

//
// odlPostUpdate methods
//

Status
odlPostUpdate::perform(Database *db)
{
  Collection *components;
  Status s = cls->getComponents(components, True);
  if (s) return s;

  Iterator c(components);

#ifdef TRACE
  printf("REALIZING %s %s\n", cls->getName(), oclsoid.toString());
#endif
  ClassComponent *cls_comp;
  for (;;) {
    Bool found;
    s = c.scanNext(found, (Object *&)cls_comp);
    if (s) return s;
    if (!found) break;
    if (cls_comp->getClassOwner()->getOid() == oclsoid) {
#ifdef TRACE
      printf("changing class owner for %s\n", cls_comp->getName());
#endif
      cls_comp->setClassOwner(cls);
      s = cls_comp->realize();
      if (s) return s;
    }
  }

  unsigned int attr_cnt;
  Attribute **attrs = (Attribute **)cls->getAttributes(attr_cnt);

  return Success;
}

Status
odlPostUpdate::realize(Database *db)
{
  LinkedListCursor c(list);
  odlPostUpdate *p;
  while (c.getNext((void *&)p)) {
    Status s = p->perform(db);
    if (s) return s;
  }
  return Success;
}

void
odlPostUpdate::add(const Oid &oclsoid, Class *cls)
{
  list.insertObjectLast(new odlPostUpdate(oclsoid, (Class *)cls));
}

Status
odl_post_update(Database *db)
{
  return odlPostUpdate::realize(db);
}

static void
odl_components_manage(const Class *cls, const Class *ocls)
{
  ClassComponent *cls_comp;
  LinkedListCursor c(cls->getCompList());

#ifdef TRACE
  printf("Components of new class %s %s\n", cls->getName(), cls->getOid().toString());
#endif
  while (c.getNext((void *&)cls_comp)) {
#ifdef TRACE
    printf("\t%s %s\n", cls_comp->getOid().toString(), cls_comp->getName());
#endif
    if (!cls_comp->getOid().isValid()) {
      // cls->getCompList()->deleteObject(comp);
      LinkedListCursor oc(ocls->getCompList());
      ClassComponent *ocls_comp;
      const char *cname = cls_comp->getName().c_str();
      while (oc.getNext((void *&)ocls_comp)) {
	if (!strcmp(cname, ocls_comp->getName().c_str())) {
	  ObjectPeer::setOid(cls_comp, ocls_comp->getOid());
#ifdef TRACE
	  printf("\tsetting %p %s to %s [%s vs. %s %p %p]\n", comp, ocls_comp->getOid().toString(), cls_comp->getName(), cls_comp->getClassOwner()->getOid().toString(),
		 ocls_comp->getClassOwner()->getOid().toString(),
		 cls_comp->getClassOwner(), ocls_comp->getClassOwner());
#endif
	  break;
	}
      }
#ifdef TRACE
      if (!cls_comp->getOid().isValid()) {
	printf("WARNING %s has no oid\n", cls_comp->getName());
      }
#endif
    }

    /*
    printf("touching component %p %s\n", comp, cls_comp->getOid().toString());
    cls_comp->touch();
    */
  }

  const_cast<LinkedList *>(cls->getCompList())->empty();
  const_cast<Class *>(cls)->setUserData("eyedb:odl::update", AnyUserData);

#ifdef TRACE
  printf("Components of old class %s %s\n", ocls->getName(), ocls->getOid().toString());

  LinkedListCursor oc(ocls->getCompList());
  while (oc.getNext((void *&)comp)) {
    printf("\t%s %s\n", cls_comp->getOid().toString(), cls_comp->getName());
  }
#endif
}

Status
odlUpdateAttribute::initClassConv(Database *db)
{
  Status s = check(db, cls);
  if (s) return s;

  clsconv = new ClassConversion(db);

  const Class *ocls = (const Class *)cls->getUserData();
  assert(ocls);
  odl_components_manage(cls, ocls);

#ifdef TRACE
  printf("ocls %s [%p] %s in_schema=%s is_removed=%s\n", ocls->getName(), ocls,
	 ocls->getOid().toString(), odlIsInSchema(db, ocls) ? "yes" : "no",
	 ocls->isRemoved() ? "yes" : "no");
  printf("cls %s [%p] %s in_schema=%s is_removed=%s\n\n", cls->getName(), cls,
	 cls->getOid().toString(), odlIsInSchema(db, cls) ? "yes" : "no",
	 cls->isRemoved() ? "yes" : "no");
#endif
  clsconv->setOidO(ocls->getOid());
  if (!ocls->isRemoved() && ocls->getOid().isValid())
    {
#ifdef TRACE
      printf("removing class %p %s %s keeping %p\n", ocls, ocls->getName(),
	     ocls->getOid().toString(), cls);
#endif
      odlPostUpdate::add(ocls->getOid(), (Class *)cls);
      Status s = const_cast<Class *>(ocls)->remove_r();
      if (s) return s;

      s = invalidateCollClassOid(db, ocls);
      if (s) return s;
      s = invalidateInverseOid(db, ocls);
      if (s) return s;
      s = reportExtentOid(db, ocls);
      if (s) return s;
      ObjectPeer::setOid(const_cast<Class *>(cls), Oid::nullOid);
      const_cast<Class *>(cls)->setXInfo(IDB_XINFO_CLASS_UPDATE);
    }

#ifdef TRACE
  printf("[after] ocls %s [%p] %s in_schema=%s is_removed=%s\n", ocls->getName(), ocls,
	 ocls->getOid().toString(), odlIsInSchema(db, ocls) ? "yes" : "no",
	 ocls->isRemoved() ? "yes" : "no");
  printf("[after] cls %s [%p] %s in_schema=%s is_removed=%s\n\n", cls->getName(), cls,
	 cls->getOid().toString(), odlIsInSchema(db, cls) ? "yes" : "no",
	 cls->isRemoved() ? "yes" : "no");
#endif

  clsconv->setClsname(cls->getName());
  clsconv->setAttrname(item->getName());
  clsconv->setAttrnum(item->getNum());
  return Success;
}

Status
odlUpdateAttribute::postPerform(Database *db, Schema *m)
{
  if (!clsconv)
    return Success;

  clsconv->setOidN(cls->getOid());
  return clsconv->realize();
}

Status
odlAddAttribute::prePerform(Database *db, Schema *m)
{
  display();
  Offset offset;
  Size item_psize, psize, item_inisize;
  item->getPersistentIDR(offset, item_psize, psize, item_inisize);

  Status s = initClassConv(db);
  if (s) return s;

  clsconv->setUpdtype(ADD_ATTR);
  clsconv->setOffsetO(0);
  clsconv->setOffsetN(offset);
  clsconv->setSizeO(0);
  clsconv->setSizeN(psize);

#ifdef TRACE
  printf("add attribute %s: %d %d %d %d\n", item->getName(), offset,
	 item_psize, psize, item_inisize);
#endif
  return Success;
}

Status
odlRemoveAttribute::prePerform(Database *db, Schema *m)
{
  display();

  Offset offset;
  Size item_psize, psize, item_inisize;
  item->getPersistentIDR(offset, item_psize, psize, item_inisize);

  Status s = initClassConv(db);
  if (s) return s;

  clsconv->setUpdtype(RMV_ATTR);
  clsconv->setOffsetO(0);
  clsconv->setOffsetN(offset);
  clsconv->setSizeO(0);
  clsconv->setSizeN(psize);

#ifdef TRACE
  printf("remove attribute %s: %d %d %d %d\n", item->getName(), offset,
	 item_psize, psize, item_inisize);
#endif

  return Success;
}

Status
odlRenameAttribute::prePerform(Database *db, Schema *m)
{
  display();
  const_cast<Class *>(cls)->touch();
  return Success;
}

#define CNV(CLSTYP, CNVTYP) \
 if (ncls->as##CLSTYP##Class()) { \
    clsconv->setCnvtype(CNVTYP); \
    odlUpdateItem::initDisplay(); \
    fprintf(odl_fd, "Converting attribute %s::%s using " #CNVTYP " conversion method\n", cls->getName(), item->getName()); \
    return Success; \
 }

#define CNV_TO_CHAR(CNVTYP) \
 if (ncls->asCharClass()) { \
    odlUpdateItem::initDisplay(); \
    if (item->isVarDim()) { \
     clsconv->setCnvtype(CNVTYP##_TO_STRING); \
     fprintf(odl_fd, "Converting attribute %s::%s using " #CNVTYP "_TO_STRING conversion method\n", cls->getName(), item->getName()); \
    } \
    else { \
     clsconv->setCnvtype(CNVTYP##_TO_CHAR); \
     fprintf(odl_fd, "Converting attribute %s::%s using " #CNVTYP "_TO_CHAR conversion method\n", cls->getName(), item->getName()); \
    } \
    return Success; \
 }

#define CNV_FROM_CHAR(CLSTYP, CNVTYP) \
 if (ncls->as##CLSTYP##Class()) { \
    odlUpdateItem::initDisplay(); \
    if (oitem->isVarDim()) { \
      clsconv->setCnvtype(STRING_TO_##CNVTYP); \
      fprintf(odl_fd, "Converting attribute %s::%s using STRING_TO_" #CNVTYP " conversion method\n", cls->getName(), item->getName()); \
    } \
    else { \
      clsconv->setCnvtype(CHAR_TO_##CNVTYP); \
      fprintf(odl_fd, "Converting attribute %s::%s using CHAR_TO_" #CNVTYP " conversion method\n", cls->getName(), item->getName()); \
    } \
    return Success; \
 }

#define CNV_FROM_CHAR_TO_CHAR() \
 if (ncls->asCharClass()) { \
    odlUpdateItem::initDisplay(); \
    if (item->isVarDim()) { \
      if (oitem->isVarDim()) \
       return Exception::make(IDB_ERROR, "internal error: unexpected string to string conversion"); \
      clsconv->setCnvtype(CHAR_TO_STRING); \
      fprintf(odl_fd, "Converting attribute %s::%s using CHAR_TO_STRING conversion method\n", cls->getName(), item->getName()); \
    } \
    else { \
      if (oitem->isVarDim()) {\
        clsconv->setCnvtype(STRING_TO_CHAR); \
        fprintf(odl_fd, "Converting attribute %s::%s using STRING_TO_CHAR conversion method\n", cls->getName(), item->getName()); \
     } \
     else { \
        clsconv->setCnvtype(CHAR_TO_CHAR); \
        fprintf(odl_fd, "Converting attribute %s::%s using CHAR_TO_CHAR conversion method\n", cls->getName(), item->getName()); \
     } \
   } \
    return Success; \
}

static Index *
odl_fill_index(Index *ridx, Index *idx)
{
  return 0;
}

static Index *
odl_clone_index(Index *idx)
{
  return 0;
}

Status
odlConvertAttribute::prePerformBasic(Schema *m, const Class *ncls,
				     const Class *ocls)
{
  assert(0);

  if (ocls->asInt16Class()) {
    CNV(Int16, INT16_TO_INT16);
    CNV(Int32, INT16_TO_INT32);
    CNV(Int64, INT16_TO_INT64);
    CNV(Float, INT16_TO_FLOAT);
    CNV(Byte, INT16_TO_BYTE);
    CNV_TO_CHAR(INT16);
  }
  else if (ocls->asInt32Class()) {
    CNV(Int32, INT32_TO_INT32);
    CNV(Int16, INT32_TO_INT16);
    CNV(Int64, INT32_TO_INT64);
    CNV(Float, INT32_TO_FLOAT);
    CNV(Byte, INT32_TO_BYTE);
    CNV_TO_CHAR(INT32);
  }
  else if (ocls->asInt64Class()) {
    CNV(Int64, INT64_TO_INT64);
    CNV(Int16, INT64_TO_INT16);
    CNV(Int32, INT64_TO_INT32);
    CNV(Float, INT64_TO_FLOAT);
    CNV(Byte, INT64_TO_BYTE);
    CNV_TO_CHAR(INT64);
  }
  else if (ocls->asFloatClass()) {
    CNV(Float, FLOAT_TO_FLOAT);
    CNV(Int16, FLOAT_TO_INT16);
    CNV(Int32, FLOAT_TO_INT32);
    CNV(Int64, FLOAT_TO_INT64);
    CNV(Byte, FLOAT_TO_BYTE);
    CNV_TO_CHAR(FLOAT);
  }
  else if (ocls->asCharClass()) {
    CNV_FROM_CHAR_TO_CHAR();
    CNV_FROM_CHAR(Int16, INT16);
    CNV_FROM_CHAR(Int32, INT32);
    CNV_FROM_CHAR(Int64, INT64);
    CNV_FROM_CHAR(Byte, BYTE);
    CNV_FROM_CHAR(Float, FLOAT);
  }
  else if (ocls->asByteClass()) {
    CNV(Byte, BYTE_TO_BYTE);
    CNV(Int16, BYTE_TO_INT16);
    CNV(Int32, BYTE_TO_INT32);
    CNV(Int64, BYTE_TO_INT64);
    CNV(Float, BYTE_TO_FLOAT);
    CNV_TO_CHAR(BYTE);
  }
  else if (ocls->asEnumClass()) {
    CNV(Enum, ENUM_TO_ENUM);
    CNV(Int16, ENUM_TO_INT16);
    CNV(Int32, ENUM_TO_INT32);
    CNV(Int64, ENUM_TO_INT64);
    CNV(Byte, ENUM_TO_BYTE);
    CNV(Char, ENUM_TO_CHAR);
    CNV(Float, ENUM_TO_FLOAT);
  }
  else
    return Exception::make(IDB_ERROR,
			      "conversion from class %s to class %s "
			      "is not supported",
			      ocls->getName(), ncls->getName());

  return Success;
}

static inline int
get_dim(const TypeModifier &tmod)
{
  return (tmod.mode & TypeModifier::VarDim) ? (-tmod.pdims) : tmod.pdims;
}

#define NOT_(X, Y) \
if (clsconv->getCnvtype() == X) \
   return Exception::make(IDB_ERROR, "conversion " #X " is not " #Y)

#define NOT_IMPLEMENTED(X) \
  NOT_(X, "yet implemented")

#define NOT_SUPPORTED(X) \
  NOT_(X, "supported")

Status
odlConvertAttribute::prePerform(Database *db, Schema *m)
{
  odlUpdateItem::initDisplay();

  const Class *ocls = oitem->getClass();
  const TypeModifier &otmod = oitem->getTypeModifier();
  const Class *ncls = item->getClass();
  const TypeModifier &tmod = item->getTypeModifier();

  if (tmod.ndims > 1 || otmod.ndims > 1)
    return Exception::make(IDB_ERROR, "attribute %s::%s: "
			      "no automatic conversion for multi-"
			      "dimensional arrays",
			      cls->getName(), item->getName());

  Status s = initClassConv(db);
  if (s) return s;
  Offset offset_o, offset_n;
  Size dummy, psize_o, psize_n, item_psize_o, item_psize_n;

  item->getPersistentIDR(offset_n, item_psize_n, psize_n, dummy);
  oitem->getPersistentIDR(offset_o, item_psize_o, psize_o, dummy);

  clsconv->setOffsetO(offset_o);
  clsconv->setOffsetN(offset_n);
  clsconv->setUpdtype(CNV_ATTR);

  clsconv->setSrcDim(get_dim(otmod));
  clsconv->setDestDim(get_dim(tmod));

#ifdef TRACE
  printf("odlConvertAttribute(%s::%s) dims = %d vs. %d\n",
	 cls->getName(), item->getName(), clsconv->getSrcDim(),
	 clsconv->getDestDim());
#endif

  // check numeric conversions
  clsconv->setSizeO(psize_o);
  clsconv->setSizeN(psize_n);

  if (ncls->asBasicClass() && (ocls->asBasicClass() || ocls->asEnumClass())) 
    return prePerformBasic(m, ncls, ocls);

  // check class modification propagation
  if (/*tmod.compare(&otmod) &&*/ !strcmp(ncls->getName(), ocls->getName()))
    {
      if (!oitem->isIndirect() && !item->isIndirect() &&
	  !ncls->asCollectionClass())
	{
#ifdef TRACE
	  printf("class %s: class modification propagation on attribute "
		 "%s::%s\n",
		 cls->getOid().toString(), cls->getName(), item->getName());
	  printf("new class %s %s\n", ncls->getOid().toString(),
		 ncls->getName());
	  printf("old class %s %s\n", ocls->getOid().toString(),
		 ocls->getName());
	  printf("attribute old class oid %s\n",
		 oitem->getClass()->getOid().toString());
#endif

#ifdef VERBOSE_2
	  odlUpdateItem::initDisplay();
	  fprintf(odl_fd, "Converting attribute %s::%s", cls->getName(),
		  item->getName());
	  fprintf(odl_fd, " using CLASS_TO_CLASS conversion method\n");
#endif

	  // changed the 12/06/01
	  //clsconv->setRoidO(ncls->getOid());
	  clsconv->setSizeO(item_psize_o);
	  clsconv->setSizeN(item_psize_n);
	  clsconv->setRoidO(ocls->getOid());
	  clsconv->setCnvtype(CLASS_TO_CLASS);
	  //clsconv->trace();
	  return Success;
	}

      if (!oitem->isIndirect() && item->isIndirect())
	return Exception::make(IDB_ERROR, "attribute %s::%s: "
				  "conversion between a direct "
				  "and an indirect attribute is not supported",
				  cls->getName(), item->getName());

      if (oitem->isIndirect() && !item->isIndirect())
	return Exception::make(IDB_ERROR, "attribute %s::%s: "
				  "conversion between an indirect "
				  "and a direct attribute is not supported",
				  cls->getName(), item->getName());

      clsconv->setCnvtype(NIL_CNV);
      /*
      clsconv->release();
      clsconv = 0;
      */
#ifdef TRACE
      printf("nil conversion on %s::%s\n", cls->getName(), item->getName());
#endif
      return Success;
    }

  // check user conversions

  // check uptype conversions

  // check no conversions: for instance, because of the renaming of a class

  //NOT_IMPLEMENTED(CHAR_TO_STRING);
  NOT_SUPPORTED(STRING_TO_CHAR);
  //NOT_IMPLEMENTED(INT16_TO_STRING);
  NOT_SUPPORTED(STRING_TO_INT16);
  //NOT_IMPLEMENTED(INT32_TO_STRING);
  NOT_SUPPORTED(STRING_TO_INT32);
  //NOT_IMPLEMENTED(INT64_TO_STRING);
  NOT_SUPPORTED(STRING_TO_INT64);
  //NOT_IMPLEMENTED(FLOAT_TO_STRING);
  NOT_SUPPORTED(STRING_TO_FLOAT);
  //NOT_IMPLEMENTED(BYTE_TO_STRING);
  NOT_SUPPORTED(STRING_TO_BYTE);

  return Exception::make(IDB_ERROR, "conversion from class %s to class %s "
			    "is not supported",
			    ocls->getName(), ncls->getName());
}

Status
odlReorderAttribute::prePerform(Database *db, Schema *m)
{
  display();
  return Success;
}

Status
odlMigrateAttribute::prePerform(Database *db, Schema *m)
{
  display();
  return Success;
}

void
odlReorderAttribute::display()
{
  odlUpdateItem::initDisplay();

  fprintf(odl_fd, "Ignoring %s::%s position in the ODL:",
	  cls->getName(), item->getName());

  fprintf(odl_fd, " has been reordered from #%d to #%d position\n",
	  asReorderAttribute()->oldnum, asReorderAttribute()->newnum);
}

void
odlReorderAttribute::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: attribute %s", cls->getName(), item->getName());
  
  fprintf(odl_fd, " automatically reordered from #%d to #%d in the ODL\n",
	  oldnum, newnum);
}

void
odlConvertAttribute::display()
{
  odlUpdateItem::initDisplay();

  fprintf(odl_fd, "Converting attribute %s::%s", cls->getName(),
	  item->getName());

  if (upd_hints)
    fprintf(odl_fd, " using %s method", upd_hints->detail);

  fprintf(odl_fd, "\n");
}

void
odlConvertAttribute::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: attribute %s", cls->getName(), item->getName());
  
  fprintf(odl_fd, " must be converted");
  if (upd_hints)
    fprintf(odl_fd, " using %s method", upd_hints->detail);

  fprintf(odl_fd, "\n");
}

void
odlRenameAttribute::display()
{
  odlUpdateItem::initDisplay();

  fprintf(odl_fd, "Renaming attribute %s::%s", cls->getName(),
	  item->getName());

  if (asRenameAttribute())
    fprintf(odl_fd, " from %s", upd_hints->detail);

  if (upd_hints->detail2)
    fprintf(odl_fd, " using %s method", upd_hints->detail2);

  fprintf(odl_fd, "\n");
}

void
odlRenameAttribute::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: attribute %s", cls->getName(), item->getName());
  
  fprintf(odl_fd, " must be renamed from %s", upd_hints->detail);

  if (upd_hints->detail2)
    fprintf(odl_fd, " using %s method", upd_hints->detail2);

  fprintf(odl_fd, "\n");
}

void
odlMigrateAttribute::display()
{
  odlUpdateItem::initDisplay();

  fprintf(odl_fd, "Migrating attribute %s::%s", cls->getName(),
	  item->getName());

  fprintf(odl_fd, " to %s::%s", upd_hints->detail, upd_hints->detail2);

  if (upd_hints->detail3)
    fprintf(odl_fd, " using %s method", upd_hints->detail3);

  fprintf(odl_fd, "\n");
}

void
odlMigrateAttribute::displayDiff(Database *db, const char *odlfile)
{
  odlUpdateItem::initDisplayDiff(db, odlfile);

  fprintf(odl_fd, "  class %s: attribute %s", cls->getName(), item->getName());
  
  fprintf(odl_fd, " must migrate to %s::%s", upd_hints->detail, upd_hints->detail2);

  if (upd_hints->detail3)
    fprintf(odl_fd, " using %s method", upd_hints->detail3);

  fprintf(odl_fd, "\n");
}

}
