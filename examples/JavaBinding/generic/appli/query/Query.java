
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

class Query {

    public static void main(String args[]) {

	String[] outargs = org.eyedb.Root.init("Query", args);
	int n = outargs.length;

	if (n != 2) {
	    System.err.println("usage: java Query dbname query");
	    System.exit(1);
	}

	try {
	    org.eyedb.Connection conn = new org.eyedb.Connection();

	    org.eyedb.Database db = new org.eyedb.Database(outargs[0]);
	    System.err.println("#### Before Opening");
	    db.open(conn, org.eyedb.Database.DBRead);
	    System.err.println("#### After Opening");

	    db.transactionBegin();

	    System.err.println("#### Before Querying");
	    String oql_str = outargs[1];

	    // one way to perform the query
	    query_1(db, oql_str);

	    // another way to perform the same query
	    query_2(db, oql_str);

	    // yet another way to perform the same query
	    query_3(db, oql_str);

	    // yet yet another way to perform the same query
	    query_4(db, oql_str);

	    // yet yet yet another way to perform the same query
	    query_5(db, oql_str);

	    conn.close();
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	}

	System.err.println("#### Done");
    }

    public static void query_1(org.eyedb.Database db, String oql_str)
	throws org.eyedb.Exception {
	System.out.println("\n#### Query #1 ####");
	org.eyedb.OQL q  = new org.eyedb.OQL(db, oql_str);
	org.eyedb.Value v = new org.eyedb.Value();
	q.execute(v);
	System.out.println("#### Full Result " + v.toString());
    }

    public static void query_2(org.eyedb.Database db, String oql_str)
	throws org.eyedb.Exception {
	System.out.println("\n#### Query #2 ####");

	org.eyedb.OQLIterator iter = new org.eyedb.OQLIterator(db, oql_str);

	org.eyedb.Value v;
	for (int n = 0; (v = iter.nextValue()) != null; n++) {
	    System.out.println("#### Result #" + n + ": " + v.toString());
	    if (v.getType() == org.eyedb.Value.OID)  {
		org.eyedb.Object o = db.loadObject(v.sgetOid());
		o.trace(System.out);
		if (o instanceof org.eyedb.Collection)  {
		    System.out.println("#### Contents BEGIN");
		    org.eyedb.CollectionIterator citer =
			new org.eyedb.CollectionIterator((org.eyedb.Collection)o);
		    org.eyedb.Object co;
		    while ((co = citer.nextObject()) != null)
			co.trace(System.out);
		    System.out.println("#### Contents END");
		}
		else if (o instanceof org.eyedb.Class) {
		    System.out.println("#### Extent BEGIN");
		    org.eyedb.ClassIterator citer =
			new org.eyedb.ClassIterator((org.eyedb.Class)o);
		    org.eyedb.Object co;
		    while ((co = citer.nextObject()) != null)
			co.trace(System.out);
		    System.out.println("#### Extent END");
		}
	    }
	    n++;
	}
    }

    public static void query_3(org.eyedb.Database db, String oql_str)
	throws org.eyedb.Exception {
	System.out.println("\n#### Query #3 ####");

	org.eyedb.OQL q = new org.eyedb.OQL(db, oql_str);
	org.eyedb.ValueArray val_arr = new org.eyedb.ValueArray();

	q.execute(val_arr);
	for (int i = 0; i < val_arr.getCount(); i++) {
	    System.out.println("#### Value #" + i + ": " +
			       val_arr.getValue(i).toString());
	}
    }

    public static void query_4(org.eyedb.Database db, String oql_str)
	throws org.eyedb.Exception {
	System.out.println("\n#### Query #4 ####");

	org.eyedb.OQL q = new org.eyedb.OQL(db, oql_str);
	org.eyedb.OidArray oid_arr = new org.eyedb.OidArray();

	q.execute(oid_arr);
	for (int i = 0; i < oid_arr.getCount(); i++) {
	    System.out.println("#### Oid #" + i + ": " +
			       oid_arr.getOid(i).toString());
	}
    }

    public static void query_5(org.eyedb.Database db, String oql_str)
	throws org.eyedb.Exception {
	System.out.println("\n#### Query #5 ####");

	org.eyedb.OQL q = new org.eyedb.OQL(db, oql_str);
	org.eyedb.ObjectArray obj_arr = new org.eyedb.ObjectArray();
	q.execute(obj_arr, org.eyedb.RecMode.FullRecurs);

	for (int i = 0; i < obj_arr.getCount(); i++) {
	    System.out.println("#### Object #" + i + ": ");
	    obj_arr.getObject(i).trace();
	}
    }
}
