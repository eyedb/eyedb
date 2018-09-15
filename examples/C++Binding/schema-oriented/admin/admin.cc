
/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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

#include <eyedb/eyedb.h>

using namespace std;

static const char *
get_string_mode(eyedb::DBAccessMode mode)
{
  if (mode == eyedb::NoDBAccessMode)
    return "eyedb::NoDBAccessMode";
  if (mode == eyedb::ReadDBAccessMode)
    return "eyedb::ReadDBAccessMode";
  if (mode == eyedb::WriteDBAccessMode)
    return "eyedb::WriteDBAccessMode";
  if (mode == eyedb::ExecDBAccessMode)
    return "eyedb::ExecDBAccessMode";
  if (mode == eyedb::ReadWriteDBAccessMode)
    return "eyedb::ReadWriteDBAccessMode";
  if (mode == eyedb::ReadExecDBAccessMode)
    return "eyedb::ReadExecDBAccessMode";
  if (mode == eyedb::ReadWriteExecDBAccessMode)
    return "eyedb::ReadWriteExecDBAccessMode";
  if (mode == eyedb::AdminDBAccessMode)
    return "eyedb::AdminDBAccessMode";

  return NULL;
}

int
main(int argc, char *argv[])
{
  // initializing the eyedb layer
  eyedb::init(argc, argv);

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;

    // connecting to the eyedb server
    conn.open();

    // opening the database EYEDBDBM using 'dbmDataBase' class
    eyedb::DBMDatabase db("EYEDBDBM");
    db.open(&conn, eyedb::Database::DBRead);

    // beginning a transaction
    db.transactionBegin();

    // display the scheme on stdout
    cout << db.getSchema() << endl;

    // looking for all user
    eyedb::OQL q_user(&db, "select user_entry");

    eyedb::ObjectArray user_arr;
    q_user.execute(user_arr);

    cout << "User List {" << endl;
    for (int i = 0; i < user_arr.getCount(); i++) {
      eyedb::UserEntry *user = (eyedb::UserEntry *)user_arr[i];
      cout << "\t" << user->name() << endl;
    }
    cout << "}\n" << endl;

    // looking for all database entry
    eyedb::OQL q_db(&db, "select database_entry");

    eyedb::ObjectArray db_arr;
    q_db.execute(db_arr);

    cout << "Database List {" << endl;

    for (int i = 0; i < db_arr.getCount(); i++) {
      eyedb::DBEntry *dbentry = (eyedb::DBEntry *)db_arr[i];
      cout << "\t" << dbentry->dbname() << " -> " << dbentry->dbfile() << endl;
      // looking for all user which has any permission on this
      // database
      eyedb::OQL q_useraccess(&db,
			      "select database_user_access->dbentry->dbname = \"%s\"",
			      dbentry->dbname().c_str());
      eyedb::ObjectArray useraccess_arr;
      q_useraccess.execute(useraccess_arr);
      if (useraccess_arr.getCount()) {
	cout << "\tUser Access {" << endl;
	for (int j = 0; j < useraccess_arr.getCount(); j++) {
	  eyedb::DBUserAccess *ua = (eyedb::DBUserAccess *)useraccess_arr[j];
	  cout << "\t\t" << ua->user()->name() << " -> " <<
	    get_string_mode(ua->mode()) << endl;
	}
	cout << "\t}" << endl;
      }
      cout << endl;
      useraccess_arr.garbage();
    }

    cout << "}" << endl;

    // releasing runtime pointers
    db_arr.garbage();
    user_arr.garbage();
  }

  catch(eyedb::Exception &e) {
    cerr << argv[0] << ": " << e;
    eyedb::release();
    return 1;
  }

  // releasing the eyedb layer: this is not mandatory
  eyedb::release();

  return 0;
}
