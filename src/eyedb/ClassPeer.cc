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

namespace eyedb {

void ClassPeer::setParent(Class *cl, const Class *parent)
{
  cl->parent = (Class *)parent;
}

Status
ClassPeer::makeColl(Database *db, Collection **pcoll,
		       Data idr, Offset offset)
{
  if (*pcoll)
    return Success;

  GBX_SUSPEND();

  *pcoll = (Collection *)1; // to avoid recurs

  eyedbsm::Oid _oid;

  oid_decode(idr, &offset, &_oid);

  Oid colloid(_oid);
  Status status;

  status = classCollectionMake(db, colloid, pcoll);

  if (status)
    {
      *pcoll = 0;
      return status;
    }

  if (!*pcoll)
    return status;

  if (status == Success)
    {
      status = (*pcoll)->setDatabase(db);
      if (status)
	return status;
      ObjectPeer::setOid(*pcoll, colloid);
    }

  if (status)
    *pcoll = 0;

  return status;
}

Status
ClassPeer::makeColls(Database *db, Class *mc,
			Data idr, Bool make_extent)
{
  Status status;

  if (make_extent &&
      (status = makeColl(db, &mc->extent, idr, IDB_CLASS_EXTENT)))
    return status;

  return makeColl(db, &mc->components, idr, IDB_CLASS_COMPONENTS);
}

Status
ClassPeer::makeColls(Database *db, Class *mc,
			Data idr, const Oid *oid)
{
  eyedbsm::Oid _oid[IDB_CLASS_COLLS_CNT];
  RPCStatus rpc_status;

  rpc_status = dataRead(db->getDbHandle(), IDB_CLASS_EXTENT,
			    IDB_CLASS_COLLS_CNT*sizeof(eyedbsm::Oid),
			    (Data)_oid, 0, oid->getOid());

  if (rpc_status == RPCSuccess)
    {
      Offset offset = IDB_CLASS_COLL_START;
      Size alloc_size = offset + IDB_CLASS_COLLS_CNT * sizeof(eyedbsm::Oid);

#ifdef E_XDR
      for (int i = 0; i < IDB_CLASS_COLLS_CNT; i++)
	buffer_code(&idr, &offset, &alloc_size,
			(unsigned char *)&_oid[i], sizeof(eyedbsm::Oid));
#else
      for (int i = 0; i < IDB_CLASS_COLLS_CNT; i++)
	oid_code(&idr, &offset, &alloc_size, &_oid[i]);
#endif

      return ClassPeer::makeColls(db, mc, idr, True);
    }

  return StatusMake(rpc_status);
}

void ClassPeer::newObjRealize(Class *cls, Object *o)
{
  cls->newObjRealize(o);
}

void ClassPeer::setMType(Class *cls,
				 Class::MType m_type)
{
  cls->m_type = m_type;
}

void ClassPeer::setParent(Class *cls, Class *parent)
{
  cls->parent = parent;
}

}
