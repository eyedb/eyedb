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


#ifndef _EYEDB_KERNEL_H
#define _EYEDB_KERNEL_H

namespace eyedb {

  extern void
  IDB_init(const char *, const char *, void *, int);

  /* database management */
  extern RPCStatus
  IDB_dbmCreate(ConnHandle *, const char *, const char *,
		const DbCreateDescription *);

  extern RPCStatus
  IDB_dbmUpdate(ConnHandle *, const char *, const char *,
		const char *);

  extern RPCStatus
  IDB_dbCreate(ConnHandle *, const char *, const char *,
	       const char *, const char *,
	       const DbCreateDescription *);

  extern RPCStatus
  IDB_dbDelete(ConnHandle *, const char *, const char *,
	       const char *, const char *);

  extern RPCStatus
  IDB_dbInfo(ConnHandle *, const char *, const char *,
	     const char *, const char *,
	     int *, DbCreateDescription *);

  extern RPCStatus
  IDB_dbMove(ConnHandle *, const char *, const char *,
	     const char *, const char *,
	     const DbCreateDescription *);

  extern RPCStatus
  IDB_dbCopy(ConnHandle *, const char *, const char *, const char *,
	     const char *, const char *,
	     Bool, const DbCreateDescription *);

  extern RPCStatus
  IDB_dbRename(ConnHandle *, const char *, const char *, const char *,
	       const char *, const char *);

  extern RPCStatus
  IDB_dbOpen(ConnHandle *, const char *, const char *,
	     const char *, const char *, int,
	     int, int, unsigned int, int *, int *, void *, char **, int *,
	     unsigned int *, DbHandle **);

  extern RPCStatus
  IDB_dbOpen_make(ConnHandle *, const char *, const char *,
		  const char *, const char *,
		  int, int, int, unsigned int, int *, int *, char **, int *,
		  unsigned int *,
		  DbHandle **);

  extern RPCStatus
  IDB_dbClose(DbHandle *);

  RPCStatus
  IDB_dbClose_make(DbHandle *dbh);

  extern RPCStatus
  IDB_dbCloseLocal(DbHandle *);

  /* database admin */
  extern RPCStatus
  IDB_userAdd(ConnHandle *, const char *, const char *, const char *,
	      const char *, const char *, int user_type);

  extern RPCStatus
  IDB_userDelete(ConnHandle *, const char *, const char *, const char *,
		 const char *);

  extern RPCStatus
  IDB_userPasswdSet(ConnHandle *, const char *, const char *, const char *,
		    const char *, const char *);

  extern RPCStatus
  IDB_passwdSet(ConnHandle *, const char *, const char *, const char *,
		const char *);

  extern RPCStatus
  IDB_defaultDBAccessSet(ConnHandle *, const char *, const char *,
			 const char *, const char *, int);

  extern RPCStatus
  IDB_userDBAccessSet(ConnHandle *, const char *,
		      const char *, const char *,
		      const char *, const char *, int);

  extern RPCStatus
  IDB_userSysAccessSet(ConnHandle *, const char *,
		       const char *, const char *,
		       const char *, int);

  extern RPCStatus
  IDB_backendInterrupt(ConnHandle *, int);

  RPCStatus
  IDB_backendInterruptReset();

  /* transaction management */

  extern RPCStatus
  IDB_transactionBegin(DbHandle *,
		       const TransactionParams *params,
		       Bool local_call);

  extern RPCStatus
  IDB_transactionCommit(DbHandle *, Bool local_call);

  extern RPCStatus
  IDB_transactionAbort(DbHandle *, Bool local_call);

  extern RPCStatus
  IDB_transactionParamsSet(DbHandle *,
			   const TransactionParams *params);

  extern RPCStatus
  IDB_transactionParamsGet(DbHandle *,
			   TransactionParams *params);

  /* object management */

  extern RPCStatus
  IDB_objectCreate(DbHandle *, short dspid, const Data, eyedbsm::Oid *, void *,
		   Data *, void *);

  extern RPCStatus
  IDB_objectDelete(DbHandle *, const eyedbsm::Oid *, unsigned int,
		   Data *, void *);

  extern RPCStatus
  IDB_objectRead(DbHandle *, Data, Data *, short *, const eyedbsm::Oid *,
		 LockMode lockmode, void *);

  extern RPCStatus
  IDB_objectReadLocal(DbHandle *, Data, Data *, short *, const eyedbsm::Oid *,
		      ObjectHeader *, LockMode lockmode, void **);

  extern RPCStatus
  IDB_objectWrite(DbHandle *, const Data, const eyedbsm::Oid *, void *, Data *, void *);

  extern RPCStatus
  IDB_objectHeaderRead(DbHandle *, const eyedbsm::Oid *, ObjectHeader *);

  extern RPCStatus
  IDB_objectSizeModify(DbHandle *, unsigned int, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_objectCheck(DbHandle *, const eyedbsm::Oid *, int *, eyedbsm::Oid *);

  extern RPCStatus
  IDB_objectCheckAccess(DbHandle *, Bool write, const eyedbsm::Oid *,
			Bool *);

  extern RPCStatus
  IDB_objectProtectionSet(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_objectProtectionGet(DbHandle *, const eyedbsm::Oid *, eyedbsm::Oid *);

  extern RPCStatus
  IDB_oidMake(DbHandle *, ObjectHeader *, short, unsigned int, eyedbsm::Oid *);

  /* raw data object */
  extern RPCStatus
  IDB_dataCreate(DbHandle *, short, unsigned int, const Data, eyedbsm::Oid *, void *);

  extern RPCStatus
  IDB_dataDelete(DbHandle *, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_dataRead(DbHandle *, int, unsigned int, Data, short *, const eyedbsm::Oid *, void *);

  extern RPCStatus
  IDB_dataWrite(DbHandle *, int, unsigned int, const Data, const eyedbsm::Oid *, void *);

  extern RPCStatus
  IDB_dataSizeGet(DbHandle *, const eyedbsm::Oid *, unsigned int *);

  extern RPCStatus
  IDB_dataSizeModify(DbHandle *, unsigned int, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_schemaComplete(DbHandle *, const char *);

  /* vardim */
  extern RPCStatus
  IDB_VDdataCreate(DbHandle *, short, const eyedbsm::Oid *, const eyedbsm::Oid *,
		   int, int, int, const Data,
		   const eyedbsm::Oid *, eyedbsm::Oid *,
		   const Data, int, void *, void *);

  extern RPCStatus
  IDB_VDdataDelete(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *,
		   int, const eyedbsm::Oid *, const eyedbsm::Oid *,
		   const Data, int, void *);

  extern RPCStatus
  IDB_VDdataWrite(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *,
		  int, int, unsigned int, const Data,
		  const eyedbsm::Oid *, const eyedbsm::Oid *,
		  const Data, int, void *, void *);

  /* agreg item indexes */
  extern RPCStatus
  IDB_attributeIndexCreate(DbHandle *, const eyedbsm::Oid *, int, int,
			   eyedbsm::Oid *, Data, Size, void *);

  extern RPCStatus
  IDB_attributeIndexRemove(DbHandle *, const eyedbsm::Oid *, int, int,
			   Data, Size, void *);

  extern RPCStatus
  IDB_indexCreate(DbHandle *, bool index_move, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_indexRemove(DbHandle *, const eyedbsm::Oid *, int);

  extern RPCStatus
  IDB_constraintCreate(DbHandle *, const eyedbsm::Oid *);

  extern RPCStatus
  IDB_constraintDelete(DbHandle *, const eyedbsm::Oid *, int);

  /* collections */
  extern RPCStatus
  IDB_collectionGetByInd(DbHandle *, const eyedbsm::Oid *, int, int *, Data,
			 void *);

  extern RPCStatus
  IDB_collectionGetByValue(DbHandle *, const eyedbsm::Oid *, Data, int *, int *);

  extern RPCStatus
  IDB_setObjectLock(DbHandle *, const eyedbsm::Oid *, int, int *);

  extern RPCStatus
  IDB_getObjectLock(DbHandle *, const eyedbsm::Oid *, int *);

  /* queries */
  extern RPCStatus
  IDB_queryLangCreate(DbHandle *, const char *, int *, int *,
		      void *, void *, void *);

  extern RPCStatus
  IDB_queryDatabaseCreate(DbHandle *, int *);

  extern RPCStatus
  IDB_queryClassCreate(DbHandle *, const eyedbsm::Oid *, int *);

  extern RPCStatus
  IDB_queryCollectionCreate(DbHandle *, const eyedbsm::Oid *, int, int *);

  extern RPCStatus
  IDB_queryAttributeCreate(DbHandle *, const eyedbsm::Oid *, int, int, Data,
			   Data, int, int, int, int *);

  extern RPCStatus
  IDB_queryDelete(DbHandle *, int);

  extern RPCStatus
  IDB_queryScanNext(DbHandle *, int qid, int wanted, int *found,
		    void *atom_array, void *);

  /* OQL */
  extern RPCStatus
  IDB_oqlCreate(DbHandle *, const char *, int *, void *, void *, void *);

  extern RPCStatus
  IDB_oqlDelete(DbHandle *, int);

  extern RPCStatus
  IDB_oqlGetResult(DbHandle *, int qid, void *atom_array, void *);

  /* executables */
  extern RPCStatus
  IDB_execCheck(DbHandle *, const char *intname, const eyedbsm::Oid *oid,
		const char *extref);

  extern RPCStatus
  IDB_execExecute(DbHandle *, const char *user, const char *passwd,
		  const char *intname,
		  const char *name, int exec_type,
		  const eyedbsm::Oid *cloid, const char *extref,
		  const void *xsign, const void *sign_data,
		  const eyedbsm::Oid *execoid,
		  const eyedbsm::Oid *objoid,
		  void *o,
		  const void *xargarray, void *argarray_data,
		  void *xargret, void *argret_data);

  extern RPCStatus
  IDB_execSetExtRefPath(const char *user, const char *passwd, const char *path);

  extern RPCStatus
  IDB_execGetExtRefPath(const char *user, const char *passwd, void *path,
			unsigned int pathlen);

  extern RPCStatus
  IDB_setConnInfo(const char *hostname, int uid, const char *username,
		  const char *progname, int pid, int *sv_pid, int *sv_uid,
		  int cli_version, char **challenge);

  extern RPCStatus
  IDB_checkAuth(const char *);

  extern RPCStatus
  IDB_setLogMask(eyedblib::int64);

  extern RPCStatus
  IDB_indexGetCount(DbHandle *, const eyedbsm::Oid *, int *);

  extern RPCStatus
  IDB_indexGetStats(DbHandle *, const eyedbsm::Oid *, Data *, void *);

  extern RPCStatus
  IDB_indexSimulStats(DbHandle *, const eyedbsm::Oid *, const Data, void *,
		      Data *, void *);

  extern RPCStatus
  IDB_collectionGetImplStats(DbHandle *, int, const eyedbsm::Oid *,
			     Data *, void *);

  extern RPCStatus
  IDB_collectionSimulImplStats(DbHandle *, int, const eyedbsm::Oid *,
			       const Data, void *, Data *, void *);

  extern RPCStatus
  IDB_indexGetImplementation(DbHandle *, const eyedbsm::Oid *, Data *, void *);

  extern RPCStatus
  IDB_collectionGetImplementation(DbHandle *, int, const eyedbsm::Oid *, Data *, void *);

  extern RPCStatus
  IDB_getDefaultDataspace(DbHandle *, int *);

  extern RPCStatus
  IDB_setDefaultDataspace(DbHandle *, int);

  extern RPCStatus
  IDB_dataspaceSetCurrentDatafile(DbHandle *, int, int);

  extern RPCStatus
  IDB_dataspaceGetCurrentDatafile(DbHandle *, int, int *);

  extern RPCStatus
  IDB_getDefaultIndexDataspace(DbHandle *, const eyedbsm::Oid *, int, int *);

  extern RPCStatus
  IDB_setDefaultIndexDataspace(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  IDB_getIndexLocations(DbHandle *, const eyedbsm::Oid *, int, Data *, void *);

  extern RPCStatus
  IDB_moveIndex(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  IDB_getInstanceClassLocations(DbHandle *, const eyedbsm::Oid *, int,
				Data *, void *);

  extern RPCStatus
  IDB_moveInstanceClass(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  IDB_getObjectsLocations(DbHandle *, const eyedbsm::Oid *, unsigned int, void *, Data *, void *);

  extern RPCStatus
  IDB_moveObjects(DbHandle *, const eyedbsm::Oid *, unsigned int, int, void *);

  extern RPCStatus
  IDB_getAttributeLocations(DbHandle *, const eyedbsm::Oid *, int, Data *, void *);

  extern RPCStatus
  IDB_moveAttribute(DbHandle *, const eyedbsm::Oid *, int, int);

  extern RPCStatus
  IDB_createDatafile(DbHandle *, const char *, const char *, int, int, int);

  extern RPCStatus
  IDB_deleteDatafile(DbHandle *, int);

  extern RPCStatus
  IDB_moveDatafile(DbHandle *, int, const char *);

  extern RPCStatus
  IDB_defragmentDatafile(DbHandle *, int);

  extern RPCStatus
  IDB_resizeDatafile(DbHandle *, int, unsigned int);

  extern RPCStatus
  IDB_getDatafileInfo(DbHandle *, int, Data *, void *);

  extern RPCStatus
  IDB_renameDatafile(DbHandle *, int, const char *);

  extern RPCStatus
  IDB_createDataspace(DbHandle *, const char *, void *, unsigned int, void *);

  extern RPCStatus
  IDB_updateDataspace(DbHandle *, int, void *, unsigned int, void *, int, int);

  extern RPCStatus
  IDB_deleteDataspace(DbHandle *, int);

  extern RPCStatus
  IDB_renameDataspace(DbHandle *, int, const char *);

  extern RPCStatus
  IDB_getServerOutOfBandData(ConnHandle *, int *, Data *,
			     unsigned int *size, void *);

  extern RPCStatus
  IDB_setMaxObjCount(DbHandle *, int);
  
  extern RPCStatus
  IDB_getMaxObjCount(DbHandle *, int *);

  extern RPCStatus
  IDB_setLogSize(DbHandle *, int);
  
  extern RPCStatus
  IDB_getLogSize(DbHandle *, int *);

  /* back end */

  extern void
  IDB_getLocalInfo(DbHandle *dbh, rpcDB_LocalDBContext *, eyedbsm::Oid *);

  extern RPCStatus
  IDB_collClassRegister(DbHandle *, const eyedbsm::Oid *, const eyedbsm::Oid *,
			Bool insert);

  extern int
  IDB_getSeTrsCount(DbHandle *);

  extern void
  object_epilogue(void *db, const eyedbsm::Oid *oid,
		      Data data, Bool creating);

  extern void
  decode_dbdescription(Data idr, void *xdata, DbCreateDescription *dbdesc);

}

#endif
