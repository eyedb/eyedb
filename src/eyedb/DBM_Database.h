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


#ifndef _eyedb_dbm_h_
#define _eyedb_dbm_h_

#include <eyedb/eyedb_p.h>
//#include <eyedb/dbm.h>

namespace eyedb {

class DBM_Database : public Database {

  static LinkedList *dbmdb_list;
  Status create();
  Status update();
  Status realize(const RecMode* = RecMode::NoRecurs);
  Status remove(const RecMode* = RecMode::NoRecurs);
  Status lockId(Oid &oid);
  Status unlockId(const Oid &oid);

  // ----------------------------------------------------------------------
  // DBM_Database Interface
  // ----------------------------------------------------------------------
 public:
  DBM_Database(const char * = NULL);

  Status getDatabases(LinkedList *);

  static const char *getDbName();
  static int getDbid();

  Status create(Connection *, const char *,
		   const char *, const char *, DbCreateDescription * = NULL);

  Status addUser(Connection *, const char *, const char *,
		    UserType user_type,
		    const char * = NULL, const char * = NULL);

  Status deleteUser(Connection *, const char *,
		       const char * = NULL, const char * = NULL);

  Status setUserPasswd(Connection *, const char *, const char *,
			  const char * = NULL, const char * = NULL);
  Status setPasswd(Connection *, const char *,
		      const char * = NULL, const char * = NULL);

  Status setUserSysAccess(Connection *, const char *, SysAccessMode,
			     const char * = NULL, const char * = NULL);
  virtual ~DBM_Database();

  // ----------------------------------------------------------------------
  // DBM_Database Restricted Access (conceptually private)
  // ----------------------------------------------------------------------
 public:
  static void init();
  static void _release();
  static void _dble_underscore_release();
  static Status updateSchema(Database *);

  static DBM_Database *getDBM_Database(const char *);
  static DBM_Database *setDBM_Database(const char *, Database *);

  DBM_Database(const char *, Database *);

  Status getDbFile(const char **, int *, const char *&);

  Status getNewUid(int &);
  Status getNewDbid(int &);

  Status createEntry(int, const char *, const char *dbfile);
  Status updateEntry(int, const char *, const char *, const char *dbfile);
  Status removeEntry(const char *);

  Status get_sys_user_access(const char *, SysUserAccess **,
				Bool justCheck, const char *);
  Status get_db_user_access(const char *, const char *, UserEntry **,
			       DBUserAccess **, DBAccessMode *);
  Status add_user(const char *, const char *, UserType user_type);
  Status delete_user(const char *);
  Status user_passwd_set(const char *, const char *);
  Status default_db_access_set(const char *, DBAccessMode);
  Status user_db_access_set(const char *, const char *, DBAccessMode);
  Status user_sys_access_set(const char *, SysAccessMode);

  Status getUser(const char *, UserEntry *&);
  Status getDBEntry(const char *, DBEntry *&);
  Status getDBEntries(const char *, DBEntry **&, int &cnt,
			 const char *op);
  Status setSchema(const char *, const Oid &sch_oid);
  static const char defaultDBMDB[];
  static std::string makeTempName(int dbid);

 private:
  Status getNewID(const char *, const char *, int, int &, Bool);
};

}

#endif
