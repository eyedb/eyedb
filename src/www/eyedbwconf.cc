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


#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <eyedb/eyedb.h>
#include "eyedbcgiP.h"
#include "eyedb/DBM_Database.h"

#define USER 0x1
#define NAME 0x2

#define SET  0x10
#define GET  0x20
#define CP   0x40
#define MV   0x80
#define RM   0x100
#define CHK  0x200
#define GETA 0x400
#define TMPL 0x800
#define EXP  0x1000

#define SETN  (SET|NAME)
#define SETU  (SET|USER)
#define SETNU (SET|USER|NAME)
#define GETN  (GET|NAME)
#define GETU  (GET|USER)
#define MVN   (MV|NAME)
#define MVU   (MV|USER)
#define CPN   (CP|NAME)
#define CPU   (CP|USER)
#define RMN   (RM|NAME)
#define RMU   (RM|USER)

static const char *prog;
static const char *dbname;
static unsigned int action;
static const char *conffile;
static const char *confhint;
static const char *confhint2;
static Connection *conn;
static Database *dbm;
static idbWDest dest;

#include "../eyedb/GetOpt.h"

//
// Utility Functions
//

static char *
get_arg(const char *value, unsigned int which)
{
  char *p = strdup(value);

  for (int n = 0; ; n++) {
    char *q = strchr(p, ':');
    if (q)
      *q = 0;

    if (n == which)
      return p;

    if (!q)
      break;

    p = q + 1;
  }

  return 0;
}

static int
getopts(int argc, char *argv[])
{
  const char *action_str = argv[1];

  string value;
  if (GetOpt::parseLongOpt(action_str, "get-name", &value))
    action = GETN;
  else if (GetOpt::parseLongOpt(action_str, "get-user", &value))
    action = GETU;
  else if (GetOpt::parseLongOpt(action_str, "get-all", &value))
    action = GETA;
  else if (GetOpt::parseLongOpt(action_str, "set-name", &value))
    action = SETN;
  else if (GetOpt::parseLongOpt(action_str, "set-user", &value))
    action = SETU;
  else if (GetOpt::parseLongOpt(action_str, "set-name-user", &value))
    action = SETNU;
  else if (GetOpt::parseLongOpt(action_str, "rm-name", &value))
    action = RMN;
  else if (GetOpt::parseLongOpt(action_str, "rm-user", &value))
    action = RMU;
  else if (GetOpt::parseLongOpt(action_str, "mv-name", &value))
    action = MVN;
  else if (GetOpt::parseLongOpt(action_str, "mv-user", &value))
    action = MVU;
  else if (GetOpt::parseLongOpt(action_str, "cp-name", &value))
    action = CPN;
  else if (GetOpt::parseLongOpt(action_str, "cp-user", &value))
    action = CPU;
  else if (GetOpt::parseLongOpt(action_str, "check"))
    action = CHK;
  else if (GetOpt::parseLongOpt(action_str, "template"))
    action = TMPL;
  else if (GetOpt::parseLongOpt(action_str, "expand"))
    action = EXP;
  else
    return 1;

  const char *arg = value.c_str();

  if (action == SETNU) {
    if (argc != 5)
      return 1;
    confhint = get_arg(arg, 0);
    if (!confhint)
      return 1;
    confhint2 = get_arg(arg, 1);
    if (!confhint2)
      return 1;
    conffile = argv[2];
    if (strcmp(argv[3], "-d"))
      return 1;
    dbname = argv[4];
    return 0;
  }

  if (action == GETA || action == TMPL) {
    if (argc != 4)
      return 1;
    if (strcmp(argv[2], "-d"))
      return 1;
    dbname = argv[3];
    return 0;
  }

  if ((action & SET) == SET) {
    if (argc != 5)
      return 1;
    confhint = get_arg(arg, 0);
    if (!confhint)
      return 1;
    conffile = argv[2];
    if (strcmp(argv[3], "-d"))
      return 1;
    dbname = argv[4];
    return 0;
  }

  if (action == EXP) {
    if (argc != 5)
      return 1;
    conffile = argv[2];
    if (strcmp(argv[3], "-d"))
      return 1;
    dbname = argv[4];
    return 0;
  }

  if ((action & CP) == CP || (action & MV) == MV) {
    if (argc != 4)
      return 1;
    confhint = get_arg(arg, 0);
    if (!confhint)
      return 1;
    confhint2 = get_arg(arg, 1);
    if (!confhint2)
      return 1;
    if (strcmp(argv[2], "-d"))
      return 1;
    dbname = argv[3];
    return 0;
  }

  if ((action & GET) == GET || (action & RM) == RM) {
    if (argc != 4)
      return 1;
    confhint = get_arg(arg, 0);
    if (!confhint)
      return 1;
    if (strcmp(argv[2], "-d"))
      return 1;
    dbname = argv[3];
    return 0;
  }

  if (action == CHK) {
    if (argc != 3)
      return 1;
    conffile = argv[2];
    return 0;
  }
}

static int
usage()
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s --get-name=NAME|default -d DBNAME\n", prog);
  fprintf(stderr, "  %s --get-user=USER -d DBNAME\n", prog);
  fprintf(stderr, "  %s --get-all -d DBNAME\n\n", prog);

  fprintf(stderr, "  %s --set-name=NAME|default CONFFILE -d DBNAME\n", prog);
  fprintf(stderr, "  %s --set-user=USER CONFFILE -d DBNAME\n", prog);
  fprintf(stderr, "  %s --set-name-user=NAME|default:USER CONFFILE -d DBNAME\n\n", prog);

  fprintf(stderr, "  %s --rm-name=NAME|default -d DBNAME\n", prog);
  fprintf(stderr, "  %s --rm-user=USER -d DBNAME\n\n", prog);

  fprintf(stderr, "  %s --mv-name=NAME|default:NEWNAME|default -d DBNAME\n", prog);
  fprintf(stderr, "  %s --mv-user=USER:NEWUSER -d DBNAME\n\n", prog);

  fprintf(stderr, "  %s --cp-name=NAME|default:NEWNAME|default -d DBNAME\n", prog);
  fprintf(stderr, "  %s --cp-user=USER:NEWUSER -d DBNAME\n\n", prog);

  fprintf(stderr, "  %s --expand CONFFILE -d DBNAME\n", prog);

  fprintf(stderr, "  %s --check CONFFILE\n", prog);
  fprintf(stderr, "  %s --template -d DBNAME\n", prog);

  return 1;
}

static Bool
check_user(Database *db, const char *user)
{
  if (!dbm)
    {
      dbm = new DBM_Database(db->getDBMDB());
      dbm->open(conn, Database::DBSRead);
      dbm->transactionBegin();
    }

  OQL oql(dbm, "select user_entry.name = \"%s\"", user);

  OidArray oid_arr;
  oql.execute(oid_arr);

  if (!oid_arr.getCount())
    {
      fprintf(stderr, "%s: unknown EyeDB user '%s'\n", prog, user);
      return False;
    }

  return True;
}

static WConfig *
getconf(Database *db, const char *hint, Bool mustExist)
{
  const char *attr = ((action & NAME) ? "name" : "user");

  OQL oql(db, "select w_config.%s = \"%s\"", attr, hint);
  ObjectArray obj_arr;
  oql.execute(obj_arr);

  if (!obj_arr.getCount())
    {
      if (mustExist)
	fprintf(stderr, "%s: unknown config %s '%s' in database '%s'\n",
		prog, attr, hint, db->getName());
      return 0;
    }

  return (WConfig *)obj_arr[0];
}

static char *
get_file_contents(const char *file)
{
  int fd = open(file, O_RDONLY);

  assert(fd >= 0);

  struct stat st;
  if (fstat(fd, &st) < 0)
    {
      perror((std::string("fstat ") + file).c_str());
      close(fd);
      return 0;
    }

  char *buf = (char *)malloc(st.st_size+1);
  if (read(fd, buf, st.st_size) != st.st_size)
    {
      perror((std::string("read ") + file).c_str());
      close(fd);
      return 0;
    }

  buf[st.st_size] = 0;
  close(fd);
  return buf;
}

static int
check_file(const char *file)
{
  int fd = open(file, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "%s: cannot open file '%s' for reading\n",
	      file);
      return 1;
    }

  close(fd);

  Config config(file); // check config
  return 0;
}

//
// Action Functions
//

static int
getall_realize(Database *db)
{
  OQL oql(db, "select w_config");
  ObjectArray obj_arr;
  oql.execute(obj_arr);

  printf("%d Web Configuration(s) found in database '%s'\n",
	 obj_arr.getCount(), db->getName());

  for (int i = 0; i < obj_arr.getCount(); i++)
    {
      WConfig *config = (WConfig *)obj_arr[i];
      printf("\nName: '%s'\nUser: '%s'\n",
	     config->getName().c_str(), config->getUser().c_str());
    }

  return 0;
}

static int
get_realize(Database *db)
{
  WConfig *config = getconf(db, confhint, True);

  if (!config)
    return 1;

  printf("%s\n", config->getConf().c_str());
}

static int
set_realize(Database *db)
{
  if (check_file(conffile))
    return 1;

  WConfig *config = getconf(db, confhint, False);

  if (!config)
    {
      config = new WConfig(db);

      if ((action & NAME))
	config->setName(confhint);
      else if (!check_user(db, confhint))
	return 1;
      else
	config->setUser(confhint);

      if (action == SETNU)
	{
	  if (!check_user(db, confhint2))
	    return 1;
	  config->setUser(confhint2);
	}
    }

  char *buf = get_file_contents(conffile);
  config->setConf(buf);
  free(buf);

  config->store();
  config->release();

  return 0;
}

static int
mv_realize(Database *db)
{
  WConfig *config = getconf(db, confhint, True);

  if (!config)
    return 1;

  if ((action & NAME))
    config->setName(confhint2);
  else if (!check_user(db, confhint2))
    return 1;
  else
    config->setUser(confhint2);

  config->store();

  return 0;
}

static int
cp_realize(Database *db)
{
  WConfig *config = getconf(db, confhint, True);

  if (!config)
    return 1;

  WConfig *destconfig = getconf(db, confhint2, False);

  if (destconfig)
    {
      fprintf(stderr, "%s: config %s '%s' already exists in database '%s'\n",
	      prog, (action & NAME) ? "name" : "user", confhint2, db->getName());
      return 1;
    }

  destconfig = new WConfig(db);

  if ((action & NAME))
    destconfig->setName(confhint2);
  else if (!check_user(db, confhint2))
    return 1;
  else
    destconfig->setUser(confhint2);

  destconfig->setConf(config->getConf());
  destconfig->store();

  return 0;
}

static int
rm_realize(Database *db)
{
  WConfig *config = getconf(db, confhint, True);

  if (!config)
    return 1;

  config->remove();

  return 0;
}

static int
check_realize()
{
  return check_file(conffile);
}

static int
expand_realize(Database *db)
{
  int r = check_file(conffile);
  if (r) return r;
  idbWExpandConf(db, &dest, conffile);
  return 0;
}

static int
template_realize(Database *db)
{
  printf("\n#\n");
  printf("# Template EyeDB WEB Configuration for database '%s'\n",
	 db->getName());
  printf("#\n\n");
  fflush(stdout);
  idbWTemplateConf(db, &dest);
  return 0;
}

static int
realize(Database *db)
{
  if (action == GETA)
    return getall_realize(db);

  if ((action & GET) == GET)
    return get_realize(db);

  if ((action & SET) == SET)
    return set_realize(db);

  if ((action & CP) == CP)
    return cp_realize(db);

  if ((action & MV) == MV)
    return mv_realize(db);

  if ((action & RM) == RM)
    return rm_realize(db);
  
  if (action == CHK)
    return check_realize();

  if (action == TMPL)
    return template_realize(db);

  if (action == EXP)
    return expand_realize(db);

  abort();
}

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  prog = argv[0];
  if (argc < 2 || getopts(argc, argv))
    return usage();

  Exception::setMode(Exception::ExceptionMode);

  try {
    Database *db = 0;
    if (dbname)
      {
	conn = new Connection();
	conn->open();
	if (getenv("EYEDBWAIT"))
	  {
	    printf("go> ");
	    getchar();
	  }
	db = new Database(dbname);
	db->open(conn,
		 (((action & SETN) ||
		   (action & MV) ||
		   (action & CP) ||
		   (action & RM)) ? Database::DBRW : Database::DBSRead));
	db->transactionBegin();
      }

    if (realize(db))
      return 1;

    if (db) db->transactionCommit();
    return 0;
  }

  catch(Exception &e) {
    cerr << e;
    return 1;
  }

  return 0;
}
