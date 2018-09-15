
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

class TestDataspace {

    public static void main(String args[]) {

	String[] outargs = org.eyedb.Root.init("TestDataspace", args);
	int n = outargs.length;

	if (n != 1){
	    System.err.println("usage: java TestDataspace <dbname>");
	    System.exit(1);
	}

	try {
	    org.eyedb.Connection conn = new org.eyedb.Connection();

	    System.err.println("#### Before Opening");

	    dspsch.Database.init();
	    dspsch.Database db = new dspsch.Database(outargs[0]);
	    db.open(conn, org.eyedb.Database.DBRW);
	    System.err.println("#### After Opening");

	    db.transactionBegin();
	    org.eyedb.DatabaseInfoDescription info = db.getInfo();
	    info.trace(System.out);

	    org.eyedb.Datafile datafiles[] = db.getDatafiles();
	    for (int m = 0; m < datafiles.length; m++) {
		System.out.println(datafiles[m]);
	    }

	    org.eyedb.Dataspace dataspaces[] = db.getDataspaces();
	    for (int m = 0; m < dataspaces.length; m++) {
		System.out.println(dataspaces[m]);
	    }

	    dspsch.O4 o4 = new dspsch.O4(db);

	    org.eyedb.Dataspace dataspace = o4.getDataspace();
	    System.out.println("before store: " + dataspace);

	    o4.store();
	    System.out.println("after store: " + dataspace);

	    o4 = (dspsch.O4)db.loadObject(o4.getOid());
	    System.out.println("after reload (same transaction): " + dataspace);

	    db.transactionCommit();
	    db.transactionBegin();
	    
	    o4 = (dspsch.O4)db.loadObject(o4.getOid());
	    System.out.println("after reload (other transaction): " + dataspace);
	    o4 = new dspsch.O4(db);
	    o4.setDataspace(db.getDataspace((short)2));

	    dataspace = o4.getDataspace();
	    System.out.println("before store: " + dataspace);

	    o4.store();
	    System.out.println("after store: " + dataspace);

	    o4 = (dspsch.O4)db.loadObject(o4.getOid());
	    System.out.println("after reload (same transaction): " + dataspace);

	    db.transactionCommit();
	    db.transactionBegin();
	    
	    o4 = (dspsch.O4)db.loadObject(o4.getOid());
	    System.out.println("after reload (other transaction): " + dataspace);
	    o4.move(db.getDataspace((short)3));
	    dataspace = o4.getDataspace();

	    System.out.println("after move (other transaction): " + dataspace);
	    db.transactionCommit();
	    db.transactionBegin();
	    o4 = (dspsch.O4)db.loadObject(o4.getOid());
	    dataspace = o4.getDataspace();

	    System.out.println("after reload (other transaction): " + dataspace);

	    db.close();
	    conn.close();
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	}
    }
}
