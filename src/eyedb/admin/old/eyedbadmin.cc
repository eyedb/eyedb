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

#include <eyedb/eyedb.h>
#include "eyedb/DBM_Database.h"

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <eyedblib/butils.h>

#include "GetOpt.h"

using namespace eyedb;
using namespace std;

#define KEEP_MAX_INFO

static Connection *conn;

enum mMode {
  mDbmCreate = 1,
  mDbCreate,
  mDbDelete,
  mDbRename,
  mDbMove,
  mDbCopy,
  mDbAccessSet,
  mDB_UP,

  /* to implement */
  mDatCreate,
  mDatDelete,
  mDatMove,
  mDatDefragment,
  mDatResize,
  mDatList,
  mDatRename,
  mDAT_UP,

  /* to implement */
  mDspCreate,
  mDspUpdate,
  mDspDelete,
  mDspRename,
  mDspList,
  mDspSetDefault,
  mDspGetDefault,
  mDspSetCurDat,
  mDspGetCurDat,
  mDSP_UP,

  mDbExport,
  mDbImport,
  mEXT_UP,

  mUserAdd,
  mUserDelete,
  mUserPasswdSet,
  mPasswdSet,
  mUserDbAccessSet,
  mUserSysAccessSet,
  mADMIN_UP,

  mUserList,
  mDbList
};

namespace eyedb {
  extern void display_datsize(ostream &, unsigned long long);
}

static mMode mode;
static const char *prog;
static const char *dbmdb;
static char userauth[32];
static char passwdauth[10];
static int complete, eyedb_prefix;
static DBM_Database *dbmdatabase;

#define CHECK(S) \
  if (S) { \
    print_prog(); \
    (S)->print(); \
    return 1; \
  }

static const char datext[] = ".dat";

static const char *
str()
{
  if (!mode)
    return "\neyedbadmin ";

  return "";
}

#define LSEEK(FD, OFF, WH) \
   do {if (lseek(FD, OFF, WH) < 0) {perror("lseek"); return 1;}} while(0)

#define WRITE(FD, PTR, SZ) \
   do {if (write(FD, PTR, SZ) != (SZ)) {perror("write"); return 1;}} while(0)

#define READ(FD, PTR, SZ) \
   do {if (read(FD, PTR, SZ) != (SZ)) {perror("read"); return 1;}} while(0)

#define READ_XDR_16(FD, PTR, SZ) \
   do {assert(SZ == 2); \
       eyedblib::int16 i16; \
       READ(FD, &i16, SZ); \
       x2h_16_cpy(PTR, &i16);} while(0)

#define READ_XDR_16_ARR(FD, PTR2, NB) \
   do { for (int j = 0; j < NB; j++) { \
          READ_XDR_16(FD, (((short *)PTR2)+j), sizeof(*PTR2)); \
        } \
   } while(0)

/*
#define READ_XDR_16_ARR(FD, PTR2, NB) \
    READ(FD, PTR2, NB*sizeof(short))
*/

#define READ_XDR_32(FD, PTR, SZ) \
   do {assert(SZ == 4); \
       eyedblib::int32 i32; \
       READ(FD, &i32, SZ); \
       x2h_32_cpy(PTR, &i32);} while(0)

#define READ_XDR_64(FD, PTR, SZ) \
   do {assert(SZ == 8); \
       eyedblib::int64 i64; \
       READ(FD, &i64, SZ); \
       x2h_64_cpy(PTR, &i64);} while(0)

/*
#define READ_XDR_64(FD, PTR, SZ) \
   READ(FD, PTR, SZ)
*/

#define WRITE_XDR_16(FD, PTR, SZ) \
   do {assert(SZ == 2); \
       eyedblib::int16 i16; \
       h2x_16_cpy(&i16, PTR); \
       WRITE(FD, &i16, SZ); } while(0)

#define WRITE_XDR_16_ARR(FD, PTR2, NB) \
   do { for (int j = 0; j < NB; j++) { \
          WRITE_XDR_16(FD, (((short *)PTR2)+j), sizeof(*PTR2)); \
       } \
   } while(0)

/*
#define WRITE_XDR_16_ARR(FD, PTR2, NB) \
    WRITE(FD, PTR2, NB*sizeof(short))
*/

#define WRITE_XDR_32(FD, PTR, SZ) \
   do {assert(SZ == 4); \
       eyedblib::int32 i32; \
       h2x_32_cpy(&i32, PTR); \
       WRITE(FD, &i32, SZ); } while(0)

#define WRITE_XDR_64(FD, PTR, SZ) \
   do {assert(SZ == 8); \
       eyedblib::int64 i64; \
       h2x_64_cpy(&i64, PTR); \
       WRITE(FD, &i64, SZ); } while(0)

/*
#define WRITE_XDR_64(FD, PTR, SZ) \
  WRITE(FD, PTR, SZ)
*/

static void
print_prog()
{
  fprintf(stderr, "%s: ", prog);
}

#define DATSZ_IN_M

#ifdef DATSZ_IN_M
static const char datafiles_opts[] = "{<datafile>[:name[:<sizeMb>[:sizeslot[:phy|log]]]]}";
#else
static const char datafiles_opts[] = "{<datafile>[:name[:<kbsize>[:sizeslot[:phy|log]]]]}";
#endif

static int
usage(const char *prog)
{
  static Bool displayed = False;
  if (displayed) return 1;
  displayed = True;

  FILE *pipe = NULL;
  
  if (!pipe)
    pipe = stderr;

  if (!mode)
    fprintf(pipe, "eyedbadmin usage:\n");
  else if (complete)
    fprintf(pipe, "usage: eyedbadmin ");
  else if (eyedb_prefix)
    fprintf(pipe, "usage: eyedb");
  else
    fprintf(pipe, "usage: ");

  if (!mode || mode == mDbmCreate)
    fprintf(pipe,
	    "%sdbmcreate [--strict-unix=<user>|@] [<user> [<passwd>]] [--datafiles=%s]\n", str(),
	    datafiles_opts);

  if (!mode || mode == mDbCreate)
    fprintf(pipe,
	    "%sdbcreate <dbname> [--dbfile=<dbfile>] [--datafiles=%s] [--filedir=<filedir>] [--object-count=<objcnt>]\n", str(), datafiles_opts);

  if (!mode || mode == mDbDelete)
    fprintf(pipe,
	    "%sdbdelete <dbname>\n", str());

  if (!mode || mode == mDbRename)
    fprintf(pipe,
	    "%sdbrename <dbname> <new dbname>\n", str());

  if (!mode || mode == mDbMove)
    fprintf(pipe,
	    "%sdbmove <dbname> [--dbfile=<dbfile>] [--datafiles=%s] [--filedir=<filedir>]\n", str(), datafiles_opts);

  if (!mode || mode == mDbCopy)
    fprintf(pipe,
	    "%sdbcopy <dbname> <new dbname> [--dbfile=<dbfile>] [--datafiles=%s] [--filedir=<filedir>] [--new-dbid]\n", str(), datafiles_opts);


  if (!mode || mode == mDbExport)
    fprintf(pipe,
	    "%sdbexport <dbname> <file>|-\n", str());


  if (!mode || mode == mDbImport)
    fprintf(pipe,
	    "%sdbimport [-l] <file>|- [-d <dbname> [--filedir=<filedir>] [--mthdir==<mthdir>]]\n", str());


  if (!mode || mode == mDatCreate)
    fprintf(pipe,
	    "%sdatcreate <dbname> [--filedir=<filedir>] %s\n", str(),
	    datafiles_opts);

  if (!mode || mode == mDatDelete)
    fprintf(pipe,
	    "%sdatdelete <dbname> <datid>|<datname>\n", str());

  if (!mode || mode == mDatMove)
    fprintf(pipe,
	    "%sdatmove <dbname> <datid>|<datname> <new datafile>|--filedir=<filedir>\n", str());

  if (!mode || mode == mDatResize)
    fprintf(pipe,
	    "%sdatresize <dbname> <datid>|<datname> <new sizeMb>\n", str());

  if (!mode || mode == mDatDefragment)
    fprintf(pipe,
	    "%sdatdefragment <dbname> <datid>|<datname>\n", str());

  if (!mode || mode == mDatList)
    fprintf(pipe,
	    "%sdatlist <dbname> [--stats|--all] [{<datid>|<datname>}]\n", str());

  if (!mode || mode == mDatRename)
    fprintf(pipe,
	    "%sdatrename <dbname> <datid>|<datname> <new name>\n", str());

  if (!mode || mode == mDspCreate)
    fprintf(pipe,
	    "%sdspcreate <dbname> <dspname> {<datid>|<datname>}\n", str());

  if (!mode || mode == mDspUpdate)
    fprintf(pipe,
	    "%sdspupdate <dbname> <dspname> {<datid>|<datname>}\n", str());

  if (!mode || mode == mDspDelete)
    fprintf(pipe,
	    "%sdspdelete <dbname> <dspname>\n", str());

  if (!mode || mode == mDspRename)
    fprintf(pipe,
	    "%sdsprename <dbname> <dspname> <new dspname>\n", str());

  if (!mode || mode == mDspList)
    fprintf(pipe,
	    "%sdsplist <dbname> [{<dspname>}]\n", str());

  if (!mode || mode == mDspSetDefault)
    fprintf(pipe,
	    "%sdspsetdefault <dbname> <dspname>\n", str());

  if (!mode || mode == mDspGetDefault)
    fprintf(pipe,
	    "%sdspgetdefault <dbname>\n", str());

  if (!mode || mode == mDspSetCurDat)
    fprintf(pipe,
	    "%sdspsetcurdat <dbname> <dspname> <datid>|<datname> \n", str());

  if (!mode || mode == mDspGetCurDat)
    fprintf(pipe,
	    "%sdspgetcurdat <dbname> <dspname> \n", str());

  if (!mode || mode == mUserAdd)
    fprintf(pipe,
	    "%suseradd [--unix|--strict-unix=]<user>|@ [<passwd>]\n", str());

  if (!mode || mode == mUserDelete)
    fprintf(pipe,
	    "%suserdelete <user>\n", str());

  if (!mode || mode == mUserPasswdSet)
    fprintf(pipe,
	    "%suserpasswd <user> [<new passwd>]\n", str());

  if (!mode || mode == mPasswdSet)
    fprintf(pipe,
	    "%spasswd <user> [<passwd>] [<new passwd>] (primary authentication not required)\n", str());

  if (!mode || mode == mDbAccessSet)
    fprintf(pipe,
	    "%sdbaccess <dbname> r|rw|rx|rwx|admin|no\n", str());

  if (!mode || mode == mUserDbAccessSet)
    fprintf(pipe,
	    "%suserdbaccess <user> <dbname> r|rw|rx|rwx|admin|no\n", str());

  if (!mode || mode == mUserSysAccessSet)
    fprintf(pipe,
	    "%ssysaccess <user> ['+' combination of] dbcreate|adduser|deleteuser|setuserpasswd|admin|superuser|no\n", str());

  if (!mode || mode == mUserList)
    fprintf(pipe,
	    "%suserlist [<users>]\n", str());

  if (!mode || mode == mDbList)
    fprintf(pipe,
	    "%sdblist [--dbname|--dbid|--dbfile|--object-count|--datafiles|--dbaccess|--userdbaccess|--all] [<databases>] [--stats]\n", str());

  /*
    fprintf(pipe, "\nOne can add the following standard EyeDB options:\n\n%s\n",
    eyedb::getStdOptionsUsage());
  */

  fflush(pipe);

  print_use_help(cerr);

  /*
    if (!mode)
    fprintf(pipe,
    "\n--------------------------------------------------------------------------------\n");
  */
  if (pipe != stderr)
    pclose(pipe);
  return 1;
}

#define DEF_SIZESLOT 16
#define DEF_SIZESLOT_STR "16"
#define DEF_DATTYPE_STR "log"

// DEF_NBOBJS is equal to 10 M objects
const int DEF_NBOBJS = 10000000;

#ifdef DATSZ_IN_M
#define DEF_DATSIZE_STR "2000"
#else
#define DEF_DATSIZE_STR "2000000"
#endif

#define MIN_DATAFILE_1 5000
#define MIN_DATAFILE   1000

static int
check_datafile_size(int ind, const eyedbsm::Datafile *datf)
{
  if (!ind && datf->maxsize < MIN_DATAFILE_1)
    {
      print_prog();
      fprintf(stderr, "datafile %s: the KB size of the first datafile must be "
	      "greater than %d [%dKb]\n", datf->file, MIN_DATAFILE_1,
	      datf->maxsize);
      return 1;
    }
      
  if (datf->maxsize < MIN_DATAFILE)
    {
      print_prog();
      fprintf(stderr, "datafile %s: the KB size of a datafile must be greater than %d [%dKb]\n",
	      datf->file, MIN_DATAFILE, datf->maxsize);
      return 1;
    }

  return 0;
}

static const char *
get_dir(const char *s)
{
  char *p = strdup(s);
  char *q = strrchr(p, '/');
  if (!q)
    {
      free(p);
      return "";
    }

  *q = 0;
  static std::string str;
  str = p;
  return str.c_str();
}

static std::string
getDatafile(const char *dbname, const char *dbfile)
{
  if (strcmp(dbname, DBM_Database::getDbName()))
    return std::string(dbname) + datext;

  const char *start = strrchr(dbfile, '/');
  const char *end   = strrchr(dbfile, '.'); 
 
  if (!start) 
    start = dbfile;
  else 
    start++; 
  
  if (!end) 
    end = &dbfile[strlen(dbfile)-1]; 
  else 
    end--; 
 
  int len = end-start+1;
  char *s = new char[len+strlen(datext)+1];
  strncpy(s, &dbfile[start - dbfile], len);
  s[len] = 0; 
  strcat(s, datext);
  std::string datafile = s;
  delete [] s;
  return datafile;
}

#define NO_CLIENT_DATDIR

static int
dbcreate_prologue(Database *db, const char *dbname,
		  char *&dbfile, char *datafiles[], int &datafiles_cnt,
		  const char *deffiledir, DbCreateDescription *dbdesc,
		  eyedbsm::Oid::NX nbobjs = DEF_NBOBJS, int dbid = 0)
{
#ifdef NO_CLIENT_DATDIR
  std::string dirname = (deffiledir ? deffiledir : "");
  if (dirname.length())
    dirname += "/";

  if (!dbfile) {
    if (strcmp(dbname, DBM_Database::getDbName()))
      dbfile = strdup((dirname + dbname + ".dbs").c_str());
    else {
      dbfile = strdup(Database::getDefaultServerDBMDB());
    }
  }
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + dbfile).c_str());

#else
  std::string dirname = (deffiledir ? deffiledir :
			 eyedb::ClientConfig::getCValue("databasedir"));
  if (!dbfile) {
    if (strcmp(dbname, DBM_Database::getDbName()))
      dbfile = strdup((dirname + "/" + dbname + ".dbs").c_str());
    else {
      dbfile = strdup(Database::getDefaultDBMDB());
    }
  }
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + "/" + dbfile).c_str());

#endif

  strcpy(dbdesc->dbfile, dbfile);
  eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
  d->dbid     = dbid;
  d->nbobjs   = nbobjs;

  if (!datafiles_cnt) {
    datafiles_cnt = 5;
    datafiles[0] = strdup(getDatafile(dbname, dbfile).c_str());
    datafiles[1] = "";
    datafiles[2] = DEF_DATSIZE_STR;
    datafiles[3] = (char *)DEF_SIZESLOT_STR;
    datafiles[4] = (char *)DEF_DATTYPE_STR;
  }

  assert(!(datafiles_cnt%5));
  d->ndat = datafiles_cnt/5;
  for (int i = 0, j = 0; i < datafiles_cnt; i += 5, j++) {
    strcpy(d->dat[j].file, datafiles[i]);
    strcpy(d->dat[j].name, datafiles[i+1]);
#ifdef DATSZ_IN_M
    d->dat[j].maxsize = atoi(datafiles[i+2]) * 1024;
#else
    d->dat[j].maxsize = atoi(datafiles[i+2]);
#endif
    if (*d->dat[j].file && check_datafile_size(j, &d->dat[j]))
      return 1;
    
    d->dat[j].mtype = eyedbsm::BitmapType;
    d->dat[j].sizeslot = atoi(datafiles[i+3]);

    // added 20/10/05
    d->dat[j].dspid = 0;
    // changed 2/12/05
    d->dat[j].dspid = j;

    if (!strcasecmp(datafiles[i+4], "phy"))
      d->dat[j].dtype = eyedbsm::PhysicalOidType;
    else if (!strcasecmp(datafiles[i+4], "log"))
      d->dat[j].dtype = eyedbsm::LogicalOidType;
    else
      return 1;
  }

  return 0;
}

static int
move_error(const char *opt)
{
  print_prog();
  fprintf(stderr, "dbmove: must use one of the two options: --filedir or %s\n",
	  opt);
  return 1;
}

static int
dbmove_prologue(Database *db, const char *dbname,
		char *&dbfile, char *datafiles[], int &datafiles_cnt,
		const char *deffiledir, DbCreateDescription *dbdesc)
{
#ifdef NO_CLIENT_DATDIR
  std::string dirname = (deffiledir ? deffiledir : "");
  if (dirname.length())
    dirname += "/";
  if (!dbfile) {
    if (!deffiledir)
      return move_error("--dbfile");
    dbfile = strdup((dirname + dbname + ".dbs").c_str());
  }
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + dbfile).c_str());
#else
  std::string dirname = (deffiledir ? deffiledir :
			 eyedb::ClientConfig::getCValue("databasedir"));
  if (!dbfile)
    {
      if (!deffiledir)
	return move_error("--dbfile");
      dbfile = strdup((dirname + "/" + dbname + ".dbs").c_str());
    }
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + "/" + dbfile).c_str());
#endif

  strcpy(dbdesc->dbfile, dbfile);

  if (!datafiles_cnt)  {
    if (!deffiledir)
      return move_error("--datafiles");

    DbCreateDescription dbdesc;
    Status status = db->getInfo(conn, 0, 0, &dbdesc);
      CHECK(status);
      
      for (int i = 0; i < dbdesc.sedbdesc.ndat; i++) {
	const char *datafile = dbdesc.sedbdesc.dat[i].file;
	char *p = (char *)strrchr(datafile, '/');
	if (p) {
	  datafiles[i] = strdup((dirname + (p+1)).c_str());
	  continue;
	}

	/*
	const char *dbfile_dir = get_dir(dbfile);
	if (strcmp(dbfile_dir, dirname.c_str())) {
	  datafiles[i] = strdup((dirname + datafile).c_str());
	  continue;
	}
	*/
	
	datafiles[i] = strdup(datafile);
      }

      datafiles_cnt = dbdesc.sedbdesc.ndat;
    }

  eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
  d->ndat = datafiles_cnt;
  for (int i = 0; i < datafiles_cnt; i++)
    {
      strcpy(d->dat[i].file, datafiles[i]);
      d->dat[i].maxsize = 0;
      d->dat[i].sizeslot = 0;
    }

  return 0;
}

static std::string
make_copy_name(Bool oneDatafile, const char *newdbname, const char *s)
{
  return std::string(oneDatafile ? newdbname : s) + datext;
}

static int
dbcopy_prologue(Database *db, const char *dbname, const char *newdbname,
		char *&dbfile, char *datafiles[], int &datafiles_cnt,
		const char *deffiledir, DbCreateDescription *dbdesc)
{
#ifdef NO_CLIENT_DATDIR
  std::string dirname = (deffiledir ? deffiledir : "");
  if (dirname.length())
    dirname += "/";

  if (!dbfile)
    dbfile = strdup((dirname + newdbname + ".dbs").c_str());
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + dbfile).c_str());
#else
  std::string dirname = (deffiledir ? deffiledir :
			 eyedb::ClientConfig::getCValue("databasedir"));
  if (!dbfile)
    dbfile = strdup((dirname + "/" + newdbname + ".dbs").c_str());
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + "/" + dbfile).c_str());
#endif


  strcpy(dbdesc->dbfile, dbfile);

  if (!datafiles_cnt) {
    DbCreateDescription dbdesc;
    Status status = db->getInfo(conn, 0, 0, &dbdesc);
    CHECK(status);

    Bool oneDatafile = IDBBOOL(dbdesc.sedbdesc.ndat == 1);
    if (!deffiledir && !oneDatafile) {
      print_prog();
      fprintf(stderr, "when copying a database with more than "
	      "one datafile, one must use one of the two options: "
	      "--datafiles or --filedir\n");
      return 1;
    }

    for (int i = 0; i < dbdesc.sedbdesc.ndat; i++) {
      char *datafile = dbdesc.sedbdesc.dat[i].file;
      char *q = strrchr(datafile, '.');
      *q = 0;
      char *p = strrchr(datafile, '/');
      if (p) {
	datafiles[i] = strdup((dirname +
			       make_copy_name(oneDatafile, newdbname, p+1)).c_str());
	continue;
      }

      /*
	const char *dbfile_dir = get_dir(dbfile);
	if (strcmp(dbfile_dir, dirname.c_str())) {
	datafiles[i] = strdup((dirname + "/" +
	make_copy_name(oneDatafile, newdbname,
	datafile)).c_str());
	continue;
	}
      */

      datafiles[i] = strdup(make_copy_name(oneDatafile, newdbname, datafile).c_str());

    }

    datafiles_cnt = dbdesc.sedbdesc.ndat;
  }

  eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
  d->ndat = datafiles_cnt;
  for (int i = 0; i < datafiles_cnt; i++) {
    strcpy(d->dat[i].file, datafiles[i]);
    d->dat[i].maxsize = 0;
    d->dat[i].sizeslot = 0;
  }

  return 0;
}

static int
dbmcreate_realize(char *username, char *passwd, char *dbfile, char *datafiles[], int datafiles_cnt)
{
  if (dbfile)
    return usage(prog);

  DbCreateDescription dbdesc;

  DBM_Database *dbm = new DBM_Database(dbmdb);

  if (dbcreate_prologue(dbm, DBM_Database::getDbName(), dbfile,
			datafiles, datafiles_cnt, (const char *)0, &dbdesc))
    return 1;

  Status status = dbm->create(conn, passwdauth, username, passwd, &dbdesc);

  CHECK(status);
  return 0;
}

static int
dbcreate_realize(char *dbname,  char *dbfile,
		 char *datafiles[], int datafiles_cnt, const char *filedir,
		 eyedbsm::Oid::NX nbobjs, int dbid = 0)
{
  DbCreateDescription dbdesc;

  Database *db = new Database(dbname, dbmdb);

  if (dbcreate_prologue(db, dbname, dbfile, datafiles, datafiles_cnt,
			filedir, &dbdesc, nbobjs, dbid))
    return 1;

  Status status = db->create(conn, userauth, passwdauth, &dbdesc);

  CHECK(status);

  return 0;
}

static int
dbdelete_realize(char *dbname)
{
  Database *db = new Database(dbname, dbmdb);

  Status status = db->remove(conn, userauth, passwdauth);
  CHECK(status);
  return 0;
}

static int
get_dbaccess(const char *accessmode, DBAccessMode &dbmode)
{
  if (!strcmp(accessmode, "r"))
    dbmode = ReadDBAccessMode;
  else if (!strcmp(accessmode, "rw"))
    dbmode = ReadWriteDBAccessMode;
  else if (!strcmp(accessmode, "rx"))
    dbmode = ReadExecDBAccessMode;
  else if (!strcmp(accessmode, "rwx"))
    dbmode = ReadWriteExecDBAccessMode;
  else if (!strcmp(accessmode, "admin"))
    dbmode = AdminDBAccessMode;
  else if (!strcmp(accessmode, "no"))
    dbmode = NoDBAccessMode;
  else
    return 1;

  return 0;
}

static int
default_dbaccess_realize(char *dbname, char *accessmode)
{
  DBAccessMode dbmode;

  if (get_dbaccess(accessmode, dbmode))
    return usage(prog);

  Database *db;
  Status status;

  db = new Database(dbname, dbmdb);

  status = db->setDefaultDBAccess(conn, dbmode, userauth, passwdauth);
  CHECK(status);
  return 0;
}

static int
dbrename_realize(char *dbname, char *newdbname)
{
  if (!newdbname)
    {
      print_prog();
      fprintf(stderr, "must specify new database name\n");
      return 1;
    }

  if (!strcmp(dbname, newdbname))
    return 0;

  Database *db = new Database(dbname, dbmdb);
  Status status = db->rename(conn, newdbname, userauth, passwdauth);
  CHECK(status);
  return 0;
}

static int
dbmove_realize(char *dbname,  char *dbfile,
	       char *datafiles[], int datafiles_cnt, const char *filedir)
{
  DbCreateDescription dbdesc;
  Database *db = new Database(dbname, dbmdb);

  if (dbmove_prologue(db, dbname, dbfile, datafiles, datafiles_cnt,
		      filedir, &dbdesc))
    return 1;

  Status status = db->move(conn, &dbdesc, userauth, passwdauth);
  CHECK(status);
  return 0;
}

static void
auth_realize();

static void
exc_handler(Status s, void*)
{
  print_prog();
  s->print();
  exit(1);
}

static const char *
get_ompfile(const char *dbfile)
{
  static char ompfile[256];
  strcpy(ompfile, dbfile);
  char *p = strrchr(ompfile, '.');
  if (p)
    *p = 0;

  strcat(ompfile, ".omp");
  return ompfile;
}

static int
code_string(int fd, const char *s)
{
  int len = strlen(s);
  WRITE_XDR_32(fd, &len, sizeof(len));
  WRITE(fd, s, len+1);
  return 0;
}

static int
decode_string(int fd, char s[])
{
  int len;
  READ_XDR_32(fd, &len, sizeof(len));
  READ(fd, s, len+1);
  return 0;
}

static unsigned int magic = 0xde728eab;

#define min(x,y) ((x)<(y)?(x):(y))

static int
copy(int fdsrc, int fddest, unsigned long long &size, int import = 0,
     unsigned long long rsize = 0,
     int info = 0, Bool lseek_ok = True)
{
  char buf[512];
  static char const zero[sizeof buf] = { 0 };
  int n;
  off_t zeros = 0;
  int cnt;
  unsigned long long cdsize;

  if (import)
    {
      cdsize = size;
      cnt = min(sizeof(buf), cdsize);
    }
  else
    cnt = sizeof(buf);

  size = 0;

  while ((n = read(fdsrc, buf, cnt)) > 0)
    {
      size += n;

      if (memcmp(buf, zero, n) == 0)
	zeros += n;
      else if (!info)
	{
	  if (zeros)
	    {
	      if (lseek_ok)
		LSEEK(fddest, zeros, SEEK_CUR);
	      else
		while (zeros--)
		  WRITE(fddest, zero, 1);

	      zeros = 0;
	    }

	  WRITE(fddest, buf, n);
	}
      else
	zeros = 0;

      if (import)
	{
	  cnt = min(sizeof(buf), cdsize-size);
	  if (size >= cdsize)
	    break;
	}
    }

  if (n < 0)
    return 1;

  size -= zeros;

  if (import && rsize > 0)
    {
      char zero = 0;
      LSEEK(fddest, rsize-1, SEEK_SET);
      WRITE(fddest, &zero, 1);
    }

  return 0;
}

static int
code_file(int fd, int file_fd, unsigned long long &totsize, Bool info, Bool lseek_ok)
{
  struct stat st = {0};
  if (file_fd != -1) {
    if (fstat(file_fd, &st) < 0)
      {
	perror("stat");
	return 1;
      }
  }

  unsigned long long size;
  if (info)
    {
      WRITE_XDR_64(fd, &st.st_size, sizeof(st.st_size)); // real size
      if (file_fd != -1) {
	if (copy(file_fd, fd, size, 0, 0, 1))
	  return 1;
      }
      else
	size = 0;
      WRITE_XDR_64(fd, &size, sizeof(size)); // sparsify size
    }
  else
    {
      WRITE_XDR_32(fd, &magic, sizeof(magic));
      if (file_fd != -1)
	if (copy(file_fd, fd, size, 0, 0, 0, lseek_ok)) return 1;
    }

  totsize += size;
  return 0;
}

struct DBMethod {
  char extref[64];
  char file[256];
  int fd;
  unsigned long long cdsize, rsize;
};

struct DBDatafile {
  int datfd, dmpfd;
  char file[eyedbsm::L_FILENAME];
  char name[eyedbsm::L_NAME+1];
  unsigned long long maxsize;
  unsigned int slotsize;
  unsigned int dtype;
#ifdef KEEP_MAX_INFO
  short dspid;
#endif
  unsigned long long dat_cdsize, dat_rsize;
  unsigned long long dmp_cdsize, dmp_rsize;
};

struct DBDataspace {
  char name[eyedbsm::L_NAME+1];
  unsigned int ndat;
  short datid[eyedbsm::MAX_DAT_PER_DSP];
};

struct DBInfo {
  char dbname[256];
  off_t offset;
  unsigned long long size;
  eyedbsm::Oid::NX nbobjs;
  unsigned int dbid;
  unsigned int ndat;
#ifdef KEEP_MAX_INFO
  unsigned int ndsp;
#endif
  unsigned int nmths;
  unsigned long long db_cdsize, db_rsize;
  unsigned long long omp_cdsize, omp_rsize;
  DBDatafile *datafiles;
#ifdef KEEP_MAX_INFO
  DBDataspace *dataspaces;
#endif
  DBMethod *mths;
};

static const char *
dmpfileGet(const char *datafile)
{
  static std::string str;
  char *s = strdup(datafile);
  char *p;
  if (p = strrchr(s, '.'))
    *p = 0;
  strcat(s, ".dmp");
  str = s;
  free(s);
  return str.c_str();
}

static int
export_realize(int fd, const DbInfoDescription &dbdesc,
	       const char *dbname, unsigned int dbid,
	       eyedbsm::Oid::NX nbobjs, int dbfd, int ompfd,
	       DBDatafile datafiles[], unsigned int ndat, 
	       DBDataspace dataspaces[], unsigned int ndsp,
	       DBMethod *mths, unsigned int nmths)
{
  Bool lseek_ok = (lseek(fd, 0, SEEK_CUR) < 0 ? False : True);

  WRITE_XDR_32(fd, &magic, sizeof(magic));
  unsigned int version = getVersionNumber();
  WRITE_XDR_32(fd, &version, sizeof(version));

  if (code_string(fd, dbname))
    return 1;

  WRITE_XDR_32(fd, &nbobjs, sizeof(nbobjs));
  WRITE_XDR_32(fd, &dbid, sizeof(dbid));
  WRITE_XDR_32(fd, &ndat, sizeof(ndat));
#ifdef KEEP_MAX_INFO
  WRITE_XDR_32(fd, &ndsp, sizeof(ndsp));
#endif
  WRITE_XDR_32(fd, &nmths, sizeof(nmths));

  unsigned long long size = 0;

  fprintf(stderr, "Coding header...\n");

  if (code_file(fd, dbfd, size, True, lseek_ok))
    return 1;
  if (code_file(fd, ompfd, size, True, lseek_ok))
    return 1;

  int i, j;

  for (i = 0; i < ndat; i++) {
    if (code_string(fd, datafiles[i].file))
      return 1;
    if (code_string(fd, datafiles[i].name))
      return 1;
    WRITE_XDR_64(fd, &datafiles[i].maxsize, sizeof(datafiles[i].maxsize));
    WRITE_XDR_32(fd, &datafiles[i].slotsize, sizeof(datafiles[i].slotsize));
    WRITE_XDR_32(fd, &datafiles[i].dtype, sizeof(datafiles[i].dtype));
#ifdef KEEP_MAX_INFO
    WRITE_XDR_16(fd, &datafiles[i].dspid, sizeof(datafiles[i].dspid));
#endif
    if (code_file(fd, datafiles[i].datfd, size, True, lseek_ok))
      return 1;
    if (code_file(fd, datafiles[i].dmpfd, size, True, lseek_ok))
      return 1;
  }

#ifdef KEEP_MAX_INFO
  for (i = 0; i < ndsp; i++) {
    if (code_string(fd, dataspaces[i].name))
      return 1;
    WRITE_XDR_32(fd, &dataspaces[i].ndat, sizeof(dataspaces[i].ndat));
    //    WRITE_XDR_16_ARR(fd, dataspaces[i].datid, dataspaces[i].ndat * sizeof(dataspaces[i].datid[0]));
    WRITE_XDR_16_ARR(fd, dataspaces[i].datid, dataspaces[i].ndat);
  }
#endif

  for (j = 0; j < nmths; j++) {
    if (code_string(fd, mths[j].extref))
      return 1;
    if (code_string(fd, mths[j].file))
      return 1;
    if (code_file(fd, mths[j].fd, size, True, lseek_ok))
      return 1;
  }

  WRITE_XDR_64(fd, &size, sizeof(size));

  //fprintf(stderr, "TOTAL SIZE = %d [%d]\n", size, lseek(fd, 0, SEEK_CUR));

  LSEEK(dbfd, 0, SEEK_SET);
  fprintf(stderr, "Coding file %s...\n", dbdesc.dbfile);
  if (code_file(fd, dbfd, size, False, lseek_ok)) return 1;
  LSEEK(ompfd, 0, SEEK_SET);
  fprintf(stderr, "Coding file %s...\n", get_ompfile(dbdesc.dbfile));
  if (code_file(fd, ompfd, size, False, lseek_ok)) return 1;

  for (i = 0; i < ndat; i++) {
    if (datafiles[i].datfd != -1)
      LSEEK(datafiles[i].datfd, 0, SEEK_SET);
    fprintf(stderr, "Coding file %s...\n", datafiles[i].file);
    if (code_file(fd, datafiles[i].datfd, size, False, lseek_ok))
      return 1;
    fprintf(stderr, "Coding file %s...\n", dmpfileGet(datafiles[i].file));
    if (datafiles[i].dmpfd != -1)
      LSEEK(datafiles[i].dmpfd, 0, SEEK_SET);
    if (code_file(fd, datafiles[i].dmpfd, size, False, lseek_ok))
      return 1;
  }

  for (j = 0; j < nmths; j++) {
    LSEEK(mths[j].fd, 0, SEEK_SET);
    fprintf(stderr, "Coding file %s...\n", mths[j].file);
    if (code_file(fd, mths[j].fd, size, False, lseek_ok))
      return 1;
  }

  return 0;
}

static int
is_system_method(const char *name)
{
  return !strncmp(name, "etcmthbe", strlen("etcmthbe")) ||
    !strncmp(name, "etcmthfe", strlen("etcmthfe")) ||
    !strncmp(name, "oqlctbmthbe", strlen("oqlctbmthbe")) || 
    !strncmp(name, "oqlctbmthfe", strlen("oqlctbmthfe")) ||
    !strncmp(name, "utilsmthfe", strlen("utilsmthfe")) ||
    !strncmp(name, "utilsmthfe", strlen("utilsmthfe"));
}

static int
methods_manage(Database &db, DBMethod*& mths, unsigned int& nmths)
{
  db.transactionBegin();

  OQL q(&db, "select method");

  ObjectArray obj_arr;
  Status s = q.execute(obj_arr);
  CHECK(s);

  int err = 0;
  mths = 0;
  nmths = 0;
  int n = obj_arr.getCount();
  if (n) {
    mths = new DBMethod[n];
    memset(mths, 0, sizeof(mths[0])*n);
    for (int i = 0; i < n; i++) {
      Method *m = (Method *)obj_arr[i];
      if (m->asBEMethod_OQL())
	continue;
      const char *extref = m->getEx()->getExtrefBody().c_str();
      if (is_system_method(extref))
	continue;

      int j;
      for (j = 0; j < nmths; j++)
	if (!strcmp(mths[j].extref, extref))
	  break;

      if (j == nmths) {
	int r = nmths;
	const char *s = Executable::getSOFile(extref);

	if (!s) {
	  print_prog();
	  fprintf(stderr, "cannot find file for extref '%s'\n",
		  extref);
	  err = 1;
	  continue;
	}

	if ((mths[nmths].fd = open(s, O_RDONLY)) < 0) {
	  print_prog();
	  fprintf(stderr, "cannot open method file '%s' for reading\n",
		  (const char *)s);
	  err = 1;
	  continue;
	}
	      
	strcpy(mths[nmths].extref, extref);
	strcpy(mths[nmths].file, Executable::makeExtRef(extref));
	nmths++;

	if (nmths == r) {
	  print_prog();
	  fprintf(stderr, "no '%s' method file found.\n",
		  Executable::makeExtRef(extref));
	  err = 1;
	  strcpy(mths[nmths++].extref, extref);
	}
      }
    }
  }

  db.transactionAbort();
  return err;
}

/*
  static int
  xlock_db(Database &db)
  {
  TransactionParams params = Database::getGlobalDefaultTransactionParams();
  params.lockmode = DatabaseX;
  params.wait_timeout = 1;

  Status s = db.transactionBegin(params);
  if (!s) return 0;

  print_prog();

  if (s->getStatus() == eyedbsm::LOCK_TIMEOUT)
  fprintf(stderr,
  "cannot acquire exclusive lock on database %s\n",
  db.getName());
  else
  s->print();

  return 1;
  }
*/

static int
dbexport_realize(const char *dbname, const char *file)
{
  if (!*userauth) {
    const char *env = eyedb::ClientConfig::getCValue("user");
    if (env)
      strcpy(userauth, env);
  }

  if (!*passwdauth) {
    const char *env = eyedb::ClientConfig::getCValue("passwd");
    if (env)
      strcpy(passwdauth, env);
  }

  if (!eyedb::ServerConfig::getSValue("sopath")) {
    print_prog();
    fprintf(stderr, "variable 'sopath' must be set in your configuration file.\n");
    return 1;
  }

  Database db(dbname);

  Status st = db.open(conn, Database::DBSRead, userauth, passwdauth);
  if (st) exc_handler(st, 0);

  st = db.transactionBeginExclusive();
  if (st) exc_handler(st, 0);

  Exception::setHandler(exc_handler);

  unsigned int dbid = db.getDbid();
  DbCreateDescription dbdesc;
  db.getInfo(conn, userauth, passwdauth, &dbdesc);

  int dbfd = open(dbdesc.dbfile, O_RDONLY);
  if (dbfd < 0) {
    print_prog();
    fprintf(stderr, "cannot open dbfile '%s' for reading\n",
	    dbdesc.dbfile);
    return 1;
  }

  const char *ompfile = get_ompfile(dbdesc.dbfile);
  int ompfd = open(ompfile, O_RDONLY);
  if (ompfd < 0) {
    print_prog();
    fprintf(stderr, "cannot open ompfile '%s' for reading\n", ompfile);
    return 1;
  }

  const eyedbsm::DbCreateDescription *s = &dbdesc.sedbdesc;

  DBDatafile *datafiles = new DBDatafile[s->ndat];
  memset(datafiles, 0, sizeof(datafiles[0])*s->ndat);
  char *pathdir = strdup(dbdesc.dbfile);
  char *x = strrchr(pathdir, '/');
  if (x) *x = 0;
  
  for (int i = 0; i < s->ndat; i++) {
    const char *file = s->dat[i].file;
    const char *p = strrchr(file, '/');

    if (!*s->dat[i].file)
      *datafiles[i].file = 0;
    else if (p)
      strcpy(datafiles[i].file, p+1);
    else {
      strcpy(datafiles[i].file, file);
      file = strdup((std::string(pathdir) + "/" + std::string(s->dat[i].file)).c_str());
    }

    if (*datafiles[i].file) {
      if ((datafiles[i].datfd = open(file, O_RDONLY)) < 0) {
	print_prog();
	fprintf(stderr, "cannot open datafile '%s' for reading\n",
		file);
	return 1;
      }

      if ((datafiles[i].dmpfd = open(dmpfileGet(file), O_RDONLY)) < 0) {
	print_prog();
	fprintf(stderr, "cannot open dmpfile '%s' for reading\n",
		file);
	return 1;
      }
    }
    else {
      datafiles[i].datfd = -1;
      datafiles[i].dmpfd = -1;
    }

    strcpy(datafiles[i].name, s->dat[i].name);
    datafiles[i].maxsize = s->dat[i].maxsize;
    datafiles[i].slotsize = s->dat[i].sizeslot;
    datafiles[i].dtype = s->dat[i].dtype;

#ifdef KEEP_MAX_INFO
    datafiles[i].dspid = s->dat[i].dspid;
#endif
  }

#ifdef KEEP_MAX_INFO
  DBDataspace *dataspaces = new DBDataspace[s->ndsp];

  for (int i = 0; i < s->ndsp; i++) {
    strcpy(dataspaces[i].name, s->dsp[i].name);
    dataspaces[i].ndat = s->dsp[i].ndat;
    memcpy(dataspaces[i].datid, s->dsp[i].datid,
	   s->dsp[i].ndat*sizeof(s->dsp[i].datid[0]));
  }
#else
  DBDataspace *dataspaces = 0;
#endif

  int fd;

  if (!strcmp(file, "-"))
    fd = 1;
  else {
    fd = creat(file, 0666);

    if (fd < 0) {
      print_prog();
      fprintf(stderr, "cannot create file `%s'\n", file);
      return 1;
    }
  }

  unsigned int nmths;
  DBMethod *mths;

  if (methods_manage(db, mths, nmths))
    return 1;

  if (export_realize(fd, dbdesc, dbname, dbid, s->nbobjs, dbfd, ompfd,
		     datafiles, s->ndat,
		     dataspaces, s->ndsp,
		     mths, nmths)) {
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

static void
getansw(const char *msg, char *&rdir, const char *defdir, Bool isdir = True)
{
  char dir[256];
  *dir = 0;
  //  while (!*dir || *dir != '/')
  {
    printf(msg);
    if (defdir)
      printf(" [%s]", defdir);
    printf(" ? ");

    if (!fgets(dir, sizeof(dir)-1, stdin))
      exit(1);

    if (strrchr(dir, '\n'))
      *strrchr(dir, '\n') = 0;

    if (!*dir && defdir)
      strcpy(dir, defdir);

    if (isdir && *dir && *dir != '/')
      printf("Please, needs an absolute path.\n");
  }

  rdir = strdup(dir);
}

static int
decode_info_file(int fd, unsigned long long &rsize, unsigned long long &cdsize)
{
  READ_XDR_64(fd, &rsize, sizeof(rsize));
  READ_XDR_64(fd, &cdsize, sizeof(cdsize));

  return 0;
}

static int
decode_file(int fd, const char *f, const char *, const char *dir,
	    unsigned long long cdsize, unsigned long long rsize,
	    int check_if_exist = 0)
{
  unsigned int m;
  READ_XDR_32(fd, &m, sizeof(m));

  if (m != magic) {
    print_prog();
    fprintf(stderr, "invalid EyeDB export file while decoding file '%s'\n",
	    f);
    return 1;
  }

  char file[256];
  int fddest;
  Bool lseek_ok = True;

  if (*dir)
    sprintf(file, "%s/%s", dir, f);
  else
    sprintf(file, f);

  fprintf(stderr, "Decoding file %s...\n", file);

  if (is_system_method(f)) {
    fddest = open(file, O_RDONLY);
    strcpy(file, "/dev/null");
    lseek_ok = False;
  }
  else {
    if (check_if_exist) {
      fddest = open(file, O_RDONLY);
      if (fddest >= 0) {
	print_prog();
	fprintf(stderr, "file '%s' already exists: cannot overwrite it\n", file);
	close(fddest);
	return 1;
      }
    }

    fddest = creat(file, 0600);
  }

  if (fddest < 0) {
    print_prog();
    fprintf(stderr, "cannot create file '%s'\n", file);
    return 1;
  }
  
  if (copy(fd, fddest, cdsize, 1, rsize, 0, lseek_ok))
    return 1;

  /*  printf("decode_file `%s' %p [%d] cdsize = %d, rsize = %d\n",
      f, magic, lseek(fd, 0, SEEK_CUR), cdsize, rsize); */

  return 0;
}

static int
dbimport_getinfo(int fd, const char *file, DBInfo &info, unsigned int &version)
{
  unsigned int m;
  READ_XDR_32(fd, &m, sizeof(m));

  if (m != magic) {
    print_prog();
    fprintf(stderr, "invalid EyeDB export file\n");
    return 1;
  }

  READ_XDR_32(fd, &version, sizeof(version));

  if (decode_string(fd, info.dbname))
    return 1;

  READ_XDR_32(fd, &info.nbobjs, sizeof(info.nbobjs));
  READ_XDR_32(fd, &info.dbid, sizeof(info.dbid));
  READ_XDR_32(fd, &info.ndat, sizeof(info.ndat));
#ifdef KEEP_MAX_INFO
  READ_XDR_32(fd, &info.ndsp, sizeof(info.ndsp));
#endif
  READ_XDR_32(fd, &info.nmths, sizeof(info.nmths));

  if (decode_info_file(fd, info.db_rsize, info.db_cdsize))
    return 1;

  if (decode_info_file(fd, info.omp_rsize, info.omp_cdsize))
    return 1;

  info.datafiles = new DBDatafile[info.ndat];
  memset(info.datafiles, 0, sizeof(info.datafiles[0])*info.ndat);

  for (int i = 0; i < info.ndat; i++) {
    if (decode_string(fd, info.datafiles[i].file))
      return 1;
    if (decode_string(fd, info.datafiles[i].name))
      return 1;
    READ_XDR_64(fd, &info.datafiles[i].maxsize, sizeof(info.datafiles[i].maxsize));
    READ_XDR_32(fd, &info.datafiles[i].slotsize, sizeof(info.datafiles[i].slotsize));
    READ_XDR_32(fd, &info.datafiles[i].dtype, sizeof(info.datafiles[i].dtype));
#ifdef KEEP_MAX_INFO
    READ_XDR_16(fd, &info.datafiles[i].dspid, sizeof(info.datafiles[i].dspid));
#endif
    if (decode_info_file(fd, info.datafiles[i].dat_rsize, info.datafiles[i].dat_cdsize))
      return 1;
    if (decode_info_file(fd, info.datafiles[i].dmp_rsize, info.datafiles[i].dmp_cdsize))
      return 1;
  }

#ifdef KEEP_MAX_INFO
  info.dataspaces = new DBDataspace[info.ndsp];
  for (int i = 0; i < info.ndsp; i++) {
    if (decode_string(fd, info.dataspaces[i].name))
      return 1;
    READ_XDR_32(fd, &info.dataspaces[i].ndat, sizeof(info.dataspaces[i].ndat));
    //    READ_XDR_16_ARR(fd, info.dataspaces[i].datid, info.dataspaces[i].ndat * sizeof(info.dataspaces[i].datid[0]));
    READ_XDR_16_ARR(fd, info.dataspaces[i].datid, info.dataspaces[i].ndat);
  }
#endif

  if (info.nmths) {
    info.mths = new DBMethod[info.nmths];
    memset(info.mths, 0, sizeof(info.mths[0]));
    for (int j = 0; j < info.nmths; j++) {
      if (decode_string(fd, info.mths[j].extref))
	return 1;
      if (decode_string(fd, info.mths[j].file))
	return 1;
      if (decode_info_file(fd, info.mths[j].rsize, info.mths[j].cdsize))
	return 1;
    }
  }

  READ_XDR_64(fd, &info.size, sizeof(info.size));
  return 0;
}

static int
dbimport_realize(int fd, const char *file, const char *dbname,
		 const char *_filedir, const char *_mthdir, Bool lseek_ok,
		 DBInfo &info)
{
  if (lseek_ok && !dbname) {
    char s[128];
    printf("database [%s]? ", info.dbname);

    if (!fgets(s, sizeof(s)-1, stdin))
      return 1;

    if (strrchr(s, '\n'))
      *strrchr(s, '\n') = 0;

    if (*s)
      strcpy(info.dbname, s);
  }
  else {
    if (!dbname) {
      print_prog();
      fprintf(stderr, "use -db option when importing from a pipe.\n");
      return 1;
    }

    strcpy(info.dbname, dbname);
  }

  char *dbdir = 0;
  char **filedir = new char*[info.ndat];
  memset(filedir, 0, sizeof(*filedir)*info.ndat);

  int i;
  if (lseek_ok && !_filedir) {
    getansw("Directory for Database File",
	    dbdir, eyedb::ServerConfig::getSValue("databasedir"));

    for (i = 0; i < info.ndat; i++) {
      if (!*info.datafiles[i].file)
	continue;
      printf("Datafile #%d%s:\n", i,
	     (*info.datafiles[i].name ? std::string(" (") + info.datafiles[i].name + ")" : std::string("")).c_str());
      char *name;
      getansw("  File name", name, info.datafiles[i].file, False);
      strcpy(info.datafiles[i].file, name);
      getansw("  Directory", filedir[i], (!i ? "" : filedir[i-1]));
    }
  }
  else {
    if (!_filedir) {
      _filedir = eyedb::ServerConfig::getSValue("databasedir");
    }

    /*
      for (i = 0; i < info.ndat; i++)
      filedir[i] = (char *)_filedir;
    */

    dbdir = (char *)_filedir;
  }

  char **mthdir = 0;
  if (info.nmths) {
    std::string def_mthdir = eyedb::ServerConfig::getSValue("sopath");
    mthdir = new char*[info.nmths];
    if (lseek_ok && !_mthdir) {
      for (i = 0; i < info.nmths; i++)
	getansw((std::string("Directory for Method File '") + info.mths[i].file +
		 "'").c_str(),
		mthdir[i], (!i ? def_mthdir.c_str() : mthdir[i-1]));
    }
    else {
      if (!_mthdir)
	_mthdir = strdup(def_mthdir.c_str());

      for (i = 0; i < info.nmths; i++)
	mthdir[i] = (char *)_mthdir;
    }
  }

  std::string dbfile = std::string(dbdir) + "/" + std::string(info.dbname) + ".dbs";
  std::string ompfile = std::string(dbdir) + "/" + std::string(info.dbname) + ".omp";
  char **datafiles = new char *[info.ndat*5];

  for (int j = 0; j < info.ndat; j++) {
    std::string s;
    if (!filedir[j] || !*filedir[j])
      datafiles[5*j] = strdup(info.datafiles[j].file);
    else
      datafiles[5*j] = strdup((std::string(filedir[j]) + "/" + info.datafiles[j].file).c_str());
    datafiles[5*j+1] = strdup(info.datafiles[j].name);

#ifdef DATSZ_IN_M
    datafiles[5*j+2] = strdup(str_convert((long)info.datafiles[j].maxsize/1024).c_str());
#else
    datafiles[5*j+2] = strdup(str_convert((long)info.datafiles[j].maxsize));
#endif

    datafiles[5*j+3] = strdup(str_convert((long)info.datafiles[j].slotsize).c_str());
    datafiles[5*j+4] = strdup(info.datafiles[j].dtype == eyedbsm::PhysicalOidType ? "phy" : "log");
  }
      
  printf("\nCreating database %s...\n", info.dbname);
  if (dbcreate_realize(info.dbname, (char *)dbfile.c_str(), 
		       datafiles, info.ndat*5, (const char *)0,
		       info.nbobjs, info.dbid))
    return 1;

  if (decode_file(fd, (std::string(info.dbname) + ".dbs").c_str(), info.dbname,
		  dbdir, info.db_cdsize, info.db_rsize)) {
    dbdelete_realize(info.dbname);
    return 1;
  }

  if (decode_file(fd, (std::string(info.dbname) + ".omp").c_str(), info.dbname,
		  dbdir, info.omp_cdsize, info.omp_rsize)) {
    dbdelete_realize(info.dbname);
    return 1;
  }

  for (i = 0; i < info.ndat; i++) {
    if (!*info.datafiles[i].file)
      continue;
    if (decode_file(fd, info.datafiles[i].file,
		    info.dbname,
		    (filedir[i] && *filedir[i] == '/' ? filedir[i] : dbdir),
		    info.datafiles[i].dat_cdsize, info.datafiles[i].dat_rsize)) {
      dbdelete_realize(info.dbname);
      return 1;
    }

    if (decode_file(fd, dmpfileGet(info.datafiles[i].file),
		    info.dbname,
		    (filedir[i] && *filedir[i] == '/' ? filedir[i] : dbdir),
		    info.datafiles[i].dmp_cdsize, info.datafiles[i].dmp_rsize)) {
      dbdelete_realize(info.dbname);
      return 1;
    }
  }

  eyedbsm::DbRelocateDescription rel;
  rel.ndat = info.ndat;
  for (i = 0; i < info.ndat; i++)
    strcpy(rel.file[i], datafiles[5*i]);

  eyedbsm::Status se = eyedbsm::dbRelocate(dbfile.c_str(), &rel);
  if (se) {
    eyedbsm::statusPrint(se, "relocation failed");
    dbdelete_realize(info.dbname);
    return 1;
  }

  for (i = 0; i < info.nmths; i++) {
    if (decode_file(fd, info.mths[i].file, "", mthdir[i], 
		    info.mths[i].cdsize, info.mths[i].rsize, 1)) {
      dbdelete_realize(info.dbname);
      return 1;
    }
  }

  return 0;
}

static void
dbinfo_display(const char *msg, unsigned long long cdsize,
	       unsigned long long rsize)
{
  printf("  %-15s %lldb [real size %lldb]\n", msg, cdsize, rsize);
}

static int
dbimport_list(const char *file, DBInfo &info, unsigned int version)
{
  if (!strcmp(file, "-") || !file)
    printf("\nImport File <stdin>\n\n");
  else
    printf("\nImport File \"%s\"\n\n", file);

  printf("  Version         %d\n", version);
  printf("  Database        %s\n", info.dbname);
  printf("  Dbid            %d\n", info.dbid);
  printf("  Total Size      %llub\n", info.size);

  dbinfo_display("Database File", info.db_cdsize, info.db_rsize);
  dbinfo_display("Object Map File", info.omp_cdsize, info.omp_rsize);

  printf("  Datafile Count  %d\n", info.ndat);

  for (int i = 0; i < info.ndat; i++) {
    dbinfo_display((std::string("Datafile #") + str_convert((long)i)).c_str(),
		   info.datafiles[i].dat_cdsize, info.datafiles[i].dat_rsize);
    dbinfo_display("  Dmpfile",
		   info.datafiles[i].dmp_cdsize, info.datafiles[i].dmp_rsize);
    printf("    File          %s\n", info.datafiles[i].file);
    if (*info.datafiles[i].name)
      printf("    Name          %s\n", info.datafiles[i].name);
    printf("    Maxsize       %lldKb\n", info.datafiles[i].maxsize);
    printf("    Slotsize      %db\n", info.datafiles[i].slotsize);
    printf("    Type          %s\n",
	   info.datafiles[i].dtype == eyedbsm::PhysicalOidType ? "Physical" : "Logical");
#ifdef KEEP_MAX_INFO
    printf("    Dspid         #%d\n", info.datafiles[i].dspid);
#endif
  }

#ifdef KEEP_MAX_INFO
  printf("  Dataspace Count  %d\n", info.ndsp);
  for (int i = 0; i < info.ndsp; i++) {
    printf("  Dataspace    #%d\n", i);
    printf("    Name    %s\n", info.dataspaces[i].name);
    printf("    Datafile Count    %d\n", info.dataspaces[i].ndat);
    for (int j = 0; j < info.dataspaces[i].ndat; j++)
      printf("      Datafile #%d\n", info.dataspaces[i].datid[j]);
  }
#endif

  if (info.nmths)
    printf("\n");

  for (int i = 0; i < info.nmths; i++) {
    std::string s = (std::string)"Method File (" + info.mths[i].extref + ")";
    printf("  %-15s '%s' [real size %db]\n", s.c_str(),
	   info.mths[i].file,
	   info.mths[i].rsize);
  }

  return 0;
}

static void
read_pipe(int fd)
{
  if (fd == 0) {
    char buf[8192];
    while (read(fd, buf, sizeof(buf)) > 0)
      ;
  }
}

#include <pwd.h>

static const char *
getUserName(int uid)
{
  struct passwd *p = getpwuid(uid);
  if (!p)
    return "<unknown>";
  return p->pw_name;
}

static int
dbimport_realize(const char *file, int list, const char *dbname = 0,
		 const char *filedir = 0, const char *mthdir = 0)
{
  int fd;
  
  if (!file || !strcmp(file, "-"))
    fd = 0;
  else {
    fd = open(file, O_RDONLY);

    if (fd < 0) {
      print_prog();
      fprintf(stderr, "cannot open file '%s' for reading\n", file);
      return 1;
    }
  }

  //  Bool lseek_ok = (lseek(fd, 0, SEEK_CUR) < 0 ? False : True);
  Bool lseek_ok = (fd == 0 ? False : True);

  DBInfo info;
  unsigned int version;
  if (dbimport_getinfo(fd, file, info, version))
    return 1;

  if (list) {
    int r = dbimport_list(file, info, version);
    read_pipe(fd);
    return r;
  }

  if (conn->getServerUid() != getuid()) {
    print_prog();
    fprintf(stderr, "EyeDB server is running under user '%s': database importation must be done under the same uid than the EyeDB server.\n",
	    getUserName(conn->getServerUid()));
    return 1;
  }

  auth_realize();

  int r = dbimport_realize(fd, file, dbname, filedir, mthdir, lseek_ok, info);
  return r;
}

static int
dbcopy_realize(char *dbname, char *newdbname,
	       char *dbfile, char *datafiles[], int datafiles_cnt,
	       const char *filedir, Bool newdbid)
{
  if (!newdbname) {
    print_prog();
    fprintf(stderr, "newdbname is not set\n");
    return 1;
  }

  DbCreateDescription dbdesc;
  Database *db = new Database(dbname, dbmdb);

  if (dbcopy_prologue(db, dbname, newdbname, dbfile, datafiles, datafiles_cnt,
		      filedir, &dbdesc))
    return 1;

  Status status = db->copy(conn, newdbname, False, &dbdesc,
			   userauth, passwdauth);
  CHECK(status);
  return 0;
}

static int
get_datafile_opts(char *s, char *datafiles[], int *datafiles_cnt)
{
  char *p = strchr(s, ':');
  if (!p) {
    datafiles[(*datafiles_cnt)++] = s;
    datafiles[(*datafiles_cnt)++] = "";
    datafiles[(*datafiles_cnt)++] = DEF_DATSIZE_STR;
    datafiles[(*datafiles_cnt)++] = DEF_SIZESLOT_STR;
    datafiles[(*datafiles_cnt)++] = DEF_DATTYPE_STR;
    return 0;
  }

  *p = 0;
  datafiles[(*datafiles_cnt)++] = s;
  datafiles[(*datafiles_cnt)++] = p+1;

  char *q = strchr(p+1, ':');
  if (!q) {
    datafiles[(*datafiles_cnt)++] = DEF_DATSIZE_STR;
    datafiles[(*datafiles_cnt)++] = DEF_SIZESLOT_STR;
    datafiles[(*datafiles_cnt)++] = DEF_DATTYPE_STR;
    return 0;
  }

  *q = 0;
  char *r = strchr(q+1, ':');
  datafiles[(*datafiles_cnt)++] = (char *)(*(q+1) ? q+1 : DEF_DATSIZE_STR);
  if (!r) {
    datafiles[(*datafiles_cnt)++] = DEF_SIZESLOT_STR;
    datafiles[(*datafiles_cnt)++] = DEF_DATTYPE_STR;
    return 0;
  }

  /*
    if (strchr(r+1, ':'))
    return 1;
  */

  *r = 0;

  char *x = strchr(r+1, ':');
  if (x) *x = 0;
  //datafiles[(*datafiles_cnt)++] = (char *)(*(p+1) ? q+1 : DEF_DATSIZE_STR);
  datafiles[(*datafiles_cnt)++] = (char *)(*(r+1) ? r+1 : DEF_SIZESLOT_STR);

  if (!x) {
    datafiles[(*datafiles_cnt)++] = DEF_DATTYPE_STR;
    return 0;
  }
   
  datafiles[(*datafiles_cnt)++] = (char *)(*(x+1) ? x+1 : DEF_DATTYPE_STR);
  return 0;
}

static void
DATDUMP(char *datafiles[], int datafiles_cnt)
{
  assert(!(datafiles_cnt % 5));
  for (int i = 0; i < datafiles_cnt; i += 5)
    {
      printf("FILE '%s'\n", datafiles[i]);
      printf("NAME '%s'\n", datafiles[i+1]);
      printf("SIZE '%s'\n", datafiles[i+2]);
      printf("SLOT '%s'\n", datafiles[i+3]);
      printf("TYPE '%s'\n", datafiles[i+4]);
    }
  //  exit(0);
}

static int
get_db_opts(int s, int argc, char *argv[],
	    string &dbfile, char *datafiles[],
	    int *datafiles_cnt, Bool dbcreate,
	    string &filedir, unsigned int &nbobjs,
	    Bool *newdbid, char **dbname, char **newdbname,
	    Bool etc)
{
  string value;

  for (int i = s; i < argc; i++) {
    char *s = argv[i];
    string value;
    if (GetOpt::parseLongOpt(s, "dbfile", &value))
      dbfile = value;
    else if (GetOpt::parseLongOpt(s, "object-count", &value))
      nbobjs = atoi(value.c_str());
    else if (GetOpt::parseLongOpt(s, "filedir", &value))
      filedir = value;
    else if (GetOpt::parseLongOpt(s, "datafiles", &value)) {
      char *p = strdup(value.c_str());
      for (;;) {
	char *q = strchr(p, ',');
	if (q)
	  *q = 0;

	if (dbcreate) {
	  if (get_datafile_opts(p, datafiles, datafiles_cnt))
	    return 1;
	}
	else
	  datafiles[(*datafiles_cnt)++] = argv[i];

	if (!q)
	  break;
	p = q+1;
      }
    }
    else if (GetOpt::parseLongOpt(s, "new-dbid", &value))
      *newdbid = True;
    // for dbmcreate
    else if (GetOpt::parseLongOpt(s, "strict-unix", &value)) {
      *dbname = strdup(value.c_str());
      *newdbname = (char *)strict_unix_user;
    }
    else if (*s == '-')
      return usage(prog);
    else if (!*dbname)
      *dbname = argv[i];
    else if (!*newdbname)
      *newdbname = argv[i];
    else if (!etc)
      return usage(prog);
  }

  return 0;
}

static int
get_dat_opts(int s, int argc, char *argv[], mMode mode, char *&dbname,
	     char *datafiles[], int &datafile_cnt, string &filedir)
{
  datafile_cnt = 0;
  for (int i = s; i < argc; )
    {
      char *s = argv[i];
      string value;
      if (GetOpt::parseLongOpt(s, "filedir", &value)) {
	filedir = value;
	i++;
      }
      else if (*s == '-') {
	if (mode != mDatList)
	  return 1;
	i++;
      }
      else if (!dbname) {
	dbname = argv[i];
	i++;
      }
      else if (mode != mDatCreate) {
	datafiles[datafile_cnt++] = argv[i];
	i++;
      }
      else {
	if (datafile_cnt)
	  return 1;
	if (get_datafile_opts(argv[i], datafiles, &datafile_cnt))
	  return 1;
	i += datafile_cnt;
      }
    }

  return 0;
}

static int
get_mode(int argc, char *argv[], int &start)
{
  start = 1;

  if (!strcmp(prog, "eyedbadmin"))
    {
      complete = 1;

      if (argc < 2)
	return usage(prog);

      const char *cmd = argv[1];

      if (!strcmp(cmd, "--help")) {
	usage(prog);
	return 1;
      }

      if (!strcmp(cmd, "--help-eyedb-options")) {
	print_common_help(std::cerr);
	return 1;
      }

      if (!strcmp(cmd, "dbmcreate"))
	mode = mDbmCreate;
      else if (!strcmp(cmd, "dbcreate"))
	mode = mDbCreate;
      else if (!strcmp(cmd, "dbdelete"))
	mode = mDbDelete;
      else if (!strcmp(cmd, "dbmove"))
	mode = mDbMove;
      else if (!strcmp(cmd, "dbcopy"))
	mode = mDbCopy;
      else if (!strcmp(cmd, "dbrename"))
	mode = mDbRename;
      else if (!strcmp(cmd, "dbexport"))
	mode = mDbExport;
      else if (!strcmp(cmd, "dbimport"))
	mode = mDbImport;
      else if (!strcmp(cmd, "datcreate"))
	mode = mDatCreate;
      else if (!strcmp(cmd, "datdelete"))
	mode = mDatDelete;
      else if (!strcmp(cmd, "datmove"))
	mode = mDatMove;
      else if (!strcmp(cmd, "datdefragment"))
	mode = mDatDefragment;
      else if (!strcmp(cmd, "datresize"))
	mode = mDatResize;
      else if (!strcmp(cmd, "datlist"))
	mode = mDatList;
      else if (!strcmp(cmd, "datrename"))
	mode = mDatRename;
      else if (!strcmp(cmd, "dspcreate"))
	mode = mDspCreate;
      else if (!strcmp(cmd, "dspupdate"))
	mode = mDspUpdate;
      else if (!strcmp(cmd, "dspdelete"))
	mode = mDspDelete;
      else if (!strcmp(cmd, "dspRename"))
	mode = mDspRename;
      else if (!strcmp(cmd, "dsplist"))
	mode = mDspList;
      else if (!strcmp(cmd, "dspsetdefault"))
	mode = mDspSetDefault;
      else if (!strcmp(cmd, "dspgetdefault"))
	mode = mDspGetDefault;
      else if (!strcmp(cmd, "dspsetcurdat"))
	mode = mDspSetCurDat;
      else if (!strcmp(cmd, "dspgetcurdat"))
	mode = mDspGetCurDat;
      else if (!strcmp(cmd, "useradd"))
	mode = mUserAdd;
      else if (!strcmp(cmd, "userdelete"))
	mode = mUserDelete;
      else if (!strcmp(cmd, "userpasswd"))
	mode = mUserPasswdSet;
      else if (!strcmp(cmd, "passwd"))
	mode = mPasswdSet;
      else if (!strcmp(cmd, "userdbaccess"))
	mode = mUserDbAccessSet;
      else if (!strcmp(cmd, "sysaccess"))
	mode = mUserSysAccessSet;
      else if (!strcmp(cmd, "dbaccess"))
	mode = mDbAccessSet;
      else if (!strcmp(cmd, "userlist"))
	mode = mUserList;
      else if (!strcmp(cmd, "dblist"))
	mode = mDbList;
      else
	return usage(prog);
      start = 2;
    }
  else if (!strcmp(prog, "eyedbdbcreate") || !strcmp(prog, "dbcreate"))
    mode = mDbCreate;
  else if (!strcmp(prog, "eyedbdbmcreate") || !strcmp(prog, "dbmcreate"))
    mode = mDbmCreate;
  else if (!strcmp(prog, "eyedbdbdelete") || !strcmp(prog, "dbdelete"))
    mode = mDbDelete;
  else if (!strcmp(prog, "eyedbdbmove") || !strcmp(prog, "dbmove"))
    mode = mDbMove;
  else if (!strcmp(prog, "eyedbdbcopy") || !strcmp(prog, "dbcopy"))
    mode = mDbCopy;
  else if (!strcmp(prog, "eyedbdbrename") || !strcmp(prog, "dbrename"))
    mode = mDbRename;
  else if (!strcmp(prog, "eyedbdbexport") || !strcmp(prog, "dbexport"))
    mode = mDbExport;
  else if (!strcmp(prog, "eyedbdbimport") || !strcmp(prog, "dbimport"))
    mode = mDbImport;
  else if (!strcmp(prog, "eyedbdatcreate") || !strcmp(prog, "datcreate"))
    mode = mDatCreate;
  else if (!strcmp(prog, "eyedbdatdelete") || !strcmp(prog, "datdelete"))
    mode = mDatDelete;
  else if (!strcmp(prog, "eyedbdatmove") || !strcmp(prog, "datmove"))
    mode = mDatMove;
  else if (!strcmp(prog, "eyedbdatdefragment") || !strcmp(prog, "datdefragment"))
    mode = mDatDefragment;
  else if (!strcmp(prog, "eyedbdatresize") || !strcmp(prog, "datresize"))
    mode = mDatResize;
  else if (!strcmp(prog, "eyedbdatlist") || !strcmp(prog, "datlist"))
    mode = mDatList;
  else if (!strcmp(prog, "eyedbdspcreate") || !strcmp(prog, "dspcreate"))
    mode = mDspCreate;
  else if (!strcmp(prog, "eyedbdspupdate") || !strcmp(prog, "dspupdate"))
    mode = mDspUpdate;
  else if (!strcmp(prog, "eyedbdspdelete") || !strcmp(prog, "dspdelete"))
    mode = mDspDelete;
  else if (!strcmp(prog, "eyedbdsprename") || !strcmp(prog, "dsprename"))
    mode = mDspRename;
  else if (!strcmp(prog, "eyedbdsplist") || !strcmp(prog, "dsplist"))
    mode = mDspList;
  else if (!strcmp(prog, "eyedbdspsetdefault") || !strcmp(prog, "dspsetdefault"))
    mode = mDspSetDefault;
  else if (!strcmp(prog, "eyedbdspgetdefault") || !strcmp(prog, "dspgetdefault"))
    mode = mDspGetDefault;
  else if (!strcmp(prog, "eyedbdspsetcurdat") || !strcmp(prog, "dspsetcurdat"))
    mode = mDspSetCurDat;
  else if (!strcmp(prog, "eyedbdspgetcurdat") || !strcmp(prog, "dspgetcurdat"))
    mode = mDspGetCurDat;
  else if (!strcmp(prog, "eyedbdatrename") || !strcmp(prog, "datrename"))
    mode = mDatRename;
  else if (!strcmp(prog, "eyedbuseradd") || !strcmp(prog, "useradd"))
    mode = mUserAdd;
  else if (!strcmp(prog, "eyedbuserdelete") || !strcmp(prog, "userdelete"))
    mode = mUserDelete;
  else if (!strcmp(prog, "eyedbuserpasswd") || !strcmp(prog, "userpasswd"))
    mode = mUserPasswdSet;
  else if (!strcmp(prog, "eyedbpasswd") || !strcmp(prog, "passwd"))
    mode = mPasswdSet;
  else if (!strcmp(prog, "eyedbuserdbaccess") || !strcmp(prog, "userdbaccess"))
    mode = mUserDbAccessSet;
  else if (!strcmp(prog, "eyedbsysaccess") || !strcmp(prog, "sysaccess"))
    mode = mUserSysAccessSet;
  else if (!strcmp(prog, "eyedbuserlist") || !strcmp(prog, "userlist"))
    mode = mUserList;
  else if (!strcmp(prog, "eyedbdblist") || !strcmp(prog, "dblist"))
    mode = mDbList;
  else if (!strcmp(prog, "eyedbdbaccess") || !strcmp(prog, "dbaccess"))
    mode = mDbAccessSet;
  else
    return usage(prog);

  if (!strncmp(prog, "eyedb", strlen("eyedb")))
    eyedb_prefix = 1;

  return 0;
}

static void 
sig_h(int sig)
{
  if (sig != SIGSEGV && sig != SIGBUS)
    {
      signal(sig, sig_h);
      printf("Signal %s ignored\n", strsignal(sig));
    }
}

static void
auth_realize()
{
  if (mode != mPasswdSet)
    {
      const char *s;
      errno = 0;
      if (mode != mDbmCreate)
	{
	  if (!(s = Connection::getDefaultUser()))
	    {
#if 0
	      printf("user authentication: ");
	      fflush(stdout);
	      fgets(userauth, sizeof(userauth)-1, stdin);

	      if (errno)
		exit(1);

	      userauth[strlen(userauth)-1] = 0;
#else
	      *userauth = 0;
#endif
	    }
	  else
	    strcpy(userauth, s);
	}

      if (!(s = Connection::getDefaultPasswd()))
	{
#if 0
	  strcpy(passwdauth, getpass("password authentication: "));
	  fflush(stdout);
	  if (errno)
	    exit(1);
#else
	  *passwdauth = 0;
#endif
	}
      else
	strcpy(passwdauth, s);
    }


  /*
    for (int i = 0; i <  NSIG; i++)
    signal(i, sig_h);
  */
}

#define N 4

static void
passwd_realize(const char *prompt, char **passwd, int retype = 1)
{
  static int n;
  static char pswd[N][12];
  char *p, *q, buf[128];

  for (;;)
    {
      errno = 0;
      p = pswd[(n >= N ? (n = 0) : 0), n++];

      sprintf(buf, "%s: ", prompt);
      strcpy(p, getpass(buf));
      if (errno)
	exit(1);

      if (!retype)
	break;

      q = pswd[(n >= N ? (n = 0) : 0), n++];

      sprintf(buf, "retype %s: ", prompt);
      strcpy(q, getpass(buf));
      if (errno)
	exit(1);
      if (!strcmp(q, p))
	break;
      printf("passwords differ, try again.\n");
    }

  *passwd = p;
}

static int
db_realize(int start, int argc, char *argv[])
{
  char *datafiles[eyedbsm::MAX_DATAFILES*5], *dbname = 0, *newdbname = 0;
  int datafiles_cnt = 0;
  int i;
  string _filedir;
  string _dbfile;
  Bool newdbid = False;
  unsigned int nbobjs = DEF_NBOBJS;

  if (get_db_opts(start, argc, argv, _dbfile, datafiles, &datafiles_cnt,
		  (mode == mDbCopy || mode == mDbMove ? False : True),
		  _filedir, nbobjs,
		  &newdbid, &dbname, &newdbname,
		  (mode == mDbDelete ? True : False)))
    return usage(prog);

  char *filedir = (char *)(_filedir.length() ? _filedir.c_str() : 0);
  char *dbfile = (char *)(_dbfile.length() ? _dbfile.c_str() : 0);

  switch(mode)
    {
    case mDbmCreate:
      if (!dbname)
	return usage(prog);

      if (!newdbname) {
	char buf[64];
	sprintf(buf, "%s password", dbname);
	passwd_realize(buf, &newdbname);
      }
      else if (!strcmp(newdbname, strict_unix_user)) {
	std::string user = Connection::makeUser(dbname);
	dbname = strdup(user.c_str());
	/*
	if (!strcmp(dbname, "@")) {
	  struct passwd *pwd = getpwuid(getuid());
	  if (!pwd)
	    return usage(prog);
	  dbname = strdup(pwd->pw_name);
	}
	*/
      }

      auth_realize();
      return dbmcreate_realize(dbname, newdbname, dbfile, datafiles, datafiles_cnt);

    case mDbCreate:
      if (!dbname || newdbname)
	return usage(prog);
      auth_realize();
      return dbcreate_realize(dbname, dbfile, datafiles, datafiles_cnt,
			      filedir, nbobjs);

    case mDbDelete:
      if (!dbname || dbfile || datafiles_cnt)
	return usage(prog);
      auth_realize();
      {
	int err = 0;
	for (i = start; i < argc; i++)
	  err += dbdelete_realize(argv[i]);
	return err;
      }

    case mDbMove:
      if (!dbname || newdbname)
	return usage(prog);
      auth_realize();
      return dbmove_realize(dbname, dbfile, datafiles, datafiles_cnt, filedir);

    case mDbCopy:
      if (!dbname || !newdbname)
	return usage(prog);
      auth_realize();
      return dbcopy_realize(dbname, newdbname, dbfile, datafiles, datafiles_cnt,
			    filedir, newdbid);

    case mDbRename:
      if (!dbname || dbfile || datafiles_cnt || !newdbname)
	return usage(prog);
      auth_realize();
      return dbrename_realize(dbname, newdbname);

    case mDbAccessSet:
      if (!dbname || !newdbname || dbfile || datafiles_cnt || newdbid)
	return usage(prog);
      auth_realize();
      return default_dbaccess_realize(dbname, newdbname);

    default:
      abort();
    }

  return 1;
}

static int
datcreate_realize(Database *db, char *datafiles[], const char *filedir)
{
  if (!eyedblib::is_number(datafiles[2]) || !eyedblib::is_number(datafiles[3]))
    return usage(prog);

  eyedbsm::DatType dtype;
  if (!strcasecmp(datafiles[4], "phy"))
    dtype = eyedbsm::PhysicalOidType;
  else if (!strcasecmp(datafiles[4], "log"))
    dtype = eyedbsm::LogicalOidType;
  else
    return usage(prog);

#ifdef DATSZ_IN_M
  Status s = db->createDatafile(filedir, datafiles[0], datafiles[1],
				atoi(datafiles[2])*1024, atoi(datafiles[3]),
				dtype);
#else
  Status s = db->createDatafile(filedir, datafiles[0], datafiles[1],
				atoi(datafiles[2]), atoi(datafiles[3]),
				dtype);
#endif
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
datmove_realize(Database *db, const Datafile *datafile,
		const char *filename, const char *filedir)
{
  Status s = datafile->move(filedir, filename);
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
datdelete_realize(Database *db, const Datafile *datafile)
{
  Status s = datafile->remove();
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
datdefragment_realize(Database *db, const Datafile *datafile)
{
  Status s = datafile->defragment();
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
datresize_realize(Database *db, const Datafile *datafile, const char *strsize)
{
  if (!strsize || !eyedblib::is_number(strsize))
    return usage(prog);

#ifdef DATSZ_IN_M
  Status s = datafile->resize(atoi(strsize) * 1024);
#else
  Status s = datafile->resize(atoi(strsize));
#endif
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
get_datlist_opts(int argc, char *argv[], Bool &stats, Bool &display,
		 unsigned int &inc)
{
  if (argc > 0 && *argv[0] == '-') {
    if (!strcmp(argv[0], "--stats")) {
      stats = True;
      display = False;
      inc = 1;
      return 0;
    }

    if (!strcmp(argv[0], "--all")) {
      stats = True;
      display = True;
      inc = 1;
      return 0;
    }

    return usage(prog);
  }

  stats = False;
  display = True;
  inc = 0;
  return 0;
}

class DatlistStats {
  unsigned int objcnt;
  unsigned int slotcnt;
  unsigned int busyslotcnt;
  unsigned long long maxsize;
  unsigned long long totalsize;
  unsigned long long busyslotsize;
  unsigned long long datafilesize;
  unsigned long long datafileblksize;
  unsigned long long dmpfilesize;
  unsigned long long dmpfileblksize;
  unsigned long long defragmentablesize;
  unsigned int slotfragcnt;
  double used;
  unsigned int cnt;
  DbInfoDescription *dbdesc;

public:
  DatlistStats(DbInfoDescription *_dbdesc = 0) {

    objcnt = slotcnt = busyslotcnt = 0;
    maxsize = totalsize = busyslotsize = datafilesize = datafileblksize =
      dmpfilesize = dmpfileblksize = defragmentablesize = 0;
    slotfragcnt = 0;
    used = 0.;
    cnt = 0;
    dbdesc = _dbdesc;
  }

  int add(const Datafile *datafile) {
    if (!datafile->isValid()) return 0;

    DatafileInfo info;
    Status s = datafile->getInfo(info);
    CHECK(s);

    const DatafileInfo::Info &in = info.getInfo();
    maxsize += datafile->getMaxsize();
    objcnt += in.objcnt;
    slotcnt += in.slotcnt;
    totalsize += in.totalsize;
    busyslotcnt += in.busyslotcnt;
    busyslotsize += in.busyslotsize;

    datafilesize += in.datfilesize;
    datafileblksize += in.datfileblksize;
    dmpfilesize += in.dmpfilesize;
    dmpfileblksize += in.dmpfileblksize;

    defragmentablesize += in.defragmentablesize;
    slotfragcnt += in.slotfragcnt;
    used += in.used;
    cnt++;

    return 0;
  }

  void display() {
    if (dbdesc) {
      cout << "Statistics\n";
      cout << "  Maximum Object Number " << dbdesc->sedbdesc.nbobjs << '\n';
    }
    else
      cout << "Datafile Statistics\n";

    cout << "  Object Number         " << objcnt << '\n';
    cout << "  Maximum Slot Count    " << slotcnt << '\n';
    cout << "  Busy Slot Count       " << busyslotcnt << '\n';
    cout << "  Maximum Size          ";
    display_datsize(cout, maxsize*1024);

    if (!dbdesc) {
      cout << "  Busy Size             ";
      display_datsize(cout, totalsize);
    }

    cout << "  Busy Slot Size        ";
    display_datsize(cout, busyslotsize);

    if (dbdesc) {
      cout << "  Disk Size Used        ";
      display_datsize(cout,
		      datafilesize +
		      dmpfilesize + 
		      dbdesc->sedbdesc.dbsfilesize + 
		      dbdesc->sedbdesc.ompfilesize +
		      dbdesc->sedbdesc.shmfilesize);
      cout << "  Disk Block Size Used  ";
      display_datsize(cout,
		      datafileblksize +
		      dmpfileblksize + 
		      dbdesc->sedbdesc.dbsfileblksize + 
		      dbdesc->sedbdesc.ompfileblksize +
		      dbdesc->sedbdesc.shmfileblksize);
    }
    else {
      cout << "  .dat File Size        ";
      display_datsize(cout, datafilesize);
      cout << "  .dat File Block Size  ";
      display_datsize(cout, datafileblksize);
      
      cout << "  .dmp File Size        ";
      display_datsize(cout, dmpfilesize);
      cout << "  .dmp File Block Size  ";
      display_datsize(cout, dmpfileblksize);
      cout << "  Disk Size Used        ";
      display_datsize(cout, datafilesize+dmpfilesize);
      cout << "  Disk Block Size Used  ";
      display_datsize(cout, datafileblksize+dmpfileblksize);
      cout << "  Defragmentable Size   ";
      display_datsize(cout, defragmentablesize);
    }

    char buf[16];
    sprintf(buf, "%2.2f", used/cnt);
    cout << "  Used                  " << buf << "%\n";
  }
};

static int
datlist_realize(const Datafile *datafile, Bool &isValid)
{
  isValid = datafile->isValid();

  if (!isValid)
    return 0;

  DatafileInfo info;
  Status s = datafile->getInfo(info);
  CHECK(s);
  cout << info;
  return 0;
}

static int
datlist_realize(Database *db, char *argv[], int argc)
{
  Status s;
  Bool isValid, stats, display;
  unsigned int inc;

  if (get_datlist_opts(argc, argv, stats, display, inc))
    return 1;
    
  if (!(argc-inc)) {
    unsigned int cnt;
    const Datafile **datafiles;
    s = db->getDatafiles(datafiles, cnt);
    CHECK(s);
    if (display) {
      for (int i = 0, n = 0; i < cnt; i++) {
	if (n) cout << '\n';
	if (datlist_realize(datafiles[i], isValid)) return 1;
	if (isValid) n = 1;
	else n = 0;
      }
    }

    if (stats) {
      DatlistStats dstats;
      for (int i = 0, n = 0; i < cnt; i++)
	if (dstats.add(datafiles[i])) return 1;
      dstats.display();
    }
    return 0;
  }

  if (display) {
    for (int i = inc, n = 0; i < argc; i++) {
      const Datafile *datafile;
      s = db->getDatafile(argv[i], datafile);
      CHECK(s);
      if (n) cout << '\n';
      if (datlist_realize(datafile, isValid)) return 1;
      if (isValid) n = 1;
      else n = 0;
    }
  }
  if (stats) {
    DatlistStats dstats;
    for (int i = inc, n = 0; i < argc; i++) {
      const Datafile *datafile;
      s = db->getDatafile(argv[i], datafile);
      CHECK(s);
      if (dstats.add(datafile)) return 1;
    }
    dstats.display();
  }

  return 0;
}

static int
datrename_realize(Database *db, const Datafile *datafile, const char *newname)
{
  Status s = datafile->rename(newname);
  CHECK(s);
  db->transactionCommit();
  return 0;
}

static int
dat_realize(int start, int argc, char *argv[])
{
  char *datafiles[eyedbsm::MAX_DATAFILES*5] = {0};
  int datafile_cnt = 0;
  string _filedir;
  char *dbname = 0;

  if (get_dat_opts(start, argc, argv, mode, dbname, datafiles, datafile_cnt,
		   _filedir))
    return usage(prog);

  const char *filedir = _filedir.length() ? _filedir.c_str() : 0;

  //DATDUMP(datafiles, datafile_cnt);
  if (!dbname || (!datafile_cnt && mode != mDatList))
    return usage(prog);

  Database *db = new Database(dbname);

  Status s = db->open(conn, (mode == mDatList ? Database::DBSRead :
			     Database::DBRW));

  CHECK(s);

  if (mode != mDatList)
    s = db->transactionBeginExclusive();
  else
    s = db->transactionBegin();

  CHECK(s);

  const Datafile *datafile;
  if (mode != mDatCreate && mode != mDatList) {
    s = db->getDatafile(datafiles[0], datafile);
    CHECK(s);
  }

  switch(mode)
    {
    case mDatCreate:
      return datcreate_realize(db, datafiles, filedir);

    case mDatDelete:
      if (datafile_cnt != 1 || filedir)
	return usage(prog);
      return datdelete_realize(db, datafile);

    case mDatMove:
      if (!((datafile_cnt == 2 && !filedir) ||
	    (datafile_cnt == 1 && filedir)))
	return usage(prog);
      if (filedir) {
	datafiles[1] = strdup(datafile->getFile());
	char *p = strrchr(datafiles[1], '/');
	if (p)
	  datafiles[1] = p + 1;
      }
      return datmove_realize(db, datafile, datafiles[1], filedir);

    case mDatDefragment:
      if (datafile_cnt != 1 || filedir)
	return usage(prog);
      return datdefragment_realize(db, datafile);

    case mDatResize:
      if (datafile_cnt != 2 || filedir)
	return usage(prog);
      return datresize_realize(db, datafile, datafiles[1]);

    case mDatList:
      if (filedir)
	return usage(prog);
      return datlist_realize(db, &argv[start+1], argc-start-1);

    case mDatRename:
      if (datafile_cnt != 2 || filedir)
	return usage(prog);
      return datrename_realize(db, datafile, datafiles[1]);

    default:
      abort();
    }

  return 1;
}

static int
dspcreate_realize(Database *db, const char *dspname, char *datid[],
		  unsigned int cnt)
{
  Status s;
  const Datafile **datafiles = new const Datafile *[cnt];
  for (int i = 0; i < cnt; i++) {
    s = db->getDatafile(datid[i], datafiles[i]);
    CHECK(s);
  }

  s = db->createDataspace(dspname, datafiles, cnt);
  CHECK(s);
  return 0;
}

static int
dspupdate_realize(Database *db, const Dataspace *dataspace,
		  char *datid[], unsigned int cnt)
{
  Status s;
  const Datafile **datafiles = new const Datafile *[cnt];
  for (int i = 0; i < cnt; i++) {
    s = db->getDatafile(datid[i], datafiles[i]);
    CHECK(s);
  }

  s = dataspace->update(datafiles, cnt);
  CHECK(s);
  return 0;
}

static int
dspdelete_realize(const Dataspace *dataspace)
{
  Status s = dataspace->remove();
  CHECK(s);
  return 0;
}

static int
dsprename_realize(const Dataspace *dataspace, const char *newname)
{
  Status s = dataspace->rename(newname);
  CHECK(s);
  return 0;
}

static int
dsplist_realize(Database *db, char *argv[], int argc)
{
  Status s;
  if (!argc) {
    unsigned int cnt;
    const Dataspace **dataspaces;
    s = db->getDataspaces(dataspaces, cnt);
    CHECK(s);
    for (unsigned int i = 0, m = 0; i < cnt; i++) {
      if (dataspaces[i]->isValid()) {
	cout << (m ? "\n" : "") << *dataspaces[i];
	m++;
      }
    }
    return 0;
  }

  for (int i = 0; i < argc; i++) {
    const Dataspace *dataspace;
    s = db->getDataspace(argv[i], dataspace);
    CHECK(s);
    cout << *dataspace;
  }

  return 0;
}

static int
dspsetdefault_realize(Database *db, const Dataspace *dataspace)
{
  Status s = db->setDefaultDataspace(dataspace);
  CHECK(s);
  return 0;
}

static int
dspgetdefault_realize(Database *db)
{
  const Dataspace *dataspace;
  Status s = db->getDefaultDataspace(dataspace);
  CHECK(s);
  cout << *dataspace;
  return 0;
}

static int
dspsetcurdat_realize(Database *db, const Dataspace *dataspace,
		     const char *datafile_str)
{
  const Datafile *datafile;
  Status s = db->getDatafile(datafile_str, datafile);
  CHECK(s);
  s = const_cast<Dataspace *>(dataspace)->setCurrentDatafile(datafile);
  CHECK(s);
  return 0;
}

static int
dspgetcurdat_realize(Database *db, const Dataspace *dataspace)
{
  const Datafile *datafile;
  Status s = dataspace->getCurrentDatafile(datafile);
  CHECK(s);
  cout << *datafile;
  return 0;
}

static int
dsp_realize(int start, int argc, char *argv[])
{
  if (argc - start < 1)
    return usage(prog);

  if (mode != mDspGetDefault && mode != mDspList && argc - start < 2)
    return usage(prog);

  const char *dbname = argv[start];
  const char *dspname = argv[start+1];

  Database *db = new Database(dbname);
  Status s = db->open(conn, (mode == mDspList ? Database::DBSRead :
			     Database::DBRW));
  CHECK(s);

  if (mode != mDspList) {
    s = db->transactionBeginExclusive();
    CHECK(s);
  }

  const Dataspace *dataspace = 0;
  if (mode != mDspGetDefault && mode != mDspCreate && mode != mDspList) {
    s = db->getDataspace(dspname, dataspace);
    CHECK(s);
  }

  switch(mode)
    {
    case mDspCreate:
      return dspcreate_realize(db, dspname, &argv[start+2], argc-start-2);

    case mDspUpdate:
      return dspupdate_realize(db, dataspace, &argv[start+2], argc-start-2);

    case mDspDelete:
      if (argc - start != 2) return usage(prog);
      return dspdelete_realize(dataspace);

    case mDspRename:
      if (argc - start != 3) return usage(prog);
      return dsprename_realize(dataspace, argv[start+2]);

    case mDspList:
      return dsplist_realize(db, &argv[start+1], argc-start-1);

    case mDspSetDefault:
      if (argc - start != 2) return usage(prog);
      return dspsetdefault_realize(db, dataspace);

    case mDspGetDefault:
      if (argc - start != 1) return usage(prog);
      return dspgetdefault_realize(db);

    case mDspSetCurDat:
      if (argc - start != 3) return usage(prog);
      return dspsetcurdat_realize(db, dataspace, argv[start+2]);

    case mDspGetCurDat:
      if (argc - start != 2) return usage(prog);
      return dspgetcurdat_realize(db, dataspace);

    default:
      abort();
    }

  return 1;
}

static int
get_ext_opts(int s, int argc, char *argv[],
	     int &list, string &filedir, string &mthdir,
	     char *&dbname, char *&extra1, char *&extra2)
{
  list = 0;
  filedir = "";
  mthdir = "";
  dbname = 0;
  extra1 = extra2 = 0;

  for (int i = s; i < argc; )
    {
      char *s = argv[i];
      string value;

      if (!strcmp(s, "-l")) {
	list = 1;
	i++;
      }
      else if (!strcmp(s, "-d")) {
	if (i+1 >= argc)
	  return usage(prog);

	if (dbname)
	  return usage(prog);
	
	dbname = argv[i+1];
	i += 2;
      }
      else if (GetOpt::parseLongOpt(s, "filedir", &value)) {
	filedir = value;
	i++;
      }
      else if (GetOpt::parseLongOpt(s, "mthdir", &value)) {
	mthdir = value;
	i++;
      }
      else if (!extra1) {
	extra1 = argv[i];
	i++;
      }
      else if (!extra2) {
	extra2 = argv[i];
	i++;
      }
      else
	return usage(prog);
    }

  return 0;
}

static int
ext_realize(int start, int argc, char *argv[])
{
  char *extra1, *extra2, *dbname;
  string filedir, mthdir;
  int list;
  if (get_ext_opts(start, argc, argv, list, filedir, mthdir, dbname, extra1, extra2))
    return 1;

  switch(mode)
    {
    case mDbExport:
      if (!extra1 || !extra2 || dbname || list || filedir.length() || mthdir.length())
	return usage(prog);

      auth_realize();

      return dbexport_realize(extra1, extra2);

    case mDbImport:
      if (!extra1 || extra2)
	return usage(prog);

      if (list)
	return dbimport_realize(extra1, list);

      auth_realize();
      return dbimport_realize(extra1, list, dbname, filedir.c_str(), mthdir.c_str());
    }

  return 0;
}

#define check_argc_u(S, ARGC) if ((ARGC) > (S)) return usage(prog)
#define check_argc(S, ARGC)   ((S) <= (ARGC))
#define check_argc_x(S, ARGC) if ((S) != (ARGC)) return usage(prog)

static int
get_admin_opts(int s, int argc, char *argv[], char **username,
	       char **passwd, char **newpasswd, char **dbname,
	       char **accessmode, UserType &user_type)
{
  *username = argv[s++];

  if (!*username)
    return usage(prog);

  string value;
  switch(mode)
    {
    case mUserAdd:
      check_argc_u(s+1, argc);
      if (GetOpt::parseLongOpt(*username, "unix", &value)) {
	user_type = UnixUser;
	*username = strdup(value.c_str());
      }
      else if (GetOpt::parseLongOpt(*username, "strict-unix", &value)) {
	user_type = StrictUnixUser;
	*username = strdup(value.c_str());
      }
      else
	user_type = EyeDBUser;

      if (**username == '-') {
	print_prog();
	fprintf(stderr, "a user name cannot start with a '-': %s\n",
		*username);
	return 1;
      }

      if (user_type == StrictUnixUser)
	*passwd = "";
      else if (check_argc(s+1, argc))
	*passwd = argv[s];
      return 0;

    case mUserDelete:
      check_argc_x(s, argc);
      return 0;

    case mUserPasswdSet:
      check_argc_u(s+1, argc);
      if (check_argc(s+1, argc))
	*newpasswd = argv[s++];
      return 0;

    case mPasswdSet:
      check_argc_u(s+2, argc);
      if (check_argc(s+1, argc))
	*passwd = argv[s++];
      if (check_argc(s+2, argc))
	*newpasswd = argv[s++];
      return 0;

    case mUserDbAccessSet:
      check_argc_x(s+2, argc);
      *dbname = argv[s++];
      *accessmode = argv[s++];
      return 0;

    case mUserSysAccessSet:
      check_argc_x(s+1, argc);
      *accessmode = argv[s++];
      return 0;

    default:
      abort();
      return 1;
    }
}

#define CHECK_RETURN(X) \
{ \
  Status s = (X); \
  CHECK(s); \
  return 0; \
}

static int
useradd_realize(const char *username, const char *passwd, UserType user_type)
{
  CHECK_RETURN(dbmdatabase->addUser(conn, username, passwd, user_type,
				    userauth, passwdauth));
}

static int
userdelete_realize(const char *username)
{
  CHECK_RETURN(dbmdatabase->deleteUser(conn, username, userauth, passwdauth));
}

static int
userpasswdset_realize(const char *username, const char *newpasswd)
{
  CHECK_RETURN(dbmdatabase->setUserPasswd(conn, username, newpasswd, userauth, passwdauth));
}

static int
passwdset_realize(const char *username, const char *passwd,
		  const char *newpasswd)
{
  CHECK_RETURN(dbmdatabase->setPasswd(conn, username, passwd, newpasswd));
}

static int
userdbaccessset_realize(const char *username, const char *dbname,
			const char *accessmode)
{
  DBAccessMode dbmode;

  if (get_dbaccess(accessmode, dbmode))
    return usage(prog);

  Database *db = new Database(dbname, dbmdb);
  CHECK_RETURN(db->setUserDBAccess(conn, username, dbmode, userauth, passwdauth));
}

static int
usersysaccessset_realize(const char *username, const char *dbname,
			 const char *accessmode)
{
  int sysmode = 0;
  char *p = strdup(accessmode);
  
  for (;;)
    {
      char *q = strchr(p, '+');
      if (q)
	*q = 0;

      if (!strcmp(p, "dbcreate"))
	sysmode |= DBCreateSysAccessMode;
      else if (!strcmp(p, "adduser"))
	sysmode |= AddUserSysAccessMode;
      else if (!strcmp(p, "deleteuser"))
	sysmode |= DeleteUserSysAccessMode;
      else if (!strcmp(p, "setuserpasswd"))
	sysmode |= SetUserPasswdSysAccessMode;
      else if (!strcmp(p, "superuser"))
	sysmode |= SuperUserSysAccessMode;
      else if (!strcmp(p, "admin"))
	sysmode |= AdminSysAccessMode;
      else if (!strcmp(p, "no"))
	sysmode = NoSysAccessMode;
      else
	return usage(prog);

      if (!q)
	break;
      p = q+1;
    }

  CHECK_RETURN(dbmdatabase->setUserSysAccess(conn, username,
					     (SysAccessMode)sysmode,
					     userauth, passwdauth));
}

static int
admin_realize(int start, int argc, char *argv[])
{
  char *username = 0;
  char *passwd = 0;
  char *newpasswd = 0;
  char *dbname = 0;
  char *accessmode = 0;
  UserType user_type;

  if (get_admin_opts(start, argc, argv, &username, &passwd, &newpasswd,
		     &dbname, &accessmode, user_type))
    return 1;

  std::string user = Connection::makeUser(username);
  username = strdup(user.c_str());
  /*
  if (!strcmp(username, "@")) {
    struct passwd *pwd = getpwuid(getuid());
    if (!pwd)
      return usage(prog);
    username = strdup(pwd->pw_name);
  }
  */

  if (!*username) {
    fprintf(stderr, "%s: a username cannot be an empty string\n", argv[0]);
    return 1;
  }

  dbmdatabase = new DBM_Database();

  switch(mode)
    {
    case mUserAdd:
      if (!passwd && user_type != StrictUnixUser) {
	char buf[128];
	sprintf(buf, "%s password", username);
	passwd_realize(buf, &passwd);
      }
      auth_realize();
      return useradd_realize(username, passwd, user_type);

    case mUserDelete:
      auth_realize();
      return userdelete_realize(username);

    case mUserPasswdSet:
      if (!newpasswd)
	passwd_realize("user new password", &newpasswd);
      auth_realize();
      return userpasswdset_realize(username, newpasswd);

    case mPasswdSet:
      if (!passwd)
	passwd_realize("user old password", &passwd, 0);
      if (!newpasswd)
	passwd_realize("user new password", &newpasswd);
      auth_realize();
      return passwdset_realize(username, passwd, newpasswd);

    case mUserDbAccessSet:
      auth_realize();
      return userdbaccessset_realize(username, dbname, accessmode);

    case mUserSysAccessSet:
      auth_realize();
      return usersysaccessset_realize(username, dbname, accessmode);

    default:
      abort();
    }

  return 1;
}

#define XC(M, SM, XM, STR, C) \
if ( ((M) & (XM)) == (M) ) \
{ \
  strcat(STR, C); \
  strcat(STR, SM); \
  C = " | "; \
}

static const char *
str_sys_mode(SysAccessMode sysmode)
{
  static char sysstr[512];

  if (sysmode == NoSysAccessMode)
    return "NO_SYSACCESS_MODE";

  if (sysmode == AdminSysAccessMode)
    return "ADMIN_SYSACCESS_MODE";

  if (sysmode == SuperUserSysAccessMode)
    return "SUPERUSER_SYSACCESS_MODE";

  char *concat;
  *sysstr = 0;

  concat = "";

  XC(DBCreateSysAccessMode, "DB_CREATE_SYSACCESS_MODE",
     sysmode, sysstr, concat);
  XC(AddUserSysAccessMode, "ADD_USER_SYSACCESS_MODE",
     sysmode, sysstr, concat);
  XC(DeleteUserSysAccessMode, "DELETE_USER_SYSACCESS_MODE",
     sysmode, sysstr, concat);
  XC(SetUserPasswdSysAccessMode, "SET_USER_PASSWD_SYSACCESS_MODE",
     sysmode, sysstr, concat);

  return sysstr;
}

static const char *
str_user_mode(DBAccessMode usermode)
{
  static char userstr[512];

  if (usermode == NoDBAccessMode)
    return "NO_DBACCESS_MODE";

  if (usermode == AdminDBAccessMode)
    return "ADMIN_DBACCESS_MODE";

  char *concat;
  *userstr = 0;

  concat = "";

  XC(ReadWriteExecDBAccessMode, "READ_WRITE_EXEC_DBACCESS_MODE",
     usermode, userstr, concat)
  else XC(ReadExecDBAccessMode, "READ_EXEC_DBACCESS_MODE",
	  usermode, userstr, concat)
  else XC(ReadWriteDBAccessMode, "READ_WRITE_DBACCESS_MODE",
	  usermode, userstr, concat)
  else XC(ReadDBAccessMode, "READ_DBACCESS_MODE",
	  usermode, userstr, concat);

  return userstr;
}

static SysAccessMode
get_sys_access(DBM_Database *dbm, const char *username)
{
  SysAccessMode mode = NoSysAccessMode;
  Status status = Success;

  dbm->transactionBegin();

  OQL q(dbm, "select system_user_access->user->name = \"%s\"", username);

  ObjectArray obj_arr;
  status = q.execute(obj_arr);
  
  if (!status && obj_arr.getCount() > 0)
    mode = ((SysUserAccess *)obj_arr[0])->mode();

  dbm->transactionCommit();

  obj_arr.garbage();
  if (status) {
    print_prog();
    status->print();
    exit(1);
  }

  return mode;
}

static int
get_db_access(DBM_Database *dbm, const char *name, const char *fieldname,
	      DBUserAccess **dbaccess)
{
  Status status = Success;
  int cnt;
  
  cnt = 0;

  dbm->transactionBegin();

  OQL q(dbm, "select database_user_access->%s = \"%s\"", fieldname, name);

  ObjectArray obj_arr;
  status = q.execute(obj_arr);
  CHECK(status);

  if (!status)
    for (int i = 0; i < obj_arr.getCount(); i++)
      dbaccess[cnt++] = (DBUserAccess *)obj_arr[i];

 out:
  dbm->transactionCommit();
  CHECK(status);
  return cnt;
}

static int
get_db_access_user(DBM_Database *dbm, const char *username,
		   DBUserAccess **dbaccess)
{
  return get_db_access(dbm, username, "user->name", dbaccess);
}

static int
get_db_access_db(DBM_Database *dbm, const char *dbname,
		 DBUserAccess **dbaccess)
{
  return get_db_access(dbm, dbname, "dbentry->dbname", dbaccess);
}


static void
print_user(DBM_Database *dbm, UserEntry *user)
{
  printf("name      : \"%s\"", user->name().c_str());
  if (user->type() == UnixUser)
    printf(" [unix user]");
  else if (user->type() == StrictUnixUser)
    printf(" [strict unix user]");
  printf("\n");
  printf("sysaccess : %s\n", str_sys_mode(get_sys_access(dbm, user->name().c_str())));
  DBUserAccess *dbaccess[128];
  int cnt = get_db_access_user(dbm, user->name().c_str(), dbaccess);
  if (cnt > 0)
    {
      printf("dbaccess  : ");
      for (int i = 0, n = 0; i < cnt; i++)
	{
	  if (dbaccess[i]->dbentry() && dbaccess[i]->dbentry()->dbname().c_str())
	    {
	      if (n)
		printf("            ");
	      printf("(dbname : \"%s\", access : %s)\n",
		     dbaccess[i]->dbentry()->dbname().c_str(),
		     str_user_mode(dbaccess[i]->mode()));
	      n++;
	    }
	}
    }
}

static int
userlist_realize(int n, char *str[], DBM_Database *dbm)
{
  int i;

  if (!n) {
    dbm->transactionBegin();
    OQL q(dbm, "select user_entry");
    
    ObjectArray obj_arr;
    Status s = q.execute(obj_arr);
    CHECK(s);
    
    for (i = 0; i < obj_arr.getCount(); i++) {
      if (i)
	printf("\n");
      print_user(dbm, (UserEntry *)obj_arr[i]);
    }
    
    obj_arr.garbage();
    dbm->transactionCommit();
    return 0;
  }
  
  int error = 0;
  dbm->transactionBegin();
  for (i = 0; i < n; i++) {
    UserEntry *user;
    Status s = dbm->getUser(str[i], user);
    CHECK(s);
    
    if (!user) {
      print_prog();
      fprintf(stderr, "user '%s' not found\n", str[i]);
      error++;
    }
    else {
      if (i)
	printf("\n");
      print_user(dbm, user);
    }
    
    if (user)
      user->release();
  }
  
  dbm->transactionCommit();

  return error;
}

static char list_indent[] = "  ";

static DbInfoDescription *
get_dbinfo(DBEntry *dbentry, Database *&db)
{
  db = new Database(dbentry->dbname().c_str(), dbmdb);

  DbInfoDescription *dbdesc = new DbInfoDescription();
  Status status = db->getInfo(conn, 0, 0, dbdesc);

  if (status) {
    delete dbdesc;
    status->print(stdout);
    return 0;
  }

  return dbdesc;
}

static void
print_datafiles(DBEntry *dbentry, Bool datafiles,
		DbInfoDescription *dbdesc)
{
  int dbid;
  static char indent[] = "            ";
  if (!dbdesc) {
    Database *db;
    dbdesc = get_dbinfo(dbentry, db);
    if (!dbdesc) return;
  }

  if (!datafiles)
    printf("Datafiles\n");

  const eyedbsm::DbCreateDescription *s = &dbdesc->sedbdesc;

  for (int i = 0; i < s->ndat; i++)
    {
      if (datafiles)
	{
	  printf("%s\n", s->dat[i].file);
	  continue;
	}

      printf("%sDatafile #%d\n", list_indent, i);
      if (*s->dat[i].name)
	printf("%s  Name      %s\n", list_indent, s->dat[i].name);
      if (s->dat[i].dspid >= 0)
	printf("%s  Dataspace #%d\n", list_indent, s->dat[i].dspid);
      printf("%s  File      %s\n", list_indent, s->dat[i].file);
      printf("%s  Maxsize   ~%dMb\n", list_indent, s->dat[i].maxsize/1024);
      printf("%s  Slotsize  %db\n", list_indent, s->dat[i].sizeslot);
      printf("%s  Server Access ", list_indent);
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

  /*
    if (!datafiles)
    printf("\n");
  */
}

enum {
  DBNAME = 0x1,
  DBID = 0x2,
  DBFILE = 0x4,
  OBJCNT = 0x8,
  DATAFILES = 0x10,
  DBACCESS = 0x20,
  USERDBACCESS = 0x40,
  STATS = 0x80,
  ALL = 0x100
};

static int
print_dbentry(DBEntry *dbentry, DBM_Database *dbm, unsigned int options)
{
  DbInfoDescription *dbdesc = 0;
  Database *db = 0;

  if ((options & ALL) || (options & DATAFILES) || (options & OBJCNT) ||
      (options & STATS)) {
    dbdesc = get_dbinfo(dbentry, db);
    if (!dbdesc) return 1;
  }
  else
    dbdesc = 0;

  if (options & ALL)
    {
      printf("Database Name\n%s%s\n", list_indent, dbentry->dbname().c_str());
      printf("Database Identifier\n%s%d\n", list_indent, dbentry->dbid());
      printf("Database File\n%s%s\n", list_indent, dbentry->dbfile().c_str());
      printf("Max Object Count\n%s%d\n", list_indent, dbdesc->sedbdesc.nbobjs);
    }
  else
    {
      if (options & DBNAME)
	printf("%s\n", dbentry->dbname().c_str());

      if (options & DBID)
	printf("%d\n", dbentry->dbid());

      if (options & DBFILE)
	printf("%s\n", dbentry->dbfile().c_str());

      if (options & OBJCNT)
	printf("%llu\n", dbdesc->sedbdesc.nbobjs);
    }

  if ((options & ALL) || (options & DATAFILES))
    print_datafiles(dbentry, IDBBOOL(options & DATAFILES), dbdesc);

  if (options & DBACCESS)
    printf("%s\n", str_user_mode(dbentry->default_access()));
  else if (options & ALL)
    printf("Default Access\n%s%s\n", list_indent, str_user_mode(dbentry->default_access()));

  DBUserAccess *dbaccess[128];
  int cnt = get_db_access_db(dbm, dbentry->dbname().c_str(), dbaccess);
  if (cnt > 0)
    {
      if (options & ALL)
	printf("Database Access\n");
      for (int i = 0; i < cnt; i++)
	if (options & USERDBACCESS)
	  printf("%s %s\n",
		 dbaccess[i]->user()->name().c_str(),
		 str_user_mode(dbaccess[i]->mode()));
	else if (options & ALL)
	  printf("%sUser Name  %s\n%sAccess Mode %s\n",
		 list_indent,
		 dbaccess[i]->user()->name().c_str(),
		 list_indent,
		 str_user_mode(dbaccess[i]->mode()));
    }

  if (options & STATS) {
    const Datafile **datafiles;
    unsigned int cnt;
    Status s = db->open(conn, Database::DBSRead);
    CHECK(s);
    s = db->getDatafiles(datafiles, cnt);
    CHECK(s);
    DatlistStats dstats(dbdesc);
    for (int i = 0, n = 0; i < cnt; i++)
      if (dstats.add(datafiles[i])) return 1;
    dstats.display();
  }

  return 0;
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

      if (s[1] == '=') {
	offset = 2;
	return "!=";
      }
    }

  if (s[0] == '=')
    {
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

static int
dblist_realize(int n, char *str[], DBM_Database *dbm)
{
  int i, j;

  int error = 0;
  dbm->transactionBegin();
  unsigned int options = 0;

  for (i = 0, j = 0; i < n; i++)
    {
      const char *s = str[i];
      if (*s == '-') {
	if (!strcmp(s, "--dbname"))
	  options |= DBNAME;
	else if (!strcmp(s, "--dbid"))
	  options |= DBID;
	else if (!strcmp(s, "--object-count"))
	  options |= OBJCNT;
	else if (!strcmp(s, "--dbfile"))
	  options |= DBFILE;
	else if (!strcmp(s, "--datafiles"))
	  options |= DATAFILES;
	else if (!strcmp(s, "--dbaccess"))
	  options |= DBACCESS;
	else if (!strcmp(s, "--userdbaccess"))
	  options |= USERDBACCESS;
	else if (!strcmp(s, "--all"))
	  options |= ALL;
	else if (!strcmp(s, "--stats"))
	  options |= STATS;
	else
	  return usage(prog);
	continue;
      }

      for (int k = i+1; k < n; k++)
	if (*str[k] == '-') {
	  if (!strcmp(str[k], "--stats"))
	    options |= STATS;
	  else
	    return usage(prog);
	}

      if (!options)
	options = ALL;

      DBEntry **dbentries;
      int cnt, offset;
      const char *op = get_op(s, offset);

      Status st = dbm->getDBEntries(s+offset, dbentries, cnt, op);
      CHECK(st);

      if (!cnt)
	{
	  print_prog();
	  fprintf(stderr, "database '%s' not found\n", str[i]);
	  error++;
	}
      else
	{
	  for (int i = 0; i < cnt; i++)
	    {
	      if (j && (!options || options & ALL))
		printf("\n");
	      if (print_dbentry(dbentries[i], dbm, options)) return 1;
	      dbentries[i]->release();
	    }
      
	}

      delete [] dbentries;
      j++;
    }

  if (!j)
    {
      OQL q(dbm, "select database_entry");

      if (!options)
	options = ALL;

      ObjectArray obj_arr;
      Status s = q.execute(obj_arr);
      CHECK(s);

      for (i = 0; i < obj_arr.getCount(); i++)
	{
	  if (i && (!options || options & ALL))
	    printf("\n");
	  if (print_dbentry((DBEntry *)obj_arr[i], dbm, options)) return 1;
	}

      obj_arr.garbage();
      //dbm->transactionCommit();
      return 0;
    }
  
  dbm->transactionCommit();
  return error;
}

static int
list_realize(int start, int argc, char *argv[])
{
  int n;

  auth_realize();

  DBM_Database *dbm = new DBM_Database(dbmdb);

  Status status = dbm->open(conn, Database::DBSRead,
			    userauth, passwdauth);

  CHECK(status);
  switch(mode)
    {
    case mUserList:
      return userlist_realize(argc-start, &argv[start], dbm);

    case mDbList:
      return dblist_realize(argc-start, &argv[start], dbm);

    default:
      abort();
    }
}

int
main(int argc, char *argv[])
{
  char *p;
  int start;

  prog = ((p = strrchr(argv[0], '/')) ? p+1 : argv[0]);

  eyedb::init();

  if (get_mode(argc, argv, start))
    return 1;

  eyedb::init(argc, argv);

  if (start < argc && (!strcmp(argv[start], "--help") ||
		       !strcmp(argv[start], "-h"))) {
    usage(prog);
    return 0;
  }
      
  dbmdb = Database::getDefaultDBMDB();

  conn = new Connection();
  if (!getenv("EYEDBLOCAL")) {
    Status status = conn->open();
    CHECK(status);
  }

  if ((int)mode < (int)mDB_UP)
    return db_realize(start, argc, argv);
  
  if ((int)mode < (int)mDAT_UP)
    return dat_realize(start, argc, argv);
  
  if ((int)mode < (int)mDSP_UP)
    return dsp_realize(start, argc, argv);
  
  if ((int)mode < (int)mEXT_UP)
    return ext_realize(start, argc, argv);
  
  if ((int)mode < (int)mADMIN_UP)
    return admin_realize(start, argc, argv);

  return list_realize(start, argc, argv);
}

/*
  #ifndef _REENTRANT
  #include <pthread.h>
  pthread_t pthread_self() {return 0;}

  int	pthread_create(pthread_t *thread, const pthread_attr_t *attr,
  void * (*start_routine)(void *),
  void *arg) {
  return 0;
  }
  #endif
*/
