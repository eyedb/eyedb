
/*
 * EyeDB Version 2.8.8-nfs-fix-nkg Copyright (c) 1995-2006 SYSRA
 *
 * File 'DBM.h'
 *
 * Package Name 'DBM'
 *
 * Generated by eyedbodl at Sun Sep 18 18:00:18 2016
 *
 * ---------------------------------------------------
 * -------------- DO NOT EDIT THIS CODE --------------
 * ---------------------------------------------------
 *
 */

#include <eyedb/eyedb.h>

#ifndef _eyedb_DBM_
#define _eyedb_DBM_

namespace eyedb {

class UserEntry;
class DBUserAccess;
class SysUserAccess;
class DBEntry;
class DBPropertyValue;
class DBProperty;


class DBM {

 public:
  DBM(int &argc, char *argv[]) {
    eyedb::init(argc, argv);
    init();
  }

  ~DBM() {
    release();
    eyedb::release();
  }

  static void init();
  static void release();
  static eyedb::Status updateSchema(eyedb::Database *db);
  static eyedb::Status updateSchema(eyedb::Schema *m);
};

class DBMDatabase : public eyedb::Database {

 public:
  DBMDatabase(const char *dbname, const char *_dbmdb_str = 0) : eyedb::Database(dbname, _dbmdb_str) {}
  DBMDatabase(eyedb::Connection *conn, const char *dbname, const char *_dbmdb_str, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);
  DBMDatabase(eyedb::Connection *conn, const char *dbname, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);
  DBMDatabase(const char *dbname, int _dbid, const char *_dbmdb_str = 0) : eyedb::Database(dbname, _dbid, _dbmdb_str) {}
  DBMDatabase(int _dbid, const char *_dbmdb_str = 0) : eyedb::Database(_dbid, _dbmdb_str) {}
  eyedb::Status open(eyedb::Connection *, eyedb::Database::OpenFlag, const char *user = 0, const char *passwd = 0);
  eyedb::Status open(eyedb::Connection *, eyedb::Database::OpenFlag, const eyedb::OpenHints *hints, const char *user = 0, const char *passwd = 0);
  static void setConsApp(eyedb::Database *);
  static eyedb::Status checkSchema(eyedb::Schema *);
  static eyedb::Bool getDynamicGetErrorPolicy();
  static eyedb::Bool getDynamicSetErrorPolicy();
  static void setDynamicGetErrorPolicy(eyedb::Bool policy);
  static void setDynamicSetErrorPolicy(eyedb::Bool policy);
};

enum SysAccessMode {
  NoSysAccessMode = 0,
  DBCreateSysAccessMode = 256,
  AddUserSysAccessMode = 512,
  DeleteUserSysAccessMode = 1024,
  SetUserPasswdSysAccessMode = 2048,
  AdminSysAccessMode = 768,
  SuperUserSysAccessMode = 4095
};

enum DBAccessMode {
  NoDBAccessMode = 0,
  ReadDBAccessMode = 16,
  WriteDBAccessMode = 32,
  ExecDBAccessMode = 64,
  ReadWriteDBAccessMode = 48,
  ReadExecDBAccessMode = 80,
  ReadWriteExecDBAccessMode = 112,
  AdminDBAccessMode = 113
};

enum UserType {
  EyeDBUser = 1,
  UnixUser = 2,
  StrictUnixUser = 3
};

class UserEntry : public eyedb::Struct {

 public:
  UserEntry(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  UserEntry(const UserEntry& x);

  virtual eyedb::Object *clone() const {return new UserEntry(*this);};

  UserEntry& operator=(const UserEntry& x);

  virtual UserEntry *asUserEntry() {return this;}
  virtual const UserEntry *asUserEntry() const {return this;}

  eyedb::Status name(const std::string &);
  eyedb::Status name(unsigned int a0, char );
  std::string name(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char name(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status passwd(const std::string &);
  eyedb::Status passwd(unsigned int a0, char );
  std::string passwd(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char passwd(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status uid(eyedblib::int32 );
  eyedblib::int32 uid(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status type(UserType , eyedb::Bool _check_value = eyedb::True);
  UserType type(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;
  virtual ~UserEntry() {garbageRealize();}

 protected:
  UserEntry(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  UserEntry(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  UserEntry(const UserEntry *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  UserEntry(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  UserEntry(const UserEntry *, eyedb::Bool = eyedb::False);
  UserEntry(const eyedb::Class *, eyedb::Data);
};

class DBUserAccess : public eyedb::Struct {

 public:
  DBUserAccess(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  DBUserAccess(const DBUserAccess& x);

  virtual eyedb::Object *clone() const {return new DBUserAccess(*this);};

  DBUserAccess& operator=(const DBUserAccess& x);

  virtual DBUserAccess *asDBUserAccess() {return this;}
  virtual const DBUserAccess *asDBUserAccess() const {return this;}

  eyedb::Status dbentry(DBEntry*);
  DBEntry*dbentry(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const DBEntry*dbentry(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid dbentry_oid(eyedb::Status * = 0) const;
  eyedb::Status dbentry_oid(const eyedb::Oid &);

  eyedb::Status user(UserEntry*);
  UserEntry*user(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const UserEntry*user(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid user_oid(eyedb::Status * = 0) const;
  eyedb::Status user_oid(const eyedb::Oid &);

  eyedb::Status mode(DBAccessMode , eyedb::Bool _check_value = eyedb::True);
  DBAccessMode mode(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;
  virtual ~DBUserAccess() {garbageRealize();}

 protected:
  DBUserAccess(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  DBUserAccess(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  DBUserAccess(const DBUserAccess *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  DBUserAccess(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  DBUserAccess(const DBUserAccess *, eyedb::Bool = eyedb::False);
  DBUserAccess(const eyedb::Class *, eyedb::Data);
};

class SysUserAccess : public eyedb::Struct {

 public:
  SysUserAccess(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  SysUserAccess(const SysUserAccess& x);

  virtual eyedb::Object *clone() const {return new SysUserAccess(*this);};

  SysUserAccess& operator=(const SysUserAccess& x);

  virtual SysUserAccess *asSysUserAccess() {return this;}
  virtual const SysUserAccess *asSysUserAccess() const {return this;}

  eyedb::Status user(UserEntry*);
  UserEntry*user(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const UserEntry*user(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid user_oid(eyedb::Status * = 0) const;
  eyedb::Status user_oid(const eyedb::Oid &);

  eyedb::Status mode(SysAccessMode , eyedb::Bool _check_value = eyedb::True);
  SysAccessMode mode(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;
  virtual ~SysUserAccess() {garbageRealize();}

 protected:
  SysUserAccess(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  SysUserAccess(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  SysUserAccess(const SysUserAccess *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  SysUserAccess(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  SysUserAccess(const SysUserAccess *, eyedb::Bool = eyedb::False);
  SysUserAccess(const eyedb::Class *, eyedb::Data);
};

class DBEntry : public eyedb::Struct {

 public:
  DBEntry(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  DBEntry(const DBEntry& x);

  virtual eyedb::Object *clone() const {return new DBEntry(*this);};

  DBEntry& operator=(const DBEntry& x);

  virtual DBEntry *asDBEntry() {return this;}
  virtual const DBEntry *asDBEntry() const {return this;}

  eyedb::Status dbname(const std::string &);
  eyedb::Status dbname(unsigned int a0, char );
  std::string dbname(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char dbname(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status dbid(eyedblib::int32 );
  eyedblib::int32 dbid(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status dbfile(const std::string &);
  eyedb::Status dbfile(unsigned int a0, char );
  std::string dbfile(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char dbfile(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status default_access(DBAccessMode , eyedb::Bool _check_value = eyedb::True);
  DBAccessMode default_access(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status schm(eyedb::Object*);
  eyedb::Object*schm(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const eyedb::Object*schm(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid schm_oid(eyedb::Status * = 0) const;
  eyedb::Status schm_oid(const eyedb::Oid &);

  eyedb::Status comment(const std::string &);
  eyedb::Status comment(unsigned int a0, char );
  std::string comment(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char comment(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status props(unsigned int a0, DBProperty*);
  eyedb::Status props_cnt(unsigned int a0);
  DBProperty*props(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const DBProperty*props(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid props_oid(unsigned int a0, eyedb::Status * = 0) const;
  eyedb::Status props_oid(unsigned int a0, const eyedb::Oid &);
  unsigned int props_cnt(eyedb::Status * = 0) const;
  virtual ~DBEntry() {garbageRealize();}

 protected:
  DBEntry(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  DBEntry(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  DBEntry(const DBEntry *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  DBEntry(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  DBEntry(const DBEntry *, eyedb::Bool = eyedb::False);
  DBEntry(const eyedb::Class *, eyedb::Data);
};

class DBPropertyValue : public eyedb::Struct {

 public:
  DBPropertyValue(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  DBPropertyValue(const DBPropertyValue& x);

  virtual eyedb::Object *clone() const {return new DBPropertyValue(*this);};

  DBPropertyValue& operator=(const DBPropertyValue& x);

  virtual DBPropertyValue *asDBPropertyValue() {return this;}
  virtual const DBPropertyValue *asDBPropertyValue() const {return this;}

  eyedb::Status ival(eyedblib::int64 );
  eyedblib::int64 ival(eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status sval(const std::string &);
  eyedb::Status sval(unsigned int a0, char );
  std::string sval(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char sval(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status bval(const unsigned char *, unsigned int len);
  eyedb::Status bval(unsigned int a0, unsigned char );
  eyedb::Status bval_cnt(unsigned int a0);
  const unsigned char *bval(unsigned int *len, eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  unsigned char bval(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;
  unsigned int bval_cnt(eyedb::Status * = 0) const;

  eyedb::Status oval(eyedb::Object*);
  eyedb::Object*oval(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const eyedb::Object*oval(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  eyedb::Oid oval_oid(eyedb::Status * = 0) const;
  eyedb::Status oval_oid(const eyedb::Oid &);
  virtual ~DBPropertyValue() {garbageRealize();}

 protected:
  DBPropertyValue(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  DBPropertyValue(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  DBPropertyValue(const DBPropertyValue *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  DBPropertyValue(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  DBPropertyValue(const DBPropertyValue *, eyedb::Bool = eyedb::False);
  DBPropertyValue(const eyedb::Class *, eyedb::Data);
};

class DBProperty : public eyedb::Struct {

 public:
  DBProperty(eyedb::Database * = 0, const eyedb::Dataspace * = 0);
  DBProperty(const DBProperty& x);

  virtual eyedb::Object *clone() const {return new DBProperty(*this);};

  DBProperty& operator=(const DBProperty& x);

  virtual DBProperty *asDBProperty() {return this;}
  virtual const DBProperty *asDBProperty() const {return this;}

  eyedb::Status key(const std::string &);
  eyedb::Status key(unsigned int a0, char );
  std::string key(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  char key(unsigned int a0, eyedb::Bool *isnull = 0, eyedb::Status * = 0)  const;

  eyedb::Status value(DBPropertyValue*);
  DBPropertyValue*value(eyedb::Bool *isnull = 0, eyedb::Status * = 0) ;
  const DBPropertyValue*value(eyedb::Bool *isnull = 0, eyedb::Status * = 0) const;
  virtual ~DBProperty() {garbageRealize();}

 protected:
  DBProperty(eyedb::Database *_db, const eyedb::Dataspace *_dataspace, int) : eyedb::Struct(_db, _dataspace) {}
  DBProperty(const eyedb::Struct *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}
  DBProperty(const DBProperty *x, eyedb::Bool share, int) : eyedb::Struct(x, share) {}

 private:
  void initialize(eyedb::Database *_db);

 public: /* restricted */
  DBProperty(const eyedb::Struct *, eyedb::Bool = eyedb::False);
  DBProperty(const DBProperty *, eyedb::Bool = eyedb::False);
  DBProperty(const eyedb::Class *, eyedb::Data);
};


extern eyedb::Object *DBMMakeObject(eyedb::Object *, eyedb::Bool=eyedb::True);
extern eyedb::Bool DBM_set_oid_check(eyedb::Bool);
extern eyedb::Bool DBM_get_oid_check();


}

#endif
