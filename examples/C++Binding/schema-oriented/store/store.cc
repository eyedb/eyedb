
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

int
main(int argc, char *argv[])
{
  // initializing the EyeDB layer
  eyedb::init(argc, argv);

  // initializing the person package
  person::init();

  if (argc != 5) {
    fprintf(stderr, "usage: %s <dbname> <person name> <person age> "
	    "<spouse name>\n", argv[0]);
    return 1;
  }

  const char *dbname = argv[1];
  const char *name = argv[2];
  int age = atoi(argv[3]);
  const char *spouse_name = argv[4];

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;

    // connecting to the EyeDB server
    conn.open();

    // opening database dbname using 'personDatabase' class
    personDatabase db(dbname);
    db.open(&conn, eyedb::Database::DBRW);

    // beginning a transaction
    db.transactionBegin();

    // first looking for spouse
    eyedb::OQL q(&db, "select Person.name = \"%s\"", spouse_name);

    eyedb::ObjectArray arr;
    q.execute(arr);

    // if not found, returns an error
    if (!arr.getCount()) {
      fprintf(stderr, "cannot find spouse '%s'\n", spouse_name);
      return 1;
    }

    // (safe!) casting returned object
    Person *spouse = Person_c(arr[0]);

    // creating a Person
    Person *p = new Person(&db);

    p->setCstate(Sir);
    p->setName(name);
    p->setAge(age);

    p->setSpouse(spouse);

    // spouse is no more necessary: releasing it
    spouse->release();

    p->getAddr()->setStreet("voltaire");
    p->getAddr()->setTown("paris");

    // creating two cars
    Car *car1 = new Car(&db);
    car1->setBrand("renault");
    car1->setNum(18374);

    Car *car2 = new Car(&db);
    car2->setBrand("ford");
    car2->setNum(233491);

    // adding the cars to the created person
    p->addToCarsColl(car1);
    p->addToCarsColl(car2);

    // car pointers are no more necessary: releasing them
    car1->release();
    car2->release();

    // creating ten children
    for (int i = 0; i < 10; i++) {
      Person *c = new Person(&db);
      char tmp[64];

      c->setAge(i);
      sprintf( tmp, "%d", i);
      c->setName( (std::string(name) + std::string("_") + std::string(tmp)).c_str() );
      p->setInChildrenCollAt(i, c);
      c->release();
    }

    // storing all in database
    p->store(eyedb::RecMode::FullRecurs);

    // committing the transaction
    db.transactionCommit();

    // releasing p
    p->release();
  }

  catch(eyedb::Exception &e) {
    std::cerr << argv[0] << ": " << e;
    eyedb::release();
    return 1;
  }

  // releasing the EyeDB layer: this is not mandatory
  eyedb::release();

  return 0;
}
