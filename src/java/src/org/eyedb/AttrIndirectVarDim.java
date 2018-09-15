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

class AttrIndirectVarDim extends AttrVD {

  public AttrIndirectVarDim(Database db, Coder coder, int num) throws Exception {
    super(db, coder, num);
    code = AttrIndirectVarDim_Code;
  }

  public AttrIndirectVarDim(Attribute item, int num) {
    super(item, num);
    code = AttrIndirectVarDim_Code;
  }

  public boolean isIndirect() {return true;}

  void newObjRealize(Database db, Object o) throws Exception {
    int size = getSize(o);

    //System.out.println("AttrIndirectVardim::newObjRealize(" + name + ") size = " + size + ", o = " + o);

    if (size > 0)
      setSizeRealize(db, o, size, false);
    else
      {
	o.pdata[num]   = new Data();
	o.oids[num]    = new OidArray();
	o.objects[num] = new ObjectArray();
      }
  }

  /*
  void makeVolatData(Object o, byte[] data_oid, int count) {
    Oid[] oids  = new Oid[count];
    Coder coder = new Coder(data_oid);

    for (int i = 0; i < count; i++)
      oids[i] = coder.decodeOid();
    
    o.oids[num]    = new OidArray(oids);
    o.objects[num] = new ObjectArray(new Object[count]);
    o.pdata[num]   = new Data(data_oid);
  }
  */

  void makeVolatData(Object o, int count) {
    byte[] data_oid = o.pdata[num].data;
    Coder coder = new Coder(data_oid);
    Oid[] oids = o.oids[num].oids;

    for (int i = 0; i < count; i++)
      oids[i] = coder.decodeOid();
  }

  void setSizeRealize(Database db, Object o, int nsize,
		      boolean make) throws Exception {

    int rsize;
    byte[] opdata, npdata;

    int wpsize;
    int osize;
      
    //System.out.println("setSizeRealize -> o = " + o + ", nsize = " +
    //nsize +  ", make = " + make + ", num = " + num);

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

    setSizeRealize(o, nsize);
  
    if (nsize == 0)
      {
	o.pdata[num]   = new Data();
	o.oids[num]    = new OidArray();
	o.objects[num] = new ObjectArray();
	return;
      }

    wpsize = idr_item_psize * nsize * typmod.pdims;
    npdata = new byte[wpsize];

    if (make)
      {
	int wrsize = idr_item_psize * rsize * typmod.pdims;
	
	if (opdata != null)
	  System.arraycopy(opdata, 0, npdata, 0, wrsize);
	Coder.memzero(npdata, wrsize, wpsize - wrsize);
      }
    else
      Coder.memzero(npdata, 0, wpsize);
    
    o.pdata[num] = new Data(npdata);
  
    Object[] nobjects = new Object[nsize];
    Oid[] noids       = new Oid[nsize];
    
    if (osize != 0)
      {
	Object[] oobjects = o.objects[num].objects;
	Oid[]       ooids = o.oids[num].oids;

	for (int j = 0; j < osize; j++)
	  {
	    nobjects[j] = oobjects[j];
	    noids[j] = ooids[j];
	  }
      }

    o.objects[num] = new ObjectArray(nobjects);
    o.oids[num]    = new OidArray(noids);
  }

  void load(Database db, Object o, Oid clsoid,
	    int lockmode, AttrIdxContext idx_ctx, RecMode rcm) throws Exception {
    //    System.out.println("AttrIndirectVardim::load(" + name + ")");

    int size = getSize(o);

    int wsize = size * Coder.OID_SIZE * typmod.pdims;

    Oid data_oid = getVarDimOid(o);

    if (wsize != 0)
      {
	RPC.rpc_DATA_READ(db, 0, wsize, null, o.pdata[num].data, data_oid);

	makeVolatData(o, size);
      }

    setSizeChanged(o, false);

    int dd = size * typmod.pdims;

    Oid[] oids       = o.oids[num].oids;
    Object[] objects = o.objects[num].objects;

    for (int j = 0; j < dd; j++)
	if (rcm.isAgregRecurs(this, j))
	    if (oids[j].isValid())
		objects[j] = db.loadObjectRealize(oids[j], lockmode, rcm);
  }

  public void realize(Database db, Object o, Oid clsoid,
		      Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception {

    int size = getSize(o);
    int dd = typmod.pdims * size;

    Object[] objects = o.objects[num].objects;

    for (int j = 0; j < dd; j++)
      {
	Object xo = objects[j];

	if (xo != null && rcm.isAgregRecurs(this, j, o))
	  {
	    xo.setDatabase(db);

	    xo.realize(rcm);

	    setOid(o, xo.getOid(), j);
	  }
      }

    update(db, o, idx_ctx);
  }

  void trace(java.io.PrintStream out, Object o, Indent indent,
	     int flags, RecMode rcm) throws Exception {

    String prefix = getPrefix(o);

    TypeModifier typmod1 = new TypeModifier(typmod);

    int size = getSize(o);

    typmod1.pdims *= size;

    Oid[] oids       = o.oids[num].oids;
    Object[] objects = o.objects[num].objects;

    for (int j = 0; j < typmod1.pdims; j++)
      {
	if (objects[j] != null && rcm.isAgregRecurs(this, j, o))
	  {
	    out.print(indent + "*" + prefix + name + "[" + j + "] " + oids[j] +
		      " " + objects[j].getClass(true).getName() + " = ");
	    objects[j].traceRealize(out, indent, flags, rcm);
	  }
	else
	  out.println(indent + "*" + prefix + name + "[" + j + "] " + oids[j] + ";");
      }
  }

  public Oid[] getOid(Object o, int nb, int from) {

    return getIndirectOid(o, nb, from);
  }

  public void setOid(Object o, Oid[] xoid, int nb, int from) {

    setIndirectOid(o, xoid, nb, from);
  }

  public Value[] getValue(Object o, int nb, int from, boolean autoload) throws Exception {
    return getIndirectValue(o, nb, from, autoload);
  }

  public void setValue(Object o, Value[] value, int nb, int from)
       throws Exception {

    setIndirectValue(o, value, nb, from);
  }

  void compile_perst(Class mcl, Value x_offset, Value x_size,
		     Value x_inisize) throws Exception {
    if (!check())
      return;

    idr_poff = x_offset.sgetInt();
    idr_inisize = 0;

    idr_item_psize = Coder.OID_SIZE;
    idr_psize = Coder.INT32_SIZE + Coder.OID_SIZE;

    x_inisize.set(idr_inisize);
      
    compile_update(mcl, idr_psize, x_offset, x_size);
  }

  void compile_volat(Class mcl, Value x_offset, Value x_size) throws Exception {

    idr_voff = x_offset.sgetInt();

    idr_item_vsize = Coder.OBJECT_SIZE;
    idr_vsize = Coder.INT32_SIZE + Coder.INT32_SIZE;

    compile_update(mcl, idr_vsize, x_offset, x_size);
  }
}
