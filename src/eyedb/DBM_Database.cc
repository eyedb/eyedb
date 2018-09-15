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


//#define FRONT_END
#include "DBM_Database.h"
#include "db_p.h"
//#include "eyedbsm/transaction.h"
#define LOCKID

namespace eyedb {

  const char DBM_Database::defaultDBMDB[] = "default";

  LinkedList *DBM_Database::dbmdb_list;

  static const char dbentry_s[]       = "database_entry";
  static const char user_s[]          = "user_entry";
  static const char dbuseraccess_s[]  = "database_user_access";
  static const char sysuseraccess_s[] = "system_user_access";

  /* static members */
  void DBM_Database::init()
  {
    dbmdb_list = new LinkedList();
    DBM::init();
  }

  Status
  DBM_Database::updateSchema(Database *db)
  {
    //  return dbmSchemaUpdate(db);
    return DBM::updateSchema(db);
  }

  const char *DBM_Database::getDbName()
  {
    return "EYEDBDBM";
  }

  int DBM_Database::getDbid()
  {
    return 1;
  }

  DBM_Database *
  DBM_Database::setDBM_Database(const char *dbmfile, Database *db)
  {
    DBM_Database *dbm;

    if (dbm = getDBM_Database(dbmfile))
      dbmdb_list->deleteObject(dbm);

    if (!db)
      return 0;

    dbm = new DBM_Database(dbmfile, db);
    dbmdb_list->insertObject(dbm);

    return dbm;
  }

  void DBM_Database::_dble_underscore_release()
  {
    _release();
    //  dbmRelease();
    DBM::release();
  }

  void DBM_Database::_release()
  {
    if (!dbmdb_list)
      return;

    LinkedListCursor *c = dbmdb_list->startScan();

    DBM_Database *dbm;

    while (dbmdb_list->getNextObject(c, (void *&)dbm))
      {
	dbm->close();
	dbm->release();
      }

    dbmdb_list->endScan(c);

    dbmdb_list->empty();

    delete dbmdb_list;
    dbmdb_list = NULL;
  }

  DBM_Database *DBM_Database::getDBM_Database(const char *dbmfile)
  {
    LinkedListCursor *c = dbmdb_list->startScan();

    DBM_Database *dbm;

    while (dbmdb_list->getNextObject(c, (void *&)dbm))
      {
	if (!strcmp(dbm->dbmdb_str, dbmfile))
	  {
	    dbmdb_list->endScan(c);
	    return dbm;
	  }
      }

    dbmdb_list->endScan(c);
    return 0;
  }

  /* non static members */
  DBM_Database::DBM_Database(const char *dbmfile) :
    Database(DBM_Database::getDbName(), dbmfile)
  {
  }

  DBM_Database::DBM_Database(const char *dbmfile, Database *_dbmdb)
  : Database(*_dbmdb)
  {
    --open_refcnt;
  }

  Status
  DBM_Database::getDbFile(const char **dbname, int *_dbid, const char *&dbfile)
  {
    OQL *q;
    Status s;

    dbfile = 0;
    s = transactionBegin();
    if (s) return s;

    // TO IMPROVE this query, should use index directly!
    //eyedblib::display_time("DBM_Database::getDbFile #1");
    if ((*dbname)[0])
      q = new OQL(this, "select %s.dbname = \"%s\"", dbentry_s, *dbname);
    else
      q = new OQL(this, "select %s.dbid = %d", dbentry_s, *_dbid);

    ObjectArray obj_arr;
    // the #1 query takes about 400ms because of initialisation which
    // must complete part of the schema and because of other 
    // Schema::manageClassDeferred calls!
    s = q->execute(obj_arr);
    //eyedblib::display_time("DBM_Database::getDbFile #2");
    if (s)
      {
	transactionCommit();
	delete q;
	return s;
      }

    if (obj_arr.getCount())
      {
	DBEntry *dbmentry = (DBEntry *)obj_arr[0];
      
	if (!(*dbname)[0])
	  *dbname = strdup(dbmentry->dbname().c_str());
	else if (_dbid)
	  *_dbid = dbmentry->dbid();

	dbfile = strdup(dbmentry->dbfile().c_str());
      }

    delete q;
    return transactionCommit();
  }

  Status
  DBM_Database::getDatabases(LinkedList *list)
  {
    Status s;

    s = transactionBegin();
    if (s) return s;
    OQL q(this, (std::string("select ") + dbentry_s).c_str());

    ObjectArray obj_arr;
    s = q.execute(obj_arr);

    if (s)
      {
	transactionAbort();
	return s;
      }

    for (int i = 0; i < obj_arr.getCount(); i++)
      {
	DBEntry *dbmentry = (DBEntry *)obj_arr[i];
	Database *_db = new Database(dbmentry->dbname().c_str(),
				     dbmentry->dbid());
	list->insertObjectLast(_db);
      }

    return transactionCommit();
  }

  Status
  DBM_Database::createEntry(int _dbid, const char *_dbname, const char *_dbfile)
  {
    Status s;
    s = transactionBegin();
    DBEntry *entry = new DBEntry(this);

    entry->dbid(_dbid);
    entry->dbname(_dbname);
    entry->dbfile(_dbfile);
    entry->default_access(NoDBAccessMode);

    if (s) return s;
    s = entry->realize();
    transactionCommit();

    entry->release();
    return s;
  }

  Status
  DBM_Database::updateEntry(int _dbid, const char *_dbname, 
			    const char *_newdbname, const char *_dbfile)
  {
    Status s;
    DBEntry *entry;

    s = getDBEntry(_dbname, entry);
    if (s) return s;

    if (!entry)
      return Exception::make(IDB_DATABASE_RENAME_ERROR,
			     "database entry '%s' does not exist", _dbname);

    transactionBegin();
    entry->dbname(_newdbname);
    entry->dbfile(_dbfile);
    s = entry->realize();
    transactionCommit();

    entry->release();
    return s;
  }

  Status DBM_Database::getNewDbid(int &newid)
  {
    return getNewID(dbentry_s, "dbid", 2, newid, True);
  }

  Status DBM_Database::getNewUid(int &newid)
  {
    return getNewID(user_s, "uid", 1, newid, False);
  }

  static int
  cmp(const void *xi1, const void *xi2)
  {
    return *(const int *)xi1 - *(const int *)xi2;
  }

  static const char keyid[] = "eyedb::id";

  std::string
  DBM_Database::makeTempName(int dbid)
  {
    return std::string("--eyedb--temporary--#") + str_convert(dbid);
  }

  Status
  DBM_Database::lockId(Oid &objoid)
  {
    Status s;
    eyedbsm::Status se;
    if (!isInTransaction())
      return Exception::make(IDB_ERROR, "transaction expected in lockId");

    eyedbsm::DbHandle *se_dbh = get_eyedbsm_DbHandle((DbHandle *)dbh->u.dbh);

    Bool setRootEntry;
    if (!eyedbsm::rootEntryGet(se_dbh, keyid, objoid.getOid(), sizeof(eyedbsm::Oid)))
      {
	Class *xcls = 0;
	// check oid
	if (s = getObjectClass(objoid, xcls))
	  setRootEntry = True;
	else
	  setRootEntry = False;
      }
    else
      setRootEntry = True;

    if (setRootEntry)
      {
	Object *lockobj = sch->getClass("char")->newObj(this);
	s = lockobj->store();
	if (s) return s;
	objoid = lockobj->getOid();
	se = eyedbsm::rootEntrySet(se_dbh, keyid, objoid.getOid(), sizeof(eyedbsm::Oid),
				   eyedbsm::False);
	lockobj->release();
	if (se)
	  return Exception::make(IDB_ERROR, eyedbsm::statusGet(se));
      }

    /* warning; was se_OWRITE|se_LOCKX */
    se = eyedbsm::objectLock(se_dbh, objoid.getOid(), eyedbsm::LockX, 0);
    if (se)
      return Exception::make(IDB_ERROR, eyedbsm::statusGet(se));

    return Success;
  }

  Status
  DBM_Database::unlockId(const Oid &objoid)
  {
    eyedbsm::DbHandle *se_dbh = get_eyedbsm_DbHandle((DbHandle *)dbh->u.dbh);
    eyedbsm::Status se = eyedbsm::objectDownLock(se_dbh, objoid.getOid());
    if (se)
      return Exception::make(IDB_ERROR, eyedbsm::statusGet(se));
    return Success;
  }

  Status
  DBM_Database::getNewID(const char *classname, const char *attrname,
			 int start_val, int &_dbid, Bool create_entry)
  {
    Status s;
    _dbid = -1;

    s = transactionBegin();
    if (s) return s;
    Oid lockoid;
    s = lockId(lockoid);
    if (s) return s;

    OQL q(this, "select %s.%s", classname, attrname);
    int maxdbid = 0;

    ValueArray val_arr;
    s = q.execute(val_arr);
    if (s)
      {
	transactionAbort();
	//unlockId(lockoid);
	return s;
      }

    int cnt = val_arr.getCount();
    if (cnt > 0)
      {
	int *values = new int[cnt];
	for (int i = 0; i < cnt; i++)
	  values[i] = val_arr[i].l;

	qsort(values, cnt, sizeof(int), cmp);
      
	for (int j = 0; j < cnt; j++)
	  if (j > 0 && values[j] - values[j-1] > 1)
	    {
	      _dbid = values[j-1]+1;
	      break;
	    }
      
	if (_dbid < 0)
	  _dbid = values[cnt-1] + 1;
      
	delete [] values;
      }
    else
      _dbid = start_val;

    if (create_entry)
      s = createEntry(_dbid, makeTempName(_dbid).c_str(), "");

    if (s)
      transactionAbort();
    else
      transactionCommit();
    //unlockId(lockoid);
    return s;
  }

  Status
  DBM_Database::removeEntry(const char *dbname)
  {
    Status s;
    s = transactionBegin();
    if (s) return s;

    OQL q(this,
	  "for (y in (select %s->dbentry->dbname = \"%s\")) delete y",
	  dbuseraccess_s, dbname);

    s = q.execute();
    if (s)
      {
	transactionAbort();
	return s;
      }

    OQL q1(this, "select %s.dbname = \"%s\"", dbentry_s, dbname);

    OidArray oid_arr;
    s = q1.execute(oid_arr);
    if (s)
      {
	transactionAbort();
	return s;
      }

    if (oid_arr.getCount())
      s = removeObject(oid_arr[0]);
    else
      s = Exception::make(IDB_DBM_ERROR,
			  "fatal error: entry '%s' not found", dbname);

    transactionCommit();
    return s;
  }

  Status
  DBM_Database::getUser(const char *username, UserEntry *&user)
  {
    Status s;
    user = 0;;

    s = transactionBegin();
    if (s) return s;

    OQL q(this, "select %s->name = \"%s\"", user_s, username);

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s)
      {
	transactionAbort();
	return s;
      }

    if (obj_arr.getCount())
      user = (UserEntry *)obj_arr[0];
    
    return transactionCommit();
  }

  Status
  DBM_Database::setSchema(const char *dbname, const Oid &sch_oid)
  {
    DBEntry *dbentry;
    Status s;
    s = getDBEntry(dbname, dbentry);
    if (s) return s;

    if (dbentry)
      {
	transactionBegin();

	/* cannot use : dbentry->sch_oid(sch_oid), because this method
	   call item->setOid(dbentry, &sch_oid, 1, 0, True), which means
	   a check of the type of sch_oid, so a database opening.
	   But as we are creating this database, it is not yet ready, so : */

	const Attribute *item =
	  dbentry->getClass()->getAttribute("sch");
	if (item)
	  {
	    s = item->setOid(dbentry, &sch_oid, 1, 0, False);
	    if (!s)
	      s = dbentry->realize();
	  }

	transactionCommit();
	dbentry->release();
	return s;
      }

    return Exception::make(IDB_DATABASE_OPEN_ERROR,
			   "database entry '%s' not found", dbname);
  }

  Status
  DBM_Database::getDBEntries(const char *dbname, DBEntry **&dbentries,
			     int &cnt, const char *op)
  {
    Status s;
    dbentries = 0;;
    cnt = 0;

    s = transactionBegin();
    if (s) return s;

    OQL q(this, "select %s->dbname %s \"%s\"", dbentry_s, op, dbname);

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s)
      {
	transactionAbort();
	return s;
      }

    cnt = obj_arr.getCount();
    if (!cnt)
      {
	dbentries = 0;
	return Success;
      }

    dbentries = new DBEntry *[cnt];
    for (int i = 0; i < cnt; i++)
      dbentries[i] = (DBEntry *)obj_arr[i];
    
    return transactionCommit();
  }

  Status
  DBM_Database::getDBEntry(const char *dbname, DBEntry *&dbentry)

  {
    Status s;
    dbentry = 0;;

    s = transactionBegin();
    if (s) return s;

    OQL q(this, "select %s->dbname = \"%s\"", dbentry_s, dbname);

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s)
      {
	transactionAbort();
	return s;
      }

    if (obj_arr.getCount())
      dbentry = (DBEntry *)obj_arr[0];
    
    return transactionCommit();
  }

  Status
  DBM_Database::get_sys_user_access(const char *username,
				    SysUserAccess **psysaccess,
				    Bool justCheck,
				    const char *msg)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (!user)
      return Exception::make(IDB_AUTHENTICATION_FAILED,
			     "user entry '%s' not found", username);

    user->release();

    s = transactionBegin();
    if (s) return s;

    OQL q(this, "select %s->user->name = \"%s\"", sysuseraccess_s, username);

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s)
      {
	transactionCommit();
	return s;
      }

    if (!obj_arr.getCount())
      {
	*psysaccess = (SysUserAccess *)0;
	if (justCheck)
	  s = Success;
	else
	  s = Exception::make(IDB_INSUFFICIENT_PRIVILEGES,
			      "user entry '%s': %s", username, msg);
      }
    else
      {
	*psysaccess = (SysUserAccess *)obj_arr[0];
	s = Success;
      }

    transactionCommit();
    return s;
  }

  Status DBM_Database::get_db_user_access(const char *dbname,
					  const char *username,
					  UserEntry **puser,
					  DBUserAccess **pdbaccess,
					  DBAccessMode *defaccess)
  {
    Status s;

    s = getUser(username, *puser);
    if (s) return s;

    if (!*puser)
      return Exception::make(IDB_AUTHENTICATION_FAILED,
			     "user entry '%s' not found", username);

    DBEntry *dbentry;
    s = getDBEntry(dbname, dbentry);
    if (s)
      {
	(*puser)->release();
	return s;
      }

    if (!dbentry)
      {
	(*puser)->release();
	return Exception::make(IDB_DATABASE_OPEN_ERROR,
			       "database entry '%s' not found", dbname);
      }

    s = transactionBegin();
    if (s)
      {
	(*puser)->release();
	dbentry->release();
	return s;
      }
    
    OQL q(this, "select x from %s x where x->user->name = \"%s\" && "
	  "x->dbentry->dbname = \"%s\"",
	  dbuseraccess_s, username, dbname);

    ObjectArray obj_arr;
    s = q.execute(obj_arr);
    if (s)
      {
	(*puser)->release();
	dbentry->release();
	transactionCommit();
	return s;
      }

    if (obj_arr.getCount())
      *pdbaccess = (DBUserAccess *)obj_arr[0];
    else
      *pdbaccess = (DBUserAccess *)0;

    *defaccess = dbentry->default_access();

    dbentry->release();
    return transactionCommit();
  }

  Status DBM_Database::add_user(const char *username, const char *passwd,
				UserType user_type)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (user)
      {
	user->release();
	return Exception::make(IDB_ADD_USER_ERROR,
			       "user entry '%s' already exists", username);
      }

    user = new UserEntry(this);
    user->name(username);
    if (passwd)
      user->passwd(passwd);

    int newid;
    s = getNewUid(newid);
    if (s)
      {
	transactionCommit();
	user->release();
      }

    user->uid(newid);
    user->type(user_type);

    s = transactionBegin();
    if (!s)
      s = user->realize();

    transactionCommit();
    user->release();

    if (s)
      return Exception::make(IDB_ADD_USER_ERROR,
			     "user entry '%s' : %s", username, s->getDesc());

    return Success;
  }

  Status DBM_Database::delete_user(const char *username)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (!user)
      return Exception::make(IDB_DELETE_USER_ERROR,
			     "user entry '%s' does not exist", username);

    const Oid xoid = user->getOid();

    user->release();

    s = transactionBegin();
    if (s) return s;

    OQL q(this, "for (y in (select %s.user = %s)) delete y",
	  dbuseraccess_s, xoid.getString());

    s = q.execute();
    if (s)
      {
	transactionAbort();
	return s;
      }

    s = removeObject(&xoid);
    transactionCommit();

    return s;
  }

  Status DBM_Database::user_passwd_set(const char *username,
				       const char *passwd)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (!user)
      return Exception::make(IDB_SET_USER_PASSWD_ERROR,
			     "user entry '%s' not found", username);

    if (passwd)
      user->passwd(passwd);

    s = transactionBegin();
    if (!s)
      s = user->realize();
    transactionCommit();

    user->release();

    return s;
  }

  Status DBM_Database::user_db_access_set(const char *dbname,
					  const char *username,
					  DBAccessMode dbmode)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (!user) {
      return Exception::make(IDB_SET_USER_DBACCESS_ERROR,
			     "user entry '%s' not found", username);
    }

    DBEntry *dbentry;

    s = getDBEntry(dbname, dbentry);
    if (s) {
      user->release();
      return s;
    }

    if (!dbentry)
      return Exception::make(IDB_SET_USER_DBACCESS_ERROR,
			     "database entry '%s' not found", dbname);


    DBUserAccess *dbaccess;
    Status status;

    s = transactionBegin();
    if (s) return s;
    dbaccess = new DBUserAccess(this);

    s = dbaccess->user(user);
    if (s) return s;
    s = dbaccess->dbentry(dbentry);
    if (s) return s;
    s = dbaccess->mode(dbmode);
    if (s) return s;

    OQL q(this,
	  "for (y in (select x from %s x where x->user->name = \"%s\" "
	  "&& x->dbentry->dbname = \"%s\")) delete y",
	  dbuseraccess_s, username, dbname);

    s = q.execute();
    if (s) {
      transactionAbort();
      return s;
    }

    s = dbaccess->realize();
    transactionCommit();

    user->release();
    dbentry->release();
    dbaccess->release();

    if (s)
      return Exception::make(IDB_SET_USER_DBACCESS_ERROR,
			     "database entry '%s', user entry '%s' : %s",
			     dbname, username, s->getString());

    return Success;
  }

  Status DBM_Database::default_db_access_set(const char *dbname,
					     DBAccessMode dbmode)
  {
    DBEntry *dbentry;
    Status s;

    s = getDBEntry(dbname, dbentry);
    if (s) return s;

    if (!dbentry)
      return Exception::make(IDB_SET_DEFAULT_DBACCESS_ERROR,
			     "database entry '%s' not found", dbname);

    DBUserAccess *dbaccess;

    dbentry->default_access(dbmode);

    s = transactionBegin();
    if (s)
      {
	dbentry->release();
	return s;
      }

    s = dbentry->realize();
    transactionCommit();

    dbentry->release();

    if (s)
      return Exception::make(IDB_SET_DEFAULT_DBACCESS_ERROR,
			     "database entry '%s' : %s", dbname, s->getString());

    return Success;
  }

  Status DBM_Database::user_sys_access_set(const char *username,
					   SysAccessMode sysmode)
  {
    UserEntry *user;
    Status s;

    s = getUser(username, user);
    if (s) return s;

    if (!user) {
      return Exception::make(IDB_SET_USER_SYSACCESS_ERROR,
			     "user entry '%s' not found", username);
    }

    SysUserAccess *sysaccess;

    sysaccess = new SysUserAccess(this);

    s = sysaccess->user(user);
    if (s) return s;
    s = sysaccess->mode(sysmode);
    if (s) return s;

    transactionBegin();
    OQL q(this, "for (y in (select %s->user->name = \"%s\")) delete y",
	  sysuseraccess_s, username);

    s = q.execute();
    if (s) {
      transactionAbort();
      return s;
    }
    s = sysaccess->realize();

    transactionCommit();

    sysaccess->release();
    user->release();

    if (s)
      return Exception::make(IDB_SET_USER_SYSACCESS_ERROR,
			     "user entry '%s' : %s",
			     username, s->getString());
    return Success;
  }

  Status DBM_Database::addUser(Connection *ch,
			       const char *username, const char *passwd,
			       UserType user_type,
			       const char *userauth, const char *passwdauth)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_ADD_USER_ERROR);

    conn = ch;
    check_auth(userauth, passwdauth, "adding user");

    RPCStatus rpc_status;
    rpc_status = userAdd(ConnectionPeer::getConnH(conn),
			 dbmdb_str, userauth, passwdauth,
			 username, passwd, user_type);
    return StatusMake(rpc_status);
  }

  Status DBM_Database::deleteUser(Connection *ch,
				  const char *username,
				  const char *userauth,
				  const char *passwdauth)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_DELETE_USER_ERROR);

    conn = ch;
    check_auth(userauth, passwdauth, "deleting user");

    RPCStatus rpc_status;
    rpc_status = userDelete(ConnectionPeer::getConnH(conn),
			    dbmdb_str, userauth, passwdauth,
			    username);
    return StatusMake(rpc_status);
  }

  Status DBM_Database::setUserPasswd(Connection *ch,
				     const char *username,
				     const char *passwd,
				     const char *userauth,
				     const char *passwdauth)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_SET_USER_PASSWD_ERROR);

    conn = ch;
    check_auth(userauth, passwdauth, "seting user passwd");

    RPCStatus rpc_status;
    rpc_status = userPasswdSet(ConnectionPeer::getConnH(conn),
			       dbmdb_str, userauth, passwdauth,
			       username, passwd);
    return StatusMake(rpc_status);
  }

  Status DBM_Database::setPasswd(Connection *ch,
				 const char *username, const char *passwd,
				 const char *newpasswd)
		     
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_SET_PASSWD_ERROR);

    conn = ch;
    RPCStatus rpc_status;
    rpc_status = passwdSet(ConnectionPeer::getConnH(conn),
			   dbmdb_str, username, passwd, newpasswd);
    return StatusMake(rpc_status);
  }

  Status DBM_Database::setUserSysAccess(Connection *ch,
					const char *username,
					SysAccessMode mode,
					const char *userauth,
					const char *passwdauth)
  {
    if (!dbmdb_str)
      return invalidDbmdb(IDB_SET_USER_SYSACCESS_ERROR);

    conn = ch;
    check_auth(userauth, passwdauth, "setting user sys access");

    RPCStatus rpc_status;
    rpc_status = userSysAccessSet(ConnectionPeer::getConnH(conn),
				  dbmdb_str, userauth, passwdauth,
				  username, mode);
    return StatusMake(rpc_status);
  }

#define dbmupdate_str "*I*D*B*D*B*M*"

  Status DBM_Database::create(Connection *ch, const char *passwdauth,
			      const char *username, const char *passwd,
			      DbCreateDescription *pdbdesc)
  {
    RPCStatus rpc_status;
    DbCreateDescription dbdesc;

    if (!dbmdb_str)
      return invalidDbmdb(IDB_DATABASE_CREATE_ERROR);

    if (!passwdauth)
      passwdauth = Connection::getDefaultUser();

    if (!passwdauth)
      return Exception::make(IDB_AUTHENTICATION_NOT_SET, "creating DBM database %s",
			     dbmdb_str);

    create_prologue(dbdesc, &pdbdesc);

    rpc_status = dbmCreate(ConnectionPeer::getConnH(ch), dbmdb_str,
			   passwdauth, pdbdesc);

    if (rpc_status == RPCSuccess)
      {
	conn = ch;

	delete _user;
	_user = strdup(dbmupdate_str);
	delete _passwd;
	_passwd = strdup(passwd);

	Status status;
	status = init_db(ch);
	if (status)
	  return status;

	delete _user;
	_user = strdup(username);

	rpc_status = dbmUpdate(ConnectionPeer::getConnH(ch), dbmdb_str,
			       username, passwd);
      
	return StatusMake(rpc_status);
      }

    return StatusMake(rpc_status);
  }

  Status DBM_Database::create()
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "DBM_Database::create");
  }

  Status DBM_Database::update()
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "DBM_Database::update");
  }

  Status DBM_Database::realize(const RecMode*)
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "DBM_Database::realize");
  }

  Status DBM_Database::remove(const RecMode*)
  {
    return Exception::make(IDB_NOT_YET_IMPLEMENTED, "DBM_Database::remove");
  }

  DBM_Database::~DBM_Database()
  {
  }
}
