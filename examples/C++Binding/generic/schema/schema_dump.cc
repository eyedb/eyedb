 
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
schema_dump(FILE *fd, const eyedb::Schema *),
  class_dump(FILE *fd, const eyedb::Class *),
  enum_class_dump(FILE *fd, const eyedb::EnumClass *),
  struct_class_dump(FILE *fd, const eyedb::StructClass *);

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <dbname>\n", argv[0]);
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

    // dumping the schema
    schema_dump(stdout, db.getSchema());

    // committing transaction
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
schema_dump(FILE *fd, const eyedb::Schema *sch)
{
  // getting class list
  const eyedb::LinkedList *list = const_cast<eyedb::Schema *>(sch)->getClassList();
  // foreach class in list, dump the class
  eyedb::LinkedListCursor c(list);
  const eyedb::Class *cls;
  while (c.getNext((void *&)cls))
    class_dump(fd, cls);
}

static void
class_dump(FILE *fd, const eyedb::Class *cls)
{
  // do not dump system classes
  if (cls->isSystem())
    return;

  // do not dump collection and basic classes
  if (cls->asCollectionClass() || cls->asBasicClass())
    return;

  if (cls->asEnumClass()) {
    enum_class_dump(fd, cls->asEnumClass());
    return;
  }

  if (cls->asStructClass()) {
    struct_class_dump(fd, cls->asStructClass());
    return;
  }

  eyedb::Exception::make("unknown class type '%s'", cls->getName());
}

static void
enum_class_dump(FILE *fd, const eyedb::EnumClass *cls)
{
  fprintf(fd, "enum %s {\n", cls->getName());

  int enum_item_cnt;
  const eyedb::EnumItem **enum_items = cls->getEnumItems(enum_item_cnt);

  for (int i = 0; i < enum_item_cnt; i++)
    fprintf(fd, "\t%s = %d%s\n", enum_items[i]->getName(),
	    enum_items[i]->getValue(), 
	    (i != enum_item_cnt-1 ? "," : ""));

  fprintf(fd, "};\n\n", cls->getName());
}

static void
struct_class_dump(FILE *fd, const eyedb::StructClass *cls)
{
  fprintf(fd, "class %s", cls->getName());
  if (cls->getParent() && !cls->getParent()->isSystem())
    fprintf(fd, " extends %s", cls->getParent()->getName());

  fprintf(fd, " {\n");
  unsigned int attr_cnt;
  const eyedb::Attribute **attrs = cls->getAttributes(attr_cnt);

  for (int i = 0; i < attr_cnt; i++) {
    const eyedb::Attribute *attr = attrs[i];

    if (attr->isNative())
      continue;
      
    eyedb::Bool upClassAttr =
      strcmp(attr->getClassOwner()->getName(), cls->getName()) ? eyedb::True :
      eyedb::False;

    fprintf(fd, "\t");
    if (upClassAttr)
      fprintf(fd, "// ");

    eyedb::Bool strdim = attr->isString() ? eyedb::True : eyedb::False;

    if (strdim) {
      fprintf(fd, "attribute string");
      if (attr->getTypeModifier().ndims == 1 &&
	  attr->getTypeModifier().dims[0] > 0)
	fprintf(fd, "<%d>", attr->getTypeModifier().dims[0]);
    }
    else {
      fprintf(fd, "%s %s",
	      (attr->hasInverse() ? "relationship" : "attribute"),
	      attr->getClass()->getName());

      if (attr->isIndirect())
	fprintf(fd, "*");
    }

    if (upClassAttr)
      fprintf(fd, " %s::%s", attr->getClassOwner()->getName(),
	      attr->getName());
    else
      fprintf(fd, " %s", attr->getName());

    if (!strdim)
      for (int j = 0; j < attr->getTypeModifier().ndims; j++) {
	if (attr->getTypeModifier().dims[j] < 0)
	  fprintf(fd, "[]");
	else
	  fprintf(fd, "[%d]", attr->getTypeModifier().dims[j]);
      }
    fprintf(fd, ";\n");
  }

  fprintf(fd, "};\n\n", cls->getName());
}
