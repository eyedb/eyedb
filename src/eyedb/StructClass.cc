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

extern void oqlctbMake(Object **);
extern void utilsMake(Object **);

//
// StructClass methods
//

StructClass::StructClass(const char *s, Class *p) :
AgregatClass(s, p)
{
  parent = (p ? p : Struct_Class);
  setClass(StructClass_Class);
  type = _StructClass_Type;
}

StructClass::StructClass(const char *s, const Oid *poid) :
AgregatClass(s, poid)
{
  parent = 0;
  setClass(StructClass_Class);
  type = _StructClass_Type;
}

StructClass::StructClass(Database *_db, const char *s,
			       Class *p) : AgregatClass(_db, s, p)
{
  parent = (p ? p : Struct_Class);
  setClass(StructClass_Class);
  type = _StructClass_Type;
}

StructClass::StructClass(Database *_db, const char *s,
			       const Oid *poid) : AgregatClass(_db, s, poid)
{
  parent = 0;
  setClass(StructClass_Class);
  type = _StructClass_Type;
}

// should be moved to a .h
extern void sysclsMake(Object **);

Object *StructClass::newObj(Database *_db) const
{
  if (!idr_objsz)
    return 0;

  Struct *t = new Struct(_db);

  ObjectPeer::make(t, this, 0, _Struct_Type, idr_objsz,
		      idr_psize, idr_vsize);
  newObjRealize(t);

  //#ifndef OPTCONSAPP

  // 6/10/01: reconnected the following code because of a the following bug:
  // (select Person.birthdate).toString() does not construct a Date because
  // the consapp facility of Database is not used.
  if (!ObjectPeer::isGRTObject(t)) {
    sysclsMake((Object **)&t); // should better be in ObjectPeer::make!!
    oqlctbMake((Object **)&t); // should better be in ObjectPeer::make!!
    utilsMake((Object **)&t); // should better be in ObjectPeer::make!!
  }
  return t;
}

StructClass::StructClass(const Oid &_oid, const char *_name)
  : AgregatClass(_oid, _name)
{
  type = _StructClass_Type;
}

Status
StructClass::loadComplete(const Class *cl)
{
  assert(cl->asStructClass());
  Status s = Class::loadComplete(cl);
  if (s) return s;
  return Success;
}

Object *StructClass::newObj(Data data, Bool _copy) const
{
  if (!idr_objsz)
    return 0;

  Struct *t = new Struct();

  ObjectPeer::make(t, this, data, _Struct_Type, idr_objsz,
		      idr_psize, idr_vsize, _copy);
  newObjRealize(t);

  //#ifndef OPTCONSAPP
  // see previous newObj method comments
  if (!ObjectPeer::isGRTObject(t)) {
    sysclsMake((Object **)&t);
    oqlctbMake((Object **)&t); // should better be in ObjectPeer::make!!
    utilsMake((Object **)&t); // should better be in ObjectPeer::make!!
  }
  //#endif
  return t;
}
}
