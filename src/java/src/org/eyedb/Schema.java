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

import java.util.*;

public class Schema extends Instance {

  static Object make(Database db, Oid oid, byte[] idr,
			Oid class_oid, int lockmode, RecMode rcm)
			throws Exception
  {
    //System.out.println("Schema::make(" + oid + ")");

    Schema sch = new Schema();
    sch.db = db;
    Coder coder = new Coder(idr);
    coder.setOffset(ObjectHeader.IDB_SCH_CNT_INDEX);

    int cnt = coder.decodeInt();
    coder.setOffset(ObjectHeader.IDB_SCH_NAME_INDEX);
    sch.setName(coder.decodeString());

    db.sch = sch;
    Class.init_1(sch);

    for (int i = 0; i < cnt; i++) {
	coder.setOffset(ObjectHeader.IDB_SCH_OID_INDEX(i));
	    
	Oid moid = coder.decodeOid();
	Object o = db.loadObject(moid);
	if (o != null)
	    sch.addClass((Class)o);
    }

    return sch;
  }

  Schema(String name) {
    super(null);
    this.name = name;
  }

  Schema() {
    super(null);
  }

  public String getName()          {return name;}

  public void setName(String name) {this.name = name;}

  public Class addClass(Class cls) {

      //System.out.println(this + ": adding class in schema " + cls.getName());
      // 16/05/05: added
      java.lang.Object ocls = hash_name.get(cls.getName());
      if (ocls != null)
	  mlist.removeElement(ocls);

      mlist.addElement(cls);
      hash_name.put(cls.getName(), cls);

      if (cls.getOid() != null)
	  hash_oid.put(cls.getOid(), cls);

      return cls;
  }

  public Class getClass(String s) {
    Class mclass = (Class)hash_name.get(s);
    return mclass;
  }

  public Class getClass(Oid oid) throws Exception {
    Class mclass = (Class)hash_oid.get(oid);
    if (mclass == null)
	return (Class)db.loadObject(oid);

    return mclass;
  }

  public void traceRealize(java.io.PrintStream out, Indent indent,
			   int flags, RecMode rcm) throws Exception {
    int count = mlist.size();

    out.println(indent.toString() + oid + " schema " + name + " {");

    indent.push();
    for (int i = 0; i < count; i++)
      {
	if (i != 0)
	  out.println("");
	((Class)mlist.elementAt(i)).traceRealize(out, indent, flags,
							 rcm);
      }

    indent.pop();
    out.println(indent + "};");
  }

  public Vector getClassList() {return mlist;}

  public void complete() throws Exception {
    int count = mlist.size();

    for (int i = 0; i < count; i++)
      ((Class)mlist.elementAt(i)).complete();
  }

  static private final int ht_defsize = 512;
  String name;
  Hashtable hash_name = new Hashtable(ht_defsize);
  Hashtable hash_oid  = new Hashtable(ht_defsize);
  Vector mlist = new Vector();

  public static Schema bootstrap = new Schema("bootstrap");

  public static Class idbclass;
}
