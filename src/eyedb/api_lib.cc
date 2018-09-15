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


#include <assert.h>
#include "eyedb/base.h"
#include "eyedb/Error.h"
#include "eyedb/Exception.h"
#include "eyedb/TransactionParams.h"
#include "api_lib.h"
#include "eyedblib/xdr.h"
#include <eyedbsm/xdr.h>

namespace eyedb {

  static rpc_Client *client;

  rpc_ClientFunction
  *DBMCREATE_RPC,
    *DBMUPDATE_RPC,

    *DBCREATE_RPC,
    *DBDELETE_RPC,

    *DBINFO_RPC,
    *DBMOVE_RPC,
    *DBCOPY_RPC,
    *DBRENAME_RPC,

    *USER_ADD_RPC,
    *USER_DELETE_RPC,
    *USER_PASSWD_SET_RPC,
    *PASSWD_SET_RPC,

    *DEFAULT_DBACCESS_SET_RPC,
    *USER_DBACCESS_SET_RPC,
    *USER_SYSACCESS_SET_RPC,

    *BACKEND_INTERRUPT_RPC,

    *TRANSACTION_BEGIN_RPC,
    *TRANSACTION_ABORT_RPC,
    *TRANSACTION_COMMIT_RPC,

    *TRANSACTION_PARAMS_SET_RPC,
    *TRANSACTION_PARAMS_GET_RPC,

    *DBOPEN_RPC,
    *DBOPENLOCAL_RPC,
    *DBCLOSE_RPC,

    *OBJECT_CREATE_RPC,
    *OBJECT_READ_RPC,
    *OBJECT_WRITE_RPC,
    *OBJECT_DELETE_RPC,
    *OBJECT_HEADER_READ_RPC,
    *OBJECT_SIZE_MODIFY_RPC,
    *OBJECT_PROTECTION_SET_RPC,
    *OBJECT_PROTECTION_GET_RPC,
    *OBJECT_CHECK_RPC,

    *OID_MAKE_RPC,

    *DATA_CREATE_RPC,
    *DATA_READ_RPC,
    *DATA_WRITE_RPC,
    *DATA_DELETE_RPC,
    *DATA_SIZE_GET_RPC,
    *DATA_SIZE_MODIFY_RPC,

    *VDDATA_CREATE_RPC,
    *VDDATA_WRITE_RPC,
    *VDDATA_DELETE_RPC,

    *SCHEMA_COMPLETE_RPC,

    *ATTRIBUTE_INDEX_CREATE_RPC,
    *ATTRIBUTE_INDEX_REMOVE_RPC,

    *INDEX_CREATE_RPC,
    *INDEX_REMOVE_RPC,

    *CONSTRAINT_CREATE_RPC,
    *CONSTRAINT_DELETE_RPC,

    *COLLECTION_GET_BY_IND_RPC,
    *COLLECTION_GET_BY_VALUE_RPC,

    *SET_OBJECT_LOCK_RPC,
    *GET_OBJECT_LOCK_RPC,

    *QUERY_LANG_CREATE_RPC,
    *QUERY_DATABASE_CREATE_RPC,
    *QUERY_CLASS_CREATE_RPC,
    *QUERY_COLLECTION_CREATE_RPC,
    *QUERY_ATTRIBUTE_CREATE_RPC,
    *QUERY_DELETE_RPC,
    *QUERY_SCAN_NEXT_RPC,

    *EXECUTABLE_CHECK_RPC,
    *EXECUTABLE_EXECUTE_RPC,
    *EXECUTABLE_SET_EXTREF_PATH_RPC,
    *EXECUTABLE_GET_EXTREF_PATH_RPC,

    *OQL_CREATE_RPC,
    *OQL_DELETE_RPC,
    *OQL_GETRESULT_RPC,

    *SET_CONN_INFO_RPC,
    *CHECK_AUTH_RPC,

    *SET_LOG_MASK_RPC,

    *INDEX_GET_COUNT_RPC,
    *INDEX_GET_STATS_RPC,
    *INDEX_SIMUL_STATS_RPC,
    *COLLECTION_GET_IMPLSTATS_RPC,
    *COLLECTION_SIMUL_IMPLSTATS_RPC,
    *INDEX_GET_IMPL_RPC,
    *COLLECTION_GET_IMPL_RPC,

    *GET_DEFAULT_DATASPACE_RPC,
    *SET_DEFAULT_DATASPACE_RPC,

    *DATASPACE_SET_CURRENT_DATAFILE_RPC,
    *DATASPACE_GET_CURRENT_DATAFILE_RPC,

    *GET_DEFAULT_INDEX_DATASPACE_RPC,
    *SET_DEFAULT_INDEX_DATASPACE_RPC,
    *GET_INDEX_LOCATIONS_RPC,
    *MOVE_INDEX_RPC,
    *GET_INSTANCE_CLASS_LOCATIONS_RPC,
    *MOVE_INSTANCE_CLASS_RPC,
    *GET_OBJECTS_LOCATIONS_RPC,
    *MOVE_OBJECTS_RPC,
    *GET_ATTRIBUTE_LOCATIONS_RPC,
    *MOVE_ATTRIBUTE_RPC,

    *CREATE_DATAFILE_RPC,
    *DELETE_DATAFILE_RPC,
    *MOVE_DATAFILE_RPC,
    *DEFRAGMENT_DATAFILE_RPC,
    *RESIZE_DATAFILE_RPC,
    *GET_DATAFILEI_NFO_RPC,
    *RENAME_DATAFILE_RPC,
    *CREATE_DATASPACE_RPC,
    *UPDATE_DATASPACE_RPC,
    *DELETE_DATASPACE_RPC,
    *RENAME_DATASPACE_RPC,
    *GET_SERVER_OUTOFBAND_DATA_RPC,

    *SET_MAXOBJCOUNT_RPC,
    *GET_MAXOBJCOUNT_RPC,
    *SET_LOGSIZE_RPC,
    *GET_LOGSIZE_RPC;

  rpc_ArgType
  rpcDB_LocalDBContextType,
    OidType,
    /*  BoolType,*/
    RPCStatusType;

  static void status_ua_client(rpc_Arg *, char **, void *,
			       rpc_SendRcv, rpc_FromTo);

  static void oid_ua_client(rpc_Arg *, char **, void *,
			    rpc_SendRcv, rpc_FromTo);

  /*#define STATUS_VARIABLE*/

  static int STATUS_SZ;

  rpc_Client *
  rpcFeInit()
  {
    /* make client */
    client = rpc_clientCreate();

    /* make types */

    STATUS_SZ = getenv("STATUS_SZ") ? atoi(getenv("STATUS_SZ")) :
      sizeof(RPCStatusRec);

    /*printf("STATUS_SZ client is %d\n", STATUS_SZ);*/
    rpcDB_LocalDBContextType = rpc_makeClientUserType(client, sizeof(rpcDB_LocalDBContext), 0, rpc_False);
    /*  BoolType = rpc_makeClientUserType(client, sizeof(Bool), 0, 0); */
    OidType = rpc_makeClientUserType(client, sizeof(eyedbsm::Oid), oid_ua_client, rpc_False);

#ifdef STATUS_VARIABLE
    RPCStatusType = rpc_makeClientUserType(client, rpc_SizeVariable, status_ua_client, rpc_False);
#else
    RPCStatusType = rpc_makeClientUserType(client, STATUS_SZ+4, status_ua_client, rpc_False);
#endif

    /* make functions */

    DBMCREATE_RPC =
      rpc_makeUserClientFunction(client, makeDBMCREATE());

    DBMUPDATE_RPC =
      rpc_makeUserClientFunction(client, makeDBMUPDATE());

    DBCREATE_RPC =
      rpc_makeUserClientFunction(client, makeDBCREATE());

    DBDELETE_RPC =
      rpc_makeUserClientFunction(client, makeDBDELETE());

    DBINFO_RPC =
      rpc_makeUserClientFunction(client, makeDBINFO());

    DBMOVE_RPC =
      rpc_makeUserClientFunction(client, makeDBMOVE());

    DBCOPY_RPC =
      rpc_makeUserClientFunction(client, makeDBCOPY());

    USER_ADD_RPC =
      rpc_makeUserClientFunction(client, makeUSER_ADD());

    USER_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeUSER_DELETE());

    USER_PASSWD_SET_RPC =
      rpc_makeUserClientFunction(client, makeUSER_PASSWD_SET());

    PASSWD_SET_RPC =
      rpc_makeUserClientFunction(client, makePASSWD_SET());

    DEFAULT_DBACCESS_SET_RPC =
      rpc_makeUserClientFunction(client, makeDEFAULT_DBACCESS_SET());

    USER_DBACCESS_SET_RPC =
      rpc_makeUserClientFunction(client, makeUSER_DBACCESS_SET());

    USER_SYSACCESS_SET_RPC =
      rpc_makeUserClientFunction(client, makeUSER_SYSACCESS_SET());

    DBRENAME_RPC =
      rpc_makeUserClientFunction(client, makeDBRENAME());

    BACKEND_INTERRUPT_RPC =
      rpc_makeUserClientFunction(client, makeBACKEND_INTERRUPT());

    TRANSACTION_BEGIN_RPC =
      rpc_makeUserClientFunction(client, makeTRANSACTION_BEGIN());

    TRANSACTION_COMMIT_RPC =
      rpc_makeUserClientFunction(client, makeTRANSACTION_COMMIT());

    TRANSACTION_ABORT_RPC =
      rpc_makeUserClientFunction(client, makeTRANSACTION_ABORT());

    TRANSACTION_PARAMS_SET_RPC =
      rpc_makeUserClientFunction(client, makeTRANSACTION_PARAMS_SET());

    TRANSACTION_PARAMS_GET_RPC =
      rpc_makeUserClientFunction(client, makeTRANSACTION_PARAMS_GET());

    DBOPEN_RPC =
      rpc_makeUserClientFunction(client, makeDBOPEN());

    DBOPENLOCAL_RPC =
      rpc_makeUserClientFunction(client, makeDBOPENLOCAL());

    DBCLOSE_RPC =
      rpc_makeUserClientFunction(client, makeDBCLOSE());

    /*
      SCHEMA_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeSCHEMA_CREATE());

      CLASS_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeCLASS_CREATE());

    */

    OBJECT_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_CREATE());

    OBJECT_WRITE_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_WRITE());

    OBJECT_READ_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_READ());

    OBJECT_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_DELETE());

    OBJECT_HEADER_READ_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_HEADER_READ());

    OBJECT_SIZE_MODIFY_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_SIZE_MODIFY());

    OBJECT_CHECK_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_CHECK());

    OBJECT_PROTECTION_SET_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_PROTECTION_SET());

    OBJECT_PROTECTION_GET_RPC =
      rpc_makeUserClientFunction(client, makeOBJECT_PROTECTION_GET());

    OID_MAKE_RPC =
      rpc_makeUserClientFunction(client, makeOID_MAKE());

    DATA_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeDATA_CREATE());

    DATA_WRITE_RPC =
      rpc_makeUserClientFunction(client, makeDATA_WRITE());

    DATA_READ_RPC =
      rpc_makeUserClientFunction(client, makeDATA_READ());

    DATA_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeDATA_DELETE());

    DATA_SIZE_GET_RPC =
      rpc_makeUserClientFunction(client, makeDATA_SIZE_GET());

    DATA_SIZE_MODIFY_RPC =
      rpc_makeUserClientFunction(client, makeDATA_SIZE_MODIFY());

    VDDATA_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeVDDATA_CREATE());

    VDDATA_WRITE_RPC =
      rpc_makeUserClientFunction(client, makeVDDATA_WRITE());

    VDDATA_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeVDDATA_DELETE());

    SCHEMA_COMPLETE_RPC =
      rpc_makeUserClientFunction(client, makeSCHEMA_COMPLETE());

    ATTRIBUTE_INDEX_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeATTRIBUTE_INDEX_CREATE());

    ATTRIBUTE_INDEX_REMOVE_RPC =
      rpc_makeUserClientFunction(client, makeATTRIBUTE_INDEX_REMOVE());

    INDEX_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_CREATE());

    INDEX_REMOVE_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_REMOVE());

    CONSTRAINT_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeCONSTRAINT_CREATE());

    CONSTRAINT_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeCONSTRAINT_DELETE());

    COLLECTION_GET_BY_IND_RPC =
      rpc_makeUserClientFunction(client, makeCOLLECTION_GET_BY_IND());

    COLLECTION_GET_BY_VALUE_RPC =
      rpc_makeUserClientFunction(client, makeCOLLECTION_GET_BY_VALUE());

    SET_OBJECT_LOCK_RPC =
      rpc_makeUserClientFunction(client, makeSET_OBJECT_LOCK());

    GET_OBJECT_LOCK_RPC =
      rpc_makeUserClientFunction(client, makeGET_OBJECT_LOCK());

    QUERY_LANG_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_LANG_CREATE());

    QUERY_DATABASE_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_DATABASE_CREATE());

    QUERY_CLASS_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_CLASS_CREATE());

    QUERY_COLLECTION_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_COLLECTION_CREATE());

    QUERY_ATTRIBUTE_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_ATTRIBUTE_CREATE());

    QUERY_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_DELETE());

    QUERY_SCAN_NEXT_RPC =
      rpc_makeUserClientFunction(client, makeQUERY_SCAN_NEXT());

    EXECUTABLE_CHECK_RPC = 
      rpc_makeUserClientFunction(client, makeEXECUTABLE_CHECK());

    EXECUTABLE_EXECUTE_RPC = 
      rpc_makeUserClientFunction(client, makeEXECUTABLE_EXECUTE());

    EXECUTABLE_SET_EXTREF_PATH_RPC = 
      rpc_makeUserClientFunction(client,
				 makeEXECUTABLE_SET_EXTREF_PATH());

    EXECUTABLE_GET_EXTREF_PATH_RPC = 
      rpc_makeUserClientFunction(client,
				 makeEXECUTABLE_GET_EXTREF_PATH());

    OQL_CREATE_RPC =
      rpc_makeUserClientFunction(client, makeOQL_CREATE());

    OQL_DELETE_RPC =
      rpc_makeUserClientFunction(client, makeOQL_DELETE());

    OQL_GETRESULT_RPC =
      rpc_makeUserClientFunction(client, makeOQL_GETRESULT());

    SET_CONN_INFO_RPC = 
      rpc_makeUserClientFunction(client, makeSET_CONN_INFO());

    CHECK_AUTH_RPC =
      rpc_makeUserClientFunction(client, makeCHECK_AUTH());

    SET_LOG_MASK_RPC =
      rpc_makeUserClientFunction(client, makeSET_LOG_MASK());

    INDEX_GET_COUNT_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_GET_COUNT());

    INDEX_GET_STATS_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_GET_STATS());

    INDEX_SIMUL_STATS_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_SIMUL_STATS());

    COLLECTION_GET_IMPLSTATS_RPC =
      rpc_makeUserClientFunction(client, makeCOLLECTION_GET_IMPLSTATS());

    COLLECTION_SIMUL_IMPLSTATS_RPC =
      rpc_makeUserClientFunction(client, makeCOLLECTION_SIMUL_IMPLSTATS());

    INDEX_GET_IMPL_RPC =
      rpc_makeUserClientFunction(client, makeINDEX_GET_IMPL());

    COLLECTION_GET_IMPL_RPC =
      rpc_makeUserClientFunction(client, makeCOLLECTION_GET_IMPL());

    GET_DEFAULT_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeGET_DEFAULT_DATASPACE());

    SET_DEFAULT_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeSET_DEFAULT_DATASPACE());

    DATASPACE_SET_CURRENT_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeDATASPACE_SET_CURRENT_DATAFILE());

    DATASPACE_GET_CURRENT_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeDATASPACE_GET_CURRENT_DATAFILE());

    GET_DEFAULT_INDEX_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeGET_DEFAULT_INDEX_DATASPACE());

    SET_DEFAULT_INDEX_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeSET_DEFAULT_INDEX_DATASPACE());

    GET_INDEX_LOCATIONS_RPC =
      rpc_makeUserClientFunction(client, makeGET_INDEX_LOCATIONS());

    MOVE_INDEX_RPC =
      rpc_makeUserClientFunction(client, makeMOVE_INDEX());

    GET_INSTANCE_CLASS_LOCATIONS_RPC =
      rpc_makeUserClientFunction(client, makeGET_INSTANCE_CLASS_LOCATIONS());

    MOVE_INSTANCE_CLASS_RPC =
      rpc_makeUserClientFunction(client, makeMOVE_INSTANCE_CLASS());

    GET_OBJECTS_LOCATIONS_RPC =
      rpc_makeUserClientFunction(client, makeGET_OBJECTS_LOCATIONS());

    MOVE_OBJECTS_RPC =
      rpc_makeUserClientFunction(client, makeMOVE_OBJECTS());

    GET_ATTRIBUTE_LOCATIONS_RPC =
      rpc_makeUserClientFunction(client, makeGET_ATTRIBUTE_LOCATIONS());

    MOVE_ATTRIBUTE_RPC =
      rpc_makeUserClientFunction(client, makeMOVE_ATTRIBUTE());

    CREATE_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeCREATE_DATAFILE());

    DELETE_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeDELETE_DATAFILE());

    MOVE_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeMOVE_DATAFILE());

    DEFRAGMENT_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeDEFRAGMENT_DATAFILE());

    RESIZE_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeRESIZE_DATAFILE());

    GET_DATAFILEI_NFO_RPC =
      rpc_makeUserClientFunction(client, makeGET_DATAFILEI_NFO());

    RENAME_DATAFILE_RPC =
      rpc_makeUserClientFunction(client, makeRENAME_DATAFILE());

    CREATE_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeCREATE_DATASPACE());

    UPDATE_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeUPDATE_DATASPACE());

    DELETE_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeDELETE_DATASPACE());

    RENAME_DATASPACE_RPC =
      rpc_makeUserClientFunction(client, makeRENAME_DATASPACE());

    GET_SERVER_OUTOFBAND_DATA_RPC =
      rpc_makeUserClientFunction(client, makeGET_SERVER_OUTOFBAND_DATA());

    SET_MAXOBJCOUNT_RPC =
      rpc_makeUserClientFunction(client, makeSET_MAXOBJCOUNT());

    GET_MAXOBJCOUNT_RPC =
      rpc_makeUserClientFunction(client, makeGET_MAXOBJCOUNT());

    SET_LOGSIZE_RPC =
      rpc_makeUserClientFunction(client, makeSET_LOGSIZE());

    GET_LOGSIZE_RPC =
      rpc_makeUserClientFunction(client, makeGET_LOGSIZE());

    /* declare arg size */
    rpc_setClientArgSize(client, sizeof(ClientArg));

    return client;
  }

  void
  rpcFeRelease()
  {
  }

  rpc_Client *
  getRpcClient()
  {
    return client;
  }

  /*
  void
  eyedb_init()
  {
    eyedbsm::init();
    rpcFeInit();
  }
  */

#include "eyedblib/rpc_lib.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

  static void
  status_ua_client(rpc_Arg *arg, char **pbuff, void *pua,
		   rpc_SendRcv send_rcv, rpc_FromTo fromto)
  {
    RPCStatusRec *s = (RPCStatusRec *)pua;
    char *buff = *pbuff;

    rpc_copy_fast_xdr(arg, buff, &s->err, sizeof(eyedblib::int32), send_rcv, fromto,
		      x2h_32_cpy, h2x_32_cpy);
    if ((arg->send_rcv & rpc_Send))
      assert(0);
    else if ((arg->send_rcv & rpc_Rcv) && fromto == rpc_From)
      {
#ifdef STATUS_VARIABLE
	strcpy(s->err_msg, buff);
	printf("receiving status '%s'\n", buff);
	*pbuff += sizeof(eyedblib::int32)+strlen(buff)+1;
#else
	strncpy(s->err_msg, buff, STATUS_SZ-4);
	s->err_msg[MIN(STATUS_SZ-4, strlen(buff))] = 0;
	*pbuff += STATUS_SZ;
#endif
      }
  }

  static void
  oid_ua_client(rpc_Arg *arg, char **pbuff, void *pua,
		rpc_SendRcv send_rcv, rpc_FromTo fromto)
  {
    eyedbsm::Oid oid;
    
    if (send_rcv & arg->send_rcv) {
      if (fromto == rpc_To) {
	memcpy(&oid, pua, sizeof(oid));
	eyedbsm::h2x_oid(&oid, &oid);
	memcpy(*pbuff, &oid, sizeof(oid));
      }
      else {
	memcpy(&oid, *pbuff, sizeof(oid));
	eyedbsm::x2h_oid(&oid, &oid);
	memcpy(pua, &oid, sizeof(oid));
      }
      *pbuff += sizeof(eyedbsm::Oid);
    }
  }
}
