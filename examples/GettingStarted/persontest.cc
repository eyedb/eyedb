
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
#include <eyedblib/strutils.h>

static void
create(eyedb::Database *db)
{
  db->transactionBegin();

  Person *john = new Person(db);
  john->setFirstname("john");
  john->setLastname("wayne");
  john->setAge(32);
  john->getAddr()->setStreet("courcelles");
  john->getAddr()->setTown("Paris");

  Person *mary = new Person(db);
  mary->setFirstname("mary");
  mary->setLastname("poppins");
  mary->setAge(30);
  mary->getAddr()->setStreet("courcelles");
  mary->getAddr()->setTown("Paris");

  // mary them
  john->setSpouse(mary);

  // creates children
  for (int i = 0; i < 3; i++) {
    std::string name = std::string("baby") + str_convert(i+1);
    Person *child = new Person(db);
    child->setFirstname(name.c_str());
    child->setLastname(name.c_str());
    child->setAge(1+i);
    john->addToChildrenColl(child);
    child->release();
  }

  john->store(eyedb::FullRecurs);

  mary->release();
  john->release();

  db->transactionCommit();
}

static void
read(eyedb::Database *db, const char *s)
{
  db->transactionBegin();

  eyedb::OQL q(db, "select Person.lastname ~ \"%s\"", s);

  eyedb::ObjectArray obj_arr;
  q.execute(obj_arr);

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Person *p   = Person_c(obj_arr[i]);
    if (p)
      printf("person = %s %s, age = %d\n", p->getFirstname().c_str(),
	     p->getLastname().c_str(), p->getAge());
  }

  db->transactionCommit();
}

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);
  person::init();

  if (argc != 2) {
    fprintf(stderr, "usage: %s <dbname>\n", argv[0]);
    return 1;
  }

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;

    conn.open();

    personDatabase db(argv[1]);

    db.open(&conn, eyedb::Database::DBRW);

    create(&db);

    read(&db, "baby");
  }
  catch(eyedb::Exception &e) {
    e.print();
  }

  person::release();
  eyedb::release();

  return 0;
}

