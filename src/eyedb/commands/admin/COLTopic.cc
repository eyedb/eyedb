
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

#include "eyedbconfig.h"

#include <eyedb/eyedb.h>
#include "eyedb/DBM_Database.h"
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <eyedblib/butils.h>
#include "GetOpt.h"
#include "COLTopic.h"

using namespace eyedb;
using namespace std;

//
// Global variables...
//

static Connection *conn;
static Database *db;
static const Attribute *attr;
static const Class *cls;

//
// Helper functions and macros
//

static const char *get_op(const char *s, int &offset);

static int
get_collimpl(const char *attrpath, CollAttrImpl *&collimpl)
{
  Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr, attrpath);
  s = Attribute::getCollAttrImpl(db, attrpath, collimpl);
  if (!attr->getClass()->asCollectionClass() || attr->isIndirect()) {
    fprintf(stderr, "attribute path %s: "
	    "a collection implementation can be tied "
	    "only to a literal collection attribute\n",
	    attrpath);
    return 1;
  }
  return 0;
}

static int
get_collimpls(const Class *cls, LinkedList &list)
{
  const LinkedList *cllist;
  Status s = const_cast<Class *>(cls)->getAttrCompList(Class::CollectionImpl_C, cllist);
    
  LinkedListCursor c(cllist);
  void *o;
  while (c.getNext(o))
    list.insertObject(o);
  return 0;
}

static int
get_collimpls(const char *info, LinkedList &list)
{
  Status s;
  if (info) {
    Status s;
    if (strchr(info, '.')) {
      s = Attribute::checkAttrPath(db->getSchema(), cls, attr, info);
      CollAttrImpl *collimpl;
      if (get_collimpl(info, collimpl))
	return 1;
      if (collimpl) {
	list.insertObject(collimpl);
	return 0;
      }
      return 0;
    }
    
    const Class *cls = db->getSchema()->getClass(info);
    if (!cls) {
      fprintf(stderr, "class '%s' not found\n", info);
      return 1;
    }
    
    return get_collimpls(cls, list);
  }

  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (get_collimpls(cls, list))
      return 1;
  }
  
  return 0;
}
static int
get_collections(const char *str, LinkedList &list)
{
  Status s;
  std::string query;

  int len = strlen(str);
  if (len > 4 && !strcmp(&str[len-4], ":oid")) {
    Oid oid(str);
    Object *o;
    s = db->loadObject(oid, o);
    list.insertObject(o);
    return 0;
  }

  Bool collname;
  if (len > 4 && !strcmp(&str[len-4], ":oql")) {
    char *x = new char[len-3];
    strncpy(x, str, len-4);
    x[len-4] = 0;
    query = x;
    free(x);
    
    collname = False;
  }
  else {
    query = std::string("select collection.name = \"") + str + "\"";
    collname = True;
  }

  OQL oql(db, query.c_str());
  ObjectArray obj_arr;
  s = oql.execute(obj_arr);
  if (!obj_arr.getCount() && collname) {
    fprintf(stderr, "No collection named '%s'\n", str);
    return 1;
  }

  for (int n = 0; n < obj_arr.getCount(); n++)
    if (obj_arr[n]->asCollection())
      list.insertObject(const_cast<Object *>(obj_arr[n]));
  return 0;
}

static void
collection_trace(Collection *coll)
{
  fprintf(stdout, "Collection %s:\n", coll->getOid().toString());
  fprintf(stdout, "  Name: '%s'\n", coll->getName());
  fprintf(stdout, "  Collection of: %s *\n", coll->getClass()->asCollectionClass()->getCollClass()->getName());
}


static int
get_colls(const char *name, ObjectArray &obj_arr)
{
  Oid oid(name);
  if (oid.isValid()) {
    Object *o;
    Status s = db->loadObject(oid, o);

    if (!o->asCollection()) {
      fprintf(stderr, "%s is not a collection\n", oid.toString());
      return 1;
    }
    Object **xo = new Object *[1];
    xo[0] = o;
    obj_arr.set(xo, 1);
    return 0;
  }

  int offset;
  const char *op = get_op(name, offset);
  OQL q(db, "select collection.name %s \"%s\"", op, &name[offset]);
  Status s = q.execute(obj_arr);

  if (!obj_arr.getCount()) {
    fprintf(stderr, "no collection %s found\n", name);
    return 1;
  }

  return 0;
}

static void
print(const Dataspace *dataspace, Bool def = False)
{
  if (!dataspace) {
    Status s = db->getDefaultDataspace(dataspace);
    if (s) {
      s->print();
      return;
    }
    //    printf("Default ");
    print(dataspace, True);
    return;
  }
  printf("  Dataspace #%d", dataspace->getId());
  if (def) printf(" (default)");
  printf("\n");
  printf("   Name %s\n", dataspace->getName());
  unsigned int datafile_cnt;
  const Datafile **datafiles = dataspace->getDatafiles(datafile_cnt);
  printf("   Composed of {\n", datafile_cnt);
  for (int i = 0; i < datafile_cnt; i++) {
    printf("     Datafile #%d\n", datafiles[i]->getId());
    if (*datafiles[i]->getName())
      printf("       Name %s\n", datafiles[i]->getName());
    printf("       File %s\n", datafiles[i]->getFile());
  }
  printf("   }\n");
}

const char *
get_op(const char *s, int &offset)
{
  if (s[0] == '~')
    {
      if (s[1] == '~')
	{
	  offset = 2;
	  return "~~";
	}

      offset = 1;
      return "~";
    }

  if (s[0] == '!')
    {
      if (s[1] == '~')
	{
	  if (s[2] == '~')
	    {
	      offset = 3;
	      return "!~~";
	    }

	  offset = 2;
	  return "!~";
	}
    }

  offset = 0;
  return "=";
}

//
// Topic definition
//

COLTopic::COLTopic() : Topic("collection")
{
  addAlias("coll");

  addCommand( new COLUpdateimplCmd(this));
  addCommand( new COLSimulimplCmd(this));
  addCommand( new COLGetimplCmd(this));
  addCommand( new COLStatsimplCmd(this));
  addCommand( new COLGetDefDSPCmd(this));
  addCommand( new COLSetDefDSPCmd(this));
  addCommand( new COLSetDefImplCmd(this));
  addCommand( new COLGetDefImplCmd(this));
  addCommand( new COLGetLocaCmd(this));
  addCommand( new COLMoveCmd(this));
}

static const std::string FULL_OPT("full");
static const std::string FORMAT_OPT("format");
static const std::string LOCA_OPT("loca");
static const std::string PROPAGATE_OPT("propagate");
static const std::string TYPE_OPT("type");

//eyedbadmin collection updateimpl DATABASE COLLECTION hashindex|btreeindex [HINTS]
// 
// COLUpdateimplCmd
//
void COLUpdateimplCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(TYPE_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Collection type (supported types are: hashindex, btreeindex)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int COLUpdateimplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION [HINTS]\n";
  return 1;
}

int COLUpdateimplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  getopt->displayOpt("HINTS", "Collection hints");
  return 1;
}

int COLUpdateimplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();

  const char *hints = "";
  if (argv.size() > 2)
    hints = argv[2].c_str();

  IndexImpl::Type type;
  if (map.find(TYPE_OPT) != map.end()) {
    const char *typeOption = map[TYPE_OPT].value.c_str();

    if (!strcmp(typeOption, "hashindex"))
      type = IndexImpl::Hash;
    else if (!strcmp(typeOption, "btreeindex"))
      type = IndexImpl::BTree;
    // TODO: add "noindex" case
    else
      return help();
  }

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  LinkedList list;
  if (get_collections(collection, list))
    return 1;

  // TODO: set collimpl according to implementation type
  IndexImpl *idximpl;
  Status s = IndexImpl::make(db, type, hints, idximpl);
  CollImpl collimpl(idximpl);

  LinkedListCursor c(list);
  Collection *coll;

  while (c.getNext((void *&)coll)) {
    coll->setImplementation( &collimpl);
    s = coll->store();
  }
    
  db->transactionCommit();

  return 0;
}

//eyedbadmin collection simulimpl [--full] [--format=FORMAT] DATABASE COLLECTION hash|btree [HINTS]
// 
// COLSimulimplCmd
//
void COLSimulimplCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FULL_OPT, OptionBoolType(), 0,
		 OptionDesc("Displays complete information")));

  opts.push_back(Option(FORMAT_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Statistics format", "FORMAT")));

  opts.push_back(Option(TYPE_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Index type (supported types are: hashindex, btreeindex)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int COLSimulimplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION [HINTS]\n";
  return 1;
}

int COLSimulimplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  getopt->displayOpt("HINTS", "Collection hints");
  return 1;
}

int COLSimulimplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();

  const char *hints = "";
  if (argv.size() > 2)
    hints = argv[2].c_str();

  IndexImpl::Type type;
  if (map.find(TYPE_OPT) != map.end()) {
    const char *typeOption = map[TYPE_OPT].value.c_str();

    if (!strcmp(typeOption, "hashindex"))
      type = IndexImpl::Hash;
    else if (!strcmp(typeOption, "btreeindex"))
      type = IndexImpl::BTree;
    else
      return help();
  }

  bool full = map.find(FULL_OPT) != map.end();

  const char *fmt = 0;
  if (map.find(FORMAT_OPT) != map.end())
    fmt = map[FORMAT_OPT].value.c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  IndexImpl *idximpl;

  Status s = IndexImpl::make(db, type, hints, idximpl);
    
  LinkedList list;
  if (get_collections( collection, list))
    return 1;

  LinkedListCursor c(list);
  Collection *coll;
  CollImpl *collimpl = new CollImpl((CollImpl::Type)idximpl->getType(), idximpl);

  int nn = 0;
  for (; c.getNext((void *&)coll); nn++) {
    if (fmt && idximpl->getType() == IndexImpl::Hash) {
      IndexStats *stats1, *stats2 = 0;
      if (coll->asCollArray())
	s = coll->asCollArray()->simulate(*collimpl, stats1, stats2);
      else
	s = coll->simulate(*collimpl, stats1);

      if (nn)
	fprintf(stdout, "\n");

      s = stats1->asHashIndexStats()->printEntries(fmt);

      delete stats1;
      if (stats2) {
	s = stats2->asHashIndexStats()->printEntries(fmt);
	
	delete stats2;
      }
    }
    else {
      std::string stats1, stats2;
      if (coll->asCollArray())
	s = coll->asCollArray()->simulate(*collimpl, stats1, stats2, True, (full)? eyedb::True : eyedb::False, "  ");
      else
	s = coll->simulate(*collimpl, stats1, True, (full)? eyedb::True : eyedb::False, "  ");

      if (nn)
	fprintf(stdout, "\n");

      collection_trace(coll);

      fprintf(stdout, stats1.c_str());
      if (stats2.size())
	fprintf(stdout, stats2.c_str());
    }
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection getimpl DATABASE COLLECTION...
// 
// COLGetimplCmd
//
void COLGetimplCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int COLGetimplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION...\n";
  return 1;
}

int COLGetimplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  return 1;
}

int COLGetimplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  for (int n = 1; n < argv.size(); n++) {
    LinkedList list;
    if (get_collections(argv[n].c_str(), list))
      return 1;

    LinkedListCursor c(list);
    Collection *coll;
    for (int n = 0; c.getNext((void *&)coll); n++) {
      CollImpl *collimpl;
      Status s = coll->getImplementation(collimpl, True);

      if (n)
	fprintf(stdout, "\n");

      collection_trace(coll);
      if (collimpl->getIndexImpl()) {
	fprintf(stdout, collimpl->getIndexImpl()->toString("  ").c_str());
      }
    }
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection statsimpl [--full] [--format=FORMAT] DATABASE COLLECTION...
// 
// COLStatsimplCmd
//
void COLStatsimplCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FULL_OPT, OptionBoolType(), 0,
		 OptionDesc("Displays complete information")));

  opts.push_back(Option(FORMAT_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Statistics format", "FORMAT")));

  getopt = new GetOpt(getExtName(), opts);
}

int COLStatsimplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION...\n";
  return 1;
}

int COLStatsimplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  return 1;
}

int COLStatsimplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();

  bool full = map.find(FULL_OPT) != map.end();

  const char *fmt = 0;
  if (map.find(FORMAT_OPT) != map.end())
    fmt = map[FORMAT_OPT].value.c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  int nn = 0;
  for (int n = 1; n < argv.size(); n++) {
    Status s;
    LinkedList list;
    if (get_collections(argv[n].c_str(), list))
      return 1;
    
    LinkedListCursor c(list);
    Collection *coll;
    for (; c.getNext((void *&)coll); nn++) {
      CollImpl *_collimpl = 0;
      s = coll->getImplementation(_collimpl);

      if (fmt && _collimpl->getIndexImpl()->getType() == IndexImpl::Hash) {
	IndexStats *stats1, *stats2 = 0;
	if (coll->asCollArray())
	  s = coll->asCollArray()->getImplStats(stats1, stats2);
	else
	  s = coll->getImplStats(stats1);

	if (nn) fprintf(stdout, "\n");
	s = stats1->asHashIndexStats()->printEntries(fmt);

	delete stats1;
	if (stats2) {
	  s = stats2->asHashIndexStats()->printEntries(fmt);

	  delete stats2;
	}
      }
      else {
	std::string stats1, stats2;
	if (coll->asCollArray())
	  s = coll->asCollArray()->getImplStats(stats1, stats2,	True, (full)? eyedb::True : eyedb::False, "  ");
	else
	  s = coll->getImplStats(stats1, True, (full)? eyedb::True : eyedb::False, "  ");

	if (nn) fprintf(stdout, "\n");
	collection_trace(coll);
	fprintf(stdout, stats1.c_str());
	if (stats2.size())
	  fprintf(stdout, stats2.c_str());
      }
      if (_collimpl)
	_collimpl->release();
    }
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection getdefdsp DATABASE COLLECTION
// 
// COLGetDefDSPCmd
//
void COLGetDefDSPCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int COLGetDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION\n";
  return 1;
}

int COLGetDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  return 1;
}

int COLGetDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  ObjectArray obj_arr;
  if (get_colls(collection, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = coll->getDefaultDataspace(dataspace);

    if (i)
      printf("\n");
    printf("Default dataspace for collection ");

    if (*coll->getName())
      printf("'%s' ", coll->getName());
    printf("{%s}\n", coll->getOid().toString());
    print(dataspace);
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection setdefdsp DATABASE COLLECTION DATASPACE
// 
// COLSetDefDSPCmd
//
void COLSetDefDSPCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int COLSetDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION DSPNAME\n";
  return 1;
}

int COLSetDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int COLSetDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();
  const char *dspName = argv[2].c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  const Dataspace *dataspace;
  Status s = db->getDataspace(dspName, dataspace);

  ObjectArray obj_arr;
  if (get_colls(collection, obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    Status s = coll->setDefaultDataspace(dataspace);
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection getdefimpl DATABASE [CLASSNAME|ATTRIBUTE_PATH]...
// 
// COLGetDefImplCmd
//
void COLGetDefImplCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int COLGetDefImplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH...\n";
  return 1;
}

int COLGetDefImplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  return 1;
}

int COLGetDefImplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  int nn = 0;
  for (int n = 1; n < argv.size(); n++) {
    const char *info = argv[n].c_str();
    LinkedList list;
    if (get_collimpls(info, list))
      return 1;

    if (list.getCount()) {
      LinkedListCursor c(list);
      CollAttrImpl *collattrimpl;
      
      for (; c.getNext((void *&)collattrimpl); nn++) {
	if (nn) fprintf(stdout, "\n");
	fprintf(stdout, "Default implementation on %s:\n",
		collattrimpl->getAttrpath().c_str());
	const CollImpl *collimpl = 0;
	Status s = collattrimpl->getImplementation(db, collimpl);

	if (collimpl->getIndexImpl()) {
	  fprintf(stdout, collimpl->getIndexImpl()->toString("  ").c_str());
	}
      }
    }
    else if (strchr(info, '.')) {
      fprintf(stdout, "Default implementation on %s:\n", info);
      fprintf(stdout, "  System default\n");
    }
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection setdefimpl DATABASE ATTRIBUTE_PATH hashindex|btreeindex [HINTS] [propagate=on|off]
// 
// COLSetDefImplCmd
//
void COLSetDefImplCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  std::vector<std::string> propagate_choices;
  const std::string ON("on");
  const std::string OFF("off");
  propagate_choices.push_back(ON);
  propagate_choices.push_back(OFF);
  opts.push_back(Option(PROPAGATE_OPT, 
			OptionChoiceType("on_off",propagate_choices,ON),
			Option::MandatoryValue,
			OptionDesc("Propagation type", "on|off")));

  opts.push_back(Option(TYPE_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Collection type (supported types are: hashindex, btreeindex)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int COLSetDefImplCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH [HINTS]\n";
  return 1;
}

int COLSetDefImplCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("HINTS", "Collection hints");
  return 1;
}

int COLSetDefImplCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *attrpath = argv[1].c_str();

  const char *hints = "";
  if (argv.size() > 2)
    hints = argv[2].c_str();

  IndexImpl::Type type;
  if (map.find(TYPE_OPT) != map.end()) {
    const char *typeOption = map[TYPE_OPT].value.c_str();

    if (!strcmp(typeOption, "hashindex"))
      type = IndexImpl::Hash;
    else if (!strcmp(typeOption, "btreeindex"))
      type = IndexImpl::BTree;
    else
      return help();
  }

  Bool propag = eyedb::True;
    
  if (map.find(PROPAGATE_OPT) != map.end()) {
    if( !strcmp(map[PROPAGATE_OPT].value.c_str(), "on"))
      propag = eyedb::True;
    else if( !strcmp(map[PROPAGATE_OPT].value.c_str(), "off"))
      propag = eyedb::False;
    else
      return help();
  }

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBegin();

  Status s;
  const Attribute *attr;
  const Class *cls;
  s = Attribute::checkAttrPath(db->getSchema(), cls, attr, attrpath);

  IndexImpl *idximpl;
  s = IndexImpl::make(db, type, hints, idximpl);

  CollAttrImpl *collimpl;
  if (get_collimpl(attrpath, collimpl))
    return 1;

  if (collimpl) {
    s = collimpl->remove();
  }

  unsigned int impl_hints_cnt;
  const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
  collimpl = new CollAttrImpl(db, const_cast<Class *>(cls), attrpath, propag, idximpl->getDataspace(),
			      (CollImpl::Type)idximpl->getType(),
			      idximpl->getType() == IndexImpl::Hash ?
			      idximpl->getKeycount() : idximpl->getDegree(),
			      idximpl->getHashMethod(),
			      impl_hints, impl_hints_cnt);
    
  s = collimpl->store();

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection getloca [--stats|--loca|--all] DATABASE COLLECTION
// 
// COLGetLocaCmd
//
void COLGetLocaCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(ALL_OPT, OptionBoolType()));
  opts.push_back( Option(LOCA_OPT, OptionBoolType()));
  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int COLGetLocaCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION\n";
  return 1;
}

int COLGetLocaCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  return 1;
}

static const unsigned int LocaOpt = 1;
static const unsigned int StatsOpt = 2;

int COLGetLocaCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();

  unsigned int lopt = 0;
  if (map.find( STATS_OPT) != map.end())
    lopt = StatsOpt;
  else if (map.find( LOCA_OPT) != map.end())
    lopt = LocaOpt;
  else if (map.find( ALL_OPT) != map.end())
    lopt = StatsOpt|LocaOpt;
  else
    return help();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  ObjectArray obj_arr;
  if (get_colls(collection, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    ObjectLocationArray locarr;
    Status s = coll->getElementLocations(locarr);

    if (lopt & LocaOpt)
      cout << locarr(db) << endl;

    if (lopt & StatsOpt) {
      PageStats *pgs = locarr.computePageStats(db);
      cout << *pgs;
      delete pgs;
    }
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin collection move DATABASE COLLECTION DESTDATASPACE
// 
// COLMoveCmd
//
void COLMoveCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int COLMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION DESTDATASPACE\n";
  return 1;
}

int COLMoveCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  getopt->displayOpt("DESTDATASPACE", "Destination dataspace name");
  return 1;
}

int COLMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *collection = argv[1].c_str();
  const char *dspname = argv[2].c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  const Dataspace *dataspace;
  Status s = db->getDataspace(dspname, dataspace);

  ObjectArray obj_arr;
  if (get_colls(collection, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    Status s = coll->moveElements( dataspace);
  }

  db->transactionCommit();

  return 0;
}
