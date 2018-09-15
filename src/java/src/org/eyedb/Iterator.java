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

public class Iterator {

    private int qid = 0;
    private Database db;
    private int count;
    private int curs = 0;
    private ValueArray tmp_val_arr = null;

    /*
     * Iterator constructors
     */

    public Iterator(Class cls)
	throws Exception
    {
	qid = 0;
	count = 0;
	Collection extent = cls.getExtent();
	if (extent == null)
	    throw new InvalidIteratorException
		(new Status(Status.IDB_ERROR, "class " + cls.getName() +
			    " " + cls.getOid().toString() + " has no extent"));
    
	queryCollCreate(extent, false);
    }

    public Iterator(Collection coll, boolean index)
	throws Exception
    {
	queryCollCreate(coll, index);
    }

    public Iterator(Collection coll)
	throws Exception
    {
	queryCollCreate(coll, false);
    }

    public int getCount() {
	return count;
    }

    /*
     * Iterator scan methods
     */

    private boolean scanNext()
	throws Exception {
	if (tmp_val_arr == null)
	    tmp_val_arr = new ValueArray();
	// should be optimized!
	// should read more objets than one at a time!
	scan(tmp_val_arr, 1, curs++);
	if (tmp_val_arr.getCount() == 0)
	    return false;
	return true;
    }

    public Object scanNextObj(RecMode rcm)
	throws Exception {
	Oid oid = scanNextOid();
	if (oid == null)
	    return null;
	return db.loadObject(oid, rcm);
    }

    public Object scanNextObj()
	throws Exception {
	return scanNextObj(RecMode.NoRecurs);
    }

    public Oid scanNextOid()
	throws Exception {
	if (!scanNext())
	    return null;
	return tmp_val_arr.getValue(0).getOid();
    }

    public Value scanNextValue()
	throws Exception {
	if (!scanNext())
	    return null;
	return tmp_val_arr.getValue(0);
    }

    public void scan(ValueArray value_array)
	throws Exception {
	scan(value_array, Integer.MAX_VALUE, 0);
    }

    public void scan(OidArray oid_array)
	throws Exception {
	scan(oid_array, Integer.MAX_VALUE, 0);
    }

    public void scan(ObjectArray obj_array)
	throws Exception {
	scan(obj_array, Integer.MAX_VALUE, 0, RecMode.NoRecurs);
    }

    public void scan(ObjectArray obj_array, RecMode rcm)
	throws Exception {
	scan(obj_array, Integer.MAX_VALUE, 0, rcm);
    }

    public void scan(ValueArray value_array, int count, int start)
	throws Exception
    {
	Status status;
	int pcount = count + (count == Integer.MAX_VALUE ? 0 : start);

	value_array.set((Value[])null);

	for (int m = 0;;)
	    {
		int wanted = Math.min(pcount, DEFAULT_WANTED);

		ValueArray tmp = new ValueArray();
		status = RPC.rpc_ITERATOR_SCAN_NEXT(db, qid, wanted, tmp);
	
		if (!status.isSuccess())
		    throw new TransactionException(status);
	
		int rcount = tmp.getCount();

		if (start >= m && start < m + rcount)
		    value_array.append(tmp, start-m, count);
		else if (m > start)
		    value_array.append(tmp, 0, count - m + start);

		//System.out.println("H4: rcount " + rcount + ", wanted " + wanted
		//+ ", value_array " + value_array.getCount());

		if (rcount < wanted)
		    {
			status = RPC.rpc_ITERATOR_DELETE(db, qid);
			if (!status.isSuccess())
			    throw new TransactionException(status);
			break;
		    }

		if (value_array.getCount() == count)
		    break;

		m += rcount; // number of value reads
	    }
    }

    public void scan(OidArray oid_array, int count, int start)
	throws Exception {
	ValueArray value_array = new ValueArray();
	oid_array.set((Oid[])null);
	int oid_count = 0;
	int cnt = count;

	for (int s = start; ; s += count)
	    {
		scan(value_array, cnt, s);
		Value[] values = value_array.getValues();

		for (int i = oid_count; i < value_array.getCount(); i++)
		    if (values[i].type == Value.OID)
			{
			    oid_array.append(values[i].oid);
			    oid_count++;
			}

		if (oid_count == count || value_array.getCount() < count)
		    break;
		cnt -= oid_count;
	    }
    }

    public void scan(ObjectArray obj_array, int count, int start)
	throws Exception {
	scan(obj_array, count, start, RecMode.NoRecurs);
    }

    public void scan(ObjectArray obj_array, int count, int start,
		     RecMode rcm) throws Exception,
					 LoadObjectException {
	obj_array.set((Object[])null);

	OidArray oid_array = new OidArray();

	scan(oid_array, count, start);

	Oid[] oids = oid_array.getOids();

	for (int i = 0; i < oid_array.getCount(); i++)
	    obj_array.append(db.loadObject(oids[i], rcm));
    }

    private void queryCollCreate(Collection coll, boolean index)
	throws Exception {
	this.db = coll.getDatabase();

	if (!db.isInTransaction())
	    throw new TransactionException
		(new Status(Status.SE_TRANSACTION_NEEDED,
			    "Iterator(" + coll.getOidC() + ")"));

	Value value = new Value();
	Status status = RPC.rpc_ITERATOR_COLLECTION_CREATE(db,
							   coll.getOidC(),
							   index, value);
    
	if (!status.isSuccess())
	    throw new InvalidIteratorException(status);

	qid = value.getInt();
	count = 0;
    }

    /*
    static final int NULL = 1;
    static final int NIL = 2;
    static final int BOOL = 3;
    static final int INT = 4;
    static final int CHAR = 5;
    static final int DOUBLE = 6;
    static final int STRING = 7;
    static final int IDENT = 8;
    static final int OID = 9;
    static final int IDR = 10;
    */

    static final int INT16 = 1;
    static final int INT32 = 2;
    static final int INT64 = 3;
    static final int CHAR = 4;
    static final int DOUBLE = 5;
    static final int STRING = 6;
    static final int OID = 7;
    static final int IDR = 8;

    static void decode(byte[] stream, ValueArray value_array, int offset,
		       int count)
	throws Exception {
	if (count == 0)
	    return;

	Coder coder = new Coder(stream);

	int n;

	Value[] values = new Value[count];

	for (n = 0; n < count; n++) {
	    short type = coder.decodeShort();

	    switch (type) {

	    case INT16:
		values[offset+n] = new Value(coder.decodeShort());
		break;

	    case INT32:
		values[offset+n] = new Value(coder.decodeInt());
		break;

	    case INT64:
		values[offset+n] = new Value(coder.decodeLong());
		break;

	    case CHAR:
		values[offset+n] = new Value(coder.decodeChar());
		break;

	    case DOUBLE:
		values[offset+n] = new Value(coder.decodeDouble());
		break;

	    case OID:
		values[offset+n] = new Value(coder.decodeOid());
		break;

	    case STRING: {
		String s = coder.decodeString();
		values[offset+n] = new Value(s);
		break;
	    }


	    case IDR: {
		/*
		  int size = coder.decodeInt();
		  byte[] b = new byte[size];
		  System.arraycopy(stream, x, b, 0, size);
		  values[offset+n] = new Value(b);
		  x += size;
		*/
	    }
		throw new InvalidIteratorTypeException(new Status("IDR decode is not yet implemented"));

		/*
		  case BOOL:
		  values[offset+n] = new Value(coder.decodeBoolean());
		  break;

		  case NULL:
		  values[offset+n] = new Value();
		  break;

		  case IDENT: {
		  String s = coder.decodeString();
		  values[offset+n] = new Value(s, true);
		  break;
		  }
		*/

	    default:
		throw new InvalidIteratorTypeException(getUnexpectedType(type));
	    }
	}

	value_array.set(values);
    }

    static Status getUnexpectedType(int type) {
	return new Status(Status.IDB_ERROR, "unexpected type " + type);
    }

    private static final int DEFAULT_WANTED = 64;
}
