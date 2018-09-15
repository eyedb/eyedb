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


#include <eyedb/eyedb.h>
#include "DBM_Database.h"

using namespace eyedb;
using namespace std;

#define HPROG "eyedbprotadmin"
#define PROG HPROG ": "

static int
usage()
{
  FILE *pipe = stderr;
  
  if (!pipe)
    pipe = stderr;

  fprintf(pipe,
	  "\nusage:\n"
	  "\n"
	  HPROG " <dbname> add <protname> <user> r|rw|no [<user> r|rw|no ...]\n"
	  HPROG " <dbname> update <protname> <user> r|rw|no [<user> r|rw|no ...]\n"
	  HPROG " <dbname> list [<protname>]\n"
	  HPROG " <dbname> setprot <query> <protname>\n"
	  HPROG " <dbname> getprot <query>\n"
	  "\n\n"
	  "Where :\n"
	  "  <dbname> is the database name.\n"
	  "  <protname> is the protection name or the protection oid (can be NULL).\n"
	  "  <user> is an user name\n"
	  "  r means read only access\n"
	  "  rw means read/write access\n"
	  "  no means no access\n"
	  "  <query> is an EyeDB OQL query\n"
	  "\n"
	  "\n"
	  "The key words: add, update, list, setprot and getprot are case insensitive.\n"
	  "\n"
	  "\n"
	  "For instance:\n"
	  "\n"
	  "  %% eyedbprotadmin    #displays the current window\n"
	  "\n"
	  "  %% eyedbprotadmin foo add myprot john r henry rw\n"
	  "\n"
	  "  %% eyedbprotadmin foo update myprot john no henry r gaston rw\n"
	  "\n"
	  "  %% eyedbprotadmin foo list myprot\n"
	  "\n"
	  "  %% eyedbprotadmin foo setprot \"select Person\" myprot\n"
	  "\n"
	  "  %% eyedbprotadmin foo getprot \"select Person.name = \\\"johnny\\\"\"\n"
	  "\n");
	  
  fflush(pipe);

  print_use_help(cerr);

  if (pipe != stderr)
    pclose(pipe);
  return 1;
}
  
//
// Options Hints
//

enum optsAction {
  NOACTION,
  ADD,
  UPDATE,
  LIST,
  SETPROT,
  GETPROT
};

static optsAction   action = NOACTION;

#define MAXETC 128
static char *       dbname = NULL;
static char *       protname = NULL;
static char *       query = NULL;
static int          startetc;
static char *       etcstr[MAXETC];

static Database *db  = NULL;
static Database *dbm = NULL;
static Status    status;

//
// options macros
//

#define opts_setaction(ACT) \
  if (!dbname || protname || action != NOACTION) return usage(); action = (ACT)

static void
capstring(char *str)
{
  char c;
  while (c = *str)
    {
      if (c >= 'a' && c <= 'z')
	*str = c + 'A' - 'a';
      str++;
    }
}

static int
getopts(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
    {
      char *s = strdup(argv[i]);
      capstring(s);
      if (!strcmp(s, "ADD"))
	{
	  opts_setaction(ADD);
	}
      else if (!strcmp(s, "UPDATE"))
	{
	  opts_setaction(UPDATE);
	}
      else if (!strcmp(s, "LIST"))
	{
	  opts_setaction(LIST);
	}
      else if (!strcmp(s, "SETPROT"))
	{
	  opts_setaction(SETPROT);
	}
      else if (!strcmp(s, "GETPROT"))
	{
	  opts_setaction(GETPROT);
	}
      else if (!dbname)
	dbname = argv[i];
      else if ((action == ADD || action == UPDATE) && !protname)
	protname = argv[i];
      else if ((action == SETPROT || action == GETPROT) && !query)
	query = argv[i];
      else if (startetc < MAXETC)
	etcstr[startetc++] = argv[i];
      else
	return usage();
      free(s);
    }

  if (!dbname || action == NOACTION)
    return usage();
    
  return 0;
}

UserEntry *
findUser(const char *user_str)
{
  OQL q(dbm, "select User.name = \"%s\"", user_str);

  ObjectArray obj_arr;
  Status s = q.execute(obj_arr);

  if (s)
    {
      fprintf(stderr, PROG);
      s->print();
      return NULL;
    }

  if (!obj_arr.getCount())
    fprintf(stderr, PROG "user '%s' not found\n", user_str);
  return (UserEntry *)obj_arr[0];
}

int
findMode(const char *mode_str)
{
  if (!strcmp(mode_str, "r") || !strcmp(mode_str, "R"))
    return ProtRead;

  if (!strcmp(mode_str, "rw") || !strcmp(mode_str, "RW"))
    return ProtRW;

  if (!strcmp(mode_str, "no") || !strcmp(mode_str, "NO"))
    return 0;

  fprintf(stderr, PROG "invalid access mode '%s'\n", mode_str);
  return -1;
}


Protection *
makeProtection(Protection *xprot = NULL)
{
  Protection *prot;
  int err = 0;

  prot = (xprot ? xprot : new Protection(db));

  prot->setName(protname);

  prot->setPusersCount(startetc/2);
  for (int i = 0; i < startetc; i += 2)
    {
      const char *user_str = etcstr[i];
      const char *mode_str = etcstr[i+1];
      UserEntry *user;
      int mode;

      if (!(user = findUser(user_str)))
	err++;

      if ((mode = findMode(mode_str)) < 0)
	err++;
      
      if (!err)
	{
	  ProtectionUser *puser = prot->getPusers(i/2);
	  puser->setUser(user);
	  puser->setMode((ProtectionMode)mode);
	}
    }

  if (!err)
    return prot;

  return NULL;
}

int
findProtection(Protection *&prot, const char *pname, Bool msg = True)
{
  prot = NULL;

  if (!strcmp(pname, "NULL"))
    return 0;

  Oid poid(pname);
  if (poid.isValid())
    {
      Status s = db->loadObject(&poid, (Object **)&prot);
      if (s)
	{
	  fprintf(stderr, PROG);
	  s->print();
	  return 1;
	}
      return 0;
    }

  OQL q(db, "select protection.name = \"%s\"", pname);

  ObjectArray obj_arr;
  Status s = q.execute(obj_arr);

  if (s)
    {
      fprintf(stderr, PROG);
      s->print();
      return 1;
    }

  if (!obj_arr.getCount())
    {
      if (msg)
	fprintf(stderr,
		PROG "protection '%s' does not exist in database '%s'\n",
		pname, dbname);
      return 1;
    }

  return 0;
}

static void
traceProtection(Protection *prot, Bool head = True);

static int
addRealize()
{
  if (!protname || !startetc || (startetc & 1))
    return usage();

  Protection *prot;

  if (!findProtection(prot, protname, False))
    {
      fprintf(stderr, PROG "protection '%s' already exists in database '%s'\n",
	      protname, dbname);
      return 1;
    }

  prot = makeProtection();

  if (!prot)
    return 1;

  prot->realize();

  printf("\nThe following protection has been added");
  traceProtection(prot, False);
  printf("\n");
  return 0;
}

static int
updateRealize()
{
  if (!protname || !startetc || (startetc & 1))
    return usage();

  Protection *prot;

  if (findProtection(prot, protname))
    return 1;

  prot = makeProtection(prot);

  if (!prot)
    return 1;

  prot->realize();
  traceProtection(prot);
  return 0;
}

static void
traceProtection(Protection *prot, Bool head)
{
  if (!prot)
    {
      printf("NULL\n");
      return;
    }
    
  printf("%s :\n", (head ? "protection" : ""));
  printf("\toid     : %s\n", prot->getOid().getString());
  printf("\tname    : \"%s\"\n", prot->getName().c_str());

  int cnt = prot->getPusersCount();

  for (int i = 0; i < cnt; i++)
    {
      ProtectionUser *puser = prot->getPusers(i);
      const char *mode_str;
      int mode = puser->getMode();

      if (mode == ProtRead)
	mode_str = "r";
      else if (mode == ProtRW)
	mode_str = "rw";
      else
	mode_str = "no";

      printf("\tuser[%d] : %s (mode : %s)\n", i,
	     ((UserEntry *)puser->getUser())->name().c_str(), mode_str);
    }

  if (!cnt)
    printf("\n");
}

static int
listAll()
{
  OQL q(db, "select protection");

  ObjectArray obj_array;
  Status s = q.execute(obj_array);

  if (s)
    {
      fprintf(stderr, PROG);
      s->print();
      return 1;
    }

  for (int i = 0; i < obj_array.getCount(); i++)
    traceProtection((Protection *)obj_array[i]);

  return 0;
}

static int
listOne(const char *prot_str)
{
  Protection *prot;

  if (findProtection(prot, prot_str))
    return 1;

  traceProtection(prot);
  return 0;
}

static int
listRealize()
{
  if (!startetc)
    return listAll();

  for (int i = 0; i < startetc; i++)
    listOne(etcstr[i]);

  return 0;
}

static int
setprotRealize()
{
  if (!query || startetc != 1)
    return usage();

  Protection *prot;

  if (findProtection(prot, etcstr[0]))
    return 1;

  Oid prot_oid;
  if (prot)
    prot_oid = prot->getOid();
    
  OQL q(db, query);

  ObjectArray obj_array;
  Status s = q.execute(obj_array);
  if (s)
    {
      fprintf(stderr, PROG);
      s->print();
      return 1;
    }

  if (!obj_array.getCount())
    {
      printf(PROG "no objects found\n");
      return 0;
    }

  if (prot)
    {
      printf("\nThe ");
      traceProtection(prot, True);
      printf("\nHas been set to the following objects :\n\n");
    }
  else
    printf("\nThe following objects have no more protection :\n\n");

  for (int i = 0; i < obj_array.getCount(); i++)
    {
      Status s = const_cast<Object *>(obj_array[i])->setProtection(prot_oid);
      if (s)
	s->print();
      printf("\t%s\n", obj_array[i]->getOid().getString());
    }

  printf("\n");
  return 0;
}

static int
getprotRealize()
{
  if (!query || startetc)
    return usage();

  OQL q(db, query);

  ObjectArray obj_array;
  Status s = q.execute(obj_array);
  if (s)
    {
      fprintf(stderr, PROG);
      s->print();
      return 1;
    }

  if (!obj_array.getCount())
    {
      printf(PROG "no objects found\n");
      return 0;
    }

  for (int i = 0; i < obj_array.getCount(); i++)
    {
      Protection *prot;
      Status s = obj_array[i]->getProtection(prot);
      if (s)
	s->print();

      if (prot)
	{
	  printf("The protection of object %s is",
		 obj_array[i]->getOid().getString());
	  traceProtection(prot, False);
	}
      else
	printf("The object %s has no protection.\n\n",
		 obj_array[i]->getOid().getString());
    }

  return 0;
}

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  if (getopts(argc, argv))
    return 1;

  Connection *conn = new Connection();

  if (status = conn->open())
    {
      status->print();
      return 1;
    }

  db = new Database(dbname);

  if (status = db->open(conn, Database::DBRW))
    {
      status->print();
      return 1;
    }

  dbm = new DBM_Database(Database::getDefaultDBMDB());

  if (status = dbm->open(conn, Database::DBSRead))
    {
      status->print();
      return 1;
    }

  int r;
  db->transactionBegin();
  dbm->transactionBegin();

  if (action == ADD)
    r = addRealize();
  else if (action == UPDATE)
    r = updateRealize();
  else if (action == LIST)
    r = listRealize();
  else if (action == SETPROT)
    r = setprotRealize();
  else if (action == GETPROT)
    r = getprotRealize();

  if (r)
    db->transactionAbort();
  else
    db->transactionCommit();

  dbm->transactionAbort();

  return r;
}
