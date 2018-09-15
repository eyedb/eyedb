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

class AttrIndirect extends Attribute {

  public AttrIndirect(Database db, Coder coder, int num)  throws Exception {
    super(db, coder, num);
    code = AttrIndirect_Code;
  }

  public AttrIndirect(Attribute item, int num) {
    super(item, num);
    code = AttrIndirect_Code;
  }

  public void realize(Database db, Object o, Oid clsoid,
		      Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception {

    Object[] objects = o.objects[num].objects;
    Oid[] oids = o.oids[num].oids;

    Coder coder = new Coder(o.idr.idr, idr_poff);

    for (int j = 0; j < typmod.pdims; j++)
      {
	Object xo = objects[j];

	if (xo != null && rcm.isAgregRecurs(this, j, o))
	  {
	    xo.setDatabase(db);

	    xo.realize(rcm);

	    setOid(o, xo.getOid(), j);
	  }

	coder.code(oids[j]);
      }
  }

  void newObjRealize(Database db, Object o) {

    Object[] objects = new Object[typmod.pdims];
    o.objects[num] = new ObjectArray(objects);

    Oid[] oids = new Oid[typmod.pdims];
    o.oids[num] = new OidArray(oids);

    for (int j = 0; j < typmod.pdims; j++)
      {
	objects[j] = null;
	oids[j] = null;
      }
  }

  private Oid getXOid(Object o, int from) {
    return new Coder(o.idr.idr, idr_poff + (from * Coder.OID_SIZE)).
      decodeOid();
  }

  void load(Database db, Object o, Oid clsoid,
	    int lockmode, AttrIdxContext idx_ctx, RecMode rcm) throws Exception {
    //    System.out.println("AttrIndirect::load(" + name + ")");

    Object[] objects = o.objects[num].objects;
    Oid[] oids = o.oids[num].oids;

    for (int j = 0; j < typmod.pdims; j++) {
	oids[j] = getXOid(o, j);
	Oid oid = oids[j];
	if (rcm.isAgregRecurs(this, j, o))
	    {
		if (oid.isValid())
		    objects[j] = db.loadObjectRealize(oid, lockmode, rcm);
	    }
    }
  }

  void trace(java.io.PrintStream out, Object o, Indent indent,
	     int flags, RecMode rcm) throws Exception {

    String prefix = getPrefix(o);

    for (int j = 0; j < typmod.pdims; j++)
      {
	Oid oid = getXOid(o, j);

	Object oo = o.objects[num].objects[j];
	
	if (oo != null && rcm.isAgregRecurs(this, j, o))
	  {
	    if (typmod.ndims == 0)
	      out.print(indent + "*" + prefix + name + " " + oid + " " +
			cls.getName() +
			" = ");
	    else
	      out.print(indent + "*" + prefix + name + "[" + j + "] " +
			oid + " " + cls.getName() +
			" = ");
	    oo.traceRealize(out, indent, flags, rcm);
	  }
	else
	  {
	    if (typmod.ndims == 0)
	      out.println(indent + "*" + prefix + name + " " + oid + ";");
	    else
	      out.println(indent + "*" + prefix + name + "[" + j + "]" +
			  oid + ";");
	  }
      }
  }

  public Oid[] getOid(Object o, int nb, int from) {

    return getIndirectOid(o, nb, from);
  }

  public void setOid(Object o, Oid[] xoid, int nb, int from) {

    setIndirectOid(o, xoid, nb, from);
  }

  public Value[] getValue(Object o, int nb, int from, boolean autoload)
      throws Exception {

    return getIndirectValue(o, nb, from, autoload);
  }

  public void setValue(Object o, Value[] value, int nb, int from)
       throws Exception {

    setIndirectValue(o, value, nb, from);
  }

  void compile_perst(Class mcl, Value x_offset, Value x_size,
		     Value x_inisize) throws Exception {

    idr_inisize = 0;
    idr_poff = x_offset.sgetInt();
    
    idr_item_psize = Coder.OID_SIZE;
    idr_psize = idr_item_psize * typmod.pdims;
    x_inisize.set(idr_inisize);
    
    compile_update(mcl, idr_psize, x_offset, x_size);
  }

  void compile_volat(Class mcl, Value x_offset, Value x_size) throws Exception {
    idr_voff = x_offset.sgetInt();
    
    idr_item_vsize = Coder.OBJECT_SIZE;
    idr_vsize = idr_item_vsize * typmod.pdims;
    
    compile_update(mcl, idr_vsize, x_offset, x_size);
  }
}

