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


#include "eyedb_p.h"
#include <assert.h>
#include "AttrNative.h"
#include "CollectionBE.h"

using namespace std;

//
// collection_class
//

namespace eyedb {

const char *
CollectionClass::make_name(const char *prefix, Class* coll_class,
			      Bool isref, int dim, Bool _alias)
{
  static char name[256];
  const char *s = _alias ? coll_class->getAliasName() : coll_class->getName();
  if (dim <= 1)
    sprintf(name, "%s<%s%s>", prefix, s , (isref ? "*" : ""));
  else
    sprintf(name, "%s<%s%s[%d]>", prefix, s, (isref ? "*" : ""), dim);
  return name;
}

const char *CollectionClass::getCName(Bool) const
{
#define NN 12
  static char c_name[NN][128];
  static int which;

  if (which >= NN)
    which = 0;

  char *sname = c_name[which++];
  sprintf(sname, "%s_%s%s", getClass()->getName(),
	  coll_class->asBasicClass() ? coll_class->getName() :
	  coll_class->getCName(), (isref ? "_ref" : ""));

  if (dim > 1)
    {
      char tok[32];
      sprintf(tok, "_%d", dim);
      strcat(sname, tok);
    }

  return sname;
}

int
get_item_size(const Class *coll_class, int dim)
{
  Size psize, vsize, isize;
  coll_class->getIDRObjectSize(&psize, &vsize, &isize);
  return (psize - isize - IDB_OBJ_HEAD_SIZE) * dim;
}

Status
CollectionClass::check(Class *_coll_class, Bool _isref, int _dim)
{
  Status s = Success;

  if (_dim <= 0)
    s = Exception::make(IDB_COLLECTION_ERROR,
			"invalid dimension: %d", _dim);
  else if (_dim > 1 && (!_coll_class || !_coll_class->asCharClass()))
    s = Exception::make(IDB_COLLECTION_ERROR,
			"dimension > 1 are supported only for collection of bounded strings");
  else if (_coll_class &&
	   !_isref &&
	   !_coll_class->asBasicClass() &&
	   !_coll_class->asEnumClass()) {
    // check for : no vardim
    unsigned int attr_cnt;
    const Attribute **attrs = _coll_class->getAttributes(attr_cnt);
    for (int n = 0; n < attr_cnt; n++)
      if (attrs[n]->isVarDim()) {
	s = Exception::make(IDB_COLLECTION_ERROR,
			    "variable dimension attribute %s::%s is not supported in collection of litterals", _coll_class->getName(),
			    attrs[n]->getName());
	break;
      }
  }

  return s;
}

CollectionClass::CollectionClass(Class *_coll_class, Bool _isref, const char *prefix) : Class("")
{
  Exception::Mode mode = Exception::setMode(Exception::StatusMode);
  _status = check(_coll_class, _isref, 1);
  Exception::setMode(mode);

  if (_status)
    return;

  coll_class = _coll_class;
  if (coll_class && coll_class->isSystem())
    m_type = System;

  isref = _isref;
  dim = 1;
  free(name);

  name = strdup(make_name(prefix, coll_class, isref, 1, False));
  aliasname = strdup(make_name(prefix, coll_class, isref, 1, True));

  cl_oid.invalidate();

  if (isref)
    item_size = sizeof(Oid);
  else
    item_size = get_item_size(coll_class, dim);

  AttrNative::copy(CollectionITEMS, items, items_cnt, this);

  // should be in a method
  // WARNING/ERROR: assigning a non null value to psize makes
  // things wrong in the IDB_objectReadLocal() function.
  idr_psize = IDB_OBJ_HEAD_SIZE + sizeof(Oid);
  idr_vsize = sizeof(Object *);
  idr_inisize = 0;
  idr_objsz = idr_vsize + idr_psize;
}

CollectionClass::CollectionClass(Class *_coll_class, int _dim, const char *prefix) : Class("")
{
  Exception::Mode mode = Exception::setMode(Exception::StatusMode);
  _status = check(_coll_class, False, _dim);
  Exception::setMode(mode);

  if (_status)
    return;

  coll_class = _coll_class;
  if (coll_class && coll_class->isSystem())
    m_type = System;

  isref = False;
  dim = _dim;
  free(name);
  name = strdup(make_name(prefix, coll_class, isref, dim, False));
  aliasname = strdup(make_name(prefix, coll_class, isref, dim, True));

  parent = Class_Class;
  cl_oid.invalidate();

  item_size = get_item_size(coll_class, dim);

  AttrNative::copy(CollectionITEMS, items, items_cnt, this);

  idr_psize = IDB_OBJ_HEAD_SIZE + sizeof(Oid);
  idr_vsize = sizeof(Object *);
  idr_inisize = 0;
  idr_objsz = idr_vsize + idr_psize;
}

void
CollectionClass::copy_realize(const CollectionClass &cl)
{
  coll_class = cl.coll_class;
  if (coll_class && coll_class->isSystem())
    m_type = System;
  isref = cl.isref;
  dim = cl.dim;
  cl_oid = cl.cl_oid;
  item_size = cl.item_size;
  _status = cl._status;
}

CollectionClass::CollectionClass(const CollectionClass &cl)
  : Class(cl)
{
  coll_class = 0;
  isref = False;
  dim = 0;
  cl_oid.invalidate();
  item_size = 0;
  _status = NULL;

  copy_realize(cl);
}

Status
CollectionClass::loadComplete(const Class *cl)
{
  assert(cl->asCollectionClass());
  Status s = Class::loadComplete(cl);
  if (s) return s;
  copy_realize(*cl->asCollectionClass());
  return Success;
}

CollectionClass::CollectionClass(const Oid &_oid, const char *_name) :
Class(_oid, _name)
{
}

CollectionClass& CollectionClass::operator=(const CollectionClass &cl)
{
  this->Class::operator=(cl);
  copy_realize(cl);
  return *this;
}

#define RECURS_BUG // added the 11/06/99

Bool CollectionClass::compare_perform(const Class *cl,
				      Bool compClassOwner,
				      Bool compNum,
				      Bool compName,
				      Bool inDepth) const
{
  if (!cl->asCollectionClass())
    return False;

#ifdef RECURS_BUG
  if (state & Realizing)
    return True;
#endif

#ifdef RECURS_BUG
  ((CollectionClass *)this)->state |= Realizing;
#endif

  Bool cl_isref;
  eyedblib::int16 cl_dim, cl_item_size;
  const Class *cl_collclass = cl->asCollectionClass()->
    getCollClass(&cl_isref, &cl_dim, &cl_item_size);

  if (cl_isref != isref || cl_dim != dim || cl_item_size != item_size)
    {
#ifdef RECURS_BUG
      ((CollectionClass *)this)->state &= ~Realizing;
#endif
      return False;
    }

  Bool r = coll_class->compare(cl_collclass, compClassOwner,
			       compNum, compName, inDepth);
#ifdef RECURS_BUG
  ((CollectionClass *)this)->state &= ~Realizing;
#endif
  return r;
}

Status
CollectionClass::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating collection_class '%s'", name);

  IDB_CHECK_WRITE(db);
  /*
  printf("creating COLLCLASS %s cl_oid %s %s\n", name, cl_oid.toString(),
	 coll_class->getName());
  */
  RPCStatus rpc_status;
  Size alloc_size;
  Offset offset, cl_oid_offset;
  ObjectHeader hdr;
  Status status;

  alloc_size = 0;
  //idr->idr = 0;
  idr->setIDR((Size)0);
  Data data = 0;

  /*
  offset = IDB_CLASS_MAG_ORDER;
  int32_code (&data, &offset, &alloc_size, (eyedblib::int32 *)&mag_order);
  */
  offset = IDB_CLASS_IMPL_TYPE;
  status = IndexImpl::code(data, offset, alloc_size, idximpl);
  if (status) return status;

  offset = IDB_CLASS_MTYPE;
  eyedblib::int32 mt = m_type;
  int32_code (&data, &offset, &alloc_size, &mt);

  offset = IDB_CLASS_DSPID;
  eyedblib::int16 dspid = get_instdspid();
  int16_code (&data, &offset, &alloc_size, &dspid);

  offset = IDB_CLASS_HEAD_SIZE;
  
  if (!cl_oid.isValid())
    {
      coll_class = db->getSchema()->getClass(coll_class->getName());
      if (!coll_class)
	return Exception::make(IDB_ERROR, "creating collection_class '%s'",
				    name);
      cl_oid = coll_class->getOid();
      if (!cl_oid.isValid() && !db->isBackEnd())
	{
	  Status s = coll_class->create();
	  if (s)
	    return s;
	  cl_oid = coll_class->getOid();
	}
    }

  // to avoid recursion!
  if (oid.isValid())
    return Success;

  status = class_name_code(db->getDbHandle(), getDataspaceID(), &data, &offset,
			       &alloc_size, name);
  if (status) return status;

  cl_oid_offset = offset;
  oid_code   (&data, &offset, &alloc_size, cl_oid.getOid());
  /* isref */
  char kc = (char)isref;
  char_code (&data, &offset, &alloc_size, &kc);

  /* dim */
  int16_code (&data, &offset, &alloc_size, &dim);

  int idr_sz = offset;
  idr->setIDR(idr_sz, data);
  headerCode(type, idr_sz);

  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());
  
  if (rpc_status == RPCSuccess && !cl_oid.isValid() && strcmp(coll_class->getName(), "object"))
    {
      Status status;
      
      if (status = coll_class->setDatabase(db))
	return status;
      
      if (status = coll_class->create())
	return status;
      
      cl_oid = coll_class->getOid();
      offset = cl_oid_offset;
      oid_code   (&data, &offset, &alloc_size, cl_oid.getOid());
      rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());
    }

  if (rpc_status == RPCSuccess)
    gbx_locked = gbxTrue;

  return StatusMake(rpc_status);
}

void
CollectionClass::invalidateCollClassOid()
{
  //printf("found coll class oid: %s\n", name);
  cl_oid.invalidate();
  touch();
}

Status
CollectionClass::update()
{
  /*
  printf("updating collclass %s %s modify=%d\n", name, cl_oid.toString(),
	 modify);
  */

  if (cl_oid.isValid() && !modify)
    return Success;

  Offset offset = 0;
  Size alloc_size = sizeof(eyedblib::int16);
  unsigned char data[sizeof(eyedblib::int16)];
  unsigned char *pdata = data;

  eyedblib::int16 dspid = get_instdspid();
  int16_code (&pdata, &offset, &alloc_size, &dspid);

  //printf("setting dspid to %d\n", dspid);
  offset = IDB_CLASS_DSPID;
  RPCStatus rpc_status = 
    dataWrite(db->getDbHandle(), offset, sizeof(eyedblib::int16),
		  data, oid.getOid());
  if (rpc_status) return StatusMake(rpc_status);

  if (cl_oid.isValid())
    return Success;

  if (!coll_class) {
    Status s = wholeComplete();
    if (s) return s;
    if (!coll_class)
      return Exception::make(IDB_ERROR, "updating collection_class '%s'",
			     name);
  }

  string clsName = coll_class->getName();
  coll_class = db->getSchema()->getClass(coll_class->getName());
  if (!coll_class)
    return Exception::make(IDB_ERROR, "updating collection_class '%s' [class '%s']",
			      name, clsName.c_str());

  if (!coll_class->getOid().isValid())
    {
      //printf("ok we create now %s\n", coll_class->getName());
      Status status;
      if (status = coll_class->setDatabase(db))
	return status;
      
      if (status = coll_class->create())
	return status;
    }

  cl_oid = coll_class->getOid();

  offset = IDB_CLASS_HEAD_SIZE + IDB_CLASS_NAME_TOTAL_LEN;

#ifdef E_XDR
  eyedbsm::Oid oid_x;
  eyedbsm::h2x_oid(&oid_x, cl_oid.getOid());
#else
  eyedbsm::Oid oid_x = *cl_oid.getOid();
#endif
  rpc_status = 
    dataWrite(db->getDbHandle(), offset, sizeof(eyedbsm::Oid),
		  (Data)&oid_x, oid.getOid());
  return StatusMake(rpc_status);
}

Status CollectionClass::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status CollectionClass::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  char *indent_str = make_indent(indent);

  if (state & Tracing)
    {
      fprintf(fd, "%s%s;\n", indent_str, oid.getString());
      delete_indent(indent_str);
      return Success;
    }

  Status status = Success;
  char *lastindent_str = make_indent(indent - INDENT_INC);

  const_cast<CollectionClass *>(this)->state |= Tracing;

  fprintf(fd, "%s%s", lastindent_str, name);

  const Class *p = getParent();
  while (p)
    {
      fprintf(fd, " : %s", p->getName());
      p = p->getParent();
    }

  fprintf(fd, " { ");
  status = trace_common(fd, indent, flags, rcm);
  if (status) goto out;
  
  if (dim > 1)
    fprintf(fd, "%sclass = \"%s[%d]\";\n", indent_str,
	    coll_class->getName(), dim);
  else
    fprintf(fd, "%scoll_class = \"%s%s\";\n", indent_str,
	    coll_class->getName(), (isref ? "*" : ""));

  fprintf(fd, "%s};\n", lastindent_str);

 out:
  delete_indent(indent_str);
  delete_indent(lastindent_str);
  const_cast<CollectionClass *>(this)->state &= ~Tracing;
  return status;
}

Status
CollectionClass::remove(const RecMode*rcm)
{
  return Class::remove(rcm);
}

Class *CollectionClass::getCollClass(Bool *_isref, eyedblib::int16 *_dim,
				     eyedblib::int16 *_item_size)
{
  if (_isref)
    *_isref = isref;

  if (_dim)
    *_dim = dim;

  if (_item_size)
    *_item_size = item_size;

  return coll_class;
}

const Class *
CollectionClass::getCollClass(Bool *_isref, eyedblib::int16 *_dim,
				     eyedblib::int16 *_item_size) const
{
  if (_isref)
    *_isref = isref;

  if (_dim)
    *_dim = dim;

  if (_item_size)
    *_item_size = item_size;

  return coll_class;
}

Status
CollectionClass::make(Database *db, Class **cls)
{
  assert(*cls);

  Class *cl;
  if (cl = db->getSchema()->getClass((*cls)->getName())) {
    *cls = cl;
    return Success;
  }
  
  if ((*cls)->isUnrealizable()) {
    int type = (*cls)->get_Type();
    CollectionClass *mcoll = (CollectionClass *)*cls;
    if (mcoll->dim == 1) {
      switch(type) {
      case _CollSetClass_Type:
	*cls = new CollSetClass(mcoll->coll_class, mcoll->isref);
	break;

      case _CollBagClass_Type:
	*cls = new CollBagClass(mcoll->coll_class, mcoll->isref);
	break;

      case _CollArrayClass_Type:
	*cls = new CollArrayClass(mcoll->coll_class, mcoll->isref);
	break;

      case _CollListClass_Type:
	*cls = new CollListClass(mcoll->coll_class, mcoll->isref);
	break;

      default:
	assert(0);
      }
    }
    else {
      switch(type) {
      case _CollSetClass_Type:
	*cls = new CollSetClass(mcoll->coll_class, mcoll->dim);
	break;

      case _CollBagClass_Type:
	*cls = new CollBagClass(mcoll->coll_class, mcoll->dim);
	break;

      case _CollArrayClass_Type:
	*cls = new CollArrayClass(mcoll->coll_class, mcoll->dim);
	break;

      case _CollListClass_Type:
	*cls = new CollListClass(mcoll->coll_class, mcoll->dim);
	break;

      default:
	assert(0);
      }
    }
  }

  Status status;
  status = (*cls)->setDatabase(db);

  if (status)
    return status;

  ClassPeer::setMType(*cls, Class::System);

  status = db->getSchema()->addClass(*cls);
  return status;
}

LinkedList *CollectionClass::mcoll_list;

void
CollectionClass::init()
{
  mcoll_list = new LinkedList();
}

struct CollClassLink {
  char *name;
  CollectionClass *coll;

  CollClassLink(const char *_name, CollectionClass *_coll) {
    name = strdup(_name);
    coll = _coll;
  }

  ~CollClassLink() {
    free(name);
    coll->release();
  }
};

void
CollectionClass::_release()
{
  CollClassLink *l;

  LinkedListCursor c(mcoll_list);

  while (mcoll_list->getNextObject(&c, (void *&)l))
    delete l;

  delete mcoll_list;
}

void
CollectionClass::set(const char *prefix, Class *coll_class,
			Bool isref, int dim, CollectionClass *coll)
{
  const char *_name = make_name(prefix, coll_class, isref, dim, False);

  ObjectPeer::setUnrealizable(coll, True);
  mcoll_list->insertObjectLast(new CollClassLink(_name, coll));
  //printf("CollectionClass::set(%s in list)\n", _name);
}

int
CollectionClass::genODL(FILE *fd, Schema *) const
{
  return 0;
}

CollectionClass *
CollectionClass::get(const char *prefix, Class *coll_class,
			Bool isref, int dim)
{
  const char *name = make_name(prefix, coll_class, isref, dim, False);
  Status status;

  CollClassLink *l;

  LinkedListCursor c(mcoll_list);
  while (c.getNext((void *&)l))
    if (!strcmp(l->name, name))
      return l->coll;

  //printf("CollectionClass::get(%s) -> NOT FOUND\n", name);
  return 0;
}

//
// bag_class
//

CollectionClass *
CollBagClass::make(Class *coll_class, Bool isref, int dim, Status &status)
{
  status = 0;
  CollectionClass *coll;
  if (coll = get("bag", coll_class, isref, dim))
    return coll;

  if (dim > 1)
    coll = new CollBagClass(coll_class, dim);
  else
    coll = new CollBagClass(coll_class, isref);

  if (coll->getStatus()) {
    status = coll->getStatus();
    return 0;
  }

  set("bag", coll_class, isref, dim, coll);
  return coll;
}

CollBagClass::CollBagClass(Class *_coll_class, Bool _isref) :
CollectionClass(_coll_class, _isref, "bag")
{
  type = _CollBagClass_Type;
  setClass(CollBagClass_Class);
  parent = CollBag_Class;
}

CollBagClass::CollBagClass(Class *_coll_class, int _dim) :
CollectionClass(_coll_class, _dim, "bag")
{
  type = _CollBagClass_Type;
  setClass(CollBagClass_Class);
  parent = CollBag_Class;
}

CollBagClass::CollBagClass(const CollBagClass &cl)
  : CollectionClass(cl)
{
}

CollBagClass::CollBagClass(const Oid &_oid, const char *_name) :
CollectionClass(_oid, _name)
{
  type = _CollBagClass_Type;
}

CollBagClass& CollBagClass::operator=(const CollBagClass &cl)
{
  this->CollectionClass::operator=(cl);
  return *this;
}

Object *CollBagClass::newObj(Database *_db) const
{
  CollBag *t;

  if (isref)  t = new CollBag(_db, "", coll_class, True);
  else        t = new CollBag(_db, "", coll_class, dim );

  ObjectPeer::make(t, this, 0, _CollBag_Type, idr_objsz,
		      idr_psize, idr_vsize);
  return t;
}

Object *CollBagClass::newObj(Data data, Bool _copy) const
{
  CollBag *t;

  if (isref)  t = new CollBag("", coll_class, True);
  else        t = new CollBag("", coll_class, dim );

  ObjectPeer::make(t, this, data, _CollBag_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  return t;
}

//
// array_class
//

CollectionClass *
CollArrayClass::make(Class *coll_class, Bool isref, int dim, Status &status)
{
  status = 0;
  CollectionClass *coll;
  if (coll = get("array", coll_class, isref, dim))
    return coll;

  if (dim > 1)
    coll = new CollArrayClass(coll_class, dim);
  else
    coll = new CollArrayClass(coll_class, isref);

  if (coll->getStatus()) {
    status = coll->getStatus();
    return 0;
  }

  set("array", coll_class, isref, dim, coll);
  return coll;
}

CollArrayClass::CollArrayClass(Class *_coll_class, Bool _isref) :
CollectionClass(_coll_class, _isref, "array")
{
  type = _CollArrayClass_Type;
  setClass(CollArrayClass_Class);
  parent = CollArray_Class;
}

CollArrayClass::CollArrayClass(Class *_coll_class, int _dim) :
CollectionClass(_coll_class, _dim, "array")
{
  type = _CollArrayClass_Type;
  setClass(CollArrayClass_Class);
  parent = CollArray_Class;
}

CollArrayClass::CollArrayClass(const CollArrayClass &cl)
  : CollectionClass(cl)
{
}

CollArrayClass::CollArrayClass(const Oid &_oid, const char *_name) :
CollectionClass(_oid, _name)
{
  type = _CollArrayClass_Type;
}

CollArrayClass& CollArrayClass::operator=(const CollArrayClass &cl)
{
  this->CollectionClass::operator=(cl);
  return *this;
}

Object *CollArrayClass::newObj(Database *_db) const
{
  CollArray *t;

  if (isref)  t = new CollArray(_db, "", coll_class, True);
  else        t = new CollArray(_db, "", coll_class, dim );

  ObjectPeer::make(t, this, 0, _CollArray_Type, idr_objsz,
		      idr_psize, idr_vsize);
  return t;
}

Object *CollArrayClass::newObj(Data data, Bool _copy) const
{
  CollArray *t;

  if (isref)  t = new CollArray("", coll_class, True);
  else        t = new CollArray("", coll_class, dim );

  ObjectPeer::make(t, this, data, _CollArray_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  return t;
}

//
// set_class
//

CollectionClass *
CollSetClass::make(Class *coll_class, Bool isref, int dim, Status &status)
{
  status = 0;
  CollectionClass *coll;
  if (coll = get("set", coll_class, isref, dim))
    return coll;

  if (dim > 1)
    coll = new CollSetClass(coll_class, dim);
  else
    coll = new CollSetClass(coll_class, isref);

  if (coll->getStatus()) {
    status = coll->getStatus();
    return 0;
  }

  set("set", coll_class, isref, dim, coll);
  return coll;
}

CollSetClass::CollSetClass(Class *_coll_class, Bool _isref) :
CollectionClass(_coll_class, _isref, "set")
{
  type = _CollSetClass_Type;
  setClass(CollSetClass_Class);
  parent = CollSet_Class;
}

CollSetClass::CollSetClass(Class *_coll_class, int _dim) :
CollectionClass(_coll_class, _dim, "set")
{
  type = _CollSetClass_Type;
  setClass(CollSetClass_Class);
  parent = CollSet_Class;
}

CollSetClass::CollSetClass(const CollSetClass &cl)
  : CollectionClass(cl)
{
}

CollSetClass::CollSetClass(const Oid &_oid, const char *_name) :
CollectionClass(_oid, _name)
{
  type = _CollSetClass_Type;
}

CollSetClass& CollSetClass::operator=(const CollSetClass &cl)
{
  this->CollectionClass::operator=(cl);
  return *this;
}

Object *CollSetClass::newObj(Database *_db) const
{
  CollSet *t;

  if (isref)  t = new CollSet(_db, "", coll_class, True);
  else        t = new CollSet(_db, "", coll_class, dim );

  ObjectPeer::make(t, this, 0, _CollSet_Type, idr_objsz,
		      idr_psize, idr_vsize);
  return t;
}

Object *CollSetClass::newObj(Data data, Bool _copy) const
{
  CollSet *t;

  if (isref)  t = new CollSet("", coll_class, True);
  else        t = new CollSet("", coll_class, dim );

  ObjectPeer::make(t, this, data, _CollSet_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  return t;
}

//
// list_class
//

CollectionClass *
CollListClass::make(Class *coll_class, Bool isref, int dim, Status &status)
{
  status = 0;
  CollectionClass *coll;
  if (coll = get("list", coll_class, isref, dim))
    return coll;

  if (dim > 1)
    coll = new CollListClass(coll_class, dim);
  else
    coll = new CollListClass(coll_class, isref);

  if (coll->getStatus()) {
    status = coll->getStatus();
    return 0;
  }

  set("list", coll_class, isref, dim, coll);
  return coll;
}

CollListClass::CollListClass(Class *_coll_class, Bool _isref) :
CollectionClass(_coll_class, _isref, "list")
{
  type = _CollListClass_Type;
  setClass(CollListClass_Class);
  parent = CollList_Class;
}

CollListClass::CollListClass(Class *_coll_class, int _dim) :
CollectionClass(_coll_class, _dim, "list")
{
  type = _CollListClass_Type;
  setClass(CollListClass_Class);
  parent = CollList_Class;
}

CollListClass::CollListClass(const CollListClass &cl)
  : CollectionClass(cl)
{
}

CollListClass::CollListClass(const Oid &_oid, const char *_name) :
CollectionClass(_oid, _name)
{
  type = _CollListClass_Type;
}

CollListClass& CollListClass::operator=(const CollListClass &cl)
{
  this->CollectionClass::operator=(cl);
  return *this;
}

Object *CollListClass::newObj(Database *_db) const
{
  CollList *t;

  if (isref)  t = new CollList(_db, "", coll_class, True);
  else        t = new CollList(_db, "", coll_class, dim );

  ObjectPeer::make(t, this, 0, _CollList_Type, idr_objsz,
		      idr_psize, idr_vsize);
  return t;
}

Object *CollListClass::newObj(Data data, Bool _copy) const
{
  CollList *t;

  if (isref)  t = new CollList("", coll_class, True);
  else        t = new CollList("", coll_class, dim );

  ObjectPeer::make(t, this, data, _CollList_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  return t;
}

Status
CollectionClass::setName(const char *s)
{
  return setNameRealize(s);
}

//
// make function
//

Status
collectionClassMake(Database *db, const Oid *oid, Object **o,
		    const RecMode *rcm, const ObjectHeader *hdr,
		    Data idr, LockMode lockmode, const Class*)
{
  RPCStatus rpc_status;
  Status status;
  Data temp;

  if (!idr)
    {
      temp = (unsigned char *)malloc(hdr->size);
      object_header_code_head(temp, hdr);

      rpc_status = objectRead(db->getDbHandle(), temp, 0, 0, oid->getOid(),
				  0, lockmode, 0);
    }
  else
    {
      temp = idr;
      rpc_status = RPCSuccess;
    }

  if (rpc_status != RPCSuccess)
    return StatusMake(rpc_status);

  if (hdr && (hdr->xinfo & IDB_XINFO_REMOVED))
    return Exception::make(IDB_INTERNAL_ERROR,
			   "collection class %s is removed", oid->toString());

  char *s;
  Offset offset;

  /*
  eyedblib::int32 mag_order;
  offset = IDB_CLASS_MAG_ORDER;
  int32_decode (temp, &offset, &mag_order);
  */

  IndexImpl *idximpl;
  offset = IDB_CLASS_IMPL_TYPE;
  status = IndexImpl::decode(db, temp, offset, idximpl);
  if (status) return status;

  eyedblib::int32 mt;
  offset = IDB_CLASS_MTYPE;
  int32_decode (temp, &offset, &mt);

  eyedblib::int16 dspid;
  offset = IDB_CLASS_DSPID;
  int16_decode (temp, &offset, &dspid);

  offset = IDB_CLASS_HEAD_SIZE;

  status = class_name_decode(db->getDbHandle(), temp, &offset, &s);
  if (status) return status;

  CollectionClass *mcoll;

  eyedbsm::Oid _cl_oid;
  oid_decode(temp, &offset, &_cl_oid);
  Oid cl_oid(_cl_oid);

  Class *coll_class;

  coll_class = db->getSchema()->getClass(cl_oid, True);

  if (!coll_class)
    coll_class = Object_Class;

  char isref;
  char_decode(temp, &offset, &isref);
  eyedblib::int16 dim;
  int16_decode(temp, &offset, &dim);

  switch(hdr->type)
    {
    case _CollSetClass_Type:
      if (dim > 1)
	mcoll = new CollSetClass(coll_class, (int)dim);
      else
	mcoll = new CollSetClass(coll_class, (Bool)isref);
      break;

    case _CollBagClass_Type:
      if (dim > 1)
	mcoll = new CollBagClass(coll_class, (int)dim);
      else
	mcoll = new CollBagClass(coll_class, (Bool)isref);
      break;

    case _CollArrayClass_Type:
      if (dim > 1)
	mcoll = new CollArrayClass(coll_class, (int)dim);
      else
	mcoll = new CollArrayClass(coll_class, (Bool)isref);
      break;

    case _CollListClass_Type:
      if (dim > 1)
	mcoll = new CollListClass(coll_class, (int)dim);
      else
	mcoll = new CollListClass(coll_class, (Bool)isref);
      break;

    default:
      abort();
    }

  mcoll->setExtentImplementation(idximpl, True);
  if (idximpl)
    idximpl->release();
  //mcoll->setMagOrder(mag_order);
  mcoll->setInstanceDspid(dspid);

  Bool addedClass = False;
  if (!db->getSchema()->getClass(*oid))
    {
      ObjectPeer::setOid(mcoll, *oid);
      db->getSchema()->addClass_nocheck(mcoll, True);
      addedClass = True;
    }

  Class *cl = NULL;
  Bool classAdded = False;
  if (!db->isOpeningState() && !db->isBackEnd())
    {
      status = mcoll->setDatabase(db);
      if (status)
	return status;
    }
  else
    {
      /* changes about the 18/10/97 */
      /* THIS MAKE A BIG BORDEL! */
      Exception::Mode mode = Exception::setMode(Exception::StatusMode);
      void (*handler)(Status, void *) = Exception::getHandler();
      Exception::setHandler(NULL);

      Exception::setHandler(handler);
      Exception::setMode(mode);
    }

  status = ClassPeer::makeColls(db, mcoll, temp);

  if (addedClass)
    db->getSchema()->suppressClass(mcoll);

  *o = (Object *)mcoll;

  if (!idr)
    {
      if (!status)
	ObjectPeer::setIDR(*o, temp, hdr->size);
    }

  free(s); s = 0;
  return status;
}

#include "CollectionBE.h"

Status classCollectionMake(Database *db, const Oid &colloid,
				      Collection **coll)
{
  if (!colloid.isValid())
    {
      *coll = 0;
      return Success;
    }

  ObjectHeader hdr;

  RPCStatus rpc_status;

  rpc_status = objectHeaderRead(db->getDbHandle(), colloid.getOid(), &hdr);

  if (rpc_status != RPCSuccess)
    return StatusMake(rpc_status);

  return collectionMake(db, &colloid, (Object **)coll, 0, &hdr, 0, 
			    DefaultLock, 0);
}
}
