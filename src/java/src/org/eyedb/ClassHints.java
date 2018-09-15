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

class ClassHints {
    String name;
    int mtype;
    short dspid;
    Oid extent_oid;
    Oid components_oid;
    Oid parent_oid;
    IndexImpl idximpl;

    ClassHints() {
    }

    static ClassHints decode(Database db, Coder coder) throws Exception {
	ClassHints hints = new ClassHints();

	coder.setOffset(ObjectHeader.IDB_CLASS_EXTENT);
    
	hints.extent_oid  = coder.decodeOid();
	hints.components_oid = coder.decodeOid();

	hints.idximpl = IndexImpl.decode(db, coder);

	hints.mtype         = coder.decodeInt();
	hints.dspid         = coder.decodeShort();
	hints.name          = class_name_decode(db, coder);
	hints.parent_oid    = new Oid();

	/*
	System.out.println("name " + hints.name);
	System.out.println("mtype " + hints.mtype);
	System.out.println("dspid " + hints.dspid);
	System.out.println("extent_oid " + hints.extent_oid);
	System.out.println("components_oid " + hints.components_oid);
	System.out.println("parent_oid " + hints.parent_oid);
	*/

	return hints;
    }

    static String class_name_decode(Database db, Coder coder) {
	byte b = coder.decodeByte();
	if (b != ObjectHeader.IDB_NAME_OUT_PLACE)
	    return coder.decodeString(ObjectHeader.IDB_CLASS_NAME_LEN);

	Oid data_oid = coder.decodeOid();
	Value vsize = new Value();
	Status status = RPC.rpc_DATA_SIZE_GET(db, data_oid, vsize);
	if (status.isSuccess())
	    {
		int size = vsize.sgetInt();
		byte[] bname = new byte[size];
		status = RPC.rpc_DATA_READ(db, 0, size, null, bname, data_oid);
		if (status.isSuccess())
		    {
			coder.decodeString(ObjectHeader.IDB_CLASS_NAME_PAD);
			return new String(bname, 0, 0, size-1);
		    }
	
	    }
	return null;
    }
}
