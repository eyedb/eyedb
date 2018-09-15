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


#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "base_p.h"
#include "eyedb/internals/ObjectHeader.h"
#include "eyedb/Exception.h"
#include "eyedb/TransactionParams.h"
#include "api_lib.h"

#include "kernel.h"
#include "misc.h"
#include "SchemaInfo.h"
#include "Argument_code.h"
#include "code.h"
#include "eyedb/internals/kern_const.h"

#define IDB_MAXARGS 16

namespace eyedbsm {
  extern unsigned int import_xid;
}

namespace eyedb {

  extern void setBackendInterrupt(Bool);

#define status_copy(D, S)				\
  {							\
    (D).err = (S).err;					\
    if ((S).err) strcpy((D).err_msg, (S).err_msg);	\
  }

#define start_rpc()

  static RPCStatusRec status_r;

#define SERVER_CRASH_MSG			\
  "EyeDB server has probably crashed. "		\
  "Contact your system administrator.\n"

#undef SERVER_CRASH_MSG

#define SERVER_CRASH_MSG				\
  "the EyeDB server has probably crashed or timed out."

  /*      fprintf(stderr, \
	  "eyedb fatal error: server failure [" #RPC ". code #%d].\n" \
	  SERVER_CRASH_MSG, r); \

	  return rpcStatusMake(IDB_SERVER_FAILURE, \
	  "[" #RPC ", code #%d].\n" SERVER_CRASH_MSG, r); \

  */

#define RPC_RPCMAKE(CH, RPC, UA)					\
  {									\
    int r;								\
    r = rpc_rpcMake(CH, 0, RPC, UA);					\
									\
    if (r)								\
      {									\
	if (errno) perror("server");					\
	return rpcStatusMake(IDB_SERVER_FAILURE, SERVER_CRASH_MSG);	\
      }									\
  }

#define STATUS_RETURN(S)				\
  return (((S).err == IDB_SUCCESS) ? RPCSuccess : &(S))

#define RDBHID_GET(DBH)							\
  ((DBH)->ldbctx.local ? (DBH)->ldbctx.rdbhid : (DBH)->u.rdbhid)

#define CHECK_DBH(DBH, OP)						\
  if (!(DBH)) return rpcStatusMake(IDB_ERROR, "operation " OP ": database must be opened")

#define DBH_IS_LOCAL(DBH) ((DBH)->ldbctx.local)

  const int CONN_COUNT = 3; // 20/03/06: can be more for multi-threading

  RPCStatus
  connOpen(const char *hostname, const char *portname,
	   ConnHandle **pch, int flags, std::string &errmsg)
  {
    *pch = rpc_new(ConnHandle);

    if (!rpc_connOpen(getRpcClient(), hostname, portname,
		      &((*pch)->ch), RPC_PROTOCOL_MAGIC, CONN_COUNT, 0,
		      errmsg))
      return RPCSuccess;
    else {
      free(*pch);
      *pch = 0;
      return rpcStatusMake(IDB_CONNECTION_FAILURE,
			   "portname '%s'", portname);
    }
  }

  RPCStatus
  connClose(ConnHandle *ch)
  {
    if (ch->ch && !rpc_connClose(ch->ch))
      {
	free(ch);
	return RPCSuccess;
      }
    else
      return rpcStatusMake(IDB_CONNECTION_FAILURE, "cannot close connection");
  }

  /* remote procedure calls */
  RPCStatus
  dbOpen(ConnHandle *ch, const char *dbmdb,
	 const char *userauth, const char *passwdauth,
	 const char *dbname, int dbid, int flags, int oh_maph,
	 unsigned int oh_mapwide, int *puid, void *db,
	 char **rname, int *rdbid, unsigned int *pversion,
	 DbHandle **dbh)
  {
    ClientArg ua[IDB_MAXARGS], *pua = ua;
      
    pua++->a_string = (char *)dbmdb;
    pua++->a_string = (char *)userauth;
    pua++->a_string = (char *)passwdauth;
    pua++->a_string = (char *)dbname;
    pua++->a_int    = dbid;
    /* LOCAL_REMOTE */
    pua++->a_int    = flags;
    pua++->a_int    = oh_maph;
    pua++->a_int    = oh_mapwide;
  
    if (flags & _DBOpenLocal)
      {
	RPC_RPCMAKE(ch->ch, DBOPENLOCAL_RPC, ua);

	status_copy(status_r, ua[14].a_status);

	if (status_r.err == IDB_SUCCESS)
	  {
	    DbHandle *tdbh;
	    RPCStatus status;
	    rpcDB_LocalDBContext *ldbctx = &ua[12].a_ldbctx;

	    /*	  se_import_xid = ldbctx->rdbhid;*/
	    eyedbsm::import_xid = ldbctx->xid;

	    /*printf("se_import_xid = %d (%s)\n", se_import_xid, dbname);*/

	    status = IDB_dbOpen(ch, dbmdb, userauth, passwdauth, dbname, dbid,
				flags & ~_DBOpenLocal, oh_maph,
				oh_mapwide, 0, puid, db, rname,
				rdbid, pversion, &tdbh);

	    if (status == RPCSuccess)
	      {
		*dbh = rpc_new(DbHandle);
		(*dbh)->ch = (ConnHandle *)ch;
#ifndef LOCKMTX
		if ((ldbctx->semid[0] = sem_openSX(ldbctx->semkey[0]))<0 || 
		    (ldbctx->semid[1] = sem_openSX(ldbctx->semkey[1]))<0)
		  return rpcStatusMake(IDB_ERROR, "");
#endif

		(*dbh)->ldbctx = *ldbctx;
		(*dbh)->u.dbh = tdbh;
		(*dbh)->flags = flags & ~_DBOpenLocal;
		(*dbh)->sch_oid = ua[13].a_oid;
	      }
	    return status;
	  }
	STATUS_RETURN(status_r);
      }
    else
      {
	RPC_RPCMAKE(ch->ch, DBOPEN_RPC, ua);

	status_copy(status_r, ua[14].a_status);

	if (status_r.err == IDB_SUCCESS)
	  {
	    *dbh = rpc_new(DbHandle);
	    (*dbh)->u.rdbhid = ua[11].a_int;
	    (*dbh)->ch = (ConnHandle *)ch;
	    (*dbh)->flags = flags & ~_DBOpenLocal;
	    (*dbh)->sch_oid = ua[13].a_oid;	
	    *puid = ua[8].a_int;
	    *rname = ua[9].a_string;
	    *rdbid = ua[10].a_int;
	    *pversion = ua[12].a_int;
	  }
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbClose(const DbHandle *dbh)
  {
    ClientArg ua[IDB_MAXARGS], *pua = ua;

    CHECK_DBH(dbh, "dbClose");

    if (DBH_IS_LOCAL(dbh))
      {
	pua++->a_int = dbh->ldbctx.rdbhid;
	IDB_dbCloseLocal((DbHandle *)dbh->u.dbh);
      }
    else
      pua++->a_int = dbh->u.rdbhid;

    RPC_RPCMAKE(dbh->ch->ch, DBCLOSE_RPC, ua);

    status_copy(status_r, ua[1].a_status);
  
    STATUS_RETURN(status_r);
  }

  RPCStatus
  backendInterrupt(ConnHandle *ch, int info)
  {
    ClientArg ua[IDB_MAXARGS], *pua = ua;
    int r;

    setBackendInterrupt(True);

    pua++->a_int = info;

    r = rpc_rpcMake(ch->ch, 1, BACKEND_INTERRUPT_RPC, ua);

    status_copy(status_r, ua[1].a_status);
  
    STATUS_RETURN(status_r);
  }

  RPCStatus
  getServerOutOfBandData(ConnHandle *ch, int * type, Data * data,
			 unsigned int *size)
  {
    if (!ch) 
      return IDB_getServerOutOfBandData(0, type, data, size, 0);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, offset;

      pua++->a_int = *type;
      pua->a_data.data = 0; /* force RPC to allocate buffer */
      pua++->a_data.size   = 0;
  
      r = rpc_rpcMake(ch->ch, 2, GET_SERVER_OUTOFBAND_DATA_RPC, ua);
    
      *type = ua[0].a_int;
    
      if (!r) {
	*data = (unsigned char *)ua[1].a_data.data;
	*size = ua[1].a_data.size;
      }
      else {
	*data = 0;
	*size = 0;
      }
    
      status_copy(status_r, ua[2].a_status);
      STATUS_RETURN(status_r);
    }
  }

  RPCStatus
  transactionBegin(DbHandle *dbh, 
		   const TransactionParams *params,
		   TransactionId *tid)
  {
    CHECK_DBH(dbh, "transactionBegin");

    if (DBH_IS_LOCAL(dbh))
      {
	*tid = 0;
	return IDB_transactionBegin((DbHandle *)dbh->u.dbh, params, True);
      }
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = params->trsmode;
	pua++->a_int = params->lockmode;
	pua++->a_int = params->recovmode;
	pua++->a_int = params->magorder;
	pua++->a_int = params->ratioalrt;
	pua++->a_int = params->wait_timeout; 

	RPC_RPCMAKE(dbh->ch->ch, TRANSACTION_BEGIN_RPC, ua);
      
	status_copy(status_r, ua[8].a_status);
      
	*tid = ua[7].a_int;

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  transactionCommit(DbHandle *dbh, TransactionId tid)
  {
    CHECK_DBH(dbh, "transactionCommit");

    if (DBH_IS_LOCAL(dbh))
      return IDB_transactionCommit((DbHandle *)dbh->u.dbh, True);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = 0;

	RPC_RPCMAKE(dbh->ch->ch, TRANSACTION_COMMIT_RPC, ua);

	status_copy(status_r, ua[2].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  transactionAbort(DbHandle *dbh, TransactionId tid)
  {
    CHECK_DBH(dbh, "transactionAbort");

    if (DBH_IS_LOCAL(dbh))
      return IDB_transactionAbort((DbHandle *)dbh->u.dbh, True);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = 0;
      
	RPC_RPCMAKE(dbh->ch->ch, TRANSACTION_ABORT_RPC, ua);
      
	status_copy(status_r, ua[2].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  transactionParamsSet(DbHandle *dbh, const TransactionParams *params)
  {
    CHECK_DBH(dbh, "transactionParamsSet");

    if (DBH_IS_LOCAL(dbh))
      return IDB_transactionParamsSet((DbHandle *)dbh->u.dbh, params);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = params->trsmode;
	pua++->a_int = params->lockmode;
	pua++->a_int = params->recovmode;
	pua++->a_int = params->magorder;
	pua++->a_int = params->ratioalrt;
	pua++->a_int = params->wait_timeout; 

	RPC_RPCMAKE(dbh->ch->ch, TRANSACTION_PARAMS_SET_RPC, ua);
      
	status_copy(status_r, ua[7].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  transactionParamsGet(DbHandle *dbh, TransactionParams *params)
  {
    CHECK_DBH(dbh, "transactionParamsGet");

    if (DBH_IS_LOCAL(dbh))
      return IDB_transactionParamsGet((DbHandle *)dbh->u.dbh, params);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_int = RDBHID_GET(dbh);

	RPC_RPCMAKE(dbh->ch->ch, TRANSACTION_PARAMS_GET_RPC, ua);

	params->trsmode = (TransactionMode)ua[1].a_int;
	params->lockmode = (TransactionLockMode)ua[2].a_int;
	params->recovmode = (RecoveryMode)ua[3].a_int;
	params->magorder = ua[4].a_int;
	params->ratioalrt = ua[5].a_int;
	params->wait_timeout = ua[6].a_int;
      
	status_copy(status_r, ua[7].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  extern Data code_dbdescription(const DbCreateDescription *, int *);
  extern Data code_oids(const eyedbsm::Oid *, unsigned int, int *);
  extern Data code_datafiles(void *datafiles, unsigned int datafile_cnt,
			     int *);
  extern void decode_locarr(Data, void *locarr);
  extern void decode_atom_array(rpc_ClientData *, void *, int);
  extern void decode_value(void *data, void *value);
  extern void decode_datinfo(Data, void *info);
  extern void decode_index_stats(Data, void *stats);
  extern void decode_index_impl(Data, void *impl);

  RPCStatus
  dbmCreate(ConnHandle *ch, const char *dbmdb, const char *passwdauth,
	    const DbCreateDescription *dbdesc)
  {
    if (!ch || !ch->ch)
      return IDB_dbmCreate(ch, dbmdb, passwdauth, dbdesc);
    else
      {
	Data dbdesc_data;
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)passwdauth;
	dbdesc_data = code_dbdescription(dbdesc, &pua->a_data.size);
	pua->a_data.data = dbdesc_data;

	RPC_RPCMAKE(ch->ch, DBMCREATE_RPC, ua);

	free(dbdesc_data);
	status_copy(status_r, ua[3].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbmUpdate(ConnHandle *ch, const char *dbmdb,
	    const char *username, const char *passwd)
  {
    if (!ch || !ch->ch)
      return IDB_dbmUpdate(ch, dbmdb, username, passwd);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)username;
	pua++->a_string = (char *)passwd;

	RPC_RPCMAKE(ch->ch, DBMUPDATE_RPC, ua);

	status_copy(status_r, ua[3].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbCreate(ConnHandle *ch, const char *dbmdb,
	   const char *userauth, const char *passwdauth,
	   const char *dbname, const DbCreateDescription *dbdesc)
  {
    if (!ch || !ch->ch)
      return IDB_dbCreate(ch, dbmdb, userauth, passwdauth, dbname, dbdesc);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	Data dbdesc_data;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	dbdesc_data = code_dbdescription(dbdesc, &pua->a_data.size);
	pua->a_data.data = dbdesc_data;

	RPC_RPCMAKE(ch->ch, DBCREATE_RPC, ua);

	free(dbdesc_data);
	status_copy(status_r, ua[5].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbDelete(ConnHandle *ch, const char *dbmdb,
	   const char *userauth, const char *passwdauth,
	   const char *dbname)
  {
    if (!ch || !ch->ch)
      return IDB_dbDelete(ch, dbmdb, userauth, passwdauth, dbname);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
      
	RPC_RPCMAKE(ch->ch, DBDELETE_RPC, ua);

	status_copy(status_r, ua[4].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbInfo(ConnHandle *ch, const char *dbmdb,
	 const char *userauth, const char *passwdauth, const char *dbname,
	 int *rdbid, DbCreateDescription *pdbdesc)
  {
    if (!ch || !ch->ch)
      return IDB_dbInfo(ch, dbmdb, userauth, passwdauth, dbname,
			rdbid, pdbdesc);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	pua++;
	pua++->a_data.data = 0;
      
	RPC_RPCMAKE(ch->ch, DBINFO_RPC, ua);

	if (rdbid)
	  *rdbid = ua[4].a_int;
	status_copy(status_r, ua[6].a_status);

	if (!status_r.err)
	  decode_dbdescription((unsigned char *)ua[5].a_data.data, 0, pdbdesc);
      
	free(ua[5].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbMove(ConnHandle *ch, const char *dbmdb,
	 const char *userauth, const char *passwdauth,
	 const char *dbname, const DbCreateDescription *dbdesc)
  {
    if (!ch || !ch->ch)
      return IDB_dbMove(ch, dbmdb, userauth, passwdauth, dbname, dbdesc);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	Data dbdesc_data;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	dbdesc_data = code_dbdescription(dbdesc, &pua->a_data.size);
	pua->a_data.data = dbdesc_data;

	RPC_RPCMAKE(ch->ch, DBMOVE_RPC, ua);
	free(dbdesc_data);

	status_copy(status_r, ua[5].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbCopy(ConnHandle *ch, const char *dbmdb,
	 const char *userauth, const char *passwdauth, const char *dbname,
	 const char *newname, Bool newdbid,
	 const DbCreateDescription *dbdesc)
  {
    if (!ch || !ch->ch)
      return IDB_dbCopy(ch, dbmdb, userauth, passwdauth, dbname,
			newname, newdbid, dbdesc);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	Data dbdesc_data;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	pua++->a_string = (char *)newname;
	pua++->a_int    = (int)newdbid;
	dbdesc_data = code_dbdescription(dbdesc, &pua->a_data.size);
	pua->a_data.data = dbdesc_data;

	RPC_RPCMAKE(ch->ch, DBCOPY_RPC, ua);

	free(dbdesc_data);

	status_copy(status_r, ua[7].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dbRename(ConnHandle *ch, const char *dbmdb,
	   const char *userauth, const char *passwdauth,
	   const char *dbname, const char *newname)
  {
    if (!ch || !ch->ch)
      return IDB_dbRename(ch, dbmdb, userauth, passwdauth,
			  dbname, newname);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	pua++->a_string = (char *)newname;

	RPC_RPCMAKE(ch->ch, DBRENAME_RPC, ua);

	status_copy(status_r, ua[5].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  userAdd(ConnHandle *ch, const char *dbmdb,
	  const char *userauth, const char *passwdauth,
	  const char *user, const char *passwd, int user_type)
  {
    if (!ch || !ch->ch)
      return IDB_userAdd(ch, dbmdb, userauth, passwdauth,
			 user, passwd, user_type);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)user;
	pua++->a_string = (char *)(passwd ? passwd : "");
	pua++->a_int    = user_type;

	RPC_RPCMAKE(ch->ch, USER_ADD_RPC, ua);

	status_copy(status_r, ua[6].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  userDelete(ConnHandle *ch, const char *dbmdb,
	     const char *userauth, const char *passwdauth,
	     const char *user)
  {
    if (!ch || !ch->ch)
      return IDB_userDelete(ch, dbmdb, userauth, passwdauth, user);
    else 
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)user;

	RPC_RPCMAKE(ch->ch, USER_DELETE_RPC, ua);

	status_copy(status_r, ua[4].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  userPasswdSet(ConnHandle *ch, const char *dbmdb,
		const char *userauth, const char *passwdauth,
		const char *user, const char *passwd)
  {
    if (!ch || !ch->ch)
      return IDB_userPasswdSet(ch, dbmdb, userauth, passwdauth,
			       user, passwd);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)user;
	pua++->a_string = (char *)passwd;

	RPC_RPCMAKE(ch->ch, USER_PASSWD_SET_RPC, ua);

	status_copy(status_r, ua[5].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  passwdSet(ConnHandle *ch, const char *dbmdb,
	    const char *user, const char *passwd,
	    const char *newpasswd)
  {
    if (!ch || !ch->ch)
      return IDB_passwdSet(ch, dbmdb, user, passwd, newpasswd);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)user;
	pua++->a_string = (char *)passwd;
	pua++->a_string = (char *)newpasswd;

	RPC_RPCMAKE(ch->ch, PASSWD_SET_RPC, ua);

	status_copy(status_r, ua[4].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  defaultDBAccessSet(ConnHandle *ch, const char *dbmdb,
		     const char *userauth, const char *passwdauth,
		     const char *dbname, int mode)
  {
    if (!ch || !ch->ch)
      return IDB_defaultDBAccessSet(ch, dbmdb, userauth, passwdauth,
				    dbname, mode);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	pua++->a_int    = mode;

	RPC_RPCMAKE(ch->ch, DEFAULT_DBACCESS_SET_RPC, ua);

	status_copy(status_r, ua[5].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  userDBAccessSet(ConnHandle *ch, const char *dbmdb,
		  const char *userauth, const char *passwdauth,
		  const char *dbname, const char *user, int mode)
  {
    if (!ch || !ch->ch)
      return IDB_userDBAccessSet(ch, dbmdb, userauth, passwdauth,
				 dbname, user, mode);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)dbname;
	pua++->a_string = (char *)user;
	pua++->a_int    = mode;

	RPC_RPCMAKE(ch->ch, USER_DBACCESS_SET_RPC, ua);

	status_copy(status_r, ua[6].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  userSysAccessSet(ConnHandle *ch, const char *dbmdb,
		   const char *userauth, const char *passwdauth,
		   const char *user, int mode)
  {
    if (!ch || !ch->ch)
      return IDB_userSysAccessSet(ch, dbmdb, userauth, passwdauth, user, mode);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_string = (char *)dbmdb;
	pua++->a_string = (char *)userauth;
	pua++->a_string = (char *)passwdauth;
	pua++->a_string = (char *)user;
	pua++->a_int    = mode;

	RPC_RPCMAKE(ch->ch, USER_SYSACCESS_SET_RPC, ua);

	status_copy(status_r, ua[5].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  /* typed object */
  RPCStatus
  objectCreate(DbHandle *dbh, short dspid, const Data idr, eyedbsm::Oid *oid)
  {
    start_rpc();

    CHECK_DBH(dbh, "objectCreate");

    if (DBH_IS_LOCAL(dbh))
      {
	Data inv_data;
	RPCStatus rpc_status = IDB_objectCreate((DbHandle *)dbh->u.dbh, dspid, idr,
						oid, 0, &inv_data, 0);
	if (!rpc_status)
	  object_epilogue(dbh->db, oid, inv_data, True);
	return rpc_status;
      }
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	ObjectHeader hdr;
	Offset offset = 0;

	if (!object_header_decode(idr, &offset, &hdr))
	  return rpcStatusMake(IDB_INVALID_OBJECT_HEADER,
			       "objectCreate: invalid object_header");

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	pua->a_data.data = (const Data)idr;
	pua++->a_data.size = hdr.size;      
	pua++->a_oid    = *oid;
	pua->a_data.data = 0; /* inv_data */
	pua++->a_data.size = 0;

	RPC_RPCMAKE(dbh->ch->ch, OBJECT_CREATE_RPC, ua);

	*oid = ua[3].a_oid;

	status_copy(status_r, ua[5].a_status);

	if (!status_r.err)
	  object_epilogue(dbh->db, oid, (unsigned char *)ua[4].a_data.data, True);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectDelete(DbHandle *dbh, const eyedbsm::Oid *oid, unsigned int flags)
  {
    CHECK_DBH(dbh, "objectDelete");

    if (DBH_IS_LOCAL(dbh))
      {
	Data inv_data;
	RPCStatus rpc_status = IDB_objectDelete((DbHandle *)dbh->u.dbh, oid,
						flags, &inv_data, 0);
	if (!rpc_status)
	  object_epilogue(dbh->db, oid, inv_data, False);
	return rpc_status;
      }
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;
	pua++->a_int = flags;
	pua->a_data.data = 0; /* inv_data */
	pua++->a_data.size = 0;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_DELETE_RPC, ua);

	status_copy(status_r, ua[4].a_status);
      
	if (!status_r.err)
	  object_epilogue(dbh->db, oid, (unsigned char *)ua[3].a_data.data, False);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectRead(DbHandle *dbh, Data idr, Data *pidr,
	     short *pdspid, const eyedbsm::Oid *oid, ObjectHeader *phdr,
	     LockMode lockmode, void **pcl)
  {
    CHECK_DBH(dbh, "objectRead");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectReadLocal((DbHandle *)dbh->u.dbh, idr, pidr, pdspid, oid, phdr, lockmode, pcl);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	ObjectHeader hdr;
	char buf[IDB_OBJ_HEAD_SIZE];

	start_rpc();

	if (idr)
	  {
	    object_header_decode_head(idr, &hdr);
	    memcpy(buf, idr, IDB_OBJ_HEAD_SIZE);
	  }
	else
	  {
	    memset(buf, 0, IDB_OBJ_HEAD_SIZE);
	    hdr.size = 0;
	  }

	pua++->a_int = RDBHID_GET(dbh);
	pua->a_data.data = buf;
	pua++->a_data.size = IDB_OBJ_HEAD_SIZE;
	pua++; /* dspid */
	pua++->a_oid = *oid;
	pua++->a_int = lockmode;

	pua->a_data.data = idr;
	pua++->a_data.size = hdr.size;

	RPC_RPCMAKE(dbh->ch->ch, OBJECT_READ_RPC, ua);

	status_copy(status_r, ua[6].a_status);
      
	if (!idr && !status_r.err)
	  *pidr = (unsigned char *)ua[5].a_data.data;
	if (!status_r.err && pdspid)
	  *pdspid = ua[2].a_int;

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectWrite(DbHandle *dbh, const Data idr, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "objectWrite");

    if (DBH_IS_LOCAL(dbh))
      {
	Data inv_data;
	RPCStatus rpc_status = IDB_objectWrite((DbHandle *)dbh->u.dbh, idr, oid, 0,
					       &inv_data, 0);
	if (!rpc_status)
	  object_epilogue(dbh->db, oid, inv_data, False);
	return rpc_status;
      }
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	Offset offset = 0;
	ObjectHeader hdr;

	start_rpc();

	if (!object_header_decode(idr, &offset, &hdr))
	  return rpcStatusMake(IDB_INVALID_OBJECT_HEADER,
			       "objectCreate: invalid object_header");

	pua++->a_int = RDBHID_GET(dbh);

	pua->a_data.data = (const Data)idr;
	pua++->a_data.size = hdr.size;
	pua++->a_oid = *oid;
	pua->a_data.data = 0; /* inv_data */
	pua++->a_data.size = 0;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_WRITE_RPC, ua);

	status_copy(status_r, ua[4].a_status);

	if (!status_r.err)
	  object_epilogue(dbh->db, oid, (unsigned char *)ua[3].a_data.data, False);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectHeaderRead(DbHandle *dbh, const eyedbsm::Oid *oid, ObjectHeader *hdr)
  {
    CHECK_DBH(dbh, "objectHeaderRead");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectHeaderRead((DbHandle *)dbh->u.dbh, oid, hdr);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;

	RPC_RPCMAKE(dbh->ch->ch, OBJECT_HEADER_READ_RPC, ua);

	status_copy(status_r, ua[8].a_status);

	if (status_r.err == IDB_SUCCESS)
	  {
	    hdr->magic   = IDB_OBJ_HEAD_MAGIC;
	    hdr->type    = ua[2].a_int;
	    hdr->size    = ua[3].a_int;
	    hdr->ctime   = ua[4].a_int;
	    hdr->mtime   = ua[5].a_int;
	    hdr->oid_cl = ua[6].a_oid;
	    hdr->oid_prot = ua[7].a_oid;
	    hdr->xinfo   = 0;
	  }

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectSizeModify(DbHandle *dbh, unsigned int size, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "objectSizeModify");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectSizeModify((DbHandle *)dbh->u.dbh, size, oid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = size;
	pua++->a_oid = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_SIZE_MODIFY_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectCheck(DbHandle *dbh, const eyedbsm::Oid *oid, int *state,
	      eyedbsm::Oid *cloid)
  {
    CHECK_DBH(dbh, "objectCheck");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectCheck((DbHandle *)dbh->u.dbh, oid, state, cloid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_CHECK_RPC, ua);

	*state = ua[2].a_int;
	*cloid = ua[3].a_oid;

	status_copy(status_r, ua[4].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectProtectionSet(DbHandle *dbh, const eyedbsm::Oid *obj_oid,
		      const eyedbsm::Oid *prot_oid)
  {
    CHECK_DBH(dbh, "objectProtectionSet");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectProtectionSet((DbHandle *)dbh->u.dbh, obj_oid, prot_oid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *obj_oid;
	pua++->a_oid = *prot_oid;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_PROTECTION_SET_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  objectProtectionGet(DbHandle *dbh, const eyedbsm::Oid *obj_oid,
		      eyedbsm::Oid *prot_oid)
  {
    CHECK_DBH(dbh, "objectProtectionGet");

    if (DBH_IS_LOCAL(dbh))
      return IDB_objectProtectionGet((DbHandle *)dbh->u.dbh, obj_oid, prot_oid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *obj_oid;
      
	RPC_RPCMAKE(dbh->ch->ch, OBJECT_PROTECTION_GET_RPC, ua);

	*prot_oid = ua[2].a_oid;

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }


  RPCStatus
  oidMake(DbHandle *dbh, short dspid, const Data idr, unsigned int size,
	  eyedbsm::Oid *oid)
  {
    /*
      #ifdef E_XDR
      ObjectHeader hdr;
      #endif
    */

    CHECK_DBH(dbh, "oidMake");

    /*
      #ifdef E_XDR
      memcpy(&hdr, idr, IDB_OBJ_HEAD_SIZE);
      Offset offset = 0;
      Size sz = IDB_OBJ_HEAD_SIZE;
      object_header_code((Data *)&idr, &offset, &sz, &hdr);
      #endif
    */

    if (DBH_IS_LOCAL(dbh)) {
#ifdef E_XDR
      ObjectHeader hdr;
      Offset offset = 0;
      object_header_decode(idr, &offset, &hdr);
      return IDB_oidMake((DbHandle *)dbh->u.dbh, &hdr, dspid, size, oid);
#else
      return IDB_oidMake((DbHandle *)dbh->u.dbh, (ObjectHeader *)idr, dspid, size, oid);
#endif
    }
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	/* XDR TBD : should code HDR */

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	pua->a_data.data = (const Data)idr;
	pua++->a_data.size = IDB_OBJ_HEAD_SIZE;
	pua++->a_int = size;
      
	RPC_RPCMAKE(dbh->ch->ch, OID_MAKE_RPC, ua);

	*oid = ua[4].a_oid;

	status_copy(status_r, ua[5].a_status);

	STATUS_RETURN(status_r);
      }
  }

  /* raw data object */
  RPCStatus
  dataCreate(DbHandle *dbh, short dspid, unsigned int size, const Data idr, eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "dataCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_dataCreate((DbHandle *)dbh->u.dbh, dspid, size, idr, oid, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = dspid;
	pua->a_data.size   = size;
	pua++->a_data.data = idr;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_CREATE_RPC, ua);

	*oid = ua[3].a_oid;

	status_copy(status_r, ua[4].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataDelete(DbHandle *dbh, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "dataDelete");

    if (DBH_IS_LOCAL(dbh))
      return IDB_dataDelete((DbHandle *)dbh->u.dbh, oid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid       = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_DELETE_RPC, ua);

	status_copy(status_r, ua[2].a_status);
      
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataRead(DbHandle *dbh, int offset, unsigned int size, Data idr, short *pdspid, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "dataRead");

    if (DBH_IS_LOCAL(dbh))
      return IDB_dataRead((DbHandle *)dbh->u.dbh, offset, size, idr, pdspid, oid, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = offset;
	pua++->a_int       = size;
	pua++; /* pdspid */
	pua++->a_oid       = *oid;
	pua->a_data.size   = size;
	pua++->a_data.data = idr;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_READ_RPC, ua);

	status_copy(status_r, ua[6].a_status);
	if (pdspid) *pdspid = ua[3].a_int;

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataWrite(DbHandle *dbh, int offset, unsigned int size, const Data idr, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "dataWrite");
  
    if (DBH_IS_LOCAL(dbh))
      return IDB_dataWrite((DbHandle *)dbh->u.dbh, offset, size, idr, oid, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = offset;
	pua->a_data.size   = size;
	pua++->a_data.data = idr;
	pua++->a_oid       = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_WRITE_RPC, ua);

	status_copy(status_r, ua[4].a_status);
  
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataSizeGet(DbHandle *dbh, const eyedbsm::Oid *oid, unsigned int *size)
  {
    CHECK_DBH(dbh, "dataSizeGet");

    if (DBH_IS_LOCAL(dbh))
      return IDB_dataSizeGet((DbHandle *)dbh->u.dbh, oid, size);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_SIZE_GET_RPC, ua);

	*size = ua[2].a_int;
	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataSizeModify(DbHandle *dbh, unsigned int size, const eyedbsm::Oid *oid)
  {
    CHECK_DBH(dbh, "dataSizeModify");

    if (DBH_IS_LOCAL(dbh))
      return IDB_dataSizeModify((DbHandle *)dbh->u.dbh, size, oid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = size;
	pua++->a_oid       = *oid;
      
	RPC_RPCMAKE(dbh->ch->ch, DATA_SIZE_MODIFY_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  VDdataCreate(DbHandle *dbh, short dspid, const eyedbsm::Oid *actual_oid_cl,
	       const eyedbsm::Oid *oid_cl, int num,
	       int count, int size, const Data data,
	       const eyedbsm::Oid *agr_oid, eyedbsm::Oid *data_oid,
	       Data idx_data, Size idx_size)
  {
    CHECK_DBH(dbh, "VDdataCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_VDdataCreate((DbHandle *)dbh->u.dbh, dspid, actual_oid_cl, oid_cl,
			      num, count, size, data,
			      agr_oid, data_oid, idx_data, idx_size, 0, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = dspid;
	pua++->a_oid	 = *actual_oid_cl;
	pua++->a_oid	 = *oid_cl;
	pua++->a_int	 = num;
	pua++->a_int	 = count;
	pua->a_data.size   = size;
	pua++->a_data.data = data;
	pua->a_data.size   = idx_size;
	pua++->a_data.data = idx_data;
	pua++->a_oid	 = *agr_oid;
      
	RPC_RPCMAKE(dbh->ch->ch, VDDATA_CREATE_RPC, ua);

	*data_oid = ua[9].a_oid;

	status_copy(status_r, ua[10].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  VDdataDelete(DbHandle *dbh, const eyedbsm::Oid *actual_oid_cl,
	       const eyedbsm::Oid *oid_cl, int num,
	       const eyedbsm::Oid *agr_oid, const eyedbsm::Oid *data_oid,
	       Data idx_data, Size idx_size)
  {
    CHECK_DBH(dbh, "VDdataDelete");

    if (DBH_IS_LOCAL(dbh))
      return IDB_VDdataDelete((DbHandle *)dbh->u.dbh, actual_oid_cl, oid_cl,
			      num, agr_oid, data_oid, idx_data, idx_size, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid	 = *actual_oid_cl;
	pua++->a_oid	 = *oid_cl;
	pua++->a_int	 = num;
	pua++->a_oid	 = *agr_oid;
	pua++->a_oid	 = *data_oid;
	pua->a_data.size   = idx_size;
	pua++->a_data.data = idx_data;

	RPC_RPCMAKE(dbh->ch->ch, VDDATA_DELETE_RPC, ua);

	status_copy(status_r, ua[7].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  VDdataWrite(DbHandle *dbh, const eyedbsm::Oid *actual_oid_cl,
	      const eyedbsm::Oid *oid_cl, int num,
	      int count, unsigned int size, const Data data,
	      const eyedbsm::Oid *agr_oid,
	      const eyedbsm::Oid *data_oid,
	      Data idx_data, Size idx_size)
  {
    CHECK_DBH(dbh, "VDdataWrite");

    if (DBH_IS_LOCAL(dbh))
      return IDB_VDdataWrite((DbHandle *)dbh->u.dbh, actual_oid_cl, oid_cl, num,
			     count, size, data,
			     agr_oid, data_oid, idx_data, idx_size, 0, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid	 = *actual_oid_cl;
	pua++->a_oid	 = *oid_cl;
	pua++->a_int	 = num;
	pua++->a_int	 = count;
	pua->a_data.size   = size;
	pua++->a_data.data = data;
	pua->a_data.size   = idx_size;
	pua++->a_data.data = idx_data;
	pua++->a_oid	 = *agr_oid;
	pua++->a_oid	 = *data_oid;
      
	RPC_RPCMAKE(dbh->ch->ch, VDDATA_WRITE_RPC, ua);

	status_copy(status_r, ua[9].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  schemaComplete(DbHandle *dbh, const char *schname)
  {
    CHECK_DBH(dbh, "schemaComplete");

    if (DBH_IS_LOCAL(dbh))
      return IDB_schemaComplete((DbHandle *)dbh->u.dbh, schname);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int    = RDBHID_GET(dbh);
	pua++->a_string = (char *)schname;
      
	RPC_RPCMAKE(dbh->ch->ch, SCHEMA_COMPLETE_RPC, ua);

	status_copy(status_r, ua[2].a_status);

	STATUS_RETURN(status_r);
      }
  }

  /* agreg item indexes */
  RPCStatus
  attributeIndexCreate(DbHandle *dbh, const eyedbsm::Oid *cloid, int num,
		       int mode, eyedbsm::Oid *multi_idx_oid,
		       Data idx_data, Size idx_size)
  {
    CHECK_DBH(dbh, "attributeIndexCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_attributeIndexCreate((DbHandle *)dbh->u.dbh, cloid, num, mode,
				      multi_idx_oid, idx_data, idx_size, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *cloid;
	pua++->a_int       = num;
	pua++->a_int       = mode;
	pua++->a_oid       = *multi_idx_oid;
	pua->a_data.data   = idx_data;
	pua++->a_data.size  = idx_size;
      
	RPC_RPCMAKE(dbh->ch->ch, ATTRIBUTE_INDEX_CREATE_RPC, ua);

	status_copy(status_r, ua[6].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  attributeIndexRemove(DbHandle *dbh, const eyedbsm::Oid *cloid, int num,
		       int mode, Data idx_data, Size idx_size)
  {
    CHECK_DBH(dbh, "attributeIndexRemove");

    if (DBH_IS_LOCAL(dbh))
      return IDB_attributeIndexRemove((DbHandle *)dbh->u.dbh, cloid, num, mode,
				      idx_data, idx_size, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *cloid;
	pua++->a_int       = num;
	pua++->a_int       = mode;
	pua->a_data.data   = idx_data;
	pua++->a_data.size  = idx_size;
      
	RPC_RPCMAKE(dbh->ch->ch, ATTRIBUTE_INDEX_REMOVE_RPC, ua);

	status_copy(status_r, ua[5].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexCreate(DbHandle * dbh, bool index_move, const eyedbsm::Oid * objoid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexCreate((DbHandle *)dbh->u.dbh, index_move, objoid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = index_move;
	pua++->a_oid = *objoid;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_CREATE_RPC, ua);


	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexRemove(DbHandle * dbh, const eyedbsm::Oid * objoid, int reentrant)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexRemove((DbHandle *)dbh->u.dbh, objoid, reentrant);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *objoid;
	pua++->a_int = reentrant;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_REMOVE_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  constraintCreate(DbHandle * dbh, const eyedbsm::Oid * objoid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_constraintCreate((DbHandle *)dbh->u.dbh, objoid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *objoid;

	RPC_RPCMAKE(dbh->ch->ch, CONSTRAINT_CREATE_RPC, ua);


	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  constraintDelete(DbHandle * dbh, const eyedbsm::Oid * objoid, int reentrant)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_constraintDelete((DbHandle *)dbh->u.dbh, objoid, reentrant);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *objoid;
	pua++->a_int = reentrant;

	RPC_RPCMAKE(dbh->ch->ch, CONSTRAINT_DELETE_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  /* collections */
  RPCStatus
  collectionGetByInd(DbHandle *dbh, const eyedbsm::Oid *oid, int ind,
		     int *found, Data buf, int size)
  {
    CHECK_DBH(dbh, "collectionGetByInd");

    if (DBH_IS_LOCAL(dbh))
      return IDB_collectionGetByInd((DbHandle *)dbh->u.dbh, oid, ind, found, buf, (void *)0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *oid;
	pua++->a_int       = ind;
	pua++;               /* found */
	pua->a_data.data   = buf;
	pua++->a_data.size = size;

	RPC_RPCMAKE(dbh->ch->ch, COLLECTION_GET_BY_IND_RPC, ua);

	*found = ua[3].a_int;
	status_copy(status_r, ua[5].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  collectionGetByValue(DbHandle *dbh, const eyedbsm::Oid *oid, Data val,
		       int size, int *found, int *ind)
  {
    CHECK_DBH(dbh, "collectionGetByValue");

    if (DBH_IS_LOCAL(dbh))
      return IDB_collectionGetByValue((DbHandle *)dbh->u.dbh, oid, val, found, ind);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *oid;
	pua->a_data.data   = val;
	pua++->a_data.size = size;
      
	RPC_RPCMAKE(dbh->ch->ch, COLLECTION_GET_BY_VALUE_RPC, ua);

	*found = ua[3].a_int;
	*ind   = ua[4].a_int;
	status_copy(status_r, ua[5].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  collectionGetByOid(DbHandle *dbh, const eyedbsm::Oid *oid,
		     const eyedbsm::Oid *loid, int *found, int *ind)
  {
    unsigned char data[sizeof(*loid)];
    oid_code(data, (Data)loid);
    return collectionGetByValue(dbh, oid, data,
				sizeof(eyedbsm::Oid), found, ind);

    return collectionGetByValue(dbh, oid, (Data)loid,
				sizeof(eyedbsm::Oid), found, ind);
  }

  /* object lock */

  RPCStatus
  setObjectLock(DbHandle * dbh, const eyedbsm::Oid * oid, int lockmode, int * rlockmode)
  {
    CHECK_DBH(dbh, "setObjectLock");

    if (DBH_IS_LOCAL(dbh))
      return IDB_setObjectLock((DbHandle *)dbh->u.dbh, oid, lockmode, rlockmode);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;
	pua++->a_int = lockmode;
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, SET_OBJECT_LOCK_RPC, ua);

	*rlockmode = ua[4].a_int;

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getObjectLock(DbHandle * dbh, const eyedbsm::Oid * oid, int * lockmode)
  {
    CHECK_DBH(dbh, "getObjectLock");

    if (DBH_IS_LOCAL(dbh))
      return IDB_getObjectLock((DbHandle *)dbh->u.dbh, oid, lockmode);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, GET_OBJECT_LOCK_RPC, ua);

	*lockmode = ua[3].a_int;

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  /* queries */
#if 0
  RPCStatus
  queryLangCreate(DbHandle *dbh, const char *str, int *qid,
		  void *pschinfo, int *count)
  {
    CHECK_DBH(dbh, "queryLangCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryLangCreate((DbHandle *)dbh->u.dbh, str, qid, count, pschinfo,
				 (void *)0, (void *)0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua->a_data.data   = (const Data)str;
	pua++->a_data.size = strlen(str)+1;
	pua++; /* qid */
	pua++; /* count */
	pua->a_data.size   = 0;
	pua++->a_data.data = 0; /* force RPC to allocate buffer */
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_LANG_CREATE_RPC, ua);

	*qid   = ua[2].a_int;
	*count = ua[3].a_int;
	status_copy(status_r, ua[5].a_status);
	if (!status_r.err)
	  decode_sch_info((unsigned char *)ua[4].a_data.data, pschinfo);

	free(ua[4].a_data.data);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  queryDatabaseCreate(DbHandle *dbh, int *qid)
  {
    CHECK_DBH(dbh, "queryDatabaseCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryDatabaseCreate((DbHandle *)dbh->u.dbh, qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_DATABASE_CREATE_RPC, ua);

	*qid   = ua[1].a_int;
	status_copy(status_r, ua[2].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  queryClassCreate(DbHandle *dbh, const eyedbsm::Oid *cloid, int *qid)
  {
    CHECK_DBH(dbh, "queryClassCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryClassCreate((DbHandle *)dbh->u.dbh, cloid, qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *cloid;
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_CLASS_CREATE_RPC, ua);

	*qid   = ua[2].a_int;
	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }
#endif

  RPCStatus
  queryCollectionCreate(DbHandle *dbh, const eyedbsm::Oid *oid, Bool index,
			int *qid)
  {
    CHECK_DBH(dbh, "queryCollectionCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryCollectionCreate((DbHandle *)dbh->u.dbh, oid, (int)index, qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int    = RDBHID_GET(dbh);
	pua++->a_oid    = *oid;
	pua++->a_int    = (int)index;
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_COLLECTION_CREATE_RPC, ua);

	*qid   = ua[3].a_int;
	status_copy(status_r, ua[4].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  queryAttributeCreate(DbHandle *dbh, const eyedbsm::Oid *cloid, int num,
		       int ind, Data start, Data end, int sexcl,
		       int eexcl, int x_size, int *qid)
  {
    CHECK_DBH(dbh, "queryAttributeCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryAttributeCreate((DbHandle *)dbh->u.dbh, cloid, num, ind,
				      start, end, sexcl, eexcl, x_size,
				      qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_oid       = *cloid;
	pua++->a_int       = num;
	pua++->a_int       = ind;
	pua->a_data.data   = start;
	pua++->a_data.size = x_size;
	pua->a_data.data   = end;
	pua++->a_data.size = x_size;
	pua++->a_int       = sexcl;
	pua++->a_int       = eexcl;
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_ATTRIBUTE_CREATE_RPC, ua);

	*qid   = ua[8].a_int;
	status_copy(status_r, ua[9].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  queryDelete(DbHandle *dbh, int qid)
  {
    CHECK_DBH(dbh, "queryDelete");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryDelete((DbHandle *)dbh->u.dbh, qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;

	start_rpc();

	pua++->a_int    = RDBHID_GET(dbh);
	pua++->a_int    = qid;
      
	RPC_RPCMAKE(dbh->ch->ch, QUERY_DELETE_RPC, ua);

	status_copy(status_r, ua[2].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  queryScanNext(DbHandle *dbh, int qid, int wanted, int *found,
		void *atom_array)
  {
    CHECK_DBH(dbh, "queryScanNext");

    if (DBH_IS_LOCAL(dbh))
      return IDB_queryScanNext((DbHandle *)dbh->u.dbh, qid, wanted, found, atom_array, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_int       = qid;
	pua++->a_int       = wanted;
	pua++;
	pua->a_data.size   = 0;
	pua++->a_data.data = 0; /* force RPC to allocate buffer */

	RPC_RPCMAKE(dbh->ch->ch, QUERY_SCAN_NEXT_RPC, ua);

	*found   = ua[3].a_int;
	status_copy(status_r, ua[5].a_status);
	if (!status_r.err)
	  decode_atom_array(&ua[4].a_data, atom_array, *found);

	STATUS_RETURN(status_r);
      }
  }

  /* oql */

  RPCStatus
  oqlCreate(ConnHandle *ch, DbHandle * dbh, const char * oql,
	    int *qid, void *schema_info)
  {
    CHECK_DBH(dbh, "oqlCreate");

    if (DBH_IS_LOCAL(dbh))
      return IDB_oqlCreate((DbHandle *)(dbh ? dbh->u.dbh : 0), oql, qid, schema_info,
			   0, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = (dbh ? RDBHID_GET(dbh) : 0);
	pua->a_data.data   = (const Data)oql;
	pua++->a_data.size = strlen(oql)+1;
	pua++; /* qid */
	pua->a_data.size   = 0;
	pua++->a_data.data = 0; /* force RPC to allocate buffer */

	RPC_RPCMAKE(ch->ch, OQL_CREATE_RPC, ua);

	*qid = ua[2].a_int;
	status_copy(status_r, ua[4].a_status);
	if (!status_r.err)
	  decode_sch_info((unsigned char *)ua[3].a_data.data, schema_info);

	free(ua[3].a_data.data);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  oqlDelete(ConnHandle *ch, DbHandle * dbh, int qid)
  {
    CHECK_DBH(dbh, "oqlDelete");

    if (DBH_IS_LOCAL(dbh))
      return IDB_oqlDelete((DbHandle *)(dbh ? dbh->u.dbh : 0), qid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = (dbh ? RDBHID_GET(dbh) : 0);
	pua++->a_int = qid;

	RPC_RPCMAKE(ch->ch, OQL_DELETE_RPC, ua);

	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  oqlGetResult(ConnHandle *ch, DbHandle * dbh, int qid,
	       void *value)
  {
    CHECK_DBH(dbh, "oqlGetResult");

    if (DBH_IS_LOCAL(dbh))
      return IDB_oqlGetResult((DbHandle *)(dbh ? dbh->u.dbh : 0), qid, value, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = (dbh ? RDBHID_GET(dbh) : 0);
	pua++->a_int = qid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(ch->ch, OQL_GETRESULT_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	if (!status_r.err)
	  decode_value(&ua[2].a_data, value);

	free(ua[2].a_data.data); /* added the 3/11/99 */

	STATUS_RETURN(status_r);
      }
  }

  /* executables */
  RPCStatus
  execExecute(DbHandle *dbh, const char *user, const char *passwd,
	      const char *intname,
	      const char *name, 
	      int exec_type,
	      const eyedbsm::Oid *cloid,
	      const char *extref,
	      const void *sign,
	      const eyedbsm::Oid *execoid,
	      const eyedbsm::Oid *objoid,
	      void *o,
	      const void *argarray,
	      void *argret)
  {
    CHECK_DBH(dbh, "execExecute");

    if (DBH_IS_LOCAL(dbh))
      return IDB_execExecute((DbHandle *)dbh->u.dbh, user, passwd, intname,
			     name, exec_type, cloid,
			     extref, sign, 0, execoid, objoid, o,
			     argarray, 0, argret, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	unsigned char *sign_data;
	unsigned char *argarray_data;
	start_rpc();

	code_signature(&ua[8], sign);
	code_arg_array(&ua[11], argarray);

	sign_data = (unsigned char *)ua[8].a_data.data;
	argarray_data = (unsigned char *)ua[11].a_data.data;

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_string    = (char *)user;
	pua++->a_string    = (char *)passwd;
	pua++->a_string    = (char *)intname;
	pua++->a_string    = (char *)name;
	pua++->a_int       = exec_type;
	pua++->a_oid       = *cloid;
	pua++->a_string    = (char *)extref;
	pua++;             /* ua[8] <= sign */
	pua++->a_oid       = *execoid;
	pua++->a_oid       = *objoid;
	pua++;             /* ua[11] <= argarray */
	pua->a_data.size   = 0;
	pua++->a_data.data = 0; /* force RPC to allocate buffer */

	RPC_RPCMAKE(dbh->ch->ch, EXECUTABLE_EXECUTE_RPC, ua);

	status_copy(status_r, ua[13].a_status);

	if (!status_r.err)
	  {
	    offset = decode_arg_array(dbh->db, &ua[12].a_data, (void **)argarray,
				      False);
	    decode_argument(dbh->db, &ua[12].a_data, argret, offset);
	  }

	/* added the 15/01/99;*/
	free(ua[12].a_data.data);

	free(sign_data);
	free(argarray_data);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  execCheck(DbHandle *dbh, const char *intname, const eyedbsm::Oid *oid,
	    const char *extref)
  {
    CHECK_DBH(dbh, "execCheck");

    if (DBH_IS_LOCAL(dbh))
      return IDB_execCheck((DbHandle *)dbh->u.dbh, intname, oid, extref);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r;

	start_rpc();

	pua++->a_int       = RDBHID_GET(dbh);
	pua++->a_string    = (char *)intname;
	pua++->a_oid       = *oid;
	pua++->a_string    = (char *)extref;

	RPC_RPCMAKE(dbh->ch->ch, EXECUTABLE_CHECK_RPC, ua);

	status_copy(status_r, ua[4].a_status);

	STATUS_RETURN(status_r);
      }
  }

  extern RPCStatus
  execSetExtRefPath(ConnHandle *ch, const char *user,
		    const char *passwd, const char *path)
  {
    if (!ch)
      return IDB_execSetExtRefPath(user, passwd, path);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r;

	start_rpc();

	pua++->a_string    = (char *)user;
	pua++->a_string    = (char *)passwd;
	pua++->a_string    = (char *)path;

	RPC_RPCMAKE(ch->ch, EXECUTABLE_SET_EXTREF_PATH_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  extern RPCStatus
  execGetExtRefPath(ConnHandle *ch, const char *user,
		    const char *passwd, char path[], unsigned int pathlen)
  {
    if (!ch)
      return IDB_execGetExtRefPath(user, passwd, path, pathlen);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, min;

	start_rpc();

	pua++->a_string    = (char *)user;
	pua++->a_string    = (char *)passwd;
	pua->a_data.size   = 0;
	pua++->a_data.data = 0; /* force RPC to allocate buffer */

	RPC_RPCMAKE(ch->ch, EXECUTABLE_GET_EXTREF_PATH_RPC, ua);

	min = (pathlen < ua[2].a_data.size ? pathlen : ua[2].a_data.size);
	memcpy(path, ua[2].a_data.data, min);
	path[min] = 0;

	free(ua[2].a_data.data); // 1/10/05

	status_copy(status_r, ua[3].a_status);

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  checkAuth(ConnHandle *ch, const char *file)
  {
    if (!ch)
      return IDB_checkAuth(file);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_string = (char *)file;

	RPC_RPCMAKE(ch->ch, CHECK_AUTH_RPC, ua);

	status_copy(status_r, ua[1].a_status);
	STATUS_RETURN(status_r);
      }
  }


  RPCStatus
  set_conn_info(ConnHandle *ch, const char *hostname,
		int uid, const char *username, const char *progname,
		int *sv_pid, int *sv_uid, int sv_version, char **challenge)
  {
    if (!ch)
      return IDB_setConnInfo(hostname, uid, username, progname, getpid(),
			     sv_pid, sv_uid, sv_version, challenge);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, min;

      start_rpc();

      pua++->a_string = (char *)hostname;
      pua++->a_int    = uid;
      pua++->a_string = (char *)username;
      pua++->a_string = (char *)progname;
      pua++->a_int    = getpid();
      pua++;            /* sv_pid */
      pua++;            /* sv_uid */
      pua++->a_int    = sv_version;

      RPC_RPCMAKE(ch->ch, SET_CONN_INFO_RPC, ua);

      *sv_pid = ua[5].a_int;
      *sv_uid = ua[6].a_int;
      *challenge = ua[8].a_string;

      status_copy(status_r, ua[9].a_status);

      STATUS_RETURN(status_r);
    }
  }

  RPCStatus
  setLogMask(ConnHandle *ch, eyedblib::int64 logmask)
  {
    if (!ch)
      return IDB_setLogMask(logmask);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int64 = logmask;

	RPC_RPCMAKE(ch->ch, SET_LOG_MASK_RPC, ua);

	status_copy(status_r, ua[1].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getDefaultDataspace(DbHandle * dbh, int * dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getDefaultDataspace((DbHandle *)dbh->u.dbh, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, GET_DEFAULT_DATASPACE_RPC, ua);

	*dspid = ua[1].a_int;

	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  setDefaultDataspace(DbHandle * dbh, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_setDefaultDataspace((DbHandle *)dbh->u.dbh, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, SET_DEFAULT_DATASPACE_RPC, ua);

	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getDefaultIndexDataspace(DbHandle * dbh, const eyedbsm::Oid *oid, int type, int * dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getDefaultIndexDataspace((DbHandle *)dbh->u.dbh, oid, type, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *oid;
	pua++->a_int = type;
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, GET_DEFAULT_INDEX_DATASPACE_RPC, ua);

	*dspid = ua[3].a_int;

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataspaceSetCurrentDatafile(DbHandle * dbh, int dspid, int datid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_dataspaceSetCurrentDatafile((DbHandle *)dbh->u.dbh, dspid, datid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	pua++->a_int = datid;

	RPC_RPCMAKE(dbh->ch->ch, DATASPACE_SET_CURRENT_DATAFILE_RPC, ua);


	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  dataspaceGetCurrentDatafile(DbHandle * dbh, int dspid, int * datid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_dataspaceGetCurrentDatafile((DbHandle *)dbh->u.dbh, dspid, datid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, DATASPACE_GET_CURRENT_DATAFILE_RPC, ua);

	*datid = ua[2].a_int;

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  setDefaultIndexDataspace(DbHandle * dbh, const eyedbsm::Oid *idxoid, int type, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_setDefaultIndexDataspace((DbHandle *)dbh->u.dbh, idxoid, type, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	start_rpc();

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua++->a_int = type;
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, SET_DEFAULT_INDEX_DATASPACE_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getIndexLocations(DbHandle * dbh, const eyedbsm::Oid * idxoid,
		    int type, void * locarr)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getIndexLocations((DbHandle *)dbh->u.dbh, idxoid, type, (unsigned char **)locarr, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua++->a_int = type;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, GET_INDEX_LOCATIONS_RPC, ua);

	if (!status_r.err)
	  decode_locarr((unsigned char *)ua[3].a_data.data, locarr);

	free(ua[3].a_data.data); // 1/10/05

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }


  RPCStatus
  moveIndex(DbHandle * dbh, const eyedbsm::Oid * idxoid, int type, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_moveIndex((DbHandle *)dbh->u.dbh, idxoid, type, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua++->a_int = type;
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, MOVE_INDEX_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getInstanceClassLocations(DbHandle * dbh, const eyedbsm::Oid * clsoid,
			    int subclasses, Data * locarr)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getInstanceClassLocations((DbHandle *)dbh->u.dbh, clsoid, subclasses, locarr, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *clsoid;
	pua++->a_int = subclasses;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, GET_INSTANCE_CLASS_LOCATIONS_RPC, ua);

	if (!status_r.err)
	  decode_locarr((unsigned char *)ua[3].a_data.data, locarr);

	free(ua[3].a_data.data); // 1/10/05

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  moveInstanceClass(DbHandle * dbh, const eyedbsm::Oid * clsoid,
		    int subclasses, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_moveInstanceClass((DbHandle *)dbh->u.dbh, clsoid, subclasses, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *clsoid;
	pua++->a_int = subclasses;
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, MOVE_INSTANCE_CLASS_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getObjectsLocations(DbHandle * dbh, const eyedbsm::Oid *oids, unsigned int oid_cnt, void * locarr)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getObjectsLocations((DbHandle *)dbh->u.dbh, oids, oid_cnt, 0, (unsigned char **)locarr, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	Data oid_data;

	pua++->a_int = RDBHID_GET(dbh);
	oid_data = code_oids(oids, oid_cnt, &pua->a_data.size);
	pua++->a_data.data = oid_data;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, GET_OBJECTS_LOCATIONS_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	if (!status_r.err)
	  decode_locarr((unsigned char *)ua[2].a_data.data, locarr);

	free(oid_data);
	free(ua[2].a_data.data); // 1/10/05

	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  moveObjects(DbHandle * dbh, const eyedbsm::Oid * oids, unsigned int oid_cnt,
	      int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_moveObjects((DbHandle *)dbh->u.dbh, oids, oid_cnt, dspid, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	Data oids_data;

	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	oids_data = code_oids(oids, oid_cnt, &pua->a_data.size);
	pua++->a_data.data = oids_data;
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, MOVE_OBJECTS_RPC, ua);

	free(oids_data);
	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getAttributeLocations(DbHandle * dbh, const eyedbsm::Oid * clsoid, int attrnum, Data * locarr)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getAttributeLocations((DbHandle *)dbh->u.dbh, clsoid, attrnum, locarr, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *clsoid;
	pua++->a_int = attrnum;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, GET_ATTRIBUTE_LOCATIONS_RPC, ua);

	*locarr = (unsigned char *)ua[3].a_data.data;

	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  moveAttribute(DbHandle * dbh, const eyedbsm::Oid * clsoid, int attrnum, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_moveAttribute((DbHandle *)dbh->u.dbh, clsoid, attrnum, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *clsoid;
	pua++->a_int = attrnum;
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, MOVE_ATTRIBUTE_RPC, ua);


	status_copy(status_r, ua[4].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  createDatafile(DbHandle * dbh, const char * datfile, const char * name, int maxsize, int slotsize, int dtype)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_createDatafile((DbHandle *)dbh->u.dbh, datfile, name, maxsize, slotsize, dtype);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_string = (char *)datfile;
	pua++->a_string = (char *)name;
	pua++->a_int = maxsize;
	pua++->a_int = slotsize;
	pua++->a_int = dtype;

	RPC_RPCMAKE(dbh->ch->ch, CREATE_DATAFILE_RPC, ua);

	status_copy(status_r, ua[6].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  deleteDatafile(DbHandle * dbh, int datid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_deleteDatafile((DbHandle *)dbh->u.dbh, datid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;

	RPC_RPCMAKE(dbh->ch->ch, DELETE_DATAFILE_RPC, ua);


	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  moveDatafile(DbHandle * dbh, int datid, const char * datafile)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_moveDatafile((DbHandle *)dbh->u.dbh, datid, datafile);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;
	pua++->a_string = (char *)datafile;

	RPC_RPCMAKE(dbh->ch->ch, MOVE_DATAFILE_RPC, ua);


	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  defragmentDatafile(DbHandle * dbh, int datid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_defragmentDatafile((DbHandle *)dbh->u.dbh, datid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;

	RPC_RPCMAKE(dbh->ch->ch, DEFRAGMENT_DATAFILE_RPC, ua);


	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  resizeDatafile(DbHandle * dbh, int datid, unsigned int size)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_resizeDatafile((DbHandle *)dbh->u.dbh, datid, size);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;
	pua++->a_int = size;

	RPC_RPCMAKE(dbh->ch->ch, RESIZE_DATAFILE_RPC, ua);


	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  getDatafileInfo(DbHandle * dbh, int datid, void * info)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getDatafileInfo((DbHandle *)dbh->u.dbh, datid, (unsigned char **)info, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, GET_DATAFILEI_NFO_RPC, ua);

	if (!status_r.err)
	  decode_datinfo((unsigned char *)ua[2].a_data.data, info);

	free(ua[2].a_data.data); // 1/10/05
	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  renameDatafile(DbHandle * dbh, int datid, const char * name)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_renameDatafile((DbHandle *)dbh->u.dbh, datid, name);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = datid;
	pua++->a_string = (char *)name;

	RPC_RPCMAKE(dbh->ch->ch, RENAME_DATAFILE_RPC, ua);


	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  createDataspace(DbHandle * dbh, const char * dspname, void *datafiles,
		  unsigned int datafile_cnt)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_createDataspace((DbHandle *)dbh->u.dbh, dspname, datafiles, datafile_cnt, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	Data datfile_data;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_string = (char *)dspname;
	datfile_data = code_datafiles(datafiles, datafile_cnt,
				      &pua->a_data.size);
	pua++->a_data.data = datfile_data;

	RPC_RPCMAKE(dbh->ch->ch, CREATE_DATASPACE_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	free(datfile_data);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  updateDataspace(DbHandle * dbh, int dspid, void *datafiles,
		  unsigned int datafile_cnt, int flags, int orphan_dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_updateDataspace((DbHandle *)dbh->u.dbh, dspid, datafiles, datafile_cnt, 0, flags, orphan_dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	Data datfile_data;

	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	datfile_data = code_datafiles(datafiles, datafile_cnt,
				      &pua->a_data.size);
	pua++->a_data.data = datfile_data;
	pua++->a_int = flags;
	pua++->a_int = orphan_dspid;

	RPC_RPCMAKE(dbh->ch->ch, UPDATE_DATASPACE_RPC, ua);

	status_copy(status_r, ua[5].a_status);
	free(datfile_data);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  deleteDataspace(DbHandle * dbh, int dspid)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_deleteDataspace((DbHandle *)dbh->u.dbh, dspid);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;

	RPC_RPCMAKE(dbh->ch->ch, DELETE_DATASPACE_RPC, ua);


	status_copy(status_r, ua[2].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  renameDataspace(DbHandle * dbh, int dspid, const char * name)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_renameDataspace((DbHandle *)dbh->u.dbh, dspid, name);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = dspid;
	pua++->a_string = (char *)name;

	RPC_RPCMAKE(dbh->ch->ch, RENAME_DATASPACE_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexGetCount(DbHandle * dbh, const eyedbsm::Oid * idxoid, int * cnt)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexGetCount((DbHandle *)dbh->u.dbh, idxoid, cnt);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua++;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_GET_COUNT_RPC, ua);

	*cnt = ua[2].a_int;

	status_copy(status_r, ua[3].a_status);
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexGetStats(DbHandle * dbh, const eyedbsm::Oid * idxoid, Data * stats)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexGetStats((DbHandle *)dbh->u.dbh, idxoid, stats, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_GET_STATS_RPC, ua);

	status_copy(status_r, ua[3].a_status);

	if (!status_r.err)
	  decode_index_stats((unsigned char *)ua[2].a_data.data, stats);

	free(ua[2].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexSimulStats(DbHandle * dbh, const eyedbsm::Oid * idxoid,
		  const Data impl, Size impl_size, Data * stats)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexSimulStats((DbHandle *)dbh->u.dbh, idxoid, impl, 0, stats, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua->a_data.data = (const Data)impl;
	pua++->a_data.size = impl_size;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_SIMUL_STATS_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	if (!status_r.err)
	  decode_index_stats((unsigned char *)ua[3].a_data.data, stats);

	free(ua[3].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  collectionGetImplStats(DbHandle * dbh, int idxtype,
			 const eyedbsm::Oid * idxoid, Data * stats)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_collectionGetImplStats((DbHandle *)dbh->u.dbh, idxtype, idxoid, stats, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = idxtype;
	pua++->a_oid = *idxoid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, COLLECTION_GET_IMPLSTATS_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	if (!status_r.err)
	  decode_index_stats((unsigned char *)ua[3].a_data.data, stats);

	free(ua[3].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  collectionSimulImplStats(DbHandle * dbh, int idxtype, const eyedbsm::Oid * idxoid, const Data impl, Size impl_size, Data * stats)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_collectionSimulImplStats((DbHandle *)dbh->u.dbh, idxtype, idxoid, impl, 0,
					  stats, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = idxtype;
	pua++->a_oid = *idxoid;
	pua->a_data.data = (const Data)impl;
	pua++->a_data.size = impl_size;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, COLLECTION_SIMUL_IMPLSTATS_RPC, ua);

	status_copy(status_r, ua[5].a_status);
	if (!status_r.err)
	  decode_index_stats((unsigned char *)ua[4].a_data.data, stats);

	free(ua[4].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  indexGetImplementation(DbHandle * dbh, const eyedbsm::Oid * idxoid, Data * impl)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_indexGetImplementation((DbHandle *)dbh->u.dbh, idxoid, impl, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_oid = *idxoid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, INDEX_GET_IMPL_RPC, ua);

	status_copy(status_r, ua[3].a_status);
	if (!status_r.err)
	  decode_index_impl((unsigned char *)ua[2].a_data.data, impl);

	free(ua[2].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  collectionGetImplementation(DbHandle * dbh, int idxtype, const eyedbsm::Oid * idxoid, Data * impl)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_collectionGetImplementation((DbHandle *)dbh->u.dbh, idxtype, idxoid, impl, 0);
    else
      {
	ClientArg ua[IDB_MAXARGS], *pua = ua;
	int r, offset;
	pua++->a_int = RDBHID_GET(dbh);
	pua++->a_int = idxtype;
	pua++->a_oid = *idxoid;
	pua->a_data.data = 0; /* force RPC to allocate buffer */
	pua++->a_data.size   = 0;

	RPC_RPCMAKE(dbh->ch->ch, COLLECTION_GET_IMPL_RPC, ua);

	status_copy(status_r, ua[4].a_status);
	if (!status_r.err)
	  decode_index_impl((unsigned char *)ua[3].a_data.data, impl);

	free(ua[3].a_data.data); // 1/10/05
	STATUS_RETURN(status_r);
      }
  }

  RPCStatus
  setMaxObjCount(DbHandle * dbh, int obj_cnt)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_setMaxObjCount((DbHandle *)dbh->u.dbh, obj_cnt);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, offset;
      pua++->a_int = RDBHID_GET(dbh);
      pua++->a_int = obj_cnt;

      RPC_RPCMAKE(dbh->ch->ch, SET_MAXOBJCOUNT_RPC, ua);


      status_copy(status_r, ua[2].a_status);
      STATUS_RETURN(status_r);
    }
  }

  RPCStatus
  getMaxObjCount(DbHandle * dbh, int * obj_cnt)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getMaxObjCount((DbHandle *)dbh->u.dbh, obj_cnt);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, offset;
      pua++->a_int = RDBHID_GET(dbh);
      pua++;

      RPC_RPCMAKE(dbh->ch->ch, GET_MAXOBJCOUNT_RPC, ua);

      *obj_cnt = ua[1].a_int;

      status_copy(status_r, ua[2].a_status);
      STATUS_RETURN(status_r);
    }
  }

  RPCStatus
  setLogSize(DbHandle * dbh, int size)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_setLogSize((DbHandle *)dbh->u.dbh, size);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, offset;
      pua++->a_int = RDBHID_GET(dbh);
      pua++->a_int = size;

      RPC_RPCMAKE(dbh->ch->ch, SET_LOGSIZE_RPC, ua);


      status_copy(status_r, ua[2].a_status);
      STATUS_RETURN(status_r);
    }
  }

  RPCStatus
  getLogSize(DbHandle * dbh, int * size)
  {
    if (DBH_IS_LOCAL(dbh))
      return IDB_getLogSize((DbHandle *)dbh->u.dbh, size);
    else {
      ClientArg ua[IDB_MAXARGS], *pua = ua;
      int r, offset;
      pua++->a_int = RDBHID_GET(dbh);
      pua++;

      RPC_RPCMAKE(dbh->ch->ch, GET_LOGSIZE_RPC, ua);

      *size = ua[1].a_int;

      status_copy(status_r, ua[2].a_status);
      STATUS_RETURN(status_r);
    }
  }
}
