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


#include "OQLParser.h"
#include <signal.h>
#include <setjmp.h>

#include "GetOpt.h"

using namespace eyedb;
using namespace std;

#undef MOZILLA
#ifdef MOZILLA

extern "C" {
#include <X11/Xlib.h>
}

const char *cgi;

Display *dpy;
Window wid;

#endif

static int
usage()
{
  fprintf(stderr, "usage: eyedboql\n");

  fprintf(stderr, "   -help|-   h     : displays this current message\n");
  fprintf(stderr, "   -db|-d <dbname> : opens database 'dbname'\n");
  fprintf(stderr, "   -rw|-w          : opens database in read/write mode\n");
  fprintf(stderr, "   -admin          : opens database in admin mode\n");
  fprintf(stderr, "   -commit         : commits the current transaction on close\n");
  fprintf(stderr, "   -local|-l       : opens database in local mode\n");
  //  fprintf(stderr, "   -trsless        : opens database in transaction less mode\n");
  fprintf(stderr, "   -c <command>    : executes command\n");
  fprintf(stderr, "   -print|-p       : displays all the objects loaded\n");
  fprintf(stderr, "   -full|-f        : the full recursive mode is used to display objects\n");
  fprintf(stderr, "   -echo           : echoes each commands\n");
  fprintf(stderr, "   <file>          : executes file\n");
  fprintf(stderr, "   -i              : enter interpreter after executing file or commands\n");
#ifdef MOZILLA
  fprintf(stderr, "   -display        : display to use for the 'show' command\n");
  fprintf(stderr, "   -cgi            : cgi to use for the 'show' command\n");
  fprintf(stderr, "   -wid            : window identifier to use with the 'show' command\n");
#endif

  print_use_help(cerr);
  fflush(stderr);

  return 1;
}

enum CFType {
  CFCommand = 1,
  CFFile    = 2
};

struct CFItem {
  CFType type;
  char *str;
  CFItem(CFType t, const char *s): type(t), str(strdup(s)) {}
};

idbOqlOperation operation = idbOqlLocal;
Bool oql_interrupt = False;

static Connection *conn;

static OQLParser *parser;

void oql_sig_h(int sig)
{
  signal(SIGINT, oql_sig_h);

  parser->clear_oql_buffer();

  if (!sig)
    oql_interrupt = False;
  else
    printf("Interrupted!\n");

  if (operation == idbOqlLocal) {
    oql_interrupt = True;
    return;
  }

  if (!sig)
    return;

  if (operation == idbOqlScanning) {
    oql_interrupt = True;
    return;
  }

  if (operation == idbOqlQuerying) {
    conn->sendInterrupt();
    oql_interrupt = True;
  }
}

static void (*user_handler)(Status s, void *);

void
check_status(Status s, void *)
{
  if (user_handler)
    user_handler(s, (void *)0);

  if (s->getStatus() == IDB_SERVER_FAILURE) {
    s->print();
    fprintf(stderr, "eyedboql exiting...\n");
    exit(1);
  }
}

#include "CollectionBE.h"
#include "eyedb_p.h"

void
dump_consts()
{
  printf("IDB_OBJ_HEAD_TYPE_INDEX = %d\n", IDB_OBJ_HEAD_TYPE_INDEX);
  printf("IDB_OBJ_HEAD_SIZE_INDEX = %d\n", IDB_OBJ_HEAD_SIZE_INDEX);
  printf("IDB_OBJ_HEAD_CTIME_INDEX = %d\n", IDB_OBJ_HEAD_CTIME_INDEX);
  printf("IDB_OBJ_HEAD_MTIME_INDEX = %d\n", IDB_OBJ_HEAD_MTIME_INDEX);
  printf("IDB_OBJ_HEAD_XINFO_INDEX = %d\n", IDB_OBJ_HEAD_XINFO_INDEX);
  printf("IDB_OBJ_HEAD_OID_MCL_INDEX = %d\n", IDB_OBJ_HEAD_OID_MCL_INDEX);
  printf("IDB_OBJ_HEAD_OID_PROT_INDEX = %d\n", IDB_OBJ_HEAD_OID_PROT_INDEX);
  printf("IDB_OBJ_HEAD_SIZE	 = %d\n", IDB_OBJ_HEAD_SIZE	);
  printf("IDB_CLASS_EXTENT = %d\n", IDB_CLASS_EXTENT);
  printf("IDB_CLASS_COMPONENTS = %d\n", IDB_CLASS_COMPONENTS);
  printf("IDB_CLASS_IMPL_BEGIN = %d\n", IDB_CLASS_IMPL_BEGIN);
  printf("IDB_CLASS_IMPL_TYPE = %d\n", IDB_CLASS_IMPL_TYPE);
  printf("IDB_CLASS_IMPL_DSPID = %d\n", IDB_CLASS_IMPL_DSPID);
  printf("IDB_CLASS_IMPL_INFO = %d\n", IDB_CLASS_IMPL_INFO);
  printf("IDB_CLASS_IMPL_MTH = %d\n", IDB_CLASS_IMPL_MTH);
  printf("IDB_CLASS_IMPL_HINTS = %d\n", IDB_CLASS_IMPL_HINTS);
  printf("IDB_CLASS_MTYPE = %d\n", IDB_CLASS_MTYPE);
  printf("IDB_CLASS_DSPID = %d\n", IDB_CLASS_DSPID);
  printf("IDB_CLASS_HEAD_SIZE = %d\n", IDB_CLASS_HEAD_SIZE);
  printf("IDB_CLASS_COLL_START = %d\n", IDB_CLASS_COLL_START);
  printf("IDB_CLASS_COLLS_CNT = %d\n", IDB_CLASS_COLLS_CNT);
  printf("IDB_CLASS_ATTR_START = %d\n", IDB_CLASS_ATTR_START);
}

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  //dump_consts();

#ifdef MOZILLA
  cgi = eyedb::getConfigValue("cgi");
#endif
  user_handler = Exception::getHandler();
  Exception::setHandler(check_status);

  LinkedList *clist = new LinkedList;
  const char *dbname = 0;
  const char *dbmfile = 0;
  unsigned int mode = 0;
  Bool printmode = False, recmode = False, echo = False,
    interact = False, commit = False;
  const char *display = NULL;

  dbmfile = Database::getDefaultDBMDB();

  static const std::string database_opt = "database";
  static const std::string read_opt = "read";
  static const std::string read_write_opt = "read-write";
  static const std::string local_opt = "local";
  static const std::string strict_read_opt = "strict-read";
  static const std::string admin_opt = "admin";
  static const std::string command_opt = "command";
  static const std::string print_opt = "print";
  static const std::string full_opt = "full";
  static const std::string commit_opt = "commit";
  static const std::string interact_opt = "interact";
  static const std::string echo_opt = "echo";
  static const std::string help_opt = "help";

  Option opts[] = {
    Option('d', database_opt, OptionStringType(),
	   Option::MandatoryValue, OptionDesc("Database name", "NAME")),
    Option('r', read_opt, OptionBoolType(),
	   0, OptionDesc("Open database in read mode")),
    Option('w', read_write_opt, OptionBoolType(),
	   0, OptionDesc("Open database in read/write mode")),
    Option('s', strict_read_opt, OptionBoolType(),
	   0, OptionDesc("Open database in strict read mode")),
    Option('l', local_opt, OptionBoolType(),
	   0, OptionDesc("Open database in local mode")),
    Option('c', command_opt, OptionStringType(),
	   Option::MandatoryValue, OptionDesc("OQL command to execute", "COMMAND")),
    Option('p', print_opt, OptionBoolType(),
	   0, OptionDesc("Display all the objects loaded")),
    Option(full_opt, OptionBoolType(),
	   0, OptionDesc("Full recursive mode is used to display objects")),
    Option(commit_opt, OptionBoolType(),
	   0, OptionDesc("Commits the current transaction on close")),
    Option('i', interact_opt, OptionBoolType(),
	   0, OptionDesc("Enter interpreter after executing file or commands")),
    Option('e', echo_opt, OptionBoolType(),
	   0, OptionDesc("Echo each command")),
    Option(admin_opt, OptionBoolType(),
	   0, OptionDesc("Open database in admin mode")),
    Option('h', help_opt, OptionBoolType(),
	   0, OptionDesc("Display this message"))
  };

  GetOpt getopt(argv[0], opts, sizeof(opts)/sizeof(opts[0]));

  if (!getopt.parse(argc, argv)) {
    print_standard_usage(getopt, "[FILE...]");
    exit(0);
  }

  GetOpt::Map &map = getopt.getMap();

  if (map.find(help_opt) != map.end()) {
    print_standard_usage(getopt, "[FILE..]");

    std::vector<std::string> options;
    options.push_back("FILE...");
    options.push_back("File(s) to execute");

    cerr << "\n\n";
    print_standard_help(getopt, options);
    exit(0);
  }

  if (map.find(database_opt) != map.end())
    dbname = strdup(map[database_opt].value.c_str());

  if (map.find(read_opt) != map.end())
    mode |= _DBRead;

  if (map.find(read_write_opt) != map.end())
    mode |= _DBRW;

  if (map.find(strict_read_opt) != map.end())
    mode |= _DBSRead;

  if (map.find(read_opt) != map.end())
    mode |= _DBRead;

  if (map.find(admin_opt) != map.end())
    mode |= _DBAdmin;

  if (map.find(local_opt) != map.end())
    mode |= _DBOpenLocal;

  if (map.find(command_opt) != map.end()) {
    CFItem *item = new CFItem(CFCommand, map[command_opt].value.c_str());
    clist->insertObjectLast(item);
  }

  if (map.find(print_opt) != map.end())
    printmode = True;

  if (map.find(commit_opt) != map.end())
    commit = True;

  if (map.find(interact_opt) != map.end())
    interact = True;

  if (map.find(full_opt) != map.end())
    recmode = True;

  if (map.find(echo_opt) != map.end())
    echo = True;

  for (int i = 1; i < argc; i++) {
    CFItem *item = new CFItem(CFFile, argv[i]);
    clist->insertObjectLast(item);
  }

#ifdef MOZILLA
  dpy = XOpenDisplay(display);
#endif

  if (!(mode & _DBRead) && !(mode & _DBRW) && !(mode & _DBSRead))
    mode |= _DBSRead; // 24/08/01: new default

  conn = new Connection();
  Status status = conn->open();
  if (status) {
    status->print();
    return 1;
  }

  conn->echoServerMessages(*new StdServerMessageDisplayer());
  parser = new OQLParser(conn, argc, argv);

  if (printmode) {
    if (recmode)
      parser->setPrintMode(OQLParser::fullRecurs);
    else
      parser->setPrintMode(OQLParser::noRecurs);
  }

  if (echo)
    parser->setEchoMode(echo);

  if (commit)
    parser->setCommitOnCloseMode(True);

  if (dbmfile)
    parser->setDbmdb(dbmfile);

  if (dbname && parser->setDatabase(dbname, (Database::OpenFlag)mode, 0))
    return 1;

  if (clist->getCount()) {
    parser->setOnErrorQuit(1);

    LinkedListCursor *c = clist->startScan();

    CFItem *item;
    while (clist->getNextObject(c, (void *&)item)) {
      if (item->type == CFCommand) {
	if (!parser->parse((std::string(item->str) + ";").c_str(), echo) ||
	    parser->getExitMode() ||
	    parser->getInterrupt())
	  break;
      }
      else  {
	if (!parser->parse_file(item->str)) {
	  fprintf(stderr, "eyedboql: cannot read file '%s'\n",
		  item->str);
	  continue;
	}

	if (parser->getExitMode() || parser->getInterrupt())
	  break;
      }
    }

    clist->endScan(c);

    if (parser->getExitMode() || !interact) {
      parser->quit();
      return 0;
    }
  }

  fprintf(stdout, "Welcome to eyedboql.\n  Type `%chelp' to display the command list.\n", parser->getEscapeChar());
  fprintf(stdout, "  Type `%ccopyright' to display the copyright.\n", parser->getEscapeChar());
  parser->setInterrupt(False);
  signal(SIGINT, oql_sig_h);
  parser->clear_oql_buffer();
  while (!parser->getExitMode()) {
    parser->prompt();
    parser->parse(stdin, False);
  }

  delete parser;
  return 0;
}
