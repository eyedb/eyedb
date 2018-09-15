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

public class EnumClass extends Class {

    static Object make(Database db, Oid oid, byte[] idr,
			  Oid class_oid, int lockmode, RecMode rcm)
	throws Exception
    {
	Coder coder = new Coder(idr);

	ClassHints hints = ClassHints.decode(db, coder);

	EnumClass menum = new EnumClass(db, hints.name);

	int count = coder.decodeInt();

	menum.items = new EnumItem[count];

	for (int n = 0; n < count; n++) {
	    String s = coder.decodeString();
	    int val = coder.decodeInt();

	    menum.items[n] = new EnumItem(s, val, n);
	}

	menum.set(db, hints, class_oid);
	menum.setInstanceDspid(hints.dspid);

	return menum;
    }

    public boolean compare_perform(Class clsx) {
	if (!(clsx instanceof EnumClass))
	    return false;

	EnumClass enx = (EnumClass)clsx;

	if (items_cnt != enx.items_cnt)
	    return false;

	for (int i = 0; i < items_cnt; i++)
	    if (!items[i].compare(enx.items[i]))
		return false;

	return true;
    }

    private void init() {
	idr_objsz = ObjectHeader.IDB_OBJ_HEAD_SIZE + 
	    /*Coder.CHAR_SIZE + */ Coder.INT32_SIZE;

	idr_psize = idr_objsz;
	idr_vsize = 0;
	type = ObjectHeader._EnumClass_Type;
    }

    public EnumClass(Database db, String name) {
	super(db, name);
	init();
    }

    public EnumClass(String name) {
	super(null, name);
	init();
    }

    public EnumItem[] getEnumItems() {return items;}

    public int getEnumItemCount() {return items.length;}

    public EnumItem getEnumItem(int n) {return items[n];}

    public EnumItem getEnumItemFromName(String name) {
	int items_cnt = items.length;

	for (int i = 0; i < items_cnt; i++)
	    if (items[i].getName().equals(name))
		return items[i];

	return null;
    }

    public EnumItem getEnumItemFromVal(int val) {
	int items_cnt = items.length;

	for (int i = 0; i < items_cnt; i++)
	    if (items[i].getValue() == val)
		return items[i];

	return null;
    }

    Value getValueValue(Coder coder) {
	//byte b = coder.decodeByte();
	return new Value(coder.decodeInt());
    }

    void setValueValue(Coder coder, Value value)
	throws Exception {
	coder.setOffset(coder.getOffset() + 1);
	coder.code(value.getInt());
    }

    public void setEnumItems(EnumItem[] items) {
	this.items = items;
    }

    public void traceRealize(java.io.PrintStream out, Indent indent,
			     int flags, RecMode rcm) throws Exception {
	out.println(indent + oid.getString() + " enum " + name + " {");

	Indent xindent = new Indent(indent);
	xindent.push();
	for (int n = 0; n < items.length; n++)
	    out.println(xindent + items[n].getName() + " = " + items[n].getValue()
			+ ",");

	out.println(indent + "};");
    }

    public Object newObj(Database db, Coder coder) throws Exception {
	Enum o = new Enum(db);

	o.make(coder, this, ObjectHeader._Enum_Type);

	newObjRealize(db, o);

	return o;
    }

    private void traceIt(java.io.PrintStream out, Coder coder) {
	// 27/11/98
	// this seems to be wrong: should be
	// decodeByte() to skip, and then decodeInt32()
	//coder.decodeByte();
	EnumItem en = getEnumItemFromVal(coder.decodeInt());

	if (en != null)
	    out.print(en.getName());
	else
	    out.print("<uninitialized>");
    }

    void traceData(java.io.PrintStream out, Coder coder,
		   TypeModifier typmod) {
	if (typmod != null && typmod.pdims > 1)
	    {
		out.print("{");
		for (int i = 0; i < typmod.pdims; i++)
		    {
			if (i != 0)
			    out.print(", ");
			traceIt(out, coder);
		    }
		out.print("}");
		return;
	    }

	traceIt(out, coder);
    }

    EnumItem[] items;
    int items_cnt;

    public static Class idbclass;
}

