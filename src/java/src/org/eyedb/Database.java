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

/**
 * This class is used to open a database and to perform operations on it.
 * <br>
 * <br>
 * For instance :
 * <pre>
 * Database db = new Database("mydatabase");
 *
 * try {
 *   db.open(conn, Database.DBRead);
 * } catch(Exception e) {
 *   System.err.println(e);
 *   System.exit(1);
 * }
 * </pre>
 * @author <a href="mailto:Eric.Viara@pobox.com">Eric Viara</a>
 */

public class Database extends Struct {

    /** use the flag DBRead to open the database in ReadOnly mode */
    public static final int DBRead = 0x2;

    /** use the flag DBRW to open the database in ReadWrite mode */
    public static final int DBRW = 0x4;

    /** use the flag DBSRead to open the database in ReadWrite mode */
    public static final int DBRSRead = 0x8;

    /** use the flag DBAdmin to open the database in ReadOnly Admin mode */
    public static final int DBReadAdmin = DBRead | 0x10;

    /** use the flag DBAdmin to open the database in ReadWrite Admin mode */
    public static final int DBRWAdmin = DBRW | 0x10;

    /**
     * constructs a database from a name.
     * <br>
     * this constructor does not open the database, only a runtime object
     * is created.
     * <br>
     * @param dbname The database name
     */

    public Database(String dbname) {
	init();
	this.dbname = dbname;
    }

    /**
     * constructs a database from a database identifier.
     * <br>
     * this constructor does not open the database, only a runtime object
     * is created.
     * <br>
     * @param dbid The database identifier
     */

    public Database(int dbid) {
	init();
	this.dbid = dbid;
    }

    /**
     * constructs a database from a name and a DBMFILE.
     * <br>
     * this constructor does not open the database, only a runtime object
     * is created.
     * <br>
     * @param dbname The database name
     * @param dbmfile The DBM (Database Manager) file
     */

    public Database(String dbname, String dbmfile) {
	init();
	this.dbname = dbname;
	this.dbmfile = dbmfile;
    }

    /**
     * constructs a database from a dbid and a DBMFILE.
     * <br>
     * this constructor does not open the database, only a runtime object
     * is created.
     * <br>
     * @param dbid The database identifier
     * @param dbmfile The DBM (Database Manager) file
     */

    public Database(int dbid, String dbmfile) {
	init();
	this.dbid = dbid;
	this.dbmfile = dbmfile;
    }

    /**
     * open a database.
     * <br>
     * @param conn The Connection
     * @param flags The opening flags : DBRW or DBRead
     * @param userauth The User authentication
     * @param passwdauth The Password authentication
     * @return None
     * <br>
     * @exception DatabaseOpeningException
     */

    public void open(Connection conn, int flags, String userauth,
		     String passwdauth) throws Exception
    {
	this.conn       = conn;
	this.userauth   = (userauth != null ? userauth : Root.user);
	this.passwdauth = (passwdauth != null ? passwdauth : Root.passwd);
	this.flags      = flags;
	this.opened_state = false;

	Status status;

	status = RPC.rpc_DBOPEN(conn,
				dbmfile,
				dbname,
				dbid,
				flags,
				this.userauth,
				this.passwdauth,
				this);

	if (status.isSuccess()) {
	    if (true) {
		transactionBegin();
		sch = (Schema)loadObject(schoid);
		if (sch != null)
		    sch.complete();
		transactionCommit();
	    }
	    else {
		/* trying low cost opening */
		sch = new Schema();
		sch.db = this;
	    }
	}
    
	if (!status.isSuccess())
	    throw new DatabaseOpeningException(status);
	else
	    this.opened_state = true;
    }

    /**
     * open a database.
     * <br>
     * @param conn The Connection
     * @param flags The opening flags : DBRW or DBRead
     * @return None
     * <br>
     * @exception DatabaseOpeningException
     */

    public void open(Connection conn, int flags)
	throws Exception
    {
	open(conn, flags, null, null);
    }

    /**
     * begins a transaction using Root.ReadWriteSharedTRMode as
     * transaction mode and Root.WriteOnCommit as writing mode
     * <br>
     * @param None
     * @return None
     * <br>
     * @exception TransactionException
     * @see Root.DefaultTRMode
     * @see Root.ReadWriteSharedTRMode
     * @see Root.WriteExclusiveTRMode
     * @see Root.ReadWriteExclusiveTRMode
     * @see Root.DefaultWRMode
     * @see Root.WriteOnCommit
     * @see Root.WriteImmediate
     */

    public void transactionBegin() throws Exception {
	if (trs_cnt++ != 0)
	    return;

	Status status = RPC.rpc_TRANSACTION_BEGIN(this, default_params);

	if (!status.isSuccess())
	    throw new TransactionException(status);
    }

    /**
     * begins a transaction using trmode
     * transaction mode and Root.WriteOnCommit as writing mode
     * <br>
     * @param None
     * @return None
     * <br>
     * @exception TransactionException
     * @see Root.DefaultTRMode
     * @see Root.ReadWriteSharedTRMode
     * @see Root.WriteExclusiveTRMode
     * @see Root.ReadWriteExclusiveTRMode
     * @see Root.DefaultWRMode
     * @see Root.WriteOnCommit
     * @see Root.WriteImmediate
     */

    public void transactionBegin(TransactionParams params) throws Exception {
	if (trs_cnt++ != 0)
	    return;

	Status status =  RPC.rpc_TRANSACTION_BEGIN(this, params);

	if (!status.isSuccess())
	    throw new TransactionException(status);
    }

    /**
     * commits the current transaction
     * @param None
     * @return None
     * <br>
     * @exception TransactionException is thrown if no transaction
     * is in course
     * <br>
     */

    public void transactionCommit() throws Exception {
	if (trs_cnt == 0)
	    throw new TransactionException(new Status(Status.IDB_NO_CURRENT_TRANSACTION,
						      "Database.transactionCommit()"));

	if (--trs_cnt != 0)
	    return;
	Status status = RPC.rpc_TRANSACTION_COMMIT(this);

	if (!status.isSuccess())
	    throw new TransactionException(status);
    }

    /**
     * aborts the current transaction
     * @param None
     * @return None
     * <br>
     * @exception TransactionException is thrown if no transaction
     * is in course
     * <br>
     */

    public void transactionAbort() throws Exception {
	if (trs_cnt == 0)
	    throw new TransactionException(new Status(Status.IDB_NO_CURRENT_TRANSACTION,
						      "Database.transactionAbort()"));
	if (--trs_cnt != 0)
	    return;

	Status status = RPC.rpc_TRANSACTION_ABORT(this);

	if (!status.isSuccess())
	    throw new TransactionException(status);
    }

    /**
     * @param None
     * @return true if in transaction, false otherwise
     */

    public boolean isInTransaction() {
	return trs_cnt > 0 ? true : false;
    }

    /** close the database
     * @param None
     * @return None
     * @exception DatabaseClosingException is thrown is the database is not
     * opened
     */

    public void close() throws Exception {
	Status status = RPC.rpc_DBCLOSE(this);
	if (!status.isSuccess())
	    throw new DatabaseClosingException(status);
    }

    /** load an object from its oid (Object IDentifier) and according
     * to the recursive mode rcm
     * @param oid The OID of the object to load
     * @param rcm The Recursive Mode to be used
     * @return The Object
     */

    public Object loadObjectRealize(Oid oid, int lockmode, RecMode rcm)
	throws Exception
    {
	//System.err.println("loadObjectRealize(" + oid + ")"); 
	Object o;

	if ((o = (Object)temp_cache.getObject(oid)) != null)
	    return o;

	int objdbid = oid.getDbid();

	if (objdbid != dbid) {
	    Database cross_db = open(conn, objdbid, DBRead, userauth,
				     passwdauth);

	    cross_db.transactionBegin();
	    
	    try {
		o = cross_db.loadObject(oid, rcm);
	    }
	    catch(Exception e) {
		cross_db.transactionCommit();
		throw e;
	    }

	    cross_db.transactionCommit();
	    
	    return o;
	}

	Status status;

	Value data_out = new Value();
	Value datid = new Value();
	status = RPC.rpc_OBJECT_READ(this, null, datid, data_out, oid);

	if (!status.isSuccess())
	    throw new LoadObjectException(status);

	byte[] idr = data_out.getData();
	ObjectHeader hdr = ObjectHeader.decode(idr);
	o = hdr.makeObject(this, oid, idr, lockmode, rcm);
    
	if (o != null) {
	    o = objectMake(o);
	    Datafile datafile = getDatafile((short)datid.getInt());
	    Dataspace dataspace = datafile.getDataspace();
	    o.setDataspace(dataspace);
	    o.make(this, oid, hdr, idr);
	    temp_cache.insertObject(oid, o);
	}

	return o;
    }

    static Object objectMake(Object o) 
	throws Exception {
	o = etcMake(o);
	o = utilsMake(o);
	return o;
    }

    public Object loadObject(Oid oid, int lockmode, RecMode rcm)
	throws Exception {
	if (oid == null || !oid.isValid())
	    return null;

	/*    if (!oid.isValid())
	      {
	      System.err.println("NOT OK3");
	      throw new LoadObjectException
	      (new Status(Status.IDB_ERROR,
	      "Database.loadObject: oid " + oid +
	      " is invalid"));
	      }
	*/

	temp_cache.empty();
	return loadObjectRealize(oid, lockmode, rcm);
    }

    public Object loadObject(Oid oid, RecMode rcm)
	throws Exception {
	return loadObject(oid, 0, rcm);
    }

    public Object loadObject(Oid oid)
	throws Exception {
	return loadObject(oid, RecMode.NoRecurs);
    }

    public void removeObject(Oid oid) throws Exception
    {
	Status status = RPC.rpc_OBJECT_DELETE(this, oid);

	if (!status.isSuccess())
	    throw new RemoveObjectException(status);
    }

    /**
     * returns the schema associated with this database
     */

    public Schema getSchema() {return sch;}

    /**
     * returns the connection associated with this database
     */

    public Connection getConnection() {return conn;}

    /**
     * returns the database name
     */

    public String getName() {return dbname;}

    /**
     * returns the database identifier
     */

    public int getDbid() {return dbid;}

    /**
     * returns the database version
     */

    public int getVersion() {return version;}

    /**
     * returns the opening flags
     */

    public int getFlags() {return flags;}

    /**
     * returns the DBMFILE associated with this database
     */

    public String getDBMFile() {return dbmfile;}

    /**
     * returns the user associated with this database
     */

    public String getUserAuth() {return userauth;}

    public int getUid() {return uid;}

    static Object etcMake(Object o) throws Exception {
	return org.eyedb.syscls.Database.makeObject(o, true);
    }

    static Object utilsMake(Object o) throws Exception {
	return org.eyedb.utils.Database.makeObject(o, true);
    }

    static Database open(Connection conn, int objdbid, int flags,
			 String userauth, String passwdauth)
	throws Exception
    {
	Database cross_db;

	if ((cross_db = OpenedDataBase.get(conn, objdbid,
					   flags, userauth, passwdauth))
	    != null)
	    return cross_db;

	cross_db = new Database(objdbid);

	cross_db.open(conn, flags, userauth, passwdauth);

	OpenedDataBase.add(cross_db);

	return cross_db;
    }

    public Class getObjectClass(Oid oid) throws Exception {
	Value state = new Value();
	Value mcloid = new Value();

	Status s = RPC.rpc_OBJECT_CHECK(this, oid, state, mcloid);

	if (!state.b || !s.isSuccess())
	    return null;

	return getSchema().getClass(mcloid.oid);
    }

    // datafile methods
    public Datafile[] getDatafiles()
	throws org.eyedb.Exception {
	return getDatafiles(false, userauth, passwdauth);
    }

    public Datafile[] getDatafiles(boolean fetch)
	throws org.eyedb.Exception {
	return getDatafiles(fetch, userauth, passwdauth);
    }

    public Datafile[] getDatafiles(boolean fetch,
				   String userauth, String passwdauth)
	throws org.eyedb.Exception {
	getDatDspPrologue(fetch, userauth, passwdauth);
	return datafiles;
    }

    public Datafile getDatafile(short id)
	throws org.eyedb.Exception {
	return getDatafile(id, false);
    }

    public Datafile getDatafile(short id, boolean fetch)
	throws org.eyedb.Exception {
	return getDatafile(id, fetch, userauth, passwdauth);
    }

    public Datafile getDatafile(short id, boolean fetch,
				String userauth, String passwdauth)
	throws org.eyedb.Exception {
	getDatDspPrologue(fetch, userauth, passwdauth);
	if (id < 0 || id >= datafiles.length)
	    return null;
	/*
	    throw new Exception
		(new Status(Status.IDB_ERROR, "datafile #" + id +
			    " not found in database " + dbname));
	*/
		 
	return datafiles[id];
    }


    public Datafile getDatafile(String name)
	throws org.eyedb.Exception {
	return getDatafile(name, false);
    }

    public Datafile getDatafile(String name, boolean fetch)
	throws org.eyedb.Exception {
	return getDatafile(name, fetch, userauth, passwdauth);
    }

    public Datafile getDatafile(String name, boolean fetch,
				String userauth, String passwdauth)
	throws org.eyedb.Exception {
	getDatDspPrologue(fetch, userauth, passwdauth);

	for (int i = 0; i < datafiles.length; i++) {
	    if (name.equals(datafiles[i].getName()) ||
		name.equals(datafiles[i].getFile()))
		return datafiles[i];
	}

	return null;
    }

    // dataspace methods
    public void moveObjects(OidArray oid_arr, Dataspace dataspace) throws org.eyedb.Exception {
	Status status;
	status = RPC.rpc_MOVE_OBJECTS(this, oid_arr.oids, dataspace.getId());

	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Database.moveOjects");

    }

    public void moveObjects(ObjectArray obj_array, Dataspace dataspace) throws org.eyedb.Exception {
	Oid oids[] = new Oid[obj_array.objects.length];
	for (int n = 0; n < oids.length; n++)
	    oids[n] = obj_array.objects[n].getOid();

	Status status;
	status = RPC.rpc_MOVE_OBJECTS(this, oids, dataspace.getId());

	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Database.moveOjects");
    }

    public Dataspace[] getDataspaces() throws org.eyedb.Exception {
	return getDataspaces(false);
    }

    public Dataspace[] getDataspaces(boolean fetch) throws org.eyedb.Exception {
	return getDataspaces(fetch, userauth, passwdauth);
    }

    public Dataspace[] getDataspaces(boolean fetch,
				     String user, String passwd) throws org.eyedb.Exception {

	getDatDspPrologue(fetch, userauth, passwdauth);
	return dataspaces;
    }

    public Dataspace getDataspace(short id) throws org.eyedb.Exception {
	return getDataspace(id, false);
    }

    public Dataspace getDataspace(short id, boolean fetch) throws org.eyedb.Exception {
	return getDataspace(id, fetch, userauth, passwdauth);
    }

    public Dataspace getDataspace(short id, boolean fetch,
				  String user, String passwd) throws org.eyedb.Exception {
	getDatDspPrologue(fetch, userauth, passwdauth);
	if (id < 0 || id >= dataspaces.length)
	    return null;

	return dataspaces[id];
    }


    public Dataspace getDataspace(String name) throws org.eyedb.Exception {
	return getDataspace(name, false);
    }

    public Dataspace getDataspace(String name, boolean fetch) throws org.eyedb.Exception {
	return getDataspace(name, fetch, userauth, passwdauth);
    }

    public Dataspace getDataspace(String name, boolean fetch,
				  String userauth, String passwdauth) throws org.eyedb.Exception {
	getDatDspPrologue(fetch, userauth, passwdauth);
	for (int i = 0; i < dataspaces.length; i++)
	    if (name.equals(dataspaces[i].getName()))
		return dataspaces[i];
	
	return null;
    }

    public Dataspace getDefaultDataspace() throws org.eyedb.Exception {
	Status status;
	Value dspid = new Value();
	status = RPC.rpc_GET_DEFAULT_DATASPACE(this, dspid);
	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Database.getDefaultDataspace");
	return getDataspace((short)dspid.sgetInt());
    }

    public void setDefaultDataspace(Dataspace dataspace) throws org.eyedb.Exception {
	Status status;
	status = RPC.rpc_SET_DEFAULT_DATASPACE(this, dataspace.getId());
	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Database.setDefaultDataspace");
    }


    public void setDefaultTransactionParams
	(TransactionParams default_params) {
	this.default_params = default_params;
    }

    public TransactionParams getDefaultTransactionParams() {
	return default_params;
    }

    public static void setGlobalDefaultTransactionParams
	(TransactionParams default_params) {
	global_default_params = default_params;
    }

    public TransactionParams getGlobalDefaultTransactionParams() {
	return global_default_params;
    }

    public DatabaseInfoDescription getInfo()
	throws org.eyedb.Exception {
	return getInfo(conn, userauth, passwdauth);
    }

    private Datafile[] getDatafiles(DatabaseInfoDescription.DataspaceInfo dsp) {
	Datafile dats[] = new Datafile[dsp.ndat];
	for (int i = 0; i < dsp.ndat; i++)
	    dats[i] = datafiles[dsp.datid[i]];
	return dats;
    }

    private void getDatDspPrologue(boolean fetch,
				   String userauth, String passwdauth)
	throws org.eyedb.Exception {

	if (datafiles != null && !fetch)
	    return;

	DatabaseInfoDescription dbinfo = getInfo(userauth, passwdauth);

	datafiles = new Datafile[dbinfo.ndat];
	for (int n = 0; n < dbinfo.ndat; n++) {
	    DatabaseInfoDescription.DatafileInfo dat = dbinfo.dat[n];
	    datafiles[n] = new Datafile(this, (short)n, dat.dspid,
					dat.file, dat.name,
					dat.maxsize, dat.mtype,
					dat.sizeslot, dat.dtype);
	}

	dataspaces = new Dataspace[dbinfo.ndsp];
	for (int n = 0; n < dbinfo.ndsp; n++) {
	    DatabaseInfoDescription.DataspaceInfo dsp = dbinfo.dsp[n];
	    dataspaces[n] = new Dataspace(this, (short)n, dsp.name,
					  getDatafiles(dsp));
	}

	for (int n = 0; n < dbinfo.ndat; n++)
	    datafiles[n].setDataspace(getDataspace((short)datafiles[n].getDspid()));
    }

    public DatabaseInfoDescription getInfo(Connection conn,
					   String userauth, String passwdauth)
	throws org.eyedb.Exception {
	Value rdbid = new Value();
	Value data_out = new Value();
	Status status = RPC.rpc_DBINFO(conn, dbmfile, userauth, passwdauth,
				       dbname, rdbid, data_out);
	if (status.isSuccess()) {
	    DatabaseInfoDescription dbinfo = new DatabaseInfoDescription();
	    Coder coder = new Coder(data_out.getData());
	    dbinfo.dbfile = coder.decodeString();
	    dbinfo.dbid = coder.decodeInt();
	    dbinfo.nbobjs = coder.decodeInt();
	    dbinfo.dbsfilesize = coder.decodeLong();
	    dbinfo.dbsfileblksize = coder.decodeLong();
	    dbinfo.ompfilesize = coder.decodeLong();
	    dbinfo.ompfileblksize = coder.decodeLong();
	    dbinfo.shmfilesize = coder.decodeLong();
	    dbinfo.shmfileblksize = coder.decodeLong();

	    dbinfo.ndat = coder.decodeInt();
	    dbinfo.ndsp = coder.decodeInt();
	    dbinfo.dat = new DatabaseInfoDescription.DatafileInfo[dbinfo.ndat];

	    for (int n = 0; n < dbinfo.ndat; n++) {
		dbinfo.dat[n] = new DatabaseInfoDescription.DatafileInfo();
		DatabaseInfoDescription.DatafileInfo dat = dbinfo.dat[n];
		dat.file = coder.decodeString();
		dat.name = coder.decodeString();
		dat.dspid = coder.decodeShort();
		dat.mtype = (short)coder.decodeInt();

		dat.sizeslot = coder.decodeInt();
		dat.maxsize = coder.decodeLong();
		dat.dtype = (short)coder.decodeInt();
		dat.extflags = coder.decodeInt();
	    }

	    dbinfo.dsp = new DatabaseInfoDescription.DataspaceInfo[dbinfo.ndsp];
	    for (int n = 0; n < dbinfo.ndsp; n++) {
		dbinfo.dsp[n] = new DatabaseInfoDescription.DataspaceInfo();
		DatabaseInfoDescription.DataspaceInfo dsp = dbinfo.dsp[n];
		dsp.name = coder.decodeString();
		dsp.ndat = coder.decodeInt();
		dsp.datid = new short[dsp.ndat];
		for (int j = 0; j < dsp.ndat; j++)
		    dsp.datid[j] = (short)coder.decodeShort();
	    }

	    return dbinfo;
	}

	throw new Exception(status);
    }


    public DatabaseInfoDescription getInfo(String userauth, String passwdauth)
	throws org.eyedb.Exception {
	return getInfo(conn, userauth, passwdauth);
    }

    public ObjectLocationArray getObjectsLocations(OidArray oid_arr)
	throws org.eyedb.Exception {
	Status status;
	ObjectLocationArray objloc_arr = new ObjectLocationArray();
	status = RPC.rpc_GET_OBJECTS_LOCATIONS(this, oid_arr.oids,
					       objloc_arr);
	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Database.getObjectsLocations");
	return objloc_arr;
    }

    private static TransactionParams global_default_params = TransactionParams.getDefaultParams();
    private TransactionParams default_params = global_default_params;

    Connection conn;
    String dbname;
    int dbid;
    int flags;
    String userauth;
    String passwdauth;
    int uid;
    int rhdbid;
    int version;
    Oid schoid;
    int trs_cnt;
    ObjCache temp_cache;
    boolean opened_state;
    Transaction curtrs, roottrs;
    Schema sch;
    String dbmfile;
    private Datafile datafiles[];
    private Dataspace dataspaces[];

    private void init() {
	conn       = null;
	dbname     = "";
	dbid       = 0;
	flags      = 0;
	userauth   = null;
	passwdauth = null;
	rhdbid     = 0;
	trs_cnt    = 0;
	sch       = null;
	temp_cache = new ObjCache(4);
	dbmfile    = Root.dbm;
    }

}
