/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
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

#ifndef _EYEDB_ADMIN_TOPIC_H
#define _EYEDB_ADMIN_TOPIC_H

#include "Const.h"
#include "Command.h"
#include "GetOpt.h"

class Topic {

  std::string name;
  std::vector<std::string> alias_v;
  std::vector<Command *> cmd_v;

public:
  Topic(const std::string &name) : name(name) { }

  const std::string &getName() const {return name;}
  std::vector<std::string> getAlias() {return alias_v;}

  void display(std::ostream &os);

  void addAlias(std::string alias) {
    alias_v.push_back(alias);
  }

  void addCommand(Command *cmd) {
    cmd_v.push_back(cmd);
  }

  bool isTopic(const std::string &name) const;

  Command *getCommand(const std::string &name);

  std::vector<Command *> getCommands() {return cmd_v;}

  int usage(const std::string &name);
  int help();

  virtual ~Topic() { }
};

class TopicSet {

  std::vector<Topic *> topic_v;
  static TopicSet *instance;

  int usage();

  int help();

  std::vector<Topic *> getTopics() {return topic_v;}
  Topic *getTopic(const std::string &name);

  void addTopic(Topic *topic) {topic_v.push_back(topic);}

  TopicSet();

public:

  static TopicSet *getInstance() {
    if (!instance)
      instance = new TopicSet();
    return instance;
  }

  int perform(eyedb::Connection &conn, std::vector<std::string> &argv);
};

extern const Option HELP_OPT, HELP_COMMON_OPT;

#endif
