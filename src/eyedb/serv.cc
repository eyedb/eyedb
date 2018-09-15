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

#include <assert.h>
#include <stdlib.h>

#define _IDB_KERN_
#include "eyedblib/semlib.h"
// FD CHECK
//#include "eyedbsm/eyedbsm_p.h"
#include "serv_lib.h"
#include "eyedb/internals/ObjectHeader.h"
#include "eyedb/base.h"
#include "eyedb/Error.h"
#include "eyedb/Exception.h"
#include "eyedb/TransactionParams.h"
#include "kernel.h"
#include "kernel_p.h"
#include "Argument_code.h"
#include "code.h"

namespace eyedb {

  static RPCStatusRec *RPCInvalidDbId, *RPCInvalidClientId;

#define SE_STATUS_MAKE(S, UA, I) \
  if (S)\
    {\
      UA[I].a_status.err = (eyedblib::int32)(S)->err;\
      strcpy((char *)UA[I].a_status.err_msg, eyedbsm::statusGet(S));\
    } \
  else\
    {\
      UA[I].a_status.err = (eyedblib::int32)IDB_SUCCESS;\
      UA[I].a_status.err_msg[0] = 0;\
    }

#define RPC_STATUS_MAKE(S, UA, I) \
  if (S)\
     UA[I].a_status = *(S); \
  else\
    {\
      UA[I].a_status.err = (eyedblib::int32)IDB_SUCCESS;\
      UA[I].a_status.err_msg[0] = 0;\
    }

  void
  be_init()
  {
    RPCInvalidDbId = rpc_new(RPCStatusRec);
    RPCInvalidDbId->err = IDB_INVALID_DB_ID;

    RPCInvalidClientId = rpc_new(RPCStatusRec);
    RPCInvalidClientId->err = IDB_INVALID_CLIENT_ID;
  }

  static int
  database_register(rpc_ClientId clientid, int id, rpc_Boolean mode,
			int flag, DbHandle *dbh)
  {
    rpcDB_DbHandleInfo *dbhinfo;

    dbhinfo = rpcDB_open_simple_realize(rpc_getMainServer(), id);
    return rpcDB_clientDbhSet(clientid, mode, flag, dbhinfo, dbh);
  }

  static int forbidden;
#define CHECK_ACCESS() if (forbidden) exit(1)

  void
  DBOPEN_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    DbHandle *dbh;
    int id = rpcDB_getDbhId();
    rpcDB_DbHandleInfo *dbhinfo;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else if ((status = IDB_dbOpen_make((ConnHandle *)ci->user_data,
				       ua[0].a_string, ua[1].a_string,
				       ua[2].a_string, ua[3].a_string,
				       ua[4].a_int, ua[5].a_int,
				       ua[6].a_int, ua[7].a_int,
				       &id, &ua[8].a_int, &ua[9].a_string,
				       &ua[10].a_int,
				       (unsigned int *)&ua[12].a_int, &dbh)) ==
	     RPCSuccess)
      {
	ua[11].a_int = database_register(clientid, id, rpc_False,
					     ua[5].a_int, dbh);
	ua[13].a_oid = dbh->sch.oid;
      }

    RPC_STATUS_MAKE(status, ua, 14);
  }

  void
  DBOPENLOCAL_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    DbHandle *ldbh; /* should be DbLocalHandle ! */
    int id = rpcDB_getDbhId();
    rpcDB_DbHandleInfo *dbhinfo;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);
  
    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else if ((status = IDB_dbOpen_make((ConnHandle *)ci->user_data,
				       ua[0].a_string, ua[1].a_string,
				       ua[2].a_string, ua[3].a_string,
				       ua[4].a_int, 
				       ua[5].a_int, 
				       ua[6].a_int, ua[7].a_int,
				       &id, &ua[8].a_int,
				       &ua[9].a_string,
				       &ua[10].a_int,
				       (unsigned int *)&ua[11].a_int, &ldbh)) ==
	     RPCSuccess)
      {
	//      ua[12].a_ldbctx = ldbh->sedbh->ldbctx;
	ua[12].a_ldbctx.rdbhid = 0;
	ua[12].a_ldbctx.xid = eyedbsm::getXID(ldbh->sedbh);
	ua[12].a_ldbctx.local = rpc_True;
	ua[12].a_ldbctx.rdbhid = database_register(clientid, id, rpc_True,
						       ua[5].a_int, ldbh);
	ua[13].a_oid = ldbh->sch.oid;
      }

    RPC_STATUS_MAKE(status, ua, 14);
  }

  void *
  close_realize(rpcDB_DbHandleClientInfo *dbhclientinfo)
  {
    return (void *)IDB_dbClose_make((DbHandle *)dbhclientinfo->dbh);
  }

  void
  DBCLOSE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    CHECK_ACCESS();

    rpcDB_close_realize(rpc_getMainServer(), clientid, ua[0].a_int,
			close_realize, (void **)&status);
    RPC_STATUS_MAKE(status, ua, 1);
  }

  extern void
  decode_dbdescription(Data, void *, DbCreateDescription *);

  void
  DBCREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      {
	DbCreateDescription *dbdesc = (DbCreateDescription *)malloc(sizeof(DbCreateDescription));
	decode_dbdescription((unsigned char *)ua[4].a_data.data, &ua[4].a_data, dbdesc);
      
	status = IDB_dbCreate((ConnHandle *)ci->user_data,
			      ua[0].a_string, ua[1].a_string,
			      ua[2].a_string, ua[3].a_string,
			      dbdesc);
	free(dbdesc);
      }

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  DBDELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus rpc_status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      rpc_status = RPCInvalidClientId;
    else
      rpc_status = IDB_dbDelete((ConnHandle *)ci->user_data,
				ua[0].a_string, ua[1].a_string,
				ua[2].a_string, ua[3].a_string);

    RPC_STATUS_MAKE(rpc_status, ua, 4);
  }

  extern Data code_dbdescription(const DbCreateDescription *,
				     int *);

  void
  DBINFO_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      {
	DbCreateDescription *dbdesc = (DbCreateDescription *)malloc(sizeof(DbCreateDescription));
      
	status = IDB_dbInfo((ConnHandle *)ci->user_data,
			    ua[0].a_string, ua[1].a_string,
			    ua[2].a_string, ua[3].a_string,
			    &ua[4].a_int,
			    dbdesc);

	if (!status)
	  ua[5].a_data.data = code_dbdescription(dbdesc, &ua[5].a_data.size);
	else
	  ua[5].a_data.data = (unsigned char *)malloc(1);

	ua[5].a_data.status = rpc_TempDataUsed;

	free(dbdesc);
      }

    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  DBMOVE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      {
	DbCreateDescription *dbdesc = (DbCreateDescription *)malloc(sizeof(DbCreateDescription));
	decode_dbdescription((unsigned char *)ua[4].a_data.data, &ua[4].a_data, dbdesc);
      
	status = IDB_dbMove((ConnHandle *)ci->user_data,
			    ua[0].a_string, ua[1].a_string,
			    ua[2].a_string, ua[3].a_string,
			    dbdesc);
	free(dbdesc);
      }

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  DBCOPY_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      {
	DbCreateDescription *dbdesc = (DbCreateDescription *)malloc(sizeof(DbCreateDescription));
	decode_dbdescription((unsigned char *)ua[6].a_data.data, &ua[6].a_data, dbdesc);
      
	status = IDB_dbCopy((ConnHandle *)ci->user_data,
			    ua[0].a_string, ua[1].a_string,
			    ua[2].a_string, ua[3].a_string,
			    ua[4].a_string, (Bool)ua[5].a_int, dbdesc);
	free(dbdesc);
      }

    RPC_STATUS_MAKE(status, ua, 7);
  }

  void
  DBRENAME_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_dbRename((ConnHandle *)ci->user_data,
			    ua[0].a_string, ua[1].a_string,
			    ua[2].a_string, ua[3].a_string,
			    ua[4].a_string);

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  DBMCREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      {
	DbCreateDescription *dbdesc = (DbCreateDescription *)malloc(sizeof(DbCreateDescription));
	decode_dbdescription((unsigned char *)ua[2].a_data.data, &ua[2].a_data, dbdesc);
      
	status = IDB_dbmCreate((ConnHandle *)ci->user_data,
			       ua[0].a_string, ua[1].a_string,
			       dbdesc);
	free(dbdesc);
      }

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  DBMUPDATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    CHECK_ACCESS();

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_dbmUpdate((ConnHandle *)ci->user_data,
			     ua[0].a_string, ua[1].a_string,
			     ua[2].a_string);

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  USER_ADD_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_userAdd((ConnHandle *)ci->user_data,
			   ua[0].a_string, ua[1].a_string,
			   ua[2].a_string, ua[3].a_string,
			   ua[4].a_string, ua[5].a_int);

    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  USER_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_userDelete((ConnHandle *)ci->user_data,
			      ua[0].a_string, ua[1].a_string,
			      ua[2].a_string, ua[3].a_string);

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  USER_PASSWD_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_userPasswdSet((ConnHandle *)ci->user_data,
				 ua[0].a_string, ua[1].a_string,
				 ua[2].a_string, ua[3].a_string,
				 ua[4].a_string);

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  PASSWD_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_passwdSet((ConnHandle *)ci->user_data,
			     ua[0].a_string, ua[1].a_string,
			     ua[2].a_string, ua[3].a_string);

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  DEFAULT_DBACCESS_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_defaultDBAccessSet((ConnHandle *)ci->user_data,
				      ua[0].a_string, ua[1].a_string,
				      ua[2].a_string, ua[3].a_string,
				      ua[4].a_int);

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  USER_DBACCESS_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_userDBAccessSet((ConnHandle *)ci->user_data,
				   ua[0].a_string, ua[1].a_string,
				   ua[2].a_string, ua[3].a_string,
				   ua[4].a_string, ua[5].a_int);

    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  USER_SYSACCESS_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_userSysAccessSet((ConnHandle *)ci->user_data,
				    ua[0].a_string, ua[1].a_string,
				    ua[2].a_string, ua[3].a_string,
				    ua[4].a_int);

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  BACKEND_INTERRUPT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidClientId;
    else
      status = IDB_backendInterrupt((ConnHandle *)ci->user_data,
				    ua[0].a_int);
				     
    RPC_STATUS_MAKE(status, ua, 1);
  }

  void
  GET_SERVER_OUTOFBAND_DATA_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_ClientInfo *ci = rpcDB_clientInfoGet(clientid);

    if (!ci)
      status = RPCInvalidDbId;
    else
      status = IDB_getServerOutOfBandData((ConnHandle *)ci->user_data, 
					  &ua[0].a_int, 0, 0, &ua[1].a_data);

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  TRANSACTION_BEGIN_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      {
	TransactionParams params;
	params.trsmode = (TransactionMode)ua[1].a_int;
	params.lockmode = (TransactionLockMode)ua[2].a_int;
	params.recovmode = (RecoveryMode)ua[3].a_int;
	params.magorder = ua[4].a_int;
	params.ratioalrt = ua[5].a_int;
	params.wait_timeout = ua[6].a_int;
	status = IDB_transactionBegin((DbHandle *)dcinfo->dbh, &params, False);
      }
    else
      status = RPCInvalidDbId;

    ua[7].a_int = 0;

    RPC_STATUS_MAKE(status, ua, 8);
  }

  void
  TRANSACTION_COMMIT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_transactionCommit((DbHandle *)dcinfo->dbh, False);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  TRANSACTION_ABORT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_transactionAbort((DbHandle *)dcinfo->dbh, False);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  TRANSACTION_PARAMS_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      {
	TransactionParams params;
	params.trsmode = (TransactionMode)ua[1].a_int;
	params.lockmode = (TransactionLockMode)ua[2].a_int;
	params.recovmode = (RecoveryMode)ua[3].a_int;
	params.magorder = ua[4].a_int;
	params.ratioalrt = ua[5].a_int;
	params.wait_timeout = ua[6].a_int;
	status = IDB_transactionParamsSet((DbHandle *)dcinfo->dbh, &params);
      }
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 7);
  }

  void
  TRANSACTION_PARAMS_GET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      {
	TransactionParams params;
	status = IDB_transactionParamsGet((DbHandle *)dcinfo->dbh, &params);

	ua[1].a_int = params.trsmode;
	ua[2].a_int = params.lockmode;
	ua[3].a_int = params.recovmode;
	ua[4].a_int = params.magorder;
	ua[5].a_int = params.ratioalrt;
	ua[6].a_int = params.wait_timeout;
      }
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 7);
  }

  void
  OBJECT_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectCreate((DbHandle *)dcinfo->dbh, ua[1].a_int,
				(unsigned char *)ua[2].a_data.data, &ua[3].a_oid,
				&ua[2].a_data, 0, &ua[4].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  OBJECT_READ_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    short dspid;
    if (dcinfo)
      status = IDB_objectRead((DbHandle *)dcinfo->dbh, (unsigned char *)ua[1].a_data.data, 0, &dspid,
			      &ua[3].a_oid,
			      (LockMode)ua[4].a_int, &ua[5].a_data);
    else
      status = RPCInvalidDbId;

    ua[2].a_int = dspid;
    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  OBJECT_WRITE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectWrite((DbHandle *)dcinfo->dbh, (unsigned char *)ua[1].a_data.data, &ua[2].a_oid,
			       &ua[1].a_data, 0, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  OBJECT_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectDelete((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
				0, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  OBJECT_HEADER_READ_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      {
	ObjectHeader hdr;
	status = IDB_objectHeaderRead((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &hdr);
	if (status == RPCSuccess)
	  {
	    ua[2].a_int = hdr.type;
	    ua[3].a_int = hdr.size;
	    ua[4].a_int = hdr.ctime;
	    ua[5].a_int = hdr.mtime;
	    ua[6].a_oid = hdr.oid_cl;
	    ua[7].a_oid = hdr.oid_prot;
	  }
      }
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 8);
  }

  void
  OBJECT_SIZE_MODIFY_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectSizeModify((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  OBJECT_CHECK_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectCheck((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_int, &ua[3].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  OBJECT_PROTECTION_SET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectProtectionSet((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  OBJECT_PROTECTION_GET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_objectProtectionGet((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  OID_MAKE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo) {
#ifdef E_XDR
      ObjectHeader hdr;
      Offset offset = 0;
      object_header_decode((unsigned char *)ua[2].a_data.data, &offset, &hdr);
      status = IDB_oidMake((DbHandle *)dcinfo->dbh,
			   &hdr,
			   ua[1].a_int,
			   ua[3].a_int, &ua[4].a_oid);
#else
      status = IDB_oidMake((DbHandle *)dcinfo->dbh,
			   (ObjectHeader *)ua[2].a_data.data,
			   ua[1].a_int,
			   ua[3].a_int, &ua[4].a_oid);
#endif
    }
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  DATA_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataCreate((DbHandle *)dcinfo->dbh, ua[1].a_int,
			      ua[2].a_data.size, (unsigned char *)ua[2].a_data.data,
			      &ua[3].a_oid, &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  DATA_READ_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    short dspid;
    if (dcinfo)
      status = IDB_dataRead((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_int,
			    (unsigned char *)ua[5].a_data.data, &dspid, &ua[4].a_oid,
			    &ua[5].a_data);
    else
      status = RPCInvalidDbId;

    ua[3].a_int = dspid;
    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  DATA_WRITE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataWrite((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_data.size,
			     (unsigned char *)ua[2].a_data.data,  &ua[3].a_oid, &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  DATA_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataDelete((DbHandle *)dcinfo->dbh, &ua[1].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  DATA_SIZE_GET_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataSizeGet((DbHandle *)dcinfo->dbh, &ua[1].a_oid, (unsigned int *)&ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  DATA_SIZE_MODIFY_realize(rpc_ClientId clientid, void *xua)
  {
    printf("coucou les lapins\n");
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataSizeModify((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  VDDATA_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_VDdataCreate((DbHandle *)dcinfo->dbh, ua[1].a_int,
				&ua[2].a_oid, &ua[3].a_oid,
				ua[4].a_int, ua[5].a_int,
				ua[6].a_data.size, (unsigned char *)ua[6].a_data.data,
				&ua[8].a_oid, &ua[9].a_oid,
				(unsigned char *)ua[7].a_data.data, ua[7].a_data.size,
				&ua[6].a_data, &ua[7].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 10);
  }

  void
  VDDATA_WRITE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_VDdataWrite((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_oid,
			       ua[3].a_int, ua[4].a_int,
			       ua[5].a_data.size, (unsigned char *)ua[5].a_data.data,
			       &ua[7].a_oid, &ua[8].a_oid,
			       (unsigned char *)ua[6].a_data.data, ua[6].a_data.size,
			       &ua[5].a_data, &ua[6].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 9);
  }

  void
  VDDATA_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_VDdataDelete((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_oid,
				ua[3].a_int, &ua[4].a_oid, &ua[5].a_oid,
				(unsigned char *)ua[6].a_data.data, ua[6].a_data.size,
				&ua[6].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 7);
  }

  void
  ATTRIBUTE_INDEX_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_attributeIndexCreate((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
					ua[2].a_int, ua[3].a_int, &ua[4].a_oid,
					(unsigned char *)ua[5].a_data.data,
					ua[5].a_data.size, &ua[5].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  CONSTRAINT_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_constraintCreate((DbHandle *)dcinfo->dbh, &ua[1].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  CONSTRAINT_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_constraintDelete((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  SCHEMA_COMPLETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_schemaComplete((DbHandle *)dcinfo->dbh, ua[1].a_string);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  ATTRIBUTE_INDEX_REMOVE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_attributeIndexRemove((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
					ua[2].a_int, ua[3].a_int,
					(unsigned char *)ua[4].a_data.data,
					ua[4].a_data.size, &ua[4].a_data);
    else
      status = RPCInvalidDbId;
  
    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  INDEX_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexCreate((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_oid);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  INDEX_REMOVE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexRemove((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  COLLECTION_GET_BY_IND_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_collectionGetByInd((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
				      &ua[3].a_int, 0, &ua[4].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  COLLECTION_GET_BY_VALUE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_collectionGetByValue((DbHandle *)dcinfo->dbh, &ua[1].a_oid, (unsigned char *)ua[2].a_data.data,
					&ua[3].a_int, &ua[4].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  SET_OBJECT_LOCK_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_setObjectLock((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int, &ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  GET_OBJECT_LOCK_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getObjectLock((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  /*
  void
  QUERY_LANG_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryLangCreate((DbHandle *)dcinfo->dbh, (const char *)ua[1].a_data.data,
				   &ua[2].a_int, &ua[3].a_int, 0,
				   &ua[1].a_data, &ua[4].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  QUERY_DATABASE_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryDatabaseCreate((DbHandle *)dcinfo->dbh, &ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  QUERY_CLASS_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryClassCreate((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }
  */

  void
  QUERY_COLLECTION_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryCollectionCreate((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
					 &ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  QUERY_ATTRIBUTE_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryAttributeCreate((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
					ua[2].a_int,
					ua[3].a_int,
					(unsigned char *)ua[4].a_data.data,
					(unsigned char *)ua[5].a_data.data,
					ua[6].a_int,
					ua[7].a_int,
					ua[4].a_data.size,
					&ua[8].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 9);
  }

  void
  QUERY_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryDelete((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  QUERY_SCAN_NEXT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_queryScanNext((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_int,
				 &ua[3].a_int, 0, &ua[4].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  /* OQL */

  void
  OQL_CREATE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status = 0;
    rpcDB_DbHandleClientInfo *dcinfo;
    DbHandle *dbh;

    if (ua[0].a_int)
      {
	dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);
	if (dcinfo)
	  dbh = (DbHandle *)dcinfo->dbh;
	else
	  status = RPCInvalidDbId;
      }
    else
      dbh = 0;

    if (!status)
      status = IDB_oqlCreate(dbh, (const char *)ua[1].a_data.data, &ua[2].a_int,
			     0, &ua[1].a_data, &ua[3].a_data);
  
    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  OQL_DELETE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status = 0;
    rpcDB_DbHandleClientInfo *dcinfo;
    DbHandle *dbh;

    if (ua[0].a_int)
      {
	dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);
	if (dcinfo)
	  dbh = (DbHandle *)dcinfo->dbh;
	else
	  status = RPCInvalidDbId;
      }
    else
      dbh = 0;

    if (!status)
      status = IDB_oqlDelete(dbh, ua[1].a_int);

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  OQL_GETRESULT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status = 0;
    rpcDB_DbHandleClientInfo *dcinfo;
    DbHandle *dbh;

    if (ua[0].a_int)
      {
	dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);
	if (dcinfo)
	  dbh = (DbHandle *)dcinfo->dbh;
	else
	  status = RPCInvalidDbId;
      }
    else
      dbh = 0;

    if (!status)
      status = IDB_oqlGetResult(dbh, ua[1].a_int, 0, &ua[2].a_data);

    RPC_STATUS_MAKE(status, ua, 3);
  }

  /* executables */
  void
  EXECUTABLE_CHECK_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_execCheck((DbHandle *)dcinfo->dbh, ua[1].a_string, &ua[2].a_oid,
			     ua[3].a_string);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  EXECUTABLE_EXECUTE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_execExecute((DbHandle *)dcinfo->dbh, 
			       ua[1].a_string,     /* user */
			       ua[2].a_string,     /* passwd */
			       ua[3].a_string,     /* intname */
			       ua[4].a_string,     /* name */
			       ua[5].a_int,        /* exec_type */
			       &ua[6].a_oid,       /* cloid */
			       ua[7].a_string,     /* extref */
			       0, &ua[8].a_data,   /* signature */
			       &ua[9].a_oid,       /* execoid */
			       &ua[10].a_oid,      /* objoid */
			       0,                  /* o */
			       0, &ua[11].a_data,  /* argarray */
			       0, &ua[12].a_data); /* argret */
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 13);
  }

  void
  EXECUTABLE_SET_EXTREF_PATH_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    status = IDB_execSetExtRefPath(ua[0].a_string, ua[1].a_string,
				   ua[2].a_string);

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  EXECUTABLE_GET_EXTREF_PATH_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    status = IDB_execGetExtRefPath(ua[0].a_string, ua[1].a_string,
				   &ua[2].a_data, 0);

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  SET_CONN_INFO_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    status = IDB_setConnInfo(ua[0].a_string, ua[1].a_int, ua[2].a_string,
			     ua[3].a_string,
			     ua[4].a_int, &ua[5].a_int, &ua[6].a_int,
			     ua[7].a_int, &ua[8].a_string);
    if (status)
      forbidden = 1;

    RPC_STATUS_MAKE(status, ua, 9);
  }

  void
  CHECK_AUTH_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    status = IDB_checkAuth(ua[0].a_string);
    
    RPC_STATUS_MAKE(status, ua, 1);
  }

  void
  SET_LOG_MASK_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;

    status = IDB_setLogMask(ua[0].a_int64);

    RPC_STATUS_MAKE(status, ua, 1);
  }

  void
  INDEX_GET_COUNT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexGetCount((DbHandle *)dcinfo->dbh, &ua[1].a_oid, &ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  INDEX_GET_STATS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexGetStats((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
				 (unsigned char **)ua[2].a_data.data, &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  INDEX_SIMUL_STATS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexSimulStats((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
				   (unsigned char *)ua[2].a_data.data, &ua[2].a_data,
				   (unsigned char **)ua[3].a_data.data, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  COLLECTION_GET_IMPLSTATS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_collectionGetImplStats((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_oid,
					  (unsigned char **)ua[3].a_data.data, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  COLLECTION_SIMUL_IMPLSTATS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_collectionSimulImplStats((DbHandle *)dcinfo->dbh, ua[1].a_int,
					    &ua[2].a_oid,
					    (unsigned char *)ua[3].a_data.data, &ua[3].a_data,
					    (unsigned char **)ua[4].a_data.data, &ua[4].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  INDEX_GET_IMPL_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_indexGetImplementation((DbHandle *)dcinfo->dbh, &ua[1].a_oid, 0, &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  COLLECTION_GET_IMPL_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_collectionGetImplementation((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_oid, 0, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  GET_DEFAULT_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getDefaultDataspace((DbHandle *)dcinfo->dbh, &ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  SET_DEFAULT_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_setDefaultDataspace((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  DATASPACE_SET_CURRENT_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataspaceSetCurrentDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  DATASPACE_GET_CURRENT_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_dataspaceGetCurrentDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int, &ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  GET_DEFAULT_INDEX_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getDefaultIndexDataspace((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int, &ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  SET_DEFAULT_INDEX_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_setDefaultIndexDataspace((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int, ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  GET_INDEX_LOCATIONS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getIndexLocations((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
				     (unsigned char **)ua[3].a_data.data, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  MOVE_INDEX_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_moveIndex((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int, ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  GET_INSTANCE_CLASS_LOCATIONS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getInstanceClassLocations((DbHandle *)dcinfo->dbh, &ua[1].a_oid,
					     ua[2].a_int,
					     (unsigned char **)ua[3].a_data.data, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  MOVE_INSTANCE_CLASS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_moveInstanceClass((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
				     ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  GET_OBJECTS_LOCATIONS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getObjectsLocations((DbHandle *)dcinfo->dbh,
				       (eyedbsm::Oid *)ua[1].a_data.data,
				       0,
				       &ua[1].a_data,
				       (unsigned char **)ua[2].a_data.data,
				       &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  MOVE_OBJECTS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_moveObjects((DbHandle *)dcinfo->dbh,
			       (const eyedbsm::Oid *)ua[1].a_data.data,
			       0,
			       ua[2].a_int,
			       &ua[1].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  GET_ATTRIBUTE_LOCATIONS_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getAttributeLocations((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int,
					 (unsigned char **)ua[3].a_data.data, &ua[3].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  MOVE_ATTRIBUTE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_moveAttribute((DbHandle *)dcinfo->dbh, &ua[1].a_oid, ua[2].a_int, ua[3].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 4);
  }

  void
  CREATE_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_createDatafile((DbHandle *)dcinfo->dbh, ua[1].a_string, ua[2].a_string, ua[3].a_int, ua[4].a_int, ua[5].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 6);
  }

  void
  DELETE_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_deleteDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  MOVE_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_moveDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_string);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  DEFRAGMENT_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_defragmentDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  RESIZE_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_resizeDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  GET_DATAFILEI_NFO_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getDatafileInfo((DbHandle *)dcinfo->dbh, ua[1].a_int, (unsigned char **)ua[2].a_data.data,
				   &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  RENAME_DATAFILE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_renameDatafile((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_string);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  CREATE_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_createDataspace((DbHandle *)dcinfo->dbh, ua[1].a_string,
				   (void *)ua[2].a_data.data, 0, &ua[2].a_data);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  UPDATE_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_updateDataspace((DbHandle *)dcinfo->dbh, ua[1].a_int,
				   (void *)ua[2].a_data.data, 0, &ua[2].a_data,
				   ua[3].a_int, ua[4].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 5);
  }

  void
  DELETE_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_deleteDataspace((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  RENAME_DATASPACE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_renameDataspace((DbHandle *)dcinfo->dbh, ua[1].a_int, ua[2].a_string);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 3);
  }

  void
  SET_MAXOBJCOUNT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_setMaxObjCount((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  GET_MAXOBJCOUNT_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getMaxObjCount((DbHandle *)dcinfo->dbh, &ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  SET_LOGSIZE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_setLogSize((DbHandle *)dcinfo->dbh, ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  GET_LOGSIZE_realize(rpc_ClientId clientid, void *xua)
  {
    ServerArg *ua = (ServerArg *)xua;
    RPCStatus status;
    rpcDB_DbHandleClientInfo *dcinfo = rpcDB_clientDbhGet(clientid, ua[0].a_int);

    if (dcinfo)
      status = IDB_getLogSize((DbHandle *)dcinfo->dbh, &ua[1].a_int);
    else
      status = RPCInvalidDbId;

    RPC_STATUS_MAKE(status, ua, 2);
  }

  void
  GARBAGE()
  {
    RPCStatus status;

    rpcDB_close_realize(0, rpc_client_id, 0,
			close_realize, (void **)&status);
  }
}
