
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

static void
object_dump(FILE *fd, eyedb::Object *),
  collection_dump(FILE *fd, eyedb::Collection *),
  attr_dump(FILE *fd, const eyedb::Attribute *attr, int offset, eyedb::Object *o);

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

    // performing the OQL query argv[2]
    eyedb::OQL q(&db, argv[2]);

    eyedb::ObjectArray obj_arr;
    q.execute(obj_arr);

    // for each value returned in the query, display it:
    for (int i = 0; i < obj_arr.getCount(); i++) {
      object_dump(stdout, obj_arr[i]);
      fprintf(stdout, ";\n\n");
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

static void
object_dump(FILE *fd, eyedb::Object *o)
{
  // to avoid recursion
  if (o->getUserData())
    return;
  o->setUserData((void *)1);

  const eyedb::Class *cls = o->getClass();
  if (!cls->asStructClass())
    eyedb::Exception::make("cannot display non struct object");

  unsigned int attr_cnt;
  const eyedb::Attribute **attrs = cls->getAttributes(attr_cnt);

  fprintf(fd, "object %s of class %s {\n", o->getOid().toString(),
	  cls->getName());

  for (int i = 0; i < attr_cnt; i++) {
    const eyedb::Attribute *attr = attrs[i];

    if (attr->isNative())
      continue;
      
    fprintf(fd, "\t%s = ", attr->getName());
    eyedb::Bool isnull;
    if (attr->isString()) {
      eyedb::Data s;
      attr->getValue(o, &s, eyedb::Attribute::directAccess, 0, &isnull);
      if (isnull) fprintf(fd, "NULL");
      else fprintf(fd, "\"%s\"", s);
    }
    else {
      eyedb::TypeModifier typmod = attr->getTypeModifier();
      eyedb::Size sz = 1;
      if (attr->isVarDim())
	attr->getSize(o, sz);
      int rsz = typmod.pdims * sz;

      if (rsz)
	for (int i = 0; i < rsz; i++) {
	  if (i) fprintf(fd, ", ");
	  attr_dump(fd, attr, i, o);
	}
      else
	fprintf(fd, "NULL");
    }

    fprintf(fd, ";\n");
  }

  fprintf(fd, "}", cls->getName());
  o->setUserData((void *)0);
}

static void
collection_dump(FILE *fd, eyedb::Collection *o)
{
  fprintf(fd, "{\n");
  fprintf(fd, "\t\tname = \"%s\";\n", o->getName());
  fprintf(fd, "\t\tcount = %d;\n", o->getCount());
  fprintf(fd, "\t\tcontents = {\n");

  eyedb::ObjectArray obj_arr;
  o->getElements(obj_arr);
  for (int i = 0; i < obj_arr.getCount(); i++) {
    object_dump(fd, obj_arr[i]);
    fprintf(fd, ";\n");
  }

  fprintf(fd, "\t\t}\n");
  fprintf(fd, "\t}");
}

static void
attr_dump(FILE *fd, const eyedb::Attribute *attr, int offset, eyedb::Object *o)
{
  eyedb::Bool isnull;

  if (attr->isIndirect()) {
    eyedb::Oid oid;
    attr->getOid(o, &oid, 1, 0);
    fprintf(fd, "%s", oid.toString());
  }
  else if (attr->getClass()->asAgregatClass()) {
    eyedb::Object *oo;
    attr->getValue(o, (eyedb::Data *)&oo, 1, offset, &isnull);
    object_dump(fd, oo);
  }
  else if (attr->getClass()->asInt16Class()) {
    eyedblib::int16 v;
    attr->getValue(o, (eyedb::Data *)&v, 1, offset, &isnull);
    if (isnull) fprintf(fd, "NULL");
    else fprintf(fd, "%d", v);
  }
  else if (attr->getClass()->asInt32Class()) {
    eyedblib::int32 v;
    attr->getValue(o, (eyedb::Data *)&v, 1, offset, &isnull);
    if (isnull) fprintf(fd, "NULL");
    else fprintf(fd, "%d", v);
  }
  else if (attr->getClass()->asInt64Class()) {
    eyedblib::int64 v;
    attr->getValue(o, (eyedb::Data *)&v, 1, offset, &isnull);
    if (isnull) fprintf(fd, "NULL");
    else fprintf(fd, "%d", v);
  }
  else if (attr->getClass()->asFloatClass()) {
    double v;
    attr->getValue(o, (eyedb::Data *)&v, 1, offset, &isnull);
    if (isnull) fprintf(fd, "NULL");
    else fprintf(fd, "%f", v);
  }
  else if (attr->getClass()->asCharClass()) {
    char v;
    attr->getValue(o, (eyedb::Data *)&v, 1, offset, &isnull);
    if (isnull) fprintf(fd, "NULL");
    else fprintf(fd, "'%c'", v);
  }
  else if (attr->getClass()->asCollectionClass()) {
    eyedb::Object *oo;
    attr->getValue(o, (eyedb::Data *)&oo, 1, offset);
    collection_dump(fd, oo->asCollection());
  }
}
