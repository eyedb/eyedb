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
#include "eyedblib/rpc_fe.h"
#include "rpc_lib.h"
#include "eyedblib/butils.h"

#ifndef _STDLIB_H
#include <stdlib.h>
//#error "stdlib not included"
#endif

namespace eyedb {

  RPCStatus _rpc_status__;

  static void
  makeSTATUS(rpc_RpcDescription *rd, int from)
  {
    /*  rd->args[from].type = RPCStatusType;*/
    rd->args[from].type = rpc_StatusType;
    rd->args[from].send_rcv = rpc_Rcv;;
    rd->arg_ret = rpc_VoidType;
  }

  rpc_RpcDescription *
  makeDBMCREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBMCREATE, 4);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* dbcreate description */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBMUPDATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBMUPDATE, 4);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* username */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBCREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBCREATE, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* dbcreate description */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBDELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBDELETE, 5);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBINFO(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBINFO, 7);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_DataType;   /* dbcreate description */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBMOVE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBMOVE, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* dbcreate description */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBCOPY(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBCOPY, 8);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* newdbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* newdbid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* dbcreate description */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBRENAME(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBRENAME, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* newdbname */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUSER_ADD(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_USER_ADD, 7);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* is_unix */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUSER_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_USER_DELETE, 5);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUSER_PASSWD_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_USER_PASSWD_SET, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* newpasswd */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makePASSWD_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_PASSWD_SET, 5);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* newpasswd */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDEFAULT_DBACCESS_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DEFAULT_DBACCESS_SET, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbaccess mode */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUSER_DBACCESS_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_USER_DBACCESS_SET, 7);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbaccess mode */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUSER_SYSACCESS_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_USER_SYSACCESS_SET, 6);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbaccess mode */
    rd->args[n++].send_rcv = rpc_Send;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeBACKEND_INTERRUPT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_BACKEND_INTERRUPT, 2);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);

    return rd;
  }

  rpc_RpcDescription *
  makeTRANSACTION_BEGIN(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_TRANSACTION_BEGIN, 9);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* trsmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* lockmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* recovmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* magorder */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* ratioalrt */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* wait_timeout */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* &tid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeTRANSACTION_COMMIT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_TRANSACTION_COMMIT, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* tid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeTRANSACTION_ABORT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_TRANSACTION_ABORT, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* tid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeTRANSACTION_PARAMS_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_TRANSACTION_PARAMS_SET, 8);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* trsmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* lockmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* recovmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* magorder */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* ratioalrt */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* wait_timeout */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeTRANSACTION_PARAMS_GET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_TRANSACTION_PARAMS_SET, 8);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* trsmode */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* lockmode */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* recovmode */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* magorder */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* ratioalrt */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* wait_timeout */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBOPEN(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBOPEN, 15);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* _DBRead or _DBRW */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* oh.maph */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* oh.mapwide */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_Int32Type;  /* ruid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_StringType; /* rname */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* rdbid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* version */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;    /* schema oid */
    rd->args[n++].send_rcv = rpc_Rcv;;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBOPENLOCAL(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBOPENLOCAL, 15);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* dbmdb */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* userauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwdauth */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dbname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dbid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* _DBRead or _DBRW */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* oh.maph */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* oh.mapwide */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_Int32Type;  /* ruid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_StringType; /* rname */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* rdbid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* version */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpcDB_LocalDBContextType;
    rd->args[n++].send_rcv = rpc_Rcv;;
    rd->args[n].type = OidType;    /* schema oid */
    rd->args[n++].send_rcv = rpc_Rcv;;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDBCLOSE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DBCLOSE, 2);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_CREATE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = (rpc_SendRcv)(rpc_Send|rpc_Rcv);
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_READ(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_READ, 7);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* lockmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_WRITE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_WRITE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_DELETE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_HEADER_READ(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_HEADER_READ, 9);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    /* object_header receive */
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_SIZE_MODIFY(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_SIZE_MODIFY, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }


  rpc_RpcDescription *
  makeOBJECT_CHECK(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_CHECK, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_PROTECTION_SET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_PROTECTION_SET, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* obj_oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* prot_oid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOBJECT_PROTECTION_GET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OBJECT_PROTECTION_GET, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* obj_oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* prot_oid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOID_MAKE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OID_MAKE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_CREATE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_READ(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_READ, 7);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;
  
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_WRITE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_WRITE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_DELETE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_SIZE_GET(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_SIZE_GET, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATA_SIZE_MODIFY(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATA_SIZE_MODIFY, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* size */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeVDDATA_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_VDDATA_CREATE, 11);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* dspid */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = OidType;    /* actual oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* attr num */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_Int32Type;  /* attr count */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* vd data */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* idx data */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* object oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* vd data oid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeVDDATA_WRITE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_VDDATA_WRITE, 10);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* actual oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* attr num */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* attr count */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_DataType;   /* vd data */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* idx data */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* object oid */
    rd->args[n++].send_rcv = rpc_Send;  
    rd->args[n].type = OidType;    /* vd data oid */
    rd->args[n++].send_rcv = rpc_Send; 

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeVDDATA_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_VDDATA_DELETE, 8);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* actual oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* oid_cl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* attr num */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* object oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* vd data oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* idx data */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSCHEMA_COMPLETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SCHEMA_COMPLETE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* schname */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeATTRIBUTE_INDEX_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ATTRIBUTE_INDEX_CREATE, 7);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* class oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* attr num */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* mode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* multi idx oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* idx context */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeATTRIBUTE_INDEX_REMOVE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ATTRIBUTE_INDEX_REMOVE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* class oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* attr num */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* mode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* idx context */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_CREATE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* index_moving */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* objoid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_REMOVE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_REMOVE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* objoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* reentrant */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCONSTRAINT_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_CONSTRAINT_CREATE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* objoid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCONSTRAINT_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_CONSTRAINT_DELETE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* objoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* reentrant */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCOLLECTION_GET_BY_IND(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_COLLECTION_GET_BY_IND, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* colloid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* ind */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* found */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_DataType;   /* rdata */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCOLLECTION_GET_BY_VALUE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_COLLECTION_GET_BY_VALUE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_LANG_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_LANG_CREATE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* query string */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* qid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type;  /* count */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_DataType;   /* schema info */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_DATABASE_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_DATABASE_CREATE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_CLASS_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_CLASS_CREATE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_COLLECTION_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_COLLECTION_CREATE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send; /* index */
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_OBJECT_LOCK(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_OBJECT_LOCK, 4);
    int n = 0;

    rd->args[n].type = OidType; /* oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* lockmode */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* rlockmode */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_OBJECT_LOCK(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_OBJECT_LOCK, 3);
    int n = 0;

    rd->args[n].type = OidType; /* oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* lockmode */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_ATTRIBUTE_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_ATTRIBUTE_CREATE, 10);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* cloid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* num agreg item */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* ind */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_DataType;  /* start */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* end */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* sexcl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* eexcl */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_Int32Type; /* &qid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_DELETE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeQUERY_SCAN_NEXT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_ITERATOR_SCAN_NEXT, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* qid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* max */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* count */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_DataType;
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOQL_CREATE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OQL_CREATE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* oql */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* qid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_DataType; /* schinfo */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOQL_DELETE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OQL_DELETE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* qid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeOQL_GETRESULT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_OQL_GETRESULT, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* qid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* value */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeEXECUTABLE_EXECUTE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_EXECUTABLE_EXECUTE, 14);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* intname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* name */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type;  /* exec_type : function/method/trigger */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* class oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* extref */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* signature */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* executable oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* object oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* argarray */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_DataType;   /* argarray + argret */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeEXECUTABLE_CHECK(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_EXECUTABLE_CHECK, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type;  /* rdbhid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* intname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;    /* oid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* extref */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeEXECUTABLE_SET_EXTREF_PATH(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_EXECUTABLE_SET_EXTREF_PATH, 4);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* path */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeEXECUTABLE_GET_EXTREF_PATH(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_EXECUTABLE_GET_EXTREF_PATH, 4);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* user */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_StringType; /* passwd */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;   /* path */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_CONN_INFO(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_CONN_INFO, 10);
    int n = 0;

    rd->args[n].type = rpc_StringType; /* hostname */
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_Int32Type; /* uid */ // no more useful
    rd->args[n++].send_rcv = rpc_Send; 
    rd->args[n].type = rpc_StringType; /* username */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* progname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* pid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* server pid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* server uid */
    rd->args[n++].send_rcv = rpc_Rcv;
    rd->args[n].type = rpc_Int32Type; /* server version */
    rd->args[n++].send_rcv = rpc_Send;

    rd->args[n].type = rpc_StringType; /* challenge */
    rd->args[n++].send_rcv = rpc_Rcv;
    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCHECK_AUTH(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_CHECK_AUTH, 2);
    int n = 0;

    rd->args[n].type = rpc_StringType; /*  */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_LOG_MASK(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_LOG_MASK, 2);
    int n = 0;

    rd->args[n].type = rpc_Int64Type; /* logmask */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_DEFAULT_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_DEFAULT_DATASPACE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_DEFAULT_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_DEFAULT_DATASPACE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }


  rpc_RpcDescription *
  makeDATASPACE_SET_CURRENT_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATASPACE_SET_CURRENT_DATAFILE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDATASPACE_GET_CURRENT_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DATASPACE_GET_CURRENT_DATAFILE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_DEFAULT_INDEX_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_DEFAULT_INDEX_DATASPACE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* type */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_DEFAULT_INDEX_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_DEFAULT_INDEX_DATASPACE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* tyoe */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_INDEX_LOCATIONS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_INDEX_LOCATIONS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* type */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* objlocs */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeMOVE_INDEX(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_MOVE_INDEX, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* type */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_INSTANCE_CLASS_LOCATIONS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_INSTANCE_CLASS_LOCATIONS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* clsoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* subclasses */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* objlocs */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeMOVE_INSTANCE_CLASS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_MOVE_INSTANCE_CLASS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;   /* clsoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* subclasses */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_OBJECTS_LOCATIONS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_OBJECTS_LOCATIONS, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* oids */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* objlocs */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeMOVE_OBJECTS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_MOVE_OBJECTS, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType;  /* oids */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_ATTRIBUTE_LOCATIONS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_ATTRIBUTE_LOCATIONS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType;  /* clsoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* attrnum */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* objlocs */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeMOVE_ATTRIBUTE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_MOVE_ATTRIBUTE, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* clsoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* attrnum */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCREATE_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_CREATE_DATAFILE, 7);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* datfile */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* name */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* maxsize */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* slotsize */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dtype */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDELETE_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DELETE_DATAFILE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeMOVE_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_MOVE_DATAFILE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* datafile */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDEFRAGMENT_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DEFRAGMENT_DATAFILE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeRESIZE_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_RESIZE_DATAFILE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* size */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_DATAFILEI_NFO(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_DATAFILEI_NFO, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* info */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeRENAME_DATAFILE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_RENAME_DATAFILE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* datid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* name */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCREATE_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_CREATE_DATASPACE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* dspname */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* datids */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeUPDATE_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_UPDATE_DATASPACE, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* datids */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int16Type; /* flags */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int16Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeDELETE_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_DELETE_DATASPACE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeRENAME_DATASPACE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_RENAME_DATASPACE, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* dspid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_StringType; /* name */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_GET_COUNT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_GET_COUNT, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* cnt */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_GET_STATS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_GET_STATS, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* stats */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_SIMUL_STATS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_SIMUL_STATS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* impl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* stats */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCOLLECTION_GET_IMPLSTATS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_COLLECTION_GET_IMPLSTATS, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* idxtype */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* stats */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCOLLECTION_SIMUL_IMPLSTATS(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_COLLECTION_SIMUL_IMPLSTATS, 6);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* idxtype */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* impl */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* stats */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeINDEX_GET_IMPL(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_INDEX_GET_IMPL, 4);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* impl */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeCOLLECTION_GET_IMPL(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_COLLECTION_GET_IMPL, 5);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* idxtype */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = OidType; /* idxoid */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_DataType; /* impl */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_SERVER_OUTOFBAND_DATA(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_SERVER_OUTOFBAND_DATA, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* type */
    rd->args[n++].send_rcv = (rpc_SendRcv)(rpc_Send|rpc_Rcv);
    rd->args[n].type = rpc_DataType; /* msg */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_MAXOBJCOUNT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_MAXOBJCOUNT, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* obj_cnt */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_MAXOBJCOUNT(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_MAXOBJCOUNT, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* obj_cnt */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeSET_LOGSIZE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_SET_LOGSIZE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* size */
    rd->args[n++].send_rcv = rpc_Send;

    makeSTATUS(rd, n);
    return rd;
  }

  rpc_RpcDescription *
  makeGET_LOGSIZE(void)
  {
    rpc_RpcDescription *rd = rpc_newRpcDescription(IDB_GET_LOGSIZE, 3);
    int n = 0;

    rd->args[n].type = rpc_Int32Type; /* dbh */
    rd->args[n++].send_rcv = rpc_Send;
    rd->args[n].type = rpc_Int32Type; /* size */
    rd->args[n++].send_rcv = rpc_Rcv;

    makeSTATUS(rd, n);
    return rd;
  }

  static RPCStatusRec status;

  RPCStatus
  rpcStatusMake_se(eyedbsm::Status status)
  {
    if (!status)
      return RPCSuccess;

    return rpcStatusMake((Error)status->err, status->err_msg);
  }

  RPCStatus
  rpcStatusMake(Error err, const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    static char *err_msg;
    static int err_msg_len;
    char *buf;

    buf = eyedblib::getFBuffer(fmt, ap);

    vsprintf(buf, fmt, aq);

    status.err = err;
    strncpy(status.err_msg, buf, sizeof(status.err_msg)-1);
    status.err_msg[sizeof(status.err_msg)-1] = 0;
    va_end(ap);

    return &status;
  }

  RPCStatus
  rpcStatusMake_s(Error err)
  {
    status.err = err;
    strcpy(status.err_msg, "");

    return &status;
  }
}
