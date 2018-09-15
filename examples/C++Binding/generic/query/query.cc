
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

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  if (argc != 3) {
    fprintf(stderr, "usage: %s <dbname> <query>\n", argv[0]);
    return 1;
  }

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;
    // connecting to the eyedb server
    conn.open();

    eyedb::Database db(argv[1]);

    // opening database argv[1]
    db.open(&conn, eyedb::Database::DBRW);

    // beginning a transaction
    db.transactionBegin();

    // performing the OQL query argv[2] using the eyedb::OQL interface
    eyedb::OQL q(&db, argv[2]);

    eyedb::ValueArray arr;
    q.execute(arr);

    cout << "###### Performing the OQL query " << argv[2] <<
      " using the eyedb::OQL interface" << endl;

    // for each value returned in the query, display it:
    for (int i = 0; i < arr.getCount(); i++) {
      // in case of the returned value is an oid, load it first:
      if (arr[i].type == eyedb::Value::tOid) {
	eyedb::Object *o;
	db.loadObject(arr[i].oid, &o);
	cout << o;
	o->release();
      }
      else
	cout << arr[i] << endl;
    }

    // performing the same query using eyedb::OQLIterator interface
    // [1]: getting all returned values

    cout << "\n###### Performing the same query using eyedb::OQLIterator "
      "interface: getting all returned values" << endl;

    eyedb::OQLIterator iter(&db, argv[2]);
    eyedb::Value val;

    while (iter.next(val)) {
      // in case of the returned value is an oid, load it first:
      if (val.getType() == eyedb::Value::tOid) {
	eyedb::Object *o;
	db.loadObject(val.oid, &o);
	cout << o;

	// in case of the returned object is a collection, display its
	// contents
	if (o->asCollection()) {
	  eyedb::CollectionIterator citer(o->asCollection());
	  cout << "contents BEGIN\n";
	  eyedb::Object *co;
	  while(citer.next(co)) {
	    cout << co;
	    co->release();
	  }
	  cout << "contents END\n\n";
	}
	// in case of the returned object is a class, display its
	// extent
	else if (o->asClass()) {
	  eyedb::ClassIterator citer(o->asClass());
	  cout << "extent BEGIN\n";
	  eyedb::Object *co;
	  while(citer.next(co)) {
	    cout << co;
	    co->release();
	  }
	  cout << "extent END\n\n";
	}

	o->release();
      }
      else
	cout << val << endl;
    }

    // [2]: getting only returned objects

    cout << "\n###### Performing the same query using eyedb::OQLIterator "
      "interface: getting only returned objects" << endl;

    eyedb::OQLIterator iter2(&db, argv[2]);
    eyedb::Object *o;

    while (iter2.next(o)) {
      cout << o;
      o->release();
    }

    // committing the transaction
    db.transactionCommit();
  }

  catch(eyedb::Exception &e) {
    cerr << argv[0] << ": " << e;
    eyedb::release();
    return 1;
  }

  eyedb::release();

  return 0;
}
