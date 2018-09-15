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


public class StructClass extends AgregatClass {

  StructClass(Database db, String name) {
    super(db, name);
  }

  public StructClass(Database db, String name, Class parent) {
    super(db, name, parent);
    //    parent = (parent != null ? parent : db.getSchema().getClass("struct"));
  }

  public StructClass(String name, Class parent) {
    super(name, parent);
    //    parent = (parent != null ? parent : db.getSchema().getClass("struct"));
  }

  public StructClass(Database db, String name, Oid parent_oid) {
    super(db, name, parent_oid);
  }

  public Object newObj(Database db, Coder coder) throws Exception {
    Struct o = new Struct(db);

    o.make(coder, this, ObjectHeader._Struct_Type);

    newObjRealize(db, o);

    return Database.objectMake(o);
  }

  public static Class idbclass;
}

