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

public class CollArray extends Collection {

  public CollArray(Database db, String name,
		      Class coll_cls, boolean isref)
    throws Exception
  {
    super(db, name, coll_cls, isref);
    type = ObjectHeader._CollArray_Type;
    cls = CollArrayClass.make(db, coll_cls, isref, (short)0);
  }

  public CollArray(Database db, String name, Class coll_cls)
    throws Exception
  {
    this(db, name, coll_cls, true);
  }

  public CollArray(Database db, String name,
		      Class coll_cls, short dim) throws Exception
  {
    super(db, name, coll_cls, dim);
    type = ObjectHeader._CollArray_Type;
    cls = CollArrayClass.make(db, coll_cls, false, (short)dim);
  }

  public String getClassName() {
    return "array";
  }

  CollArray(Database db, String name, Class cls,
	     int items_cnt, boolean locked, IndexImpl idximpl, int bottom,
	     int top, Oid card_oid, boolean is_literal,
	     byte idx_data[], int idx_data_size)
    throws Exception
  {
    super(db, name, cls, items_cnt, locked, idximpl, bottom, top,
	  card_oid, is_literal, idx_data, idx_data_size);

    allow_dup = false;
    ordered = false;
    type = ObjectHeader._CollArray_Type;
  }

  public void insert(Value a, boolean noDup) throws Exception {
    throw new CollectionException
      (new Status(Status.IDB_COLLECTION_ERROR),
       "cannot use non indexed insertion in an array");
  }

  public void insertAt(int ind, Oid oid) throws Exception
  {
    insertAt(ind, new Value(oid));
  }

  public void insertAt(int ind, Object o) throws Exception
  {
    insertAt(ind, new Value(o));
  }

  public void append(Value a) throws Exception {
    insertAt(getTop(), a);
  }

  public void append(Oid oid) throws Exception {
    insertAt(getTop(), oid);
  }

  public void append(Object o) throws Exception {
    insertAt(getTop(), o);
  }

  public void insertAt(int ind, Value a) throws Exception {
    if (locked)
      throw new CollectionLockedException(name, oid);

    getInsertOid(a);
    check(a);

    CollItem x = cache.get(ind);

    if (x != null)
      cache.suppress(x);
    else
      v_items_cnt++;

    cache.insert(new CollItem(a, CollItem.added, ind));
    if (ind >= top) top = ind+1;
    if (ind <= bottom) bottom = ind;
    read_cache.invalidate();
  }

  private void checkInd(int ind) 
    throws Exception {
    if (ind < getBottom() || ind >= getTop())
      throw new CollectionException
	  (new Status(Status.IDB_COLLECTION_ERROR),
	   "index out of range #" + ind + " for collection array " +
	   oid.toString());
  }

    /*
  public Value getValueAt(int where) throws Exception {
      return retrieveValueAt(where);
  }

  public Oid getOidAt(int where) throws Exception {
      return retrieveOidAt(where);
  }

  public Object getObjectAt(int where) throws Exception {
      return retrieveObjectAt(where);
  }
    */

  public Oid retrieveOidAt(int ind)
    throws Exception {
      Value val = retrieveValueAt(ind);
      if (val == null) return null;
      return val.getObjOid();
  }

  public Value retrieveValueAt(int ind)
    throws Exception {
    checkInd(ind);

    CollItem x = cache.get(ind);

    if (x != null) {
	int state = x.getState();
	if (state != CollItem.removed)
	    return x.getValue();

	return null;
      }

    if (!getOidC().isValid())
      return null;

    Value found = new Value();
    Value data_out = new Value();
    Status status;
	    
    status = RPC.rpc_COLLECTION_GET_BY_IND(db, getOidC(), ind, found,
					      data_out);
    if (!status.isSuccess())
      throw new CollectionException(status);

    byte[] data = data_out.getData();
    
    if (found.i != 0) {
	Coder coder = new Coder(data);
	return new Value(coder.decodeOid());
    }

    return null;
  }

  public Object retrieveObjectAt(int ind, RecMode rcm)
    throws Exception {
      Value val = retrieveValueAt(ind);
      if (val == null) return null;
      if (val.getType() == Value.OBJECT)
	  return val.sgetObject();
      Oid xoid = val.getOid();
      if (!xoid.isValid()) return null;
      return db.loadObject(xoid, rcm);
  }

  public Object retrieveObjectAt(int ind)
    throws Exception {
      return retrieveObjectAt(ind, RecMode.NoRecurs);
  }

  public void suppressAt(int ind) throws Exception {
    suppressAt(ind, false);
  }

  public void suppressAt(int ind, boolean checkFirst) throws Exception {
    loadDeferred();

    if (locked)
      throw new CollectionLockedException(name, oid);

    CollItem x = cache.get(ind);

    if (x != null)
      {
	int state = x.getState();
	if (state == CollItem.removed)
	  {
	    if (checkFirst)
	      return;
	    throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "item at index #" + ind + " already removed"));
	  }
	else if (state == CollItem.coherent)
	  x.setState(CollItem.removed);
	else
	  cache.suppress(x);

	v_items_cnt--;
	read_cache.invalidate();
	if (ind == top-1) --top; // in fact, more complicated than that!
	if (ind == bottom) ++bottom; // in fact, more complicated than that!
	return;
      }

    Value found = new Value();
    Value data_out = new Value();
    Status status;
	    
    status = RPC.rpc_COLLECTION_GET_BY_IND(db, getOidC(), ind, found,
					      data_out);
    if (!status.isSuccess())
      throw new CollectionException(status);

    byte[] data = data_out.getData();
    Coder coder = new Coder(data);
    Value val = new Value(coder.decodeOid());

    if (found.i == 0)
      throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, ind + ": " +" not found in array"));
	
    cache.insert(new CollItem(val, CollItem.removed, ind));
    v_items_cnt--;
    read_cache.invalidate();
    if (ind == top-1) --top; // in fact, more complicated than that!
    if (ind == bottom) ++bottom; // in fact, more complicated than that!
  }

  public int isAt(Oid oid) throws Exception {
      return isAt(new Value(oid));
  }

  public int isAt(Object o) throws Exception {
      return isAt(new Value(o));
  }

  public int isAt(Value a) throws Exception {

    CollItem x = cache.get(new CollItem(a));

    if (x != null)
      {
	if (x.getState() == CollItem.removed)
	    return -1;

	return x.getInd();
      }

    if (!getOidC().isValid())
	return -1;

    Oid o_oid = null;
    if (a.getType() == Value.OID)
	o_oid = a.sgetOid();
    else if (a.getType() == Value.OBJECT)
	o_oid = a.sgetObject().getOid();
    
    if (o_oid == null || !o_oid.isValid())
	return -1;

    Value ind = new Value();
    Value found = new Value();
    Coder coder = new Coder();
    coder.code(o_oid);

    Status status = RPC.rpc_COLLECTION_GET_BY_VALUE
      (db, getOidC(), coder.getData(), Coder.OID_SIZE, found, ind);

    if (!status.isSuccess())
      throw new CollectionException(status);

    if (found.i == 0)
	return -1;

    return ind.getInt();
  }

  public void suppress(Value a, boolean checkFirst) throws Exception {
      int ind = isAt(a);
      if (ind < 0) {
	  if (checkFirst)
	      return;
	  throw new CollectionException
	      (new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR),
	       "value " + a.toString() + " not found in array");
      }

      suppressAt(ind);
  }
}
