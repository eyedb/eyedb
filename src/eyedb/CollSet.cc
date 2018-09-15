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
#include "ValueCache.h"
#include <assert.h>
#include "AttrNative.h"

/* CollSet */

namespace eyedb {

  void CollSet::init()
  {
    allow_dup = False;
    ordered = False;
    type = _CollSet_Type;
    if (!status)
      setClass(CollSetClass::make(coll_class, isref, dim, status));
  }

  CollSet::CollSet(const char *n, Class *_class,
		   const Oid& _idx1_oid,
		   const Oid& _idx2_oid,
		   int icnt,
		   int _bottom, int _top,
		   const CollImpl *_collimpl,
		   Object *_card,
		   Bool _is_literal,
		   Bool _is_pure_literal,
		   Data _idx_data, Size _idx_data_size)
    : Collection(n, _class, _idx1_oid, _idx2_oid, icnt,
		 _bottom, _top, _collimpl, _card, _is_literal, _is_pure_literal, _idx_data,
		 _idx_data_size)
  {
    init();
    setClass(_class);
  }

  CollSet::CollSet(const char *n, Class *_coll_class, Bool _isref,
		   const CollImpl *_collimpl) :
    Collection(n, _coll_class, _isref, _collimpl)
  {
    init();
  }

  CollSet::CollSet(const char *n, Class *_coll_class, int _dim,
		   const CollImpl *_collimpl) :
    Collection(n, _coll_class, _dim, _collimpl)
  {
    init();
  }

  CollSet::CollSet(Database *_db, const char *n, Class *_coll_class, Bool _isref, const CollImpl *_collimpl) 
    : Collection(n, _coll_class, _isref, _collimpl)
  {
    init();
    if (!status)
      status = setDatabase(_db);
  }

  CollSet::CollSet(Database *_db, const char *n, Class *_coll_class, int _dim, const CollImpl *_collimpl) :
    Collection(n, _coll_class, _dim, _collimpl)
  {
    init();
    if (!status)
      status = setDatabase(_db);
  }

  CollSet::CollSet(const CollSet& o) : Collection(o)
  {
    allow_dup = False;
    ordered = False;
    type = _CollSet_Type;
  }

  CollSet& CollSet::operator=(const CollSet& o)
  {
    *(Collection *)this = (Collection &)o;
    allow_dup = False;
    ordered = False;
    type = _CollSet_Type;
    return *this;
  }

  const char *CollSet::getClassName() const
  {
    return CollSet_Class->getName();
  }

  Status CollSet::insert_p(const Oid &item_oid, Bool noDup)
  {
    if (status)
      return Exception::make(status);
    /*
      1/ on regarde si oid est present dans la cache: si oui -> return error
      2/ si la collection est realize'e (oid valid) on fait:
      collectionGetByValue() -> si found -> return error
      3/ on insere dans le cache avec le state `added'
      v_items_cnt++;
    */

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' is locked for writing", name);

    Status s = check(item_oid, IDB_COLLECTION_INSERT_ERROR);
    if (s) return s;

    ValueItem *item;

    IDB_COLL_LOAD_DEFERRED();
    touch();

    if (cache && (item = cache->get(item_oid)))
	{
	  if (item->getState() != removed)
	    {
	      if (noDup)
		return Success;
	      return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "item '%s' is already in collection", item_oid.getString());
	    }
      

	  item->setState(added);
	  v_items_cnt++;
	  return Success;
	}

    if (getOidC().isValid())
      {
	int found, ind;
      
	RPCStatus rpc_status;

	if ((rpc_status = collectionGetByOid(db->getDbHandle(),
						 getOidC().getOid(),
						 item_oid.getOid(), &found, &ind)) ==
	    RPCSuccess)
	  {
	    if (found)
	      {
		if (noDup)
		  return Success;

		return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "item '%s' is already in collection", item_oid.getString());
	      }
	  }
	else
	  {
	    return StatusMake(IDB_COLLECTION_INSERT_ERROR, rpc_status);
	  }
      }

    create_cache();
    //    cache->insert(item_oid, v_items_cnt, added);
    cache->insert(item_oid, ValueCache::DefaultItemID, added);
    v_items_cnt++;

    return Success;
  }

  Status CollSet::insert_p(const Object *item_o, Bool noDup)
  {
    if (status)
      return Exception::make(status);
    /*
      1/ on regarde si o est present dans la cache: si oui -> return error
      2/ o->getOid() est valid, on regarde si presen dans la cache: si oui ->
      return error
      3/ si o->getOid() est valid ET si la collection est realize'e (oid 
      valid) on fait:
      collectionGetByOid() -> si found -> return error
      4/ on insere dans le cache avec le state `added'
      v_items_cnt++;
    */

    if (!isref) {
      Status s = check(item_o, IDB_COLLECTION_INSERT_ERROR);
      if (s) return s;
      return insert_p(item_o->getIDR() + IDB_OBJ_HEAD_SIZE, noDup);
    }

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' is locked for writing", name);

    Status s = check(item_o, IDB_COLLECTION_INSERT_ERROR);
    if (s) return s;

    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    if (cache && (item = cache->get(item_o))){
      if (item->getState() != removed) {
	if (noDup)
	  return Success;

	return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "item 0x%x is already in the collection cache", item_o);
      }
      
#if 0
      cache->suppressObject(item);
      cache->insert(item_o, item->getInd(), added);
#else
      item->setState(added);
#endif
      v_items_cnt++;
      return Success;
    }

    Oid item_oid = item_o->getOid();
    Bool valid = item_oid.isValid();

    if (valid && cache && (item = cache->get(item_oid)))
	{
	  if (noDup)
	    return Success;
	  return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "item '%s' is already in collection", item_oid.getString());
	}

    if (valid && getOidC().isValid())
      {
	int found, ind;
  
	RPCStatus rpc_status;
      
	if ((rpc_status = collectionGetByOid(db->getDbHandle(), getOidC().getOid(),
						 item_oid.getOid(), &found, &ind)) ==
	    RPCSuccess)
	  {
	    if (found)
	      {
		if (noDup)
		  return Success;
		return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "item '%s' is already in collection '%s'", item_oid.getString(), name);
	      }
	  }	  
	else
	  return StatusMake(IDB_COLLECTION_INSERT_ERROR, rpc_status);
      }

    create_cache();
    //    cache->insert(item_o, v_items_cnt, added);
    cache->insert(item_o, ValueCache::DefaultItemID, added);
    v_items_cnt++;

    return Success;
  }

  Status CollSet::insert_p(Data val, Bool noDup, Size size)
  {
    if (status)
      return Exception::make(status);

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' is locked for writing", name);

    Status s = check(val, size, IDB_COLLECTION_INSERT_ERROR);
    if (s) return s;

    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    Data item_data = make_data(val, size, True);

    if (!item_data)
      return Exception::make(IDB_COLLECTION_ERROR, "data too long for collection insertion");

    if (cache && (item = cache->get(item_data, item_size))) {
      int s = item->getState();
      if (s != removed) {
	if (noDup)
	  return Success;
	return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "value is already in the cache");
      }
      
      item->setState(added);
      v_items_cnt++;
      return Success;
    }

    if (getOidC().isValid()) {
      int found, ind;
	
      RPCStatus rpc_status;

      if ((rpc_status = collectionGetByValue(db->getDbHandle(), getOidC().getOid(),
						 item_data, item_size, &found,
						 &ind)) ==
	  RPCSuccess)
	{
	  if (found) {
	    if (noDup)
	      return Success;
	    return Exception::make(IDB_COLLECTION_DUPLICATE_INSERT_ERROR, "value is already in the collection");
	  }
	}
      else
	return StatusMake(IDB_COLLECTION_INSERT_ERROR, rpc_status);
      }

    create_cache();
    //    cache->insert(Value(item_data, item_size), v_items_cnt, added);
    cache->insert(Value(item_data, item_size), ValueCache::DefaultItemID, added);
    v_items_cnt++;

    return Success;
  }
}
