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


#ifndef _EYEDB_EYEDB_P_H
#define _EYEDB_EYEDB_P_H

#include <eyedb/eyedb.h>
#include <eyedb/ObjCache.h>
#include <eyedblib/xdr.h>
#include <eyedbsm/xdr.h>
#include <eyedb/IteratorAtom.h>
#include <eyedb/Iterator.h>
#include <eyedb/api_lib.h>
#include <eyedb/base_p.h>
#include <eyedb/internals/ObjectHeader.h>
#include <eyedb/ObjCache.h>
#include <eyedb/make_obj.h>
#include <eyedb/code.h>
#include <eyedb/SchemaInfo.h>
#include <eyedblib/log.h>
#include <eyedb/Log.h>
#include <string>
#include <eyedblib/probe.h>
#include <eyedblib/strutils.h>

#include <assert.h>

/*extern "C" {*/
#include <eyedb/misc.h>
/*}*/

#include <eyedb/internals/kern_const.h>
#include <eyedb/internals/ObjectPeer.h>
#include <eyedb/ConnectionPeer.h>
#include <eyedb/internals/ClassPeer.h>
#include <eyedb/CollectionPeer.h>
#include <eyedb/BufferString.h>
#include <eyedb/GenContext.h>
#include <eyedb/Argument_code.h>
#include <eyedb/Argument.h>
#include <eyedb/Executable.h>
#include <eyedb/odlgen_utils.h>
#include <eyedblib/std_alloc.h>

namespace eyedbsm {
  extern Boolean trace_idx;
  extern Boolean trace_idx_sync;
  extern FILE *trace_idx_fd;
}

namespace eyedb {

  enum {
    StructClass_Code = 100,
    UnionClass_Code
  };

#define idbVarDim(X) (-(X))
  //#define idbWholeSize ((Size)0xffffffff)

  extern eyedbsm::Oid ClassOidDecode(Data);

#define idbNumber(X) (sizeof(X)/sizeof(X[0]))

  extern Status StatusMake(RPCStatus);
  extern Status StatusMake(Error, RPCStatus);
  extern RPCStatus rpcStatusMake(Status);
  extern Status StatusMake(Error, RPCStatus, const char *, ...);
  extern DbHandle *database_getDbHandle(Database *db);
  extern void (*garbage_handler)(void);

#define mcp(D, S, N) \
{ \
  int __n__ = (N); \
  char *__d__ = (char *)(D), *__s__ = (char *)(S); \
  while(__n__--) \
    *__d__++ = *__s__++; \
}

#define mset(D, V, N) \
{ \
  int __n__ = (N); \
  char *__d__ = (char *)(D); \
  while(__n__--) \
    *__d__++ = V; \
}

void setBackendInterrupt(Bool);
Bool isBackendInterrupted();
extern void setServerMessage(const char *msg);
extern void setServerOutOfBandData(unsigned int type, unsigned char *data,
				       unsigned int len);
#define IDB_SERVER_MESSAGE 1

#define IDB_CHECK_INTR() \
if (isBackendInterrupted()) \
{ \
    setBackendInterrupt(False); \
    return Exception::make(IDB_BACKEND_INTERRUPTED, ""); \
}

extern const char *eyedb_time();

#define IDB_VECT

#define idbNewVect(T, N) (T *)calloc(sizeof(T), N)

#define idbFreeVect(X, T, N) \
  { \
    for (int i = 0; i < N; i++) \
      (X)[i].~T(); \
    free((char *)X); \
  }

#define idbFreeIndVect(X, N) \
  { \
    for (int i = 0; i < N; i++) \
      delete (X)[i]; \
    free((char *)X); \
  }

#define IDB_CLASS_NAME_LEN 33
#define IDB_CLASS_NAME_OVERHEAD 1
#define IDB_CLASS_NAME_TOTAL_LEN (IDB_CLASS_NAME_LEN+IDB_CLASS_NAME_OVERHEAD)
#define IDB_CLASS_NAME_PAD (IDB_CLASS_NAME_LEN-8) // assert(sizeof(eyedbsm::Oid)==8)

#define IDB_NAME_OUT_PLACE ((char)1)
#define IDB_NAME_IN_PLACE  ((char)2)

extern FILE *get_file(Bool init = True);
extern std::ostream& convert_to_stream(std::ostream &);

#define IDB_LOCAL_CALL ((void *)0xff125341)

#define IDB_COLL_LOAD_DEFERRED() \
if (!is_complete) \
 { \
  Status __s = loadDeferred(); \
  if (__s) return __s; \
 }

extern Status post_etc_update(Database *);
extern Status
class_name_code(DbHandle *dbh, short dspid, Data *idr,
		    Offset *offset, Size *alloc_size, const char *name);

extern Status
class_name_decode(DbHandle *dbh, Data idr, Offset *offset,
		      char **name);
extern p_ProbeHandle *eyedb_probe_h;

#define IDB_CHECK_WRITE(DB) \
  if (!DB) \
    return Exception::make(IDB_ERROR, "no database associated with object"); \
  if (!((DB)->getOpenFlag() & _DBRW)) \
    return Exception::make(IDB_ERROR, "database is not opened for writing");

#define IDB_COLLCACHE_OPT1 // added 24/07/01
#define IDB_COLLMOD_OPT1   // added 24/07/01

#define CHK_VALID(O) \
  if (!(O)->isValidObject()) \
    return Exception::make(IDB_ERROR, "object %p is not a valid runtime object", O)

#define CHK_DAMAGED(O) \
if ((O)->getDamaged()) \
   return Exception::make(IDB_ERROR, \
          "attribute %s of object %p of class %s has been damaged " \
	  "during a prematured release", \
          (O)->getDamaged()->getName(), O, (O)->getClass()->getName())

#define CHK_OBJ(O) \
  CHK_VALID(O); \
  CHK_DAMAGED(O)

#define GBX_SUSPEND() gbxAutoGarbSuspender _gbxsusp_
//#define GBX_SUSPEND() gbxAutoGarb _gbxsusp_(gbxAutoGarb::SUSPEND)

#define AnyUserData ((void *)1)

//#define IDB_UNDEF_ENUM_HINT

#define IDB_NEWATTRNAT_VERSION 206002
//#define IDB_SKIP_HASH_XCOEF

#define IDB_IDX_MAGIC_HINTS 12
#define IDB_IDX_MAGIC_HINTS_VALUE 0x8efea341

#define IDB_ATTR_IS_STRING 0x8000

#define CHECK_INCSIZE(MTHNAME, INCSIZE, SZ)  \
  if ((INCSIZE) != (SZ)) { \
    std::cerr << name << "::" << MTHNAME << " size: " << INCSIZE << " vs. " << SZ << std::endl; \
    assert(0); \
  }

eyedbsm::DbHandle *get_eyedbsm_DbHandle(DbHandle *);

const char strict_unix_user[] = "**||STRICT||UNIX||USER||**";

}

// 8/07/06 and 15/08/06
#define DONT_SET_LITERAL_IN_NEWOBJ

#endif
