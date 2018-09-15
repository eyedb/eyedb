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

class AttrNative extends Attribute {

  static Attribute[] make(Database db, int type) {

    Schema sch = ((db != null) ? db.getSchema() : Schema.bootstrap);

    switch(type)
      {
      case OBJECT:
	return makeObjectItems(sch);

      case CLASS:
	return makeClassItems(sch);
	
      case COLLECTION:
	return makeCollectionItems(sch);
	
      case COLLECTION_CLASS:
	return makeCollectionClassItems(sch);
	
      default:
	return null;
      }
  }

  public AttrNative(Class cls, Class class_owner,
			String name, int num, boolean isref) {
    super(cls, class_owner, name, num, isref);
  }

  public AttrNative(Class cls, Class class_owner,
			String name, int num, int dim) {
    super(cls, class_owner, name, num, dim);
  }

  public boolean isNative() {
    return true;
  }

  static Attribute[] makeObjectItems(Schema sch)
  {
    Attribute[] items = new Attribute[2];
    int n = 0;
    items[n++] = new AttrNative(sch.getClass("class"),
				    sch.getClass("object"),
				    "class", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("object"),
				    sch.getClass("object"),
				    "protection", 0, true);
						     
    return items;
  }

  static Attribute[] makeClassItems(Schema sch)
  {
    Class cls = sch.getClass("class");
    Attribute[] items = new Attribute[7];
    int n = 0;
    items[n++] = new AttrNative(cls,
				    sch.getClass("object"),
				    "class", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("object"),
				    sch.getClass("object"),
				    "protection", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("char"),
				    cls,
				    "mtype", 0, 16);
						     
    items[n++] = new AttrNative(sch.getClass("char"),
				    cls,
				    "name", 0, 128);
						     
    items[n++] = new AttrNative(cls,
				    cls,
				    "parent", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("set<object*>"),
				    cls,
				    "extent", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("set<object*>"),
				    cls,
				    "components", 0, true);
						     
    return items;
  }

  static Attribute[] makeCollectionItems(Schema sch)
  {
    Class collcls   = sch.getClass("collection");
    Class setobjcls = sch.getClass("set<object*>");
    Class int32cls  = sch.getClass("int32");
    Attribute[] items = new Attribute[8];

    int n = 0;

    items[n++] = new AttrNative(sch.getClass("class"),
				    sch.getClass("object"),
				    "class", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("object"),
				    sch.getClass("object"),
				    "protection", 0, true);
						     
    items[n++] = new AttrNative(sch.getClass("char"),
				    collcls,
				    "cname", 0, 128);
						     
    items[n++] = new AttrNative(int32cls,
				   collcls,
				   "count", 0, 0);
						     
    items[n++] = new AttrNative(setobjcls,
				   collcls,
				   "collclass", 0, true);
						     
    items[n++] = new AttrNative(int32cls,
				    collcls,
				    "isref", 0, 0);
						     
    items[n++] = new AttrNative(int32cls,
				    collcls,
				    "dim", 0, 0);
						     
    items[n++] = new AttrNative(int32cls,
				    collcls,
				    "magorder", 0, 0);
						     
    return items;
  }

  static Attribute[] makeCollectionClassItems(Schema sch)
  {
    return makeClassItems(sch);
  }

  static final int OBJECT = 0;
  static final int CLASS = 1;
  static final int COLLECTION = 2;
  static final int COLLECTION_CLASS = 3;
}

