/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-1999,2004-2006 SYSRA
   
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
  Author: Francois Dechelle <francois@dechelle.net>
*/

#include "eyedbconfig.h"
#include <eyedb/eyedb.h>
#include <eyedblib/butils.h>
#include "eyedb/DBM_Database.h"
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <eyedblib/butils.h>
#include "GetOpt.h"
#include "IDXTopic.h"

using namespace eyedb;
using namespace std;

IDXTopic::IDXTopic() : Topic("index")
{
  addAlias("idx");

  addCommand(new IDXCreateCmd(this));
  addCommand(new IDXDeleteCmd(this));
  addCommand(new IDXUpdateCmd(this));
  addCommand(new IDXListCmd(this));
  addCommand(new IDXStatsCmd(this));
  addCommand(new IDXSimulateCmd(this));
  addCommand(new IDXMoveCmd(this));
  addCommand(new IDXSetdefdspCmd(this));
  addCommand(new IDXGetdefdspCmd(this));
  addCommand(new IDXGetlocaCmd(this));

}

static const std::string PROPAGATE_OPT("propagate");
static const std::string TYPE_OPT("type");
static const std::string FULL_OPT("full");
static const std::string FORMAT_OPT("format");
static const std::string COLLAPSE_OPT("collapse");
static const std::string LOCA_OPT("loca");


//
// Helper functions
//
static void indexTrace(Database *db, Index *idx, bool full)
{
  if (!full) {
    printf("%s index on %s\n", idx->asHashIndex() ? "hash" : "btree",
	   idx->getAttrpath().c_str());
    return;
  }

  printf("Index on %s:\n", idx->getAttrpath().c_str());
  printf("  Propagation: %s\n", idx->getPropagate() ? "on" : "off");

  const Dataspace *dataspace = 0;
  idx->makeDataspace(db, dataspace);
  if (dataspace)
    printf("  Dataspace: %s #%d\n", dataspace->getName(), dataspace->getId());

  if (idx->asHashIndex()) {
    HashIndex *hidx = idx->asHashIndex();

    printf("  Type: Hash\n");
    printf("  Key count: %d\n", hidx->getKeyCount());
    if (hidx->getHashMethod())
      printf("  Hash method: %s::%s;",
	     hidx->getHashMethod()->getClassOwner()->getName(),
	     hidx->getHashMethod()->getEx()->getExname().c_str());
    int cnt = idx->getImplHintsCount();
    for (int n = 0; n < cnt; n++)
      if (idx->getImplHints(n))
	printf("  %s: %d\n", IndexImpl::hashHintToStr(n, True),
	       idx->getImplHints(n));
  } else {
    BTreeIndex *bidx = idx->asBTreeIndex();
    printf("  Type: BTree\n");
    printf("  Degree: %d\n", bidx->getDegree());
  }
}

static int
indexGet(Database *db, const Class *cls, LinkedList &indexlist)
{
  const LinkedList *classindexlist;

  const_cast<Class *>(cls)->getAttrCompList(Class::Index_C, classindexlist);
      
  LinkedListCursor c(classindexlist);
  void *o;
  while (c.getNext(o)) {
    indexlist.insertObject(o);
  }

  return 0;
}

static int
indexGet(Database *db, const char *name, LinkedList &indexlist)
{
  Status s;
  const Class *cls;
  const Attribute *attr;

  if (strchr(name, '.')) {
    // name is an attribute name
    Attribute::checkAttrPath(db->getSchema(), cls, attr, name);
    
    Index *index;
    Attribute::getIndex(db, name, index);
    
    if (!index) {
      std::cerr << PROGNAME;
      fprintf(stderr, ": index '%s' not found\n", name);
      return 1;
    }

    indexlist.insertObject(index);

    return 0;
  } else {
    // name is a class name
    cls = db->getSchema()->getClass(name);
    if (!cls) {
      std::cerr << PROGNAME;
      fprintf(stderr, ": class '%s' not found\n", name);
      return 1;
    }
      
    return indexGet(db, cls, indexlist);
  }
  
  return 0;
}

static int
indexGetAll(Database *db, LinkedList &indexlist, bool all)
{
  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (!cls->isSystem() || all)
      if (indexGet(db, cls, indexlist))
	return 1;
  }
  
  return 0;
}

// 
// IDXCreateCmd
//
void IDXCreateCmd::init()
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
			OptionDesc("Index type (supported types are: hash, btree)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXCreateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH [HINTS]\n";
  return 1;
}

int IDXCreateCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("HINTS", "Index hints");
  return 1;
}

int IDXCreateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *attributePath = argv[1].c_str();

  const char *hints = 0;
  if (argv.size() > 2)
    hints = argv[2].c_str();

  const char *typeOption = 0;
  if (map.find(TYPE_OPT) != map.end()) {
    typeOption = map[TYPE_OPT].value.c_str();

    if (strcmp(typeOption, "hash") && strcmp(typeOption, "btree"))
      return help();
  }

  bool propagate = true;
  if (map.find(PROPAGATE_OPT) != map.end()) {
    propagate = !strcmp(map[PROPAGATE_OPT].value.c_str(), "on");
  }

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Class *cls;
  const Attribute *attribute;

  Attribute::checkAttrPath(db->getSchema(), cls, attribute, attributePath);
  
  if (!typeOption) {
    if (attribute->isString() 
	|| attribute->isIndirect() 
	|| attribute->getClass()->asCollectionClass()) // what about enums
      typeOption = "hash";
    else
      typeOption = "btree";
  }

  Index *index;

  printf("Creating %s index on %s\n", typeOption, attributePath);

  if (!strcmp(typeOption, "hash")) {
    HashIndex *hidx;
    HashIndex::make(db, const_cast<Class *>(cls), attributePath,
		    (propagate)? eyedb::True : eyedb::False, 
		    attribute->isString(), hints, hidx);
    index = hidx;
  }
  else if (!strcmp(typeOption, "btree")) {
    BTreeIndex *bidx;
    BTreeIndex::make(db, const_cast<Class *>(cls), attributePath, 
		     (propagate)? eyedb::True : eyedb::False, 
		     attribute->isString(), hints, bidx);
    index = bidx;
  }

  index->store();
  
  db->transactionCommit();

  return 0;
}

//
// IDXDeleteCmd
//
void IDXDeleteCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int IDXDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH\n";
  return 1;
}

int IDXDeleteCmd::help()
{
  getopt->adjustMaxLen("ATTRPATH");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  return 1;
}

int IDXDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *attributePath = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Class *cls;
  const Attribute *attribute;

  Attribute::checkAttrPath(db->getSchema(), cls, attribute, attributePath);
  
  Index *index;

  Attribute::getIndex(db, attributePath, index);
      
  if (!index) {
    std::cerr << PROGNAME;
    fprintf(stderr, ": index '%s' not found\n", attributePath);
    return 1;
  }

  printf("Deleting index %s\n", index->getAttrpath().c_str());
  index->remove();
      
  db->transactionCommit();

  return 0;
}

//
// IDXUpdateCmd
//
void IDXUpdateCmd::init()
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
			OptionDesc("Index type (supported types are: hash, btree)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXUpdateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH [HINTS]\n";
  return 1;
}

int IDXUpdateCmd::help()
{
  getopt->adjustMaxLen("ATTRPATH");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("HINTS", "Index hints");
  return 1;
}

int IDXUpdateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *attributePath = argv[1].c_str();

  const char *hints = "";
  if (argv.size() > 2)
    hints = argv[2].c_str();

  const char *typeOption = 0;
  if (map.find(TYPE_OPT) != map.end()) {
    typeOption = map[TYPE_OPT].value.c_str();

    if (strcmp(typeOption, "hash") && strcmp(typeOption, "btree"))
      return help();
  }

  const char *propagateOption = 0;
  if (map.find(PROPAGATE_OPT) != map.end()) {
    propagateOption = map[TYPE_OPT].value.c_str();
  }

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  LinkedList indexList;
  if (indexGet(db, attributePath, indexList))
    return 1;

  Index *index = (Index *)indexList.getObject(0);

  IndexImpl::Type type;
  if (typeOption && !strcmp(typeOption, "hash"))
    type = IndexImpl::Hash;
  else if (typeOption && !strcmp(typeOption, "btree"))
    type = IndexImpl::BTree;
  else
    type = index->asBTreeIndex() ? IndexImpl::BTree : IndexImpl::Hash;

  bool onlyPropagate = false;
  if (propagateOption 
      && !strcmp(hints,"") 
      && ((type == IndexImpl::Hash && index->asHashIndex()) ||
	  (type == IndexImpl::BTree && index->asBTreeIndex())))
    onlyPropagate = true;

  bool propagate;
  if (propagateOption)
    propagate = !strcmp(propagateOption, "on");
  else
    propagate = index->getPropagate();

  printf("Updating %s index on %s\n", (type == IndexImpl::Hash) ? "hash" : "btree", attributePath);

  index->setPropagate((propagate)? eyedb::True : eyedb::False);

  if (onlyPropagate) {
    index->store();
  } else {
    IndexImpl *impl = 0;

    IndexImpl::make(db, type, hints, impl, index->getIsString());
    
    // The index type has changed
    if ((type == IndexImpl::Hash && index->asBTreeIndex()) ||
	(type == IndexImpl::BTree && index->asHashIndex())) {
      Index *newIndex;
      index->reimplement(*impl, newIndex);
    }
    else {
      index->setImplementation(impl);
      
      index->store();
    }
  }    

  db->transactionCommit();

  return 0;
}

//
// IDXListCmd
//
void IDXListCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FULL_OPT, OptionBoolType(), 0,
			OptionDesc("Displays complete information")));

  opts.push_back(Option(ALL_OPT, OptionBoolType(), 0,
			OptionDesc("Displays all indexes")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME [ATTRPATH|CLASSNAME]\n";
  return 1;
}

int IDXListCmd::help()
{
  getopt->adjustMaxLen("CLASSNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int IDXListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbName = argv[0].c_str();

  bool full = map.find(FULL_OPT) != map.end();
  bool all = map.find(ALL_OPT) != map.end();

  if (all && argv.size() > 1)
    return usage();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBSRead);
  
  db->transactionBegin();
  
  LinkedList indexList;

  if (argv.size() < 2) {
    if (indexGetAll(db, indexList, all))
      return 1;
  } else {
    for (int i = 1; i < argv.size(); i++) {
      if (indexGet(db, argv[i].c_str(), indexList))
	return 1;
    }
  }

  LinkedListCursor c(indexList);
  Index *index;
  while (c.getNext((void *&)index))
    indexTrace(db, index, full);

  return 0;
}

//
// IDXStatsCmd
//
void IDXStatsCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FULL_OPT, OptionBoolType(), 0,
			OptionDesc("Display all index entry statistics")));

  opts.push_back(Option(FORMAT_OPT, 
			OptionStringType(),
			Option::MandatoryValue,
			OptionDesc("Statistics format", "FORMAT")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXStatsCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME [ATTRPATH|CLASSNAME]\n";
  return 1;
}

static void displayFormatOptionHelp()
{
  std::cout << 
    "\n  The --format option indicates an output format for hash index stat entries.\n\
    <format> is a printf-like string where:\n\
      %n denotes the number of keys,\n\
      %O denotes the count of object entries for this key,\n\
      %o denotes the count of hash objects for this key,\n\
      %s denotes the size of hash objects for this key,\n\
      %b denotes the busy size of hash objects for this key,\n\
      %f denotes the free size of hash objects for this key.\n\
\n\
  Examples:\n\
      --format=\"%n %O\\n\"\n\
      --format=\"%n -> %O, %o, %s\\n\"\n";
}

int IDXStatsCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("CLASSNAME", "Class name");

  displayFormatOptionHelp();

  return 1;
}

int IDXStatsCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbName = argv[0].c_str();

  bool full = map.find(FULL_OPT) != map.end();

  const char *format = 0;
  if (map.find(FORMAT_OPT) != map.end())
    format = map[FORMAT_OPT].value.c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBSRead);
  
  db->transactionBegin();
  
  LinkedList indexList;

  if (argv.size() < 2) {
    if (indexGetAll(db, indexList, false))
      return 1;
  } else {
    for (int i = 1; i < argv.size(); i++) {
      if (indexGet(db, argv[i].c_str(), indexList))
	return 1;
    }
  }

  LinkedListCursor c(indexList);
  Index *index;
  while (c.getNext((void *&)index)) {
    if (format && index->asHashIndex()) {
      IndexStats *stats;
      index->getStats(stats);
      fprintf(stdout, "\n");
      stats->asHashIndexStats()->printEntries(format);
      delete stats;
    } else {
      std::string stats;
      index->getStats(stats, False, (full)? eyedb::True : eyedb::False, "    ");
      indexTrace(db, index, True);
      fprintf(stdout, "  Statistics:\n");
      fprintf(stdout, stats.c_str());
    }
  }

  return 0;
}

//
// IDXSimulateCmd
//
void IDXSimulateCmd::init()
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
			OptionDesc("Index type (supported types are: hash, btree)", "TYPE")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXSimulateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME ATTRPATH [HINTS]\n";
  return 1;
}

int IDXSimulateCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("ATTRPATH", "Attribute path");
  getopt->displayOpt("HINTS", "Index hints");
  return 1;
}

int IDXSimulateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *attributePath = argv[1].c_str();

  const char *hints = "";
  if (argv.size() > 2)
    hints = argv[2].c_str();

  const char *typeOption = 0;
  if (map.find(TYPE_OPT) != map.end()) {
    typeOption = map[TYPE_OPT].value.c_str();

    if (strcmp(typeOption, "hash") && strcmp(typeOption, "btree"))
      return help();
  }

  bool full = map.find(FULL_OPT) != map.end();

  const char *format = 0;
  if (map.find(FORMAT_OPT) != map.end())
    format = map[FORMAT_OPT].value.c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  LinkedList indexList;

  if (indexGet(db, attributePath, indexList))
    return 1;

  IndexImpl::Type type;
  if (!strcmp(typeOption, "hash"))
    type = IndexImpl::Hash;
  else if (!strcmp(typeOption, "btree"))
    type = IndexImpl::BTree;

  Index *index = (Index *)indexList.getObject(0);
  IndexImpl *impl;

  IndexImpl::make(db, type, hints, impl, index->getIsString());
  
  if (format && impl->getType() == IndexImpl::Hash) {
    IndexStats *stats;
    index->simulate(*impl, stats);
    stats->asHashIndexStats()->printEntries(format);
    delete stats;
  }
  else {
    std::string stats;
    index->simulate(*impl, stats, True, (full)? eyedb::True : eyedb::False, "  ");
    printf("Index on %s:\n", index->getAttrpath().c_str());
    fprintf(stdout, stats.c_str());
  }

  return 0;
}

//
// IDXMoveCmd
//
//  # index move: ajouter une option --collapse + une info dans le protocole
//  eyedbadmin index move <dbname> <index name> <dest dataspace>
void IDXMoveCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  // not yet implemented
  //opts.push_back(Option(COLLAPSE_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME INDEXNAME DESTDATASPACE\n";
  return 1;
}

int IDXMoveCmd::help()
{
  getopt->adjustMaxLen("DESTDATASPACE");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("INDEXNAME", "Name of index to move");
  getopt->displayOpt("DESTDATASPACE", "Destination dataspace");
  return 1;
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

#define CHECK(S) \
  if (S) { \
   print_prog(); \
   (S)->print(); \
   return 1; \
   }

static int
get_idxs(Database *db, const char *attrpath, ObjectArray &obj_arr)
{
  int offset;
  const char *op = get_op(attrpath, offset);
  OQL q(db, "select index.attrpath %s \"%s\"", op, &attrpath[offset]);
  q.execute(obj_arr);

  if (!obj_arr.getCount()) {
    std::cerr << PROGNAME << ": ";
    fprintf(stderr, "no index %s found\n", attrpath);
    return 1;
  }

  return 0;
}

int IDXMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 3)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *indexName = argv[1].c_str();
  const char *dataspaceName = argv[2].c_str();

  bool collapse = map.find(COLLAPSE_OPT) != map.end();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  db->getDataspace(dataspaceName, dataspace);

  ObjectArray obj_arr;
  if (get_idxs(db, indexName, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    idx->move(dataspace);
    //     idx->move(dataspace, collapse);
  }

  db->transactionCommit();

  return 0;
}

//
// IDXSetdefdspCmd
//
//  eyedbadmin index setdefdsp <dbname> <idx name> <dataspace>
void IDXSetdefdspCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int IDXSetdefdspCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME INDEXNAME DATASPACE\n";
  return 1;
}

int IDXSetdefdspCmd::help()
{
  getopt->adjustMaxLen("DATASPACE");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("INDEXNAME", "Name of index");
  getopt->displayOpt("DATASPACE", "Dataspace");
  return 1;
}

int IDXSetdefdspCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 3)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *indexName = argv[1].c_str();
  const char *dataspaceName = argv[2].c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  db->getDataspace(dataspaceName, dataspace);

  ObjectArray obj_arr;
  if (get_idxs(db, indexName, obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    idx->setDefaultDataspace(dataspace);
  }

  db->transactionCommit();

  return 0;
}

//
// IDXGetdefdspCmd
//
//  eyedbadmin index getdefdsp <dbname> <idx name>
void IDXGetdefdspCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int IDXGetdefdspCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME INDEXNAME\n";
  return 1;
}

int IDXGetdefdspCmd::help()
{
  getopt->adjustMaxLen("INDEXNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("INDEXNAME", "Name of index");
  return 1;
}

static void
print(Database *db, const Dataspace *dataspace, Bool def = False)
{
  if (!dataspace) {
    db->getDefaultDataspace(dataspace);
    print(db, dataspace, True);
    return;
  }

  printf("  Dataspace #%d", dataspace->getId());
  if (def)
    printf(" (default)");
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

int IDXGetdefdspCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *indexName = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  ObjectArray obj_arr;
  if (get_idxs(db, indexName, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    const Dataspace *dataspace;
    idx->getDefaultDataspace(dataspace);

    if (i) 
      printf("\n");
    printf("Default dataspace for index '%s':\n", idx->getAttrpath().c_str());

    print(db, dataspace);
  }

  db->transactionCommit();

  return 0;
}

//
// IDXGetlocaCmd
//
//  eyedbadmin index --stats|--loca|--all getloca <dbname> <idx name>
void IDXGetlocaCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(STATS_OPT, OptionBoolType(), 0, OptionDesc("Displays statistics information")));
  opts.push_back(Option(LOCA_OPT, OptionBoolType(), 0, OptionDesc("Displays localization information")));
  opts.push_back(Option(ALL_OPT, OptionBoolType(), 0, OptionDesc("Displays localization and statistics information")));

  getopt = new GetOpt(getExtName(), opts);
}

int IDXGetlocaCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME INDEXNAME\n";
  return 1;
}

int IDXGetlocaCmd::help()
{
  getopt->adjustMaxLen("INDEXNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("INDEXNAME", "Index name");
  return 1;
}

//#define GET_ARG(ARGV, I) ((ARGV).size() <= (I) ? usage(), exit(1), std::string() : (ARGV)[I])

#define GETARG(VAR, ARGV, I) \
 if ((ARGV).size() <= (I)) \
   return usage(); \
 VAR = (ARGV)[I].c_str()

inline std::string get_arg(std::vector<std::string> &argv, unsigned int idx)
{
  if (argv.size() <= idx) {
    exit(1);
  }

  return argv[idx];
}

int IDXGetlocaCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  //const char *dbName = argv[0].c_str();
  //const char *dbName;
  GETARG(const char *dbName, argv, 0);
  //const char *dbName	    = get_arg(argv, 0).c_str();
  const char *indexName = argv[1].c_str();

  bool locaOpt = false;
  bool statsOpt = false;
  if (map.find(LOCA_OPT) != map.end())
    locaOpt = true;
  if (map.find(STATS_OPT) != map.end())
    statsOpt = true;
  if (map.find(ALL_OPT) != map.end()) {
    locaOpt = true;
    statsOpt = true;
  }    

  conn.open();

  Database *db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  ObjectArray obj_arr;
  if (get_idxs(db, indexName, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    ObjectLocationArray locarr;
    idx->getObjectLocations(locarr);

    if (locaOpt)
      cout << locarr(db) << endl;

    if (statsOpt) {
      PageStats *pgs = locarr.computePageStats(db);
      cout << *pgs;
      delete pgs;
    }
  }

  db->transactionCommit();

  return 0;
}
