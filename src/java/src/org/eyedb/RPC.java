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

class RPC extends RPClib {

    static public final int DBMCREATE = 0x100;
    static public final int DBMUPDATE = 0x100+0x1;
    static public final int DBCREATE = 0x100+0x2;
    static public final int DBDELETE = 0x100+0x3;
    static public final int USER_ADD = 0x100+0x4;
    static public final int USER_DELETE = 0x100+0x5;
    static public final int USER_PASSWD_SET = 0x100+0x6;
    static public final int PASSWD_SET = 0x100+0x7;
    static public final int DEFAULT_DBACCESS_SET = 0x100+0x8;
    static public final int USER_DBACCESS_SET = 0x100+0x9;
    static public final int USER_SYSACCESS_SET = 0x100+0xa;
    static public final int DBINFO = 0x100+0xb;
    static public final int DBMOVE = 0x100+0xc;
    static public final int DBCOPY = 0x100+0xd;
    static public final int DBRENAME = 0x100+0xe;
    static public final int BACKEND_INTERRUPT = 0x100+0xf;
    static public final int TRANSACTION_BEGIN = 0x100+0x10;
    static public final int TRANSACTION_COMMIT = 0x100+0x11;
    static public final int TRANSACTION_ABORT = 0x100+0x12;
    static public final int TRANSACTION_PARAMS_SET = 0x100+0x13;
    static public final int TRANSACTION_PARAMS_GET = 0x100+0x14;
    static public final int DBOPEN = 0x100+0x15;
    static public final int DBOPENLOCAL = 0x100+0x16;
    static public final int DBCLOSE = 0x100+0x17;
    static public final int OBJECT_CREATE = 0x100+0x18;
    static public final int OBJECT_READ = 0x100+0x19;
    static public final int OBJECT_WRITE = 0x100+0x1a;
    static public final int OBJECT_DELETE = 0x100+0x1b;
    static public final int OBJECT_HEADER_READ = 0x100+0x1c;
    static public final int OBJECT_SIZE_MODIFY = 0x100+0x1d;
    static public final int OBJECT_CHECK = 0x100+0x1e;
    static public final int OID_MAKE = 0x100+0x1f;
    static public final int DATA_CREATE = 0x100+0x20;
    static public final int DATA_READ = 0x100+0x21;
    static public final int DATA_WRITE = 0x100+0x22;
    static public final int DATA_DELETE = 0x100+0x23;
    static public final int DATA_SIZE_GET = 0x100+0x24;
    static public final int DATA_SIZE_MODIFY = 0x100+0x25;
    static public final int VDDATA_CREATE = 0x100+0x26;
    static public final int VDDATA_WRITE = 0x100+0x27;
    static public final int VDDATA_DELETE = 0x100+0x28;
    static public final int SCHEMA_COMPLETE = 0x100+0x29;
    static public final int ATTRIBUTE_INDEX_CREATE = 0x100+0x2a;
    static public final int ATTRIBUTE_INDEX_REMOVE = 0x100+0x2b;
    static public final int INDEX_CREATE = 0x100+0x2c;
    static public final int INDEX_REMOVE = 0x100+0x2d;
    static public final int CONSTRAINT_CREATE = 0x100+0x2e;
    static public final int CONSTRAINT_DELETE = 0x100+0x2f;
    static public final int COLLECTION_GET_BY_IND = 0x100+0x30;
    static public final int COLLECTION_GET_BY_VALUE = 0x100+0x31;
    static public final int SET_OBJECT_LOCK = 0x100+0x32;
    static public final int GET_OBJECT_LOCK = 0x100+0x33;
    static public final int ITERATOR_LANG_CREATE = 0x100+0x34;
    static public final int ITERATOR_DATABASE_CREATE = 0x100+0x35;
    static public final int ITERATOR_CLASS_CREATE = 0x100+0x36;
    static public final int ITERATOR_COLLECTION_CREATE = 0x100+0x37;
    static public final int ITERATOR_ATTRIBUTE_CREATE = 0x100+0x38;
    static public final int ITERATOR_DELETE = 0x100+0x39;
    static public final int ITERATOR_SCAN_NEXT = 0x100+0x3a;
    static public final int EXECUTABLE_CHECK = 0x100+0x3b;
    static public final int EXECUTABLE_EXECUTE = 0x100+0x3c;
    static public final int EXECUTABLE_SET_EXTREF_PATH = 0x100+0x3d;
    static public final int EXECUTABLE_GET_EXTREF_PATH = 0x100+0x3e;
    static public final int SET_CONN_INFO = 0x100+0x3f;
    static public final int CHECK_AUTH = 0x100+0x40;
    static public final int INDEX_GET_COUNT = 0x100+0x41;
    static public final int INDEX_GET_STATS = 0x100+0x42;
    static public final int INDEX_SIMUL_STATS = 0x100+0x43;
    static public final int COLLECTION_GET_IMPLSTATS = 0x100+0x44;
    static public final int COLLECTION_SIMUL_IMPLSTATS = 0x100+0x45;
    static public final int INDEX_GET_IMPL = 0x100+0x46;
    static public final int COLLECTION_GET_IMPL = 0x100+0x47;
    static public final int OBJECT_PROTECTION_SET = 0x100+0x48;
    static public final int OBJECT_PROTECTION_GET = 0x100+0x49;
    static public final int OQL_CREATE = 0x100+0x4a;
    static public final int OQL_DELETE = 0x100+0x4b;
    static public final int OQL_GETRESULT = 0x100+0x4c;
    static public final int SET_LOG_MASK = 0x100+0x4d;
    static public final int GET_DEFAULT_DATASPACE = 0x100+0x4e;
    static public final int SET_DEFAULT_DATASPACE = 0x100+0x4f;
    static public final int DATASPACE_SET_CURRENT_DATAFILE = 0x100+0x50;
    static public final int DATASPACE_GET_CURRENT_DATAFILE = 0x100+0x51;
    static public final int GET_DEFAULT_INDEX_DATASPACE = 0x100+0x52;
    static public final int SET_DEFAULT_INDEX_DATASPACE = 0x100+0x53;
    static public final int GET_INDEX_LOCATIONS = 0x100+0x54;
    static public final int MOVE_INDEX = 0x100+0x55;
    static public final int GET_INSTANCE_CLASS_LOCATIONS = 0x100+0x56;
    static public final int MOVE_INSTANCE_CLASS = 0x100+0x57;
    static public final int GET_OBJECTS_LOCATIONS = 0x100+0x58;
    static public final int MOVE_OBJECTS = 0x100+0x59;
    static public final int GET_ATTRIBUTE_LOCATIONS = 0x100+0x5a;
    static public final int MOVE_ATTRIBUTE = 0x100+0x5b;
    static public final int CREATE_DATAFILE = 0x100+0x5c;
    static public final int DELETE_DATAFILE = 0x100+0x5d;
    static public final int MOVE_DATAFILE = 0x100+0x5e;
    static public final int DEFRAGMENT_DATAFILE = 0x100+0x5f;
    static public final int RESIZE_DATAFILE = 0x100+0x60;
    static public final int GET_DATAFILEI_NFO = 0x100+0x61;
    static public final int RENAME_DATAFILE = 0x100+0x62;
    static public final int CREATE_DATASPACE = 0x100+0x63;
    static public final int UPDATE_DATASPACE = 0x100+0x64;
    static public final int DELETE_DATASPACE = 0x100+0x65;
    static public final int RENAME_DATASPACE = 0x100+0x66;
    static public final int GET_SERVER_OUTOFBAND_DATA = 0x100+0x67;

    static void notImpl(String msg) {
	System.err.println("rpc_" + msg + " not implemented");
    }

    static Status rpc_DBMCREATE(Database db) {
	notImpl("DBMCREATE");

	start(DBMCREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBMUPDATE(Database db) {
	notImpl("DBMUPDATE");

	start(DBMUPDATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBCREATE(Database db) {
	notImpl("DBCREATE");

	start(DBCREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBDELETE(Database db) {
	notImpl("DBDELETE");

	start(DBDELETE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_USER_ADD(Database db) {
	notImpl("USER_ADD");

	start(USER_ADD);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_USER_DELETE(Database db) {
	notImpl("USER_DELETE");

	start(USER_DELETE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_USER_PASSWD_SET(Database db) {
	notImpl("USER_PASSWD_SET");

	start(USER_PASSWD_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_PASSWD_SET(Database db) {
	notImpl("PASSWD_SET");

	start(PASSWD_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DEFAULT_DBACCESS_SET(Database db) {
	notImpl("DEFAULT_DBACCESS_SET");

	start(DEFAULT_DBACCESS_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_USER_DBACCESS_SET(Database db) {
	notImpl("USER_DBACCESS_SET");

	start(USER_DBACCESS_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_USER_SYSACCESS_SET(Database db) {
	notImpl("USER_SYSACCESS_SET");

	start(USER_SYSACCESS_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBINFO(Connection conn, String dbmdb,
			     String userauth, String passwdauth,
			     String dbname, Value rdbid,
			     Value data_out) {
	start(DBINFO);
	addArg(dbmdb);
	addArg(userauth);
	addArg(passwdauth);
	addArg(dbname);

	Status status = new Status();

	if (realize(conn, status)) {
	    rdbid.set(getIntArg());
	    data_out.set(getDataArg());
	}

	return status;
    }

    static Status rpc_DBMOVE(Database db) {
	notImpl("DBMOVE");

	start(DBMOVE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBCOPY(Database db) {
	notImpl("DBCOPY");

	start(DBCOPY);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBRENAME(Database db) {
	notImpl("DBRENAME");

	start(DBRENAME);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_BACKEND_INTERRUPT(Database db) {
	notImpl("BACKEND_INTERRUPT");

	start(BACKEND_INTERRUPT);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_TRANSACTION_BEGIN(Database db,
					TransactionParams params) {
	start(TRANSACTION_BEGIN);

	addArg(db.rhdbid);
	addArg(params.getTransactionMode());
	addArg(params.getLockMode());
	addArg(params.getRecoveryMode());
	addArg(params.getMagnitudeOrder());
	addArg(params.getRatioAlert());
	addArg(params.getWaitTimeout());

	Status status = new Status();

	if (realize(db.conn, status))
	    {
		getIntArg(); // tid
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_TRANSACTION_COMMIT(Database db) {
	start(TRANSACTION_COMMIT);

	addArg(db.rhdbid);
	addArg(0);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_TRANSACTION_ABORT(Database db) {
	start(TRANSACTION_ABORT);

	addArg(db.rhdbid);
	addArg(0);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_TRANSACTION_PARAMS_SET(Database db) {
	notImpl("TRANSACTION_PARAMS_SET");

	start(TRANSACTION_PARAMS_SET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_TRANSACTION_PARAMS_GET(Database db) {
	notImpl("TRANSACTION_PARAMS_GET");

	start(TRANSACTION_PARAMS_GET);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBOPEN(Connection conn, String dbmfile,
			     String dbname,
			     int dbid, int flags, String userauth,
			     String passwdauth, Database db) {
	start(DBOPEN);

	addArg(dbmfile);
	addArg(userauth);
	addArg(passwdauth);
	addArg(dbname);
	addArg(dbid);
	addArg(flags);
	addArg(0);
	addArg(0);

	Status status = new Status();
	if (realize(conn, status))
	    {
		db.uid     = getIntArg();
		db.dbname  = getStringArg();
		db.dbid    = getIntArg();
		db.rhdbid  = getIntArg();
		db.version = getIntArg();
		db.schoid  = getOidArg();
	
		/*
		  System.out.println("db.uid " + db.uid +
		  ", dbname  " +  db.dbname +
		  ", dbid " + db.dbid +
		  ", db.rdbhid " + db.rhdbid +
		  ", db.version " + db.version +
		  ", db.schoid " + db.schoid);
		*/
			   
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_DBOPENLOCAL(Database db) {
	notImpl("DBOPENLOCAL");

	start(DBOPENLOCAL);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DBCLOSE(Database db) {
	start(DBCLOSE);

	addArg(db.rhdbid);

	Status status = new Status();
	if (realize(db.conn, status)) {
	    getStatusArg(db.conn, status);
	}

	return status;
    }

    static Status rpc_OBJECT_CREATE(Database db, int dspid, byte[] data,
				    Value xoid) {
	start(OBJECT_CREATE);

	addArg(db.rhdbid);
	addArg(dspid);
	addArg(data);
	addArg(xoid.sgetOid());

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		xoid.set(getOidArg());
		// ignores returned arg for now!
		new Value().set(getDataArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_OBJECT_READ(Database db, byte[] data_in,
				  Value datid, Value data_out, Oid oid) {
	start(OBJECT_READ);

	if (data_in == null || data_in.length == 0)
	    data_in = ObjectHeader.NullCodeObjectHeader;

	addArg(db.rhdbid);
	addArg(data_in);
	addArg(oid);
	addArg(0);

	Status status = new Status();

	if (realize(db.conn, status)) {
	    int d = getIntArg();
	    if (datid != null)
		datid.set(d);
	    data_out.set(getDataArg());
	    getStatusArg(db.conn, status);
	}

	return status;
    }

    static Status rpc_OBJECT_WRITE(Database db, byte[] b, Oid oid) {
	start(OBJECT_WRITE);

	addArg(db.rhdbid);
	addArg(b);
	addArg(oid);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		// ignores returned arg for now!
		new Value().set(getDataArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_OBJECT_DELETE(Database db, Oid oid) {
	start(OBJECT_DELETE);

	addArg(db.rhdbid);
	addArg(oid);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		// ignores returned arg for now!
		new Value().set(getDataArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_OBJECT_HEADER_READ(Database db) {
	start(OBJECT_HEADER_READ);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_OBJECT_SIZE_MODIFY(Database db) {
	start(OBJECT_SIZE_MODIFY);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_OBJECT_CHECK(Database db, Oid oid, Value state,
				   Value mcloid) {
	start(OBJECT_CHECK);

	addArg(db.rhdbid);
	addArg(oid);
	Status status = new Status();

	if (realize(db.conn, status))
	    {
		state.set(getIntArg() != 0);
		mcloid.set(getOidArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_OID_MAKE(Database db, int dspid, byte[] b, int size,
			       Value xoid) {
	start(OID_MAKE);

	addArg(db.rhdbid);
	addArg(dspid);
	addArg(b, ObjectHeader.IDB_OBJ_HEAD_SIZE);
	addArg(size);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		xoid.set(getOidArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_DATA_CREATE(Database db) {
	start(DATA_CREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DATA_READ(Database db, int offset, int size,
				Value datid,
				byte[] data_out, Oid oid) {
	start(DATA_READ);

	addArg(db.rhdbid);
	addArg(offset);
	addArg(size);
	addArg(oid);

	Status status = new Status();

	if (realize(db.conn, status)) {
	    int d = getIntArg();
	    if (datid != null)
		datid.set(d);
	    byte[] b = getDataArg();
	    System.arraycopy(b, 0, data_out, 0, b.length);
	    getStatusArg(db.conn, status);
	}

	return status;
    }

    static Status rpc_DATA_WRITE(Database db, int offset, byte data[],
				 int len, Oid oid) {
	start(DATA_WRITE);

	addArg(db.rhdbid);
	addArg(offset);
	addArg(data, len);
	addArg(oid);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_DATA_DELETE(Database db) {
	notImpl("DATA_DELETE");

	start(DATA_DELETE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_DATA_SIZE_GET(Database db, Oid oid,
				    Value size) {
	start(DATA_SIZE_GET);

	addArg(db.rhdbid);
	addArg(oid);
	Status status = new Status();

	if (realize(db.conn, status))
	    {
		size.set(getIntArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_DATA_SIZE_MODIFY(Database db) {
	notImpl("DATA_SIZE_MODIFY");

	start(DATA_SIZE_MODIFY);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_VDDATA_CREATE(Database db, int dspid, Oid actual_mcl_oid, 
				    Oid mcl_oid, int num,
				    int count, int size, byte[] pdata,
				    Oid o_oid, Value vd_oid,
				    byte idx_data[]) {
	start(VDDATA_CREATE);

	addArg(db.rhdbid);
	addArg(dspid);
	addArg(actual_mcl_oid);
	addArg(mcl_oid);
	addArg(num);
	addArg(count);
	addArg(pdata, size);
	addArg(idx_data);
	addArg(o_oid);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		vd_oid.set(getOidArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_VDDATA_WRITE(Database db, Oid actual_mcl_oid,
				   Oid mcl_oid,
				   int num, int count, int size,
				   byte[] pdata, Oid o_oid, Oid vd_oid,
				   byte idx_data[]) {
	start(VDDATA_WRITE);

	addArg(db.rhdbid);
	addArg(actual_mcl_oid);
	addArg(mcl_oid);
	addArg(num);
	addArg(count);
	addArg(pdata, size);
	addArg(idx_data);
	addArg(o_oid);
	addArg(vd_oid);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_VDDATA_DELETE(Database db, Oid actual_mcl_oid,
				    Oid mcl_oid,
				    int num, Oid o_oid, Oid vd_oid,
				    byte idx_data[]) {
	start(VDDATA_DELETE);

	addArg(db.rhdbid);
	addArg(actual_mcl_oid);
	addArg(mcl_oid);
	addArg(num);
	addArg(o_oid);
	addArg(vd_oid);
	addArg(idx_data);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_METASCHEME_COMPLETE(Database db) {
	notImpl("SCHEMA_COMPLETE");

	start(SCHEMA_COMPLETE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_AGREG_ITEM_INDEX_CREATE(Database db) {
	notImpl("ATTRIBUTE_INDEX_CREATE");

	start(ATTRIBUTE_INDEX_CREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_AGREG_ITEM_INDEX_REMOVE(Database db) {
	notImpl("ATTRIBUTE_INDEX_REMOVE");

	start(ATTRIBUTE_INDEX_REMOVE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_COLLECTION_GET_BY_IND(Database db, Oid colloid,
					    int ind, Value found,
					    Value data_out) {
	start(COLLECTION_GET_BY_IND);

	addArg(db.rhdbid);
	addArg(colloid);
	addArg(ind);

	Status status = new Status();

	if (realize(db.conn, status))
	    {
		found.set(getIntArg());
		data_out.set(getDataArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_COLLECTION_GET_BY_VALUE(Database db,
					      Oid oid, byte[] data,
					      int length, Value found,
					      Value ind) {
	start(COLLECTION_GET_BY_VALUE);

	addArg(db.rhdbid);
	addArg(oid);
	addArg(data);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		found.set(getIntArg());
		ind.set(getIntArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_ITERATOR_DATABASE_CREATE(Database db) {
	notImpl("ITERATOR_DATABASE_CREATE");

	start(ITERATOR_DATABASE_CREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_ITERATOR_CLASS_CREATE(Database db) {
	notImpl("ITERATOR_CLASS_CREATE");

	start(ITERATOR_CLASS_CREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_ITERATOR_COLLECTION_CREATE(Database db, Oid oid,
						 boolean index, Value value) {
	start(ITERATOR_COLLECTION_CREATE);

	addArg(db.rhdbid);
	addArg(oid);
	addArg(index ? 1 : 0);

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		value.set(getIntArg());
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_ITERATOR_AGREG_ITEM_CREATE(Database db) {
	notImpl("ITERATOR_ATTRIBUTE_CREATE");

	start(ITERATOR_ATTRIBUTE_CREATE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_ITERATOR_DELETE(Database db, int qid) {
	start(ITERATOR_DELETE);

	addArg(db.rhdbid);
	addArg(qid);

	Status status = new Status();

	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    static Status rpc_ITERATOR_SCAN_NEXT(Database db, int qid, int max,
					 ValueArray value_array) {
	start(ITERATOR_SCAN_NEXT);

	addArg(db.rhdbid);
	addArg(qid);
	addArg(max);

	Status status = new Status();
	if (realize(db.conn, status)) {
	    int count = getIntArg();
	    byte[] data = getDataArg();

	    try {
		Iterator.decode(data, value_array, 0, count);
	    }
	    catch(Exception e) {
		return e.status;
	    }

	    getStatusArg(db.conn, status);
	}
    
	return status;
    }

    static Status rpc_EXECUTABLE_CHECK(Database db) {
	notImpl("EXECUTABLE_CHECK");

	start(EXECUTABLE_CHECK);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_EXECUTABLE_EXECUTE(Database db) {
	notImpl("EXECUTABLE_EXECUTE");

	start(EXECUTABLE_EXECUTE);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_EXECUTABLE_SET_EXTREF_PATH(Database db) {
	start(EXECUTABLE_SET_EXTREF_PATH);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_EXECUTABLE_GET_EXTREF_PATH(Database db) {
	notImpl("EXECUTABLE_GET_EXTREF_PATH");

	start(EXECUTABLE_GET_EXTREF_PATH);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_SET_CONN_INFO(Connection conn, String hostname,
				    String username, String progname,
				    int pid, int version, StringBuffer challenge) {
	start(SET_CONN_INFO);

	addArg(conn.getHost() + ":" + conn.getPort());
	addArg((int)0); // uid
	addArg(username);
	addArg(progname);
	addArg(pid);
	addArg(version);

	Status status = new Status();
	if (realize(conn, status)) {
	    int xid = getIntArg(); // server pid
	    int uid = getIntArg(); // server uid
	    challenge.append( getStringArg()); // challenge
	    getStatusArg(conn, status);
	}

	return status;
    }

    static Status rpc_check_auth(Connection conn, String filename) {
	start(CHECK_AUTH);
	addArg( filename);

	Status status = new Status();
	if (realize(conn, status)) {
	    getStatusArg(conn, status);
	}

	return status;
    }

    static Status rpc_OQL_CREATE(Connection conn, Database db,
				 String query, Value vqid) {
	start(OQL_CREATE);

	addArg(db.rhdbid);
	addArg(query.getBytes());

	Status status = new Status();
	if (realize(db.conn, status))
	    {
		vqid.set(getIntArg()); // qid
		getDataArg(); // schema info
		getStatusArg(db.conn, status);
	    }

	return status;
    }

    static Status rpc_OQL_GETRESULT(Connection conn, Database db,
				    int qid, Value result) {
	start(OQL_GETRESULT);

	addArg(db.rhdbid);
	addArg(qid);

	Status status = new Status();
	if (realize(db.conn, status)) {
	    byte[] data = getDataArg();
	    try {
		result.decode(new Coder(data));
	    }
	    catch(Exception e) {
		return e.status;
	    }
	    getStatusArg(db.conn, status);
	}

	return status;
    }

    static Status rpc_OQL_DELETE(Connection conn, Database db,
				 int qid) {
	start(OQL_DELETE);

	addArg(db.rhdbid);
	addArg(qid);

	Status status = new Status();
	if (realize(db.conn, status))
	    getStatusArg(db.conn, status);

	return status;
    }

    //
    // dataspace management
    //

    static Status rpc_GET_DEFAULT_DATASPACE(Database db, Value dspid) {
	start(GET_DEFAULT_DATASPACE);
	addArg(db.rhdbid);

	Status status = new Status();
	if (realize(db.conn, status)) {
	    dspid.set(getIntArg());
	}

	return status;
    }
    
    static Status rpc_SET_DEFAULT_DATASPACE(Database db, int dspid) {
	start(SET_DEFAULT_DATASPACE);
	addArg(db.rhdbid);
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);
	return status;
    }

    // not used
    static Status rpc_GET_DEFAULT_INDEX_DATASPACE(Database db, Oid idxoid,
						  int type, Value dspid) {
	start(GET_DEFAULT_INDEX_DATASPACE);
	addArg(db.rhdbid);
	addArg(idxoid);
	addArg(type);

	Status status = new Status();
	if (realize(db.conn, status)) {
	    dspid.set(getIntArg());
	}
	
	return status;
    }

    // not used
    static Status rpc_SET_DEFAULT_INDEX_DATASPACE(Database db, Oid idxoid,
						  int type, int dspid) {
	start(SET_DEFAULT_INDEX_DATASPACE);
	addArg(db.rhdbid);
	addArg(idxoid);
	addArg(type);
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);
	return status;
    }

    static byte[] oid_code(Oid oid[]) {
	Coder coder = new Coder();
	coder.code(oid.length);
	for (int n = 0; n < oid.length; n++)
	    coder.code(oid[n]);

	return coder.getData();
    }

    static Status rpc_MOVE_OBJECTS(Database db, Oid oid[], int dspid) {
	start(MOVE_OBJECTS);
	addArg(db.rhdbid);
	addArg(oid_code(oid));
	
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    // not used
    static Status rpc_MOVE_INDEX(Database db, Oid idxoid, int type,
				 int dspid) {
	start(MOVE_INDEX);
	addArg(db.rhdbid);
	addArg(idxoid);
	addArg(type);
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static Status rpc_MOVE_INSTANCE_CLASS(Database db, Oid clsoid,
					  int subclasses, int dspid) {
	start(MOVE_INSTANCE_CLASS);
	addArg(db.rhdbid);
	addArg(clsoid);
	addArg(subclasses);
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    // not used
    static Status rpc_MOVE_ATTRIBUTE(Database db, Oid clsoid, int attrnum,
				     int dspid) {
	start(MOVE_ATTRIBUTE);
	addArg(db.rhdbid);
	addArg(clsoid);
	addArg(attrnum);
	addArg(dspid);

	Status status = new Status();
	realize(db.conn, status);

	return status;
    }

    static void decode_objloc_arr(Database db, byte data[],
				  ObjectLocationArray objloc_arr) {
	Coder coder = new Coder(data);
	int cnt = coder.decodeInt();
	Oid oid = coder.decodeOid();
	short dspid = coder.decodeShort();
	short datid = coder.decodeShort();
	ObjectLocation.Info info = new ObjectLocation.Info();
	info.size = coder.decodeInt();
	info.slot_start_num = coder.decodeInt();
	info.slot_end_num = coder.decodeInt();
	info.dat_start_pagenum = coder.decodeInt();
	info.dat_end_pagenum = coder.decodeInt();
	info.omp_start_pagenum = coder.decodeInt();
	info.omp_end_pagenum = coder.decodeInt();
	info.dmp_start_pagenum = coder.decodeInt();
	info.dmp_end_pagenum = coder.decodeInt();

	ObjectLocation objloc = new ObjectLocation(db, oid, dspid, datid, info);
	objloc_arr.add(objloc);
    }

    static Status rpc_GET_OBJECTS_LOCATIONS(Database db, Oid oids[],
					    ObjectLocationArray objloc_arr) {
	start(GET_OBJECTS_LOCATIONS);
	addArg(db.rhdbid);
	addArg(oid_code(oids));

	Status status = new Status();
	if (realize(db.conn, status)) {
	    decode_objloc_arr(db, getDataArg(), objloc_arr);
	    getStatusArg(db.conn, status);
	}
	
	return status;
    }
}
