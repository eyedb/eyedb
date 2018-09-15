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

package org.eyedb;

public class Status {

  int status;
  String str = null;


  static public final int SE_SUCCESS = 0;
  static public final int SE_ERROR = 1;
  static public final int SE_SYS_ERROR = 2;
  static public final int SE_CONNECTION_FAILURE = 3;
  static public final int SE_SERVER_FAILURE = 4;
  static public final int SE_CANNOT_LOCK_SHMFILE = 5;
  static public final int SE_DB_ALREADY_LOCK_BY_A_SERVER = 6;
  static public final int SE_INVALID_DBID = 7;
  static public final int SE_INVALID_SIZESLOT = 8;
  static public final int SE_INVALID_NBSLOTS = 9;
  static public final int SE_INVALID_NBOBJS = 10;
  static public final int SE_INVALID_MAXSIZE = 11;
  static public final int SE_INVALID_MAPTYPE = 12;
  static public final int SE_DATABASE_CREATION_ERROR = 13;
  static public final int SE_DATABASE_ACCESS_DENIED = 14;
  static public final int SE_DATABASE_OPEN_FAILED = 15;
  static public final int SE_INVALID_DATAFILE_CNT = 16;
  static public final int SE_INVALID_DATASPACE_CNT = 17;
  static public final int SE_INVALID_DATAFILE_CNT_IN_DATASPACE = 18;
  static public final int SE_INVALID_DATASPACE = 19;
  static public final int SE_INVALID_DBFILE = 20;
  static public final int SE_INVALID_DBFILE_ACCESS = 21;
  static public final int SE_INVALID_SHMFILE = 22;
  static public final int SE_INVALID_SHMFILE_ACCESS = 23;
  static public final int SE_INVALID_OBJMAP_ACCESS = 24;
  static public final int SE_INVALID_DATAFILE = 25;
  static public final int SE_INVALID_DMPFILE = 26;
  static public final int SE_INVALID_DATAFILEMAXSIZE = 27;
  static public final int SE_INVALID_FILES_COPY = 28;
  static public final int SE_INVALID_DBFILES_COPY = 29;
  static public final int SE_INVALID_DATAFILES_COPY = 30;
  static public final int SE_INVALID_SHMFILES_COPY = 31;
  static public final int SE_INVALID_OBJMAPFILES_COPY = 32;
  static public final int SE_DBFILES_IDENTICAL = 33;
  static public final int SE_DATAFILES_IDENTICAL = 34;
  static public final int SE_DBFILE_ALREADY_EXISTS = 35;
  static public final int SE_SHMFILE_ALREADY_EXISTS = 36;
  static public final int SE_OBJMAPFILE_ALREADY_EXISTS = 37;
  static public final int SE_DATAFILE_ALREADY_EXISTS = 38;
  static public final int SE_SIZE_TOO_LARGE = 39;
  static public final int SE_WRITE_FORBIDDEN = 40;
  static public final int SE_CONN_RESET_BY_PEER = 41;
  static public final int SE_LOCK_TIMEOUT = 42;
  static public final int SE_FATAL_MUTEX_LOCK_TIMEOUT = 43;
  static public final int SE_BACKEND_INTERRUPTED = 44;
  static public final int SE_INVALID_TRANSACTION_MODE = 45;
  static public final int SE_RW_TRANSACTION_NEEDED = 46;
  static public final int SE_TRANSACTION_NEEDED = 47;
  static public final int SE_TRANSACTION_LOCKING_FAILED = 48;
  static public final int SE_TRANSACTION_UNLOCKING_FAILED = 49;
  static public final int SE_TOO_MANY_TRANSACTIONS = 50;
  static public final int SE_TRANSACTION_TOO_MANY_NESTED = 51;
  static public final int SE_DEADLOCK_DETECTED = 52;
  static public final int SE_INVALID_FLAG = 53;
  static public final int SE_INVALID_DB_HANDLE = 54;
  static public final int SE_MAP_ERROR = 55;
  static public final int SE_TOO_MANY_OBJECTS = 56;
  static public final int SE_INVALID_OBJECT_SIZE = 57;
  static public final int SE_NO_DATAFILESPACE_LEFT = 58;
  static public final int SE_NO_SHMSPACE_LEFT = 59;
  static public final int SE_INVALID_SIZE = 60;
  static public final int SE_INVALID_OFFSET = 61;
  static public final int SE_INVALID_OID = 62;
  static public final int SE_INVALID_ROOT_ENTRY_SIZE = 63;
  static public final int SE_INVALID_ROOT_ENTRY_KEY = 64;
  static public final int SE_INVALID_READ_ACCESS = 65;
  static public final int SE_INVALID_WRITE_ACCESS = 66;
  static public final int SE_OBJECT_PROTECTED = 67;
  static public final int SE_PROTECTION_INVALID_UID = 68;
  static public final int SE_PROTECTION_DUPLICATE_UID = 69;
  static public final int SE_PROTECTION_DUPLICATE_NAME = 70;
  static public final int SE_PROTECTION_NOT_FOUND = 71;
  static public final int SE_ROOT_ENTRY_EXISTS = 72;
  static public final int SE_TOO_MANY_ROOT_ENTRIES = 73;
  static public final int SE_ROOT_ENTRY_NOT_FOUND = 74;
  static public final int SE_PROT_NAME_TOO_LONG = 75;
  static public final int SE_NOTIMPLEMENTED = 76;
  static public final int SE_NO_SETUID_PRIVILEGE = 77;
  static public final int SE_NOT_YET_IMPLEMENTED = 78;
  static public final int SE_COMPATIBILITY_ERROR = 79;
  static public final int SE_INTERNAL_ERROR = 80;
  static public final int SE_FATAL_ERROR = 81;
  static public final int SE_N_ERROR = 82;

  static public final int IDB_SUCCESS = SE_SUCCESS;
  static public final int IDB_ERROR = SE_N_ERROR;
  static public final int IDB_FATAL_ERROR = SE_N_ERROR+1;
  static public final int IDB_NOT_YET_IMPLEMENTED = SE_N_ERROR+2;
  static public final int IDB_INTERNAL_ERROR = SE_N_ERROR+3;
  static public final int IDB_BUG = SE_N_ERROR+4;
  static public final int IDB_CONNECTION_FAILURE = SE_N_ERROR+5;
  static public final int IDB_SERVER_FAILURE = SE_N_ERROR+6;
  static public final int IDB_SE_ERROR = SE_N_ERROR+7;
  static public final int IDB_INVALID_DBOPEN_FLAG = SE_N_ERROR+8;
  static public final int IDB_INVALID_DB_ID = SE_N_ERROR+9;
  static public final int IDB_INVALID_CLIENT_ID = SE_N_ERROR+10;
  static public final int IDB_INVALID_SCHEMA = SE_N_ERROR+11;
  static public final int IDB_INVALID_OBJECT_HEADER = SE_N_ERROR+12;
  static public final int IDB_INVALID_TRANSACTION = SE_N_ERROR+13;
  static public final int IDB_INVALID_TRANSACTION_MODE = SE_N_ERROR+14;
  static public final int IDB_INVALID_TRANSACTION_WRITE_MODE = SE_N_ERROR+15;
  static public final int IDB_INVALID_TRANSACTION_PARAMS = SE_N_ERROR+16;
  static public final int IDB_AUTHENTICATION_NOT_SET = SE_N_ERROR+17;
  static public final int IDB_AUTHENTICATION_FAILED = SE_N_ERROR+18;
  static public final int IDB_INSUFFICIENT_PRIVILEGES = SE_N_ERROR+19;
  static public final int IDB_NO_CURRENT_TRANSACTION = SE_N_ERROR+20;
  static public final int IDB_TRANSACTION_COMMIT_UNEXPECTED = SE_N_ERROR+21;
  static public final int IDB_TRANSACTION_ABORT_UNEXPECTED = SE_N_ERROR+22;
  static public final int IDB_ADD_USER_ERROR = SE_N_ERROR+23;
  static public final int IDB_DELETE_USER_ERROR = SE_N_ERROR+24;
  static public final int IDB_SET_USER_PASSWD_ERROR = SE_N_ERROR+25;
  static public final int IDB_SET_PASSWD_ERROR = SE_N_ERROR+26;
  static public final int IDB_SET_USER_DBACCESS_ERROR = SE_N_ERROR+27;
  static public final int IDB_SET_DEFAULT_DBACCESS_ERROR = SE_N_ERROR+28;
  static public final int IDB_SET_USER_SYSACCESS_ERROR = SE_N_ERROR+29;
  static public final int IDB_SETDATABASE_ERROR = SE_N_ERROR+30;
  static public final int IDB_OBJECT_REMOVE_ERROR = SE_N_ERROR+31;
  static public final int IDB_IS_OBJECT_OF_CLASS_ERROR = SE_N_ERROR+32;
  static public final int IDB_ITERATOR_ERROR = SE_N_ERROR+33;
  static public final int IDB_DBM_ERROR = SE_N_ERROR+34;
  static public final int IDB_SCHEMA_ERROR = SE_N_ERROR+35;
  static public final int IDB_DATABASE_OPEN_ERROR = SE_N_ERROR+36;
  static public final int IDB_DATABASE_CLOSE_ERROR = SE_N_ERROR+37;
  static public final int IDB_DATABASE_CREATE_ERROR = SE_N_ERROR+38;
  static public final int IDB_DATABASE_REMOVE_ERROR = SE_N_ERROR+39;
  static public final int IDB_DATABASE_COPY_ERROR = SE_N_ERROR+40;
  static public final int IDB_DATABASE_MOVE_ERROR = SE_N_ERROR+41;
  static public final int IDB_DATABASE_RENAME_ERROR = SE_N_ERROR+42;
  static public final int IDB_DATABASE_LOAD_OBJECT_ERROR = SE_N_ERROR+43;
  static public final int IDB_DATABASE_GET_OBJECT_CLASS_ERROR = SE_N_ERROR+44;
  static public final int IDB_INCONSISTANT_OBJECT_HEADERS = SE_N_ERROR+45;
  static public final int IDB_CANNOT_CREATE_SCHEMA = SE_N_ERROR+46;
  static public final int IDB_CANNOT_UPDATE_SCHEMA = SE_N_ERROR+47;
  static public final int IDB_SCHEMA_ALREADY_CREATED = SE_N_ERROR+48;
  static public final int IDB_OBJECT_ALREADY_CREATED = SE_N_ERROR+49;
  static public final int IDB_OBJECT_NOT_CREATED = SE_N_ERROR+50;
  static public final int IDB_OUT_OF_MEMORY = SE_N_ERROR+51;
  static public final int IDB_BACKEND_INTERRUPTED = SE_N_ERROR+52;
  static public final int IDB_ITERATOR_ATTRIBUTE_NO_IDX = SE_N_ERROR+53;
  static public final int IDB_ITERATOR_ATTRIBUTE_INVALID_SIZE = SE_N_ERROR+54;
  static public final int IDB_ITERATOR_ATTRIBUTE_INVALID_INDICE = SE_N_ERROR+55;
  static public final int IDB_OQL_SYNTAX_ERROR = SE_N_ERROR+56;
  static public final int IDB_OQL_ERROR = SE_N_ERROR+57;
  static public final int IDB_OQL_INTERRUPTED = SE_N_ERROR+58;
  static public final int IDB_CLASS_READ = SE_N_ERROR+59;
  static public final int IDB_ATTRIBUTE_ERROR = SE_N_ERROR+60;
  static public final int IDB_ATTRIBUTE_INVERSE_ERROR = SE_N_ERROR+61;
  static public final int IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR = SE_N_ERROR+62;
  static public final int IDB_MAG_ORDER_ERROR = SE_N_ERROR+63;
  static public final int IDB_ENUM_ERROR = SE_N_ERROR+64;
  static public final int IDB_NO_CLASS = SE_N_ERROR+65;
  static public final int IDB_CLASS_NOT_CREATED = SE_N_ERROR+66;
  static public final int IDB_CLASS_NOT_FOUND = SE_N_ERROR+67;
  static public final int IDB_INDEX_ERROR = SE_N_ERROR+68;
  static public final int IDB_COLLECTION_ERROR = SE_N_ERROR+69;
  static public final int IDB_COLLECTION_INSERT_ERROR = SE_N_ERROR+70;
  static public final int IDB_COLLECTION_DUPLICATE_INSERT_ERROR = SE_N_ERROR+71;
  static public final int IDB_COLLECTION_SUPPRESS_ERROR = SE_N_ERROR+72;
  static public final int IDB_COLLECTION_IS_IN_ERROR = SE_N_ERROR+73;
  static public final int IDB_COLLECTION_ITEM_SIZE_TOO_LARGE = SE_N_ERROR+74;
  static public final int IDB_COLLECTION_ITEM_SIZE_UNKNOWN = SE_N_ERROR+75;
  static public final int IDB_COLLECTION_BACK_END_ERROR = SE_N_ERROR+76;
  static public final int IDB_COLLECTION_LOCKED = SE_N_ERROR+77;
  static public final int IDB_CLASS_COMPLETION_ERROR = SE_N_ERROR+78;
  static public final int IDB_GENERATION_CODE_ERROR = SE_N_ERROR+79;
  static public final int IDB_EXECUTABLE_ERROR = SE_N_ERROR+80;
  static public final int IDB_UNIQUE_CONSTRAINT_ERROR = SE_N_ERROR+81;
  static public final int IDB_UNIQUE_COMP_CONSTRAINT_ERROR = SE_N_ERROR+82;
  static public final int IDB_NOTNULL_CONSTRAINT_ERROR = SE_N_ERROR+83;
  static public final int IDB_NOTNULL_COMP_CONSTRAINT_ERROR = SE_N_ERROR+84;
  static public final int IDB_CARDINALITY_CONSTRAINT_ERROR = SE_N_ERROR+85;
  static public final int IDB_SESSION_LOG_CREATION_ERROR = SE_N_ERROR+86;
  static public final int IDB_SESSION_LOG_OPEN_ERROR = SE_N_ERROR+87;
  static public final int IDB_SESSION_LOG_NO_SPACE_LEFT = SE_N_ERROR+88;
  static public final int IDB_N_ERROR = SE_N_ERROR+89;

  public static String[] errmsg;
  private static String se_error = "storage manager: ";

  static {
    errmsg = new String[IDB_N_ERROR];

    errmsg[SE_SUCCESS] =
      se_error + "success";

    errmsg[SE_ERROR] =
      se_error + "error";

    errmsg[SE_SYS_ERROR] =
      se_error + "system error";

    errmsg[SE_CONNECTION_FAILURE] =
      se_error + "connection failure";

    errmsg[SE_SERVER_FAILURE] =
      se_error + "server failure";

    errmsg[SE_CANNOT_LOCK_SHMFILE] =
      se_error + "cannot lock shm file";

    errmsg[SE_DB_ALREADY_LOCK_BY_A_SERVER] =
      se_error + "db already lock by a server";

    errmsg[SE_INVALID_DBID] =
      se_error + "invalid dbid";

    errmsg[SE_INVALID_MAXSIZE] =
      se_error + "invalid maxsize";

    errmsg[SE_INVALID_SIZESLOT] =
      se_error + "invalid sizeslot";

    errmsg[SE_INVALID_NBSLOTS] =
      se_error + "invalid nbslots";

    errmsg[SE_DATABASE_CREATION_ERROR] =
      se_error + "database creation error";

    errmsg[SE_DATABASE_ACCESS_DENIED] =
      se_error + "database access denied";

    errmsg[SE_DATABASE_OPEN_FAILED] =
      se_error + "database open failed";

    errmsg[SE_INVALID_DATAFILE_CNT] =
	se_error + "invalid datafile count";

    errmsg[SE_INVALID_DATASPACE_CNT] =
	se_error + "invalid dataspace count";

    errmsg[SE_INVALID_DATAFILE_CNT_IN_DATASPACE] =
	se_error + "invalid datafile count in a dataspace";

    errmsg[SE_INVALID_DATASPACE] =
	se_error + "invalid dataspace";

    errmsg[SE_INVALID_DBFILE] =
      se_error + "invalid database file";

    errmsg[SE_INVALID_DBFILE_ACCESS] =
      se_error + "invalid database file access";

    errmsg[SE_INVALID_SHMFILE] =
      se_error + "invalid shm file";

    errmsg[SE_INVALID_SHMFILE_ACCESS] =
      se_error + "invalid shm file access";

    errmsg[SE_INVALID_DATAFILE] =
      se_error + "invalid data file";

    errmsg[SE_INVALID_DMPFILE] =
      se_error + "invalid data map file";

    errmsg[SE_INVALID_DATAFILEMAXSIZE] =
      se_error + "invalid volume maxsize";

    errmsg[SE_INVALID_FILES_COPY] =
      se_error + "invalid files copy";

    errmsg[SE_INVALID_DBFILES_COPY] =
      se_error + "invalid database files copy";

    errmsg[SE_INVALID_DATAFILES_COPY] =
      se_error + "invalid data files copy";

    errmsg[SE_INVALID_SHMFILES_COPY] =
      se_error + "invalid shm files copy";

    errmsg[SE_DBFILES_IDENTICAL] =
      se_error + "database files are identical";

    errmsg[SE_DATAFILES_IDENTICAL] =
      se_error + "data files are identical";

    errmsg[SE_DBFILE_ALREADY_EXISTS] =
      se_error + "database file already exists";

    errmsg[SE_SHMFILE_ALREADY_EXISTS] =
      se_error + "shm file already exists";

    errmsg[SE_DATAFILE_ALREADY_EXISTS] =
      se_error + "data file already exists";

    errmsg[SE_SIZE_TOO_LARGE] =
      se_error + "size too large";

    errmsg[SE_WRITE_FORBIDDEN] =
      se_error + "write forbidden";

    errmsg[SE_BACKEND_INTERRUPTED] =
      se_error + "backend interrupted";

    errmsg[SE_CONN_RESET_BY_PEER] =
      se_error + "connection reset by peer";

    errmsg[SE_LOCK_TIMEOUT] =
      se_error + "fatal mutex lock timeout: the shmem must be cleanup " +
      "(or possibly the computer is overloaded)";

    errmsg[SE_FATAL_MUTEX_LOCK_TIMEOUT] =
      se_error + "lock timeout";

    errmsg[SE_INVALID_FLAG] =
      se_error + "invalid flag";

    errmsg[SE_INVALID_DB_HANDLE] =
      se_error + "invalid database handle";

    errmsg[SE_TRANSACTION_TOO_MANY_NESTED] =
      se_error + "too many transactions nested";

    errmsg[SE_TOO_MANY_TRANSACTIONS] =
      se_error + "too many transactions";

    errmsg[SE_TRANSACTION_NEEDED] =
      se_error + "transaction needed";

    errmsg[SE_TRANSACTION_LOCKING_FAILED] =
      se_error + "transaction locking failed";

    errmsg[SE_TRANSACTION_UNLOCKING_FAILED] =
      se_error + "transaction unlocking failed";

    errmsg[SE_DEADLOCK_DETECTED] =
      se_error + "deadlock detected";

    errmsg[SE_INVALID_TRANSACTION_MODE] =
      se_error + "invalid transaction mode";

    errmsg[SE_RW_TRANSACTION_NEEDED] =
      se_error + "read write mode transaction needed";

    errmsg[SE_NOT_YET_IMPLEMENTED] =
      se_error + "not yet implemented";

    errmsg[SE_MAP_ERROR] =
      se_error + "map error";

    errmsg[SE_TOO_MANY_OBJECTS] =
      se_error + "too many objects";

    errmsg[SE_INVALID_MAPTYPE] =
      se_error + "invalid map type";

    errmsg[SE_INVALID_OBJECT_SIZE] =
      se_error + "invalid object size";

    errmsg[SE_INVALID_OFFSET] =
      se_error + "invalid object offset";

    errmsg[SE_NO_DATAFILESPACE_LEFT] =
      se_error + "no space left on volume";

    errmsg[SE_NO_SHMSPACE_LEFT] =
      se_error + "no space left on shm";

    errmsg[SE_INVALID_SIZE] =
      se_error + "invalid size";

    errmsg[SE_PROTECTION_INVALID_UID] =
      se_error + "protection invalid uid";

    errmsg[SE_PROTECTION_DUPLICATE_UID] =
      se_error + "protection duplicate uid";

    errmsg[SE_PROTECTION_DUPLICATE_NAME] =
      se_error + "protection duplicate name";

    errmsg[SE_PROTECTION_NOT_FOUND] =
      se_error + "protection not found";

    errmsg[SE_INVALID_OID] =
      se_error + "invalid oid";

    errmsg[SE_OBJECT_PROTECTED] =
      se_error + "object protected";

    errmsg[SE_INVALID_ROOT_ENTRY_SIZE] =
      se_error + "invalid root entry size";

    errmsg[SE_INVALID_ROOT_ENTRY_KEY] =
      se_error + "invalid root entry key";

    errmsg[SE_INVALID_READ_ACCESS] =
      se_error + "invalid read access";

    errmsg[SE_INVALID_WRITE_ACCESS] =
      se_error + "invalid write access";

    errmsg[SE_PROT_NAME_TOO_LONG] =
      se_error + "prot name too long";

    errmsg[SE_ROOT_ENTRY_EXISTS] =
      se_error + "root entry exists";

    errmsg[SE_TOO_MANY_ROOT_ENTRIES] =
      se_error + "too many root entries";

    errmsg[SE_ROOT_ENTRY_NOT_FOUND] =
      se_error + "root entry not found";

    errmsg[SE_NOTIMPLEMENTED] =
      se_error + "notimplemented";

    errmsg[SE_NO_SETUID_PRIVILEGE] =
      se_error + "no setuid privilege";

    errmsg[SE_INVALID_NBOBJS] =
      se_error + "invalid object number";

    errmsg[SE_INVALID_OBJMAP_ACCESS] =
      se_error + "invalid oid map file access";

    errmsg[SE_INVALID_OBJMAPFILES_COPY] =
      se_error + "invalid object map files copy";

    errmsg[SE_OBJMAPFILE_ALREADY_EXISTS] =
      se_error + "object map file already exists";

    errmsg[SE_COMPATIBILITY_ERROR] =
      se_error + "compatibility error";

    errmsg[SE_INTERNAL_ERROR] =
      se_error + "internal error";

    errmsg[SE_FATAL_ERROR] =
      se_error + "fatal error";

    errmsg[IDB_SUCCESS] =
      "operation succesful";

    errmsg[IDB_ERROR] =
      "eyedb error";

    errmsg[IDB_FATAL_ERROR] =
      "eyedb fatal error";

    errmsg[IDB_NOT_YET_IMPLEMENTED] =
      "feature not yet implemented";

    errmsg[IDB_INTERNAL_ERROR] =
      "eyedb internal error";

    errmsg[IDB_BUG] =
      "eyedb internal bug corrected soon";

    errmsg[IDB_CONNECTION_FAILURE] =
      "connection failure";

    errmsg[IDB_SERVER_FAILURE] =
      "server failure";

    errmsg[IDB_SE_ERROR] =
      "storage manager error";

    errmsg[IDB_INVALID_DBOPEN_FLAG] =
      "invalid database open flag";

    errmsg[IDB_INVALID_DB_ID] =
      "invalid database identifier";

    errmsg[IDB_INVALID_CLIENT_ID] =
      "invalid client identifier";

    errmsg[IDB_INVALID_SCHEMA] =
      "invalid schema";

    errmsg[IDB_INVALID_OBJECT_HEADER] =
      "invalid object header";

    errmsg[IDB_INVALID_TRANSACTION] =
      "invalid transaction";

    errmsg[IDB_INVALID_TRANSACTION_MODE] =
      "invalid transaction mode";

    errmsg[IDB_INVALID_TRANSACTION_WRITE_MODE] =
      "invalid transaction write mode";

    errmsg[IDB_INVALID_TRANSACTION_PARAMS] =
      "invalid transaction parameters";

    errmsg[IDB_AUTHENTICATION_NOT_SET] =
      "authentication parameters are not set (check your IDBUSER and IDBPASSWD environment variables or use -idbuser and -idbpasswd command line options)";

    errmsg[IDB_ADD_USER_ERROR] =
      "add user access error";

    errmsg[IDB_DELETE_USER_ERROR] =
      "delete user access error";

    errmsg[IDB_SET_USER_PASSWD_ERROR] =
      "set user password error";

    errmsg[IDB_SET_PASSWD_ERROR] =
      "set password error";

    errmsg[IDB_SET_USER_DBACCESS_ERROR] =
      "set user database access error";

    errmsg[IDB_SET_DEFAULT_DBACCESS_ERROR] =
      "set default database access error";

    errmsg[IDB_SET_USER_SYSACCESS_ERROR] =
      "set user system access error";

    errmsg[IDB_AUTHENTICATION_FAILED] =
      "authentication failed";

    errmsg[IDB_TRANSACTION_COMMIT_UNEXPECTED] =
      "commit unexpected";

    errmsg[IDB_TRANSACTION_ABORT_UNEXPECTED] =
      "abort unexpected";

    errmsg[IDB_INSUFFICIENT_PRIVILEGES] =
      "insufficient privileges";

    errmsg[IDB_NO_CURRENT_TRANSACTION] =
      "no current transaction";

    errmsg[IDB_DATABASE_OPEN_ERROR] =
      "database open error";

    errmsg[IDB_DATABASE_CLOSE_ERROR] =
      "database close error";

    errmsg[IDB_DATABASE_CREATE_ERROR] =
      "database create error";

    errmsg[IDB_DATABASE_REMOVE_ERROR] =
      "database remove error";

    errmsg[IDB_DATABASE_COPY_ERROR] =
      "database copy error";

    errmsg[IDB_DATABASE_MOVE_ERROR] =
      "database move error";

    errmsg[IDB_DATABASE_REMOVE_ERROR] =
      "database remove error";

    errmsg[IDB_DATABASE_RENAME_ERROR] =
      "database rename error";

    errmsg[IDB_DATABASE_LOAD_OBJECT_ERROR] =
      "database load object error";

    errmsg[IDB_DATABASE_GET_OBJECT_CLASS_ERROR] =
      "database get object class error";

    errmsg[IDB_INCONSISTANT_OBJECT_HEADERS] =
      "inconsistant object headers";

    errmsg[IDB_IS_OBJECT_OF_CLASS_ERROR] =
      "is object of class error";

    errmsg[IDB_SETDATABASE_ERROR] =
      "set database error";

    errmsg[IDB_OBJECT_REMOVE_ERROR] =
      "object remove";

    errmsg[IDB_CANNOT_CREATE_SCHEMA] =
      "cannot create schema";

    errmsg[IDB_CANNOT_UPDATE_SCHEMA] =
      "cannot update schema";

    errmsg[IDB_SCHEMA_ALREADY_CREATED] =
      "schema already created";

    errmsg[IDB_OBJECT_ALREADY_CREATED] =
      "object already created";

    errmsg[IDB_OBJECT_NOT_CREATED] =
      "object not yet created";

    errmsg[IDB_OUT_OF_MEMORY] =
      "eyedb out of memory";

    errmsg[IDB_BACKEND_INTERRUPTED] =
      "backend interrupted";

    errmsg[IDB_ITERATOR_ATTRIBUTE_NO_IDX] =
      "no index for attribute";

    errmsg[IDB_ITERATOR_ATTRIBUTE_INVALID_SIZE] =
      "attribute invalid size";

    errmsg[IDB_ITERATOR_ATTRIBUTE_INVALID_INDICE] =
      "attribute invalid indice";

    errmsg[IDB_ITERATOR_ERROR] =
      "iterator error";

    errmsg[IDB_DBM_ERROR] =
      "database manager error";

    errmsg[IDB_SCHEMA_ERROR] =
      "schema error";

    errmsg[IDB_OQL_SYNTAX_ERROR] =
      "oql syntax error";

    errmsg[IDB_OQL_ERROR] =
      "oql error";

    errmsg[IDB_OQL_INTERRUPTED] =
      "oql interrupted";

    errmsg[IDB_CLASS_READ] =
      "reading class";

    errmsg[IDB_ATTRIBUTE_ERROR] =
      "attribute error";

    errmsg[IDB_ATTRIBUTE_INVERSE_ERROR] =
      "attribute inverse directive error";

    errmsg[IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR] =
       "out of range attribute error";

    errmsg[IDB_NO_CLASS] =
      "no class for object";

    errmsg[IDB_CLASS_NOT_CREATED] =
      "class is not created";

    errmsg[IDB_CLASS_NOT_FOUND] =
      "class not found";

    errmsg[IDB_MAG_ORDER_ERROR] =
      "magnitude order error";

    errmsg[IDB_CARDINALITY_CONSTRAINT_ERROR] =
      "cardinality constraint error";

    errmsg[IDB_SESSION_LOG_CREATION_ERROR] =
      "session shm creation error";

    errmsg[IDB_SESSION_LOG_OPEN_ERROR] =
      "session shm opening error";

    errmsg[IDB_SESSION_LOG_NO_SPACE_LEFT] =
      "no space left on session shm";

    errmsg[IDB_INDEX_ERROR] =
      "attribute index error";

    errmsg[IDB_ENUM_ERROR] =
      "enum error";

    errmsg[IDB_COLLECTION_BACK_END_ERROR] =
      "back end collection error";

    errmsg[IDB_COLLECTION_ITEM_SIZE_TOO_LARGE] =
      "collection item size is too large";

    errmsg[IDB_COLLECTION_ITEM_SIZE_UNKNOWN] =
      "collection unknown item size";

    errmsg[IDB_COLLECTION_ERROR] =
      "collection error";

    errmsg[IDB_COLLECTION_LOCKED] =
      "collection lock error";

    errmsg[IDB_COLLECTION_INSERT_ERROR] =
      "collection insert error";

    errmsg[IDB_COLLECTION_DUPLICATE_INSERT_ERROR] =
      "collection duplicate insert error";

    errmsg[IDB_COLLECTION_SUPPRESS_ERROR] =
      "collection suppress error";

    errmsg[IDB_COLLECTION_IS_IN_ERROR] =
      "collection is in error";

    errmsg[IDB_CLASS_COMPLETION_ERROR] =
      "class completion error";

    errmsg[IDB_GENERATION_CODE_ERROR] =
      "generation code error";

    errmsg[IDB_UNIQUE_CONSTRAINT_ERROR] =
      "unique constraint error";

    errmsg[IDB_UNIQUE_COMP_CONSTRAINT_ERROR] =
      "unique[] constraint error";

    errmsg[IDB_NOTNULL_CONSTRAINT_ERROR] =
      "not null constraint error";

    errmsg[IDB_NOTNULL_COMP_CONSTRAINT_ERROR] =
      "notnull[] constraint error";

    errmsg[IDB_EXECUTABLE_ERROR] =
      "executable error";

    for (int i = 0; i < IDB_N_ERROR; i++)
      if (errmsg[i] == null)
	System.out.println("error for status " + i);
  }

  public Status() {
    status = 0;
  }

  public Status(int status) {
    set(status, "");
  }

  public Status(String s) {
    set(IDB_ERROR, s);
  }

  public Status(int status, String s) {
    set(status, s);
  }

  public void set(int status, String s) {
    this.status = status;
    this.str = s;

    /*
    if (status != 0 && s != null)
      System.err.println("status #" + status + " [" + errmsg[status]  + "] : " + s);
    */
  }

  public boolean isSuccess() {
    return status == 0;
  }

  public void print(String s) {
    if (status != 0)
      System.err.println(s + ": " + getString());
  }

  public String getString() {
    String s = errmsg[status];
    if (str != null)
	s += " : " + str;
    return s;
  }
}
