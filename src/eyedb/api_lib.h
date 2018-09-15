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


#ifndef _EYEDB_API_LIB_H
#define _EYEDB_API_LIB_H

#include <eyedblib/rpc_fe.h>
#include "rpc_lib.h"
#include <eyedblib/rpcdb.h>
#include <eyedb/internals/ObjectHeader.h>

namespace eyedb {

#ifndef _IDB_KERN_
  typedef union {
    eyedblib::int32 a_int;
    eyedblib::int64 a_int64;
    char *a_string;
    rpc_ClientData a_data;

    /* specific idb */
    rpcDB_LocalDBContext a_ldbctx;
    eyedbsm::Oid a_oid;
    Bool a_bool;
    RPCStatusRec a_status;
  } ClientArg;

  extern rpc_ClientFunction
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

    *INDEX_GET_COUNT_RPC,
    *INDEX_GET_STATS_RPC,
    *INDEX_SIMUL_STATS_RPC,
    *COLLECTION_GET_IMPLSTATS_RPC,
    *COLLECTION_SIMUL_IMPLSTATS_RPC,
    *INDEX_GET_IMPL_RPC,
    *COLLECTION_GET_IMPL_RPC,

    *SET_LOG_MASK_RPC,

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
#endif

  extern rpc_Client *
  rpcFeInit(void);

  extern void
  rpcFeRelease(void);

  extern rpc_Client *
  getRpcClient(void);

  extern RPCStatus
  connOpen(const char *, const char *, ConnHandle **, int, std::string &);

  extern RPCStatus
  connClose(ConnHandle *);

  extern RPCStatus
  dbmCreate(ConnHandle *, const char *, const char *,
		const DbCreateDescription *);

  extern RPCStatus
  dbmUpdate(ConnHandle *, const char *, const char *, const char *);

  extern RPCStatus
  dbCreate(ConnHandle *, const char *, const char *, const char *,
	       const char *, const DbCreateDescription *);

  extern RPCStatus
  dbDelete(ConnHandle *, const char *, const char *, const char *,
	       const char *);

  extern RPCStatus
  dbInfo(ConnHandle *, const char *, const char *, const char *,
	     const char *, int *, DbCreateDescription *);

  extern RPCStatus
  dbMove(ConnHandle *, const char *, const char *, const char *,
	     const char *, const DbCreateDescription *);

  extern RPCStatus
  dbCopy(ConnHandle *, const char *, const char *, const char *,
	     const char *, const char *,
	     Bool, const DbCreateDescription *);

  extern RPCStatus
  dbRename(ConnHandle *, const char *, const char *, const char *,
	       const char *, const char *);

  extern RPCStatus
  userAdd(ConnHandle *, const char *, const char *, const char *,
	      const char *, const char *, int);

  extern RPCStatus
  userDelete(ConnHandle *, const char *, const char *, const char *,
		 const char *);

  extern RPCStatus
  userPasswdSet(ConnHandle *, const char *, const char *, const char *,
		    const char *, const char *);

  extern RPCStatus
  passwdSet(ConnHandle *, const char *, const char *, const char *,
		const char *);

  extern RPCStatus
  defaultDBAccessSet(ConnHandle *, const char *,
			 const char *, const char *, const char *, int);

  extern RPCStatus
  userDBAccessSet(ConnHandle *, const char *,
		      const char *, const char *,
		      const char *, const char *, int);

  extern RPCStatus
  userSysAccessSet(ConnHandle *, const char *,
		       const char *, const char *,
		       const char *, int);

  extern RPCStatus
  backendInterrupt(ConnHandle *, int);

  /* transactions */

  extern RPCStatus
  transactionBegin(DbHandle *, 
		       const TransactionParams *,
		       TransactionId *);

  extern RPCStatus
  transactionCommit(DbHandle *, TransactionId);

  extern RPCStatus
  transactionAbort(DbHandle *, TransactionId);

  extern RPCStatus
  transactionParamsSet(DbHandle *, 
			   const TransactionParams *);

  extern RPCStatus
  transactionParamsGet(DbHandle *, TransactionParams *);

  /* databases */

  extern RPCStatus
  dbOpen(ConnHandle *, const char *, const char *, const char *,
	     const char *, int, int, int, unsigned int,
	     int *, void *, char **, int *,
	     unsigned int *, DbHandle **);

  extern RPCStatus
  dbClose(const DbHandle *);

  /* typed object */
  extern RPCStatus
  objectCreate(DbHandle *, short, const Data, eyedbsm::Oid *);

  extern RPCStatus
  objectDelete(DbHandle *, const eyedbsm::Oid *, unsigned int flags);

  extern RPCStatus
  objectRead(DbHandle *, Data, Data *, short *, const eyedbsm::Oid *,
		 ObjectHeader *phdr, LockMode lockmode, void **);

  extern RPCStatus
  objectWrite(DbHandle *, const Data, const eyedbsm::Oid *);

  extern RPCStatus
  objectHeaderRead(DbHandle *, const eyedbsm::Oid *, ObjectHeader *);

  extern RPCStatus
  objectSizeModify(DbHandle *, unsigned int, const eyedbsm::Oid *);

  extern RPCStatus
  objectCheck(DbHandle *, const eyedbsm::Oid *, int *, eyedbsm::Oid *);

  extern RPCStatus
  objectProtectionSet(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *);

  extern RPCStatus
  objectProtectionGet(DbHandle *, const eyedbsm::Oid *, eyedbsm::Oid *);

  extern RPCStatus
  oidMake(DbHandle *, short, const Data, unsigned int, eyedbsm::Oid *);

  /* raw data object */
  extern RPCStatus
  dataCreate(DbHandle *, short, unsigned int, const Data, eyedbsm::Oid *);

  extern RPCStatus
  dataDelete(DbHandle *, const eyedbsm::Oid *);

  extern RPCStatus
  dataRead(DbHandle *, int, unsigned int, Data, short *, const eyedbsm::Oid *);

  extern RPCStatus
  dataWrite(DbHandle *, int, unsigned int, const Data, const eyedbsm::Oid *);

  extern RPCStatus
  dataSizeGet(DbHandle *, const eyedbsm::Oid *, unsigned int *);

  extern RPCStatus
  dataSizeModify(DbHandle *, unsigned int, const eyedbsm::Oid *);

  extern RPCStatus
  schemaComplete(DbHandle *, const char *);

  /* vardim */
  extern RPCStatus
  VDdataCreate(DbHandle *, short, const eyedbsm::Oid *, const eyedbsm::Oid *,
		   int, int, int, const Data, const eyedbsm::Oid *, eyedbsm::Oid *,
		   Data, Size);

  extern RPCStatus
  VDdataDelete(DbHandle *,const eyedbsm::Oid *,  const eyedbsm::Oid *,
		   int, const eyedbsm::Oid *, const eyedbsm::Oid *,
		   Data, Size);

  extern RPCStatus
  VDdataWrite(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *,
		  int, int, unsigned int, const Data, const eyedbsm::Oid *, const eyedbsm::Oid *,
		  Data, Size);

  /* agreg item indexes */
  extern RPCStatus
  attributeIndexCreate(DbHandle *, const eyedbsm::Oid *, int, int,
			   eyedbsm::Oid *, Data, Size);

  extern RPCStatus
  attributeIndexRemove(DbHandle *, const eyedbsm::Oid *, int, int, Data,
			   Size);

  extern RPCStatus
  indexCreate(DbHandle *, bool index_move, const eyedbsm::Oid *);

  extern RPCStatus
  indexRemove(DbHandle *, const eyedbsm::Oid *, int);

  extern RPCStatus
  constraintCreate(DbHandle *, const eyedbsm::Oid *);

  extern RPCStatus
  constraintDelete(DbHandle *, const eyedbsm::Oid *, int);

  /* collections */
  extern RPCStatus
  collectionGetByInd(DbHandle *, const eyedbsm::Oid *, int, int *, Data,
			 int);

  extern RPCStatus
  collectionGetByOid(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *, int *,
			 int *);

  extern RPCStatus
  collectionGetByValue(DbHandle *, const eyedbsm::Oid *, Data, int,
			   int *, int *);

  /* object lock */
  extern RPCStatus
  setObjectLock(DbHandle *, const eyedbsm::Oid *, int, int *);

  extern RPCStatus
  getObjectLock(DbHandle *, const eyedbsm::Oid *, int *);

  /* queries */
  extern RPCStatus
  queryLangCreate(DbHandle *, const char *, int *, void *, int *);

  extern RPCStatus
  queryDatabaseCreate(DbHandle *, int *);

  extern RPCStatus
  queryClassCreate(DbHandle *, const eyedbsm::Oid *, int *);

  extern RPCStatus
  queryCollectionCreate(DbHandle *, const eyedbsm::Oid *, Bool, int *);

  extern RPCStatus
  queryAttributeCreate(DbHandle *, const eyedbsm::Oid *, int, int, Data,
			   Data, int, int, int, int *);

  extern RPCStatus
  queryDelete(DbHandle *, int);

  extern RPCStatus
  queryScanNext(DbHandle *, int qid, int wanted, int *found,
		    void *atom_array);

  /* executables */

  extern RPCStatus
  execCheck(DbHandle *, const char *intname, const eyedbsm::Oid *oid,
		const char *extref);

  extern RPCStatus
  execExecute(DbHandle *, const char *user, const char *passwd,
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
		  void *argret);

  extern RPCStatus
  execSetExtRefPath(ConnHandle *, const char *user,
			const char *passwd, const char *path);

  extern RPCStatus
  execGetExtRefPath(ConnHandle *, const char *user, const char *passwd,
			char path[], unsigned int pathlen);

  extern RPCStatus
  set_conn_info(ConnHandle *, const char *, int, const char *, const char *,
		int *, int *, int, char **);

  extern RPCStatus
  checkAuth(ConnHandle *, const char *);

  extern RPCStatus
  oqlCreate(ConnHandle *, DbHandle *, const char *, int *, void *);

  extern RPCStatus
  oqlDelete(ConnHandle *, DbHandle *, int);

  extern RPCStatus
  oqlGetResult(ConnHandle *, DbHandle *, int, void *);

  extern RPCStatus
  setLogMask(ConnHandle *, eyedblib::int64);

  extern RPCStatus
  indexGetCount(DbHandle *, const eyedbsm::Oid *, int *);

  extern RPCStatus
  indexGetStats(DbHandle *, const eyedbsm::Oid *, Data *);

  extern RPCStatus
  indexSimulStats(DbHandle *, const eyedbsm::Oid *, const Data,
		      Size, Data *);

  extern RPCStatus
  collectionGetImplStats(DbHandle *, int, const eyedbsm::Oid *, Data *);

  extern RPCStatus
  collectionSimulImplStats(DbHandle *, int, const eyedbsm::Oid *, const Data, Size, Data *);

  extern RPCStatus
  indexGetImplementation(DbHandle *, const eyedbsm::Oid *, Data *);

  extern RPCStatus
  collectionGetImplementation(DbHandle *, int, const eyedbsm::Oid *, Data *);

  extern RPCStatus
  getDefaultDataspace(DbHandle *, int *);

  extern RPCStatus
  setDefaultDataspace(DbHandle *, int);

  extern RPCStatus
  dataspaceSetCurrentDatafile(DbHandle *, int, int);

  extern RPCStatus
  dataspaceGetCurrentDatafile(DbHandle *, int, int *);

  extern RPCStatus
  getDefaultIndexDataspace(DbHandle *, const eyedbsm::Oid *, int, int *);

  extern RPCStatus
  setDefaultIndexDataspace(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  getIndexLocations(DbHandle *, const eyedbsm::Oid *, int, void *);

  extern RPCStatus
  moveIndex(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  getInstanceClassLocations(DbHandle *, const eyedbsm::Oid *, int, Data *);

  extern RPCStatus
  moveInstanceClass(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  getObjectsLocations(DbHandle *, const eyedbsm::Oid *, unsigned int, void *);

  extern RPCStatus
  moveObjects(DbHandle *, const eyedbsm::Oid *, unsigned int cnt,
		  int dspid);

  extern RPCStatus
  getAttributeLocations(DbHandle *, const eyedbsm::Oid *, int, Data *);

  extern RPCStatus
  moveAttribute(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  createDatafile(DbHandle *, const char *, const char *, int, int, int);

  extern RPCStatus
  deleteDatafile(DbHandle *, int);

  extern RPCStatus
  moveDatafile(DbHandle *, int, const char *);

  extern RPCStatus
  defragmentDatafile(DbHandle *, int);

  extern RPCStatus
  resizeDatafile(DbHandle *, int, unsigned int);

  extern RPCStatus
  getDatafileInfo(DbHandle *, int, void *);

  extern RPCStatus
  renameDatafile(DbHandle *, int, const char *);

  extern RPCStatus
  createDataspace(DbHandle *, const char *, void *, unsigned int);

  extern RPCStatus
  updateDataspace(DbHandle *, int, void *, unsigned int, int, int);

  extern RPCStatus
  deleteDataspace(DbHandle *, int);

  extern RPCStatus
  renameDataspace(DbHandle *, int, const char *);

  extern RPCStatus
  getServerOutOfBandData(ConnHandle *, int *, Data *,
			     unsigned int *);

  extern RPCStatus
  setMaxObjCount(DbHandle *, int);

  extern RPCStatus
  getMaxObjCount(DbHandle *, int *);

  extern RPCStatus
  setLogSize(DbHandle *, int);

  extern RPCStatus
  getLogSize(DbHandle *, int *);

  /* misc */
  extern RPCStatus
  SEconnOpen(const char *, ConnHandle *);
}

#endif
