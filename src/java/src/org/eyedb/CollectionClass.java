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

public class CollectionClass extends Class {

  static Object make(Database db, Oid oid, byte[] idr,
			Oid class_oid, int lockmode, RecMode rcm)
			throws Exception
  {
    //    System.out.println("CollectionClass::make(" + oid + ")");

    Coder coder = new Coder(idr);

    coder.setOffset(ObjectHeader.IDB_CLASS_IMPL_TYPE);

    // added 17/05/05
    IndexImpl idximpl = IndexImpl.decode(db, coder);

    coder.setOffset(ObjectHeader.IDB_CLASS_EXTENT);
    Oid extent_oid = coder.decodeOid();

    coder.setOffset(ObjectHeader.IDB_CLASS_MTYPE);
    int mt = coder.decodeInt();

    coder.setOffset(ObjectHeader.IDB_CLASS_DSPID);
    short dspid = coder.decodeShort();

    coder.setOffset(ObjectHeader.IDB_CLASS_HEAD_SIZE);
    String name = ClassHints.class_name_decode(db, coder);
    
    Oid mcloid = coder.decodeOid();
    boolean isref = coder.decodeBoolean();
    short dim     = coder.decodeShort();

    CollectionClass mcoll = null;

    ObjectHeader hdr = null;

    hdr = ObjectHeader.decode(idr);

    Class xmcoll;
    if ((xmcoll = db.getSchema().getClass(name)) != null) {
	// added 16/05/05
	xmcoll.class_oid = class_oid;
	xmcoll.extent_oid = extent_oid;
	return xmcoll;
    }

    //System.out.println("mcloid " + mcloid + ", isref " + isref +
    //", dim " + dim + ", hdr.type " + hdr.type);

    if (hdr.type == ObjectHeader._CollSetClass_Type) {
	if (dim > 1)
	    mcoll = new CollSetClass(db, name, mcloid, dim);
	else
	    mcoll = new CollSetClass(db, name, mcloid, isref );
    }
    else if (hdr.type == ObjectHeader._CollBagClass_Type) {
	if (dim > 1)
	    mcoll = new CollBagClass(db, name, mcloid, dim);
	else
	    mcoll = new CollBagClass(db, name, mcloid, isref );
    }
    else if (hdr.type == ObjectHeader._CollArrayClass_Type) {
	if (dim > 1)
	    mcoll = new CollArrayClass(db, name, mcloid, dim);
	else
	    mcoll = new CollArrayClass(db, name, mcloid, isref );
    }
    
    mcoll.setIndexImpl(idximpl);

    mcoll.class_oid = class_oid;
    mcoll.setInstanceDspid(dspid);

    // added 16/05/05
    mcoll.extent_oid = extent_oid;

    if (db.opened_state)
      mcoll.complete();

    return mcoll;
  }
  
  public boolean compare_perform(Class cls1) {
      if (!(cls1 instanceof CollectionClass))
	  return false;
      
      state |= Realizing;
      boolean r = coll_cls.compare_perform(((CollectionClass)cls1).coll_cls);
      state &= ~Realizing;
      return r;
  }

  void create() throws Exception
  {
    if (oid.isValid())
      return;

    Coder coder = new Coder();

    coder.setOffset(ObjectHeader.IDB_CLASS_IMPL_TYPE);
    IndexImpl.code(idximpl, coder);
    coder.setOffset(ObjectHeader.IDB_CLASS_MTYPE);
    coder.code(mtype);

    coder.setOffset(ObjectHeader.IDB_CLASS_HEAD_SIZE);
    codeName(coder);
  
    if (!coll_mcloid.isValid()) {
	coll_cls = db.getSchema().
	    getClass(coll_cls.getName());
	if (coll_cls == null) {
	    System.err.println("COLL_MCL is NULL");
	    return;
	}
	coll_mcloid = coll_cls.getOid();
    }

    int mcl_oid_offset = coder.getOffset();
    coder.code(coll_mcloid);

    coder.code((byte)(isref ? 1 : 0));
    coder.code((short)dim);

    idr = new IDR(null);
    idr.idr = coder.getData();

    headerCode(type, coder.getOffset(), 0, true);

    Value value = new Value();
    RPC.rpc_OBJECT_CREATE(db, getDataspaceID(), idr.idr, value);
    oid = value.getOid();
  }

  private void init(Database db) {
    this.dim = 1;
    this.db = db;
    item_size = 0;
    isref = false;

    idr_psize = ObjectHeader.IDB_OBJ_HEAD_SIZE + Coder.OID_SIZE;
    idr_vsize = Coder.OBJECT_SIZE;
    idr_inisize = 0;
    idr_objsz = idr_vsize + idr_psize;
  }

  public CollectionClass(Database db, String name, Oid coll_mcloid,
			    boolean isref) {
    super(db, name);
    init(db);
    this.coll_mcloid = coll_mcloid;
    this.isref = isref;
    //    parent = db.getSchema().getClass("class");
  }
  
  public CollectionClass(Database db, String name, Oid coll_mcloid,
			    int dim) {
    super(db, name);
    init(db);
    this.coll_mcloid = coll_mcloid;
    this.dim = (short)dim;
    //    parent = db.getSchema().getClass("class");
  }

  public CollectionClass(String typename, Class coll_cls,
			    boolean isref) {
    super(null, "");
    init(null);
    this.coll_cls = coll_cls;
    this.coll_mcloid = coll_cls.getOid();
    this.isref = isref;
    this.name = makeName(typename);
  }

  private String makeName(String typename) {
    return makeName(typename, coll_cls, isref, dim);
  }

  static String makeName(String typename, Class coll_cls,
			 boolean isref, int dim) {
    if (dim <= 1)
      return typename + "<" + coll_cls.getName() +
	(isref ? "*" : "") + ">";

    return typename + "<" + coll_cls.getName() + (isref ? "*" : "") +
      "[" + dim + "]>";
  }

    void setIndexImpl(IndexImpl idximpl) {
	this.idximpl = idximpl;
    }

  void complete() throws Exception {
    super.complete();
    if (coll_cls == null && coll_mcloid.isValid())
      coll_cls = db.getSchema().getClass(coll_mcloid);
    else if (coll_cls == null)
      coll_cls = db.getSchema().getClass("object");
  }

  public Class getCollClass() {return coll_cls;}
  public Oid getCollClassOid() {return coll_mcloid;}
  public boolean isRef() {return isref;}
  public short getDimension() {return dim;}
  public int getItemSize() {return item_size;}
  public String getTypename() {return "";}

  Class coll_cls;
  Oid coll_mcloid;
  boolean isref;
  short dim;
  int item_size;

  public static Class idbclass;

    void completeClassAndParent(Database db) {
	System.err.println("ERROR: should not be called !");
    }
}

