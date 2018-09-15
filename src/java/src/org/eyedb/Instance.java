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

public class Instance extends Object {

  Instance(Database db) {
    super(db);
  }

  Instance(Database db, Dataspace dataspace) {
    super(db, dataspace);
  }

  Instance(Object o, boolean share) {
    super(o, share);
  }

  public void traceRealize(java.io.PrintStream out, Indent indent,
			   int flags, RecMode rcm) throws Exception {

    Attribute[] items = cls.items;

    out.print("{\n");

    if ((state & Tracing) != 0)
      {
	indent.push();
	out.println(indent + "<trace cycle>");
	indent.pop();
	out.println(indent + "};");
	return;
      }

    state |= Tracing;

    indent.push();

    for (int n = 0; n < items.length; n++)
      items[n].trace(out, this, indent, flags, rcm);

    indent.pop();

    out.println(indent + "};");

    state &= ~Tracing;
  }

  /*
  public void trace(java.io.PrintStream out, int flags, RecMode rcm) {

    if (oid != null)
      out.print(oid.getString() + " " + cls.getName() + " = ");
    else
      out.print(Root.getNullString() + " " + cls.getName() + " = ");

    traceRealize(out, new Indent(), flags, rcm);
  }
  */

  void create_realize(boolean realizing) throws Exception {

    if (cls == null)
      return;

    Status status;

    if (!cls.getOid().isValid())
      {
	System.err.println("creating agregat of class `" + cls.getName() +
			   "'");
	return;
      }

    classOidCode();

    if (!realizing)
      {
	Value xoid = new Value();
	status = RPC.rpc_OID_MAKE(db, getDataspaceID(), idr.idr,
				  idr.idr.length, xoid); 
	if (!status.isSuccess())
	  throw new StoreException(status);
	oid = xoid.sgetOid();
	return;
      }

    modify = true;
  }

  void update_realize(boolean realizing) throws Exception
  {
    if (!cls.getOid().isValid())
      {
	System.err.println("updating agregat of class `" + cls.getName() +
			   "'");
	return;
      }

    if (!realizing)
      {
	Status status = RPC.rpc_OBJECT_WRITE(db, idr.idr, oid);
      	if (!status.isSuccess())
	  throw new StoreException(status);
      }
  }

  void update() throws Exception
  {
    if (!oid.isValid())
      {
	System.err.println("updating agregat of class `" + cls.getName()
			   + "'");
	return;
      }

    update_realize(false);
  }

  public void realize(RecMode rcm) throws Exception
  {
    if ((state & Realizing) != 0)
      return;

    if (master_object != null)
      {
	master_object.realize(rcm);
	return;
      }

    boolean creating;

    state |= Realizing;

    if (oid == null || !oid.isValid())
      {
	create_realize(false);
	creating = true;
      }
    else
      {
	update_realize(true);
	creating = false;
      }

    AttrIdxContext idx_ctx = new AttrIdxContext();
    realizePerform(cls.getOid(), getOid(), idx_ctx, rcm);
    //realize_items(rcm);

    Status status = null;

    if (creating)
      status = RPC.rpc_OBJECT_CREATE(db, getDataspaceID(), 
				     idr.idr, new Value(oid));
    else if (modify)
      {
	status = RPC.rpc_OBJECT_WRITE(db, idr.idr, oid);
	modify = false;
      }

    if (status != null && !status.isSuccess())
      throw new StoreException(status);

    state &= ~Realizing;
  }

  /*
  void realize_items(Oid clsoid, Oid objoid,
		     AttrIdxContext idx_ctx, RecMode rcm)
    throws Exception {
      Attribute[] items = cls.getAttributes();
      int items_cnt = items.length;
  
      for (int i = 0; i < items_cnt; i++)
	items[i].realize(db, this, clsoid, objoid, idx_ctx, rcm);
  }
  */

  void realizePerform(Oid clsoid, Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception
    {
      //realize_items(clsoid, objoid, idx_ctx, rcm);
      Attribute[] items = cls.getAttributes();
      int items_cnt = items.length;
  
      for (int i = 0; i < items_cnt; i++)
	items[i].realize(db, this, clsoid, objoid, idx_ctx, rcm);
    }

  public static Class idbclass;
}

