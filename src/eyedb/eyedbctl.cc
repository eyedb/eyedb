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


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <eyedb/eyedb.h>
#include <eyedbsm/smd.h>
#include <eyedblib/strutils.h>
#include "SessionLog.h"
#include "comp_time.h"
#include "compile_builtin.h"

using namespace eyedb;
using namespace std;

namespace eyedb {
  extern void printVersion();
}

static int
usage(const char *prog)
{
  cerr << "usage: " << prog << " ";
  cerr << "[-h|--help|-v|--version] [start|stop|status] [-f] [--creating-dbm] [--allow-root] ";
  print_common_usage(cerr, true);
  cerr << endl;
  return 1;
}

static void help(const char *prog)
{
  usage(prog);
  cerr << "\nProgram Options:\n";
  cerr << "  start [-f]     Launch the server. Set -f to force launch\n";
  cerr << "  stop [-f]      Stop the server. Set -f to force stop\n";
  cerr << "  status         Display status on the running server\n";
  cerr << "  --creating-dbm Bootstrap option for DBM database creation\n";
  cerr << "  --allow-root   Allow running as root (not allowed by default)\n";
  print_common_help(cerr, true);
}

static void
check(const char *opt, int i, int argc)
{
  if (i >= argc - 1) {
    fprintf(stderr, "eyedb options: missing argument after '%s'\n", opt);
    exit(1);
  }
}

enum Command {
  Start = 1,
  Stop,
  CStatus
};

static void
ld_libpath_manage()
{
  static const char ld_libpath_env[] = "LD_LIBRARY_PATH";
  char *ld_library_path = getenv(ld_libpath_env);
  char env[2048];
  sprintf(env, "%s=%s%s%s", ld_libpath_env,
	  eyedb::ServerConfig::getSValue("sopath"),
	  (ld_library_path ? ":" : ""),
	  (ld_library_path ? ld_library_path : ""));
  putenv(strdup(env));
}

static const char *sbindir;
#define PIPE_SYNC

extern char **environ;

static int
execute(const char *prog, const char *arg, Bool pipes)
{
  int pid;

#ifdef PIPE_SYNC
  int pfd[2];
  if (pipes && pipe(pfd) < 0) {
    fprintf(stderr, "eyedbctl: cannot create unnamed pipes\n");
    return 1;
  }
#endif

  if ((pid = fork()) == 0) {
    std::string cmd = std::string(sbindir) + "/" + prog;
    char *argv[3];

    argv[0] = (char *)prog;
    argv[1] = (char *)arg;
    argv[2] = 0;
#ifdef PIPE_SYNC
    if (pipes) {
      close(pfd[0]);
      putenv(strdup((std::string("EYEDBPFD=") + str_convert((long)pfd[1])).c_str()));
    }
#endif
    if (execve(cmd.c_str(), argv, environ) < 0) {
      fprintf(stderr, "eyedbctl: cannot execute '%s'\n", cmd.c_str());
      return -1;
    }
  }

#ifdef PIPE_SYNC
  if (pipes) {
    int status = 0;
    if (read(pfd[0], &status, sizeof(status)) != sizeof(status)) {
      fprintf(stderr, "eyedbctl: eyedbd smd start failed\n");
      return 0;
    }
      
    if (status)
      return -1;
  }
#endif
  return pid;
}

static int
startServer(int argc, char *argv[], const char *smdport)
{
#ifdef HAVE_EYEDBSMD
  smdcli_conn_t *conn = smdcli_open(smd_get_port());
  if (conn) {
    if (smdcli_declare(conn) < 0)
      return 1;
    smdcli_close(conn);
    conn = 0;
  }
  else if (execute("eyedbsmd", (std::string("--port=") + smdport).c_str(), True) < 0)
    return 1;
#endif

#ifdef PIPE_SYNC
  int pfd[2];
  if (pipe(pfd) < 0) {
    fprintf(stderr, "eyedbctl: cannot create unnamed pipes\n");
    return 1;
  }
#endif

  int pid;

  if ((pid = fork()) == 0) {
    ld_libpath_manage();
    std::string cmd = std::string(sbindir) + "/eyedbd";
    std::string s = std::string("eyedbd");

    argv[0] = (char *)s.c_str();
#ifdef PIPE_SYNC
    close(pfd[0]);
    putenv(strdup((std::string("EYEDBPFD=") + str_convert((long)pfd[1])).c_str()));
#endif
    if (execve(cmd.c_str(), argv, environ) < 0) {
      kill(getppid(), SIGINT);
      fprintf(stderr, "eyedbctl: cannot execute '%s'\n", cmd.c_str());
    }

    exit(1);
  }

  if (pid < 0)
    return 1;

  close(pfd[1]);
  int status = 0;

#ifdef PIPE_SYNC
  if (read(pfd[0], &status, sizeof(status)) != sizeof(status)) {
    fprintf(stderr, "eyedbctl: eyedbd daemon start failed\n");
    return 1;
  }

  if (!status)
    fprintf(stderr, "Starting EyeDB Server\n"
	    " Version      V%s\n"
	    " Compiled     %s\n"
	    " Architecture %s\n"
	    " Program Pid  %d\n",
	    eyedb::getVersion(),
	    getCompilationTime(),
	    Architecture::getArchitecture()->getArch(),
	    pid);
#endif

  return status;
}

static void unlink_port(const char *port)
{
  if (*port == '/' || strchr(port, ':') == 0) {
    unlink(port);
  }
}

static void unlink_ports(const char *smdport, const char *_listen)
{
  char *listen = strdup(_listen);
  char *p = listen;

  for (;;) {
    char *q = strchr(p, ',');
    if (q)
      *q = 0;
    unlink_port(p);
    if (!q)
      break;
    p = q + 1;
  }

#ifdef HAVE_EYEDBSMD
  unlink(smdport);
#endif
}

static void make_host_port(const char *_listen, const char *&host,
			   const char *&port)
{
  char *listen = strdup(_listen);

  char *p;
  if (p = strchr(listen, ','))
    *p = 0;

  if (p = strchr(listen, ':')) {
    *p = 0;
    host = listen;
    port = p+1;
  }
  else {
    host = "localhost";
    port = listen;
  }
}

static int checkPostInstall(Bool creatingDbm)
{
  if (!creatingDbm) {
    const char *dbm = Database::getDefaultServerDBMDB();
    if (!dbm || access(dbm, R_OK)) {
      fprintf(stderr, "\nThe EYEDBDBM database file '%s' is not accessible\n", dbm);
      fprintf(stderr, "Did you run the post install script 'eyedb-postinstall.sh' ?\n");
      fprintf(stderr, "If yes, check your configuration or launch eyedbctl with option --creating-dbm\n");
      return 1;
    }
  }

  return 0;
}

static int checkRoot( Bool allowRoot)
{
  if (!allowRoot) {
    if (!getuid()) {
      fprintf(stderr, "\nLaunching EyeDB server as root is not allowed for security.\n");
      fprintf(stderr, "You can overide this by launching eyedbctl with option --allow-root\n");
      return 1;
    }
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  const char *listen, *smdport, *s;
  eyedbsm::Status status;
  Bool force = False;
  Bool creatingDbm = False;
  Bool allowRoot = False;
  string sv_host, sv_port;

  string sv_listen;
  try {
    if (argc < 2)
      return usage(argv[0]);

    Command cmd;

    if (!strcmp(argv[1], "start"))
      cmd = Start;
    else if (!strcmp(argv[1], "stop"))
      cmd = Stop;
    else if (!strcmp(argv[1], "status"))
      cmd = CStatus;
    else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
      help(argv[0]);
      return 0;
    }
    else if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
      eyedb::printVersion();
      return 0;
    }
    else
      return usage(argv[0]);

    eyedb::init(argc, argv, &sv_listen, false);

    listen = sv_listen.c_str();

    eyedb::Exception::setMode(eyedb::Exception::StatusMode);

    for (int i = 2; i < argc; i++) {
      char *s = argv[i];
      if (!strcmp(s, "-f"))
	force = True;
      else if (!strcmp(s, "--creating-dbm"))
	creatingDbm = True;
      else if (!strcmp(s, "--allow-root"))
	allowRoot = True;
      else
	break;
    }

    if (!*listen && (s = eyedb::ServerConfig::getSValue("listen")))
      listen = s;

    smdport = smd_get_port();

    if (cmd == Start && force) {
      unlink_ports(smdport, listen);
    }

    sbindir = ServerConfig::getSValue("@sbindir");

    int ac;
    char **av;

    if (cmd == Start) {
      if (checkRoot( allowRoot))
	return 1;

      if (checkPostInstall(creatingDbm))
	return 1;

      int st = force + creatingDbm + allowRoot;

      ac = argc - st;
      av = &argv[st+1];
    }

    const char *host, *port;
    make_host_port(listen, host, port);
    
    SessionLog sesslog(host, port, eyedb::ServerConfig::getSValue("tmpdir"),
		       True);
		       //(cmd == CStatus ? False : True));


    if (sesslog.getStatus()) {
      if (cmd == Start)
	return startServer(ac, av, smdport);
      if (sesslog.getStatus()->getStatus() == IDB_CONNECTION_LOG_FILE_ERROR) {
	cerr << "No EyeDB Server is running on " << host << ":" << port << endl;
	return 1;
      }
      sesslog.getStatus()->print();
      return 1;
    }

    if (cmd == Start)
      return startServer(ac, av, smdport);

    if (cmd == Stop) {
      Status status = sesslog.stopServers(force);
      if (status) {
	status->print();
	return 1;
      }

#ifndef HAVE_EYEDBSMD
      return 0;
#else
      smdcli_conn_t *conn = smdcli_open(smd_get_port());
      if (!conn) {
	fprintf(stderr, "cannot connect to eyedbsmd daemon\n");
	return 1;
      }
    
      int r = smdcli_stop(conn);
      smdcli_close(conn);
      conn = 0;
      return r;
#endif
    }

    sesslog.display(stdout, force);
  }
  catch(Exception &e) {
    cerr << e << flush;
    return 1;
  }

  return 0;
}
