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
#include <eyedb/opts.h>
#include "eyedb/DBM_Database.h"
#include "GetOpt.h"
#include "DSPTopic.h"

using namespace eyedb;
using namespace std;

DSPTopic::DSPTopic() : Topic("dataspace")
{
  addAlias("dsp");

  addCommand(new DSPCreateCmd(this));
  addCommand(new DSPUpdateCmd(this));
  addCommand(new DSPDeleteCmd(this));
  addCommand(new DSPRenameCmd(this));
  addCommand(new DSPListCmd(this));
  addCommand(new DSPSetDefCmd(this));
  addCommand(new DSPGetDefCmd(this));
  addCommand(new DSPSetCurDatCmd(this));
  addCommand(new DSPGetCurDatCmd(this));
}

// DSPCreateCmd 
void DSPCreateCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPCreateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME DATID|DATNAME\n";
  return 1;
}

int DSPCreateCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DSPCreateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  int count = argv.size() - 2;
  const Datafile **datafiles = new const Datafile *[count];
  for (int i = 0; i < count; i++) {
    db->getDatafile(argv[i+2].c_str(), datafiles[i]);
  }

  db->createDataspace(dspname, datafiles, count);
  
  db->transactionCommit();

  return 0;
}

// DSPUpdateCmd 
void DSPUpdateCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPUpdateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME DATID|DATNAME\n";
  return 1;
}

int DSPUpdateCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DSPUpdateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace = 0;
  db->getDataspace(dspname, dataspace);
  
  int count = argv.size() - 2;
  const Datafile **datafiles = new const Datafile *[count];
  for (int i = 0; i < count; i++) {
    db->getDatafile(argv[i+2].c_str(), datafiles[i]);
  }

  dataspace->update(datafiles, count);
  
  db->transactionCommit();

  return 0;
}

// DSPDeleteCmd 
void DSPDeleteCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME\n";
  return 1;
}

int DSPDeleteCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int DSPDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace = 0;
  db->getDataspace(dspname, dataspace);
  
  dataspace->remove();
  
  db->transactionCommit();

  return 0;
}

// DSPRenameCmd 
void DSPRenameCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPRenameCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME NEWDSPNAME\n";
  return 1;
}

int DSPRenameCmd::help()
{
  getopt->adjustMaxLen("NEWDSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  getopt->displayOpt("NEWDSPNAME", "New dataspace name");
  return 1;
}

int DSPRenameCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();
  const char *newdspname = argv[2].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace = 0;
  db->getDataspace(dspname, dataspace);
  
  dataspace->rename(newdspname);
  
  db->transactionCommit();

  return 0;
}

// DSPListCmd 
void DSPListCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME [DSPNAME]\n";
  return 1;
}

int DSPListCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int DSPListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbname = argv[0].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBSRead);
  
  if (argv.size() > 1) {
    for (int i = 1; i < argv.size(); i++) {
      const Dataspace *dataspace;
      db->getDataspace(argv[i].c_str(), dataspace);
      cout << *dataspace;
    }
  }
  else {
    unsigned int count;
    const Dataspace **dataspaces;
    db->getDataspaces(dataspaces, count);
    for (unsigned int i = 0; i < count; i++) {
      if (dataspaces[i]->isValid()) {
	cout << *dataspaces[i];
      }
    }
  }

  return 0;
}

// DSPSetDefCmd 
void DSPSetDefCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPSetDefCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME\n";
  return 1;
}

int DSPSetDefCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int DSPSetDefCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace = 0;
  db->getDataspace(dspname, dataspace);
  
  db->setDefaultDataspace(dataspace);
  
  db->transactionCommit();

  return 0;
}

// DSPGetDefCmd 
void DSPGetDefCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPGetDefCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME\n";
  return 1;
}

int DSPGetDefCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  return 1;
}

int DSPGetDefCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  const char *dbname = argv[0].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  db->getDefaultDataspace(dataspace);
  cout << *dataspace;

  db->transactionCommit();

  return 0;
}

// DSPSetCurDatCmd 
void DSPSetCurDatCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPSetCurDatCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME DATID|DATNAME\n";
  return 1;
}

int DSPSetCurDatCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DSPSetCurDatCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();
  const char *datname = argv[2].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  db->getDataspace(dspname, dataspace);
  
  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  const_cast<Dataspace *>(dataspace)->setCurrentDatafile(datafile);
  
  db->transactionCommit();

  return 0;
}

// DSPGetCurDatCmd 
void DSPGetCurDatCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DSPGetCurDatCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DSPNAME\n";
  return 1;
}

int DSPGetCurDatCmd::help()
{
  getopt->adjustMaxLen("DSPNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DSPNAME", "Dataspace name");
  return 1;
}

int DSPGetCurDatCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *dspname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);
  
  db->transactionBeginExclusive();
  
  const Dataspace *dataspace;
  db->getDataspace(dspname, dataspace);
  
  const Datafile *datafile;
  dataspace->getCurrentDatafile(datafile);
  
  cout << *datafile;

  db->transactionCommit();

  return 0;
}
