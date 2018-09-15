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

public class Enum extends Instance {

  static Object make(Database db, Oid oid, byte[] idr,
			Oid class_oid, int lockmode, RecMode rcm)
			throws Exception
  {
    Class mclass = db.getSchema().getClass(class_oid);

    Coder coder = new Coder(idr, ObjectHeader.IDB_OBJ_HEAD_SIZE);

    Object o = mclass.newObj(db, coder);

    coder.setOffset(ObjectHeader.IDB_OBJ_HEAD_SIZE);

    //boolean isset = coder.decodeBoolean();

    /*if (isset) */ {
	int val = coder.decodeInt();
	((Enum)o).setIntValue(val);
    }
    
    return o;
  }

  public Enum(Database db) {
    super(db);
  }

  public Enum(Database db, Dataspace dataspace) {
    super(db, dataspace);
  }

  public Enum() {
    super(null);
  }

  void setIntValue(int val) {
    EnumItem item = ((EnumClass)cls).getEnumItemFromVal(val);
  }

  public static Class idbclass;
}

