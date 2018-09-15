
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

import org.eyedb.*;

class Collections {

  public static void main(String args[]) {

    String[] outargs = org.eyedb.Root.init("Collections", args);
    int n = outargs.length;

    if (n != 2) {
        display_("usage: java Collections dbname prefix");
	System.exit(1);
      }

    prefix = outargs[1];

    try {
      display("Before Connection");
      org.eyedb.Connection conn = new org.eyedb.Connection();
      display("Connection Established");

      org.eyedb.Database db = new org.eyedb.Database(outargs[0]);
      display("Before Opening");
      db.open(conn, org.eyedb.Database.DBRW, null, null);
      display("After Opening");

      display_("Starting");
      db.transactionBegin();
      init_person_class(db);

      collset_test(db);
      collbag_test(db);
      collarray_test(db);

      db.transactionCommit();
      display_("Ending");
    }

    catch(org.eyedb.Exception e) {
      e.print();
      System.exit( 1);
    }

    display_("Done");
  }

  static org.eyedb.Class person_cls;
  static org.eyedb.Attribute name_attr;
  static org.eyedb.Attribute age_attr;
  static org.eyedb.Attribute children_attr;
  static String prefix;
  static boolean displaying = true;
  static final int ITEM_COUNT = 10;

  static void display_(String s) {
      display(s, true);
  }

  static void display(String s, boolean force) {
      if (displaying || force) System.out.println("#### " + s);
  }

  static void display(String s) {
      display(s, false);
  }

  static void display(org.eyedb.Object o) throws org.eyedb.Exception {
      if (displaying) o.trace();
  }

  public static void init_person_class(org.eyedb.Database db) {
    if (person_cls != null)
      return;

    person_cls = db.getSchema().getClass("Person");
    name_attr = person_cls.getAttribute("name");
    if (name_attr == null)
      {
	display_("Attribute name not found");
	System.exit(1);
      }

    age_attr = person_cls.getAttribute("age");
    if (age_attr == null)
      {
	display_("Attribute age not found");
	System.exit(1);
      }
  }

  private static void dump(org.eyedb.CollArray coll, String msg, int count) throws org.eyedb.Exception {
      int n = coll.getCount();
      display("Collection Array count " + n + " [" + coll.getBottom() +
	      ", " + coll.getTop() + " [" + msg + "]");
      if (count != n) {
	  display_("Count error: " + count + " expected");
	  System.exit(1);
      }

      int bottom = coll.getBottom();
      int top = coll.getTop();
      for (int i = bottom; i < top; i++) {
	  org.eyedb.Object o = coll.retrieveObjectAt(i);
	  display("Object at #" + i + ": " + (o == null ? "<null>" : o.toString()));
      }

      for (int i = bottom; i < top; i++) {
	  org.eyedb.Oid o = coll.retrieveOidAt(i);
	  display("Oid at #" + i + ": " +
		  (o == null ? "<null>" : o.toString()));
      }

      for (int i = bottom; i < top; i++) {
	  org.eyedb.Value v = coll.retrieveValueAt(i);
	  display("Value at #" + i + ": " + (v == null ? "<null>" : v.toString()));
      }
  }

  private static void dump(org.eyedb.Collection coll, String msg, int count) throws org.eyedb.Exception {
      int n = coll.getCount();
      display("Collection count " + n + " [" + msg + "]");
      if (count != n) {
	  display_("Count error: " + count + " expected");
	  System.exit(1);
      }

      for (int i = 0; i < n; i++) {
	org.eyedb.Object o = coll.getObjectAt(i);
	display("Object at #" + i + ": ");
	display(o);
      }

      for (int i = 0; i < n; i++) {
	org.eyedb.Oid o = coll.getOidAt(i);
	display("Oid at #" + i + ": " + o);
      }

      for (int i = 0; i < n; i++) {
	org.eyedb.Value v = coll.getValueAt(i);
	display("Value at #" + i + ": " + v);
      }
  }

  private static void suppress_objects(org.eyedb.CollArray coll, int where[]) throws org.eyedb.Exception {
      int n = coll.getCount();
      display("Suppressing " + where.length + " Objects from Collection Array count " + n);

      for (int i = 0; i < where.length; i++) {
	  display("Suppress Object at #" + where[i]);
	  coll.suppressAt(where[i]);
      }
  }

  private static void suppress_objects(org.eyedb.Collection coll, int count) throws org.eyedb.Exception {
      int n = coll.getCount();
      display("Suppressing Objects " + count + " from Collection count " + n);

      for (int i = 0; i < count; i++) {
	  org.eyedb.Object o = coll.getObjectAt(i);
	  display("Suppressing object " + o + ", " + o.getOid().toString());
	  coll.suppress(o);
      }
  }

  private static void suppress_oids(org.eyedb.Collection coll, int count) throws org.eyedb.Exception {
      int n = coll.getCount();
      display("Suppressing Oids " + count + " from Collection count " + n);

      for (int i = 0; i < count; i++) {
	  org.eyedb.Oid oid = coll.getOidAt(i);
	  display("Suppressing oid " + oid.toString());
	  coll.suppress(oid);
      }
  }

  private static void collset_test(org.eyedb.Database db) throws org.eyedb.Exception {
    display_("Testing a set");
    org.eyedb.CollSet set = new org.eyedb.CollSet(db, "set_" + prefix, person_cls);
    org.eyedb.Object p = null;

    int count = ITEM_COUNT;
    for (int i = 0; i < count; i++) {
	p = person_cls.newObj(db);
	String s = prefix + i;
	name_attr.setSize(p, s.length()+1);
	name_attr.setStringValue(p, s);
	display("Inserting a new person");
	set.insert(p);
    }
    
    org.eyedb.Object lastP = p;
    dump(set, "before store", count);

    boolean caught;
    try {
	caught = false;
	set.insert(p); // try to insert the same p!
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (!caught) {
	display_("ERROR: duplicate not caught");
	System.exit(1);
    }

    set.store(org.eyedb.RecMode.FullRecurs);
    org.eyedb.Oid set_oid = set.getOid();
    display("set " + set_oid);
    dump(set, "after store", count);
    p = person_cls.newObj(db);
    set.insert(p);
    count++;
    dump(set, "after insert", count);
    set.store(org.eyedb.RecMode.FullRecurs);
    dump(set, "after insert and store again", count);

    try {
	caught = false;
	set.insert(new org.eyedb.Oid()); // try to insert a null oid
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (!caught) {
	display_("ERROR: inserting null oid not caught");
	System.exit(1);
    }

    // loading it
    set = (org.eyedb.CollSet)db.loadObject(set_oid);
    dump(set, "after load", count);

    int sup_cnt = 3;
    suppress_objects(set, sup_cnt);
    count -= sup_cnt;

    dump(set, "after suppress objects", count);

    sup_cnt = 2;
    suppress_oids(set, sup_cnt);
    count -= sup_cnt;

    dump(set, "after suppress oids", count);
    set.store();
    dump(set, "after suppress oids and store", count);
  }

  private static void collbag_test(org.eyedb.Database db) throws org.eyedb.Exception {
    display_("Testing a bag");
    org.eyedb.CollBag bag = new org.eyedb.CollBag(db, "bag_" + prefix, person_cls);
    org.eyedb.Object p = null;

    int count = ITEM_COUNT;
    for (int i = 0; i < count; i++) {
	p = person_cls.newObj(db);
	String s = prefix + i;
	name_attr.setSize(p, s.length()+1);
	name_attr.setStringValue(p, s);
	display("Inserting a new person");
	bag.insert(p);
    }
    
    org.eyedb.Object lastP = p;
    dump(bag, "before store", count);

    boolean caught;
    try {
	caught = false;
	bag.insert(p); // try to insert the same p!
	count++;
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (caught) {
	display_("ERROR: duplicate caught");
	System.exit(1);
    }

    bag.store(org.eyedb.RecMode.FullRecurs);
    org.eyedb.Oid bag_oid = bag.getOid();
    display("bag " + bag_oid);

    dump(bag, "after store", count);
    p = person_cls.newObj(db);
    bag.insert(p);
    count++;
    dump(bag, "after insert", count);
    bag.store(org.eyedb.RecMode.FullRecurs);
    dump(bag, "after insert and store again", count);

    try {
	caught = false;
	bag.insert(new org.eyedb.Oid()); // try to insert a null oid
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (!caught) {
	display_("ERROR: inserting null oid not caught");
	System.exit(1);
    }

    // loading it
    bag = (org.eyedb.CollBag)db.loadObject(bag_oid);
    dump(bag, "after load", count);

    int sup_cnt = 3;
    suppress_objects(bag, sup_cnt);
    count -= sup_cnt;

    dump(bag, "after suppress objects", count);

    sup_cnt = 2;
    suppress_oids(bag, sup_cnt);
    count -= sup_cnt;

    dump(bag, "after suppress oids", count);
    bag.store();
    dump(bag, "after suppress oids and store", count);
  }

  private static void collarray_test(org.eyedb.Database db) throws org.eyedb.Exception {
    display_("Testing an array");
    org.eyedb.CollArray array = new org.eyedb.CollArray(db, "array_" + prefix, person_cls);
    org.eyedb.Object p = null;

    int count = ITEM_COUNT;
    for (int i = 0; i < count; i++) {
	p = person_cls.newObj(db);
	String s = prefix + i;
	name_attr.setSize(p, s.length()+1);
	name_attr.setStringValue(p, s);
	display("Inserting a new person");
	array.append(p);
    }
    
    org.eyedb.Object lastP = p;
    dump(array, "before store", count);

    boolean caught;
    try {
	caught = false;
	array.append(p); // try to insert the same p!
	count++;
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (caught) {
	display_("ERROR: duplicate caught");
	System.exit(1);
    }

    array.store(org.eyedb.RecMode.FullRecurs);
    org.eyedb.Oid array_oid = array.getOid();
    display("array " + array_oid);
    dump(array, "after store", count);
    p = person_cls.newObj(db);
    array.append(p);
    count++;
    dump(array, "after insert", count);
    array.store(org.eyedb.RecMode.FullRecurs);
    dump(array, "after insert and store again", count);

    try {
	caught = false;
	array.append(new org.eyedb.Oid()); // try to insert a null oid
    }
    catch(org.eyedb.Exception e) {
	caught = true;
    }

    if (!caught) {
	display_("ERROR: inserting null oid not caught");
	System.exit(1);
    }

    // loading it
    array = (org.eyedb.CollArray)db.loadObject(array_oid);
    dump(array, "after load", count);

    int sup_cnt = 3;
    int where[] = new int[sup_cnt];
    where[0] = 0;
    where[1] = 2;
    where[2] = 3;
    suppress_objects(array, where);
    count -= sup_cnt;

    dump(array, "after suppress objects", count);

    sup_cnt = 2;
    where = new int[sup_cnt];
    where[0] = 4;
    where[1] = 8;
    suppress_objects(array, where);
    count -= sup_cnt;

    dump(array, "after suppress oids", count);
    array.store();
    dump(array, "after suppress oids and store", count);
  }
}
