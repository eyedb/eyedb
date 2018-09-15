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

namespace eyedb {

  int CollectionPeer::coherent = Collection::coherent,
    CollectionPeer::added = Collection::added,
    CollectionPeer::removed = Collection::removed;

  CollSet *CollectionPeer::collSet(const char *name,
				   const CollImpl *collimpl)
  {
    return new CollSet(name, 0, True, collimpl);
  }

  CollSet *CollectionPeer::collSet(const char *name, Class *cls,
				   const Oid& idx1_oid,
				   const Oid& idx2_oid, int icnt,
				   int bottom, int top,
				   const CollImpl *collimpl,
				   Object *card,
				   Bool is_literal,
				   Bool is_pure_literal,
				   Data idx_data, Size idx_data_size)
  {
    return new CollSet(name, cls, idx1_oid, idx2_oid, icnt,
		       bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);
  }

  CollBag *CollectionPeer::collBag(const char *name,
				   const CollImpl *collimpl)
  {
    return new CollBag(name, 0, True, collimpl);
  }

  CollBag *CollectionPeer::collBag(const char *name, Class *cls,
				   const Oid& idx1_oid,
				   const Oid& idx2_oid, int icnt,
				   int bottom, int top,
				   const CollImpl *collimpl,
				   Object *card,
				   Bool is_literal,
				   Bool is_pure_literal,
				   Data idx_data, Size idx_data_size)
  {
    return new CollBag(name, cls, idx1_oid, idx2_oid, icnt,
		       bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);
  }

  CollList *CollectionPeer::collList(const char *name,
				     const CollImpl *collimpl)
  {
    return new CollList(name, 0, True, collimpl);
  }

  CollList *CollectionPeer::collList(const char *name, Class *cls,
				     const Oid& idx1_oid,
				     const Oid& idx2_oid, int icnt,
				     int bottom, int top,
				     const CollImpl *collimpl,
				     Object *card,
				     Bool is_literal,
				     Bool is_pure_literal,
				     Data idx_data, Size idx_data_size)
  {
    return new CollList(name, cls, idx1_oid, idx2_oid, icnt,
		       bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);
  }

  CollArray *CollectionPeer::collArray(const char *name,
				       const CollImpl *collimpl)
  {
    return new CollArray(name, 0, True, collimpl);
  }

  CollArray *CollectionPeer::collArray(const char *name, Class *cls,
				       const Oid& idx1_oid,
				       const Oid& idx2_oid, int icnt,
				       int bottom, int top,
				       const CollImpl *collimpl,
				       Object *card,
				       Bool is_literal,
				       Bool is_pure_literal,
				       Data idx_data, Size idx_data_size)
  {
    return new CollArray(name, cls, idx1_oid, idx2_oid, icnt,
			 bottom, top, collimpl, card, is_literal, is_pure_literal, idx_data, idx_data_size);
  }

  void CollectionPeer::setLock(Collection *coll, Bool locked)
  {
    coll->locked = locked;
  }

  void CollectionPeer::setInvOid(Collection *coll, const Oid& inv_oid,
				 int inv_item)
  {
    coll->inv_oid = inv_oid;
    coll->inv_item = inv_item;
  }

  Bool CollectionPeer::isLocked(Collection *coll)
  {
    Database *db = coll->getDatabase();
    return IDBBOOL(coll->locked && (!db || !(db->getOpenFlag() & _DBAdmin)));
  }
}
