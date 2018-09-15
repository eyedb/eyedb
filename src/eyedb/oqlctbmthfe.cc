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

// WARNING: <iostream> must be NOT be included before this file on MacOS X
#include <eyedblib/math.h>

#include <math.h>
#include <sstream>

#include <eyedb/eyedb.h>
#include <oql_p.h>
#include <eyedb/oqlctb.h>
#include "comp_time.h"

using std::ostringstream;
using namespace eyedb;

#define CHKCONN(CONN, MSG)					\
  OqlCtbConnection *CONN = (OqlCtbConnection *)_o;		\
								\
  if (!(CONN))							\
    return Exception::make(MSG ": invalid null object");	\
								\
  if (!(CONN)->conn)						\
    {								\
      (CONN)->conn = new Connection();				\
      (CONN)->conn->setOQLInfo(CONN);				\
    } 

#define CHK2CONN(CONN, MSG)					\
  OqlCtbConnection *CONN = (OqlCtbConnection *)_o;		\
								\
  if (!(CONN))							\
    return Exception::make(MSG ": invalid null object");	\
								\
  if (!(CONN)->conn)						\
    return Exception::make(MSG ": connection is not opened")

#define CHKDB(DB, MSG)							\
  OqlCtbDatabase *DB = (OqlCtbDatabase *)_o;				\
									\
  if (!(DB))								\
    return Exception::make(MSG ": invalid null object");		\
									\
  if (!(DB)->xdb)							\
    {									\
      const char *dbname = (DB)->getDbname().c_str();			\
      int dbid = (DB)->getDbid();					\
      if ((!dbname || !*dbname) && !dbid)				\
	return Exception::make(MSG ": database name or dbid must not set"); \
									\
      const char *dbmdb = (DB)->getDbmdb().c_str();			\
      if (!dbname || !*dbname)						\
        (DB)->xdb = new Database(dbid, (*dbmdb ? dbmdb : 0));		\
      else								\
        (DB)->xdb = new Database(dbname, (*dbmdb ? dbmdb : 0));		\
      (DB)->xdb->setOQLInfo(DB);					\
    } 

#define CHK2DB(DB, MSG)						\
  OqlCtbDatabase *DB = (OqlCtbDatabase *)_o;			\
								\
  if (!(DB))							\
    return Exception::make(MSG ": invalid null object");	\
								\
  if (!(DB)->xdb)						\
    return Exception::make(MSG ": database is not opened")

#define POSTDB(DB, S)				\
  if (!S) {					\
    (DB)->setDbname((DB)->xdb->getName());	\
    (DB)->setDbid((DB)->xdb->getDbid());	\
    (DB)->setDbmdb((DB)->xdb->getDBMDB());	\
  }

static void
not_impl(const char *msg)
{
  fprintf(stderr, "oqlctbmthfe.cc: %s not implemented\n", msg);
}

//
// string eyedb_::getConfigValue(in string) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_string_getConfigValue_eyedb__IN_string(Database *_db, FEMethod_C *_m, const char * name, char * &retarg)
{
  const char *x = eyedb::ClientConfig::getCValue(name);
  retarg = Argument::dup(x ? x : "");
  return Success;
}

//
// string eyedb_::getVersion() [oqlctbmthfe.cc]
//

Status
__method_static_OUT_string_getVersion_eyedb(Database *_db, FEMethod_C *_m, char * &retarg)
{
  retarg = Argument::dup(eyedb::getVersion());
  return Success;
}

//
// int32 eyedb_::getVersionNumber() [oqlctbmthfe.cc]
//

Status
__method_static_OUT_int32_getVersionNumber_eyedb(Database *_db, FEMethod_C *_m, eyedblib::int32 &retarg)
{
  retarg = eyedb::getVersionNumber();
  return Success;
}

//
// string eyedb_::getArchitecture() [oqlctbmthfe.cc]
//

Status
__method_static_OUT_string_getArchitecture_eyedb(Database *_db, FEMethod_C *_m, char * &retarg)
{
  retarg = Argument::dup(Architecture::getArchitecture()->getArch());
  return Success;
}

//
// string eyedb_::getCompilationTime() [oqlctbmthfe.cc]
//

Status
__method_static_OUT_string_getCompilationTime_eyedb(Database *_db, FEMethod_C *_m, char * &retarg)
{
  retarg = Argument::dup(getCompilationTime());
  return Success;
}

//
// oid object::getOid() [oqlctbfe.cc]
//

Status
__method__OUT_oid_getOid_object(Database *_db, FEMethod_C *_m, Object *_o, Oid &retarg)
{

  retarg = _o->getOid();
  return Success;
}

//
// object *object::getObject() [oqlctbfe.cc]
//

Status
__method__OUT_object_REF__getObject_object(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, eyedb::Object * &retarg)
{
  retarg = _o;
  _o->incrRefCount();
  return Success;
}

static OqlCtbDatafile *
makeDatafile(Database *db, const Datafile *dat)
{
  if (!dat)
    return 0;

  OqlCtbDatafile *rdat = new OqlCtbDatafile(db);
      
  rdat->xdatfile = const_cast<Datafile*>(dat);
  rdat->setId(dat->getId());
  rdat->setDspid(dat->getDspid());
  rdat->setFile(dat->getFile());
  rdat->setName(dat->getName());
  rdat->setMtype(dat->getMaptype() ==
		 eyedbsm::BitmapType ?
		 OqlCtbMapType::BitmapType :
		 OqlCtbMapType::LinkmapType);
  rdat->setMaxsize(dat->getMaxsize());
  rdat->setSlotsize(dat->getSlotsize());
  rdat->setDtype(dat->isPhysical() ?
		 OqlCtbDatType::PhysicalOidType :
		 OqlCtbDatType::LogicalOidType);

  return rdat;
}

static OqlCtbDataspace *
makeDataspace(Database *db, const Dataspace *dsp)
{
  if (!dsp)
    return 0;

  OqlCtbDataspace *rdsp = new OqlCtbDataspace(db);

  rdsp->xdataspace = const_cast<Dataspace*>(dsp);
  rdsp->setId(dsp->getId());
  rdsp->setName(dsp->getName());

  unsigned int datafile_cnt;
  const Datafile **datafiles = dsp->getDatafiles(datafile_cnt);
  OqlCtbDatafile **rdatafiles = new OqlCtbDatafile*[datafile_cnt];
  for (int n = 0; n < datafile_cnt; n++) {
    OqlCtbDatafile *rdat = makeDatafile(db, datafiles[n]);
    rdat->setDsp(rdsp);
    rdsp->setDatafiles(n, rdat);
  }

  return rdsp;
}

//
// dataspace* object::getDataspace() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDataspace_object(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * &rdataspace)
{
  const Dataspace *dataspace;
  Status s = _o->getDataspace(dataspace);
  if (s)
    return s;

  rdataspace = makeDataspace(_db, dataspace);
  return Success;
}

//
// void object::setDataspace(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setDataspace_object__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  return _o->setDataspace(dsp->xdataspace);
}

//
// void object::move(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_move_object__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  return _o->move(dsp->xdataspace);
}

//
// void object::store() [oqlctbfe.cc]
//

Status
__method__OUT_void_store_object(Database *_db, FEMethod_C *_m, Object *_o)
{
  return _o->store();
}

//
// void object::setDatabase(in database*) [oqlctbfe.cc]
//

Status
__method__OUT_void_setDatabase_object__IN_database_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDatabase * db)
{
  if (!db->xdb)
    return Exception::make("object::setDatabase(in database *): "
			   "database is not opened");

  return _o->setDatabase(db->xdb);
}

//
// string object::toString() [oqlctbfe.cc]
//

// disconnected 20/10/05
//#define AUTO_GARB

static Status
toStringRealize(Database *_db, Object *_o, int flags, char * &retarg)
{
  Object *o = _o;
#ifdef AUTO_GARB
  gbxAutoGarb _;
#endif

  if ((flags & OqlCtbToStringFlags::FullRecursTrace) &&
      o->getOid().isValid()) {
    Status s = _db->reloadObject(o->getOid(), o, RecMode::FullRecurs);
    if (s) return s;
  }
  
  Status status;
  std::string str = o->toString(flags, (flags & OqlCtbToStringFlags::FullRecursTrace ? RecMode::FullRecurs : RecMode::NoRecurs), &status);
  retarg = Argument::dup(str.c_str());

#ifndef AUTO_GARB
  if (o != _o) {
    //printf("NOT releasing %p [refcnt=%d] %p [refcnt=%d]\n", o, o->getRefCount(),
    //_o, _o->getRefCount());
    o->release();
  }
#endif

  return status;
}

Status
__method__OUT_string_toString_object(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  return toStringRealize(_db, _o, 0, retarg);
}

//
// string object::toString(in int flags) [oqlctbfe.cc]
//

Status
__method__OUT_string_toString_object__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 flags, char * &retarg)
{
  return toStringRealize(_db, _o, flags, retarg);
}

//
// database* object::getDatabase() [oqlctbfe.cc]
//

Status
__method__OUT_database_REF__getDatabase_object(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDatabase * &retarg)
{
  retarg = (OqlCtbDatabase *)(_o->getDatabase()->getOQLInfo());

  if (retarg)
    {
      Database *db = retarg->xdb;
      retarg = new OqlCtbDatabase(*retarg);
      retarg->xdb = db;
    }
  else
    {
      const Class *clsdb = _db->getSchema()->getClass("database");
      if (clsdb)
	{
	  Database *xdb = _o->getDatabase();
	  retarg = new OqlCtbDatabase(xdb);
	  retarg->setDbname(xdb->getName());
	  retarg->setDbid(xdb->getDbid());
	  retarg->setDbmdb(xdb->getDBMDB());
	  retarg->xdb = xdb;
	  xdb->setOQLInfo(retarg);
	}
    }


  return Success;
}

//
// object* object::clone() [oqlctbmthfe.cc]
//

Status
__method__OUT_object_REF__clone_object(Database *_db, FEMethod_C *_m, Object *_o, Object * &retarg)
{
  retarg = _o->clone();
  return Success;
}

//
// int64 object::getCTime() [oqlctbfe.cc]
//

Status
__method__OUT_int64_getCTime_object(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int64 &retarg)
{
  retarg = _o->getCTime();
  return Success;
}

//
// int64 object::getMTime() [oqlctbfe.cc]
//

Status
__method__OUT_int64_getMTime_object(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int64 &retarg)
{
  retarg = _o->getMTime();
  return Success;
}

//
// string object::getStringCTime() [oqlctbfe.cc]
//

Status
__method__OUT_string_getStringCTime_object(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  retarg = Argument::dup(_o->getStringCTime());
  return Success;
}

//
// string object::getStringMTime() [oqlctbfe.cc]
//

Status
__method__OUT_string_getStringMTime_object(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  retarg = Argument::dup(_o->getStringMTime());
  return Success;
}

//
// Bool object::isRemoved() [oqlctbfe.cc]
//

Status
__method__OUT_bool_isRemoved_object(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  retarg = (Bool)_o->isRemoved();
  return Success;
}

//
// int32 object::isModify() [oqlctbfe.cc]
//

Status
__method__OUT_bool_isModify_object(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  retarg = (Bool)_o->isModify();
  return Success;
}

//
// void object::setLock(in lock_mode) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setLock_object__IN_lock_mode(Database *_db, FEMethod_C *_m, Object *_o, const OqlCtbLockMode::Type mode)
{
  return _o->setLock((LockMode)mode);
}

//
// void object::setLock(in lock_mode, out int32) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setLock_object__IN_lock_mode__OUT_int32(Database *_db, FEMethod_C *_m, Object *_o, const OqlCtbLockMode::Type mode, eyedblib::int32 &rmode)
{
  LockMode _rmode;
  Status s = _o->setLock((LockMode)mode, _rmode);
  rmode = _rmode;
  return s;
}

//
// void object::getLock(out lock_mode) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_getLock_object__OUT_lock_mode(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbLockMode::Type &rmode)
{
  LockMode _rmode;
  Status s = _o->getLock(_rmode);
  rmode = (OqlCtbLockMode::Type)_rmode;
  return s;
}

//
// void Connection_::open() [oqlctbfe.cc]
//

Status
__method__OUT_void_open_connection(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHKCONN(conn, "connection::open()");
  return conn->conn->open();
}

//
// void Connection_::open(in string, in string) [oqlctbfe.cc]
//

Status
__method__OUT_void_open_connection__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * host, const char * port)
{
  CHKCONN(conn, "connection::open(in string host, in string port)");
  return conn->conn->open(host, port);
}

//
// void Connection_::close() [oqlctbfe.cc]
//

Status
__method__OUT_void_close_connection(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2CONN(conn, "connection::close()");
  return conn->conn->close();
}

//
// void Database_::open(in connection*, in int32) [oqlctbfe.cc]
//

Status
__method__OUT_void_open_database__IN_connection_REF___IN_int32(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbConnection * conn, const eyedblib::int32 mode)
{
  CHKDB(db, "database::open(in connection conn, in int mode)");
  Status s = db->xdb->open(((OqlCtbConnection *)conn)->conn,
			   (Database::OpenFlag)(mode|_DBOpenLocal));
  POSTDB(db, s);
  return s;
}

//
// void Database_::open(in connection*, in int32, in string, in string) [oqlctbfe.cc]
//

Status
__method__OUT_void_open_database__IN_connection_REF___IN_int32__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbConnection * conn, const eyedblib::int32 mode, const char * userauth, const char * passwdauth)
{
  CHKDB(db, "database::open(in connection *, in int mode, in string user, in string passwd)");
  Status s = db->xdb->open(((OqlCtbConnection *)conn)->conn,
			   (Database::OpenFlag)(mode|_DBOpenLocal), userauth, passwdauth);
  POSTDB(db, s);
  return s;
}

//
// void Database_::open(in int32) [oqlctbfe.cc]
//

Status
__method__OUT_void_open_database__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 mode)
{
  CHKDB(db, "database::open(in int mode)");
  Status s = db->xdb->open(_db->getConnection(), (Database::OpenFlag)(mode|_DBOpenLocal));

  POSTDB(db, s);
  return s;
}

//
// void Database_::open(in int32, in string, in string) [oqlctbfe.cc]
//

Status
__method__OUT_void_open_database__IN_int32__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 mode, const char * userauth, const char * passwdauth)
{
  CHKDB(db, "database::open(in int mode, in string user, in string passwd)");
  Status s = db->xdb->open(_db->getConnection(), (Database::OpenFlag)(mode|_DBOpenLocal),
			   userauth, passwdauth);
  POSTDB(db, s);
  return s;
}

//
// void Database_::close() [oqlctbfe.cc]
//

Status
__method__OUT_void_close_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2DB(db, "database::close()");
  return db->xdb->close();
}

//
// void Database_::create() [oqlctbmthfe.cc]
//

Status
__method__OUT_void_create_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHKDB(db, "database::create()");
  return db->xdb->create(_db->getConnection());
}

//
// void Database_::create(in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_create_database__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * userauth, const char * passwdauth)
{
  CHKDB(db, "database::create(userauth, passwduth)");
  return db->xdb->create(_db->getConnection(), userauth, passwdauth);
}

//
// void Database_::destroy() [oqlctbmthfe.cc]
//

Status
__method__OUT_void_destroy_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHKDB(db, "database::destroy()");
  Status s =  db->xdb->remove(_db->getConnection());
  return s;
}

//
// void Database_::destroy(in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_destroy_database__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * userauth, const char * passwdauth)
{
  CHKDB(db, "database::destroy(userauth, passwdauth)");
  return db->xdb->remove(_db->getConnection(), userauth, passwdauth);
}

//
// void Database_::rename(in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_rename_database__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * newname)
{
  CHKDB(db, "database::rename(newname)");
  return db->xdb->rename(_db->getConnection(), newname);
}

//
// void Database_::rename(in string, in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_rename_database__IN_string__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * newname, const char * userauth, const char * passwdauth)
{
  CHKDB(db, "database::rename(newname, userauth, passwdauth)");
  return db->xdb->rename(_db->getConnection(), newname, userauth, passwdauth);
}

//
// int32 Database_::isAutoPersistMode() [oqlctbfe.cc]
//

Status
__method_static_OUT_bool_isAutoPersistMode_database(Database *_db, FEMethod_C *_m, Bool &retarg)
{
  retarg = (Bool)oqml_auto_persist;
  return Success;
}

//
// void Database_::setAutoPersistMode(in int32) [oqlctbfe.cc]
//

Status
__method_static_OUT_void_setAutoPersistMode_database__IN_bool(Database *_db, FEMethod_C *_m, const Bool arg1)
{
  oqml_auto_persist = OQMLBOOL(arg1);
  return Success;
}

//
// bool Database_::isDefaultDatabase() [oqlctbfe.cc]
//

Status
__method__OUT_bool_isDefaultDatabase_database(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  CHK2DB(db, "database::isDefaultDatabase()");
  retarg = (oqml_default_db == db ? True : False);
  return Success;
}

//
// void Database_::setDefaultDatabase(in database*) [oqlctbfe.cc]
//

Status
__method__OUT_void_setDefaultDatabase_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2DB(db, "database::setDefaultDatabase()");
  oqml_default_db = db;
  return Success;
}

//
// connection* Database_::getConnection() [oqlctbfe.cc]
//

Status
__method__OUT_connection_REF__getConnection_database(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbConnection * &retarg)
{
  retarg = (OqlCtbConnection *)(_o->getDatabase()->getConnection()->getOQLInfo());
  if (!retarg)
    {
      Connection *conn  =_o->getDatabase()->getConnection();
      retarg = new OqlCtbConnection();
      retarg->conn = conn;
      conn->setOQLInfo(retarg);
    }

  return Success;
}

//
// int32 Database_::getOpenMode() [oqlctbfe.cc]
//

Status
__method__OUT_int32_getOpenMode_database(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  CHK2DB(db, "database::getOpenMode()");
  retarg = db->xdb->getOpenFlag();
  return Success;
}

//
// int32 Database_::getCommitOnClose() [oqlctbfe.cc]
//

Status
__method__OUT_bool_getCommitOnClose_database(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  CHK2DB(db, "database::getCommitOnClose()");
  retarg = (Bool)db->xdb->getCommitOnClose();
  return Success;
}

//
// void Database_::setCommitOnClose(in int32) [oqlctbfe.cc]
//

Status
__method__OUT_void_setCommitOnClose_database__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const Bool arg1)
{
  CHK2DB(db, "database::setCommitOnClose(in int)");
  db->xdb->setCommitOnClose((Bool)arg1);
  return Success;
}

//
// int32 Database_::getVersionNumber() [oqlctbfe.cc]
//

Status
__method__OUT_int32_getVersionNumber_database(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  CHK2DB(db, "database::getVersionNumber()");
  retarg = db->xdb->getVersionNumber();
  return Success;
}

//
// string Database_::getVersion() [oqlctbfe.cc]
//

Status
__method__OUT_string_getVersion_database(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  CHK2DB(db, "database::getVersion()");
  retarg = Argument::dup(db->xdb->getVersion());
  return Success;
}

//
// void Database_::removeObject(in oid) [oqlctbfe.cc]
//

Status
__method__OUT_void_removeObject_database__IN_oid(Database *_db, FEMethod_C *_m, Object *_o, const Oid arg1)
{
  CHK2DB(db, "database::removeObject(in oid)");
  return db->xdb->removeObject(arg1);
}

//
// void Database_::uncacheObject(in object*) [oqlctbfe.cc]
//

Status
__method__OUT_void_uncacheObject_database__IN_object_REF_(Database *_db, FEMethod_C *_m, Object *_o, Object * arg1)
{
  CHK2DB(db, "database::uncacheObject(in object *)");
  db->xdb->uncacheObject(arg1);
  return Success;
}

//
// void Database_::uncacheObject(in oid) [oqlctbfe.cc]
//

Status
__method__OUT_void_uncacheObject_database__IN_oid(Database *_db, FEMethod_C *_m, Object *_o, const Oid arg1)
{
  CHK2DB(db, "database::uncacheObject(in oid)");
  db->xdb->uncacheObject(arg1);
  return Success;
}

//
// void Database_::transactionBegin(in int32, in int32, in int32, in int32, in int32, in int32) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_transactionBegin_database__IN_int32__IN_int32__IN_int32__IN_int32__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 trsmode, const eyedblib::int32 lockmode, const eyedblib::int32 recovmode, const eyedblib::int32 _magorder, const eyedblib::int32 ratioalrt, const eyedblib::int32 wait_timeout)
{
  CHK2DB(db, "database::transactionBegin()");

  TransactionParams params;
  params.trsmode = (TransactionMode)trsmode;
  params.lockmode = (TransactionLockMode)lockmode;
  params.recovmode = (RecoveryMode)recovmode;
  params.magorder = _magorder;
  params.ratioalrt = ratioalrt;
  params.wait_timeout = wait_timeout;

  return db->xdb->transactionBegin(params);
}

//
// void Database_::transactionBegin() [oqlctbmthfe.cc]
//

Status
__method__OUT_void_transactionBegin_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2DB(db, "database::transactionBegin()");
  return db->xdb->transactionBegin();
}

//
// void Database_::transactionCommit() [oqlctbfe.cc]
//

Status
__method__OUT_void_transactionCommit_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2DB(db, "database::transactionCommit()");
  return db->xdb->transactionCommit();
}

//
// void Database_::transactionAbort() [oqlctbfe.cc]
//

Status
__method__OUT_void_transactionAbort_database(Database *_db, FEMethod_C *_m, Object *_o)
{
  CHK2DB(db, "database::transactionAbort()");
  return db->xdb->transactionAbort();
}
//
// Bool Database_::isInTransaction() [oqlctbmthfe.cc]
//

Status
__method__OUT_bool_isInTransaction_database(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  CHK2DB(db, "database::isInTransaction()");
  retarg = (Bool)db->xdb->isInTransaction();
  return Success;
}

//
// datafile*[] Database_::getDatafiles() [oqlctbmthfe.cc]
//

Status
__method__OUT_datafile_REF__ARRAY__getDatafiles_database(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDatafile * * &rdatafiles, int &rdatafile_cnt)
{
  CHK2DB(db, "database::getDatafiles()");

  const Datafile **datafiles;
  unsigned int cnt;
  Status s = db->xdb->getDatafiles(datafiles, cnt);
  if (s)
    return s;

  rdatafile_cnt = cnt;
  rdatafiles = new OqlCtbDatafile*[rdatafile_cnt];
  for (int n = 0; n < rdatafile_cnt; n++)
    rdatafiles[n] = makeDatafile(db->xdb, datafiles[n]);

  return Success;
}

//
// datafile* Database_::getDatafile(in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_datafile_REF__getDatafile_database__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * name, OqlCtbDatafile * &rdatafile)
{
  CHK2DB(db, "database::getDatafile(in string)");

  const Datafile *datafile;
  Status s = db->xdb->getDatafile(name, datafile);
  if (s)
    return s;

  rdatafile = makeDatafile(db->xdb, datafile);

  return Success;
}

//
// datafile* Database_::getDatafile(in int16) [oqlctbmthfe.cc]
//

Status
__method__OUT_datafile_REF__getDatafile_database__IN_int16(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int16 id, OqlCtbDatafile * &rdatafile)
{
  CHK2DB(db, "database::getDatafile(in int16)");

  const Datafile *datafile;
  Status s = db->xdb->getDatafile(id, datafile);
  if (s)
    return s;

  rdatafile = makeDatafile(db->xdb, datafile);

  return Success;
}

//
// dataspace*[] Database_::getDataspaces() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__ARRAY__getDataspaces_database(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * * &rdataspaces, int &rdataspaces_cnt)
{

  CHK2DB(db, "dataspace*[] database::getDataspaces()");
  const Dataspace **dataspaces;
  unsigned int cnt;
  Status s = db->xdb->getDataspaces(dataspaces, cnt);
  if (s)
    return s;

  rdataspaces_cnt = cnt;
  rdataspaces = new OqlCtbDataspace*[cnt];

  for (int n = 0; n < cnt; n++)
    rdataspaces[n] = makeDataspace(db->xdb, dataspaces[n]);

  return Success;
}

//
// dataspace* Database_::getDataspace(in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDataspace_database__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * name, OqlCtbDataspace * &rdsp)
{
  CHK2DB(db, "dataspace* database::getDataspace(in string)");
  const Dataspace *dataspace;
  Status s = db->xdb->getDataspace(name, dataspace);
  if (s || !dataspace)
    return s;
  rdsp = makeDataspace(db->xdb, dataspace);
  return Success;
}

//
// dataspace* Database_::getDataspace(in int16) [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDataspace_database__IN_int16(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int16 id, OqlCtbDataspace * &rdsp)
{
  CHK2DB(db, "dataspace* database::getDataspace(in int16)");
  const Dataspace *dataspace;
  Status s = db->xdb->getDataspace(id, dataspace);
  if (s || !dataspace)
    return s;
  rdsp = makeDataspace(db->xdb, dataspace);
  return Success;
}

//
// dataspace* Database_::getDefaultDataspace() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDefaultDataspace_database(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * &rdataspace)
{
  CHK2DB(db, "dataspace* database::getDefaultDataspace()");
  const Dataspace *dataspace;
  Status s = db->xdb->getDefaultDataspace(dataspace);
  if (s)
    return s;

  rdataspace = makeDataspace(db->xdb, dataspace);
  return Success;
}

//
// void Database_::setDefaultDataspace(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setDefaultDataspace_database__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dataspace)
{
  CHK2DB(db, "void database::setDefaultDataspace(in dataspace *)");
  return db->xdb->setDefaultDataspace(dataspace->xdataspace);
}

//
// void Database_::moveObjects(in oid[], in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_moveObjects_database__IN_oid_ARRAY___IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, const Oid *oids, int oid_cnt, OqlCtbDataspace * dsp)
{
  OidArray oid_arr(oids, oid_cnt);

  return _db->moveObjects(oid_arr, dsp->xdataspace);
}

//
// void Database_::setObjectLock(in oid, in lock_mode) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setObjectLock_database__IN_oid__IN_lock_mode(Database *_db, FEMethod_C *_m, Object *_o, const Oid _oid, const OqlCtbLockMode::Type mode)
{
  CHK2DB(db, "database::setObjectLock(oid, mode)");
  return db->xdb->setObjectLock(_oid, (LockMode)mode);
}

//
// void Database_::setObjectLock(in oid, in lock_mode, out lock_mode) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setObjectLock_database__IN_oid__IN_lock_mode__OUT_lock_mode(Database *_db, FEMethod_C *_m, Object *_o, const Oid _oid, const OqlCtbLockMode::Type mode, OqlCtbLockMode::Type &rmode)
{
  CHK2DB(db, "database::setObjectLock(oid, mode, rmode)");
  LockMode _rmode;
  Status s = db->xdb->setObjectLock(_oid, (LockMode)mode, _rmode);
  rmode = (OqlCtbLockMode::Type)_rmode;
  return s;
}

//
// void Database_::getObjectLock(in oid, out lock_mode) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_getObjectLock_database__IN_oid__OUT_lock_mode(Database *_db, FEMethod_C *_m, Object *_o, const Oid _oid, OqlCtbLockMode::Type &rmode)
{
  CHK2DB(db, "database::getObjectLock(oid, rmode)");
  LockMode _rmode;
  Status s = db->xdb->getObjectLock(_oid, _rmode);
  rmode = (OqlCtbLockMode::Type)_rmode;
  return s;
}

//
// void Database_::updateSchema(in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_updateSchema_database__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * odlfile, const char * package)
{
  CHK2DB(db, "database::updateSchema(odlfile, package)");
  return db->xdb->updateSchema(odlfile, package);
}

//
// void Database_::updateSchema(in string, in string, in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_updateSchema_database__IN_string__IN_string__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * odlfile, const char * package, const char * schname, const char * db_prefix)
{
  CHK2DB(db, "database::updateSchema(odlfile, package, schname, db_prefix)");
  return db->xdb->updateSchema(odlfile, package, schname, db_prefix);
}

//
// void Database_::updateSchema(in string, in string, in string, in string, in string, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_updateSchema_database__IN_string__IN_string__IN_string__IN_string__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * odlfile, const char * package, const char * schname, const char * db_prefix, const char * cpp_cmd, const char * cpp_flags)
{
  CHK2DB(db, "database::updateSchema(odlfile, package, schname, db_prefix, cpp_cmd, cpp_flags)");
  return db->xdb->updateSchema(odlfile, package, schname, db_prefix,
			       0, cpp_cmd, cpp_flags);
}

//
// float OqlCtbMath::acos(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_acos_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = acos(arg1);
  return Success;
}

//
// float OqlCtbMath::asin(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_asin_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = asin(arg1);
  return Success;
}

//
// float OqlCtbMath::atan(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_atan_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = atan(arg1);
  return Success;
}

//
// float OqlCtbMath::atan2(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_atan2_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = atan2(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::cos(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_cos_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = cos(arg1);
  return Success;
}

//
// float OqlCtbMath::sin(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_sin_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = sin(arg1);
  return Success;
}

//
// float OqlCtbMath::tan(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_tan_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = tan(arg1);
  return Success;
}

//
// float OqlCtbMath::cosh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_cosh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = cosh(arg1);
  return Success;
}

//
// float OqlCtbMath::sinh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_sinh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = sinh(arg1);
  return Success;
}

//
// float OqlCtbMath::tanh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_tanh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = tanh(arg1);
  return Success;
}

//
// float OqlCtbMath::exp(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_exp_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = exp(arg1);
  return Success;
}

//
// float OqlCtbMath::log(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_log_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = log(arg1);
  return Success;
}

//
// float OqlCtbMath::log10(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_log10_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = log10(arg1);
  return Success;
}

//
// float OqlCtbMath::pow(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_pow_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = pow(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::sqrt(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_sqrt_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = sqrt(arg1);
  return Success;
}

//
// float OqlCtbMath::ceil(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_ceil_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = ceil(arg1);
  return Success;
}

//
// float OqlCtbMath::fabs(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_fabs_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = fabs(arg1);
  return Success;
}

//
// float OqlCtbMath::floor(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_floor_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = floor(arg1);
  return Success;
}

//
// float OqlCtbMath::fmod(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_fmod_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = fmod(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::erf(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_erf_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = erf(arg1);
  return Success;
}

//
// float OqlCtbMath::erfc(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_erfc_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = erfc(arg1);
  return Success;
}

//
// float OqlCtbMath::gamma(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_gamma_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = gamma(arg1);
  return Success;
}

//
// float OqlCtbMath::hypot(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_hypot_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = hypot(arg1, arg2);
  return Success;
}

//
// int32 OqlCtbMath::isnan(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_int32_isnan_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, eyedblib::int32 &retarg)
{
  retarg = eyedblib::eyedb_isnan(arg1);
  return Success;
}

//
// float OqlCtbMath::j0(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_j0_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = j0(arg1);
  return Success;
}

//
// float OqlCtbMath::j1(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_j1_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = j1(arg1);
  return Success;
}

//
// float OqlCtbMath::jn(in int32, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_jn_math__IN_int32__IN_float(Database *_db, FEMethod_C *_m, const eyedblib::int32 arg1, const double arg2, double &retarg)
{
  retarg = jn(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::lgamma(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_lgamma_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = lgamma(arg1);
  return Success;
}

//
// float OqlCtbMath::y0(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_y0_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = y0(arg1);
  return Success;
}

//
// float OqlCtbMath::y1(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_y1_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = y1(arg1);
  return Success;
}

//
// float OqlCtbMath::yn(in int32, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_yn_math__IN_int32__IN_float(Database *_db, FEMethod_C *_m, const eyedblib::int32 arg1, const double arg2, double &retarg)
{
  retarg = yn(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::acosh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_acosh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = acosh(arg1);
  return Success;
}

//
// float OqlCtbMath::asinh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_asinh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = asinh(arg1);
  return Success;
}

//
// float OqlCtbMath::atanh(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_atanh_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = atanh(arg1);
  return Success;
}

//
// float OqlCtbMath::cbrt(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_cbrt_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = cbrt(arg1);
  return Success;
}

//
// float OqlCtbMath::logb(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_logb_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = logb(arg1);
  return Success;
}

//
// float OqlCtbMath::nextafter(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_nextafter_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = nextafter(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::remainder(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_remainder_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = remainder(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::scalb(in float, in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_scalb_math__IN_float__IN_float(Database *_db, FEMethod_C *_m, const double arg1, const double arg2, double &retarg)
{
  retarg = scalb(arg1, arg2);
  return Success;
}

//
// float OqlCtbMath::expm1(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_expm1_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = expm1(arg1);
  return Success;
}

//
// int32 OqlCtbMath::ilogb(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_int32_ilogb_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, eyedblib::int32 &retarg)
{
  retarg = ilogb(arg1);
  return Success;
}

//
// float OqlCtbMath::log1p(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_log1p_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = log1p(arg1);
  return Success;
}

//
// float OqlCtbMath::rint(in float) [oqlctbmthfe.cc]
//

Status
__method_static_OUT_float_rint_math__IN_float(Database *_db, FEMethod_C *_m, const double arg1, double &retarg)
{
  retarg = rint(arg1);
  return Success;
}

//
// dataspace* class::getDefaultInstanceDataspace() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDefaultInstanceDataspace_class(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * &rdsp)
{
  
  Class *cls = (Class *)_o;
  const Dataspace *dataspace;
  Status s = cls->getDefaultInstanceDataspace(dataspace);
  if (s)
    return s;

  rdsp = makeDataspace(_db, dataspace);
  return Success;
}

//
// void class::setDefaultInstanceDataspace(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setDefaultInstanceDataspace_class__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Class *cls = (Class *)_o;

  Status s = cls->setDefaultInstanceDataspace(dsp->xdataspace);
  if (s)
    return s;

  return Success;
}

//
// void class::moveInstances(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_moveInstances_class__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Class *cls = (Class *)_o;

  Status s = cls->moveInstances(dsp->xdataspace);
  if (s)
    return s;

  return Success;
}

//
// int32 index::getCount() [oqlctbmthfe.cc]
//

Status
__method__OUT_int32_getCount_index(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  Index *idx = (Index *)_o;
  Status s = Attribute::openMultiIndexRealize(_db, idx);
  if (s) return s;
  retarg = idx->idx->getCount();
  return Success;
}

static Status
indexGetStats(Database *_db, Object *_o, Bool full, char * &retarg)
{
  std::string stats;
  Index *idx = (Index *)_o;
  Status s = idx->getStats(stats, True, full);
  if (s) return s;

  retarg = strdup((std::string("Index: '") + idx->getAttrpath() + "';\n" +
		   "Oid: " + idx->getOid().toString() + ";\n" + stats).c_str());
  return Success;
}

//
// string index::getStats() [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getStats_index(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  return indexGetStats(_db, _o, False, retarg);
}

//
// string index::getStats(in bool) [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getStats_index__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const Bool full, char * &retarg)
{
  return indexGetStats(_db, _o, full, retarg);
}

//
// string index::getImplementation(in bool) [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getImplementation_index__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const Bool local, char * &retarg)
{
  Index *idx = (Index *)_o;
  IndexImpl *idximpl;
  Status s = idx->getImplementation(idximpl, local);
  if (s) return s;
  retarg = strdup(idximpl->toString().c_str());
  if (idximpl)
    idximpl->release();
  return Success;
}

//
// dataspace* index::getDefaultDataspace() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDefaultDataspace_index(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * &rdsp)
{
  Index *idx = (Index *)_o;
  const Dataspace *dataspace;
  Status s = idx->getDefaultDataspace(dataspace);
  if (s)
    return s;
      
  rdsp = makeDataspace(_db, dataspace);
  return Success;
}

//
// void index::setDefaultDataspace(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setDefaultDataspace_index__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Index *idx = (Index *)_o;
  return idx->setDefaultDataspace(dsp->xdataspace);
}

//
// void index::move(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_move_index__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Index *idx = (Index *)_o;
  return idx->move(dsp->xdataspace);
}

//
// string index::simulate(in int32, in string, in bool) [oqlctbmthfe.cc]
//

static Status
getIndexImpl(Database *db, int idxtype, const char *hints,
	     IndexImpl *&idximpl)
{
  IndexImpl::Type type;
  if (idxtype == HashIndexType)
    type = IndexImpl::Hash;
  else if (idxtype == BTreeIndexType)
    type = IndexImpl::BTree;
  else
    return Exception::make(IDB_ERROR, "invalid index type: expected "
			   "HASH_INDEX_TYPE (%d) or BTREE_INDEX_TYPE (%d)",
			   HashIndexType, BTreeIndexType);
  return IndexImpl::make(db, type, hints, idximpl);
}

Status
__method__OUT_string_simulate_index__IN_int32__IN_string__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 idxtype, const char * hints, const Bool full, char * &retarg)
{
  Status s;
  Index *idx = (Index *)_o;

  IndexImpl *idximpl;
  s = getIndexImpl(_db, idxtype, hints, idximpl);
  if (s) return s;
  std::string stats;
  s = idx->simulate(*idximpl, stats, True, full);
  if (s) return s;
  retarg = strdup(stats.c_str());
  return Success;
}

//
// void index::reimplement(in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_reimplement_index__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * hints)
{
  Status s;
  Index *idx = (Index *)_o;

  IndexImpl *idximpl;
  s = getIndexImpl(_db, idx->asHashIndex() ? HashIndexType :
		   BTreeIndexType, hints, idximpl);
  if (s) return s;

  idx->setImplementation(idximpl);
  if (idximpl)
    idximpl->release();
  return idx->store();
}

//
// oid index::reimplement(in int32, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_oid_reimplement_index__IN_int32__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 idxtype, const char * hints, Oid &retarg)
{
  Status s;
  Index *idx = (Index *)_o;

  IndexImpl *idximpl;
  s = getIndexImpl(_db, idxtype, hints, idximpl);
  if (s) return s;
  Index *idxn;
  s = idx->reimplement(*idximpl, idxn);
  if (s) return s;
  retarg = idxn->getOid();
  return Success;
}

//
// int32 collection::getCount() [oqlctbmthfe.cc]
//

Status
__method__OUT_int32_getCount_collection(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  retarg = dynamic_cast<Collection *>(_o)->getCount();
  return Success;
}

static Status
collectionGetImplStats(Database *_db, Object *_o, Bool full, char * &retarg)
{
  Collection *coll = (Collection *)_o;
  if (coll->asCollArray()) {
    std::string stats1, stats2;
    Status s = coll->asCollArray()->getImplStats(stats1, stats2, True, full);
    if (s) return s;

    retarg = strdup
      ((std::string("Collection: '") +  coll->getName() + "';\n" +
	"Oid: " + coll->getOidC().toString() + ";\n" +
	"First Index Statistics:\n" + stats1 + "\n" +
	"Second Index Statistics:\n" + stats2).c_str());
    
    return Success;
  }

  std::string stats;
  Status s = coll->getImplStats(stats, True, full);
  if (s) return s;

  retarg = strdup
    ((std::string("Collection: '") +  coll->getName() + "';\n" +
      "Oid: " + coll->getOidC().toString() + ";\n" + stats).c_str());

  return Success;
}

//
// string collection::getImplStats() [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getImplStats_collection(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  return collectionGetImplStats(_db, _o, False, retarg);
}

//
// bool collection::isLiteral() [oqlctbmthfe.cc]
//

Status
__method__OUT_bool_isLiteral_collection(Database *_db, FEMethod_C *_m, Object *_o, Bool &retarg)
{
  retarg = dynamic_cast<Collection *>(_o)->isLiteral();
  return Success;
}

//
// bool collection::isPureLiteral() [oqlctbmthfe.cc]
//

Status
__method__OUT_bool_isPureLiteral_collection(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, eyedb::Bool &retarg)
{

  retarg = dynamic_cast<Collection *>(_o)->isPureLiteral();
  return eyedb::Success;
}

//
// bool collection::isLiteralObject() [oqlctbmthfe.cc]
//

Status
__method__OUT_bool_isLiteralObject_collection(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, eyedb::Bool &retarg)
{
  retarg = dynamic_cast<Collection *>(_o)->isLiteralObject();
  return eyedb::Success;
}

//
// void collection::setLiteralObject() [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setLiteralObject_collection(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o)
{
  return dynamic_cast<Collection *>(_o)->setLiteralObject(true);
}

//
// oid collection::getLiteralOid() [oqlctbmthfe.cc]
//

Status
__method__OUT_oid_getLiteralOid_collection(Database *_db, FEMethod_C *_m, Object *_o, Oid &retarg)
{
  if (dynamic_cast<Collection *>(_o)->isLiteral())
    retarg = dynamic_cast<Collection *>(_o)->getOidC();
  return Success;
}

//
// string collection::getImplStats(in bool) [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getImplStats_collection__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const Bool full, char * &retarg)
{
  return collectionGetImplStats(_db, _o, full, retarg);
}

//
// dataspace* collection::getDefaultDataspace() [oqlctbmthfe.cc]
//

Status
__method__OUT_dataspace_REF__getDefaultDataspace_collection(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * &rdsp)
{
  Collection *coll = (Collection *)_o;
  const Dataspace *dataspace;
  Status s = coll->getDefaultDataspace(dataspace);
  if (s)
    return s;

  rdsp = makeDataspace(_db, dataspace);
  return Success;
}

//
// void collection::setDefaultDataspace(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setDefaultDataspace_collection__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Collection *coll = (Collection *)_o;

  Status s = coll->setDefaultDataspace(dsp->xdataspace);
  if (s)
    return s;

  return Success;
}

//
// void collection::moveElements(in dataspace*) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_moveElements_collection__IN_dataspace_REF_(Database *_db, FEMethod_C *_m, Object *_o, OqlCtbDataspace * dsp)
{
  Collection *coll = (Collection *)_o;

  Status s = coll->moveElements(dsp->xdataspace);
  if (s)
    return s;

  return Success;
}


//
// string collection::getImplementation(in bool) [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getImplementation_collection__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const Bool local, char * &retarg)
{
  Collection *coll = (Collection *)_o;
  CollImpl *collimpl;
  Status s = coll->getImplementation(collimpl, local);
  if (s) return s;
  if (collimpl->getIndexImpl()) {
    retarg = strdup(collimpl->getIndexImpl()->toString().c_str());
  }
  else if (collimpl->getType() == CollImpl::NoIndex) {
    retarg = strdup("noindex");
  }
  else {
    retarg = strdup("<unknown>");
  }
  return Success;
}

//
// string collection::simulate(in int32, in string, in bool) [oqlctbmthfe.cc]
//

Status
__method__OUT_string_simulate_collection__IN_int32__IN_string__IN_bool(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 idxtype, const char * hints, const Bool full, char * &retarg)
{
  Status s;
  Collection *coll = (Collection *)_o;
  IndexImpl *idximpl = 0;

  s = getIndexImpl(_db, idxtype, hints, idximpl);
  if (s) return s;

  std::string stats;
  CollImpl *collimpl = new CollImpl((CollImpl::Type)idxtype, idximpl);
  s = coll->simulate(*collimpl, stats, True, full);
  if (s) return s;
  collimpl->release();
  idximpl->release();
  retarg = strdup(stats.c_str());
  return Success;
}

//
// void collection::reimplement(in int32, in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_reimplement_collection__IN_int32__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 idxtype, const char * hints)
{
  Status s;
  Collection *coll = (Collection *)_o;

  IndexImpl *idximpl = 0;
  if (idxtype == CollImpl::HashIndex || idxtype == CollImpl::BTreeIndex) {
    s = getIndexImpl(_db, idxtype, hints, idximpl);
    if (s) return s;
  }
      
  CollImpl *collimpl = new CollImpl((CollImpl::Type)idxtype, idximpl);
  coll->setImplementation(collimpl);

  if (idximpl) {
    idximpl->release();
  }

  collimpl->release();

  if (coll->isLiteral()) {
    return coll->getMasterObject(true)->store();
  }
  return coll->store();
}

//
// string collection::getName() [oqlctbmthfe.cc]
//

Status
__method__OUT_string_getName_collection(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, char * &retarg)
{
  retarg = Argument::dup(dynamic_cast<Collection *>(_o)->getName());
  return eyedb::Success;
}

//
// void collection::setName(in string) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setName_collection__IN_string(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const char * name)
{
  Collection* coll = dynamic_cast<Collection *>(_o);
  coll->setName(name);
  return coll->store();
}

//
// bool collection::isIn(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_bool_isIn_collection__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj, eyedb::Bool &retarg)
{
  return dynamic_cast<Collection *>(_o)->isIn(obj, retarg);
}

// not implemented methods

//
// oid[] collection::getElements(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_oid_ARRAY__getElements_collection__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj, Oid * &retarg, int &retarg_cnt)
{
  return Exception::make(IDB_ERROR, "oid[] collection::getElements(in oid) not yet implemented");
}

//
// oid array::retrieveAt(in int32) [oqlctbmthfe.cc]
//

Status
__method__OUT_oid_retrieveAt_array__IN_int32(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const eyedblib::int32 arg1, Oid &retarg)
{
  return Exception::make(IDB_ERROR, "oid array::retrieveAt(in int32) not yet implemented");
}


//
// void bag::addTo(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_addTo_bag__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void bag::addTo(in oid) not yet implemented");
}

//
// void bag::suppress(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_suppress_bag__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void bag::suppress(in oid) not yet implemented");
}

//
// void array::setInAt(in int32, in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_setInAt_array__IN_int32__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const eyedblib::int32 pos, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void array::setInAt(in int32, in oid) not yet implemented");
}

//
// void array::append(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_append_array__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void array::append(in oid) not yet implemented");
}

//
// void array::suppressAt(in int32) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_suppressAt_array__IN_int32(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const eyedblib::int32 arg1)
{
  return Exception::make(IDB_ERROR, "void array::suppressAt(in int32) not yet implemented");
}

//
// void set::addTo(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_addTo_set__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void set::addTo(in oid) not yet implemented");
}

//
// void set::suppress(in oid) [oqlctbmthfe.cc]
//

Status
__method__OUT_void_suppress_set__IN_oid(eyedb::Database *_db, eyedb::FEMethod_C *_m, eyedb::Object *_o, const Oid obj)
{
  return Exception::make(IDB_ERROR, "void set::suppress(in oid) not yet implemented");
}
