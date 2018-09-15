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

#include <string>

#include "Topic.h"

struct Shortcut {
  std::string topic;
  std::string cmd;

  Shortcut() {}
  Shortcut(const std::string &topic, const std::string &cmd) : topic(topic), cmd(cmd) {}
};

using namespace eyedb;

static Shortcut find_shortcut(const std::string &progname);

std::string PROGNAME = "eyedbadmin";

int main(int c_argc, char *c_argv[])
{
  eyedb::init(c_argc, c_argv);

  std::vector<std::string> argv;

  std::string progname = c_argv[0];
  std::string::size_type pos = progname.rfind("/");

  if (pos != std::string::npos) {
    progname = progname.substr(pos+1);
  }

  if (progname != PROGNAME) {
    Shortcut shortcut = find_shortcut(progname);

    if (!shortcut.cmd.length()) {
      std::cerr << PROGNAME << ": unknown alias " << progname << std::endl;
      return 1;
    }

    argv.push_back(shortcut.topic);
    argv.push_back(shortcut.cmd);
    Command::setShortcutMode(true);

    PROGNAME = progname;
  }

  for (int n = 1; n < c_argc; n++) {
    argv.push_back(c_argv[n]);
  }

  Exception::setMode(Exception::ExceptionMode);

  try {
    Connection conn;
    return TopicSet::getInstance()->perform(conn, argv);
  }

  catch(Exception &e) {
    std::cerr << e << std::endl;
    return 1;
  }
}

static Shortcut find_shortcut(const std::string &progname)
{
  // note: a std::map is not necessary as this is used only once
  // (i.e. the std::map construct process will take longer...)

  if (progname == "eyedb_dbcreate")
    return Shortcut("database", "create");

  if (progname == "eyedb_dbdelete")
    return Shortcut("database", "delete");

  if (progname == "eyedb_dblist")
    return Shortcut("database", "list");

  if (progname == "eyedb_dbrename")
    return Shortcut("database", "rename");

  if (progname == "eyedb_dbexport")
    return Shortcut("database", "export");

  if (progname == "eyedb_dbimport")
    return Shortcut("database", "import");

  if (progname == "eyedb_dtfcreate")
    return Shortcut("datafile", "create");

  if (progname == "eyedb_dtfdelete")
    return Shortcut("datafile", "delete");

  if (progname == "eyedb_dtflist")
    return Shortcut("datafile", "list");

  if (progname == "eyedb_dspcreate")
    return Shortcut("dataspace", "create");

  if (progname == "eyedb_dspdelete")
    return Shortcut("dataspace", "delete");

  if (progname == "eyedb_dsplist")
    return Shortcut("dataspace", "list");

  if (progname == "eyedb_useradd")
    return Shortcut("user", "add");

  if (progname == "eyedb_userdelete")
    return Shortcut("user", "delete");

  if (progname == "eyedb_userlist")
    return Shortcut("user", "list");

  if (progname == "eyedb_userpasswd")
    return Shortcut("user", "passwd");

  return Shortcut();
}
