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

import java.util.*;

public class Collection extends Instance {

    static Object make(Database db, Oid oid, byte[] idr,
		       Oid class_oid, int lockmode, RecMode rcm)
	throws Exception
    {
	Coder coder = new Coder(idr);

	Class cls = db.getSchema().getClass(class_oid);

	coder.setOffset(ObjectHeader.IDB_OBJ_HEAD_TYPE_INDEX);

	int type = coder.decodeInt();

	coder.setOffset(ObjectHeader.IDB_OBJ_HEAD_SIZE);

	boolean locked = (coder.decodeChar() != 0 ? true : false);

	short item_size = coder.decodeShort();
    
	IndexImpl idximpl = IndexImpl.decode(db, coder);

	Oid idx1_oid, idx2_oid;

	idx1_oid = coder.decodeOid(); // not used in frontend
	idx2_oid = coder.decodeOid(); // not used in frontend

	int items_cnt = coder.decodeInt();

	int bottom = coder.decodeInt();
	int top = coder.decodeInt();
	Oid card_oid = coder.decodeOid();

	Oid inv_oid = coder.decodeOid();
	short inv_item = coder.decodeShort();

	boolean is_literal = false;
	int idx_data_size = 0;
	byte idx_data[] = null;

	is_literal = coder.decodeChar() != 0 ? true : false;
	idx_data_size = coder.decodeShort();
	idx_data = coder.decodeBuffer(idx_data_size);

	String name = coder.decodeString();

	Object o;

	if ((type & ObjectHeader._CollSet_Type) == 
	    ObjectHeader._CollSet_Type)
	    o = new CollSet(db, name, cls, items_cnt, locked, idximpl,
			    bottom, top, card_oid, is_literal,
			    idx_data, idx_data_size);
	else if ((type & ObjectHeader._CollBag_Type) == 
		 ObjectHeader._CollBag_Type)
	    o = new CollBag(db, name, cls, items_cnt, locked, idximpl,
			    bottom, top, card_oid, is_literal,
			    idx_data, idx_data_size);
	else if ((type & ObjectHeader._CollArray_Type) == 
		 ObjectHeader._CollArray_Type)
	    o = new CollArray(db, name, cls, items_cnt, locked, idximpl,
			      bottom, top, card_oid, is_literal,
			      idx_data, idx_data_size);
	else
	    o = null;

	if (inv_oid.isValid())
	    {
		((Collection)o).inv_oid = inv_oid;
		((Collection)o).inv_item = inv_item;
	    }

	if (is_literal)
	    ((Collection)o).setLiteralOid(oid);

	return o;
    }

    Collection(Database db, String name, Class coll_cls,
	       boolean isref) throws Exception
    {
	super(db);
	cache = new CollCache();
	this.coll_cls = coll_cls;
	this.isref = isref;
	this.dim = 0;
	this.name = name;
	this.locked = false;
	this.idx_data = null;
	this.idx_data_size = 0;
	this.is_literal = false;
	this.is_complete = true;
	this.literal_oid = new Oid();
	this.inv_oid = new Oid();
	this.card_oid = new Oid();
	this.inv_item = 0;
	this.idximpl = IndexImpl.getDefaultIndexImpl();
	setObjectType();
    }

    Collection(Database db, String name, Class coll_cls,
	       short dim) throws Exception
    {
	super(db);
	cache = new CollCache();
	this.coll_cls = coll_cls;
	this.isref = false;
	this.dim = dim;
	this.name = name;
	this.locked = false;
	this.idx_data = null;
	this.idx_data_size = 0;
	this.is_literal = false;
	this.is_complete = true;
	this.literal_oid = new Oid();
	this.inv_oid = new Oid();
	this.card_oid = new Oid();
	this.inv_item = 0;
	this.idximpl = IndexImpl.getDefaultIndexImpl();
	setObjectType();
    }

    Collection(Database db, String name, Class cls,
	       int items_cnt, boolean locked, IndexImpl idximpl,
	       int bottom, int top, Oid card_oid, boolean is_literal,
	       byte idx_data[], int idx_data_size)
	throws Exception
    {
	super(db);

	this.cls = cls;
	this.name = name;
	this.items_cnt = items_cnt;
	this.v_items_cnt = this.items_cnt;
	this.locked = locked;
	this.is_literal = is_literal;
	this.is_complete = true;
	this.literal_oid = new Oid();
	this.inv_oid = new Oid();
	this.idx_data = idx_data;
	this.idx_data_size = idx_data_size;
	this.idximpl = idximpl;
	this.bottom = bottom;
	this.top = top;
	this.card_oid = card_oid;
	this.cache = new CollCache();
	this.inv_item = 0;

	if (cls instanceof CollectionClass)
	    {
		CollectionClass mcoll = (CollectionClass)cls;
		this.dim = mcoll.dim;
		this.isref = mcoll.isref;
		this.coll_cls = mcoll.coll_cls;
		setObjectType();
	    }
	else
	    throw new CollectionInvalidObjectTypeException("1");
    }

    private void setObjectType()
	throws Exception
    {
	if (coll_cls == null)
	    {
		obj_type = Value.OID;
		item_size = 8;
		//System.out.println("warning coll cls is null " + oid);
		return;
	    }

	String name = coll_cls.getName();

	if (isref)
	    {
		obj_type = Value.OID;
		item_size = 8;
	    }
	else if (dim <= 1)
	    {
		if (name.equals("int") || name.equals("int32"))
		    {
			obj_type = Value.INT;
			item_size = 4;
		    }
		else if (name.equals("int16"))
		    {
			obj_type = Value.SHORT;
			item_size = 2;
		    }
		else if (name.equals("float"))
		    {
			obj_type = Value.DOUBLE;
			item_size = 8;
		    }
		else if (name.equals("char"))
		    {
			obj_type = Value.CHAR;
			item_size = 1;
		    }
		else if (name.equals("byte"))
		    {
			obj_type = Value.BYTE;
			item_size = 1;
		    }
		else
		    throw new CollectionInvalidObjectTypeException("2");
	    }
	else if (name.equals("char"))
	    {
		obj_type = Value.STRING;
		item_size = dim;
	    }
	else
	    throw new CollectionInvalidObjectTypeException("3");
    }
  
    private void traceContents(java.io.PrintStream out, Indent indent,
			       int flags, RecMode rcm) throws Exception
    {
	indent.push();
	boolean isidx = ((this instanceof CollArray) ? true : false);
    
	Iterator iter = new Iterator(this, isidx);
      
	ValueArray value_array = new ValueArray();
	iter.scan(value_array);
      
	int inc = (isidx ? 2 : 1);
	int value_off = (isidx ? 1 : 0);
	int ind = 0;
      
	Value[] values = value_array.getValues();
      
	for (int i = 0; i < value_array.getCount(); i += inc)
	    {
		Value value = values[i+value_off];
	  
		if (isidx)
		    ind = values[i].sgetInt(); // ind = values[i].i;
	  
		if (value.getType() == Value.OID)
		    {
			Object o = db.loadObject(value.getOid(), rcm);
	      
			if (isidx)
			    out.print(indent + "[" + ind + "] = " +
				      value.getOid().getString() + " " +
				      o.getClass(true).getName() + " ");
			else
			    out.print(indent + value.getOid().getString() + " " +
				      o.getClass(true).getName() + " = ");
			//indent.push();
			o.traceRealize(out, indent, flags, rcm);
			//indent.pop();
		    }
		else if (isidx)
		    out.print(indent + "[" + ind + "] = " +
			      value.toString() + " = ");
		else
		    out.print(indent + value.toString() + ";");
	    }
	indent.pop();
    }

    public void traceRealize(java.io.PrintStream out, Indent indent,
			     int flags, RecMode rcm) throws Exception {
    
	loadDeferred();

	out.print(getClassName() + " { ");

	if ((flags & Object.NativeTrace) == Object.NativeTrace)
	    {
		out.println(indent + "object::class cls = " +
			    cls.getOid() + ";");
	
		out.println(indent + "collclass = " + coll_cls.getOid() + ";");
	
	    }

	out.println("");

	indent.push();

	out.println(indent + "name      = \""    + name + "\";");
	out.println(indent + "count     = "     + v_items_cnt + ";");
	if (this instanceof CollArray)
	    out.println(indent + "range     = [" + bottom + ", " + top + "[;");
	out.println(indent + "dimension = "       + dim + ";");
	out.println(indent + "reference = "     + isref + ";");
	//out.println(indent + "magorder  = " + mag_order + ";");

	if ((flags & Object.ContentsTrace) != 0)
	    {
		out.println(indent + "contents  = {");
		traceContents(out, indent, flags, rcm);
		out.println(indent + "};");
	    }

	indent.pop();
	out.println(indent + "};");
    }

    public void trace(java.io.PrintStream out, int flags, RecMode rcm) throws Exception {

	out.print(oid.getString() + " " + cls.getName() + " = ");
	traceRealize(out, new Indent(), flags, rcm);
    }

    public String getClassName() {
	return null;
    }

    public boolean isIn(Oid oid) throws Exception {
	return isIn(new Value(oid));
    }

    public boolean isIn(Object obj) throws Exception {
	return isIn(new Value(obj));
    }

    public boolean isIn(Value a) throws Exception {

	CollItem x = cache.get(new CollItem(a));

	if (x != null)
	    {
		if (x.getState() == CollItem.removed)
		    return false;

		return true;
	    }

	if (!getOidC().isValid())
	    return false;

	Oid o_oid = null;
	if (a.getType() == Value.OID)
	    o_oid = a.sgetOid();
	else if (a.getType() == Value.OBJECT)
	    o_oid = a.sgetObject().getOid();
    
	if (o_oid == null || !o_oid.isValid())
	    return false;

	Value ind = new Value();
	Value found = new Value();
	Coder coder = new Coder();
	coder.code(o_oid);

	Status status = RPC.rpc_COLLECTION_GET_BY_VALUE
	    (db, getOidC(), coder.getData(), Coder.OID_SIZE, found, ind);

	if (!status.isSuccess())
	    throw new CollectionException(status);

	if (found.i == 0)
	    return false;

	return true;
    }

    public void insert(Oid oid, boolean noDup) throws Exception
    {
	insert(new Value(oid), noDup);
    }

    public void insert(Object o, boolean noDup) throws Exception
    {
	if (o == null)
	    throw new CollectionException(new Status(Status.IDB_COLLECTION_INSERT_ERROR, "cannot insert a null object"));

	insert(new Value(o), noDup);
    }

    public void insert(Value a, boolean noDup) throws Exception
    {
	throw new CollectionException(new Status(Status.IDB_COLLECTION_INSERT_ERROR, "not implemented"));
    }

    protected void insert_realize(Value a, boolean noDup) throws Exception
    {
	loadDeferred();
	if (locked)
	    throw new CollectionLockedException(name, oid);
	check(a);
    }

    public void insert(Oid oid) throws Exception
    {
	insert(oid, false);
    }

    public void insert(Object o) throws Exception
    {
	insert(o, false);
    }

    public void insert(Value a) throws Exception
    {
	insert(a, false);
    }

    public void suppress(Oid oid, boolean checkFirst) throws Exception
    {
	suppress(new Value(oid), checkFirst);
    }

    public void suppress(Object o, boolean checkFirst) throws Exception
    {
	suppress(new Value(o), checkFirst);
    }

    public void suppress(Value a, boolean checkFirst) throws Exception
    {
	loadDeferred();
	if (locked)
	    throw new CollectionLockedException(name, oid);
	check(a);

	CollItem x = cache.get(new CollItem(a));

	if (x != null)
	    {
		int s = x.getState();
		if (s == CollItem.removed)
		    {
			if (checkFirst)
			    return;
			throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "item '" + a.toString() + "' is already suppressed"));
		    }
		else if (s == CollItem.coherent)
		    x.setState(CollItem.removed);
		else if (s == CollItem.added)
		    cache.suppress(x);

		read_cache.invalidate();
		touch();
		v_items_cnt--;
		return;
	    }

	if (!getOidC().isValid())
	    throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "collection oid is invalid (collection has not been realized)"));
	Oid o_oid = getSuppressOid(a);

	Value ind = new Value();
	Value found = new Value();
	Coder coder = new Coder();
	coder.code(o_oid);

	Status status = RPC.rpc_COLLECTION_GET_BY_VALUE
	    (db, getOidC(), coder.getData(), Coder.OID_SIZE, found, ind);

	if (!status.isSuccess())
	    throw new CollectionException(status);

	if (found.i == 0)
	    {
		if (checkFirst)
		    return;
		throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "item '" + a.toString() + "' not found in collection '" +
							 name + "'"));
	    }

	touch();
	cache.insert(new CollItem(a, CollItem.removed, v_items_cnt));
	v_items_cnt--;
	read_cache.invalidate();
    }

    public void suppress(Oid oid) throws Exception
    {
	suppress(new Value(oid), false);
    }

    public void suppress(Object o) throws Exception
    {
	suppress(new Value(o), false);
    }

    public void suppress(Value a) throws Exception
    {
	suppress(a, false);
    }

    protected void check(Value a) throws Exception
    {
	if (obj_type != Value.OID)
	    throw new CollectionException(new Status("only collection " +
						     "objects are supported"));
	if (a.getType() != Value.OBJECT && a.getType() != Value.OID)
	    throw new CollectionInvalidValueTypeException();

	Oid a_oid;
	if (a.getType() == Value.OID)
	    a_oid = a.oid;
	else if (a.getType() == Value.OBJECT && a.o != null) {
	    Class obj_class = a.o.getClass(true);
	    if (!coll_cls.isSubClass(obj_class))
		throw new CollectionInvalidValueTypeException();
	    return;
	}
	else
	    return;

	if (a_oid.isValid())
	    {
		Class obj_class = db.getObjectClass(a_oid);
	
		if (!coll_cls.isSubClass(obj_class))
		    throw new CollectionInvalidValueTypeException();
	    }
    }

    void create_realize(RecMode rcm) throws Exception {

	if (getOidC().isValid())
	    throw new StoreException((is_literal ? "literal " : "") +
				     "collection already created");

	if (!cls.getOid().isValid())
	    cls.create();

	Coder cache_coder = null;

	if (!is_literal)
	    cache_coder = cacheCompile(rcm);

	Coder coder = new Coder();

	coder.setOffset(ObjectHeader.IDB_OBJ_HEAD_SIZE);

	/* locked byte */
	coder.code((byte)(locked ? 1 : 0));

	/* item size */
	coder.code((short)item_size);

	IndexImpl.code(idximpl, coder);

	/* idx1 oid */
	coder.code(new Oid());

	/* idx2 oid */
	coder.code(new Oid());

	/* item count */
	coder.code((int)0);

	/* bottom */
	coder.code((int)0);

	/* top */
	coder.code((int)0);

	/* cardinality */
	coder.code(card_oid);

	if (is_literal)
	    {
		Object o = get_master_object(master_object);
		if (!inv_oid.isValid())
		    inv_oid = o.getOid();

		if (!o.getOid().isValid())
		    throw new StoreException("inner object of class '" +
					     o.getClass().getName() +
					     "' containing collection of type '" +
					     cls.getName() + "' has no valid oid");
	    }

	coder.code(inv_oid);
	coder.code(inv_item);

	if (db.getVersion() >= Root.COLL_LITERAL_VERSION)
	    {
		coder.code((byte)(is_literal ? 1 : 0));
		coder.code((short)idx_data_size);
		if (idx_data != null)
		    coder.code(idx_data);
	    }

	coder.code(name);

	idr = new IDR(null);

	idr.idr = coder.getData();
	headerCode(type, coder.getOffset(), 0, true);
	idr.idr = coder.getData();

	Value value = new Value();

	Status status = RPC.rpc_OBJECT_CREATE(db, getDataspaceID(),
					      idr.idr, value);

	if (!status.isSuccess())
	    throw new StoreException(status);
	if (is_literal)
	    literal_oid = value.getOid();
	else
	    oid = value.getOid();

	if (is_literal)
	    return;

	cache_coder.code(ObjectHeader.IDB_COLL_IMPL_UNCHANGED);
	idr.idr = cache_coder.getData();
	headerCode(type, cache_coder.getOffset(), 0, true);
	idr.idr = cache_coder.getData();

	status = RPC.rpc_OBJECT_WRITE(db, idr.idr, getOidC());

	if (!status.isSuccess())
	    throw new StoreException(status);

	cache.empty();
    }

    void update_realize(RecMode rcm) throws Exception {
	Coder cache_coder = cacheCompile(rcm);

	cache_coder.code(ObjectHeader.IDB_COLL_IMPL_UNCHANGED);
	idr.idr = cache_coder.getData();
	headerCode(type, cache_coder.getOffset(), 0, true);
	idr.idr = cache_coder.getData();

	Status status = RPC.rpc_OBJECT_WRITE(db, idr.idr, getOidC());

	if (!status.isSuccess())
	    throw new StoreException(status);

	cache.empty();
    }

    private final static String IDB_MAGIC_COLL = new String("IDB_MAGIC_COLL");
    private final static String IDB_MAGIC_COLL2 = new String("IDB_MAGIC_COLL2");

    public void realize(RecMode rcm) throws Exception {

	if ((state & Realizing) != 0)
	    return;

	if (is_literal && cache.value_table.isEmpty())
	    return;

	if (is_literal && !literal_oid.isValid() &&
	    getUserData() != IDB_MAGIC_COLL)
	    {
		java.lang.Object ud = setUserData(IDB_MAGIC_COLL2);
		master_object.realize(rcm);
		setUserData(ud);
		return;
	    }
	
	loadDeferred();
	
	state |= Realizing;
	
	if (!getOidC().isValid())
	    create_realize(rcm);
	else
	    update_realize(rcm);
	
	state &= ~Realizing;
    }

    private void literalMake(Collection o) {
	is_literal = true;
	oid = new Oid();
	literal_oid = o.getOid();

	bottom = o.bottom;
	top = o.top;
	locked = o.locked;
	idximpl = o.idximpl;
	inv_oid = o.inv_oid;
	inv_item = o.inv_item;
    
	// WHY NO CARD OBJECT ??
	/*
	  card = o.card;
	  card_bottom = o.card_bottom;
	  card_bottom_excl = o.card_bottom_excl;
	  card_top = o.card_top;
	  card_top_excl = o.card_top_excl;
	*/
	card_oid = o.card_oid;
    
	cache = o.cache;
	// SHOULD RECONNECT THIS CODE
	//cache.setColl(this);
	o.cache = null;
	// WHY NO p_items_cnt in Java Collection ?
	//p_items_cnt = o.p_items_cnt;
	v_items_cnt = o.v_items_cnt;
    }

    protected void loadDeferred() throws Exception {
	loadDeferred(RecMode.NoRecurs);
    }

    private void loadDeferred(RecMode rcm) throws Exception {
	if (is_complete)
	    return;

	if (!literal_oid.isValid())
	    return;

	Object o = db.loadObject(literal_oid, rcm);
	literalMake((Collection)o);
	is_complete = true;
    }

    Class coll_cls;
    String name;
    int items_cnt;
    boolean locked;
    int v_items_cnt = 0;
    int obj_type;
    boolean is_literal;
    Oid literal_oid;
    boolean is_complete;
    int idx_data_size;
    byte idx_data[];
    IndexImpl idximpl;

    boolean isref;
    short dim;
    int item_size;

    boolean allow_dup;
    boolean ordered;
    int type;
    int bottom, top;
    Oid card_oid;
    CollCache cache;
    Oid inv_oid;
    short inv_item;
    protected CollReadCache read_cache = new CollReadCache();

    protected final int COHERENT = 1;
    protected final int ADDED    = 2;
    protected final int REMOVED  = 3;

    public Oid getLiteralOid() {return literal_oid;}
    public Oid getOidC() {return is_literal ? literal_oid : oid;}
    public boolean isLiteral() {return is_literal;}

    void setLiteral(boolean is_literal) {this.is_literal = is_literal;}
    void setLiteralOid(Oid literal_oid) {this.literal_oid = literal_oid;}

    public static Class idbclass;

    Coder cacheCompile(RecMode rcm) throws Exception {
	//System.out.println("cacheCompile");
	/* checkCardinality should be done in the backend! */

	/*if (_status = checkCardinality())
	  return _status;*/

	Hashtable table = (this instanceof CollArray ? cache.index_table : cache.value_table);

	Coder coder = new Coder();
	coder.setOffset(ObjectHeader.IDB_OBJ_HEAD_SIZE);

	boolean trace = false;
	if (trace)
	    System.out.println("** cache compile " + v_items_cnt + ", " +
			       table.size() + ", " + cache.getCount(this instanceof CollArray));
	coder.code(v_items_cnt);
	//    coder.code(table.size());
	coder.code(cache.getCount(this instanceof CollArray));

	Enumeration en = table.elements();

	while (en.hasMoreElements()) {
	    CollItem item = (CollItem)en.nextElement();
	    Value value = item.getValue();

	    for (int i = 0; i < item.getCount(); i++) {
		if (isref) {
		    Object o = value.o;
		
		    if (o != null) {
			if (rcm.getType() == RecMode.RecMode_FullRecurs)
			    o.realize(rcm);
			coder.code(o.getOid());
			if (trace) System.out.println("** coding " + o.getOid());
		    }
		    else {
			coder.code(value.oid);
			if (trace) System.out.println("** coding " + value.getOid());
		    }
		}
		else
		    coder.code(value.getData(item_size));
	    
		coder.code(item.getInd());
		if (trace) System.out.println("** coding index " + item.getInd());
		coder.code((byte)item.getState());
		if (trace) System.out.println("** coding state " + item.getState());
	    }
	}
    
	return coder;
    }

    void realizePerform(Oid clsoid, Oid objoid, AttrIdxContext idx_ctx,
			RecMode rcm) throws Exception
    {
	if (!is_literal)
	    throw new StoreException("internal error in CollectionPerform" +
				     ": collection expected to be literal");
	if (idx_data == null)
	    {
		idx_data = idx_ctx.code();
		idx_data_size = idx_data.length;
	    }

	java.lang.Object ud = setUserData(IDB_MAGIC_COLL);
	realize(rcm);
	setUserData(ud);

	Coder coder = new Coder(idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE);
	coder.code(literal_oid);
    }

    void postRealizePerform(int offset, Oid clsoid, Oid objoid,
			    AttrIdxContext idx_ctx, RecMode rcm)
	throws Exception {

	if (!getOidC().isValid())
	    return;

	Coder coder = new Coder();
	coder.code(getOidC());

	RPC.rpc_DATA_WRITE(db, offset, coder.getData(),
			   Coder.OID_SIZE, objoid);

	realizePerform(clsoid, objoid, idx_ctx, rcm);
    }

    public void loadPerform(Database db, Oid clsoid, int lockmode,
			    AttrIdxContext idx_ctx, RecMode rcm) 
	throws Exception {

	Coder coder = new Coder(idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE);
	literal_oid = coder.decodeOid();

	if (idx_data == null)
	    {
		idx_data = idx_ctx.code();
		idx_data_size = idx_data.length;
	    }

	if (rcm == RecMode.NoRecurs)
	    {
		if (literal_oid.isValid())
		    is_complete = false;
		return;
	    }

	loadDeferred(rcm);
    }

    static private Object get_master_object(Object o) {

	if (o.getMasterObject() != null)
	    return get_master_object(o.getMasterObject());

	return o;
    }

    public int getCount() throws Exception {
	loadDeferred();
	return v_items_cnt;
    }

    public int getBottom() throws Exception {
	loadDeferred();
	return bottom;
    }

    public int getTop() throws Exception {
	loadDeferred();
	return top;
    }

    public boolean isEmpty() throws Exception {
	loadDeferred();
	return v_items_cnt == 0 ? true : false;
    }

    public void setName(String name) throws Exception {
	loadDeferred();
	this.name = name;
    }

    public void getElements(ValueArray val_array, boolean index)
	throws Exception {

	boolean trace = false;
	if (!read_cache.isValidVal(index))
	    {
		if (trace) System.out.println("** read_cache not valid");
		ValueArray tmp_val_arr = new ValueArray();
		if (getOidC().isValid()) {
		    Iterator iter = new Iterator(this, index);
		    iter.scan(tmp_val_arr);
		}

		if (trace) System.out.println("** " + tmp_val_arr.getCount() +
					      " items in database");

		java.util.Vector v = tmp_val_arr.toVector();
		Hashtable table = cache.value_table;

		if (trace) System.out.println("** " + table.size() + " items in runtime cache");
		for (Enumeration e = table.elements(); e.hasMoreElements();) {
		    CollItem item = (CollItem)e.nextElement();
		    Value val = item.getValue();
		    int s = item.getState();
		    if (trace) {
			Value sval = item.getValue();
			Oid o_oid;
			if (sval.getType() == Value.OID)
			    o_oid = sval.sgetOid();
			else
			    o_oid = sval.getObject().getOid();
			System.out.println("** item " + o_oid.toString() + ", state " + s);
		    }

		    if (s == CollItem.added) {
			//if (!v.contains(val)) // I think that this test should disapear
			v.add(val);
		    }
		    else if (s == CollItem.removed) {
			boolean r = v.remove(val);
			if (!r) {
			    val = new Value(val.getObjOid());
			    r = v.remove(val);
			    if (!r)
				throw new CollectionException
				    (new Status
				     (Status.IDB_COLLECTION_ERROR, "internal cache error: oid " + val.toString() + " not found in read cache"));
			}
		    }
		}

		read_cache.val_arr = new ValueArray(v);
		if (trace) System.out.println("** read_cache.val_arr " +
					      read_cache.val_arr.getCount());
	    }
	else if (trace) System.out.println("** read cache is valid");
        
	val_array.set(read_cache.val_arr.getValues());
	read_cache.validateVal(index);
    }

    public void getElements(OidArray oid_array, boolean index)
	throws Exception {
	if (!read_cache.isValidOid(index))
	    {
		ValueArray val_arr = new ValueArray();
		getElements(val_arr, index);
		Vector v = new Vector();
		for (int i = 0; i < val_arr.getCount(); i++) {
		    Value val = val_arr.getValue(i);
		    if (val.getType() == Value.OID)
			v.add(val.sgetOid());
		    else if (val.getType() == Value.OBJECT &&
			     val.sgetObject() != null)
			v.add(val.sgetObject().getOid());
		}
		read_cache.oid_arr = new OidArray(v);
	    }

	oid_array.set(read_cache.oid_arr.getOids());
	read_cache.validateVal(index);
    }

    public void getElements(ObjectArray obj_array, boolean index,
			    RecMode rcm)
	throws Exception {
	if (!read_cache.isValidObj(index, rcm))
	    {
		ValueArray val_arr = new ValueArray();
		getElements(val_arr, index);
		Vector v = new Vector();
		for (int i = 0; i < val_arr.getCount(); i++) {
		    Value val = val_arr.getValue(i);
		    Oid o_oid = null;
		    if (val.getType() == Value.OBJECT &&
			val.sgetObject() != null) {
			o_oid = val.sgetObject().getOid();
			if (o_oid == null || !o_oid.isValid())
			    v.add(val.sgetObject());
		    }
		    else if (val.getType() == Value.OID)
			o_oid = val.sgetOid();

		    if (o_oid != null && o_oid.isValid()) {
			Object o = db.loadObject(val.sgetOid(), rcm);
			v.add(o);
		    }
		}

		read_cache.obj_arr = new ObjectArray(v);
	    }

	obj_array.set(read_cache.obj_arr.getObjects());
	read_cache.validateObj(index, rcm);
    }

    // overloaded getElements methods
    public void getElements(ValueArray val_array)
	throws Exception {
	getElements(val_array, false);
    }

    public void getElements(OidArray oid_array)
	throws Exception {
	getElements(oid_array, false);
    }

    public void getElements(ObjectArray obj_array)
	throws Exception {
	getElements(obj_array, false, RecMode.NoRecurs);
    }

    public void getElements(ObjectArray obj_array, boolean index)
	throws Exception {
	getElements(obj_array, index, RecMode.NoRecurs);
    }

    public void getElements(ObjectArray obj_array, RecMode rcm)
	throws Exception {
	getElements(obj_array, false, rcm);
    }

    // getXxxAt(int where)
    public Value getValueAt(int where)
	throws Exception {
	getElements(read_cache.val_arr, false);
	return read_cache.val_arr.getValue(where);
    }

    public Oid getOidAt(int where)
	throws Exception {
	getElements(read_cache.oid_arr, false);
	return read_cache.oid_arr.getOid(where);
    }

    public Object getObjectAt(int where, RecMode rcm)
	throws Exception {
	getElements(read_cache.obj_arr, false, rcm);
	return read_cache.obj_arr.getObject(where);
    }

    public Object getObjectAt(int where)
	throws Exception {
	return getObjectAt(where, RecMode.NoRecurs);
    }

    protected Oid getInsertOid(Value a) throws Exception {
	if (a.getType() == Value.OID) {
	    Oid o_oid = a.sgetOid();
	    if (!o_oid.isValid())
		throw new CollectionException(new Status(Status.IDB_COLLECTION_INSERT_ERROR, "cannot insert a null oid"));
	    return o_oid;
	}

	if (a.getType() == Value.OBJECT) {
	    if (a.sgetObject() == null)
		throw new CollectionException(new Status(Status.IDB_COLLECTION_INSERT_ERROR, "cannot insert a null object"));
	    return a.sgetObject().getOid();
	}

	throw new CollectionException(new Status(Status.IDB_COLLECTION_INSERT_ERROR, "only object or oid can be inserted"));
    }

    protected Oid getSuppressOid(Value a) throws Exception {
	if (a.getType() == Value.OID) {
	    Oid o_oid = a.sgetOid();
	    if (!o_oid.isValid())
		throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "cannot suppress a null oid"));
	    return o_oid;
	}

	if (a.getType() == Value.OBJECT) {
	    if (a.sgetObject() == null)
		throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "cannot suppress a null object"));
	    return a.sgetObject().getOid();
	}

	throw new CollectionException(new Status(Status.IDB_COLLECTION_SUPPRESS_ERROR, "only object or oid can be suppressed"));
    }

    Dataspace getDefaultDataspace() throws org.eyedb.Exception {
	Dataspace.notImpl("Collection.getDefaultInstanceDataspace");
	return null;
    }

    void setDefaultDataspace(Dataspace instance_dataspace)
	throws org.eyedb.Exception {
	Dataspace.notImpl("Collection.setDefaultInstanceDataspace");
	this.dataspace = dataspace;
    }

    void moveElements(Dataspace target_dataspace)
	throws org.eyedb.Exception {
	moveElements(target_dataspace, false);
    }

    void moveElements(Dataspace target_dataspace,
		      boolean include_subclasses) {
	Dataspace.notImpl("Collection.moveElements");
    }
}

