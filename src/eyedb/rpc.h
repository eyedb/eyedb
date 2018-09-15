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


#ifndef _eyedb_idbrpc_
#define _eyedb_idbrpc_

#include <eyedblib/rpc_lib.h>
#include "basetypes.h"

typedef enum {
  RPC_CONNSHM,

/*
  RPC_DBOPENLOCAL,
  RPC_DBOPEN,
  RPC_DBCLOSE,
  RPC_DBSETUID,
  RPC_GUESTUIDGET,
  RPC_OBJECTCREATE,
  RPC_OBJECTREAD,
  RPC_OBJECTWRITE,
  RPC_OBJECTDELETE,
  RPC_OBJECTSIZEGET,
  RPC_OBJECTSIZEMODIFY,
  RPC_TRANSACTIONBEGIN,
  RPC_TRANSACTIONCOMMIT,
  RPC_FIRSTOIDGET,
  RPC_NEXTOIDGET,
*/

  RPC_LASTCODE
} U_rpc_RpcCode;

typedef enum {
  rpc_IntType,
  rpc_pIntType,
  rpc_pChType,
  rpc_pDbhType,
  rpc_ppDbhType,
  rpc_StringType,
  rpc_DataType,
  rpc_pLocalDBContextType,

  /* specific idb */
  U_rpc_pBoolType,
  U_rpc_pOidType
} U_rpc_ArgType;

extern const void *rpc_ObjectNone;

extern void
U_rpc_init(),
U_rpc_rpcInit();

#endif

