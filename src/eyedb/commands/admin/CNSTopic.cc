
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
#include "CNSTopic.h"

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
// Helper functions and macros, from eyedbconsadmin.cc
//
#define CHECK(S)
#define print_prog(S)

static int
get_cons(const char *constraint, const char *&cnsname,
	 Class::AttrCompIdx &idx)
{
  if (!strcmp(constraint, "notnull")) {
    idx = Class::NotnullConstraint_C;
    cnsname = "notnull";
    return 0;
  }

  if (!strcmp(constraint, "unique")) {
    idx = Class::UniqueConstraint_C;
    cnsname = "unique";
    return 0;
  }

  return 1;
}

static int
get_cons(const Class *cls, Class::AttrCompIdx idx,
	 LinkedList &conslist)
{
  const LinkedList *clconslist;
  Status s = const_cast<Class *>(cls)->getAttrCompList(idx, clconslist);
  CHECK(s);
    
  LinkedListCursor c(clconslist);
  void *o;
  while (c.getNext(o))
    conslist.insertObject(o);
  return 0;
}

static int
get_cons(const char *info, Class::AttrCompIdx idx, LinkedList &conslist)
{
  Status s;
  if (info) {
    Status s;
    if (strchr(info, '.')) {
      s = Attribute::checkAttrPath(db->getSchema(), cls, attr, info);
      CHECK(s);
      AttributeComponent *comp;

      if (idx == Class::UniqueConstraint_C)
	s = Attribute::getUniqueConstraint(db, info,
					      (UniqueConstraint *&)comp);
      else if (idx == Class::NotnullConstraint_C)
	s = Attribute::getNotNullConstraint(db, info,
					       (NotNullConstraint *&)comp);
      else
	abort();

      CHECK(s);
      if (comp) {
	conslist.insertObject(comp);
	return 0;
      }
      print_prog();
      fprintf(stderr, "%s constraint on %s not found\n", 
	      (idx == Class::UniqueConstraint_C ? "unique" : "notnull"),
	      info);
      return 1;
    }
    
    const Class *cls = db->getSchema()->getClass(info);
    if (!cls) {
      print_prog();
      fprintf(stderr, "class %s not found\n", info);
      return 1;
    }
    
    return get_cons(cls, idx, conslist);
  }

  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (get_cons(cls, idx, conslist))
      return 1;
  }
  
  return 0;
}

static void
cons_print(int &n, LinkedList &conslist)
{
  LinkedListCursor c(conslist);
  AttributeComponent *attr_comp;
  for (; c.getNext((void *&)attr_comp); n++) {
    if (n)
      fprintf(stdout, "\n");
    fprintf(stdout, "%s constraint on %s:\n",
	    (attr_comp->asNotNullConstraint() ? "Notnull" : "Unique"),
	    attr_comp->getAttrpath().c_str());
    fprintf(stdout, "  Propagation: %s\n", attr_comp->getPropagate() ? "on" : "off");
  }
}

//
// Topic definition
//

CNSTopic::CNSTopic() : Topic("constraint")
{
  addAlias("cons");

  addCommand(new CNSCreateCmd(this));
  addCommand(new CNSDeleteCmd(this));
  addCommand(new CNSListCmd(this));
}

static const std::string PROPAGATE_OPT("propagate");

//eyedbadmin constraint create ...
// 
// CNSCreateCmd
//
void CNSCreateCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int CNSCreateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION ...\n";
  return 1;
}

int CNSCreateCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  //...
  return 1;
}

int CNSCreateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *constraint = argv[1].c_str();

  //...

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  //...

  db->transactionCommit();

  return 0;
}


// eyedbadmin constraint delete ...
// 
// CNSDeleteCmd
//
void CNSDeleteCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int CNSDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION ...\n";
  return 1;
}

int CNSDeleteCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  //...
  return 1;
}

int CNSDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *constraint = argv[1].c_str();

  //...

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  //...

  db->transactionCommit();

  return 0;
}

// eyedbadmin constraint list ...
// 
// CNSListCmd
//
void CNSListCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int CNSListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION ...\n";
  return 1;
}

int CNSListCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  //...
  return 1;
}

int CNSListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *constraint = argv[1].c_str();

  //...

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  //...

  db->transactionCommit();

  return 0;
}
