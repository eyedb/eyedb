
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

class Basic {

    public static void main(String args[]) {

	String[] outargs = org.eyedb.Root.init("Basic", args);
	int n = outargs.length;

	if (n != 1) {
	    System.err.println("usage: java Basic dbname");
	    System.exit(1);
	}

	try {
	    org.eyedb.Connection conn = new org.eyedb.Connection();

	    org.eyedb.Database db = new org.eyedb.Database(outargs[0]);
	    System.err.println("#### Before Opening");
	    db.open(conn, org.eyedb.Database.DBRead);
	    System.err.println("#### After Opening");
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	    System.exit(1);
	}

	System.err.println("#### Done");
    }
}
