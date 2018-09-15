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
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <eyedblib/butils.h>
#include "GetOpt.h"
#include "DBSTopic.h"
#include "DatafileStats.h"
#include "DatabaseExportImport.h"

using namespace eyedb;

DBSTopic::DBSTopic() : Topic("database")
{
  addAlias("db");

  addCommand(new DBSCreateCmd(this));
  addCommand(new DBSDeleteCmd(this));
  addCommand(new DBSListCmd(this));

  addCommand(new DBSMoveCmd(this));
  addCommand(new DBSCopyCmd(this));
  addCommand(new DBSRenameCmd(this));

  addCommand(new DBSDefAccessCmd(this));

  addCommand(new DBSExportCmd(this));
  addCommand(new DBSImportCmd(this));
}


//
// DBSCreateCmd
//
void DBSCreateCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);
  opts.push_back(Option(DBFILE_OPT, OptionStringType(), /*Option::Mandatory|*/Option::MandatoryValue, OptionDesc("Database file", "DBFILE")));
  opts.push_back(Option(FILEDIR_OPT, OptionStringType(), Option::MandatoryValue, OptionDesc("Database file directory", "FILEDIR")));
  opts.push_back(Option(MAXOBJCNT_OPT, OptionIntType(), Option::MandatoryValue, OptionDesc("Maximum database object count", "OBJECT_COUNT")));

  getopt = new GetOpt(getExtName(), opts);
}

int DBSCreateCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME\n";
  return 1;
}

int DBSCreateCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to create");
  return 1;
}

int DBSCreateCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  bool r = getopt->parse(PROGNAME, argv);

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (!r) {
    return usage();
  }

  if (argv.size() != 1) { // dbname is missing
    return usage();
  }

  conn.open();

  std::string dbname = argv[0];

  const char *filedir = 0;
  if (map.find(FILEDIR_OPT) != map.end()) {
    filedir = map[FILEDIR_OPT].value.c_str();
  }

  if (!filedir) {
    filedir = eyedb::ServerConfig::getSValue("databasedir");
  }

  std::string dirname = filedir;

  std::string dbfile;
  if (map.find(DBFILE_OPT) != map.end()) {
    dbfile = map[DBFILE_OPT].value;
  }

  if (dbfile.length() == 0) {
    dbfile = dirname + "/" + dbname + DBS_EXT;
  }

  Database *db = new Database(argv[0].c_str());

  DbCreateDescription dbdesc;
  strcpy(dbdesc.dbfile, dbfile.c_str());

  eyedbsm::DbCreateDescription *d = &dbdesc.sedbdesc;
  d->dbid     = 0;

  if (map.find(MAXOBJCNT_OPT) != map.end()) {
    d->nbobjs = atoi(map[MAXOBJCNT_OPT].value.c_str());
  }
  else
    d->nbobjs = DEFAULT_MAXOBJCNT;

  d->ndat = 1;
  eyedbsm::Datafile *dat = &d->dat[0];

  std::string datfile = dbname + DTF_EXT;
  strcpy(dat->file, datfile.c_str());
  strcpy(dat->name, DEFAULT_DTFNAME);
  d->dat[0].maxsize = DEFAULT_DTFSIZE * ONE_K;

  dat->mtype = eyedbsm::BitmapType;
  dat->sizeslot = DEFAULT_DTFSZSLOT;
  dat->dtype = eyedbsm::LogicalOidType;

  dat->dspid = 0;

  db->create(&conn, &dbdesc);

  return 0;
}

//
// DBSListCmd
//
void DBSListCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);
  opts.push_back(Option(DBNAME_OPT, OptionBoolType(), 0, OptionDesc("Lists database names")));
  opts.push_back(Option(DBID_OPT, OptionBoolType(), 0, OptionDesc("Lists database identifier")));
  opts.push_back(Option(MAXOBJCNT_OPT, OptionBoolType(), 0, OptionDesc("Lists database max object count")));
  opts.push_back(Option(DATAFILES_OPT, OptionBoolType(), 0, OptionDesc("Lists database datafiles")));
  opts.push_back(Option(DEFACCESS_OPT, OptionBoolType(), 0, OptionDesc("Lists database default access")));
  opts.push_back(Option(USERACCESS_OPT, OptionBoolType(), 0, OptionDesc("Lists user database accesses")));
  opts.push_back(Option(STATS_OPT, OptionBoolType(), 0, OptionDesc("Lists database statistics")));
  opts.push_back(Option(ALL_OPT, OptionBoolType(), 0, OptionDesc("Lists all database info")));

  getopt = new GetOpt(getExtName(), opts);
}

int DBSListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " [~]DBNAMES...\n";
  return 1;
}

int DBSListCmd::help()
{
  stdhelp();
  getopt->displayOpt("[~]DBNAMES...", "Database(s) to list (~ means regular expression)");
  return 1;
}

///----- should be factorized in another module
enum {
  DBNAME_FLG = 0x1,
  DBID_FLG = 0x2,
  DBFILE_FLG = 0x4,
  MAXOBJCNT_FLG = 0x8,
  DATAFILES_FLG = 0x10,
  DEFACCESS_FLG = 0x20,
  USERACCESS_FLG = 0x40,
  STATS_FLG = 0x80,
  ALL_FLG = 0x100
};

static const char LIST_INDENT[] = "  ";

static DbInfoDescription *getDBInfo(Connection &conn, DBEntry *dbentry, Database *&db)
{
  db = new Database(dbentry->dbname().c_str());

  DbInfoDescription *dbdesc = new DbInfoDescription();
  db->getInfo(&conn, 0, 0, dbdesc);

  return dbdesc;
}

static void
printDatafiles(Connection &conn, DBEntry *dbentry, Bool datafiles,
	       DbInfoDescription *dbdesc)
{
  int dbid;
  static char indent[] = "            ";
  if (!dbdesc) {
    Database *db;
    dbdesc = getDBInfo(conn, dbentry, db);
    if (!dbdesc) return;
  }

  if (!datafiles)
    printf("Datafiles\n");

  const eyedbsm::DbCreateDescription *s = &dbdesc->sedbdesc;

  for (int i = 0; i < s->ndat; i++) {
    if (datafiles) {
      printf("%s\n", s->dat[i].file);
      continue;
    }

    printf("%sDatafile #%d\n", LIST_INDENT, i);
    if (*s->dat[i].name)
      printf("%s  Name      %s\n", LIST_INDENT, s->dat[i].name);
    if (s->dat[i].dspid >= 0)
      printf("%s  Dataspace #%d\n", LIST_INDENT, s->dat[i].dspid);
    printf("%s  File      %s\n", LIST_INDENT, s->dat[i].file);
    printf("%s  Maxsize   ~%dMb\n", LIST_INDENT, s->dat[i].maxsize/1024);
    printf("%s  Slotsize  %db\n", LIST_INDENT, s->dat[i].sizeslot);
    printf("%s  Server Access ", LIST_INDENT);
    if (s->dat[i].extflags == R_OK)
      printf("read", indent);
    else if (s->dat[i].extflags == R_OK|W_OK)
      printf("read/write", indent);
    else if (s->dat[i].extflags == W_OK)
      printf("write", indent);
    else
      printf("DENIED", indent);
    printf("\n");
  }
}

static int getDBAccess(DBM_Database *dbm, const char *name, const char *fieldname, DBUserAccess **dbaccess)
{
  Status status = Success;
  int cnt;
  
  cnt = 0;

  dbm->transactionBegin();

  OQL q(dbm, "select database_user_access->%s = \"%s\"", fieldname, name);

  ObjectArray obj_arr;
  q.execute(obj_arr);

  for (int i = 0; i < obj_arr.getCount(); i++)
    dbaccess[cnt++] = (DBUserAccess *)obj_arr[i];
  
  dbm->transactionCommit();
  return cnt;
}

static int getDBAccessUser(DBM_Database *dbm, const char *username,
			   DBUserAccess **dbaccess)
{
  return getDBAccess(dbm, username, "user->name", dbaccess);
}

static int getDBAccessDB(DBM_Database *dbm, const char *dbname,
			 DBUserAccess **dbaccess)
{
  return getDBAccess(dbm, dbname, "dbentry->dbname", dbaccess);
}


#define OPTCONCAT(M, SM, XM, STR, C) \
 if (((M) & (XM)) == (M) ) { \
   strcat(STR, C); \
   strcat(STR, SM); \
   C = " | "; \
 }

static const char *sysModeStr(SysAccessMode sysmode)
{
  static char sysstr[512];

  if (sysmode == NoSysAccessMode)
    return "NO_SYSACCESS_MODE";

  if (sysmode == AdminSysAccessMode)
    return "ADMIN_SYSACCESS_MODE";

  if (sysmode == SuperUserSysAccessMode)
    return "SUPERUSER_SYSACCESS_MODE";

  const char *concat;
  *sysstr = 0;

  concat = "";

  OPTCONCAT(DBCreateSysAccessMode, "DB_CREATE_SYSACCESS_MODE",
	    sysmode, sysstr, concat);
  OPTCONCAT(AddUserSysAccessMode, "ADD_USER_SYSACCESS_MODE",
	    sysmode, sysstr, concat);
  OPTCONCAT(DeleteUserSysAccessMode, "DELETE_USER_SYSACCESS_MODE",
	    sysmode, sysstr, concat);
  OPTCONCAT(SetUserPasswdSysAccessMode, "SET_USER_PASSWD_SYSACCESS_MODE",
	    sysmode, sysstr, concat);
  
  return sysstr;
}

static const char *userModeStr(DBAccessMode usermode)
{
  static char userstr[512];

  if (usermode == NoDBAccessMode)
    return "NO_DBACCESS_MODE";

  if (usermode == AdminDBAccessMode)
    return "ADMIN_DBACCESS_MODE";

  const char *concat;
  *userstr = 0;

  concat = "";

  OPTCONCAT(ReadWriteExecDBAccessMode, "READ_WRITE_EXEC_DBACCESS_MODE",
	    usermode, userstr, concat)
    else OPTCONCAT(ReadExecDBAccessMode, "READ_EXEC_DBACCESS_MODE",
		   usermode, userstr, concat)
	   else OPTCONCAT(ReadWriteDBAccessMode, "READ_WRITE_DBACCESS_MODE",
			  usermode, userstr, concat)
		  else OPTCONCAT(ReadDBAccessMode, "READ_DBACCESS_MODE",
				 usermode, userstr, concat);

  return userstr;
}

static int printDBEntry(Connection &conn, DBEntry *dbentry, DBM_Database *dbm, unsigned int options)
{
  DbInfoDescription *dbdesc = 0;
  Database *db = 0;

  if ((options & ALL_FLG) || (options & DATAFILES_FLG) || (options & MAXOBJCNT_FLG) ||
      (options & STATS_FLG)) {
    dbdesc = getDBInfo(conn, dbentry, db);
    if (!dbdesc) return 1;
  }
  else
    dbdesc = 0;

  if (options & ALL_FLG) {
    printf("Database Name\n%s%s\n", LIST_INDENT, dbentry->dbname().c_str());
    printf("Database Identifier\n%s%d\n", LIST_INDENT, dbentry->dbid());
    printf("Database File\n%s%s\n", LIST_INDENT, dbentry->dbfile().c_str());
    printf("Max Object Count\n%s%d\n", LIST_INDENT, dbdesc->sedbdesc.nbobjs);
  }
  else {
    if (options & DBNAME_FLG)
      printf("%s\n", dbentry->dbname().c_str());

    if (options & DBID_FLG)
      printf("%d\n", dbentry->dbid());

    if (options & DBFILE_FLG)
      printf("%s\n", dbentry->dbfile().c_str());
    
    if (options & MAXOBJCNT_FLG)
      printf("%llu\n", dbdesc->sedbdesc.nbobjs);
  }

  if ((options & ALL_FLG) || (options & DATAFILES_FLG))
    printDatafiles(conn, dbentry, IDBBOOL(options & DATAFILES_FLG), dbdesc);

  if (options & DEFACCESS_FLG)
    printf("%s\n", userModeStr(dbentry->default_access()));
  else if (options & ALL_FLG)
    printf("Default Access\n%s%s\n", LIST_INDENT, userModeStr(dbentry->default_access()));

  DBUserAccess *dbaccess[128];
  int cnt = getDBAccessDB(dbm, dbentry->dbname().c_str(), dbaccess);
  if (cnt > 0) {
    if (options & ALL_FLG)
      printf("Database Access\n");
    for (int i = 0; i < cnt; i++)
      if (options & USERACCESS_FLG)
	printf("%s %s\n",
	       dbaccess[i]->user()->name().c_str(),
	       userModeStr(dbaccess[i]->mode()));
      else if (options & ALL_FLG)
	printf("%sUser Name  %s\n%sAccess Mode %s\n",
	       LIST_INDENT,
	       dbaccess[i]->user()->name().c_str(),
	       LIST_INDENT,
	       userModeStr(dbaccess[i]->mode()));
  }
  
  if (options & STATS_FLG) {
    const Datafile **datafiles;
    unsigned int cnt;
    db->open(&conn, Database::DBSRead);
    db->getDatafiles(datafiles, cnt);
    DatafileStats dstats(dbdesc);
    for (int i = 0, n = 0; i < cnt; i++)
      if (dstats.add(datafiles[i])) return 1;
    dstats.display();
  }

  //printf("\n");

  return 0;
}

const char *getOP(const char *s, unsigned int &offset)
{
  if (s[0] == '~') {
    if (s[1] == '~') {
      offset = 2;
      return "~~";
    }

    offset = 1;
    return "~";
  }

  if (s[0] == '!') {
    if (s[1] == '~') {
      if (s[2] == '~') {
	offset = 3;
	return "!~~";
      }

      offset = 2;
      return "!~";
    }

    if (s[1] == '=') {
      offset = 2;
      return "!=";
    }
  }

  if (s[0] == '=') {
    if (s[1] == '=') {
      offset = 2;
      return "==";
    }

    offset = 1;
    return "=";
  }

  offset = 0;
  return "=";
}

/// ---- end of factorization

static unsigned getListOptions(const GetOpt::Map &map)
{
  unsigned int options = 0;

  if (map.find(DBNAME_OPT) != map.end()) {
    options |= DBNAME_FLG;
  }

  if (map.find(DBID_OPT) != map.end()) {
    options |= DBID_FLG;
  }

  if (map.find(DBFILE_OPT) != map.end()) {
    options |= DBFILE_FLG;
  }

  if (map.find(MAXOBJCNT_OPT) != map.end()) {
    options |= MAXOBJCNT_FLG;
  }

  if (map.find(DATAFILES_OPT) != map.end()) {
    options |= DATAFILES_FLG;
  }

  if (map.find(DEFACCESS_OPT) != map.end()) {
    options |= DEFACCESS_FLG;
  }

  if (map.find(USERACCESS_OPT) != map.end()) {
    options |= USERACCESS_FLG;
  }

  if (map.find(STATS_OPT) != map.end()) {
    options |= STATS_FLG;
  }

  if (map.find(ALL_OPT) != map.end()) {
    options |= ALL_FLG;
  }

  if (!options) {
    options = ALL_FLG;
  }

  return options;
}

int DBSListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  bool r = getopt->parse(PROGNAME, argv);

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (!r) {
    return usage();
  }

  if (argv.size() == 0) {
    argv.push_back("~.*");
  }

  unsigned int options = getListOptions(map);;

  conn.open();

  DBM_Database *dbm = new DBM_Database(Database::getDefaultDBMDB());
  dbm->open(&conn, Database::DBSRead);
  dbm->transactionBegin();

  unsigned int size = argv.size();
  for (unsigned int j = 0; j < size; j++) {
    DBEntry **dbentries;
    int cnt;
    unsigned int offset;
    const char *op = getOP(argv[j].c_str(), offset);

    dbm->getDBEntries(argv[0].c_str()+offset, dbentries, cnt, op);

    if (!cnt) {
      std::cerr << "Database '" << argv[j].c_str() + offset << "' not found\n";
    }

    for (int i = 0; i < cnt; i++) {
      if (j && (options & ALL_FLG))
	printf("\n");
      if (printDBEntry(conn, dbentries[i], dbm, options)) {
	delete [] dbentries;
	return 1;
      }
      dbentries[i]->release();
    }
  
    delete [] dbentries;
  }

  return 0;
}

//
// DBSRenameCmd
//
void DBSRenameCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int DBSRenameCmd::usage()
{
  getopt->usage("", "");

  std::cerr << " DBNAME NEWDBNAME\n";

  return 1;
}

int DBSRenameCmd::help()
{
  getopt->adjustMaxLen("NEWDBNAME");
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to rename");
  getopt->displayOpt("NEWDBNAME", "New database name");

  return 1;
}

int DBSRenameCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  bool r = getopt->parse(PROGNAME, argv);

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (!r) {
    return usage();
  }

  if (argv.size() != 2) {
    return usage();
  }

  conn.open();

  std::string dbname = argv[0];
  std::string new_dbname = argv[1];

  if (dbname == new_dbname)
    return 0;

  Database *db = new Database(dbname.c_str());
  //  db->rename(&conn, new_dbname.c_str(), (const char *)0, (const char *)0);
  db->rename(&conn, new_dbname.c_str());

  return 0;
}

//
// DBSDeleteCmd
//
void DBSDeleteCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int DBSDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAMES...\n";
  return 1;
}

int DBSDeleteCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAMES...", "Database(s) to delete");
  return 1;
}

int DBSDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  bool r = getopt->parse(PROGNAME, argv);

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (!r) {
    return usage();
  }

  if (argv.size() < 1) { // dbname is missing
    return usage();
  }

  conn.open();

  bool error = false;

  for (unsigned int n = 0; n < argv.size(); n++) {
    try {
      eyedb::Database db(&conn, argv[n].c_str());
      const char *dbfile;
      db.getDatabasefile(dbfile);
    }
    catch(Exception &e) {
      std::cerr << e << std::endl;
      error = true;
    }
  }

  if (error)
    return 1;

  for (unsigned int n = 0; n < argv.size(); n++) {
    eyedb::Database db(argv[n].c_str());
    db.remove(&conn);
  }

  return 0;
}

//
// DBSMoveCmd
//
void DBSMoveCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(DBFILE_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Database file", "DBFILE")));

  opts.push_back(Option(FILEDIR_OPT, 
			OptionStringType(), 
			Option::Mandatory|Option::MandatoryValue, 
			OptionDesc("Database file directory", "FILEDIR")));

  getopt = new GetOpt(getExtName(), opts);
}

int DBSMoveCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME\n";
  return 1;
}

int DBSMoveCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to move");
  return 1;
}

int DBSMoveCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 1)
    return usage();

  const char *dbname = argv[0].c_str();

#if 0
  std::string filedir;
  if (map.find(FILEDIR_OPT) != map.end())
    filedir = map[FILEDIR_OPT].value;
  else
    filedir = std::string(eyedb::ServerConfig::getSValue("databasedir"));
#else
  // filedir is a mandatory option
  std::string filedir = map[FILEDIR_OPT].value;
#endif

  std::string dbfile;
  if (map.find(DBFILE_OPT) != map.end())
    dbfile = map[DBFILE_OPT].value;

  if (dbfile.length() == 0)
    dbfile = filedir + "/" + dbname + DBS_EXT;

  conn.open();

  Database *db = new Database(dbname);

  DbCreateDescription oldDesc;
  db->getInfo(&conn, 0, 0, &oldDesc);

  DbCreateDescription newDesc;
  strcpy(newDesc.dbfile, dbfile.c_str());

  newDesc.sedbdesc.ndat = oldDesc.sedbdesc.ndat;

  // FIXME: what if old file name does not contain '/', as --filedir is given? 
  for (int i = 0; i < oldDesc.sedbdesc.ndat; i++) {
    const char *datafile = oldDesc.sedbdesc.dat[i].file;
    char *p = (char *)strrchr(datafile, '/');
    if (p)
      strcpy(newDesc.sedbdesc.dat[i].file, strdup((filedir + (p+1)).c_str()));
    else
      strcpy(newDesc.sedbdesc.dat[i].file, strdup(datafile));

    newDesc.sedbdesc.dat[i].maxsize = 0;
    newDesc.sedbdesc.dat[i].sizeslot = 0;
  }

  db->move(&conn, &newDesc);

  return 0;
}

//
// DBSCopyCmd
//
void DBSCopyCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(DBFILE_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Database file", "DBFILE")));

  opts.push_back(Option(FILEDIR_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Database file directory", "FILEDIR")));

  getopt = new GetOpt(getExtName(), opts);
}

int DBSCopyCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME NEWDBNAME\n";
  return 1;
}

int DBSCopyCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to copy");
  getopt->displayOpt("NEWDBNAME", "New database name");
  return 1;
}

int DBSCopyCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *newDbname = argv[1].c_str();

  // No need to check if dbname == newDbname, this is done in Database::copy

  std::string filedir;
  if (map.find(FILEDIR_OPT) != map.end())
    filedir = map[FILEDIR_OPT].value;
  else
    filedir = std::string(eyedb::ServerConfig::getSValue("databasedir"));

  std::string dbfile;
  if (map.find(DBFILE_OPT) != map.end()) {
    dbfile = map[DBFILE_OPT].value;
    if (dbfile[0] != '/')
      dbfile = filedir + "/" + dbfile;
  } else
    dbfile = filedir + "/" + newDbname + DBS_EXT;

  conn.open();

  Database *db = new Database(dbname);

  DbCreateDescription oldDesc;
  db->getInfo(&conn, 0, 0, &oldDesc);

  if (oldDesc.sedbdesc.ndat > 1 && map.find(FILEDIR_OPT) == map.end()) {
    std::cerr << PROGNAME << ": error: when copying a database with more than one datafile, option --filedir is mandatory\n";
    return 1;
  }

  DbCreateDescription newDesc;

  strcpy(newDesc.dbfile, dbfile.c_str());

  newDesc.sedbdesc.ndat = oldDesc.sedbdesc.ndat;

  if (oldDesc.sedbdesc.ndat == 1) {
    std::string newDatafile = filedir + "/" + newDbname + ".dat";
    strcpy(newDesc.sedbdesc.dat[0].file, newDatafile.c_str());
    newDesc.sedbdesc.dat[0].maxsize = 0;
    newDesc.sedbdesc.dat[0].sizeslot = 0;
  } else {
    for (int i = 0; i < oldDesc.sedbdesc.ndat; i++) {
      const char *datafile = oldDesc.sedbdesc.dat[i].file;
      char *p = (char *)strrchr(datafile, '/');
      if (p)
	strcpy(newDesc.sedbdesc.dat[i].file, strdup((filedir + (p+1)).c_str()));
      else
	strcpy(newDesc.sedbdesc.dat[i].file, strdup(datafile));

      newDesc.sedbdesc.dat[i].maxsize = 0;
      newDesc.sedbdesc.dat[i].sizeslot = 0;
    }
  }

  db->copy(&conn, newDbname, False, &newDesc);

  return 0;
}

//
// DBSDefAccessCmd
//
void DBSDefAccessCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int DBSDefAccessCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME MODE\n";
  return 1;
}

int DBSDefAccessCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to copy");
  getopt->displayOpt("MODE", "Access mode, is one of r, rw, rx, rwx, admin, no");
  return 1;
}

static int getDbAccessMode(const char *accessMode, DBAccessMode &dbMode)
{
  if (!strcmp(accessMode, "r"))
    dbMode = ReadDBAccessMode;
  else if (!strcmp(accessMode, "rw"))
    dbMode = ReadWriteDBAccessMode;
  else if (!strcmp(accessMode, "rx"))
    dbMode = ReadExecDBAccessMode;
  else if (!strcmp(accessMode, "rwx"))
    dbMode = ReadWriteExecDBAccessMode;
  else if (!strcmp(accessMode, "admin"))
    dbMode = AdminDBAccessMode;
  else if (!strcmp(accessMode, "no"))
    dbMode = NoDBAccessMode;
  else
    return 1;

  return 0;
}

int DBSDefAccessCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (! getopt->parse(PROGNAME, argv))
    return usage();

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end())
    return help();

  if (argv.size() != 2)
    return usage();

  const char *dbname = argv[0].c_str();
  const char *accessMode= argv[1].c_str();

  DBAccessMode dbMode;
  if (getDbAccessMode(accessMode, dbMode))
    return help();

  conn.open();

  Database *db = new Database(dbname);

  db->setDefaultDBAccess(&conn, dbMode);

  return 0;
}

//
// DBSExportCmd
//
void DBSExportCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int DBSExportCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME FILE\n";
  return 1;
}

int DBSExportCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to export");
  getopt->displayOpt("FILE", "File for export (- for standard output)");
  return 1;
}

int DBSExportCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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
  const char *filename = argv[1].c_str();

  conn.open();

  return databaseExport(conn, dbname, filename);
}

//
// DBSImportCmd
//
void DBSImportCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  opts.push_back(Option(FILEDIR_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Database file directory", "FILEDIR")));

  opts.push_back(Option(MTHDIR_OPT, 
			OptionStringType(), 
			Option::MandatoryValue, 
			OptionDesc("Method directory", "MTHDIR")));

  opts.push_back(Option(LIST_OPT, 
			OptionBoolType(), 
			0, 
			OptionDesc("List only, do not import")));

  getopt = new GetOpt(getExtName(), opts);
}

int DBSImportCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " DBNAME FILE\n";
  return 1;
}

int DBSImportCmd::help()
{
  stdhelp();
  getopt->displayOpt("DBNAME", "Database to import");
  getopt->displayOpt("FILE", "File for import (- for standard output)");
  return 1;
}

int DBSImportCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
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
  const char *filename = argv[1].c_str();

  const char *filedir = 0;
  if (map.find(FILEDIR_OPT) != map.end())
    filedir = map[FILEDIR_OPT].value.c_str();

  const char *mthdir = 0;
  if (map.find(MTHDIR_OPT) != map.end())
    mthdir = map[MTHDIR_OPT].value.c_str();

  bool listOnly = (map.find(LIST_OPT) != map.end());

  conn.open();

  return databaseImport(conn, dbname, filename, filedir, mthdir, listOnly);
}
