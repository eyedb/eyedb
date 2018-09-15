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


#ifndef _EYEDB_GETOPT_H
#define _EYEDB_GETOPT_H

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unistd.h>

// feel free to inherit from this class to define more precise type
class OptionType {

public:
  OptionType() { }
  OptionType(const std::string &name) : name(name) { }
  virtual bool checkValue(const std::string &value, const std::string &prog, std::ostream &err_os) const = 0;

  virtual const OptionType *clone() const = 0;
  const std::string getName() const {return name;}

  const std::string& getStringValue(const std::string &value) const {
    return value;
  }

  virtual std::string getDefaultValue() const {return "";}

  virtual ~OptionType() { }

private:
  std::string name;
};

class OptionStringType : public OptionType {
public:
  OptionStringType() : OptionType("string") { }
  virtual bool checkValue(const std::string &value, const std::string &prog, std::ostream &err_os) const { return true; }

  virtual const OptionType *clone() const {return new OptionStringType();}
};

class OptionIntType : public OptionType {
public:
  OptionIntType() : OptionType("int") { }

  virtual bool checkValue(const std::string &value, const std::string &prog, std::ostream &err_os) const;

  int getIntValue(const std::string &value) const;

  virtual const OptionType *clone() const {return new OptionIntType();}
};

class OptionBoolType : public OptionType {
public:
  OptionBoolType() : OptionType("bool") { }

  virtual bool checkValue(const std::string &value, const std::string &prog, std::ostream &err_os) const;
  bool getBoolValue(const std::string &value) const;

  virtual std::string getDefaultValue() const {return "true";}

  virtual const OptionType *clone() const {return new OptionBoolType();}
};

class OptionChoiceType : public OptionType {

public:
  OptionChoiceType(const std::string &name,
		   const std::vector<std::string> &choice,
		   const std::string &defval = "") :
    OptionType(name), choice(choice), defval(defval) { }

  virtual bool checkValue(const std::string &value, const std::string &prog, std::ostream &err_os) const;
  virtual std::string getDefaultValue() const {return defval;}

  virtual const OptionType *clone() const {
    return new OptionChoiceType(getName(), choice, defval);
  }

private:
  std::vector<std::string> choice;
  std::string defval;
};

class OptionValue {

public:
  const OptionType *type;

  OptionValue() { type = 0; }

  OptionValue(const OptionType &_type, const std::string &_value) {
    set(_type, _value);
  }

  OptionValue(const OptionValue &ov) {
    type = 0;
    *this = ov;
  }

  OptionValue& operator=(const OptionValue &ov) {
    delete type;
    type = (ov.type ? ov.type->clone() : 0);
    value = ov.value;
    return *this;
  }

  void set(const OptionType &_type, const std::string &_value) {
    type = _type.clone();
    value = _value;
  }

  std::string value;
  bool def;

  ~OptionValue() {
    delete type;
  }
};

class OptionDesc {

public:
  OptionDesc(const std::string &help = "",
	     const std::string &value_name = "value") :
    value_name(value_name),
    help(help) { }

  std::string value_name;
  std::string help;
};

class Option {

public:
  Option() : opt(0), flags(0), type(0) {}

  Option(char opt,
	 const std::string &long_opt,
	 const OptionType &type,
	 unsigned int flags,
	 const std::string &defval,
	 const OptionDesc &optdesc = OptionDesc());

  Option(char opt, const std::string &long_opt,
	 const OptionType &type = OptionBoolType(),
	 unsigned int flags = 0,
	 const OptionDesc &optdesc = OptionDesc());

  Option(char opt,
	 const OptionType &type = OptionBoolType(),
	 unsigned int flags = 0,
	 const OptionDesc &optdesc = OptionDesc());

  Option(const std::string &long_opt,
	 const OptionType &type = OptionBoolType(),
	 unsigned int flags = 0,
	 const OptionDesc &optdesc = OptionDesc());

  Option(char opt,
	 const OptionType &type,
	 unsigned int flags,
	 const std::string &defval,
	 const OptionDesc &optdesc = OptionDesc());

  Option(const std::string &long_opt,
	 const OptionType &type,
	 unsigned int flags,
	 const std::string &defval,
	 const OptionDesc &optdesc = OptionDesc());

  Option(const Option &o) {
    type = 0;
    *this = o;
  }

  Option& operator=(const Option &o) {
    delete type;
    type = (o.type ? o.type->clone() : 0);
    opt = o.opt;
    long_opt = o.long_opt;
    flags = o.flags;
    optdesc = o.optdesc;
    defval = o.defval;
    return *this;
  }

  enum {
    Mandatory = 0x1,
    MandatoryValue = 0x2,
    OptionalValue = 0x4,

    SetByDefault = 0x8,

    Usage = 0x100,
    Help = 0x200
  };

  char getOpt() const {return opt;}
  const std::string &getLongOpt() const {return long_opt;}
  const OptionType &getOptionType() const {return *type;}
  unsigned int getFlags() const {return flags;}
  const std::string &getDefaultValue() const {return defval;}
  const OptionDesc &getOptionDesc() const {return optdesc;}

  ~Option() { delete type; }

private:
  char opt;
  std::string long_opt;
  const OptionType *type;
  unsigned int flags;
  OptionDesc optdesc;
  std::string defval;

  void init(char opt, const std::string &long_opt,
	    const OptionType &type,
	    unsigned int flags,
	    const std::string &defval, const OptionDesc &optdesc);
};

class GetOpt {

public:
  enum {
    SkipUnknownOption = 0x1,
    PurgeArgv = 0x2,

    DisplayUsageOnError = 0x10,
    DisplayHelpOnError = 0x20
  };

  GetOpt(const std::string &prog,
	 unsigned int flags = PurgeArgv, std::ostream &err_os = std::cerr) :
    prog(prog), flags(flags), err_os(err_os), _maxlen(0) { }

  GetOpt(const std::string &prog,
	 const std::vector<Option> &opts,
	 unsigned int flags = PurgeArgv,
	 std::ostream &err_os = std::cerr);

  GetOpt(const std::string &prog,
	 const Option opts[], unsigned int opt_cnt,
	 unsigned int flags = PurgeArgv,
	 std::ostream &err_os = std::cerr);

  void add(const Option &opt);

  bool parse(int &argc, char *argv[]);
  bool parse(const std::string &prog, std::vector<std::string> &argv);
  
  void usage(const std::string &append = "\n",
	     const std::string &prefix = "usage: ",
	     std::ostream &os = std::cerr) const;
  void help(std::ostream &os = std::cerr, const std::string &indent = "  ") const;

  void helpLine(const std::string &option, const std::string &detail,
		std::ostream &os = std::cerr, const std::string &indent = "  ") const;

  void displayOpt(const std::string &opt, const std::string &detail, std::ostream &os = std::cerr, const std::string &indent = "  ") const;

  static bool parseLongOpt(const std::string &arg, const std::string &opt,
			   std::string *value = 0);

  std::ostream &getErrorOS() {return err_os;}

  typedef std::map<std::string, OptionValue> Map;
  GetOpt::Map &getMap() {return map;}

  bool isset(const std::string &opt) const {return map.find(opt) != map.end();}
  const std::string &get(const std::string& opt) const {
    return (*map.find(opt)).second.value;
  }

  void adjustMaxLen(const std::string &opt);
  void adjustMaxLen(unsigned int maxlen);

private:
  std::string prog;
  std::map<std::string, Option> opt_map, long_opt_map;
  std::vector<Option> opt_v;
  Map map;
  unsigned int flags;
  unsigned int add_map(const Option &opt, const std::string &value);
  void init_map(std::map<std::string, Option> &);
  unsigned int check_mandatory();
  std::ostream &err_os;
  unsigned int getMaxLen() const;
  void displayHelpOpt(const Option &opt, std::ostream &os) const;
  unsigned int _maxlen;
};

#endif
