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
//  static void make(Object *, Class *, Data, Size, Type);
  
namespace eyedb {

void ObjectPeer::incrRefCount(Object *o)
{
  o->incrRefCount();
}

void ObjectPeer::decrRefCount(Object *o)
{
  o->decrRefCount();
}

void ObjectPeer::setUnrealizable(Object *o, Bool unreal)
{
  o->setUnrealizable(unreal);
}

void ObjectPeer::setOid(Object *o, const Oid& oid)
{
  o->setOid(oid);
}

void ObjectPeer::setProtOid(Object *o, const Oid& prot_oid)
{
  o->oid_prot = prot_oid;
}

void ObjectPeer::setTimes(Object *o, const ObjectHeader &hdr)
{
  if (hdr.ctime)
    o->c_time = hdr.ctime;
  else
    o->c_time = hdr.mtime;

  o->m_time = hdr.mtime;
}

void ObjectPeer::make(Object *o, const Class *cls,
			 Data data, Type type, Size idr_objsz,
			 Size idr_psize, Size idr_vsize, Bool _copy)
{
  Object::IDR *idr = o->idr;
  o->cls = (Class *)cls;
  o->type   = type;

  if (_copy)
    {
      idr->setIDR(idr_objsz);
      /*
      idr->idr = (unsigned char *)malloc(idr_objsz);
      idr->idr_sz = idr_objsz;
      */

      if (data)
	{
	  memcpy(idr->getIDR() + IDB_OBJ_HEAD_SIZE, data,
		 idr_psize - IDB_OBJ_HEAD_SIZE);
	  memset(idr->getIDR() + idr_psize, 0, idr_vsize);
	}
      else
	memset(idr->getIDR() + IDB_OBJ_HEAD_SIZE, 0,
	       idr->getSize() - IDB_OBJ_HEAD_SIZE);
    }
  else
    {
      //idr->idr_sz = idr_objsz;

      if (data)
	{
	  idr->setIDR(idr_objsz, data - IDB_OBJ_HEAD_SIZE);
	  //idr->idr = data - IDB_OBJ_HEAD_SIZE;
	}
      else
	{
	  //idr->idr = (unsigned char *)malloc(idr_objsz);
	  idr->setIDR(idr_objsz);
	  memset(idr->getIDR() + IDB_OBJ_HEAD_SIZE, 0,
		 idr->getSize() - IDB_OBJ_HEAD_SIZE);
	}
    }

  o->headerCode(type, idr_psize, IDB_XINFO_LOCAL_OBJ);
}

void ObjectPeer::setIDR(Object *o, Data idr, Size idr_sz)
{
  o->idr->setIDR(idr_sz, idr);
  /*
  if (o->idr->idr)
    free(o->idr->idr);
  o->idr->idr = idr;
  o->idr->idr_sz = idr_sz;
  */
}

Status ObjectPeer::trace_realize(const Object *o, FILE *fd, int indent, unsigned int flags, const RecMode *rcm)
{
  return o->trace_realize(fd, indent, flags, rcm);
}

void ObjectPeer::setModify(Object *o, Bool b)
{
  o->modify = b;
}

void ObjectPeer::setClass(Object *o, const Class *cls)
{
  o->setClass(cls);
}

Bool ObjectPeer::isRemoved(const ObjectHeader &hdr)
{
  return (hdr.xinfo & IDB_XINFO_REMOVED) ? True : False;
}

void ObjectPeer::setRemoved(Object *o)
{
  o->removed = True;
}

void
ObjectPeer::loadEpilogue(Object *o, const Oid &oid,
			 const ObjectHeader &hdr, Data o_idr)
{
  //o->setOid(oid);;
  o->oid = oid;
  o->modify = False;
  setTimes(o, hdr);
  o->oid_prot = hdr.oid_prot;

  if (isRemoved(hdr))
    o->removed = True;

  if (!o->idr->getIDR())
    setIDR(o, o_idr, hdr.size);
  else if (o->idr->getIDR() != o_idr)
    free(o_idr);
}
}
