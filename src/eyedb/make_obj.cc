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

  Status (*getMakeFunction(int type))(Database *,
					  const Oid *, Object **,
					  const RecMode *,
					  const ObjectHeader *,
					  Data, LockMode, const Class *)
  {
    switch(type) {
      /* classes */
    case _Class_Type:
      return classMake;

    case _StructClass_Type:
    case _UnionClass_Type:
      return agregatClassMake;

    case _EnumClass_Type:
      return enumClassMake;

    case _BasicClass_Type:
      return basicClassMake;

      /* instances */
    case _Basic_Type:
      return basicMake;

    case _Struct_Type:
    case _Union_Type:
      //case _Agregat_Type:
      return agregatMake;

    case _Enum_Type:
      return enumMake;

    case _Schema_Type:
      return schemaClassMake;

    case _Collection_Type:
    case _CollSet_Type:
    case _CollBag_Type:
    case _CollList_Type:
    case _CollArray_Type:
      return collectionMake;

    case _CollectionClass_Type:
    case _CollSetClass_Type:
    case _CollBagClass_Type:
    case _CollListClass_Type:
    case _CollArrayClass_Type:
      return collectionClassMake;

    default:
      return 0;
    }
  }
}
