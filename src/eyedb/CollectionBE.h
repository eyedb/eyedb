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


#ifndef _EYEDB_COLLECTION_BE_H
#define _EYEDB_COLLECTION_BE_H

#include <eyedb/IteratorAtom.h>

namespace eyedb {

  class CollectionBE {

    eyedbsm::Idx *idx1, *idx2;
    Oid idx1_oid, idx2_oid;
    Oid oid;
    Database *db;
    DbHandle *dbh;
    Class *cls;
    Class *coll_class;
    eyedblib::int16 dim;
    Status status;
    eyedblib::int16 item_size;
    Bool isref;
    Data buff;
    int items_cnt;
    Bool locked;
    Oid inv_oid;
    eyedblib::int16 inv_num_item;
    const Attribute *inv_item;
    Bool inv_item_done;
    IndexImpl *idximpl;
    AttrIdxContext *idx_ctx;
    Bool is_literal;
    Bool is_pure_literal;
    IteratorAtomType type;

  public:
    CollectionBE(Database *, DbHandle *, const Oid *,
		 Class *,
		 const Oid&, const Oid&,
		 eyedbsm::Idx *, eyedbsm::Idx *, int, Bool, const Oid&,
		 eyedblib::int16,
		 IndexImpl *idximpl,
		 Data idx_data, Size idx_data_size,
		 Bool _is_literal,
		 Bool _is_pure_literal);
    CollectionBE(Database *, DbHandle *, const Oid *, Bool);

    void decode(const void* k, IteratorAtom &atom);
    void decode(const void* k, Data idr);
    IteratorAtomType getType();

    void getIdx(eyedbsm::Idx **, eyedbsm::Idx **);
    const Oid& getOid() const;
    const Oid& getInvOid() const {return inv_oid;}
    const eyedblib::int16 getInvNumItem() const {return inv_num_item;}
    const Attribute *getInvItem() const {return inv_item;}
    Status getInvItem(Database *, const Attribute*&, Oid&,
		      eyedbsm::Idx*&) const;
    void setInvItem(const Attribute *_inv_item) {inv_item = _inv_item;}
    Data getTempBuff();
    eyedblib::int16 getItemSize() const;
    Bool getIsRef() const;
    Database *getDatabase();
    DbHandle *getDbHandle();

    void setItemsCount(int cnt) {items_cnt = cnt;}
    int getItemsCount() const {return items_cnt;}

    Bool isBIdx() const {return IDBBOOL(idximpl->getType() == IndexImpl::BTree);}

    Class *getClass(Bool &isref, int&, eyedblib::int16&) const;

    Bool isLocked() const {return locked;}
    void unlock() {locked = False;}
    Status getStatus() const;
    ~CollectionBE();
  };

#define IDB_MAX_HINTS_CNT         8
#define IDB_IMPL_CODE_SIZE (sizeof(char) + sizeof(eyedblib::int16) + sizeof(eyedblib::int32) + sizeof(eyedbsm::Oid) + IDB_MAX_HINTS_CNT * sizeof(eyedblib::int32))

#define IDB_COLL_OFF_LOCKED        IDB_OBJ_HEAD_SIZE
#define IDB_COLL_OFF_ITEM_SIZE     (IDB_COLL_OFF_LOCKED    + sizeof(char))
#define IDB_COLL_OFF_IMPL_BEGIN    (IDB_COLL_OFF_ITEM_SIZE + sizeof(eyedblib::int16))
#define IDB_COLL_OFF_IMPL_TYPE     IDB_COLL_OFF_IMPL_BEGIN
#define IDB_COLL_OFF_IMPL_DSPID    (IDB_COLL_OFF_IMPL_TYPE + sizeof(char))
#define IDB_COLL_OFF_IMPL_INFO     (IDB_COLL_OFF_IMPL_DSPID + sizeof(eyedblib::int16))
#define IDB_COLL_OFF_IMPL_MTH      (IDB_COLL_OFF_IMPL_INFO + sizeof(eyedblib::int32))
#define IDB_COLL_OFF_IMPL_HINTS    (IDB_COLL_OFF_IMPL_MTH + sizeof(eyedbsm::Oid))
#define IDB_COLL_OFF_IDX1_OID      (IDB_COLL_OFF_IMPL_HINTS + \
                                    IDB_MAX_HINTS_CNT * sizeof(eyedblib::int32))
#define IDB_COLL_OFF_IDX2_OID      (IDB_COLL_OFF_IDX1_OID  + sizeof(eyedbsm::Oid))

#define IDB_COLL_OFF_ITEMS_CNT     (IDB_COLL_OFF_IDX2_OID  + sizeof(eyedbsm::Oid))
#define IDB_COLL_OFF_ITEMS_BOT     (IDB_COLL_OFF_ITEMS_CNT + sizeof(eyedblib::int32))
#define IDB_COLL_OFF_ITEMS_TOP     (IDB_COLL_OFF_ITEMS_BOT + sizeof(eyedblib::int32))

#define IDB_COLL_OFF_CARD_OID      (IDB_COLL_OFF_ITEMS_TOP + sizeof(eyedblib::int32))

#define IDB_COLL_OFF_INV_OID       (IDB_COLL_OFF_CARD_OID + sizeof(eyedbsm::Oid))
#define IDB_COLL_OFF_INV_ITEM      (IDB_COLL_OFF_INV_OID  + sizeof(eyedbsm::Oid))

#define IDB_COLL_OFF_COLL_NAME   (IDB_COLL_OFF_INV_ITEM  + sizeof(eyedblib::int16))

#define IDB_COLL_SIZE_HEAD       (IDB_COLL_OFF_COLL_NAME)

#define IDB_COLL_IMPL_CHANGED   (short)0x0001
#define IDB_COLL_NAME_CHANGED   (short)0x0002
}

#endif
