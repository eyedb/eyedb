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
#include "GetOpt.h"

using namespace std;

Option::Option(char opt,
	       const std::string &long_opt,
	       const OptionType &type,
	       unsigned int flags,
	       const std::string &defval,
	       const OptionDesc &optdesc)
{
  init(opt, long_opt, type, flags, defval, optdesc);
}

Option::Option(char opt,
	       const std::string &long_opt,
	       const OptionType &type,
	       unsigned int flags,
	       const OptionDesc &optdesc)
{
  if (flags & SetByDefault)
    defval = type.getDefaultValue();

  init(opt, long_opt, type, flags, defval, optdesc);
}

Option::Option(char opt,
	       const OptionType &type,
	       unsigned int flags,
	       const OptionDesc &optdesc)
{
  if (flags & SetByDefault)
    defval = type.getDefaultValue();

  init(opt, "", type, flags, defval, optdesc);
}


Option::Option(const std::string &long_opt,
	       const OptionType &type,
	       unsigned int flags,
	       const OptionDesc &optdesc)
{
  if (flags & SetByDefault)
    defval = type.getDefaultValue();

  init(0, long_opt, type, flags, defval, optdesc);
}

Option::Option(char opt,
	       const OptionType &type,
	       unsigned int flags,
	       const std::string &defval,
	       const OptionDesc &optdesc)
{
  init(opt, "", type, flags, defval, optdesc);
}

Option::Option(const std::string &long_opt,
	       const OptionType &type,
	       unsigned int flags,
	       const std::string &defval,
const OptionDesc &optdesc)
{
  init(0, long_opt, type, flags, defval, optdesc);
}

void Option::init(char _opt,
		  const std::string &_long_opt,
		  const OptionType &_type,
		  unsigned int _flags,
		  const std::string &_defval,
		  const OptionDesc &_optdesc)
{
  opt = _opt;
  long_opt = _long_opt;
  type = _type.clone();
  flags = _flags;
  defval = _defval;

  optdesc = _optdesc;
}


GetOpt::GetOpt(const std::string &prog, const Option opts[],
	       unsigned int opt_cnt, unsigned int flags,
	       std::ostream &err_os) : prog(prog), flags(flags),
				       err_os(err_os), _maxlen(0)
{
  for (unsigned int n = 0; n < opt_cnt; n++)
    add(opts[n]);
}

GetOpt::GetOpt(const std::string &prog, const std::vector<Option> &opts,
	       unsigned int flags,
	       std::ostream &err_os) : prog(prog), flags(flags),
				       err_os(err_os), _maxlen(0)
{
  unsigned int opt_cnt = opts.size();

  for (unsigned int n = 0; n < opt_cnt; n++)
    add(opts[n]);
}

void GetOpt::usage(const std::string &append, const std::string &prefix,
		   std::ostream &os) const
{
  std::vector<Option>::const_iterator begin = opt_v.begin();
  std::vector<Option>::const_iterator end = opt_v.end();

  os << prefix << prog;
  if (prog.length() > 0)
    os << ' ';

  while (begin != end) {
    const Option &opt = (*begin);
    if (begin != opt_v.begin())
      os << " ";

    if (!(opt.getFlags() & Option::Mandatory))
      os << '[';

    if (opt.getOpt()) {
      os << "-" << opt.getOpt();
      if (opt.getFlags() & Option::MandatoryValue)
	os << " " << opt.getOptionDesc().value_name;
      else if (opt.getFlags() & Option::OptionalValue)
	os << " [" << opt.getOptionDesc().value_name << "]";
    }

    if (opt.getLongOpt().length() > 0) {
      os << (opt.getOpt() ? "|" : "") << "--" << opt.getLongOpt();
      if (opt.getFlags() & Option::MandatoryValue)
	os << "=" << opt.getOptionDesc().value_name;
      else if (opt.getFlags() & Option::OptionalValue)
	os << "[=" << opt.getOptionDesc().value_name << "]";
    }

    if (!(opt.getFlags() & Option::Mandatory))
      os << ']';

    ++begin;
  }

  os << append;
}

void GetOpt::displayHelpOpt(const Option &opt, ostream &os) const
{
  if (opt.getOpt()) {
    os << "-" << opt.getOpt();
    if (opt.getFlags() & Option::MandatoryValue)
      os << " " << opt.getOptionDesc().value_name;
    else if (opt.getFlags() & Option::OptionalValue)
      os << " [" << opt.getOptionDesc().value_name << "]";
  }

  if (opt.getLongOpt().length() > 0) {
    os << (opt.getOpt() ? ", " : "") << "--" << opt.getLongOpt();
    if (opt.getFlags() & Option::MandatoryValue)
      os << "=" << opt.getOptionDesc().value_name;
    else if (opt.getFlags() & Option::OptionalValue)
      os << "[=" << opt.getOptionDesc().value_name << "]";
  }
}

void GetOpt::adjustMaxLen(const std::string &opt)
{
  adjustMaxLen(opt.length());
}

void GetOpt::adjustMaxLen(unsigned int maxlen)
{
  (void)getMaxLen();
  if (_maxlen < maxlen) {
    _maxlen = maxlen;
  }
}

unsigned int GetOpt::getMaxLen() const
{
  if (_maxlen)
    return _maxlen;

  std::vector<Option>::const_iterator begin = opt_v.begin();
  std::vector<Option>::const_iterator end = opt_v.end();

  while (begin != end) {
    const Option &opt = *begin;

    ostringstream ostr;

    displayHelpOpt(opt, ostr);

    if (ostr.str().length() > _maxlen)
      const_cast<GetOpt*>(this)->_maxlen = ostr.str().length();

    ++begin;
  }

  return _maxlen;
}

void GetOpt::helpLine(const std::string &option, const std::string &detail,
		      std::ostream &os, const std::string &indent) const
{
  os << indent;

  unsigned int maxlen = getMaxLen();

  ostringstream ostr;
  ostr << option;

  os << ostr.str();
  for (unsigned int n = ostr.str().length(); n < maxlen; n++)
    os << ' ';

  os << ' ' << detail << '\n';
}

void GetOpt::displayOpt(const std::string &opt, const std::string &detail, std::ostream &os, const std::string &indent) const
{
  unsigned int maxlen = getMaxLen();

  os << indent;
  ostringstream ostr;
  ostr << opt;

  os << ostr.str();

  for (unsigned int n = ostr.str().length(); n < maxlen; n++)
    os << ' ';
  
  os << ' ' << detail << endl;
}

void GetOpt::help(std::ostream &os, const std::string &indent) const
{
  std::vector<Option>::const_iterator begin = opt_v.begin();
  std::vector<Option>::const_iterator end = opt_v.end();

  unsigned int maxlen = getMaxLen();

  while (begin != end) {
    const Option &opt = (*begin);
    os << indent;
    ostringstream ostr;

    displayHelpOpt(opt, ostr);

    os << ostr.str();
    for (unsigned int n = ostr.str().length(); n < maxlen; n++)
      os << ' ';

    os << ' ' << opt.getOptionDesc().help << endl;
    ++begin;
  }
}

void GetOpt::add(const Option &opt)
{
  opt_v.push_back(opt);

  char c = opt.getOpt();
  if (c) {
    char s[3];
    s[0] = '-';
    s[1] = c;
    s[2] = 0;
    opt_map[s] = opt;
  }

  const std::string &l = opt.getLongOpt();
  if (l.length()) {
    std::string s = "--" + l;
    long_opt_map[s] = opt;
  }
}

bool OptionIntType::checkValue(const std::string &value,
			       const std::string &prog,
			       ostream &err_os) const
{
  const char *s = value.c_str();
  while (*s) {
    if (*s < '0' || *s > '9') {
      err_os << prog << ": invalid integer value " << s << endl;
      return false;
    }
    s++;
  }

  return true;
}

bool OptionBoolType::checkValue(const std::string &value,
				const std::string &prog,
				ostream &err_os) const
{
  const char *s = value.c_str();

  if (!strcasecmp(s, "true") ||
      !strcasecmp(s, "yes") ||
      !strcasecmp(s, "on") ||
      !strcasecmp(s, "false") ||
      !strcasecmp(s, "no") ||
      !strcasecmp(s, "off"))
    return true;

  err_os << prog << ": unexpected boolean value " << s << endl;
  return false;
}

bool OptionBoolType::getBoolValue(const std::string &value) const
{
  const char *s = value.c_str();

  if (!strcasecmp(s, "true") ||
      !strcasecmp(s, "yes") ||
      !strcasecmp(s, "on"))
    return true;

  if (!strcasecmp(s, "false") ||
      !strcasecmp(s, "no") ||
      !strcasecmp(s, "off"))
    return false;

  return false;
}


int OptionIntType::getIntValue(const std::string &value) const
{
  return atoi(value.c_str());
}

bool OptionChoiceType::checkValue(const std::string &value,
				  const std::string &prog,
				  ostream &err_os) const
{
  std::vector<std::string>::const_iterator begin = choice.begin();
  std::vector<std::string>::const_iterator end = choice.end();

  while (begin != end) {
    if (*begin == value)
      return true;
    ++begin;
  }

  err_os << prog << ": invalid value " << value << endl;
  return false;
}

unsigned int GetOpt::add_map(const Option &opt, const std::string &_value)
{
  if (!opt.getOptionType().checkValue(_value, prog, err_os))
    return 1;

  OptionValue value(opt.getOptionType(), _value);

  char c = opt.getOpt();
  if (c) {
    char s[2];
    s[0] = c;
    s[1] = 0;
    if (map.find(s) != map.end()) {
      if (!map[s].def) {
	err_os << prog << ": option -" << s;
	if (opt.getLongOpt().length())
	  err_os << "/--" << opt.getLongOpt();
	err_os << " already set" << endl;
	return 1;
      }
    }
    map[s] = value;
  }

  const std::string &s = opt.getLongOpt();
  if (s.length()) {
    if (map.find(s) != map.end()) {
      if (!map[s].def) {
	err_os << prog << ": option ";
	if (opt.getOpt())
	  err_os << " -" << opt.getOpt() << "/";
	err_os << "--" << opt.getLongOpt();
	err_os << " already set" << endl;
	return 1;
      }
    }
    map[s] = value;
  }
  return 0;
}

void GetOpt::init_map(std::map<std::string, Option> &_map)
{
  std::map<std::string, Option>::iterator begin = _map.begin();
  std::map<std::string, Option>::iterator end = _map.end();
  
  while (begin != end) {
    const Option &opt = (*begin).second;
    unsigned int flags = opt.getFlags();
    if (flags & Option::SetByDefault) {
      add_map(opt, opt.getDefaultValue());
    }
    ++begin;
  }
}

unsigned int GetOpt::check_mandatory()
{
  unsigned int error = 0;

  std::map<std::string, Option>::iterator begin = opt_map.begin();
  std::map<std::string, Option>::iterator end = opt_map.end();

  bool found = false;
  while (begin != end) {
      const Option &opt = (*begin).second;
      if (opt.getFlags() & Option::Mandatory) {
	char s[2];
	s[0] = opt.getOpt();
	s[1] = 0;
	if (map.find(s) == map.end()) {
	  err_os << prog << ": mandatory option -" << opt.getOpt() << " is missing" <<
	    endl;
	  if (opt.getLongOpt().length() > 0)
	    map[opt.getLongOpt()] = OptionValue();
	  error++;
	}
      }
      ++begin;
    }
  
  begin = long_opt_map.begin();
  end = long_opt_map.end();

  while (begin != end) {
    const Option &opt = (*begin).second;
    if (opt.getFlags() & Option::Mandatory) {
      if (map.find(opt.getLongOpt()) == map.end()) {
	err_os << prog << ": mandatory option -" << opt.getLongOpt() << " is missing" <<
	  endl;
	error++;
      }
    }
    ++begin;
  }

  return error;
}

bool GetOpt::parseLongOpt(const std::string &arg, const std::string &opt,
			  std::string *value)
{
  if (value) {
    if (!strncmp((arg + "=").c_str(),
		 ("--" + opt + "=").c_str(),
		 2 + strlen(opt.c_str()) + 1)) {
      *value = arg.c_str() + 3 + strlen(opt.c_str());
      return true;
    }

    return false;
  }

  if (!strcmp(arg.c_str(), ("--" + opt).c_str()))
    return true;

  return false;
}

static void display(const std::string &action, const std::vector<std::string> &argv)
{
  std::cout << action << ":\n";
  for (unsigned int n = 0; n < argv.size(); n++) {
    std::cout << "#" << n << ": " << argv[n] << '\n';
  }
}

bool GetOpt::parse(const std::string &prog, std::vector<std::string> &argv)
{
  unsigned int argv_cnt = argv.size();
  char ** c_argv = new char*[argv_cnt + 2];

  c_argv[0] = strdup(prog.c_str());

  for (unsigned int n = 0; n < argv_cnt; n++) {
    c_argv[n+1] = strdup(argv[n].c_str());
  }  

  c_argv[argv_cnt + 1] = 0;

  //display("before: " , argv);
  int argc = argv_cnt + 1;
  int oargc = argc;
  bool r = parse(argc, c_argv);

  if (oargc != argc) {
    argv.clear();
    for (unsigned int n = 1; n < argc; n++) {
      argv.push_back(c_argv[n]);
    }
  }

  //display("after: " , argv);

#if 1
  for (unsigned int n = 0; n < argc; n++) {
    free(c_argv[n]);
  }
#else
  for (unsigned int n = 0; n < argv_cnt + 1; n++) {
    free(c_argv[n]);
  }
#endif

  delete [] c_argv;
  return r;
}

bool GetOpt::parse(int &argc, char *argv[])
{
  init_map(opt_map);
  init_map(long_opt_map);

  unsigned int error = 0;

  vector<char *> keep_v;
  for (int n = 1; n < argc; n++) {
    char *s = argv[n];
    if (*s != '-') {
      keep_v.push_back(argv[n]);
      continue;
    }

    if (!(flags & PurgeArgv))
      keep_v.push_back(argv[n]);

    s = strdup(s);

    std::map<std::string, Option> *omap;
    char *value = 0;
    bool long_opt = false;

    if (strlen(s) > 2 && *(s+1) == '-') {
      long_opt = true;
      omap = &long_opt_map;
      char *c = strchr(s, '=');
      if (c) {
	*c = 0;
	value = c+1;
      }
    }
    else {
      long_opt = false;
      omap = &opt_map;
      if (n < argc-1) {
	if (*argv[n+1] != '-')
	  value = argv[n+1];
      }
    }

    std::map<std::string, Option>::iterator begin = omap->begin();
    std::map<std::string, Option>::iterator end = omap->end();

    bool found = false;
    while (begin != end) {
      if (!strcmp(s, (*begin).first.c_str())) {
	const Option &opt = (*begin).second;
	found = true;
	if (opt.getFlags() & Option::MandatoryValue) {
	  if (!value) {
	    err_os << prog << ": missing value after " <<
	      s << endl;
	    error++;
	    ++begin;
	    continue;
	  }
	  error += add_map(opt, value);
	  if (!long_opt) {
	    if (!(flags & PurgeArgv))
	      keep_v.push_back(value);
	    ++n;
	  }
	}
	else if (opt.getFlags() & Option::OptionalValue) {
	  if (!value)
	    value = (char *)opt.getDefaultValue().c_str();
	  error += add_map(opt, value);
	  if (!long_opt) {
	    if (!(flags & PurgeArgv))
	      keep_v.push_back(value);
	    ++n;
	  }
	}
	// disconnected 19/10/05. But was added a few days before because
	// of a good reason, I don't remember which one ...
	// I added '&& long_opt' in the test
#if 0
	else if (value) {
	  error += add_map(opt, value);
	  if (!long_opt)
	    ++n;
	}
#else
	else if (value && long_opt) {
	  error += add_map(opt, value);
	  if (!long_opt) {
	    if (!(flags & PurgeArgv))
	      keep_v.push_back(value);
	    ++n;
	  }
	}
#endif
	else
	  error += add_map(opt, "true");
      }
      ++begin;
    }

    if (!found) {
      if (!(flags & SkipUnknownOption)) {
	err_os << prog << ": unknown option " << s << endl;
	error++;
      }
      if (flags & PurgeArgv)
	keep_v.push_back(argv[n]);
    }

    free(s);
  }

  if (!error)
    error += check_mandatory();

  if (error)
    map.erase(map.begin(), map.end());

  if (!error) {
    vector<char *>::iterator b = keep_v.begin();
    vector<char *>::iterator e = keep_v.end();
    for (int n = 1; b != e; n++) {
      argv[n] = *b;
      ++b;
    }

    argc = keep_v.size() + 1;
    argv[argc] = 0;
  }

  return error == 0;
}

// unitary test
#if 0
int
main(int argc, char *argv[])
{
  Option opts[] = {
    Option('d', "database", OptionStringType(),
	   Option::Mandatory|Option::MandatoryValue,
	   OptionDesc("name of the database", "dbname")),
    Option('p', "print", OptionStringType(),
	   Option::Mandatory|Option::OptionalValue,
	   "/dev/tty1",
	   OptionDesc("name of the device", "device")),
    Option('w', "read-write"),
    Option('x'),
    Option("zoulou")
  };

  GetOpt getopt("test", opts, sizeof(opts)/sizeof(opts[0]),
		GetOpt::SkipUnknownOption|GetOpt::PurgeArgv);

  getopt.usage();
  getopt.help(cerr, "\t", 30);

  if (getopt.parse(argc, argv)) {
    cout << "argc is: " << argc << endl;
    for (int n = 0; n < argc; n++)
      cout << "argv[" << n << "] = " << argv[n] << endl;
    const GetOpt::Map &map = getopt.getMap();
    GetOpt::Map::const_iterator begin = map.begin();
    GetOpt::Map::const_iterator end = map.end();
    while (begin != end) {
      const OptionValue &value = (*begin).second;
      cout << (*begin).first << "=" << value.value << endl;
      ++begin;
    }
  }
}
#endif
