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


/*
  il est inutile de passer par des OQLAtom: on peut utiliser directement
  les Value
*/

#include "eyedb_p.h"
#include "api_lib.h"
#include <assert.h>
#include "eyedblib/butils.h"

namespace eyedbsm {
  extern Boolean backend;
}

namespace eyedb {

#define CHKEXEC() \
do { \
  Status _status_; \
  if (_status_ = execute()) return _status_; \
  if (_status_ = getResult()) return _status_; \
} while(0)


  void
  clean(char *s)
  {
    int n;

    for (n = strlen(s)-1; n >= 0; n--)
      if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
	break;

    if (s[n] != ';')
      s[++n] = ';';

    s[n+1] = 0;
  }

  OQL::OQL(Database *_db, const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);
    char *s = eyedblib::getFBuffer(fmt, ap); 
    vsprintf(s, fmt, aq);
    clean(s);
    init(_db->getConnection(), _db, s);
    va_end(ap);
  }

  OQL::OQL(Connection *_conn, const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    char *s = eyedblib::getFBuffer(fmt, ap); 
    vsprintf(s, fmt, aq);
    clean(s);
    init(_conn, 0, s);
    va_end(ap);
  }

  void OQL::init(Connection *_conn, Database *_db, const char *s)
  {
    oql_string = strdup(s);
    schema_info = 0;
    state = True;
    db = _db;
    conn = _conn;
    qid = 0;
    executed = False;
    value_read = False;
    oql_status = Success;
  }

  Status
  OQL::initDatabase(Database *db)
  {
    Status status;
    status = db->transactionBegin();
    if (status)
      return status;
    OQL oql(db, "import \"stdlib\";");
    status = oql.execute();
    db->transactionCommit();
    return status;
  }

  //
  // execute methods
  //

  Status OQL::execute()
  {
    if (executed) return oql_status;

    IDB_LOG(IDB_LOG_OQL_EXEC, ("before '%s'\n", oql_string));

    RPCStatus rpc_status =
      oqlCreate(conn->getConnHandle(), (db ? db->getDbHandle() : 0),
		oql_string, &qid, &schema_info);

    if (!rpc_status)
      db->updateSchema(*schema_info);

    if (db->isLocal() || eyedbsm::backend)
      schema_info = 0;

    executed = True;

    if (rpc_status)
      oql_status = new Exception(*StatusMake(rpc_status));

    IDB_LOG(IDB_LOG_OQL_EXEC,
	    ("'%s' done => %s\n",
	     oql_string,
	     oql_status ? oql_status->getDesc() : "successful"));

    return oql_status;
  }

  void
  OQL::log_result() const
  {
    FILE *fd = utlogFDGet();
    if (!fd)
      return;

    utlog_p("IDB_LOG_OQL_RESULT");
    utlog("result value of '%s': '", oql_string);
    result_value.print(fd);
    fprintf(fd, "'\n");
    fflush(fd);
  }

  Status
  OQL::getResult()
  {
    if (value_read)
      return Success;

    Status status = StatusMake(oqlGetResult
			       (conn->getConnHandle(), 
				(db ? db->getDbHandle() : 0),
				qid, &result_value));

    if (status) return status;

    if ((IDB_LOG_OQL_RESULT & eyedblib::log_mask) == IDB_LOG_OQL_RESULT)
      log_result();

    value_read = True;
    return Success;
  }

  Status
  OQL::getResult(Value &value)
  {
    return execute(value);
  }

  Status
  OQL::getResult(ValueArray &val_array)
  {
    return execute(val_array);
  }

  Status
  OQL::getResult(ObjectPtrVector &obj_vect, const RecMode *rcm)
  {
    return execute(obj_vect, rcm);
  }

  Status
  OQL::getResult(ObjectArray &obj_array, const RecMode *rcm)
  {
    return execute(obj_array, rcm);
  }

  Status
  OQL::getResult(OidArray &oid_array)
  {
    return execute(oid_array);
  }

  Status
  OQL::execute(ValueArray &val_array)
  {
    CHKEXEC();
    result_value.toArray(val_array);
    return Success;
  }

  Status
  OQL::execute(ObjectPtrVector &obj_vect, const RecMode *rcm)
  {
    CHKEXEC();
    result_value.toArray(db, obj_vect, rcm);
    return Success;
  }

  Status
  OQL::execute(ObjectArray &obj_array, const RecMode *rcm)
  {
    CHKEXEC();
    result_value.toArray(db, obj_array, rcm);
    return Success;
  }

  Status
  OQL::execute(OidArray &oid_array)
  {
    CHKEXEC();
    result_value.toArray(oid_array);
    return Success;
  }

  Status
  OQL::execute(Value &value)
  {
    CHKEXEC();
    value = result_value;
    return Success;
  }

  OQL::~OQL()
  {
    if (db && db->getDbHandle())
      oqlDelete(conn->getConnHandle(), (db ? db->getDbHandle() : 0), qid);

    free(oql_string);
    delete schema_info;
  }
}
