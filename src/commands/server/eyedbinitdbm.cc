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
  Author: Francois Dechelle <francois@dechelle.net>
*/

#include "eyedbconfig.h"
#include <eyedb/eyedb.h>
#include "eyedb/DBM_Database.h"

using namespace eyedb;

const std::string PROG_NAME = "eyedbinitdbm";

static const char DBS_EXT[] = ".dbs";
static const char DAT_EXT[] = ".dat";

static const int ONE_K = 1024;
static const int DEFAULT_DATSIZE = 2048;
static const int DEFAULT_DATSZSLOT = 16;
static const char DEFAULT_DATNAME[] = "DEFAULT";
static const unsigned int DEFAULT_DATOBJCNT = 10000000;

//    echo ==== Creating EYEDBDBM database
//    $bindir/eyedbadmin dbmcreate --strict-unix=@
//    echo ==== Setting EYEDBDBM database permissions
//    $bindir/eyedbadmin dbaccess EYEDBDBM r --user=@

static int
help()
{
  std::cerr << "Usage: " + PROG_NAME + "[--help]" << std::endl;
  std::cerr << "Create the EyeDB EYEDBDBM system database." << std::endl;
  std::cerr << std::endl;
  std::cerr << "This command must be run once after installing EyeDB" << std::endl;
  std::cerr << std::endl;
  std::cerr << "  --help    display this help and exit" << std::endl;
  std::cerr << std::endl;

  return 1;
}

static const char datext[] = ".dat";

static std::string
getDatafile(const char *dbfile)
{
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

static int
check_status( Status status)
{
  if (status) {
    std::cerr << PROG_NAME << ":";
    status->print();
    return 1;
  }

  return 0;
}

static int
initdbm( Connection *conn)
{
  std::string user = Connection::makeUser("@");
  char *username = strdup(user.c_str());
  char *passwd = (char *)strict_unix_user;
  char *passwdauth = "";

  DBM_Database *dbm = new DBM_Database( Database::getDefaultDBMDB());

  const char *dbname = DBM_Database::getDbName();
  const char *dbfile = strdup(Database::getDefaultServerDBMDB());

  DbCreateDescription dbdesc;

  strcpy( dbdesc.dbfile, dbfile);
  dbdesc.sedbdesc.dbid = 0;
  dbdesc.sedbdesc.nbobjs = DEFAULT_DATOBJCNT;
  dbdesc.sedbdesc.ndat = 1;

  std::string datfile = getDatafile(dbfile);
  strcpy( dbdesc.sedbdesc.dat[0].file, datfile.c_str());
  strcpy( dbdesc.sedbdesc.dat[0].name, DEFAULT_DATNAME);
  dbdesc.sedbdesc.dat[0].maxsize = DEFAULT_DATSIZE * ONE_K;
  dbdesc.sedbdesc.dat[0].mtype = eyedbsm::BitmapType;
  dbdesc.sedbdesc.dat[0].sizeslot = DEFAULT_DATSZSLOT;
  dbdesc.sedbdesc.dat[0].dtype = eyedbsm::LogicalOidType;
  dbdesc.sedbdesc.dat[0].dspid = 0;

  Status status = dbm->create(conn, passwdauth, username, passwd, &dbdesc);

  return check_status( status);
}

static int
dbmaccess( Connection *conn)
{
  char userauth[64];
  char passwdauth[64];

  strcpy( userauth, Connection::getDefaultUser());
  strcpy( passwdauth, Connection::getDefaultPasswd());

  Database *db = new Database( "EYEDBDBM", Database::getDefaultDBMDB());

  Status status = db->setDefaultDBAccess(conn, ReadDBAccessMode, userauth, passwdauth);

  return check_status(status);
}

int
main(int c_argc, char *c_argv[])
{
  eyedb::init(c_argc, c_argv);

  if (c_argc == 2 && !strcmp(c_argv[1], "--help"))
    return help();
  else if (c_argc != 1)
    return help();

  //  Exception::setMode(Exception::ExceptionMode);

  Connection *conn = new Connection();
  conn->open();

  if (initdbm( conn))
    return 1;

  if (dbmaccess( conn))
    return 1;

  return 0;
}
