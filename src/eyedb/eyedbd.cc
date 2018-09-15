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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <eyedb/eyedb.h>
#include "base_p.h"
#include "eyedb/internals/ObjectHeader.h"
#include <eyedblib/rpc_lib.h>
#include <eyedbsm/smd.h>
#include "SessionLog.h"
#include <eyedblib/connman.h>
#include "eyedblib/log.h"
#include "eyedblib/butils.h"
#include "eyedb/Log.h"
#include "GetOpt.h"

#include "serv_lib.h"
#include "kernel.h"

//#define RPC_TIMEOUT

using namespace std;

namespace eyedb {
  extern void (*garbage_handler)(void);
  extern void printVersion();
}

#ifdef RPC_TIMEOUT
extern void (*settimeout)(int);
#endif
namespace eyedbsm {
  extern Boolean backend;
}

using namespace eyedb;

static Bool nod;

//#define NO_DATDIR

static int
get_opts(int argc, char *argv[],
	 const char **accessfile,
	 const char **datdir,
	 const char **sesslogdev,
	 int *sessloglevel,
	 int *nofork)
{
  int i;
  const char *s;

  *accessfile = eyedb::ServerConfig::getSValue("access_file");
  *nofork = 0;
  *sesslogdev = 0;
  *sessloglevel = 0;

#ifndef NO_DATDIR
  *datdir = 0;
#else
  *datdir = "";
#endif

  static const std::string access_file_opt = "access-file";
  static const std::string datdir_opt = "databasedir";
  static const std::string nod_opt = "nod";
  static const std::string sesslogdev_opt = "sesslogdev";
  static const std::string sessloglevel_opt = "sessloglevel";
  static const std::string help_opt = "help";

  Option opts[] = {
    Option(access_file_opt, OptionStringType(),
	   Option::MandatoryValue, OptionDesc("Access file", "ACCESS_FILE")),
#ifndef NO_DATDIR
    Option(datdir_opt, OptionStringType(),
	   Option::MandatoryValue, OptionDesc("Default datafile directory", "DATDIR")),
#endif
    Option(nod_opt, OptionBoolType(),
	   0, OptionDesc("No daemon: does not close fd 0, 1 & 2")),
    Option('h', help_opt, OptionBoolType(),
	   0, OptionDesc("Display this message"))
  };

  GetOpt getopt(argv[0], opts, sizeof(opts)/sizeof(opts[0]));

  if (!getopt.parse(argc, argv)) {
    print_standard_usage(getopt, "", std::cerr, true);
    //getopt.usage("\n");
    return 1;
  }

  GetOpt::Map &map = getopt.getMap();

  if (map.find(help_opt) != map.end()) {
    print_standard_help(getopt, vector<string>(), std::cerr, true);
    //getopt.help(cerr, "  ");
    return 1;
  }

  if (map.find(access_file_opt) != map.end())
    *accessfile = strdup(map[access_file_opt].value.c_str());

#ifndef NO_DATDIR
  if (map.find(datdir_opt) != map.end())
    *datdir = strdup(map[datdir_opt].value.c_str());
#endif

  if (map.find(nod_opt) != map.end())
    nod = True;

  if (map.find(sesslogdev_opt) != map.end())
    *sesslogdev = strdup(map[sesslogdev_opt].value.c_str());

  if (map.find(sessloglevel_opt) != map.end())
    *sessloglevel = ((OptionIntType *)map[sessloglevel_opt].type)->
      getIntValue(map[sessloglevel_opt].value);

#ifndef NO_DATDIR
  if (!*datdir) {
    if (s = eyedb::ServerConfig::getSValue("databasedir"))
      *datdir = strdup(s);
    else {
      fprintf(stderr, "configuration variable databasedir is not set\n");
      exit(1);
    }
  }
#endif

  return 0;
}

static SessionLog *sesslog;

void rpc_unlink_socket();

static void
_quit_handler(void *xpid, int)
{
  if (*(int *)xpid == rpc_getpid()) {
    sesslog->remove();
    rpc_unlink_socket();
  }
}

static int
notice(int status)
{
  const char *s;
  if (s = getenv("EYEDBPFD")) {
    int pfd = atoi(s);
    if (pfd > 0) {
      write(pfd, &status, sizeof(status));
      close(pfd);
    }
  }

  return status;
}

static unsigned int get_ports(const char *ports[], const char *hosts[],
			      const char *_listen)
{
  if (!_listen)
    return 0;

  char *listen = strdup(_listen);
  unsigned int nlisten = 0;
  for (;;) {
    ports[nlisten++] = listen;
    listen = strchr(listen, ',');
    if (!listen)
      break;
    *listen++ = 0;
  }

  for (int n = 0; n < nlisten; n++) {
    const char *p = strchr(ports[n], ':');
    if (p) {
      *(char *)p = 0;
      hosts[n] = ports[n];
      ports[n] = p + 1;
    }
    else {
      hosts[n] = "localhost";
    }
  }
	

  return nlisten;
}

int
main(int argc, char *argv[])
{
  rpc_Server *server;
  rpc_PortHandle *port[32];
  const char *ports[32];
  const char *hosts[32];
  eyedbsm::Status status;
  const char *hostname;
  const char *accessfile, *datdir;
  const char *listen;
  const char *sesslogdev;
  int sessloglevel;
  unsigned int nlisten;

  int nofork;

  string sv_listen;

  try {
    eyedb::init(argc, argv, &sv_listen, true);

    listen = sv_listen.c_str();

    server = rpcBeInit();

    garbage_handler = GARBAGE;
#ifdef RPC_TIMEOUT
    settimeout = rpc_setTimeOut;
#endif

    eyedbsm::backend = eyedbsm::True;

    if (get_opts(argc, argv, &accessfile, &datdir, &sesslogdev,
		 &sessloglevel, &nofork))
      return notice(1);

    rpc_setProgName(argv[0]);

    if (!*listen)
      listen = eyedb::ServerConfig::getSValue("listen");

    nlisten = get_ports(ports, hosts, listen);

    for (int n = 0; n < nlisten; n++) {
      if (!eyedblib::is_number(ports[n]) && strcmp(hosts[n], "localhost")) {
	fprintf(stderr, "eyedbd: cannot specify host '%s' for named pipe '%s'\n",
		hosts[n], ports[n]);
	return 1;
      }
    }

    if (rpc_connman_init(accessfile))
      return notice(1);

    if (!nlisten) {
      fprintf(stderr, "eyedbd: at least one port must be set.\n");
      return notice(1);
    }

    for (int i = 0; i < nlisten; i++)
      if (rpc_portOpen(server, hosts[i], ports[i], &port[i]))
	return notice(1);

    int *pid = new int;
    *pid = getpid();
    rpc_setQuitHandler(_quit_handler, pid);

    const char *sv_tmpdir = eyedb::ServerConfig::getSValue("tmpdir");

    if (!sv_tmpdir)
      sv_tmpdir = "/tmp";

    std::string logdir = sv_tmpdir;

    eyedb::Exception::setMode(eyedb::Exception::StatusMode);

    sesslog = new SessionLog(logdir.c_str(), eyedb::getVersion(),
			     nlisten, hosts, ports,
			     datdir, sesslogdev, sessloglevel);

#ifndef NO_DATDIR
    IDB_init(datdir, 0 /* passwdfile */, sesslog, 0 /*timeout*/);
#else
    IDB_init(0, 0 /* passwdfile */, sesslog, 0 /*timeout*/);
#endif

    if (sesslog->getStatus()) {
      sesslog->getStatus()->print();
      return notice(1);
    }

    (void)notice(0);

    if (!nod) {
      close(0);
      if (!Log::getLog() || strcmp(Log::getLog(), "stdout"))
	close(1);
      if (!Log::getLog() || strcmp(Log::getLog(), "stderr"))
	close(2);
    }

    rpc_serverMainLoop(server, port, nlisten);
  }
  catch(Exception &e) {
    cerr << e << flush;
    return 1;
  }
  catch(int status) {
    return status;
  }
  return 0;
}
