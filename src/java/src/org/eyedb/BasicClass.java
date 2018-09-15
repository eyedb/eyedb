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



public class BasicClass extends Class {

  static Object make(Database db, Oid oid, byte[] idr,
			Oid class_oid, int lockmode, RecMode rcm)
			throws Exception
  {
    //    System.out.println("BasicClass::make(" + oid + ")");
    Coder coder = new Coder(idr);

    ClassHints hints = ClassHints.decode(db, coder);

    short code = coder.decodeShort();
    
    Class mclass = db.getSchema().getClass(hints.name);

    mclass.set(db, hints, class_oid);

    return mclass;
  }

  BasicClass(Database db, String name, int bsize) {
    super(db, name);

    idr_objsz = ObjectHeader.IDB_OBJ_HEAD_SIZE + bsize;
    idr_psize = idr_objsz;
    idr_vsize = 0;
  }

  static final int CharClass_Code  = 10;
  static final int ByteClass_Code  = 11;
  static final int BOidClass_Code  = 12;
  static final int Int16Class_Code = 13;
  static final int Int32Class_Code = 14;
  static final int FloatClass_Code = 15;

  public static Class idbclass;
}

