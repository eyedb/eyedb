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


#define _IDB_KERN_
#include <assert.h>
#include <dlfcn.h>
#include "eyedb_p.h"
#include "kernel_p.h"
#include "DBM_Database.h"

#include "serv_lib.h"
#include "kernel.h"
#include "ExecutableCache.h"

namespace eyedb {

  // ---------------------------------------------------------------------------
  //
  // Backend Methods
  //
  // ---------------------------------------------------------------------------

  RPCStatus
  IDB_execCheck(DbHandle *, const char *intname, const eyedbsm::Oid *oid,
		const char *extref)
  {
    //  printf("EXEC CHECK BACKEND %s %s\n", intname, extref);
    return RPCSuccess;
  }


  extern RPCStatus
  IDB_checkDBAuth(ConnHandle *ch, const char *dbmdb,
		  const char *dbname, const char *&userauth,
		  const char *&passwdauth, DBAccessMode dbmode,
		  Bool, int *, DBM_Database **pdbm, const char *msg);

  static RPCStatus
  checkAuth(Database *db, const char *userauth, const char *passwdauth)
  {
    return IDB_checkDBAuth(db->getConnection()->getConnHandle(),
			   db->getDBMDB(),
			   db->getName(),
			   userauth,
			   passwdauth,
			   ExecDBAccessMode,
			   False,
			   NULL,
			   0,
			   "execute method in backend");
  }

  //#define USE_GBXCONTEXT

  RPCStatus
  IDB_execExecute(DbHandle *dbh,
		  const char *userauth, const char *passwdauth,
		  const char *intname,
		  const char *name,
		  int exec_type,
		  const eyedbsm::Oid *cloid,
		  const char *extref,
		  const void *xsign, const void *sign_data,
		  const eyedbsm::Oid *execoid,
		  const eyedbsm::Oid *xobjoid,
		  void *xo,
		  const void *xargarray, void *argarray_data,
		  void *xargret, void *argarrayret_data)
  {
    Database *db = (Database *)dbh->db;
    RPCStatus rpc_status;
#ifdef USE_GBXCONTEXT
    gbxContext _;
#endif
    int isStaticExec = (exec_type & STATIC_EXEC);
    exec_type &= ~STATIC_EXEC;

    if (!dbh->exec_auth) {
      rpc_status = checkAuth(db, userauth, passwdauth);
      if (!rpc_status)
	dbh->exec_auth = 1;
    }
    else
      rpc_status = RPCSuccess;

    if (rpc_status)
      {
	if (argarrayret_data)
	  {
	    Argument arg;
	    rpc_Data *data = (rpc_Data *)argarrayret_data;
	    data->size = 0;
	    data->data = NULL;
	    code_argument(argarrayret_data, (const void *)&arg);
	  }
	return rpc_status;
      }

    ArgArray *argarray = 0;
    Signature *sign = 0;
    ExecutableItem *item;
    Status status = Success;
    Argument arg;
    Argument *parg = (argarrayret_data ? &arg : (Argument *)xargret);
    Oid objoid(xobjoid);
    Object *o = (Object *)xo;

    if (argarray_data)
      decode_arg_array(db, argarray_data, (void **)&argarray, True);
    else
      argarray = (ArgArray *)xargarray;

    if (sign_data)
      {
	sign = new Signature();
	decode_signature(sign_data, (void *)sign);
      }
    else
      sign = (Signature *)xsign;

    if (!(item = db->exec_cache->get(intname)))
      {
	item = new ExecutableItem(db, intname, name, exec_type, isStaticExec,
				  cloid,
				  extref, sign, execoid);
	if (status = item->check())
	  {
	    delete item;
	    goto out;
	  }
	db->exec_cache->insert(item);
      }
 
    if (objoid.isValid() && !o)
      {
	status = db->loadObject(&objoid, &o);
	if (status)
	  goto out;
      }

    status = item->execute(o, argarray, parg);

#define NO_EXEC_CHECK

#ifndef NO_EXEC_CHECK
    if (!status) {
      const char *which = "method";
      status = eyedb_CHECKArgument(db, *sign->getRettype(), *parg,
				   which, name, "return");
      if (!status)
	status = eyedb_CHECKArguments(db, sign, *argarray, which,
				      name, OUT_ARG_TYPE);
    }
#endif

  out:

    if (argarrayret_data)
      {
	code_arg_array(argarrayret_data, argarray);
	code_argument(argarrayret_data, (const void *)&arg);
      }
    else
      {
	//*(Argument *)xargret = arg;
#ifdef USE_GBXCONTEXT
	((Argument *)xargret)->keep();
	((Argument *)xargret)->getType()->keep();
#endif
      }
  
#ifdef USE_GBXCONTEXT
    if (!status && (db->isLocal() || db->getUserData() == IDB_LOCAL_CALL))
      _.keepObjs();
#endif
    
#ifndef USE_GBXCONTEXT
    if (argarray_data && argarray)
      argarray->release();
    if (sign_data && sign)
      sign->release();
#endif
    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_execSetExtRefPath(const char *user, const char *passwd, const char *path)
  {
    // not yet implemented
    return RPCSuccess;
  }

  RPCStatus
  IDB_execGetExtRefPath(const char *user, const char *passwd, void *path,
			unsigned int pathlen)
  { 
    // not yet implemented
    return RPCSuccess;
  }

  // ---------------------------------------------------------------------------
  //
  // Executable Cache Management
  //
  // ---------------------------------------------------------------------------

  ExecutableItem::ExecutableItem(Database *_db, 
				 const char *_intname,
				 const char *name,
				 int _exec_type,
				 int isStaticExec,
				 const Oid &cloid,
				 const char *_extref,
				 Signature *sign,
				 const Oid& oid)
  {
    db = _db;
    exec_type = _exec_type;
    dl = NULL;
    csym = NULL;
    intname = strdup(_intname);
    extref = strdup(_extref);

    if (exec_type == METHOD_C_TYPE)
      {
	const Class *cl = db->getSchema()->getClass(cloid); 
	assert(cl);
	exec = new BEMethod_C(db, (Class *)cl, name, sign,
			      (isStaticExec?True:False), False,
			      extref);
      }
    else
      {
	utlog("EXEC_TYPE %d not yet implemented, ABORTING PROCESSUS!\n");
	abort();
      }

    ObjectPeer::setOid(exec, oid);
  }

  Status ExecutableItem::check()
  {
    return Executable::checkRealize(extref, intname, &dl, &csym);
  }

  Status
  MethodLaunch(Status (*method)(Database *, Method *, Object *,
				ArgArray &, Argument &),
	       Database *db,
	       Method *mth,
	       Object *o,
	       ArgArray &argarray,
	       Argument &retarg)
  {
    return method(db, mth, o, argarray, retarg);
  }

  Status ExecutableItem::execute(Object *o, ArgArray *argarray,
				 Argument *retarg)
  {
    if (exec_type == METHOD_C_TYPE)
      return MethodLaunch((Status (*)(Database *, Method *,
				      Object *, ArgArray &,
				      Argument &))csym,
			  db,
			  (Method *)exec,
			  o, *argarray,
			  *retarg);

    abort();
  }

  ExecutableItem::~ExecutableItem()
  {
    free(intname);
    free(extref);
    exec->release();
  }

  inline unsigned int ExecutableCache::get_key(const char *tok)
  {
    int len = strlen(tok);
    int k = 0;

    for (int i = 0; i < len; i++)
      k += *tok++;

    return k & mask;
  }

#define CACHESIZE 128

  ExecutableCache::ExecutableCache()
  {
    nkeys = CACHESIZE;
    mask = nkeys-1;
    lists = (LinkedList **)malloc(sizeof(LinkedList *) * nkeys);
    memset(lists, 0, sizeof(LinkedList *)*nkeys);
  }

  void ExecutableCache::insert(ExecutableItem *item)
  {
    int k = get_key(item->intname);
  
    if (!lists[k])
      lists[k] = new LinkedList();

    lists[k]->insertObjectLast(item);
  }

  void ExecutableCache::remove(const char *intname)
  {
    remove(get(intname));
  }

  void ExecutableCache::remove(ExecutableItem *item)
  {
    if (!item)
      return;

    LinkedList *list;

    if (list = lists[get_key(item->intname)])
      list->deleteObject(item);
  }

  ExecutableItem *ExecutableCache::get(const char *intname)
  {
    LinkedList *list;

    if (list = lists[get_key(intname)])
      {
	ExecutableItem *item;
	LinkedListCursor *c = list->startScan();

	while (list->getNextObject(c, (void *&)item))
	  if (!strcmp(item->intname, intname))
	    {
	      list->endScan(c);
	      return item;
	    }

	list->endScan(c);
      }

    return NULL;
  }

  ExecutableCache::~ExecutableCache()
  {
    for (int i = 0; i < nkeys; i++)
      {
	LinkedList *list = lists[i];

	if (!list)
	  continue;

	ExecutableItem *item;
	LinkedListCursor *c = list->startScan();

	while (list->getNextObject(c, (void *&)item))
	  delete item;

	list->endScan(c);
	delete list;
      }

    free(lists);
  }

}
