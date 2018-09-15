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

public class AgregatClass extends Class {

  static Object make(Database db, Oid oid, byte[] idr,
			Oid class_oid, int lockmode, RecMode rcm)
			throws Exception
  {
    Coder coder = new Coder(idr);

    ClassHints hints = ClassHints.decode(db, coder);

    //short code        = coder.decodeShort();
    Oid parent_oid = coder.decodeOid();

    AgregatClass magreg = null;

    // CURRENTLY DO NOT TAKE SUPPORT UNIONs
    //if (code == StructClass_Code)
      magreg = new StructClass(db, hints.name, parent_oid);
      /*
    else if (code == UnionClass_Code)
      magreg = new UnionClass(db, hints.name,  parent_oid);
    else
      {
	System.err.println("FATAL ERROR code = " + code);
	System.exit(1);
      }
      */

    magreg.set(db, hints, class_oid);

    magreg.idr_psize  = coder.decodeInt();
    magreg.idr_vsize  = coder.decodeInt();
    magreg.idr_objsz = coder.decodeInt();

    int count = coder.decodeInt();

    /*
    System.out.println("magreg.idr_psize = " + magreg.idr_psize);
    System.out.println("magreg.idr_vsize = " + magreg.idr_vsize);
    System.out.println("magreg.idr_objsize = " + magreg.idr_objsz);
    System.out.println("count = " + count);
    */

    Attribute[] nat_items = magreg.getAttributes();

    magreg.items = new Attribute[count];

    for (int n = 0; n < nat_items.length; n++)
      magreg.items[n] = nat_items[n];

    for (int n = nat_items.length; n < count; n++)
      magreg.items[n] = Attribute.make(db, coder, n);

    magreg.setInstanceDspid(hints.dspid);

    return magreg;
  }

  AgregatClass(Database db, String name) {
    super(db, name);
    init();
  }

  AgregatClass(Database db, String name, Class parent) {
    super(db, name, parent);
    init();
  }

  AgregatClass(String name, Class parent) {
    super(name, parent);
    init();
  }

  AgregatClass(Database db, String name, Oid parent_oid) {
    super(db, name, parent_oid);
    init();
  }

  private void init() {
    items = AttrNative.make(db, AttrNative.OBJECT);
  }

  static final int StructClass_Code = 100;
  static final int UnionClass_Code  = 101;

  public static Class idbclass;
}

