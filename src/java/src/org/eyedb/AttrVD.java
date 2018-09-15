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

class AttrVD extends Attribute {

  public AttrVD(Database db, Coder coder, int num) throws Exception {
    super(db, coder, num);
  }

  public AttrVD(Attribute item, int num)
  {
    super(item, num);
  }

  public int getSize(Object o) {
    return clean_size(new Coder(o.idr.idr, idr_poff).decodeInt());
  }

  protected int getPOff() {
    return 0;
  }

  protected int getIniSize(Object o) {
    return iniSize(getSize(o));
  }

  boolean check() {

    for (int i = 0; i < typmod.ndims; i++)
      if (typmod.dims[i] < 0 && i != 0)
	{
	  System.err.println("only left dimension is allowed to be " +
			     "variable for agregat item " + name +
			     " in agregat class " + class_owner.getName());

	  return false;
	}

    return true;
  }

  void update_realize(Database db, Object o, int count, 
		      int wpsize, byte[] pdata, Oid oid,
		      AttrIdxContext idx_ctx) throws Exception {

    /*
    System.out.println("update_realize: " + name + " #1 " + oid.getString() +
		       ", wpsize=" + wpsize);
		       */
    if (VARS_COND() && wpsize <= VARS_TSZ && wpsize > 0)
      System.arraycopy(pdata, 0, o.getIDR(), idr_poff + VARS_OFFSET,
		       wpsize);

    if (!oid.isValid() && wpsize == 0)
      return;

    Oid mcl_oid = o.getClass(true).getOid();
    Oid agr_oid = o.getOid();
    // added for 2.4.0 : this is kludge waiting for a correct value
    // for actual_mcl_oid -> refers to attr.cc
    Oid actual_mcl_oid = mcl_oid;

    if (isSizeChanged(o))
      {
	byte idx_data[] = idx_ctx.code();
	if (oid.isValid())
	  RPC.rpc_VDDATA_DELETE(db, actual_mcl_oid, mcl_oid, num,
				   agr_oid, oid, idx_data);

	if (wpsize != 0 && !(VARS_COND() && wpsize <= VARS_TSZ))
	  {
	    Value xoid = new Value();
	    Status s = RPC.rpc_VDDATA_CREATE(db, dspid,
					     actual_mcl_oid, 
					     mcl_oid, num, count,
					     wpsize, pdata,
					     agr_oid, xoid, idx_data);
	    setVarDimOid(o, xoid.sgetOid());
	  }
	else
	  setVarDimOid(o, new Oid());

	setSizeChanged(o, false);
	return;
      }

    if (o.isModify() && !(VARS_COND() && wpsize <= VARS_TSZ))
      {
	byte idx_data[] = idx_ctx.code();
	RPC.rpc_VDDATA_WRITE(db, actual_mcl_oid, mcl_oid, num, count,
				wpsize, pdata, agr_oid, oid, idx_data);
      }
  }

  void update(Database db, Object o, AttrIdxContext idx_ctx) throws Exception {
    byte[] pdata = o.pdata[num].data;
    Oid oid = getVarDimOid(o);
    int size = getSize(o);
    int wpsize = size * idr_item_psize * typmod.pdims;

    if (basic_enum)
      wpsize += iniSize(size);

    update_realize(db, o, size, wpsize, pdata, oid, idx_ctx);
  }

  Oid getVarDimOid(Object o) {
    return new Coder(o.idr.idr, idr_poff + Coder.INT32_SIZE).decodeOid();
  }

  public void setSize(Object o, int nsize) throws Exception {
    int osize = getSize(o);

    if (osize == nsize)
      return;

    setSizeRealize(o.getDatabase(), o, nsize, true);
    o.touch();

    setSizeChanged(o, true);
  }

  void setSizeChanged(Object o, boolean changed) throws Exception {
    setSizeRealize(o, set_size_changed(getSize(o), changed));
  }

  boolean isSizeChanged(Object o) {
    return is_size_changed(new Coder(o.idr.idr, idr_poff).decodeInt());
  }

  void setVarDimOid(Object o, Oid oid) {
    new Coder(o.idr.idr, idr_poff + Coder.INT32_SIZE).code(oid);
  }

  void setSizeRealize(Object o, int size) throws Exception {
    new Coder(o.idr.idr, idr_poff).code(size);
  }

  void setSizeRealize(Database db, Object o, int nsize,
		      boolean make) throws Exception {
  }
}
