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

public class CollSet extends Collection {

  public CollSet(Database db, String name, Class coll_cls,
		    boolean isref) throws Exception
  {
    super(db, name, coll_cls, isref);
    type = ObjectHeader._CollSet_Type;
    cls = CollSetClass.make(db, coll_cls, isref, (short)0);
  }

  public CollSet(Database db, String name, Class coll_cls)
      throws Exception
  {
    this(db, name, coll_cls, true);
  }

  public CollSet(Database db, String name, Class coll_cls,
		    short dim) throws Exception
  {
    super(db, name, coll_cls, dim);
    type = ObjectHeader._CollSet_Type;
    cls = CollSetClass.make(db, coll_cls, false, (short)dim);
  }

  public String getClassName() {
    return "set";
  }

  CollSet(Database db, String name, Class cls,
	     int items_cnt, boolean locked, IndexImpl idximpl, int bottom,
	     int top, Oid card_oid, boolean is_literal,
	     byte idx_data[], int idx_data_size)
    throws Exception
  {
    super(db, name, cls, items_cnt, locked, idximpl, bottom, top,
	  card_oid, is_literal, idx_data, idx_data_size);

    allow_dup = false;
    ordered = false;
    type = ObjectHeader._CollSet_Type;
  }

  public void insert(Value a, boolean noDup) throws Exception {
    super.insert_realize(a, noDup);

    CollItem x = cache.get(new CollItem(a));

    if (x != null) {
	int s = x.getState();
	if (s == CollItem.added) {
	    if (noDup)
		return;
	    throw new CollectionDuplicateException(a.toString());
	}
	else if (s == CollItem.removed) {
	    x.setState(CollItem.added);
	    read_cache.invalidate();
	    return;
	}
    }
    else if (getOidC().isValid()) {
	Oid o_oid = getInsertOid(a);
	if (o_oid.isValid()) {
	    Value found = new Value();
	    Value ind = new Value();
	    Status status;
	    
	    Coder coder = new Coder();
	    coder.code(o_oid);
	    status = RPC.rpc_COLLECTION_GET_BY_VALUE(db, getOidC(),
							coder.getData(),
							item_size, found, ind);
	    if (found.i != 0)
		throw new CollectionDuplicateException(a.toString());
	    
	    if (!status.isSuccess())
		throw new CollectionException(status);
	}
    }

    touch();
    cache.insert(new CollItem(a, CollItem.added));
    v_items_cnt++;
    read_cache.invalidate();
  }

  public static Class idbclass;
}

