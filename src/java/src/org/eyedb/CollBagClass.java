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


public class CollBagClass extends CollectionClass {

  public CollBagClass(Database db, String name, Oid coll_mcloid,
		  boolean isref) {
    super(db, name, coll_mcloid, isref);
    type = ObjectHeader._CollBagClass_Type;
  }

  public CollBagClass(Database db, String name, Oid coll_mcloid,
		  int dim) {
    super(db, name, coll_mcloid, dim);
    type = ObjectHeader._CollBagClass_Type;
  }

  public CollBagClass(Class coll_cls, boolean isref) {
    super("bag", coll_cls, isref);
    type = ObjectHeader._CollBagClass_Type;
  }

  public static Class idbclass;

  static Class make(Database db,
		       Class coll_cls,
		       boolean isref, short dim)
  {
    String name = makeName("bag", coll_cls, isref, dim);

    Class cls;

    if ((cls = db.getSchema().getClass(name)) != null)
      return cls;

    if (dim > 1)
      return new CollBagClass(db, name, coll_cls.getOid(), dim);

    return new CollBagClass(db, name, coll_cls.getOid(), isref);
  }

  public Object newObj(Database db, Coder coder) throws Exception {
      CollBag o;

      if (isref)
	o = new CollBag(db, "", coll_cls, true);
      else
	o = new CollBag(db, "", coll_cls, dim);

      o.make(coder, this, ObjectHeader._CollBag_Type);
      o.is_literal = true;
      return o;
  }

  public String getTypename() {return "bag";}

    void completeClassAndParent(Database db) {
	if (cls == null)
	    cls = db.getSchema().getClass("bag_class");
	if (parent == null)
	    parent = db.getSchema().getClass("bag");
    }
}
