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

public class Attribute {

  public boolean compare(Attribute item)
  {
    /*
    System.out.println("attribute: " +
		       item.num + " vs. " + num + " " +
		       item.name + " vs. " + name + " " +
		       item.cls.getName() + " vs. " + cls.getName());
    */

    if (item.num != num)
      return false;

    if (!item.name.equals(name))
      return false;

    if (!item.cls.compare(cls))
      return false;

    return typmod.compare(item.typmod);
  }

  public String getName() {
    return name;
  }

  public boolean hasInverse() {
    return inverse != null && inverse.class_oid.isValid();
  }

  public AttrInverse getInverse() {
    return inverse;
  }

  public TypeModifier getTypeModifier() {
    return typmod;
  }

  public Class getFClass() {
    return cls;
  }

  public Class getClassOwner() {
    return class_owner;
  }

  public int getNum() {
    return num;
  }

  public Attribute(Class cls, Class class_owner,
		      String name, int num, boolean isref) {
    init(cls, class_owner, name, num);
    typmod = TypeModifier.make(isref, 0, null);
  }

  public Attribute(Class cls, Class class_owner,
		   String name, int num, boolean isref, int ndims, int[] dims) {
    init(cls, class_owner, name, num);
    typmod = TypeModifier.make(isref, ndims, dims);
  }

  public Attribute(Attribute item, int num) {
    this.num          = num;
    cls               = item.cls;
    class_owner       = item.class_owner;
    class_oid         = item.cls.getOid();
    class_owner_oid   = item.class_owner.getOid();
    name              = item.name;

    inverse           = item.inverse;
    basic_enum        = item.basic_enum;
    dspid             = item.dspid;
    idx_item_oid      = item.idx_item_oid;
    idx_comp_oid      = item.idx_comp_oid;
    idx_item_obj_oid  = item.idx_item_obj_oid;
    idx_comp_obj_oid  = item.idx_comp_obj_oid;

    idx_item          = item.idx_item;
    idx_comp          = item.idx_comp;

    idr_inisize       = item.idr_inisize;
    idr_poff          = item.idr_poff;
    idr_item_psize    = item.idr_item_psize;
    idr_psize         = item.idr_psize;
    idr_voff          = item.idr_voff;
    idr_item_vsize    = item.idr_item_vsize;
    idr_vsize         = item.idr_vsize;

    is_string         = item.is_string;

    notnull           = item.notnull;
    notnull_comp      = item.notnull_comp;
    unique            = item.unique;
    unique_comp       = item.unique_comp;
    dataspace	      = item.dataspace;

    typmod = TypeModifier.make
      ((item.typmod.mode & TypeModifier._Indirect) != 0,
       item.typmod.ndims,
       item.typmod.dims);
  }

  public Attribute(int idx_mode, Class cls,
		      Class class_owner, String name, int num,
		      boolean isref, int ndims, int[] dims) {
    init(cls, class_owner, name, num);
    typmod = TypeModifier.make(isref, ndims, dims);
  }

  public Attribute(Class cls, Class class_owner,
		      String name, int num, int dim) {
    init(cls, class_owner, name, num);
    int[] dims;
    if (dim > 0)  {
      dims = new int[1];
      dims[0] = dim;
    }
    else
      dims = null;
       
    typmod = TypeModifier.make(false, (dims == null ? 0 : 1), dims);
  }

  public Attribute(Database db, Coder coder, int num) throws Exception {
    this.num = num;

    // added 16/05/05
    endoff = coder.decodeInt();
    short code = coder.decodeShort();

    class_oid   = coder.decodeOid();
    cls = null;
    class_owner_oid = coder.decodeOid();
    class_owner = null;

    if (db.getVersion() >= Root.MAGORDER_VERSION)
      magorder = coder.decodeInt();

    Oid inv_class_oid = coder.decodeOid();
    short inv_num = coder.decodeShort();

    inverse = new AttrInverse(db, inv_class_oid, inv_num, this);

    basic_enum = (coder.decodeChar() > 0 ? true : false);
    is_string = (coder.decodeChar() > 0 ? true : false);

    dspid = coder.decodeShort();

    attr_comp_set_oid = coder.decodeOid();

    name = coder.decodeString();

    // 16/05/05: disconnected
    /*
    idx_mode = coder.decodeShort();

    idx_item_oid = coder.decodeOid();
    idx_item = null;
    idx_comp_oid = coder.decodeOid();
    idx_comp = null;

    idx_item_obj_oid = coder.decodeOid();
    idx_comp_obj_oid = coder.decodeOid();
    */

    idr_inisize = coder.decodeInt();
    idr_poff = coder.decodeInt();
    idr_item_psize = coder.decodeInt();
    idr_psize = coder.decodeInt();
    idr_voff = coder.decodeInt();
    idr_item_vsize = coder.decodeInt();
    idr_vsize = coder.decodeInt();

    typmod = new TypeModifier();

    typmod.decode(coder);

    notnull = notnull_comp = unique = unique_comp = false;

    //trace();
  }

  public void realize(Database db, Object o, Oid clsoid,
		      Oid objoid, AttrIdxContext idx_ctx,
		      RecMode rcm) throws Exception {
  }

  public void trace() {
    System.out.println("name " + name);
    System.out.println("num " + num);
    System.out.println("code " + code);
    System.out.println("class " + cls);
    System.out.println("class_owner " + class_owner);
    System.out.println("typmod " + typmod);
    System.out.println("basic_enum " + basic_enum);
    System.out.println("dspid " + dspid);
    System.out.println("idr_poff " + idr_poff);
    System.out.println("idr_item_psize " + idr_item_psize);
    System.out.println("idr_psize " + idr_psize);
    System.out.println("idr_inisize " + idr_inisize);
    System.out.println("idr_voff " + idr_voff);
    System.out.println("idr_item_vsize " + idr_item_vsize);
    System.out.println("idr_vsize " + idr_vsize);
    System.out.println("idx_item_oid " + idx_item_oid);
    System.out.println("idx_comp_oid " + idx_comp_oid);
    System.out.println("idx_item_obj_oid " + idx_item_obj_oid);
    System.out.println("idx_comp_obj_oid " + idx_comp_obj_oid);
    System.out.println("class_oid " + class_oid);
    System.out.println("class_owner_oid " + class_owner_oid);

    typmod.trace();
  }

  public static Attribute make(Database db, Coder coder, int num) throws Exception {

      int endoff = coder.decodeInt();
      short code = coder.decodeShort();

      coder.setOffset(coder.getOffset() - 2);
      coder.setOffset(coder.getOffset() - 4);

    switch(code)
      {
      case AttrDirect_Code:
	return new AttrDirect(db, coder, num);

      case AttrIndirect_Code:
	return new AttrIndirect(db, coder, num);

      case AttrVarDim_Code:
	return new AttrVarDim(db, coder, num);

      case AttrIndirectVarDim_Code:
	return new AttrIndirectVarDim(db, coder, num);

      default:
	System.err.println("Attribute.make(" + code + ") return null");
	return null;
      }
  }

  void complete(Database db) throws Exception {
    /*   System.out.println("complete(" + name + ")" +
	 class_oid + ", " +
	 class_owner_oid); */
    Schema sch = db.getSchema();

    if (cls == null && class_oid.isValid())
      cls = sch.getClass(class_oid);

    if (class_owner == null && class_owner_oid.isValid())
      class_owner = sch.getClass(class_owner_oid);

    // what about inverse ?
    if (inverse != null)
      inverse.complete();
  }

  void newObjRealize(Database db, Object o) throws Exception {
  }

  String name;
  int num;
  int code;
  Class cls;
  Class class_owner;
  TypeModifier typmod;

  boolean basic_enum;
  Oid attr_comp_set_oid; // 16/05/05: added
  short dspid;

  int idr_poff;
  int idr_item_psize;
  int idr_psize;
  int idr_inisize;

  int idr_voff;
  int idr_item_vsize;
  int idr_vsize;

  Oid idx_item_oid;
  Oid idx_comp_oid;
  Oid idx_item_obj_oid;
  Oid idx_comp_obj_oid;

    java.lang.Object idx_item; // not used in frontend
    java.lang.Object idx_comp;

  short idx_mode;

  Oid class_oid;
  Oid class_owner_oid;
  int magorder;
  int endoff;
 
  boolean is_string = false;
  boolean is_string_init = false;

  boolean notnull;
  boolean notnull_comp;
  boolean unique;
  boolean unique_comp;

  AttrInverse inverse;

  public boolean isIndirect() {
    return (typmod.mode == TypeModifier.Indirect ||
	    typmod.mode == TypeModifier.IndirectVarDim);
  }

  public boolean isVarDim()   {
    return (typmod.mode == TypeModifier.VarDim ||
	    typmod.mode == TypeModifier.IndirectVarDim);
  }

  public boolean isNative()   {return false;}

  void load(Database db, Object o, Oid clsoid,
	    int lockmode, AttrIdxContext idx_ctx, RecMode rcm) throws Exception {
  }

  static protected final int AttrDirect_Code         = 0x31;
  static protected final int AttrIndirect_Code       = 0x32;
  static protected final int AttrVarDim_Code         = 0x33;
  static protected final int AttrIndirectVarDim_Code = 0x34;
  static protected final int AttrNative_Code         = 0x35;

  public static final int IndexDefaultMode   = 0x0;
  public static final int IndexItemMode      = 0x01;
  public static final int IndexCompMode      = 0x02;
  public static final int IndexBTree         = 0x10;
  public static final int IndexHash          = 0x20;
  public static final int IndexBTreeItemMode = IndexBTree | IndexItemMode;
  public static final int IndexBTreeCompMode = IndexBTree | IndexCompMode;
  public static final int IndexHashItemMode  = IndexHash  | IndexItemMode;
  public static final int IndexHashCompMode  = IndexHash  | IndexCompMode;

  void trace(java.io.PrintStream out, Object o, Indent indent,
	     int flags, RecMode rcm) throws Exception {
  }

  protected String getPrefix(Object o)
  {
    String name = class_owner.getName();

    if (!name.equals(o.getClass(true).getName()))
      return name + "::";

    return "";
  }

  protected int shift(int x)   {return x >> 3;}

  protected int unshift(int x) {return x << 3;}

  protected int mask(int from, int sfrom) {
    return (1 << (7 - (from - (unshift(sfrom)))));
  }

  protected int smask(int from, int sfrom) {
    return (0xff >> (from - (unshift(sfrom))));
  }

  protected int emask(int to, int sto) {
    return (0xff << (7 - (to - (unshift(sto)))));
  }

  protected int IS_LOADED_MASK = 0x40000000;
  protected int SZ_MASK = 0x80000000;

  protected int clean_size(int x) {
    return x & ~(SZ_MASK|IS_LOADED_MASK);
  }

  protected boolean is_size_changed(int x) {
    return ((x & SZ_MASK) != 0 ? true : false);
  }

  protected int set_size_changed(int x, boolean changed) {
    return (changed ? (x | SZ_MASK) : (x & ~SZ_MASK));
  }

  protected int iniSize(int sz)
  {
    if (sz == 0)
      return 0;
    return ((sz-1) >> 3) + 1;
  }

  protected boolean isNull(byte[] inidata, int offset, int nb, int from)
  {
    if (nb == 0)
      return true;

    int sfrom = shift(from);

    if (nb == 1) {
      int m = mask(from, sfrom);
      if ((inidata[sfrom+offset] & m) != 0)
	return false;
      return true;
    }

    int to = from + nb - 1;
    int sto = shift(to);

    int sm = smask(from, sfrom);
    int em = emask(to, sto);

    if ((inidata[sfrom+offset] & sm) != 0)
      return false;

    if ((inidata[sto+offset] & em) != 0)
      return false;

    int end = sto+offset;
    for (int j = sfrom+offset+1; j < end; j++)
      if (inidata[j] != 0)
	return false;

    return true;
  }

  protected boolean isNull(byte[] inidata, int offset,
			   TypeModifier typmod) {
    return isNull(inidata, offset, typmod.pdims, 0);
  }


  protected void setIni(byte[] inidata, int offset, int nb, int from,
			boolean val)
  {
    if (nb == 0)
      return;

    int sfrom = shift(from);

    if (nb == 1) {
      int m = mask(from, sfrom);
      if (val)
	inidata[sfrom+offset] |= m;
      else
	inidata[sfrom+offset] &= ~m;
      return;
    }

    int to = from + nb - 1;
    int sto = shift(to);

    int sm = smask(from, sfrom);
    int em = emask(to, sto);

    int end = sto+offset;

    if (val) {
      inidata[sfrom+offset] |= sm;
      inidata[sto+offset]   |= em;

      for (int j = sfrom+offset+1; j < end; j++)
	inidata[j] = (byte)0xff;
    }
    else
      {
	inidata[sfrom+offset] &= ~sm;
	inidata[sto+offset]   &= ~em;

	for (int j = sfrom+offset+1; j < end; j++)
	  inidata[j] = 0;
      }
  }

  //
  // Oid accessor methods
  //

  public Oid getOid(Object o) {
    Oid[] oid = getOid(o, 1, 0);
    if (oid == null)
      return null;
    return oid[0];
  }

  public Oid getOid(Object o, int which) {
    Oid[] oid = getOid(o, 1, which);
    if (oid == null)
      return null;
    return oid[0];
  }

  void setSizeChanged(Object o, boolean changed) throws Exception {
  }

  boolean isSizeChanged(Object o) {
    return false;
  }

  //
  // Oid modifier methods
  //

  public Oid[] getOid(Object o, int nb, int from) {
    return null;
  }

  public void setOid(Object o, Oid oid) {
    Oid oids[] = new Oid[1];
    oids[0] = oid;
    setOid(o, oids, 1, 0);
  }

  public void setOid(Object o, Oid oid, int which) {
    Oid oids[] = new Oid[1];
    oids[0] = oid;
    setOid(o, oids, 1, which);
  }

  public void setOid(Object o, Oid[] oid, int nb, int from) {
  }

  //
  // Value accessor methods
  //

  public Value getValue(Object o) throws Exception {

    return getValue(o, false);
  }

  public Value getValue(Object o, int which) throws Exception {

    return getValue(o, which, false);
  }

  public Value[] getValue(Object o, int nb, int from) throws Exception {

    return getValue(o, nb, from, false);
  }

  public Value getValue(Object o, boolean autoload) throws Exception {

    Value[] value = getValue(o, 1, 0, autoload);
    if (value == null)
      return null;
    return value[0];
  }

  public Value getValue(Object o, int which, boolean autoload) throws Exception {

    Value[] value = getValue(o, 1, which, autoload);
    if (value == null)
      return null;
    return value[0];
  }

  public Value[] getValue(Object o, int nb, int from, boolean autoload) throws Exception {

    return null; // abstract, must be redefined in inherited clses
  }

  public String getStringValue(Object o) throws Exception {
    return null;
  }

  public byte[] getRawValue(Object o) throws Exception {
    return null;
  }

  //
  // Value modifier methods
  //

  public void setValue(Object o, Value value)
    throws Exception {
    Value[] values = new Value[1];
    values[0] = value;
    setValue(o, values, 1, 0);
  }

  public void setValue(Object o, Value value, int which)
    throws Exception {
    Value[] values = new Value[1];
    values[0] = value;
    setValue(o, values, 1, which);
  }

  public void setValue(Object o, Value[] value, int nb, int from)
    throws Exception {
    // abstract
  }


  public int getSize(Object o) {
    return 1;
  }

  public void setSize(Object o, int size) throws Exception {
  }

  public void setStringValue(Object o, String s)
    throws Exception {
  }

  public void setRawValue(Object o, byte[] b)
    throws Exception {
  }

  //
  // private methods
  //

  private void init(Class cls, Class class_owner,
		    String name, int num) {
    this.cls           = cls;
    this.class_owner     = class_owner;
    this.class_oid       = cls.getOid();
    this.class_owner_oid = class_owner.getOid();
    this.name         = name;
    this.num          = num;

    inverse = null;
    basic_enum = cls instanceof EnumClass ||
      cls instanceof BasicClass;

    dspid = Root.DefaultDspid;
    idx_item_oid = null;
    idx_comp_oid = null;
    idx_item_obj_oid = null;
    idx_comp_obj_oid = null;

    idx_item = null;
    idx_comp = null;

    idr_inisize = 0;
    idr_poff = 0;
    idr_item_psize = 0;
    idr_psize = 0;
    idr_voff = 0;
    idr_item_vsize = 0;
    idr_vsize = 0;

    notnull = notnull_comp = unique = unique_comp = false;
  }

  private void initIsString() {
    if (is_string_init)
      return;

    if (cls != null) {
      is_string = cls.getName().equals("char") && !isIndirect() &&
	typmod.ndims == 1 && typmod.dims[0] != 0;
      is_string_init = true;
    }
    /*
      System.out.println("is_string = " + is_string + " for " + name +
      ", class = " + (cls != null ? cls.getName() : "NULL") +
      ", ndims: " + typmod.ndims);
    */
  }

  //
  // protected methods
  //

  protected int getPOff() {
    return idr_poff;
  }

  protected int getIniSize(Object o) {
    return idr_inisize;
  }

  protected Oid[] getIndirectOid(Object o, int nb, int from) {

    Oid[] xoid = new Oid[nb];
    Oid[] oids = o.oids[num].oids;

    for (int i = 0; i < nb; i++)
    xoid[i] = oids[i+from];

    return xoid;
  }

  protected void setIndirectOid(Object o, Oid[] xoid, int nb, int from) {

    Oid[] oids = o.oids[num].oids;

    for (int i = 0; i < nb; i++)
      oids[i+from] = xoid[i];
  }

  protected Value[] getIndirectValue(Object o, int nb, int from,
					boolean autoload) throws Exception {

					  Value[] values     = new Value[nb];
					  Object[] objects = o.objects[num].objects;
					  Oid[]    oids    = o.oids[num].oids;
					  Database db      = o.getDatabase();

					  for (int i = 0; i < nb; i++) {
					    Object oo = objects[i+from];
					    if (oo == null && oids[i+from] != null &&
						oids[i+from].isValid() && autoload)
					      oo = db.loadObjectRealize(oids[i+from],
									TransactionParams.DefaultLock,
									RecMode.NoRecurs);
					    values[i] = new Value(oo);
					  }
					  return values;
					}

  protected void setIndirectValue(Object o, Value[] value, int nb, int from)
    throws Exception {
	 
    Object[] objects = o.objects[num].objects;

    for (int i = 0; i < nb; i++)
      objects[i+from] = value[i].getObject();
  }

  //
  //
  //

  protected Value[] getDirectValue(byte[] pdata, Object o, int nb,
				      int from, boolean autoload)
  {
    
    Value[] value = new Value[nb];
    
    if (basic_enum) {
      int poff = getPOff();
      int inisize = getIniSize(o);

      for (int i = 0,
	     offset = poff + inisize + from * idr_item_psize;
	   i < nb; i++, offset += idr_item_psize) {
	if (isNull(pdata, poff, 1, i+from))
	  value[i] = new Value();
	else
	  value[i] = cls.getValueValue(new Coder(pdata, offset));
      }

      return value;
    }

    Object[] objects = o.objects[num].objects;

    for (int i = 0; i < nb; i++)
    value[i] = new Value(objects[i+from]);

    return value;
  }

  protected void setDirectValue(byte[] pdata, Object o, Value[] value,
				int nb, int from) 
    throws Exception
  {

    if (basic_enum) {
      int poff = getPOff();
      int inisize = getIniSize(o);

      for (int i = 0,
	     offset = poff + inisize + from * idr_item_psize;
	   i < nb; i++, offset += idr_item_psize) {
	cls.setValueValue(new Coder(pdata, offset, true), value[i]);
	setIni(pdata, poff, 1, i+from, true);
      }
      o.touch();
      return;
    }

    Object[] objects = o.objects[num].objects;

    for (int i = 0; i < nb; i++) {
      objects[i+from] = value[i].getObject();
      if (!isIndirect()) {
	Object master_object = objects[i+from].getMasterObject();
	if (master_object != null && master_object != o)
	  throw new Exception
	    (new Status(Status.IDB_ATTRIBUTE_ERROR,
			   "setting attribute value " + name +
			   ": object of class " +
			   o.getClass(true).getName() +
			   " cannot be shared between several objects."));
	objects[i+from].setMasterObject(o);
      }
    }


  }

  //
  // string type
  //

  protected String getDirectStringValue(byte[] pdata, Object o) 
    throws Exception
  {

    if (pdata == null)
      return null;

    if (basic_enum && cls instanceof CharClass &&
	(typmod.pdims > 1 || this instanceof AttrVD)) {
      int poff = getPOff();
      int inisize = getIniSize(o);

      if (isNull(pdata, poff, typmod))
	return null;

      return new Coder(pdata, poff + inisize).decodeString(typmod.pdims * getSize(o));
    }

    throw new InvalidValueTypeException
      (new Status(Status.IDB_ERROR, name + " is not of string type"));
  }

  protected void setDirectStringValue(byte[] pdata, Object o, String s)
    throws Exception 
  {

    if (basic_enum && cls instanceof CharClass &&
	(typmod.pdims > 1 || this instanceof AttrVD)) {
      int poff = getPOff();
      int inisize = getIniSize(o);

      if (pdata.length < s.length() + inisize + 1)
	throw new InvalidValueTypeException
	  (new Status(Status.IDB_ERROR, "attribute " + name +
			 ": size (=" + pdata.length +
			 ") must be at least " +
			 (s.length() + inisize + 1)));

      new Coder(pdata, poff + inisize, true).code(s, true);
      setIni(pdata, poff, s.length() + 1, 0, true);
      o.touch();
      return;
    }

    throw new InvalidValueTypeException
      (new Status(Status.IDB_ERROR, name + " is not of string type"));
  }

  //
  // raw type
  //

  protected byte[] getDirectRawValue(byte[] pdata, Object o)
    throws Exception
  {

    if (basic_enum && cls instanceof ByteClass &&
	(typmod.pdims > 1 || this instanceof AttrVD)) {
	  int poff = getPOff();
	  int inisize = getIniSize(o);

	  if (isNull(pdata, poff, typmod))
	  return null;

	  int size = getSize(o);
	  byte b[] = new byte[size];
	  System.arraycopy(pdata, poff + inisize, b, 0, size);
	  return b;
	}

    throw new InvalidValueTypeException
      (new Status(Status.IDB_ERROR, name + " is not of raw type"));
  }

  protected void setDirectRawValue(byte[] pdata, Object o, byte data[])
    throws Exception
  {

    if (basic_enum && cls instanceof ByteClass &&
	(typmod.pdims > 1 || this instanceof AttrVD))
      {
	int poff = getPOff();
	int inisize = getIniSize(o);

	System.arraycopy(data, 0, pdata, poff + inisize, data.length);
	setIni(pdata, poff, data.length, 0, true);
	o.touch();
	return;
      }

    throw new InvalidValueTypeException
      (new Status(Status.IDB_ERROR, name + " is not of raw type"));
  }

  void compile_perst(Class mcl, Value x_offset, Value x_size,
		     Value x_inisize) throws Exception {
  }

  void compile_volat(Class mcl, Value x_offset, Value x_size) throws Exception {
  }

  static Attribute make(Attribute item, int num) {

    if (item.isNative())
      return item;

    boolean is_vardim = item.isVarDim();
    boolean is_indirect = item.isIndirect();

    if (is_vardim && is_indirect)
      return new AttrIndirectVarDim(item, num);

    if (!is_vardim && is_indirect)
      return new AttrIndirect(item, num);

    if (is_vardim && !is_indirect)
      return new AttrVarDim(item, num);

    if (!is_vardim && !is_indirect)
      return new AttrDirect(item, num);

    return null;
  }

  void compile_update(Class mcl, int isz, Value x_offset,
		      Value x_size) throws Exception
  {
    if (mcl instanceof UnionClass) {
      if (isz > x_size.getInt())
	x_size.set(isz);
    }
    else {
      x_offset.set(x_offset.getInt() + isz);
      x_size.set(isz);
    }
  }

  protected void dump(byte b[], int sz) {
    System.out.println("dump " + b + ", size = " + sz);
    for (int i = 0; i < sz; i++)
      System.out.print("[" + i + "] = " + (int)b[i] + " ");
    System.out.println("");
  }

  protected boolean VARS_COND() {
    return isString();
  }

  protected final int VARS_INISZ = 3;
  protected final int VARS_SZ = 24;
  protected final int VARS_TSZ = VARS_INISZ + VARS_SZ;

  protected final int VARS_OFFSET = Coder.INT32_SIZE + Coder.OID_SIZE;

  protected boolean isString() {
    if (!is_string_init)
      initIsString();
    return is_string;
  }

  Oid getVarDimOid(Object o) {
    return null;
  }

  void setVarDimOid(Object o, Oid oid) {
  }

    Dataspace getDefaultDataspace() throws org.eyedb.Exception {
	Dataspace.notImpl("Attribute.getDefaultDataspace");
	if (dataspace != null)
	    return dataspace;

	if (dspid == Root.DefaultDspid)
	    return null;

	if (cls == null)
	    throw new org.eyedb.Exception
		(new Status(Status.IDB_ERROR, "attribute " + name +
			    "is not completed"));

	dataspace = cls.getDatabase().getDataspace(dspid);
	return dataspace;
    }

    void setDefaultDataspace(Dataspace dataspace) throws org.eyedb.Exception {
	Dataspace.notImpl("Attribute.setDefaultDataspace: partially implemented");
	if (this.dataspace == null && dspid != Root.DefaultDspid) {
	    if (cls == null)
		throw new org.eyedb.Exception
		    (new Status(Status.IDB_ERROR, "attribute " + name +
				"is not completed"));
	    this.dataspace = cls.getDatabase().getDataspace(dspid);
	}


	if (this.dataspace != dataspace) {
	    dspid = (dataspace != null ? dataspace.getId() :
		     Root.DefaultDspid);
	}

	this.dataspace = dataspace;
    }

    Dataspace dataspace;
}



