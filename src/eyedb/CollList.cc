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

/* CollList */

namespace eyedb {

  void CollList::init()
  {
    allow_dup = True;
    ordered = True;
    type = _CollList_Type;
    if (!status)
      setClass(CollListClass::make(coll_class, isref, dim, status));
  }

  CollList::CollList(const char *n, Class *_class,
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

  CollList::CollList(const char *n, Class *mc, Bool _isref,
		     const CollImpl *_collimpl) :
    Collection(n, mc, _isref, _collimpl)
  {
    init();
  }

  CollList::CollList(const char *n, Class *mc, int _dim,
		     const CollImpl *_collimpl) :
    Collection(n, mc, _dim, _collimpl)
  {
    init();
  }

  CollList::CollList(Database *_db, const char *n, Class *mc,
		     Bool _isref, const CollImpl *_collimpl) :
    Collection(n, mc, _isref, _collimpl)
  {
    init();
    if (!status) 
     status = setDatabase(_db);
  }

  CollList::CollList(Database *_db, const char *n, Class *mc,
		     int _dim, const CollImpl *_collimpl) :
    Collection(n, mc, _dim, _collimpl)
  {
    init();
    if (!status)
      status = setDatabase(_db);
  }

  CollList::CollList(const CollList& o) : Collection(o)
  {
    allow_dup = True;
    ordered = True;
    type = _CollList_Type;
  }

  CollList& CollList::operator=(const CollList& o)
  {
    *(Collection *)this = (Collection &)o;
    allow_dup = True;
    ordered = True;
    type = _CollList_Type;
    return *this;
  }

  const char *CollList::getClassName() const
  {
    return CollList_Class->getName();
  }

  Status CollList::insert_p(const Oid &, Bool)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "in a list, must use insertFirst, Last, Before or After");
  }

  Status CollList::insert_p(const Object *, Bool)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "in a list, must use insertFirst, Last, Before or After");
  }

  Status CollList::insert_p(Data val, Bool, Size size)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "in a list, must use insertFirst, Last, Before or After");
  }

  Status
  CollList::insert(const Value &, Bool noDup)
  {
    return Exception::make(IDB_COLLECTION_ERROR,
			   "in a list, must use insertFirst, Last, Before or After");
  }

  Status CollList::insertFirst(const Value &, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertFirst_p(const Oid &, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertFirst_p(const Object *, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertLast(const Value &, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertLast_p(const Oid &, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertLast_p(const Object *, Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertBefore_p(Collection::ItemId id, const Oid &item_oid,
				  Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertBefore_p(Collection::ItemId id, const Object *item_o,
				Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertBefore(Collection::ItemId id, const Value &item_val,
				Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertAfter_p(Collection::ItemId id, const Oid &,
			       Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertAfter_p(Collection::ItemId id, const Object *,
			       Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::insertAfter(Collection::ItemId id, const Value &,
			       Collection::ItemId *iid)
  {
    return Success;
  }

  Status CollList::replaceAt_p(Collection::ItemId id, const Oid &)
  {
    return Success;
  }

  Status CollList::replaceAt_p(Collection::ItemId id, const Object *)
  {
    return Success;
  }

  Status CollList::replaceAt(Collection::ItemId id, const Value &)
  {
    return Success;
  }

  Status CollList::suppressAt(Collection::ItemId id)
  {
    return Success;
  }

  Status CollList::retrieveAt(Collection::ItemId id, Oid &) const
  {
    return Success;
  }

  Status CollList::retrieveAt(Collection::ItemId id, Object *&, const RecMode *) const
  {
    return Success;
  }

  Status CollList::retrieveAt(Collection::ItemId id, Value &item_value) const
  {
    return Success;
  }
}
