
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
import java.util.*;

class SchemaDump {

    private static void usage() {
	System.err.println("usage: java SchemaDump dbname [system]");
	System.exit(1);
    }

    private static boolean system;

    public static void main(String args[]) {

	String[] outargs = org.eyedb.Root.init("SchemaDump", args);
	int n = outargs.length;

	if (n != 1 && n != 2) {
	    usage();
	}

	if (n == 2) {
	    if (outargs[1].equals("system"))
		system = true;
	    else
		usage();
	}
	else
	    system = false;

	try {
	    org.eyedb.Connection conn = new org.eyedb.Connection();

	    org.eyedb.Database db = new org.eyedb.Database(outargs[0]);
	    System.err.println("#### Before Opening");
	    db.open(conn, org.eyedb.Database.DBRead);
	    System.err.println("#### After Opening");

	    db.transactionBegin();
	    schemaDump(db);
	    db.transactionCommit();
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	}

	System.err.println("#### Done");
    }

    static private void schemaDump(org.eyedb.Database db) {
	Vector v = db.getSchema().getClassList();
	for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
	    classDump((org.eyedb.Class)e.nextElement());
	}
    }

    static private void classDump(org.eyedb.Class cls) {
	if (!system && cls.isSystem())
	    return;

	if (cls instanceof org.eyedb.EnumClass)
	    enumClassDump((org.eyedb.EnumClass)cls);
	else if (cls instanceof org.eyedb.BasicClass)
	    basicClassDump((org.eyedb.BasicClass)cls);
	else if (cls instanceof org.eyedb.StructClass)
	    structClassDump((org.eyedb.StructClass)cls);
	else if (system) {
	    if (cls instanceof org.eyedb.CollectionClass)
		collectionClassDump((org.eyedb.CollectionClass)cls);
	    else
		nativeClassDump(cls);
	}
    }

    static private void enumClassDump(org.eyedb.EnumClass cls) {
	System.out.println("enum " + cls.getName() + " " + cls.getOid() +
			   " {");
	org.eyedb.EnumItem items[] = cls.getEnumItems();
	for (int i = 0; i < items.length; i++) {
	    System.out.println("\t" + items[i].getName() + " = " +
			       items[i].getValue() +
			       ((i == items.length - 1) ? "" : ","));
	}
	System.out.println("};\n");
    }

    static private void basicClassDump(org.eyedb.BasicClass cls) {
	System.out.println(cls.getName() + " " + cls.getOid() +
			   ";\n");
			   
    }

    static private void collectionClassDump(org.eyedb.CollectionClass cls) {
	System.out.println("class " + cls.getName() + " " +
			   cls.getOid() + " {");
	System.out.println("\ttypename = " + cls.getTypename() + ";");
	System.out.println("\tinner class = " + cls.getCollClass().getName() +";");
	System.out.println("\tisref = " + cls.isRef() + ";");
	System.out.println("\tdimension = " + cls.getDimension() + ";");
	System.out.println("};\n");
    }

    static private void structClassDump(org.eyedb.StructClass cls) {
	System.out.println("class " + cls.getName() + " " + cls.getOid() +
			   " {");
	org.eyedb.Attribute attrs[] = cls.getAttributes();
	for (int i = 0; i < attrs.length; i++) {
	    org.eyedb.Attribute attr = attrs[i];
	    System.out.print("\t" + (attr.hasInverse() ? "relationship" :
				     "attribute") + " ");
	    System.out.print(attr.getFClass().getName() + " ");
	    if (attr.isIndirect())
		System.out.print("*");
	    if (!attr.getClassOwner().getOid().equals(cls.getOid()))
		System.out.print(attr.getClassOwner().getName() + "::");
	    System.out.print(attr.getName());
	    org.eyedb.TypeModifier typmod = attr.getTypeModifier();
	    for (int j = 0; j < typmod.getNdims(); j++) {
		String s = "";
		if (typmod.getDims()[j] > 0)
		    s = "" + typmod.getDims()[j];
		System.out.print("[" + s + "]");
	    }
	    if (attr.hasInverse()) {
		org.eyedb.AttrInverse inverse = attr.getInverse();
		System.out.print(" (inverse<" + inverse.getClassName() +
				   "::" + inverse.getAttrName() + ">)");
	    }
	    System.out.println(";");
	}
	System.out.println("};\n");
    }

    static private void nativeClassDump(org.eyedb.Class cls) {
	System.out.println("class " + cls.getName() + " " + cls.getOid() +
			   ";\n");
    }
}


