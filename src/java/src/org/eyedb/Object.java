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

public class Object {

    protected Database db = null;
    protected Oid oid = new Oid();
    protected long m_time     = 0;
    protected long c_time     = 0;
    protected IDR idr     = null;
    protected boolean modify = false;
    int refcnt     = 1;
    protected org.eyedb.Class cls = null;
    protected int type;
    boolean grt_obj = false;
    protected int state      = 0;
    protected Dataspace dataspace = null;
    protected short dspid = Root.DefaultDspid;

    ObjectArray[] objects = null;
    OidArray[] oids = null;
    Data[] pdata = null;

    java.lang.Object user_data = null;

    //
    // Constructors
    //

    public Object() {
    }

    public Object(Database db) {
	this.db = db;
    }

    public Object(Database db, Dataspace dataspace) {
	this.db = db;
	this.dataspace = dataspace;
    }

    public void move(Dataspace dataspace) throws org.eyedb.Exception {
	Oid oids[] = new Oid[1];
	oids[0] = oid;
	OidArray oid_arr = new OidArray(oids);
	db.moveObjects(oid_arr, dataspace);
    }

    public ObjectLocation getLocation() throws org.eyedb.Exception {
	Oid oids[] = new Oid[1];
	oids[0] = oid;
	OidArray oid_arr = new OidArray(oids);
	return db.getObjectsLocations(oid_arr).get(0);
    }

    public Dataspace getDataspace() throws org.eyedb.Exception {
	return getDataspace(false);
    }

    public Dataspace getDataspace(boolean refetch) throws org.eyedb.Exception {

	if (oid.isValid() &&
	    ((dataspace == null && dspid == Root.DefaultDspid) || refetch)) {
	    ObjectLocation objloc = getLocation();
	    System.out.println(objloc);
	    dspid = objloc.getDspid();
	    dataspace = db.getDataspace(dspid);
	    return dataspace;
	}

	if (dataspace != null)
	    return dataspace;

	if (dspid != Root.DefaultDspid) {
	    dataspace = db.getDataspace(dspid);
	    return dataspace;
	}

	return null;
    }

    short getDataspaceID() throws org.eyedb.Exception {
	if (dataspace != null)
	    return dataspace.getId();

	return dspid;
    }

    public void setDataspace(Dataspace dataspace) throws org.eyedb.Exception {

	this.dataspace = getDataspace();

	if (oid.isValid() &&
	    this.dataspace != null &&
	    this.dataspace.getId() != dataspace.getId())
	    throw new Exception
		(new Status(Status.IDB_ERROR,
			    "use the move method to change the " +
			    "dataspace on the already created object " +
			    oid));

	this.dataspace = dataspace;
	dspid = dataspace.getId();
    }

    public Object(Object o, boolean share) {
	this.db = db;
	oid = o.oid;

	db = o.db;

	cls = o.cls;
	type = o.type;
	m_time = o.m_time;
	c_time = o.c_time;
	modify = true;
	dataspace = o.dataspace;

	grt_obj = o.grt_obj;

	objects = ObjectArray.copyArray(o.objects);
	oids    = OidArray.copyArray(o.oids);
	pdata   = Data.copyArray(o.pdata);

	if (share)
	    {
		idr = o.idr;
		idr.refcnt++;
	    }
	else if (o.idr == null)
	    idr = null;
	else
	    {
		idr = new IDR(new byte[o.idr.idr.length]);
		if (idr.idr.length != 0)
		    System.arraycopy(o.idr.idr, 0, idr.idr, 0, idr.idr.length);
	    }
    }

    //
    // Accessors
    //
    public Oid getOid() {return oid;}

    public Database getDatabase() {return db;}

    public byte[] getIDR() {return (idr != null ? idr.idr : null);}

    public int getIDRSize() {return (idr != null ? idr.idr.length : 0);}

    public long getCTime() {return c_time;}

    public long getMTime() {return m_time;}

    public int getRefCount() {return refcnt;}

    boolean isModify() {return modify;}

    public java.lang.Object getUserData() {return user_data;}

    /*
      void realize_items(RecMode rcm) throws Exception {
      }
    */

    //
    // Modifiers
    //

    public void setOid(Oid oid) {
	this.oid = oid;
    }

    public void setIDR(byte[] idr) {
	this.idr = new IDR(idr);
    }

    public void setDatabase(Database db) {
	if (this.db != db)
	    {
		this.db = db;
		this.oid = null;
	    }
	//    System.err.println("Object.setDatabase: not yet implemented!");
	//    System.exit(1);
    }

    void touch() {modify = true;}

    public java.lang.Object setUserData(java.lang.Object user_data) {
	java.lang.Object o_user_data = this.user_data;
	this.user_data = user_data;
	return o_user_data;
    }

    public boolean isGRTObject() {return grt_obj;}

    public void userInitialize() {}

    void make(Database db, Oid oid, ObjectHeader hdr, byte[] idr)
    {
	this.db  = db;
	this.oid = oid;

	m_time = hdr.mtime;
	c_time = hdr.ctime;

	if (this.idr == null)
	    this.idr = new IDR(idr);
    }

    void make(Coder coder, org.eyedb.Class cls, int type) {
	//System.out.println("Object make " + cls.getName() + " type = " + type);
	this.cls = cls;

	this.type = type;

	idr = new IDR(cls.idr_objsz);

	if (coder != null)
	    {
		System.arraycopy(coder.getData(),
				 coder.getOffset(),
				 idr.idr, 
				 ObjectHeader.IDB_OBJ_HEAD_SIZE,
				 cls.idr_psize -
				 ObjectHeader.IDB_OBJ_HEAD_SIZE);
		Coder.memzero(idr.idr, cls.idr_psize, cls.idr_vsize);
	    }
	else
	    Coder.memzero(idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE,
			  idr.idr.length - ObjectHeader.IDB_OBJ_HEAD_SIZE);
    
	headerCode(type, cls.idr_psize,
		   ObjectHeader.IDB_XINFO_LOCAL_OBJ, true); // was false
    }

    protected void headerCode(int type, int psize, int xinfo, boolean magic) {

	ObjectHeader hdr = new ObjectHeader();

	hdr.magic = (magic ? ObjectHeader.IDB_OBJ_HEAD_MAGIC : 0);
	hdr.type  = type;
	hdr.size  = psize;
	hdr.xinfo = xinfo;

	if (cls != null)
	    hdr.oid_mcl = cls.getOid();

	hdr.code(idr.idr);
    }

    void create() throws Exception {
    }

    void update() throws Exception {
    }

    public void realize(RecMode rcm) throws Exception {
    }

    public void realize() throws Exception {
	realize(RecMode.NoRecurs);
    }

    public void store(RecMode rcm) throws Exception {
	realize(rcm);
    }

    public void store() throws Exception {
	realize();
    }

    public void remove() throws Exception {
	Status status = RPC.rpc_OBJECT_DELETE(db, oid);

	if (!status.isSuccess())
	    throw new RemoveObjectException(status);
    }

    public void trace() throws Exception {
	trace(System.out, 0, RecMode.FullRecurs);
    }

    public void trace(java.io.PrintStream out) throws Exception {
	trace(out, 0, RecMode.FullRecurs);
    }

    public void trace(java.io.PrintStream out, int flags) throws Exception {
	trace(out, flags, RecMode.FullRecurs);
    }

    public void trace(java.io.PrintStream out, int flags, RecMode rcm)
	throws Exception {
	if (oid != null)
	    out.print(oid.getString() + " " + (cls == null ? "null" :
					       cls.getName()) + " = ");
	else
	    out.print(Root.getNullString() + " " + (cls == null ? "null" :
						    cls.getName()) + " = ");

	traceRealize(out, new Indent(), flags, rcm);
    }

    public org.eyedb.Class getClass(boolean dummy) {
	return cls;
    }

    public void setClass(org.eyedb.Class cls) {
	//if (cls != null)
	// throw something
	this.cls = cls;
    }

    public Object getMasterObject() {
	return master_object;
    }

    public void setMasterObject(Object master_object) {
	this.master_object = master_object;
    }

    // should be overriden
    public void traceRealize(java.io.PrintStream out, Indent indent,
			     int flags, RecMode rcm) throws Exception {
    }

    public void loadPerform(Database db, Oid clsoid, int lockmode,
			    AttrIdxContext idx_ctx, RecMode rcm) 
	throws Exception {

	int items_cnt = cls.items.length;

	for (int n = 0; n < items_cnt; n++)
	    cls.items[n].load(db, this, clsoid, lockmode, idx_ctx, rcm);
    }

    protected void setGRTObject(boolean grt_obj) {this.grt_obj = grt_obj;}

    static public final int MTimeTrace    = 0x01;
    static public final int CTimeTrace    = 0x02;
    static public final int CMTimeTrace   = MTimeTrace | CTimeTrace;
    static public final int PointerTrace  = 0x04;
    static public final int CompOidTrace  = 0x08;
    static public final int NativeTrace   = 0x10;
    static public final int ContentsTrace = 0x20;

    public static org.eyedb.Class idbclass;

    protected final int Tracing   = 0x1;
    protected final int Realizing = 0x2;

    // warning: master_object has been introduced the 23/03/01 but
    // has not been tested at all!
    protected Object master_object = null;

    protected void classOidCode() {
	new Coder(idr.idr, ObjectHeader.IDB_OBJ_HEAD_OID_MCL_INDEX).
	    code(cls.oid);
    }

    // only for debug
    public void tracePdata(String s) {
	System.out.println(s + ", " + this + ", pdata = " + pdata.length);
	for (int i = 0; i < pdata.length; i++)
	    System.out.println("pdata[" + i + " ] = " + pdata[i]);
    }

    void realizePerform(Oid clsoid, Oid objoid, AttrIdxContext idx_ctx,
			RecMode rcm) throws Exception {
    }

    void postRealizePerform(int offset, Oid clsoid, Oid objoid,
			    AttrIdxContext idx_ctx, RecMode rcm)
	throws Exception {
    }
}
