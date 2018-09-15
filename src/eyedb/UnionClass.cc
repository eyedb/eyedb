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
#include <iostream>

namespace eyedb {

UnionClass::UnionClass(const char *s, Class *p) :
AgregatClass(s, p)
{
  parent = (p ? p : Union_Class);
  setClass(UnionClass_Class);
  type = _UnionClass_Type;
}

UnionClass::UnionClass(const char *s, const Oid *poid) :
AgregatClass(s, poid)
{
  parent = 0;
  setClass(UnionClass_Class);
  type = _UnionClass_Type;
}

UnionClass::UnionClass(Database *_db, const char *s,
			     Class *p) : AgregatClass(_db, s, p)
{
  parent = (p ? p : Union_Class);
  setClass(UnionClass_Class);
  type = _UnionClass_Type;
}

UnionClass::UnionClass(Database *_db, const char *s,
			     const Oid *poid) : AgregatClass(_db, s, poid)
{
  parent = 0;
  setClass(UnionClass_Class);
  type = _UnionClass_Type;
}

Status
UnionClass::loadComplete(const Class *cl)
{
  assert(cl->asUnionClass());
  Status s = Class::loadComplete(cl);
  if (s) return s;
  return Success;
}

Object *UnionClass::newObj(Database *_db) const
{
  if (!idr_objsz)
    return 0;

  Union *t = new Union(_db);

  ObjectPeer::make(t, this, 0, _Union_Type, idr_objsz,
		      idr_psize, idr_vsize);
  newObjRealize(t);
  return t;
}

Object *UnionClass::newObj(Data data, Bool _copy) const
{
  if (!idr_objsz)
    return 0;

  Union *t = new Union();

  ObjectPeer::make(t, this, data, _Union_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  newObjRealize(t);
  return t;
}

}
