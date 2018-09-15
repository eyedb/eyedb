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
#include "USRTopic.h"
#include <errno.h>

using namespace eyedb;
using namespace std;

USRTopic::USRTopic() : Topic("user")
{
  addAlias("usr");

  addCommand(new USRAddCmd(this));
  addCommand(new USRDeleteCmd(this));
  addCommand(new USRListCmd(this));
  addCommand(new USRSysAccessCmd(this));
  addCommand(new USRDBAccessCmd(this));
  addCommand(new USRPasswdCmd(this));
}


//
// Helper functions
//

#define N 4

static void
passwd_realize(const char *prompt, const char *&passwd, int retype = 1)
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

  passwd = p;
}

static void
auth_realize(char *userauth, char *passwdauth)
{
  const char *s;
  errno = 0;

  if (!(s = Connection::getDefaultUser()))
    *userauth = 0;
  else
    strcpy(userauth, s);

  if (!(s = Connection::getDefaultPasswd()))
    *passwdauth = 0;
  else
    strcpy(passwdauth, s);
}

//
// Commands
//

//
// eyedbadmin user add ...
//	    "%suseradd [--unix|--strict-unix] [<user>|@ [<passwd>]]\n", str());
//
static const string ADD_UNIX_OPT("unix");
static const string ADD_STRICT_UNIX_OPT("strict-unix");

void USRAddCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);
  opts.push_back(Option(ADD_UNIX_OPT,
			OptionBoolType(),
			0,
			OptionDesc("User is mapped on a Unix user: Unix and eyedb authentication mechanisms are supported")));
  opts.push_back(Option(ADD_STRICT_UNIX_OPT, 
			OptionBoolType(), 0,
			OptionDesc("User is mapped on a Unix user: only Unix authentication mechanism is supported")));
  getopt = new GetOpt(getExtName(), opts);
}

int USRAddCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " [USER [PASSWORD]]\n";
  return 1;
}

int USRAddCmd::help()
{
  getopt->adjustMaxLen("PASSWORD");
  stdhelp();
  getopt->displayOpt("USER", "User name");
  getopt->displayOpt("PASSWORD", "Password for specified user");
  return 1;
}

int USRAddCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  UserType user_type;

  if (map.find(ADD_UNIX_OPT) != map.end()) {
    user_type = UnixUser;
  } else if (map.find(ADD_STRICT_UNIX_OPT) != map.end()) {
    user_type = StrictUnixUser;
  } else
    user_type = EyeDBUser;

  const char *username = 0;
  const char *passwd = 0;

  if (argv.size() >= 1) {
    username = strdup(argv[0].c_str());

    if (argv.size() >= 2)
      passwd = strdup(argv[1].c_str());
  }

  if (!username && user_type == EyeDBUser)
    return help();

  if (!username)
    username = "@";

  std::string user = Connection::makeUser(username);
  username = strdup(user.c_str());

  if (!passwd && user_type != StrictUnixUser) {
    char buf[128];

    sprintf(buf, "%s password", username);
    passwd_realize(buf, passwd);
  }
  if (!passwd)
    passwd = "";

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  DBM_Database *dbmdatabase = new DBM_Database();

  conn.open();

  dbmdatabase->addUser(&conn, username, passwd, user_type, userauth, passwdauth);

  return 0;
}


//
// eyedbadmin user delete ...
//

void USRDeleteCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int USRDeleteCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " USER\n";
  return 1;
}

int USRDeleteCmd::help()
{
  stdhelp();
  getopt->displayOpt("USER", "User name");
  return 1;
}

int USRDeleteCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (argv.size() < 1) {
    return usage();
  }

  char *username = strdup(argv[0].c_str());

  std::string user = Connection::makeUser(username);
  username = strdup(user.c_str());

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  DBM_Database *dbmdatabase = new DBM_Database();

  conn.open();

  dbmdatabase->deleteUser(&conn, username, userauth, passwdauth);

  return 0;
}


//
// eyedbadmin user list ...
//

void USRListCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int USRListCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " [USER]\n";
  return 1;
}

int USRListCmd::help()
{
  stdhelp();
  getopt->displayOpt("USER", "User name");
  return 1;
}

#define XC(M, SM, XM, STR, C) \
if (((M) & (XM)) == (M) ) \
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

  const char *concat;
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

  const char *concat;
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
    std::cerr << PROGNAME;
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

  if (!status)
    for (int i = 0; i < obj_arr.getCount(); i++)
      dbaccess[cnt++] = (DBUserAccess *)obj_arr[i];

  dbm->transactionCommit();

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
list_all_users(DBM_Database *dbm)
{
  dbm->transactionBegin();

  OQL q(dbm, "select user_entry");
  ObjectArray obj_arr;

  q.execute(obj_arr);
    
  for (int i = 0; i < obj_arr.getCount(); i++) {
    if (i)
      printf("\n");
    print_user(dbm, (UserEntry *)obj_arr[i]);
  }
    
  obj_arr.garbage();

  dbm->transactionCommit();

  return 0;
}

static int
list_selected_users(DBM_Database *dbm, std::vector<std::string> &argv)
{
  int error = 0;

  dbm->transactionBegin();

  for (int i = 0; i < argv.size(); i++) {
    UserEntry *user;
    const char *username = argv[i].c_str();
    dbm->getUser(username, user);
    
    if (!user) {
      std::cerr << PROGNAME;
      std::cerr << ": user " << username << " not found\n";
      error = 1;
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

int USRListCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  DBM_Database *dbm = new DBM_Database();

  conn.open();

  dbm->open(&conn, Database::DBSRead, userauth, passwdauth);

  if (argv.size() < 1) {
    return list_all_users(dbm);
  }

  return list_selected_users(dbm, argv);
}


//
// eyedbadmin user sysaccess ...
//

void USRSysAccessCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int USRSysAccessCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " USER ['+' combination of] dbcreate|adduser|deleteuser|setuserpasswd|admin|superuser|no\n";
  return 1;
}

int USRSysAccessCmd::help()
{
  stdhelp();
  getopt->displayOpt("USER", "User name");
  getopt->displayOpt("MODE", "['+' combination of] dbcreate|adduser|deleteuser|setuserpasswd|admin|superuser|no");

  return 1;
}

int USRSysAccessCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (argv.size() < 2) {
    return usage();
  }

  const char *username = argv[0].c_str();
  int sysmode = 0;
  char *p = strdup(argv[1].c_str());
  
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
	return usage();

      if (!q)
	break;
      p = q+1;
    }

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  DBM_Database *dbmdatabase = new DBM_Database();

  conn.open();

  dbmdatabase->setUserSysAccess(&conn, username, (SysAccessMode)sysmode, userauth, passwdauth);

  return 0;
}


// 
// eyedbadmin user dbaccess ...
//

void USRDBAccessCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int USRDBAccessCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " USER DBNAME MODE\n";
  return 1;
}

int USRDBAccessCmd::help()
{
  getopt->adjustMaxLen("DBNAME");
  stdhelp();
  getopt->displayOpt("USER", "User name");
  getopt->displayOpt("DBNAME", "Database name");
  getopt->displayOpt("MODE", "r|rw|rx|rwx|admin|no");
  return 1;
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
userdbaccessset_realize(const char *username, const char *dbname,
			const char *accessmode)
{
  return 1;
}

int USRDBAccessCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (argv.size() < 3) {
    return usage();
  }

  const char *username = argv[0].c_str();
  const char *dbname = argv[1].c_str();

  DBAccessMode dbmode;

  if (get_dbaccess(argv[2].c_str(), dbmode))
    return usage();

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  Database *db = new Database(dbname, Database::getDefaultDBMDB());

  conn.open();

  db->setUserDBAccess( &conn, username, dbmode, userauth, passwdauth);

  return 0;
}

//
// eyedbadmin user passwd ...
//

void USRPasswdCmd::init()
{
  std::vector<Option> opts;

  opts.push_back(HELP_OPT);

  getopt = new GetOpt(getExtName(), opts);
}

int USRPasswdCmd::usage()
{
  getopt->usage("", "");
  std::cerr << " USER [PASSWORD] [NEWPASSWORD]\n";
  return 1;
}

int USRPasswdCmd::help()
{
  getopt->adjustMaxLen("NEWPASSWORD");
  stdhelp();
  getopt->displayOpt("USER", "User name");
  getopt->displayOpt("PASSWORD", "User password");
  getopt->displayOpt("NEWPASSWORD", "New user password");
  return 1;
}


int USRPasswdCmd::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (!getopt->parse(PROGNAME, argv)) {
    return usage();
  }

  GetOpt::Map &map = getopt->getMap();

  if (map.find("help") != map.end()) {
    return help();
  }

  if (argv.size() < 1) {
    return usage();
  }

  char *username = strdup(argv[0].c_str());
  const char *passwd = 0;
  const char *newpasswd = 0;

  if (argv.size() >= 2) {
    passwd = strdup(argv[1].c_str());

    if (argv.size() >= 3)
      newpasswd = strdup(argv[2].c_str());
  }

  if (!passwd)
    passwd_realize("user old password", passwd, 0);

  if (!newpasswd)
    passwd_realize("user new password", newpasswd);

  char userauth[32];
  char passwdauth[10];

  auth_realize(userauth, passwdauth);

  DBM_Database *dbmdatabase = new DBM_Database();

  conn.open();

  dbmdatabase->setPasswd(&conn, username, passwd, newpasswd);

  return 0;
}
