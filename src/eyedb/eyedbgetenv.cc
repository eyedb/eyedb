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


#include <eyedb/eyedb.h>
#include "eyedb/DBM_Database.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <GetOpt.h>
#include <sys/stat.h>

using namespace eyedb;

Bool _server = False;

static int
usage(const char *prog)
{
  fprintf(stderr, "usage: %s [--server] [--config|--csh|--sh] [--export] [--expand-user] [--expand-vars] [VARIABLES]\n", prog);
  print_use_help(std::cerr);
  return 1;
}

static std::string
capstring(const char *s)
{
  char *sx = (char *)malloc(strlen(s)+1);
  char *x = sx, c;

  while (c = *s++)
    *x++ = (c >= 'a' && c <= 'z') ? c + 'A' - 'a' : c;
  *x = 0;
  std::string rs = sx;
  free(sx);
  return rs;
}

int
main(int argc, char *argv[])
{
  for (int n = 1; n < argc; n++) {
    const char *s = argv[n];

    if (!strcmp(s, "--server")) {
      _server = True;
      break;
    }
  }

  try {
    if (_server) {
      std::string sv_listen;
      eyedb::init(argc, argv, &sv_listen, true);
    }
    else
      eyedb::init(argc, argv);

    if (argc < 2)
      return usage(argv[0]);

    LinkedList list;
    Bool shell, C_shell, _export, conf, expand_user, expand_vars;

    shell = C_shell =  _export = _server = False, conf = False, expand_user = expand_vars = False;

    std::string value;

    for (int n = 1; n < argc; n++) {
      const char *s = argv[n];

      if (!strcmp(s, "--sh")) {
	if (C_shell || shell)
	  return usage(argv[0]);
	shell = True;
      }
      else if (!strcmp(s, "--csh")) {
	if (C_shell || shell)
	  return usage(argv[0]);
	C_shell = True;
      }
      else if (!strcmp(s, "--export"))
	_export = True;
      else if (!strcmp(s, "--server"))
	_server = True;
      else if (!strcmp(s, "--config"))
	conf = True;
      else if (!strcmp(s, "--expand-user"))
	expand_user = True;
      else if (!strcmp(s, "--expand-vars"))
	expand_vars = True;
      else if (*s == '-')
	return usage(argv[0]);
      else
	list.insertObject((void *)s);
    }

    if (_export && !shell && !C_shell)
      return usage(argv[0]);

    if (!list.getCount() && !shell && !C_shell && !conf)
      return usage(argv[0]);

    if (shell + C_shell + conf > 1)
      return usage(argv[0]);

    unsigned int item_cnt;
    Config::Item *items;

    Config *cfg;
    if (_server)
      cfg = ServerConfig::getInstance();
    else
      cfg = ClientConfig::getInstance();

    if (!list.getCount())
      items = cfg->getValues(item_cnt, expand_vars ? true : false);
    else  {
      item_cnt = list.getCount();
      items = new Config::Item[list.getCount()];
      LinkedListCursor c(list);
      const char *s;
      for (int n = 0; c.getNext((void *&)s); n++) {
	const char *v = cfg->getValue(s);
	items[n] = Config::Item(strdup(s), strdup(v ? v : ""));
      }
    }

    if (expand_user) {
      for (int n = 0; n < item_cnt; n++) {
	if (!strcmp(items[n].name, "user")) {
	  items[n].value = strdup(Connection::makeUser(items[n].value).c_str());
	  break;
	}
      }
    }

    if (shell) {
      fprintf(stdout, "#\n");
      fprintf(stdout, "# Bourne Shell EyeDB Environment\n");
      fprintf(stdout, "#\n\n");

      for (int n = 0; n < item_cnt; n++) {
	if (!Config::isBuiltinVar(items[n].name) && !Config::isUserVar(items[n].name)) {
	  std::string var = std::string("EYEDB") + capstring(items[n].name);
	  fprintf(stdout, "%s=%s", var.c_str(), items[n].value);
	  if (_export)
	    fprintf(stdout, "; export %s", var.c_str());
	  fprintf(stdout, "\n");
	}
      }

      delete [] items;
      return 0;
    }

    if (C_shell) {
      fprintf(stdout, "#\n");
      fprintf(stdout, "# C-Shell EyeDB Environment\n");
      fprintf(stdout, "#\n\n");

      for (int n = 0; n < item_cnt; n++) {
	if (!Config::isBuiltinVar(items[n].name) && !Config::isUserVar(items[n].name)) {
	  std::string var = std::string("EYEDB") + capstring(items[n].name);
	  if (_export)
	    fprintf(stdout, "setenv %s %s\n", var.c_str(),
		    items[n].value);
	  else
	    fprintf(stdout, "set %s=%s\n", var.c_str(), items[n].value);
	}
      }

      delete [] items;
      return 0;
    }

    if (conf) {
      fprintf(stdout, "#\n");
      fprintf(stdout, "# EyeDB %s Configuration File\n",
	      _server ? "Server" : "Client");
      fprintf(stdout, "#\n\n");

      for (int n = 0; n < item_cnt; n++)
	fprintf(stdout, "%s = %s;\n", items[n].name, items[n].value);

      delete [] items;
      return 0;
    }

    for (int n = 0; n < item_cnt; n++)
      fprintf(stdout, "%s\n", items[n].value);

    delete [] items;
  }
  catch(Exception &e) {
    std::cerr << e << std::flush;
    return 1;
  }

  return 0;
}
