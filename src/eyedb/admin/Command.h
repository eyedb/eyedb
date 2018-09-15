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

#ifndef _EYEDB_ADMIN_COMMAND_H
#define _EYEDB_ADMIN_COMMAND_H

#include <string>
#include <vector>
#include <iostream>

#include "eyedbconfig.h"
#include <eyedb/eyedb.h>

class Topic;

extern std::string PROGNAME;

class Command {

protected:
  std::string name;
  Topic *topic;

  Command(Topic *topic, const std::string &name) : topic(topic), name(name) { }
  static bool shortcut;

public:

  const std::string &getName() const {return name;}
  bool isCommand(std::string name) const;

  virtual int usage() = 0;
  virtual int help() = 0;
  virtual int perform(eyedb::Connection &conn, std::vector<std::string> &argv) = 0;
  std::string getExtName() const;

  static void setShortcutMode(bool _shortcut) {shortcut = _shortcut;}
  static bool isShortcutMode() {return shortcut;}
};

#define CMDCLASS(CLS, NAME) \
\
class CLS : public Command { \
\
public: \
  CLS(Topic *topic) : Command(topic, NAME) { } \
\
  virtual int usage() { std::cout << PROGNAME << " " << topic->getName() << " " << NAME << " OPTIONS\n"; return 1; } \
  virtual int help() { std::cout << "help: " << NAME << '\n'; return 1; } \
  virtual int perform(eyedb::Connection &conn, std::vector<std::string> &argv) { return 1; } \
}

#define CMDCLASS_GETOPT(CLS, NAME) \
\
class CLS : public Command { \
\
  GetOpt *getopt; \
  void init(); \
  void stdhelp() {usage(); std::cerr << '\n'; getopt->help();} \
\
public: \
  CLS(Topic *topic) : Command(topic, NAME), getopt(0) { init(); } \
\
  virtual int usage(); \
  virtual int help(); \
  virtual int perform(eyedb::Connection &conn, std::vector<std::string> &argv); \
\
  virtual ~CLS() {delete getopt;} \
}

#endif
