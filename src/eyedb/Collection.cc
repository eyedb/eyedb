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


// 15/08/06 : avoid use of exists() method
#define NEW_GET_ELEMENTS

#include "eyedb_p.h"
#include "ValueCache.h"
#include "CollectionBE.h"
#include <assert.h>
#include "AttrNative.h"
#include "Attribute_p.h"

#define NEW_MASTER_OBJ

#define TRY_GETELEMS_GC

//#define INIT_IDR

static bool eyedb_support_stack = getenv("EYEDB_SUPPORT_STACK") ? true : false;

using namespace std;

namespace eyedb {

  static Offset LITERAL_OFFSET;

  extern int
  get_item_size(const Class *coll_class, int dim);

  static Bool collupd_noopt = getenv("EYEDB_COLLUPD_NOOPT") ? True : False;

#define CACHE_LIST(C)       ((C) ? (C)->getList() : (LinkedList *)0)
#define CACHE_LIST_COUNT(C) ((C) ? (C)->getIdMap().size() : 0)

  // WARNING: the optimisation for create makes a problem in the
  // inverses! => 
#define COLL_OPTIM_CREATE
#define COLL_OPTIM_LOAD

#define COLL_REENTRANT
  //#define COLL_SIZE_MAX     0x400

  //#define COLLTRACE

  static int COLLIMPL_DEFAULT = (int)CollImpl::HashIndex;

  const Size Collection::defaultSize = (Size)-1;

  //
  // Collection init method
  //

  static inline IndexImpl *getDefaultIndexImpl()
  {
    static IndexImpl *defIndexImpl;
    if (!defIndexImpl) {
      defIndexImpl = new IndexImpl(IndexImpl::Hash, 0, 0, 0, 0, 0);
    }
    return defIndexImpl;
  }

  static inline CollImpl *getDefaultCollImpl(int impl_type)
  {
    static CollImpl *defCollImpl_noindex;
    static CollImpl *defCollImpl_hashindex;
    if (impl_type == CollImpl::NoIndex) {
      if (!defCollImpl_noindex) {
	defCollImpl_noindex = new CollImpl(CollImpl::NoIndex);
      }
      return defCollImpl_noindex;
    }

    if (!defCollImpl_hashindex) {
      defCollImpl_hashindex = new CollImpl(getDefaultIndexImpl());
    }
    return defCollImpl_hashindex;
  }

  void Collection::_init(const CollImpl *_collimpl)
  {
    if (isref || !coll_class)
      item_size = sizeof(Oid);
    else
      item_size = get_item_size(coll_class, dim);

    Exception::Mode mode = Exception::setMode(Exception::StatusMode);

#if 0
    if (item_size > COLL_SIZE_MAX)
      status = Exception::make(IDB_COLLECTION_ITEM_SIZE_TOO_LARGE,
			       "collection '%s', item size is '%d', "
			       "max size is '%d'",
			       name, item_size, COLL_SIZE_MAX);
    else
#endif
      if (item_size == 0)
	status = Exception::make(IDB_COLLECTION_ITEM_SIZE_UNKNOWN,
				 "collection '%s'", name);
      else
	status = Success;

    Exception::setMode(mode);

    cache = 0;

    read_cache.obj_arr = 0;
    read_cache.oid_arr = 0;
    read_cache.val_arr = 0;

    read_cache_state_oid = read_cache_state_value = read_cache_state_object = coherent;
    read_cache_state_index = False;
    locked = False;
    if (!_collimpl) {
      _collimpl = getDefaultCollImpl(COLLIMPL_DEFAULT);
    }

    collimpl = _collimpl->clone();
    isStringColl();
    card = NULL;
    card_oid.invalidate();
    bottom = top = 0;
    inv_oid.invalidate();
    inv_item = 0;
    inverse_valid = True;
    literal_oid.invalidate();
    is_literal = False;
    is_pure_literal = False;
    is_complete = True;
    idx_data = 0;
    idx_data_size = 0;
    implModified = False;
    nameModified = False;
#ifdef INIT_IDR
    inv_oid_offset = 0;
    init_idr();
#endif
  }

  void Collection::unvalidReadCache()
  {
    read_cache_state_oid = read_cache_state_object = read_cache_state_value = modified;
    read_cache_state_index = False;
  }

  void Collection::create_cache()
  {
    if (!cache)
      cache = new ValueCache(this);
  }

  void Collection::isStringColl()
  {
    if (!strcmp(coll_class->getName(), char_class_name) && dim > 1)
      string_coll = True;
    else
      string_coll = False;
  }

  void Collection::decode(Data data) const
  {
    Offset offset = 0;
    if (isref) {
      eyedbsm::Oid smoid;
      oid_decode(data, &offset, &smoid);
      memcpy(data, &smoid, sizeof(smoid));
    }
    else if (coll_class->asInt16Class()) {
      eyedblib::int16 i16;
      int16_decode(data, &offset, &i16);
      memcpy(data, &i16, sizeof(i16));
    }
    else if (coll_class->asInt32Class()) {
      eyedblib::int32 i32;
      int32_decode(data, &offset, &i32);
      memcpy(data, &i32, sizeof(i32));
    }
    else if (coll_class->asInt64Class()) {
      eyedblib::int64 i64;
      int64_decode(data, &offset, &i64);
      memcpy(data, &i64, sizeof(i64));
    }
    else if (coll_class->asFloatClass()) {
      double d;
      double_decode(data, &offset, &d);
      memcpy(data, &d, sizeof(d));
    }
  }

  Data Collection::make_data(Data data, Size size, Bool swap) const
  {
    if (size == defaultSize)
      size = item_size;

    if (size < 0 || size > item_size)
      return 0;

    Data item_data;

    item_data = (unsigned char *)malloc(item_size);

    if (string_coll) {
      int len = strlen((char *)data);

      if (len >= item_size) {
	free(item_data);
	return 0;
      }
      
      strcpy((char *)item_data, (char *)data);
      memset((char *)item_data+len, 0, item_size-len);
    }
    else if (swap) {
      if (isref)
	oid_code(item_data, data);
      else if (coll_class->asInt16Class())
	int16_code(item_data, data);
      else if (coll_class->asInt32Class())
	int32_code(item_data, data);
      else if (coll_class->asInt64Class())
	int64_code(item_data, data);
      else if (coll_class->asFloatClass())
	double_code(item_data, data);
      else { // byte class
	memcpy(item_data, data, size);
	
	if (size < item_size)
	  memset(item_data+size, 0, item_size-size);
      }
    }
    else {
      memcpy(item_data, data, size);

      if (size < item_size)
	memset(item_data+size, 0, item_size-size);
    }

    return item_data;
  }

  Status Collection::realizeInverse(const Oid& _inv_oid, int _inv_item)
  {
    setInverse(_inv_oid, _inv_item);

    //  printf("realizeInverse(%s, %p)\n", inv_oid.toString(), inv_item);
    if (!inv_oid.isValid())
      return Success;

    if (status != Success)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (!getOidC().isValid())
      return Exception::make(IDB_COLLECTION_ERROR, "collection oid '%s' is not valid", name);

    Data temp = 0;
    Offset offset = IDB_OBJ_HEAD_SIZE;
    Size alloc_size = 0;

    oid_code(&temp, &offset, &alloc_size, inv_oid.getOid());
    int16_code(&temp, &offset, &alloc_size, &inv_item);

    ObjectHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.type = type;
    hdr.size = alloc_size;
    hdr.xinfo = IDB_XINFO_INV;

    offset = 0;

    object_header_code(&temp, &offset, &alloc_size, &hdr);

    RPCStatus rpc_status;

    rpc_status = objectWrite(db->getDbHandle(), temp, getOidC().getOid());

    free(temp);

    return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
  }

  void Collection::setInverse(const Oid& _inv_oid, int _inv_item)
  {
    //  printf("Collection::setInverse(%s)\n", _inv_oid.toString());
    inv_oid = _inv_oid;
    inv_item = _inv_item;
  }

  void Collection::setCardinalityConstraint(Object *_card)
  {
#ifdef CARD_TRACE
    printf("Collection::setCardinalityConstraint(%p)\n", _card);
#endif
    if (!_card) {
      card = NULL;
      card_oid.invalidate();
      return;
    }

    if (!strcmp(_card->getClass()->getName(), "cardinality_constraint")) {
      card = ((CardinalityConstraint *)_card)->getCardDesc();
      if (!card) {
	card_oid.invalidate();
	return;
      }
    }
    else
      card = (CardinalityDescription *)_card;
    
    card_oid = _card->getOid();
    card_bottom = card->getBottom();
    card_bottom_excl = card->getBottomExcl();
    card_top = card->getTop();
    card_top_excl = card->getTopExcl();
  }

  CardinalityDescription *Collection::getCardinalityConstraint() const
  {
    return card;
  }

  //
  // Collection constructors
  //

  Collection::Collection(const char *n, Class *_class,
			 const Oid& _idx1_oid,
			 const Oid& _idx2_oid,
			 int icnt,
			 int _bottom, int _top,
			 const CollImpl *_collimpl,
			 Object *_card, Bool _is_literal, Bool _is_pure_literal,
			 Data _idx_data, Size _idx_data_size) :
    Instance()
  {
    name = strdup(n);
    p_items_cnt = icnt;
    v_items_cnt = icnt;
    setClass(_class);
    bottom = _bottom;
    top = _top;
    setCardinalityConstraint(_card);
    coll_class = getClass()->asCollectionClass()->getCollClass(&isref,
							       &dim,
							       &item_size);
    if (dim <= 0) {
      Exception::Mode mode = Exception::setMode(Exception::StatusMode);
      status = Exception::make(IDB_COLLECTION_ERROR, "invalid dimension: %d\n", dim);
      Exception::setMode(mode);
      return;
    }

    idx1_oid = _idx1_oid;
    idx2_oid = _idx2_oid;
  
    status = Success;

    locked = False;

    if (!_collimpl) {
      _collimpl = getDefaultCollImpl(COLLIMPL_DEFAULT);
    }

    collimpl = _collimpl->clone();
    cache = 0;

    unvalidReadCache();

    read_cache.obj_arr = 0;
    read_cache.oid_arr = 0;
    read_cache.val_arr = 0;

    isStringColl();
    inv_oid.invalidate();
    inv_item = 0;
    inverse_valid = True;
    literal_oid.invalidate();
    is_literal = _is_literal;
    is_pure_literal = _is_pure_literal;
    is_complete = True;
    idx_data_size = _idx_data_size;
    idx_data = (idx_data_size ? (Data)malloc(idx_data_size) : 0);
    memcpy(idx_data, _idx_data, idx_data_size);
    implModified = False;
    nameModified = False;
#ifdef INIT_IDR
    inv_oid_offset = 0;
    init_idr();
#endif

#ifdef COLLTRACE
    printf("Collection[%p] -> %s\n", this,
	   (const char *)AttrIdxContext(idx_data, idx_data_size).getString());
#endif
  }

  void Collection::make(const char *n, Class *mc, Bool _isref,
			const CollImpl *_collimpl)
  {
    name = strdup(n);
    p_items_cnt = 0;
    v_items_cnt = 0;
    coll_class = (mc ? mc : Object_Class);
    isref = _isref;
    dim = 1;

    status = Success;

    _init(_collimpl);
  }

  void Collection::make(const char *n, Class *mc, int _dim,
			const CollImpl *_collimpl)
  {
    name = strdup(n);
    p_items_cnt = 0;
    v_items_cnt = 0;
    coll_class = (mc ? mc : Object_Class);
    isref = False;
    dim = _dim;

    if (dim <= 0) {
      Exception::Mode mode = Exception::setMode(Exception::StatusMode);
      status = Exception::make(IDB_COLLECTION_ERROR, "invalid dimension: %d\n", dim);
      Exception::setMode(mode);
      return;
    }

    status = Success;

    _init(_collimpl);
  }

  Collection::Collection(const char *n, Class *mc, Bool _isref,
			 const CollImpl *_collimpl) : Instance()
  {
    make(n, mc, _isref, _collimpl);
  }

  Collection::Collection(const char *n, Class *mc, int _dim,
			 const CollImpl *_collimpl) : Instance()
  {
    make(n, mc, _dim, _collimpl);
  }

  Collection::Collection(const Collection &coll) : Instance(coll)
  {
    name = 0;
    cache = 0;
    idx_data = 0;
    idx_data_size = 0;
    memset(&read_cache, 0, sizeof(ReadCache));
    status = Success;
    collimpl = 0;
    *this = coll;
  }

#define CLONE(X) ((X) ? (X)->clone() : 0)
#define STRDUP(X) ((X) ? strdup(X) : 0)

#ifndef NEW_MASTER_OBJ
  static Object *
  get_master_object(Object *o)
  {
    if (!o) return 0;

    if (o->getMasterObject())
      return get_master_object(o->getMasterObject());

    return o;
  }
#endif

  void Collection::setImplementation(const CollImpl *_collimpl)
  {
    if (!collimpl->compare(_collimpl)) {
      if (collimpl)
	collimpl->release();
      collimpl = _collimpl->clone();
#ifdef NEW_MASTER_OBJ
      if (is_literal && getMasterObject(true))
	getMasterObject(true)->touch();
#else
      if (is_literal && getMasterObject())
	get_master_object(getMasterObject())->touch();
#endif
      touch();
      implModified = True;
    }
  }

  Collection& Collection::operator=(const Collection &coll)
  {
    //garbage(); // will be done in Object::operator=

    *(Object *)this = (const Object &)coll;
    type = coll.type;
    name = STRDUP(coll.name);
    ordered = coll.ordered;
    allow_dup = coll.allow_dup;
    string_coll = coll.string_coll;
    bottom = coll.bottom;
    top = coll.top;
  
    coll_class = coll.coll_class;

    isref = coll.isref;
    dim = coll.dim;
    item_size = coll.item_size;
    cl_oid = coll.cl_oid;
  
    locked = coll.locked;
    collimpl = coll.collimpl->clone();
    inv_oid = coll.inv_oid;
    inv_item = coll.inv_item;
  
    card = (CardinalityDescription *)CLONE(coll.card);
    card_bottom = coll.card_bottom;
    card_bottom_excl = coll.card_bottom_excl;
    card_top = coll.card_top;
    card_top_excl = coll.card_top_excl;
    card_oid = coll.card_oid;

    idx1_oid = coll.idx1_oid;
    idx2_oid = coll.idx2_oid;
    idx1 = coll.idx1;
    idx2 = coll.idx2;
    p_items_cnt = coll.p_items_cnt;
    v_items_cnt = coll.v_items_cnt;
    inverse_valid = coll.inverse_valid;
    is_literal = coll.is_literal;
    is_pure_literal = coll.is_pure_literal;
    is_complete = coll.is_complete;
    literal_oid = coll.literal_oid;
    idx_data_size = coll.idx_data_size;
    idx_data = (idx_data_size ? (Data)malloc(idx_data_size) : 0);
    memcpy(idx_data, coll.idx_data, idx_data_size);

    // should copy the cache???
    cache = 0;

    read_cache.obj_arr = 0;
    read_cache.oid_arr = 0;
    read_cache.val_arr = 0;

    return *this;
  }

  Status Collection::setDatabase(Database *mdb)
  {
    if (mdb == db)
      return Success;

    Database *odb = db;

    if (odb && odb != mdb) {
      assert(0);
      return Exception::make(IDB_ERROR, "cannot change dynamically database of an object");
    }

    db = mdb;

    Class *_cls = getClass();
    Status s = CollectionClass::make(db, &_cls);
    if (!s)
      setClass(_cls);
    return s;
  }

  void Collection::setLiteral(Bool _is_literal)
  {
    is_literal = _is_literal;
  }

  void Collection::setPureLiteral(Bool _is_pure_literal)
  {
    is_pure_literal = _is_pure_literal;
  }

  char Collection::codeLiteral() const
  {
    if (is_pure_literal)
      return CollPureLiteral;

    if (is_literal)
      return CollLiteral;

    return CollObject;
  }

  void Collection::decodeLiteral(char c, Bool &is_literal, Bool &is_pure_literal)
  {
    if (c == CollPureLiteral) {
      is_pure_literal = True;
      is_literal = True;
    }
    else if (c == CollLiteral) {
      is_pure_literal = False;
      is_literal = True;
    }
    else {
      is_pure_literal = False;
      is_literal = False;
    }
  }

  Status Collection::loadLiteral()
  {
    unsigned char data[1];
    short dspid = 0;
    Oid toid = literal_oid;
    if (!toid.isValid())
      toid = getOid();
      
    if (!toid.isValid())
      return Success;

    RPCStatus rpc_status = dataRead(db->getDbHandle(), LITERAL_OFFSET,
				    sizeof(char),
				    data, &dspid, toid.getOid());
    if (rpc_status)
      return StatusMake(rpc_status);

    Offset offset = 0;
    char c;
    char_decode (data, &offset, &c);
    Collection::decodeLiteral(c, is_literal, is_pure_literal);

    return Success;
  }

  Status Collection::updateLiteral()
  {
    if (db) {
      char new_code = codeLiteral();

      Oid toid = literal_oid;
      if (!toid.isValid())
	toid = getOid();
      
      if (toid.isValid()) {
	Offset offset = 0;
	Size alloc_size = sizeof(char);
	unsigned char data[1];
	unsigned char *pdata = data;
	char_code(&pdata, &offset, &alloc_size, &new_code);
	RPCStatus rpc_status = dataWrite(db->getDbHandle(), LITERAL_OFFSET,
					 sizeof(char),
					 data, toid.getOid());
	
	if (rpc_status)
	  return StatusMake(rpc_status);
      }
    }

    return Success;
  }

  Bool Collection::isLiteralObject() const
  {
    return is_literal && !is_pure_literal ? True : False;
  }

  Status Collection::setLiteralObject(bool reload)
  {
    if (reload) {
      Status s = loadLiteral();
      if (s)
	return s;
    }

    if (!isLiteral())
      return Exception::make(IDB_COLLECTION_ERROR,
			     "collection %s is not a literal",
			     getOid().toString());


#if 0
    if (isLiteralObject())
      return Exception::make(IDB_COLLECTION_ERROR,
			     "collection %s is already a literal object",
			     getOid().toString());
#endif

    setPureLiteral(False);

    return updateLiteral();
  }

  Status Collection::setMasterObject(Object *_master_object)
  {
    Object *master_obj = 0;
    if (is_literal && (master_obj = getMasterObject(true))) {
      Object *r_master_object = _master_object->getMasterObject(true);
      if (!r_master_object)
	r_master_object = _master_object;

      if ((master_obj->getOid().isValid() ||
	   r_master_object->getOid().isValid()) &&
	  master_obj->getOid() != r_master_object->getOid()) {
	return Exception::make("collection setting master object %s: "
			       "%s is already a literal attribute "
			       "for the object",
			       r_master_object->getOid().toString(),
			       getOidC().toString(),
			       master_obj->getOid().toString());
      }
    }

    Status s = Object::setMasterObject(_master_object);
    if (s)
      return s;

    // must set to pure literal if and only if !getOid().isValid()
    // -> eventually patch the Collection data with dataWrite
    // at literal_offset
    // pure_literal -> char=1
    // literal but not pure -> char=2
    // not literal and not pure -> char=0
    char old_code = codeLiteral();

    setLiteral(True);

    if (getOid().isValid())
      setPureLiteral(False); // collection is also an object
    else
      setPureLiteral(True);

    // EV 26/01/07
    // copy oid to literal oid and set oid to null
    // in main cases both oids should be invalid
    if (!getLiteralOid().isValid())
      setLiteralOid(getOid());

    if (!is_pure_literal)
      ObjectPeer::setOid(this, Oid::nullOid);

    if (!getDatabase())
      setDatabase(_master_object->getDatabase());

    char new_code = codeLiteral();
    if (new_code != old_code) {
      Status s = updateLiteral();
      if (s)
	return s;
    }

    return Success;
  }

  Status Collection::releaseMasterObject()
  {
    Status s = loadLiteral();
    if (s)
      return s;

    bool to_release = (is_pure_literal && getOidC().isValid());
    char old_code = codeLiteral();

    setLiteral(False);
    setPureLiteral(False);

    char new_code = codeLiteral();

    if (new_code != old_code) {
      s = updateLiteral();
      if (s)
	return s;
    }

    // EV 26/01/07
    // copy literal oid to oid and set literal oid to null
    // in main cases both oids should be invalid

    ObjectPeer::setOid(this, getLiteralOid());
    setLiteralOid(Oid::nullOid);

    s = Object::releaseMasterObject();
    if (s)
      return s;

    if (to_release) {
      return remove();
    }

    return Success;
  }

  //
  // Collection check methods
  //

  Status Collection::check(const Oid &item_oid,
			   const Class *item_class,
			   Error err) const
  {
    Bool is;
    Status s = coll_class->isSuperClassOf(item_class, &is);

    if (s)
      return s;

    if (is)
      return Success;

    return Exception::make(err, "item '%s' is of class '%s', expected subclass of '%s'", item_oid.getString(),
			   item_class->getName(),
			   coll_class->getName());
  }

  Status Collection::check(const Oid& item_oid, Error err) const
  {
    if (status != Success)
      return Exception::make(err,
			     "invalid collection status: \"%s\"",
			     status->getDesc());
    if (!isref)
      return Exception::make(err,
			     "must use Collection::insert(Data, Size)"
			     " or Collection::insert(const Value &)");
  
    Status s;
    Class *item_class;

    s = db->getObjectClass(item_oid, item_class);

    if (s)
      return s;

    return check(item_oid, item_class, err);
  }

  Status Collection::check(const Object *item_o, Error err) const
  {
    if (status != Success)
      return Exception::make(err,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (!item_o)
      return Exception::make(err, "");

    if (item_o->isOnStack()) {
      if (!eyedb_support_stack)
	return Exception::make(IDB_COLLECTION_ERROR,
			       "cannot insert a stack allocated object in collection '%s'", oid.toString());
    }


    if (!isref &&
	!coll_class->asBasicClass() &&
	!coll_class->asEnumClass()) {
      unsigned int attr_cnt;
      const Attribute **attrs = coll_class->getAttributes(attr_cnt);
      for (int n = 0; n < attr_cnt; n++) {
	const Attribute *attr = attrs[n];
	if (!attr->isIndirect())
	  continue;

	const TypeModifier &tmod = attr->getTypeModifier();
	for (int i = 0; i < tmod.pdims; i++) {
	  Data data_obj = 0;
	  Status s = attr->getValue(item_o, &data_obj, 1, i);
	  if (s) 
	    return s;
	  if (!data_obj)
	    continue;
	  
	  Oid item_oid;
	  s = attr->getOid(item_o, &item_oid, 1, i);
	  if (s) 
	    return s;
	  if (!item_oid.isValid())
	    return Exception::make(IDB_COLLECTION_ERROR,
				   "object is not completed: attribute "
				   "%s::%s has a value and no oid",
				   coll_class->getName(),
				   attr->getName());
	}
      }
    }

    if (isref)
      return check(item_o->getOid(), item_o->getClass(), err);

    return Success;
  }

  Status Collection::check(Data val, Size size, Error err) const
  {
    if (status != Success)
      return Exception::make(err,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (isref)
      return Exception::make(err, "must use Collection::insert(const Object *) or insert(const Oid&)");
      
    if (!val)
      return Exception::make(err, "trying to insert a null value");

    if (size != defaultSize && size > item_size)
      return Exception::make(err, "size too large %d, expected %d", size, item_size);
    
    return Success;
  }

  static const char invalid_type_fmt[] = "invalid type: expected %s, got %s";

  Status Collection::check(const Value &v, Error err) const
  {
    if (v.type == Value::tObject)
      return check(v.o, err);

    if (v.type == Value::tObjectPtr)
      return check(v.o_ptr->getObject(), err);

    if (v.type == Value::tOid)
      return check(Oid(*v.oid), err);

    if (v.type == Value::tString) {
      if (!string_coll)
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)v.str, strlen(v.str), err);
    }

    if (v.type == Value::tChar) {
      if (!coll_class->asCharClass())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)&v.c, sizeof(v.c), err);
    }

    if (v.type == Value::tShort) {
      if (!coll_class->asInt16Class())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)&v.s, sizeof(v.s), err);
    }

    if (v.type == Value::tInt) {
      if (!coll_class->asInt32Class())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)&v.i, sizeof(v.i), err);
    }

    if (v.type == Value::tLong) {
      if (!coll_class->asInt64Class())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)&v.l, sizeof(v.l), err);
    }

    if (v.type == Value::tDouble) {
      if (!coll_class->asFloatClass())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check((Data)&v.d, sizeof(v.d), err);
    }

    if (v.type == Value::tData) {
      if (!coll_class->asByteClass())
	return Exception::make(err, invalid_type_fmt,
			       getStringType().c_str(), v.getStringType());
      return check(v.data.data, v.data.size, err);
    }

    return Exception::make(err, invalid_type_fmt, getStringType().c_str(),
			   v.getStringType());
  }

  std::string Collection::getStringType() const
  {
    string s = coll_class->getName();
    if (isref)
      s += "*";
    if (dim > 1)
      s += "[" + str_convert(dim) + "]";
    return s;
  }

  Status Collection::insert(const Value &v, Bool noDup)
  {
    Status s = check(v, IDB_COLLECTION_INSERT_ERROR);
    if (s)
      return s;

    if (v.type == Value::tObject)
      return insert_p(v.o, noDup);

    if (v.type == Value::tObjectPtr)
      return insert_p(v.o_ptr->getObject(), noDup);

    if (v.type == Value::tOid)
      return insert_p(Oid(*v.oid), noDup);

    Size size;
    Data data = v.getData(&size);

    return insert_p(data, noDup, size);
  }

  Status Collection::suppress(const Value &v, Bool checkFirst)
  {
    Status s = check(v, IDB_COLLECTION_SUPPRESS_ERROR);
    if (s)
      return s;

    if (v.type == Value::tObject)
      return suppress_p(v.o, checkFirst);

    if (v.type == Value::tObjectPtr)
      return suppress_p(v.o_ptr->getObject(), checkFirst);

    if (v.type == Value::tOid)
      return suppress_p(Oid(*v.oid), checkFirst);

    Size size;
    Data data = v.getData(&size);

    return suppress_p(data, checkFirst, size);
  }

  //
  // Collection suppress methods
  //

  Status Collection::suppress_p(const Oid &item_oid, Bool checkFirst)
  {
    /*
      1/ on regarde si oid est present dans la cache: si oui:
      si removed    -> error deja detruit
      si coherent   -> change le state a` removed; v_items_cnt--;
      si added      -> on detruit du cache; v_items_cnt--;

      2/ si oidcoll n'est pas valid -> return error

      3/ on fait:
      collectionGetByOid() -> si not found -> error
      4/ on insere dans la cache avec le state removed
      v_items_cnt--;
    */

    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' is locked for writing", name);

    if (status)
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());
    /*
      printf("Collection::suppress(%s) -> %s\n", item_oid.toString(),
      getOid().toString());
    */
    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    if (cache && (item = cache->get(item_oid))) {
      int s = item->getState();
      if (s == removed) {
	if (checkFirst)
	  return Success;
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "item '%s' is already suppressed", item_oid.getString());
      }
      else if (s == coherent)
	item->setState(removed);
      else if (s == added)
	cache->suppressOid(item);

      v_items_cnt--;
      return Success;
    }

    if (!getOidC().isValid())
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "collection oid is invalid (collection has not been stored)");

    int found, ind;
      
    RPCStatus rpc_status;

    if ((rpc_status = collectionGetByOid(db->getDbHandle(),
					 getOidC().getOid(),
					 item_oid.getOid(), &found, &ind)) ==
	RPCSuccess) {
      if (!found) {
	if (checkFirst)
	  return Success;
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "item '%s' not found in collection '%s'", item_oid.getString(), name);
      }
    }
    else
      return StatusMake(IDB_COLLECTION_SUPPRESS_ERROR, rpc_status);

    create_cache();
    //cache->insert(item_oid, v_items_cnt, removed);
    cache->insert(item_oid, ValueCache::DefaultItemID, removed);
    v_items_cnt--;
    return Success;
  }

  Status Collection::suppress_p(const Object *item_o, Bool checkFirst)
  {
    /*
      1/ on regarde si o est present dans la cache: si oui:
      si removed    -> error deja detruit
      si coherent   -> change le state a` removed; v_items_cnt--;
      si added      -> on detruit du cache; v_items_cnt--;
      le state -> removed
      return;
      2/ si o->getOid() est non valid
      return error;

      3/  on regarde si present dans le cache: si oui:
      si removed    -> error deja detruit
      si coherent   -> change le state a` removed; v_items_cnt--;
      si added      -> on detruit du cache; v_items_cnt--;
      le state -> removed
      return;

      4/ si la collection est realize'e (oid 
      valid) on fait:
      collectionGetByOid() -> si not found -> return error
      5/ on insere dans la cache avec le state removed
      v_items_cnt--;
    */
    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED, "collection '%s' is locked for writing", name);

    if (status != Success)
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());
    if (!item_o)
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "trying to suppress a null object");

    IDB_COLL_LOAD_DEFERRED();
    touch();
    ValueItem *item;

    if (cache && (item = cache->get(item_o))) {
      int s = item->getState();
      if (s == removed) {
	if (checkFirst)
	  return Success;
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "object 0x%x has already been suppressed", item_o);
      }
      else if (s == coherent)
	item->setState(removed);
      else if (s == added)
	cache->suppressObject(item);

      v_items_cnt--;
      return Success;
    }

    Oid item_oid = item_o->getOid();

    if (!item_oid.isValid())
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "oid item of object 0x%x is invalid", item_o);

    if (cache && (item = cache->get(item_oid))) {
      int s = item->getState();
      if (s == removed) {
	if (checkFirst)
	  return Success;
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "item '%s' has been already suppressed", item_oid.getString());
      }
      else if (s == coherent)
	item->setState(removed);
      else if (s == added)
	item->setState(removed); /* cache->suppress(item); */

      v_items_cnt--;
      return Success;
    }

    if (!getOidC().isValid())
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "collection oid is invalid (collection has not been stored)");

    int found, ind;
      
    RPCStatus rpc_status;

    if ((rpc_status = collectionGetByOid(db->getDbHandle(),
					 getOidC().getOid(),
					 item_oid.getOid(), &found, &ind)) ==
	RPCSuccess) {
      if (!found) {
	if (checkFirst)
	  return Success;
	return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR, "item '%s' not found in collection", item_oid.toString());
      }
    }
    else
      return StatusMake(IDB_COLLECTION_SUPPRESS_ERROR, rpc_status);

    create_cache();
    //cache->insert(item_o, v_items_cnt, removed);
    cache->insert(item_o, ValueCache::DefaultItemID, removed);
    v_items_cnt--;
    return Success;
  }

  Status Collection::suppress_p(Data val, Bool, Size size)
  {
    if (status != Success)
      return Exception::make(IDB_COLLECTION_SUPPRESS_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());
    return Exception::make(IDB_ERROR, "Collection::suppress(Data val) is not implemented");
  }

  //
  // Collection empty method
  //

  Status Collection::empty()
  {
    if (CollectionPeer::isLocked(this))
      return Exception::make(IDB_COLLECTION_LOCKED,
			     "collection '%s' is locked for writing", name);

    emptyReadCache();

    if (cache) {
      cache->setState(removed);
    }

    IDB_COLL_LOAD_DEFERRED();
    touch();
    if (!getOidC().isValid()) {
      p_items_cnt = 0;
      v_items_cnt = 0;
      top = bottom = 0;
      return Success;
    }

    Status s;
    Iterator q(this, True);

    if ((s = q.getStatus()))
      return s;

    IteratorAtom qatom;
    int ind = 0;
    for (int n = 0; ; n++) {
      Bool found;
      s = q.scanNext(&found, &qatom);
      if (s) {
	//cache->empty();
	return s;
      }

      if (!found)
	break;
      
      if (asCollArray()) {
	if (!(n & 1)) {
	  assert(qatom.type == IteratorAtom_INT32);
	  ind = qatom.i32;
	  continue;
	}
      }
      else
	ind = ValueCache::DefaultItemID;

      create_cache();
      if (isref)
	cache->insert(&qatom.oid, ind, removed);
      else {
	Data _idr = 0;
	Offset offset = 0;
	Size alloc_size = 0;
	qatom.code(&_idr, &offset, &alloc_size);
	cache->insert(Value(idr->getIDR(), item_size), ind, removed);
      }
    }

    p_items_cnt = 0;
    v_items_cnt = 0;
    top = bottom = 0;
    return Success;
  }

  //
  // Collection isIn methods
  //

  Status Collection::isIn_p(const Oid &item_oid, Bool &found,
			    Collection::ItemId *where) const
  {
    /*
      1/ on regarde si _oid est present dans la cache: si oui -> si le
      state est non removed return true 
      2/ si oidcoll est :
      collectionGetByOid() -> true ou false
    */
    //  printf("Collection::isIn(%s)\n", item_oid.getString());
    ValueItem *item;

    found = False;

    if (cache && (item = cache->get(item_oid))) {
      if (item->getState() != removed)
	found = True;

      return Success;
    }
      
    if (!getOidC().isValid())
      return Success;

    //  printf("Collection::isIn_p #1\n");
    int f, ind;
      
    RPCStatus rpc_status;

    rpc_status = collectionGetByOid(db->getDbHandle(), getOidC().getOid(),
				    item_oid.getOid(), &f, &ind);
    //  printf("Collection::isIn_p #2 -> %d\n", f);
    found = (f ? True : False);
    if (found && where) *where = ind;

    return StatusMake(IDB_COLLECTION_IS_IN_ERROR, rpc_status);
  }

  Status Collection::isIn_p(const Object *item_o, Bool &found,
			    Collection::ItemId *where) const
  {
    /*
      1/ on regarde si _o est present dans la cache: si oui return true
      2/ si o->getOid() est valid ET si la collection est realize'e (oid 
      valid) on fait:
      collectionGetByOid() -> true ou false
    */
    found = False;

    if (!item_o)
      return Exception::make(IDB_COLLECTION_IS_IN_ERROR, "trying to check presence of a null object");

    if (!isref)
      return isIn_p(item_o->getIDR() + IDB_OBJ_HEAD_SIZE, found, defaultSize,
		    where);

    ValueItem *item;

    if (cache && (item = cache->get(item_o)) && item->getState() != removed) {
      found = True;
      return Success;
    }
      
    Oid item_oid = item_o->getOid();

    if (item_oid.isValid() && cache && (item = cache->get(item_oid)) &&
	item->getState() != removed) {
      found = True;
      return Success;
    }

    if (!getOidC().isValid())
      return Success;

    int f, ind;
      
    RPCStatus rpc_status;

    rpc_status = collectionGetByOid(db->getDbHandle(), getOidC().getOid(),
				    item_oid.getOid(), &f, &ind);

    found = (f ? True : False);
    if (found && where) *where = ind;
    return StatusMake(IDB_COLLECTION_IS_IN_ERROR, rpc_status);
  }

  Status Collection::isIn_p(Data data, Bool &found, Size size,
			    Collection::ItemId *where) const
  {
    /*
      1/ on regarde si _o est present dans la cache: si oui return true
      2/ si la collection est realize'e (oid valid) on fait:
      collectionGetByValue() -> true ou false
    */
    ((Collection *)this)->status = check(data, size, IDB_COLLECTION_IS_IN_ERROR);

    if (status != Success)
      return status;

    ValueItem *item;

    Data item_data = make_data(data, size, True);

    if (!item_data)
      return Exception::make(IDB_COLLECTION_ERROR, "data too long for collection search");

    if (cache && (item = cache->get(item_data, item_size))) {
      if (item->getState() != removed)
	found = True;
      return Success;
    }
      
    if (!getOidC().isValid())
      return Success;

    int f, ind;
      
    RPCStatus rpc_status;

    rpc_status = collectionGetByValue(db->getDbHandle(),
				      getOidC().getOid(), item_data,
				      item_size, &f, &ind);

    found = (f ? True : False);
    if (found && where) *where = ind;
    return StatusMake(IDB_COLLECTION_IS_IN_ERROR, rpc_status);
  }

  Status Collection::isIn(const Value &v, Bool &found,
			  Collection::ItemId *where) const
  {
    Status s = check(v, IDB_COLLECTION_ERROR);
    if (s)
      return s;

    if (v.type == Value::tObject)
      return isIn_p(v.o, found, where);

    if (v.type == Value::tObjectPtr)
      return isIn_p(v.o_ptr->getObject(), found, where);

    if (v.type == Value::tOid)
      return isIn_p(Oid(*v.oid), found, where);

    Size size;
    Data data = v.getData(&size);

    return isIn_p(data, found, size, where);
  }

  Status Collection::getStatus() const
  {
    return status;
  }

  int Collection::getCount() const
  {
    if (!is_complete)
      (void)const_cast<Collection *>(this)->loadDeferred();
    return v_items_cnt;
  }

  int Collection::getBottom() const
  {
    if (!is_complete)
      (void)const_cast<Collection *>(this)->loadDeferred();
    return bottom;
  }

  int Collection::getTop() const
  {
    if (!is_complete)
      (void)const_cast<Collection *>(this)->loadDeferred();
    return top;
  }

  Bool Collection::isEmpty() const
  {
    if (!is_complete)
      (void)const_cast<Collection *>(this)->loadDeferred();
    return (v_items_cnt == 0 ? True : False);
  }

  void Collection::setName(const char *_name)
  {
    if (!is_complete)
      (void)const_cast<Collection *>(this)->loadDeferred();
    free(name);
    name = strdup(_name);
    touch();
    nameModified = True;
  }

  void Collection::garbage()
  {
    free(name);
    delete cache;
    emptyReadCache();
    free(idx_data);
    Instance::garbage();
    if (collimpl)
      collimpl->release();
  }

  Collection::~Collection()
  {
    garbageRealize();
  }

  Status Collection::setValue(Data)
  {
    return Success;
  }

  Status Collection::getValue(Data*) const
  {
    return Success;
  }

  Status Collection::create()
  {
    return create_realize(NoRecurs);
  }

  void Collection::cardCode(Data &data, Offset &offset, Size &alloc_size)
  {
    //  printf("collection_realize(card_code = %s)\n", card_oid.toString());
    if (card_oid.isValid()) {
      oid_code(&data, &offset, &alloc_size, card_oid.getOid());
      return;
    }

    oid_code(&data, &offset, &alloc_size, getInvalidOid());
  }

  Object *Collection::cardDecode(Database *db, Data temp, Offset &offset)
  {
    eyedbsm::Oid xoid;

    oid_decode(temp, &offset, &xoid);

    Oid coid(xoid);

    //  printf("collection_make(card_decode = %s)\n", coid.toString());

    if (!coid.isValid())
      return NULL;

    Object *card;

    Status status = db->loadObject(&coid, &card);

    if (status) {
      status->print();
      return NULL;
    }

    return card;
  }

  Status Collection::codeCollImpl(Data &data, Offset &offset,
				  Size &alloc_size)
  {
    return IndexImpl::code(data, offset, alloc_size, collimpl->getIndexImpl(), collimpl->getType());
  }

  Status Collection::getImplStats(std::string &xstats, Bool dspImpl, Bool full,
				  const char *indent)
  {
    IndexStats *stats;
    Status s = getImplStats(stats);
    if (s) return s;
    xstats = (stats ? stats->toString(dspImpl, full, indent) : std::string(""));
    delete stats;
    return Success;
  }

  void Collection::completeImplStats(IndexStats *stats) const
  {
    const IndexImpl *idximpl = collimpl->getIndexImpl();
    if (idximpl) {
      if (idximpl->getDataspace())
	stats->idximpl->setDataspace(idximpl->getDataspace());
      if (idximpl->getHashMethod())
	stats->idximpl->setHashMethod(idximpl->getHashMethod());
    }
  }

  Status Collection::getIdxOid(Oid &idx1oid, Oid &idx2oid) const
  {
    idx1oid = idx1_oid;
    idx2oid = idx2_oid;

    if (is_literal && !idx1oid.isValid()) {
      if (literal_oid.isValid()) {
	Object *o;
	Status s = db->loadObject(literal_oid, (Object *&)o);
	if (s) return s;
	idx1oid = o->asCollection()->idx1_oid;
	idx2oid = o->asCollection()->idx2_oid;
	o->release();
      }
    }

    return Success;
  }

  Status Collection::getImplStats(IndexStats *&stats)
  {
    Oid idx1oid, idx2oid;
    Status s = getIdxOid(idx1oid, idx2oid);
    if (s) return s;

    if (idx1oid.isValid()) {
      RPCStatus rpc_status =
	collectionGetImplStats(db->getDbHandle(), collimpl->getType(),
			       idx1oid.getOid(), (Data *)&stats);
      if (rpc_status)
	return StatusMake(rpc_status);
      completeImplStats(stats);
      return Success;
    }

    stats = 0;
    return Success;
  }

  Status Collection::simulate(const CollImpl &_collimpl, std::string &xstats,
			      Bool dspImpl, Bool full, const char *indent)
  {
    IndexStats *stats;
    Status s = simulate(_collimpl, stats);
    if (s) return s;
    xstats = (stats ? stats->toString(dspImpl, full, indent) : std::string(""));
    delete stats;
    return Success;
  }

  Status Collection::simulate(const CollImpl &_collimpl, IndexStats *&stats)
  {
    Oid idx1oid, idx2oid;
    Status s = getIdxOid(idx1oid, idx2oid);
    if (s) return s;

    if (idx1oid.isValid()) {
      Data data;
      Offset offset = 0;
      Size size = 0;
      Status s = IndexImpl::code(data, offset, size, _collimpl.getIndexImpl());
      if (s) return s;
      RPCStatus rpc_status =
	collectionSimulImplStats(db->getDbHandle(), _collimpl.getType(),
				 idx1oid.getOid(), data, size,
				 (Data *)&stats);
      return StatusMake(rpc_status);
    }

    stats = 0;
    return Success;
  }

  Status Collection::init_idr() {
#ifdef INIT_IDR
    assert(!getIDR());
    Size alloc_size = 0;
    Offset offset = IDB_OBJ_HEAD_SIZE;

    idr->setIDR((Size)0);
    Data data = 0;

    /* locked */
    char c = locked;
    char_code (&data, &offset, &alloc_size, &c);

    /* item_size */
    eyedblib::int16 kk = (eyedblib::int16)item_size;
    int16_code (&data, &offset, &alloc_size, &kk);

    Status s = codeCollImpl(data, offset, alloc_size);
    if (s)
      return s;

    /* null oid */
    oid_code (&data, &offset, &alloc_size, getInvalidOid());
    /* null oid */
    oid_code (&data, &offset, &alloc_size, getInvalidOid());

    eyedblib::int32 zero = 0;
    /* item count */

    int32_code (&data, &offset, &alloc_size, &zero);
    /* bottom */
    int32_code (&data, &offset, &alloc_size, &zero);
    /* top */
    int32_code (&data, &offset, &alloc_size, &zero);
  
    cardCode(data, offset, alloc_size);

    inv_oid_offset = offset;
    oid_code(&data, &offset, &alloc_size, inv_oid.getOid());
    int16_code(&data, &offset, &alloc_size, &inv_item);

    char c = codeLiteral();
    /* is_literal */
    LITERAL_OFFSET = offset;
    char_code (&data, &offset, &alloc_size, &c);
    eyedblib::int16 sz = idx_data_size;
    int16_code (&data, &offset, &alloc_size, &sz);
    buffer_code(&data, &offset, &alloc_size, idx_data, idx_data_size);

    /* collection name */
    string_code(&data, &offset, &alloc_size, name);

    int idr_sz = offset;
    idr->setIDR(idr_sz, data);
    headerCode(type, idr_sz);
#endif
    return Success;
  }

  Status Collection::create_realize(const RecMode *rcm)
  {
    if (status != Success)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (getOidC().isValid())
      return Exception::make(IDB_OBJECT_ALREADY_CREATED, "%scollection %s",
			     is_literal ? "literal " : "",
			     getOidC().toString());

    IDB_CHECK_WRITE(db);
  
    Status s;
    if (!getClass()->getOid().isValid()) {
      s = getClass()->create();
      if (s) return s;
    }

#if 0
    Size wr_size = 0;
    unsigned char *temp = 0;
    Offset cache_offset = 0;
    if (!is_literal) {
      s = cache_compile(cache_offset, wr_size, &temp, rcm);
      if (s) return s;
    }
#endif

#ifdef INIT_IDR
    assert(getIDR());
    //init_idr();
#else
    Size alloc_size = 0;
    Offset offset = IDB_OBJ_HEAD_SIZE;

    idr->setIDR((Size)0);
    Data data = 0;

    /* locked */
    char c = locked;
    char_code (&data, &offset, &alloc_size, &c);

    /* item_size */
    eyedblib::int16 kk = (eyedblib::int16)item_size;
    int16_code (&data, &offset, &alloc_size, &kk);

    s = codeCollImpl(data, offset, alloc_size);
    if (s) return s;

    /* null oid */
    oid_code (&data, &offset, &alloc_size, getInvalidOid());
    /* null oid */
    oid_code (&data, &offset, &alloc_size, getInvalidOid());

    eyedblib::int32 zero = 0;
    /* item count */

    int32_code (&data, &offset, &alloc_size, &zero);
    /* bottom */
    int32_code (&data, &offset, &alloc_size, &zero);
    /* top */
    int32_code (&data, &offset, &alloc_size, &zero);
  
    cardCode(data, offset, alloc_size);
#endif

    if (is_literal) {
#ifdef NEW_MASTER_OBJ
      Object *o = getMasterObject(true);
#else
      Object *o = get_master_object(getMasterObject());
#endif
      if (!inv_oid.isValid())
	inv_oid = o->getOid();

      if (!o->getOid().isValid())
	return Exception::make(IDB_ERROR,
			       "inner object of class '%s' containing "
			       "collection of type '%s' has no valid oid",
			       o->getClass()->getName(), getClass()->getName());

      assert(inv_oid == o->getOid());
    }

#ifdef INIT_IDR
    Offset offset = inv_oid_offset;
    assert(offset);
    Size alloc_size = idr->getSize();
    Data data = idr->getIDR();

    oid_code(&data, &offset, &alloc_size, inv_oid.getOid());

    int16_code(&data, &offset, &alloc_size, &inv_item);

    c = codeLiteral();
    /* is_literal */
    char_code (&data, &offset, &alloc_size, &c);
    eyedblib::int16 sz = idx_data_size;
    printf("IDX_DATA_SIZE = %d\n", idx_data_size);
    int16_code (&data, &offset, &alloc_size, &sz);
    buffer_code(&data, &offset, &alloc_size, idx_data, idx_data_size);
#else
    //printf("inv_oid_offset %d idx_data_size %d name %d ", offset, idx_data_size,  strlen(name));
    oid_code(&data, &offset, &alloc_size, inv_oid.getOid());

    int16_code(&data, &offset, &alloc_size, &inv_item);

    c = codeLiteral();
    /* is_literal */
    char_code (&data, &offset, &alloc_size, &c);
    eyedblib::int16 sz = idx_data_size;
    int16_code (&data, &offset, &alloc_size, &sz);
    buffer_code(&data, &offset, &alloc_size, idx_data, idx_data_size);

    /* collection name */
    string_code(&data, &offset, &alloc_size, name);

    int idr_sz = offset;
    idr->setIDR(idr_sz, data);
    headerCode(type, idr_sz);
    //printf("IDR size %d\n", idr_sz);
    // -------------- end of IDR creation
#endif

    // -------------- begin of object creation
    RPCStatus rpc_status;
    //printf("collection create %s %s %d\n", getClass()->getName(), getClass()->getOid().toString(), getDataspaceID());
    rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, getOidC().getOid());

    if (rpc_status != RPCSuccess)
      return StatusMake(IDB_COLLECTION_ERROR, rpc_status);

    if (!getOidC().getDbid() && getOidC().getNX())
      abort();

    db->cacheObject(this); // added the 30/08/99

    if (is_literal) {
      implModified = False;
      return Success;
    }

    // -------------- end of object creation

    ObjectHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
  
#if 1
    Size wr_size = 0;
    unsigned char *temp = 0;
    Offset cache_offset = 0;
    if (!is_literal) {
      s = cache_compile(cache_offset, wr_size, &temp, rcm);
      if (s) return s;
    }
#endif

    if (wr_size) {
      //short x = IDB_COLL_IMPL_UNCHANGED;
      short x = 0;
      int16_code(&temp, &cache_offset, &wr_size, &x);

      hdr.type = type;
      hdr.size = wr_size;
  
      object_header_code_head(temp, &hdr);
  
      rpc_status = objectWrite(db->getDbHandle(), temp, getOidC().getOid());
    }
  
    free(temp);
    if (!rpc_status) {
      delete cache;
      cache = 0;
      emptyReadCache();
      implModified = False;
      nameModified = False;
      modify = False;
    }

    return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
  }

  Status Collection::update()
  {
    return update_realize(NoRecurs);
  }

  Status Collection::update_realize(const RecMode *rcm)
  {
    if (status != Success)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (!getOidC().isValid())
      return Exception::make(IDB_COLLECTION_ERROR, "collection oid '%s' is not valid", name);

    if (!getClass()->getOid().isValid())
      return Exception::make(IDB_COLLECTION_ERROR, "collection '%s' has not a valid class", name);

    //printf("Collection::update_realize(%s)\n", getOidC().toString());
    Size alloc_size;

    unsigned char *temp;
    Offset offset;
    Status _status = cache_compile(offset, alloc_size, &temp, rcm);
    if (_status)
      return _status;

    RPCStatus rpc_status;

    ObjectHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    short c = 0;
    if (implModified) {
      c |=  IDB_COLL_IMPL_CHANGED;
    }
    if (nameModified) {
      c |= IDB_COLL_NAME_CHANGED;
    }
    int16_code(&temp, &offset, &alloc_size, &c);
    if (implModified) {
      _status = IndexImpl::code(temp, offset, alloc_size, collimpl->getIndexImpl(), collimpl->getType());
      if (_status) {
	return _status;
      }
    }

    if (nameModified) {
      string_code(&temp, &offset, &alloc_size, name);
    }

    hdr.type = type;
    hdr.size = alloc_size;
    hdr.xinfo = (!inverse_valid ? IDB_XINFO_INVALID_INV : 0);

    object_header_code_head(temp, &hdr);

    rpc_status = objectWrite(db->getDbHandle(), temp, getOidC().getOid());
    free(temp);
    if (!rpc_status) {
      delete cache;
      cache = 0;
      emptyReadCache();
      modify = False;
      implModified = False;
      nameModified = False;
    }

    return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
  }

#define IDB_MAGIC_COLL  ((void *)0x2e372811)
#define IDB_MAGIC_COLL2 ((void *)0xe3728113)

  Status Collection::realize(const RecMode *rcm)
  {
    if (state & Realizing)
      return Success;

    CHK_OBJ(this);

#ifdef COLLTRACE
    printf("realizing %p %s %d\n", this, getOidC().toString(),
	   CACHE_LIST_COUNT(cache));
#endif

#ifdef COLL_OPTIM_CREATE
    if (is_literal && !CACHE_LIST_COUNT(cache) && !implModified) {
      //if (!getCount() || literal_oid.isValid())
      return Success;
    }
#endif

    if (is_literal && !literal_oid.isValid() && getUserData() != IDB_MAGIC_COLL) {
#ifdef COLLTRACE
      printf("collection realizing master_object %p\n",
	     getMasterObject(true));
#endif
      assert(getMasterObject(true));
      void *ud = setUserData(IDB_MAGIC_COLL2);
      Status s = getMasterObject(true)->realize(rcm);
      (void)setUserData(ud);
      return s;
    }

    Status s;
    IDB_COLL_LOAD_DEFERRED();
    state |= Realizing;

    if (!getOidC().isValid())
      s = create_realize(rcm);
    else if (modify || collupd_noopt)
      s = update_realize(rcm);
    else
      s = Success;

    state &= ~Realizing;
    return s;
  }

  Status Collection::remove(const RecMode *rcm)
  {
    Status s;

#ifdef COLLTRACE
    printf("removing collection %s\n", getOidC().toString());
#endif

    s = loadLiteral();
    if (s)
      return s;

    if (is_literal)
      return Exception::make("collection %s is a literal object: could not be removed", getOid().toString());

    s = Object::remove();

    if (!s) {
      delete cache;
      cache = 0;
      p_items_cnt = v_items_cnt = 0;
    }

    return s;
  }

  Status Collection::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
  {
    fprintf(fd, "%s %s = ", oid.getString(), getClass()->getName());
    return trace_realize(fd, INDENT_INC, flags, rcm);
  }

  Status Collection::trace_contents_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    // To get contents, make a query on 
    char *indent_str = make_indent(indent);
    ValueArray array;
    Bool isidx = (asCollArray() || asCollList()) ? True : False;

    Status s = getElements(array, (isidx ? True : False));

    if (s)
      return s;

    int inc = (isidx ? 2 : 1);
    int value_off = (isidx ? 1 : 0);
    int ind;

    unsigned int cnt = array.getCount();
    for (int i = 0; i < cnt; i += inc) {
      IDB_CHECK_INTR();
      Value value = array[i+value_off]; //Value value = array.values[i+value_off];

      if (isidx)
	ind = array[i].l; // ind = array.values[i].l;

      if (value.type == Value::tOid) {
	Object *o;
	s = db->loadObject(value.oid, &o, rcm);
	if (s) {
	  delete_indent(indent_str);
	  return s;
	}

	if (isidx)
	  fprintf(fd, "%s[%d] = %s %s ", indent_str, ind,
		  value.oid->getString(), o->getClass()->getName());
	else
	  fprintf(fd, "%s%s %s = ", indent_str, value.oid->getString(),
		  o->getClass()->getName());
	s = ObjectPeer::trace_realize(o, fd, indent + INDENT_INC, flags, rcm);
	if (s) {
	  delete_indent(indent_str);
	  return s;
	}

	o->release();
      }
      else if (isidx)
	fprintf(fd, "%s[%d] = %s;\n", indent_str, ind, value.getString());
      else
	fprintf(fd, "%s%s;\n", indent_str, value.getString());
    }
    
    delete_indent(indent_str);
    return Success;
  }

  Status Collection::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    // IDB_COLL_LOAD_DEFERRED();
    Status s = Success;
    char *indent_str = make_indent(indent);

    if (state & Tracing) {
      fprintf(fd, "%s%s;\n", indent_str, oid.getString());
      delete_indent(indent_str);
      return Success;
    }
    
    if (!is_complete) {
      s = const_cast<Collection *>(this)->loadDeferred();
      if (s) return s;
    }

    const_cast<Collection *>(this)->state |= Tracing;

    char *lastindent_str = make_indent(indent - INDENT_INC);

    fprintf(fd, "%s { ", getClassName());
    if (traceRemoved(fd, indent_str))
      goto out;
    trace_flags(fd, flags);
    fprintf(fd, "\n");

    if ((flags & NativeTrace) == NativeTrace) {
      if (rcm->getType() == RecMode_FullRecurs) {
	fprintf(fd, "%s%s class = { ", indent_str,
		getClass()->getOid().getString());
	  
	if (s = ObjectPeer::trace_realize(getClass(), fd, indent + INDENT_INC, flags, rcm))
	  goto out;

	fprintf(fd, "%s};\n", indent_str);

	fprintf(fd, "%s%s collclass = { ", indent_str,
		coll_class->getOid().getString());

	if (s = ObjectPeer::trace_realize(coll_class, fd, indent + INDENT_INC, flags, rcm))
	  goto out;

	fprintf(fd, "%s};\n", indent_str);
      } else {
	fprintf(fd, "%sclass = %s;\n", indent_str,
		getClass()->getOid().getString());
  
	fprintf(fd, "%scollclass = %s;\n", indent_str,
		coll_class->getOid().getString());

	fprintf(fd, "%sreference = %s;\n", indent_str, (isref ? "true" : "false"));
	if (isPureLiteral())
	  fprintf(fd, "%stype = pure_literal;\n", indent_str);
	else if (isLiteral())
	  fprintf(fd, "%stype = object_literal;\n", indent_str);
	else
	  fprintf(fd, "%stype = object;\n", indent_str);

	if (isLiteral())
	  fprintf(fd, "%sliteral_oid = %s;\n", indent_str, getOidC().toString());

	if (collimpl->getType() == CollImpl::NoIndex) {
	  fprintf(fd, "%sidxtype = 'noindex';\n", indent_str);
	}
	else if (collimpl->getType() == CollImpl::HashIndex) {
	  fprintf(fd, "%sidxtype = 'hasindex';\n", indent_str);
	}
	else if (collimpl->getType() == CollImpl::BTreeIndex) {
	  fprintf(fd, "%sidxtype = 'btreeindex';\n", indent_str);
	}

	if (collimpl->getIndexImpl()) {
	  std::string hints = collimpl->getIndexImpl()->getHintsString();
	  if (hints.size())
	    fprintf(fd, "%shints = \"%s\";\n", indent_str,
		    hints.c_str());
	}

	if (idx1_oid.isValid())
	  fprintf(fd, "%sidx1oid = %s;\n", indent_str,
		  idx1_oid.toString());
	if (idx2_oid.isValid())
	  fprintf(fd, "%sidx2oid = %s;\n", indent_str,
		  idx2_oid.toString());
      }
    }

    fprintf(fd, "%sname = \"%s\";\n", indent_str, name);
    fprintf(fd, "%scount = %d;\n", indent_str, v_items_cnt);

    if (asCollArray())
      fprintf(fd, "%srange = [%d,%d[;\n", indent_str, bottom, top);

    if (card)
      fprintf(fd, "%sconstraint = (%s);\n", indent_str, card->getString());

    if (flags & ContentsFlag) {
      fprintf(fd, "%scontents = {\n", indent_str);
      s = trace_contents_realize(fd, indent + INDENT_INC, flags, rcm);
      fprintf(fd, "%s};\n", indent_str);
    }

  out:
    const_cast<Collection *>(this)->state &= ~Tracing;
    fprintf(fd, "%s};\n", lastindent_str);
    delete_indent(indent_str);
    delete_indent(lastindent_str);

    return s;
  }

  //
  // MIND: these methods are not good, because they do not take the
  // collection cache into account.
  //
  // for instance:
  // Collection *coll;
  // coll->insert(oid1);
  // coll->insert(oid2);
  //
  // coll->getElements(oid_array);
  // 
  // oid_array will contains only the oids in database and not oid1 and
  // oid2.
  //
  // Another pb is the coherency between getCount() (v_items_cnt) and
  // the actual item count in the database.
  //

  Status Collection::getElements(OidArray &oid_array) const
  {
    if (status)
      return Exception::make(status);

    Status s =  const_cast<Collection *>(this)->getOidElementsRealize();
    if (s) return s;

    oid_array = *read_cache.oid_arr;
    return Success;
  }

  Status Collection::getOidElementsRealize()
  {
    if (!isref)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "cannot get oid elements");

    IDB_COLL_LOAD_DEFERRED();

    if (read_cache.oid_arr && read_cache_state_oid == coherent)
      return Success;

    delete read_cache.oid_arr;
    read_cache.oid_arr = new OidArray();
    if (getOidC().isValid()) {
      Iterator q(this);
      if (q.getStatus())
	return q.getStatus();
      
      Status s = q.scan(*read_cache.oid_arr);
      if (s || read_cache_state_oid == coherent) return s;
    }
    else if (read_cache_state_oid == coherent)
      return Success;

    if (read_cache_state_oid == coherent) {
      assert(CACHE_LIST_COUNT(cache) == 0);
      return Success;
    }

    ValueItem *item;
    OidList *oid_list = read_cache.oid_arr->toList();

    if (cache) {
      ValueCache::IdMapIterator begin = cache->getIdMap().begin();
      ValueCache::IdMapIterator end = cache->getIdMap().end();

      while (begin != end) {
	item = (*begin).second;
	const Value &v = item->getValue();
	if (v.type == Value::tOid) {
	  Oid item_oid = *v.oid;

	  int s = item->getState();

#ifdef NEW_GET_ELEMENTS
	  if (s == removed) {
	    if (item_oid.isValid())
	      oid_list->suppressOid(item_oid);
	  }
	  else if (s == added) {
	    oid_list->insertOidLast(item_oid);
	  }
#else
	  if (s == removed && item_oid.isValid())
	    oid_list->suppressOid(item_oid);
	  else if (s == added) {
	    if (!item_oid.isValid() || !oid_list->exists(item_oid))
	      oid_list->insertOidLast(item_oid);
	  }
#endif
	}
	++begin;
      }
    }

    delete read_cache.oid_arr;
    read_cache.oid_arr = oid_list->toArray();
    delete oid_list;

    read_cache_state_oid = coherent;
    return Success;
  }

  Status Collection::getElements(ObjectPtrVector &obj_vect,
				 const RecMode *rcm) const
  {
    ObjectArray obj_array; // true or false;
    Status s = getElements(obj_array, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status Collection::getElements(ObjectArray &obj_array,
				 const RecMode *rcm) const
  {
    if (status)
      return Exception::make(status);

#ifdef TRY_GETELEMS_GC
    if (obj_array.isAutoGarbage()) {
      return Exception::make(IDB_ERROR,
			     "Collection::getElements(ObjectArray &): "
			     "ObjectArray argument cannot be in auto-garbaged mode");
    }
#endif

    if (!isref && !coll_class->asBasicClass() &&
	!coll_class->asEnumClass()) {
      ValueArray val_arr;
      Status s = getElements(val_arr, False);
      if (s) return s;
      int count = val_arr.getCount();
      obj_array.set(0, count);
      for (int n = 0; n < count; n++) {
	if (val_arr[n].getType() == Value::tObject)
	  obj_array.setObjectAt(n, val_arr[n].o); //obj_array[n] = val_arr[n].o;
	else if (val_arr[n].getType() == Value::tObjectPtr)
	  obj_array.setObjectAt(n, val_arr[n].o_ptr->getObject());
	else
	  return Exception::make(IDB_ERROR, "unexpected value type");
      }
      return Success;
    }

    Status s = const_cast<Collection *>(this)->getObjElementsRealize(rcm);
    if (s) return s;

    obj_array = *read_cache.obj_arr;
    return Success;
  }

  Status Collection::getObjElementsRealize(const RecMode *rcm)
  {
    if (!isref) {
      return Exception::make(IDB_COLLECTION_ERROR,
			     "cannot get object elements");
    }

    IDB_COLL_LOAD_DEFERRED();

    if (read_cache.obj_arr && read_cache_state_object == coherent)
      return Success;

#ifdef TRY_GETELEMS_GC
    assert(!read_cache.obj_arr || read_cache.obj_arr->isAutoGarbage());
    assert(!read_cache.val_arr || read_cache.val_arr->isAutoObjGarbage());
#endif
    delete read_cache.obj_arr;

    // 31/10/06: changed because of a memory leaks bug
    // but not really shure that could not have side effect
    //    read_cache.obj_arr = new ObjectArray();
#ifdef TRY_GETELEMS_GC
    read_cache.obj_arr = new ObjectArray(true);
#else
    read_cache.obj_arr = new ObjectArray();
#endif

    if (getOidC().isValid()) {
      Iterator q(this);
      if (q.getStatus())
	return q.getStatus();
	  
      Status s = q.scan(*read_cache.obj_arr);
      if (s || read_cache_state_object == coherent)
	return s;
    }
    else if (read_cache_state_object == coherent)
      return Success;

    if (read_cache_state_object == coherent) {
      assert(CACHE_LIST_COUNT(cache) == 0);
      return Success;
    }

    ValueItem *item;

    ObjectList *obj_list = read_cache.obj_arr->toList();

    if (cache) {
      ValueCache::IdMapIterator begin = cache->getIdMap().begin();
      ValueCache::IdMapIterator end = cache->getIdMap().end();

      while (begin != end) {
	item = (*begin).second;
	const Value &v = item->getValue();
	Object *item_o = 0;
	if (v.type == Value::tOid) {
	  Oid item_oid = *v.oid;
	  if (item_oid.isValid() && db) {
	    Status s = db->loadObject(item_oid, item_o);
	    if (s)
	      return s; // what about garbage??
	  }
	}
	else if (v.type == Value::tObject)
	  item_o = v.o;
	else if (v.type == Value::tObjectPtr)
	  item_o = v.o_ptr->getObject();

	if (!item_o)
	  return Exception::make(IDB_INTERNAL_ERROR, "invalid null object "
				 "found in Collection::getObjElementsRealize()");

	int s = item->getState();

	if (s == removed)
	  obj_list->suppressObject(item_o);
	else if (s == added) {
	  obj_list->insertObjectLast(item_o);
	  item_o->incrRefCount();
	}

	++begin;
      }
    }

#ifdef TRY_GETELEMS_GC
    assert(!read_cache.obj_arr || read_cache.obj_arr->isAutoGarbage());
    assert(!read_cache.val_arr || read_cache.val_arr->isAutoObjGarbage());
#endif
    delete read_cache.obj_arr;

    read_cache.obj_arr = obj_list->toArray();
#ifdef TRY_GETELEMS_GC
    read_cache.obj_arr->setAutoGarbage(true);
    read_cache.obj_arr->setMustRelease(false);
#endif

    delete obj_list;

    read_cache_state_object = coherent;
    return Success;
  }


  void Collection::emptyReadCache()
  {
    // WARNING DISCONNECTED the 20/01/00 because of a memory bug!
    /*
      if (read_cache.obj_arr)
      read_cache.obj_arr->garbage();
    */

    // 31/10/06: the previous disconnection leads to a memory leaks (sometimes)
    // so...

#ifdef TRY_GETELEMS_GC
    assert(!read_cache.obj_arr || read_cache.obj_arr->isAutoGarbage());
    assert(!read_cache.val_arr || read_cache.val_arr->isAutoObjGarbage());
#endif
    delete read_cache.obj_arr;

    delete read_cache.oid_arr;
    delete read_cache.val_arr;
    read_cache.obj_arr = 0;
    read_cache.oid_arr = 0;
    read_cache.val_arr = 0;
    unvalidReadCache();
  }

#define CONVERT(VALUE_ARRAY, TYPE, OFF)					\
  do {									\
    for (int i = (OFF); i < (VALUE_ARRAY)->value_cnt; i += (OFF)+1)	\
      {									\
	Value v = (VALUE_ARRAY)->values[i];				\
	TYPE k;								\
	memcpy(&k, v.data, sizeof(TYPE));				\
	(VALUE_ARRAY)->values[i].set(k);				\
      }									\
  } while(0)

  static int
  value_index_cmp (const void *x1, const void *x2)
  {
    Value *v1 = (Value *)x1;
    Value *v2 = (Value *)x2;
    return v1->i - v2->i;
  }

  static void sortValues(ValueArray &value_array)
  {
    qsort(value_array.getValues(), value_array.getCount()/2, 2*sizeof(Value),
	  value_index_cmp);
  }

  Status Collection::getElements(ValueArray &value_array, Bool index) const
  {
    if (status)
      return Exception::make(status);

#ifdef TRY_GETELEMS_GC
    if (value_array.isAutoObjGarbage()) {
      return Exception::make(IDB_ERROR,
			     "Collection::getElements(ValueArray &): "
			     "ValueArray argument cannot be in auto-object-garbaged mode");
    }
#endif

    Status s =  const_cast<Collection *>(this)->getValElementsRealize(index);
    if (s) return s;

    value_array = *read_cache.val_arr;
    return Success;
  }

  Status
  Collection::getValElementsRealize(Bool index)
  {
    IDB_COLL_LOAD_DEFERRED();

    if (read_cache.val_arr &&
	read_cache_state_value == coherent &&
	read_cache_state_index == index)
      return Success;
 
    delete read_cache.val_arr;
#ifdef TRY_GETELEMS_GC
    read_cache.val_arr = new ValueArray(true);
#else
    read_cache.val_arr = new ValueArray();
#endif

    if (getOidC().isValid()) {
      Iterator q(this, index);
      if (q.getStatus())
	return q.getStatus();
      
      Status s = q.scan(*read_cache.val_arr);
      
      if (s) return s;
    }

    if (read_cache_state_value == coherent &&
	read_cache_state_index == index) {
      assert(CACHE_LIST_COUNT(cache) == 0);
      /*
	int idx = (index ? 1 : 0);
	for (int i = idx; i < read_cache.val_arr->value_cnt; i += idx+1)
	if (read_cache.val_arr->values[i].type == Value::tObject)
	read_cache.val_arr->values[i].o->incrRefCount();
      */
      return Success;
    }

    if (read_cache_state_value != coherent ||
	read_cache_state_index != index) {
      ValueItem *item;
      ValueList *value_list = read_cache.val_arr->toList();
      
      // IMPORTANT NOTICE 15/08/06
      // Two bugs
      // - does not work when index is true
      // - the test 'if (!value_list->exists(item_value)' is not correct
      //   in case of the collection contains duplicate elements (not sets,
      //   but bags and arrays).
      //   The exists method should deal with pointers and not values.
      //   The problem is the same for getOidElements and getObjElements
      //   Question : why do we need value_list in case of cache is not
      //   coherent ?
      // patchs for these bugs : #define NEW_GET_ELEMENTS

      if (cache) {
	ValueCache::IdMapIterator begin = cache->getIdMap().begin();
	ValueCache::IdMapIterator end = cache->getIdMap().end();

	while (begin != end) {
	  item = (*begin).second;
	  const Value &item_value = item->getValue();
	  
	  int s = item->getState();
	  if (s == removed) {
	    /*
	      if (index)
	      value_list->suppressValue(Value((int)(*begin).first));
	      value_list->suppressValue(item_value);
	    */
	    if (index)
	      value_list->suppressPairValues(Value((int)(*begin).first),
					     item_value);
	    else
	      value_list->suppressValue(item_value);
	  }
	  else if (s == added) {
	    if (item_value.type == Value::tObject)
	      item_value.o->incrRefCount();
	    if (index)
	      value_list->insertValueLast(Value((int)(*begin).first));
	    value_list->insertValueLast(item_value);
	  }
	  ++begin;
	}
      }
      
      delete read_cache.val_arr;
      read_cache.val_arr = value_list->toArray();
#ifdef TRY_GETELEMS_GC
      read_cache.val_arr->setAutoObjGarbage(true);
      read_cache.val_arr->setMustRelease(false);
#endif
      delete value_list;
      
      read_cache_state_value = coherent;
      read_cache_state_index = index;
    }
    
    if (index && (asCollArray() || asCollList()))
      sortValues(*read_cache.val_arr);

    int idx = (index ? 1 : 0);

    if (isref) {
      /*
	for (int i = idx; i < read_cache.val_arr->value_cnt; i += idx+1)
	if (read_cache.val_arr->values[i].type == Value::tObject)
	read_cache.val_arr->values[i].o->incrRefCount();
      */
      return Success;
    }

    // 9/9/05: must take dim into account !!!
    // Value is of type DATA and is not decoded (XDR) :
    // - must decode here
    // - before, in IteratorAtom::decode ??

    unsigned int value_cnt = read_cache.val_arr->getCount();
    for (int i = idx; i < value_cnt; i += idx+1) {
      makeValue(const_cast<Value&>((*read_cache.val_arr)[i])); // makeValue(read_cache.val_arr->values[i]);
      /*
	if (read_cache.val_arr->values[i].type == Value::tObject)
	read_cache.val_arr->values[i].o->incrRefCount();
      */
    }

    
    return Success;
  }

  void Collection::makeValue(Value &v) {
    if (v.type == Value::tData &&
	!isref && !coll_class->asBasicClass() && !coll_class->asEnumClass()) {
      Database::consapp_t consapp = getDatabase()->getConsApp(coll_class);
      Object *o;
      if (consapp) {
	o = consapp(coll_class, 0);
	memcpy(o->getIDR() + IDB_OBJ_HEAD_SIZE, v.data.data, item_size);
      }
      else
	o = coll_class->newObj(v.data.data, True);
      
      o->setDatabase(getDatabase());
      v.set(o);
    }
  }

  Status Collection::failedCardinality() const
  {
    return Exception::make(IDB_CARDINALITY_CONSTRAINT_ERROR,
			   "items count %d does not respect constraint '%s'",
			   v_items_cnt, card->getString());
  }

  Status Collection::checkCardinality() const
  {
#ifdef CARD_TRACE
    printf("Collection::checkCardinality()\n");
#endif
    if (status != Success)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (!card)
      return Success;

    if (card_bottom_excl && (v_items_cnt <= card_bottom))
      return failedCardinality();

    if (!card_bottom_excl && (v_items_cnt < card_bottom))
      return failedCardinality();

    if (card_top == CardinalityConstraint::maxint)
      return Success;

    if (card_top_excl && (v_items_cnt >= card_top))
      return failedCardinality();

    if (!card_top_excl && (v_items_cnt > card_top))
      return failedCardinality();

    return Success;
  }

  Status Collection::realizeCardinality()
  {
    printf("Collection::realizeCardinality(%p)\n", card);
    if (!card)
      return Success;

    if (status != Success)
      return Exception::make(IDB_COLLECTION_ERROR,
			     "invalid collection status: \"%s\"",
			     status->getDesc());

    if (!getOidC().isValid())
      return Exception::make(IDB_COLLECTION_ERROR, "collection oid '%s' is not valid", name);

    IDB_CHECK_WRITE(db);

    Data temp = 0;
    Offset offset = IDB_OBJ_HEAD_SIZE;
    Size alloc_size = 0;

    cardCode(temp, offset, alloc_size);

    ObjectHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.type = type;
    hdr.size = alloc_size;
    hdr.xinfo = IDB_XINFO_CARD;

    offset = 0;

    object_header_code(&temp, &offset, &alloc_size, &hdr);

    RPCStatus rpc_status;

    rpc_status = objectWrite(db->getDbHandle(), temp, getOidC().getOid());

    free(temp);

    return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
  }

  Status Collection::cache_compile(Offset &offset,
				   Size &alloc_size,
				   unsigned char **ptemp,
				   const RecMode *rcm)
  {
    Status _status;

    if (_status = checkCardinality())
      return _status;

    unsigned int count = CACHE_LIST_COUNT(cache);

    alloc_size = IDB_OBJ_HEAD_SIZE + 2 * sizeof(eyedblib::int32) +
      (count * (item_size + sizeof(eyedblib::int32) + sizeof(char)));

    // idx_data_size + idx_data
    //  alloc_size += sizeof(eyedblib::int16) + idx_data_size;

    offset = IDB_OBJ_HEAD_SIZE;

    unsigned char *temp = (unsigned char *)malloc(alloc_size);
    //unsigned char *temp = 0;

    int32_code (&temp, &offset, &alloc_size, &v_items_cnt);

    int skip_cnt = 0;
    Offset list_cnt_offset = offset;
    eyedblib::int32 kh = count;
    int32_code (&temp, &offset, &alloc_size, &kh);

    ValueItem *item;

    if (cache) {
      ValueCache::IdMapIterator begin = cache->getIdMap().begin();
      ValueCache::IdMapIterator end = cache->getIdMap().end();

      begin = cache->getIdMap().begin();
      end = cache->getIdMap().end();

      while (begin != end) {
	item = (*begin).second;
	const Value &v = item->getValue();

	if (isref)  {
	  Oid _oid;
	  Object *_o = 0;

	  if (v.type == Value::tObject)
	    _o = v.o;
	  else if (v.type == Value::tObjectPtr)
	    _o = v.o_ptr->getObject();

	  if (_o) {
	    if (item->getState() == added &&
		rcm->getType() == RecMode_FullRecurs)  {
	      Status _status = _o->realize(rcm);
	      if (_status) {
		free(temp);
		return _status;
	      }
	    }
	    _oid = _o->getOid();
	  }
	  else
	    _oid = *v.oid;

	  if (!_oid.isValid() && item->getState() == removed) {
	    skip_cnt++;
	    ++begin;
	    continue;
	  }

	  if (!_oid.isValid() && item->getState() == added)
	    return Exception::make(IDB_COLLECTION_ERROR,
				   "cannot insert a null oid into "
				   "a collection: must store first "
				   "collection elements");
	  
	  oid_code(&temp, &offset, &alloc_size, _oid.getOid());
	}
	else {
	  buffer_code   (&temp, &offset, &alloc_size, v.getData(),
			 item_size);

	}


	Collection::ItemId id = item->getId();
	int32_code (&temp, &offset, &alloc_size, (int*)&id);
	char kc = (char)item->getState();
	char_code (&temp, &offset, &alloc_size, &kc);
	++begin;
      }
    }
    
    if (skip_cnt) {
      kh -= skip_cnt;
      int32_code (&temp, &list_cnt_offset, &alloc_size, &kh);
    }

    *ptemp = temp;

    return Success;
  }

  Status collectionMake(Database *db, const Oid *oid, Object **o,
			const RecMode *rcm, const ObjectHeader *hdr,
			Data idr, LockMode lockmode, const Class *_class)
  {
    RPCStatus rpc_status;
    Data temp;
    Oid _oid(hdr->oid_cl);

    if (_oid.isValid()) {
      Oid xoid(hdr->oid_cl);
      if (!_class)
	_class = db->getSchema()->getClass(hdr->oid_cl, True);
      if (!_class)
	return Exception::make(IDB_CLASS_NOT_FOUND, "collection class '%s'",
			       OidGetString(&hdr->oid_cl));
    }
    else
      _class = 0;

    if (!idr) {
      temp = (unsigned char *)malloc(hdr->size);
      object_header_code_head(temp, hdr);
      
      rpc_status = objectRead(db->getDbHandle(), temp, 0, 0, oid->getOid(),
			      0, lockmode, 0);
    } else {
      temp = idr;
      rpc_status = RPCSuccess;
    }

    if (rpc_status != RPCSuccess)
      return StatusMake(rpc_status);

    char locked;
    Object *card;
    int items_cnt, bottom, top;
    char *name;
    Oid idx1_oid, idx2_oid;
    Oid inv_oid;
    eyedblib::int16 inv_item = 0;
    Bool is_literal = False;
    Bool is_pure_literal = False;
    Data idx_data = 0;
    Size idx_data_size = 0;
    IndexImpl *idximpl = 0;
    int impl_type = 0;
    CollImpl *collimpl = 0;

    if (ObjectPeer::isRemoved(*hdr)) {
      locked = 0;
      card = NULL;
      collimpl = 0;
      items_cnt = bottom = top = 0;
      name = (char*)""; /*@@@@warning cast*/
    } else {
      Offset offset = IDB_OBJ_HEAD_SIZE;
    
      char_decode (temp, &offset, &locked);
    
      /* item_size */
      eyedblib::int16 kk;
      int16_decode (temp, &offset, &kk);

      Status s = IndexImpl::decode(db, temp, offset, idximpl, &impl_type);
      if (s) return s;

      if (idximpl) {
	collimpl = new CollImpl(idximpl);
      }
      else {
	collimpl = new CollImpl((CollImpl::Type)impl_type);
      }

      oid_decode (temp, &offset, idx1_oid.getOid());
      oid_decode (temp, &offset, idx2_oid.getOid());
      int32_decode (temp, &offset, &items_cnt);
      int32_decode (temp, &offset, &bottom);
      int32_decode (temp, &offset, &top);

      card = Collection::cardDecode(db, temp, offset);

      eyedbsm::Oid se_inv_oid;
      oid_decode(temp, &offset, &se_inv_oid); 
      inv_oid.setOid(se_inv_oid);
      int16_decode(temp, &offset, &inv_item);

      char c;
      /* is_literal */
      LITERAL_OFFSET = offset;
      char_decode (temp, &offset, &c);
      Collection::decodeLiteral(c, is_literal, is_pure_literal);

      //is_literal = IDBBOOL(c);
      /* idx_data */
      eyedblib::int16 sz;
      int16_decode (temp, &offset, &sz);
      idx_data_size = sz;
      idx_data = temp+offset;
      offset += idx_data_size;

      string_decode(temp, &offset, &name);
    }
  
#ifdef COLLTRACE
    printf("collection make idx_data_size=%d\n", idx_data_size);
#endif
    if (eyedb_is_type(*hdr, _CollSet_Type))
      *o = (Object *)CollectionPeer::collSet
	(name, (Class *)_class, idx1_oid, idx2_oid, items_cnt,
	 bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);

    else if (eyedb_is_type(*hdr, _CollBag_Type))
      *o = (Object *)CollectionPeer::collBag
	(name, (Class *)_class, idx1_oid, idx2_oid, items_cnt,
	 bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);

    else if (eyedb_is_type(*hdr, _CollList_Type))
      *o = (Object *)CollectionPeer::collList
	(name, (Class *)_class, idx1_oid, idx2_oid, items_cnt,
	 bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);

    else if (eyedb_is_type(*hdr, _CollArray_Type))
      *o = (Object *)CollectionPeer::collArray
	(name, (Class *)_class, idx1_oid, idx2_oid, items_cnt,
	 bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);
    else {
      if (collimpl)
	collimpl->release();
      return Exception::make(IDB_INTERNAL_ERROR, "invalid collection type: "
			     "%p", hdr->type);
    }


    if (collimpl)
      collimpl->release();
    CollectionPeer::setLock((Collection *)*o, (Bool)locked);

    if (is_literal)
      (*o)->asCollection()->setLiteralOid(*oid);

    if (inv_oid.isValid())
      CollectionPeer::setInvOid((Collection *)*o, inv_oid, inv_item);

    if (!idr)
      free(temp);

    return Success;
  }

  Status Collection::getImplementation(CollImpl *&_collimpl, Bool remote) const
  {
    if (!remote) {
      if (is_literal) {
	Status s = const_cast<Collection *>(this)->loadDeferred();
	if (s) return s;
      }
      _collimpl = collimpl->clone();
      return Success;
    }

    _collimpl = 0;

    Oid idx1oid, idx2oid;
    Status s = getIdxOid(idx1oid, idx2oid);
    if (s) return s;

    if (idx1oid.isValid()) {
      // TBD 2009-03-31
      IndexImpl *idximpl = 0;
      RPCStatus rpc_status =
	collectionGetImplementation(db->getDbHandle(),
				    collimpl->getType(),
				    idx1oid.getOid(), (Data *)&idximpl);
      if (rpc_status)
	return StatusMake(rpc_status);
    
      if (collimpl->getIndexImpl()) {
	idximpl->setHashMethod(collimpl->getIndexImpl()->getHashMethod());
      }
      if (idximpl) {
	_collimpl = new CollImpl(idximpl);
      }
      else {
	_collimpl = new CollImpl(collimpl->getType());
      }
    }

    return Success;
  }

  Status
  Collection::literalMake(Collection *o)
  {
#if 1
    is_literal = o->isLiteral();
    is_pure_literal = o->isPureLiteral();
#endif

    assert(literal_oid == o->getOid());
    oid.invalidate();
    literal_oid = o->getOid();

#if 0
    //printf("o->isLiteral %d %d\n", o->isLiteral(), o->isPureLiteral());
    Status s = loadLiteral();
    if (s)
      return s;
    //printf("isLiteral %d %d\n", isLiteral(), isPureLiteral());

    if (o->isLiteral() != isLiteral() || o->isPureLiteral() != isPureLiteral())
      printf("---------------------------- ARGH -----------------------\n");
#endif

#ifdef COLLTRACE
    printf("literalMake:: literal oid is %s\n", literal_oid.toString());
#endif
    /*
      printf("type %d %d name '%s' '%s' ordered %d %d allow_dup %d %d "
      "string_coll %d %d",
      type, o->type, name, o->name, ordered, o->ordered,
      allow_dup, o->allow_dup, string_coll, o->string_coll);
      printf("isref %d %d\n", isref, o->isref);
      printf("coll_class %p %p\n", coll_class, o->coll_class);
      printf("cache before %p %d %d %d\n",
      cache, read_cache.obj_arr, read_cache.oid_arr, read_cache.val_arr);

      printf("cl_oid %s %s\n", cl_oid.toString(), o->cl_oid.toString());
    */

    bottom = o->bottom;
    top = o->top;
    locked = o->locked;
    inv_oid = o->inv_oid;
    inv_item = o->inv_item;
  
    card = (CardinalityDescription *)CLONE(o->card);
    card_bottom = o->card_bottom;
    card_bottom_excl = o->card_bottom_excl;
    card_top = o->card_top;
    card_top_excl = o->card_top_excl;
    card_oid = o->card_oid;

    if (db && db->isBackEnd()) {
      idx1_oid = o->idx1_oid;
      idx2_oid = o->idx2_oid;
      idx1 = o->idx1;
      idx2 = o->idx2;
    }

    // added the 25/11/99
    delete cache;
    // ...
    cache = o->cache;
    if (cache)
      cache->setObject(this);

    o->cache = 0;
    p_items_cnt = o->p_items_cnt;
    v_items_cnt = o->v_items_cnt;
    inverse_valid = o->inverse_valid;
    if (!implModified) {
      if (collimpl)
	collimpl->release();
      if (o->collimpl) {
	collimpl = o->collimpl->clone();
      }
    }
    /*
      printf("literalMake idr=%p o_idr=%p refcnt=%d o_refcnt=%d\n",
      data, o->idr->idr, idr->refcnt, o->idr->refcnt);
    */
    return Success;
  }

  //
  // WARNING: the literal_oid MUST BE the first field of the idr for
  // literal collections because of inverse implementation.
  //

  Status Collection::realizePerform(const Oid& cloid,
				    const Oid& objoid,
				    AttrIdxContext &idx_ctx,
				    const RecMode *rcm)
  {
    assert(is_literal);

    CHK_OBJ(this);

#ifdef COLLTRACE
    printf("Collection::realizePerform(%p)\n", this);

    Object *master_obj = getMasterObject(true);
    printf("master_object %p\n", master_obj);
    if (master_obj)
      printf("MASTER_OBJECT %s %s [%s]\n",
	     master_obj->getOid().toString(),
	     master_obj->getClass()->getName(), idx_ctx.getString().c_str());
#endif
    if (!idx_data)
      idx_data = idx_ctx.code(idx_data_size);

    void *ud = setUserData(IDB_MAGIC_COLL);
    Status s = realize(rcm);
    (void)setUserData(ud);
    if (s)
      return s;

#ifdef COLLTRACE
    printf("Collection::realizePerform(-> %s, %p)\n", literal_oid.toString(),
	   idx_data);
#endif

    Offset offset = IDB_OBJ_HEAD_SIZE;
    Size alloc_size = idr->getSize();
    Data data = idr->getIDR();

#if 0
    eyedbsm::Oid hoid;
    oid_decode(data, &offset, &hoid);
    printf("before coding %s on %p\n", Oid(hoid).toString(), data);
    offset = IDB_OBJ_HEAD_SIZE;
    printf("coding %s on %p\n", literal_oid.toString(), data);
#endif

    oid_code(&data, &offset, &alloc_size, literal_oid.getOid());

    return Success;
  }

  Status Collection::loadPerform(const Oid&, LockMode lockmode,
				 AttrIdxContext &idx_ctx,
				 const RecMode *rcm)
  {
    Offset offset = IDB_OBJ_HEAD_SIZE;
    oid_decode(idr->getIDR(), &offset, literal_oid.getOid());

#ifdef COLLTRACE
    Object *master_obj = getMasterObject(true);
    printf("master_object %p %s %s [%s]\n",
	   master_obj, master_object->getOid().toString(),
	   master_obj->getClass()->getName(),
	   (const char *)idx_ctx.getString());

    printf("Collection::loadPerform(%p, %s)\n", this, literal_oid.toString());
#endif

    if (!idx_data)
      idx_data = idx_ctx.code(idx_data_size);

    // 17/09/01: moved from bottom
    if (literal_oid.isValid())
      is_complete = False;

    if (rcm == RecMode::NoRecurs) {
      // 17/09/01: moved this code above ^
      /*
	if (literal_oid.isValid())
	is_complete = False;
      */
      return Success;
    }

    return Collection::loadDeferred(lockmode, rcm);
  }

  Status
  Collection::loadDeferred(LockMode lockmode, const RecMode *rcm)
  {
    if (is_complete) // added the 10/11/99
      return Success;

    Collection *o;
#ifdef COLLTRACE
    printf("loading deferred %p %s\n", this, literal_oid.toString());
#endif

#ifdef COLL_OPTIM_LOAD
    if (!literal_oid.isValid())
      return Success;
#endif
    // changed the 29/05/02
#if 1
    Status s = db->loadObject_realize(&literal_oid, (Object **)&o,
				      lockmode, rcm);
    if (s)
      return s;
#else
    Status s = db->loadObject(literal_oid, (Object *&)o, rcm);
    if (s)
      return s;
#endif

    s = literalMake(o);
    if (s)
      return s;
    /*
      printf("releasing initial collection object o=%p refcnt=%d "
      "this=%p refcnt=%d\n",
      o, o->getRefCount(), this, getRefCount());
    */
    o->release();
    is_complete = True;
    return Success;
  }

  Status Collection::removePerform(const Oid& cloid, 
				   const Oid& objoid,
				   AttrIdxContext &idx_ctx,
				   const RecMode *rcm)
  {
#ifdef COLLTRACE
    printf("Collection::removePerform(%p, %s) should remove only if pure_literal %d %d\n", this,
	   literal_oid.toString(), is_literal, is_pure_literal);
#endif

#ifdef COLLTRACE
    printf("trying to remove literal collection %s [%s]\n",
	   literal_oid.toString(),
	   getMasterObject(true) ? getMasterObject(true)->getOid().toString() : "<no master>");
#endif

    Status s = loadLiteral();
    if (s)
      return s;
    assert(is_literal);

#ifdef COLL_OPTIM_CREATE
    if (!literal_oid.isValid())
      return Success;
#endif
    Bool was_pure_literal = is_pure_literal;

    is_literal = False;
    is_pure_literal = False;

    s = updateLiteral();
    if (s)
      return s;

    if (was_pure_literal) {
      s = db->removeObject(literal_oid, rcm);
      if (s)
	return s;
    }

    literal_oid.invalidate();
    return Success;
  }

  Status Collection::postRealizePerform(const Oid& cloid,
					const Oid& objoid,
					AttrIdxContext &idx_ctx,
					Bool &mustTouch,
					const RecMode *rcm)
  {
    mustTouch = False;
    if (!getOidC().isValid())
      return Success;

    //printf("postRealizePerform(%p)\n", this);

    // 27/06/05: disconnected
#if 0
#ifdef E_XDR
    eyedbsm::Oid xoid;
    eyedbsm::h2x_oid(&xoid, getOidC().getOid());
#else
    memcpy(&xoid, getOidC().getOid());
#endif

    // 22/05/06 : disconnected because I do not know what is it for : inverse ?
    // when disconnecting nothing happens
    Oid dataoid = idx_ctx.getDataOid();
    if (!dataoid.isValid()) {
      dataoid = objoid;
    }

#if 1
    if (getenv("EYEDB_XX"))
      printf("not writing %s in %s [%s] at %d\n",
	     getOidC().toString(),
	     dataoid.toString(),
	     objoid.toString(),
	     idx_ctx.getOff());
    else if (getenv("EYEDB_ZZ"))
      printf("setting to NULL in %s [%s] at %d\n",
	     dataoid.toString(),
	     objoid.toString(),
	     idx_ctx.getOff());
    else
      printf("writing %s in %s [%s] at %d\n",
	     getOidC().toString(),
	     dataoid.toString(),
	     objoid.toString(),
	     idx_ctx.getOff());
#endif
    
    if (getenv("EYEDB_YY")) {
      RPCStatus rpc_status =
	dataWrite(db->getDbHandle(), idx_ctx.getOff() /*idr_poff*/,
		  sizeof(eyedbsm::Oid),
		  (Data)Oid::nullOid.getOid(),
		  dataoid.getOid());

      if (rpc_status)
	return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
    }
  
    else if (!getenv("EYEDB_XX")) {
      RPCStatus rpc_status =
	dataWrite(db->getDbHandle(), idx_ctx.getOff() /*idr_poff*/,
		  sizeof(eyedbsm::Oid),
		  (Data)&xoid,
		  dataoid.getOid());

      if (rpc_status)
	return StatusMake(IDB_COLLECTION_ERROR, rpc_status);
    }
#endif
  
    Status s;
#ifdef COLLTRACE
    printf("postRealizePerform(%p, idx_data = %p)\n", getUserData(), idx_data);
#endif

    if (getUserData() != IDB_MAGIC_COLL2) {
      // WARNING: disconnected (in)validateInverse() the 5/02/01!!!!!
      // the problem is that I don't know why the (in)validateInverse()
      // were for!

      //invalidateInverse(); 
      s = realizePerform(cloid, objoid, idx_ctx, rcm);
      //validateInverse();
    }
    else {
#ifdef COLLTRACE
      printf("warning magic coll for collection %p\n", this);
#endif
      s = realizePerform(cloid, objoid, idx_ctx, rcm);
    }

    return s;
  }

  Status Collection::moveElements(const Dataspace *dataspace)
  {
    OidArray oid_arr;
    Status s = getElements(oid_arr);
    if (s)
      return s;

    return db->moveObjects(oid_arr, dataspace);
  }

  Status Collection::getElementLocations(ObjectLocationArray &locarr)
  {
    OidArray oid_arr;
    Status s = getElements(oid_arr);
    if (s)
      return s;

    return db->getObjectLocations(oid_arr, locarr);
  }

  Status Collection::getDefaultDataspace(const Dataspace *&dataspace) const
  {
    RPCStatus rpc_status;
    if (idx1_oid.isValid()) {
      int dspid;
      rpc_status = getDefaultIndexDataspace(db->getDbHandle(),
					    idx1_oid.getOid(),
					    1, &dspid);

      if (rpc_status) return StatusMake(rpc_status);
      return db->getDataspace(dspid, dataspace);
    }

    dataspace = 0;
    return Success;
  }

  Status Collection::setDefaultDataspace(const Dataspace *dataspace)
  {
    RPCStatus rpc_status;
    if (idx1_oid.isValid()) {
      rpc_status = setDefaultIndexDataspace(db->getDbHandle(),
					    idx1_oid.getOid(),
					    1, dataspace->getId());

      if (rpc_status) return StatusMake(rpc_status);
    }

    if (idx2_oid.isValid()) {
      rpc_status = setDefaultIndexDataspace(db->getDbHandle(),
					    idx2_oid.getOid(),
					    1, dataspace->getId());

      if (rpc_status) return StatusMake(rpc_status);
    }

    return Success;
  }

  Bool Collection::isPartiallyStored() const
  {
    return (cache != 0 && cache->size() > 0) ? True : False;
  }

  CollImpl *CollImpl::clone() const
  {
    if (idximpl) {
      return new CollImpl(idximpl->clone());
    }

    return new CollImpl(impl_type);
  }
}
