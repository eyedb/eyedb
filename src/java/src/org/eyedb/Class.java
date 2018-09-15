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

public class Class extends Object {

    Class parent = null;
    String name = null;
    IndexImpl idximpl;
    int mtype;
    Oid parent_oid = null;
    Oid extent_oid = null;
    Oid components_oid = null;
    Oid class_oid = null;
    Collection extent = null;
    Collection components = null;
    short instance_dspid = Root.DefaultDspid;

    public final int _System = 1;
    public final int _User = 2;

    int idr_psize;
    int idr_vsize;
    int idr_inisize;
    int idr_objsz;

    static Object make(Database db, Oid oid, byte[] idr,
		       Oid class_oid, int lockmode, RecMode rcm)
	throws Exception
    {
	Coder coder = new Coder(idr);

	ClassHints hints = ClassHints.decode(db, coder);
	Class cls = db.getSchema().getClass(hints.name);

	cls.set(db, hints, class_oid);
	//System.out.println("making class " + cls.name + " items = " + (cls.items != null ? cls.items.length : 0) + ", oid = " + cls.oid);

	return cls;
    }

    void set(Database db, ClassHints hints, Oid class_oid) throws Exception {
	idximpl       = hints.idximpl;
	mtype           = hints.mtype;
	extent_oid      = hints.extent_oid;
	components_oid  = hints.components_oid;
	instance_dspid  = hints.dspid;
	this.db         = db;

	this.class_oid = class_oid;

	if (db.opened_state)
	    complete();
    }

    Collection getComponents() throws Exception {
	if (components == null && components_oid != null && components_oid.isValid())
	    components = (Collection)db.loadObject(components_oid);
	return components;
    }

    Collection getExtent() throws Exception {
	if (extent == null && extent_oid != null && extent_oid.isValid())
	    extent = (Collection)db.loadObject(extent_oid);
	return extent;
    }

    private void init(Database db, String name, Class parent,
		      Oid parent_oid) {
	this.db = db;
	this.name = name;
	this.parent_oid = (parent != null ? parent.getOid() : parent_oid);
	this.parent = parent;
	this.idximpl = IndexImpl.getDefaultIndexImpl();
	this.cls = null;
    }

    public boolean compare(Class cls1) {
	//System.out.println("comparing " + name + " with " + cls1.name);
	if ((state & Realizing) != 0)
	    return true;

	if (cls1 == this || cls1.getOid().equals(getOid()))
	    return true;

	if (!cls1.getName().equals(name))
	    return false;
      
	if (type != cls1.type)
	    return false;

	state |= Realizing;
	boolean r = compare_perform(cls1);
	state &= ~Realizing;
	return r;
    }

    public boolean compare_perform(Class cls1) {

	int items_cnt = items.length;

	if (items_cnt != cls1.items.length) {
	    /*
	      System.out.println("comparing because of items: " + items_cnt +
	      " vs. " + cls1.items.length);
	    */
	    return false;
	}

	for (int i = 0; i < items_cnt; i++)
	    if (!cls1.items[i].compare(items[i])) {
		//System.out.println("comparing failed for " + items[i].getName());
		return false;
	    }    
	return true;
    }

    public boolean isSubClass(Class cls1) {
	Class mcl = cls1;

	while (mcl != null)
	    {
		if (compare(mcl))
		    return true;
		mcl = mcl.parent;
	    }

	return false;
    }

    public Class(Database db, String name) {
	init(db, name, null, null);
    }

    public Class(Database db, String name, Class parent) {
	init(db, name, parent, null);
    }

    public Class(String name, Class parent) {
	init(null, name, parent, null);
    }

    Class(Database db, String name, Oid parent_oid) {
	init(db, name, null, parent_oid);
    }

    static private final int DEF_MAG_ORDER = 20000;

    public String getName() {return name;}

    public void setName() {this.name = name;}

    public Class getParent() {
	return parent;
    }

    Value getValueValue(Coder coder) {
	System.err.println("getValueValue(" + name + ")");
	return null;
    }

    void setValueValue(Coder coder, Value value)
	throws Exception {
    }

    Dataspace getDefaultInstanceDataspace() throws org.eyedb.Exception {
	System.out.println("Class.getDefaultInstanceDataspace: " +
			   instance_dspid);
	if (instance_dataspace != null)
	    return instance_dataspace;
	
	if (instance_dspid == Root.DefaultDspid)
	    return null;
	
	instance_dataspace = db.getDataspace(instance_dspid);
	return instance_dataspace;
    }

    void setDefaultInstanceDataspace(Dataspace instance_dataspace)
	throws org.eyedb.Exception {
	Dataspace.notImpl("Class.setDefaultInstanceDataspace: not fully implemented");

	if (this.instance_dataspace == null &&
	    instance_dspid != Root.DefaultDspid) {
	    this.instance_dataspace = db.getDataspace(instance_dspid);
	}

	if (this.instance_dataspace != instance_dataspace) {
	    this.instance_dataspace = instance_dataspace;
	    instance_dspid = (instance_dataspace != null ?
			      instance_dataspace.getId() :
			      Root.DefaultDspid);
	    //touch();
	    //return store();
	}
    }

    void moveInstances(Dataspace target_dataspace)
	throws org.eyedb.Exception {
	moveInstances(target_dataspace, false);
    }

    void moveInstances(Dataspace target_dataspace,
		       boolean include_subclasses)
	throws org.eyedb.Exception {
	Status status;
	status = RPC.rpc_MOVE_INSTANCE_CLASS(db, getOid(),
					     include_subclasses ? 1 : 0,
					     target_dataspace.getId());
	if (!status.isSuccess())
	    throw new org.eyedb.Exception(status, "Class.moveInstances");
    }

    public int getObjectSize() {return idr_objsz;}
    public int getObjectPSize() {return idr_psize;}
    public int getObjectVSize() {return idr_vsize;}
    public int getObjectIniSize() {return idr_inisize;}

    public void traceRealize(java.io.PrintStream out, Indent indent,
			     int flags, RecMode rcm) throws Exception {
	if (this instanceof StructClass)
	    out.print("struct ");
	else if (this instanceof UnionClass)
	    out.print("union ");
	else
	    out.print("class ");

	out.print(name);
    
	Class p = parent;

	while (p != null)
	    {
		out.print(" : " + p.name);
		p = p.parent;
	    }

	out.println(" {");

	indent.push();

	//if (items == null)
	complete();

	int items_cnt = items.length;
	for (int n = 0; n < items_cnt; n++)
	    {
		Attribute item = items[n];

		if (item.cls == null || item.class_owner == null)
		    continue;

		TypeModifier typmod = item.typmod;
		boolean isString = false;
		if (item.cls.name.equals("char") && typmod.ndims == 1)
		    isString = true;

		if (isString)
		    {
			out.print(indent + "string");
			if (typmod.dims[0] > 1)
			    out.print("<" + typmod.dims[0] + ">");
		    }
		else
		    out.print(indent + item.cls.name);

		if (item.isIndirect())
		    out.print("*");

		out.print(" ");

		if (!item.class_owner.name.equals(name))
		    out.print(item.class_owner.name + "::");

		out.print(item.name);

		if (!isString)
		    for (int j = 0; j < typmod.ndims; j++)
			{
			    if (typmod.dims[j] < 0)
				out.print("[]");
			    else
				out.print("[" + typmod.dims[j] + "]");
			}

		int idx_mode = item.idx_mode;
		boolean attr_list = false;

		if ((idx_mode & Attribute.IndexBTreeItemMode) ==
		    Attribute.IndexBTreeItemMode)	  
		    {
			if (!attr_list) {attr_list = true; out.print(" (");}
			else {out.print(", ");}
			out.print("index<btree>");
		    }

		if ((idx_mode & Attribute.IndexBTreeCompMode) ==
		    Attribute.IndexBTreeCompMode)	  
		    {
			if (!attr_list) {attr_list = true; out.print(" (");}
			else {out.print(", ");}
			if (isString)
			    out.print("index<btree>");
			else
			    out.print("index[]<btree>");
		    }

		if ((idx_mode & Attribute.IndexHashItemMode) ==
		    Attribute.IndexHashItemMode)	  
		    {
			if (!attr_list) {attr_list = true; out.print(" (");}
			else {out.print(", ");}
			out.print("index<hash>");
		    }

		if ((idx_mode & Attribute.IndexHashCompMode) ==
		    Attribute.IndexHashCompMode)	  
		    {
			if (!attr_list) {attr_list = true; out.print(" (");}
			else {out.print(", ");}
			if (isString)
			    out.print("index<hash>");
			else
			    out.print("index[]<hash>");
		    }

		if (attr_list)
		    out.print(")");

		out.println(";");
	    }

	indent.pop();

	out.println(indent + "};");
    }

    void makeObjectArrays(Object o) {
	if (items == null)
	    return;
	
	int count = items.length;
	o.objects = new ObjectArray[count];
	o.pdata   = new Data[count];
	o.oids    = new OidArray[count];
    }
    
    private Attribute[] makeNative()
    {
	if (name.equals("class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("basic_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("enum_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("agregat_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("struct_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("union_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("collection_class"))
	    return AttrNative.make(db, AttrNative.CLASS);

	if (name.equals("set_class"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("bag_class"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("list_class"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("array_class"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("set"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("bag"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("list"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.equals("array"))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.regionMatches(0, "set<", 0, 4))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.regionMatches(0, "bag<", 0, 4))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.regionMatches(0, "list<", 0, 4))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	if (name.regionMatches(0, "array<", 0, 4))
	    return AttrNative.make(db, AttrNative.COLLECTION);

	return AttrNative.make(db, AttrNative.OBJECT);
    }

    void complete() throws Exception {
	if (items == null)
	    items = makeNative();

	if (parent_oid != null && parent_oid.isValid() && parent == null)
	    parent = db.getSchema().getClass(parent_oid);

	if (class_oid != null && cls == null)
	    cls = db.getSchema().getClass(class_oid);

	if ((cls == null || parent == null) &&
	    this instanceof CollectionClass)
	    ((CollectionClass)this).completeClassAndParent(db);

	for (int n = 0; n < items.length; n++)
	    items[n].complete(db);
    }

    public Object newObj(Database db) throws Exception {
	return newObj(db, null);
    }

    public Object newObj(Database db, Coder coder) throws Exception {
	return null;
    }

    public Attribute[] getAttributes() {return items;}

    public Attribute getAttribute(String s) {
	for (int i = items.length-1; i >= 0; i--)
	    if (items[i].getName().equals(s))
		return items[i];

	return null;
    }

    public void setAttributes(Attribute[] items, int from, int nb) throws Exception {

	Attribute[] nitems = new Attribute[from + nb];

	Attribute[] nat = AttrNative.make(db, AttrNative.OBJECT);

	for (int i = 0; i < nat.length; i++)
	    nitems[i] = nat[i];

	if (parent != null && parent instanceof AgregatClass)
	    for (int i = nat.length; i < parent.items.length; i++)
		nitems[i] = Attribute.make(parent.items[i], i);

	for (int i = from; i < from+nb; i++)
	    nitems[i] = Attribute.make(items[i], i);

	this.items = nitems;

	compile();
    }

    public void newObjRealize(Database db, Object o) throws Exception {

	makeObjectArrays(o);

	int items_cnt = items.length;

	for (int n = 0; n < items_cnt; n++)
	    items[n].newObjRealize(db, o);
    }

    void compile() throws Exception {
	int n;
	int offset;
	int size, inisize = 0;
	Value x_offset;
	Value x_size;
	Value x_inisize;

	boolean isunion = (this instanceof UnionClass);

	offset = ObjectHeader.IDB_OBJ_HEAD_SIZE;

	if (isunion)
	    offset += Coder.INT16_SIZE;

	size = 0;

	x_offset  = new Value(offset);
	x_size    = new Value(size);
	x_inisize = new Value(inisize);

	int items_cnt = items.length;

	for (n = 0; n < items_cnt; n++)
	    items[n].compile_perst(this, x_offset, x_size, x_inisize);

	size    = x_size.getInt();
	offset  = x_offset.getInt();
	inisize = x_inisize.getInt();

	if (isunion)
	    {
		idr_psize = size + ObjectHeader.IDB_OBJ_HEAD_SIZE +
		    Coder.INT16_SIZE;
		offset = idr_psize;
	    }
	else
	    idr_psize = offset;
    
	size = 0;
	idr_inisize = inisize;

	x_offset = new Value(offset);
	x_size   = new Value(size);

	for (n = 0; n < items_cnt; n++)
	    items[n].compile_volat(this, x_offset, x_size);

	size    = x_size.getInt();
	offset  = x_offset.getInt();

	if (isunion)
	    idr_vsize = size;
	else
	    idr_vsize = offset - idr_psize;

	idr_objsz = idr_psize + idr_vsize;

	/*
	  System.out.println("Class " + name + " {");
	  System.out.println("   Persistent Size = " + idr_psize);
	  System.out.println("   Volatile Size   = " + idr_vsize);
	  System.out.println("   Total Size      = " + idr_objsz);
	  System.out.println("}");
	*/
    }

    public boolean isSystem() {
	return mtype == _System;
    }

    void traceData(java.io.PrintStream out, Coder coder, TypeModifier typmod) {
    }

    Attribute[] items = null;

    static Class make(Schema sch, String name,
		      Class parent) {
	Class cls = new Class(name, parent);
	return sch.addClass(cls);
    }

    static void init_1(Schema sch) {
	Object.idbclass          = make(sch, "object", null);

	Class.idbclass      = make(sch, "class", Object.idbclass);

	BasicClass.idbclass      = make(sch, "basic_class", Class.idbclass);
	EnumClass.idbclass       = make(sch, "enum_class", Class.idbclass);
	AgregatClass.idbclass    = make(sch, "agregat_class", Class.idbclass);
	StructClass.idbclass     = make(sch, "struct_class", Class.idbclass);
	UnionClass.idbclass      = make(sch, "union_class", Class.idbclass);
    
	Instance.idbclass           = make(sch, "instance", Object.idbclass);

	Basic.idbclass           = make(sch, "basic", Instance.idbclass);
	Enum.idbclass            = make(sch, "enum", Instance.idbclass);
	Agregat.idbclass         = make(sch, "agregat", Instance.idbclass);
	Struct.idbclass          = make(sch, "struct", Agregat.idbclass);
	Union.idbclass           = make(sch, "union", Agregat.idbclass);
	Schema.idbclass          = make(sch, "schema", Agregat.idbclass);
    
	CollectionClass.idbclass = make(sch, "collection_class", Class.idbclass);

	CollSetClass.idbclass    = make(sch, "set_class", CollectionClass.idbclass);
	CollBagClass.idbclass    = make(sch, "bag_class", CollectionClass.idbclass);
	CollListClass.idbclass   = make(sch, "list_class", CollectionClass.idbclass);
	CollArrayClass.idbclass  = make(sch, "array_class", CollectionClass.idbclass);
    
	Collection.idbclass      = make(sch, "collection", Instance.idbclass);

	CollSet.idbclass         = make(sch, "set", Collection.idbclass);
	CollBag.idbclass         = make(sch, "bag", Collection.idbclass);
	CollList.idbclass        = make(sch, "list", Collection.idbclass);
	CollArray.idbclass       = make(sch, "array", Collection.idbclass);

	Class collsetcls = new CollSetClass(Object.idbclass, true);
	sch.addClass(collsetcls);

	sch.addClass(Bool.idbclass  = makeBoolClass());
	sch.addClass(Char.idbclass  = new CharClass());
	sch.addClass(Byte.idbclass  = new ByteClass());
	sch.addClass(OidP.idbclass  = new OidClass());
	sch.addClass(Int16.idbclass = new Int16Class());
	sch.addClass(Int32.idbclass = new Int32Class());
	sch.addClass(Int64.idbclass = new Int64Class());
	sch.addClass(Float.idbclass = new FloatClass());
    }

    static void init_2(Schema sch) {

	Attribute[] object_items =
	    AttrNative.makeObjectItems(sch);

	Attribute[] cls_items =
	    AttrNative.makeClassItems(sch);

	Attribute[] collection_items =
	    AttrNative.makeCollectionItems(sch);

	Attribute[] collection_class_items =
	    AttrNative.makeCollectionClassItems(sch);

	Object.idbclass.items         = object_items;

	Class.idbclass.items      = cls_items;

	BasicClass.idbclass.items   = cls_items;
	Int16Class.idbclass.items   = cls_items;
	Int32Class.idbclass.items   = cls_items;
	Int64Class.idbclass.items   = cls_items;
	FloatClass.idbclass.items   = cls_items;
	OidClass.idbclass.items     = cls_items;
	CharClass.idbclass.items    = cls_items;
	ByteClass.idbclass.items    = cls_items;
	EnumClass.idbclass.items    = cls_items;
	AgregatClass.idbclass.items = cls_items;
	StructClass.idbclass.items  = cls_items;
	UnionClass.idbclass.items   = cls_items;
    
	Instance.idbclass.items     = object_items;

	Basic.idbclass.items   = object_items;
	Int16.idbclass.items   = object_items;
	Int32.idbclass.items   = object_items;
	Int64.idbclass.items   = object_items;
	Float.idbclass.items   = object_items;
	OidP.idbclass.items    = object_items;
	Bool.idbclass.items    = object_items;
	Char.idbclass.items    = object_items;
	Byte.idbclass.items    = object_items;
	Enum.idbclass.items    = object_items;
	Agregat.idbclass.items = object_items;
	Struct.idbclass.items  = object_items;
	Union.idbclass.items   = object_items;
	Schema.idbclass.items  = object_items;
    
	CollectionClass.idbclass.items = cls_items;

	CollSetClass.idbclass.items    = collection_class_items;
	CollBagClass.idbclass.items    = collection_class_items;
	CollListClass.idbclass.items   = collection_class_items;
	CollArrayClass.idbclass.items  = collection_class_items;
    
	Collection.idbclass.items     = object_items;

	CollSet.idbclass.items        = collection_items;
	CollBag.idbclass.items        = collection_items;
	CollList.idbclass.items       = collection_items;
	CollArray.idbclass.items      = collection_items;
    }

    static void init_0() {

	init_1(Schema.bootstrap);
	init_2(Schema.bootstrap);
    }

    static {
	init_0();
    }

    void codeName(Coder coder) throws Exception {
	int len = name.length();
	if (len >= ObjectHeader.IDB_CLASS_NAME_LEN)
	    throw new Exception("INTERNAL_ERROR", "Name of Class '" + name + "' is too long: not yet implemented");

	coder.code((byte)ObjectHeader.IDB_NAME_IN_PLACE);
	coder.code(name, ObjectHeader.IDB_CLASS_NAME_LEN);
    }

    public short getInstanceDspid() {
	return instance_dspid;
    }

    public void setInstanceDspid(short instance_dspid) {
	this.instance_dspid = instance_dspid;
    }

    public static Class idbclass;

    private static Class makeBoolClass() {
	EnumClass bool_class = new EnumClass("bool");

	EnumItem[] en = new EnumItem[2];
	en[0] = new EnumItem("FALSE", 0, 0);
	en[1] = new EnumItem("TRUE", 1, 1);

	bool_class.setEnumItems(en);

	return bool_class;
    }

    Dataspace instance_dataspace;
}
