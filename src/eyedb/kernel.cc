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


//#define PASSWD_FILE

using namespace std;

#define NEW_COLLBE_MAN

#define _IDB_KERN_

#include <eyedbconfig.h>

// @@@ ???????????????
// #define private public

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#include "eyedb_p.h"
#include "eyedb/DBM_Database.h"
#include <eyedbsm/eyedbsm.h>
//#include <eyedbsm/eyedbsm_p.h>
#include "serv_lib.h"
#include "kernel.h"
#include "kernel_p.h"
#include "IteratorBE.h"
#include "CollectionBE.h"
#include "OQLBE.h"
#include "BEQueue.h"
#include "InvOidContext.h"
#include <eyedblib/m_malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include "Attribute_p.h"
#include "version_p.h"

#include <eyedblib/rpc_lib.h>

#include "eyedb/SessionLog.h"
#include <new>

#define COLLBE_BTREE

#define RCHECK(S) \
if (S)  \
{ \
  IDB_free(db, collbe); \
  return rpcStatusMake_se(S); \
}

namespace eyedbsm {
  void mutexes_release();
  extern Boolean backend;
  Status dbClose(const eyedbsm::DbHandle *);
}

namespace eyedb {

  struct ConnContext {
    SessionLog *sesslog;
    ClientSessionLog *clinfo;

    std::string challenge;
    int uid;
    time_t ctime;
    rpc_ConnInfo *ci;
  };

  static ConnContext conn_ctx;

  static const char *edb_passwdfile;

#ifdef PASSWD_FILE
  static void
  get_dbm_passwd(const char *passwdfile);
#endif

  /*
    static char *
    crypt(const char *passwd, const char *salt)
    {
    return crypt(passwd, salt);
    }
  */

  //static const char sch_default_name[] = "eyedb";

  // introduced the 17/12/99
#define LOAD_RM_BUG

#ifdef PASSWD_FILE
  static char dbm_passwd[14];
#endif

  static const char salt[] = "r8";

  static LinkedList st_inv_oid_ctx;
  LinkedList *inv_oid_ctx;

  static eyedbsm::Oid *
  decode_oids(Data data, unsigned int *oid_cnt);

  static const char **
  decode_datids(Data data, unsigned int *oid_cnt);

  static Data
  code_locarr(const eyedbsm::ObjectLocation *locarr, const eyedbsm::Oid *oids,
	      unsigned int cnt, int *size);

  static Data
  code_datinfo(const eyedbsm::DatafileInfo *info, int *size);

  static void
  code_value(rpc_ServerData *, Value *);

  static void
  make_locarr(const eyedbsm::ObjectLocation *se_locarr, const eyedbsm::Oid *oids,
	      unsigned int oid_cnt, void *locarr);
  static void
  make_datinfo(const eyedbsm::DatafileInfo *se_datinfo, Data *info);

  static void
  code_atom_array(rpc_ServerData *, void *, int, int);

  static Data
  code_index_stats(IndexImpl::Type, const void *, int *);

  static void
  make_index_stats(const eyedbsm::HIdx::Stats &, Data *);

  static void
  make_index_stats(const eyedbsm::BIdx::Stats &, Data *);

  static Data
  code_index_impl(const eyedbsm::HIdx::_Idx &, int *);

  static Data
  code_index_impl(const eyedbsm::BIdx &, int *);

  static void
  make_index_impl(const eyedbsm::HIdx::_Idx &, Data *);

  static void
  make_index_impl(const eyedbsm::BIdx &, Data *);

  static RPCStatus
  IDB_schemaClassHeadGet(DbHandle *),
    IDB_schemaClassCreate(DbHandle *),
    IDB_instanceCreate(DbHandle *, short, Data, ObjectHeader *,
		       eyedbsm::Oid *, void *, Bool),
    IDB_instanceRead(DbHandle *dbh, Data idr, Data *pidr,
		     ObjectHeader *hdr,
		     LockMode lockmode, const eyedbsm::Oid *oid, void *xdata,
		     int minsize),
    IDB_instanceWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		      const eyedbsm::Oid *oid, void *xdata),
    IDB_instanceDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		       const eyedbsm::Oid *oid, Bool really = False),
    IDB_agregatCreate(DbHandle *dbh, short dspid, Data idr,
		      ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata),
    IDB_agregatWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		     const eyedbsm::Oid *oid, void *xdata),
    IDB_agregatDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		      const eyedbsm::Oid *oid, Bool really = False),
    IDB_classCreate(DbHandle *, short dspid, Data, ObjectHeader *,
		    eyedbsm::Oid *, void *),
    IDB_classWrite(DbHandle *, Data, ObjectHeader *,
		   const eyedbsm::Oid *, void *),
    IDB_classDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		    const eyedbsm::Oid *oid, unsigned int flags),
    IDB_collectionCreate(DbHandle *, short dspid, Data,
			 ObjectHeader *, eyedbsm::Oid *, void *),
    IDB_collectionWrite(DbHandle *, Data, ObjectHeader *,
			const eyedbsm::Oid *, void *),
    IDB_collectionRead(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		       LockMode lockmode, const eyedbsm::Oid *oid, void *xdata),
    IDB_collectionDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr,
			 const eyedbsm::Oid *oid),
    IDB_protectionCreate(DbHandle *, short dspid, Data,
			 ObjectHeader *, eyedbsm::Oid *, void *, Bool),
    IDB_protectionWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr,
			const eyedbsm::Oid *oid, void *xdata),
    IDB_protectionDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr,
			 const eyedbsm::Oid *oid);
  /*
    IDB_collClassCreate(DbHandle *, short dspid, Data, const eyedbsm::Oid *,
			const char *, int),
    IDB_collClassUpdate(DbHandle *, Data, const eyedbsm::Oid *, 
			const char *, Bool insert);
  */

  static LinkedList *db_list;

  void IDB_releaseConn();

  void (*garbage_handler)(void);
  void (*settimeout)(int);

  static void
  quit_handler(void *quit_data, int fromcore)
  {
    Exception::setMode(Exception::StatusMode);

    eyedbsm::mutexes_release();
    IDB_releaseConn();

    LinkedList *list = (LinkedList *)quit_data;
    LinkedListCursor c(list);

    if (!fromcore) {
      Database *db;
      void (*handler)(Status, void *) = Exception::getHandler();

      Exception::setHandler(0);

      while (c.getNext((void *&)db)) {
	for (;;) {
	  Status status = db->transactionAbort();
	  if (status)
	    break;
	}
      }
      
      Exception::setHandler(handler);
    }

    if (garbage_handler) {
      garbage_handler();
      eyedb::release();
    }
  }

#define TRIGGER_MANAGE(DB, TTYPE, HDR, IDR, OID, MCL) \
{ \
  Status status = IDB_triggerManage(DB, TTYPE, HDR, IDR, OID, MCL); \
\
   if (status) \
     return rpcStatusMake(status); \
}

  static Status
  IDB_triggerManage(Database *db, int w, const ObjectHeader *hdr,
		    Data idr, const eyedbsm::Oid *oid, const Class *cls)
  {
    if (!db || !cls ||
	cls->asCollectionClass() || cls->isSystem()) // added the 5/01/01
      return Success;

    const LinkedList *list;

    try {
      list = cls->getCompList((Class::CompIdx)w);
    }
    catch (Exception &e) {
      return Exception::make(e.getStatus(), e.getDesc());
    }

    if (list)
      {
	Oid obj_oid(oid);
	Object *o = 0;
	Status status;
	Trigger *trigger;

	/*
	  printf("THERE IS SOME ACTION TO DO for '%s' %d, %s\n",
	  cls->getName(), w, obj_oid.getString());
	*/

	if (idr && obj_oid.isValid())
	  {
	    LinkedListCursor c(list);
	    while (list->getNextObject(&c, (void *&)trigger))
	      if (!trigger->getLight())
		{
		  status = db->makeObject(&obj_oid, hdr, idr, &o, True);
		  if (status)
		    return status;

		  break;
		}
	  }

	LinkedListCursor c(list);
	while (list->getNextObject(&c, (void *&)trigger))
	  {
	    status = trigger->apply(obj_oid, o);
	    if (o && (trigger->getType() & 1) && idr != o->getIDR())
	      memcpy(idr, o->getIDR(), hdr->size);

	    if (status)
	      {
		if (o) o->release();
		return status;
	      }
	  }
	if (o) o->release();
      }

    return Success;
  }

  static Status IDB_makeDatabase(ConnHandle *, const char *,
				 const char *, int, int, DbHandle *,
				 rpcDB_LocalDBContext *, const eyedbsm::Oid *,
				 unsigned int version, Database *&);

#define IS_DATA(X) ((X) && (X)->fd >= 0)

  static void lock_data(Data *pidr, void *xdata)
  {
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (IS_DATA(data))
      {
	Data temp = (Data)malloc(data->size);

	if (temp)
	  {
	    rpc_socketRead(data->fd, temp, data->size);
	    *pidr = temp;
	  }
      }
  }

  static void unlock_data(Data idr, void *xdata)
  {
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (IS_DATA(data))
      free(idr);
  }

  RPCStatus
  IDB_checkAuth(const char *file)
  {
    // check file name
    const char *p = strrchr(file, '/');
    if (p)
      p++;
    else
      p = file;

    if (strcmp(p, strrchr(conn_ctx.challenge.c_str(), '.') + 1))
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid file name: "
			 "%s", file));

    int fd = open(file, O_RDONLY);
    if (fd < 0)
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "cannot open authentification file: "
			 "%s", file));

    char buf[1024];
    int n = read(fd, buf, sizeof buf);
    if (n <= 0) {
      close(fd);
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "cannot read authentification file: "
			 "%s", file));
    }

    buf[n] = 0;

    // check challenge
    if (strcmp(buf, conn_ctx.challenge.c_str())) {
      close(fd);
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid challenge in authentification file: "
			 "%s", file));
    }

    struct stat rstat;
    int r = fstat(fd, &rstat);
    close(fd);

    if (r)
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "cannot stat authentification file: "
			 "%s", file));

    // check uid
    if (conn_ctx.uid != rstat.st_uid)
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid uid authentification file: "
			 "%s", file));

    // check time
    if (rstat.st_ctime < conn_ctx.ctime-1)
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid creation time for "
			 "authentification file: "
			 "%s (%u >= %u)", file, rstat.st_ctime,
			 conn_ctx.ctime));

    // check mode
    if (0664 != (rstat.st_mode & 0777))
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid mode 0%o for authentification file: "
			 "%s", rstat.st_mode & 0777, file));
    
    conn_ctx.ci->auth.uid = conn_ctx.uid;
    return RPCSuccess;
  }

  static int
  get_rand()
  {
    //#define SUPPORT_DEV_RANDOM
#ifdef SUPPORT_DEV_RANDOM
    int fd = open("/dev/random", O_RDONLY);
    if (fd >= 0) {
      unsigned int i = 0;
      time_t t0;
      time(&t0);
      printf("before read\n");
      int r = read(fd, &i, sizeof(int));
      printf("r = %d %d %d\n", r, i, time(0)-t0);
      close(fd);
      return i;
    }
    return 0;
#else
    static int n = 1;
    struct timeval tv;
    gettimeofday(&tv, 0);
    srand(tv.tv_usec + (tv.tv_sec % n));
    n++;
    return rand();
#endif
  }

  static std::string
  gen_random()
  {
    struct timeval tv;
    int r0, r1;
    char buf[512];

    r0 = get_rand();

    // check for file name
    bool found = false;
    for (int n = 0 ; n < 100; n++) {
      r1 = get_rand();
      sprintf(buf, "/tmp/%d", r1);
      int fd = open(buf, O_RDONLY);
      if (fd < 0) {
	found = true;
	break;
      }
      close(fd);
    }
    
    if (!found)
      return "";

    gettimeofday(&tv, 0);
    sprintf(buf, "%d.%06d.%d.%d", tv.tv_sec, tv.tv_usec, r0, r1);

    return buf;
  }

  RPCStatus
  IDB_setConnInfo(const char *_hostname, int uid, const char *username,
		  const char *progname, int pid, int *sv_pid,
		  int *sv_uid, int cli_version, char **challenge)
  {
    char *host = strdup(_hostname);
    char *p = strchr(host, ':');
    *challenge = (char*)""; /*@@@@warning cast*/
    if (!p) {
      free(host);
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "invalid hostname, got '%s' "
			 "expected host:port",
			 _hostname));
    }

    *p = 0;
    char *port = p + 1;

    IDB_LOG(IDB_LOG_CONN,
	    ("connected host='%s:%s', username='%s', progname='%s', pid=%d\n",
	     host, port, username, progname, pid));

    *sv_pid = rpc_getpid();
    *sv_uid = getuid();

    if (cli_version != eyedb::getVersionNumber())
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "incompatible versions: "
			 "client version is %s, server version is %s",
			 eyedb::convertVersionNumber(cli_version),
			 eyedb::getVersion()));

    struct passwd *pwd = 0;

    // if (conn_ctx.ci && conn_ctx.ci->mode == rpc_ConnInfo::UNIX) {
    if (conn_ctx.ci && conn_ctx.ci->is_localhost &&
	(pwd = getpwnam(username))) {
      conn_ctx.challenge = gen_random();
      conn_ctx.ctime = time(0);
      conn_ctx.uid = pwd->pw_uid;
    }
    else {
      conn_ctx.challenge = "";
      conn_ctx.ctime = 0;
      conn_ctx.uid = -1;
    }

    *challenge = (char *)conn_ctx.challenge.c_str();

    return rpcStatusMake(conn_ctx.sesslog->add(host, port, username,
					       progname, pid, conn_ctx.clinfo));
  }

  void
  IDB_releaseConn()
  {
    /*
      if (conn_ctx.clinfo)
      fprintf(stderr, "release conn CONN_CTX.CLINFO is OK %d\n", rpc_getpid());
      else
      fprintf(stderr, "release conn CONN_CTX.CLINFO is NULL %d\n", rpc_getpid());
    */

    SessionLog::release();

    if (!conn_ctx.clinfo)
      return;

    conn_ctx.sesslog->suppress(conn_ctx.clinfo);
    conn_ctx.clinfo = 0;
  }

  static RPCStatus
  IDB_checkAuthDbm(const char *&dbmdb)
  {
#if 1
    std::string dbmdb_s;
    if (!strcmp(dbmdb, DBM_Database::defaultDBMDB)) {
      dbmdb = strdup(Database::getDefaultServerDBMDB());
    }
    const std::vector<string> &granted_dbm = Database::getGrantedDBMDB();
    int size = granted_dbm.size();
    for (int n = 0; n < size; n++) {
      if (!strcmp(granted_dbm[n].c_str(), dbmdb))
	return RPCSuccess;
    }
#else
    if (!strcmp(dbmdb, Database::getDefaultDBMDB()))
      return RPCSuccess;
#endif

    // TBD: should improved that!
    // should check whether `dbmdb' belongs to an authoritative list!
    return rpcStatusMake(IDB_ERROR,
			 "security violation: cannot use "
			 "'%s' EYEDBDBM database.", dbmdb);
  }

  static RPCStatus
  IDB_dbmOpen(ConnHandle *ch, const char *dbmdb,
	      Bool rw_mode, DBM_Database **dbm)
  {
    /*
    printf("IDB_dbmOpen(%s) #1\n", dbmdb);

#if 1
    if (!strcmp(dbmdb, DBM_Database::defaultDBMDB))
      dbmdb = Database::getDefaultServerDBMDB();
#else
    if (!strcmp(dbmdb, DBM_Database::defaultDBMDB))
      dbmdb = Database::getDefaultDBMDB();
#endif

    printf("IDB_dbmOpen(%s) #2\n", dbmdb);
    */

    if (*dbm = DBM_Database::getDBM_Database(dbmdb))
      {
	if (!rw_mode || ((*dbm)->getOpenFlag() & _DBRW))
	  return RPCSuccess;

	/* added the 8/04/01 because on linux one cannot have 1 database opened
	   and 2 EYEDBDBM opened (for instance to move a database) */
	Status s = (*dbm)->close();
	if (s) return rpcStatusMake(s);
	(void)DBM_Database::setDBM_Database(dbmdb, (Database *)0);
      }

    int rdbid;
    char *rname;
    RPCStatus rpc_status;
    DbHandle *dbh;
    int pid;

    if (rpc_status = IDB_checkAuthDbm(dbmdb))
      return rpc_status;

    rpc_status = IDB_dbOpen(ch, dbmdb, (char *)0, (char *)0,
			    DBM_Database::getDbName(),
			    DBM_Database::getDbid(),
			    ((rw_mode ? _DBRW : (_DBSRead)) |
			     _DBOpenLocal),
			    0, 0,
			    &pid, NULL, 0, &rname,
			    &rdbid, 0, &dbh);

    if (rpc_status == RPCSuccess)
      *dbm = DBM_Database::setDBM_Database(dbmdb, (Database *)dbh->db);

    return rpc_status;
  }

  static RPCStatus get_file_mask_group(mode_t &file_mask, const char *&file_group)
  {
    file_mask = 0;
    const char *file_mask_str = ServerConfig::getSValue("default_file_mask");
    if (file_mask_str && strcmp(file_mask_str, "0")) {
      unsigned int fmask = 0;
      sscanf(file_mask_str, "%o", &fmask);
      if (!fmask) {
	return rpcStatusMake(IDB_ERROR, "invalid file mode: %s", file_mask_str);
      }
      file_mask = fmask;
    }

    file_group = ServerConfig::getSValue("default_file_group");
    return RPCSuccess;
  }

  RPCStatus
  IDB_dbCreate_realize(ConnHandle *ch, DBM_Database *dbm, int dbid,
		       const char *dbmdb, const char *userauth,
		       const char *passwdauth, const char *dbname,
		       const char *dbfile, Bool makeTemp,
		       const eyedbsm::DbCreateDescription *sedbdesc)
  {
    eyedbsm::Status se_status;

    ((eyedbsm::DbCreateDescription *)sedbdesc)->dbid = dbid;

    std::string dbs;
    if (*dbfile != '/') {
      dbs = string(ServerConfig::getSValue("databasedir")) + "/" + dbfile;
      dbfile = dbs.c_str();
    }

    mode_t file_mask;
    const char *file_group;
    RPCStatus rpc_status = get_file_mask_group(file_mask, file_group);
    if (rpc_status)
      return rpc_status;

    se_status = eyedbsm::dbCreate(dbfile, eyedb::getVersionNumber(),
				  sedbdesc, file_mask, file_group);

    if (!se_status) {
	RPCStatus rpc_status;
	int pid;
	DbHandle *dbh = 0;
	Status status;

	if (dbm && makeTemp) {
	    status = dbm->updateEntry(dbid,
				      DBM_Database::makeTempName(dbid).c_str(),
				      dbname, dbfile);

	    if (status)
	      return rpcStatusMake(status);
	  }

	if (strcmp(dbname, DBM_Database::getDbName()) &&
	    (rpc_status = IDB_userDBAccessSet(ch, dbmdb, (char *)0, (char *)0,
					      dbname, userauth,
					      AdminDBAccessMode)) !=
	    RPCSuccess) {
	  //printf("rpc_status %p\n", rpc_status);
	  return rpc_status;
	}

	int rdbid;
	char *rname;
	int xflags = _DBRW|_DBOpenLocal;
	if ((rpc_status = IDB_dbOpen(ch, dbmdb, userauth, passwdauth, dbname, dbid,
				     xflags, 0, 0, &pid, NULL,
				     0, &rname, &rdbid, 0, &dbh)) !=
	    RPCSuccess && rpc_status->err == IDB_INVALID_SCHEMA)
	  {
	    if ((rpc_status = IDB_transactionBegin(dbh, 0, True))
		== RPCSuccess)
	      {
		rpc_status = IDB_schemaClassCreate(dbh);
		IDB_transactionCommit(dbh, True);
	      }

	    if (dbm && !rpc_status)
	      {
		Oid schoid(dbh->sch.oid);
		dbm->setSchema(dbname, schoid);
	      }

	  }
      
	if (dbh && !IDB_dbClose(dbh) && conn_ctx.clinfo)
	  conn_ctx.clinfo->suppressDatabase(dbname, userauth, xflags);

	return rpc_status;
      }
    else {
      if (dbm)
	dbm->removeEntry(makeTemp ? DBM_Database::makeTempName(dbid).c_str() : std::string(dbname).c_str());
      return rpcStatusMake_se(se_status);
    }
  }

  RPCStatus
  IDB_dbmCreate(ConnHandle *ch, const char *dbmdb,
		const char *passwdauth, const DbCreateDescription *dbdesc)
  {
    if (!access(dbmdb, F_OK))
      return rpcStatusMake(IDB_ERROR, "DBM_Database database '%s' already exists", dbmdb);

    // check file $IDBSVPASSWDFILE against passwdauth

    /*
      if (!*dbm_passwd) {
      const char *passwdfile;
      const char *s;
      if (passwdfile)
      passwdfile = strdup(passwdfile);
      else if (s = eyedb::getConfigValue("passwd_file"))
      passwdfile = strdup(s);
      else {
      fprintf(stderr, "eyedbd: EyeDB passwd file is not set, check your 'sv_passwd_file' configuration variable or the '-passwdfile' command line option\n");
      exit(1);
      }
      
      get_dbm_passwd(passwdfile);
      }
    */

    RPCStatus rpc_status;
#ifdef PASSWD_FILE
    if (strcmp(crypt(passwdauth, salt), dbm_passwd))
      return rpcStatusMake(Exception::make(IDB_AUTHENTICATION_FAILED,
					   "not a valid password to create a DBM database"));
#else
    rpc_status = IDB_checkAuthDbm(dbmdb);
    if (rpc_status)
      return rpc_status;
#endif

    rpc_status = IDB_dbCreate_realize(ch, 0, DBM_Database::getDbid(), dbmdb,
				      (char *)0, (char *)0, DBM_Database::getDbName(), dbmdb, False, &dbdesc->sedbdesc);

    /*
      if (!rpc_status)
      status = dbm->updateEntry(dbid, DBM_Database::makeTempName(dbid),
      dbname, dbfile);
    */

    return rpc_status;
  }

  RPCStatus
  IDB_dbmUpdate(ConnHandle *ch, const char *dbmdb,
		const char *username, const char *passwd)
  {
    DBM_Database *dbm;
    RPCStatus rpc_status;
    Status status;

    rpc_status = IDB_dbmOpen(ch, dbmdb, True, &dbm);
  
    if (rpc_status != RPCSuccess)
      return rpc_status;

    status = dbm->createEntry(DBM_Database::getDbid(),
			      DBM_Database::getDbName(), dbmdb);

    if (status)
      return rpcStatusMake(status);

    int user_type;
    if (!strcmp(passwd, strict_unix_user)) {
      passwd = "";
      user_type = StrictUnixUser;
    } else
      user_type = EyeDBUser;
    
    rpc_status = IDB_userAdd(ch, dbmdb, (char *)0, (char *)0, username, passwd,
			     user_type);

    if (rpc_status != RPCSuccess)
      return rpc_status;

    rpc_status = IDB_userSysAccessSet(ch, dbmdb, (char *)0, (char *)0,
				      username, SuperUserSysAccessMode);
    if (rpc_status != RPCSuccess)
      return rpc_status;

    return RPCSuccess;
  }

  //#undef IS_DATA

  static RPCStatus
  IDB_checkSysAuth(ConnHandle *ch, const char *dbmdb,
		   const char *&userauth, const char *&passwdauth,
		   SysAccessMode sysmode, Bool rw_mode,
		   DBM_Database **pdbm, const char *msg, Bool * = 0);

  RPCStatus
  IDB_dbCreate(ConnHandle *ch, const char *dbmdb,
	       const char *userauth, const char *passwdauth,
	       const char *dbname, const DbCreateDescription *dbdesc)
  {
    DBM_Database *dbm;
    RPCStatus rpc_status;

    std::string msg = std::string("creating database '") + dbname + "'";
    if ((rpc_status = IDB_checkSysAuth(ch, dbmdb, userauth, passwdauth,
				       DBCreateSysAccessMode, True, &dbm,
				       msg.c_str())) !=
	RPCSuccess)
      return rpc_status;


    const char *xdbfile;
    Status s = dbm->getDbFile(&dbname, 0, xdbfile);
    if (s) return rpcStatusMake(s);

    if (xdbfile)
      return rpcStatusMake(IDB_ERROR, "database '%s' already exists", dbname);

    int newid;
    Bool makeTemp;
    if (dbdesc->sedbdesc.dbid)
      {
	newid = dbdesc->sedbdesc.dbid;
	makeTemp = False;
	Status status = dbm->createEntry(newid, dbname, dbdesc->dbfile);
	if (status)
	  return rpcStatusMake(status);
      }
    else
      {
	if (s = dbm->getNewDbid(newid))
	  return rpcStatusMake(s);
	makeTemp = True;
      }

    rpc_status = IDB_dbCreate_realize(ch, dbm, newid, dbmdb,
				      userauth, passwdauth, dbname,
				      dbdesc->dbfile, makeTemp,
				      &dbdesc->sedbdesc);
    if (rpc_status)
      return rpc_status;

    return RPCSuccess;
  }

  /*static*/ RPCStatus
  IDB_checkDBAuth(ConnHandle *ch, const char *dbmdb,
		  const char *dbname, const char *&userauth,
		  const char *&passwdauth, DBAccessMode dbmode,
		  Bool rw_mode, int *puid, DBM_Database **pdbm,
		  const char *msg);

  RPCStatus
  IDB_dbDelete(ConnHandle *ch, const char *dbmdb, const char *userauth,
	       const char *passwdauth, const char *dbname)
  {
    if (!strcmp(dbname, DBM_Database::getDbName()))
      return rpcStatusMake_se(eyedbsm::dbDelete(dbmdb));

    DBM_Database *dbm;
    RPCStatus rpc_status = IDB_dbmOpen(ch, dbmdb, True, &dbm);
  
    if (rpc_status != RPCSuccess)
      return rpc_status;

    const char *dbfile;
    Status s = dbm->getDbFile(&dbname, 0, dbfile);
    if (s) return rpcStatusMake(s);

    if (!dbfile)
      return rpcStatusMake(IDB_ERROR, "database '%s' does not exists", dbname);
    if ((rpc_status = IDB_checkDBAuth(ch, dbmdb, dbname, userauth, passwdauth,
				      AdminDBAccessMode, False, NULL, NULL,
				      "deleting database")) !=
	RPCSuccess)
      return rpc_status;

    eyedbsm::Status se_status = eyedbsm::dbDelete(dbfile);
  
    if (se_status == eyedbsm::Success)
      {
	Status status = dbm->removeEntry(dbname);
	if (status)
	  return rpcStatusMake(status);
      }
  
    return rpcStatusMake_se(se_status);
  }

  static RPCStatus
  IDB_getDbFile(ConnHandle *ch, const char **pdbname, const char *dbmdb,
		const char **dbfile, int *dbid, Bool rw_mode,
		DBM_Database **pdbm = 0)
  {
    RPCStatus rpc_status = IDB_checkAuthDbm(dbmdb);
    if (rpc_status)
      return rpc_status;

    if (strcmp(*pdbname, DBM_Database::getDbName())) {
      DBM_Database *dbm;

      rpc_status = IDB_dbmOpen(ch, dbmdb, rw_mode, &dbm);
  
      if (rpc_status != RPCSuccess)
	return rpc_status;

      if (pdbm)
	*pdbm = dbm;

      //eyedblib::display_time("#9");
      Status s = dbm->getDbFile(pdbname, dbid, *dbfile);
      //eyedblib::display_time("#10");
      if (s)
	return rpcStatusMake(s);

      if (!*dbfile) {
	if ((*pdbname)[0])
	  return rpcStatusMake(IDB_ERROR,
			       "cannot open database '%s'", *pdbname);
	else
	  return rpcStatusMake(IDB_ERROR,
			       "cannot open database dbid #%d", *dbid);
      }
    }
    else {
      /*
#if 1
	if (!strcmp(dbmdb, DBM_Database::defaultDBMDB))
	  dbmdb = Database::getDefaultServerDBMDB();
#else
	if (!strcmp(dbmdb, DBM_Database::defaultDBMDB))
	  dbmdb = Database::getDefaultDBMDB();
#endif
      */

	/*
	RPCStatus rpc_status = IDB_checkAuthDbm(dbmdb);
	if (rpc_status)
	  return rpc_status;
	*/

	*dbfile = dbmdb;

	if (dbid)
	  *dbid = DBM_Database::getDbid();

	if (pdbm) {
	  // added 16/01/06
	  RPCStatus rpc_status;
	  DBM_Database *dbm;

	  rpc_status = IDB_dbmOpen(ch, dbmdb, rw_mode, &dbm);
	  if (rpc_status != RPCSuccess)
	    return rpc_status;
	  *pdbm = dbm;
	}
      }

    return RPCSuccess;
  }

  RPCStatus
  IDB_dbMoveCopy(ConnHandle *ch, const char *dbmdb,
		 const char *userauth, const char *passwdauth,
		 const char *dbname,
		 const char *newdbname, const DbCreateDescription *dbdesc,
		 Bool copy)
  {
    const char *dbfile;
    int dbid;

    DBM_Database *dbm;
    RPCStatus rpc_status = IDB_getDbFile(ch, &dbname, dbmdb, &dbfile, &dbid,
					 True, &dbm);
    if (rpc_status)
      return rpc_status;

    const char *xdbfile;
    Status s = dbm->getDbFile(&newdbname, 0, xdbfile);
    if (s)
      return rpcStatusMake(s);
    if (copy && xdbfile)
      return rpcStatusMake(IDB_ERROR,
			   "database '%s' already exists", newdbname);

    eyedbsm::DbMoveDescription mvcpdesc;
    strcpy(mvcpdesc.dbfile, dbdesc->dbfile);
    mvcpdesc.dcr = dbdesc->sedbdesc;
    eyedbsm::Status se_status;

    if (copy)
      se_status = eyedbsm::dbCopy(dbfile, &mvcpdesc);
    else
      se_status = eyedbsm::dbMove(dbfile, &mvcpdesc);

    if (se_status)
      return rpcStatusMake_se(se_status);

    eyedbsm::DbRelocateDescription rldesc;
    rldesc.ndat = mvcpdesc.dcr.ndat;

    for (int i = 0; i < rldesc.ndat; i++)
      strcpy(rldesc.file[i], mvcpdesc.dcr.dat[i].file);

    Status status;

    if (!copy)
      {
	status = dbm->removeEntry(dbname);
	if (status)
	  return rpcStatusMake(status);
      }

    status = dbm->createEntry(dbid, newdbname, mvcpdesc.dbfile);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_dbMove(ConnHandle *ch, const char *dbmdb,
	     const char *userauth, const char *passwdauth, const char *dbname,
	     const DbCreateDescription *dbdesc)
  {
    if (!strcmp(dbmdb, DBM_Database::getDbName()))
      return rpcStatusMake(Exception::make(IDB_ERROR, "cannot move %s database, use the unix tool 'mv' and update your configuration file",
					   DBM_Database::getDbName()));  
    return IDB_dbMoveCopy(ch, dbmdb, userauth, passwdauth, dbname, dbname, dbdesc, False);
  }

  RPCStatus
  IDB_dbCopy(ConnHandle *ch, const char *dbmdb,
	     const char *userauth, const char *passwdauth, const char *dbname,
	     const char *newdbname, Bool newdbid,
	     const DbCreateDescription *dbdesc)
  {
    return IDB_dbMoveCopy(ch, dbmdb, userauth, passwdauth, dbname, newdbname,
			  dbdesc, True);
  }

  RPCStatus
  IDB_dbRename(ConnHandle *ch, const char *dbmdb,
	       const char *userauth, const char *passwdauth,
	       const char *dbname,  const char *newdbname)
  {
    if (!strcmp(dbname, DBM_Database::getDbName()))
      return rpcStatusMake(IDB_DATABASE_RENAME_ERROR, "cannot rename a DBM_Database database");

    DBM_Database *dbm;
    RPCStatus rpc_status;

    rpc_status = IDB_dbmOpen(ch, dbmdb, True, &dbm);
  
    if (rpc_status != RPCSuccess)
      return rpc_status;

    int dbid;
    const char *dbfile;
    Status s = dbm->getDbFile(&dbname, &dbid, dbfile);
    if (s) return rpcStatusMake(s);

    if (!dbfile)
      return rpcStatusMake(IDB_DATABASE_RENAME_ERROR, "cannot open database '%s'", dbname);

    if ((rpc_status = IDB_checkDBAuth(ch, dbmdb, dbname, userauth, passwdauth,
				      AdminDBAccessMode, True, NULL, NULL,
				      "renaming database")) !=
	RPCSuccess)
      return rpc_status;

   const char *ndbfile;
    s = dbm->getDbFile(&newdbname, 0, ndbfile);
    if (s) return rpcStatusMake(s);

    if (ndbfile)
      return rpcStatusMake(IDB_DATABASE_RENAME_ERROR,
			   "database '%s' already exists",
			   newdbname);
    return rpcStatusMake(dbm->updateEntry(dbid, dbname, newdbname, dbfile));
  }

  /* administration */

  static SessionLog *sesslogTempl;

  void setConnInfo(rpc_ConnInfo *ci)
  {
    conn_ctx.ci = ci;
    conn_ctx.sesslog = new SessionLog(*sesslogTempl);
    //rpc_print_tcpip(stdout, &conn_ctx.ci->u.tcpip);
  }

  //#define STUART_SUGGESTION

  static const char *
  getUserAuth(const char *username, Bool &need_passwd,
	      Bool &superuser)
  {
    int i;
    rpc_TcpIp *tcpip;
    need_passwd = True;
    superuser = False;

    if (conn_ctx.ci && conn_ctx.ci->auth.uid >= 0) {
      int uid = (conn_ctx.ci ? conn_ctx.ci->auth.uid : getuid());
      passwd *pwd = getpwuid(uid);
      const char *authusername = (pwd ? pwd->pw_name : "");
      if (!*username || !strcmp(username, authusername)) {
	need_passwd = False;
#ifdef STUART_SUGGESTION
	if (uid == getuid())
	  superuser = True;
#endif
	return strdup(authusername); // memory leak (not important)
      }
      
      return username;
    }

    if (true) { //conn_ctx.ci->mode == rpc_ConnInfo::TCPIP) {
      const char *chuser = 0;
      tcpip = &conn_ctx.ci->tcpip;

      for (i = 0; i < tcpip->user_cnt; i++) {
	if (!*username) {
	  if (tcpip->users[i].mode == rpc_User::DEF)
	    chuser = tcpip->users[i].user;
	}
	else if (!strcmp(username, tcpip->users[i].user)) {

	  if (tcpip->users[i].mode == rpc_User::ON ||
	      tcpip->users[i].mode == rpc_User::DEF)
	    chuser = username;
	  else if (tcpip->users[i].mode == rpc_User::NOT)
	    chuser = 0;
	}
	else if (tcpip->users[i].mode == rpc_User::ALL ||
		 tcpip->users[i].mode == rpc_User::NOT)
	  chuser = username;
      }
      
      return chuser;
    }

    return username;
  }

  static RPCStatus
  IDB_checkAuth(const char *&userauth, Bool &need_passwd, Bool &superuser)
  {
    const char *u;
    if (!(u = getUserAuth(userauth, need_passwd, superuser))) {
      if (*userauth)
	return rpcStatusMake(IDB_INSUFFICIENT_PRIVILEGES,
			     "access denied for user '%s'",
			     userauth);

      return rpcStatusMake(IDB_INSUFFICIENT_PRIVILEGES,
			   "access denied for unspecified user");
    }

    userauth = u;
    return RPCSuccess;
  }

  static RPCStatus
  IDB_checkSysAuth(ConnHandle *ch, const char *dbmdb,
		   const char *&userauth, const char *&passwdauth,
		   SysAccessMode sysmode, Bool rw_mode,
		   DBM_Database **pdbm, const char *msg, Bool *justCheck)
  {
    if (pdbm)
      *pdbm = 0;

    DBM_Database *dbm;
    RPCStatus rpc_status;
    Status status;
    SysUserAccess *sysaccess;

    if ((rpc_status = IDB_dbmOpen(ch, dbmdb, rw_mode, &dbm)) != RPCSuccess)
      return rpc_status;
    if (pdbm)
      *pdbm = dbm;

    if (!userauth || !passwdauth)
      return RPCSuccess;

    Bool need_passwd, superuser;
    if (rpc_status = IDB_checkAuth(userauth, need_passwd, superuser))
      return rpc_status;

#ifdef STUART_SUGGESTION
    if (superuser)
      return RPCSuccess;
#endif

    if (status = dbm->get_sys_user_access(userauth, &sysaccess,
					  (justCheck ? True : False),
					  msg))
      return rpcStatusMake(status);

    if (justCheck && !sysaccess) {
      *justCheck = False;
      return RPCSuccess;
    }

    dbm->transactionBegin();
    /* check that if in case of need_passwd is false, user is really an
       unix user; if not, need_passwd becomes true */

    if (!need_passwd && (sysaccess->user()->type() == EyeDBUser))
      need_passwd = True;

    if (need_passwd) {
      if (sysaccess->user()->type() == StrictUnixUser) {
	sysaccess->release();
	dbm->transactionCommit();
	return rpcStatusMake(IDB_AUTHENTICATION_FAILED,
			     "user '%s' can be used only in a "
			     "strict unix authentication mode",
			     userauth);
      }

      const char *pwd = sysaccess->user()->passwd().c_str();
      if (pwd && strcmp(pwd, crypt(passwdauth, salt))) {
	if (justCheck) {
	  *justCheck = False;
	  status = Success;
	}
	else
	  status = Exception::make(IDB_AUTHENTICATION_FAILED,
				   "user '%s': %s: invalid password",
				   userauth, msg);
	sysaccess->release();
	dbm->transactionCommit();
	return rpcStatusMake(status);
      }
    }

    if ((sysmode & sysaccess->mode()) != sysmode) {
      if (justCheck) {
	*justCheck = False;
	status = Success;
      }
      else
	status = Exception::make(IDB_INSUFFICIENT_PRIVILEGES,
				   "user '%s': %s", userauth, msg);
      sysaccess->release();
      dbm->transactionCommit();
      return rpcStatusMake(status);
    }

    if (justCheck)
      *justCheck = True;

    sysaccess->release();
    dbm->transactionCommit();
    return RPCSuccess;
  }

#define dbmupdate_str "*I*D*B*D*B*M*"

  /*static*/ RPCStatus
  IDB_checkDBAuth(ConnHandle *ch, const char *dbmdb,
		  const char *dbname, const char *&userauth,
		  const char *&passwdauth, DBAccessMode dbmode,
		  Bool rw_mode, int *puid, DBM_Database **pdbm,
		  const char *msg)
  {
    if (!pdbm && !strcmp(dbname, DBM_Database::getDbName()) &&
	userauth && !strcmp(userauth, dbmupdate_str))
      return RPCSuccess;

    if (puid)
      *puid = 0;

    if (!userauth && !pdbm)
      return RPCSuccess;

    if (pdbm)
      *pdbm = 0;

    DBM_Database *dbm;
    RPCStatus rpc_status;
    Status status;
    DBUserAccess *dbaccess;
    DBAccessMode defaccess;

    if ((rpc_status = IDB_dbmOpen(ch, dbmdb, rw_mode, &dbm)) != RPCSuccess)
      return rpc_status;

    if (pdbm)
      *pdbm = dbm;

    if (!userauth || !passwdauth)
      return RPCSuccess;

    Bool need_passwd, superuser;
    if (rpc_status = IDB_checkAuth(userauth, need_passwd, superuser))
      return rpc_status;

#ifdef STUART_SUGGESTION
    if (superuser)
      return RPCSuccess;
#endif

    if (status = dbm->transactionBegin())
      return rpcStatusMake(status);

    Bool justCheck = False;
    if (rpc_status = IDB_checkSysAuth(ch, dbmdb, userauth, passwdauth,
				      SuperUserSysAccessMode, False, 0, msg,
				      &justCheck))
      {
	dbm->transactionCommit();
	return rpc_status;
      }

    if (justCheck)
      {
	dbm->transactionCommit();
	return RPCSuccess;
      }

    UserEntry *user;

    if (status = dbm->get_db_user_access(dbname, userauth, &user,
					 &dbaccess, &defaccess))
      {
	dbm->transactionCommit();
	return rpcStatusMake(status);
      }

    /* check that if in case of need_passwd is false, user is really an
       unix user; if not, need_passwd becomes true */

    if (!need_passwd && (user->type() == EyeDBUser))
      need_passwd = True;

    if (need_passwd)
      {
	if (user->type() == StrictUnixUser)
	  {
	    if (dbaccess)
	      dbaccess->release();
	    user->release();
	    dbm->transactionCommit();
	    return rpcStatusMake(IDB_AUTHENTICATION_FAILED,
				 "user '%s' can be used only in a "
				 "strict unix authentication mode",
				 userauth);
	  }

	if (user->passwd().c_str() &&
	    strcmp(user->passwd().c_str(), crypt(passwdauth, salt)))
	  {
	    if (dbaccess)
	      dbaccess->release();
	    user->release();
	    status = Exception::make(IDB_AUTHENTICATION_FAILED,
				     "user '%s': %s: invalid password",
				     userauth, msg);
	    dbm->transactionCommit();
	    return rpcStatusMake(status);
	  }
      }

    if (puid &&
	((dbaccess &&
	  (dbaccess->mode() & AdminDBAccessMode) != AdminDBAccessMode) ||
	 !dbaccess))
      *puid = user->uid();

    user->release();

    if ((dbmode & defaccess) == dbmode)
      {
	if (dbaccess)
	  dbaccess->release();
	dbm->transactionCommit();
	return RPCSuccess;
      }

    if (!dbaccess || (dbmode & dbaccess->mode()) != dbmode)
      {
	if (dbaccess)
	  dbaccess->release();
	status = Exception::make(IDB_INSUFFICIENT_PRIVILEGES,
				 "user '%s': %s", userauth, msg);
	dbm->transactionCommit();
	return rpcStatusMake(status);
      }

    if (dbaccess)
      dbaccess->release();
    dbm->transactionCommit();
    return RPCSuccess;
  }

  static Status
  checkUserName(const char *username, int user_type)
  {
    if (!*username)
      return Exception::make(IDB_ERROR, "a username cannot be the empty string");

    char c;
    const char *p = username;
    while (c = *p++)
      if (!((c >= 'a' && c <= 'z') ||
	    (c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9') ||
	    c == '_' || c == '.' || c == '-'))
	return Exception::make(IDB_ERROR,
			       "a username must be under the form: "
			       "[a-zA-Z0-9_\\.\\-]+");

    if ((user_type == UnixUser || user_type == StrictUnixUser) &&
	!getpwnam(username))
      return Exception::make(IDB_ERROR, "username '%s' is an unknown "
			     "unix user", username);

    return Success;
  }

  RPCStatus
  IDB_userAdd(ConnHandle *ch, const char *dbmdb,
	      const char *userauth, const char *passwdauth,
	      const char *username, const char *passwd, int user_type)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkSysAuth(ch, dbmdb, userauth, passwdauth,
				       AddUserSysAccessMode, True, &dbm,
				       "adding user")) !=
	RPCSuccess)
      return rpc_status;

    if (status = checkUserName(username, user_type))
      return rpcStatusMake(status);
    
    //  const char *pwd = (*passwd ? crypt(passwd, salt) : 0);
    std::string pwd = (*passwd ? crypt(passwd, salt) : "");
    status = dbm->add_user(username, pwd.c_str(), (UserType)user_type);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_userDelete(ConnHandle *ch, const char *dbmdb,
		 const char *userauth, const char *passwdauth,
		 const char *username)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkSysAuth(ch, dbmdb, userauth, passwdauth,
				       DeleteUserSysAccessMode, True, &dbm,
				       "deleting user")) !=
	RPCSuccess)
      return rpc_status;

    status = dbm->delete_user(username);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_userPasswdSet(ConnHandle *ch, const char *dbmdb,
		    const char *userauth, const char *passwdauth,
		    const char *username, const char *passwd)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkSysAuth(ch, dbmdb, userauth, passwdauth,
				       SetUserPasswdSysAccessMode, True,
				       &dbm,"setting passwd")) !=
	RPCSuccess)
      return rpc_status;


    const char *pwd = (*passwd ? crypt(passwd, salt) : 0);
    status = dbm->user_passwd_set(username, pwd);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_passwdSet(ConnHandle *ch, const char *dbmdb,
		const char *username, const char *passwd,
		const char *newpasswd)
  {
    DBM_Database *dbm;
    RPCStatus rpc_status;
    Status s;
    UserEntry *user;

    if ((rpc_status = IDB_dbmOpen(ch, dbmdb, True, &dbm)) != RPCSuccess)
      return rpc_status;

    s = dbm->getUser(username, user);
    if (s) return rpcStatusMake(s);

    if (!user)
      return rpcStatusMake(Exception::make(IDB_SET_PASSWD_ERROR,
					   "user '%s' not found",
					   username));

    if (user->passwd().c_str() && strcmp(user->passwd().c_str(), crypt(passwd, salt)))
      {
	user->release();
	s = Exception::make(IDB_AUTHENTICATION_FAILED,
			    "user '%s': %s", username,
			    "setting passwd");
	return rpcStatusMake(s);
      }

    user->release();

    s = dbm->user_passwd_set(username, crypt(newpasswd, salt));
    return rpcStatusMake(s);
  }

  RPCStatus
  IDB_userDBAccessSet(ConnHandle *ch, const char *dbmdb,
		      const char *userauth, const char *passwdauth,
		      const char *dbname, const char *username, int mode)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkDBAuth(ch, dbmdb, dbname, userauth, passwdauth,
				      AdminDBAccessMode, True, NULL, &dbm,
				      "setting user db access")) !=
	RPCSuccess) {
      return rpc_status;
    }

    status = dbm->user_db_access_set(dbname, username, (DBAccessMode)mode);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_userSysAccessSet(ConnHandle *ch, const char *dbmdb,
		       const char *userauth, const char *passwdauth,
		       const char *username, int mode)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkSysAuth(ch, dbmdb,userauth, passwdauth,
				       SuperUserSysAccessMode, True, &dbm,
				       "setting sys user access")) !=
	RPCSuccess)
      return rpc_status;

    status = dbm->user_sys_access_set(username, (SysAccessMode)mode);

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_defaultDBAccessSet(ConnHandle *ch, const char *dbmdb,
			 const char *userauth, const char *passwdauth,
			 const char *dbname, int mode)
  {
    RPCStatus rpc_status;
    Status status;
    DBM_Database *dbm;

    if ((rpc_status = IDB_checkDBAuth(ch, dbmdb, dbname, userauth, passwdauth,
				      AdminDBAccessMode, True, NULL, &dbm,
				      "setting database access")) !=
	RPCSuccess)
      return rpc_status;

    status = dbm->default_db_access_set(dbname, (DBAccessMode)mode);

    return rpcStatusMake(status);
  }

  extern DbHandle *
  database_getDbHandle(Database *);

  RPCStatus
  IDB_dbOpen_make(ConnHandle *ch, const char *dbmdb,
		  const char *userauth, const char *passwdauth,
		  const char *dbname, int dbid, int flag,
		  int oh_maph, unsigned oh_mapwide, 
		  int *pid, int *puid, char **rname,
		  int *rdbid, unsigned int *pversion, DbHandle **pdbh)
  {
    Database *db;
    Connection *conn = ConnectionPeer::newIdbConnection(ch);
    Status status;
    RPCStatus rpc_status;

    /*
    rpc_status = IDB_checkAuthDbm(dbmdb);
    if (rpc_status)
      return rpc_status;
    */

    OpenHints hints;
    hints.maph = (MapHints)oh_maph;
    hints.mapwide = oh_mapwide;

    if (*dbname)
      status = Database::open(conn, dbname, dbmdb, userauth, passwdauth,
			      (Database::OpenFlag)flag, &hints, &db);
    else
      status = Database::open(conn, dbid, dbmdb, userauth, passwdauth,
			      (Database::OpenFlag)flag, &hints, &db);

    if (status == Success)
      {
	*rname = (char *)db->getName();
	*rdbid = db->getDbid();
	*pdbh = database_getDbHandle(db);
	if (pversion)
	  *pversion = db->getVersionNumber();
	*puid = db->getUid();
	/*
	  if (pid)
	  *pid = (*pdbh)->sedbh->ldbctx.rdbhid;
	  */
      }
    else
      *rname = (char*)""; /*@@@@warning cast*/

    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_dbOpen(ConnHandle *ch, const char *dbmdb,
	     const char *userauth, const char *passwdauth,
	     const char *dbname,
	     int dbid, int flags, int oh_maph, unsigned int oh_mapwide,
	     int *pid, int *puid, void *db, char **rname,
	     int *rdbid, unsigned int *pversion, DbHandle **pdbh)
  {
    eyedbsm::Status status;
    eyedbsm::DbHandle *sedbh;
    int se_flags;
    RPCStatus rpc_status;
    int flags_o = flags;

    /*
    rpc_status = IDB_checkAuthDbm(dbmdb);
    if (rpc_status)
      return rpc_status;
    */

    //eyedblib::display_time("%s %s", dbname, "#1");
    *rname = (char*)"";/*@@@@warning cast*/
    *rdbid = 0;

    flags &= ~(_DBOpenLocal);

    if ((flags & _DBSRead) == _DBSRead) {
      //se_flags = eyedbsm::VOLREAD;
      se_flags = eyedbsm::VOLREAD|eyedbsm::NO_SHM_ACCESS;
    }
    else if ((flags & _DBRW) == _DBRW || (flags & _DBRead) == _DBRead) {
      se_flags = eyedbsm::VOLRW;
    }
    else {
      return rpcStatusMake(IDB_INVALID_DBOPEN_FLAG,
			   "opening flag '%u' is invalid", flags);
    }

    const char *dbfile;

    rpc_status = IDB_getDbFile(ch, &dbname, dbmdb, &dbfile, &dbid, False);
    if (rpc_status != RPCSuccess)
      return rpc_status;

    //eyedblib::display_time("%s %s", dbname, "#2");
    std::string msg = std::string("opening database '") + dbname + "' in " +
      Database::getStringFlag((Database::OpenFlag)flags_o) + " mode";
    DBAccessMode dbmode;

    if (flags & _DBAdmin)
      dbmode = AdminDBAccessMode;
    else if (flags == _DBRead || flags == _DBSRead)
      dbmode = ReadDBAccessMode;
    else
      dbmode = ReadWriteDBAccessMode;

    //  if (!(flags_o & _DBOpenLocal))
    if (eyedbsm::backend)
      {
	if ((rpc_status = IDB_checkDBAuth(ch, dbmdb, dbname, userauth,
					  passwdauth, dbmode, False,
					  puid, 0, msg.c_str())) !=
	    RPCSuccess)
	  return rpc_status;
      }

    //eyedblib::display_time("%s %s", dbname, "#3");
    //char *pwd = getcwd(NULL, PATH_MAX);
    unsigned int version;
    if (!pversion)
      pversion = &version;

    *pversion = eyedb::getVersionNumber();
    eyedbsm::OpenHints hints;
    hints.maph = (eyedbsm::MapHints)oh_maph;
    hints.mapwide = oh_mapwide;

    status = eyedbsm::dbOpen(dbfile, se_flags|eyedbsm::LOCAL, (oh_maph ? &hints : 0),
			     (puid ? *puid : 0), pversion, &sedbh);

    //eyedblib::display_time("%s %s", dbname, "#4");
    /*
      if (pwd)
      {
      chdir(pwd);
      free(pwd);
      }
    */

    if (status)
      return rpcStatusMake_se(status);

    // disconnected 2/05/05
    /*
      if (pid) {
      *pid = sedbh->ldbctx.rdbhid;
      }
    */

    *rdbid = dbid; //eyedbsm::dataBaseIdGet((eyedbsm::DbHandle *)sedbh->u.dbh);
    *rname = (char *)dbname;
    *pdbh = rpc_new(DbHandle);
    memset(*pdbh, 0, sizeof(DbHandle));
    (*pdbh)->sedbh = sedbh;
    (*pdbh)->ch = ch;

    //eyedblib::display_time("%s %s", dbname, "#5");
    rpc_status = IDB_schemaClassHeadGet(*pdbh);

    //eyedblib::display_time("%s %s", dbname, "#6");
    if (rpc_status == RPCSuccess)
      {
	if (db)
	  (*pdbh)->db = db;
	else
	  {
	    rpcDB_LocalDBContext ldbctx;
	    ldbctx.rdbhid = 0;
	    ldbctx.xid = eyedbsm::getXID(sedbh);
	    Status status = IDB_makeDatabase(ch, dbmdb, dbname, dbid,
					     flags_o, *pdbh, &ldbctx,
					     &(*pdbh)->sch.oid,
					     *pversion,
					     (Database *&)(*pdbh)->db);
	    if (status)
	      return rpcStatusMake(status);
	  }

	//eyedblib::display_time("%s %s", dbname, "#7");
	if (!db_list)
	  {
	    db_list = new LinkedList();
	    rpc_setQuitHandler(quit_handler, db_list);
	  }

	db_list->insertObjectLast((*pdbh)->db);
	if (conn_ctx.clinfo)
	  conn_ctx.clinfo->addDatabase
	    (dbname,
	     /* was: ((Database *)(*pdbh)->db)->getName() */
	     userauth,
	     /* was: ((Database *)(*pdbh)->db)->getUserAuth() */
	     flags_o);
      }

    //eyedblib::display_time("%s %s", dbname, "#8");
    return rpc_status;
  }

  extern void oqml_reinit(Database *db);

  RPCStatus
  IDB_dbClose_make(DbHandle *dbh)
  {
    oqml_reinit((Database *)dbh->db);
    RPCStatus rpc_status = rpcStatusMake(((Database *)(dbh->db))->close());

    /*
      if (rpc_status == RPCSuccess)
      db_list->deleteObject(dbh->db);

      if (conn_ctx.clinfo)
      conn_ctx.clinfo->suppressDatabase(((Database *)dbh->db)->getName(),
      ((Database *)dbh->db)->getUserAuth(),
      ((Database *)dbh->db)->getOpenFlag());
    */

    return rpc_status;
  }

  RPCStatus
  IDB_dbClose(DbHandle *dbh)
  {
    oqml_reinit((Database *)dbh->db);
    RPCStatus s = rpcStatusMake_se(eyedbsm::dbClose(dbh->sedbh));
    if (!s && dbh->db) {
      if (conn_ctx.clinfo)
	conn_ctx.clinfo->suppressDatabase(((Database *)dbh->db)->getName(),
					  ((Database *)dbh->db)->getUser(),
					  ((Database *)dbh->db)->getOpenFlag());
      db_list->deleteObject(dbh->db);
      dbh->db = 0;
    }

    return s;

  }

  void
  IDB_getLocalInfo(DbHandle *dbh, rpcDB_LocalDBContext *ldbctx,
		   eyedbsm::Oid *sch_oid)
  {
    ldbctx->rdbhid = 0;
    ldbctx->xid = eyedbsm::getXID(dbh->sedbh);
    //*ldbctx = dbh->sedbh->ldbctx;
    *sch_oid = dbh->sch.oid;
  }

  RPCStatus
  IDB_dbCloseLocal(DbHandle *dbh)
  {
    oqml_reinit((Database *)dbh->db);
    //printf("IDB_dbCloseLocal: sedbh: %p\n", dbh->sedbh);
    //printf("IDB_dbCloseLocal: sedbh->u.dbh: %p\n", dbh->sedbh->u.dbh);
    // changed 09/05/05: 
    //  return rpcStatusMake_se(eyedbsm::dbClose((eyedbsm::DbHandle *)dbh->sedbh->u.dbh));
    return rpcStatusMake_se(eyedbsm::dbClose((eyedbsm::DbHandle *)dbh->sedbh));
  }

  RPCStatus
  IDB_dbInfo(ConnHandle *ch, const char *dbmdb,
	     const char *userauth, const char *passwdauth, const char *dbname,
	     int *rdbid, DbCreateDescription *dbdesc)
  {
    RPCStatus rpc_status;
    const char *dbfile;

    rpc_status = IDB_getDbFile(ch, &dbname, dbmdb, &dbfile, rdbid, False);
    if (rpc_status != RPCSuccess)
      return rpc_status;

    strcpy(dbdesc->dbfile, dbfile);
    return rpcStatusMake_se(eyedbsm::dbInfo(dbdesc->dbfile, &dbdesc->sedbdesc));
  }

  static Bool backend_interrupt = False;

  void
  setBackendInterrupt(Bool to)
  {
    backend_interrupt = to;
  }

  Bool
  isBackendInterrupted()
  {
    return backend_interrupt;
  }

  RPCStatus
  IDB_backendInterrupt(ConnHandle *, int)
  {
    setBackendInterrupt(True);
    eyedbsm::backendInterrupt();
    return RPCSuccess;
  }

  RPCStatus
  IDB_backendInterruptReset()
  {
    setBackendInterrupt(False);
    eyedbsm::backendInterruptReset();
    return RPCSuccess;
  }

  /* transaction management */

  //#define TRS_TRACE

  RPCStatus
  IDB_transactionBegin(DbHandle *dbh, 
		       const TransactionParams *params,
		       Bool local_call)
  {
    Database *db = (Database *)dbh->db;

    IDB_LOG(IDB_LOG_TRANSACTION,
	    ("transaction begin(db=%p, dbh=%p, tr_cnt=%d, "
	     "local_call=%d, BE=%d, dbname=%s)\n",
	     db, dbh, dbh->tr_cnt, local_call,
	     (db ? db->isBackEnd() : -1), (db ? db->getName() : "NIL")));

    if (!local_call && db && db->isBackEnd())
      {
	if (params)
	  return rpcStatusMake(db->transactionBegin(*params));
	return rpcStatusMake(db->transactionBegin());
      }

    eyedbsm::Status status = eyedbsm::transactionBegin(dbh->sedbh,
						       (const eyedbsm::TransactionParams *)params);

    if (!status)
      dbh->tr_cnt++;

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_objectDeletedManage(DbHandle *dbh, Bool commit)
  {
    Database *db = (Database *)dbh->db;

    if (!db)
      return RPCSuccess;

    RPCStatus rpc_status;
    LinkedListCursor c(db->getMarkDeleted());
    Oid *oid;
    int error = 0;

    while (c.getNext((void *&)oid)) {
      if (commit && !error) {
	rpc_status =
	  IDB_objectSizeModify(dbh, IDB_OBJ_HEAD_SIZE, oid->getOid());
	if (rpc_status)
	  error++;
      }
      delete oid;
    }

    db->getMarkDeleted().empty();
    db->markCreatedEmpty();

    if (error)
      return rpc_status;

    return RPCSuccess;
  }

  RPCStatus
  IDB_transactionCommit(DbHandle *dbh, Bool local_call)
  {
    Database *db = (Database *)dbh->db;
    RPCStatus rpc_status;

    IDB_LOG(IDB_LOG_TRANSACTION,
	    ("transaction commit(db=%p, dbh=%p, tr_cnt=%d, "
	     "local_call=%d, BE=%d, dbname=%s)\n",
	     db, dbh, dbh->tr_cnt, local_call,
	     (db ? db->isBackEnd() : -1), (db ? db->getName() : "NIL")));

    if (!local_call && db && db->isBackEnd())
      return rpcStatusMake(db->transactionCommit());

    if (rpc_status = IDB_objectDeletedManage(dbh, True))
      return rpc_status;

    eyedbsm::Status status = eyedbsm::transactionCommit(dbh->sedbh);

    if (!status)
      dbh->tr_cnt--;

    if (!status && db && db->getSchema())
      db->getSchema()->revert(False);

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_transactionAbort(DbHandle *dbh, Bool local_call)
  {
    RPCStatus rpc_status;
    Database *db = (Database *)dbh->db;

    IDB_LOG(IDB_LOG_TRANSACTION,
	    ("transaction abort(db=%p, dbh=%p, tr_cnt=%d, "
	     "local_call=%d, BE=%d, dbname=%s)\n",
	     db, dbh, dbh->tr_cnt, local_call,
	     (db ? db->isBackEnd() : -1), (db ? db->getName() : "NIL")));

    if (!local_call && db && db->isBackEnd())
      return rpcStatusMake(db->transactionAbort());

    if (rpc_status = IDB_objectDeletedManage(dbh, False))
      return rpc_status;

    eyedbsm::Status status = eyedbsm::transactionAbort(dbh->sedbh);

    if (!status)
      dbh->tr_cnt--;

    if (!status && db && db->getSchema())
      db->getSchema()->revert(True);

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_transactionParamsSet(DbHandle *dbh,
			   const TransactionParams *params)
  {
    eyedbsm::Status status = eyedbsm::transactionParamsSet(dbh->sedbh,
							   (const eyedbsm::TransactionParams *)params);

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_transactionParamsGet(DbHandle *dbh,
			   TransactionParams *params)
  {
    eyedbsm::Status status = eyedbsm::transactionParamsGet(dbh->sedbh,
							   (eyedbsm::TransactionParams *)params);

    return rpcStatusMake_se(status);
  }

#define CHECK_WRITE(DB) \
  if ((DB) && !((DB)->getOpenFlag() & _DBRW)) \
    return rpcStatusMake(IDB_ERROR, "database is not opened for writing")

  static RPCStatus
  getDspid(Database *db, const Class *cls, short &dspid)
  {
    if (dspid != Dataspace::DefaultDspid)
      return RPCSuccess;

    const Dataspace *dataspace;

    if (cls) {
      Status s = cls->getDefaultInstanceDataspace(dataspace);
      if (s)
	return rpcStatusMake(s);
    
      if (dataspace) {
	dspid = dataspace->getId();
	return RPCSuccess;
      }
    }

    Status s = db->getDefaultDataspace(dataspace);
    if (s)
      return rpcStatusMake(s);
    dspid = (dataspace ? dataspace->getId() : 0);
    return RPCSuccess;
  }

  /* typed object */
  RPCStatus
  IDB_objectCreate(DbHandle *dbh, short dspid, const Data idr,
		   eyedbsm::Oid *oid, void *xdata, Data *inv_data, void *xinv_data)
  {
    ObjectHeader hdr;
    Offset offset = 0;
    RPCStatus rpc_status;
    Database *db = (Database *)dbh->db;

    CHECK_WRITE(db);

    lock_data((Data *)&idr, xdata);

    Bool new_ctx = InvOidContext::getContext();

    if (!object_header_decode(idr, &offset, &hdr))
      {
	unlock_data(idr, xdata);
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectCreate: invalid object_header");
      }

    if (hdr.type == _Schema_Type)
      {
	unlock_data(idr, xdata);
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return rpcStatusMake(IDB_CANNOT_CREATE_SCHEMA, "objectCreate: cannot create a schema");
      }

    const Class *cls = db->getSchema()->getClass(hdr.oid_cl);

    rpc_status = getDspid(db, cls, dspid);
    if (rpc_status)
      return rpc_status;

    TRIGGER_MANAGE(db, Class::TrigCreateBefore_C, &hdr, idr, oid, cls);

    Oid moid(hdr.oid_cl);
    const Oid &mprot_oid = ((Database *)dbh->db)->getMprotOid();

    if (mprot_oid.isValid() && moid.compare(mprot_oid))
      rpc_status = IDB_protectionCreate(dbh, dspid, idr, &hdr, oid, xdata, True);
    else if (eyedb_is_type(hdr, _Class_Type))
      rpc_status = IDB_classCreate(dbh, dspid, idr, &hdr, oid, xdata);
    else if (eyedb_is_type(hdr, _Struct_Type) ||
	     eyedb_is_type(hdr, _Union_Type))
      rpc_status = IDB_agregatCreate(dbh, dspid, idr, &hdr, oid, xdata);
    else if (eyedb_is_type(hdr, _Collection_Type))
      rpc_status = IDB_collectionCreate(dbh, dspid, idr, &hdr, oid, xdata);
    else
      rpc_status = IDB_instanceCreate(dbh, dspid, idr, &hdr, oid, xdata, True);

    if (!rpc_status)
      TRIGGER_MANAGE(db, Class::TrigCreateAfter_C, &hdr, idr, oid, cls);

    InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);

    unlock_data(idr, xdata);
    return rpc_status;
  }

  RPCStatus
  IDB_objectDelete(DbHandle *dbh, const eyedbsm::Oid *oid, unsigned int flags,
		   Data *inv_data, void *xinv_data)
  {
    RPCStatus rpc_status;
    ObjectHeader hdr;
    Data idr;
    Database *db = (Database *)dbh->db;
    Oid toid(oid);

    IDB_LOG(IDB_LOG_OBJ_REMOVE, ("removing object %s\n", toid.toString()));

    Bool new_ctx = InvOidContext::getContext();

    rpc_status = IDB_objectHeaderRead(dbh, oid, &hdr);

    if (rpc_status != RPCSuccess)
      {
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return rpc_status;
      }

    if (hdr.xinfo & IDB_XINFO_REMOVED)
      {
	IDB_LOG(IDB_LOG_OBJ_REMOVE, ("object %s already removed\n",
				     toid.toString()));
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return RPCSuccess;
      }

    idr = (unsigned char *)malloc(hdr.size);
    object_header_code_head(idr, &hdr);

    rpc_status = IDB_objectRead(dbh, idr, 0, 0, oid, DefaultLock, 0);

    if (rpc_status != RPCSuccess)
      {
	free(idr);
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return rpc_status;
      }

    const Class *cls = db->getSchema()->getClass(hdr.oid_cl);
    TRIGGER_MANAGE(db, Class::TrigRemoveBefore_C, &hdr, idr, oid, cls);

    Oid moid(hdr.oid_cl);
    const Oid &mprot_oid = ((Database *)dbh->db)->getMprotOid();

    if (mprot_oid.isValid() && moid.compare(mprot_oid))
      rpc_status = IDB_protectionDelete(dbh, idr, &hdr, oid);
    else if (eyedb_is_type(hdr, _Class_Type))
      rpc_status = IDB_classDelete(dbh, idr, &hdr, oid, flags);
    else if (eyedb_is_type(hdr, _Struct_Type) ||
	     eyedb_is_type(hdr, _Union_Type))
      rpc_status = IDB_agregatDelete(dbh, idr, &hdr, oid);
    else if (eyedb_is_type(hdr, _Collection_Type))
      rpc_status = IDB_collectionDelete(dbh, idr, &hdr, oid);
    else
      rpc_status = IDB_instanceDelete(dbh, idr, &hdr, oid);

    if (!rpc_status)
      TRIGGER_MANAGE(db, Class::TrigRemoveAfter_C, &hdr, idr, oid, cls);

    InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
    free(idr);
    if (!rpc_status)
      IDB_LOG(IDB_LOG_OBJ_REMOVE, ("object %s is now removed\n", toid.toString()));
    return rpc_status;
  }

  static int
  IDB_getObjectSize(const Class *cls, Size *psize, Size *vsize,
		    ObjectHeader *hdr)
  {
    return ((cls && cls->getIDRObjectSize(psize, vsize) &&
	     !cls->asCollectionClass()) ?
	    cls->getIDRObjectSize() : hdr->size);
  }

  static RPCStatus
  IDB_convertPrologue(Database *db, const Oid &oidcl, const Class *&cls,
		      ClassConversion::Context *&conv_ctx)
  {
    conv_ctx = 0;
    if (!cls && oidcl.isValid() && !db->isOpeningState()) {
      Status status = ClassConversion::getClass_(db, oidcl, cls,
						 conv_ctx);
      if (status) return rpcStatusMake(status);
      // 11/06/01: should add:
      // if (db->getOpenFlag() & idbDBSREAD)
      // return Exception::make(IDB_ERROR, "....");
      return RPCSuccess;
    }

    return RPCSuccess;
  }

  static RPCStatus
  IDB_convertEpilogue(DbHandle *dbh, Database *db,
		      const Class *cls,
		      const ClassConversion::Context *conv_ctx,
		      Data idr, ObjectHeader *xhdr, Size idr_psize,
		      const eyedbsm::Oid *oid)
  {
    Size osize = xhdr->size;

    xhdr->oid_cl = *cls->getOid().getOid();
    Size hdr_size = xhdr->size; // added the 6/6/01
    xhdr->size = (xhdr->xinfo & IDB_XINFO_REMOVED) ? IDB_OBJ_HEAD_SIZE :
      idr_psize;

    Offset offset = 0;
    Size size = xhdr->size;
    object_header_code(&idr, &offset, &size, xhdr);

    if (xhdr->xinfo & IDB_XINFO_REMOVED)
      return RPCSuccess;

    Status status = ClassConversion::convert(db, conv_ctx, idr, hdr_size);
    if (status) return rpcStatusMake(status);

    if (!db->writeBackConvertedObjects())
      return RPCSuccess;
    /*
    // added the 17/09/01
    if (db->getOpenFlag() & _DBSRead)
    return RPCSuccess;
    */

    IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("write object %s back\n", Oid(oid).toString()));
    eyedbsm::Status se_status;
    if (idr_psize != osize)
      {
	IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("... and change size from %d to %d!\n", osize, idr_psize));
	se_status = eyedbsm::objectSizeModify(dbh->sedbh, idr_psize, eyedbsm::False, oid);
	if (se_status)
	  return rpcStatusMake_se(se_status);
      }
      
    se_status = eyedbsm::objectWrite(dbh->sedbh, 0, xhdr->size, idr, oid);
      
    if (se_status)
      return rpcStatusMake_se(se_status);

    return RPCSuccess;
  }

  RPCStatus
  IDB_objectReadLocal(DbHandle *dbh, Data idr, Data *pidr,
		      short *pdspid, const eyedbsm::Oid *oid, ObjectHeader *phdr,
		      LockMode lockmode, void **pcls)
  {
    Status status;
    RPCStatus rpc_status;
    ObjectHeader hdr;
    ObjectHeader *xhdr = (phdr ? phdr : &hdr);
#define hdr xxxxx
    Database *db = (Database *)dbh->db;
    eyedbsm::Status se_status;

    Offset offset = 0;

    if (idr)
      object_header_decode(idr, &offset, xhdr);
    else
      xhdr->magic = 0;

    char buf[IDB_OBJ_HEAD_SIZE];

    if (!xhdr->magic) {
      Offset offset = 0;
      se_status =
	eyedbsm::objectRead(dbh->sedbh, 0, IDB_OBJ_HEAD_SIZE, buf,
			    (eyedbsm::LockMode)lockmode, pdspid, 0, oid);
    
      if (se_status)
	return rpcStatusMake_se(se_status);
    
      if (!object_header_decode((Data)buf, &offset, xhdr))
	return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectRead: invalid object_header");
      /*
	printf("decoding: %p %x %x %d %d %d %s\n",
	xhdr,
	xhdr->magic,
	xhdr->type,
	xhdr->size,
	xhdr->ctime,
	xhdr->mtime,
	se_getOidString(&xhdr->oid_cl));
      */
    }

    const Class *cls = db->getSchema()->getClass(xhdr->oid_cl);
    ClassConversion::Context *conv_ctx;
    rpc_status = IDB_convertPrologue(db, xhdr->oid_cl, cls, conv_ctx);
    if (rpc_status) return rpc_status;

    if (pcls)
      *(const Class **)pcls = cls;

    Bool trig_man = IDBBOOL(!db->isOpeningState() && !db->getSchema()->getClass(*oid));
    if (trig_man)
      TRIGGER_MANAGE(db, Class::TrigLoadBefore_C, xhdr, 0, oid, cls);

    Size idr_psize = 0;
    Size idr_vsize = 0;
    int objsize = IDB_getObjectSize(cls, &idr_psize, &idr_vsize, xhdr);

    if (!idr) {
      Size objsize_x;

      if (conv_ctx) {
	Size size = ClassConversion::computeSize(conv_ctx, xhdr->size);
	objsize_x = (size < objsize ? objsize : size);
      }
      else
	objsize_x = objsize;

      idr = *pidr = (unsigned char *)malloc(objsize_x);
      assert(idr);
      memcpy(idr, buf, IDB_OBJ_HEAD_SIZE);
    }

    if (xhdr->size > IDB_OBJ_HEAD_SIZE && !(xhdr->xinfo & IDB_XINFO_REMOVED))
      rpc_status =
	rpcStatusMake_se(eyedbsm::objectRead(dbh->sedbh, IDB_OBJ_HEAD_SIZE,
					     xhdr->size - IDB_OBJ_HEAD_SIZE,
					     idr + IDB_OBJ_HEAD_SIZE,
					     (eyedbsm::LockMode)lockmode, 0, 0, oid));
    else
      rpc_status = RPCSuccess;

    if (conv_ctx)
      rpc_status = IDB_convertEpilogue(dbh, db, cls, conv_ctx, idr, xhdr,
				       idr_psize, oid);

    if (!rpc_status)
      {
	if (trig_man)
	  TRIGGER_MANAGE(db, Class::TrigLoadAfter_C, xhdr, idr, oid, cls);

	if (phdr)
	  {
	    if (objsize > xhdr->size)
	      {
		memset(idr + xhdr->size, 0, objsize - xhdr->size);
		xhdr->size = objsize;
	      }
	  }
      }

#undef hdr
    return rpc_status;
  }

  RPCStatus
  IDB_objectRead(DbHandle *dbh, Data idr, Data *pidr, short *pdspid,
		 const eyedbsm::Oid *oid, LockMode lockmode, void *xdata)
  {
    Status status;
    RPCStatus rpc_status;
    Offset offset;
    ObjectHeader hdr;
    rpc_ServerData *data = (rpc_ServerData *)xdata;
    Database *db = (Database *)dbh->db;

    if (data) {
      data->status = rpc_BuffUsed;
      data->size = 0;
    }

    offset = 0;
    if (idr)
      object_header_decode(idr, &offset, &hdr);
    else
      hdr.magic = 0;

    if (!hdr.magic)
      {
	char buf[IDB_OBJ_HEAD_SIZE];
	Offset offset = 0;
	eyedbsm::Status se_status =
	  eyedbsm::objectRead(dbh->sedbh, 0, IDB_OBJ_HEAD_SIZE, buf,
			      (eyedbsm::LockMode)lockmode, pdspid, 0, oid);

	if (se_status)
	  return rpcStatusMake_se(se_status);

	if (!object_header_decode((Data)buf, &offset, &hdr))
	  return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectRead: invalid object_header");
      }

    const Class *cls = db->getSchema()->getClass(hdr.oid_cl);
    ClassConversion::Context *conv_ctx = 0;
    if (!cls) {
      rpc_status = IDB_convertPrologue(db, hdr.oid_cl, cls, conv_ctx);
      if (rpc_status) return rpc_status;
    }

    TRIGGER_MANAGE(db, Class::TrigLoadBefore_C, &hdr, 0, oid, cls);

    Size idr_psize = 0;
    int objsize = IDB_getObjectSize(cls, &idr_psize, 0, &hdr);
    Size minsize = (!conv_ctx ? idr_psize :
		    ClassConversion::computeSize(conv_ctx, hdr.size));

    rpc_status = IDB_instanceRead(dbh, idr, pidr, &hdr, lockmode, oid, xdata,
				  minsize); // was idr_psize
    if (!rpc_status && conv_ctx)
      rpc_status = IDB_convertEpilogue(dbh, db, cls, conv_ctx, 
				       (data ? (Data)data->data : idr),
				       &hdr, idr_psize, oid);
					   
    if (!rpc_status)
      TRIGGER_MANAGE(db, Class::TrigLoadAfter_C, &hdr,
		     (data ? (Data)data->data : idr), oid, cls);

    return rpc_status;
  }

  RPCStatus
  IDB_objectWrite(DbHandle *dbh, const Data idr, const eyedbsm::Oid *oid,
		  void *xdata, Data *inv_data, void *xinv_data)
  {
    ObjectHeader hdr;
    Offset offset = 0;
    RPCStatus rpc_status;
    Database *db = (Database *)dbh->db;

    CHECK_WRITE(db);

    Oid toid(oid);

    lock_data((Data *)&idr, xdata);

    Bool new_ctx = InvOidContext::getContext();

    IDB_LOG(IDB_LOG_OBJ_UPDATE, ("writing %s object\n",  toid.toString()));
    if (!object_header_decode(idr, &offset, &hdr))
      {
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	unlock_data(idr, xdata);
	return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectWrite: invalid object_header");
      }

    if (hdr.xinfo & IDB_XINFO_REMOVED)
      {
	IDB_LOG(IDB_LOG_OBJ_UPDATE, ("object %s already removed\n",  toid.toString()));
	unlock_data(idr, xdata);
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return RPCSuccess;
      }

    if (hdr.type == _Schema_Type)
      {
	unlock_data(idr, xdata);
	InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
	return rpcStatusMake(IDB_CANNOT_UPDATE_SCHEMA, "objectWrite: cannot update a schema");
      }

    const Class *cls = db->getSchema()->getClass(hdr.oid_cl);
    TRIGGER_MANAGE(db, Class::TrigUpdateBefore_C, &hdr, idr, oid,
		   cls);

    Oid moid(hdr.oid_cl);
    const Oid &mprot_oid = ((Database *)dbh->db)->getMprotOid();

    if (mprot_oid.isValid() && moid.compare(mprot_oid))
      rpc_status = IDB_protectionWrite(dbh, idr, &hdr, oid, xdata);
    else if (eyedb_is_type(hdr, _Class_Type))
      rpc_status = IDB_classWrite(dbh, idr, &hdr, oid, xdata);
    else if (eyedb_is_type(hdr, _Struct_Type) ||
	     eyedb_is_type(hdr, _Union_Type))
      rpc_status = IDB_agregatWrite(dbh, idr, &hdr, oid, xdata);
    else if (eyedb_is_type(hdr, _Collection_Type))
      rpc_status = IDB_collectionWrite(dbh, idr, &hdr, oid, xdata);
    else
      rpc_status = IDB_instanceWrite(dbh, idr, &hdr, oid, xdata);

    if (!rpc_status)
      TRIGGER_MANAGE(db, Class::TrigUpdateAfter_C, &hdr, idr, oid, cls);

    InvOidContext::releaseContext(new_ctx, inv_data, xinv_data);
    unlock_data(idr, xdata);
    if (!rpc_status)
      IDB_LOG(IDB_LOG_OBJ_UPDATE, ("object %s wrote\n",  toid.toString()));
    return rpc_status;
  }

  RPCStatus
  IDB_objectHeaderRead(DbHandle *dbh, const eyedbsm::Oid *oid, ObjectHeader *hdr)
  {
    eyedbsm::Status status;
    char buf[IDB_OBJ_HEAD_SIZE];
    Offset offset = 0;

    status = eyedbsm::objectRead(dbh->sedbh, 0, IDB_OBJ_HEAD_SIZE, buf, eyedbsm::DefaultLock, 0, 0, oid);

    if (status == eyedbsm::Success) {
      if (!object_header_decode((Data)buf, &offset, hdr))
	return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectHeaderRead: invalid object_header");
    }

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_objectHeaderWrite(DbHandle *dbh, const eyedbsm::Oid *oid, const ObjectHeader *hdr)
  {
    CHECK_WRITE((Database *)dbh->db);

    eyedbsm::Status status;
    char *buf = NULL;
    Offset offset = 0;
    Size alloc_size = 0;

    if (!object_header_code((Data *)&buf, &offset, &alloc_size, hdr))
      {
	free(buf);
	return rpcStatusMake(IDB_INVALID_OBJECT_HEADER, "objectHeaderRead: invalid object_header");
      }

    status = eyedbsm::objectWrite(dbh->sedbh, 0, IDB_OBJ_HEAD_SIZE, buf, oid);

    free(buf);
    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_objectSizeModify(DbHandle *dbh, unsigned int size, const eyedbsm::Oid *oid)
  {
    CHECK_WRITE((Database *)dbh->db);

    ObjectHeader hdr;
    eyedbsm::Status status;
    eyedbsm::DbHandle *sedbh = dbh->sedbh;

    if ((status = eyedbsm::objectSizeModify(sedbh, size, eyedbsm::True, oid)) ==
	eyedbsm::Success) {
#ifdef E_XDR
      eyedblib::int32 size_x = h2x_32(size);
#else
      eyedblib::int32 size_x = size;
#endif
      status = eyedbsm::objectWrite(sedbh, IDB_OBJ_HEAD_SIZE_INDEX,
				    IDB_OBJ_HEAD_SIZE_SIZE, &size_x,  oid);
    }
    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_objectCheck(DbHandle *dbh, const eyedbsm::Oid *oid, int *type,
		  eyedbsm::Oid *cls_oid)
  {
    ObjectHeader hdr;
    RPCStatus rpc_status = IDB_objectHeaderRead(dbh, oid, &hdr);

    if (rpc_status != RPCSuccess)
      {
	*type = 0;
	return rpc_status;
      }

    // WARNING: disconnected on the 8/01/99
    // is it a good idea to have disconnected this code?
#if 0
    if (ObjectPeer::isRemoved(hdr))
      {
	Oid xoid(oid);
	return rpcStatusMake(Exception::make(IDB_ERROR,
					     "object '%s' does not exist "
					     "anymore",
					     xoid.getString()));
      }
#endif

    *type = hdr.type;

    Database *db = (Database *)dbh->db;
    Oid cloid(hdr.oid_cl);

    if (cloid.isValid() && !db->getSchema()->getClass(cloid)) {

      ClassConversion::Context *conv_ctx = 0;
      const Class *cls = 0;
      RPCStatus rpc_status;
      rpc_status = IDB_convertPrologue(db, hdr.oid_cl, cls, conv_ctx);
      if (rpc_status) return rpc_status;
      if (!cls)
	return rpcStatusMake
	  (Exception::make(IDB_ERROR, "internal conversion error: "
			   "cannot find old class %s",
			   Oid(hdr.oid_cl).toString()));
      Size idr_psize = 0;
      int objsize = IDB_getObjectSize(cls, &idr_psize, 0, &hdr);
      Size minsize = (!conv_ctx ? idr_psize :
		      ClassConversion::computeSize(conv_ctx, hdr.size));

      Data idr = 0;
      rpc_status = IDB_instanceRead(dbh, 0, &idr, &hdr, LockS, oid, 0,
				    minsize);
      if (rpc_status) {
	free(idr);
	return rpc_status;
      }

      rpc_status = IDB_convertEpilogue(dbh, db, cls, conv_ctx, idr, &hdr,
				       idr_psize, oid);

      free(idr);
      *cls_oid = *cls->getOid().getOid();
      return rpc_status;
    }

    *cls_oid = hdr.oid_cl;
    return RPCSuccess;
  }

  RPCStatus
  IDB_objectCheckAccess(DbHandle *dbh, Bool write, const eyedbsm::Oid *oid,
			Bool *access)
  {
    eyedbsm::Status se_status;
    eyedbsm::Boolean xaccess;
    se_status = eyedbsm::objectCheckAccess(dbh->sedbh, write ? eyedbsm::True : eyedbsm::False,
					   oid, &xaccess);
    if (se_status)
      {
	//se_statusPrint(se_status, "IDB_objectCheckAccess");
	return rpcStatusMake_se(se_status);
      }

    *access = (xaccess ? True : False);
    return RPCSuccess;
  }

  RPCStatus
  IDB_oidMake(DbHandle *dbh, ObjectHeader *hdr, short dspid, unsigned int size,
	      eyedbsm::Oid *oid)
  {
    CHECK_WRITE((Database *)dbh->db);

    RPCStatus rpc_status;

    Database *db = (Database *)dbh->db;
    const Class *cls = db->getSchema()->getClass(hdr->oid_cl);
    rpc_status = getDspid(db, cls, dspid);
    if (rpc_status)
      return rpc_status;

    /* changed the 23/08/99 */
    /*rpc_status = rpcStatusMake_se(eyedbsm::objectCreate(dbh->sedbh, se_ObjectNone,
      size, oid));*/
    rpc_status = rpcStatusMake_se(eyedbsm::objectCreate(dbh->sedbh,
							eyedbsm::ObjectZero,
							size, dspid, oid));

    if (rpc_status)
      return rpc_status;

    db->addMarkCreated(Oid(*oid));

    unsigned char temp[IDB_OBJ_HEAD_SIZE];

    Offset offset = 0;
    Size alloc_size = sizeof(temp);
    Data idr = temp;
    object_header_code(&idr, &offset, &alloc_size, hdr);

    rpc_status = rpcStatusMake_se
      (eyedbsm::objectWrite(dbh->sedbh, 0, IDB_OBJ_HEAD_SIZE, temp, oid));

    return rpc_status;
  }

  /* raw data functions */

  RPCStatus
  IDB_dataCreate(DbHandle *dbh, short dspid, unsigned int size, const Data idr,
		 eyedbsm::Oid *oid, void *xdata)
  {
    CHECK_WRITE((Database *)dbh->db);
    eyedbsm::Status status;

    lock_data((Data *)&idr, xdata);

    status = eyedbsm::objectCreate(dbh->sedbh, idr, size, dspid, oid);
    if (!status)
      ((Database *)dbh->db)->addMarkCreated(Oid(*oid));

    unlock_data(idr, xdata);
    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_dataDelete(DbHandle *dbh, const eyedbsm::Oid *oid)
  {
    CHECK_WRITE((Database *)dbh->db);
    return rpcStatusMake_se(eyedbsm::objectDelete(dbh->sedbh, oid));
  }

  RPCStatus
  IDB_dataRead(DbHandle *dbh, int offset, unsigned int size, Data idr, short *pdspid, const eyedbsm::Oid *oid, void *xdata)
  {
    eyedbsm::Status status;
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (data)
      {
	if (size <= data->buff_size)
	  data->status = rpc_BuffUsed;
	else
	  {
	    data->status = rpc_TempDataUsed;
	    data->data = malloc(size);
	  }

	status = eyedbsm::objectRead(dbh->sedbh, offset, size, data->data, eyedbsm::DefaultLock,
				     pdspid, 0, oid);

	if (status == eyedbsm::Success)
	  data->size = size;
	else
	  data->size = 0;
      }
    else
      status = eyedbsm::objectRead(dbh->sedbh, offset, size, idr, eyedbsm::DefaultLock, pdspid, 0, oid);

    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_dataWrite(DbHandle *dbh, int offset, unsigned int size, const Data idr, const eyedbsm::Oid *oid, void *xdata)
  {
    CHECK_WRITE((Database *)dbh->db);

    eyedbsm::Status status;
    lock_data((Data *)&idr, xdata);

    status = eyedbsm::objectWrite(dbh->sedbh, offset, size, idr, oid);

    unlock_data(idr, xdata);
    return rpcStatusMake_se(status);
  }

  RPCStatus
  IDB_dataSizeGet(DbHandle *dbh, const eyedbsm::Oid *oid, unsigned int *_psize) // TBD: V3_API_size_t
  {
    RPCStatus rpc_status;
    size_t psize; // V3_API_size_t
    rpc_status = rpcStatusMake_se(eyedbsm::objectSizeGet(dbh->sedbh, &psize, eyedbsm::DefaultLock, oid));
    *_psize = psize;
    return rpc_status;
  }

  RPCStatus
  IDB_dataSizeModify(DbHandle *dbh, unsigned int size, const eyedbsm::Oid *oid)
  {
    return rpcStatusMake_se(eyedbsm::objectSizeModify(dbh->sedbh, size, eyedbsm::True,
						      oid));
  }

  /* vardim */
  static RPCStatus
  IDB_VDgetAttribute(Database *db, const eyedbsm::Oid *oid_cl, int num,
		     const char *fname, Attribute **pitem)
  {
    Class *cls = db->getSchema()->getClass(oid_cl);

    if (!cls)
      return rpcStatusMake(IDB_FATAL_ERROR,
			   "class '%s' not found in %s()",
			   OidGetString(oid_cl), fname);

    if (!cls->asAgregatClass())
      return rpcStatusMake(IDB_FATAL_ERROR,
			   "class '%s' is not a agregat_class in %s()",
			   cls->getName(), fname);

    unsigned int count;
    const Attribute **items = ((AgregatClass *)cls)->getAttributes(count);
    if (num < 0 || num >= count)
      return rpcStatusMake(IDB_FATAL_ERROR, "invalid item number `%d' in for class '%s' in %s()",
			   num, cls->getName(), fname);

    *pitem = (Attribute *)items[num];
    return RPCSuccess;
  }

  static RPCStatus
  IDB_VDmakeIDR(DbHandle *dbh, Attribute *item, const eyedbsm::Oid *oid,
		Data *pidr, int *count, int &offset)
  {
    eyedbsm::Status se_status;
    size_t size; // V3_API_size_t

    se_status = eyedbsm::objectSizeGet(dbh->sedbh, &size, eyedbsm::DefaultLock, oid);

    if (se_status)
      return rpcStatusMake_se(se_status);

    if (!size)
      {
	*pidr = 0;
	return RPCSuccess;
      }

    *pidr = (unsigned char *)malloc(size);
    se_status = eyedbsm::objectRead(dbh->sedbh, 0, size, *pidr, eyedbsm::DefaultLock, 0, 0, oid);

    if (se_status)
      {
	free(*pidr);
	return rpcStatusMake_se(se_status);
      }

    Size idr_item_psize, idr_psize, idr_inisize;
    Offset off;
    item->getPersistentIDR(off, idr_item_psize, idr_psize, idr_inisize);

    offset = (int)off;
    *count = size/idr_item_psize;

    return RPCSuccess;
  }

  RPCStatus IDB_VDcheckSize(Attribute *item, int count, unsigned int size,
			    int &offset)
  {
    Size idr_item_psize, idr_psize, idr_inisize;
    Offset off;
    item->getPersistentIDR(off, idr_item_psize, idr_psize, idr_inisize);

    offset = (int)off;
    return RPCSuccess;
  }

  RPCStatus
  IDB_VDdataCreate(DbHandle *dbh, short dspid, const eyedbsm::Oid *actual_oid_cl,
		   const eyedbsm::Oid *oid_cl, int num,
		   int count, int size, const Data idr,
		   const eyedbsm::Oid *agr_oid, eyedbsm::Oid *data_oid,
		   const Data idx_data, int idx_size, void *xdata,
		   void *xidx_data)
  {
    Database *db = (Database *)dbh->db;
    Status status;
    RPCStatus rpc_status;
    Attribute *item;

    rpc_status = IDB_VDgetAttribute(db, oid_cl, num, "VDdataCreate", &item);

    if (rpc_status)
      return rpc_status;

    int offset;
    rpc_status = IDB_VDcheckSize(item, count, size, offset);

    if (rpc_status)
      return rpc_status;

    lock_data((Data *)&idr, xdata);
    lock_data((Data *)&idx_data, xidx_data);

    rpc_status = IDB_dataCreate(dbh, dspid, size, idr, data_oid, 0);

    if (rpc_status) {
      unlock_data(idx_data, xidx_data);
      unlock_data(idr, xdata);
      return rpc_status;
    }

    Oid _agr_oid(agr_oid);
    Oid moid(actual_oid_cl);
    AttrIdxContext idx_ctx(idx_data, idx_size);
    //printf("VDdataCreate(%s)\n", item->getName());
    status = item->createIndexEntry(db, idr, &_agr_oid, &moid, -offset,
				    count, size, 0, False, idx_ctx);

    if (status) {
      idx_ctx.realizeIdxOP(False);
      unlock_data(idr, xdata);
      unlock_data(idx_data, xidx_data);
      return rpcStatusMake(status);
    }

    status = idx_ctx.realizeIdxOP(True);
    unlock_data(idr, xdata);
    unlock_data(idx_data, xidx_data);
    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_VDdataWrite(DbHandle *dbh, const eyedbsm::Oid *actual_oid_cl,
		  const eyedbsm::Oid *oid_cl, int num,
		  int count, unsigned int size, const Data idr, const eyedbsm::Oid *agr_oid,
		  const eyedbsm::Oid *data_oid,
		  const Data idx_data, int idx_size, void *xdata,
		  void *xidx_data)
  {
    Database *db = (Database *)dbh->db;
    RPCStatus rpc_status;
    Status status;
    Attribute *item;

    rpc_status = IDB_VDgetAttribute(db, oid_cl, num, "VDdataWrite", &item);

    if (rpc_status)
      return rpc_status;

    int offset;
    rpc_status = IDB_VDcheckSize(item, count, size, offset);

    if (rpc_status)
      return rpc_status;

    lock_data((Data *)&idr, xdata);
    lock_data((Data *)&idx_data, xidx_data);

    Oid _data_oid(data_oid);
    Oid _agr_oid(agr_oid);

    Oid moid(actual_oid_cl);
    AttrIdxContext idx_ctx(idx_data, idx_size);
    //printf("VDdataWrite(%s)\n", item->getName());
    status = item->updateIndexEntry(db, idr, &_agr_oid, &moid, -offset,
				    &_data_oid, count, 0, False, idx_ctx);

    if (status) {
      idx_ctx.realizeIdxOP(False);
      unlock_data(idr, xdata);
      unlock_data(idx_data, xidx_data);
      return rpcStatusMake(status);
    }

    status = idx_ctx.realizeIdxOP(True);
    if (status) {
      unlock_data(idr, xdata);
      unlock_data(idx_data, xidx_data);
      return rpcStatusMake(status);
    }

    rpc_status = IDB_dataWrite(dbh, 0, size, idr, data_oid, 0);

    unlock_data(idr, xdata);
    unlock_data(idx_data, xidx_data);

    return rpc_status;
  }

  RPCStatus
  IDB_VDdataDelete(DbHandle *dbh, const eyedbsm::Oid *actual_oid_cl,
		   const eyedbsm::Oid *oid_cl, int num,
		   const eyedbsm::Oid *agr_oid, const eyedbsm::Oid *data_oid,
		   const Data idx_data, int idx_size, void *xidx_data)
  {
    Database *db = (Database *)dbh->db;
    RPCStatus rpc_status;
    Status status;
    Attribute *item;
    int count;

    rpc_status = IDB_VDgetAttribute(db, oid_cl, num, "VDdataDelete", &item);

    if (rpc_status)
      return rpc_status;

    Data idr;
    int offset;
    rpc_status = IDB_VDmakeIDR(dbh, item, data_oid, &idr, &count, offset);

    if (rpc_status)
      return rpc_status;

    lock_data((Data *)&idx_data, xidx_data);

    Oid _data_oid(data_oid);
    Oid _agr_oid(agr_oid);
    Oid moid(actual_oid_cl);
    AttrIdxContext idx_ctx(idx_data, idx_size);
    //  printf("VDdataDelete(%s)\n", item->getName());
    status = item->removeIndexEntry(db, idr, &_agr_oid, &moid, -offset,
				    &_data_oid, count, 0, False, idx_ctx);


    if (status) {
      idx_ctx.realizeIdxOP(False);
      free(idr);
      unlock_data(idx_data, xidx_data);
      return rpcStatusMake(status);
    }

    status = idx_ctx.realizeIdxOP(True);
    if (status) {
      free(idr);
      unlock_data(idx_data, xidx_data);
      return rpcStatusMake(status);
    }

    free(idr);
    unlock_data(idx_data, xidx_data);

    if (status)
      return rpcStatusMake(status);

    return IDB_dataDelete(dbh, data_oid);
  }

  RPCStatus
  IDB_schemaComplete(DbHandle *dbh, const char *schname)
  {
    CHECK_WRITE((Database *)dbh->db);

    Status status;
    RPCStatus rpc_status;
    Database *db = (Database *)dbh->db;
    Schema *sch = db->getSchema();

    if (status = sch->deferredCollRegisterRealize(dbh))
      return rpcStatusMake(status);

    if (!*schname)
      return rpcStatusMake(IDB_ERROR, "schema name must be set");

    sch->setName(schname);

    if (sch->getOid().isValid())
      {
	char name[IDB_SCH_NAME_SIZE];
	unsigned char *data = (unsigned char *)name;
	Offset start = 0;
	Size size = sizeof name;
	string_code(&data, &start, &size, schname);
      
	eyedbsm::Status se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_SCH_NAME_INDEX,
							 IDB_SCH_NAME_SIZE,
							 name, sch->getOid().getOid());
	if (se_status)
	  return rpcStatusMake_se(se_status);
      }
      
    return rpcStatusMake(sch->complete(True, True));
  }


  RPCStatus
  IDB_attributeIndexCreate(DbHandle *dbh, const eyedbsm::Oid *cloid, int num,
			   int mode, eyedbsm::Oid *multi_idx_oid,
			   Data idx_ctx_data, Size size, void *xdata)
  {
#if 1
    assert(0);
    return RPCSuccess;
#else
    Database *db = (Database *)dbh->db;
    Class *cl = db->getSchema()->getClass(*cloid);
    Attribute *attr = (Attribute *)cl->asAgregatClass()->getAttribute(num);

    if (!attr)
      return rpcStatusMake(IDB_ERROR, "cannot find attribute #%d "
			   "in class %s\n", num, cl->getName());
  
    lock_data((Data *)&idx_ctx_data, xdata);

    AttrIdxContext idx_ctx(idx_ctx_data, size);
    Oid idx_oid(multi_idx_oid);
    Status s = attr->createIndex_realize(db, mode, cl, idx_ctx,
					 idx_oid);

    if (s)
      {
	unlock_data(idx_ctx_data, xdata);
	return rpcStatusMake(s);
      }

    unlock_data(idx_ctx_data, xdata);
    return RPCSuccess;
#endif
  }

  RPCStatus
  IDB_attributeIndexRemove(DbHandle *dbh, const eyedbsm::Oid *cloid, int num,
			   int mode, Data idx_ctx_data, Size size,
			   void *xdata)
  {
#if 1
    assert(0);
    return RPCSuccess;
#else
    Database *db = (Database *)dbh->db;
    Class *cl = db->getSchema()->getClass(*cloid);
    Attribute *attr = (Attribute *)cl->asAgregatClass()->getAttribute(num);
    if (!attr)
      return rpcStatusMake(IDB_ERROR, "cannot find attribute #%d in class %s\n",
			   num, cl->getName());
    lock_data((Data *)&idx_ctx_data, xdata);

    AttrIdxContext idx_ctx(idx_ctx_data, size);

    Status s = 
      (attr->destroyIndex(db, idx_ctx,
			  (Attribute::AttributeIndexMode)mode));

    unlock_data(idx_ctx_data, xdata);
    return rpcStatusMake(s);
#endif
  }

  char *attrcomp_delete_ud =  (char*)"eyedb:attr_comp_delete:reentrant"; /*@@@@warning cast*/

  RPCStatus
  IDB_attrCompPrologue(Database *db, const eyedbsm::Oid * objoid,
		       const Class *&cls,
		       AttributeComponent *&attr_comp,
		       const Attribute *&attr, Bool check,
		       AttrIdxContext *idx_ctx = 0,
		       AttributeComponent **oattr_comp = 0)
  {
    Oid oid(*objoid);
    Status s;

    if (oattr_comp) {
      s = db->loadObject(oid, (Object *&)*oattr_comp);
      if (s) return rpcStatusMake(s);
    }

    s = db->reloadObject(oid, (Object *&)attr_comp);
    if (s) return rpcStatusMake(s);

    if (attr_comp->isRemoved())
      return rpcStatusMake(IDB_ERROR, "attribute component %s is removed",
			   oid.toString());

    s = Attribute::checkAttrPath(db->getSchema(), cls, attr,
				 attr_comp->getAttrpath().c_str(), idx_ctx);
    if (s) return rpcStatusMake(s);

    if (!attr->isIndirect() && !attr->isBasicOrEnum() &&
	!attr->getClass()->asCollectionClass())
      return rpcStatusMake(Exception::make
			   (IDB_ERROR,
			    "attribute path '%s' is not indirect neither "
			    "basic literal", attr_comp->getAttrpath().c_str()));

    /*
      if (!check || !attr_comp->getPropagate())
      return RPCSuccess;
    */
    if (!check)
      return RPCSuccess;

    // magic value set in Index::reimplement() to inform backend that it is
    // a reimplementation
    if (attr_comp->asIndex() &&
	attr_comp->asIndex()->getImplHintsCount() > IDB_IDX_MAGIC_HINTS &&
	attr_comp->asIndex()->getImplHints(IDB_IDX_MAGIC_HINTS) ==
	IDB_IDX_MAGIC_HINTS_VALUE) {
      return RPCSuccess;
    }

    AttributeComponent *cattr_comp;
    s = attr_comp->find(db, cls->getParent(), cattr_comp);
    if (s) return rpcStatusMake(s);

    //if (cattr_comp) {
    if (cattr_comp && cattr_comp->getPropagate()) {
      return rpcStatusMake
	(Exception::make(IDB_ERROR,
			 "cannot delete component %s because "
			 "of propagation property: "
			 "must delete component "
			 "of parent class first",
			 attr_comp->getName().c_str()));
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_attrCompCheckInClass(AttributeComponent *comp, const Class *cls,
			   Bool &found)
  {
    AttributeComponent *xcomp;
    Status s = cls->getAttrComp(comp->getName().c_str(), xcomp);
    if (s) return rpcStatusMake(s);

    if (xcomp) {
      /*
	printf("comp %s [%s] found for class %s ",
	comp->getName(), comp->getOid().toString(), cls->getName());
      */
      found = True;
      return RPCSuccess;
    }

    /*
      printf("comp %s [%s] NOT found for class %s: should create\n",
      comp->getName(), comp->getOid().toString(),
      cls->getName());
    */
  
    found = False;
    return RPCSuccess;
  }

  RPCStatus
  IDB_attrCompPropagate(Database *db, const Class *cls,
			AttributeComponent *attr_comp, Bool create)
  {
    // should be factorized into: attrCompPropagate
    if (!attr_comp->getPropagate())
      return RPCSuccess;

    //printf("\nPropagating...\n");
    Class **subclasses;
    unsigned int subclass_cnt;
    Status s = cls->getSubClasses(subclasses, subclass_cnt);
    if (s) return rpcStatusMake(s);

    for (int i = 0; i < subclass_cnt; i++) {
      if (!subclasses[i]->getParent()->compare(cls)) continue;
      AttributeComponent *cattr_comp;
      if (create) {
	cattr_comp = attr_comp->xclone(db, subclasses[i]);
	s = cattr_comp->store();
      }
      else {
	s = attr_comp->find(db, subclasses[i], cattr_comp);
	if (s) return rpcStatusMake(s);
	if (!cattr_comp)
	  return rpcStatusMake
	    (Exception::make(IDB_ERROR, "attribute component propagation "
			     "removing internal error: component %s does "
			     "not exist in class %s",
			     attr_comp->getName().c_str(),
			     subclasses[i]->getName()));

	assert(cattr_comp);
	cattr_comp->setUserData(attrcomp_delete_ud, (void *)1);
	s = cattr_comp->remove();
      }
      if (s) return rpcStatusMake(s);
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_attrCompPropagate(Database *db, Class *cls, Bool skipIfFound)
  {
    Class *parent;
    Status status = cls->getParent(db, parent);
    if (status) return rpcStatusMake(status);
    if (!parent)
      return RPCSuccess;

    const LinkedList *complist;
    status = parent->getAttrCompList(complist);
    if (status) return rpcStatusMake(status);
    LinkedListCursor c(complist);
    AttributeComponent *comp;
    while (c.getNext((void *&)comp)) {
      if (comp->getPropagate()) {
	comp = comp->xclone(db, cls);

	if (skipIfFound) {
	  Bool found;
	  RPCStatus rpc_status = IDB_attrCompCheckInClass(comp, cls, found);
	  if (rpc_status) return rpc_status;

	  if (found) {
	    comp->release();
	    continue;
	  }
	}

	status = comp->store();
	if (status) {
	  comp->release();
	  return rpcStatusMake(status);
	}
      }
    }

    return RPCSuccess;
  }

  extern eyedbsm::Status
  hash_key(const void *key, unsigned int len, void *hash_data, unsigned int &x);
  //extern eyedbsm::HIdx::hash_key_t hash_key;

  //#define REIMPL_TRACE

  extern char index_backend[];

  RPCStatus
  IDB_indexCreate(DbHandle * dbh, bool index_move, const eyedbsm::Oid * objoid)
  {
    Database *db = (Database *)dbh->db;
    AttrIdxContext idx_ctx;
    AttributeComponent *oattr_comp, *attr_comp;
    const Attribute *attr;

    const Class *cls;
    RPCStatus rpc_status = IDB_attrCompPrologue(db, objoid, cls,
						attr_comp, attr, False,
						&idx_ctx, &oattr_comp);
    if (rpc_status) return rpc_status;
    ObjectReleaser _(attr_comp);
    Index *idx = attr_comp->asIndex();

#ifdef REIMPL_TRACE
    printf("INDEX_CREATE(%s, %s, oid=%s, idxoid=%s, dspid=%d)\n",
	   attr_comp->getAttrpath(), cls->getName(), Oid(objoid).toString(),
	   idx->getIdxOid().toString(),
	   idx->getDspid());
#endif

    if (!idx->getIdxOid().isValid()) {
      Status s = attr->addComponent(db, idx);
      if (s) return rpcStatusMake(s);
      s = attr->updateIndexEntries(db, idx_ctx);
      if (s) return rpcStatusMake(s);
      return IDB_attrCompPropagate(db, cls, idx, True);
    }

    Oid newoid;
    eyedbsm::Status s;

#ifdef REIMPL_TRACE
    printf("Server: INDEX reimplementation process\n");
#endif
    eyedbsm::Idx *seidx = 0;
    s = eyedbsm::Idx::make(dbh->sedbh, *idx->getIdxOid().getOid(), seidx);
    if (s)
      return rpcStatusMake_se(s);

    Bool swap = False;

    if (index_move) {
      //printf("Index moving...\n");
      if (idx->asBTreeIndex()) {
	s = seidx->asBIdx()->move(idx->getDspid(), *newoid.getOid());
      }
      else if (idx->asHashIndex()) {
	HashIndex *hidx = idx->asHashIndex();
	BEMethod_C *mth = hidx->getHashMethod();
	s = seidx->asHIdx()->move(idx->getDspid(), *newoid.getOid(), (mth ? hash_key : 0), mth);
      }
    }
    else if (idx->asHashIndex()) {
      printf("Index reimplementing...\n");
      if (seidx->asBIdx())
	swap = True;
      HashIndex *hidx = idx->asHashIndex();
      int impl_hints_cnt = eyedbsm::HIdxImplHintsCount;
      int impl_hints[eyedbsm::HIdxImplHintsCount];
      int cnt = hidx->getImplHintsCount();
      memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
      for (int i = 0; i < cnt; i++)
	impl_hints[i] = hidx->getImplHints(i);
#ifdef REIMPL_TRACE
      printf("Server: reimplementing HASH INDEX %s new keycount = %d\n",
	     Oid(seidx->oid()).toString(), hidx->getKeyCount());
#endif
      BEMethod_C *mth = hidx->getHashMethod();
      s = seidx->reimplementToHash(*newoid.getOid(), hidx->getKeyCount(), 0,
				   hidx->getDspid(),
				   impl_hints, impl_hints_cnt,
				   (mth ? hash_key : 0), mth);
    }
    else {
      if (seidx->asHIdx())
	swap = True;
      BTreeIndex *bidx = idx->asBTreeIndex();
#ifdef REIMPL_TRACE
      printf("Server: reimplementing BTREE INDEX %s\n", Oid(seidx->oid()).toString());
#endif
      s = seidx->reimplementToBTree(*newoid.getOid(), bidx->getDegree(), bidx->getDspid());
    }

    delete seidx;

    if (!s) {
      Status status = idx->report(dbh->sedbh, newoid);
      if (status) return rpcStatusMake(status);
    }
  
    if (!s) {
      void *ud = idx->setUserData(index_backend, AnyUserData);
      idx->setIdxOid(newoid);
      idx->idx = 0;
      if (swap) {
	Status st = attr->addComponent(db, idx);
	if (st) return rpcStatusMake(st);
      }

      rpc_status = rpcStatusMake(idx->store());
      idx->setUserData(index_backend, ud);
      return rpc_status;
    }

    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_indexRemove(DbHandle * dbh, const eyedbsm::Oid * objoid, int reentrant)
  {
    Database *db = (Database *)dbh->db;
    AttributeComponent *attr_comp;
    const Attribute *attr;

    const Class *cls;
    RPCStatus rpc_status = IDB_attrCompPrologue(db, objoid, cls,
						attr_comp, attr,
						IDBBOOL(!reentrant));
    if (rpc_status) return rpc_status;
    /*
      printf("INDEX_REMOVE(%s, %s, oid=%s, idxoid=%s)\n",
      attr_comp->getAttrpath(), cls->getName(), Oid(objoid).toString(),
      attr_comp->asIndex()->getIdxOid().toString());
    */

    ObjectReleaser _(attr_comp);

    Status s = attr->rmvComponent(db, attr_comp);
    if (s) return rpcStatusMake(s);

    s = attr->destroyIndex(db, (Index *)attr_comp);
    if (s) return rpcStatusMake(s);

    return IDB_attrCompPropagate(db, cls, attr_comp, False);
    //return RPCSuccess;
  }

  RPCStatus
  IDB_constraintCreate(DbHandle * dbh, const eyedbsm::Oid * objoid)
  {
    Database *db = (Database *)dbh->db;
    AttrIdxContext idx_ctx;
    AttributeComponent *attr_comp;
    const Attribute *attr;

    const Class *cls;
    RPCStatus rpc_status = IDB_attrCompPrologue(db, objoid, cls,
						attr_comp,
						attr, False);
    if (rpc_status) return rpc_status;
    /*
      printf("CONSTRAINT_CREATE(%s, %s, %s)\n",
      attr_comp->getAttrpath(), cls->getName(), Oid(objoid).toString());
    */
    ObjectReleaser _(attr_comp);

    if (attr_comp->asCollAttrImpl()) {
      if (!attr->getClass()->asCollectionClass() || attr->isIndirect())
	return rpcStatusMake(IDB_ERROR,
			     "attribute path %s: "
			     "a collection implementation can be tied "
			     "only to a literal collection attribute",
			     attr_comp->getAttrpath().c_str());
    }

    Status s = attr->addComponent(db, attr_comp);
    if (s) return rpcStatusMake(s);

    return IDB_attrCompPropagate(db, cls, attr_comp, True);
  }

  RPCStatus
  IDB_constraintDelete(DbHandle * dbh, const eyedbsm::Oid * objoid, int reentrant)
  {
    Database *db = (Database *)dbh->db;
    AttrIdxContext idx_ctx;
    AttributeComponent *attr_comp;
    const Attribute *attr;

    const Class *cls;
    RPCStatus rpc_status = IDB_attrCompPrologue(db, objoid, cls,
						attr_comp,
						attr, IDBBOOL(!reentrant));
    if (rpc_status) return rpc_status;

    /*
      printf("CONSTRAINT_DELETE(%p, %s, %s, %s, reentrant=%d)\n", attr_comp,
      attr_comp->getAttrpath(), cls->getName(), Oid(objoid).toString(),
      reentrant);
    */
    ObjectReleaser _(attr_comp);
    Status s = attr->rmvComponent(db, attr_comp);
    if (s) return rpcStatusMake(s);

    return IDB_attrCompPropagate(db, cls, attr_comp, False);
  }


  static RPCStatus
  IDB_indexPrologue(DbHandle *dbh, const eyedbsm::Oid * _idxoid, Index *&idx)
  {
    Database *db = (Database *)dbh->db;
    Oid idxoid(_idxoid);
    Status s = db->loadObject(idxoid, (Object *&)idx);
    if (s) return rpcStatusMake(s);

    if (idx->getIdxOid().isValid()) {
      s = Attribute::openMultiIndexRealize(db, idx);
      if (s) return rpcStatusMake(s);
      if (!idx->idx)
	return rpcStatusMake(IDB_ERROR, "invalid null index %s",
			     idxoid.toString());
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_indexGetCount(DbHandle * dbh, const eyedbsm::Oid * idxoid, int * cnt)
  {
    Index *idx;
    RPCStatus rpc_status = IDB_indexPrologue(dbh, idxoid, idx);
    if (rpc_status) return rpc_status;

    if (!idx->getIdxOid().isValid()) {
      *cnt = 0;
      return RPCSuccess;
    }

    *cnt = idx->idx->getCount();
    return RPCSuccess;
  }

  RPCStatus
  IDB_indexGetStats(DbHandle * dbh, const eyedbsm::Oid * idxoid,
		    Data *rstats, void *xdata)
  {
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (data) {
      data->status = rpc_BuffUsed;
      data->size = 0;
    }

    Index *idx;
    RPCStatus rpc_status = IDB_indexPrologue(dbh, idxoid, idx);
    if (rpc_status) return rpc_status;

    if (!idx->getIdxOid().isValid())
      return RPCSuccess;

    if (idx->asHashIndex()) {
      eyedbsm::HIdx::Stats stats;
      eyedbsm::Status s = idx->asHashIndex()->idx->asHIdx()->getStats(stats);
      if (s) return rpcStatusMake_se(s);
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_stats(IndexImpl::Hash, &stats,
				      &data->size);
      }
      else
	make_index_stats(stats, rstats);
    }
    else {
      eyedbsm::BIdx::Stats stats;
      eyedbsm::Status s = idx->asBTreeIndex()->idx->asBIdx()->getStats(stats);
      if (s) return rpcStatusMake_se(s);
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_stats(IndexImpl::BTree, &stats,
				      &data->size);
      }
      else
	make_index_stats(stats, rstats);
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_indexSimulStats(DbHandle * dbh, const eyedbsm::Oid * idxoid,
		      const Data impl, void *xidata,
		      Data *rstats, void * xrdata)
  {
    rpc_ServerData *rdata = (rpc_ServerData *)xrdata;

    if (rdata) {
      rdata->status = rpc_BuffUsed;
      rdata->size = 0;
    }

    Index *idx;
    Database *db = (Database *)dbh->db;
    RPCStatus rpc_status = IDB_indexPrologue(dbh, idxoid, idx);
    if (rpc_status) return rpc_status;

    if (!idx->getIdxOid().isValid()) {
      if (rstats)
	*(IndexStats **)rstats = 0;
      return RPCSuccess;
    }

    lock_data((Data *)&impl, xidata);
    IndexImpl *idximpl;
    Offset offset = 0;
    Status status = IndexImpl::decode(db, impl, offset, idximpl);
    if (status) {
      unlock_data(impl, xidata);
      return rpcStatusMake(status);
    }

    if (idx->asHashIndex()) {
      eyedbsm::HIdx hidx(dbh->sedbh, idx->getIdxOid().getOid(),
			 hash_key, idximpl->getHashMethod());
      eyedbsm::Status s = hidx.status();
      unsigned int impl_hints_cnt;
      const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
      eyedbsm::HIdx::Stats stats;
      s = hidx.simulate(stats, idximpl->getKeycount(), 0,
			impl_hints, impl_hints_cnt,
			(idximpl->getHashMethod() ? hash_key : 0),
			idximpl->getHashMethod());
      if (s) {
	unlock_data(impl, xidata);
	return rpcStatusMake_se(s);
      }
      if (rdata) {
	rdata->status = rpc_TempDataUsed;
	rdata->data = code_index_stats(IndexImpl::Hash, &stats,
				       &rdata->size);
      }
      else
	make_index_stats(stats, rstats);
    }

    unlock_data(impl, xidata);
    return RPCSuccess;
  }

  RPCStatus
  IDB_collectionGetImplStats(DbHandle * dbh, int idxtype,
			     const eyedbsm::Oid * idxoid,
			     Data * rstats, void *xrdata)
  {
    rpc_ServerData *rdata = (rpc_ServerData *)xrdata;

    if (rdata) {
      rdata->status = rpc_BuffUsed;
      rdata->size = 0;
    }

    if (idxtype == IndexImpl::Hash) {
      eyedbsm::HIdx hidx(dbh->sedbh, idxoid);
      eyedbsm::Status s = hidx.status();
      if (s) return rpcStatusMake_se(s);
      eyedbsm::HIdx::Stats stats;
      s = hidx.getStats(stats);
      if (s) return rpcStatusMake_se(s);
      if (rdata) {
	rdata->status = rpc_TempDataUsed;
	rdata->data = code_index_stats(IndexImpl::Hash, &stats,
				       &rdata->size);
      }
      else
	make_index_stats(stats, rstats);
    }
    else {
      eyedbsm::BIdx bidx(dbh->sedbh, *idxoid);
      eyedbsm::Status s = bidx.status();
      if (s) return rpcStatusMake_se(s);
      eyedbsm::BIdx::Stats stats;
      s = bidx.getStats(stats);
      if (s) return rpcStatusMake_se(s);
      if (rdata) {
	rdata->status = rpc_TempDataUsed;
	rdata->data = code_index_stats(IndexImpl::BTree, &stats,
				       &rdata->size);
      }
      else
	make_index_stats(stats, rstats);
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_collectionSimulImplStats(DbHandle * dbh, int idxtype,
			       const eyedbsm::Oid * idxoid,
			       const Data impl, void *xidata,
			       Data *rstats, void *xrdata)
  {
    rpc_ServerData *rdata = (rpc_ServerData *)xrdata;

    if (rdata) {
      rdata->status = rpc_BuffUsed;
      rdata->size = 0;
    }

    Database *db = (Database *)dbh->db;
    lock_data((Data *)&impl, xidata);
    IndexImpl *idximpl;
    Offset offset = 0;
  
    Status status = IndexImpl::decode(db, impl, offset, idximpl);
    if (status) {
      unlock_data(impl, xidata);
      return rpcStatusMake(status);
    }

    if (idxtype == IndexImpl::Hash) {
      eyedbsm::HIdx hidx(dbh->sedbh, idxoid,
			 hash_key, idximpl->getHashMethod());
      eyedbsm::Status s = hidx.status();
      unsigned int impl_hints_cnt;
      const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
      eyedbsm::HIdx::Stats stats;
      s = hidx.simulate(stats, idximpl->getKeycount(), 0,
			impl_hints, impl_hints_cnt,
			(idximpl->getHashMethod() ? hash_key : 0),
			idximpl->getHashMethod());
      if (s) {
	unlock_data(impl, xidata);
	return rpcStatusMake_se(s);
      }

      if (rdata) {
	rdata->status = rpc_TempDataUsed;
	rdata->data = code_index_stats(IndexImpl::Hash,
				       &stats, &rdata->size);
      }
      else
	make_index_stats(stats, rstats);
    }
    else {// not implemented
      *(BTreeIndexStats **)rstats = 0;
      unlock_data(impl, xidata);
      return rpcStatusMake(Exception::make(IDB_ERROR,
					   "btree simulation is not "
					   "yet implemented"));
    }

    unlock_data(impl, xidata);
    return RPCSuccess;
  }

  RPCStatus
  IDB_indexGetImplementation(DbHandle * dbh, const eyedbsm::Oid * idxoid, Data * impl, void * ximpl)
  {
    rpc_ServerData *data = (rpc_ServerData *)ximpl;

    if (data) {
      data->status = rpc_BuffUsed;
      data->size = 0;
    }

    Index *idx;
    RPCStatus rpc_status = IDB_indexPrologue(dbh, idxoid, idx);
    if (rpc_status) return rpc_status;

    if (!idx->getIdxOid().isValid())
      return RPCSuccess;

    if (idx->asHashIndex()) {
      const eyedbsm::HIdx::_Idx &ix = idx->asHashIndex()->idx->asHIdx()->getIdx();
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_impl(ix, &data->size);
      }
      else
	make_index_impl(ix, impl);
    }

    if (idx->asBTreeIndex()) {
      eyedbsm::BIdx *ix = idx->asBTreeIndex()->idx->asBIdx();
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_impl(*ix, &data->size);
      }
      else
	make_index_impl(*ix, impl);
    }

    return RPCSuccess;
  }

  RPCStatus
  IDB_collectionGetImplementation(DbHandle * dbh, int idxtype, const eyedbsm::Oid * idxoid, Data * impl, void * ximpl)
  {
    rpc_ServerData *data = (rpc_ServerData *)ximpl;

    if (data) {
      data->status = rpc_BuffUsed;
      data->size = 0;
    }

    eyedbsm::Idx *seidx = 0;
    eyedbsm::Status s = eyedbsm::Idx::make(dbh->sedbh, *idxoid, seidx);
    if (s)
      return rpcStatusMake_se(s);

    //if (idxtype == IndexImpl::Hash)
    //eyedbsm::HIdx hidx(dbh->sedbh, idxoid);
    if (seidx->asHIdx()) {
      eyedbsm::HIdx *hidx = seidx->asHIdx();
      s = hidx->status();
      if (s) {delete seidx; return rpcStatusMake_se(s);}
      const eyedbsm::HIdx::_Idx &ix = hidx->getIdx();
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_impl(ix, &data->size);
      }
      else
	make_index_impl(ix, impl);
    }
    else {
      eyedbsm::BIdx *bidx = seidx->asBIdx();
      //eyedbsm::BIdx bidx(dbh->sedbh, *idxoid);
      s = bidx->status();
      if (s) {delete seidx; return rpcStatusMake_se(s);}
      if (data) {
	data->status = rpc_TempDataUsed;
	data->data = code_index_impl(*bidx, &data->size);
      }
      else
	make_index_impl(*bidx, impl);
    }

    delete seidx;
    return RPCSuccess;
  }

  /* collections */
  CollectionBE *
  IDB_getCollBE(const char *from, Database *db, DbHandle *dbh,
		const eyedbsm::Oid *colloid, Status *status,
		Bool locked = False)
  {
    CollectionBE *collbe;

    if (!isOidValid(colloid)) {
      *status = Exception::make(IDB_ERROR, "invalid null oid collection");
      return 0;
    }

    /*
      printf("getCollBE called from %s\n", from);
      printf("%s: size=%d locked=%d: ", 
      db->getName(), db->getBEQueue()->getCollectionCount(), locked);
    */

    Oid _oid(colloid);
    if (!(collbe = db->getBEQueue()->getCollection(&_oid, dbh))) {
      collbe = new CollectionBE(db, dbh, &_oid, locked);

      //printf("not found -> %p\n", collbe);
      if (*status = collbe->getStatus()) {
	delete collbe;
	return 0;
      }

      if (locked)
	db->getBEQueue()->addCollection(collbe, dbh);
    }
    /*
      else
      printf("FOUND -> %p\n", collbe);
    */

    *status = Success;
    return collbe;
  }

  //static Bool collectionLocked = False;

  void
  IDB_free(Database *db, CollectionBE *collbe)
  {
    if (!collbe->isLocked())
      delete collbe;
  }

  // 10/09/05
  static Bool coll_hidx_oid = False;

  static RPCStatus
  IDB_collectionCreate(DbHandle *dbh, short dspid, Data idr,
		       ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata)
  {
    Database *db = (Database *)dbh->db;
    Offset offset;

    offset = IDB_COLL_OFF_ITEM_SIZE;

    /* item_size */
    eyedblib::int16 item_size;
    int16_decode (idr, &offset, &item_size);

    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Class *cls = db->getSchema()->getClass(oid_cl);

    eyedblib::int32 items_cnt;
    offset = IDB_COLL_OFF_ITEMS_CNT;
    int32_decode (idr, &offset, &items_cnt);

    IndexImpl *idximpl;
    offset = IDB_COLL_OFF_IMPL_BEGIN;
    int impl_type;
    IndexImpl::decode(db, idr, offset, idximpl, &impl_type);

    Oid inv_oid;
    eyedblib::int16 inv_item;

    offset = IDB_COLL_OFF_INV_OID;
    eyedbsm::Oid se_inv_oid;
    oid_decode(idr, &offset, &se_inv_oid);
    inv_oid.setOid(se_inv_oid);
    int16_decode(idr, &offset, &inv_item);

    Data idx_data = 0;
    eyedblib::int16 idx_data_size = 0;

    Bool is_literal = False;
    Bool is_pure_literal = False;
    if (db->getVersionNumber() >= 20414) {
      char x;
      char_decode(idr, &offset, &x);
      Collection::decodeLiteral(x, is_literal, is_pure_literal);
      //is_literal = IDBBOOL(x);
      int16_decode(idr, &offset, &idx_data_size);
      idx_data = idr+offset;
    }

    eyedbsm::Idx *idx1, *idx2;
    eyedbsm::Oid idx1_oid, idx2_oid;

    /*
    printf("IDB_collectionCreate dspid = %d\n", dspid);
    printf("creating collection %s\n", idximpl->getHintsString().c_str());
    printf("idximpl->getType() %d %d\n", idximpl->getType(), IndexImpl::BTree);
    */

    if (impl_type == CollImpl::BTreeIndex) {
      eyedbsm::BIdx::KeyType ktypes;

      ktypes.type   = eyedbsm::Idx::tUnsignedChar;
      ktypes.count  = item_size;
      ktypes.offset = 0;
    
      idx1 = new eyedbsm::BIdx(dbh->sedbh, sizeof(eyedblib::int32), &ktypes, dspid,
			       idximpl->getDegree());
      idx1->asBIdx()->open();
    }
    else if (impl_type == CollImpl::HashIndex) {
      eyedbsm::Idx::KeyType ktype;
      if (coll_hidx_oid) {
	ktype.type = eyedbsm::Idx::tOid;
	ktype.count = 1;
      }
      else {
	ktype.type = eyedbsm::Idx::tUnsignedChar;
	ktype.count = item_size;
      }
      ktype.offset = 0;
      unsigned int impl_hints_cnt;
      const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
      idx1 = new eyedbsm::HIdx(dbh->sedbh, ktype, 
			       sizeof(eyedblib::int32), dspid, 0, idximpl->getKeycount(),
			       impl_hints, impl_hints_cnt);
      idx1->asHIdx()->open();
    }
    else if (impl_type == CollImpl::NoIndex) {
      return rpcStatusMake(IDB_ERROR, "collection: noindex implementation is not yet supported");
    }
    else {
      return rpcStatusMake(IDB_ERROR, "collection: unknown implementation %d", impl_type);
    }

    idx1_oid = idx1->oid();

    /*
      if (idx1->status())
      se_statusPrint(idx1->status(), "collection create");
    */

    if (idx1->status())
      return rpcStatusMake_se(idx1->status());

    if (eyedb_is_type(*hdr, _CollList_Type) ||
	eyedb_is_type(*hdr, _CollArray_Type)) {

      eyedbsm::BIdx::KeyType ktypes;
      
      // EV: changed 14/12/01
      ktypes.type   = eyedbsm::Idx::tInt32;
      ktypes.count  = 1;
      ktypes.offset = 0;
	  

      idx2 = new eyedbsm::BIdx(dbh->sedbh, item_size, &ktypes, dspid);
      idx2->asBIdx()->open();

      idx2_oid = idx2->oid();
      
      if (idx2->status())
	return rpcStatusMake(IDB_ERROR, "");
    }
    else {
      idx2 = 0;
      idx2_oid = *getInvalidOid();
    }

    Size alloc_size = hdr->size;

    offset = IDB_COLL_OFF_IDX1_OID;
    oid_code (&idr, &offset, &alloc_size, &idx1_oid);
    offset = IDB_COLL_OFF_IDX2_OID;
    oid_code (&idr, &offset, &alloc_size, &idx2_oid);

    RPCStatus rpc_status = IDB_instanceCreate(dbh, dspid,
					      idr, hdr, oid, xdata, True);

    if (rpc_status != RPCSuccess)
      return rpc_status;

    Oid _oid(oid);

    CollectionBE *collbe =
      new CollectionBE(db, dbh, &_oid, cls,
		       Oid(idx1_oid), Oid(idx2_oid), idx1, idx2,
		       items_cnt, True, inv_oid, inv_item, idximpl,
		       idx_data, idx_data_size, is_literal, is_pure_literal);

    db->getBEQueue()->addCollection(collbe, dbh);

    return RPCSuccess;
  }
  
  static RPCStatus
  IDB_collectionCardWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid)
  {
    CHECK_WRITE((Database *)dbh->db);

    Offset offset = IDB_OBJ_HEAD_SIZE;
    eyedbsm::Status se_status;

    eyedbsm::Oid xoid;

    oid_decode(idr, &offset, &xoid);

    // XDR: should really decode ??
    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_CARD_OID,
				     sizeof(eyedbsm::Oid), &xoid, oid);
    if (se_status)
      return rpcStatusMake_se(se_status);
	
    return RPCSuccess;
  }

  static RPCStatus
  IDB_collectionInvWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid)
  {
    Database *db = (Database *)dbh->db;
    CHECK_WRITE(db);

    eyedbsm::Status se_status;

    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_INV_OID,
				     sizeof(eyedbsm::Oid)+sizeof(eyedblib::int16),
				     idr+IDB_OBJ_HEAD_SIZE, oid);
    if (se_status)
      return rpcStatusMake_se(se_status);
	
    return RPCSuccess;
  }

  static RPCStatus
  IDB_collectionUpdateCount(DbHandle *dbh, CollectionBE *collbe,
			    const eyedbsm::Oid *oid,
			    int is_idx2, int change_cnt,
			    int items_bottom, int items_top)
  {
    Database *db = (Database *)dbh->db;
    CHECK_WRITE(db);
    eyedbsm::Status se_status;

    if (is_idx2) {
      int xitems_top, xitems_bottom;

      se_status = eyedbsm::objectRead(dbh->sedbh, IDB_COLL_OFF_ITEMS_BOT,
				      sizeof(eyedblib::int32), &xitems_bottom, eyedbsm::DefaultLock,
				      0, 0, oid);

      RCHECK(se_status);
      se_status = eyedbsm::objectRead(dbh->sedbh, IDB_COLL_OFF_ITEMS_TOP,
				      sizeof(eyedblib::int32), &xitems_top, eyedbsm::DefaultLock,
				      0, 0, oid);
    
      RCHECK(se_status);
    
      xitems_top = x2h_32(xitems_top);
      xitems_bottom = x2h_32(xitems_bottom);

      items_top    = (items_top > xitems_top) ? items_top : xitems_top;
      items_bottom = (items_bottom < xitems_bottom) ? items_bottom :
	xitems_bottom;
    
      if (items_bottom != xitems_bottom) {
#ifdef E_XDR
	eyedblib::int32 items_bottom_x = h2x_32(items_bottom);
#else
	eyedblib::int32 items_bottom_x = items_bottoms;
#endif
	se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_ITEMS_BOT,
					 sizeof(eyedblib::int32), &items_bottom_x, oid);
      
	RCHECK(se_status);
      }

      if (items_top != xitems_top) {
#ifdef E_XDR
	eyedblib::int32 items_top_x = h2x_32(items_top);
#else
	eyedblib::int32 items_top_x = items_tops;
#endif
	se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_ITEMS_TOP,
					 sizeof(eyedblib::int32), &items_top_x, oid);
	RCHECK(se_status);
      }
    }
  
    if (!change_cnt)
      return RPCSuccess;

    eyedblib::int32 items_cnt, xitems_cnt;
    se_status = eyedbsm::objectRead(dbh->sedbh, IDB_COLL_OFF_ITEMS_CNT,
				    sizeof(eyedblib::int32), &xitems_cnt, eyedbsm::DefaultLock,
				    0, 0, oid);

#ifdef E_XDR
    eyedblib::int32 xitems_cnt_x = x2h_32(xitems_cnt);
#else
    eyedblib::int32 xitems_cnt_x = xitems_cnt;
#endif
    /*
      printf("items_cnt %d vs. xitems_cnt %d, change_cnt %d, new count %d\n",
      items_cnt, xitems_cnt, change_cnt, xitems_cnt + change_cnt);
    */

    items_cnt = xitems_cnt_x + change_cnt;

#ifdef E_XDR
    eyedblib::int32 items_cnt_x = x2h_32(items_cnt);
#else
    eyedblib::int32 items_cnt_x = items_cnt;
#endif
    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_ITEMS_CNT,
				     sizeof(eyedblib::int32), &items_cnt_x, oid);
    RCHECK(se_status);

    IDB_free(db, collbe);  
    return RPCSuccess;
  }

#define UPDATE_COUNT() \
  IDB_collectionUpdateCount(dbh, collbe, oid, \
			    is_idx2, change_cnt, items_bottom, \
			    items_top)

  static RPCStatus
  IDB_collNameManage(DbHandle *dbh, const eyedbsm::Oid *oid,
		     Data idr, short code, Offset &offset)
  {
    if (code & IDB_COLL_NAME_CHANGED) {
      char *name;
      // 2009-11-15: cannot easily change name because should change size of
      // object
      string_decode(idr, &offset, (char **)&name);
      size_t size; // V3_API_size_t
      eyedbsm::Status se_status = eyedbsm::objectSizeGet(dbh->sedbh, &size, eyedbsm::DefaultLock, oid);
      if (se_status) {
	return rpcStatusMake_se(se_status);
      }
      //printf("NAME CHANGED [%s] size=%u, headsize=%u %u\n", name, size, IDB_COLL_SIZE_HEAD, IDB_OBJ_HEAD_SIZE);
    }
    return RPCSuccess;
  }

  static RPCStatus
  IDB_collImplManage(DbHandle *dbh, const eyedbsm::Oid *oid,
		     eyedbsm::Idx *idx1, eyedbsm::Idx *idx2,
		     Data idr, short code, Offset &offset)
  {
    Database *db = (Database *)dbh->db;

    if (code & IDB_COLL_IMPL_CHANGED) {
      IndexImpl *idximpl;
      Offset offset_impl = offset;
      Status status = IndexImpl::decode(db, idr, offset, idximpl);
      if (status)
	return rpcStatusMake(status);
      //printf("NEW IMPLEMENTATION %s\n", (const char *)idximpl->toString());

      eyedbsm::Idx *idxs[2] = {idx1, idx2};
      eyedbsm::Status s;
      eyedbsm::Oid newoid[2] = {eyedbsm::Oid::nullOid, eyedbsm::Oid::nullOid};

      if (idximpl->getType() == IndexImpl::Hash) {
	unsigned int impl_hints_cnt;
	const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);

	for (int i = 0; i < 2; i++) {
	  eyedbsm::HIdx *idx = (idxs[i] ? idxs[i]->asHIdx() : 0);
	  if (idx) {
	    eyedbsm::Idx::KeyType ktype = idx->getKeyType();
	    eyedbsm::Boolean change;

	    //printf("KTYPE BEFORE %d\n", ktype.type);

	    if (ktype.type == eyedbsm::Idx::tUnsignedChar &&
		ktype.count == sizeof(eyedbsm::Oid)) {
	      ktype.type = eyedbsm::Idx::tOid;
	      ktype.count = 1;
	      change = eyedbsm::True;
	    }
	    else
	      change = eyedbsm::False;

	    s = idx->reimplementToHash
	      (newoid[i], idximpl->getKeycount(), 0,
	       idx->getDefaultDspid(), 
	       impl_hints, impl_hints_cnt,
	       (idximpl->getHashMethod() ? hash_key : 0),
	       idximpl->getHashMethod(), (change ? &ktype : 0));
      
	    if (s)
	      return rpcStatusMake_se(s);
	    //printf("KTYPE AFTER %d\n", idx->getKeyType().type);
	  }
	}
      }
      else {
	for (int i = 0; i < 2; i++) {
	  eyedbsm::Idx *idx = idxs[i];
	  if (idx) {
	    s = idx->reimplementToBTree(newoid[i], idximpl->getDegree(),
					idx->getDefaultDspid());
      
	    if (s)
	      return rpcStatusMake_se(s);
	  }
	}
      }

      /*
	printf("NEWOIDS IDX1 %s vs. %s\n", Oid(newoid[0]).toString(),
	Oid(idx1->oid()).toString());
	if (idx2)
	printf("NEWOIDS IDX2 %s vs. %s\n", Oid(newoid[1]).toString(),
	Oid(idx2->oid()).toString());
      */

      eyedbsm::Oid data_oid[2];
      Data data = (Data)&data_oid;
      Size alloc_size = 2 * sizeof(eyedbsm::Oid);
      offset = 0;
      oid_code(&data, &offset, &alloc_size, &newoid[0]);
      oid_code(&data, &offset, &alloc_size, &newoid[1]);
      assert(data == (Data)data_oid);
      RPCStatus rpc_status = 
	IDB_dataWrite(dbh, IDB_COLL_OFF_IDX1_OID, 2 * sizeof(eyedbsm::Oid),
		      data, oid, 0);
      if (rpc_status) return rpc_status;
      return IDB_dataWrite(dbh, IDB_COLL_OFF_IMPL_BEGIN, IDB_IMPL_CODE_SIZE,
			   &idr[offset_impl], oid, 0);
    }

    /*
    if (!(code & IDB_COLL_IMPL_UNCHANGED))
      return rpcStatusMake(IDB_ERROR, "collection write internal error: "
			   "unexpected flags %x", c);
    */

    return RPCSuccess;
  }


  static RPCStatus
  IDB_collectionWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, void *xdata)
  {
    if (hdr->xinfo == IDB_XINFO_CARD)
      return IDB_collectionCardWrite(dbh, idr, hdr, oid);

    if (hdr->xinfo == IDB_XINFO_INV)
      return IDB_collectionInvWrite(dbh, idr, hdr, oid);

    Database *db = (Database *)dbh->db;

    CollectionBE *collbe;
    Status status;
    collbe = IDB_getCollBE("collectionWrite", db, dbh, oid, &status);

    if (!collbe)
      return rpcStatusMake(status);

    collbe->unlock();

    eyedbsm::Idx *idx1, *idx2;
    collbe->getIdx(&idx1, &idx2);

    eyedbsm::Status se_status;

    Offset offset = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int32);
    eyedblib::int32 oid_cnt;
    int32_decode (idr, &offset, &oid_cnt);

    int is_idx2 = (eyedb_is_type(*hdr, _CollList_Type) ||
		   eyedb_is_type(*hdr, _CollArray_Type));

    Data temp = collbe->getTempBuff();
    eyedblib::int16 item_size = collbe->getItemSize();

    int change_cnt = 0;
    int items_bottom = 0xffffff;
    int items_top = 0;

    Oid inv_oid;
    const Attribute *inv_item = NULL;
    eyedbsm::Idx *idx = 0;
    status = collbe->getInvItem(db, inv_item, inv_oid, idx);

    if (status)
      return rpcStatusMake(status);
    
    int i;
    for (i = 0; i < oid_cnt; i++) {
      eyedbsm::Boolean found;
      eyedblib::int32 ind;
      char state;

      buffer_decode(idr, &offset, temp, item_size);

      int32_decode(idr, &offset, &ind);
      char_decode(idr, &offset, &state);
      Oid i_oid;
      if (collbe->getIsRef())
	eyedbsm::x2h_oid(i_oid.getOid(), temp);

      if (is_idx2) {
	unsigned char *temp1 = (unsigned char *)malloc(collbe->getItemSize());
	if (state == CollectionPeer::removed) {
	  se_status = idx1->remove(temp, &ind, &found);
	  if (se_status || !found) {
	    UPDATE_COUNT();
	    free(temp1);
	    if (se_status)
	      return rpcStatusMake_se(se_status);
	  
	    return rpcStatusMake(IDB_COLLECTION_ERROR,
				 "item %s not found at %d in %s",
				 i_oid.toString(), ind,
				 eyedbsm::getOidString(oid));
	  }

	  se_status = idx2->remove(&ind, temp, &found);
	  if (se_status || !found) {
	    UPDATE_COUNT();
	    free(temp1);
	    if (se_status)
	      return rpcStatusMake_se(se_status);
	    return rpcStatusMake(IDB_COLLECTION_ERROR,
				 "item %s not found at %d in %s",
				 i_oid.toString(), ind,
				 eyedbsm::getOidString(oid));
	  }
	
	  if (idx) {
	    //se_status = idx->remove(temp, inv_oid.getOid(), &found);
	    //se_status = idx->remove(temp, inv_temp, &found);
	    se_status = idx->remove(i_oid.getOid(), inv_oid.getOid(), &found);
	    if ((eyedblib::log_mask & IDB_LOG_IDX_SUPPRESS) == IDB_LOG_IDX_SUPPRESS) {
	      IDB_LOG(IDB_LOG_IDX_SUPPRESS,
		      ("Removing Collection index entry '%s'"
		       " %s <-> %s\n", inv_item->getName(),
		       i_oid.toString(), inv_oid.toString()));
	    }
	    if (se_status || !found)
	      return rpcStatusMake_se(se_status);
	  }
	
	  change_cnt--;
	
	  if (ind < items_bottom)
	    items_bottom = ind;
	  // added the 15/08/06 but not correct
	  if (ind+1  == items_top)
	    items_top--;
	}
	else if (state == CollectionPeer::added) {
	  se_status = idx1->insert(temp, &ind);
	  if (se_status) {
	    UPDATE_COUNT();
	    free(temp1);
	    return rpcStatusMake_se(se_status);
	  }

	  se_status = idx2->searchAny(&ind, &found, temp1);
	  if (se_status) {
	    UPDATE_COUNT();
	    free(temp1);
	    return rpcStatusMake_se(se_status);
	  }

	  if (found) {
	    if (se_status = idx1->remove(temp1, &ind)) {
	      UPDATE_COUNT();
	      free(temp1);
	      return rpcStatusMake_se(se_status);
	    }

	    if (se_status = idx2->remove(&ind, temp1)) {
	      UPDATE_COUNT();
	      free(temp1);
	      return rpcStatusMake_se(se_status);
	    }
	    change_cnt--; // to cancel next change_cnt++
	  }

	  se_status = idx2->insert(&ind, temp);
	  if (se_status) {
	    UPDATE_COUNT();
	    free(temp1);
	    return rpcStatusMake_se(se_status);
	  }

	  if (idx) {
	    se_status = idx->insert(i_oid.getOid(), inv_oid.getOid());
	    if ((eyedblib::log_mask & IDB_LOG_IDX_CREATE) ==
		IDB_LOG_IDX_CREATE) {
	      IDB_LOG(IDB_LOG_IDX_CREATE,
		      ("Inserting Collection index entry '%s'"
		       " %s <-> %s\n",
		       inv_item->getName(),
		       i_oid.toString(), inv_oid.toString()));
	    }

	    if (se_status)
	      return rpcStatusMake_se(se_status);
	  }

	  if (ind >= items_top)
	    items_top = ind+1;

	  change_cnt++;
	}
      
	free(temp1);
      }
      else {
	ind = 1;
	if (state == CollectionPeer::removed) {
	  se_status = idx1->remove(temp, &ind, &found);
	  if (se_status || !found) {
	    UPDATE_COUNT();
	    if (se_status)
	      return rpcStatusMake_se(se_status);
	    return rpcStatusMake(IDB_COLLECTION_ERROR,
				 "item %s not found in %s",
				 i_oid.toString(),
				 eyedbsm::getOidString(oid));
	  }

	  if (idx) {
	    se_status = idx->remove(i_oid.getOid(), inv_oid.getOid(), &found);
	    if ((eyedblib::log_mask & IDB_LOG_IDX_SUPPRESS) ==
		IDB_LOG_IDX_SUPPRESS) {
	      IDB_LOG(IDB_LOG_IDX_SUPPRESS,
		      ("Removing Collection index entry '%s'"
		       " %s <-> %s\n",
		       inv_item->getName(),
		       i_oid.toString(), inv_oid.toString()));
	    }
	    if (se_status || !found)
	      return rpcStatusMake_se(se_status);
	  }

	  change_cnt--;
	}
	else if (state == CollectionPeer::added) {
	  se_status = idx1->insert(temp, &ind);
	  if (se_status) {
	    UPDATE_COUNT();
	    return rpcStatusMake_se(se_status);
	  }

	  if (idx) {
	    se_status = idx->insert(i_oid.getOid(), inv_oid.getOid());
	    if ((eyedblib::log_mask & IDB_LOG_IDX_CREATE) == IDB_LOG_IDX_CREATE) {
	      IDB_LOG(IDB_LOG_IDX_CREATE,
		      ("Inserting Collection index entry '%s'"
		       " %s <-> %s\n",
		       inv_item->getName(),
		       i_oid.toString(), inv_oid.toString()));
	    }

	    if (se_status)
	      return rpcStatusMake_se(se_status);
	  }
	  change_cnt++;
	}
	/* else coherent: do nothing!! */
      }
    }

    if (inv_item && inv_item->hasInverse() &&
	hdr->xinfo != IDB_XINFO_INVALID_INV)
      {
	offset = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int32) + sizeof(eyedblib::int32);
	for (i = 0; i < oid_cnt; i++) {
	  eyedblib::int32 ind;
	  char state;

#ifdef E_XDR
	  eyedbsm::Oid toid;
	  oid_decode(idr, &offset, &toid);
	  memcpy(temp, &toid, sizeof(toid));
#else
	  buffer_decode  (idr, &offset, temp, item_size);
#endif
	  int32_decode   (idr, &offset, &ind);
	  char_decode    (idr, &offset, &state);
	
	  Oid _oid;
	  memcpy(&_oid, temp, sizeof(Oid));
	
	  status = inv_item->inverse_coll_perform
	    (db, (state == CollectionPeer::removed ?
		  Attribute::invObjRemove :
		  Attribute::invObjUpdate),
	     inv_oid, _oid);
	
	  if (status) {
	    IDB_free(db, collbe);
	    return rpcStatusMake(status);
	  }
	}
      }

    RPCStatus rpc_status;
    short code;
    int16_decode (idr, &offset, &code);
    if ((rpc_status = IDB_collImplManage(dbh, oid, idx1, idx2, idr, code, offset)) != 0) {
      return rpc_status;
    }

    if ((rpc_status = IDB_collNameManage(dbh, oid, idr, code, offset)) != 0) {
      return rpc_status;
    }

    return UPDATE_COUNT();
  }

  RPCStatus
  IDB_collectionRead(DbHandle *dbh, Data idr, ObjectHeader *hdr,
		     LockMode lockmode,
		     const eyedbsm::Oid *oid, void *xdata)
  {
    return RPCSuccess;
  }

  static RPCStatus
  IDB_collectionInverseDelete(Database *db, CollectionBE *collbe,
			      eyedbsm::Idx *idx, const Attribute *inv_item,
			      const Oid &inv_oid)
  {
    if (!inv_item)
      return RPCSuccess;

    Data temp = collbe->getTempBuff();
    eyedblib::int16 item_size = collbe->getItemSize();

    eyedbsm::IdxCursor *curs;
    //if (collbe->isBIdx())
    if (idx->asBIdx())
      curs = new eyedbsm::BIdxCursor(idx->asBIdx(), 0, 0, eyedbsm::False, eyedbsm::False);
    else
      curs = new eyedbsm::HIdxCursor(idx->asHIdx(), 0, 0, eyedbsm::False, eyedbsm::False);

    for (;;)
      {
	eyedbsm::Boolean sefound;
	memset(temp, 0, item_size);
	eyedbsm::Idx::Key key;
	eyedbsm::Status se_status = curs->next(&sefound, temp, &key);
	if (!sefound)
	  break;

	if (se_status)
	  {
	    IDB_free(db, collbe);  
	    delete curs;
	    return rpcStatusMake_se(se_status);
	  }

	Oid _oid;
	memcpy(&_oid, key.getKey(), sizeof(Oid));
	inv_item->inverse_coll_perform(db, Attribute::invObjRemove,
				       inv_oid, _oid);
      }

    delete curs;
    return RPCSuccess;
  }

  static RPCStatus
  IDB_collectionDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid)
  {
    Database *db = (Database *)dbh->db;

    CollectionBE *collbe;

    Status status;

    collbe = IDB_getCollBE("collectionDelete", db, dbh, oid, &status);

    if (!collbe)
      return rpcStatusMake(status);

    eyedbsm::Idx *idx1, *idx2;
    collbe->getIdx(&idx1, &idx2);

    eyedbsm::Status se_status;

    Oid inv_oid;
    const Attribute *inv_item = NULL;
    eyedbsm::Idx *se_idx = 0;
    if (hdr->xinfo != IDB_XINFO_INVALID_INV)
      {
	status = collbe->getInvItem(db, inv_item, inv_oid, se_idx);

	if (status)
	  return rpcStatusMake(status);
      }

    if (idx1)
      {
	RPCStatus rpc_status =
	  IDB_collectionInverseDelete(db, collbe, idx1, inv_item, inv_oid);

	if (rpc_status)
	  {
	    IDB_free(db, collbe);  
	    return rpc_status;
	  }

	se_status = idx1->destroy();
	if (se_status)
	  {
	    IDB_free(db, collbe);  
	    return rpcStatusMake_se(se_status);
	  }
      }

    if (idx2)
      {
	se_status = idx2->destroy();
	if (se_status)
	  {
	    IDB_free(db, collbe);  
	    return rpcStatusMake_se(se_status);
	  }
      }

    IDB_free(db, collbe);  
    return IDB_instanceDelete(dbh, idr, hdr, oid);
  }

  RPCStatus
  IDB_collectionGetByInd(DbHandle *dbh, const eyedbsm::Oid *colloid, int ind, int *found, Data idr, void *xdata)
  {
    /*
      1) get coll from the cache:
      if is not put it (coll = new CollectionBE(db, colloid) -> bequeue)
      2) coll->getIdx1()->searchAny(ind) -> found?
    */
    Database *db = (Database *)dbh->db;
    CollectionBE *collbe;
    Status status;

    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (data) {
      data->status = rpc_BuffUsed;
      data->size = 0;
    }

    if (!(collbe = IDB_getCollBE("collectionGetByInd", db, dbh, colloid,
				 &status)))
      return rpcStatusMake(status);

    int item_size = collbe->getItemSize();
  
    if (data) {
      if (item_size <= data->buff_size)
	data->status = rpc_BuffUsed;
      else {
	data->status = rpc_TempDataUsed;
	data->data = malloc(item_size);
      }

      data->size = item_size;
      idr = (Data)data->data;
    }

    *found = 0;

    eyedbsm::Idx *idx2;
    collbe->getIdx(0, &idx2);
    eyedbsm::Boolean sefound;

    eyedbsm::Status se_status;

    if (!(se_status = idx2->searchAny(&ind, &sefound, idr))) {
      if (sefound) {
	*found = 1;
      }
    }
    else {
      IDB_free(db, collbe);  
      return rpcStatusMake_se(se_status);
    }

    IDB_free(db, collbe);  
    return RPCSuccess;
  }

  RPCStatus
  IDB_collectionGetByValue(DbHandle *dbh, const eyedbsm::Oid *colloid,
			   Data val, int *found, int *ind)
  {
    /*
      1) get coll from the cache (see above)
      2) coll->getIdx2()->searchAny(oid) -> found?
    */

    Database *db = (Database *)dbh->db;
    *found = 0;

    CollectionBE *collbe;

    Status status;

    if (!(collbe = IDB_getCollBE("collectionGetByValue", db, dbh, colloid, &status)))
      return rpcStatusMake(status);

    eyedbsm::Idx *idx1;
    collbe->getIdx(&idx1, 0);
    eyedbsm::Boolean sefound;

    // 30/08/05 : code suppressed because HIdx::searchAny() performs the swap
    // seems that other code does not perform swap before
    // 09/09/05: reconnected

    // 9/9/05 (later): new doit pas etre fait la, mais dans le client (si E_XDR)
    // disconnected !
#if 0 /*defined(E_XDR) && defined(NEW_COLL_XDR)*/
    eyedbsm::Oid toid;
    /*
      Offset offset = 0;
      printf("GET_BY_VALUE\n");
      oid_decode(val, &offset, &toid);
      val = (Data)&toid;
    */
    memcpy(&toid, val, sizeof(toid));
    Offset offset_0 = 0;
    Size alloc_size_0 = sizeof(toid);
    oid_code(&val, &offset_0, &alloc_size_0, &toid);
#endif

    eyedbsm::Status se_status;
    if (!(se_status = idx1->searchAny(val, &sefound, ind))) {
      if (sefound)
	*found = 1;
    }
    else {
      IDB_free(db, collbe);  
      return rpcStatusMake_se(se_status);
    }

    IDB_free(db, collbe);  
    return RPCSuccess;
  }

  RPCStatus
  IDB_setObjectLock(DbHandle *dbh, const eyedbsm::Oid * oid, int lockmode, int * rlockmode)
  {
    eyedbsm::LockMode _rlockmode;
    eyedbsm::Status se = eyedbsm::objectLock(dbh->sedbh, oid, (eyedbsm::LockMode)lockmode, &_rlockmode);
    if (se) return rpcStatusMake_se(se);

    if (rlockmode)
      *rlockmode = _rlockmode;

    return RPCSuccess;
  }

  RPCStatus
  IDB_getObjectLock(DbHandle *dbh, const eyedbsm::Oid * oid, int * lockmode)
  {
    eyedbsm::LockMode _rlockmode;
    eyedbsm::Status se = eyedbsm::objectGetLock(dbh->sedbh, oid, &_rlockmode);
    if (se) return rpcStatusMake_se(se);

    *lockmode = _rlockmode;

    return RPCSuccess;
  }

  /* queries */

  /*
    static void
    IDB_schCode(IteratorBE *qbe, void *pschinfo, void *sch_data)
    {
    rpc_ServerData *data = (rpc_ServerData *)sch_data;

    if (data)
    {
    data->status = rpc_TempDataUsed;
    code_sch_info(qbe->getSchemaInfo(),
    (Data *)&data->data, &data->size);
    }
    else
    *((SchemaInfo **)pschinfo) = qbe->getSchemaInfo();
    }

    RPCStatus
    IDB_queryLangCreate(DbHandle *dbh, const char *qstr, int *qid,
    int *pcount, void *pschinfo, void *qstr_data,
    void *sch_data)
    {
    Database *db = (Database *)dbh->db;
    Status status;

    lock_data((Data *)&qstr, qstr_data);
    IteratorBE *qbe = new IteratorBE(db, dbh, qstr, *pcount);

    if ((status = qbe->getStatus()) == Success)
    {
    *qid = qbe->getQid();

    IDB_schCode(qbe, pschinfo, sch_data);
    unlock_data((Data)qstr, qstr_data);

    return RPCSuccess;
    }
  
    *qid = 0;

    IDB_schCode(qbe, pschinfo, sch_data);

    db->getBEQueue()->removeIterator(qbe);
    delete qbe;

    unlock_data((Data)qstr, qstr_data);
    return rpcStatusMake(status);
    }

    RPCStatus
    IDB_queryDatabaseCreate(DbHandle *dbh, int *qid)
    {
    return RPCSuccess;
    }

    RPCStatus
    IDB_queryClassCreate(DbHandle *dbh, const eyedbsm::Oid *cloid, int *qid)
    {
    return RPCSuccess;
    }
  */

  RPCStatus
  IDB_queryCollectionCreate(DbHandle *dbh, const eyedbsm::Oid *colloid, int index,
			    int *qid)
  {
    Oid xcolloid(colloid);

    if (!xcolloid.isValid())
      return rpcStatusMake(Exception::make(IDB_ERROR, "invalid null oid for collection query"));

    Database *db = (Database *)dbh->db;
    CollectionBE *collbe;

    Status status;

    if (!(collbe = IDB_getCollBE("queryCollectionCreate", db, dbh, colloid, &status)))
      return rpcStatusMake(status);

    IteratorBE *qbe = new IteratorBE(collbe, (index ? True : False));

    if ((status = qbe->getStatus()) == Success)
      {
	*qid = qbe->getQid();
	return RPCSuccess;
      }
  
    IDB_free(db, collbe);  
    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_queryAttributeCreate(DbHandle *dbh, const eyedbsm::Oid *cloid,
			   int num, int ind, Data start,
			   Data end, int sexcl, int eexcl, int x_size,
			   int *qid)
  {
    Database *db = (Database *)dbh->db;
    Class *cl = db->getSchema()->getClass(*cloid);
    const Attribute *attr = (Attribute *)((AgregatClass *)cl)->getAttribute(num);

    if (attr)
      {
	IteratorBE *qbe = new IteratorBE((Database *)dbh->db,
					 attr, ind, start, end,
					 (Bool)sexcl, (Bool)eexcl,
					 x_size);
	Status status;
	if ((status = qbe->getStatus()) == Success)
	  {
	    *qid = qbe->getQid();
	    return RPCSuccess;
	  }

	return rpcStatusMake(status);
      }

    return rpcStatusMake(IDB_ERROR, "invalid attribute");
  }

  RPCStatus
  IDB_queryDelete(DbHandle *dbh, int qid)
  {
    Database *db = (Database *)dbh->db;
    IteratorBE *qbe;
    if (qbe = db->getBEQueue()->getIterator(qid))
      {
	delete qbe;
	db->getBEQueue()->removeIterator(qid);
	return RPCSuccess;
      }

    return rpcStatusMake(IDB_ERROR, "invalid query");
  }

  int
  IDB_getSeTrsCount(DbHandle *dbh)
  {
    return eyedbsm::getTransactionCount(dbh->sedbh);
  }

  RPCStatus
  IDB_queryScanNext(DbHandle *dbh, int qid, int wanted, int *found,
		    void *atom_array, void *xdata)
  {
    Database *db = (Database *)dbh->db;
    IteratorBE *qbe;
    IteratorAtom *temp_array;
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (data)
      {
#ifdef IDB_VECT
	temp_array = idbNewVect(IteratorAtom, wanted);
#else
	temp_array = new IteratorAtom[wanted];
#endif
      }
    else
      temp_array = (IteratorAtom *)atom_array;

    if (!qid)
      {
	*found = 0;
	if (data)
	  code_atom_array(data, temp_array, *found, wanted);
	return RPCSuccess;
      }

    if (qbe = db->getBEQueue()->getIterator(qid))
      {
	Status status;

	if ((status = qbe->scanNext(wanted, found, temp_array)) == Success)
	  {
	    if (data)
	      code_atom_array(data, temp_array, *found, wanted);
	    return RPCSuccess;
	  }

	return rpcStatusMake(status);
      }

    *found = 0;
    return rpcStatusMake(IDB_ERROR, "invalid query");
  }

  /* OQL */

  static void
  IDB_schCode(OQLBE *oqlbe, void *pschinfo, void *sch_data)
  {
    rpc_ServerData *data = (rpc_ServerData *)sch_data;

    if (data)
      {
	data->status = rpc_TempDataUsed;
	code_sch_info(oqlbe->getSchemaInfo(),
		      (Data *)&data->data, &data->size);
      }
    else
      *((SchemaInfo **)pschinfo) = oqlbe->getSchemaInfo();
  }

  RPCStatus
  IDB_oqlCreate(DbHandle *dbh, const char *qstr, int *qid,
		void *pschinfo, void *qstr_data,
		void *sch_data)
  {
    Database *db = (Database *)dbh->db;
    Status status;

    lock_data((Data *)&qstr, qstr_data);
    OQLBE *oqlbe = new OQLBE(db, dbh, qstr);

    if ((status = oqlbe->getStatus()) == Success)
      {
	*qid = oqlbe->getQid();

	IDB_schCode(oqlbe, pschinfo, sch_data);
	unlock_data((Data)qstr, qstr_data);

	return RPCSuccess;
      }
  
    *qid = 0;

    IDB_schCode(oqlbe, pschinfo, sch_data);

    db->getBEQueue()->removeOQL(oqlbe);
    delete oqlbe;

    unlock_data((Data)qstr, qstr_data);
    return rpcStatusMake(status);
  }

  RPCStatus
  IDB_oqlDelete(DbHandle *dbh, int qid)
  {
    Database *db = (Database *)dbh->db;
    OQLBE *oqlbe;
    if (oqlbe = db->getBEQueue()->getOQL(qid))
      {
	delete oqlbe;
	db->getBEQueue()->removeOQL(qid);
	return RPCSuccess;
      }

    return rpcStatusMake(IDB_ERROR, "invalid query");
  }

  RPCStatus
  IDB_oqlGetResult(DbHandle *dbh, int qid, void *value, void *xdata)
  {
    Database *db = (Database *)dbh->db;
    OQLBE *oqlbe;
    Value *temp_value;
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (data)
      temp_value = new Value();
    else
      temp_value = (Value *)value;

    if (oqlbe = db->getBEQueue()->getOQL(qid))
      {
	Status status;

	if ((status = oqlbe->getResult(temp_value)) == Success)
	  {
	    if (data)
	      code_value(data, temp_value);
	    return RPCSuccess;
	  }

	if (data) delete temp_value;
	return rpcStatusMake(status);
      }

    if (data) delete temp_value;
    return rpcStatusMake(IDB_ERROR, "invalid query");
  }

  /* schema functions */

  static const char schema_key[] = ".idb.schema";

  static RPCStatus
  IDB_schemaClassHeadGet(DbHandle *dbh)
  {
    eyedbsm::DbHandle *sedbh = dbh->sedbh;
    eyedbsm::Status status;
    RPCStatus rpc_status;
    SchemaHead *sch = &dbh->sch;

    //  printf("IDB_schemaClassHeadGet()\n");
    if ((rpc_status = IDB_transactionBegin(dbh, 0, True)) ==
	RPCSuccess)
      {
	eyedbsm::Oid toid;
	status = eyedbsm::rootEntryGet(sedbh, schema_key,
				       &sch->oid, sizeof(eyedbsm::Oid));
	// 9/9/05: must swap oid read
#ifdef E_XDR
	eyedbsm::x2h_oid(&sch->oid, &sch->oid);
#endif
	if (status == eyedbsm::Success) {
	  status = eyedbsm::objectRead(sedbh, IDB_SCH_CNT_INDEX, IDB_SCH_CNT_SIZE,
				       &sch->cnt, eyedbsm::DefaultLock, 0, 0, &sch->oid);
#ifdef E_XDR
	  sch->cnt = x2h_32(sch->cnt);
#endif
	  sch->modify = False;
	  rpc_status = rpcStatusMake_se(status);
	}
	else
	  rpc_status = rpcStatusMake(IDB_INVALID_SCHEMA, "no schema associated with database");

	IDB_transactionCommit(dbh, True);

	return rpc_status;
      }

    return rpc_status;
  }

  static RPCStatus
  IDB_schemaClassCreate(DbHandle *dbh)
  {
    CHECK_WRITE((Database *)dbh->db);
    eyedbsm::DbHandle *sedbh = dbh->sedbh;
    eyedbsm::Status status;
    char temp[IDB_OBJ_HEAD_SIZE + IDB_SCH_CNT_SIZE + IDB_SCH_NAME_SIZE];
    Data data = (Data)temp;
    Size alloc = sizeof temp;
    ObjectHeader hdr;
    SchemaHead *sch = &dbh->sch;
    Offset offset;

    memset(temp, 0, sizeof(temp));
    status = eyedbsm::rootEntryGet(sedbh, schema_key,
				   &sch->oid, sizeof(eyedbsm::Oid));

#ifdef E_XDR
    eyedbsm::x2h_oid(&sch->oid, &sch->oid);
#endif

    if (status == eyedbsm::Success)
      return rpcStatusMake(IDB_SCHEMA_ALREADY_CREATED, 
			   "schema already created");

    // must swap sch->oid
    eyedb_clear(hdr);
    hdr.magic = IDB_OBJ_HEAD_MAGIC;
    hdr.type = _Schema_Type;
    hdr.size = sizeof temp;
    memset(&hdr.oid_cl, 0, sizeof(eyedbsm::Oid));

    offset = 0;
    object_header_code(&data, &offset, &alloc, &hdr);
    sch->cnt = 0;
    int32_code(&data, &offset, &alloc, &sch->cnt);
    string_code(&data, &offset, &alloc, "");

    assert(offset <= sizeof(temp));

    status = eyedbsm::objectCreate(sedbh, temp,
				   sizeof(temp), Dataspace::DefaultDspid, &sch->oid);

    if (status)
      return rpcStatusMake_se(status);

    // must swap before sch->oid
#ifdef E_XDR
    eyedbsm::Oid toid;
    eyedbsm::h2x_oid(&toid, &sch->oid);
#else
    toid = sch->oid;
#endif

    status = eyedbsm::rootEntrySet(sedbh, schema_key, &toid,
				   sizeof(eyedbsm::Oid), eyedbsm::True);

    if (status)
      return rpcStatusMake_se(status);

    sch->modify = False;

    return RPCSuccess;
  }

  /* classes functions */
  static RPCStatus
  IDB_makeColl(DbHandle *dbh, Data idr, const eyedbsm::Oid *oid,
	       const char *name, const CollImpl *collimpl,
	       Offset offset, Bool lock)
  {
    CHECK_WRITE((Database *)dbh->db);

    CollSet *coll = CollectionPeer::collSet(name, collimpl);
    Status status;
    eyedbsm::Oid colloid;

    if ((status = coll->getStatus()) != Success)
      {
	coll->release();
	return rpcStatusMake(status);
      }

    if (lock)
      CollectionPeer::setLock(coll, True);

    status = coll->setDatabase((Database *)dbh->db);
    if (status)
      return rpcStatusMake(status);

    //collectionLocked = True;
    status = coll->realize();
    //collectionLocked = False;

    /*
      printf("has created %s collection -> %s class=[%d, %s]\n",
      name, coll->getOid().getString(),
      coll->getClass(), coll->getClass()->getName());
    */

    if (status != Success)
      return rpcStatusMake(status);

    colloid = *coll->getOid().getOid();

    eyedbsm::Status se_status;

#ifndef E_XDR
    se_status = eyedbsm::objectWrite(dbh->sedbh, offset, sizeof(eyedbsm::Oid),
				     &colloid, oid);
    // XDR: should be h2x ??
    memcpy(idr + offset, &colloid, sizeof(eyedbsm::Oid));
#else
    eyedbsm::Oid xoid;
    eyedbsm::h2x_oid(&xoid, &colloid);
    se_status = eyedbsm::objectWrite(dbh->sedbh, offset, sizeof(eyedbsm::Oid),
				     &xoid, oid);
    // XDR: we h2x !
    memcpy(idr + offset, &xoid, sizeof(eyedbsm::Oid));
    // DONT!
    //memcpy(idr + offset, &colloid, sizeof(eyedbsm::Oid));
#endif

    coll->release();
    return rpcStatusMake_se(se_status);
  }

  static RPCStatus
  IDB_collClassCreate(DbHandle *dbh, Data idr, const eyedbsm::Oid *oid,
		      const char *name, const CollImpl *collimpl)
  {
    RPCStatus rpc_status;
    char tok[256];

    Offset offset = IDB_CLASS_EXTENT;
    Oid extent_oid;
    oid_decode(idr, &offset, extent_oid.getOid());
    if (!extent_oid.isValid())
      {
	sprintf(tok, "%s::extent", name);
	rpc_status = IDB_makeColl(dbh, idr, oid, tok, collimpl,
				  IDB_CLASS_EXTENT, True);
	if (rpc_status)
	  return rpc_status;
      }

    offset = IDB_CLASS_COMPONENTS;
    Oid comp_oid;
    oid_decode(idr, &offset, comp_oid.getOid());
    if (!comp_oid.isValid())
      {
	sprintf(tok, "%s::component", name);
	return IDB_makeColl(dbh, idr, oid, tok, 0, IDB_CLASS_COMPONENTS,
			    False);
      }

    return RPCSuccess;
  }

  RPCStatus
  IDB_collClassRegister(DbHandle *dbh, const eyedbsm::Oid *colloid,
			const eyedbsm::Oid *oid, Bool insert)
  {
    Database *db = (Database *)dbh->db;
    CHECK_WRITE(db);
    CollectionBE *collbe;
	  
    Status status;
    if (!(collbe = IDB_getCollBE("collClassRegister", db, dbh, colloid, &status, True)))
      return rpcStatusMake(status);
  
    eyedbsm::Idx *idx1;
    collbe->getIdx(&idx1, 0);
  
    eyedblib::int32 ind = 1;
    eyedbsm::Status se_status;

    unsigned char data[sizeof(eyedbsm::Oid)];
    oid_code(data, (Data)oid);

    if (insert)
      se_status = idx1->insert(data, &ind);
    else {
      eyedblib::int32 ind = 1;
      eyedbsm::Boolean found = eyedbsm::False;
      se_status = idx1->remove(data, &ind, &found);
      if (!found)
	return rpcStatusMake(IDB_ERROR,
			     "instance delete: oid %s not found "
			     "in collection",
			     getOidString(oid));
    }

    if (se_status)
      return rpcStatusMake_se(se_status);


    int items_cnt;
    se_status = eyedbsm::objectRead(dbh->sedbh, IDB_COLL_OFF_ITEMS_CNT,
				    sizeof(eyedblib::int32), &items_cnt, eyedbsm::DefaultLock, 0, 0, colloid);
    if (se_status)
      return rpcStatusMake_se(se_status);

    items_cnt = x2h_32(items_cnt);
    items_cnt += (insert ? 1 : -1);

    collbe->setItemsCount(items_cnt);
  
#ifdef E_XDR
    eyedblib::int32 items_cnt_x = h2x_32(items_cnt);
#else
    eyedblib::int32 items_cnt_x = items_cnt;
#endif

    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_COLL_OFF_ITEMS_CNT,
				     sizeof(eyedblib::int32), &items_cnt_x, colloid);
    if (se_status)
      return rpcStatusMake_se(se_status);

    return RPCSuccess;
  }

  static RPCStatus
  IDB_collClassUpdate(DbHandle *dbh, Data idr, const eyedbsm::Oid *oid,
		      const char *name, Bool insert)
  {
    Database *db = (Database *)dbh->db;
    eyedbsm::Oid colloid;

    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Schema *sch = db->getSchema();
    Class *cl = sch->getClass(oid_cl);

    if (!cl) {
      sch->deferredCollRegister(name ? name : "class", oid);
      return RPCSuccess;
    }

    Collection *extent;
    Status status = cl->getExtent(extent);
    if (status)
      return rpcStatusMake(status);

    if (!extent) {
      // tries again
      status = cl->wholeComplete();
      if (status)
	return rpcStatusMake(status);
      status = cl->getExtent(extent);
      if (status)
	return rpcStatusMake(status);

      if (!extent) {
	// no chance 
	sch->deferredCollRegister(cl->getName(), oid);
	return RPCSuccess;
      }
    }
  
    colloid = *extent->getOid().getOid();

    return IDB_collClassRegister(dbh, &colloid, oid, insert);
  }

  static const int SCH_INC = 128;

  static void
  DUMP_SCH_OID(DbHandle *dbh, const eyedbsm::Oid *oid, int cnt)
  {
    for (int i = 0; i < cnt; i++)
      {
	eyedbsm::Oid zoid;
	eyedbsm::objectRead(dbh->sedbh, IDB_SCH_OID_INDEX(i), sizeof(Oid), &zoid,
			    eyedbsm::DefaultLock, 0, 0, oid);
	printf("%s ", Oid(zoid).getString());
      }
    printf("\n");
  }

  static RPCStatus
  be_class_name_code(DbHandle *dbh, Data *idr, Offset *offset,
		     Size *alloc_size, const char *name)
  {
    int len = strlen(name);
    if (len >= IDB_CLASS_NAME_LEN)
      {
	eyedbsm::Oid data_oid;
	RPCStatus rpc_status = IDB_dataCreate(dbh, Dataspace::DefaultDspid,
					      len+1, (Data)name,
					      &data_oid, 0);
	if (rpc_status) return rpc_status;
	char c = IDB_NAME_OUT_PLACE;
	char_code(idr, offset, alloc_size, &c);
	oid_code (idr, offset, alloc_size, &data_oid);
	bound_string_code (idr, offset, alloc_size,
			   IDB_CLASS_NAME_PAD, 0);
	return RPCSuccess;
      }

    char c = IDB_NAME_IN_PLACE;
    char_code(idr, offset, alloc_size, &c);
    bound_string_code(idr, offset, alloc_size, IDB_CLASS_NAME_LEN,
		      name);
    return RPCSuccess;
  }


  static RPCStatus
  be_class_name_decode(DbHandle *dbh, Data idr, Offset *offset,
		       char **name)
  {
    char c;
    char_decode(idr, offset, &c);

    if (c == IDB_NAME_OUT_PLACE)
      {
	eyedbsm::Oid data_oid;
	RPCStatus rpc_status;

	oid_decode (idr, offset, &data_oid);
	unsigned int size;
	rpc_status = IDB_dataSizeGet(dbh, &data_oid, &size);
	if (rpc_status) return rpc_status;
	*name = (char *)malloc(size);
	rpc_status = IDB_dataRead(dbh, 0, 0, (Data)*name, 0, &data_oid, 0);
	if (rpc_status) return rpc_status;
	bound_string_decode (idr, offset, IDB_CLASS_NAME_PAD, 0);
	return RPCSuccess;
      }

    char *s;
    bound_string_decode(idr, offset, IDB_CLASS_NAME_LEN, &s);
    *name = strdup(s);
    return RPCSuccess;
  }

  static RPCStatus
  IDB_classCreate(DbHandle *dbh, short dspid, Data idr, ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata)
  {
    CHECK_WRITE((Database *)dbh->db);
    SchemaHead *sch = &dbh->sch;
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    if (isOidValid(&sch->oid)) {
      eyedbsm::DbHandle *sedbh = dbh->sedbh;
      eyedbsm::Status se_status;
      RPCStatus rpc_status;
      eyedblib::int32 cnt;

      Bool skipIfFound = IDBBOOL(hdr->xinfo == IDB_XINFO_CLASS_UPDATE);
      hdr->xinfo = 0;

      rpc_status = IDB_instanceCreate(dbh, dspid, idr, hdr, oid, data, False);
      if (rpc_status) return rpc_status;

      Database *db = (Database *)dbh->db;
      Offset offset = IDB_CLASS_HEAD_SIZE;
      char *name;
      rpc_status = be_class_name_decode(dbh, idr, &offset, &name);
      if (rpc_status) return rpc_status;
    
      //printf("ClassCreate(%s, %s)\n", name, Oid(oid).toString());
      offset = IDB_CLASS_IMPL_TYPE;
      IndexImpl *idximpl = 0;
      int impl_type;
      Status status = IndexImpl::decode(db, idr, offset, idximpl, &impl_type);
      if (status) return rpcStatusMake(status);
      /*
	printf("decoding idximpl for class %s: %s\n", name,
	(const char *)idximpl->toString());
      */
      /*
	offset = IDB_CLASS_MAG_ORDER;
	eyedblib::int32 mag_order;
	int32_decode(idr, &offset, &mag_order);
      */
    
      CollImpl *collimpl = new CollImpl((CollImpl::Type)impl_type, idximpl);
      rpc_status = IDB_collClassCreate(dbh, idr, oid, name, collimpl);
    
      collimpl->release();

      if (rpc_status != RPCSuccess)
	return rpc_status;

      if ((sch->cnt % SCH_INC) == 0) {
	rpc_status = 
	  IDB_objectSizeModify(dbh,
			       IDB_SCH_OID_SIZE(sch->cnt + SCH_INC),
			       &sch->oid);
	if (rpc_status != RPCSuccess)
	  return rpc_status;
      }
    
      cnt = sch->cnt + 1;
#ifdef E_XDR
      int cnt_x = h2x_32(cnt);
#else
      int cnt_x = cnt;
#endif
      se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_CNT_INDEX, IDB_SCH_CNT_SIZE,
				       (void *)&cnt_x, &sch->oid);

      if (se_status)
	return rpcStatusMake_se(se_status);
      
      eyedbsm::Oid oid_x;
#ifdef E_XDR
      eyedbsm::h2x_oid(&oid_x, oid);
#else
      oid_x = *oid;
#endif
      se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_OID_INDEX(sch->cnt),
				       sizeof(eyedbsm::Oid),
				       (void *)&oid_x, &sch->oid);
      
      if (se_status)
	return rpcStatusMake_se(se_status);
      
#ifdef E_XDR
      eyedblib::int32 hdr_type = h2x_32(hdr->type);
#else
      eyedblib::int32 hdr_type = hdr->type;
#endif
      se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_TYPE_INDEX(sch->cnt),
				       sizeof(eyedblib::int32),
				       (void *)&hdr_type, &sch->oid);
      
      if (se_status)
	return rpcStatusMake_se(se_status);
      
      Data tmpdata = 0;
      offset = 0;
      Size alloc_size = 0;
      rpc_status = be_class_name_code(dbh, &tmpdata, &offset, &alloc_size,
				      name);
      if (rpc_status) return rpc_status;
      
      se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_CLSNAME_INDEX(sch->cnt),
				       IDB_CLASS_NAME_TOTAL_LEN,
				       (void *)tmpdata, &sch->oid);
      
      free(tmpdata);

      if (se_status)
	return rpcStatusMake_se(se_status);
      
      sch->cnt++;
      sch->modify = True;

      Object *o;
      Oid _oid(oid);
      status = db->makeObject(&_oid, hdr, idr, &o);

      free(name); name = 0;
      if (status) return rpcStatusMake(status);

      assert(o->getOid() == Oid(_oid));
      if (!(db->getOpenFlag() & _DBOpenLocal) ||
	  !strcmp(db->getName(), DBM_Database::getDbName())) {
	if (status = db->getSchema()->addClass(o->asClass())) {
	  /*
	    status->print(stdout);
	    const Class *x = db->getSchema()->getClass(o->asClass()->getName());
	    printf("adding class error %s [%s %s]\n", o->asClass()->getName(),
	    o->getOid().toString(), x->getOid().toString());
	  */
	}
      }
	
      rpc_status = IDB_attrCompPropagate(db, o->asClass(), skipIfFound);
      if (rpc_status) return rpc_status;

      return IDB_collClassUpdate(dbh, idr, oid, "class", True);
    }

    return RPCSuccess;
  }

  static RPCStatus
  IDB_collClassCheck(DbHandle *dbh, Offset offset, Data idr,
		     const eyedbsm::Oid *oid)
  {
    eyedbsm::Oid colloid;
    Offset xoffset = offset;

    oid_decode(idr, &xoffset, &colloid);

    Oid xoid(colloid);

    if (!xoid.isValid())
      {
	eyedbsm::Status status =
	  eyedbsm::objectRead(dbh->sedbh, offset, sizeof(eyedbsm::Oid), &colloid,
			      eyedbsm::DefaultLock, 0, 0, oid);

	if (status)
	  return rpcStatusMake_se(status);

	Size alloc_size = offset + sizeof(eyedbsm::Oid);
#ifdef E_XDR
	buffer_code(&idr, &offset, &alloc_size, (unsigned char *)&colloid,
		    sizeof(colloid));
#else
	oid_code(&idr, &offset, &alloc_size, &colloid);
#endif
      }

    return RPCSuccess;
  }

  static void
  trace_oids(Class *cls)
  {
    printf("TRACE %s cls=%p\n", cls->getName(), cls);
    unsigned int attr_cnt;
    const Attribute **attrs = cls->getAttributes(attr_cnt);
    for (int i = 0; i < attr_cnt; i++)
      printf("%s -> %s\n", attrs[i]->getName(), attrs[i]->getAttrCompSetOid().toString());
  }

  static void
  IDB_reportAttrCompSetOids(Class *cls, Data idr)
  {
    unsigned int attr_cnt;
    const Attribute **attrs = cls->getAttributes(attr_cnt);
    Offset offset = IDB_CLASS_ATTR_START;
    for (int i = 0; i < attr_cnt; i++)
      attrs[i]->reportAttrCompSetOid(&offset, idr);
  }

  static RPCStatus
  IDB_classWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, void *xdata)
  {
    Database *db = (Database *)dbh->db;
    Class *cl = db->getSchema()->getClass(*oid);
    Object *o;
    Oid _oid(oid);
    Status status;

    //printf("ClassWrite(%s, %s, cl=%p)\n", cl->getName(),
    //cl->getOid().toString(), cl);
    //trace_oids(cl);
    IDB_reportAttrCompSetOids(cl, idr);
    if ((status = db->getSchema()->suppressClass(cl)) != Success)
      return rpcStatusMake(status);

    status = db->makeObject(&_oid, hdr, idr, &o);

    //printf("writing class %s [%p vs. %p]\n", o->asClass()->getName(), o, cl);

    dbh->sch.modify = True;

    if (status == Success)
      {
	if (!(db->getOpenFlag() & _DBOpenLocal) ||
	    !strcmp(db->getName(), DBM_Database::getDbName()))
	  {
	    //printf("ClassWrite adding class %p\n", o);
	    //trace_oids((Class *)o);
	    if ((status = db->getSchema()->addClass((Class *)o))
		!= Success)
	      {
		return rpcStatusMake(status);
	      
	      }
	  }
      }

#if 0
    status = db->getSchema()->checkDuplicates();
    if (status)
      return rpcStatusMake(status);
#endif
    status = db->getSchema()->clean(db);
    if (status)
      return rpcStatusMake(status);

    RPCStatus rpc_status;

    if (rpc_status = IDB_collClassCheck(dbh, IDB_CLASS_EXTENT, idr, oid))
      return rpc_status;

    if (rpc_status = IDB_collClassCheck(dbh, IDB_CLASS_COMPONENTS, idr, oid))
      return rpc_status;

    return IDB_instanceWrite(dbh, idr, hdr, oid, xdata);
  }

  static void
  IDB_suppressClassFromSchema(Database *db, const Oid &cloid)
  {
    Schema *m = db->getSchema();
    const LinkedList *list = m->getClassList();
    Class **clx = new Class *[list->getCount()];
    int clx_cnt = 0;
    LinkedListCursor c(list);
  
    Class *cls;
    while (c.getNext((void *&)cls))
      if (cls->getOid().compare(cloid))
	clx[clx_cnt++] = cls;

    for (int i = 0; i < clx_cnt; i++)
      m->suppressClass(clx[i]);

    delete [] clx;
  }

  struct ClassHead {
    eyedbsm::Oid oid;
    eyedblib::int32 type;
    char clsname[IDB_CLASS_NAME_TOTAL_LEN];

    void x2h() {
      eyedbsm::x2h_oid(&oid, &oid);
      type = x2h_32(type);
    }

    void h2x() {
      eyedbsm::h2x_oid(&oid, &oid);
      type = h2x_32(type);
    }
  };

  static RPCStatus
  IDB_removeInstances(DbHandle *dbh, Database *db, Class *cl)
  {
    Collection *extent;
    Status s = cl->getExtent(extent, True);
    if (s) return rpcStatusMake(s);

    if (!extent)
      return rpcStatusMake(Exception::make(IDB_ERROR,
					   "cannot find extent for "
					   "class %s %s",
					   cl->getOid().toString(),
					   cl->getName()));
    IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("removing the %d instance(s) of class %s)\n",
				    extent->getCount(), cl->getName()));
    // 19/06/01: instance removing does not work
    // suppressed for now!
    // 4/04/03: reconnected
    OidArray oid_arr;
    s = extent->getElements(oid_arr);
    if (s) return rpcStatusMake(s);

    int cnt = oid_arr.getCount();
    //printf("removing %d instances of class %s\n", cnt, cl->getName());
    for (int i = 0; i < cnt; i++) {
      Data inv_data;
      RPCStatus rpc_status = IDB_objectDelete(dbh, oid_arr[i].getOid(),
					      0, &inv_data, 0);
      if (rpc_status) return rpc_status;
    }
    
    s = extent->remove();
    if (s) return rpcStatusMake(s);

    return RPCSuccess;
  }

  static RPCStatus
  IDB_removeAttrCompSet(Database *db, Class *cl)
  {
    unsigned int attr_cnt;
    const Attribute **attrs = cl->getAttributes(attr_cnt);
    for (int n = 0; n < attr_cnt; n++) {
      if (!cl->compare(attrs[n]->getClassOwner()))
	continue;

      Oid attr_comp_oid = attrs[n]->getAttrCompSetOid();
      if (attr_comp_oid.isValid()) {
	Bool removed;
	Status s = db->isRemoved(attr_comp_oid, removed);
	if (s)
	  return rpcStatusMake(s);
	if (!removed) {
	  s = db->removeObject(attr_comp_oid);
	  printf("removing comp set %s %s\n",
		 attr_comp_oid.getString(), cl->getName());
	  if (s) return rpcStatusMake(s);
	}
      }
    }

    return RPCSuccess;
  }

  static RPCStatus
  IDB_classDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, unsigned int flags)
  {
    Database *db = (Database *)dbh->db;
    CHECK_WRITE(db);
    Class *cl = db->getSchema()->getClass(*oid);
    SchemaHead *sch = &dbh->sch;

    IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("deleting class(%s, %p, %s, flags=%x)\n", Oid(oid).toString(), cl, cl->getName(), flags));
    if (!cl)
      return rpcStatusMake(IDB_ERROR, "class %s not found",
			   Oid(*oid).toString());

    if (!isOidValid(&sch->oid))
      return rpcStatusMake(IDB_ERROR, "schema oid is null");

    RPCStatus rpc_status;

    if (flags == Class::RemoveInstances) {
      rpc_status = IDB_removeInstances(dbh, db, cl);
      if (rpc_status) return rpc_status;

      // remove components collection
      Collection *components;
      Status s = cl->getComponents(components, True);
      if (s)
	return rpcStatusMake(s);
      Bool isremoved;
      s = db->isRemoved(components->getOid(), isremoved);
      if (s)
	return rpcStatusMake(s);
      if (!isremoved && !components->isRemoved()) {
	s = components->remove();
	if (s)
	  return rpcStatusMake(s);
      }
    }

    // disconnected 3/05/06 because it produced a bug in ODL updates when
    // we add an attribute in some case, for instance :
    // adding the attribute int contig_count produces a problem :
    /*
    class A {
      int j; index<hash> on j;
    };

    class C extends A {
      // int contig_count;  index<btree> on contig_count;
      string s;
      int h; constraint<notnull> on h;
    };
    */
    // but this deconnexion leads to DB memory leaks, because some
    // unuseful attr_comp_sets still in the DB : is it really a problem ?
#if 0
    rpc_status = IDB_removeAttrCompSet(db, cl);
    if (rpc_status) return rpc_status;
#endif
  
    eyedbsm::DbHandle *sedbh = dbh->sedbh;
    eyedbsm::Status se_status;
    eyedblib::int32 cnt;

    se_status = eyedbsm::objectRead(sedbh, IDB_SCH_CNT_INDEX, IDB_SCH_CNT_SIZE,
				    (void *)&cnt, eyedbsm::DefaultLock, 0, 0, &sch->oid);

    cnt = x2h_32(cnt);

    ClassHead *clsheads = new ClassHead[cnt];

    for (int k = 0; k < cnt; k++) {
      se_status = eyedbsm::objectRead(sedbh, IDB_SCH_OID_INDEX(k),
				      IDB_SCH_INCR_SIZE,
				      &clsheads[k], eyedbsm::DefaultLock, 0, 0, &sch->oid);
  
      if (se_status) {
	assert(0);
	delete [] clsheads;
	return rpcStatusMake_se(se_status);
      }

      clsheads[k].x2h();
    }

    int i, n;
    for (i = 0, n = 0; i < cnt; i++) {
      if (n != i)
	clsheads[n] = clsheads[i];

      if (memcmp(&clsheads[i].oid, oid, sizeof(*oid)))
	n++;
    }
  
    for (int j = n; j < cnt; j++)
      memset(&clsheads[j], 0, sizeof(ClassHead));

    if (n == i) {
      assert(0);
      delete [] clsheads;
      return rpcStatusMake
	(Exception::make(IDB_ERROR, "incoherent schema for class "
			 "deletion %s", Oid(oid).toString()));
    }

    assert(n == cnt-1);
    cnt--;
    sch->cnt--;
#ifdef E_XDR
    eyedblib::int32 cnt_x = h2x_32(cnt);
#else
    eyedblib::int32 cnt_x = cnt;
#endif
    se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_CNT_INDEX, IDB_SCH_CNT_SIZE,
				     (void *)&cnt_x, &sch->oid);
  
    if (se_status) {
      assert(0);
      delete [] clsheads;
      return rpcStatusMake_se(se_status);
    }

    for (int k = 0; k < cnt+1; k++) {
      clsheads[k].h2x();
      se_status = eyedbsm::objectWrite(sedbh, IDB_SCH_OID_INDEX(k),
				       IDB_SCH_INCR_SIZE,
				       &clsheads[k], &sch->oid);
  
      if (se_status) {
	assert(0);
	delete [] clsheads;
	return rpcStatusMake_se(se_status);
      }
    }

    delete [] clsheads;

    IDB_suppressClassFromSchema(db, cl->getOid());
    return IDB_instanceDelete(dbh, idr, hdr, oid);
  }

  /* class functions */
  static eyedblib::int32
  clean_xinfo(Data idr, ObjectHeader *hdr)
  {
    eyedblib::int32 xinfo;
    Offset offset = IDB_OBJ_HEAD_XINFO_INDEX;

    int32_decode(idr, &offset, &xinfo);
    if ((xinfo & IDB_XINFO_LOCAL_OBJ) == IDB_XINFO_LOCAL_OBJ)
      {
	Size alloc_size = hdr->size;
	eyedblib::int32 xinfo_n = xinfo & ~IDB_XINFO_LOCAL_OBJ;
	offset = IDB_OBJ_HEAD_XINFO_INDEX;
	int32_code(&idr, &offset, &alloc_size, &xinfo_n);
      }

    return xinfo;
  }

  static void
  restore_xinfo(Data idr, ObjectHeader *hdr, eyedblib::int32 xinfo)
  {
    if ((xinfo & IDB_XINFO_LOCAL_OBJ) == IDB_XINFO_LOCAL_OBJ)
      {
	Size alloc_size = hdr->size;
	Offset offset = IDB_OBJ_HEAD_XINFO_INDEX;
	int32_code(&idr, &offset, &alloc_size, &xinfo);
      }
  }

  static eyedblib::int64 current_time()
  {
    struct timeval tv;

    gettimeofday(&tv, 0);

    return (eyedblib::int64)tv.tv_sec * 1000000 + tv.tv_usec;
  }

  static RPCStatus
  IDB_instanceCreate(DbHandle *dbh, short dspid, Data idr, ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata, Bool coll_update)
  {
    CHECK_WRITE((Database *)dbh->db);
    eyedbsm::Status se_status;
    Offset offset;
    rpc_ServerData *data = (rpc_ServerData *)xdata;
    Size alloc_size = hdr->size;

    eyedblib::int32 xinfo = clean_xinfo(idr, hdr);

    // 24/08/05: MODIF time()
    eyedblib::int64 t = current_time();

    offset = IDB_OBJ_HEAD_CTIME_INDEX;
    int64_code(&idr, &offset, &alloc_size, &t);
    offset = IDB_OBJ_HEAD_MTIME_INDEX;
    int64_code(&idr, &offset, &alloc_size, &t);

    if (isOidValid(oid))
      se_status = eyedbsm::objectWrite(dbh->sedbh, 0, hdr->size, idr, oid);
    else {
      se_status = eyedbsm::objectCreate(dbh->sedbh, idr, hdr->size, dspid, oid);
      if (!se_status)
	((Database *)dbh->db)->addMarkCreated(Oid(*oid));
    }

    restore_xinfo(idr, hdr, xinfo);

    /* then must pass object to high level back end to create
       index entries */

    if (!se_status && coll_update)
      return IDB_collClassUpdate(dbh, idr, oid, 0, True);

    return rpcStatusMake_se(se_status);
  }

  static RPCStatus
  IDB_instanceWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, void *xdata)
  {
    CHECK_WRITE((Database *)dbh->db);
    eyedbsm::Status status;
    eyedblib::int32 k;
    Offset offset;
    rpc_ServerData *data = (rpc_ServerData *)xdata;
    Size alloc_size = hdr->size;

    eyedblib::int32 xinfo = clean_xinfo(idr, hdr);

    (void)eyedbsm::objectRead(dbh->sedbh, IDB_OBJ_HEAD_CTIME_INDEX, sizeof(int),
			      &k, eyedbsm::DefaultLock, 0, 0, oid);
#ifdef E_XDR
    k = x2h_32(k);
#endif
    offset = IDB_OBJ_HEAD_CTIME_INDEX;
    int32_code(&idr, &offset, &alloc_size, &k);

    // 24/08/05: MODIF time()
    eyedblib::int64 t = current_time();

    offset = IDB_OBJ_HEAD_MTIME_INDEX;
    int64_code(&idr, &offset, &alloc_size, &t);

    status = eyedbsm::objectWrite(dbh->sedbh, 0, hdr->size, idr, oid);

    restore_xinfo(idr, hdr, xinfo);
    return rpcStatusMake_se(status);
  }

  static RPCStatus
  IDB_instanceRead(DbHandle *dbh, Data idr, Data *pidr,
		   ObjectHeader *hdr, LockMode lockmode, const eyedbsm::Oid *oid,
		   void *xdata, int minsize)
  {
    eyedbsm::Status status;
    rpc_ServerData *data = (rpc_ServerData *)xdata;
    unsigned int size;

#ifdef LOAD_RM_BUG
    if (hdr->xinfo & IDB_XINFO_REMOVED)
      size = hdr->size = IDB_OBJ_HEAD_SIZE;
#endif
    else if (hdr->size < minsize)
      size = minsize;
    else
      size = hdr->size;

    if (data)
      {
	if (size <= data->buff_size)
	  data->status = rpc_BuffUsed;
	else
	  {
	    data->status = rpc_TempDataUsed;
	    data->data = malloc(size);
	  }

	data->size = size;
	status = eyedbsm::objectRead(dbh->sedbh, 0, hdr->size, data->data,
				     (eyedbsm::LockMode)lockmode, 0, 0, oid);
      }
    else
      {
	if (pidr)
	  idr = *pidr = (Data)malloc(size);

	status = eyedbsm::objectRead(dbh->sedbh, 0, hdr->size, idr,
				     (eyedbsm::LockMode)lockmode, 0, 0, oid);
      }


    return rpcStatusMake_se(status);
  }

  static RPCStatus
  IDB_instanceDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, Bool really)
  {
    Database *db = (Database *)dbh->db;
    CHECK_WRITE(db);
    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Class *cl = db->getSchema()->getClass(oid_cl);
    eyedbsm::Status se_status;
    Oid toid(oid);

    RPCStatus rpc_status = IDB_collClassUpdate(dbh, idr, oid, 0, False);
    if (rpc_status)
      return rpc_status;

#define DELETED_KEEP
    /*
      BUG concurrence sur le delete.

      client A: delete obj1 (sans faire commit ou abort!).

      client B: obj1 :
      oql error: eyedb error: object `obj1' does not exist anymore
      ou bien
      oql error: storage manager: invalid oid: invalid oid `obj1'

      -> ce probleme vient du fait qu'on ne detruit pas un objet (lorsque
      DELETED_KEEP est definit)
      -> on le modifie, puis on le change de taille! -> c.a.d. qu'on
      en cree un nouveau de la nouvelle taille et on affecte l'OID
      a ce nouveau. => ce nouvel oid (qui est en fait l'ancien) est
      locké en private.
      -> j'ai donc essaye de le locker en lockS normal!
      dans ce cas, l'object peut etre lu mais l'object_header qu'il
      contient est different de l'objet stocké puisque la taille stockée et
      la taille souhaitée (qui est la taille de tous les objets de sa classe)
      est differente.

      Solutions:
      - ne plus garder les OIDs detruits
      - les garder et:
      + reparer le probleme de concurrence de object size modify
      + ou ne pas modifier la taille de l'object: c.a.d. le garder en
      totalité (un peu couteux!).

      De toute facon, le fait de garder ou non les OIDs detruits devrait
      etre controllable:
      = un flag stocké dans la DB (default flag): modifiable par programme.
      = ce flag pourrait etre overloadé au runtime.
    */

    /*
      Une list markDeleted attaché à Database a été introduite
      afin de différer le changement de size => le bug précédant
      a été corrigé.
      Cependant il en reste un:
      si le create/remove d'un objet a lieu dans la meme transaction,
      cet objet est definitivement oublié: meme apres un commit,
      il n'apparait pas dans les autres clients: ce qui peut etre
      assez normal (en fait, non!).
      Cependant, il continue apparaitre dans le processus qui
      a cree et detruit cet objet => un peu bizarre.

      client 1: [eyedb] new Person();
      this_oid
      [eyedb] delete this_oid;
      [eyedb] !print
      <object removed>

      client 2: [eyedb] this_oid;
      invalid oid ....

      client 1: [eyedb] !commit
      [eyedb] !print this_oid
      <object removed>

      client 2: [eyedb] this_oid;
      invalid oid ....

      client 3: eyedboql -db <db>
      [eyedb] this_oid;
      invalid oid ....

      Par contre, si un !abort est fait au lieu d'un commit, c'est completement
      coherent.

      A noter que c'est le sizeModify qui fout le bordel.
      En supprimant l'appel a sizeModify, l'objet apparait bien dans
      les autres clients apres le commit sous forme d'une 'object removed'.
      C'est donc bien un bug!

      => corrected! (see se_trs.c: SE_TRObjectUnlock)

    */

#ifdef DELETED_KEEP
    int info;
    se_status = eyedbsm::objectRead(dbh->sedbh, IDB_OBJ_HEAD_XINFO_INDEX,
				    sizeof(int), &info, eyedbsm::DefaultLock, 0, 0, oid);
    if (se_status)
      return rpcStatusMake_se(se_status);

    info |= IDB_XINFO_REMOVED;

#ifdef E_XDR
    eyedblib::int32 info_x = h2x_32(info);
#else
    eyedblib::int32 info_x = info;
#endif
    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_OBJ_HEAD_XINFO_INDEX,
				     sizeof(int), &info_x, oid);
    // 24/08/05: MODIF time()
    eyedblib::int64 t = current_time();

#ifdef E_XDR
    eyedblib::int64 t_x = h2x_64(t);
#else
    eyedblib::int64 t_x = t;
#endif
    se_status = eyedbsm::objectWrite(dbh->sedbh, IDB_OBJ_HEAD_MTIME_INDEX,
				     sizeof(eyedblib::int64), &t_x, oid);
    if (se_status)
      return rpcStatusMake_se(se_status);

    // added the 27/02/02
    if (really)
      return rpcStatusMake_se(eyedbsm::objectDelete(dbh->sedbh, oid));

    if (db->isMarkCreated(Oid(oid))) {
      se_status = eyedbsm::objectDelete(dbh->sedbh, oid);
      return rpcStatusMake_se(se_status);
    }

    db->getMarkDeleted().insertObject(new Oid(oid));
    return RPCSuccess;
#else
    se_status = eyedbsm::objectDelete(dbh->sedbh, oid);
#endif
    return rpcStatusMake_se(se_status);
  }

#if 1
#define IDX_MANAGE(DB, MCL, HDR, IDR, OID, OP)
#else
#define IDX_MANAGE(DB, MCL, HDR, IDR, OID, OP) \
do { \
  if (!strcmp((MCL)->getName(), "hashindex") || \
      !strcmp((MCL)->getName(), "btreeindex")) \
  { \
    Status status; \
    Object *o; \
    Size objsz, psize, vsize; \
    objsz = (MCL)->getIDRObjectSize(&psize, &vsize); \
    Data nidr = (Data)malloc(objsz); \
    memcpy(nidr, idr, psize); \
    memset(nidr+psize, 0, vsize); \
    if (status = (DB)->makeObject(OID, HDR, nidr, &o)) \
      { \
	free(nidr); \
	return rpcStatusMake(status); \
      } \
    status = ((Index *)o)->##OP(DB); \
    o->release(); \
    free(nidr); \
    if (status) \
      return rpcStatusMake(status); \
  } \
} while(0)
#endif

  /* agregats */
  static RPCStatus
  IDB_agregatCreate(DbHandle *dbh, short dspid, Data idr, ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata)
  {
    RPCStatus rpc_status = IDB_instanceCreate(dbh, dspid, idr, hdr, oid, xdata, True);

    if (rpc_status != RPCSuccess)
      return rpc_status;

    Database *db = (Database *)dbh->db;
    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Class *cl = db->getSchema()->getClass(oid_cl);

    if (!cl)
      {
	rpc_status = IDB_agregatDelete(dbh, idr, hdr, oid, True);
	if (rpc_status) {
	  //fprintf(stderr, "error deletion %s\n", rpc_status->err_msg);
	  IDB_instanceDelete(dbh, idr, hdr, oid, True);
	}
	return rpcStatusMake(IDB_ERROR, "creating agregat");
      }

    Oid _oid(oid);
    Status status;

    AttrIdxContext idx_ctx;
    status = cl->asAgregatClass()->createIndexEntries_realize(db, idr, &_oid,
							      idx_ctx);
    if (status)
      {
	rpc_status = IDB_agregatDelete(dbh, idr, hdr, oid, True);
	if (rpc_status) {
	  //fprintf(stderr, "error deletion %s\n", rpc_status->err_msg);
	  IDB_instanceDelete(dbh, idr, hdr, oid, True);
	}
	return rpcStatusMake(status);
      }

    status = cl->asAgregatClass()->createInverses_realize(db, idr, &_oid);

    if (status)
      {
	rpc_status = IDB_agregatDelete(dbh, idr, hdr, oid, True);
	if (rpc_status) {
	  //fprintf(stderr, "error deletion %s\n", rpc_status->err_msg);
	  IDB_instanceDelete(dbh, idr, hdr, oid, True);
	}
      }

    if (!status)
      IDX_MANAGE(db, cl, hdr, idr, &_oid, create_index);

    return rpcStatusMake(status);
  }

  static RPCStatus
  IDB_agregatWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, void *xdata)
  {
    Database *db = (Database *)dbh->db;
    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Class *cl = db->getSchema()->getClass(oid_cl);
    RPCStatus rpc_status;

    Oid _oid(oid);
    AttrIdxContext idx_ctx;
    Status status = ((AgregatClass *)cl)->updateIndexEntries_realize(db, idr, &_oid, idx_ctx);

    if (status)
      return rpcStatusMake(status);

    status = ((AgregatClass *)cl)->updateInverses_realize(db, idr, &_oid);

    if (status)
      return rpcStatusMake(status);

    if (!status)
      IDX_MANAGE(db, cl, hdr, idr, &_oid, update_index);

    return IDB_instanceWrite(dbh, idr, hdr, oid, xdata);
  }

  static RPCStatus
  IDB_agregatDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, Bool really)
  {
    Database *db = (Database *)dbh->db;

    eyedbsm::Oid oid_cl = ClassOidDecode(idr);
    Class *cl = db->getSchema()->getClass(oid_cl);
    Oid _oid(oid);
    AttrIdxContext idx_ctx;
    Status status = cl->asAgregatClass()->removeIndexEntries_realize(db, idr, &_oid, idx_ctx);

    if (status)
      return rpcStatusMake(status);

    status = cl->asAgregatClass()->removeInverses_realize(db, idr, &_oid);

    if (status)
      return rpcStatusMake(status);

    if (!status)
      IDX_MANAGE(db, cl, hdr, idr, &_oid, delete_index);

    return IDB_instanceDelete(dbh, idr, hdr, oid, really);
  }

  /* protections */
  static RPCStatus
  IDB_protectionDescriptionMake(DbHandle *dbh,
				Protection *prot, Database *dbm,
				eyedbsm::ProtectionDescription *&pdesc)
  {
    int pusers_cnt = prot->getPusersCount();

    if (!pusers_cnt)
      return rpcStatusMake(IDB_ERROR, "invalid protection description");

    pdesc = (eyedbsm::ProtectionDescription *)
      calloc(eyedbsm::protectionDescriptionSize(pusers_cnt), 1);

    strcpy(pdesc->name, prot->getName().c_str());
    pdesc->nprot = pusers_cnt;

    for (int i = 0; i < pusers_cnt; i++)
      {
	ProtectionUser *puser = prot->getPusers(i);
	if (!puser)
	  return rpcStatusMake(IDB_ERROR, "user #%d is not set in protection description", i);

	ProtectionMode mode = puser->getMode();

	if (!mode)
	  mode = (ProtectionMode)0; // idbProtRead;

	UserEntry *user = (UserEntry *)puser->getUser();
	if (!puser)
	  return rpcStatusMake(IDB_ERROR, "user #%d is not set in protection description", i);

	pdesc->desc[i].uid    = user->uid();
	pdesc->desc[i].prot.r = ((mode == ProtRead || mode == ProtRW) ?
				 eyedbsm::ReadAll : eyedbsm::ReadNone);
	pdesc->desc[i].prot.w = ((mode == ProtRW) ? eyedbsm::WriteAll : eyedbsm::WriteNone);

	eyedbsm::DbProtectionDescription dbdesc;
	dbdesc.uid = pdesc->desc[i].uid;
	dbdesc.prot.r = eyedbsm::ReadAll;
	dbdesc.prot.w = eyedbsm::WriteAll;
	eyedbsm::Status se_status = eyedbsm::dbProtectionAdd(dbh->sedbh, &dbdesc, 1);

	if (se_status)
	  return rpcStatusMake_se(se_status);
      }

    return RPCSuccess;
  }

  static RPCStatus
  IDB_protectionRealize(DbHandle *dbh, Data idr, ObjectHeader *hdr, eyedbsm::Oid *oid, Bool create)
  {
    Database *db = (Database *)dbh->db;
    Protection *prot;
    Oid xoid(oid);
    Status status;
    RPCStatus rpc_status;

    status = db->makeObject(&xoid, hdr, idr, (Object **)&prot);

    if (status)
      return rpcStatusMake(status);

    //  printf(create ? "IDB_protectionCreate()\n" : "IDB_protectionMofidy\n");

    DBM_Database *dbm;
    const char *dbmdb = db->getDBMDB();
    rpc_status = IDB_dbmOpen(dbh->ch, dbmdb, True, &dbm);

    if (rpc_status)
      {
	prot->release();
	return rpc_status;
      }

    eyedbsm::ProtectionDescription *pdesc;

    rpc_status = IDB_protectionDescriptionMake(dbh, prot, dbm, pdesc);

    if (rpc_status)
      {
	prot->release();
	return rpc_status;
      }

    eyedbsm::Oid poid;

    if (create)
      {
	rpc_status = rpcStatusMake_se
	  (eyedbsm::protectionCreate(dbh->sedbh, pdesc, &poid));
	if (!rpc_status)
	  {
	    prot->setPoid(poid);
	    Size psize;
	    (void)prot->getClass()->getIDRObjectSize(&psize);
	    memcpy(idr, prot->getIDR(), psize);
	    //	  printf("OID : %s\n", prot->getPoid().getString());
	  }
      }
    else
      {
	//      printf("ROID : %s\n", prot->getOid().getString());
	rpc_status = rpcStatusMake_se
	  (eyedbsm::protectionModify(dbh->sedbh, pdesc, prot->getPoid().getOid()));
      }

    free(pdesc);
    prot->release();
    return rpc_status;
  }

  static RPCStatus
  IDB_protectionGetPoid(DbHandle *dbh, const eyedbsm::Oid *prot_oid, eyedbsm::Oid &poid)
  {
    Oid xoid(prot_oid);
    Status status;
    RPCStatus rpc_status;
    Protection *prot;
    Database *db = (Database *)dbh->db;

    status = db->loadObject(&xoid, (Object **)&prot);

    if (status)
      return rpcStatusMake(status);

    poid = *prot->getPoid().getOid();
    prot->release();

    return RPCSuccess;
  }

  static RPCStatus
  IDB_protectionCreate(DbHandle *dbh, short dspid, Data idr, ObjectHeader *hdr, eyedbsm::Oid *oid, void *xdata, Bool coll_update)
  {
    RPCStatus rpc_status;

    rpc_status = IDB_protectionRealize(dbh, idr, hdr, oid, True);
    if (rpc_status)
      return rpc_status;

    return IDB_agregatCreate(dbh, dspid, idr, hdr, oid, xdata);
  }

  static RPCStatus
  IDB_protectionWrite(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid, void *xdata)
  {
    RPCStatus rpc_status;

    rpc_status = IDB_protectionRealize(dbh, idr, hdr, (eyedbsm::Oid *)oid, False);
    if (rpc_status)
      return rpc_status;

    return IDB_agregatWrite(dbh, idr, hdr, oid, xdata);
  }

  static RPCStatus
  IDB_protectionDelete(DbHandle *dbh, Data idr, ObjectHeader *hdr, const eyedbsm::Oid *oid)
  {
    printf("IDB_protectionDelete()\n");
    return IDB_agregatDelete(dbh, idr, hdr, oid);
  }

  RPCStatus
  IDB_objectProtectionSet(DbHandle *dbh, const eyedbsm::Oid *obj_oid, const eyedbsm::Oid *prot_oid)
  {
    Oid xobj_oid(obj_oid);
    Oid xprot_oid(prot_oid);
    RPCStatus rpc_status;
    eyedbsm::Oid rprot_oid;

    /*
      printf("objectProtectionSet(%s, %s)\n", xobj_oid.getString(),
      xprot_oid.getString());
    */

    if (xprot_oid.isValid())
      {
	rpc_status = IDB_protectionGetPoid(dbh, prot_oid, rprot_oid);

	if (rpc_status)
	  return rpc_status;
      }
    else
      memset(&rprot_oid, 0, sizeof(rprot_oid));

    rpc_status = rpcStatusMake_se(eyedbsm::objectProtectionSet(dbh->sedbh, obj_oid, &rprot_oid));

    if (!rpc_status)
      {
	ObjectHeader hdr;
	rpc_status = IDB_objectHeaderRead(dbh, obj_oid, &hdr);
	if (rpc_status)
	  return rpc_status;
	hdr.oid_prot = *prot_oid;
	return IDB_objectHeaderWrite(dbh, obj_oid, &hdr);
      }

    return rpc_status;
  }

  RPCStatus
  IDB_objectProtectionGet(DbHandle *dbh, const eyedbsm::Oid *obj_oid, eyedbsm::Oid *prot_oid)
  {
    Oid xobj_oid(obj_oid);
    //  printf("objectProtectionGet(%s)\n", xobj_oid.getString());
    ObjectHeader hdr;
    RPCStatus rpc_status = IDB_objectHeaderRead(dbh, obj_oid, &hdr);
    if (rpc_status)
      return rpc_status;
    *prot_oid = hdr.oid_prot;
    return RPCSuccess;
    //  return rpcStatusMake_se(eyedbsm::objectProtectionGet(dbh->sedbh, obj_oid, prot_oid));
  }

#ifdef PASSWD_FILE
  static void
  get_dbm_passwd(const char *passwdfile)
  {
    int fd = open(passwdfile, O_RDONLY);

    if (fd < 0)
      {
	fprintf(stderr, "eyedbd: cannot open passwd file '%s' for reading\n",
		passwdfile);
	exit(1);
      }

    if (read(fd, dbm_passwd, sizeof(dbm_passwd)) != sizeof(dbm_passwd))
      {
	fprintf(stderr, "eyedbd: error while reading passwd file '%s'\n",
		passwdfile);
	exit(1);
      }

    close(fd);
  }
#endif

  //extern void idbInit_();

  void new_handler()
  {
    static Bool _new = False;
    if (!_new)
      {
	char tok[128];
	sprintf(tok, "PID %d: Ran out of memory\n", rpc_getpid());
	write(2, tok, strlen(tok));

	_new = True;
	quit_handler(db_list, 0);
      }

    exit(1);
  }

  static int timeout = -1;

  void
  config_init()
  {
    const char *s;
#ifdef PASSWD_FILE
    const char *passwdfile;

    if (edb_passwdfile)
      passwdfile = strdup(edb_passwdfile);
    else if (s = eyedb::getConfigValue("passwd_file"))
      passwdfile = strdup(s);
    else
      {
	fprintf(stderr, "eyedbd: EyeDB passwd file is not set, check your 'sv_passwd_file' configuration variable or the '-passwdfile' command line option\n");
	exit(1);
      }
#endif

    if (settimeout)
      {
	if (timeout >= 0)
	  settimeout(timeout);
	else
	  {
	    const char *x = eyedb::ServerConfig::getSValue("timeout");
	    if (x)
	      settimeout(atoi(x));
	  }
      }

#ifdef PASSWD_FILE
    get_dbm_passwd(passwdfile);
#endif

    /*
    // kludge
    s = eyedb::ServerConfig::getSValue("coll_hidx_oid");
    if (s) {
      if (!strcasecmp(s, "no")) {
	coll_hidx_oid = False;
      }
      else if (!strcasecmp(s, "yes")) {
	coll_hidx_oid = True;
      }
      else {
	printf("invalid value for coll_hidx_oid %s\n", s);
	exit(1);
      }
    }
    */
  }

  void
  IDB_init(const char *databasedir,
	   const char *dummy, // was  _passwdfile,
	   void *xsesslog, int _timeout)
  {
    //conn_ctx.sesslog = (SessionLog *)xsesslog;
    sesslogTempl = (SessionLog *)xsesslog;
    set_new_handler(new_handler);

#ifdef PASSWD_FILE
    edb_passwdfile = _passwdfile;
#endif
    timeout = _timeout;

    eyedb::init();
    config_init();

    if (databasedir)
      ServerConfig::getInstance()->setValue("databasedir", databasedir);
  }

  RPCStatus
  IDB_setLogMask(eyedblib::int64)
  {
    return RPCSuccess;
  }

  RPCStatus
  IDB_getDefaultDataspace(DbHandle * dbh, int *_dspid)
  {
    short dspid;
    eyedbsm::Status s = eyedbsm::dspGetDefault(dbh->sedbh, &dspid);
    if (s)
      return rpcStatusMake_se(s);

    *_dspid = dspid;
    return RPCSuccess;
  }

  RPCStatus
  IDB_setDefaultDataspace(DbHandle * dbh, int dspid)
  {
    std::string dataspace = str_convert((long)dspid);
    eyedbsm::Status s = eyedbsm::dspSetDefault(dbh->sedbh, dataspace.c_str());

    if (!s) return RPCSuccess;
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_dataspaceSetCurrentDatafile(DbHandle * dbh, int dspid, int datid)
  {
    std::string dataspace = str_convert((long)dspid);
    std::string datafile = str_convert((long)datid);
    eyedbsm::Status s = eyedbsm::dspSetCurDat(dbh->sedbh, dataspace.c_str(),
					      datafile.c_str());
    if (!s) return RPCSuccess;
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_dataspaceGetCurrentDatafile(DbHandle * dbh, int dspid, int * datid)
  {
    std::string dataspace = str_convert((long)dspid);
    short datid_s;
    eyedbsm::Status s = eyedbsm::dspGetCurDat(dbh->sedbh, dataspace.c_str(), &datid_s);

    if (!s) {
      *datid = datid_s;
      return RPCSuccess;
    }

    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_realizeDefaultIndexDataspace(DbHandle * dbh, const eyedbsm::Oid * idxoid,
				   int * dspid, int type, Bool get)
  {
    eyedbsm::Idx *idx;

    if (type)
      idx = new eyedbsm::HIdx(dbh->sedbh, idxoid);
    else
      idx = new eyedbsm::BIdx(dbh->sedbh, *idxoid);

    eyedbsm::Status s = idx->status();
    if (s) {
      delete idx;
      return rpcStatusMake_se(s);
    }

    if (get)
      *dspid = idx->getDefaultDspid();
    else
      idx->setDefaultDspid(*dspid);

    delete idx;
    return RPCSuccess;
  }

  RPCStatus
  IDB_getDefaultIndexDataspace(DbHandle * dbh, const eyedbsm::Oid * oid, int type, int * dspid)
  {
    return IDB_realizeDefaultIndexDataspace(dbh, oid, dspid, type, True);
  }

  RPCStatus
  IDB_setDefaultIndexDataspace(DbHandle * dbh, const eyedbsm::Oid * oid, int type, int dspid)
  {
    return IDB_realizeDefaultIndexDataspace(dbh, oid, &dspid, type, False);
  }

  static RPCStatus
  IDB_getIndexObjects(DbHandle * dbh, const eyedbsm::Oid * idxoid, int type,
		      eyedbsm::Oid *&oids, unsigned int &cnt)
  {
    cnt = 0;
    eyedbsm::Idx *idx;

    if (type) 
      idx = new eyedbsm::HIdx(dbh->sedbh, idxoid);
    else
      idx = new eyedbsm::BIdx(dbh->sedbh, *idxoid);

    eyedbsm::Status s = idx->status();
    if (s) {
      delete idx;
      return rpcStatusMake_se(s);
    }

    time_t t0, t1;
    time(&t0);
    //printf("getIndexObjects start\n");
    s = idx->getObjects(oids, cnt);
    time(&t1);
    //printf("getIndexObjects end count=%d [elapsed %d]\n", t1-t0);

    delete idx;
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_getIndexLocations(DbHandle * dbh, const eyedbsm::Oid * oid, int type,
			Data * locarr, void *xlocarr)
  {
    eyedbsm::Oid *oids;
    unsigned int cnt;
    RPCStatus rpc_status = IDB_getIndexObjects(dbh, oid, type, oids, cnt);
    if (rpc_status) return rpc_status;
    rpc_status = IDB_getObjectsLocations(dbh, oids, cnt, 0, locarr, xlocarr);
    free(oids);
    return rpc_status;
  }

  RPCStatus
  IDB_moveIndex(DbHandle * dbh, const eyedbsm::Oid * oid, int type, int dspid)
  {
    eyedbsm::Oid *oids;
    unsigned int cnt;
    RPCStatus rpc_status = IDB_getIndexObjects(dbh, oid, type, oids, cnt);
    if (rpc_status) return rpc_status;

    rpc_status = IDB_moveObjects(dbh, oids, cnt, dspid, 0);
    free(oids);
    return rpc_status;
  }

  extern eyedbsm::Oid *
  oidArrayToOids(const OidArray &oid_arr, unsigned int &cnt);

  RPCStatus
  IDB_getInstanceClassLocations(DbHandle * dbh, const eyedbsm::Oid * oid,
				int subclasses, Data * locarr, void *xlocarr)
  {
    Database *db = (Database *)dbh->db;
    const Class *cls = db->getSchema()->getClass(*oid);
    if (!cls)
      return rpcStatusMake(IDB_ERROR, "class %s not found",
			   Oid(*oid).toString());
    Iterator iter(const_cast<Class *>(cls),	IDBBOOL(subclasses));
    OidArray oid_arr;
    Status s = iter.scan(oid_arr);
    if (s) return rpcStatusMake(s);

    unsigned int cnt;
    eyedbsm::Oid *oids = oidArrayToOids(oid_arr, cnt);
    RPCStatus rpc_status = IDB_getObjectsLocations(dbh, oids, cnt, 0, locarr,
						   xlocarr);
    delete [] oids;
    return rpc_status;
  }

  RPCStatus
  IDB_moveInstanceClass(DbHandle * dbh, const eyedbsm::Oid * oid, int subclasses,
			int dspid)
  {
    Database *db = (Database *)dbh->db;
    const Class *cls = db->getSchema()->getClass(*oid);
    if (!cls)
      return rpcStatusMake(IDB_ERROR, "class %s not found",
			   Oid(*oid).toString());
    Iterator iter(const_cast<Class *>(cls), IDBBOOL(subclasses));
    OidArray oid_arr;
    Status s = iter.scan(oid_arr);
    if (s) return rpcStatusMake(s);

    unsigned int cnt;
    eyedbsm::Oid *oids = oidArrayToOids(oid_arr, cnt);
    if (!cnt) return RPCSuccess;
    RPCStatus rpc_status = IDB_moveObjects(dbh, oids, cnt, dspid, 0);
    delete [] oids;
    return rpc_status;
  }

  RPCStatus
  IDB_getObjectsLocations(DbHandle * dbh, const eyedbsm::Oid * yoids, unsigned int oid_cnt, void *xoids, Data * locarr, void *xlocarr)
  {
    lock_data((Data *)&yoids, xoids);

    const eyedbsm::Oid * oids;
    if (xoids)
      oids = decode_oids((Data)yoids, &oid_cnt);
    else
      oids = yoids;

    eyedbsm::ObjectLocation *se_locarr = new eyedbsm::ObjectLocation[oid_cnt];
    eyedbsm::Status s = eyedbsm::objectsLocationGet(dbh->sedbh, oids, se_locarr, oid_cnt);

    if (xlocarr) {
      rpc_ServerData *data = (rpc_ServerData *)xlocarr;
      data->status = rpc_TempDataUsed;
      data->data = code_locarr(se_locarr, oids, oid_cnt, &data->size);
    }
    else
      make_locarr(se_locarr, oids, oid_cnt, locarr);

    if (xoids)
      delete [] oids;

    delete [] se_locarr;

    unlock_data((Data)yoids, xoids);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_moveObjects(DbHandle * dbh, const eyedbsm::Oid *yoids, unsigned oid_cnt,
		  int dspid, void *xoids)
  {
    CHECK_WRITE((Database *)dbh->db);
    lock_data((Data *)&yoids, xoids);

    const eyedbsm::Oid * oids;
    if (xoids)
      oids = decode_oids((Data)yoids, &oid_cnt);
    else
      oids = yoids;

    eyedbsm::Status s = eyedbsm::objectsMoveDsp(dbh->sedbh, oids, oid_cnt, dspid);

    if (xoids)
      delete [] oids;

    unlock_data((Data)yoids, xoids);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_getAttributeLocations(DbHandle * dbh, const eyedbsm::Oid * clsoid, int attrnum, Data * locarr, void *xlocarr)
  {
    return RPCSuccess;
  }

  RPCStatus
  IDB_moveAttribute(DbHandle * dbh, const eyedbsm::Oid * clsoid, int attrnum, int dspid)
  {
    return RPCSuccess;
  }

  RPCStatus
  IDB_createDatafile(DbHandle * dbh, const char * datfile, const char * name, int maxsize, int slotsize, int dtype)
  {
    mode_t file_mask;
    const char *file_group;
    RPCStatus rpc_status = get_file_mask_group(file_mask, file_group);
    if (rpc_status)
      return rpc_status;

    eyedbsm::Status s = eyedbsm::datCreate(dbh->sedbh, datfile, name, maxsize,
					   eyedbsm::BitmapType, slotsize,
					   (eyedbsm::DatType)dtype, file_mask, file_group);

    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_deleteDatafile(DbHandle * dbh, int datid)
  {
    eyedbsm::Status s = eyedbsm::datDelete(dbh->sedbh,
					   str_convert((long)datid).c_str());
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_moveDatafile(DbHandle * dbh, int datid, const char * newdatafile)
  {
    eyedbsm::Status s = eyedbsm::datMove(dbh->sedbh,
					 str_convert((long)datid).c_str(),
					 newdatafile);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_defragmentDatafile(DbHandle * dbh, int datid)
  {
    mode_t file_mask;
    const char *file_group;
    RPCStatus rpc_status = get_file_mask_group(file_mask, file_group);
    if (rpc_status)
      return rpc_status;

    eyedbsm::Status s = eyedbsm::datDefragment(dbh->sedbh,
					       str_convert((long)datid).c_str(), file_mask, file_group);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_resizeDatafile(DbHandle * dbh, int datid, unsigned int size)
  {
    eyedbsm::Status s = eyedbsm::datResize(dbh->sedbh,
					   str_convert((long)datid).c_str(),
					   size);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_getDatafileInfo(DbHandle * dbh, int datid, Data * info,
		      void *xinfo)
  {
    eyedbsm::DatafileInfo datinfo;
    eyedbsm::Status s = eyedbsm::datGetInfo(dbh->sedbh,
					    str_convert((long)datid).c_str(),
					    &datinfo);
    if (xinfo) {
      rpc_ServerData *data = (rpc_ServerData *)xinfo;
      data->status = rpc_TempDataUsed;
      data->data = code_datinfo(&datinfo, &data->size);
    }
    else
      make_datinfo(&datinfo, info);

    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_renameDatafile(DbHandle * dbh, int datid, const char * name)
  {
    eyedbsm::Status s = eyedbsm::datRename(dbh->sedbh,
					   str_convert((long)datid).c_str(),
					   name);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_createDataspace(DbHandle * dbh, const char * dspname, void *ydatids,
		      unsigned int datafile_cnt, void *xdatids)
  {
    CHECK_WRITE((Database *)dbh->db);
    lock_data((Data *)&ydatids, xdatids);

    const char **datids;
    if (xdatids)
      datids = decode_datids((Data)ydatids, &datafile_cnt);
    else
      datids = (const char **)ydatids;

    eyedbsm::Status s = eyedbsm::dspCreate(dbh->sedbh, dspname, datids, datafile_cnt);
    if (xdatids)
      free(datids);

    unlock_data((Data)ydatids, xdatids);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_updateDataspace(DbHandle * dbh, int dspid,  void *ydatids,
		      unsigned int datafile_cnt, void *xdatids,
		      int flags, int orphan_dspid)
  {
    CHECK_WRITE((Database *)dbh->db);
    lock_data((Data *)&ydatids, xdatids);

    const char **datids;
    if (xdatids)
      datids = decode_datids((Data)ydatids, &datafile_cnt);
    else
      datids = (const char **)ydatids;

    eyedbsm::Status s = eyedbsm::dspUpdate(dbh->sedbh,
					   str_convert((long)dspid).c_str(),
					   datids, datafile_cnt,
					   flags, orphan_dspid);
    if (xdatids)
      free(datids);

    unlock_data((Data)ydatids, xdatids);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_deleteDataspace(DbHandle * dbh, int dspid)
  {
    eyedbsm::Status s = eyedbsm::dspDelete(dbh->sedbh,
					   str_convert((long)dspid).c_str());
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_renameDataspace(DbHandle * dbh, int dspid, const char * name)
  {
    eyedbsm::Status s = eyedbsm::dspRename(dbh->sedbh,
					   str_convert((long)dspid).c_str(),
					   name);
    return rpcStatusMake_se(s);
  }

  static Status
  IDB_makeDatabase(ConnHandle *ch, const char *dbmdb, const char *dbname,
		   int dbid, int flags, DbHandle *ldbh,
		   rpcDB_LocalDBContext *ldbctx,
		   const eyedbsm::Oid *sch_oid, unsigned int version, Database *&db)
  {
    Status status;
    db = new Database(dbname, dbmdb);

    Oid _oid(sch_oid);

    if ((status = db->set(ch, dbid, flags, ldbh, ldbctx, &_oid, version)) == Success)
      return status;
    else
      {
	db->release();
	return status;
      }
  }

  /*
    const char *
    time()
    {
    time_t t;
    time(&t);
    char *s = ctime(&t);
    s[strlen(s)-1] = 0;
    return s;
    }
  */

  //#define EPITRACE

  void object_epilogue(void *xdb, const eyedbsm::Oid *oid,
		       Data inv_data, Bool creating)
  {
    // should cache Object.here
    /*
      if (creating)
      db->cacheObject(oid);
    */

    if (!inv_data)
      return;

    Database *db = (Database *)xdb;

    LinkedList inv_list;

    InvOidContext::decode(inv_data, inv_list);

    free(inv_data);

    LinkedListCursor c(inv_list);
    InvOidContext *ctx;

#ifdef EPITRACE
    if (inv_list.getCount())
      printf("object_epilogue -> %d\n", inv_list.getCount());
#endif

    for (int i = 0; c.getNext((void *&)ctx); i++)
      {
#ifdef EPITRACE
	printf("object_epilogue[%d] -> %s %d %d %s\n",
	       i, ctx->objoid.toString(), ctx->attr_num, ctx->attr_offset,
	       ctx->valoid.toString());
#endif
	Object *o = db->getCacheObject(ctx->objoid);
	if (o)
	  {
#ifdef EPITRACE
	    printf("object_epilogue: updating object %s\n",
		   ctx->objoid.toString());
#endif
	    // 22/05/06
	    //mcp(o->getIDR()+ctx->attr_offset, &ctx->valoid, sizeof(Oid));
	    eyedbsm::Oid toid;
	    eyedbsm::h2x_oid(&toid, ctx->valoid.getOid());
	    mcp(o->getIDR()+ctx->attr_offset, &toid, sizeof(Oid));
	  }

	delete ctx;
      }
  }

  eyedbsm::DbHandle *
  IDB_get_se_DbHandle(Database *db)
  {
    return database_getDbHandle(db)->sedbh;
  }

  //
  // coding and decoding functions
  //

  Data code_oids(const eyedbsm::Oid *oids, unsigned int oid_cnt, int *size)
  {
    Data idr = 0;
    Size alloc_size = 0;
    Offset offset = 0;
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&oid_cnt);

    for (int i = 0; i < oid_cnt; i++)
      oid_code(&idr, &offset, &alloc_size, &oids[i]);

    *size = offset;
    return idr;
  }

  static eyedbsm::Oid *
  decode_oids(Data idr, unsigned int *poid_cnt)
  {
    *poid_cnt = 0;
    Offset offset = 0;
    int32_decode(idr, &offset, (eyedblib::int32 *)poid_cnt);
    eyedbsm::Oid *oids = new eyedbsm::Oid[*poid_cnt];
    for (int i = 0; i < *poid_cnt; i++) 
      oid_decode(idr, &offset, &oids[i]);
    return oids;
  }

  void
  decode_locarr(Data idr, void *xlocarr)
  {
    Offset offset = 0;
    eyedblib::int32 cnt;
    int32_decode(idr, &offset, &cnt);

    ObjectLocation *locs = new ObjectLocation[cnt];
    for (int i = 0; i < cnt; i++) {
      eyedbsm::Oid oid;
      eyedblib::int16 dspid;
      eyedblib::int16 datid;
      ObjectLocation::Info info;

      oid_decode(idr, &offset, &oid);
      int16_decode(idr, &offset, &dspid);
      int16_decode(idr, &offset, &datid);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.size);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.slot_start_num);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.slot_end_num);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.dat_start_pagenum);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.dat_end_pagenum);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.omp_start_pagenum);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.omp_end_pagenum);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.dmp_start_pagenum);
      int32_decode(idr, &offset, (eyedblib::int32 *)&info.dmp_end_pagenum);

      locs[i].set(oid, dspid, datid, info);
    }

    ((ObjectLocationArray *)xlocarr)->set(locs, cnt);
  }

  static Data
  code_locarr(const eyedbsm::ObjectLocation *se_locarr, const eyedbsm::Oid *oids,
	      unsigned int cnt, int *size)
  {
    Data idr = 0;
    Size alloc_size = 0;
    Offset offset = 0;

    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&cnt);
    for (int i = 0; i < cnt; i++) {
      const eyedbsm::ObjectLocation *loc = &se_locarr[i];

      oid_code(&idr, &offset, &alloc_size, &oids[i]);
      int16_code(&idr, &offset, &alloc_size, &loc->dspid);
      int16_code(&idr, &offset, &alloc_size, &loc->datid);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->slot_start_num);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->slot_end_num);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->dat_start_pagenum);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->dat_end_pagenum);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->omp_start_pagenum);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->omp_end_pagenum);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->dmp_start_pagenum);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&loc->dmp_end_pagenum);
    }

    *size = offset;
    return idr;
  }

  static void
  make_locarr(const eyedbsm::ObjectLocation *se_locarr, const eyedbsm::Oid *oids,
	      unsigned int cnt, void *xlocarr)
  {
    ObjectLocationArray *locarr = (ObjectLocationArray *)xlocarr;
    ObjectLocation *locs = new ObjectLocation[cnt];
    for (int i = 0; i < cnt; i++) {
      const eyedbsm::ObjectLocation *loc = &se_locarr[i];

      ObjectLocation::Info info;
      info.size = loc->size;
      info.slot_start_num = loc->slot_start_num;
      info.slot_end_num = loc->slot_end_num;
      info.dat_start_pagenum = loc->dat_start_pagenum;
      info.dat_end_pagenum = loc->dat_end_pagenum;
      info.omp_start_pagenum = loc->omp_start_pagenum;
      info.omp_end_pagenum = loc->omp_end_pagenum;
      info.dmp_start_pagenum = loc->dmp_start_pagenum;
      info.dmp_end_pagenum = loc->dmp_end_pagenum;

      locs[i].set(oids[i], loc->dspid, loc->datid, info);
    }

    locarr->set(locs, cnt);
  }

  Data
  code_dbdescription(const DbCreateDescription *dbdesc, int *size)
  {
    Data idr = 0;
    Size alloc_size = 0;
    Offset offset = 0;
    const eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
    int i;

    string_code(&idr, &offset, &alloc_size, dbdesc->dbfile);
    /*
    eyedblib::int32 mode = d->file_mode;
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&mode);
    string_code(&idr, &offset, &alloc_size, d->file_group);
    */

    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&d->dbid);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&d->nbobjs);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->dbsfilesize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->dbsfileblksize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->ompfilesize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->ompfileblksize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->shmfilesize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&d->shmfileblksize);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&d->ndat);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&d->ndsp);
    //int type = d->mtype;
    //int32_code(&idr, &offset, &alloc_size, &type);

    for (i = 0; i < d->ndat; i++)
      {
	const eyedbsm::Datafile *v = &d->dat[i];
	string_code(&idr, &offset, &alloc_size, v->file);
	string_code(&idr, &offset, &alloc_size, v->name);
	int type = v->mtype;
	int16_code(&idr, &offset, &alloc_size, &v->dspid);
	int32_code(&idr, &offset, &alloc_size, &type);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&v->sizeslot);
	int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&v->maxsize);
	eyedblib::int32 dtype = v->dtype;
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&dtype);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&v->extflags);
      }

    for (i = 0; i < d->ndsp; i++)
      {
	const eyedbsm::Dataspace *v = &d->dsp[i];
	string_code(&idr, &offset, &alloc_size, v->name);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&v->ndat);
	for (int j = 0; j < v->ndat; j++)
	  int16_code(&idr, &offset, &alloc_size, &v->datid[j]);
      }

    *size = offset;
    return idr;
  }

  static const char **
  decode_datids(Data idr, unsigned int *oid_cnt)
  {
    Offset offset = 0;
    eyedblib::int32 cnt;
    int32_decode(idr, &offset, &cnt);
    const char **datids = new const char *[cnt];

    for (int i = 0; i < cnt; i++)
      string_decode(idr, &offset, (char **)&datids[i]);

    *oid_cnt = cnt;
    return datids;
  }

  Data
  code_datafiles(void *datafiles, unsigned int datafile_cnt, int *size)
  {
    char **s = (char **)datafiles;
    Data idr = 0;
    Size alloc_size = 0;
    Offset offset = 0;
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&datafile_cnt);
    for (int i = 0; i < datafile_cnt; i++)
      string_code(&idr, &offset, &alloc_size, s[i]);
    *size = offset;
    return idr;
  }

  static Data
  code_datinfo(const eyedbsm::DatafileInfo *info, int *size)
  {
    Data idr = 0;
    Size alloc_size = 0;
    Offset offset = 0;

    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->objcnt);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->slotcnt);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->busyslotcnt);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->totalsize);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->avgsize);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->lastbusyslot);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->lastslot);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->busyslotsize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->datfilesize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->datfileblksize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->dmpfilesize);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->dmpfileblksize);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->curslot);
    int64_code(&idr, &offset, &alloc_size, (eyedblib::int64 *)&info->defragmentablesize);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&info->slotfragcnt);
    double_code(&idr, &offset, &alloc_size, &info->used);

    *size = offset;
    return idr;
  }

  void
  decode_datinfo(Data idr, void *xinfo)
  {
    DatafileInfo::Info *info = (DatafileInfo::Info *)xinfo;
    Offset offset = 0;

    int32_decode(idr, &offset, (eyedblib::int32 *)&info->objcnt);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->slotcnt);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->busyslotcnt);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->totalsize);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->avgsize);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->lastbusyslot);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->lastslot);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->busyslotsize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->datfilesize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->datfileblksize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->dmpfilesize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->dmpfileblksize);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->curslot);
    int64_decode(idr, &offset, (eyedblib::int64 *)&info->defragmentablesize);
    int32_decode(idr, &offset, (eyedblib::int32 *)&info->slotfragcnt);
    double_decode(idr, &offset, &info->used);
  }

  static void
  make_datinfo(const eyedbsm::DatafileInfo *datinfo, Data *xinfo)
  {
    DatafileInfo::Info *info = (DatafileInfo::Info *)xinfo;

    info->objcnt = datinfo->objcnt;
    info->slotcnt = datinfo->slotcnt;
    info->busyslotcnt = datinfo->busyslotcnt;
    info->totalsize = datinfo->totalsize;
    info->avgsize = datinfo->avgsize;
    info->lastbusyslot = datinfo->lastbusyslot;
    info->lastslot = datinfo->lastslot;
    info->busyslotsize = datinfo->busyslotsize;
    info->datfilesize = datinfo->datfilesize;
    info->datfileblksize = datinfo->datfileblksize;
    info->dmpfilesize = datinfo->dmpfilesize;
    info->dmpfileblksize = datinfo->dmpfileblksize;
    info->curslot = datinfo->curslot;
    info->defragmentablesize = datinfo->defragmentablesize;
    info->slotfragcnt = datinfo->slotfragcnt;
    info->used = datinfo->used;
  }


  void
  decode_dbdescription(Data idr, void *xdata, DbCreateDescription *dbdesc)
  {
    Offset offset = 0;
    eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
    int i;
    char *s;

    memset(dbdesc, 0, sizeof(*dbdesc));
    lock_data(&idr, xdata);
    string_decode(idr, &offset, &s);
    strcpy(dbdesc->dbfile, s);

    /*
    eyedblib::int32 mode;
    int32_decode(idr, &offset, (eyedblib::int32 *)&mode);
    d->file_mode = mode;
    string_decode(idr, &offset, &s);
    int len = strlen(s);
    if (len > sizeof(d->file_group)-1)
      len = sizeof(d->file_group);
    strncpy(d->file_group, s, len);
    d->file_group[len] = 0;
    */

    int32_decode(idr, &offset, (eyedblib::int32 *)&d->dbid);
    int32_decode(idr, &offset, (eyedblib::int32 *)&d->nbobjs);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->dbsfilesize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->dbsfileblksize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->ompfilesize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->ompfileblksize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->shmfilesize);
    int64_decode(idr, &offset, (eyedblib::int64 *)&d->shmfileblksize);
    int32_decode(idr, &offset, (eyedblib::int32 *)&d->ndat);
    int32_decode(idr, &offset, (eyedblib::int32 *)&d->ndsp);
    //int mtype;
    //int32_decode(idr, &offset, &mtype);
    //d->mtype = (se_MapType)mtype;

    for (i = 0; i < d->ndat; i++)
      {
	eyedbsm::Datafile *v = &d->dat[i];
	string_decode(idr, &offset, &s);
	strcpy(v->file, s);
	string_decode(idr, &offset, &s);
	strcpy(v->name, s);
	eyedblib::int32 type;
	int16_decode(idr, &offset, &v->dspid);
	int32_decode(idr, &offset, &type);
	v->mtype = (eyedbsm::MapType)type;
	int32_decode(idr, &offset, (eyedblib::int32 *)&v->sizeslot);
	int64_decode(idr, &offset, (eyedblib::int64 *)&v->maxsize);
	eyedblib::int32 dtype;
	int32_decode(idr, &offset, (eyedblib::int32 *)&dtype);
	v->dtype = (DatType)dtype;
	int32_decode(idr, &offset, (eyedblib::int32 *)&v->extflags);
      }

    for (i = 0; i < d->ndsp; i++)
      {
	eyedbsm::Dataspace *v = &d->dsp[i];
	string_decode(idr, &offset, &s);
	strcpy(v->name, s);
	int32_decode(idr, &offset, (eyedblib::int32 *)&v->ndat);
	for (int j = 0; j < v->ndat; j++)
	  int16_decode(idr, &offset, &v->datid[j]);
      }	

    unlock_data(idr, xdata);
  }

  static void
  decode_index_impl_r(Data data, void *ximpl, Offset &offset)
  {
    short type;
    int16_decode(data, &offset, &type);

    if (type == IndexImpl::Hash) {
      int key_count;
      /*
	short dspid;
	int16_decode(data, &offset, &dspid);
      */
      int32_decode(data, &offset, &key_count);
      int impl_hints[eyedbsm::HIdxImplHintsCount];
      for (int i = 0; i < eyedbsm::HIdxImplHintsCount; i++)
	int32_decode(data, &offset, &impl_hints[i]);

      *(IndexImpl **)ximpl = new IndexImpl(IndexImpl::Hash,
					   0, // dspid ???
					   key_count,
					   0, // mth ???
					   impl_hints,
					   eyedbsm::HIdxImplHintsCount);
    }
    else {
      int degree;
      int32_decode(data, &offset, &degree);
      int impl_hints[eyedbsm::HIdxImplHintsCount];
      for (int i = 0; i < eyedbsm::HIdxImplHintsCount; i++)
	int32_decode(data, &offset, &impl_hints[i]);

      *(IndexImpl **)ximpl = new IndexImpl(IndexImpl::BTree,
					   0,
					   degree,
					   0,
					   impl_hints,
					   eyedbsm::HIdxImplHintsCount);
    }
  }

  void decode_index_impl(Data data, void *ximpl)
  {
    Offset offset = 0;
    decode_index_impl_r(data, ximpl, offset);
  }

  void decode_index_stats(Data data, void *xstats)
  {
    Offset offset = 0;

    short type;
    int16_decode(data, &offset, &type);

    if (type == IndexImpl::Hash) {
      HashIndexStats *stats = new HashIndexStats();
      decode_index_impl_r(data, &stats->idximpl, offset);

      int32_decode(data, &offset, (eyedblib::int32 *)&stats->min_objects_per_entry);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->max_objects_per_entry);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_object_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_hash_object_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_hash_object_size);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_hash_object_busy_size);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->busy_key_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->free_key_count);

      int32_decode(data, &offset, (eyedblib::int32 *)&stats->key_count);
      HashIndexStats::Entry *entry = stats->entries =
	new HashIndexStats::Entry[stats->key_count];
      for (int i = 0; i < stats->key_count; i++, entry++) {
	int32_decode(data, &offset, (eyedblib::int32 *)&entry->object_count);
	int32_decode(data, &offset, (eyedblib::int32 *)&entry->hash_object_count);
	int32_decode(data, &offset, (eyedblib::int32 *)&entry->hash_object_size);
	int32_decode(data, &offset, (eyedblib::int32 *)&entry->hash_object_busy_size);
      }
      *(HashIndexStats **)xstats = stats;
    }
    else {
      BTreeIndexStats *stats = new BTreeIndexStats();
      decode_index_impl_r(data, &stats->idximpl, offset);

      int32_decode(data, &offset, (eyedblib::int32 *)&stats->degree);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->dataSize);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->keySize);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->keyOffset);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->keyType);

      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_object_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_btree_object_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->btree_node_size);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_btree_node_count);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->btree_key_object_size);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->btree_data_object_size);
      int32_decode(data, &offset, (eyedblib::int32 *)&stats->total_btree_object_size);

      *(BTreeIndexStats **)xstats = stats;
    }
  }

  static void
  make_index_stats(const eyedbsm::BIdx::Stats &stats, Data *xrstats)
  {
    BTreeIndexStats *rstats = new BTreeIndexStats();
    rstats->degree = stats.idx->getDegree();
    rstats->dataSize = stats.idx->getDataSize();
    rstats->keySize = stats.idx->getKeySize();
    rstats->keyOffset = stats.keyOffset;
    rstats->keyType = stats.keyType;

    rstats->total_object_count = stats.total_object_count;
    rstats->total_btree_object_count = stats.total_btree_object_count;
    rstats->btree_node_size = stats.btree_node_size;
    rstats->total_btree_node_count = stats.total_btree_node_count;
    rstats->btree_key_object_size = stats.btree_key_object_size;
    rstats->btree_data_object_size = stats.btree_data_object_size;
    rstats->total_btree_object_size = stats.total_btree_object_size;

    rstats->idximpl = new IndexImpl(IndexImpl::BTree, 0,
				    stats.idx->getDegree(),
				    0, 0, 0);
    *(BTreeIndexStats **)xrstats = rstats;
  }

  static void
  make_index_stats(const eyedbsm::HIdx::Stats &stats, Data *xrstats)
  {
    HashIndexStats *rstats = new HashIndexStats();
    rstats->min_objects_per_entry = stats.min_objects_per_entry;
    rstats->max_objects_per_entry = stats.max_objects_per_entry;
    rstats->total_object_count = stats.total_object_count;
    rstats->total_hash_object_count = stats.total_hash_object_count;
    rstats->total_hash_object_size = stats.total_hash_object_size;
    rstats->total_hash_object_busy_size = stats.total_hash_object_busy_size;
    rstats->busy_key_count = stats.busy_key_count;
    rstats->free_key_count = stats.free_key_count;

    rstats->key_count = stats.idx.key_count;
    rstats->entries = new HashIndexStats::Entry[rstats->key_count];
    for (int i = 0; i < rstats->key_count; i++) {
      rstats->entries[i].object_count = stats.entries[i].object_count;
      rstats->entries[i].hash_object_count = stats.entries[i].hash_object_count;
      rstats->entries[i].hash_object_size = stats.entries[i].hash_object_size;
      rstats->entries[i].hash_object_busy_size = stats.entries[i].hash_object_busy_size;
    }

    rstats->idximpl = new IndexImpl(IndexImpl::Hash, 0,
				    stats.idx.key_count,
				    0, stats.idx.impl_hints,
				    eyedbsm::HIdxImplHintsCount);
    *(HashIndexStats **)xrstats = rstats;
  }


  static Data
  code_index_impl(const eyedbsm::BIdx &idx, int *size,
		  Data &idr, Offset &offset, Size &alloc_size)
  {

    short xtype = IndexImpl::BTree;
    int16_code(&idr, &offset, &alloc_size, &xtype);

    //int16_code(&idr, &offset, &alloc_size, &idx.dspid);
    unsigned int degree = idx.getDegree();
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&degree);
    for (int i = 0; i < eyedbsm::HIdxImplHintsCount; i++) {
      int zero = 0;
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&zero);
    }
    *size = offset;
    return idr;
  }

  static Data
  code_index_impl(const eyedbsm::BIdx &idx, int *size)
  {
    Data idr = 0;
    Offset offset = 0;
    Size alloc_size = 0;
    return code_index_impl(idx, size, idr, offset, alloc_size);
  }

  static Data
  code_index_impl(const eyedbsm::HIdx::_Idx &idx, int *size,
		  Data &idr, Offset &offset, Size &alloc_size)
  {

    short xtype = IndexImpl::Hash;
    int16_code(&idr, &offset, &alloc_size, &xtype);

    //int16_code(&idr, &offset, &alloc_size, &idx.dspid);
    int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&idx.key_count);
    for (int i = 0; i < eyedbsm::HIdxImplHintsCount; i++)
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&idx.impl_hints[i]);

    *size = offset;
    return idr;
  }

  static Data
  code_index_impl(const eyedbsm::HIdx::_Idx &idx, int *size)
  {
    Data idr = 0;
    Offset offset = 0;
    Size alloc_size = 0;
    return code_index_impl(idx, size, idr, offset, alloc_size);
  }

  static void
  make_index_impl(const eyedbsm::BIdx &idx, Data *data)
  {
    *(IndexImpl **)data = new IndexImpl(IndexImpl::BTree,
					0, idx.getDegree(), 0,
					0, 0);
  }

  static void
  make_index_impl(const eyedbsm::HIdx::_Idx &idx, Data *data)
  {
    *(IndexImpl **)data = new IndexImpl(IndexImpl::Hash,
					0,
					idx.key_count, 0,
					idx.impl_hints,
					eyedbsm::HIdxImplHintsCount);
  }

  static Data
  code_index_stats(IndexImpl::Type type, const void *xstats,
		   int *size)
  {
    Data idr = 0;
    Offset offset = 0;
    Size alloc_size = 0;
    short xtype = type;
    int16_code(&idr, &offset, &alloc_size, &xtype);

    if (type == IndexImpl::Hash) {
      const eyedbsm::HIdx::Stats *stats = (const eyedbsm::HIdx::Stats *)xstats;
      int dummy;
      code_index_impl(stats->idx, &dummy, idr, offset, alloc_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->min_objects_per_entry);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->max_objects_per_entry);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_object_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_hash_object_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_hash_object_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_hash_object_busy_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->busy_key_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->free_key_count);
      const eyedbsm::HIdx::Stats::Entry *entry = stats->entries;
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->idx.key_count);
      for (int i = 0; i < stats->idx.key_count; i++, entry++) {
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&entry->object_count);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&entry->hash_object_count);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&entry->hash_object_size);
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&entry->hash_object_busy_size);
      }
    }
    else {
      const eyedbsm::BIdx::Stats *stats = (const eyedbsm::BIdx::Stats *)xstats;
      int dummy;
      code_index_impl(*stats->idx, &dummy, idr, offset, alloc_size);

      unsigned int x = stats->idx->getDegree();
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&x);
      x = stats->idx->getDataSize();
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&x);
      x = stats->idx->getKeySize();
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&x);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->keyOffset);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->keyType);

      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_object_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_btree_object_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->btree_node_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_btree_node_count);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->btree_key_object_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->btree_data_object_size);
      int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&stats->total_btree_object_size);
    }

    *size = offset;
    return idr;
  }



  static void
  code_atom_array(rpc_ServerData *data, void *atom_array, int count,
		  int nalloc)
  {
    Offset offset = 0;
    Data idr;
    Size alloc_size;
    int c, size = sizeof(eyedblib::int32);
    IteratorAtom *p = (IteratorAtom *)atom_array;

    for (c = 0; c < count; c++, p++)
      size += sizeof(eyedblib::int16) + p->getSize();

    if (size <= data->buff_size)
      data->status = rpc_BuffUsed;
    else
      {
	data->status = rpc_TempDataUsed;
	data->data = (unsigned char *)malloc(size);
      }

    alloc_size = size;
    data->size = size;
    idr = (Data)data->data;

    p = (IteratorAtom *)atom_array;
  
    for (c = 0; c < count; c++, p++)
      p->code(&idr, &offset, &alloc_size);

    idbFreeVect((IteratorAtom *)atom_array, IteratorAtom, nalloc);
  }

  static void
  code_value(rpc_ServerData *data, Value *value)
  {
    Offset offset = 0;
    Size alloc_size = 0;
    Data idr = 0;

    value->code(idr, offset, alloc_size);

    if (alloc_size <= data->buff_size)
      {
	data->status = rpc_BuffUsed;
	memcpy(data->data, idr, alloc_size);
	// MIND! ne faut-il pas détruire idr !?
      }
    else
      {
	data->status = rpc_TempDataUsed;
	data->data = idr;
      }

    data->size = offset;
    delete value;
  }

  //
  // Server message management
  //

  struct ServerOutOfBandData {
    ServerOutOfBandData(unsigned int _type, unsigned char *_data,
			unsigned int _size) {
      type = _type;
      size = _size;
      data = new unsigned char[size];
      memcpy(data, _data, size);
    }

    ~ServerOutOfBandData() {delete [] data;}

    unsigned int type;
    unsigned char *data;
    unsigned int size;
  };

  LinkedList server_data_list;
  static unsigned max_server_data = 1024;
  static eyedblib::Mutex server_data_mt;
  static eyedblib::Condition server_data_cnd;

  void
  setServerOutOfBandData(unsigned int type, unsigned char *data,
			 unsigned int len)
  {
    server_data_mt.lock();

    while (server_data_list.getCount() >= max_server_data)
      server_data_list.deleteObject(server_data_list.getFirstObject());
    
    server_data_list.insertObjectLast
      (new ServerOutOfBandData(type, data, len));
    server_data_mt.unlock();
    server_data_cnd.signal();
  }

  void
  setServerMessage(const char *msg)
  {
    setServerOutOfBandData(IDB_SERVER_MESSAGE, (unsigned char *)msg,
			   strlen(msg)+1);
  }

  //
  // should be executed in a parallel thread
  //

  RPCStatus
  IDB_getServerOutOfBandData(ConnHandle *, int *type, Data *ldata,
			     unsigned int *size, void *xdata)
  {
    rpc_ServerData *data = (rpc_ServerData *)xdata;

    for (;;) {
      server_data_cnd.wait();
      server_data_mt.lock();
      ServerOutOfBandData *srvdata = (ServerOutOfBandData *)server_data_list.getFirstObject();
      if (srvdata) {
	if ((*type) & srvdata->type) {
	  *type = srvdata->type;
	  if (data) {
	    data->status = rpc_TempDataUsed;
	    data->size = srvdata->size;
	    data->data = new unsigned char[data->size];
	    memcpy(data->data, srvdata->data, data->size);
	  }
	  else {
	    *size = srvdata->size;
	    *ldata = new unsigned char[*size];
	    memcpy(*ldata, srvdata->data, *size);
	  }
	  server_data_list.deleteObject(srvdata);
	  delete srvdata;
	  server_data_mt.unlock();
	  return RPCSuccess;
	}

	server_data_list.deleteObject(srvdata);
	delete srvdata;
      }
      server_data_mt.unlock();
    }
  }

  RPCStatus
  IDB_setMaxObjCount(DbHandle * dbh, int obj_cnt)
  {
    eyedbsm::Status s = eyedbsm::objectNumberSet(dbh->sedbh, (eyedbsm::Oid::NX)obj_cnt);
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_getMaxObjCount(DbHandle * dbh, int * obj_cnt)
  {
    eyedbsm::Oid::NX esm_obj_cnt;
    eyedbsm::Status s = eyedbsm::objectNumberGet(dbh->sedbh, &esm_obj_cnt);
    *obj_cnt = esm_obj_cnt;
    return rpcStatusMake_se(s);
  }

  RPCStatus
  IDB_setLogSize(DbHandle * dbh, int size)
  {
    eyedbsm::Status s = eyedbsm::shmSizeSet(dbh->sedbh, size);
    return rpcStatusMake_se(s);
  }


  RPCStatus
  IDB_getLogSize(DbHandle * dbh, int * size)
  {
    eyedbsm::Status s = eyedbsm::shmSizeGet(dbh->sedbh, size);
    return rpcStatusMake_se(s);
  }

  // moved from p.h
  eyedbsm::DbHandle *get_eyedbsm_DbHandle(DbHandle *dbh)
  {
    return dbh->sedbh;
  }
}
