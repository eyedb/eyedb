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

#include <eyedbconfig.h>

#define BUILTIN_VARS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <eyedb/eyedb.h>
#include "eyedblib/strutils.h"
#include "compile_builtin.h"

#define MAXFILES 16

namespace eyedb {

  ClientConfig *ClientConfig::instance = 0;
  std::string ClientConfig::config_file;

  ServerConfig *ServerConfig::instance = 0;
  std::string ServerConfig::config_file;

  static bool initialized = false;

  /*
   * Config file parser
   */

  static int fd_w;
  static FILE *fd;
  static int *pline;
  static const char *pfile;

  static FILE *fd_sp[MAXFILES];
  static int line[MAXFILES];
  static char *file_sp[MAXFILES];

  static const char *
  uppercase(const char *s)
  {
    static char buf[128];
    char c, *p;
    p = buf;

    while (c = *s++)
      *p++ = c + (c >= 'a' && c <= 'z' ? ('A' - 'a') : 0);

    *p = 0;
    return buf;
  }

  static int
  skip_spaces()
  {
    for (;;) {
      char c = fgetc(fd);
      if (c == EOF)
	return 0;

      if (c != ' ' && c != '\t' && c != '\n') {
	ungetc(c, fd);
	break;
      }

      if (c == '\n')
	(*pline)++;
    }

    return 1;
  }
  
  static int
  skip_comments()
  {
    char c = fgetc(fd);
    if (c == '#')
      for (;;) {
	c = fgetc(fd);
	if (c == EOF)
	  return 0;
	
	if (c == '\n') {
	  ungetc(c, fd);
	  break;
	}
      }
    else
      ungetc(c, fd);

    return 1;
  }

  static const char assign[] = "=";
  static const char term[] = ";";

  static int
  check_spe(int &is_spe)
  {
    char c;

    is_spe = 0;
    c = fgetc(fd);
    if (c == EOF)
      return 0;

    if (c == ';') {
      is_spe = 1;
      return 1;
    }
    else if (c == '=') {
      is_spe = 2;
      return 1;
    }

    ungetc(c, fd);
    return 1;
  }

  static inline const char *
  line_str()
  {
    static std::string str;
    if (!pline || !pfile)
      return "";

    str = std::string("file \"" ) + pfile + "\" near line " + str_convert((long)*pline) + ": ";
    return str.c_str();
  }

  static void
  error(const char *fmt, const char *x1 = 0, const char *x2 = 0, const char *x3 = 0)
  {
#if 1
    if (Exception::getMode() == Exception::ExceptionMode) {
      (void)Exception::make(IDB_ERROR, fmt, x1, x2, x3);
      return;
    }

    fprintf(stderr, "eyedb configuration: ");
    fprintf(stderr, fmt, x1, x2, x3);
    fprintf(stderr, "\n");
    exit(1);
#else
    if (initialized) {
      Exception::Mode mode = Exception::setMode(Exception::ExceptionMode);
      (void)Exception::make(IDB_ERROR, fmt, x1, x2, x3);
      Exception::setMode(mode);
      return;
    }

    fprintf(stderr, fmt, x1, x2, x3);
    fprintf(stderr, "\n");
    exit(1);
#endif
  }

  static void
  syntax_error(const char *msg)
  {
    error((line_str() + std::string("syntax error: ") + msg).c_str());
  }

  static void
  syntax_error(const std::string &s)
  {
    syntax_error(s.c_str());
  }

  // Returns true if file is found
  // false if file not found and quietFileNotFoundError == true
  // generates an exception if file not found and quietFileNotFoundError == false
  static bool
  push_file(const char *file, bool quietFileNotFoundError)
  {
    FILE *try_fd;
    std::string sfile;

    if (strlen(file) > 2 && file[0] == '/' && file[1] == '/') {
      file += 2;
      sfile =  std::string( eyedb::CompileBuiltin::getSysconfdir()) + "/eyedb/" + file;
    }
    else 
      sfile = file;

    try_fd = fopen(sfile.c_str(), "r");
    if (!try_fd) {
      if (quietFileNotFoundError)
	return false;
      else
	error("%scannot open file '%s' for reading",
	      line_str(), sfile.c_str());
    }

    fd = try_fd;

    pline = &line[fd_w];
    fd_sp[fd_w] = fd;
    file_sp[fd_w] = strdup(file);
    pfile = file_sp[fd_w];
    *pline = 1;
    fd_w++;

    return true;
  }

  static const char *
  nexttoken_realize(Config *config)
  {
    static char tok[512];
    char c;

    if (!skip_spaces())
      return 0;

    int is_spe;

    if (!check_spe(is_spe))
      return 0;
    if (is_spe == 1)
      return term;
    if (is_spe == 2)
      return assign;

    char *p = tok;
    int force = 0;
    int hasvar = 0;
    int backslash = 0;
    char svar[128];
    char *var = 0;

    for (;;) {
      if (!force && !skip_comments())
	return 0;

      c = fgetc(fd);
      if (c == EOF)
	return 0;

      if (c == '%') {
	if (!var) {
	  var = svar;
	  continue;
	}

	*var = 0;
	*p = 0;

	if (!*svar)
	  strcat(tok, "%");
	else {
	  const char *val = config->getValue(svar);
	  if (!val)
	    error("%sunknown configuration variable '%s'", line_str(), svar);
	  strcat(tok, val);
	}

	p = tok + strlen(tok);
	var = 0;
	hasvar = 1;
	continue;
      }

      if (var) {
	if (var - svar >= sizeof(svar)) {
	  svar[sizeof(svar)-1] = 0;
	  error("%sconfiguration variable too long: '%s' "
		"(maximum size is %s)", line_str(), svar,
		str_convert((long)sizeof(svar)-1).c_str());
	}

	*var++ = c;
	continue;
      }

      if (c == '"' && !force) {
	force = 1;
	continue;
      }

      if (force && c == '\\') {
	backslash = 1;
	continue;
      }

      if (c == '\n') {
	(*pline)++;
	if (!force)
	  break;
      }
      else if (!force) {
	if (c == ' ' || c == '\t')
	  break;
	else if (c == ';' || c == '=') {
	  ungetc(c, fd);
	  break;
	}
      }
      else {
	if (backslash) {
	  if (c == 'n')
	    c = '\n';
	  else if (c == 'a')
	    c = '\a';
	  else if (c == 'b')
	    c = '\b';
	  else if (c == 'f')
	    c = '\f';
	  else if (c == 'r')
	    c = '\r';
	  else if (c == 't')
	    c = '\t';
	  else if (c == 'v')
	    c = '\v';
	  else if (c == '\\')
	    c = '\\';

	  backslash = 0;
	}
	else if (c == '"')
	  break;
      }

      *p++ = c;
    }

    *p = 0;

    return (p != tok || force || hasvar) ? tok : nexttoken_realize(config);
  }

  static const char *
  nexttoken(Config *config)
  {
    const char *p = nexttoken_realize(config);

    if (!p) {
      if (fd_w > 0 && --fd_w > 0)
	{
	  fd = fd_sp[fd_w-1];
	  pline = &line[fd_w-1];
	  pfile = file_sp[fd_w-1];
	  return nexttoken(config);
	}

      return 0;
    }

    if (!strcmp(p, "include") || !strcmp(p, "@include")) {
      bool quiet = !strcmp(p, "@include");

      const char *file = nexttoken_realize(config);
      if (!file) {
	syntax_error("file name expected after include");
	return 0;
      }

      push_file(file, quiet);

      return nexttoken(config);
    }

    return p;
  }

  bool Config::add(const char *file, bool quietFileNotFoundError)
  {
    if (!push_file(file, quietFileNotFoundError))
      return false;

    int state = 0;
    char *name = 0, *value = 0;

    bool last = false;

    while (!last) {
      const char *p = nexttoken(this);

      if (!p) {
	p = "\n";
	last = true;
      }

      switch(state) {
      case 0:
	if (!strcmp(p, term))
	  break;
	if (!strcmp(p, assign))
	  syntax_error(std::string("unexpected '") + p + "'");
	name = strdup(p);
	state = 1;
	break;

      case 1:
	if (strcmp(p, assign))
	  syntax_error(std::string("'") + assign + "' expected, got '" + p + "'");
	state = 2;
	break;

      case 2:
	if (!strcmp(p, assign) || !strcmp(p, term))
	  syntax_error(std::string("unexpected '") + p + "'");
	value = strdup(p);
	state = 3;
	break;

      case 3:
	if (strcmp(p, term))
	  syntax_error(std::string("'") + term + "' expected, got '" + p + "'");
	setValue( name, value);
	free(name);
	free(value);
	name = 0;
	value = 0;
	state = 0;
	break;
      }
    }

    return true;
  }


  /*
   * Config and Config::Item constructors, destructor and operator=
   */

  Config::Config()
    : list(), var_map(0)
  {
  }

  /*
  Config::Config(const char *file, const VarMap &_var_map)
  {
    var_map = new VarMap(_var_map);
    add(file);
  }
  */

  Config::Config(const std::string &name) :
    name(name)
  {
    var_map = 0;
  }

  Config::Config(const std::string &name, const VarMap &_var_map) :
    name(name)
  {
    var_map = new VarMap(_var_map);
  }

  Config::Config(const Config &config)
  {
    *this = config;
  }

  void
  Config::garbage()
  {
    LinkedListCursor c(list);
    Item *item;

    while (c.getNext((void *&)item))
      delete item;

    list.empty();
  }

  Config& Config::operator=(const Config &config)
  {
    garbage();

    LinkedListCursor c(config.list);
    Item *item;
    while(c.getNext((void *&)item)) {
      list.insertObjectFirst(new Item(*item));
    }

    var_map = config.var_map ? new VarMap(*config.var_map) : 0;
    return *this;
  }

  Config::~Config()
  {
    garbage();
  }

  Config::Item& Config::Item::operator=(const Item &item)
  {
    if (this == &item)
      return *this;

    free(name);
    free(value);
    name = strdup(item.name);
    value = strdup(item.value);
    return *this;
  }

  Config::Item::Item()
  {
    name = 0;
    value = 0;
  }

  Config::Item::Item(const char *_name, const char *_value)
  {
    name = strdup(_name);
    value = strdup(_value);
  }

  Config::Item::Item(const Item &item)
  {
    name = strdup(item.name);
    value = strdup(item.value);
  }

  Config::Item::~Item()
  {
    free(name);
    free(value);
  }


  /*
   * Config operator<<
   */

  std::ostream& operator<<( std::ostream& os, const Config& config)
  {
    LinkedListCursor c(config.list);
    Config::Item *item;

    while (c.getNext((void *&)item)) {
      os << "name= " << item->name << " value= " << item->value << std::endl;
    }

    return os;
  }


  /*
   * Config static _release method
   */

  void
  Config::_release()
  {
    ClientConfig::_release();
    ServerConfig::_release();
  }

  void ClientConfig::_release()
  {
    delete instance;
    instance = 0;
  }

  void ServerConfig::_release()
  {
    delete instance;
    instance = 0;
  }

  /*
   * Config variable set and get
   */

  void
  Config::checkIsIn(const char *name)
  {
    if (!var_map)
      return;

    if (*name == '$')
      return;

    if (var_map->find(name) != var_map->end())
      return;

    VarMap::const_iterator begin = var_map->begin();
    VarMap::const_iterator end = var_map->end();
    std::string msg = "known variables are: ";
    for (int n = 0; begin != end; n++) {
      if (n)
	msg += ", ";
      msg += (*begin).first;
      ++begin;
    }
    
    error("unknown variable '%s' found in %s configuration file.\n%s",
	  name, getName().c_str(), msg.c_str());
  }

  void 
  Config::setValue(const char *name, const char *value)
  {
    checkIsIn(name);

    Item *item = new Item(name, value);
    list.insertObjectFirst(item);
  }

  static char *copySTDString(const std::string &value)
  {
    static const int VALUE_BUFFER_CNT = 128;
    static unsigned int n = 0;
    static char *buf[VALUE_BUFFER_CNT];

    if (n == VALUE_BUFFER_CNT)
      n = 0;

    if (!buf[n] || strlen(buf[n]) < value.length()) {
      delete [] buf[n];
      buf[n] = new char[value.length()+1];
    }

    strcpy(buf[n], value.c_str());
    return buf[n++];
  }

  bool Config::isBuiltinVar(const char *name)
  {
    return *name == '@';
  }

  bool Config::isUserVar(const char *name)
  {
    return *name == '$';
  }

  const char *Config::getValue(const char *name, bool expand_vars) const
  {
    if (!isBuiltinVar(name) && !isUserVar(name)) {
      const char *s = getenv((std::string("EYEDB") + uppercase(name)).c_str());
      if (s) {
	return s;
      }
    }

    LinkedListCursor c(list);
    Item *item;

    while (c.getNext((void *&)item))
      if (!strcasecmp(item->name, name)) {
	if (!expand_vars) {
	  return item->value;
	}

	if (!strchr(item->value, '%')) {
	  return item->value;
	}

	std::string value;

	char *s = item->value;
	char *var;
	bool state = false;

	while (char c = *s++) {
	  if (c == '%') {
	    if (state) {
	      char svar[128];
	      unsigned int len = s - var - 1;
	      if (len >= sizeof(svar)) {
		error("%sconfiguration variable too long: '%s' "
		      "(maximum size is %s)", line_str(), svar,
		      str_convert((long)sizeof(svar)-1).c_str());
	      }

	      strncpy(svar, var, len);
	      svar[len] = 0;
	      const char *val = getValue(svar);
	      if (!val)
		error("%sunknown configuration variable '%s'", line_str(), svar);
	      value += val;
	    }
	    else {
	      var = s;
	    }
	    state = !state;
	  }
	  else if (!state) {
	    char tok[4];
	    sprintf(tok, "%c", c);
	    value += tok;
	  }
	}

	return copySTDString(value);
      }

    return (const char *)0;
  }

  Config::Item *Config::getValues(unsigned int &item_cnt, bool expand_vars) const
  {
    item_cnt = list.getCount();

    if (!item_cnt)
      return (Item *)0;

    Item *items = new Item[list.getCount()];

    Item *item;
    LinkedListCursor c(list);

    unsigned int n;
    for (n = 0; c.getNext((void *&)item); ) {
      int _not = 0;
      for (int i = 0; i < n; i++)
	if (!strcmp(items[i].name, item->name)) {
	  _not = 1;
	  break;
	}

      if (!_not) {
	items[n] = *item;
	items[n].value = strdup(getValue(item->name, expand_vars));
	n++;
      }
    }

    Item *ritems = new Item[n];
    for (unsigned int i = 0; i < n; i++) {
      ritems[i] = items[n-i-1];
    }

    delete [] items;
    item_cnt = n;
    return ritems;
  }


  /*
   * Client and server config management
   */

  static const std::string tcp_port = "6240";

  void
  ClientConfig::setDefaults()
  {
    std::string pipedir = eyedb::CompileBuiltin::getPipedir();

    // Port
    setValue( "port", (pipedir + "/eyedbd").c_str());

    // TCP Port
    setValue( "tcp_port", tcp_port.c_str());

    // Hostname
    setValue( "host", "localhost");

    // User
    setValue( "user", "@");

    // EYEDBDBM Database
    setValue( "dbm", "default");
  }

  void
  ServerConfig::setDefaults()
  {
    std::string bindir = eyedb::CompileBuiltin::getBindir();
    std::string sbindir = eyedb::CompileBuiltin::getSbindir();
    std::string libdir = eyedb::CompileBuiltin::getLibdir();
    std::string databasedir = eyedb::CompileBuiltin::getDatabasedir();
    std::string pipedir = eyedb::CompileBuiltin::getPipedir();
    std::string tmpdir = eyedb::CompileBuiltin::getTmpdir();
    std::string sysconfdir = eyedb::CompileBuiltin::getSysconfdir();

#ifdef BUILTIN_VARS
    setValue("@bindir", bindir.c_str());;
    setValue("@sbindir", sbindir.c_str());;
    setValue("@libdir", libdir.c_str());;
    setValue("@databasedir", databasedir.c_str());;
    setValue("@pipedir", pipedir.c_str());;
    setValue("@tmpdir", tmpdir.c_str());;
    setValue("@sysconfdir", sysconfdir.c_str());;

    // Bases directory
    setValue( "databasedir", "%@databasedir%");

    // tmpdir
    setValue( "tmpdir", "%@tmpdir%");

    // sopath
    setValue( "sopath", "%@libdir%/eyedb");

    // Default EYEDBDBM Databases
    setValue( "default_dbm", (databasedir + "/dbmdb.dbs").c_str());

    // Granted EYEDBDBM Databases
    //setValue( "granted_dbm", (localstatedir + "/lib/eyedb/db/dbmdb.dbs").c_str());
    // EV : 22/01/06
    // when variable expansion will be done in getValue(), granted_dbm will be:
    //setValue( "granted_dbm", "%default_dbm%");

    // Server Parameters
    setValue( "maximum_memory_size", "0");
    setValue( "access_file", "%@sysconfdir%/eyedb/Access");
#ifdef HAVE_EYEDBSMD
    setValue( "smdport", "%@pipedir%/eyedbsmd");
#endif
    setValue( "default_file_group", "");
    setValue( "default_file_mask", "0600");

    // Server Parameters
    setValue( "listen", ("localhost:" + tcp_port + ",%@pipedir%/eyedbd").c_str());

    // OQL path
    setValue( "oqlpath", "%@libdir%/eyedb/oql");
#else
    // Bases directory
    setValue( "databasedir", databasedir.c_str());

    // tmpdir
    setValue( "tmpdir", tmpdir.c_str());

    // sopath
    setValue( "sopath", (libdir + "/eyedb").c_str());

    // Default EYEDBDBM Databases
    setValue( "default_dbm", (databasedir + "/dbmdb.dbs").c_str());

    // Granted EYEDBDBM Databases
    //setValue( "granted_dbm", (localstatedir + "/lib/eyedb/db/dbmdb.dbs").c_str());
    // EV : 22/01/06
    // when variable expansion will be done in getValue(), granted_dbm will be:
    //setValue( "granted_dbm", "%default_dbm%");

    // Server Parameters
    setValue( "maximum_memory_size", "0");
    setValue( "access_file", (sysconfdir + "/eyedb/Access").c_str());
#ifdef HAVE_EYEDBSMD
    setValue( "smdport", (pipedir + "/eyedbsmd").c_str());
#endif
    setValue( "default_file_group", "");
    setValue( "default_file_mask", "0600");

    // Server Parameters
    setValue( "listen", ("localhost:" + tcp_port + "," + pipedir + "/eyedbd").c_str());

    // OQL path
    setValue( "oqlpath", (libdir + "/eyedb/oql").c_str());
#endif
  }

  void
  Config::loadConfigFile( const std::string& configFilename, const char* envVariable, const char* defaultFilename)
  {
    const char* envFileName;

    if (configFilename.length() != 0) {
      add(configFilename.c_str(), false);
    }
    else if ((envFileName = getenv(envVariable))) {
      add(envFileName, false);
    }
    else {
      struct passwd* pw = getpwuid(getuid());

      if (pw) {
	std::string homeConfigFile = std::string(pw->pw_dir) + "/.eyedb/" + defaultFilename;
	if (add(homeConfigFile.c_str(), true))
	  return;
      }

      const char *confdir = getenv("EYEDBCONFDIR");
      if (confdir) {
	add((std::string(confdir) + "/" + defaultFilename).c_str(), false);
	return;
      }
      std::string sysConfigFile = std::string(eyedb::CompileBuiltin::getSysconfdir()) + "/eyedb/" + defaultFilename;

      add(sysConfigFile.c_str(), true);
    }
  }

  Status
  ClientConfig::setConfigFile(const std::string &file)
  {
    if (instance)
      return Exception::make(IDB_INTERNAL_ERROR, "Cannot set client config file after configuration");

    config_file = file;
    
    return Success;
  }

  static Config::VarMap client_map;
  static Config::VarMap server_map;

  static struct C {

    C() {
#ifdef BUILTIN_VARS
      server_map["@bindir"] = true;
      server_map["@sbindir"] = true;
      server_map["@libdir"] = true;
      server_map["@sysconfdir"] = true;
      server_map["@databasedir"] = true;
      server_map["@pipedir"] = true;
      server_map["@tmpdir"] = true;
#endif
      client_map["port"] = true;
      client_map["tcp_port"] = true;
      client_map["host"] = true;
      client_map["user"] = true;
      client_map["passwd"] = true;
      client_map["dbm"] = true;

      server_map["databasedir"] = true;
      server_map["tmpdir"] = true;
      server_map["sopath"] = true;
      server_map["granted_dbm"] = true;
      server_map["default_dbm"] = true;
      server_map["access_file"] = true;
      server_map["maximum_memory_size"] = true;
      server_map["default_file_group"] = true;
      server_map["default_file_mask"] = true;
#ifdef HAVE_EYEDBSMD
      server_map["smdport"] = true;
#endif
      server_map["listen"] = true;
      server_map["oqlpath"] = true;
    }
  } _;

  ClientConfig::ClientConfig() : Config("client", client_map)
  {
  }

  ClientConfig* 
  ClientConfig::getInstance()
  {
    if (instance)
      return instance;

    instance = new ClientConfig();

    instance->setDefaults();

    instance->loadConfigFile(config_file, "EYEDBCONF", "eyedb.conf");

    return instance;
  }
  
  ServerConfig::ServerConfig() : Config("server", server_map)
  {
  }

  Status
  ServerConfig::setConfigFile(const std::string &file)
  {
    if (instance)
      return Exception::make(IDB_INTERNAL_ERROR, "Cannot set server config file after configuration");

    config_file = file;
    return Success;
  }

  ServerConfig* 
  ServerConfig::getInstance()
  {
    if (instance)
      return instance;

    instance = new ServerConfig();
    
    instance->setDefaults();

    instance->loadConfigFile(config_file, "EYEDBDCONF", "eyedbd.conf");

    return instance;
  }
}
