
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

#include "DBSTopic.h"
#include "DSPTopic.h"
#include "DTFTopic.h"
#include "USRTopic.h"
#include "IDXTopic.h"
#include "CLSTopic.h"
#include "COLTopic.h"
#include "ATRTopic.h"
#include "OBJTopic.h"
#include "CNSTopic.h"

const Option HELP_OPT("help", OptionStringType(), Option::Help, OptionDesc("Displays the current help"));

const Option HELP_COMMON_OPT("help-eyedb-options", OptionStringType(), 0, OptionDesc("Displays the EyeDB common options"));

TopicSet *TopicSet::instance;
bool Command::shortcut = false;

TopicSet::TopicSet()
{
  addTopic(new DBSTopic());
  addTopic(new DTFTopic());
  addTopic(new DSPTopic());
  addTopic(new USRTopic());
  addTopic(new IDXTopic());

  // not yet implemented
  //addTopic(new CLSTopic());
  //addTopic(new COLTopic());
  //addTopic(new ATRTopic());
  //addTopic(new OBJTopic());
  //addTopic(new CNSTopic());
}

int TopicSet::perform(eyedb::Connection &conn, std::vector<std::string> &argv)
{
  if (argv.size() == 0) {
    return usage();
  }

  if (argv.size() == 1 && argv[0] == "--help") {
    return help();
  }

  std::string topic_name = argv[0];

  Topic *topic = 0;
  if (argv.size() >= 1) {
    topic = getTopic(topic_name);
  }

  if (!topic) {
    return usage();
  }

  if (argv.size() == 1) {
    return topic->usage(topic_name);
  }
  
  std::string cmd_name = argv[1];

  if (cmd_name == "--help") {
    return topic->help();
  }

  Command *cmd = topic->getCommand(cmd_name);
  if (!cmd) {
    return topic->usage(topic_name);
  }

  // get rid of the two first arguments
  argv.erase(argv.begin());
  argv.erase(argv.begin());
  return cmd->perform(conn, argv);
}

int TopicSet::usage()
{
  std::cerr << PROGNAME << " usage:\n";
  std::cerr << "  " << PROGNAME << " --help\n";
  std::cerr << "  " << PROGNAME << " --help-eyedb-options\n";
  std::cerr << "  " << PROGNAME << " TOPIC --help\n";
  std::cerr << "  " << PROGNAME << " TOPIC COMMAND --help\n";
  std::cerr << "  " << PROGNAME << " TOPIC COMMAND OPTIONS\n\n";
  std::cerr << "where TOPIC is one of the following:\n";

  std::vector<Topic *>::iterator begin = topic_v.begin();
  std::vector<Topic *>::iterator end = topic_v.end();

  while (begin != end) {
    Topic *topic = *begin;
    std::cerr << "  ";
    topic->display(std::cerr);
    std::cerr << '\n';
    ++begin;
  }

  return 1;
}

int TopicSet::help()
{
  std::vector<Topic *>::iterator begin = topic_v.begin();
  std::vector<Topic *>::iterator end = topic_v.end();

  std::cerr << PROGNAME << " help:\n\n";

  for (int n = 0; begin != end; n++) {
    Topic *topic = *begin;
    std::vector<Command *> cmd_v = topic->getCommands();
    std::vector<Command *>::iterator b = cmd_v.begin();
    std::vector<Command *>::iterator e = cmd_v.end();
    if (n) {
      std::cerr << '\n';
    }

    //std::cerr << "  " << PROGNAME << " " << topic->getName() << " --help\n";
    while (b != e) {
      Command *cmd = *b;
      std::cerr << "  ";
      cmd->usage();
      ++b;
    }
    ++begin;
  }

  return 1;
}

Topic *TopicSet::getTopic(const std::string &name)
{
  std::vector<Topic *>::iterator begin = topic_v.begin();
  std::vector<Topic *>::iterator end = topic_v.end();

  while (begin != end) {
    Topic *topic = *begin;
    if (topic->isTopic(name))
      return topic;
    ++begin;
  }

  return 0;
}

void Topic::display(std::ostream &os)
{
  os << name;
  std::vector<std::string>::iterator begin = alias_v.begin();
  std::vector<std::string>::iterator end = alias_v.end();

  while (begin != end) {
    os << " | " << (*begin);
    ++begin;
  }
}

bool Topic::isTopic(const std::string &name) const
{
  if (this->name == name)
    return true;

  std::vector<std::string>::const_iterator begin = alias_v.begin();
  std::vector<std::string>::const_iterator end = alias_v.end();

  while (begin != end) {
    if (name == *begin)
      return true;
    ++begin;
  }

  return false;
}

Command *Topic::getCommand(const std::string &name)
{
  std::vector<Command *>::iterator begin = cmd_v.begin();
  std::vector<Command *>::iterator end = cmd_v.end();

  while (begin != end) {
    Command *cmd = *begin;
    if (cmd->getName() == name)
      return cmd;
    ++begin;
  }

  return 0;
}

int Topic::usage(const std::string &tname)
{
  std::cerr << PROGNAME << " " << name << " usage:\n\n";
  std::cerr << "  " << PROGNAME << " " << tname << " --help\n";
  std::cerr << "  " << PROGNAME << " " << tname << " COMMAND OPTIONS\n\n";
  std::cerr << "where COMMAND is one of the following:\n";

  std::vector<Command *>::iterator begin = cmd_v.begin();
  std::vector<Command *>::iterator end = cmd_v.end();

  while (begin != end) {
    Command *cmd = *begin;
    std::cerr << "  " << cmd->getName() << '\n';
    ++begin;
  }

  return 1;
}

int Topic::help()
{
  std::vector<Command *>::iterator begin = cmd_v.begin();
  std::vector<Command *>::iterator end = cmd_v.end();

  std::cerr << PROGNAME << " " << name << " help:\n\n";

  while (begin != end) {
    Command *cmd = *begin;
    std::cerr << "  ";
    cmd->usage();
    ++begin;
  }

  return 1;
}

std::string Command::getExtName() const
{
  if (isShortcutMode())
    return PROGNAME;
  return PROGNAME + " " + topic->getName() + " " + getName();
}
