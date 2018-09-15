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
#include "eyedblib/butils.h"

using namespace std;

namespace eyedb {

  Status _status_;

#define S_SIZE 2048

  Exception::Mode Exception::mode = Exception::StatusMode;
  void (*Exception::handler)(Status, void *);
  void *Exception::handler_user_data;
  void (*Exception::print_method)(Status, FILE*);

  // static functions

  static const char *getStringSeverity(SeverityStatus sev)
  {
    switch(sev)
      {
      case _Warning:
	return "warning";

      case _Error:
	return "error";

      case _FatalError:
	return "fatal error";
      }
  }

  static void
  setString(char **st, const char *fmt, va_list ap)
  {
    va_list aq;
    va_copy(aq, ap);

    char *s = eyedblib::getFBuffer(fmt, ap);
    vsprintf(s, fmt, aq);
    if (*st)
      free(*st);
    *st = (char *)malloc(strlen(s)+1);
    strcpy(*st, s);
  }

  //
  // Status
  //

#define MAKE_DESC(ST, MSG) \
statusDesc[ST].status_string = #ST; \
statusDesc[ST].desc = MSG

  struct Exception::_desc Exception::statusDesc[IDB_N_ERROR];

  void Exception::init(void)
  {
    if (!statusDesc[IDB_SUCCESS].status_string)
      {
	MAKE_DESC(IDB_SUCCESS, "operation succesful");
	MAKE_DESC(IDB_ERROR, "eyedb error");
	MAKE_DESC(IDB_FATAL_ERROR, "eyedb fatal error");
	MAKE_DESC(IDB_NOT_YET_IMPLEMENTED, "feature not yet implemented");
	MAKE_DESC(IDB_INTERNAL_ERROR, "eyedb internal error");
	MAKE_DESC(IDB_EXIT_CALLED, "exit function has been called");
	MAKE_DESC(IDB_SERVER_NOT_RUNNING, "eyedb server is not running");
	MAKE_DESC(IDB_CONNECTION_LOG_FILE_ERROR, "cannot access connection log file");
	MAKE_DESC(IDB_INTERNAL_BUG, "eyedb internal bug");
	MAKE_DESC(IDB_CONNECTION_FAILURE, "connection failure");
	MAKE_DESC(IDB_SERVER_FAILURE, "server failure");
	MAKE_DESC(IDB_SM_ERROR, "storage manager error");
	MAKE_DESC(IDB_INVALID_DBOPEN_FLAG, "invalid database open flag");
	MAKE_DESC(IDB_INVALID_DB_ID, "invalid database identifier");
	MAKE_DESC(IDB_INVALID_CLIENT_ID, "invalid client identifier");
	MAKE_DESC(IDB_INVALID_SCHEMA, "invalid schema");
	MAKE_DESC(IDB_INVALID_OBJECT_HEADER, "invalid object_header");
	MAKE_DESC(IDB_INVALID_TRANSACTION, "invalid transaction");
	MAKE_DESC(IDB_INVALID_TRANSACTION_MODE, "invalid transaction mode");
	MAKE_DESC(IDB_INVALID_TRANSACTION_WRITE_MODE, "invalid transaction write mode");
	MAKE_DESC(IDB_INVALID_TRANSACTION_PARAMS, "invalid transaction parameters");
	MAKE_DESC(IDB_AUTHENTICATION_NOT_SET, "authentication parameters are not set (check your 'user' and 'passwd' configuration variables or use -eyedbuser and -eyedbpasswd command line options)");
	MAKE_DESC(IDB_ADD_USER_ERROR, "add user error");
	MAKE_DESC(IDB_DELETE_USER_ERROR, "delete user error");
	MAKE_DESC(IDB_SET_USER_PASSWD_ERROR, "set user password error");
	MAKE_DESC(IDB_SET_PASSWD_ERROR, "set password error");
	MAKE_DESC(IDB_SET_USER_DBACCESS_ERROR, "set user database access error");
	MAKE_DESC(IDB_SET_DEFAULT_DBACCESS_ERROR, "set default database access error");
	MAKE_DESC(IDB_SET_USER_SYSACCESS_ERROR, "set user system access error");
	MAKE_DESC(IDB_AUTHENTICATION_FAILED, "authentication failed");
	MAKE_DESC(IDB_TRANSACTION_COMMIT_UNEXPECTED, "commit unexpected");
	MAKE_DESC(IDB_TRANSACTION_ABORT_UNEXPECTED, "abort unexpected");
	MAKE_DESC(IDB_INSUFFICIENT_PRIVILEGES, "insufficient privileges");
	MAKE_DESC(IDB_NO_CURRENT_TRANSACTION, "no current transaction");
	MAKE_DESC(IDB_DATABASE_OPEN_ERROR, "database open error");
	MAKE_DESC(IDB_DATABASE_CLOSE_ERROR, "database close error");
	MAKE_DESC(IDB_DATABASE_CREATE_ERROR, "database create error");
	MAKE_DESC(IDB_DATABASE_REMOVE_ERROR, "database remove error");
	MAKE_DESC(IDB_DATABASE_COPY_ERROR, "database copy error");
	MAKE_DESC(IDB_DATABASE_MOVE_ERROR, "database move error");
	MAKE_DESC(IDB_DATABASE_REMOVE_ERROR, "database remove error");
	MAKE_DESC(IDB_DATABASE_RENAME_ERROR, "database rename error");
	MAKE_DESC(IDB_DATABASE_LOAD_OBJECT_ERROR, "database load object error");
	MAKE_DESC(IDB_DATABASE_GET_OBJECT_CLASS_ERROR, "database get object class error");
	MAKE_DESC(IDB_INCONSISTANT_OBJECT_HEADERS, "inconsistant object_headers");
	MAKE_DESC(IDB_IS_OBJECT_OF_CLASS_ERROR, "is object of class error");
	MAKE_DESC(IDB_SETDATABASE_ERROR, "set database error");
	MAKE_DESC(IDB_OBJECT_REMOVE_ERROR, "object remove");
	MAKE_DESC(IDB_CANNOT_CREATE_SCHEMA, "cannot create schema");
	MAKE_DESC(IDB_CANNOT_UPDATE_SCHEMA, "cannot update schema");
	MAKE_DESC(IDB_SCHEMA_ALREADY_CREATED, "schema already created");
	MAKE_DESC(IDB_OBJECT_ALREADY_CREATED, "object already created");
	MAKE_DESC(IDB_OBJECT_NOT_CREATED, "object not yet created");
	MAKE_DESC(IDB_OUT_OF_MEMORY, "eyedb out of memory");
	MAKE_DESC(IDB_BACKEND_INTERRUPTED, "backend interrupted");
	MAKE_DESC(IDB_ITERATOR_ATTRIBUTE_NO_IDX, "no index for attribute");
	MAKE_DESC(IDB_ITERATOR_ATTRIBUTE_INVALID_SIZE, "attribute invalid size");
	MAKE_DESC(IDB_ITERATOR_ATTRIBUTE_INVALID_INDICE, "attribute invalid indice");
	MAKE_DESC(IDB_ITERATOR_ERROR, "query error");
	MAKE_DESC(IDB_DBM_ERROR, "database manager error");
	MAKE_DESC(IDB_SCHEMA_ERROR, "schema error");
	MAKE_DESC(IDB_OQL_SYNTAX_ERROR, "oql syntax error");
	MAKE_DESC(IDB_OQL_ERROR, "oql error");
	MAKE_DESC(IDB_OQL_INTERRUPTED, "oql interrupted");
	MAKE_DESC(IDB_CLASS_READ, "reading class");
	MAKE_DESC(IDB_ATTRIBUTE_ERROR, "attribute error");
	MAKE_DESC(IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR, "out of range attribute error");
	MAKE_DESC(IDB_ATTRIBUTE_INVERSE_ERROR, "attribute relationship error");
	MAKE_DESC(IDB_MAG_ORDER_ERROR, "magnitude order error");
	MAKE_DESC(IDB_NO_CLASS, "no class for object");
	MAKE_DESC(IDB_CLASS_NOT_CREATED, "class is not created");
	MAKE_DESC(IDB_CLASS_NOT_FOUND, "class not found");
	MAKE_DESC(IDB_INDEX_ERROR, "attribute index error");
	MAKE_DESC(IDB_ENUM_ERROR, "enum error");
	MAKE_DESC(IDB_COLLECTION_BACK_END_ERROR, "back end collection error");
	MAKE_DESC(IDB_COLLECTION_ITEM_SIZE_TOO_LARGE, "collection item size is too large");
	MAKE_DESC(IDB_COLLECTION_ITEM_SIZE_UNKNOWN, "collection unknown item size");
	MAKE_DESC(IDB_COLLECTION_ERROR, "collection error");
	MAKE_DESC(IDB_COLLECTION_LOCKED, "collection lock error");
	MAKE_DESC(IDB_COLLECTION_INSERT_ERROR, "collection insert error");
	MAKE_DESC(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "collection duplicate insert error");
	MAKE_DESC(IDB_COLLECTION_SUPPRESS_ERROR, "collection suppress error");
	MAKE_DESC(IDB_COLLECTION_IS_IN_ERROR, "collection is in error");
	MAKE_DESC(IDB_CLASS_COMPLETION_ERROR, "class completion error");
	MAKE_DESC(IDB_GENERATION_CODE_ERROR, "generation code error");
	MAKE_DESC(IDB_UNIQUE_CONSTRAINT_ERROR, "unique constraint error");
	MAKE_DESC(IDB_UNIQUE_COMP_CONSTRAINT_ERROR,
		  "unique[] constraint error");
	MAKE_DESC(IDB_NOTNULL_CONSTRAINT_ERROR, "not null constraint error");
	MAKE_DESC(IDB_NOTNULL_COMP_CONSTRAINT_ERROR,
		  "notnull[] constraint error");
	MAKE_DESC(IDB_CARDINALITY_CONSTRAINT_ERROR,
		  "cardinality constraint error");
	MAKE_DESC(IDB_EXECUTABLE_ERROR, "executable error");
	MAKE_DESC(IDB_SESSION_LOG_CREATION_ERROR, "session log creation error");
	MAKE_DESC(IDB_SESSION_LOG_OPEN_ERROR, "session log opening error");
	MAKE_DESC(IDB_SESSION_LOG_NO_SPACE_LEFT, "no space left on session log");
	MAKE_DESC(IDB_UNSERIALIZABLE_TYPE_ERROR, "non serializable type used in serialization");

	// checking
	int error = 0;
	for (int i = IDB_ERROR; i < IDB_N_ERROR; i++)
	  if (!statusDesc[i].desc)
	    {
	      fprintf(stderr, "missing status description: #%d\n", i);
	      error++;
	    }

	if (error) abort();
      }
  }

  void Exception::_release(void)
  {
  }

  void Exception::statusMake(Status p, int sta, SeverityStatus sev)
  {
    parent = (Exception *)p;
    status = sta;
    severity = sev;
    print_method = 0;

    string = 0;
    user_data = 0;
  }

  Exception::Exception(int sta, SeverityStatus sev)
  {
    statusMake(0, sta, sev);
  }

  Exception::Exception(Status p, int sta, SeverityStatus sev)
  {
    statusMake(p, sta, sev);
  }

  Exception::Exception(const Exception &e)
  {
    *this = e;
  }

  Exception& Exception::operator=(const Exception &e)
  {
    parent   = e.parent;
    status   = e.status;
    string   = e.string ? strdup(e.string) : 0;
    severity = e.severity;
    return *this;
  }

  Status Exception::setSeverity(SeverityStatus sev)
  {
    severity = sev;
    return this;
  }

  SeverityStatus Exception::getSeverity(void) const
  {
    return severity;
  }

  void Exception::setStatus(int sta)
  {
    status = sta;
  }

  int Exception::getStatus(void) const
  {
    return status;
  }

  Status Exception::setString(const char *fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);

    eyedb::setString(&string, fmt, ap);

    va_end(ap);
    return this;
  }

  Status Exception::setString(SeverityStatus sev, const char *fmt, ...)
  {
    va_list ap;

    setSeverity(sev);
    va_start(ap, fmt);

    eyedb::setString(&string, fmt, ap);

    va_end(ap);
    return this;
  }

  Status Exception::setString(int sta, SeverityStatus sev, const char *fmt, ...)
  {
    /*
      va_list ap;

      va_start(ap, fmt);

      ::setString(&string, fmt, ap);

      va_end(ap);
      return this;
    */
    setSeverity(sev);
    setStatus(sta);
    string = strdup(fmt);
    return this;
  
  }

  const char *Exception::getDesc(void) const
  {
    static std::string str;

    str = "";

    if (status >= IDB_ERROR && status < IDB_N_ERROR)
      {
	const char *desc = statusDesc[(int)status].desc;
	if (desc && *desc)
	  str = desc;
	else
	  str = statusDesc[(int)status].status_string;
      }
    else if (status >= eyedbsm::SUCCESS && status < IDB_ERROR)
      str = eyedbsm::statusGet_err((int)status);
    else
      str = "unknown error: probably connection failure";
  
    if (string && *string)
      {
	if (str.size())
	  str += ": ";
	str += string;
      }

    return str.c_str();
  }

  const char *Exception::getString(void) const
  {
    return (string ? string : "");
  }

  void Exception::setPrintMethod(void (*pm)(Status, FILE*))
  {
    print_method = pm;
  }

  void (*Exception::getPrintMethod(void))(Status, FILE*)
  {
    return print_method;
  }

  Status Exception::setUserData(Any ud)
  {
    user_data = ud;
    return this;
  }

  Any Exception::getUserData(void) const
  {
    return user_data;
  }

  Status Exception::getParent() const
  {
    return parent;
  }

  void Exception::print_realize(FILE *fd, bool newline) const
  {
    fprintf(fd, "%s%s", getDesc(), (newline ? "\n" : ""));
    fflush(fd);
  }

  Status Exception::print(FILE *fd, bool newline) const
  {
    print_realize(fd, newline);
    return this;
  }

  void Exception::setHandler(void (*_handler)(Status, void *), void *hud)
  {
    handler = _handler;
    handler_user_data = hud;
  }

  void (*Exception::getHandler())(Status, void *)
  {
    return handler;
  }

  void *Exception::getHandlerUserData()
  {
    return handler_user_data;
  }

  static bool ECHO_ERR = getenv("EYEDB_ECHO_ERROR") ? true : false;

  void
  stop_here()
  {
  }

  void Exception::applyHandler() const
  {
    IDB_LOG(IDB_LOG_EXCEPTION, ("%s\n", getDesc()));

    if (ECHO_ERR)
      fprintf(stderr, "%s\n", getDesc());

    stop_here();

    if (handler)
      (*handler)(this, handler_user_data);

    if (mode == ExceptionMode) {
      throw *this;
    }
  }

  Exception::Mode Exception::setMode(Exception::Mode _mode)
  {
    Mode omode = mode;
    mode = _mode;
    return omode;
  }

  Exception::Mode Exception::getMode()
  {
    return mode;
  }

  Exception::~Exception()
  {
    free(string);
  }

  static inline Exception *getError()
  {
    static const int NERRS = 12;
    static Exception error[NERRS];
    static int err;

    if (err >= NERRS)
      err = 0;

    return &error[err++];
  }

  Status Exception::make(int err, const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    if (err == IDB_SUCCESS)
      return Success;

    char *s = eyedblib::getFBuffer(fmt, ap);
  
    vsprintf(s, fmt, aq);
  
    Exception *error = getError();
    error->setString(err, _Error, s);

    error->applyHandler();
    va_end(ap);
    return error;
  }

  Status Exception::make(int err, const std::string &s)
  {
    if (err == IDB_SUCCESS)
      return Success;

    Exception *error = getError();
    error->setString(err, _Error, s.c_str());

    error->applyHandler();
    return error;
  }

  Status Exception::make(const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    char *s = eyedblib::getFBuffer(fmt, ap);

    vsprintf(s, fmt, aq);

    Exception *error = getError();
    error->setString(IDB_ERROR, _Error, s);

    error->applyHandler();
    va_end(ap);
    return error;
  }

  Status Exception::make(const std::string &s)
  {
    Exception *error = getError();
    error->setString(IDB_ERROR, _Error, s.c_str());

    error->applyHandler();
    return error;
  }

  Status Exception::make(int err)
  {
    if (err == IDB_SUCCESS)
      return Success;

    Exception *error = getError();
    error->setString(err, _Error, "");
    error->applyHandler();
    return error;
  }

  Status Exception::make(Status status)
  {
    if (status)
      status->applyHandler();

    return status;
  }

  Status StatusMake(Error err, RPCStatus rpc_status,
		    const char *fmt, ...)
  {
    if (rpc_status && err != IDB_SUCCESS)
      {
	va_list ap, aq;
	va_start(ap, fmt);
	va_copy(aq, ap);

	char *s = eyedblib::getFBuffer(fmt, ap);

	vsprintf(s, fmt, aq);
      
	char t[512];
      
	sprintf(t, "%s: %s", s, rpc_status->err_msg);
      
	Exception *error = getError();
	error->setString(err, _Error, t);
      
	error->applyHandler();
	va_end(ap);
	return error;
      }

    return Success;
  }

  Status StatusMake(RPCStatus rpc_status)
  {
    if (rpc_status)
      return Exception::make(rpc_status->err, std::string(rpc_status->err_msg));

    return Success;
  }

  Status StatusMake(Error err, RPCStatus rpc_status)
  {
    if (rpc_status)
      {
	static char *str;
	Status status = Exception::make(rpc_status->err,
					std::string(rpc_status->err_msg));

	delete str;
	str = strdup(status->getDesc());

	return Exception::make(err, std::string(str));
      }

    return Success;
  }

  static void
  percent_manage(std::string &prefix)
  {
    if (!strchr(prefix.c_str(), '%'))
      return;

    char *p = (char *)malloc(strlen(prefix.c_str())*2+1);
    char *q = p;
    const char *x = prefix.c_str();
    char c;
    while (c = *x++)
      {
	*q++ = c;

	if (c == '%')
	  *q++ = '%';
      }
    *q = 0;
    prefix = p;
    free(p);
  }

  RPCStatus rpcStatusMake(Status status)
  {
    if (status)
      {
	std::string s = status->getString();
	percent_manage(s);
	return rpcStatusMake((Error)status->getStatus(), s.c_str());
      }

    return RPCSuccess;
  }

  ostream& operator<<(ostream& os, const Exception &e)
  {
    e.print(get_file(), false);
    return convert_to_stream(os);
  }

  ostream& operator<<(ostream& os, Status s)
  {
    if (!s)
      return os;
    s->print(get_file(), false);
    return convert_to_stream(os);
  }
}
