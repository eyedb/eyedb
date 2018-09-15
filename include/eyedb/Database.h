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


#ifndef _EYEDB_DATABASE_H
#define _EYEDB_DATABASE_H

#include <eyedb/ObjCache.h>
#include <vector>
#include <map>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class BEQueue;
  class ObjCache;
  class ExecutableCache;
  class Protection;
  class SchemaInfo;
  class DBProperty;
  class Transaction;
  class GenHashTable;

  /**
     Not yet documented.
  */
  class Database : public Struct {

    // ----------------------------------------------------------------------
    // Database Interface
    // ----------------------------------------------------------------------
  public:

    enum OpenFlag {
      DBRead                    = _DBRead,
      DBRW                      = _DBRW,
      DBReadLocal               = DBRead | _DBOpenLocal,
      DBRWLocal                 = DBRW | _DBOpenLocal,
      DBReadAdmin               = DBRead | _DBAdmin,
      DBRWAdmin                 = DBRW | _DBAdmin,
      DBReadAdminLocal          = DBReadAdmin | _DBOpenLocal,
      DBRWAdminLocal            = DBRWAdmin | _DBOpenLocal,
      DBSRead                   = _DBSRead,
      DBSReadLocal              = DBSRead | _DBOpenLocal,
      DBSReadAdmin              = DBSRead | _DBAdmin
    };

    /**
       Not yet documented
       @param dbname
       @param dbmdb_str
    */
    Database(const char *dbname, const char *dbmdb_str = 0);

    /**
       Not yet documented
       @param conn
       @param dbname
       @param flag
       @param user
       @param passwd
    */
    Database(Connection *conn,
	     const char *dbname,
	     Database::OpenFlag flag = Database::DBRead,
	     const char *user = 0,
	     const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param dbname
       @param dbmdb_str
       @param flag
       @param user
       @param passwd
    */
    Database(Connection *conn,
	     const char *dbname,
	     const char *dbmdb_str,
	     Database::OpenFlag flag = Database::DBRead,
	     const char *user = 0,
	     const char *passwd = 0);

    /**
       Not yet documented
       @param dbname
       @param dbid
       @param dbmdb_str
    */
    Database(const char *dbname, int dbid, const char *dbmdb_str = 0);

    /**
       Not yet documented
       @param dbid
       @param dbmdb_str
    */
    Database(int dbid, const char *dbmdb_str = 0);

    /**
       Not yet documented
       @param db
    */
    Database(const Database &db);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Database(*this);}

    /**
       Not yet documented
       @param ccnn
       @param user
       @param passwd
       @param pdbdesc
       @return
    */
    Status create(Connection *conn, const char *user = 0,
		  const char *passwd = 0,
		  DbCreateDescription *pdbdesc = 0);

    /**
       Not yet documented
       @param conn
       @param pdbdesc
       @return
    */
    Status create(Connection *conn, DbCreateDescription *pdbdesc);

    /**
       Not yet documented
       @param conn
       @param user
       @param passwd
       @return
    */
    Status remove(Connection *conn, const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param user
       @param passwd
       @return
    */
    Status remove(const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param mode
       @param user
       @param passwd
       @return
    */
    Status setDefaultDBAccess(Connection *conn, int mode,
			      const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param username
       @param mode
       @param user
       @param passwd
       @return
    */
    Status setUserDBAccess(Connection *conn, const char *username,
			   int mode,
			   const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param user
       @param passwd
       @param pdbdesc
       @return
    */
    Status getInfo(Connection *conn,
		   const char *user, const char *passwd, DbInfoDescription *pdbdesc) const;

    /**
       Not yet documented
       @param user
       @param passwd
       @param pdbdesc
       @return
    */
    Status getInfo(const char *user, const char *passwd, DbInfoDescription *pdbdesc) const;

    /**
       Not yet documented
       @param conn
       @param flag
       @param user
       @param passwd
       @return
    */
    virtual Status open(Connection *conn,
			Database::OpenFlag flag = Database::DBRead,
			const char *user = 0,
			const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param flag
       @param hints
       @param user
       @param passwd
       @return
    */
    virtual Status open(Connection *conn,
			Database::OpenFlag flag,
			const OpenHints *hints,
			const char *user = 0,
			const char *passwd = 0);

    /**
       Not yet documented
       @return
    */
    Status close();

    /**
       Not yet documented
       @param newdbname
       @param user
       @param passwd
       @return
    */
    Status rename(const char *newdbname, const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param dbdesc
       @param user
       @param passwd
       @return
    */
    Status move(DbCreateDescription *dbdesc, const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param newdbname
       @param newdbid
       @param dbdesc
       @param user
       @param passwd
       @return
    */
    Status copy(const char *newdbname, Bool newdbid, DbCreateDescription *dbdesc,
		const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param newdbname
       @param user
       @param passwd
       @return
    */
    Status rename(Connection *conn, const char *newdbname,
		  const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param dbdesc
       @param user
       @param passwd
       @return
    */
    Status move(Connection *conn, DbCreateDescription *dbdesc,
		const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param conn
       @param newdbname
       @param newdbid
       @param dbdesc
       @param user
       @param passwd
       @return
    */
    Status copy(Connection *conn, const char *newdbname, Bool newdbid,
		DbCreateDescription *dbdesc,
		const char *user = 0, const char *passwd = 0);

    /**
       Not yet documented
       @param oid
       @param found
       @return
    */
    Status containsObject(const Oid &oid, Bool &found);

    /**
       Not yet documented
       @param oid
       @param cl
       @return
    */
    Status getObjectClass(const Oid &oid, Class *&cl);

    /**
       Not yet documented
       @param oid
       @param cls_oid
       @return
    */
    Status getObjectClass(const Oid &oid, Oid &cls_oid);

    // object locking
    /**
       Not yet documented
       @param oid
       @param lockmode
       @return
    */
    Status setObjectLock(const Oid &oid, LockMode lockmode);

    /**
       Not yet documented
       @param oid
       @param lockmode
       @param alockmode
       @return
    */
    Status setObjectLock(const Oid &oid, LockMode lockmode,
			 LockMode &alockmode);

    /**
       Not yet documented
       @param oid
       @param alockmode
       @return
    */
    Status getObjectLock(const Oid &oid, LockMode &alockmode);

    // object loading
    /**
       Not yet documented
       @param oid
       @param o
       @param recmode
       @return
    */
    Status loadObject(const Oid &oid, ObjectPtr &o,
		      const RecMode *recmode = RecMode::NoRecurs);

    // object loading
    /**
       Not yet documented
       @param oid
       @param o
       @param recmode
       @return
    */
    Status loadObject(const Oid &oid, Object *&o,
		      const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param xoid
       @param o
       @param lockmode
       @param recmode
       @return
    */
    Status loadObject(const Oid &xoid, ObjectPtr &o,
		      LockMode lockmode,
		      const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param xoid
       @param o
       @param lockmode
       @param recmode
       @return
    */
    Status loadObject(const Oid &xoid, Object *&o,
		      LockMode lockmode,
		      const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid
       @param o
       @param recmode
       @return
    */
    Status reloadObject(const Oid &oid, ObjectPtr &o,
			const RecMode *recmode = RecMode::NoRecurs);


    /**
       Not yet documented
       @param oid
       @param o
       @param recmode
       @return
    */
    Status reloadObject(const Oid &oid, Object *&o,
			const RecMode *recmode = RecMode::NoRecurs);


    /**
       Not yet documented
       @param oid
       @param o
       @param lockmode
       @param recmode
       @return
    */
    Status reloadObject(const Oid &oid, ObjectPtr &o,
			LockMode lockmode,
			const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid
       @param o
       @param lockmode
       @param recmode
       @return
    */
    Status reloadObject(const Oid &oid, Object *&o,
			LockMode lockmode,
			const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @param obj_vect
       @param recmode
       @return
    */
    Status loadObjects(const OidArray &oid_array, ObjectPtrVector &obj_vect,
		       const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @param obj_array
       @param recmode
       @return
    */
    Status loadObjects(const OidArray &oid_array, ObjectArray &obj_array,
		       const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @param obj_vect
       @param lockmode
       @param recmode
       @return
    */
    Status loadObjects(const OidArray &oid_array, ObjectPtrVector &obj_vect,
		       LockMode lockmode,
		       const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @param obj_array
       @param lockmode
       @param recmode
       @return
    */
    Status loadObjects(const OidArray &oid_array, ObjectArray &obj_array,
		       LockMode lockmode,
		       const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param poid
       @param recmode
       @return
    */
    Status removeObject(const Oid &poid, const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param poid
       @param isremoved
       @return
    */
    Status isRemoved(const Oid &poid, Bool &isremoved) const;

    /**
       Not yet documented
       @param poid
       @param hdr
       @param idr
       @param o
       @param useCache
       @return
    */
    Status makeObject(const Oid *poid, const ObjectHeader *hdr,
		      Data idr, Object **o, Bool useCache = False);

    Status makeObject_realize(const Oid *, const ObjectHeader *,
			      Data, Object **,
			      Bool useCache = False);

    /**
       Not yet documented
       @param obj_oid
       @param prot_oid
       @return
    */
    Status setObjectProtection(const Oid &obj_oid, const Oid &prot_oid);

    /**
       Not yet documented
       @param obj_oid
       @param prot
       @return
    */
    Status setObjectProtection(const Oid &obj_oid, Protection *prot);

    /**
       Not yet documented
       @param obj_oid
       @param prot_oid
       @return
    */
    Status getObjectProtection(const Oid &obj_oid, Oid &prot_oid);

    /**
       Not yet documented
       @param obj_oid
       @param prot
       @return
    */
    Status getObjectProtection(const Oid &obj_oid, Protection *&prot);

    // location management
    /**
       Not yet documented
       @param oid_arr
       @param locarr
       @return
    */
    Status getObjectLocations(const OidArray &oid_arr, ObjectLocationArray &locarr);

    /**
       Not yet documented
       @param obj_arr
       @param locarr
       @return
    */
    Status getObjectLocations(const ObjectArray &obj_arr, ObjectLocationArray &locarr);

    /**
       Not yet documented
       @param oid_arr
       @param dataspace
       @return
    */
    Status moveObjects(const OidArray &oid_arr, const Dataspace *dataspace);

    /**
       Not yet documented
       @param obj_arr
       @param dataspace
       @return
    */
    Status moveObjects(const ObjectArray &obj_arr, const Dataspace *dataspace);

    // transaction support
  
    /**
       Not yet documented
       @return
    */
    Status transactionBegin();

    /**
       Not yet documented
       @param params
       @return
    */
    Status transactionBegin(const TransactionParams &params);

    /**
       Not yet documented
       @return
    */
    Status transactionBeginExclusive();

    /**
       Not yet documented
       @return
    */
    Status transactionCommit();

    /**
       Not yet documented
       @return
    */
    Status transactionAbort();

    /**
       Not yet documented
       @return
    */
    Transaction *getCurrentTransaction();

    /**
       Not yet documented
       @return
    */
    Transaction *getRootTransaction();

    /**
       Not yet documented
       @return
    */
    Bool isInTransaction() const;

    /**
       Not yet documented
       @param params
       @return
    */
    Status setDefaultTransactionParams(const TransactionParams &params);

    /**
       Not yet documented
       @return
    */
    TransactionParams getDefaultTransactionParams();

    /**
       Not yet documented
       @param commit_on_close
    */
    void setCommitOnClose(Bool commit_on_close);

    /**
       Not yet documented
       @return
    */
    Bool getCommitOnClose() const {return commit_on_close;}

    /**
       Not yet documented
       @param _def_commit_on_close
    */
    static void setDefaultCommitOnClose(Bool _def_commit_on_close) {
      def_commit_on_close = _def_commit_on_close;
    }    

    /**
       Not yet documented
       @return
    */
    static Bool getDefaultCommitOnClose() {return def_commit_on_close;}
  
    /**
       Not yet documented
       @return
    */
    const char *getName() const;

    /**
       Not yet documented
       @return
    */
    int getDbid() const;

    /**
       Not yet documented
       @return
    */
    int getVersionNumber() const {return version;}

    /**
       Not yet documented
       @return
    */
    const char *getVersion() const;

    /**
       Not yet documented
       @return
    */
    Bool isOpened() const;

    /**
       Not yet documented
       @return
    */
    Database::OpenFlag getOpenFlag() const;

    /**
       Not yet documented
       @return
    */
    DbHandle *getDbHandle();

    /**
       Not yet documented
       @return
    */
    const Schema *getSchema() const;

    /**
       Not yet documented
       @return
    */
    Schema *getSchema();

    /**
       Not yet documented
       @return
    */
    Connection *getConnection();

    /**
       Not yet documented
       @param dbmdb_str
    */
    static void setDefaultDBMDB(const char *dbmdb_str);

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultDBMDB();

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultServerDBMDB();

    /**
       Not yet documented
       @return
    */
    static const std::vector<std::string> &getGrantedDBMDB();

    /**
       Not yet documented
       @return
    */
    const char *getDBMDB() const;

    /**
       Not yet documented
       @return
    */
    Bool isBackEnd() const;

    /**
       Not yet documented
       @return
    */
    Bool isLocal() const;

    /**
       Not yet documented
       @return
    */
    const char *getUser() const {return _user;}

    /**
       Not yet documented
       @return
    */
    const char *getPassword() const {return _passwd;}

    /**
       Not yet documented
       @return
    */
    int getUid() const {return uid;}
  
    // datafile management
    /**
       Not yet documented
       @param database_file
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDatabasefile(const char *&database_file,
			   Bool fetch = False,
			   const char *user = 0,
			   const char *passwd = 0);

    /**
       Not yet documented
       @param datafiles
       @param cnt
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDatafiles(const Datafile **&datafiles, unsigned int &cnt,
			Bool fetch = False,
			const char *user = 0,
			const char *passwd = 0);

    /**
       Not yet documented
       @param id
       @param datafile
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDatafile(unsigned short id, const Datafile *&datafile,
		       Bool fetch = False,
		       const char *user = 0,
		       const char *passwd = 0);

    /**
       Not yet documented
       @param name_or_file
       @param datafile
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDatafile(const char *name_or_file, const Datafile *&datafile,
		       Bool fetch = False,
		       const char *user = 0,
		       const char *passwd = 0);

    /**
       Not yet documented
       @param filedir
       @param filename
       @param name
       @param maxsize
       @param slotsize
       @param dtype
       @return
    */
    Status createDatafile(const char *filedir, const char *filename,
			  const char *name, unsigned int maxsize,
			  unsigned int slotsize, DatType dtype);

    // dataspace management
    /**
       Not yet documented
       @param dataspace
       @param cnt
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDataspaces(const Dataspace **&dataspace, unsigned int &cnt,
			 Bool fetch = False,
			 const char *user = 0,
			 const char *passwd = 0);

    /**
       Not yet documented
       @param id
       @param dataspace
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDataspace(unsigned short id, const Dataspace *&dataspace,
			Bool fetch = False,
			const char *user = 0,
			const char *passwd = 0);

    /**
       Not yet documented
       @param name
       @param dataspace
       @param fetch
       @param user
       @param passwd
       @return
    */
    Status getDataspace(const char *name, const Dataspace *&dataspace,
			Bool fetch = False,
			const char *user = 0,
			const char *passwd = 0);

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status getDefaultDataspace(const Dataspace *&dataspace);

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status setDefaultDataspace(const Dataspace *dataspace);

    /**
       Not yet documented
       @param dspname
       @param datafiles
       @param datafile_cnt
       @return
    */
    Status createDataspace(const char *dspname, const Datafile **datafiles,
			   unsigned int datafile_cnt);

    const DBProperty *getProperty(const char *) const;
    DBProperty *getProperty(const char *);
    const DBProperty **getProperties(int &prop_cnt) const;
    DBProperty **getProperties(int &prop_cnt);

    /**
       Not yet documented
       @param open_flag
       @return
    */
    static const char *getStringFlag(Database::OpenFlag open_flag);

    /**
       Not yet documented
       @return
    */
    virtual Database *asDatabase() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Database *asDatabase() const {return this;}

    /**
       Not yet documented
    */
    virtual ~Database();

    /**
       Not yet documented
       @param xoid
       @return
    */
    Object *getCacheObject(const Oid &xoid);

    // auto register
    /**
       Not yet documented
       @param on
    */
    void autoRegisterObjects(Bool on);

    /**
       Not yet documented
       @return
    */
    Bool areObjectsAutoRegistered() const {return auto_register_on;}

    /**
       Not yet documented
       @param o
       @param force
    */
    void addToRegister(const Object *o, Bool force = False);

    /**
       Not yet documented
       @param o
    */
    void rmvFromRegister(const Object *o);

    /**
       Not yet documented
       @return
    */
    ObjectList *getRegisteredObjects();

    /**
       Not yet documented
       @return
    */
    Status storeRegisteredObjects();

    /**
       Not yet documented
    */
    void clearRegister();

    /**
       Not yet documented
       @param on
    */
    void storeOnCommit(Bool on);

    /**
       Not yet documented
       @return
    */
    Bool isStoreOnCommit() const {return store_on_commit;}

    /**
       Not yet documented
       @param max_obj_cnt
    */
    Status setMaxObjectCount(unsigned int max_obj_cnt);

    /**
       Not yet documented
       @param max_obj_cnt
    */
    Status getMaxObjectCount(unsigned int& max_obj_cnt);

    /**
       Not yet documented
       @param logsize
    */
    Status setLogSize(unsigned int logsize);

    /**
       Not yet documented
       @param logsize
    */
    Status getLogSize(unsigned int& logsize);

    // deprecated
    Status loadObject(const Oid *, Object **,
		      const RecMode * = RecMode::NoRecurs);

    Status loadObject(const Oid *, Object **,
		      LockMode lockmode,
		      const RecMode * = RecMode::NoRecurs);

    Status reloadObject(const Oid *, Object **,
			const RecMode * = RecMode::NoRecurs);

    Status reloadObject(const Oid *, Object **,
			LockMode lockmode,
			const RecMode * = RecMode::NoRecurs);

    Status removeObject(const Oid *,
			const RecMode * = RecMode::NoRecurs);

    Status updateSchema(const char *odlfile, const char *package,
			const char *schname = 0, const char *db_prefix = 0,
			FILE *fd = stdout, const char *cpp_cmd = 0,
			const char *cpp_flags = 0);

    // not implemented (yet ?)
    /*
    Status execOQL(Object *&, const char *fmt, ...);
    Status execOQL(Object *&, const RecMode *,
		   const char *fmt, ...);
    Status execOQL(ObjectArray &, const char *fmt, ...);
    Status execOQL(ObjectArray &, const RecMode *,
		   const char *fmt, ...);
    Status execOQL(Oid &, const char *fmt, ...);
    Status execOQL(OidArray &, const char *fmt, ...);
    Status execOQL(Value &, const char *fmt, ...);
    Status execOQL(ValueArray &, const char *fmt, ...);
    */

    static void operator delete(void *);

    // ----------------------------------------------------------------------
    // Database Protected Part
    // ----------------------------------------------------------------------
  protected:
    char *name;
    int dbid;
    unsigned int version;
    Connection *conn;
    OpenFlag open_flag;
    DbHandle *dbh;
    Schema *sch;
    ObjCache *temp_cache;
    ObjCache *obj_register;
    Bool auto_register_on;
    Bool store_on_commit;
    Bool open_state;
    int open_refcnt;
    BEQueue *bequeue;
    Bool is_back_end;
    static char *defaultDBMDB;
    static LinkedList *dbopen_list;
    char *_user, *_passwd;
    int uid;
    Transaction *curtrs, *roottrs;
    Bool commit_on_close_set;
    Bool commit_on_close;
    static Bool def_commit_on_close;
    Oid m_protoid;
    Bool oqlInit;

  public:
    typedef Object *(*consapp_t)(const Class *, Data);

  protected:
    int consapp_cnt;
    GenHashTable *hashapp[4];
    consapp_t *consapp[4];

    Status init_db(Connection *);
    Bool initialized;
    Status create_prologue(DbCreateDescription &,
			   DbCreateDescription **);
    Status transactionBegin_realize(const TransactionParams *params);
    Status transactionCommit_realize();
    Status transactionAbort_realize();
    void init(const char *);
    static Status open_realize(Connection *,
			       Bool (*)(const Database *, const void *),
			       const void *, const char *, OpenFlag,
			       const OpenHints *,
			       Database *(*)(const void *, const char *),
			       const char *, const char *,
			       Database **);
    const char *getTName() const;

    TransactionParams def_params;

    char *dbmdb_str;
    Status invalidDbmdb(Error) const;

    // ----------------------------------------------------------------------
    // Database Private Part
    // ----------------------------------------------------------------------
  private:
    void init_open(Connection *conn,
		   const char *dbname,
		   const char *dbmdb_str,
		   Database::OpenFlag flag,
		   const char *user,
		   const char *passwd);

    Status create();
    Status update();
    Status realize(const RecMode * = RecMode::NoRecurs);
    Status remove(const RecMode * = RecMode::NoRecurs);
    int trs_cnt;
    LinkedList mark_deleted;
    LinkedList purge_action_list;
    std::map<Oid, bool> mark_created;
    void purgeOnAbort();
    void purgeRelease();
    static ObjCache *makeRegister();
    ObjCache conv_cache;

    char *database_file;
    unsigned int datafile_cnt;
    Datafile **datafiles;
    unsigned int dataspace_cnt;
    Dataspace **dataspaces;
    std::map<Object *, bool> realizedMap;
    bool useMap;
    void make_dat_dsp(const DbCreateDescription &dbdesc);
    void garbage_dat_dsp();
    const Datafile **get_datafiles(const eyedbsm::Dataspace *dsp);
    Status getDatDspPrologue(Bool fetch,
			     const char *user, const char *passwd);

    // ----------------------------------------------------------------------
    // Database Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init();
    static void _release();
    Status set(ConnHandle *, int, int, DbHandle *,
	       rpcDB_LocalDBContext *, const Oid *, unsigned int);

    void beginRealize();
    bool isRealized(Object *o) const {
      return useMap && realizedMap.find(o) != realizedMap.end();
    }
    void setRealized(Object *o) {if (useMap) {realizedMap[o] = true;}}
    void endRealize();

    void cacheObject(Object *);
    void uncacheObject(Object *);
    void uncacheObject(const Oid &_oid);

    void insertTempCache(const Oid&, void *);
    Bool isOpeningState();
    BEQueue *getBEQueue();
    virtual void garbage();

    void setSchema(Schema *);

    static Status getOpenedDB(int dbid, Database *, Database *&);

    static Status open(Connection *, int,
		       const char *, const char *, const char *,
		       OpenFlag, const OpenHints *, Database **);

    static Status open(Connection *, const char *,
		       const char *, const char *, const char *,
		       OpenFlag, const OpenHints *, Database **);

    virtual Status loadObject_realize(const Oid *, Object **,
				      LockMode lockmode,
				      const RecMode * =
				      RecMode::NoRecurs,
				      Bool reload = False);
    Bool writeBackConvertedObjects() const;
    void addPurgeActionOnAbort(void (*purge_action)(void *), void *);
    ExecutableCache *exec_cache;
    void *trig_dl;
    const Oid& getMprotOid() const {return m_protoid;}
    void updateSchema(const SchemaInfo &schinfo);
    LinkedList& getMarkDeleted() {return mark_deleted;}
    void addMarkCreated(const Oid &);
    bool isMarkCreated(const Oid &) const;
    void markCreatedEmpty();
    void setIncoherency();
    Bool isOQLInit() const {return oqlInit;}
    void setOQLInit() {oqlInit = True;}
    Bool isApp() const {return IDBBOOL(hashapp);}
    void add(GenHashTable *, consapp_t *);
    consapp_t getConsApp(const Class *);
    ObjCache &getConvCache() {return conv_cache;}
  };

  /**
     @}
  */

}

#endif

  
