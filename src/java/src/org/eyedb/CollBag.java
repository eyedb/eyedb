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

package org.eyedb;

public class CollBag extends Collection {

  public CollBag(Database db, String name, Class coll_cls,
		    boolean isref) throws Exception
  {
    super(db, name, coll_cls, isref);
    type = ObjectHeader._CollBag_Type;
    cls = CollBagClass.make(db, coll_cls, isref, (short)0);
  }

  public CollBag(Database db, String name, Class coll_cls)
      throws Exception
  {
    this(db, name, coll_cls, true);
  }

  public CollBag(Database db, String name, Class coll_cls,
		    short dim) throws Exception
  {
    super(db, name, coll_cls, dim);
    type = ObjectHeader._CollBag_Type;
    cls = CollBagClass.make(db, coll_cls, false, (short)dim);
  }

  public String getClassName() {
    return "bag";
  }

  CollBag(Database db, String name, Class cls,
	     int items_cnt, boolean locked, IndexImpl idximpl, int bottom,
	     int top, Oid card_oid, boolean is_literal,
	     byte idx_data[], int idx_data_size)
    throws Exception
  {
    super(db, name, cls, items_cnt, locked, idximpl, bottom, top,
	  card_oid, is_literal, idx_data, idx_data_size);

    allow_dup = true;
    ordered = false;
    type = ObjectHeader._CollBag_Type;
  }

  public void insert(Value a, boolean noDup) throws Exception {
    super.insert_realize(a, noDup);

    getInsertOid(a);

    if (noDup && isIn(a))
	return;

    CollItem x = cache.get(new CollItem(a));

    if (x != null) {
	int s = x.getState();
	if (s == CollItem.added) {
	    x.incrCount();
	}
	else if (s == CollItem.removed) {
	    x.setState(CollItem.added);
	    read_cache.invalidate();
	}
    }
    else 
	cache.insert(new CollItem(a, CollItem.added));

    v_items_cnt++;
    touch();
    read_cache.invalidate();
  }

  public static Class idbclass;
}

