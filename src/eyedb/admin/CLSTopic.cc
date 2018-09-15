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

CLSTopic::CLSTopic() : Topic("class")
{
  addAlias("cls");

  addCommand(new CLSGetInstDefDSPCmd(this));
  addCommand(new CLSSetInstDefDSPCmd(this));
  addCommand(new CLSMoveInstCmd(this));
  addCommand(new CLSGetInstLocaCmd(this));
}

static const std::string LOCA_OPT("loca");
static const std::string SUBCLASSES_OPT("subclasses");

//
// CLSGetInstDefDSPCmd
//
// eyedbloca getdefinstdsp <dbname> <class name>
void CLSGetInstDefDSPCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSGetInstDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME\n";
  return 1;
}

int CLSGetInstDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int CLSGetInstDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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

  conn.open();

  Database *db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
#if 0
  ObjectArray obj_arr;
  const char *str = (argc == 2 ? argv[1] : "~");
  if (get_cls(str, obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = cls->getDefaultInstanceDataspace(dataspace);
    CHECK(s);
    if (i) printf("\n");
    printf("Default dataspace for instances of class '%s':\n", cls->getName());
    print(dataspace);
  }
#endif

  db->transactionCommit();

  return 0;
}

//
// CLSSetInstDefDSPCmd
//
// eyedbloca setdefinstdsp <dbname> <class name> <dest dataspace>
void CLSSetInstDefDSPCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSSetInstDefDSPCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME\n";
  return 1;
}

int CLSSetInstDefDSPCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int CLSSetInstDefDSPCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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

  conn.open();

  Database *db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  // ...

  db->transactionCommit();

  return 0;
}

//
// CLSMoveInstCmd
//
// eyedbloca mvinst <dbname> [--subclasses] <class name> <dest dataspace>
void CLSMoveInstCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSMoveInstCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME\n";
  return 1;
}

int CLSMoveInstCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int CLSMoveInstCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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

  bool locaOpt = false;
  if (map.find(LOCA_OPT) != map.end())
    locaOpt = true;

  conn.open();

  Database *db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  // ...

  db->transactionCommit();

  return 0;
}

//
// CLSGetInstLocaCmd
//
// eyedbloca getinstloca --stats|--loca|--all [--subclasses] <dbname> <class name>
void CLSGetInstLocaCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int CLSGetInstLocaCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME CLASSNAME\n";
  return 1;
}

int CLSGetInstLocaCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("CLASSNAME", "Class name");
  return 1;
}

int CLSGetInstLocaCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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

  bool locaOpt = false;
  if (map.find(LOCA_OPT) != map.end())
    locaOpt = true;

  conn.open();

  Database *db = new Database(dbname);

  db->open( &conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  // ...

  db->transactionCommit();

  return 0;
}

