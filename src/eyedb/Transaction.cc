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


#include "eyedb_p.h"
#include "DBM_Database.h"
#include "IteratorBE.h"
#include "OQLBE.h"
#include "BEQueue.h"
#include "kernel.h"
#include "db_p.h"

namespace eyedb {

static const int defMagOrder = 10240;

Transaction::Transaction(Database *_db,
			       const TransactionParams &_params) :
  db(_db), params(_params), incoherency(False)
{
  cacheInit();
  cache_on = True;
}

void
Transaction::cacheInit()
{
  int n = (params.magorder >= defMagOrder ? params.magorder : defMagOrder) / 20;
  int p = 1;

  for (;;)
    {
      if (p >= n)
	break;
      p <<= 1;
    }

  cache = new ObjCache(p);
}

Status Transaction::setParams(const TransactionParams &_params)
{
  Status s;

  if (s = checkParams(_params))
    return s;

  params = _params;

  RPCStatus rpc_status;
  if ((rpc_status = transactionParamsSet(db->getDbHandle(), &params)) !=
      RPCSuccess)
    return StatusMake(rpc_status);

  return Success;
}

Status Transaction::begin()
{
  RPCStatus rpc_status;

  if ((rpc_status = transactionBegin(db->getDbHandle(),
					 &params,
					 &tid)) != RPCSuccess)
    return StatusMake(rpc_status);

  return Success;
}

Status Transaction::commit()
{
  RPCStatus rpc_status;

  if (incoherency)
    {
      //this->abort();
      //incoherency = False;
      return Exception::make(IDB_ERROR, "current transaction could not "
				"committed because of an incoherency state: "
				"must abort current transaction.");
    }


  //printf("Transaction::commit()\n");
  if ((rpc_status = transactionCommit(db->getDbHandle(), tid)) != RPCSuccess)
    return StatusMake(rpc_status);

  return Success;
}

Status Transaction::abort()
{
  RPCStatus rpc_status;

  if ((rpc_status = transactionAbort(db->getDbHandle(), tid)) != RPCSuccess)
    return StatusMake(rpc_status);

  incoherency = False;
  if (db->getSchema()->isModify())
    {
      begin();
      db->getSchema()->purge();
      commit();
    }

  return Success;
}
	       
Status Transaction::checkParams(const TransactionParams &params,
				      Bool strict)
{
  if (params.trsmode != TransactionOff &&
      params.trsmode != TransactionOn)
    return Exception::make(IDB_INVALID_TRANSACTION_PARAMS,
			      "invalid transaction mode %d", params.trsmode);

  if (params.lockmode != ReadSWriteS &&
      params.lockmode != ReadSWriteSX &&
      params.lockmode != ReadSWriteX &&
      params.lockmode != ReadSXWriteSX &&
      params.lockmode != ReadSXWriteX &&
      params.lockmode != ReadXWriteX &&
      params.lockmode != ReadNWriteS &&
      params.lockmode != ReadNWriteSX &&
      params.lockmode != ReadNWriteX &&
      params.lockmode != ReadNWriteN &&
      params.lockmode != DatabaseW &&
      params.lockmode != DatabaseRW &&
      params.lockmode != DatabaseWtrans)
    return Exception::make(IDB_INVALID_TRANSACTION_PARAMS,
			      "invalid lock mode %d", params.lockmode);

  if (params.recovmode != RecoveryOff &&
      params.recovmode != RecoveryFull &&
      params.recovmode != RecoveryPartial)
    return Exception::make(IDB_INVALID_TRANSACTION_PARAMS,
			      "invalid recovery mode %d", params.recovmode);
  return Success;
}

void Transaction::cacheObject(const Oid &oid, Object *o)
{
  if (cache_on && o && o->getOid().isValid())
    cache->insertObject(oid, o);
}

void Transaction::uncacheObject(Object *o)
{
  if (o && o->getOid().isValid())
    cache->deleteObject(o->getOid());
}

void Transaction::uncacheObject(const Oid &_oid)
{
  cache->deleteObject(_oid);
}

Status
Transaction::cacheInvalidate()
{
  delete cache;
  cacheInit();
  return Success;
}

Status
Transaction::setCacheActive(Bool on_off)
{
  cache_on = on_off;
  return Success;
}

void
Transaction::setIncoherency()
{
  incoherency = True;
}

Transaction::~Transaction()
{
  delete cache;
}
}
