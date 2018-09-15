/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
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

#ifndef _EYEDB_KERN_CONST_H
#define _EYEDB_KERN_CONST_H

namespace eyedb {

  enum {
    Object_Type = 0,

    Class_Type,
    BasicClass_Type,
    EnumClass_Type,
    AgregatClass_Type,
    StructClass_Type,
    UnionClass_Type,

    Instance_Type,
    Basic_Type,
    Enum_Type,
    Agregat_Type,
    Struct_Type,
    Union_Type,
    Schema_Type,

    CollectionClass_Type,
    CollSetClass_Type,
    CollBagClass_Type,
    CollListClass_Type,
    CollArrayClass_Type,

    Collection_Type,
    CollSet_Type,
    CollBag_Type,
    CollList_Type,
    CollArray_Type,

    idbLAST_Type
  };

#define IDB_SHIFT(X) (1L << (X))

  enum {
    _Object_Type      = IDB_SHIFT(Object_Type),

    _Class_Type   = IDB_SHIFT(Class_Type),
    _BasicClass_Type   = _Class_Type   | IDB_SHIFT(BasicClass_Type),
    _EnumClass_Type    = _Class_Type   | IDB_SHIFT(EnumClass_Type),
    _AgregatClass_Type = _Class_Type   | IDB_SHIFT(AgregatClass_Type),
    _StructClass_Type  = _AgregatClass_Type | IDB_SHIFT(StructClass_Type),
    _UnionClass_Type   = _AgregatClass_Type | IDB_SHIFT(UnionClass_Type),

    _Instance_Type       = IDB_SHIFT(Instance_Type),
    _Basic_Type       = _Instance_Type   | IDB_SHIFT(Basic_Type),
    _Enum_Type        = _Instance_Type   | IDB_SHIFT(Enum_Type),
    _Agregat_Type     = _Instance_Type   | IDB_SHIFT(Agregat_Type),
    _Struct_Type      = _Agregat_Type | IDB_SHIFT(Struct_Type),
    _Union_Type       = _Agregat_Type | IDB_SHIFT(Union_Type),

    _Schema_Type  = _Instance_Type   | IDB_SHIFT(Schema_Type),

    _CollectionClass_Type = _Class_Type | IDB_SHIFT(CollectionClass_Type),
    _CollSetClass_Type    = _CollectionClass_Type | IDB_SHIFT(CollSetClass_Type),
    _CollBagClass_Type    = _CollectionClass_Type | IDB_SHIFT(CollBagClass_Type),
    _CollListClass_Type   = _CollectionClass_Type | IDB_SHIFT(CollListClass_Type),
    _CollArrayClass_Type  = _CollectionClass_Type | IDB_SHIFT(CollArrayClass_Type),

    _Collection_Type  = _Instance_Type   | IDB_SHIFT(Collection_Type),
    _CollSet_Type     = _Collection_Type | IDB_SHIFT(CollSet_Type),
    _CollBag_Type     = _Collection_Type | IDB_SHIFT(CollBag_Type),
    _CollList_Type    = _Collection_Type | IDB_SHIFT(CollList_Type),
    _CollArray_Type   = _Collection_Type | IDB_SHIFT(CollArray_Type)
  };

#define IDB_OBJ_HEAD_MAGIC_SIZE    sizeof(::eyedblib::int32)
#define IDB_OBJ_HEAD_TYPE_SIZE     sizeof(::eyedblib::int32)
#define IDB_OBJ_HEAD_SIZE_SIZE     sizeof(::eyedblib::int32)
#define IDB_OBJ_HEAD_CTIME_SIZE    sizeof(::eyedblib::int64)
#define IDB_OBJ_HEAD_MTIME_SIZE    sizeof(::eyedblib::int64)
#define IDB_OBJ_HEAD_XINFO_SIZE    sizeof(::eyedblib::int32)
#define IDB_OBJ_HEAD_OID_MCL_SIZE  sizeof(::eyedbsm::Oid)
#define IDB_OBJ_HEAD_OID_PROT_SIZE sizeof(::eyedbsm::Oid)

#define IDB_OBJ_HEAD_MAGIC_INDEX 0

#define IDB_OBJ_HEAD_TYPE_INDEX     (IDB_OBJ_HEAD_MAGIC_INDEX + \
				    IDB_OBJ_HEAD_MAGIC_SIZE)

#define IDB_OBJ_HEAD_SIZE_INDEX     (IDB_OBJ_HEAD_TYPE_INDEX  + \
				    IDB_OBJ_HEAD_TYPE_SIZE)

#define IDB_OBJ_HEAD_CTIME_INDEX    (IDB_OBJ_HEAD_SIZE_INDEX  + \
				    IDB_OBJ_HEAD_SIZE_SIZE)

#define IDB_OBJ_HEAD_MTIME_INDEX    (IDB_OBJ_HEAD_CTIME_INDEX + \
				    IDB_OBJ_HEAD_CTIME_SIZE)

#define IDB_OBJ_HEAD_XINFO_INDEX    (IDB_OBJ_HEAD_MTIME_INDEX + \
				    IDB_OBJ_HEAD_MTIME_SIZE)

#define IDB_OBJ_HEAD_OID_MCL_INDEX  (IDB_OBJ_HEAD_XINFO_INDEX + \
				    IDB_OBJ_HEAD_XINFO_SIZE)

#define IDB_OBJ_HEAD_OID_PROT_INDEX (IDB_OBJ_HEAD_OID_MCL_INDEX + \
				    IDB_OBJ_HEAD_OID_MCL_SIZE)

#define IDB_OBJ_HEAD_SIZE	    (IDB_OBJ_HEAD_OID_PROT_INDEX + \
				     IDB_OBJ_HEAD_OID_PROT_SIZE)

#define IDB_CLASS_EXTENT      (IDB_OBJ_HEAD_SIZE)
#define IDB_CLASS_COMPONENTS  (IDB_CLASS_EXTENT  + sizeof(::eyedbsm::Oid))
#define IDB_CLASS_IMPL_BEGIN  (IDB_CLASS_COMPONENTS + sizeof(::eyedbsm::Oid))
#define IDB_CLASS_IMPL_TYPE   IDB_CLASS_IMPL_BEGIN
#define IDB_CLASS_IMPL_DSPID  (IDB_CLASS_IMPL_TYPE + sizeof(char))
#define IDB_CLASS_IMPL_INFO   (IDB_CLASS_IMPL_DSPID + sizeof(::eyedblib::int16))
#define IDB_CLASS_IMPL_MTH    (IDB_CLASS_IMPL_INFO + sizeof(::eyedblib::int32))
#define IDB_CLASS_IMPL_HINTS  (IDB_CLASS_IMPL_MTH + sizeof(::eyedbsm::Oid))
#define IDB_CLASS_MTYPE       (IDB_CLASS_IMPL_HINTS + \
                               IDB_MAX_HINTS_CNT * sizeof(::eyedblib::int32))

#define IDB_CLASS_DSPID       (IDB_CLASS_MTYPE + sizeof(::eyedblib::int32))
#define IDB_CLASS_HEAD_SIZE   (IDB_CLASS_DSPID + sizeof(::eyedblib::int16))

#define IDB_CLASS_COLL_START  IDB_CLASS_EXTENT 
#define IDB_CLASS_COLLS_CNT   2
#define IDB_CLASS_ATTR_START  (IDB_CLASS_HEAD_SIZE + 1 + IDB_CLASS_NAME_LEN + \
                              sizeof(::eyedbsm::Oid) + 4 * sizeof(::eyedblib::int32))

#define IDB_OBJ_HEAD_MAGIC ((::eyedblib::int32)0xe8fa6efc)

#define IDB_XINFO_BIG_ENDIAN     0x1
#define IDB_XINFO_LITTLE_ENDIAN  0x2
#define IDB_XINFO_LOCAL_OBJ      0xf200
#define IDB_XINFO_REMOVED        0x40
#define IDB_XINFO_CARD           0x100
#define IDB_XINFO_INV            0x400
#define IDB_XINFO_INVALID_INV    0x800
#define IDB_XINFO_CLASS_UPDATE   0x1000

#define IDB_SCH_CNT_INDEX  IDB_OBJ_HEAD_SIZE
#define IDB_SCH_CNT_SIZE   sizeof(::eyedblib::int32)
#define IDB_SCH_NAME_INDEX (IDB_SCH_CNT_INDEX + IDB_SCH_CNT_SIZE)
#define IDB_SCH_NAME_SIZE  32
#define IDB_SCH_INCR_SIZE  (sizeof(::eyedbsm::Oid)+sizeof(::eyedblib::int32)+IDB_CLASS_NAME_TOTAL_LEN)
#define IDB_SCH_OID_INDEX(X)     (IDB_SCH_NAME_INDEX + IDB_SCH_NAME_SIZE + \
				  (X) * IDB_SCH_INCR_SIZE)
#define IDB_SCH_TYPE_INDEX(X)    (IDB_SCH_OID_INDEX(X)+sizeof(::eyedbsm::Oid))
#define IDB_SCH_CLSNAME_INDEX(X) (IDB_SCH_TYPE_INDEX(X)+sizeof(::eyedblib::int32))

#define IDB_SCH_OID_SIZE     IDB_SCH_OID_INDEX

}

#endif
