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
#include "IteratorBE.h"
#include "CollectionBE.h"
#include <assert.h>
#include "Attribute_p.h"
#include "eyedb/gbxcyctx.h"

//
// Agregat methods
//


namespace eyedb {

Status Agregat::checkAgreg(const Attribute *agreg) const
{
  unsigned int num;
  const Attribute **items = ((AgregatClass *)getClass())->getAttributes(num);

  if (!agreg)
    return Exception::make(IDB_ATTRIBUTE_ERROR, "invalid null attribute in agregat class '%s'", getClass()->getName());

  if (agreg->num >= num || agreg != items[agreg->num])
    return Exception::make(IDB_ATTRIBUTE_ERROR, "attribute '%s' [%d vs %d] is not valid for agregat class '%s'",
			   agreg->name, agreg->num, num, getClass()->getName());

  return Success;
}

  Status Agregat::setItemOid(const Attribute *agreg, const Oid *poid, int nb, int from, Bool check_class)
{
  Status status;
#if 1
  Oid roid;
  status = agreg->getOid(this, &roid, nb, from);
  if (status) {
    return status;
  }
  if (roid == *poid) {
    return Success;
  }
#endif
  status = agreg->setOid(this, poid, nb, from, check_class);

  if (status == Success) {
    modify = True;
  }

  return status;
}

Status Agregat::getItemOid(const Attribute *agreg, Oid *poid, int nb, int from) const
{
  Status status = checkAgreg(agreg);

  if (status) {
    return status;
  }

  return agreg->getOid(this, poid, nb, from);
}

Status Agregat::setItemValue(const Attribute* agreg, Data data,
			     int nb, int from)
{
  Status status = checkAgreg(agreg);

  if (status)
    return status;

  status = agreg->setValue(this, data, nb, from);
#if 1
  if (status == Success)
    {
      if (((AgregatClass *)getClass())->asUnionClass())
	((Union *)this)->setCurrentItem(agreg);
      modify = True;
    }
#endif
  return status;
}

Status Agregat::setItemSize(const Attribute* agreg, Size size)
{
  Status status = checkAgreg(agreg);

  if (status)
    return status;

  return agreg->setSize(this, size);
}

Status Agregat::getItemSize(const Attribute* agreg, Size* psize) const
{
  Status status = checkAgreg(agreg);

  if (status)
    return status;

  return agreg->getSize(this, *psize);
}

Status Agregat::trace_realize(FILE *fd, int indent, unsigned int flags,
			      const RecMode *rcm) const
{
  IDB_CHECK_INTR();
  AgregatClass *ma = (AgregatClass *)getClass();
  Status status = Success;
  int j;
  char *indent_str = make_indent(indent);
  char *lastindent_str = make_indent(indent-INDENT_INC);

  fprintf(fd, "{ ");
  trace_flags(fd, flags);
  fprintf(fd, "\n");

  if (state & Tracing)
    {
      fprintf(fd, "%s<trace cycle>\n", indent_str);
      fprintf(fd, "%s};\n", lastindent_str);
      delete_indent(indent_str);
      delete_indent(lastindent_str);
      return Success;
    }

  const_cast<Agregat *>(this)->state |= Tracing;

  if (traceRemoved(fd, indent_str))
    goto out;
 
  if (!getClass()->isAttrsComplete())
    {
      Status status = const_cast<Class*>(getClass())->attrsComplete();
      if (status) goto out;
    }

  if (((AgregatClass *)getClass())->asUnionClass())
    {
      const Attribute *item;
      if ((item = ((const Union *)this)->getCurrentItem()) >= 0)
	status = item->trace(this, fd, &indent, flags, rcm);
    }
  else
    {
      unsigned int items_cnt;
      int isnat = ((flags & NativeTrace) == NativeTrace);
      const Attribute **items = getClass()->getAttributes(items_cnt);
      for (int n = 0; n < items_cnt; n++)
	if (!items[n]->isNative() || isnat)
	  {
	    if ((status = items[n]->trace(this, fd, &indent, flags, rcm)) != Success)
	      break;
	  }
    }

 out:
  const_cast<Agregat *>(this)->state &= ~Tracing;
  fprintf(fd, "%s};\n", lastindent_str);
  delete_indent(indent_str);
  delete_indent(lastindent_str);

  return status;
}

Status Agregat::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s {%s} = ", getClass()->getName(), oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Agregat::create_realize(Bool realizing)
{
  if (!getClass())
    return Exception::make(IDB_NO_CLASS);

  if (!getClass()->getOid().isValid())
    return Exception::make(IDB_CLASS_NOT_CREATED, "creating agregat of class '%s'", getClass()->getName());

  classOidCode();

  if (!realizing)
    {
      RPCStatus rpc_status;

      rpc_status = oidMake(db->getDbHandle(), getDataspaceID(),
			       idr->getIDR(), idr->getSize(),
			       oid.getOid());

      return StatusMake(rpc_status);
    }

  modify = True;
  return Success;
}

Status Agregat::create()
{
  fprintf(stderr, "!cannot use create method!\n");
  abort();
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating agregat of class '%s'", getClass()->getName());

  return create_realize(False);
}

Status Agregat::update_realize(Bool realizing)
{
  if (!getClass()->getOid().isValid())
    return Exception::make(IDB_CLASS_NOT_CREATED, "updating agregat of class '%s'", getClass()->getName());

#if 1
  classOidCode(); // added the 28/08/01
#endif

  if (!realizing)
    {
      RPCStatus rpc_status;
      rpc_status = objectWrite(db->getDbHandle(), idr->getIDR(), oid.getOid());

      if (rpc_status != RPCSuccess)
	return StatusMake(rpc_status);

    }
  
  return Success;
}

Status Agregat::update()
{
  if (!oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "updating agregat of class '%s'", getClass()->getName());

  return update_realize(False);
}

  //#define REALIZE_TRACE
#ifdef REALIZE_TRACE
static std::string INDENT = "";
#endif

Status Agregat::realize(const RecMode *rcm)
{
  //printf("Agregat::realize(%p %s)\n", this, getClass()->getName());
  if (state & Realizing) {
#ifdef REALIZE_TRACE
    printf("%s[Agregat is realizing(%p: %s)]\n", (INDENT + "  ").c_str(), this, getClass()->getName());
#endif
    return Success;
  }

#ifdef REALIZE_TRACE
  std::string s_indent = INDENT;
  INDENT += "  ";
  printf("%sAgregat::realize(%p: %s)\n", INDENT.c_str(), this, getClass()->getName());
#endif

  CHK_OBJ(this);

  IDB_CHECK_WRITE(db);

  Status status;

  if (getMasterObject(true)) {
    return getMasterObject(true)->realize(rcm);
  }

  Bool creating;

  state |= Realizing;

  if (db->isRealized(this)) {
    return Success;
  }

  if (!oid.isValid()) {
    status = create_realize(False);
    creating = True;
  }
  else {
    status = update_realize(True);
    creating = False;
  }

  if (!status) {
    AttrIdxContext idx_ctx;
    status = realizePerform(getClass()->getOid(), getOid(), idx_ctx, rcm);

    if (!status) {
      if (creating) {
	status = StatusMake
	  (objectCreate(db->getDbHandle(), getDataspaceID(),
			idr->getIDR(), oid.getOid()));
	if (!status)
	  modify = False;
      }
      else if (modify) {
	status = StatusMake
	  (objectWrite(db->getDbHandle(),
		       idr->getIDR(), oid.getOid()));
	if (!status)
	  modify = False;
      }
    }
  }

  // added the 17/05/99 because of cache incohency
  //if (status && isDirty())
  if (status) { // changed the 2/05/99 because of index incoherency
    int err = status->getStatus();
    if (err != IDB_UNIQUE_CONSTRAINT_ERROR &&
	err != IDB_UNIQUE_COMP_CONSTRAINT_ERROR &&
	err != IDB_NOTNULL_CONSTRAINT_ERROR &&
	err != IDB_NOTNULL_COMP_CONSTRAINT_ERROR) {
      db->setIncoherency();
      db->uncacheObject(this);
      std::string str = status->getString();
      status = Exception::make(status->getStatus(),
			       str + ": the current transaction must "
			       "be aborted");
    }
  }
  else if (creating) {
    db->cacheObject(this);
  }

  state &= ~Realizing;

  db->setRealized(this);

#ifdef REALIZE_TRACE
  INDENT = s_indent;
#endif
  return status;
}

Status Agregat::realizePerform(const Oid& cloid,
			       const Oid& objoid,
			       AttrIdxContext &idx_ctx,
			       const RecMode *rcm)
{
  CHK_OBJ(this);

  Status status;
  unsigned int items_cnt;
  const Attribute **items;

  items = getClass()->getAttributes(items_cnt);
  
  for (int i = 0; i < items_cnt; i++)
    if (status = items[i]->realize(db, this, cloid, objoid, idx_ctx, rcm))
      return status;

  return Success;
}

void
Agregat::manageCycle(gbxCycleContext &r)
{
  if (r.isCycle()) {
#ifdef MANAGE_CYCLE_TRACE
    printf("found #1 premature cycle for %p\n", this);
#endif
    return;
  }

  if (gbx_chgRefCnt) {
    r.manageCycle(this);
    return;
  }

  if (!getClass())
    return;

  gbx_chgRefCnt = gbxTrue;

  unsigned int items_cnt;
  const Attribute **items;
  items = getClass()->getAttributes(items_cnt);

  for (int i = 0; i < items_cnt; i++)
    items[i]->manageCycle(db, this, r);

  gbx_chgRefCnt = gbxFalse;
  return;
}

Status Agregat::removePerform(const Oid& cloid, 
			      const Oid& objoid,
			      AttrIdxContext &idx_ctx,
			      const RecMode *rcm)
{
  if (((AgregatClass *)getClass())->asUnionClass())
    {
      Attribute *item = (Attribute *)((Union *)this)->getCurrentItem();
      if (item)
	return item->remove(db, this, cloid, objoid, idx_ctx, rcm);
      return Success;
    }

  Status status;
  unsigned int items_cnt;
  const Attribute **items;
  
  items = ((AgregatClass *)getClass())->getAttributes(items_cnt);
  
  for (int i = 0; i < items_cnt; i++)
    if ((status = items[i]->remove(db, this, cloid, objoid, idx_ctx, rcm)) !=
	Success &&
	status->getStatus() != IDB_OBJECT_NOT_CREATED)
      return status;
  
  return Success;
}

Status Agregat::remove(const RecMode *rcm)
{
  if (!oid.isValid())
    return Exception::make(IDB_OBJECT_NOT_CREATED,
			   "removing agregat of class '%s'",
			   getClass()->getName());

  if (state & Removing)
    return Success;

  Status status;
  state |= Removing;

  AttrIdxContext idx_ctx;
  status = removePerform(getClass()->getOid(), getOid(), idx_ctx, rcm);

  if (status == Success)
    status = Object::remove(rcm);

  if (status) // changed the 9/07/99 because of index incoherency
    {
      db->setIncoherency();
      db->uncacheObject(this);
      std::string str = status->getString();
      status = Exception::make(status->getStatus(),
			       str + ": the current transaction must "
			       "be aborted");
    }

  state &= ~Removing;
  return status;
}

void Agregat::initialize(Database *_db)
{
  Instance::initialize(_db);
}

Agregat::Agregat(Database *_db, const Dataspace *_dataspace) :
  Instance(_db, _dataspace)
{
  //  Instance::initialize(_db); // conceptually is Agregat::initialize(_db)
}

Status
Agregat::loadPerform(const Oid& cloid,
		     LockMode lockmode,
		     AttrIdxContext &idx_ctx,
		     const RecMode *rcm)
{
  unsigned int items_cnt;
  const Attribute **items = getClass()->getAttributes(items_cnt);
  
  for (int i = 0; i < items_cnt; i++)
    {
      Status status;
      status = items[i]->load(db, this, cloid, lockmode, idx_ctx, rcm);
      if (status) return status;
    }

  return Success;
}

Status
agregatMake(Database *db, const Oid *oid, Object **o,
		const RecMode *rcm, const ObjectHeader *hdr,
		Data idr, LockMode lockmode, const Class *_class)
{
  RPCStatus rpc_status;

  if (!_class)
    {
      _class = db->getSchema()->getClass(hdr->oid_cl, True);

      if (!_class)
	return Exception::make(IDB_CLASS_NOT_FOUND, "agregat class '%s'",
			       OidGetString(&hdr->oid_cl));
    }

  Status status;
  Agregat *a;
  *o = 0;

  Database::consapp_t consapp = db->getConsApp(_class);
  if (consapp) 
    {
      if (!ObjectPeer::isRemoved(*hdr))
	*o = consapp(_class, idr);
      else
	*o = consapp(_class, 0);
    }
  
  if (!*o)
    {
      if (idr && !ObjectPeer::isRemoved(*hdr))
	*o = _class->newObj(idr + IDB_OBJ_HEAD_SIZE, False);
      else
	*o = _class->newObj();
    }
  
  a = (Agregat *)*o;
  /*
    status = a->setDatabase(db);
    if (status) return status;
  */

  ObjectPeer::setDatabase(*o, db);

  if (idr)
    rpc_status = RPCSuccess;
  else
    rpc_status = objectRead(db->getDbHandle(), a->getIDR(), 0, 0,
				oid->getOid(), 0, lockmode, 0);

  if (rcm->getType() != RecMode_NoRecurs)
    db->insertTempCache(*oid, *o);

  if (!rpc_status)
    {
      AttrIdxContext idx_ctx;
      return a->loadPerform(_class->getOid(), lockmode, idx_ctx, rcm);
    }

  return StatusMake(rpc_status);
}

Status Agregat::getValue(unsigned char**) const
{
  return Success;
}

Status Agregat::setValue(unsigned char*)
{
  return Success;
}

#ifdef GBX_NEW_CYCLE
void Agregat::decrRefCountPropag()
{
  int items_cnt;
  const Attribute **items =
    ((AgregatClass *)getClass())->getAttributes(items_cnt);
  
  for (int i = 0; i < items_cnt; i++)
    items[i]->decrRefCountPropag(this, idr->getRefCount());
}
#endif

void Agregat::garbage()
{
  if (getClass() && getClass()->isValidObject())  {
    if (((AgregatClass *)getClass())->asUnionClass()) {
      Attribute *item = (Attribute *)((Union *)this)->getCurrentItem();
      if (item)
	item->garbage(this, idr->getRefCount());
    }
    else {
      unsigned int items_cnt;
      const Attribute **items =
	((AgregatClass *)getClass())->getAttributes(items_cnt);

      for (int i = 0; i < items_cnt; i++)
	items[i]->garbage(this, idr->getRefCount());
    }
  }

  Instance::garbage();
}

Agregat::~Agregat()
{
  garbageRealize();
}

void
Agregat::copy(const Agregat *o, Bool share)
{
  if (((AgregatClass *)getClass())->asUnionClass()) {
    Attribute *item = (Attribute *)((Union *)this)->getCurrentItem();
    if (item)
      item->copy(this, share);
  }
  else {
    unsigned int items_cnt;
    const Attribute **items =
      ((AgregatClass *)getClass())->getAttributes(items_cnt);
      
    for (int i = 0; i < items_cnt; i++)
      items[i]->copy(this, share);
  }
}

Agregat::Agregat(const Agregat *o, Bool share) : Instance(o, share) {
  copy(o, share);
}

Agregat::Agregat(const Agregat &o) : Instance(o)
{
  copy(&o, False);
}

Agregat& Agregat::operator=(const Agregat &o)
{
  if (&o == this)
    return *this;

  //garbageRealize();

  *(Instance *)this = Instance::operator=((const Instance &)o);

  copy(&o, False);

  return *this;
}
}
