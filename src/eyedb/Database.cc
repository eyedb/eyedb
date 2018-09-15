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
#include "eyedb_p.h"
#include "eyedb/DBM_Database.h"
#include "IteratorBE.h"
#include "OQLBE.h"
#include "BEQueue.h"
//#define FRONT_END
#include "kernel.h"
#include <eyedblib/butils.h>
#include "db_p.h"
#include "ExecutableCache.h"
#include "oqlctb.h"
#include <eyedb/GenHashTable.h>
#include "version_p.h"

#include <map>

#define FULL_CACHE_OBJECTS
#define CACHE_OBJECTS

//#define STRICT_NOT_NESTED_TRANSACTIONS

namespace eyedb {

  //static std::map<Database *, bool> db_list;

  LinkedList *Database::dbopen_list;

  //TransactionMode Database::global_def_trmod = idbReadWriteSharedTRMode;
  // changed the 19/10/99

  Bool Database::def_commit_on_close = False;

  Bool edb_is_back_end = False;
  //static char *default_voldir;

#if 0
  static char *
  getPath(char *path, Bool inplace = False)
  {
    if (!path)
      return 0;

    if (*path == '/')
      return path;

    static char dirname[256], name[256];

    if (!*dirname) {
      const char *voldir = default_voldir;

      if (!voldir)
	voldir = ClientConfig::getCValue("databasedir");

      if (voldir)
	strcpy(dirname, voldir);
      else
	getcwd(dirname, sizeof(dirname)-1);
    }

    sprintf(name, "%s/%s", dirname, path);

    if (inplace) {
      strcpy(path, name);
      return path;
    }
    else {
      delete path;
      return strdup(name);
    }
  }
#endif

  /*
    void Database::setDefaultVolumeDirectory(const char *voldir)
    {
    if (voldir)
    default_voldir = strdup(voldir);
    }
  */

  void Database::init(const char *_dbmdb_str)
  {
    //  printf("Database::Database(this=%p)\n", this);
    //db_list[this] = true;

    if (_dbmdb_str)
      dbmdb_str = strdup(_dbmdb_str);
    else {
      const char *str = getDefaultDBMDB();
      if (str)
	dbmdb_str = strdup(str);
      else
	dbmdb_str = 0;
    }

    name = 0;
    dbid = 0;
    uid = 0;
    m_protoid.invalidate();

    conn = (Connection *)0;
    dbh = (DbHandle *)0;
    open_refcnt = 0;
    temp_cache = new ObjCache(64);
    curtrs = 0;
    roottrs = 0;
    commit_on_close = False;
    commit_on_close_set = False;
    trs_cnt = 0;
    exec_cache = new ExecutableCache();
    trig_dl = 0;
    oqlInit = False;
    obj_register = 0;
    auto_register_on = False;
    store_on_commit = True;

    sch = 0;
    open_state = False;
    open_flag = (OpenFlag)0;
    is_back_end = edb_is_back_end;
    bequeue = new BEQueue;

    def_params = TransactionParams::getGlobalDefaultTransactionParams();
    version = 0;

    database_file = 0;
    datafile_cnt = 0;
    datafiles = 0;

    dataspace_cnt = 0;
    dataspaces = 0;

    useMap = false;

    _user   = (char *)0;
    _passwd = (char *)0;
    consapp_cnt = 0;

    setClass(Object_Class);
    sysclsDatabase::setConsApp(this);
    oqlctbDatabase::setConsApp(this);
    utilsDatabase::setConsApp(this);
  }

  Status
  Database::setDefaultTransactionParams(const TransactionParams &params)
  {
    Status s;

    if (s = Transaction::checkParams(params, False))
      return s;

    def_params = params;
    return Success;
  }

  TransactionParams
  Database::getDefaultTransactionParams()
  {
    return def_params;
  }

  Database::Database(const char *s, const char * _dbmdb_str) : Struct()
  {
    init(_dbmdb_str);
    name = strdup(s);
  }

  Database::Database(const char *s, int _dbid, const char * _dbmdb_str) : Struct()
  {
    init(_dbmdb_str);
    name = strdup(s);
    dbid = _dbid;
  }

  Database::Database(Connection *conn,
		     const char *dbname,
		     Database::OpenFlag flag,
		     const char *user,
		     const char *passwd)
  {
    init_open(conn, dbname, 0, flag, user, passwd);
  }

  Database::Database(Connection *conn,
		     const char *dbname,
		     const char *_dbmdb_str,
		     Database::OpenFlag flag,
		     const char *user,
		     const char *passwd)
  {
    init_open(conn, dbname, _dbmdb_str, flag, user, passwd);
  }

  void Database::init_open(Connection *conn,
			   const char *dbname,
			   const char *_dbmdb_str,
			   Database::OpenFlag flag,
			   const char *user,
			   const char *passwd)
  {
    init(_dbmdb_str);
    name = strdup(dbname);
    Status status = open(conn, flag, user, passwd);
    if (status)
      throw *status;
  }

  Database::Database(const Database &_db) : Struct(_db)
  {
    int r = _db.open_refcnt + 1;
    init(_db.dbmdb_str);
    name        = strdup(_db.name);
    dbid        = _db.dbid;
    conn        = _db.conn;
    sch         = _db.sch;
    open_flag   = _db.open_flag;
    dbh         = _db.dbh;
    is_back_end = _db.is_back_end;
    open_refcnt = r;
    version     = _db.version;
  }

  Database::Database(int _dbid, const char *_dbmdb_str) : Struct()
  {
    init(_dbmdb_str);
    dbid = _dbid;
  }

  char *Database::defaultDBMDB;

  const std::vector<std::string> & Database::getGrantedDBMDB()
  {
    static std::vector<std::string> granted_dbm;

    if (granted_dbm.size())
      return granted_dbm;

    const char *path = ServerConfig::getSValue("granted_dbm");
    if (!path) {
      const char *default_dbm = getDefaultServerDBMDB();
      if (default_dbm)
	granted_dbm.push_back(default_dbm);
      return granted_dbm;
    }

    char *p = strdup(path);
    char *sp = p;

    for (;;) {
      char *q = strchr(p, ',');
      if (q)
	*q = 0;
      granted_dbm.push_back(p);
      if (!q)
	break;
      p = q + 1;
    }

    free(sp);
    return granted_dbm;
  }

  const char *Database::getDefaultServerDBMDB()
  {
    return ServerConfig::getSValue("default_dbm");
    /*
      const std::vector<std::string> & granted_dbm = getGrantedDBMDB();
      if (granted_dbm.size())
      return granted_dbm[0].c_str();

      return "";
    */
  }

  const char *Database::getDefaultDBMDB()
  {
    if (!defaultDBMDB) {
      static char buff[256];
      const char *path = ClientConfig::getCValue("dbm");
      if (!path)
	return DBM_Database::defaultDBMDB;
      strcpy(buff, path);
      return buff;
    }

    return defaultDBMDB;
  }

  void Database::setDefaultDBMDB(const char *dbmdb_str)
  {
    free(defaultDBMDB);
    defaultDBMDB = strdup(dbmdb_str);
  }

  void Database::garbage()
  {
    //db_list.erase(db_list.find(this));
    //printf("Database::garbage(this=%p, %p, oql_info = %p)\n", this, dbh, oql_info);
    if (dbh)
      close();

    Exception::Mode mode = Exception::setMode(Exception::StatusMode);
    void (*handler)(Status, void *) = Exception::getHandler();
    Exception::setHandler(NULL);

    if (dbh)
      (void)close();
    Exception::setHandler(handler);
    Exception::setMode(mode);
    free(name);
    free(_user);
    free(_passwd);
    free(dbmdb_str);
    delete temp_cache;
    delete exec_cache;
    delete bequeue;

    if (sch)
      {
	sch->release();
	sch = NULL;
      }

    garbage_dat_dsp();
    Struct::garbage();
    delete obj_register;
  }

  Database::~Database()
  {
    garbageRealize();
  }

  const char *Database::getName(void) const
  {
    return name;
  }

  int Database::getDbid(void) const
  {
    return dbid;
  }

  Connection *Database::getConnection()
  {
    return conn;
  }

  BEQueue *Database::getBEQueue()
  {
    return bequeue;
  }

  void Database::setCommitOnClose(Bool _commit_on_close)
  {
    commit_on_close = _commit_on_close;
    commit_on_close_set = True;
  }

  Status Database::close(void)
  {
    RPCStatus status;
    if (dbh) {
      open_refcnt--;
      if (open_refcnt > 0) {
	ObjectPeer::decrRefCount(this);
	return Success;
      }

      Bool commit = (commit_on_close_set ? commit_on_close :
		     def_commit_on_close);
      if (curtrs) {
	RPCStatus rpc_status;
	if ((rpc_status = (commit ? eyedb::transactionCommit(dbh, 0 ) :
			   eyedb::transactionAbort(dbh, 0))) != RPCSuccess) {
	  delete curtrs;
	  curtrs = 0;
	}
      }

      if (isBackEnd())
	status = IDB_dbClose((DbHandle *)dbh->u.dbh);
      else
	status = dbClose(dbh);

      if (status == RPCSuccess) {
	free(dbh);
	dbh = 0;
	/*
	  sch->release();
	  sch = 0;
	*/
	dbid = 0;

	dbopen_list->deleteObject(this);

	return Success;
      }
      else
	return StatusMake(status);
    }

    return Exception::make(IDB_DATABASE_CLOSE_ERROR, "database '%s' is not opened", getTName());
  }

  Status Database::init_db(Connection *ch)
  {
    sch = new Schema();
    eyedb_CHECK_RETURN(open(ch, DBRW, _user, _passwd));

    Status status = sch->init(this, True);

    if (status) {
      close();
      return status;
    }
    else {
      sch->setDatabase(this);
      return close();
    }
  }

  const char *
  Database::getTName() const
  {
    static char buff[16];

    if (name)
      return name;

    sprintf(buff, "#%d", dbid);
    return buff;
  }

  Status
  Database::invalidDbmdb(Error err) const
  {
    return Exception::make(err,
			   "dbmfile is not set for Database.handle '%s': check your `dbm' configuration variable or use `-eyedbdbm' command line option",
			   getTName());
    /* , or use argument #2 of Database constructor */
  }

  Status Database::create(Connection *ch,
			  DbCreateDescription *pdbdesc)
  {
    return create(ch, (char *)0, (char *)0, pdbdesc);
  }

  Status Database::create_prologue(DbCreateDescription &dbdesc,
				   DbCreateDescription **pdbdesc)
  {
    if (!*pdbdesc) {
      eyedb_clear(dbdesc);

      eyedbsm::DbCreateDescription *d = &dbdesc.sedbdesc;
      if (!strcmp(name, DBM_Database::getDbName())) {
	strcpy(dbdesc.dbfile, dbmdb_str);
	char *q = strdup(dbmdb_str), *p;

	if ((p = strrchr(q, '.')) && !strchr(p, '/'))
	  *p = 0;

	sprintf(d->dat[0].file, "%s.dat", q);
	delete q;
      }
      else {
	sprintf(dbdesc.dbfile, "%s.dbs", name);
	sprintf(d->dat[0].file, "%s.dat", name);
      }

      d->dat[0].mtype    = eyedbsm::BitmapType;
      d->dat[0].sizeslot = 32;
      d->dbid     = 0;
      d->ndat     = 1;
      d->dat[0].maxsize = 2000000;
      *pdbdesc = &dbdesc;
    }
    else if (!strcmp(name, DBM_Database::getDbName()) &&
	     strcmp((*pdbdesc)->dbfile, dbmdb_str))
      return Exception::make(IDB_DATABASE_CREATE_ERROR,
			     "when creating a dbmfile, dbfile must be equal to dbmdb_str ('%s' != '%s')", (*pdbdesc)->dbfile, dbmdb_str);

    //getPath(dbdesc.dbfile, True);

    eyedbsm::DbCreateDescription *d = &(*pdbdesc)->sedbdesc;

    return Success;
  }

  Status Database::create(Connection *ch, const char *user,
			  const char *passwd,
			  DbCreateDescription *pdbdesc)
  {
    RPCStatus rpc_status;
    DbCreateDescription dbdesc;

    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_CREATE_ERROR);

    if (!strcmp(name, DBM_Database::getDbName()))
      return Exception::make(IDB_DATABASE_CREATE_ERROR, "must use a DBM_Database object to create a DBM database\n");

    check_auth(user, passwd, "creating database");
    set_auth(user, passwd);

    create_prologue(dbdesc, &pdbdesc);

    rpc_status = dbCreate(ConnectionPeer::getConnH(ch), dbmdb_str,
			  user, passwd, name,
			  pdbdesc);

    if (rpc_status == RPCSuccess) {
      conn = ch;
      return init_db(ch);
    }
    else
      return StatusMake(rpc_status);
  }

  static Status
  remove_db_realize(ConnHandle *connh, const char *dbmdb_str,
		    const char *user, const char *passwd,
		    const char *name)
  {
    if (!connh)
      return Exception::make(IDB_DATABASE_REMOVE_ERROR, "connection is not set");

    RPCStatus status;

    check_auth_st(user, passwd, "deleting database", name);

    status = dbDelete(connh, dbmdb_str, user, passwd, name);
    if (status == RPCSuccess)
      return Success;
    else
      return StatusMake(status);
  }


  Status Database::remove(Connection *ch, const char *user,
			  const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_REMOVE_ERROR);

    check_auth(user, passwd, "removing database");

    return remove_db_realize(ConnectionPeer::getConnH(ch), dbmdb_str,
			     user, passwd, name);
  }

  Status Database::remove(const char *user, const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_REMOVE_ERROR);
  
    check_auth(user, passwd, "removing database");
  
    return remove_db_realize(ConnectionPeer::getConnH(conn), dbmdb_str,
			     user, passwd, name);
  }

#define check_dbaccess(s, mode)  \
if ((mode) !=  NoDBAccessMode && \
    (mode) !=  ReadDBAccessMode && \
    (mode) !=  ReadWriteDBAccessMode && \
    (mode) !=  ReadWriteExecDBAccessMode && \
    (mode) !=  ReadExecDBAccessMode && \
    (mode) !=  AdminDBAccessMode) \
  return Exception::make(s, "invalid database access mode 0x%x", (mode))

  Status Database::setUserDBAccess(Connection *ch, const char *username,
				   int mode,
				   const char *user,
				   const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_SET_USER_DBACCESS_ERROR);

    check_auth(user, passwd, "setting user dbaccess");

    check_dbaccess(IDB_SET_USER_DBACCESS_ERROR, mode);

    conn = ch;
    RPCStatus rpc_status;
    rpc_status = userDBAccessSet(ConnectionPeer::getConnH(conn),
				 dbmdb_str, user, passwd,
				 name, username, mode);
    return StatusMake(rpc_status);
  }

  Status Database::setDefaultDBAccess(Connection *ch,
				      int mode,
				      const char *user,
				      const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_SET_USER_DBACCESS_ERROR);

    check_auth(user, passwd, "setting default dbacess");

    check_dbaccess(IDB_SET_DEFAULT_DBACCESS_ERROR, mode);

    conn = ch;
    RPCStatus rpc_status;
    rpc_status = defaultDBAccessSet(ConnectionPeer::getConnH(conn),
				    dbmdb_str, user, passwd,
				    name, mode);
    return StatusMake(rpc_status);
  }

  Status Database::getInfo(const char *user, const char *passwd,
			   DbInfoDescription *pdbdesc) const
  {
    return getInfo(conn, user, passwd, pdbdesc);
  }

  Status Database::getInfo(Connection *ch, 
			   const char *user, const char *passwd,
			   DbInfoDescription *pdbdesc) const
  {
    if (!ch)
      return Exception::make(IDB_ERROR, "invalid null connection");

    check_auth_st(user, passwd, "get info database", name);

    return StatusMake(dbInfo(ConnectionPeer::getConnH(ch), dbmdb_str,
			     user, passwd,
			     name, 0, pdbdesc));
  }

  /* static */
  void Database::init(void)
  {
    dbopen_list = new LinkedList;
  }

  void Database::_release(void)
  {
    /*
    std::map<Database *, bool>::iterator begin = db_list.begin();
    std::map<Database *, bool>::iterator end = db_list.end();
    
    printf("oqml_release: %d\n", db_list.size());
    while (begin != end) {
      printf("release: %p %d\n", (*begin).first, (*begin).first->isValidObject());
      (*begin).first->release();
      ++begin;
    }

    db_list.clear();
    */
  }

  static Bool
  cmp_dbid(const Database *db, const void *xdbid)
  {
    int dbid = *(const int *)xdbid;
    return (db->getDbid() == dbid ? True : False);
  }

  static Bool
  cmp_dbname(const Database *db, const void *xdbname)
  {
    if (!xdbname || !db->getName())
      return False;
    return (!strcmp(db->getName(), (const char *)xdbname) ? True : False);
  }

  static Database *
  dbid_make_db(const void *xdbid, const char *dbmdb_str)
  {
    return new Database(*(const int *)xdbid, dbmdb_str);
  }

  static Database *
  dbname_make_db(const void *xdbname, const char *dbmdb_str)
  {
    return new Database((const char *)xdbname, dbmdb_str);
  }

  Status
  Database::getOpenedDB(int dbid, Database *dbref, Database *&pdb)
  {
    LinkedListCursor c(dbopen_list);
    Database *db;
    const char *dbmdb = dbref->getDBMDB();

    pdb = 0;
    while (c.getNext((void *&)db))
      if (!strcmp(db->getDBMDB(), dbref->getDBMDB()) && db->getDbid() == dbid) {
	if (pdb)
	  return Exception::make
	    (IDB_ERROR,
	     "several opened databases with dbid #%d are opened: "
	     "cannot choose", dbid);
	pdb = db;
      }

    if (pdb) {
      pdb->open_refcnt++; // preventing closing
      ObjectPeer::incrRefCount(pdb);    // preventing deleting
    }

    return Success;
  }

  Status
  Database::open(Connection *conn, int dbid,
		 const char *dbmdb_str,
		 const char *user, const char *passwd,
		 OpenFlag flag, const OpenHints *oh, Database **pdb)
  {
    return open_realize(conn, cmp_dbid, &dbid, dbmdb_str, flag, oh,
			dbid_make_db, user, passwd, pdb);
  }

  Status
  Database::open(Connection *conn, const char *dbname,
		 const char *dbmdb_str,
		 const char *user, const char *passwd,
		 OpenFlag flag, const OpenHints *oh, Database **pdb)
  {
    return open_realize(conn, cmp_dbname, dbname, dbmdb_str, flag, oh,
			dbname_make_db, user, passwd, pdb);
  }

  Status
  Database::open_realize(Connection *conn,
			 Bool (*cmp)(const Database *, const void *),
			 const void *user_data,
			 const char *dbmdb_str, OpenFlag flag,
			 const OpenHints *oh,
			 Database *(*make_db)(const void *, const char *),
			 const char *user,
			 const char *passwd,
			 Database **pdb)
  {
    LinkedListCursor c(dbopen_list);

    Database *db;

    /* LOCAL_REMOTE */
    OpenFlag oflag = flag;
    flag = (OpenFlag)(flag & ~_DBOpenLocal);

    while (c.getNext((void *&)db)) {
      // 26/08/02: changed : ``db->getConnection() == conn'' to :
      if (db->getConnection()->getConnHandle() == conn->getConnHandle() &&
	  (*cmp)(db, user_data) && flag == (db->open_flag & ~_DBOpenLocal)) {
	// check authentication
	if ((!user && !passwd && !db->_user && !db->_passwd) ||
	    (user && passwd && db->_user && db->_passwd &&
	     !strcmp(db->_user, user) &&
	     !strcmp(db->_passwd, passwd))) {
	  db->open_refcnt++; // preventing closing
	  ObjectPeer::incrRefCount(db);    // preventing deleting
	  *pdb = db;
	  return Success;
	}
      }
    }

    db = make_db(user_data, dbmdb_str);

    /* LOCAL_REMOTE */
    Status status = db->open(conn, oflag, oh, user, passwd);
    *pdb = !status ? db : 0;
    return status;
  }

  static Status
  check_open_flag(Database::OpenFlag flag)
  {
    return (flag == Database::DBRead ||
	    flag == Database::DBRW ||
	    flag == Database::DBReadLocal ||
	    flag == Database::DBRWLocal ||
	    flag == Database::DBReadAdmin ||
	    flag == Database::DBRWAdmin ||
	    flag == Database::DBReadAdminLocal ||
	    flag == Database::DBRWAdminLocal ||
	    flag == Database::DBSRead ||
	    flag == Database::DBSReadLocal ||
	    flag == Database::DBSReadAdmin) ?
      Success : Exception::make(IDB_INVALID_DBOPEN_FLAG,
				"opening flag %s",
				Database::getStringFlag(flag));
  }

  Status
  Database::open(Connection *ch, OpenFlag flag,
		 const char *user, const char *passwd)
  {
    return open(ch, flag, 0, user, passwd);
  }

  Status
  Database::open(Connection *ch, OpenFlag flag, const OpenHints *oh,
		 const char *user, const char *passwd)
  {
    if (dbh)
      return Exception::make(IDB_DATABASE_OPEN_ERROR,
			     "database '%s' already opened",
			     getTName());

    //eyedblib::display_time("%s %s", name, "::open #1");

    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_OPEN_ERROR);

    check_auth(user, passwd, "opening database");

    Status status;
    if (status = check_open_flag(flag))
      return status;

    set_auth(user, passwd);

    is_back_end = edb_is_back_end;

    //eyedblib::display_time("%s %s", name, "::open #2");
    RPCStatus rpc_status;

    conn = ch;
    char *rname = 0;
    int rdbid = 0;
    char *pname = (char *)(name ? name : "");

    //eyedblib::display_time("%s %s", name, "::open #3");
    if (isBackEnd()) {
      int pid;
      DbHandle *ldbh;

      rpc_status = IDB_dbOpen(ConnectionPeer::getConnH(ch), (const char *&)dbmdb_str,
			      user, passwd,
			      pname, dbid, flag, (oh ? oh->maph : 0),
			      (oh ? oh->mapwide : 0),
			      &pid, &uid, (void *)this, &rname, &rdbid, 
			      &version, &ldbh);
      if (rpc_status != RPCSuccess)
	return StatusMake(rpc_status);

      //eyedblib::display_time("%s %s", name, "::open #4");
      dbh = (DbHandle *)malloc(sizeof(DbHandle));

      IDB_getLocalInfo(ldbh, &dbh->ldbctx, &dbh->sch_oid);

      dbh->ch = ConnectionPeer::getConnH(ch);
      dbh->ldbctx.local = rpc_True; // ?? oups
      dbh->u.dbh = ldbh;
      dbh->flags = flag & ~_DBOpenLocal;
      dbh->tr_cnt = 0;
      dbh->db = (void *)this;
      //eyedblib::display_time("%s %s", name, "::open #5");
    }
    else {
      rpc_status = dbOpen(ConnectionPeer::getConnH(ch), dbmdb_str,
			  user, passwd,
			  pname, dbid, flag, (oh ? oh->maph : 0),
			  (oh ? oh->mapwide : 0), &uid,
			  (void *)this, &rname, &rdbid, &version, &dbh);
      //eyedblib::display_time("%s %s", name, "::open #6");
      if (!rpc_status)
	dbh->db = (void *)this;
    }

    if (rpc_status != RPCSuccess)
      return StatusMake(rpc_status);

    if (rdbid)
      dbid = rdbid;

    if (rname) {
      if (rname != name)
	free(name);
      name = strdup(rname);
    }

    open_flag = flag;

    if (sch)
      return Success;

    open_state = True;

    //eyedblib::display_time("%s %s", name, "::open #7");
    transactionBegin();

    Oid sch_oid(dbh->sch_oid);
    if ((status = loadObject(&sch_oid,
			     (Object **)&sch)) ==
	Success)
      status = Success;
    else {
      /*
	fprintf(stderr, "error here!\n");
	status->print();
      */
    }

    //eyedblib::display_time("%s %s", name, "::open #8");
    transactionCommit();

    //eyedblib::display_time("%s %s", name, "::open #9");
    open_state = False;

    if (status == Success) {
      dbopen_list->insertObject(this);
      open_refcnt = 1;
    }

    if (!status) {
      transactionBegin();
      const Class *m_protclass = sch->getClass("protection");
      if (m_protclass)
	m_protoid = m_protclass->getOid();
      transactionCommit();
    }
  
    //eyedblib::display_time("%s %s", name, "::open #10");
    return status;
  }

  Status Database::set(ConnHandle *ch,
		       int _dbid,
		       int flag,
		       DbHandle *ldbh,
		       rpcDB_LocalDBContext *ldbctx,
		       const Oid *sch_oid,
		       unsigned int _version)
  {
    conn = ConnectionPeer::newIdbConnection(ch);

    dbid = _dbid;
    version = _version;
    dbh = (DbHandle *)malloc(sizeof(DbHandle));

    dbh->ch = ch;
    dbh->ldbctx = *ldbctx;
    dbh->tr_cnt = 0;
    dbh->ldbctx.local = rpc_True; // ?? oups
    dbh->u.dbh = ldbh;
    dbh->flags = flag & ~_DBOpenLocal;
    dbh->sch_oid = *sch_oid->getOid(); // was *sch_oid;
    dbh->db = (void *)this;

    is_back_end = True;

    open_state = True;
    open_flag = (OpenFlag)flag;

    Status status;
  
    sch = 0;

    if (status = transactionBegin())
      return status;

    if ((status = reloadObject(sch_oid,
			       (Object **)&sch)) ==
	Success)
      ;

    open_state = False;

    if (status == Success) {
      dbopen_list->insertObject(this);
      open_refcnt = 1;
    }

    transactionCommit();
    return status;
  }

  Status Database::rename(Connection *_conn, const char *newdbname,
			  const char *user, const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_RENAME_ERROR);

    check_auth(user, passwd, "renaming database");

    RPCStatus rpc_status;

    rpc_status = dbRename(ConnectionPeer::getConnH(_conn), dbmdb_str,
			  user, passwd,
			  name, newdbname);
    if (rpc_status == RPCSuccess) {
      free(name);
      name = strdup(newdbname);
      return Success;
    }

    return StatusMake(rpc_status);
  }

  Status Database::move(Connection *_conn,
			DbCreateDescription *dbdesc,
			const char *user, const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_MOVE_ERROR);

    // (FD) added 21/09/2007
    check_auth(user, passwd, "moving database");

    RPCStatus rpc_status;

    rpc_status = dbMove(ConnectionPeer::getConnH(_conn), dbmdb_str,
			user, passwd,
			name, dbdesc);

    return StatusMake(rpc_status);
  }

  Status Database::copy(Connection *_conn, const char *newdbname,
			Bool newdbid, DbCreateDescription *dbdesc,
			const char *user, const char *passwd)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_COPY_ERROR);

    if (!strcmp(name, newdbname))
      return Exception::make(IDB_DATABASE_COPY_ERROR, "cannot copy databases, names are identical '%s'", name);

    // (FD) added 26/09/2007
    check_auth(user, passwd, "copying database");

    RPCStatus rpc_status;

    rpc_status = dbCopy(ConnectionPeer::getConnH(_conn), dbmdb_str,
			user, passwd,
			name, newdbname, newdbid, dbdesc);

    return StatusMake(rpc_status);
  }

  Status Database::rename(const char *newdbname, const char *user,
			  const char *passwd)
  {
    if (!conn)
      return Exception::make(IDB_DATABASE_RENAME_ERROR, "connection is not set");

    //check_auth(user, passwd, "renaming database");

    return rename(conn, newdbname, user, passwd);
  }

  Status Database::move(DbCreateDescription *dbdesc,
			const char *user, const char *passwd)
  {
    check_auth(user, passwd, "move database");

    return move(conn, dbdesc, user, passwd);
  }

  Status Database::copy(const char *newdbname, Bool newdbid,
			DbCreateDescription *dbdesc,
			const char *user, const char *passwd)
  {
    check_auth(user, passwd, "copying database");

    return copy(conn, newdbname, newdbid, dbdesc, user, passwd);
  }


  Bool Database::isBackEnd() const
  {
    return is_back_end;
  }

  Bool Database::isLocal() const
  {
    return (open_flag & _DBOpenLocal) ? True : False;
  }

  const char *Database::getDBMDB() const
  {
    return dbmdb_str;
  }

  DbHandle *Database::getDbHandle(void)
  {
    return dbh;
  }

  Transaction *Database::getCurrentTransaction(void)
  {
#if 0
    if (!curtrs && dbh->tr_cnt)
      {
	fprintf(stderr, "in fact is in transaction!\n");
	return (Transaction *)1;
      }
    else if (!curtrs)
      fprintf(stderr, "is NOT in transaction!\n");
#endif
    return curtrs;
  }

  Transaction *Database::getRootTransaction(void)
  {
    return roottrs;
  }

  DbHandle *
  database_getDbHandle(Database *db)
  {
    return (DbHandle *)(db->getDbHandle()->u.dbh);
  }

  Bool Database::isInTransaction() const
  {
    if (curtrs)
      return True;
  
    if (is_back_end)
      return IDB_getSeTrsCount(database_getDbHandle((Database *)this)) ?
	True : False;

    return dbh && dbh->tr_cnt ? True : False;
  }

  Status Database::transactionBegin(const TransactionParams &params)
  {
    if (!trs_cnt) {
      Status status = transactionBegin_realize(&params);

      if (status)
	return status;
    }
#ifdef STRICT_NOT_NESTED_TRANSACTIONS
    else
      return Exception::make(IDB_ERROR,
			     "nested transactions are not yet implemented");
#endif

    ++trs_cnt;
    return Success;
  }

  Status
  Database::transactionBegin()
  {
    if (!trs_cnt) {
      Status status = transactionBegin_realize(0);

      if (status)
	return status;
    }
#ifdef STRICT_NOT_NESTED_TRANSACTIONS
    else
      return Exception::make(IDB_ERROR,
			     "nested transactions are not yet implemented");
#endif

    ++trs_cnt;
    return Success;
  }

  Status
  Database::transactionBeginExclusive()
  {
    TransactionParams params = TransactionParams::getGlobalDefaultTransactionParams();
    params.lockmode = DatabaseW;
    params.wait_timeout = 1;

    Status s = transactionBegin(params);
    if (s && s->getStatus() == eyedbsm::LOCK_TIMEOUT)
      return Exception::make(IDB_ERROR,
			     "cannot acquire exclusive lock on database %s",
			     getName());
    return s;
  }

  Status Database::transactionBegin_realize
  (const TransactionParams *params)
  {
    RPCStatus rpc_status;
    TransactionId tid;
    Status s;
    TransactionParams sparams = def_params;

    IDB_LOG(IDB_LOG_TRANSACTION, ("database transaction begin\n"));

    if (!params)
      params = &sparams;
  
    if (s = Transaction::checkParams(*params))
      return s;
  
    Transaction *trs = new Transaction(this, *params);

    s = trs->begin();

    if (s == Success) {
      if (!curtrs)
	roottrs = trs;
      curtrs = trs;
    }
    else
      curtrs = 0;

    return s;
  }

  Status Database::transactionCommit(void)
  {
    //  printf("db->transactionCommit()\n");
    return transactionCommit_realize();
  }

  Status Database::transactionCommit_realize()
  {
    //printf("commitRealize(%d) %p\n", trs_cnt, curtrs);
    if (!curtrs || !trs_cnt)
      return Exception::make(IDB_NO_CURRENT_TRANSACTION, "transactionCommit");

    if (trs_cnt > 1) {
      --trs_cnt;
      return Success;
    }

    IDB_LOG(IDB_LOG_TRANSACTION, ("database transaction commit\n"));

    Status s;

    if (store_on_commit && obj_register) {
      s = storeRegisteredObjects();
      if (s) return s;
    }

    //  printf("db->transactionCommit_realize()\n\n");

    s = curtrs->commit();
    if (s) return s;

    Transaction *trs = curtrs;

    curtrs = 0;

    if (trs == roottrs)
      roottrs = 0;

    delete trs;

    purgeRelease();
    if (sch)
      sch->revert(False);

    --trs_cnt;
    return s;
  }

  Status Database::transactionAbort(void)
  {
    //  printf("db->transactionAbort()\n");
    return transactionAbort_realize();
  }

  Status Database::transactionAbort_realize()
  {
    //utlog("abort _ realize(%p, trs_cnt = %d)\n", curtrs, trs_cnt);

    //printf("abortRealize(%d) %p\n", trs_cnt, curtrs);

    if (!curtrs || !trs_cnt)
      return Exception::make(IDB_NO_CURRENT_TRANSACTION, "transactionAbort");

    if (trs_cnt > 1) {
      --trs_cnt;
      return Success;
    }

    IDB_LOG(IDB_LOG_TRANSACTION, ("database transaction abort\n"));

    Status s;

    s = curtrs->abort();
    if (s) return s;

    Transaction *trs = curtrs;

    curtrs = 0;

    if (trs == roottrs)
      roottrs = 0;

    delete trs;

    purgeOnAbort();
    purgeRelease();
    if (sch)
      sch->revert(True);

    --trs_cnt;
    return s;
  }

  Status
  Database::loadObjects(const OidArray &oid_array,
			ObjectPtrVector &obj_vect,
			const RecMode *rcm)
  {
    ObjectArray obj_array; // true of false ?
    Status s = loadObjects(oid_array, obj_array, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status
  Database::loadObjects(const OidArray &oid_array,
			ObjectPtrVector &obj_vect,
			LockMode lockmode,
			const RecMode *rcm)
  {
    ObjectArray obj_array; // true of false ?
    Status s = loadObjects(oid_array, obj_array, lockmode, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status
  Database::loadObjects(const OidArray &oid_array,
			ObjectArray &obj_array,
			const RecMode *rcm)
  {
    return loadObjects(oid_array, obj_array, DefaultLock, rcm);
  }

  Status
  Database::loadObjects(const OidArray &oid_array,
			ObjectArray &obj_array,
			LockMode lockmode,
			const RecMode *rcm)
  {
    int count = oid_array.getCount();
    Object **objs = (Object **)calloc(sizeof(Object *), count);

    obj_array.set(objs, count);

    for (int i = 0; i < count; i++) {
      const Oid *poid = &oid_array[i];
      Status status = loadObject(*poid, objs[i], lockmode, rcm);
      if (status)
	return status;
    }

    return Success;
  }

  Status
  Database::loadObject(const Oid &xoid, ObjectPtr &o_ptr,
		       const RecMode *recmode)
  {
    Object *o = 0;
    Status s = loadObject(xoid, o, recmode);
    o_ptr = o;
    return s;
  }

  Status
  Database::loadObject(const Oid &xoid, ObjectPtr &o_ptr,
		       LockMode lockmode,
		       const RecMode *recmode)
  {
    Object *o = 0;
    Status s = loadObject(xoid, o, lockmode, recmode);
    o_ptr = o;
    return s;
  }

  Status
  Database::reloadObject(const Oid &xoid, ObjectPtr &o_ptr,
			 const RecMode *recmode)
  {
    Object *o = 0;
    Status s = reloadObject(xoid, o, recmode);
    o_ptr = o;
    return s;
  }

  Status
  Database::reloadObject(const Oid &xoid, ObjectPtr &o_ptr,
			 LockMode lockmode,
			 const RecMode *recmode)
  {
    Object *o = 0;
    Status s = reloadObject(xoid, o, lockmode, recmode);
    o_ptr = o;
    return s;
  }

  Status
  Database::loadObject(const Oid &xoid, Object *&o,
		       const RecMode *recmode)
  {
    return loadObject(xoid, o, DefaultLock, recmode);
  }

  Status
  Database::loadObject(const Oid &xoid, Object *&o,
		       LockMode lockmode,
		       const RecMode *recmode)
  {
    return loadObject(&xoid, &o, lockmode, recmode);
  }

  Status
  Database::loadObject(const Oid *poid, Object **o,
		       const RecMode *recmode)
  {
    return loadObject(poid, o, DefaultLock, recmode);
  }

  Status
  Database::loadObject(const Oid *poid, Object **o,
		       LockMode lockmode,
		       const RecMode *recmode)
  {
    if (!poid->isValid())
      return Exception::make(IDB_ERROR, "loadObject: oid '%s' is invalid",
			     poid->getString());

    if (recmode->getType() != RecMode_NoRecurs)
      temp_cache->empty();

    return loadObject_realize(poid, o, lockmode, recmode);
  }

  Status
  Database::reloadObject(const Oid &_oid, Object *&o,
			 const RecMode *recmode)
  {
    return reloadObject(&_oid, &o, DefaultLock, recmode);
  }

  Status
  Database::reloadObject(const Oid &_oid, Object *&o,
			 LockMode lockmode,
			 const RecMode *recmode)
  {
    return reloadObject(&_oid, &o, lockmode, recmode);
  }

  Status
  Database::reloadObject(const Oid *poid, Object **o,
			 const RecMode *recmode)
  {
    return reloadObject(poid, o, DefaultLock, recmode);
  }

  Status
  Database::reloadObject(const Oid *poid, Object **o,
			 LockMode lockmode,
			 const RecMode *recmode)
  {
    if (!poid->isValid())
      return Exception::make(IDB_ERROR, "loadObject: oid '%s' is invalid",
			     poid->getString());

    if (recmode->getType() != RecMode_NoRecurs)
      temp_cache->empty();

    return loadObject_realize(poid, o, lockmode, recmode, True);
  }

  Status
  Database::loadObject_realize(const Oid *poid,
			       Object **o,
			       LockMode lockmode,
			       const RecMode *recmode,
			       Bool reload)
  {
    int obj_dbid = poid->getDbid();
    static int cnt;

    if (!poid->isValid())
      return Exception::make(IDB_ERROR, "invalid null oid");

    if (!obj_dbid)
      return Exception::make(IDB_ERROR, "oid '%s': invalid null database",
			     poid->toString());

    if (obj_dbid != dbid) {
      Database *xdb;
      Status status = getOpenedDB(obj_dbid, this, xdb);
      if (status) return status;
      if (!xdb)
	return Exception::make(IDB_DATABASE_LOAD_OBJECT_ERROR,
			       "cannot load object %s: "
			       "database ID #%d must be manually "
			       "opened by the client",
			       poid->toString(), obj_dbid);
      if (reload)
	return xdb->reloadObject(poid, o, lockmode, recmode);
      return xdb->loadObject(poid, o, lockmode, recmode);
    }

    //printf("trying to load %s\n", poid->toString());
#ifdef CACHE_OBJECTS
    if (reload)
      uncacheObject(*poid);
    else if (curtrs && (*o = (Object *)curtrs->getObject(*poid))) {
      ObjectPeer::incrRefCount(*o);
      //printf("object %s in transaction cache\n", poid->getString());
      return Success;
    }
#endif

    if (recmode->getType() != RecMode_NoRecurs &&
	(*o = (Object *)temp_cache->getObject(*poid)))
      return Success;

    RPCStatus rpc_status;
    ObjectHeader hdr;
    Status status = Success;

    Data o_idr = 0;
    const Class *cl;

#ifdef PROBE
    p_probeM(eyedb_probe_h, 1);
#endif
  
    short datid;
    if (isLocal())
      rpc_status = objectRead(dbh, 0, &o_idr, &datid, poid->getOid(), &hdr,
			      lockmode, (void **)&cl);
    else {
      if ((rpc_status = objectRead(dbh, 0, &o_idr, &datid, poid->getOid(),
				   0,
				   lockmode, 0)) ==
	  RPCSuccess) {
	Offset offset = 0;
	object_header_decode(o_idr, &offset, &hdr);

	cl = sch->getClass(hdr.oid_cl, True);
	if (cl && !ObjectPeer::isRemoved(hdr)) {
	  Size o_idr_size = cl->getIDRObjectSize();
	  if (o_idr_size > hdr.size) {
	    o_idr = (Data)realloc(o_idr, o_idr_size);
	    memset(o_idr + hdr.size, 0, o_idr_size - hdr.size);
	    hdr.size = o_idr_size;
	  }
	}
      }
    }

#ifdef PROBE
    p_probeM(eyedb_probe_h, 1);
#endif
    if (!rpc_status) {
      const Datafile *datafile;
      status = getDatafile(datid, datafile);
      if (status)
	return status;

      const Dataspace *dataspace = datafile->getDataspace();
      if (!dataspace) {
	return Exception::make(IDB_DATABASE_LOAD_OBJECT_ERROR,
			       "loading object %s: "
			       "cannot find dataspace for datafile %d",
			       poid->getString(),
			       datafile->getId());
      }

      short dspid = dataspace->getId();

      Status (*make)(Database *, const Oid *, Object **,
		     const RecMode *,
		     const ObjectHeader *, Data,
		     LockMode lockmode,
		     const Class *) =
	getMakeFunction(hdr.type);

      if (!make)
	return Exception::make(IDB_ERROR,
			       "internal error: unknown object type "
			       "for oid %s (type:%x)", poid->getString(),
			       hdr.type);

      status = make(this, poid, o, recmode, &hdr, o_idr, lockmode, cl);
      
#ifdef PROBE
      p_probeM(eyedb_probe_h, 2);
#endif
      if (!status) {
	ObjectPeer::setDatabase(*o, this);
	ObjectPeer::loadEpilogue(*o, *poid, hdr, o_idr);
	if (recmode->getType() != RecMode_NoRecurs)
	  temp_cache->insertObject(*poid, (void *)*o);

	Status s = (*o)->setDataspace(dataspace);
	if (s)
	  s->print();
	(*o)->setDspid(dspid);

#ifdef FULL_CACHE_OBJECTS
	if (curtrs)
	  curtrs->cacheObject(*poid, *o);
#endif
      }

#ifdef PROBE
      p_probeM(eyedb_probe_h, 2);
#endif
      return status;
    }

    free(o_idr);

    if (rpc_status->err == eyedbsm::OBJECT_PROTECTED) {
      *o = new UnreadableObject(this);
      ObjectPeer::setOid(*o, *poid);
      return Success;
    }

    return StatusMake(rpc_status);
  }

  Status
  Database::isRemoved(const Oid &poid, Bool &isremoved) const
  {
    RPCStatus rpc_status;
    Class *cl;
    unsigned char data[IDB_OBJ_HEAD_SIZE];
    rpc_status = dataRead(dbh, 0, IDB_OBJ_HEAD_SIZE, data, 0, poid.getOid());
    if (rpc_status)
      return StatusMake(rpc_status);

    Offset offset = 0;
    ObjectHeader hdr;

    if (!object_header_decode(data, &offset, &hdr))
      return Exception::make(IDB_INVALID_OBJECT_HEADER,
			     "Database::isRemoved");

    isremoved = IDBBOOL(hdr.xinfo & IDB_XINFO_REMOVED);
    return Success;
  }

  Status Database::removeObject(const Oid &poid,
				const RecMode *rcm)
  {
    return removeObject(&poid, rcm);
  }

  Status Database::removeObject(const Oid *poid,
				const RecMode *rcm)
  {
    Object *o;
    Status status;

    if ((open_flag & _DBRW) != _DBRW)
      return Exception::make(IDB_OBJECT_REMOVE_ERROR, "remove object '%s': database '%s' is not opened for writing", poid->getString(), name);

#ifdef CACHE_OBJECTS
    int must_load = (!curtrs || !(o = (Object *)curtrs->getObject(*poid)));

    if (must_load)
#endif
      {
	status = loadObject(poid, &o, rcm);

	if (status)
	  return status;
      }

    status = o->remove(rcm);

#ifdef CACHE_OBJECTS
    if (must_load)
#endif
      o->release();

    return status;
  }

  Status
  Database::setObjectLock(const Oid &_oid, LockMode lockmode)
  {
    RPCStatus rpc_status;
    int alockmode;
    if ((rpc_status = eyedb::setObjectLock(dbh, _oid.getOid(), lockmode, &alockmode))
	!= RPCSuccess)
      return StatusMake(rpc_status);
    
    return Success;
  }

  Status
  Database::setObjectLock(const Oid &_oid, LockMode lockmode,
			  LockMode &alockmode)
  {
    RPCStatus rpc_status;
    int _alockmode;
    if ((rpc_status = eyedb::setObjectLock(dbh, _oid.getOid(), lockmode, &_alockmode))
	!= RPCSuccess)
      return StatusMake(rpc_status);

    alockmode = (LockMode)_alockmode;
    return Success;
  }

  Status
  Database::getObjectLock(const Oid &_oid, LockMode &alockmode)
  {
    RPCStatus rpc_status;
    int _alockmode;
    if ((rpc_status = eyedb::getObjectLock(dbh, _oid.getOid(), &_alockmode))
	!= RPCSuccess)
      return StatusMake(rpc_status);

    alockmode = (LockMode)_alockmode;
    return Success;
  }

  Status Database::makeObject(const Oid *poid,
			      const ObjectHeader *hdr,
			      Data _idr,
			      Object **o, Bool useCache)
  {
    if (!poid->isValid())
      return Exception::make(IDB_ERROR, "makeObject: oid '%s' is invalid",
			     poid->getString());
    temp_cache->empty();
    return makeObject_realize(poid, hdr, _idr, o, useCache);
  }

  Status Database::makeObject_realize(const Oid *poid,
				      const ObjectHeader *hdr,
				      Data _idr,
				      Object **o, Bool useCache)
  {
    if (*o = (Object *)temp_cache->getObject(*poid))
      return Success;

#ifdef CACHE_OBJECTS
    // 18/05/99 WE DO NOT USE CACHE anymore
    //  if (useCache && curtrs && (*o = (Object *)curtrs->getObject(*poid)))
    if (0) {
      ObjectPeer::incrRefCount(*o);
      if (!memcmp(_idr, (*o)->getIDR(), hdr->size))
	return Success;
      else {
	fprintf(stderr, "invalid cache for object '%s'\n", 
		poid->getString());
	utlog("invalid cache for object '%s'\n", 
	      poid->getString());
      }
    }
#endif

    Status status = Success;
    Bool copy = False;

    Class *cl = sch->getClass(ClassOidDecode(_idr));
    Size _obj_sz = hdr->size;
    if (cl) {
      Size _psize, _vsize;
      _obj_sz = cl->getIDRObjectSize(&_psize, &_vsize);
      if (_vsize) {
	Data cp_idr = (unsigned char *)malloc(_obj_sz);
	memcpy(cp_idr, _idr, hdr->size);
	memset(cp_idr + _psize, 0, _vsize);
	_idr = cp_idr;
	copy = True;
      }
    }

    Status (*make)(Database *, const Oid *, Object **,
		   const RecMode *,
		   const ObjectHeader *, Data,
		   LockMode lockmode,
		   const Class *) =
      getMakeFunction(hdr->type);

    if (!make)
      return Exception::make(IDB_ERROR,
			     "internal error: unknown object type "
			     "for oid %s (type:%x)", poid->getString(),
			     hdr->type);

    status = make(this, poid, o, NoRecurs,
		  hdr, _idr, DefaultLock, 0);

    if (!status) {
      status = (*o)->setDatabase(this);
      if (status) {
	if (copy) free(_idr);
	return status;
      }

      ObjectPeer::setOid(*o, *poid);
      ObjectPeer::setModify(*o, False);
      
      if (!(*o)->getIDR()) {
	Data cp_idr = (unsigned char *)malloc(hdr->size);
	memcpy(cp_idr, _idr, hdr->size);
	ObjectPeer::setIDR(*o, cp_idr, hdr->size);
      }

      ObjectPeer::setTimes(*o, *hdr);
      temp_cache->insertObject(*poid, (void *)*o);
    }
    // added the else if on the 15/04/99
    else if (copy)
      free(_idr);

    return status;
  }

  Bool
  Database::isOpened() const
  {
    return IDBBOOL(open_flag);
  }

  Database::OpenFlag Database::getOpenFlag(void) const
  {
    return open_flag;
  }

  const Schema *Database::getSchema(void) const
  {
    return sch;
  }

  void Database::setSchema(Schema *_sch)
  {
    sch = _sch;
    if (sch)
      sch->setDatabase(this);
  }

  Schema *Database::getSchema(void)
  {
    return sch;
  }

  void Database::insertTempCache(const Oid& poid, void *o)
  {
    temp_cache->insertObject(poid, o);
  }

  void Database::cacheObject(Object *o)
  {
#ifdef CACHE_OBJECTS
    if (curtrs) {
      Oid _oid = o->getOid();
      if (_oid.isValid())
	curtrs->cacheObject(_oid, o);
    }
#endif
  }

  Object *
  Database::getCacheObject(const Oid &xoid)
  {
#ifdef CACHE_OBJECTS
    return (curtrs ? curtrs->getObject(xoid) : 0);
#else
    return 0;
#endif
  }

  ObjectList *
  Database::getRegisteredObjects()
  {
    return (obj_register ? obj_register->getObjects() : 0);
  }

  static inline Oid
  obj2oid(const Object *o)
  {
    //  assert(!(((unsigned long)o) & 0x7));
    return Oid(((unsigned long)o)>>3, 1, 1);
  }

  ObjCache *
  Database::makeRegister()
  {
    return new ObjCache(512);
  }

  void
  Database::clearRegister()
  {
    delete obj_register;
    obj_register = (auto_register_on ? makeRegister() : 0);
  }

  void
  Database::storeOnCommit(Bool on)
  {
    if (on == store_on_commit)
      return;

    store_on_commit = on;
    if (store_on_commit && !auto_register_on) {
      auto_register_on = True;
      clearRegister();
    }
  }

  void
  Database::autoRegisterObjects(Bool on)
  {
    if (on == auto_register_on)
      return;

    auto_register_on = on;
    clearRegister();
  }

  void
  Database::addToRegister(const Object *o, Bool force)
  {
    if (auto_register_on || force) {
      if (!obj_register)
	obj_register = makeRegister();
      obj_register->insertObject(obj2oid(o), (void *)o);
    }
  }

  void
  Database::rmvFromRegister(const Object *o)
  {
    if (obj_register)
      obj_register->deleteObject(obj2oid(o));
  }

  Status
  Database::storeRegisteredObjects()
  {
    if (!obj_register)
      return Exception::make(IDB_ERROR,
			     "Database::storeRegisteredObjects(): "
			     "objects are not registered: use Database::registerObjects(True)");

    ObjectList *list = getRegisteredObjects();

    if (!list)
      return Success;

    ObjectListCursor c(list);
    Object *o;
    while (c.getNext(o)) {
      //printf("storing registered objects %p\n", o);
      Status s = o->store(RecMode::FullRecurs);
      if (s) {delete list; return s;}
    }

    delete list;
    return Success;
  }

  void
  Database::uncacheObject(Object *o)
  {
#ifdef CACHE_OBJECTS
    if (curtrs)
      curtrs->uncacheObject(o);
#endif
  }

  void Database::uncacheObject(const Oid &_oid)
  {
#ifdef CACHE_OBJECTS
    if (curtrs)
      curtrs->uncacheObject(_oid);
#endif
  }

  Bool Database::isOpeningState()
  {
    return open_state;
  }

  Status Database::containsObject(const Oid &_oid, Bool &found)
  {
    found = False;
    int _state;
    eyedbsm::Oid moid;

    RPCStatus rpc_status = objectCheck(dbh, _oid.getOid(), &_state, &moid);

    if (rpc_status != RPCSuccess)
      return StatusMake(rpc_status);

    found = (_state ? True : False);
    return Success;
  }


  Status Database::create()
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "Database::create");
  }

  Status Database::update()
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "Database::update");
  }

  Status Database::realize(const RecMode*)
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "Database::realize");
  }

  Status Database::remove(const RecMode*)
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "Database::remove");
  }

  Status
  Database::getObjectClass(const Oid &poid, Oid &cls_oid)
  {
    RPCStatus rpc_status;
    Class *cl;
    unsigned char data[IDB_OBJ_HEAD_SIZE];
    rpc_status = dataRead(dbh, 0, IDB_OBJ_HEAD_SIZE, data, 0, poid.getOid());
    if (rpc_status)
      return StatusMake(rpc_status);

    Offset offset = 0;
    ObjectHeader hdr;

    if (!object_header_decode(data, &offset, &hdr))
      return Exception::make(IDB_INVALID_OBJECT_HEADER,
			     "Database::isRemoved");

    cls_oid = hdr.oid_cl;
    return Success;
  }

  Status
  Database::getObjectClass(const Oid &_oid, Class *&_class)
  {
    _class = 0;

    if (!_oid.isValid())
      return Exception::make(IDB_ERROR, "invalid null oid");

    int obj_dbid = _oid.getDbid();

    if (!obj_dbid)
      return Exception::make(IDB_ERROR, "oid '%s': invalid null database",
			     _oid.toString());

    if (obj_dbid != dbid) {
      Database *xdb;
      Status status = getOpenedDB(obj_dbid, this, xdb);
      if (status) return status;

      if (!xdb)
	return Exception::make(IDB_DATABASE_GET_OBJECT_CLASS_ERROR,
			       "cannot get class of object %s: "
			       "database ID #%d must be manually "
			       "opened by the client",
			       _oid.getString(), obj_dbid);
      
      return xdb->getObjectClass(_oid, _class);
    }

    eyedbsm::Oid moid;
    int _state;

    RPCStatus rpc_status = objectCheck(dbh, _oid.getOid(), &_state, &moid);

    if (rpc_status != RPCSuccess)
      return StatusMake(rpc_status);

    if (!_state)
      return Exception::make(IDB_ERROR, "cannot find class of object %s",
			     _oid.toString());

    Oid cloid(moid);

    // added this kludge (?) the 6/10/99
    Class *clx;
    if (!cloid.isValid() && (clx = sch->getClass(_oid))) {
      if (!strcmp(clx->getName(), "class") || !strcmp(clx->getName(), "set") ||
	  !strcmp(clx->getName(), "set<object*>") ||
	  !strcmp(clx->getName(), "object")) // last test added 2/5/00
	_class = sch->getClass("class");
    }
    else
      _class = sch->getClass(cloid);

#if 0
    if (!_class) {
      ClassConversion::Context *conv_ctx = 0;
      Status s = ClassConversion::getClass(this, cloid,
					   (const Class *&)_class,
					   conv_ctx);
      if (s) return s;
    }
#endif

    if (!_class)
      return Exception::make(IDB_ERROR, "cannot find class of object %s: "
			     "invalid class %s",
			     _oid.toString(), cloid.toString());
    return Success;
  }

  //
  // protection part
  //

  Status Database::setObjectProtection(const Oid &obj_oid,
				       const Oid &prot_oid)
  {
    RPCStatus rpc_status;

    rpc_status = objectProtectionSet(getDbHandle(), obj_oid.getOid(),
				     prot_oid.getOid());

    return StatusMake(rpc_status);
  }

  Status Database::setObjectProtection(const Oid &obj_oid,
				       Protection *prot)
  {
    return setObjectProtection(obj_oid, prot->getOid());
  }

  Status Database::getObjectProtection(const Oid &obj_oid,
				       Oid &prot_oid)
  {
    RPCStatus rpc_status;
    eyedbsm::Oid _prot_oid;

    rpc_status = objectProtectionGet(getDbHandle(), obj_oid.getOid(),
				     &_prot_oid);

    if (!rpc_status) {
      prot_oid.setOid(_prot_oid);
      return Success;
    }

    prot_oid.invalidate();
    return StatusMake(rpc_status);
  }

  Status Database::getObjectProtection(const Oid &obj_oid,
				       Protection *&prot)
  {
    Oid prot_oid;
    Status status = getObjectProtection(obj_oid, prot_oid);

    if (status)
      return status;

    return loadObject(&prot_oid, (Object **)&prot);
  }

  void Database::updateSchema(const SchemaInfo &schinfo)
  {
    if (schinfo.class_cnt) {
      for (int i = 0; i < schinfo.class_cnt; i++) {
	Object *o;
	Class *cl = sch->getClass(schinfo.class_oid[i]);
	if (cl) {uncacheObject(cl); sch->suppressClass(cl);}
	loadObject(&schinfo.class_oid[i], &o);
      }

      sch->complete(True, True);
    }
  }

  const char *
  Database::getVersion(void) const
  {
    return convertVersionNumber(version);
  }

  void
  Database::setIncoherency()
  {
    if (curtrs)
      curtrs->setIncoherency();
  }

  const char *
  Database::getStringFlag(OpenFlag flags)
  {
    static std::string s;

    if (flags & _DBRead)
      s = "read";
    else if (flags & _DBSRead)
      s = "sread";
    else if (flags & _DBRW)
      s = "read/write";
    else
      s = "<unknown>";

    if (flags & _DBAdmin)
      s += "/admin";
    if (flags & _DBOpenLocal)
      s += "/local";

    return s.c_str();
  }

  //
  // Purge action management
  //

  struct PurgeAction {
    void (*purge_action)(void *);
    void *purge_data;

    PurgeAction(void (*_purge_action)(void *), void *_purge_data) :
      purge_action(_purge_action), purge_data(_purge_data) { }
  };

  void
  Database::addPurgeActionOnAbort(void (*purge_action)(void *),
				  void *purge_data)
  {
    purge_action_list.insertObjectLast
      (new PurgeAction(purge_action, purge_data));
  }

  void
  Database::purgeOnAbort()
  {
    LinkedListCursor c(purge_action_list);
    PurgeAction *purge_action;

    while (c.getNext((void *&)purge_action))
      purge_action->purge_action(purge_action->purge_data);
  }

  void
  Database::purgeRelease()
  {
    LinkedListCursor c(purge_action_list);
    PurgeAction *purge_action;
    while (c.getNext((void *&)purge_action))
      delete purge_action;
    purge_action_list.empty();
  }

  void 
  Database::add(GenHashTable *_hashapp, consapp_t *_consapp)
  {
    hashapp[consapp_cnt] = _hashapp;
    consapp[consapp_cnt] = _consapp;
    consapp_cnt++;
    assert(consapp_cnt <= sizeof(hashapp)/sizeof(hashapp[0]));
  }

  Database::consapp_t
  Database::getConsApp(const Class *_class)
  {
    if (!consapp_cnt)
      return 0;

    const char *_name = _class->getName();
    const char *_aliasname = _class->getStrictAliasName();

    for (int i = 0; i < consapp_cnt; i++)
      {
	int ind = hashapp[i]->get(_name);
	if (ind >= 0) return consapp[i][ind];

	if (_aliasname)
	  {
	    ind = hashapp[i]->get(_aliasname);
	    if (ind >= 0) return consapp[i][ind];
	  }
      }

    return 0;
  }

  void
  Database::garbage_dat_dsp()
  {
    free(database_file);
    for (int i = 0; i < datafile_cnt; i++)
      delete datafiles[i];
    delete[] datafiles;

    for (int i = 0; i < dataspace_cnt; i++)
      delete dataspaces[i];
    delete[] dataspaces;
  }

  const Datafile **
  Database::get_datafiles(const eyedbsm::Dataspace *dsp)
  {
    const Datafile **dats = new const Datafile *[dsp->ndat];
    for (int i = 0; i < dsp->ndat; i++)
      dats[i] = datafiles[dsp->datid[i]];
    return dats;
  }

  void
  Database::make_dat_dsp(const DbInfoDescription &dbdesc)
  {
    garbage_dat_dsp();

    database_file = strdup(dbdesc.dbfile);
    const eyedbsm::DbInfoDescription *sed = &dbdesc.sedbdesc;
    datafile_cnt = sed->ndat;
    datafiles = new Datafile *[datafile_cnt];

    for (int i = 0; i < datafile_cnt; i++) {
      const eyedbsm::Datafile *dat = &sed->dat[i];
      datafiles[i] = new Datafile(this, (unsigned short)i,
				  dat->dspid, dat->file, dat->name,
				  dat->maxsize,
				  (eyedbsm::MapType)dat->mtype, dat->sizeslot,
				  (DatType)dat->dtype);
    }

    dataspace_cnt = sed->ndsp;
    dataspaces = new Dataspace *[dataspace_cnt];

    for (int i = 0; i < dataspace_cnt; i++) {
      const eyedbsm::Dataspace *dsp = &sed->dsp[i];
      const Datafile **dsp_datafiles = get_datafiles(dsp);
      dataspaces[i] = new Dataspace(this, (unsigned short)i,
				    dsp->name, dsp_datafiles,
				    dsp->ndat);
      for (int j = 0; j < dsp->ndat; j++)
	const_cast<Datafile *>(dsp_datafiles[j])->setDataspace(dataspaces[i]);
    }
  }

  Status
  Database::getDatDspPrologue(Bool fetch, const char *user,
			      const char *passwd)
  {
    if (datafiles && !fetch)
      return Success;

    DbInfoDescription dbdesc;
    Status s = getInfo(user, passwd, &dbdesc);
    if (s) return s;
    make_dat_dsp(dbdesc);
    return Success;
  }

  Status
  Database:: getDatabasefile(const char *&_database_file,
			     Bool fetch, const char *user,
			     const char *passwd)
  {
    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    _database_file = database_file;
    return Success;
  }

  Status
  Database::getDatafiles(const Datafile **&_datafiles, unsigned int &cnt,
			 Bool fetch, const char *user, const char *passwd)
  {
    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    _datafiles = const_cast<const Datafile **>(datafiles);
    cnt = datafile_cnt;
    return Success;
  }

  Status
  Database::getDatafile(unsigned short id, const Datafile *&datafile,
			Bool fetch, const char *user, const char *passwd)
  {
    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    if (id >= datafile_cnt)
      return Exception::make(IDB_ERROR, "datafile #%d not found in database "
			     "%s", id, name);

    datafile = datafiles[id];
    return Success;
  }

  Status
  Database::getDatafile(const char *name_or_file,
			const Datafile *&datafile,
			Bool fetch, const char *user, const char *passwd)
  {
    if (eyedblib::is_number(name_or_file))
      return getDatafile(atoi(name_or_file), datafile, fetch, user, passwd);

    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    for (int i = 0; i < datafile_cnt; i++)
      if (!strcmp(name_or_file, datafiles[i]->getName()) ||
	  !strcmp(name_or_file, datafiles[i]->getFile())) {
	datafile = datafiles[i];
	return Success;
      }

    return Exception::make(IDB_ERROR, "datafile %s not found in database "
			   "%s", name_or_file, name);
  }

  Status
  Database::getDataspaces(const Dataspace **&_dataspaces,
			  unsigned int &cnt,
			  Bool fetch, const char *user, const char *passwd)
  {
    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    _dataspaces = const_cast<const Dataspace **>(dataspaces);
    cnt = dataspace_cnt;
    return Success;
  }

  Status
  Database::getDataspace(unsigned short id, const Dataspace *&dataspace,
			 Bool fetch, const char *user, const char *passwd)
  {
    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    if ((short)id == Dataspace::DefaultDspid) {
      dataspace = 0;   //OR: return getDefaultDataspace(dataspace);
      return Success;
    }

    if (id >= dataspace_cnt)
      return Exception::make(IDB_ERROR, "dataspace #%d not found in database "
			     "%s", id, name);

    dataspace = dataspaces[id];
    return Success;
  }

  Status
  Database::getDataspace(const char *dspname,
			 const Dataspace *&dataspace,
			 Bool fetch, const char *user, const char *passwd)
  {
    if (eyedblib::is_number(dspname))
      return getDataspace(atoi(dspname), dataspace, fetch, user, passwd);

    Status s = getDatDspPrologue(fetch, user, passwd);
    if (s) return s;

    for (int i = 0; i < dataspace_cnt; i++)
      if (!strcmp(dspname, dataspaces[i]->getName())) {
	dataspace = dataspaces[i];
	return Success;
      }

    return Exception::make(IDB_ERROR, "dataspace %s not found in database "
			   "%s", dspname, name);
  }


  Status
  Database::getDefaultDataspace(const Dataspace *&dataspace)
  {
    int dspid;
    RPCStatus rpc_status = eyedb::getDefaultDataspace(dbh, &dspid);
    if (rpc_status)
      return StatusMake(rpc_status);

    return getDataspace(dspid, dataspace);
  }

  Status
  Database::setDefaultDataspace(const Dataspace *dataspace)
  {
    RPCStatus rpc_status = eyedb::setDefaultDataspace(dbh, dataspace->getId());
    if (rpc_status)
      return StatusMake(rpc_status);

    return Success;
  }

  Status
  Database::setMaxObjectCount(unsigned int max_obj_cnt)
  {
    RPCStatus rpc_status = eyedb::setMaxObjCount(dbh, max_obj_cnt);
    if (rpc_status)
      return StatusMake(rpc_status);

    return Success;
  }

  Status
  Database::getMaxObjectCount(unsigned int& max_obj_cnt)
  {
    int obj_cnt = 0;
    RPCStatus rpc_status = eyedb::getMaxObjCount(dbh, &obj_cnt);
    if (rpc_status)
      return StatusMake(rpc_status);

    max_obj_cnt = obj_cnt;
    return Success;
  }

  Status
  Database::setLogSize(unsigned int logsize)
  {
    RPCStatus rpc_status = eyedb::setLogSize(dbh, logsize);
    if (rpc_status)
      return StatusMake(rpc_status);

    return Success;
  }

  Status
  Database::getLogSize(unsigned int& logsize)
  {
    int ilogsize = 0;
    RPCStatus rpc_status = eyedb::getLogSize(dbh, &ilogsize);
    if (rpc_status)
      return StatusMake(rpc_status);

    logsize = ilogsize;
    return Success;
  }

  eyedbsm::Oid *
  oidArrayToOids(const OidArray &oid_arr, unsigned int &cnt)
  {
    cnt = oid_arr.getCount();
    if (!cnt) return 0;

    eyedbsm::Oid *oids = new eyedbsm::Oid[cnt];
    for (int i = 0; i < cnt; i++)
      oids[i] = *oid_arr[i].getOid();

    return oids;
  }

  eyedbsm::Oid *
  objArrayToOids(const ObjectArray &obj_arr, unsigned int &cnt)
  {
    cnt = obj_arr.getCount();
    if (!cnt) return 0;

    eyedbsm::Oid *oids = new eyedbsm::Oid[cnt];
    for (int i = 0; i < cnt; i++) {
      oids[i] = *obj_arr[i]->getOid().getOid();
    }

    return oids;
  }

  Status
  Database::moveObjects(const OidArray &oid_arr, const Dataspace *dataspace)
  {
    unsigned int cnt;
    eyedbsm::Oid *oids = oidArrayToOids(oid_arr, cnt);
    if (!cnt) return Success;
    RPCStatus rpc_status = eyedb::moveObjects(dbh, oids, cnt, dataspace->getId());
    delete [] oids;
    return StatusMake(rpc_status);
  }

  Status
  Database::moveObjects(const ObjectArray &obj_arr, const Dataspace *dataspace)
  {
    unsigned int cnt;
    eyedbsm::Oid *oids = objArrayToOids(obj_arr, cnt);
    if (!cnt) return Success;
    RPCStatus rpc_status = eyedb::moveObjects(dbh, oids, cnt, dataspace->getId());
    delete [] oids;
    return StatusMake(rpc_status);
  }

  Status
  Database::getObjectLocations(const OidArray &oid_arr,
			       ObjectLocationArray &locarr)
  {
    unsigned int cnt;
    eyedbsm::Oid *oids = oidArrayToOids(oid_arr, cnt);
    if (!cnt)
      return Success;

    RPCStatus rpc_status = getObjectsLocations(dbh, oids, cnt,
					       (Data *)&locarr);
    delete [] oids;
    return StatusMake(rpc_status);
  }

  Status
  Database::getObjectLocations(const ObjectArray &obj_arr,
			       ObjectLocationArray &locarr)
  {
    return Success;
  }

  Status
  Database::createDatafile(const char *filedir, const char *filename,
			   const char *name, unsigned int maxsize,
			   unsigned int slotsize, DatType dtype)
  {
    std::string s;
    if (filedir) {
      s = std::string(filedir);
      if (s.length() > 0 && s[s.length()-1] != '/')
	s += "/";
      s += filename;
    }
    else 
      s = filename;

    RPCStatus rpc_status =
      eyedb::createDatafile(dbh, s.c_str(), name, maxsize, slotsize, dtype);

    return StatusMake(rpc_status);
  }

  Status
  Database::createDataspace(const char *dspname,
			    const Datafile **datafiles,
			    unsigned int datafile_cnt)
  {
    char **datids = Dataspace::makeDatid(datafiles, datafile_cnt);
    RPCStatus rpc_status = eyedb::createDataspace(dbh, dspname,
						  datids, datafile_cnt);
    Dataspace::freeDatid(datids, datafile_cnt);
    return StatusMake(rpc_status);
  }

  Bool
  Database::writeBackConvertedObjects() const
  {
    return IDBBOOL(!(open_flag & _DBSRead));
  }

  void Database::addMarkCreated(const Oid &oid)
  {
    mark_created[oid] = true;
  }

  bool Database::isMarkCreated(const Oid &oid) const
  {
    return mark_created.find(oid) != mark_created.end();
  }

  void Database::markCreatedEmpty()
  {
    mark_created.erase(mark_created.begin(), mark_created.end());
  }

  void Database::beginRealize() {
    realizedMap.clear();
    useMap = true;
  }

  void Database::endRealize() {
    useMap = false;
  }

  void Database::operator delete(void *o)
  {
  }
}
