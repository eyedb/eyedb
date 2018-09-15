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
#include "CollectionBE.h"
#include "OQLBE.h"
#include "BEQueue.h"
#include "kernel.h"

#include "Attribute_p.h"

#define COLLBE_BTREE

namespace eyedb {

  CollectionBE::CollectionBE(Database *_db, DbHandle *_dbh,
			     const Oid *_oid,
			     Class *_class,
			     const Oid& _idx1_oid,
			     const Oid& _idx2_oid,
			     eyedbsm::Idx *_idx1, eyedbsm::Idx *_idx2,
			     int _items_cnt, Bool _locked,
			     const Oid& _inv_oid,
			     eyedblib::int16 _inv_num_item,
			     IndexImpl *_idximpl,
			     Data idx_data, Size idx_data_size,
			     Bool _is_literal,
			     Bool _is_pure_literal) :
    cls(_class),
    db(_db), dbh(_dbh), oid(*_oid),
    idx1_oid(_idx1_oid),
    idx2_oid(_idx2_oid), idx1(_idx1), idx2(_idx2),
    items_cnt(_items_cnt), locked(_locked),
    inv_oid(_inv_oid),
    inv_num_item(_inv_num_item),
    inv_item(NULL),
    idximpl(_idximpl),
    is_literal(_is_literal),
    is_pure_literal(_is_pure_literal),
    inv_item_done(False)
  {
    status = Success;
    if (!cls) {
      isref = True;
      dim = 1;
      item_size = sizeof(eyedbsm::Oid);
    }
    else
      coll_class = cls->asCollectionClass()->getCollClass(&isref, &dim,
							  &item_size);

    buff = (unsigned char *)malloc(item_size);
    idx_ctx = new AttrIdxContext(idx_data, idx_data_size);
    /*
      printf("CollectionBE_1(size=%d, %s, %s, inv_oid=%s, is_literal=%d)\n",
      idx_data_size, _oid->toString(), (const char *)idx_ctx->getString(),
      inv_oid.toString(), is_literal);
    */

    type = (IteratorAtomType)0;

    if (is_pure_literal) {
      assert(inv_oid.isValid());
      assert(idx_data_size);
    }
  }

  CollectionBE::CollectionBE(Database *_db, DbHandle *_dbh,
			     const Oid *_oid,
			     Bool _locked) :
    db(_db), dbh(_dbh), oid(*_oid), locked(_locked), inv_item(NULL), inv_item_done(False)
  {
    // must load collection and open index!!
    static const int _size = IDB_COLL_SIZE_HEAD + sizeof(char) + sizeof(eyedblib::int16);
    unsigned char temp[_size];

    buff = 0;
    idx_ctx = 0;
    idx1 = idx2 = 0;
    RPCStatus rpc_status = dataRead(db->getDbHandle(), 0, _size,
					temp, 0, oid.getOid());
    status = StatusMake(rpc_status);
    if (status)
      return;

    Offset offset;

    eyedbsm::Oid oid_cl = ClassOidDecode(temp);
    cls = db->getSchema()->getClass(oid_cl);
    if (!cls) {
      isref = True;
      dim = 1;
      item_size = sizeof(eyedbsm::Oid);
    }
    else
      coll_class = ((CollectionClass *)cls)->getCollClass(&isref, &dim, &item_size);

    offset = IDB_COLL_OFF_ITEMS_CNT;
    int32_decode(temp, &offset, &items_cnt);
  
    offset = IDB_COLL_OFF_IDX1_OID;
    oid_decode(temp, &offset, idx1_oid.getOid());

    offset = IDB_COLL_OFF_IDX2_OID;
    oid_decode(temp, &offset, idx2_oid.getOid());
  
    offset = IDB_COLL_OFF_INV_OID;
    eyedbsm::Oid se_inv_oid;
    oid_decode(temp, &offset, &se_inv_oid);
    inv_oid.setOid(se_inv_oid);
    int16_decode(temp, &offset, &inv_num_item);
  
    is_literal = False;
    is_pure_literal = False;
    if (db->getVersionNumber() >= 20414) {
      offset = IDB_COLL_OFF_COLL_NAME;
      char x;
      char_decode(temp, &offset, &x);
      Collection::decodeLiteral(x, is_literal, is_pure_literal);
      //is_literal = IDBBOOL(x);
      eyedblib::int16 idx_data_size;
      int16_decode(temp, &offset, &idx_data_size);
      assert(offset == sizeof(temp));

      if (is_pure_literal) {
	assert(inv_oid.isValid());
	assert(idx_data_size);
      }

      if (idx_data_size) {
	unsigned char *x = (unsigned char*)malloc(idx_data_size);
	rpc_status = dataRead(db->getDbHandle(), _size, idx_data_size,
				  x, 0, oid.getOid());
	status = StatusMake(rpc_status);
	if (status)
	  return;

	idx_ctx = new AttrIdxContext(x, idx_data_size);
	free(x);
	/*  printf("CollectionBE_2(%s, %s)\n",
	    _oid->toString(), (const char *)idx_ctx->getString());
	*/
      }
    }

    /*
      printf("inv_oid for collection %s is %s [is_literal = %d]\n",
      inv_oid.toString(), _oid->toString(), is_literal);
    */

    offset = IDB_COLL_OFF_IMPL_BEGIN;
    IndexImpl::decode(db, temp, offset, idximpl);
    /*
      int mag_order;
      offset = IDB_COLL_OFF_MAG_ORDER;
      int32_decode (temp, &offset, &mag_order);

      // NCOLLIMPL: should be taken from IDB_COLL_OFF_IMPL
      is_bidx = IDB_IS_COLL_BIDX(mag_order);
    */

    if (isBIdx())
      idx1 = new eyedbsm::BIdx
	(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	 *idx1_oid.getOid());
    else
      idx1 = new eyedbsm::HIdx
	(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	 idx1_oid.getOid(), 0, 0);

    static const char fmt_error[] = "storage manager error '%s' reported when opening index in back end collection";

    if (idx1->status())
      {
	status = Exception::make(IDB_COLLECTION_BACK_END_ERROR, fmt_error, eyedbsm::statusGet(idx1->status()));
	delete idx1;
	idx1 = 0;
	idx2 = 0;
	return;
      }

    if (idx2_oid.isValid()) {
#ifdef COLLBE_BTREE
      if (1) 
#else
      if (isBIdx())
#endif
	idx2 = new eyedbsm::BIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   *idx2_oid.getOid());
      else
	idx2 = new eyedbsm::HIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   idx2_oid.getOid(), 0, 0);
      
      if (idx2->status()) {
	status = Exception::make(IDB_COLLECTION_BACK_END_ERROR, fmt_error, eyedbsm::statusGet(idx2->status()));
	delete idx1;
	delete idx2;
	idx1 = 0;
	idx2 = 0;
	return;
      }
    }
    else
      idx2 = 0;

    status = Success;
    type = (IteratorAtomType)0;
    buff = (unsigned char *)malloc(item_size);
  }

  
  IteratorAtomType
  CollectionBE::getType()
  {
    if (type)
      return type;

    if (isref && dim == 1)
      return IteratorAtom_OID;

    if (!cls)
      return IteratorAtom_IDR;

    const char *name = cls->asCollectionClass()->getCollClass()->getName();

    if (!strcmp(name, char_class_name) && dim > 1)
      return IteratorAtom_STRING;

    assert(dim == 1);

    if (!strcmp(name, char_class_name))
      return IteratorAtom_CHAR;

    if (!strcmp(name, int16_class_name))
      return IteratorAtom_INT16;

    if (!strcmp(name, int32_class_name))
      return IteratorAtom_INT32;

    if (!strcmp(name, int64_class_name))
      return IteratorAtom_INT64;

    if (!strcmp(name, float_class_name))
      return IteratorAtom_DOUBLE;

    return IteratorAtom_IDR;
  }

  void
  CollectionBE::decode(const void* k, IteratorAtom &atom)
  {
    type = getType();

    atom.type = type;

    Offset offset = 0;

    if (type == IteratorAtom_OID)
      oid_decode((Data)k, &offset, &atom.oid);

    else if (type == IteratorAtom_INT16)
      int16_decode((Data)k, &offset, &atom.i16);

    else if (type == IteratorAtom_INT32)
      int32_decode((Data)k, &offset, &atom.i32);

    else if (type == IteratorAtom_INT64)
      int64_decode((Data)k, &offset, &atom.i64);

    else if (type == IteratorAtom_DOUBLE)
      double_decode((Data)k, &offset, &atom.d);

    else if (type == IteratorAtom_CHAR)
      memcpy(&atom.c, k, item_size);

    else if (type == IteratorAtom_STRING)
      atom.str = strdup((char *)k);

    else if (type == IteratorAtom_IDR) {
      atom.data.size = item_size;
      atom.data.idr = (unsigned char *)malloc(item_size);
      memcpy(atom.data.idr, k, item_size);
    }

    else
      assert(0);
  }

  void
  CollectionBE::decode(const void* k, Data idr)
  {
    IteratorAtom atom;
    decode(k, atom);

    if (type == IteratorAtom_OID)
      memcpy(idr, &atom.oid, item_size);

    else if (type == IteratorAtom_INT16)
      memcpy(idr, &atom.i16, item_size);

    else if (type == IteratorAtom_INT32)
      memcpy(idr, &atom.i32, item_size);

    else if (type == IteratorAtom_INT64)
      memcpy(idr, &atom.i64, item_size);

    else if (type == IteratorAtom_DOUBLE)
      memcpy(idr, &atom.d, item_size);

    else if (type == IteratorAtom_CHAR)
      memcpy(idr, &atom.c, item_size);

    else if (type == IteratorAtom_STRING)
      memcpy(idr, atom.str, item_size);

    else if (type == IteratorAtom_IDR)
      memcpy(idr, atom.data.idr, item_size);

    else
      assert(0);
  }

  Status
  CollectionBE::getInvItem(Database *_db,
			   const Attribute *&_inv_item,
			   Oid& _inv_oid, eyedbsm::Idx *&se_idx) const
  {
    _inv_oid = inv_oid;

    if (!inv_oid.isValid()) {
      _inv_item = 0;
      assert(!is_pure_literal);
      return Success;
    }
  
    Index *idx = 0;
    Status s;
    if (inv_item_done) {
      _inv_item = inv_item;
      if (inv_item && idx_ctx) {
	s = const_cast<Attribute *>(inv_item)->indexPrologue(db, *idx_ctx, idx, True);
	if (s) return s;
      }
    
      se_idx = (idx ? idx->idx : 0);
      return Success;
    }

    // 31/08/05 : for class removal
    Oid inv_cls_oid;
    //printf("inv_oid %s\n", inv_oid.getString());
    if (s = _db->getObjectClass(inv_oid, inv_cls_oid))
      return s;

    Bool removed;
    if (s = _db->isRemoved(inv_cls_oid, removed)) 
      return s;

    if (removed) {
      const_cast<CollectionBE *>(this)->inv_item = 0;
      const_cast<CollectionBE *>(this)->inv_item_done = True;
      _inv_item = inv_item;
      se_idx = 0;
      return Success;
    }
    // ...

    Class *inv_class;

    if (s = _db->getObjectClass(inv_oid, inv_class))
      return s;

    const_cast<CollectionBE *>(this)->inv_item =
      (idx_ctx && idx_ctx->getClassOwner() ? idx_ctx->getAttribute(inv_class) :
       inv_class->getAttributes()[inv_num_item]);

    const_cast<CollectionBE *>(this)->inv_item_done = True;
    assert(inv_item);

    _inv_item = inv_item;

    if (idx_ctx) {
      s = const_cast<Attribute *>(inv_item)->indexPrologue(db, *idx_ctx, idx, True);
      if (s) return s;
    }

    se_idx = (idx ? idx->idx : 0);

    return Success;
  }

  CollectionBE::~CollectionBE()
  {
    //printf("removing collbe: %p %d\n", this, locked);
    db->getBEQueue()->removeCollection(this, dbh);
    if (idximpl)
      idximpl->release();
    idximpl = 0;
    free(buff);
    delete idx1;
    delete idx2;
    delete idx_ctx;
    idx1 = idx2 = 0;
    idx_ctx = 0;
  }

  Class *CollectionBE::getClass(Bool &_isref,
				int &_dim,
				eyedblib::int16 &_item_size) const
  {
    _isref = isref;
    _dim = dim;
    _item_size = item_size;
    return cls;
  }

  void CollectionBE::getIdx(eyedbsm::Idx **ridx1, eyedbsm::Idx **ridx2)
  {
    if (ridx1)
      *ridx1 = idx1;
    if (ridx2)
      *ridx2 = idx2;
  }

  Data CollectionBE::getTempBuff()
  {
    return buff;
  }

  eyedblib::int16 CollectionBE::getItemSize() const
  {
    return item_size;
  }

  Bool CollectionBE::getIsRef() const
  {
    return isref;
  }

  const Oid& CollectionBE::getOid() const
  {
    return oid;
  }

  Database *CollectionBE::getDatabase()
  {
    return db;
  }

  DbHandle *CollectionBE::getDbHandle()
  {
    return dbh;
  }

  Status CollectionBE::getStatus() const
  {
    return status;
  }
}
