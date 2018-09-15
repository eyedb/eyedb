 
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

  if (argc != 4) {
    fprintf(stderr, "usage: %s <dbname> <person_name> <person_age>\n",
	    argv[0]);
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

    // looking for class 'Person'
    eyedb::Class *personClass = db.getSchema()->getClass("Person");

    // looking for the attribute 'name' and 'age' in the class 'Person'
    const eyedb::Attribute *name_attr = personClass->getAttribute("name");
    if (!name_attr) {
      fprintf(stderr, "cannot find name attribute in database\n");
      return 1;
    }

    const eyedb::Attribute *age_attr = personClass->getAttribute("age");

    if (!age_attr) {
      fprintf(stderr, "cannot find age attribute in database\n");
      return 1;
    }

    // instanciating a 'Person' object
    eyedb::Object *p = personClass->newObj(&db);

    // setting the name argv[2] to the new Person instance
    name_attr->setSize(p, strlen(argv[2])+1);
    name_attr->setValue(p, (eyedb::Data)argv[2], strlen(argv[2])+1, 0);

    // setting the age argv[3] to the new Person instance
    int age = atoi(argv[3]);
    age_attr->setValue(p, (eyedb::Data)&age, 1, 0);
    p->store();

    // committing the transaction
    db.transactionCommit();
  }

  catch(eyedb::Exception &e) {
    cerr << e;
    eyedb::release();
    return 1;
  }

  eyedb::release();

  return 0;
}
