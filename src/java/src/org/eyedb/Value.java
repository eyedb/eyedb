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

//
// Value class
//

package org.eyedb;

public class Value {

  int     type;

  boolean   b   = false;
  byte      by  = 0;
  short     s   = 0;
  long      i   = 0;
  char      c   = 0;
  double    d   = 0.;
  String    str = null;
  Oid    oid = null;
  Object o   = null;
  byte[]    idr = null;
  ValueStruct  stru = null;
  java.util.LinkedList list = null;

  static public final int NIL = 0;
  static public final int NULL = 1;
  static public final int BOOL = 2;
  static public final int BYTE = 3;
  static public final int CHAR = 4;
  static public final int SHORT = 5;
  static public final int INT = 6;
  static public final int LONG = 7;
  static public final int DOUBLE = 8;
  static public final int IDENT = 9;
  static public final int STRING = 10;
  static public final int DATA = 11;
  static public final int OID = 12;
  static public final int OBJECT = 13;
  static public final int OBJECTPTR = 14;
  static public final int POBJ = 15;
  static public final int LIST = 16;
  static public final int SET = 17;
  static public final int ARRAY = 18;
  static public final int BAG = 19;
  static public final int STRUCT = 20;

  /*
   * Constructor Methods
   */

  public Value() {
    set();
  }

  public Value(byte by) {
    set(by);
  }

  public Value(boolean b) {
    set(b);
  }

  public Value(int i) {
    set(i);
  }

  public Value(long i) {
    set(i);
  }

  public Value(short s) {
    set(s);
  }

  public Value(char c) {
    set(c);
  }

  public Value(double d) {
    set(d);
  }

  public Value(String str) {
    set(str);
  }

  public Value(String str, boolean isident) {
    set(str, isident);
  }

  public Value(Oid oid) {
    set(oid);
  }

  public Value(Object o) {
    set(o);
  }

  public Value(byte[] idr) {
    set(idr);
  }

  public Value(ValueStruct stru) {
    set(stru);
  }

  /*
   * Modifier Methods
   */

  public void set() {
    type = NIL;
  }

  public void setToNil() {
    type = NIL;
  }

  public void setToNull() {
    type = NULL;
  }

  public void set(boolean b) {
    type = BOOL;
    this.b = b;
  }

  public void set(byte by) {
    type = BYTE;
    this.by = by;
  }

  public void set(char c) {
    type = CHAR;
    this.c = c;
  }

  public void set(short s) {
    type = SHORT;
    this.s = s;
  }

  public void set(int i) {
    type = INT;
    this.i = (long)i;
  }

  public void set(long i) {
    type = LONG;
    this.i = i;
  }

  public void set(double d) {
    type = DOUBLE;
    this.d = d;
  }

  public void set(String str, boolean isident) {
    type = (isident ? IDENT : STRING);
    this.str = str;
  }

  public void set(String str) {
    type = STRING;
    this.str = str;
  }

  public void set(byte[] idr) {
    type = DATA;
    this.idr = idr;
  }

  public void set(Oid oid) {
    type = OID;
    this.oid = oid;
  }

  public void set(Object o) {
    type = OBJECT;
    this.o = o;
  }

  public void set(ValueStruct stru) {
    type = STRUCT;
    this.stru = stru;
  }

  public void set(java.util.LinkedList list, int type)
    throws Exception {
    if (type != LIST && type != SET && type != ARRAY && type != BAG)
      throw new InvalidValueTypeException(new Status(Status.IDB_ERROR, "invalid value type: expected LIST, SET, ARRAY or BAG"));

    this.type = type;
    this.list = list;
  }

  public void set(Value v) throws Exception {
    type = v.type;

    if (type == BOOL)
      this.b = v.b;
    else if (type == BYTE)
      this.by = v.by;
    else if (type == SHORT)
      this.s = v.s;
    else if (type == LONG || type == INT)
      this.i = v.i;
    else if (type == CHAR)
      this.c = v.c;
    else if (type == DOUBLE)
      this.d = v.d;
    else if (type == STRING)
      this.str = v.str;
    else if (type == OID)
      this.oid = v.oid;
    else if (type == OBJECT)
      this.o = v.o;
    else if (type == IDENT)
      this.str = v.str;
    else if (type == DATA)
      this.idr = v.idr;
    else if (type == BAG || type == LIST || type == ARRAY || type == SET)
      this.list = v.list;
    else if (type == STRUCT)
      this.stru = v.stru;
    else
      throw new InvalidValueTypeException(getUnexpectedType(type));
  }

  /*
   * Accessor Methods
   */

  public int getType() {return type;}

  public boolean sgetBoolean() {
    return b;
  }

  public byte sgetByte() {
    return by;
  }

  public char sgetChar() {
    return c;
  }

  public short sgetShort() {
    return s;
  }

  public int sgetInt() {
    return (int)i;
  }

  public long sgetLong() {
    return i;
  }

  public double sgetDouble() {
    return d;
  }

  public String sgetString() {
    return str;
  }

  public Oid sgetOid() {
    return oid;
  }

  public Oid sgetObjOid() {
      if (type == OID)
	  return oid;

      if (o == null)
	  return Oid.nullOid;

      return o.getOid();
  }

  public Object sgetObject() {
    return o;
  }

  public byte[] sgetData() {
    return idr;
  }

  public boolean getBoolean() throws Exception {
    if (type != BOOL && type != NULL)
      throw new InvalidValueTypeException(getStatus(BOOL, type));
						  
    return b;
  }

  public byte getByte() throws Exception {
    if (type != BYTE && type != NULL)
      throw new InvalidValueTypeException(getStatus(BYTE, type));
						  
    return by;
  }

  public char getChar() throws Exception {
    if (type != CHAR && type != NULL)
      throw new InvalidValueTypeException(getStatus(CHAR, type));
						  
    return c;
  }

  public short getShort() throws Exception {
    if (type != SHORT && type != NULL)
      throw new InvalidValueTypeException(getStatus(SHORT, type));
						  
    return s;
  }

  public int getInt() throws Exception {
    if (type != INT && type != NULL)
      throw new InvalidValueTypeException(getStatus(INT, type));
						  
    return (int)i;
  }

  public long getLong() throws Exception {
    if (type != LONG && type != NULL)
      throw new InvalidValueTypeException(getStatus(LONG, type));
						  
    return i;
  }

  public double getDouble() throws Exception {
    if (type != DOUBLE && type != NULL)
      throw new InvalidValueTypeException(getStatus(DOUBLE, type));

    return d;
  }

  public String getString () throws Exception {
    if (type != STRING && type != NULL)
      throw new InvalidValueTypeException(getStatus(STRING, type));

    return str;
  }

  public Oid getObjOid() throws Exception {
    if (type != OID && type != OBJECT && type != NULL)
      throw new InvalidValueTypeException(getStatus(OID, type));
						  
    return sgetObjOid();
  }

  public Oid getOid() throws Exception {
    if (type != OID && type != NULL)
      throw new InvalidValueTypeException(getStatus(OID, type));
						  
    return oid;
  }

  public Object getObject() throws Exception {
    if (type != OBJECT && type != NULL)
      throw new InvalidValueTypeException(getStatus(OBJECT, type));
						  
    return o;
  }

  public byte[] getData() throws Exception {
    if (type != DATA && type != NULL)
      throw new InvalidValueTypeException(getStatus(DATA, type));
						  
    return idr;
  }

  public void print() throws Exception {
    System.out.print(toString());
  }

  public String toString() {

   try {
      switch(type)
	{
	case NULL:
	  return Root.getNullString();
	  
	case INT:
	  return new Integer((int)i).toString();
	  
	case LONG:
	  return new Long(i).toString();
	  
	case BYTE:
	  return new java.lang.Byte(by).toString();
	  
	case SHORT:
	  return new Short(s).toString();
	  
	case OID:
	  return oid.toString();
	  
	case BOOL:
	  return b ? "true" : "false";
	  
	case CHAR:
	  return "'" + c + "'";
	  
	case STRING:
	  return "\"" + str + "\"";
	  
	case IDENT:
	  return str;
	  
	case DOUBLE:
	  return new Double(i).toString();
	  
	case OBJECT:
	  if (o == null)
	    return Root.getNullString();
	  else
	    return o.toString();
	  
	case BAG:
	case LIST:
	case ARRAY:
	case SET:
	  {
	    String s = getTypeString(type).toLowerCase();
	    s += "(";
	    java.util.ListIterator iter = list.listIterator(0);
	    for (int n = 0; iter.hasNext(); n++)
	      {
		if (n != 0) s += ", ";
		s += ((Value)iter.next()).toString();
	      }	    

	    s += ")";
	    return s;
	  }
	
	case STRUCT:
	  return stru.toString();

	default:
	  throw new InvalidValueTypeException(getUnexpectedType(type));
	}
   } catch(Exception e) {
       e.print();
       return "";
   }
  }

  void code(Coder coder) throws Exception {

    coder.code((char)type);
    switch (type)
      {
      case NIL:
      case NULL:
	break;

      case INT:
	coder.code(i);
	break;

      case CHAR:
	coder.code(c);
	break;

      case BYTE:
	coder.code(by);
	break;

      case OID:
	coder.code(oid);
	break;

      case STRING:
      case IDENT:
	coder.code(str);
	break;

      case DOUBLE:
	coder.code(d);
	break;

      case LIST:
      case SET:
      case BAG:
      case ARRAY:
	{
	  int cnt = list.size();
	  coder.code(cnt);
	  java.util.ListIterator iter = list.listIterator(0);
	  while (iter.hasNext())
	    {
	      Value v = (Value)iter.next();
	      v.code(coder);
	    }
	}
      break;

      case STRUCT:
	coder.code(stru.attr_cnt);
	for (int ii = 0; ii < stru.attr_cnt; i++)
	  {
	    coder.code(stru.attrs[ii].name);
	    stru.attrs[ii].value.code(coder);
	  }
	break;

      default:
	throw new InvalidValueTypeException(getUnexpectedType(type));
      }
  }

  byte [] getData(int item_size) throws Exception {
    Coder coder = new Coder();

    code(coder);

    if (item_size != coder.data.length)
      {
	byte[] b = new byte[item_size];
	System.arraycopy(coder.data, 0, b, 0, item_size);
	return b;
      }

    return coder.data;
  }

  /*
   * package decode Methods
   */

  void decode(Coder coder) throws Exception {
    type = (int)coder.decodeChar();

    switch (type)
      {
      case NULL:
      case NIL:
	break;

      case INT:
      case LONG:
	i = coder.decodeLong();
	break;

      case CHAR:
	c = coder.decodeChar();
	break;

      case BOOL:
	b = coder.decodeBoolean();
	break;
	
      case OID:
	oid = coder.decodeOid();
	break;

      case STRING:
      case IDENT:
	str = coder.decodeString();
	break;
	  
      case DOUBLE:
	d = coder.decodeDouble();
	break;

      case LIST:
      case SET:
      case BAG:
      case ARRAY:
	{
	  int cnt = coder.decodeInt();
	  list = new java.util.LinkedList();
	  for (int ii = 0; ii < cnt; ii++)
	    {
	      Value v = new Value();
	      v.decode(coder);
	      list.add(v);
	    }
	}
      break;

      case STRUCT:
	int cnt = coder.decodeInt();
	stru = new ValueStruct(cnt);
	for (int ii = 0; ii < stru.attr_cnt; ii++)
	  {
	    String name = coder.decodeString();
	    Value value = new Value();
	    value.decode(coder);
	    stru.attrs[ii] = new ValueStructAttr(name, value);
	  }

	break;

      default:
	throw new InvalidValueTypeException(getUnexpectedType(type));
      }
  }

  static String getTypeString(int type) throws Exception {

    switch(type)
      {
      case NULL:
	return "NULL";

      case INT:
	return "INT";
	
      case OID:
	return "OID";
	
      case BOOL:
	return "BOOL";
	
      case CHAR:
	return "CHAR";
	
      case STRING:
	return "STRING";

      case IDENT:
	return "IDENT";

      case DOUBLE:
	return "DOUBLE";

      case OBJECT:
	return "OBJECT";

      case BAG:
	return "BAG";

      case SET:
	return "SET";

      case LIST:
	return "LIST";

      case ARRAY:
	return "ARRAY";

      case STRUCT:
	return "STRUCT";

      default:
	throw new InvalidValueTypeException(getUnexpectedType(type));
      }
  }

  static Status getStatus(int expected_type, int got_type) 
       throws Exception
  {
    return new Status(Status.IDB_ERROR, "value type mismatch: expected " +
			 getTypeString(expected_type) + ", got " +
			 getTypeString(got_type));
  }

  static Status getUnexpectedType(int type) {
    return new Status(Status.IDB_ERROR, "unexpected type " + type);
  }

  public int hashCode() {

    switch(type)
      {
      case NULL:
	return 0;
	  
      case INT:
	return (int)i;
	  
      case LONG:
	return (int)i;
	  
      case BYTE:
	return by;
	
      case SHORT:
	return s;
	
      case OID:
	return oid.hashCode();
	
      case BOOL:
	return b ? 1 : 0;
	
      case CHAR:
	return (int)c;
	
      case STRING:
      case IDENT:
	return str.hashCode();
	
      case DOUBLE:
	return new Double(d).hashCode();
	
      case OBJECT:
	return o.hashCode();

      case BAG:
      case LIST:
      case ARRAY:
      case SET:
	return list.hashCode();

      case STRUCT:
	return stru.hashCode();
      }

    return 0;
  }

  public boolean equals(java.lang.Object obj) {
    if (!(obj instanceof Value))
      return false;

    Value a = (Value)obj;

    if (a.type != type)
      return false;

    switch(type)
      {
      case NULL:
	return true;
	  
      case INT:
	return a.i == i;
	  
      case BYTE:
	return a.by == by;
	
      case SHORT:
	return a.s == s;
	
      case OID:
	return a.oid.equals(oid);
	
      case BOOL:
	return a.b == b;
	
      case CHAR:
	return a.c == c;
	
      case STRING:
      case IDENT:
	return a.str.equals(str);
	
      case DOUBLE:
	return a.d == d;
	
      case OBJECT:
	return a.o == o;

      case LIST:
      case BAG:
      case SET:
      case ARRAY:
	return list.equals(a.list);

      case STRUCT:
	return stru.equals(a.stru);
      }

    return false;
  }

  public void toArray(Database db, ObjectArray obj_arr) throws Exception {
    toArray(db, obj_arr, RecMode.NoRecurs);
  }

  public void toArray(Database db, ObjectArray obj_arr,
		      RecMode rcm) throws Exception {
    java.util.LinkedList tlist = new java.util.LinkedList();
    toOidObjectArray(db, tlist, true, rcm);
    obj_arr.set(tlist);
  }

  public void toArray(OidArray oid_arr) throws Exception {
    java.util.LinkedList tlist = new java.util.LinkedList();
    toOidObjectArray(null, tlist, false, null);
    oid_arr.set(tlist);
  }

  public void toOidObjectArray(Database db, java.util.LinkedList tlist,
			       boolean isobj, RecMode rcm)
    throws Exception {

    if (type == OID) {
      if (isobj && db != null)
	tlist.add(db.loadObject(oid, rcm));
      else
	tlist.add(oid);
    }
    else if (type == OBJECT) {
      if (isobj)
	tlist.add(o);
      else if (o != null)
	tlist.add(o.getOid());
    }
    else if (type == LIST || type == BAG || type == SET || type == ARRAY) {
      java.util.ListIterator iter = list.listIterator(0);
      while (iter.hasNext())
	{
	  Value v = (Value)iter.next();
	  v.toOidObjectArray(db, tlist, isobj, rcm);
	}
    }
    else if (type == STRUCT) {
      for (int ii = 0; ii < stru.attr_cnt; ii++)
	stru.attrs[ii].value.toOidObjectArray(db, tlist, isobj, rcm);
    }
  }

  public void toArray(ValueArray val_arr) {
    java.util.LinkedList tlist = new java.util.LinkedList();
    toValueArray(tlist);

    Value values[] = new Value[tlist.size()];

    java.util.ListIterator iter = tlist.listIterator(0);

    for (int n = 0; iter.hasNext(); n++)
      values[n] = (Value)iter.next();

    val_arr.set(values);
  }

  private void toValueArray(java.util.LinkedList tlist) {

    if (type == LIST || type == BAG || type == SET || type == ARRAY)
      {
	java.util.ListIterator iter = list.listIterator(0);
	while (iter.hasNext())
	  {
	    Value v = (Value)iter.next();
	    v.toValueArray(tlist);
	  }
      }
    else if (type == STRUCT)
      {
	for (int ii = 0; ii < stru.attr_cnt; ii++)
	  stru.attrs[ii].value.toValueArray(tlist);
      }
    else
      tlist.add(this);
  }
}


