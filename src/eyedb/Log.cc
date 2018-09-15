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


#include "eyedb_p.h"

namespace eyedb {

  Status Log::setLogMask(LogMask mask)
  {
    eyedblib::log_mask = mask;
    return Success;
  }

  Status Log::setLogMask(const char *str)
  {
    LogMask mask;
    Status s = logStringToMask(str, mask);
    if (s) return s;
    eyedblib::log_mask = mask;
    return Success;
  }

  LogMask Log::getLogMask()
  {
    return eyedblib::log_mask;
  }

  static const char *progName;
  static const char *logName;

  static Bool log_date;
  static Bool log_timer;
  static Bool log_pid;
  static Bool log_progname;

  static const char logdev_var[] = "logdev";
  static const char logmask_var[] = "logmask";

  Status Log::init(const char *_progName, const char *_logName)
  {
    progName = _progName;
    logName = (_logName ? _logName : eyedb::ServerConfig::getSValue(logdev_var));

    if (!logName)
      logName = eyedb::ClientConfig::getCValue(logdev_var);

    utlogInit(progName, logName);

    setLogDate(log_date);
    setLogTimer(log_timer);
    setLogPid(log_pid);
    setLogProgName(log_progname);

    if (getLogMask())
      return Success;

    const char *s = eyedb::ServerConfig::getSValue(logmask_var);
    if (!s)
      s = eyedb::ClientConfig::getCValue(logmask_var);

    if (s)
      return Log::setLogMask(s);

    return Log::setLogMask(IDB_LOG_DEFAULT);
  }

  const char *Log::getLog()
  {
    return logName;
  }

  void Log::setLog(const char *_logName)
  {
    logName = _logName;
    utlogInit(progName, logName);
  }

  void Log::setLogTimer(Bool on)
  {
    log_timer = on;
    utlogSetLogTimer(on);
  }

  Bool Log::getLogTimer()
  {
    return log_timer;
  }

  void Log::resetLogTimer()
  {
    utlogResetTimer();
  }

  void Log::setLogDate(Bool on)
  {
    log_date = on;
    utlogSetLogDate(on);
  }

  Bool Log::getLogDate()
  {
    return log_date;
  }

  void Log::setLogPid(Bool on)
  {
    log_pid = on;
    utlogSetLogPid(on);
  }

  Bool Log::getLogPid()
  {
    return log_pid;
  }

  void Log::setLogProgName(Bool on)
  {
    log_progname = on;
    utlogSetLogProgName(on);
  }

  Bool Log::getLogProgName()
  {
    return log_progname;
  }

#define APPEND(S) do {if (!first) str += "+"; first = 0; str += (S);} while(0)

  Status Log::logMaskToString(LogMask mask, std::string &str)
  {
    str = "";
    int first = 1;

    if (mask & IDB_LOG_LOCAL) APPEND("local");
    if (mask & IDB_LOG_SERVER) APPEND("server");
    if (mask & IDB_LOG_CONN) APPEND("connection");
    if (mask & IDB_LOG_TRANSACTION) APPEND("transaction");
    if (mask & IDB_LOG_DATABASE) APPEND("database");
    if (mask & IDB_LOG_ADMIN) APPEND("admin");
    if (mask & IDB_LOG_EXCEPTION) APPEND("exception");
    if (mask & IDB_LOG_OID_CREATE) APPEND("oid:create");
    if (mask & IDB_LOG_OID_READ) APPEND("oid:read");
    if (mask & IDB_LOG_OID_WRITE) APPEND("oid:write");
    if (mask & IDB_LOG_OID_DELETE) APPEND("oid:delete");
    if (mask & IDB_LOG_MMAP) APPEND("memory:map");
    if (mask & IDB_LOG_MMAP_DETAIL) APPEND("memory:map:detail");
    if (mask & IDB_LOG_MTX) APPEND("mutex");
    if (mask & IDB_LOG_IDX_CREATE) APPEND("index:create");
    if (mask & IDB_LOG_IDX_REMOVE) APPEND("index:remove");
    if (mask & IDB_LOG_IDX_INSERT) APPEND("index:insert");
    if (mask & IDB_LOG_IDX_SUPPRESS) APPEND("index:suppress");
    if (mask & IDB_LOG_IDX_SEARCH) APPEND("index:search");
    if (mask & IDB_LOG_IDX_SEARCH_DETAIL) APPEND("index:search:detail");
    if (mask & IDB_LOG_DEV) APPEND("dev");
    if (mask & IDB_LOG_OBJ_LOAD) APPEND("object:load");
    if (mask & IDB_LOG_OBJ_CREATE) APPEND("object:create");
    if (mask & IDB_LOG_OBJ_UPDATE) APPEND("object:update");
    if (mask & IDB_LOG_OBJ_REMOVE) APPEND("object:remove");
    if (mask & IDB_LOG_OBJ_GBX) APPEND("object:gbx");
    if (mask & IDB_LOG_OBJ_GARBAGE) APPEND("object:garbage");
    if (mask & IDB_LOG_OBJ_INIT) APPEND("object:init");
    if (mask & IDB_LOG_OBJ_COPY) APPEND("object:copy");
    if (mask & IDB_LOG_EXECUTE) APPEND("execute");
    if (mask & IDB_LOG_DATA_READ) APPEND("data:read");
    if (mask & IDB_LOG_DATA_CREATE) APPEND("data:create");
    if (mask & IDB_LOG_DATA_WRITE) APPEND("data:write");
    if (mask & IDB_LOG_DATA_DELETE) APPEND("data:delete");
    if (mask & IDB_LOG_OQL_EXEC) APPEND("oql:exec");
    if (mask & IDB_LOG_OQL_RESULT) APPEND("oql:result");

    for (int i = 0; i < IDB_LOG_USER_MAX; i++)
      if (mask & IDB_LOG_USER(i)) APPEND((std::string("user:") + str_convert((long)i)).c_str());

    if (mask & IDB_LOG_RELSHIP) APPEND("relationship");
    if (mask & IDB_LOG_RELSHIP_DETAILS) APPEND("relationship:details");
    if (mask & IDB_LOG_SCHEMA_EVOLVE) APPEND("schema:evolve");
    if (mask & IDB_LOG_NOLOG) APPEND("nolog");

    return Success;
  }

  static LogMask
  getMask(const char *p, std::string &err_str)
  {
    if (!strcmp(p, "default")) return IDB_LOG_DEFAULT;
    if (!strcmp(p, "local")) return IDB_LOG_LOCAL;
    if (!strcmp(p, "server")) return IDB_LOG_SERVER;
    if (!strcmp(p, "connection")) return IDB_LOG_CONN;
    if (!strcmp(p, "transaction")) return IDB_LOG_TRANSACTION;
    if (!strcmp(p, "database")) return IDB_LOG_DATABASE;
    if (!strcmp(p, "admin")) return IDB_LOG_ADMIN;
    if (!strcmp(p, "exception")) return IDB_LOG_EXCEPTION;
    if (!strcmp(p, "oid:create")) return IDB_LOG_OID_CREATE;
    if (!strcmp(p, "oid:read")) return IDB_LOG_OID_READ;
    if (!strcmp(p, "oid:write")) return IDB_LOG_OID_WRITE;
    if (!strcmp(p, "oid:delete")) return IDB_LOG_OID_DELETE;
    if (!strcmp(p, "oid:all")) return IDB_LOG_OID_ALL;
    if (!strcmp(p, "memory:map")) return IDB_LOG_MMAP;
    if (!strcmp(p, "memory:map:detail")) return IDB_LOG_MMAP_DETAIL;
    if (!strcmp(p, "mutex")) return IDB_LOG_MTX;
    if (!strcmp(p, "index:create")) return IDB_LOG_IDX_CREATE;
    if (!strcmp(p, "index:remove")) return IDB_LOG_IDX_REMOVE;
    if (!strcmp(p, "index:insert")) return IDB_LOG_IDX_INSERT;
    if (!strcmp(p, "index:suppress")) return IDB_LOG_IDX_SUPPRESS;
    if (!strcmp(p, "index:search")) return IDB_LOG_IDX_SEARCH;
    if (!strcmp(p, "index:search:detail")) return IDB_LOG_IDX_SEARCH_DETAIL;
    if (!strcmp(p, "index:all")) return IDB_LOG_IDX_ALL;
    if (!strcmp(p, "dev")) return IDB_LOG_DEV;
    if (!strcmp(p, "object:load")) return IDB_LOG_OBJ_LOAD;
    if (!strcmp(p, "object:create")) return IDB_LOG_OBJ_CREATE;
    if (!strcmp(p, "object:update")) return IDB_LOG_OBJ_UPDATE;
    if (!strcmp(p, "object:remove")) return IDB_LOG_OBJ_REMOVE;
    if (!strcmp(p, "object:gbx")) return IDB_LOG_OBJ_GBX;
    if (!strcmp(p, "object:garbage")) return IDB_LOG_OBJ_GARBAGE;
    if (!strcmp(p, "object:init")) return IDB_LOG_OBJ_INIT;
    if (!strcmp(p, "object:copy")) return IDB_LOG_OBJ_COPY;
    if (!strcmp(p, "object:alloc")) return IDB_LOG_OBJ_ALLOC;
    if (!strcmp(p, "object:all")) return IDB_LOG_OBJ_ALL;
    if (!strcmp(p, "execute")) return IDB_LOG_EXECUTE;
    if (!strcmp(p, "data:read")) return IDB_LOG_DATA_READ;
    if (!strcmp(p, "data:create")) return IDB_LOG_DATA_CREATE;
    if (!strcmp(p, "data:write")) return IDB_LOG_DATA_WRITE;
    if (!strcmp(p, "data:delete")) return IDB_LOG_DATA_DELETE;
    if (!strcmp(p, "data:all")) return IDB_LOG_DATA_ALL;
    if (!strcmp(p, "oql:exec")) return IDB_LOG_OQL_EXEC;
    if (!strcmp(p, "oql:result")) return IDB_LOG_OQL_RESULT;
    if (!strcmp(p, "relationship")) return IDB_LOG_RELSHIP;
    if (!strcmp(p, "relationship:details")) return IDB_LOG_RELSHIP_DETAILS;
    if (!strcmp(p, "schema:evolve")) return IDB_LOG_SCHEMA_EVOLVE;
    if (!strcmp(p, "nolog")) return IDB_LOG_NOLOG;

    for (int i = 0; i < IDB_LOG_USER_MAX; i++) {
      std::string s = std::string("user:") + str_convert((long)i);
      if (!strcmp(p, s.c_str()))
	return IDB_LOG_USER(i);
    }
      
    if (err_str != std::string(""))
      err_str += ", ";
    err_str += p;

    return 0;
  }

  Status Log::logStringToMask(const std::string &str, LogMask &mask)
  {
    if (sscanf(str.c_str(), "%xll", &mask) == 1)
      return Success;

    std::string err_str;
    char *s = strdup(str.c_str());
    char *p = s;
    char *q = s;
    mask = 0;
    Bool add;

    if (*q == '-') {
      add = False;
      p++; q++;
    }
    else if (*q == '+') {
      add = True;
      p++; q++;
    }
    else
      add = True;

    for ( ; (q = strpbrk(q, "+-")); ) {
      char c = *q;

      *q = 0;

      if (add)
	mask |= getMask(p, err_str);
      else
	mask &= ~getMask(p, err_str);

      p = ++q;

      add = (c == '+' ? True : False);
    }

    if (add)
      mask |= getMask(p, err_str);
    else
      mask &= ~getMask(p, err_str);

    free(s);

    if (err_str != std::string(""))
      return Exception::make(IDB_ERROR, "unknown mask string(s): \"%s\".\n%s",
			     err_str.c_str(),
			     getUsage().c_str());
    return Success;
  }

  std::string Log::getUsage()
  {
    std::string str;

    str = "The logmask is an hexadecimal number or a '+/-' combination of the following strings: "
      "default, "
      "dev, "
      "local, "
      "server, "
      "connection, "
      "transaction, "
      "database, "
      "admin, "
      "exception, "
      "oid:create, "
      "oid:read, "
      "oid:write, "
      "oid:delete, "
      "oid:all, "
      "memory:map, "
      "memory:map:detail, "
      "mutex, "
      "index:create, "
      "index:remove, "
      "index:extend, "
      "index:insert, "
      "index:suppress, "
      "index:search, "
      "index:search:detail, "
      "index:all, "
      "object:load, "
      "object:create, "
      "object:update, "
      "object:remove, "
      "object:all, "
      "object:gbx, "
      "object:garbage, "
      "object:copy, "
      "object:init, "
      "object:alloc, "
      "execute, "
      "data:read, "
      "data:create, "
      "data:write, "
      "data:delete, "
      "data:all, "
      "oql:exec, "
      "oql:result, "
      "relationship, "
      "relationship:details, ";

    str += std::string("user:[0-") + str_convert((long)(IDB_LOG_USER_MAX-1)) +
      std::string("], ");
    str += "nolog.";

    return str;
  }

  //
  // C function for DBX interpreteur
  //

  void setLogMask_int(LogMask mask)
  {
    Status s = Log::setLogMask(mask);
    if (s) s->print();
  }

  void setLogMask_str(const char *mask)
  {
    Status s = Log::setLogMask(mask);
    if (s) s->print();
  }

  LogMask getLogMask()
  {
    return Log::getLogMask();
  }

  const char *getLog()
  {
    return Log::getLog();
  }

  void setLog(const char *log)
  {
    Log::setLog(log);
  }

  void resetLogTimer()
  {
    Log::resetLogTimer();
  }

  void setLogTimer(int on)
  {
    Log::setLogTimer((Bool)on);
  }

  int getLogTimer()
  {
    return (int)Log::getLogTimer();
  }

  void setLogDate(int on)
  {
    Log::setLogDate((Bool)on);
  }

  int getLogDate()
  {
    return (int)Log::getLogDate();
  }

  void setLogPid(int on)
  {
    Log::setLogPid((Bool)on);
  }

  int getLogPid()
  {
    return (int)Log::getLogPid();
  }

  void setLogProgName(int on)
  {
    Log::setLogProgName((Bool)on);
  }

  int getLogProgName()
  {
    return (int)Log::getLogProgName();
  }
}
