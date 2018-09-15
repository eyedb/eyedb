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

class AttrVarDim extends AttrVD {

  public AttrVarDim(Database db, Coder coder, int num)  throws Exception {
    super(db, coder, num);
    code = AttrVarDim_Code;
  }

  public AttrVarDim(Attribute item, int num) {
    super(item, num);
    code = AttrVarDim_Code;
  }

  void setSizeRealize(Database db, Object o, int nsize,
		      boolean make) throws Exception {
    int rsize;
    byte[] opdata, npdata;

    int wpsize;
    int osize;
    int oinisize, rinisize, ninisize;
      
    npdata = null;

    if (make)
      {
	osize = getSize(o);

	opdata = o.pdata[num].data;

	rsize = ((osize < nsize) ? osize : nsize);
      }
    else
      {
	osize = 0;
	opdata = null;

	rsize = 0;
      }

    if (basic_enum)
      {
	oinisize = iniSize(osize);
	ninisize = iniSize(nsize);
	rinisize = ((oinisize < ninisize) ? oinisize : ninisize);
      }
    else
      rinisize = oinisize = ninisize = 0;
    
    setSizeRealize(o, nsize);
  
    if (nsize == 0)
      {
	o.pdata[num] = new Data();
	o.objects[num] = new ObjectArray();
      }
    else
      {
	wpsize = idr_item_psize * nsize * typmod.pdims;
	npdata = new byte[wpsize + ninisize];

	if (make)
	  {
	    int wrsize = idr_item_psize * rsize * typmod.pdims;
	    
	    if (opdata != null)
	      System.arraycopy(opdata, 0, npdata, 0, rinisize);
	    Coder.memzero(npdata, rinisize, ninisize - rinisize);

	    if (opdata != null)
	      System.arraycopy(opdata, oinisize,
			       npdata, ninisize, wrsize);
	    Coder.memzero(npdata, ninisize + wrsize, wpsize - wrsize);
	  }
	else
	  Coder.memzero(npdata, 0, wpsize + ninisize);
  
	o.pdata[num] = new Data(npdata);
      }
  
    if (basic_enum)
      return;

    Object[] nobjects = new Object[nsize];
    
    if (osize != 0)
      {
	Object[] oobjects = o.objects[num].objects;
	for (int j = 0; j < osize; j++)
	  if (oobjects[j] == null)
	    nobjects[j] = cls.newObj
	      (db, new Coder(npdata, (j * idr_item_psize)));
  	  else
	    nobjects[j] = oobjects[j];
      }

    for (int j = osize; j < nsize; j++)
      {
	nobjects[j] = cls.newObj
	  (db, new Coder(npdata, (j * idr_item_psize)));
	nobjects[j].setMasterObject(o);
      }

    o.objects[num] = new ObjectArray(nobjects);
  }

  public void realize(Database db, Object o, Oid clsoid,
		      Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception {

    idx_ctx.push(db, clsoid, this);

    if (basic_enum)
      {
	update(db, o, idx_ctx);
	idx_ctx.pop();
	return;
      }

    int size = getSize(o);

    byte[] pdata = o.pdata[num].data;

    int dd;
    dd = typmod.pdims * size;

    Object[] objects = o.objects[num].objects;

    for (int j = 0, offset = 0; j < dd; j++, offset += idr_item_psize)
      {
	Object xo = objects[j];

	if (xo != null)
	  {
	    xo.setDatabase(db);

	    //xo.realize_items(rcm);
	    xo.realizePerform(clsoid, objoid, idx_ctx, rcm);

	    if (!Coder.memcmp(xo.getIDR(),
				 ObjectHeader.IDB_OBJ_HEAD_SIZE,
				 pdata,
				 offset,
				 idr_item_psize))
	    {
	      System.arraycopy(xo.getIDR(),
			       ObjectHeader.IDB_OBJ_HEAD_SIZE,
			       pdata,
			       offset,
			       idr_item_psize);

	      if (!(xo instanceof Collection))
		o.touch();

	      xo.postRealizePerform(idr_poff, clsoid, objoid,
				    idx_ctx, rcm);
	    }
	  }
      }

    update(db, o, idx_ctx);
    idx_ctx.pop();
  }

  void newObjRealize(Database db, Object o) throws Exception {

    int size = getSize(o);

    if (size > 0)
      setSizeRealize(db, o, size, false);
    else if (basic_enum &&
	     ((idx_mode & IndexCompMode) != 0 ||
	      ((idx_mode & IndexItemMode) != 0)))
      {
	size = 1;
	// should make a test according to VARS_COND() ??
	setSizeRealize(db, o, size, false);
      }
    else
      {
	o.pdata[num]   = new Data();
	o.objects[num] = new ObjectArray();
      }
  }

  void load(Database db, Object o, Oid clsoid,
	    int lockmode, AttrIdxContext idx_ctx, RecMode rcm) throws Exception {

    int size = getSize(o);
    int wpsize = size * idr_item_psize * typmod.pdims;

    setSizeRealize(db, o, size, false);

    byte[] pdata = o.pdata[num].data;

    if (basic_enum)
      wpsize += iniSize(size);

    Oid oid = getVarDimOid(o);

    if (!oid.isValid())
      {
	if (VARS_COND() && wpsize > 0)
	  {
	    System.arraycopy(o.getIDR(), idr_poff + VARS_OFFSET, pdata, 0,
			     wpsize);
	  }
	return;
      }

    if (size != 0)
      RPC.rpc_DATA_READ(db, 0, wpsize, null, pdata, oid);

    if (basic_enum || size == 0)
      return;

    int dd = typmod.pdims * size;

    Object[] objects = o.objects[num].objects;

    for (int j = 0; j < dd; j++)
      {
	System.arraycopy(pdata, (j * idr_item_psize),
			 objects[j].idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE,
			 idr_item_psize);

	objects[j].loadPerform(db, clsoid, lockmode, idx_ctx, rcm);

	System.arraycopy(objects[j].idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE,
			 pdata, (j * idr_item_psize),
			 idr_item_psize);
      }
  }

  void trace(java.io.PrintStream out, Object o, Indent indent,
	     int flags, RecMode rcm) throws Exception {

    String prefix = getPrefix(o);

    int size = getSize(o);
    byte[] pdata;

    if (size == 0)
      pdata = null;
    else
      pdata = o.pdata[num].data;

    TypeModifier typmod1 = new TypeModifier(typmod);
    typmod1.pdims *= size;

    if (basic_enum)
      {
	int inisize = iniSize(size);

	out.print(indent + prefix + name + " = ");

	if (isNull(pdata, 0, typmod1))
	  Root.printNull(out);
	else
	  cls.traceData(out, new Coder(pdata, inisize), typmod1);

	out.println(";");
      }
    else
      {
	Object[] objects = o.objects[num].objects;

	for (int j = 0; j < typmod1.pdims; j++)
	  {
	    if (typmod1.ndims == 0)
	      out.print(indent + prefix + name + " " + cls.getName() + " = ");
	    else
	      out.print(indent + prefix + name + "[" + j + "] " +
			cls.getName() + " = ");;
	    objects[j].traceRealize(out, indent, flags, rcm);
	  }
      }
  }

  public Value[] getValue(Object o, int nb, int from, boolean autoload) {

    return getDirectValue(o.pdata[num].data, o, nb, from, autoload);
  }

  public void setValue(Object o, Value[] value,
		int nb, int from) 
       throws Exception {

     setDirectValue(o.pdata[num].data, o, value, nb, from);
  }

  public String getStringValue(Object o) 
    throws Exception {
    return getDirectStringValue(o.pdata[num].data, o);
  }

  public void setStringValue(Object o, String s)
    throws Exception {
      setDirectStringValue(o.pdata[num].data, o, s);
  }

  public byte[] getRawValue(Object o)
    throws Exception {
      return getDirectRawValue(o.pdata[num].data, o);
  }

  public void setRawValue(Object o, byte data[])
    throws Exception {
    setDirectRawValue(o.pdata[num].data, o, data);
  }

  void compile_perst(Class mcl, Value x_offset, Value x_size,
		     Value x_inisize) throws Exception {

    if (!check())
      return;

    idr_poff = x_offset.sgetInt();
    idr_inisize = 0;

    if (cls.getObjectSize() == 0)
      {
	System.err.println("error compilation in " + class_owner.getName() +
			   "::" + name + ", cls = " + cls + ", psize= " +
			   cls.getObjectPSize());
	return;
      }
    
    idr_item_psize = cls.getObjectPSize() -
      ObjectHeader.IDB_OBJ_HEAD_SIZE;

    if (VARS_COND())
      idr_psize = VARS_OFFSET + VARS_TSZ;
    else
      idr_psize = Coder.INT32_SIZE + Coder.OID_SIZE;

    x_inisize.set(idr_inisize);
    
    compile_update(mcl, idr_psize, x_offset, x_size);
  }

  void compile_volat(Class mcl, Value x_offset, Value x_size) throws Exception {

    idr_voff = x_offset.sgetInt();

    if (basic_enum)
      {
	idr_item_vsize = 0;
	idr_vsize = Coder.INT32_SIZE;
      }
    else
      {
	idr_item_vsize = Coder.OBJECT_SIZE;
	idr_vsize = Coder.INT32_SIZE + Coder.INT32_SIZE;
      }
    compile_update(mcl, idr_vsize, x_offset, x_size);
  }
}
