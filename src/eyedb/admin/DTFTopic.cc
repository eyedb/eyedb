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
#include "eyedb/DBM_Database.h"
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <eyedblib/butils.h>
#include "GetOpt.h"
#include "DTFTopic.h"
#include "DatafileStats.h"

using namespace eyedb;

DTFTopic::DTFTopic() : Topic("datafile")
{
  addAlias("dtf");

  addCommand(new DTFCreateCmd(this));
  addCommand(new DTFDeleteCmd(this));
  addCommand(new DTFMoveCmd(this));
  addCommand(new DTFRenameCmd(this));
  addCommand(new DTFResizeCmd(this));
  addCommand(new DTFDefragmentCmd(this));
  addCommand(new DTFListCmd(this));
}

//
// DFTCreateCmd
//
void DTFCreateCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FILEDIR_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("File directory", "FILEDIR")));

  opts.push_back(Option(NAME_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Datafile name", "NAME")));

  opts.push_back(Option(SIZE_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Datafile size (in mega-bytes)", "SIZE")));

  opts.push_back(Option(SLOTSIZE_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Slot size (in bytes)", "SLOTSIZE")));

  opts.push_back(Option(PHYSICAL_OPT, 
			OptionBoolType(), 
			0, 
			OptionDesc("Physical datafile type")));

  getopt = new GetOpt(getExtName(), opts);
}

int DTFCreateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATAFILE\n";
  return 1;
}

int DTFCreateCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATAFILE", "Datafile");
  return 1;
}

int DTFCreateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *filename = argv[1].c_str();

  const char *filedir = 0;
  if (map.find(FILEDIR_OPT) != map.end())
    filedir = map[FILEDIR_OPT].value.c_str();

  const char *name = "";
  if (map.find(NAME_OPT) != map.end())
    name = map[NAME_OPT].value.c_str();

  unsigned int size = DEFAULT_DTFSIZE * ONE_K;
  if (map.find(SIZE_OPT) != map.end())
    size = atoi(map[SIZE_OPT].value.c_str());

  unsigned int slotsize = DEFAULT_DTFSZSLOT;
  if (map.find(SLOTSIZE_OPT) != map.end())
    slotsize = atoi(map[SLOTSIZE_OPT].value.c_str());

  eyedbsm::DatType dtfType = eyedbsm::LogicalOidType;
  if (map.find(PHYSICAL_OPT) != map.end())
    dtfType = eyedbsm::PhysicalOidType;

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  db->createDatafile(filedir, filename, name, size, slotsize, dtfType);

  db->transactionCommit();

  return 0;
}

//
// DTFDeleteCmd
//
void DTFDeleteCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DTFDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATID|DATNAME\n";
  return 1;
}

int DTFDeleteCmd::help()
{
  getopt->adjustMaxLen("DATNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DTFDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *datname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  datafile->remove();

  db->transactionCommit();

  return 0;
}

//
// DTFMoveCmd
//
void DTFMoveCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FILEDIR_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("File directory", "FILEDIR")));

  getopt = new GetOpt(getExtName(), opts);
}

int DTFMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATID|DATNAME NEWDATAFILE\n";
  return 1;
}

int DTFMoveCmd::help()
{
  getopt->adjustMaxLen("NEWDATAFILE");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  getopt->displayOpt("NEWDATAFILE", "New datafile");
  return 1;
}

int DTFMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *datname = argv[1].c_str();
  const char *new_filename = argv[2].c_str();

  const char *filedir = 0;
  if (map.find(FILEDIR_OPT) != map.end())
    filedir = map[FILEDIR_OPT].value.c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  datafile->move(filedir, new_filename);

  db->transactionCommit();

  return 0;
}

//
// DTFRenameCmd
//
void DTFRenameCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int DTFRenameCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATID|DATNAME NEWNAME\n";
  return 1;
}

int DTFRenameCmd::help()
{
  getopt->adjustMaxLen("NEWNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  getopt->displayOpt("NEWNAME", "New name");
  return 1;
}

int DTFRenameCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *datname = argv[1].c_str();
  const char *new_name = argv[2].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  datafile->rename(new_name);

  db->transactionCommit();

  return 0;
}

//
// DTFResizeCmd
//
void DTFResizeCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DTFResizeCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATID|DATNAME NEWSIZE\n";
  return 1;
}

int DTFResizeCmd::help()
{
  getopt->adjustMaxLen("NEWSIZE");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  getopt->displayOpt("NEWSIZE", "New size (in Mb)");
  return 1;
}

int DTFResizeCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 3)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *datname = argv[1].c_str();
  const char *new_size = argv[2].c_str();

  if (!eyedblib::is_number(new_size))
    return usage();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  datafile->resize(atoi(new_size)*1024);

  db->transactionCommit();

  return 0;
}

//
// DTFDefragmentCmd
//
void DTFDefragmentCmd::init()
{
  std::vector<Option> opts;
  opts.push_back(HELP_OPT);
  getopt = new GetOpt(getExtName(), opts);
}

int DTFDefragmentCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME DATID|DATNAME\n";
  return 1;
}

int DTFDefragmentCmd::help()
{
  getopt->adjustMaxLen("DATNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DTFDefragmentCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2) {
    return usage();
  }

  const char *dbname = argv[0].c_str();
  const char *datname = argv[1].c_str();

  conn.open();

  Database *db = new Database(dbname);

  db->open(&conn, Database::DBRW);

  db->transactionBeginExclusive();

  const Datafile *datafile;
  db->getDatafile(datname, datafile);
  
  datafile->defragment();

  db->transactionCommit();

  return 0;
}

//
// DTFListCmd
//
void DTFListCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(ALL_OPT,
			OptionBoolType(),
			0,
			OptionDesc("List all")));

  opts.push_back(Option(STATS_OPT, 
			OptionBoolType(),
			0,
			OptionDesc("List only statistics on datafiles")));

  getopt = new GetOpt(getExtName(), opts);
}

int DTFListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME [DATID|DATNAME]\n";
  return 1;
}

int DTFListCmd::help()
{
  getopt->adjustMaxLen("DATNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("DATID", "Datafile id");
  getopt->displayOpt("DATNAME", "Datafile name");
  return 1;
}

int DTFListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() < 1)
    return usage();

  std::string dbname = argv[0];

  // By default, we show files and not statistics
  bool showFiles = true;
  bool showStats = false;

  // If --stats option, we show statistics and not files
  if (map.find(STATS_OPT) != map.end()) {
    showFiles = false;
    showStats = true;
  }

  // If --all option, we show both files and statistics
  // This test is after the test on --stats so that if you pass both 
  // --all and --stats options you get files and statistics
  if (map.find(ALL_OPT) != map.end()) {
    showFiles = true;
    showStats = true;
  }

  conn.open();

  Database *db = new Database(dbname.c_str());

  db->open(&conn, Database::DBSRead);

  db->transactionBegin();

  const Datafile **datafiles;
  unsigned int count;

  if (argv.size() == 1) {
    db->getDatafiles(datafiles, count);
  } else {
    count = argv.size() - 1;
    datafiles = new const Datafile*[count];
    for (int i = 0; i < count; i++) {
      db->getDatafile(argv[i+1].c_str(), datafiles[i]);
    }
  }

  if (showFiles) {
    for (int i = 0; i < count; i++)
      if (datafiles[i]->isValid()) {
	DatafileInfo info;
	datafiles[i]->getInfo(info);
	std::cout << info << std::endl;
      }
  }

  if (showStats) {
    DatafileStats stats;
    for (int i = 0, n = 0; i < count; i++)
      if (stats.add(datafiles[i])) 
	return 1;
    stats.display();
  }

  return 0;
}
