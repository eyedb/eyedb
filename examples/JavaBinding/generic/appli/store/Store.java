
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

import org.eyedb.*;
import java.lang.reflect.*;

class Store {

    public static void main(String args[]) {

	args = org.eyedb.Root.init("Store", args);
	int n = args.length;

	if (n != 3)  {
	    System.err.println("usage: java Store dbname name age");
	    System.exit(1);
	}

	try {
	    org.eyedb.Connection conn = new org.eyedb.Connection();

	    org.eyedb.Database db = new org.eyedb.Database(args[0]);
	    System.err.println("#### Before Opening");
	    db.open(conn, org.eyedb.Database.DBRW);
	    System.err.println("#### After Opening");

	    org.eyedb.Oid oid = create_person(db, args);
	    org.eyedb.Object o = reload_person(db, oid);
	    add_children(db, o);

	    conn.close();
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	}

	catch(java.lang.Exception e) {
	    System.err.println(e);
	}

	System.err.println("#### Done");
    }

    static org.eyedb.Class person_cls;
    static org.eyedb.Attribute name_attr;
    static org.eyedb.Attribute age_attr;
    static org.eyedb.Attribute children_attr;

    // using EyeDB schema introspection (i.e. reflection)
    public static void init_person_class(org.eyedb.Database db) throws org.eyedb.Exception {
	if (person_cls != null)
	    return;

	person_cls = db.getSchema().getClass("Person");
	if (person_cls == null)  {
	    System.out.println("Class Person not found");
	    System.exit(1);
	}

	name_attr = person_cls.getAttribute("name");
	if (name_attr == null) {
	    System.out.println("Attribute name not found");
	    System.exit(1);
	}

	age_attr = person_cls.getAttribute("age");
	if (age_attr == null) {
	    System.out.println("Attribute age not found");
	    System.exit(1);
	}

	children_attr = person_cls.getAttribute("children");
	if (children_attr == null) {
	    System.out.println("Attribute children not found");
	    System.exit(1);
	}
    }

    public static org.eyedb.Oid create_person(org.eyedb.Database db, String args[])
	throws org.eyedb.Exception {

	db.transactionBegin();

	init_person_class(db);

	System.err.println("#### Before Creating");

	org.eyedb.Object o = person_cls.newObj(db);
      
	name_attr.setSize(o, args[1].length()+1);
	name_attr.setStringValue(o, args[1]);

	age_attr.setValue(o, new org.eyedb.Value(Integer.parseInt(args[2])));
      
	o.trace();
	o.store();
	o.trace();

	db.transactionCommit();
	return o.getOid();
    }

    public static org.eyedb.Object reload_person(org.eyedb.Database db, org.eyedb.Oid oid)
	throws org.eyedb.Exception {
	db.transactionBegin();

	org.eyedb.Object o = db.loadObject(oid);
	System.out.println("Reloading object");
	o.trace();

	db.transactionCommit();
	return o;
    }

    public static void add_children(org.eyedb.Database db, org.eyedb.Object o)
	throws org.eyedb.Exception {
	db.transactionBegin();
	init_person_class(db);
	org.eyedb.Object xchildren = children_attr.getValue(o).sgetObject();
	org.eyedb.Collection children = (org.eyedb.Collection)xchildren;

	if (xchildren != null)
	    xchildren.trace();

	String base_name = name_attr.getStringValue(o);

	for (int n = 0; n < 10; n++) {
	    org.eyedb.Object child = person_cls.newObj(db);
	    String name = base_name + "_" + (n+1);
	    name_attr.setSize(child, name.length()+1);
	    name_attr.setStringValue(child, name);
	    age_attr.setValue(child, new org.eyedb.Value(n+1));
	    child.store();
	    children.insert(child.getOid());
	}

	o.store();
	o.trace();
	db.transactionCommit();
    }
}
