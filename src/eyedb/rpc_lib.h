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


#ifndef _EYEDB_RPCLIB_H
#define _EYEDB_RPCLIB_H

#include <eyedbsm/eyedbsm.h>
#include <eyedb/base_p.h>
#include <eyedblib/rpc.h>
#include <eyedblib/rpcdb.h>
#include <eyedblib/rpc_lib.h>

namespace eyedb {

  typedef rpc_StatusRec RPCStatusRec;

  typedef const RPCStatusRec *RPCStatus;

  enum {
    IDB_DBMCREATE = 0x100,
    IDB_DBMUPDATE,

    IDB_DBCREATE,
    IDB_DBDELETE,

    IDB_USER_ADD,
    IDB_USER_DELETE,
    IDB_USER_PASSWD_SET,
    IDB_PASSWD_SET,

    IDB_DEFAULT_DBACCESS_SET,
    IDB_USER_DBACCESS_SET,
    IDB_USER_SYSACCESS_SET,

    IDB_DBINFO,

    IDB_DBMOVE,
    IDB_DBCOPY,
    IDB_DBRENAME,

    IDB_BACKEND_INTERRUPT,

    IDB_TRANSACTION_BEGIN,
    IDB_TRANSACTION_COMMIT,
    IDB_TRANSACTION_ABORT,

    IDB_TRANSACTION_PARAMS_SET,
    IDB_TRANSACTION_PARAMS_GET,

    IDB_DBOPEN,
    IDB_DBOPENLOCAL,
    IDB_DBCLOSE,

    IDB_OBJECT_CREATE,
    IDB_OBJECT_READ,
    IDB_OBJECT_WRITE,
    IDB_OBJECT_DELETE,
    IDB_OBJECT_HEADER_READ,
    IDB_OBJECT_SIZE_MODIFY,
    IDB_OBJECT_CHECK,

    IDB_OID_MAKE,

    IDB_DATA_CREATE,
    IDB_DATA_READ,
    IDB_DATA_WRITE,
    IDB_DATA_DELETE,
    IDB_DATA_SIZE_GET,
    IDB_DATA_SIZE_MODIFY,

    IDB_VDDATA_CREATE,
    IDB_VDDATA_WRITE,
    IDB_VDDATA_DELETE,

    IDB_SCHEMA_COMPLETE,

    IDB_ATTRIBUTE_INDEX_CREATE,
    IDB_ATTRIBUTE_INDEX_REMOVE,
  
    IDB_INDEX_CREATE,
    IDB_INDEX_REMOVE,

    IDB_CONSTRAINT_CREATE,
    IDB_CONSTRAINT_DELETE,

    IDB_COLLECTION_GET_BY_IND,
    IDB_COLLECTION_GET_BY_VALUE,

    IDB_SET_OBJECT_LOCK,
    IDB_GET_OBJECT_LOCK,
  
    IDB_ITERATOR_LANG_CREATE,
    IDB_ITERATOR_DATABASE_CREATE,
    IDB_ITERATOR_CLASS_CREATE,
    IDB_ITERATOR_COLLECTION_CREATE,
    IDB_ITERATOR_ATTRIBUTE_CREATE,
    IDB_ITERATOR_DELETE,
    IDB_ITERATOR_SCAN_NEXT,

    IDB_EXECUTABLE_CHECK,
    IDB_EXECUTABLE_EXECUTE,
    IDB_EXECUTABLE_SET_EXTREF_PATH,
    IDB_EXECUTABLE_GET_EXTREF_PATH,

    IDB_SET_CONN_INFO,
    IDB_CHECK_AUTH,

    IDB_INDEX_GET_COUNT,
    IDB_INDEX_GET_STATS,
    IDB_INDEX_SIMUL_STATS,
    IDB_COLLECTION_GET_IMPLSTATS,
    IDB_COLLECTION_SIMUL_IMPLSTATS,
    IDB_INDEX_GET_IMPL,
    IDB_COLLECTION_GET_IMPL,

    IDB_OBJECT_PROTECTION_SET,
    IDB_OBJECT_PROTECTION_GET,

    IDB_OQL_CREATE,
    IDB_OQL_DELETE,
    IDB_OQL_GETRESULT,

    IDB_SET_LOG_MASK,

    IDB_GET_DEFAULT_DATASPACE,
    IDB_SET_DEFAULT_DATASPACE,

    IDB_DATASPACE_SET_CURRENT_DATAFILE,
    IDB_DATASPACE_GET_CURRENT_DATAFILE,

    IDB_GET_DEFAULT_INDEX_DATASPACE,
    IDB_SET_DEFAULT_INDEX_DATASPACE,
    IDB_GET_INDEX_LOCATIONS,
    IDB_MOVE_INDEX,
    IDB_GET_INSTANCE_CLASS_LOCATIONS,
    IDB_MOVE_INSTANCE_CLASS,
    IDB_GET_OBJECTS_LOCATIONS,
    IDB_MOVE_OBJECTS,
    IDB_GET_ATTRIBUTE_LOCATIONS,
    IDB_MOVE_ATTRIBUTE,

    IDB_CREATE_DATAFILE,
    IDB_DELETE_DATAFILE,
    IDB_MOVE_DATAFILE,
    IDB_DEFRAGMENT_DATAFILE,
    IDB_RESIZE_DATAFILE,
    IDB_GET_DATAFILEI_NFO,
    IDB_RENAME_DATAFILE,
    IDB_CREATE_DATASPACE,
    IDB_UPDATE_DATASPACE,
    IDB_DELETE_DATASPACE,
    IDB_RENAME_DATASPACE,

    IDB_GET_SERVER_OUTOFBAND_DATA,

    IDB_SET_MAXOBJCOUNT,
    IDB_GET_MAXOBJCOUNT,
    IDB_SET_LOGSIZE,
    IDB_GET_LOGSIZE
  };

  struct ConnHandle {
    rpc_ConnHandle *ch;
    //se_ConnHandle  *sech;
  };

#ifndef _IDB_KERN_

  struct DbHandle {
    ConnHandle *ch;
    rpcDB_LocalDBContext ldbctx;
    int tr_cnt;
    eyedbsm::Oid sch_oid;
    int flags;
    union {
      void *dbh;
      int rdbhid;
    } u;
    void *db;
  };
#endif


  extern rpc_RpcDescription
  *makeDBMCREATE(void),
    *makeDBMUPDATE(void),

    *makeDBCREATE(void),
    *makeDBDELETE(void),
    *makeDBINFO(void),
    *makeDBMOVE(void),
    *makeDBCOPY(void),
    *makeDBRENAME(void),

    *makeUSER_ADD(void),
    *makeUSER_DELETE(void),
    *makeUSER_PASSWD_SET(void),
    *makePASSWD_SET(void),

    *makeDEFAULT_DBACCESS_SET(void),
    *makeUSER_DBACCESS_SET(void),
    *makeUSER_SYSACCESS_SET(void),

    *makeBACKEND_INTERRUPT(void),

    *makeTRANSACTION_BEGIN(void),
    *makeTRANSACTION_COMMIT(void),
    *makeTRANSACTION_ABORT(void),

    *makeTRANSACTION_PARAMS_SET(void),
    *makeTRANSACTION_PARAMS_GET(void),

    *makeDBOPEN(void),
    *makeDBOPENLOCAL(void),
    *makeDBCLOSE(void),

    *makeOBJECT_CREATE(void),
    *makeOBJECT_READ(void),
    *makeOBJECT_WRITE(void),
    *makeOBJECT_DELETE(void),
    *makeOBJECT_HEADER_READ(void),
    *makeOBJECT_SIZE_MODIFY(void),
    *makeOBJECT_PROTECTION_SET(void),
    *makeOBJECT_PROTECTION_GET(void),
    *makeOBJECT_CHECK(void),

    *makeOID_MAKE(void),

    *makeDATA_CREATE(void),
    *makeDATA_READ(void),
    *makeDATA_WRITE(void),
    *makeDATA_DELETE(void),
    *makeDATA_SIZE_GET(void),
    *makeDATA_SIZE_MODIFY(void),

    *makeVDDATA_CREATE(void),
    *makeVDDATA_WRITE(void),
    *makeVDDATA_DELETE(void),

    *makeSCHEMA_COMPLETE(void),

    *makeATTRIBUTE_INDEX_CREATE(void),
    *makeATTRIBUTE_INDEX_REMOVE(void),

    *makeINDEX_CREATE(void),
    *makeINDEX_REMOVE(void),

    *makeCONSTRAINT_CREATE(void),
    *makeCONSTRAINT_DELETE(void),

    *makeCOLLECTION_GET_BY_IND(void),
    *makeCOLLECTION_GET_BY_VALUE(void),

    *makeSET_OBJECT_LOCK(void),
    *makeGET_OBJECT_LOCK(void),

    *makeQUERY_LANG_CREATE(void),
    *makeQUERY_DATABASE_CREATE(void),
    *makeQUERY_CLASS_CREATE(void),
    *makeQUERY_COLLECTION_CREATE(void),
    *makeQUERY_ATTRIBUTE_CREATE(void),
    *makeQUERY_DELETE(void),
    *makeQUERY_SCAN_NEXT(void),

    *makeEXECUTABLE_CHECK(void),
    *makeEXECUTABLE_EXECUTE(void),
    *makeEXECUTABLE_SET_EXTREF_PATH(void),
    *makeEXECUTABLE_GET_EXTREF_PATH(void),

    *makeOQL_CREATE(void),
    *makeOQL_DELETE(void),
    *makeOQL_GETRESULT(void),

    *makeSET_CONN_INFO(void),
    *makeCHECK_AUTH(void),

    *makeSET_LOG_MASK(void),

    *makeGET_DEFAULT_DATASPACE(void),
    *makeSET_DEFAULT_DATASPACE(void),
    *makeDATASPACE_SET_CURRENT_DATAFILE(void),
    *makeDATASPACE_GET_CURRENT_DATAFILE(void),
    *makeGET_DEFAULT_INDEX_DATASPACE(void),
    *makeSET_DEFAULT_INDEX_DATASPACE(void),
    *makeGET_INDEX_LOCATIONS(void),
    *makeMOVE_INDEX(void),
    *makeGET_INSTANCE_CLASS_LOCATIONS(void),
    *makeMOVE_INSTANCE_CLASS(void),
    *makeGET_OBJECTS_LOCATIONS(void),
    *makeMOVE_OBJECTS(void),
    *makeGET_ATTRIBUTE_LOCATIONS(void),
    *makeMOVE_ATTRIBUTE(void),

    *makeCREATE_DATAFILE(void),
    *makeDELETE_DATAFILE(void),
    *makeMOVE_DATAFILE(void),
    *makeDEFRAGMENT_DATAFILE(void),
    *makeRESIZE_DATAFILE(void),
    *makeGET_DATAFILEI_NFO(void),
    *makeRENAME_DATAFILE(void),
    *makeCREATE_DATASPACE(void),
    *makeUPDATE_DATASPACE(void),
    *makeDELETE_DATASPACE(void),
    *makeRENAME_DATASPACE(void),
    *makeINDEX_GET_COUNT(void),
    *makeINDEX_GET_STATS(void),
    *makeINDEX_SIMUL_STATS(void),
    *makeCOLLECTION_GET_IMPLSTATS(void),
    *makeCOLLECTION_SIMUL_IMPLSTATS(void),
    *makeINDEX_GET_IMPL(void),
    *makeCOLLECTION_GET_IMPL(void),
    *makeGET_SERVER_OUTOFBAND_DATA(void),
    *makeSET_MAXOBJCOUNT(void),
    *makeGET_MAXOBJCOUNT(void),
    *makeSET_LOGSIZE(void),
    *makeGET_LOGSIZE(void);

  extern rpc_ArgType
  rpcDB_LocalDBContextType,
    OidType,
    BoolType,
    RPCStatusType;

  extern RPCStatus
  rpcStatusMake(Error, const char *, ...);

  extern RPCStatus
  rpcStatusMake_s(Error);

  extern RPCStatus
  rpcStatusMake_se(eyedbsm::Status);

  extern RPCStatus _rpc_status__;

  extern const int CONN_COUNT;

  const eyedblib::uint32 RPC_PROTOCOL_MAGIC  = 0x43f2e341;
}

#endif
