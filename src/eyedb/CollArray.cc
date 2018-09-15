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

#define TEMP_COLL_ARRAY_READ_CACHE

// 15/08/06 : does not raise an exception when itemid in retrieveAt is
// out of range
#define NO_EXC_WHEN_OUT_OF_RANGE

/* CollArray */

namespace eyedb {

  void CollArray::init()
  {
    allow_dup = True;
    ordered = True;
    type = _CollArray_Type;

    read_arr_cache = new ValueCache(this);

    if (!status)
      setClass(CollArrayClass::make(coll_class, isref, dim, status));
  }

  CollArray::CollArray(const char *n, Class *_class,
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
		 _bottom, _top, _collimpl, _card, _is_literal, _is_pure_literal, _idx_data, _idx_data_size)
  {
    init();
    setClass(_class);
  }

  CollArray::CollArray(const char *n, Class *mc, Bool _isref,
		       const CollImpl *_collimpl) :
    Collection(n, mc, _isref, _collimpl)
  {
    init();
  }

  CollArray::CollArray(const char *n, Class *mc, int _dim,
		       const CollImpl *_collimpl) :
    Collection(n, mc, _dim, _collimpl)
  {
    init();
  }

  CollArray::CollArray(Database *_db, const char *n, Class *mc,
		       Bool _isref, const CollImpl *_collimpl) :
    Collection(n, mc, _isref, _collimpl)
  {
    init();
    if (!status)
      status = setDatabase(_db);
  }

  CollArray::CollArray(Database *_db, const char *n, Class *mc,
		       int _dim, const CollImpl *_collimpl ) :
    Collection(n, mc, _dim, _collimpl)
  {
    init();
    if (!status)
      status = setDatabase(_db);
  }

  CollArray::CollArray(const CollArray& o) : Collection(o)
  {
    allow_dup = True;
    ordered = True;
    type = _CollArray_Type;

    read_arr_cache = new ValueCache(this);
  }

  CollArray& CollArray::operator=(const CollArray& o)
  {
    *(Collection *)this = (Collection &)o;
    allow_dup = True;
    ordered = True;
    type = _CollArray_Type;

    delete read_arr_cache;
    read_arr_cache = new ValueCache(this);

    return *this;
  }

  const char *CollArray::getClassName() const
  {
    return CollArray_Class->getName();
  }

  Status CollArray::insert_p(const Oid &item_oid, Bool)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "cannot use non indexed insertion in an array");
  }

  Status CollArray::insert_p(const Object *, Bool)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "cannot use non indexed insertion in an array");
  }

  Status CollArray::insert_p(Data val, Bool, Size size)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "cannot use non indexed insertion in an array");
  }

  Status CollArray::insert(const Value &v, Bool)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "cannot use non indexed insertion in an array");
  }

  Status CollArray::suppress_p(const Oid &item_oid, Bool checkFirst)
  {
    if (status)
      return Exception::make(status);

    Bool found;
    Collection::ItemId where;
    Status s = isIn(item_oid, found, &where);
    if (s) return s;
    if (!found && checkFirst) return Success;
    return suppressAt(where);
  }

  Status CollArray::suppress_p(const Object *item_o, Bool checkFirst)
  {
    if (status)
      return Exception::make(status);

    Bool found;
    Collection::ItemId where;
    Status s = isIn(item_o, found, &where);
    if (s) return s;
    if (!found && checkFirst) return Success;
    return suppressAt(where);
  }

  Status CollArray::suppress(const Value &item_value, Bool checkFirst)
  {
    if (status)
      return Exception::make(status);

    Bool found;
    Collection::ItemId where;
    Status s = isIn(item_value, found, &where);
    if (s) return s;
    if (!found && checkFirst) return Success;
    return suppressAt(where);
  }

  Status CollArray::suppress_p(Data, Bool, Size size)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "cannot use non indexed suppression in an array");
  }

  Status CollArray::insertAt_p(Collection::ItemId id, const Oid &item_oid)
  {
    if (status)
      return Exception::make(status);

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' [%s] is locked for writing", name, oid.getString());

    if (item_oid.isValid()) {
      Status s = check(item_oid, IDB_COLLECTION_INSERT_ERROR);
      if (s) return s;
    }

    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    create_cache();

    if (item = cache->get(id))
      cache->suppress(item);
    else
      v_items_cnt++;

    cache->insert(item_oid, id, added);

    if (id >= top)
      top = id+1;

    return Success;
  }

  Status CollArray::insertAt_p(Collection::ItemId id, const Object *item_o)
  {
    if (status)
      return Exception::make(status);

    if (!isref) {
      Status s = check(item_o, IDB_COLLECTION_INSERT_ERROR);
      if (s) return s;
      if (!item_o->getIDR())
	return Exception::make(IDB_COLLECTION_INSERT_ERROR, "%s object IDR is not allocated", item_o->getClass()->getName());
      return insertAt_p(id, item_o->getIDR() + IDB_OBJ_HEAD_SIZE);
    }

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' [%s] is locked for writing", name, oid.getString());

    if (status)
      return Exception::make(status);

    Status s = check(item_o, IDB_COLLECTION_INSERT_ERROR);
    if (s) return s;

    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    create_cache();

    if (item = cache->get(id))
      cache->suppress(item);
    else
      v_items_cnt++;

    cache->insert(item_o, id, added);

    if (id >= top)
      top = id+1;

    return Success;
  }

  Status CollArray::insertAt_p(Collection::ItemId id, Data val, Size size)
  {
    if (status)
      return Exception::make(status);

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' [%s] is locked for writing", name, oid.getString());

    Status s = check(val, size, IDB_COLLECTION_INSERT_ERROR);
    if (s) return s;

    IDB_COLL_LOAD_DEFERRED();
    touch();
    Data item_data = make_data(val, size, True);
 
    ValueItem *item;

    create_cache();

    if (item = cache->get(id))
      cache->suppress(item);
    else
      v_items_cnt++;

    cache->insert(Value(item_data, item_size), id, added);

    if (id >= top)
      top = id+1;

    return Success;
  }

  Status CollArray::insertAt(Collection::ItemId id, const Value &v)
  {
    Status s = check(v, IDB_COLLECTION_INSERT_ERROR);
    if (s)
      return s;

    if (v.type == Value::tObject)
      return insertAt_p(id, v.o);

    if (v.type == Value::tObjectPtr)
      return insertAt_p(id, v.o_ptr->getObject());

    if (v.type == Value::tOid)
      return insertAt_p(id, Oid(*v.oid));

    Size size;
    Data data = v.getData(&size);

    return insertAt_p(id, data, size);
  }

  Status CollArray::suppressAt(Collection::ItemId id)
  {
    if (status)
      return Exception::make(status);

    ValueItem *item;
    IDB_COLL_LOAD_DEFERRED();
    touch();
    if (cache && (item = cache->get(id))) {
      int s = item->getState();
      if (s == removed)
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "item index %d is already suppressed", id);
      else if (s == coherent)
	item->setState(removed);
      else if (s == added) {
	if (isref) {
	  cache->suppressOid(item);
	  cache->suppressObject(item);
	}
	else
	  cache->suppressData(item);
      }

      v_items_cnt--;
      if (top == id+1)
	top--;
      return Success;
    }
      
    int found = 0;
      
    unsigned char *item_data = (unsigned char *)malloc(item_size);

    if (getOidC().isValid()) {
      RPCStatus rpc_status;
      rpc_status = collectionGetByInd(db->getDbHandle(), getOidC().getOid(),
				      id, &found, item_data, item_size);
      if (rpc_status) {
	free(item_data);
	return StatusMake(rpc_status);
      }
    }

    if (!found) {
      free(item_data);
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "no item found at index %d in collection '%s' [%s]", id, name, oid.getString());
    }

    create_cache();
    if (isref) {
      Oid item_oid;
      Offset offset = 0;
      oid_decode(item_data, &offset, item_oid.getOid());
      //memcpy(&item_oid, item_data, sizeof(Oid));
      cache->insert(item_oid, id, removed);
    }
    else
      cache->insert(Value(item_data, item_size), id, removed);

    v_items_cnt--;

    free(item_data);
    return Success;
  }

  Status CollArray::retrieveAt(Collection::ItemId id, Oid &item_oid) const
  {
    if (status)
      return Exception::make(status);

#ifdef NO_EXC_WHEN_OUT_OF_RANGE
    if (id < getBottom() || id >= getTop()) {
      item_oid.invalidate();
      return Success;
    }
#else
    if (id < getBottom() || id >= getTop())
      return Exception::make(IDB_COLLECTION_ERROR,
			     "index out of range #%d for collection array",
			     id, oid.toString());
#endif
    ValueItem *item;

    // should take read_arr_cache into account and merge cache and read_arr_cache
    if (cache && (item = cache->get(id))) {
      if (item->getState() == added) {
	if (item->getValue().type == Value::tOid)
	  item_oid = *item->getValue().oid;
	else if (item->getValue().type == Value::tObject)
	  item_oid = item->getValue().o->getOid();
	else if (item->getValue().type == Value::tObjectPtr)
	  item_oid = item->getValue().o_ptr->getObject()->getOid();
	else
	  item_oid = Oid::nullOid;
      }
      else
	item_oid = Oid::nullOid;

      decode((Data)item_oid.getOid());
      return Success;
    }
      
    if (!getOidC().isValid()) {
      item_oid.invalidate();
      return Success;
    }

    int found;
    RPCStatus rpc_status;

    rpc_status = collectionGetByInd(db->getDbHandle(), getOidC().getOid(),
					id, &found, (Data)item_oid.getOid(),
					sizeof(eyedbsm::Oid));

    if (rpc_status)
      return StatusMake(rpc_status);

    if (found)
      decode((Data)item_oid.getOid());
    else
      item_oid.invalidate();

    //return StatusMake(IDB_COLLECTION_IS_IN_ERROR, rpc_status);
    return Success;
  }

  Status CollArray::retrieveAt(Collection::ItemId id, Object* &o, const RecMode *rcm) const
  {
    if (status)
      return Exception::make(status);

#ifdef NO_EXC_WHEN_OUT_OF_RANGE
    if (id < getBottom() || id >= getTop()) {
      o = 0;
      return Success;
    }
#else
    if (id < getBottom() || id >= getTop())
      return Exception::make(IDB_COLLECTION_ERROR,
			     "index out of range #%d for collection array",
			     id, oid.toString());
#endif

    ValueItem *item;
    Oid item_oid;

#ifdef TEMP_COLL_ARRAY_READ_CACHE
    item = read_arr_cache->get(id);
    if (!item && cache)
      item = cache->get(id);
    if (item) {
#else
    if (cache && (item = cache->get(id))) {
#endif
      if (item->getState() == added) {
	if (item->getValue().type == Value::tObject)
	  o = item->getValue().o;
	else if (item->getValue().type == Value::tObjectPtr)
	  o = item->getValue().o_ptr->getObject();
	else
	  o = 0;

	if (o)
	  return Success;
	
	if (item->getValue().type == Value::tOid)
	  item_oid = *item->getValue().oid;
	else
	  item_oid = Oid::nullOid;
	
	decode((Data)item_oid.getOid());

	if (item_oid.isValid())	{
	  if (db)
	    return db->loadObject(item_oid, o, rcm);
	  return Exception::make(IDB_COLLECTION_ERROR,
				 "database is not set in collection");
	}
      }
      
      item_oid.invalidate();
      return Success;
    }
      
    int found = 0;

    if (getOidC().isValid()) {
      RPCStatus rpc_status;
      rpc_status = collectionGetByInd(db->getDbHandle(), getOidC().getOid(),
				      id, &found, (Data)item_oid.getOid(),
				      sizeof(eyedbsm::Oid));

      if (rpc_status)
	return StatusMake(rpc_status);
    }

    if (found)
      decode((Data)item_oid.getOid());
    else {
      o = 0;
      return Success;
    }

    if (db) {
      Status s = db->loadObject(item_oid, o, rcm);
#ifdef TEMP_COLL_ARRAY_READ_CACHE
      if (!s && o) {
	read_arr_cache->insert(o, id, added);
	o->decrRefCount();
      }
#endif
      return s;
    }

    return Exception::make(IDB_COLLECTION_ERROR,
			   "database is not set in collection");
  }

  Status CollArray::retrieveAt_p(Collection::ItemId id, Data data, Size size) const
  {
    if (status)
      return Exception::make(status);

#ifdef NO_EXC_WHEN_OUT_OF_RANGE
    if (id < getBottom() || id >= getTop()) {
      memset(data, 0, size);
      return Success;
    }
#else
    if (id < getBottom() || id >= getTop())
      return Exception::make(IDB_COLLECTION_ERROR,
			     "index out of range #%d for collection array",
			     id, oid.toString());
#endif

    if (size == defaultSize)
      size = item_size;

    if (size < 0 || size > item_size)
      return Exception::make(IDB_COLLECTION_ERROR, "data too long for collection search");

    ValueItem *item;

    if (cache && (item = cache->get(id)) && item->getState() != removed) {
      memcpy(data, item->getValue().getData(), size);

      decode(data);

      return Success;
    }
      
    if (!getOidC().isValid()) {
      memset(data, 0, size);
      return Success;
    }

    int found;
      
    RPCStatus rpc_status;

    size = (size = defaultSize ? item_size : size);
    rpc_status = collectionGetByInd(db->getDbHandle(), getOidC().getOid(),
					id, &found, (Data)data, size);
  
    if (found)
      decode(data);
    else
      memset(data, 0, size);

    return StatusMake(IDB_COLLECTION_IS_IN_ERROR, rpc_status);
  }

  Status CollArray::retrieveAt(Collection::ItemId id, Value &v) const
  {
    if (isref) {
      Oid _oid;
      Status s = retrieveAt(id, _oid);
      if (!s)
	v.set(_oid);
      return s;
    }

    if (!isref && !coll_class->asBasicClass() && !coll_class->asEnumClass()) {
      Object *o;
      Status s = retrieveAt(id, o);
      if (!s)
	v.set(o);
      return s;
    }

    if (string_coll) {
      char *str = new char[item_size];
      Status s = retrieveAt_p(id, (Data)str, item_size);
      if (!s)
	v.set(str);
      delete [] str;
      return s;
    }

    if (coll_class->asByteClass() && dim > 1) {
      unsigned char *data = new unsigned char[item_size];
      Status s = retrieveAt_p(id, data, item_size);
      if (!s)
	v.set(data, item_size);
      else
	delete [] data;
      return s;
    }

    if (coll_class->asCharClass()) {
      char c;
      Status s = retrieveAt_p(id, (Data)&c, item_size);
      if (!s)
	v.set(c);
      return s;
    }

    if (coll_class->asInt16Class()) {
      eyedblib::int16 i16;
      Status s = retrieveAt_p(id, (Data)&i16, item_size);
      if (!s)
	v.set(i16);
      return s;
    }

    if (coll_class->asInt32Class()) {
      eyedblib::int32 i32;
      Status s = retrieveAt_p(id, (Data)&i32, item_size);
      if (!s)
	v.set(i32);
      return s;
    }

    if (coll_class->asInt64Class()) {
      eyedblib::int64 i64;
      Status s = retrieveAt_p(id, (Data)&i64, item_size);
      if (!s)
	v.set(i64);
      return s;
    }

    if (coll_class->asFloatClass()) {
      double d;
      Status s = retrieveAt_p(id, (Data)&d, item_size);
      if (!s)
	v.set(d);
      return s;
    }

    return Exception::make(IDB_COLLECTION_ERROR, "invalid collection type");
  }

  Status CollArray::append_p(const Oid &item_oid, Bool noDup)
  {
    if (status)
      return Exception::make(status);

    if (!noDup)
      return insertAt_p(getTop(), item_oid);

    Bool found;
    Status s = isIn_p(item_oid, found);
    if (found)
      return Success;

    return insertAt_p(getTop(), item_oid);
  }

  Status CollArray::append_p(const Object *item_o, Bool noDup)
  {
    if (!noDup)
      return insertAt_p(getTop(), item_o);

    Bool found;
    Status s = isIn(item_o, found);
    if (found)
      return Success;

    return insertAt_p(getTop(), item_o);
  }

  Status CollArray::append_p(Data item_val, Bool noDup, Size size)
  {
    if (status)
      return Exception::make(status);

    if (!noDup)
      return insertAt_p(getTop(), item_val, size);

    Bool found;
    Status s = isIn_p(item_val, found, size);
    if (found)
      return Success;

    return insertAt_p(getTop(), item_val, size);
  }

  Status CollArray::append(const Value &v)
  {
    return insertAt(getTop(), v);
    /*
      if (status)
      return Exception::make(status);

      if (!noDup)
      return insertAt(getTop(), item_val);

      Bool found;
      Status s = isIn(item_val, found);
      if (found)
      return Success;

      return insertAt(getTop(), item_val);
    */
  }

  Status
  CollArray::getImplStats(std::string &xstats1, std::string &xstats2,
			  Bool dspImpl, Bool full,
			  const char *indent)
  {
    if (status)
      return Exception::make(status);

    IndexStats *stats1, *stats2;
    Status s = getImplStats(stats1, stats2);
    if (s) return s;
    xstats1 = (stats1 ? stats1->toString(dspImpl, full, indent) : std::string(""));
    xstats2 = (stats2 ? stats2->toString(dspImpl, full, indent) : std::string(""));
    delete stats1;
    delete stats2;
    return Success;
  }

  Status
  CollArray::getImplStats(IndexStats *&stats1, IndexStats *&stats2)
  {
    if (status)
      return Exception::make(status);

    RPCStatus rpc_status;

    Oid idx1oid, idx2oid;
    Status s = getIdxOid(idx1oid, idx2oid);
    if (s) return s;

    stats1 = stats2 = 0;
    Oid idxoids[] = {idx1oid, idx2oid};
    IndexStats **stats[] = {&stats1, &stats2};

    for (int n = 0; n < sizeof(idxoids)/sizeof(idxoids[0]); n++) {
      Oid xoid = idxoids[n];
      if (xoid.isValid()) {
	rpc_status = collectionGetImplStats
	  (db->getDbHandle(), collimpl->getType(),
	   xoid.getOid(), (Data *)stats[n]);
	if (rpc_status) return StatusMake(rpc_status);
	completeImplStats(*stats[n]);
      }
    }

    return Success;
  }

  Status
  CollArray::simulate(const CollImpl &_collimpl,
		      std::string &xstats1, std::string &xstats2,
		      Bool dspImpl, Bool full, const char *indent)
  {
    IndexStats *stats1, *stats2;
    Status s = simulate(_collimpl, stats1, stats2);
    if (s) return s;
    xstats1 = (stats1 ? stats1->toString(dspImpl, full, indent) : std::string(""));
    xstats2 = (stats2 ? stats2->toString(dspImpl, full, indent) : std::string(""));
    delete stats1;
    delete stats2;
    return Success;
  }

  Status
  CollArray::simulate(const CollImpl &_collimpl,
		      IndexStats *&stats1, IndexStats *&stats2)
  {
    Oid idx1oid, idx2oid;
    Status s = getIdxOid(idx1oid, idx2oid);
    if (s) return s;

    stats1 = stats2 = 0;
    RPCStatus rpc_status;
    Oid idxoids[] = {idx1oid, idx2oid};
    IndexStats **stats[] = {&stats1, &stats2};
    for (int n = 0; n < sizeof(idxoids)/sizeof(idxoids[0]); n++) {
      Oid xoid = idxoids[n];
      if (xoid.isValid()) {
	Data data;
	Offset offset = 0;
	Size size = 0;
	Status s = IndexImpl::code(data, offset, size, _collimpl.getIndexImpl(), _collimpl.getType());
	if (s) return s;
	rpc_status =
	  collectionSimulImplStats(db->getDbHandle(), _collimpl.getType(),
				       xoid.getOid(), data, size,
				       (Data *)*stats[n]);
	if (rpc_status)
	  return StatusMake(rpc_status);
      }
    }

    return Success;
  }

  void CollArray::garbage() 
  {
    delete read_arr_cache;
    Collection::garbage();
  }
}

