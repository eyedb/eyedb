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

package org.eyedb;

public class ObjectHeader {

    int    magic;
    int    type;
    int    size;
    long   ctime;
    long   mtime;
    int    xinfo;
    Oid oid_mcl;
    Oid oid_prot;

    static ObjectHeader NullObjectHeader = new ObjectHeader();
    static byte[] NullCodeObjectHeader = NullObjectHeader.code();

    ObjectHeader() {
	//magic = IDB_OBJ_HEAD_MAGIC;
	magic = 0;
	type = size = xinfo = 0;
	ctime = mtime = 0;
	oid_mcl = new Oid();
	oid_prot = new Oid();
    }

    // !must be in Coder.java!
    static ObjectHeader decode(byte[] b) throws Exception {
	Coder coder = new Coder(b);
	int magic = coder.decodeInt();

	if (magic != IDB_OBJ_HEAD_MAGIC)
	    {
		throw new LoadObjectException
		    (new Status(Status.IDB_ERROR, "invalid magic " + magic));
	    }

	ObjectHeader objh = new ObjectHeader();
	objh.magic    = magic;
	objh.type     = coder.decodeInt();
	objh.size     = coder.decodeInt();
	objh.ctime    = coder.decodeLong();
	objh.mtime    = coder.decodeLong();
	objh.xinfo    = coder.decodeInt();
	objh.oid_mcl  = coder.decodeOid();
	objh.oid_prot = coder.decodeOid();
    
	/*System.out.println("decoding object header " + objh.ctime + ", " +
	  objh.mtime + ", " + objh.oid_mcl + ":" + objh.oid_mcl.dbid +":"+objh.oid_mcl.unique);
	*/
	return objh;
    }

    // !must be in Coder.java!
    byte[] code() {
	byte[] b = new byte[IDB_OBJ_HEAD_SIZE];
	code(b);
	return b;
    }

    // !must be in Coder.java!
    void code(byte[] b) {
	Coder coder = new Coder(b);

	coder.code(magic);
	coder.code(type);
	coder.code(size);
	coder.code(ctime);
	coder.code(mtime);
	coder.code(xinfo);
	coder.code(oid_mcl);
	coder.code(oid_prot);
    }

    void print() {
	System.out.println("magic = " +
			   Integer.toHexString(magic) +
			   ", type = " +
			   Integer.toHexString(type) +
			   ", size = " +
			   size +
			   ", ctime " +
			   ctime +
			   ", mtime " +
			   mtime +
			   ", oid_mcl " +
			   oid_mcl.getString() +
			   ", oid_prot " +
			   oid_prot.getString());
    }

    public static final int IDB_OBJ_HEAD_MAGIC_SIZE    = 4;
    public static final int IDB_OBJ_HEAD_TYPE_SIZE     = 4;
    public static final int IDB_OBJ_HEAD_SIZE_SIZE     = 4;
    public static final int IDB_OBJ_HEAD_CTIME_SIZE    = 8;
    public static final int IDB_OBJ_HEAD_MTIME_SIZE    = 8;
    public static final int IDB_OBJ_HEAD_XINFO_SIZE    = 4;
    public static final int IDB_OBJ_HEAD_OID_MCL_SIZE  = 8;
    public static final int IDB_OBJ_HEAD_OID_PROT_SIZE = 8;

    public static final int IDB_OBJ_HEAD_MAGIC_INDEX = 0;

    public static final int IDB_OBJ_HEAD_TYPE_INDEX  =
	IDB_OBJ_HEAD_MAGIC_INDEX + IDB_OBJ_HEAD_MAGIC_SIZE;

    public static final int IDB_OBJ_HEAD_SIZE_INDEX  =
	IDB_OBJ_HEAD_TYPE_INDEX + IDB_OBJ_HEAD_TYPE_SIZE;

    public static final int IDB_OBJ_HEAD_CTIME_INDEX =
	IDB_OBJ_HEAD_SIZE_INDEX  + IDB_OBJ_HEAD_SIZE_SIZE;

    public static final int IDB_OBJ_HEAD_MTIME_INDEX =
	IDB_OBJ_HEAD_CTIME_INDEX + IDB_OBJ_HEAD_CTIME_SIZE;

    public static final int IDB_OBJ_HEAD_XINFO_INDEX =
	IDB_OBJ_HEAD_MTIME_INDEX + IDB_OBJ_HEAD_MTIME_SIZE;

    public static final int IDB_OBJ_HEAD_OID_MCL_INDEX =
	IDB_OBJ_HEAD_XINFO_INDEX + IDB_OBJ_HEAD_XINFO_SIZE;

    public static final int IDB_OBJ_HEAD_OID_PROT_INDEX =
	IDB_OBJ_HEAD_OID_MCL_INDEX + IDB_OBJ_HEAD_OID_MCL_SIZE;

    public static final int IDB_OBJ_HEAD_SIZE =
	IDB_OBJ_HEAD_OID_PROT_INDEX + IDB_OBJ_HEAD_OID_PROT_SIZE;

    public static final int IDB_CLASS_NAME_LEN      = 33;
    public static final int IDB_CLASS_NAME_PAD      = IDB_CLASS_NAME_LEN -
	Coder.OID_SIZE;

    public static final int IDB_CLASS_NAME_OVERHEAD = 1;

    public static final int IDB_CLASS_NAME_TOTAL_LEN = IDB_CLASS_NAME_LEN +
	IDB_CLASS_NAME_OVERHEAD;

    public static final int IDB_SCH_INCR_SIZE = 12 + IDB_CLASS_NAME_TOTAL_LEN;

    public static final int IDB_CLASS_EXTENT  = IDB_OBJ_HEAD_SIZE;

    public static final int IDB_CLASS_COMPONENTS = IDB_CLASS_EXTENT  + Coder.OID_SIZE;

    public static final int IDB_CLASS_IMPL_BEGIN = IDB_CLASS_COMPONENTS + Coder.OID_SIZE;
    public static final int IDB_CLASS_IMPL_TYPE = IDB_CLASS_IMPL_BEGIN;
    public static final int IDB_CLASS_IMPL_DSPID = IDB_CLASS_IMPL_TYPE + 1;
    public static final int IDB_CLASS_IMPL_INFO = IDB_CLASS_IMPL_DSPID + 2;
    public static final int IDB_CLASS_IMPL_MTH = IDB_CLASS_IMPL_INFO + 4;
    public static final int IDB_MAX_HINTS_CNT = 8;

    public static final int IDB_CLASS_IMPL_HINTS = IDB_CLASS_IMPL_MTH + Coder.OID_SIZE;


    public static final int IDB_CLASS_MTYPE     = IDB_CLASS_IMPL_HINTS + IDB_MAX_HINTS_CNT * 4;

    public static final int IDB_CLASS_DSPID      = IDB_CLASS_MTYPE + 4;
    public static final int IDB_CLASS_HEAD_SIZE  = IDB_CLASS_DSPID + 2;

    public static final int IDB_CLASS_COLL_START = IDB_CLASS_EXTENT;
    public static final int IDB_CLASS_COLLS_CNT  = 2;

    public static final int IDB_OBJ_HEAD_MAGIC       = 0xe8fa6efc;

    public static final int IDB_XINFO_BIG_ENDIAN     = 0x1;
    public static final int IDB_XINFO_LITTLE_ENDIAN  = 0x2;
    public static final int IDB_XINFO_LOCAL_OBJ      = 0xf200;

    public static final int IDB_SCH_CNT_INDEX       = IDB_OBJ_HEAD_SIZE;
    public static final int IDB_SCH_CNT_SIZE        = 4;
    public static final int IDB_SCH_NAME_INDEX      = 
	(IDB_SCH_CNT_INDEX + IDB_SCH_CNT_SIZE);

    public static final int IDB_SCH_NAME_SIZE       = 32;
    public static final int IDB_NAME_OUT_PLACE = 1;
    public static final int IDB_NAME_IN_PLACE = 2;

    static int IDB_SCH_OID_INDEX(int x)
    {
	//    return IDB_SCH_NAME_INDEX + IDB_SCH_NAME_SIZE + x * 8;
	return IDB_SCH_NAME_INDEX + IDB_SCH_NAME_SIZE + x * IDB_SCH_INCR_SIZE;
    }

    static int IDB_SCH_OID_SIZE(int x) {return IDB_SCH_OID_INDEX(x);}

    static int shift(int x) {return (1 << x);}
  
    public static final int Object_Type          =  0;
    public static final int Class_Type           =  1;
    public static final int BasicClass_Type      =  2;
    public static final int EnumClass_Type       =  3;
    public static final int AgregatClass_Type    =  4;
    public static final int StructClass_Type     =  5;
    public static final int UnionClass_Type      =  6;
    public static final int Instance_Type        =  7;
    public static final int Basic_Type           =  8;
    public static final int Enum_Type            =  9;
    public static final int Agregat_Type         = 10;
    public static final int Struct_Type          = 11;
    public static final int Union_Type           = 12;
    public static final int Schema_Type          = 13;
    public static final int CollectionClass_Type = 14;
    public static final int CollSetClass_Type    = 15;
    public static final int CollBagClass_Type    = 16;
    public static final int CollListClass_Type   = 17;
    public static final int CollArrayClass_Type  = 18;
    public static final int Collection_Type      = 19;
    public static final int CollSet_Type         = 20;
    public static final int CollBag_Type         = 21;
    public static final int CollList_Type        = 22;
    public static final int CollArray_Type       = 23;

    public static final int _Object_Type = shift(Object_Type);

    public static final int _Class_Type =  shift(Class_Type);
    public static final int _BasicClass_Type = _Class_Type | shift(BasicClass_Type);
    public static final int _EnumClass_Type = _Class_Type | shift(EnumClass_Type);
    public static final int _AgregatClass_Type = _Class_Type | shift(AgregatClass_Type);
    public static final int _StructClass_Type = _AgregatClass_Type | shift(StructClass_Type);
    public static final int _UnionClass_Type = _AgregatClass_Type | shift(UnionClass_Type);

    public static final int _Instance_Type = shift(Instance_Type);
    public static final int _Basic_Type = _Instance_Type | shift(Basic_Type);
    public static final int _Enum_Type  = _Instance_Type | shift(Enum_Type);
    public static final int _Agregat_Type = _Instance_Type | shift(Agregat_Type);
    public static final int _Struct_Type = _Agregat_Type | shift(Struct_Type);
    public static final int _Union_Type = _Agregat_Type | shift(Union_Type);

    public static final int _Schema_Type = _Instance_Type | shift(Schema_Type);

    public static final int _CollectionClass_Type = _Class_Type | shift(CollectionClass_Type);
    public static final int _CollSetClass_Type = _CollectionClass_Type | shift(CollSetClass_Type);
    public static final int _CollBagClass_Type = _CollectionClass_Type | shift(CollBagClass_Type);
    public static final int _CollListClass_Type = _CollectionClass_Type | shift(CollListClass_Type);
    public static final int _CollArrayClass_Type = _CollectionClass_Type | shift(CollArrayClass_Type);

    public static final int _Collection_Type = _Instance_Type | shift(Collection_Type);
    public static final int _CollSet_Type = _Collection_Type | shift(CollSet_Type);
    public static final int _CollBag_Type = _Collection_Type | shift(CollBag_Type);
    public static final int _CollList_Type = _Collection_Type | shift(CollList_Type);
    public static final int _CollArray_Type = _Collection_Type | shift(CollArray_Type);

    public static final short IDB_COLL_IMPL_CHANGED  = 0x1273;
    public static final short IDB_COLL_IMPL_UNCHANGED = 0x4e1a;

    Object makeObject(Database db, Oid oid, byte[] idr, int lockmode,
			 RecMode rcm) throws Exception
    {
	//print();
    
	if (type == _Class_Type)
	    return Class.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _StructClass_Type ||
	    type == _UnionClass_Type)
	    return AgregatClass.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _EnumClass_Type)
	    return EnumClass.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _BasicClass_Type)
	    return BasicClass.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _Basic_Type)
	    return Basic.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _Struct_Type ||
	    type == _Union_Type ||
	    type == _Agregat_Type)
	    return Agregat.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _Enum_Type)
	    return Enum.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _Schema_Type)
	    return Schema.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _Collection_Type ||
	    type == _CollSet_Type ||
	    type == _CollBag_Type ||
	    type == _CollList_Type ||
	    type == _CollArray_Type)
	    return Collection.make(db, oid, idr, oid_mcl, lockmode, rcm);

	if (type == _CollectionClass_Type ||
	    type == _CollSetClass_Type ||
	    type == _CollBagClass_Type ||
	    type == _CollListClass_Type ||
	    type == _CollArrayClass_Type)
	    return CollectionClass.make(db, oid, idr, oid_mcl, lockmode, rcm);

	throw new Exception("INTERNAL_ERROR",
			       "Invalid type in objectHeader " + type);
    }

    public String toString() {
	return "type: " + type + 
	    "\nsize: " + size +
	    "\nctime: " + ctime +
	    "\nmtime: " + mtime +
	    "\noid_mcl: " + oid_mcl;
    }
}
