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
#include "CLSTopic.h"


using namespace eyedb;
using namespace std;

//
// Global variables...
//

static Connection *conn;
static Database *db;

//
// Helper functions and macros
//

static const char *get_op(const char *s, int &offset);

static int
get_cls(const char *name, ObjectArray &obj_arr)
{
  int offset;
  const char *op = get_op(name, offset);
  OQL q(db, "select class.name %s \"%s\"", op, &name[offset]);
  Status s = q.execute(obj_arr);

  if (!obj_arr.getCount()) {
    fprintf(stderr, "no class %s found\n", name);
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

CLSTopic::CLSTopic() : Topic("class")
{
  addAlias("cls");

  addCommand(new CLSGetDefDSPCmd(this));
  addCommand(new CLSSetDefDSPCmd(this));
  addCommand(new CLSMoveCmd(this));
  addCommand(new CLSGetLocaCmd(this));
}

static const std::string LOCA_OPT("loca");
static const std::string SUBCLASSES_OPT("subclasses");

//
// CLSGetDefDSPCmd
//
// eyedbadmin class getdefdsp DATABASE CLASSNAME
void CLSGetDefDSPCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int CLSGetDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME [CLASSNAME]\n";
  return 1;
}

int CLSGetDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int CLSGetDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbname = argv[0].c_str();

  const char *classname = "~";
  if (argv.size() > 1)
    classname = argv[1].c_str();

  conn.open();

  db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  ObjectArray obj_arr;
  if (get_cls(classname, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = cls->getDefaultInstanceDataspace(dataspace);

    if (i)
      printf("\n");

    printf("Default dataspace for instances of class '%s':\n", cls->getName());
    print(dataspace);
  }

  db->transactionCommit();

  return 0;
}

//
// CLSSetDefDSPCmd
//
// eyedbadmin class setdefdsp DATABASE CLASSNAME DATASPACE
void CLSSetDefDSPCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSSetDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME DSPNAME\n";
  return 1;
}

int CLSSetDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int CLSSetDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *classname = argv[1].c_str();
  const char *dspname = argv[2].c_str();

  conn.open();

  db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  Status s = db->getDataspace(dspname, dataspace);

  ObjectArray obj_arr;
  if (get_cls(classname, obj_arr)) 
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    Status s = cls->setDefaultInstanceDataspace(dataspace);
  }

  db->transactionCommit();

  return 0;
}

//
// CLSMoveCmd
//
// eyedbadmin class move [--subclasses] DATABASE CLASSNAME DESTDATASPACE
// eyedbloca mvinst <dbname> [--subclasses] <class name> <dest dataspace>
void CLSMoveCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(SUBCLASSES_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME COLLECTION DESTDATASPACE\n";
  return 1;
}

int CLSMoveCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("COLLECTION", "Collection (can be a collection name, a collection oid or an OQL query)");
  getopt->displayOpt("DESTDATASPACE", "Destination dataspace name");
  return 1;
}

int CLSMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *classname = argv[1].c_str();
  const char *dspname = argv[2].c_str();

  bool subclassesOpt = false;
  if (map.find(SUBCLASSES_OPT) != map.end())
    subclassesOpt = true;

  conn.open();

  db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  Status s = db->getDataspace(dspname, dataspace);

  ObjectArray obj_arr;
  if (get_cls(classname, obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    Status s = cls->moveInstances(dataspace, (subclassesOpt)? eyedb::True : eyedb::False);
  }

  db->transactionCommit();

  return 0;
}

//
// CLSGetLocaCmd
//
// eyedbadmin class getloca [--stats|--loca|--all] [--subclasses] DATABASE CLASSNAME
void CLSGetLocaCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(ALL_OPT, OptionBoolType()));
  opts.push_back( Option(LOCA_OPT, OptionBoolType()));
  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  opts.push_back( Option(SUBCLASSES_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSGetLocaCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME\n";
  return 1;
}

int CLSGetLocaCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

static const unsigned int LocaOpt = 1;
static const unsigned int StatsOpt = 2;

int CLSGetLocaCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *classname = argv[1].c_str();

  unsigned int lopt = 0;
  if (map.find( STATS_OPT) != map.end())
    lopt = StatsOpt;
  else if (map.find( LOCA_OPT) != map.end())
    lopt = LocaOpt;
  else if (map.find( ALL_OPT) != map.end())
    lopt = StatsOpt|LocaOpt;
  else
    return help();

  bool subclassesOpt = false;
  if (map.find(SUBCLASSES_OPT) != map.end())
    subclassesOpt = true;

  conn.open();

  db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  ObjectArray obj_arr;
  if (get_cls(classname, obj_arr))
    return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    ObjectLocationArray locarr;
    Status s = cls->getInstanceLocations(locarr, (subclassesOpt)? eyedb::True : eyedb::False);

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

