
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

#include "person.h"

using namespace std;

int
main(int argc, char *argv[])
{
  // initializing the EyeDB layer
  eyedb::init(argc, argv);

  // initializing the person package
  person::init();

  if (argc != 4) {
    fprintf(stderr, "usage: %s <dbname> <person name> <person age>\n",
	    argv[0]);
    return 1;
  }

  const char *dbname = argv[1];
  const char *name = argv[2];
  int age = atoi(argv[3]);

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;

    // connecting to the EyeDB server
    conn.open();

    // opening database dbname using 'personDatabase' class
    //    personDatabase db(dbname);
    eyedb::Database db(dbname);
    db.open(&conn, (getenv("EYEDBLOCAL") ?
		    eyedb::Database::DBRWLocal :
		    eyedb::Database::DBRW));
    
    // beginning a transaction
    db.transactionBegin();

    // creating a Person
    Person *p = new Person(&db);

    p->setCstate(Sir);
    p->setName(name);
    p->setAge(age);

    p->getAddr()->setStreet("voltaire");
    p->getAddr()->setTown("paris");

    // storing all in database
    p->store(eyedb::RecMode::FullRecurs);

    int r;
    
    char *oldstreet, *oldtown;

    p->change_address("champs elysees", "new york", oldstreet, oldtown, r);

    cout << "oldstreet: " << oldstreet << endl;
    cout << "oldtown: " << oldtown << endl;
    cout << "change_address returns -> " << r << endl;

    eyedb::Argument::free(oldstreet);
    eyedb::Argument::free(oldtown);

    // committing the transaction
    db.transactionCommit();

    // releasing p
    p->release();
    db.close();
  }

  catch(eyedb::Exception &e) {
    cerr << argv[0] << ": " << e;
    eyedb::release();
    return 1;
  }

  // releasing the EyeDB layer: this is not mandatory
  person::release();
  eyedb::release();

  return 0;
}
