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

class AttrDirect extends Attribute {

  public AttrDirect(Database db, Coder coder, int num) throws Exception {
    super(db, coder, num);
    code = AttrDirect_Code;
  }

  public AttrDirect(Attribute item, int num) {
    super(item, num);
    code = AttrDirect_Code;
  }

  void newObjRealize(Database db, Object o) throws Exception {
    if (basic_enum)
      return;

    byte[] idr = o.idr.idr;
    
    Object[] objects = new Object[typmod.pdims];
    o.objects[num] = new ObjectArray(objects);

    for (int j = 0, offset = idr_poff; j < typmod.pdims;
	 j++, offset += idr_item_psize)
      {
	objects[j] = cls.newObj(db, new Coder(idr, offset));
	objects[j].setMasterObject(o);
      }
  }

  void load(Database db, Object o, Oid clsoid,
	    int lockmode, AttrIdxContext idx_ctx, RecMode rcm) throws Exception {

    if (basic_enum)
      return;

    Object[] objects = o.objects[num].objects;

    byte[] idr = o.idr.idr;

    for (int j = 0, offset = idr_poff + idr_inisize; j < typmod.pdims;
	 j++, offset += idr_item_psize)
      {
	Object oo = objects[j];
	/*
	System.out.println("name=" + name +
			   ", oo=" + oo);
	System.out.println("idr=" + idr +
			   ", oo.idr=" + oo.idr +
			   ", oo.idr.idr=" + oo.idr.idr);
	*/
	System.arraycopy(idr, offset, oo.idr.idr,
			 ObjectHeader.IDB_OBJ_HEAD_SIZE, idr_item_psize);

	//oo.loadItems(db, rcm);
	oo.loadPerform(db, clsoid, lockmode, idx_ctx, rcm);

	System.arraycopy(oo.idr.idr, ObjectHeader.IDB_OBJ_HEAD_SIZE,
			 idr, offset, idr_item_psize);
      }

  }

  void trace(java.io.PrintStream out, Object o, Indent indent,
	     int flags, RecMode rcm) throws Exception {

    String prefix = getPrefix(o);

    if (basic_enum)
      {
	out.print(indent + prefix + name + " = ");

	if (isNull(o.idr.idr, idr_poff, typmod))
	  Root.printNull(out);
	else
	  cls.traceData
	    (out, new Coder(o.idr.idr, idr_poff + idr_inisize),
	     typmod);
	out.println(";");
	return;
      }

    Object[] objects = o.objects[num].objects;

    for (int j = 0; j < typmod.pdims; j++)
      {
	Object oo = objects[j];
	
	out.print(indent + prefix + name);
	if (typmod.ndims != 0)
	  out.print("[" + j + "]");
	out.print(" = ");
	
	oo.traceRealize(out, indent, flags, rcm);
      }
  }

  public Value[] getValue(Object o, int nb, int from, boolean autoload) {

    return getDirectValue(o.idr.idr, o, nb, from, autoload);
  }

  public void setValue(Object o, Value[] value,
				int nb, int from) 
       throws Exception {

     setDirectValue(o.idr.idr, o, value, nb, from);
  }

  public String getStringValue(Object o)
    throws Exception {
    return getDirectStringValue(o.idr.idr, o);
  }

  public byte[] getRawValue(Object o)
    throws Exception {
    return getDirectRawValue(o.idr.idr, o);
  }

  public void realize(Database db, Object o, Oid clsoid,
		      Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception {

    if (basic_enum)
      return;

    idx_ctx.push(db, clsoid, this);

    Object[] objects = o.objects[num].objects;

    for (int j = 0, offset = idr_poff; j < typmod.pdims; j++,
	   offset += idr_item_psize)
      {
	Object xo = objects[j];

	if (xo != null)
	  {
	    xo.setDatabase(db);

	    // MISSING : setCollImp(db, xo, idx_ctx)

	    xo.realizePerform(clsoid, objoid, idx_ctx, rcm);

	    if (!Coder.memcmp(xo.getIDR(),
				 ObjectHeader.IDB_OBJ_HEAD_SIZE,
				 o.getIDR(),
				 offset,
				 idr_item_psize))
	      {
		System.arraycopy(xo.getIDR(),
				 ObjectHeader.IDB_OBJ_HEAD_SIZE,
				 o.getIDR(),
				 offset,
				 idr_item_psize);
		if (!(xo instanceof Collection))
		  o.touch();

		xo.postRealizePerform(idr_poff, clsoid, objoid,
				      idx_ctx, rcm);
	      }
	  }
      }

    idx_ctx.pop();
  }

  public void setStringValue(Object o, String s)
    throws Exception {
    setDirectStringValue(o.idr.idr, o, s);
  }

  public void setRawValue(Object o, byte data[])
    throws Exception {
    setDirectRawValue(o.idr.idr, o, data);
  }

  void compile_perst(Class mcl, Value x_offset, Value x_size,
		     Value x_inisize) throws Exception {

    idr_poff = x_offset.sgetInt();

    if (basic_enum)
      idr_inisize = iniSize(typmod.pdims);
    else
      idr_inisize = 0;

    if (cls.getObjectSize() == 0) {
	System.err.println("erreur compile in " + class_owner.getName() +
			   "::" + name + ", cls = " + cls + ", psize = " +
			   cls.getObjectPSize() + " " + cls.getName());
	return;
      }
    
    idr_item_psize = cls.getObjectPSize() -
      ObjectHeader.IDB_OBJ_HEAD_SIZE;
    idr_psize = idr_item_psize * typmod.pdims + idr_inisize;
    x_inisize.set(idr_inisize);
    
    compile_update(mcl, idr_psize, x_offset, x_size);
  }

  void compile_volat(Class mcl, Value x_offset, Value x_size) throws Exception {

    idr_voff = x_offset.sgetInt();

    if (basic_enum)
      {
	idr_item_vsize = 0;
	idr_vsize = 0;
      }
    else
      {
	idr_item_vsize = Coder.OBJECT_SIZE;
	idr_vsize = idr_item_vsize * typmod.pdims;
      }

    compile_update(mcl, idr_vsize, x_offset, x_size);
  }
}
