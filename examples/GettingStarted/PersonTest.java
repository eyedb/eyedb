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

import person.*;

class PersonTest {
  public static void main(String args[]) {

    // Initialize the eyedb package and parse the default eyedb options
    // on the command line
    String[] outargs = org.eyedb.Root.init("PersonTest", args);
     
    // Check that a database name is given on the command line
    int argc = outargs.length;
    if (argc != 1)
      {
        System.err.println("usage: java PersonTest dbname");
        System.exit(1);
      }

    try {
      // Initialize the person package
      person.Database.init();

      // Open the connection with the backend
      org.eyedb.Connection conn = new org.eyedb.Connection();

      // Open the database named outargs[0]
      person.Database db = new person.Database(outargs[0]);
      db.open(conn, org.eyedb.Database.DBRW);

      db.transactionBegin();
      // Create two persons john and mary
      Person john = new Person(db);
      john.setFirstname("john");
      john.setLastname("travolta");
      john.setAge(26);
     
      Person mary = new Person(db);
      mary.setFirstname("mary");
      mary.setLastname("stuart");
      mary.setAge(22);
     
      // Mary them ;-)
      john.setSpouse(mary);

      // Store john and mary in the database
      john.store(org.eyedb.RecMode.FullRecurs);

      john.trace();

      db.transactionCommit();
    }
    catch(org.eyedb.Exception e) { // Catch any eyedb exception
       e.print();
       System.exit(1);
    }
  }
}
