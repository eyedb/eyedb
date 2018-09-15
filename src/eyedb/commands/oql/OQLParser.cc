/* 
   EyeDB Object Database Management System
   OQL interpreter command-line tool
   Copyright (C) 1994-2018 SYSRA
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   Author: Eric Viara <viara@sysra.com>
*/


#undef MOZILLA

#include <eyedbconfig.h>
#include <sstream>
using namespace std;

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#ifdef HAVE_LIBEDITLINE
extern "C" {
  // This include file is broken: not ANSI-C, cannot compile it with a C++ compiler
  // #include <editline.h>
  // So we prototype the functions by hand :(
  extern char *readline( const char*);
  extern void add_history( const char*);
}
#endif

#ifdef MOZILLA
#include <X11/Xlib.h>

const char *expected_mozilla_version = "3.01";
const char *progname = "eyedboql";

void mozilla_remote_commands(Display *, Window, char **, Bool);

extern const char *cgi;

extern Display *dpy;
extern Window wid;

#endif

#include "eyedb/base.h"
#include "eyedb/Error.h"
#include "eyedb/Exception.h"
#include "eyedb/TransactionParams.h"
#include "OQLParser.h"
#include "DBM_Database.h"
#include <signal.h>
#include <eyedblib/m_malloc.h>
#include "oqlctb.h"
#include <eyedb/utils.h>

extern void oql_sig_h(int);
extern eyedb::Bool oql_interrupt;
extern eyedb::idbOqlOperation operation;

static unsigned int indent_num = 15;
static const char command_help[] = "  Command:\n";
static const char options_help[ ] = "  Options:\n";
static const char option_help[ ] = "  Option:\n";
static const char help_indent[]  = "    ";

namespace eyedb {

  FILE *
  run_cpp(FILE *fd, const char *cpp_cmd, const char *cpp_flags,
	  const char *file);

  static Bool print_remote = True;
  static OQLParser *mParser;

  static Bool auto_trmode = True;
  static int tr_cnt;
  static TransactionParams params;

  static void usage(char esc)
  {
    fprintf(stderr, "No database opened.\n");
    fprintf(stderr, "The first thing to do is to open a database, using the metacommand `%copen'.\n", esc);
    fprintf(stderr, "Use the metacommand `%chelp' to display the command list.\n", esc);
  }

  OQLParser::OQLParser(Connection *_conn, int &argc, char *argv[])
  {
    conn = _conn;

    params = TransactionParams::getGlobalDefaultTransactionParams();

    int_mode = False;
    prmode = none;
    mutemode = 0;
    echofound = 0;
    prompt_str = 0;
    second_prompt_str = 0;
    setPrompt("? ");
    setSecondPrompt(">> ");
    setPrintMode(none);
    setShowMode(False);
    setOnErrorAbort(0);
    setOnErrorQuit(0);
    setOnErrorEcho(1);
    setEscapeChar('\\');
    cmmode = 0;
    bindClose = 0;
    bindOpen = 0;
    sep = 0;
    nl = 0;
    resetFormat();

    setTermChar(';');
    head = 0;
    last = 0;
    set_standard_command();
    exit_mode = False;
    oql_buffer = (char *)0;
    oql_buffer_len = 0;
    oql_buffer_alloc = 0;
    db = 0;
    last_cnt = 0;
    last_alloc = 0;
    dbmdb = 0;
    setEchoMode(False);

    mParser = this;

    const char *home;
    const char initfile[] = ".eyedboqlinit";

    if (home = getenv("HOME"))
      {
	char *file = (char *)malloc(strlen(home)+strlen(initfile)+2);
	sprintf(file, "%s/%s", home, initfile);
	parse_file(file);
	free(file);
      }

    parse_file(initfile);
  }

  void OQLParser::resetFormat()
  {
    splitLen = 80;
    free(sep);
    sep = strdup(", ");
    free(bindOpen);
    bindOpen = strdup("(");
    free(bindClose);
    bindClose = strdup(")");
    free(nl);
    nl = strdup("\n");
    align = 0;
    align_left = 0;
    columns = 0;
  }

  static int curline, errorline;
  static const char *curfile;

  int OQLParser::parse_file(const char *file)
  {
    int ocurline = curline;
    const char *ocurfile = curfile;

    FILE *fd = fopen(file, "r");
    if (!fd)
      return 0;

    curline = 0;
    errorline = 0;
    curfile = file;

    fd = run_cpp(fd, 0, 0, file); // added the 16/11/99

    if (fd) {
      while(parse(fd, echo_mode) && !oql_interrupt && !getExitMode())
	;
      fclose(fd);
      if (oql_interrupt)
	oql_sig_h(0);
    }

    curline = ocurline;
    curfile = ocurfile;
    return fd ? 1 : 0;
  }

  OQLCommand::OQLCommand(const char *_name,
			 int (*_exec)(OQLParser *, int, char *[]),
			 void (*_usage)(FILE *, OQLCommand *, bool indent),
			 void (*_help)(FILE *, OQLCommand *))
  {
    name_cnt = 1;
    names = new char*[name_cnt];
    names[0] = strdup(_name);
    exec = _exec;
    usage = _usage;
    help = _help;
    prev = next = 0;
  }

  OQLCommand::OQLCommand(const char **_names, int _name_cnt,
			 int (*_exec)(OQLParser *, int, char *[]),
			 void (*_usage)(FILE *, OQLCommand *, bool indent),
			 void (*_help)(FILE *, OQLCommand *))
  {
    name_cnt = _name_cnt;
    names = new char*[name_cnt];
    for (int i = 0; i < name_cnt; i++)
      names[i] = strdup(_names[i]);
    exec = _exec;
    usage = _usage;
    help = _help;
    prev = next = 0;
  }

  static void
  command_s_print(FILE *fd, OQLCommand *cmd, bool indent = false)
  {
    int name_cnt;
    const char **names = cmd->getNames(name_cnt);
    char esc = mParser->getEscapeChar();
    ostringstream ostr;
    for (int i = 0; i < name_cnt; i++)
      ostr << (i > 0 ? "|" : "") <<  esc << names[i];
    if (indent)
      for (int i = ostr.str().length(); i < indent_num; i++)
	ostr << ' ';
    else
      ostr << ' ';
    //  fprintf(fd, "%s%c%s", (i > 0 ? "|" : ""), esc, names[i]);
    fprintf(fd, "%s", ostr.str().c_str());
  }

  static void
  quit_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  quit_help(FILE *fd, OQLCommand *cmd)
  {
    quit_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%squits the current eyedboql session\n", help_indent);
  }

  static void
  clear_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  clear_help(FILE *fd, OQLCommand *cmd)
  {
    clear_usage(fd, cmd, false);
    fprintf(fd, "\tclears the current output buffer.\n");
  }

  static void
  begin_args_usage(FILE *fd, OQLCommand *cmd)
  {
    fprintf(fd, "[R_S_W_S|R_S_W_SX|R_S_W_X|R_SX_W_SX|R_SX_W_X|R_X_W_X|R_N_W_S|R_N_W_SX|R_N_W_X|R_N_W_N|DB_W|DB_RW|DB_Wtrans] [RV_OFF|RV_FULL|RV_PARTIAL] [MG=MAGORDER] [RT=RATIOALRT] [TM=WAIT_TIMEOUT] [TRSOFF]\n");
  }

  static void
  begin_args_help(FILE *fd, OQLCommand *cmd)
  {
    fprintf(fd, command_help);
    fprintf(fd, "%sbegin a new transaction\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%sR_S_W_S     read shared/write shared\n", help_indent);
    fprintf(fd, "%sR_S_W_SX    read shared/write shared exclusive\n", help_indent);
    fprintf(fd, "%sR_S_W_X     read shared/write exclusive\n", help_indent);
    fprintf(fd, "%sR_SX_W_SX   read shared exclusive/write shared exclusive\n", help_indent);
    fprintf(fd, "%sR_SX_W_X    read shared exclusive/write exclusive\n", help_indent);
    fprintf(fd, "%sR_X_W_X     read exclusive/write exclusive\n", help_indent);
    fprintf(fd, "%sR_N_W_S     read no lock/write shared\n", help_indent);
    fprintf(fd, "%sR_N_W_SX    read no lock/write shared exclusive\n", help_indent);
    fprintf(fd, "%sR_N_W_X     read no lock/write exclusive\n", help_indent);
    fprintf(fd, "%sR_N_W_N     read no lock/write no lock\n", help_indent);
    fprintf(fd, "%sDB_W        database exclusive for writing\n", help_indent);
    fprintf(fd, "%sDB_RW       database exclusive for reading and writing\n", help_indent);
    fprintf(fd, "%sDB_Wtrans   database exclusive for writing transactions\n", help_indent);
    fprintf(fd, "\n");
    fprintf(fd, "%sRV_OFF      recovery off\n", help_indent);
    fprintf(fd, "%sRV_FULL     full recovery (not yet implemented)\n", help_indent);
    fprintf(fd, "%sRV_PARTIAL  partial recovery\n", help_indent);
    fprintf(fd, "\n");
    fprintf(fd, "%sMG=MAGORDER  magnitude order of object count\n", help_indent);
    fprintf(fd, "%sRT=RATIOALRT ratio alert\n", help_indent);
    fprintf(fd, "%sTM=TIMEOUT   wait timeout in seconds\n", help_indent);
    fprintf(fd, "%sTRSOFF      no transaction processing\n", help_indent);
  }

  static void
  settrmode_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "noauto|auto ");
    begin_args_usage(fd, cmd);
  }

  static void
  settrmode_help(FILE *fd, OQLCommand *cmd)
  {
    settrmode_usage(fd, cmd, false);
    begin_args_help(fd, cmd);
  }

  static void
  begin_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    begin_args_usage(fd, cmd);
  }

  static void
  begin_help(FILE *fd, OQLCommand *cmd)
  {
    begin_usage(fd, cmd, false);
    begin_args_help(fd, cmd);
  }

  static void
  commit_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  commit_help(FILE *fd, OQLCommand *cmd)
  {
    commit_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%scommit the current transaction\n", help_indent);
  }

  static void
  copyright_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    //command_s_print(fd, cmd, indent);
    //fprintf(fd, "\n");
  }

  static void
  copyright_help(FILE *fd, OQLCommand *cmd)
  {
    return;
    copyright_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays the copyright\n", help_indent);
  }

  static void
  abort_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  abort_help(FILE *fd, OQLCommand *cmd)
  {
    command_s_print(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sabort the current transaction\n", help_indent);
  }

  static void
  user_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[USER [PASSWD]]\n");
  }

  static void
  user_help(FILE *fd, OQLCommand *cmd)
  {
    user_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets the current user\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%sUSER   the new user. Ask for it, if not present\n", help_indent);
    fprintf(fd, "%sPASSWD the password. Ask for it, if not present\n", help_indent);

  }

  static void
  help_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[COMMAND]\n");
  }

  static void
  help_help(FILE *fd, OQLCommand *cmd)
  {
    help_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays usage of all commands or help of one command\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%sCOMMAND  name of the command for which to display help\n", help_indent);
  }

  static void
  setprompt_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[1|2] PROMPT\n");
  }

  static void
  setprompt_help(FILE *fd, OQLCommand *cmd)
  {
    setprompt_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets the prompt level 1 or 2\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%s1        prompt level 1 (default)\n", help_indent);
    fprintf(fd, "%s2        prompt level 2\n", help_indent);
    fprintf(fd, "%sPROMPT   new prompt\n", help_indent);
  }

  static void
  onerror_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "abort|quit|echo yes|no\n");
  }

  static void
  onerror_help(FILE *fd, OQLCommand *cmd)
  {
    onerror_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets the error policy\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%sabort   aborts (according to yes/no option) on any error\n", help_indent);
    fprintf(fd, "%squit    quits (according to yes/no option) the session  on any error\n", help_indent);
    fprintf(fd, "%secho    echoes (according to yes/no option) any error  on any error\n", help_indent);
    fprintf(fd, "%syes     sets the previous option to yes\n", help_indent);
    fprintf(fd, "%sno      sets the previous option to no\n", help_indent);
  }

  static void
  settermchar_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[CHAR]\n");
  }

  static void
  settermchar_help(FILE *fd, OQLCommand *cmd)
  {
    settermchar_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets the terminal statement character\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%sCHAR  the terminal statement character (default ';')\n", help_indent);
  }

  static void
  schema_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[sys] [native] [attrcomp] [attrcomp*] [all]\n");
  }

  static void
  schema_help(FILE *fd, OQLCommand *cmd)
  {
    schema_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays the database schema\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%ssys       displays system classes\n", help_indent);
    fprintf(fd, "%snative    displays native attributes\n", help_indent);
    fprintf(fd, "%sattrcomp  displays attribute components\n", help_indent);
    fprintf(fd, "%sattrcomp* displays attribute component details\n", help_indent);
    fprintf(fd, "%sall       displays all\n", help_indent);
  }

  static void
  setecho_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "true|false\n");
  }

  static void
  setecho_help(FILE *fd, OQLCommand *cmd)
  {
    setecho_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sechoes commands when reading a file\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%strue   sets echo mode to true\n", help_indent);
    fprintf(fd, "%sfalse  sets echo mode to false\n", help_indent);
  }

  static void
  time_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[-raz] [MESSAGE]\n");
  }

  static void
  break_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  break_help(FILE *fd, OQLCommand *cmd)
  {
    break_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sinterrupts the current OQL statement\n", help_indent);
  }

  static void
  time_help(FILE *fd, OQLCommand *cmd)
  {
    time_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays the time in seconds since beginning of last 'time -raz' and optional message\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%s-raz      reset the time\n", help_indent);
    fprintf(fd, "%sMESSAGE displays the message\n", help_indent);
  }

  static void
  setescapechar_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "CHAR\n");
  }

  static void
  setescapechar_help(FILE *fd, OQLCommand *cmd)
  {
    setescapechar_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%schanges escape character to CHAR (default is \\)\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%sCHAR    the new escape character\n", help_indent);
  }

  static void
  shell_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[ARG...]\n");
  }

  static void
  shell_help(FILE *fd, OQLCommand *cmd)
  {
    shell_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sexecutes shell command\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%s[ARG...] optional shell command (if no present executes a shell)\n", help_indent);
  }

  static void
  open_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[DATABASE [rw|ro|sr] [local]\n");
  }

  static void
  open_help(FILE *fd, OQLCommand *cmd)
  {
    open_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sopens a database or return the current database\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%sDATABASE the database to open (if not present, returns the current opened database)\n", help_indent);
    fprintf(fd, "%srw         opens in read/write mode\n", help_indent);
    fprintf(fd, "%sro         opens in read only mode\n", help_indent);
    fprintf(fd, "%ssr         opens in strict read mode\n", help_indent);
    fprintf(fd, "%slocal      opens in local mode\n", help_indent);
  }

  static void
  setdbmdb_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[DBMDB]\n");
  }

  static void
  setdbmdb_help(FILE *fd, OQLCommand *cmd)
  {
    setdbmdb_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets the EYEDBDBM database to DBMDB\n", help_indent);
    fprintf(fd, option_help);
    fprintf(fd, "%sDBMDB the EYEDBM database\n", help_indent);
  }

  static void
  print_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[OID...] [full] [ctime] [mtime] [cmtime] [contents] [native] [attrcomp] [attrcomp*] [compoid] [method] [all] [local|remote]\n");
  }

  static void
  print_help(FILE *fd, OQLCommand *cmd)
  {
    print_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays objets from an oid list or from the last oids returned\n", help_indent);
    fprintf(fd, options_help);
    fprintf(fd, "%s[OID...]    optional oid list\n", help_indent);
    fprintf(fd, "%sfull      follows all links between objects\n", help_indent);
    fprintf(fd, "%sctime     displays creation time\n", help_indent);
    fprintf(fd, "%smtime     displays modification time\n", help_indent);
    fprintf(fd, "%scmtime    displays creation modification time\n", help_indent);
    fprintf(fd, "%scontents  displays contents of collections\n", help_indent);
    fprintf(fd, "%snative    displays native attributes\n", help_indent);
    fprintf(fd, "%sattrcomp  displays attribute components\n", help_indent);
    fprintf(fd, "%sattrcomp* displays attribute component details\n", help_indent);
    fprintf(fd, "%scompoid   displays component oids\n", help_indent);
    fprintf(fd, "%smethod    displays methods\n", help_indent);
    fprintf(fd, "%sall       all options\n", help_indent);
    fprintf(fd, "%sremote    use remote method toString() (default)\n", help_indent);
    fprintf(fd, "%slocal     use local method trace()\n", help_indent);
  }

#ifdef MOZILLA
  static void
  show_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  show_help(FILE *fd, OQLCommand *cmd)
  {
    show_usage(fd, cmd, false);
    fprintf(fd, "\t...\n");
  }
#endif

  static void
  format_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[align [-]number|no] | [begin | end | nl string] | [col number|no]\n"
	    "         | reset | [sep string] | [split number|no]\n");
  }

  static void
  format_help(FILE *fd, OQLCommand *cmd)
  {
    format_usage(fd, cmd, false);
    fprintf(fd, "\t...\n");
  }

  static void
  mssetup_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "\n");
  }

  static void
  mssetup_help(FILE *fd, OQLCommand *cmd)
  {
    mssetup_usage(fd, cmd, false);
    fprintf(fd, "\t...\n");
  }

  static void
  setprint_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[true|false] [full|no]]\n");
  }

  static void
  setprint_help(FILE *fd, OQLCommand *cmd)
  {
    setprint_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets auto-print to true or false\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%strue   sets auto print to true\n", help_indent);
    fprintf(fd, "%sfalse  sets auto print to false\n", help_indent);
    fprintf(fd, "%sfull   follows all links between objects\n", help_indent);
    fprintf(fd, "%sno     does not follow any link between objects\n", help_indent);
  }

  static void
  setshow_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[true|false]\n");
  }

  static void
  setshow_help(FILE *fd, OQLCommand *cmd)
  {
    setshow_usage(fd, cmd, false);
    fprintf(fd, "\t...\n");
  }

  static void
  commitonclose_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[true|false]\n");
  }

  static void
  commitonclose_help(FILE *fd, OQLCommand *cmd)
  {
    commitonclose_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%ssets automatic transaction commit when leaving session to true or false\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%strue   sets auto commit to true\n", help_indent);
    fprintf(fd, "%sfalse  sets auto commit to false\n", help_indent);
  }

  static void
  setmute_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[true|false]\n");
  }

  static void
  setmute_help(FILE *fd, OQLCommand *cmd)
  {
    setmute_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%ssets quiet mode (mute output) true or false\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%strue   sets mute to true\n", help_indent);
    fprintf(fd, "%sfalse  sets mute to false\n", help_indent);
  }

  /*
    static void
    setechofound_usage(FILE *fd, OQLCommand *cmd, bool indent)
    {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "[true|false]\n");
    }

    static void
    setechofound_help(FILE *fd, OQLCommand *cmd)
    {
    setechofound_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%scontrols display of the number of found objects\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%strue   displays the number of found objects\n", help_indent);
    fprintf(fd, "%strue   does not display the number of found objects\n", help_indent);
    }
  */

  static void
  echo_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "ARG...\n");
  }

  static void
  echo_help(FILE *fd, OQLCommand *cmd)
  {
    echo_usage(fd, cmd, false);

    fprintf(fd, command_help);
    fprintf(fd, "%sdisplays the given arguments\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%sARG... the arguments to display\n", help_indent);
  }

  static void
  read_usage(FILE *fd, OQLCommand *cmd, bool indent)
  {
    command_s_print(fd, cmd, indent);
    fprintf(fd, "FILE\n");
  }

  static void
  read_help(FILE *fd, OQLCommand *cmd)
  {
    read_usage(fd, cmd, false);
    fprintf(fd, command_help);
    fprintf(fd, "%sexecutes OQL and meta commands contained in a file\n", help_indent);

    fprintf(fd, options_help);
    fprintf(fd, "%sFILE   the file to read\n", help_indent);
  }

#include <pwd.h>

  static char *
  userNameGet()
  {
    struct passwd *p;
  
    if (!(p = getpwuid(getuid())))
      return (char*)"";/*@@@@warning cast*/
    else
      {
	/*
	  char *x;
	  if (x = strchr(p->pw_comment, ','))
	  *x = 0;
	  */
	return p->pw_name;
      }
  }

  void
  OQLParser::quit()
  {
    if (getDatabase() && !getCommitOnCloseMode())
      getDatabase()->transactionAbort();

    closeDB();
    setExitMode(True);
  }

  static int
  quit_command(OQLParser *parser, int, char *[])
  {
    Database *db = parser->getDatabase();
    if (db && db->isInTransaction() && (db->getOpenFlag() & _DBRW)) {
      for (;;) {
	printf("A transaction is currently running.\n"
	       "Do you want to commit the current transaction [y/n]? ");
	char c = getchar();

	if (c == 'y' || c == 'Y') {
	  Status s = db->transactionCommit();
	  if (s && s->getStatus() != IDB_NO_CURRENT_TRANSACTION) {
	    s->print();
	  }
	  else
	    printf("Transaction committed.\n");
	  break;
	}

	if (c == 'n' || c == 'N')
	  break;

	printf("\nPlease, answer y or n\n");
	if (c != '\n')
	  while (getchar() != '\n');
      }
    }

    parser->quit();
    return 1;
  }

  static int
  setprint_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc > 3)
      return 0;
    else if (argc == 1)
      printf("%s\n", (parser->getPrintMode() == OQLParser::fullRecurs ?
		      "full recursion print mode" : (parser->getPrintMode() == OQLParser::noRecurs ?
						     "no recursion print mode" : "no print mode")));
    else if (!strcmp(argv[1], "true"))
      {
	if (argc > 2)
	  {
	    if (!strcmp(argv[2], "full"))
	      parser->setPrintMode(OQLParser::fullRecurs);
	    else if (!strcmp(argv[2], "no"))
	      parser->setPrintMode(OQLParser::noRecurs);
	    else
	      return 0;
	  }
	else
	  parser->setPrintMode(OQLParser::noRecurs);
      }
    else if (!strcmp(argv[1], "false"))
      parser->setPrintMode(OQLParser::none);
    else
      return 0;
  
    return 1;
  }

  static int
  setshow_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc > 2)
      return 0;
    else if (argc == 1)
      printf("%s\n", parser->getShowMode() ? "true" : "false");
    else if (!strcmp(argv[1], "true"))
      parser->setShowMode(True);
    else if (!strcmp(argv[1], "false"))
      parser->setShowMode(False);
    else
      return 0;
  
    return 1;
  }

  static int
  commitonclose_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1)
      printf("%s\n", (parser->getCommitOnCloseMode() ? "true" : "false"));
    else if (argc != 2)
      return 0;
    else if (!strcmp(argv[1], "true"))
      parser->setCommitOnCloseMode(1);
    else if (!strcmp(argv[1], "false"))
      parser->setCommitOnCloseMode(0);
    else
      return 0;
  
    return 1;
  }

  static int
  setmute_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1)
      {
	printf("%s\n", (parser->getMuteMode() ? "true" : "false"));
	return 1;
      }
    else if (argc != 2)
      return 0;

    if (!strcmp(argv[1], "true"))
      parser->setMuteMode(1);
    else if (!strcmp(argv[1], "false"))
      parser->setMuteMode(0);
    else
      return 0;
  
    return 1;
  }

  /*
    static int
    setechofound_command(OQLParser *parser, int argc, char *argv[])
    {
    if (argc == 1)
    {
    printf("%s\n", (parser->getEchoFound() ? "true" : "false"));
    return 1;
    }
    else if (argc != 2)
    return 0;

    if (!strcmp(argv[1], "true"))
    parser->setEchoFound(1);
    else if (!strcmp(argv[1], "false"))
    parser->setEchoFound(0);
    else
    return 0;
  
    return 1;
    }
  */

  static int
  help_command(OQLParser *parser, int argc, char *argv[])
  {
    OQLCommand *cmd = parser->get_first_command();
    FILE *pipe = popen(PATH_TO_MORE, "w");
  
    if (pipe) 
      {
	pclose(pipe); 
	pipe = 0;
      }

    if (!pipe)
      pipe = stderr;

    if (argc == 2)
      {
	const char *s;
	if (argv[1][0] == parser->getEscapeChar())
	  s = &argv[1][1];
	else
	  s = argv[1];

	while (cmd)
	  {
	    int name_cnt;
	    const char **names = cmd->getNames(name_cnt);
	    for (int i = 0; i < name_cnt; i++)
	      if (!strcmp(names[i], s)) {
		(*cmd->help)(pipe, cmd);
		if (pipe != stderr)
		  pclose(pipe);
		return 1;
	      }
	    cmd = parser->get_next_command(cmd);
	  }

	fprintf(pipe, "%chelp: unknown command '%s'\n",
		parser->getEscapeChar(), argv[1]);
	if (pipe != stderr)
	  pclose(pipe);
	return 1;
      }

    if (argc != 1)
      {
	if (pipe != stderr)
	  pclose(pipe);
	return 0;
      }

    fprintf(pipe,
	    "Type `%chelp name' to find out more about the command `name'.\n"
	    "\neyedboql commands:\n\n",
	    parser->getEscapeChar(), parser->getEscapeChar());
  
    while (cmd)
      {
	(*cmd->usage)(pipe, cmd, true);
	cmd = parser->get_next_command(cmd);
      }

#ifdef HAVE_LIBREADLINE
    fprintf(pipe, "\nThis version of eyedboql was compiled with GNU readline library for command line editing.\n");
    fprintf(pipe, "See readline man page for a complete description of available command line editing features.\n");
#elif defined(HAVE_LIBEDITLINE)
    fprintf(pipe, "\nThis version of eyedboql was compiled with editline library for command line editing.\n");
    fprintf(pipe, "See editline man page for a complete description of available command line editing features.\n");
#endif

    if (pipe != stderr)
      pclose(pipe);

    return 1;
  }

  static int
  setprompt_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 2)
      {
	parser->setPrompt(argv[1]);
	return 1;
      }

    else if (argc == 3)
      {
	int n = atoi(argv[1]);
	if (n == 1)
	  parser->setPrompt(argv[2]);
	else if (n == 2)
	  parser->setSecondPrompt(argv[2]);
	else
	  return 0;
	return 1;
      }

    return 0;
  }

  static int
  onerror_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc != 3)
      return 0;

    int on;

    if (!strcmp(argv[2], "yes"))
      on = 1;
    else if (!strcmp(argv[2], "no"))
      on = 0;
    else
      return 0;

    if (!strcmp(argv[1], "abort"))
      parser->setOnErrorAbort(on);
    else if (!strcmp(argv[1], "quit"))
      parser->setOnErrorQuit(on);
    else if (!strcmp(argv[1], "echo"))
      parser->setOnErrorEcho(on);
    else
      return 0;
  }

  static int
  settermchar_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 2)
      {
	char *p = argv[1];
	if (strlen(p) == 1)
	  {
	    parser->setTermChar(p[0]);
	    return 1;
	  }
	else if (strlen(p) == 2 && p[0] == '\\' && p[1] == 'n')
	  {
	    parser->setTermChar('\n');
	    return 1;
	  }
	return 0;
      }
    else if (argc == 1)
      {
	char c = parser->getTermChar();
	if (c == '\n')
	  fprintf(stdout, "'\\n'\n");
	else
	  fprintf(stdout, "'%c'\n", parser->getTermChar());
	return 1;
      }
    return 0;
  }

  static int
  schema_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc > 4)
      return 0;

    Database *db = parser->getDatabase();
    if (!db)
      {
	usage(parser->getEscapeChar());
	return 0;
      }

    Bool sys = False;
    int flags = 0;
    for (int i = 1; i < argc; i++)
      {
	if (!strcmp(argv[i], "sys"))
	  sys = True;
	else if (!strcmp(argv[i], "pointer"))
	  flags |= PointerTrace;
	else if (!strcmp(argv[i], "attrcomp"))
	  flags |= AttrCompTrace;
	else if (!strcmp(argv[i], "attrcomp*"))
	  flags |= AttrCompDetailTrace;
	else if (!strcmp(argv[i], "native"))
	  flags |= NativeTrace;
	else if (!strcmp(argv[i], "all"))
	  flags |= CompOidTrace|NativeTrace|CMTimeTrace|ExecBodyTrace|SysExecTrace|ContentsFlag|AttrCompTrace|AttrCompDetailTrace;
	else
	  return 0;
      }

    db->transactionBegin();

    try {
      LinkedListCursor c(db->getSchema()->getClassList());
      Class *cl;
      while (c.getNext((void *&)cl))
	if (sys || (!cl->isSystem() && !cl->asCollectionClass()))
	  cl->trace(stdout, flags, RecMode::NoRecurs);
    }
    catch(Exception &e) {
      e.print();
    }

    db->transactionAbort();

    return 1;
  }

  static int
  setecho_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1) {
      printf("%s\n", (parser->getEchoMode() ? "true" : "false"));
      return 1;
    }
    else if (argc == 2) {
      char *p = argv[1];
      if (!strcmp(p, "true"))
	parser->setEchoMode(True);
      else if (!strcmp(p, "false"))
	parser->setEchoMode(False);
      else
	return 0;

      return 1;
    }

    return 0;
  }

  static int
  break_command(OQLParser *parser, int argc, char *argv[])
  {
    oql_interrupt = True;
    return 1;
  }

  static unsigned long long ms_start;

  static int
  time_command(OQLParser *parser, int argc, char *argv[])
  {
    time_t t;
    time(&t);
    struct timeval tp;
    gettimeofday(&tp, 0);

    int st;
    if (argc >= 2 && !strcmp(argv[1], "-raz")) {
      ms_start = 0;
      st = 2;
    }
    else
      st = 1;

    if (argc > st) {
      for (int i = st; i < argc; i++)
	printf("%s%s", (i > st ? " " : ""), argv[i]);
      printf(": ");
    }

    char *s = ctime(&t);
    s[strlen(s)-1] = 0;
    unsigned long long ms = tp.tv_sec * 1000 + tp.tv_usec/1000;
    if (!ms_start)
      ms_start = ms;
    printf("%s [%lld ms]\n", s, ms-ms_start);
    return 1;
  }

  static int
  setescapechar_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1) {
      printf("%c\n", parser->getEscapeChar());
      return 1;
    }
    else if (argc == 2) {
      char *p = argv[1];
      if (strlen(p) == 1) {
	parser->setEscapeChar(p[0]);
	return 1;
      }
    }
    return 0;
  }

  static int
  read_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc >= 2 && !strcmp(argv[1], "-"))
      {
	for (int i = 2; i < argc; i++)
	  printf("%s%s", (i > 2 ? " " : ""), argv[i]);
	while (getchar() != '\n');
	return 1;
      }

    if (argc == 2)
      {
	if (!parser->parse_file(argv[1]))
	  {
	    fprintf(stderr, "cannot open file '%s' for reading\n", argv[1]);
	    return 0;
	  }

	return 1;
      }
    return 0;
  }

  static int
  shell_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1) {
      const char *shell = getenv("SHELL");
      if (!shell) shell = "/bin/sh";
      system(shell);
      return 1;
    }

    int len = 0;
    int i;
    for (i = 1; i < argc; i++)
      len += strlen(argv[i]) + 1;

    char *buf = (char *)malloc(len+1);
    buf[0] = 0;

    for (i = 1; i < argc; i++)
      {
	strcat(buf, argv[i]);
	strcat(buf, " ");
      }

    system(buf);
    free(buf);
    return 1;
  }

  static int
  open_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc > 4)
      return 0;

    if (argc == 1)
      {
	Database *db = parser->getDatabase();
	if (db)
	  printf("%s\n", db->getName());
	else
	  printf("<no database opened>\n");
	return 1;
      }

    int flag = 0;

    OpenHints oh;
    oh.maph = DefaultMap;
    oh.mapwide = 0;

    for (int n = argc - 1; n > 1; n--)
      {
	if (!strcmp(argv[n], "rw"))
	  flag |= _DBRW;
	else if (!strcmp(argv[n], "r"))
	  flag |= _DBRead;
	else if (!strcmp(argv[n], "sr"))
	  flag |= _DBSRead;
	else if (!strcmp(argv[n], "local"))
	  flag |= _DBOpenLocal;
	else if (!strcmp(argv[n], "admin"))
	  flag |= _DBAdmin;
	else if (!strcmp(argv[n], "whole"))
	  oh.maph = WholeMap;
	else if (!strcmp(argv[n], "segment"))
	  oh.maph = SegmentMap;
	else
	  {
	    printf("unknown flag '%s'\n", argv[n]);
	    return 0;
	  }
      }

    if (!flag || flag == _DBOpenLocal || flag == _DBAdmin)
      flag |= _DBSRead;

    parser->setDatabase(argv[1], (Database::OpenFlag)flag, &oh);
    return 1;
  }

  static int
  setdbdmdb_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 2)
      parser->setDbmdb(argv[1]);
    else if (argc == 1)
      {
	const char *dbmdb = parser->getDbmdb();
	if (dbmdb)
	  printf("%s\n", dbmdb);
	else
	  printf("no dbmdb specified, using default\n");
      }
    else
      return 0;

    return 1;
  }

  static void
  transactionBegin(Database *db)
  {
    if (!tr_cnt)
      {
	Status status = db->transactionBegin(params);
	if (status)
	  {
	    status->print();
	    return;
	  }
      }
  }

  static void
  transactionCommit(Database *db)
  {
    if (!tr_cnt)
      db->transactionCommit();
  }

  static char print_statement[] = "%s.toString(%u)";

  static void
  percent_manage(std::string &prefix)
  {
    if (!strchr(prefix.c_str(), '%'))
      return;

    char *p = (char *)malloc(strlen(prefix.c_str())*2+1);
    char *q = p;
    const char *x = prefix.c_str();
    char c;
    while (c = *x++)
      {
	*q++ = c;

	if (c == '%')
	  *q++ = '%';
      }
    *q = 0;
    prefix = p;
    free(p);
  }

  static void
  print_oid(Database *db, const char *oidstr, unsigned int flags, const RecMode *rcm)
  {
    if (print_remote) {

      if (rcm == RecMode::FullRecurs)
	flags |= OqlCtbToStringFlags::FullRecursTrace;

      operation = idbOqlQuerying;
      OQL q(db, print_statement, oidstr, flags);
      Value v;
      Status status = q.execute(v);

      /*
	if (oql_interrupt)
	oql_sig_h(0);
      */
    
      if (status && status->getStatus() != IDB_BACKEND_INTERRUPTED)
	status->print();
      else if (!status) {
	std::string s = v.str;
	percent_manage(s);
	fflush(stdout);
	write(1, s.c_str(), strlen(s.c_str()));
      }
    }
    else {
      Oid oid(oidstr);

      if (!oid.isValid())
	return;

      Object *o = 0;
      operation = idbOqlQuerying;
      Status status;

      db->uncacheObject(oid);
      status = db->loadObject(&oid, &o, rcm);

      if (oql_interrupt)
	oql_sig_h(0);
  
      if (status)
	status->print();
      else if (o)
	{
	  status = o->trace(stdout, flags, rcm);
	  if (status && status->getStatus() != IDB_BACKEND_INTERRUPTED)
	    status->print();
	}
  
      if (o && !o->isLocked())
	o->release();
    }

    operation = idbOqlLocal;
  }

  static int
  print_command(OQLParser *parser, int argc, char *argv[])
  {
    int n;
    Database *db = parser->getDatabase(); // in fact bad: must depend on dbid in oid !!!!!

    if (!db)
      return 1;

    const RecMode *rcm;

    n = 0;
    rcm = NoRecurs;
    unsigned int flags = 0;

    print_remote = True;

    for (int i = 1; i < argc; i++)
      {
	const char *s = argv[i];
	Oid oid(s);
	if (oid.isValid())
	  {
	    n = i+1;
	    continue;
	  }

	if (!strcmp(s, "full"))
	  {
	    rcm = RecMode::FullRecurs;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "pointer"))
	  {
	    flags |= PointerTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "method"))
	  {
	    flags |= ExecBodyTrace|SysExecTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "local"))
	  {
	    print_remote = False;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "remote"))
	  {
	    print_remote = True;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "all"))
	  {
	    flags |= CompOidTrace|NativeTrace|CMTimeTrace|ExecBodyTrace|SysExecTrace|ContentsFlag|AttrCompTrace|AttrCompDetailTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "compoid"))
	  {
	    flags |= CompOidTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "native"))
	  {
	    flags |= NativeTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "ctime"))
	  {
	    flags |= CTimeTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "mtime"))
	  {
	    flags |= MTimeTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "cmtime"))
	  {
	    flags |= CMTimeTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "contents"))
	  {
	    flags |= ContentsFlag;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "attrcomp"))
	  {
	    flags |= AttrCompTrace;
	    if (!n)
	      n = i;
	  }
	else if (!strcmp(s, "attrcomp*"))
	  {
	    flags |= AttrCompDetailTrace;
	    if (!n)
	      n = i;
	  }
	else
	  return 0;
      }

    if (n <= 1)
      {
	ValueArray& value_arr = parser->getLastValueArray();
	int cnt = value_arr.getCount();
	transactionBegin(db);
	for (int j = 0; j < cnt && !oql_interrupt; j++)
	  if (value_arr[j].getType() == Value::tOid ||
	      value_arr[j].getType() == Value::tPobj) {
	    print_oid(db, value_arr[j].getString(), flags, rcm);
	    if (oql_interrupt) {
	      oql_sig_h(0);
	      break;
	    }
	  }

	transactionCommit(db);
      }
    else
      {
	transactionBegin(db);
	for (int i = 1; i < n && !oql_interrupt; i++) {
	  print_oid(db, argv[i], flags, rcm);
	  if (oql_interrupt) {
	    oql_sig_h(0);
	    break;
	  }
	}
	transactionCommit(db);
      }

    return 1;
  }

  struct Buffer {
    char *buf;
    int len;

    Buffer() {
      buf = 0;
      len = 0;
    }

    Buffer(int ilen) {
      buf = (char *)malloc(ilen);
      len = ilen;
    }

    void append(const char *s) {
      int l = strlen(s) + (buf ? strlen(buf) : 0);
      if (l >= len)
	{
	  len = l + 100;
	  buf = (char *)realloc(buf, len);
	}
      strcat(buf, s);
    }

    ~Buffer() {
      free(buf);
    }
  };

#ifdef MOZILLA
  static int
  show_command(OQLParser *parser, int argc, char *argv[])
  {
    if (!dpy)
      {
	fprintf(stderr, "show: display cannot be opened\n");
	return 1;
      }

    if (!cgi)
      {
	fprintf(stderr, "show: eyedb cgi is not specified: use the '-cgi' option or use the 'cgi' configuration variable\n");
	return 1;
      }

    if (!parser->getDatabase())
      return 1;

    Buffer buffer(1024);

    sprintf(buffer.buf,
	    "openURL(%s/cgi-bin/eyedbcgi.sh?-db&%s&-u&%s&-p&%s&-dumpgen",
	    cgi, parser->getDatabase()->getName(),
	    Connection::getDefaultUser(),
	    Connection::getDefaultPasswd());

    if (argc == 1)
      {
	ValueArray &value_arr = parser->getLastValueArray();
	int cnt = value_arr.getCount();

	if (!cnt)
	  return 1;

	for (int j = 0; j < cnt; j++)
	  if (value_arr[j].getType() == Value::tOid) {
	    buffer.append("&");
	    eyedbsm::Oid *xoid = value_arr[j].oid->getOid();
	    char oidstr[64];
#if 0
	    sprintf(oidstr, "%u.%u.%u:oid", xoid->nx, se_dbidGet(xoid),
		    xoid->unique);
	    buffer.append(oidstr);
#else
	    buffer.append(value_arr[j].getString());
#endif
	  }
      }
    else
      {
	for (int j = 1; j < argc; j++)
	  {
	    buffer.append("&");
	    buffer.append(argv[j]);
	  }
      }

    buffer.append(")");

    char *cmds[2];
    cmds[0] = buffer.buf;
    cmds[1] = NULL;
    //  mozilla_remote_commands(dpy, wid, cmds, False);
    mozilla_remote_commands(dpy, wid, cmds, True);
    return 1;
  }
#endif

  static const char *
  trs(const char *s)
  {
    static char ts[256];
    const char *p;
    char *q, c;

    p = s;
    q = ts;
    while (c = *p)
      {
	if (c == '\\' && *(p+1) == 'n')
	  {
	    *q++ = '\n';
	    p += 2;
	  }
	else
	  {
	    *q++ = c;
	    p++;
	  }
      }
    *q = 0;

    return ts;
  }

  static const char *
  untrs(const char *s)
  {
    static const int NN = 8;
    static char ts[NN][256];
    static int nts;
    const char *p;
    char *q, c;

    p = s;
    if (nts >= NN)
      nts = 0;
    q = ts[nts];
    while (c = *p++)
      {
	if (c == '\n')
	  {
	    *q++ = '\\';
	    *q++ = 'n';
	  }
	else
	  *q++ = c;
      }
    *q = 0;

    return ts[nts++];
  }

  static int
  format_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1)
      {
	printf("sep \"%s\"\nbegin \"%s\"\nend \"%s\"\nnl \"%s\"\nsplit %d\nalign %s%d\ncol %d\n",
	       untrs(parser->getSep()),
	       untrs(parser->getBindOpen()),
	       untrs(parser->getBindClose()),
	       untrs(parser->getBindNL()),
	       parser->getSplitLen(),
	       parser->getAlignLeft() ? "-" : "",
	       parser->getAlign(),
	       parser->getColumns());
	return 1;
      }
  
    const char *cmd = argv[1];
    if (!strcmp(cmd, "reset"))
      {
	parser->resetFormat();
	return 1;
      }

    if (argc != 3)
      return 0;

    if (!strcmp(cmd, "sep"))
      parser->setSep(trs(argv[2]));
    else if (!strcmp(cmd, "begin"))
      parser->setBindOpen(trs(argv[2]));
    else if (!strcmp(cmd, "end"))
      parser->setBindClose(trs(argv[2]));
    else if (!strcmp(cmd, "nl"))
      parser->setBindNL(trs(argv[2]));
    else if (!strcmp(cmd, "col"))
      {
	if (!strcmp(argv[2], "no"))
	  parser->setColumns(0);
	else
	  {
	    int n = atoi(argv[2]);
	    if (n < 1)
	      printf("more than one column is required\n");
	    else
	      parser->setColumns(n);
	  }
      }
    else if (!strcmp(cmd, "split"))
      {
	if (!strcmp(argv[2], "no"))
	  parser->setSplitLen(0);
	else
	  {
	    int n = atoi(argv[2]);
	    if (n < 20)
	      printf("split length must be longer than 20\n");
	    else
	      parser->setSplitLen(n);
	  }
      }
    else if (!strcmp(cmd, "align"))
      {
	if (!strcmp(argv[2], "no"))
	  parser->setAlign(0, 0);
	else
	  {
	    if (argv[2][0] == '-')
	      parser->setAlign(atoi(&argv[2][1]), 1);
	    else
	      parser->setAlign(atoi(argv[2]), 0);
	  }
      }
    else
      return 0;

    return 1;
  }

  static int
  mssetup_command(OQLParser *parser, int argc, char *argv[])
  {
    int n;
    Database *db = parser->getDatabase();

    if (!db)
      return 1;

    Status status = db->getSchema()->setup(True);

    if (status)
      {
	status->print();
	return 1;
      }
    return 1;
  }

  Database *
  OQLParser::getDatabase()
  {
    return db;
  }

  Connection *
  OQLParser::getConnection()
  {
    return conn;
  }

  void
  OQLParser::setDbmdb(const char *_dbmdb)
  {
    free(dbmdb);
    dbmdb = strdup(_dbmdb);
  }

  const char *
  OQLParser::getDbmdb() const
  {
    return dbmdb;
  }

  static Database *dbm;

  void
  OQLParser::closeDB()
  {
    if (!db || db == dbm)
      return;

    db->close();
    db->release();
    db = NULL;
  }

  int
  OQLParser::setDatabase(const char *name, Database::OpenFlag flag,
			 const OpenHints *oh)
  {
    operation = idbOqlCritical;

    closeDB();
    operation = idbOqlLocal;

    int dbid = atoi(name);

    if (dbid > 0)
      db = new Database(dbid, dbmdb);
    else
      db = new Database(name, dbmdb);

    Status status;
    operation = idbOqlQuerying;
    status = db->open(conn, flag, oh);
    operation = idbOqlLocal;

    if (status)
      {
	status->print();
	db->release();
	db = 0;
	return 1;
      }

    tr_cnt = 0;
    return 0;
  }

  static int
  echo_command(OQLParser *parser, int argc, char *argv[])
  {
    int nl = 1, s = 1;
    if (argc > 1 && !strcmp(argv[1], "-n"))
      {
	nl = 0;
	s = 2;
      }

    for (int i = s; i < argc; i++)
      fprintf(stdout, "%s ", argv[i]);
    if (nl)
      fprintf(stdout, "\n");
    return 1;
  }

  static int
  clear_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1)
      {
	parser->clear_oql_buffer();
	return 1;
      }
    return 0;
  }

  static int
  user_command(OQLParser *parser, int argc, char *argv[])
  {
    char user[32], passwd[32];

    if (argc > 3)
      return 0;

    if (argc < 2)
      {
	printf("user authentication: ");
	fgets(user, sizeof(user)-1, stdin);

	user[strlen(user)-1] = 0;
      }
    else
      strcpy(user, argv[1]);

    Connection::setDefaultUser(user);

    if (argc < 3)
      strcpy(passwd, getpass("password authentication: "));
    else
      strcpy(passwd, argv[2]);

    Connection::setDefaultPasswd(passwd);

    return 1;
  }

  static int
  settrmode_realize(char *argv[], int argc, TransactionParams &params)
  {
    if (argc > 6)
      return 0;

    params = TransactionParams::getGlobalDefaultTransactionParams();

    if (argc == 0)
      return 1;

    if (!strcmp(argv[0], "TRSOFF")) {
      if (argc != 1)
	return 0;
      params.trsmode = TransactionOff;
      return 1;
    }

    params.trsmode = TransactionOn;

    for (int n = 0; n < argc; n++) {
      if (!strcmp(argv[n], "R_S_W_S"))
	params.lockmode = ReadSWriteS;
      else if (!strcmp(argv[n], "R_S_W_SX"))
	params.lockmode = ReadSWriteSX;
      else if (!strcmp(argv[n], "R_S_W_X"))
	params.lockmode = ReadSWriteX;
      else if (!strcmp(argv[n], "R_SX_W_SX"))
	params.lockmode = ReadSXWriteSX;
      else if (!strcmp(argv[n], "R_SX_W_X"))
	params.lockmode = ReadSXWriteX;
      else if (!strcmp(argv[n], "R_X_W_X"))
	params.lockmode = ReadXWriteX;
      else if (!strcmp(argv[n], "R_N_W_S"))
	params.lockmode = ReadNWriteS;
      else if (!strcmp(argv[n], "R_N_W_SX"))
	params.lockmode = ReadNWriteSX;
      else if (!strcmp(argv[n], "R_N_W_X"))
	params.lockmode = ReadNWriteX;
      else if (!strcmp(argv[n], "R_N_W_N"))
	params.lockmode = ReadNWriteN;
      else if (!strcmp(argv[n], "DB_W"))
	params.lockmode = DatabaseW;
      else if (!strcmp(argv[n], "DB_RW"))
	params.lockmode = DatabaseRW;
      else if (!strcmp(argv[n], "DB_Wtrans"))
	params.lockmode = DatabaseWtrans;

      else if (!strcmp(argv[n], "RV_OFF"))
	params.recovmode = RecoveryOff;
      else if (!strcmp(argv[n], "RV_FULL"))
	params.recovmode = RecoveryFull;
      else if (!strcmp(argv[n], "RV_PARTIAL"))
	params.recovmode = RecoveryPartial;

      else if (!strncmp(argv[n], "MG=", 3))
	params.magorder = atoi(&argv[n][3]);
      else if (!strncmp(argv[n], "RT=", 3))
	params.ratioalrt = atoi(&argv[n][3]);
      else if (!strncmp(argv[n], "TM=", 3))
	params.wait_timeout = atoi(&argv[n][3]);

      else
	return 0;
    }

    return 1;
  }

  static int
  settrmode_command(OQLParser *parser, int argc, char *argv[])
  {
    if (argc == 1) {
      if (auto_trmode) {
	printf("auto ");

	if (params.trsmode == TransactionOff) {
	  printf("TRSOFF");
	  return 1;
	}

	if (params.lockmode == ReadSWriteS)
	  printf("R_S_W_S");
	else if (params.lockmode == ReadSWriteSX)
	  printf("R_S_W_SX");
	else if (params.lockmode == ReadSWriteX)
	  printf("R_S_W_X");
	else if (params.lockmode == ReadSXWriteSX)
	  printf("R_SX_W_SX");
	else if (params.lockmode == ReadSXWriteX)
	  printf("R_SX_W_X");
	else if (params.lockmode == ReadXWriteX)
	  printf("R_X_W_X");
	else if (params.lockmode == ReadNWriteS)
	  printf("R_N_W_S");
	else if (params.lockmode == ReadNWriteSX)
	  printf("R_N_W_SX");
	else if (params.lockmode == ReadNWriteX)
	  printf("R_N_W_X");
	else if (params.lockmode == ReadNWriteN)
	  printf("R_N_W_N");
	else if (params.lockmode == DatabaseW)
	  printf("DB_W");
	else if (params.lockmode == DatabaseRW)
	  printf("DB_RW");
	else if (params.lockmode == DatabaseWtrans)
	  printf("DB_Wtrans");

	if (params.recovmode == RecoveryOff)
	  printf(" RV_OFF");
	else if (params.recovmode == RecoveryFull)
	  printf(" RV_FULL");
	else if (params.recovmode == RecoveryPartial)
	  printf(" RV_PARTIAL");

	printf(" ");

	printf("MG=%u RT=%u TM=%u\n",
	       params.magorder, params.ratioalrt,
	       params.wait_timeout);
      }
      else
	printf("noauto\n");

      return 1;
    }
  
    if (!strcmp(argv[1], "noauto")) {
      if (argc != 2)
	return 0;

      auto_trmode = False;
      return 1;
    }

    if (strcmp(argv[1], "auto"))
      return 0;

    TransactionParams params;
    if (!settrmode_realize(&argv[2], argc-2, params))
      return 0;

    eyedb::params = params;

    auto_trmode = True;

    return 1;
  }

  static int
  begin_command(OQLParser *parser, int argc, char *argv[])
  {
    Database *db;

    if (!(db = parser->getDatabase()))
      {
	fprintf(stderr, "no database opened\n");
	return 1;
      }

    if (auto_trmode)
      {
	fprintf(stderr, "cannot begin transaction when auto mode is set\n");
	return 1;
      }

    TransactionParams params = eyedb::params;
#if 0
    printf("params mode %d vs. %d\n", params.trsmode, TransactionParams::getGlobalDefaultTransactionParams().trsmode);
#endif

    if (!settrmode_realize(&argv[1], argc-1, params))
      return 0;

    Status status = db->transactionBegin(params);

    if (status)
      status->print();
    else
      tr_cnt++;

    return 1;
  }

  static int
  commit_command(OQLParser *parser, int argc, char *argv[])
  {
    Database *db;

    if (argc > 1)
      return 0;

    if (!(db = parser->getDatabase()))
      {
	fprintf(stderr, "no database opened\n");
	return 1;
      }

    Status status = db->transactionCommit();

    if (status)
      status->print();
    else
      tr_cnt--;

    return 1;
  }

  static int
  copyright_command(OQLParser *parser, int argc, char *argv[])
  {
    printf( "EyeDB Object Database Management System\n");
    printf( "OQL interpreter command-line tool\n");
    printf( "Copyright (C) 1994-2018 SYSRA\n");
    printf( "\n");
    printf( "This program is free software: you can redistribute it and/or modify\n");
    printf( "it under the terms of the GNU General Public License as published by\n");
    printf( "the Free Software Foundation, either version 2 of the License, or\n");
    printf( "(at your option) any later version.\n");
    printf( "\n");
    printf( "This program is distributed in the hope that it will be useful,\n");
    printf( "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf( "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf( "GNU General Public License for more details.\n");
    printf( "\n");
    printf( "You should have received a copy of the GNU General Public License\n");
    printf( "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");

    return 1;
  }

  static int
  abort_command(OQLParser *parser, int argc, char *argv[])
  {
    Database *db;

    if (argc > 1)
      return 0;

    if (!(db = parser->getDatabase()))
      {
	fprintf(stderr, "no database opened\n");
	return 1;
      }

    Status status = db->transactionAbort();

    if (status)
      status->print();
    else
      tr_cnt--;

    return 1;
  }

  void OQLParser::add_command(OQLCommand *cmd)
  {
    if (!head)
      head = cmd;
    else
      {
	last->next = cmd;
	cmd->prev = last;
      }

    last = cmd;
  }

  OQLCommand *OQLParser::find_command(const char *name) const
  {

    OQLCommand *cmd = head;

    while (cmd)
      {
	int name_cnt;
	const char **names = cmd->getNames(name_cnt);
	for (int i = 0; i < name_cnt; i++)
	  if (!strcmp(names[i], name))
	    return cmd;
	cmd = cmd->next;
      }
    return 0;
  }

  OQLCommand *OQLParser::get_first_command() const
  {
    return head;
  }

  OQLCommand *OQLParser::get_next_command(OQLCommand *cmd) const
  {
    return cmd->next;
  }


  void OQLParser::set_standard_command()
  {
    const char *names[8];
    add_command(new OQLCommand("abort", abort_command, abort_usage, abort_help));
    add_command(new OQLCommand("begin", begin_command, begin_usage, begin_help));
    add_command(new OQLCommand("break", break_command, break_usage, break_help));
    add_command(new OQLCommand("clear", clear_command, clear_usage, clear_help));
    add_command(new OQLCommand("commit", commit_command, commit_usage, commit_help));
    add_command(new OQLCommand("commitonclose", commitonclose_command,
			       commitonclose_usage, commitonclose_help));
    add_command(new OQLCommand("copyright", copyright_command, copyright_usage, copyright_help));
    add_command(new OQLCommand("echo", echo_command, echo_usage, echo_help));
    //  add_command(new OQLCommand("format", format_command, format_usage, format_help));
    //add_command(new OQLCommand("help", help_command, help_usage, help_help));
    names[0] = "help"; names[1] = "h";
    add_command(new OQLCommand(names, 2, help_command, help_usage, help_help));
    //  add_command(new OQLCommand("mssetup", mssetup_command, mssetup_usage, mssetup_help));
    add_command(new OQLCommand("onerror", onerror_command,
			       onerror_usage, onerror_help));
    add_command(new OQLCommand("open", open_command, open_usage, open_help));
    names[0] = "print"; names[1] = "p";
    add_command(new OQLCommand(names, 2, print_command, print_usage, print_help));
    names[0] = "quit"; names[1] = "q";
    add_command(new OQLCommand(names, 2, quit_command, quit_usage, quit_help));
    add_command(new OQLCommand("read", read_command, read_usage, read_help));
    //  add_command(new OQLCommand("setdbmdb", setdbdmdb_command, setdbmdb_usage, setdbmdb_help));
    names[0] = "schema"; names[1] = "s";
    add_command(new OQLCommand(names, 2, schema_command, schema_usage, schema_help));
    add_command(new OQLCommand("setecho", setecho_command, setecho_usage, setecho_help));
    //add_command(new OQLCommand("setechofound", setechofound_command, setechofound_usage, setechofound_help));
    add_command(new OQLCommand("setescapechar", setescapechar_command, setescapechar_usage, setescapechar_help));
    add_command(new OQLCommand("setmute", setmute_command, setmute_usage, setmute_help));
    add_command(new OQLCommand("setprint", setprint_command, setprint_usage, setprint_help));
    add_command(new OQLCommand("setprompt", setprompt_command,
			       setprompt_usage, setprompt_help));
    add_command(new OQLCommand("settermchar", settermchar_command, settermchar_usage, settermchar_help));
    add_command(new OQLCommand("settrmode", settrmode_command, settrmode_usage, settrmode_help));
    names[0] = "shell"; names[1] = "!";
    add_command(new OQLCommand(names, 2, shell_command, shell_usage, shell_help));
#ifdef MOZILLA
    add_command(new OQLCommand("show", show_command, show_usage, show_help));
    add_command(new OQLCommand("setshow", setshow_command, setshow_usage, setshow_help));
#endif
    add_command(new OQLCommand("time", time_command, time_usage, time_help));
    add_command(new OQLCommand("user", user_command, user_usage, user_help));
  }

  void OQLParser::setPrompt(const char *prompt)
  {
    delete prompt_str;
    prompt_str = strdup(prompt);
  }

  void OQLParser::setSecondPrompt(const char *prompt)
  {
    delete second_prompt_str;
    second_prompt_str = strdup(prompt);
  }

  void OQLParser::setEscapeChar(char esc)
  {
    escape_char = esc;
  }

  void OQLParser::setTermChar(char term)
  {
    term_char = term;
  }

  const char *OQLParser::getPrompt() const
  {
    return prompt_str;
  }

  const char *OQLParser::getSecondPrompt() const
  {
    return second_prompt_str;
  }

  char OQLParser::getEscapeChar() const
  {
    return escape_char;
  }

  char OQLParser::getTermChar() const
  {
    return term_char;
  }

  static int is_empty_buf(const char *s)
  {
    if (!s) return 1;
    char c;
    while (c = *s++)
      if (c != ' ' && c != '\t' && c != '\n')
	return 0;

    return 1;
  }

  void OQLParser::prompt(FILE *fd)
  {
    operation = idbOqlLocal;
    oql_interrupt = False;
    setBackendInterrupt(False);

#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
    if (fd != stdout)
      fprintf(fd, getEffectivePrompt());
#else
    fprintf(fd, getEffectivePrompt());
#endif    
  }

  const char *OQLParser::getEffectivePrompt() const
  {
    if (oql_buffer_len && !is_empty_buf(oql_buffer))
      return getSecondPrompt();
    return getPrompt();
  }

  int OQLParser::parse(FILE *fd, Bool mode)
  {
    char linebuf[4096];
    char *line;

#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
    int use_readline = fd == stdin && isatty( STDIN_FILENO);

    if (use_readline)
      line = readline( getEffectivePrompt());
    else
      line = fgets(linebuf, sizeof(linebuf)-1, fd);
#else
    line = fgets(linebuf, sizeof(linebuf)-1, fd);
#endif
      
    if (line) {
      int ret = parse(line, mode);

#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
      if (use_readline) {
	add_history( line);
	free( line);
      }
#endif

      if (!ret)
	return 0;
      return 1;
    } else if (!oql_interrupt) {
      if (fd == stdin) {
	printf("\n");
	quit_command(this, 0, 0);
      }
    }
  
    return 0;
  }

  /*
    static Bool cplus_comments = False;
    static Bool c_comments = False;
  */

  void OQLParser::clear_oql_buffer()
  {
    //  c_comments = False;
    //  cplus_comments = False;
    oql_buffer_len = 0;
  }

  void OQLParser::last_reset()
  {
    last_cnt = 0;
  }

  void
  OQLParser::setLastValueArray(OQL &q)
  {
    q.getResult(value_arr);
  }

  ValueArray &OQLParser::getLastValueArray()
  {
    return value_arr;
  }

  static Bool
  is_comment(const char *str)
  {
    char c;
    while((c = *str) == ' ' || c == '\t' || c == '\n' || c == ';')
      str++;

    if (!c)
      return True;

    c = *str++;

    if (c == '/') {
      c = *str++;
    
      if (c == '/') {
	while ((c = *str) != '\n')
	  str++;
	return is_comment(str);
      }
    }

    return False;
  }

#define max(x, y) ((x) > (y) ? (x) : (y))

  Database *
  OQLParser::openDBM()
  {
    if (!dbm)
      {
	dbm = new Database(DBM_Database::getDbName());
	Status s = dbm->open(conn, Database::DBSRead);
	if (s)
	  {
	    s->print();
	    dbm->release();
	    dbm = 0;
	  }			    
      }

    return dbm;
  }

  static const int TempFileManager_maxfiles = 10;

  class TempFileManager {
    LinkedList flist;
    char *dir, *prefix;
    static TempFileManager instance;
    void garbage() {
      int cnt = flist.getCount() - TempFileManager_maxfiles;
      for (int n = 0; n < cnt; n++) {
	char *file = (char *)flist.getFirstObject();
	unlink(file);
	free(file);
	flist.deleteObject(0);
      }
    }

  public:
    TempFileManager(const char *_dir = "/var/tmp",
		    const char *_prefix = "oql") :
      dir(strdup(_dir)), prefix(strdup(_prefix)) {}

    const char *getTempFile() {
      if (flist.getCount() > TempFileManager_maxfiles)
	garbage();

      char *file = tempnam(dir, prefix);
      flist.insertObject(file);
      return file;
    }

    static TempFileManager &getInstance() {return instance;}

    ~TempFileManager() {
      LinkedListCursor c(flist);
      char *file;
      while (c.getNext((void *&)file)) {
	unlink(file);
	free(file);
      }

      free(dir);
      free(prefix);
    }
  };

  TempFileManager TempFileManager::instance;

  static void
  rvalue_manage(OQLParser *parser, const Value &v)
  {
    bool done = false;

    if (v.type == Value::tStruct) {
      Value::Struct *stru = v.stru;
      if (stru->attr_cnt == 2) {
	Value::Attr *content_type_a = stru->attrs[0];
	Value::Attr *content_a = stru->attrs[1];
	if (!strcasecmp(content_type_a->name, "content_type") &&
	    !strcasecmp(content_a->name, "content") &&
	    content_type_a->value->getType() == Value::tString &&
	    content_a->value->getType() == Value::tString &&
	    !strcasecmp(content_type_a->value->str, "text/html")) {
	  const char *file = TempFileManager::getInstance().getTempFile();
	  int fd = creat(file, 0666);
	  if (fd >= 0) {
	    const char *content = content_a->value->str;	    
	    write(fd, content, strlen(content));
	    close(fd);
	    //write(1, content, strlen(content));
	    char *cmds[2];
	    std::string s = std::string("openURL(") + file + ")";
	    cmds[0] = strdup(s.c_str());
	    cmds[1] = 0;
#ifdef MOZILLA
	    mozilla_remote_commands(dpy, wid, cmds, True);
#endif
	    free(cmds[0]);
	    done = true;
	  }
	}
      }
    }

    if (!done && v.type != Value::tNil) { // added the 12/01/00
      if (!parser->getMuteMode()) {
	printf("= ");
	v.print(stdout);
	printf("\n");
      }
    }
  }

  Status OQLParser::oql_send()
  {
    if (is_comment(oql_buffer)) {
      clear_oql_buffer();
      return Success;
    }

    if (!db)
      db = openDBM();

    if (!db) {
      usage(getEscapeChar());
      clear_oql_buffer();
      return Success;
    }

    last_reset();
    operation = idbOqlQuerying;
    Status status;

    if (!tr_cnt) {
      if (auto_trmode) {
	status = db->transactionBegin(params);
	  
	if (status) {
	  oql_buffer_len = 0;
	  status->print();
	  return status;
	}
	tr_cnt++;
      }
    }

    std::string oql_send_buffer = oql_buffer;
    percent_manage(oql_send_buffer);

    /*
      if (getEchoMode())
      printf("%s", oql_send_buffer.c_str());
    */

    OQL q(db, oql_send_buffer.c_str());

    if (oql_interrupt) {
      oql_sig_h(0);
      oql_buffer_len = 0;
      return Success;
    }

    Bool valid = False;

    char fmt[64];

    if (align)
      sprintf(fmt, "%%%s%ds", align_left ? "-" : "", align);
    else
      sprintf(fmt, "%%s");

    int ccol = -1;
    int sep_len = strlen(sep);

    status = q.execute(query_value);
  
    operation = idbOqlScanning;

    if (!status) {
      rvalue_manage(this, query_value);
    }
    else {
      if (curfile)
	fprintf(stderr, "file \"%s\" ", curfile);

      fprintf(stderr, "near line %d: ", errorline+1);

      if (getOnErrorEcho()) {
	char *p = strrchr(oql_buffer, '\n');
	if (p) *p = 0;
	p = strrchr(oql_buffer, ';');
	if (p) *p = 0;
	p = oql_buffer;
	while (*p == ' ' || *p == '\t' || *p == '\n') p++;
	fprintf(stderr, "'%s' => ", p);
      }

      status->print();

      if (getOnErrorAbort()) {
	fprintf(stderr, "aborting current transaction\n");
	tr_cnt--;
	db->transactionAbort();
      }

      if (getOnErrorQuit()) {
	//fprintf(stderr, "exiting eyedboql\n");
	exit(1);
      }
    }

    setLastValueArray(q);

    if (status)
      clear_oql_buffer();
    else if (getPrintMode() == fullRecurs) {
      char *argv[2];
      argv[0] = (char*)"print";/*@@@@warning cast*/
      argv[1] = (char*)"full";/*@@@@warning cast*/
      print_command(this, 2, argv);
    }
    else if (getPrintMode() == noRecurs) {
      char *argv[1];
      argv[0] = (char*)"print";/*@@@@warning cast*/
      print_command(this, 1, argv);
    }

#ifdef MOZILLA
    if (getShowMode()) {
      char *argv[1];
      argv[0] = "show";
      show_command(this, 1, argv);
    }
#endif

    oql_buffer_len = 0;
    errorline = curline;
    return status;
  }

  Bool
  is_balanced(const char *s)
  {
    Bool cplus_comments = False;
    Bool c_comments = False;
    int par_cnt = 0, acc_cnt = 0, brk_cnt = 0, quote_cnt = 0;
    char c;
    int backslash = 0;

    while(c = *s++) {
      if (cplus_comments) {
	if (c == '\n')
	  cplus_comments = False;
      }
      else if (c_comments) {
	if (c == '*') {
	  c = *s;
	  if (!c) break;

	  if (c == '/') {
	    c_comments = False;
	    s++;
	  }
	}
      }

      else if (c == '\\') backslash = 1;

      else if (c == '"' && !backslash) quote_cnt = !quote_cnt;

      else if (c == '(' && !quote_cnt && !backslash) par_cnt++;
      else if (c == ')' && !quote_cnt && !backslash) par_cnt--;

      else if (c == '[' && !quote_cnt && !backslash) brk_cnt++;
      else if (c == ']' && !quote_cnt && !backslash) brk_cnt--;

      else if (c == '{' && !quote_cnt && !backslash) acc_cnt++;
      else if (c == '}' && !quote_cnt && !backslash) acc_cnt--;

      else if (c == '/' && !quote_cnt) {
	c = *s;
	if (!c)
	  break;

	if (c == '*') {
	  c_comments = True;
	  s++;
	}
	else if (c == '/') {
	  cplus_comments = True;
	  s++;
	}
      }
    
      if (c != '\\')
	backslash = 0;
    }

#if 1
    brk_cnt = 0;
#endif
    return !c_comments && !cplus_comments && par_cnt <= 0 &&
      acc_cnt <= 0 && brk_cnt <= 0 && !quote_cnt ? True : False;
  }

  Status OQLParser::oql_make(const char *buf)
  {
    if (!oql_buffer_len && is_empty_buf(buf))
      return Success;

    int len = strlen(buf) + 1;

    operation = idbOqlLocal;
    oql_interrupt = False;

    if (len + oql_buffer_len >= oql_buffer_alloc) {
      oql_buffer_alloc = len + oql_buffer_len + 120;
      oql_buffer = (char *)realloc(oql_buffer, oql_buffer_alloc);
    }

    oql_buffer[oql_buffer_len] = 0;
    strcat(oql_buffer, buf);
    oql_buffer_len = strlen(oql_buffer);
    if (!oql_buffer_len)
      return Success;

    char *p = &oql_buffer[oql_buffer_len-1];
    len = oql_buffer_len;

    char c;
    while ((c = *p) == ' ' || c == '\t' || c == '\n')
      p--;

    if (term_char == '\n') {
      if (c != '\\') {
	strcat(oql_buffer, ";");
	return oql_send();
      }
      return Success;
    }
  
    if (*p == term_char && is_balanced(oql_buffer))
      return oql_send();

    return Success;
  }

  int OQLParser::parse(const char *str, Bool echo)
  {
    curline++;
    char *buf = strdup(str);
    int isescape;
    int argc;
    char *argv[4096];

    if (echo) {
      prompt(stdout);
      printf("%s", str);
    }

    argc = buffer_parse(buf, argv, sizeof(argv)/sizeof(argv[0]), isescape);

    if (argc < 0)
      fprintf(stderr, "eyedboql: command too long\n");
    else {
      if (isescape) {
	const char *comname = argv[0]; 
	if (!comname)
	  comname = "";
	OQLCommand *com = find_command(comname);
	if (!com) {
	  fprintf(stderr, "eyedboql: unknown command `%c%s', type `%chelp' to display the command list.\n", getEscapeChar(), comname, getEscapeChar());
	  free(buf);
	  return 0;
	}

	if (!(*com->exec)(this, argc, argv)) {
	  //	fprintf(stderr, "usage: %c", getEscapeChar());
	  fprintf(stderr, "usage: ", getEscapeChar());
	  (*com->usage)(stderr, com, false);
	}
      }
      else {
	// send to OQL parser: add buf to oql buffer while last char !';'
	Status s;
	if ((s = oql_make(str)) != Success) {
	  free(buf);
	  return 0;
	}
      }
    
    }

    if (is_empty_buf(oql_buffer))
      errorline = curline;
    free(buf);
    return 1;
  }

  Bool OQLParser::getExitMode() const
  {
    return exit_mode;
  }

  void OQLParser::setExitMode(Bool _exit_mode)
  {
    exit_mode = _exit_mode;
  }

  int
  OQLParser::buffer_parse(char *buf, char **words, int maxwords, int &isescape)
  {
    char *p = buf;
    char c;
    int empt = 1;
    int nw = 0;
    int isquote = 0;

    while ((c = *p) == ' ' || c == '\t')
      p++;

    isescape = (c == escape_char);

    if (isescape)
      p++;

    while (c = *p)
      {
	switch(c)
	  {
	  case ' ':
	  case '\t':
	  case '\n':
	    if (!isquote)
	      {
		empt = 1;
		*p++ = 0;
	      }
	    else
	      p++;
	    break;

	  case '\"':
	    if (isescape)
	      {
		if (!isquote)
		  {
		    isquote = 1;
		    empt = 1;
		    p++;
		    if (*p == '\"')
		      *p = 0;
		  }
		else
		  {
		    *p++ = 0;
		    isquote = 0;
		    break;
		  }
	      }

	  default:
	    if (empt)
	      {
		empt = 0;
		if (nw >= maxwords)
		  return -1;
		else
		  words[nw++] = p;
	      }
	    p++;
	    break;
	  }
      }

    *p = 0;
    words[nw] = 0;
    return nw;
  }
}
