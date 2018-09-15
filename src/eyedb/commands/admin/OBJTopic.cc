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
#include "OBJTopic.h"

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

static int
get_objs(const char *oql, OidArray &oid_arr)
{
  OQL q(db, oql);
  Status s = q.execute(oid_arr);
  return 0;
}


//
// Topic definition
//

OBJTopic::OBJTopic() : Topic("object")
{
  addAlias("obj");

  addCommand(new OBJGetLocaCmd(this));
  addCommand(new OBJMoveCmd(this));
}

static const std::string LOCA_OPT("loca");

//eyedbadmin object getloca [--stats|--loca|--all] DATABASE OBJECT
// 
// OBJGetLocaCmd
//
void OBJGetLocaCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back( Option(ALL_OPT, OptionBoolType()));
  opts.push_back( Option(LOCA_OPT, OptionBoolType()));
  opts.push_back( Option(STATS_OPT, OptionBoolType()));

  getopt = new GetOpt(getExtName(), opts);
}

int OBJGetLocaCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME OBJECT\n";
  return 1;
}

int OBJGetLocaCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("OBJECT", "Object (an OQL query)");
  return 1;
}

static const unsigned int LocaOpt = 1;
static const unsigned int StatsOpt = 2;

int OBJGetLocaCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *obj = argv[1].c_str();

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

  OidArray oid_arr;
  if (get_objs(obj, oid_arr))
    return 1;

  ObjectLocationArray locarr;
  Status s = db->getObjectLocations(oid_arr, locarr);

  if (lopt & LocaOpt)
    cout << locarr(db) << endl;

  if (lopt & StatsOpt) {
    PageStats *pgs = locarr.computePageStats(db);
    cout << *pgs;
    delete pgs;
  }

  db->transactionCommit();

  return 0;
}

//eyedbadmin object move DATABASE OBJECT DESTDATASPACE
// 
// OBJMoveCmd
//
void OBJMoveCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int OBJMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME OBJECT DESTDATASPACE\n";
  return 1;
}

int OBJMoveCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("OBJECT", "Object (an OQL query)");
  getopt->displayOpt("DESTDATASPACE", "Destination dataspace name");
  return 1;
}

int OBJMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbName = argv[0].c_str();
  const char *obj = argv[1].c_str();
  const char *dspname = argv[2].c_str();

  conn.open();

  db = new Database(dbName);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();

  const Dataspace *dataspace;
  Status s = db->getDataspace(dspname, dataspace);

  OidArray oid_arr;
  if (get_objs( obj, oid_arr))
    return 1;

  s = db->moveObjects(oid_arr, dataspace);

  db->transactionCommit();

  return 0;
}
