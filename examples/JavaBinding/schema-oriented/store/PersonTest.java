
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
   Authors: 
   Eric Viara <viara@sysra.com>
   Francois Dechelle <francois@dechelle.net>
*/

import org.eyedb.*;
import person.*;

public class PersonTest {

    public static void main(String args[]) {

	String[] outargs = org.eyedb.Root.init("PersonTest", args);
	int n = outargs.length;

	if (n != 2) {
	    display_("usage: java PersonTest dbname prefix");
	    System.exit(1);
	}

	prefix = outargs[1];

	try {
	    person.Database.init();
	    display("Before Connection");
	    org.eyedb.Connection conn = new org.eyedb.Connection();
	    display("Connection Established");

	    person.Database db = new person.Database(outargs[0]);
	    display("Before Opening");
	    db.open(conn, org.eyedb.Database.DBRW, null, null);
	    display("After Opening");

	    db.transactionBegin();
	    test0(db);
	    db.transactionCommit();

	    display_("Starting");
	    db.transactionBegin();
	    test(db);
	    db.transactionCommit();

	    db.transactionBegin();
	    test2(db);
	    db.transactionCommit();

	    display_("Ending");
	}

	catch(org.eyedb.Exception e) {
	    e.print();
	}

	display_("Done");
    }

    static boolean displaying = true;
    static String prefix;

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
	if (displaying) {
	    if (o != null) o.trace();
	    else display("<null>");
	}
    }

    static Employee makeEmployee(org.eyedb.Database db, int i, String s) throws org.eyedb.Exception {
	return (Employee)makePerson(new Employee(db), i, s);
    }

    static Person makePerson(org.eyedb.Database db, int i, String s) throws org.eyedb.Exception {
	return makePerson(new Person(db), i, s);
    }

    static Person makePerson(Person p, int i, String s) throws org.eyedb.Exception {
	p.setFirstname(prefix + i + "_f_" + s);
	p.setLastname(prefix + i + "_l_" + s);
	p.setAge(i+20);
	Address addr = p.getAddr();
	addr.setStreet(prefix + i + "_s_" + s);
	addr.setTown(prefix + i + "_t_" + s);
	addr.setCountry(prefix + i + "_c_" + s);

	if (p.getChildrenCount() != 0) {
	    System.out.println("internal error for children count");
	    System.exit(1);
	}

	// trying user method
	p.checkName();

	return p;
    }

    static Car makeCar(org.eyedb.Database db, int i, String s) throws org.eyedb.Exception {
	Car c = new Car(db);
	c.setMark(prefix + i + "_f_" + s);
	c.setNum(i+100);
	return c;
    }

    static void test_simple(org.eyedb.Database db) throws org.eyedb.Exception {
	Employee p = makeEmployee(db, 0, "");
	p.store();
    }

    static void test0(org.eyedb.Database db) throws org.eyedb.Exception {
	long ms = System.currentTimeMillis();
	long ms2 = org.eyedb.RPClib.read_ms;
	Person ps[] = new Person[1000];
	for (int i = 0; i < ps.length; i++) {
	    ps[i] = makePerson(db, i, "#R");
	    ps[i].store();
	}
	long ms1 = System.currentTimeMillis();
	System.out.println("test0: ms " + (ms1 - ms) + " " +
			   (RPClib.read_ms - ms2));
    }

    static void test(org.eyedb.Database db) throws org.eyedb.Exception {
	Person ps[] = new Person[10];
	for (int i = 0; i < ps.length; i++)
	    ps[i] = makePerson(db, i, "#H");

	for (int i = 0; i < ps.length; i++) {
	    Person p = makePerson(db, i, "#W");
	    p.setSpouse(ps[i]);
	    for (int j = 0; j < 5; j++) {
		Person c = makePerson(db, j, "#C");
		p.setInChildrenCollAt(j, c);
		Car car = makeCar(db, j, "#C");
		p.addToCarsColl(car);
	    }
	    p.store(org.eyedb.RecMode.FullRecurs);
	}

	for (int i = 0; i < ps.length; i++)
	    ps[i] = makeEmployee(db, i, "#EH");

	for (int i = 0; i < ps.length; i++) {
	    Person p = makeEmployee(db, i, "#EW");
	    p.setSpouse(ps[i]);
	    for (int j = 0; j < 5; j++) {
		Person c = makePerson(db, j, "#EC");
		p.setInChildrenCollAt(j, c);
		Car car = makeCar(db, j, "#C");
		p.addToCarsColl(car);
	    }
	    p.store(org.eyedb.RecMode.FullRecurs);
	}

	// trying user methods
	Person p = new Person(db, "john", "wayne", 78);
	p.setFirstname("John");
	p.setLastname("Wayne");

	p.store();
    }

    static void trace(Car c) throws org.eyedb.Exception {
	System.out.println("\tmark: " + c.getMark());
    }

    static void trace(Person p, String indent) throws org.eyedb.Exception {
	if (p == null) {
	    System.out.println("<null>");
	    return;
	}

	System.out.println(indent + "first name: " + p.getFirstname() + "\n" +
			   indent + "last name: " + p.getLastname() + "\n" +
			   indent + "age: " + p.getAge() + "\n" +
			   indent + "addr.street: " + p.getAddr().getStreet() + "\n" +
			   indent + "addr.country: " + p.getAddr().getCountry() + "\n" +
			   indent + "addr.town: " + p.getAddr().getTown());
	int bottom = p.getChildrenColl().getBottom();
	int top = p.getChildrenColl().getTop();
	for (int j = bottom; j < top; j++) {
	    System.out.println("child #" + j + ": ");
	    trace(p.retrieveChildrenAt(j), indent + "\t");
	}

	for (int j = 0; j < p.getCarsCount(); j++) {
	    System.out.println("car #" + j + ": ");
	    trace(p.getCarsAt(j));
	}
    }

    static void test2(org.eyedb.Database db) throws org.eyedb.Exception {
	String str = "select Person.firstname ~ \"" + prefix + "\"";
	org.eyedb.OQL q = new org.eyedb.OQL(db, str);
	org.eyedb.ObjectArray obj_arr = new org.eyedb.ObjectArray();
	q.execute(obj_arr);
	System.out.println(str + " : count " + obj_arr.getCount());
	for (int i = 0; i < obj_arr.getCount(); i++) {
	    Person p = (Person)obj_arr.getObject(i);
	    trace(p, "");
	}			       

	q = new org.eyedb.OQL(db, str);
	org.eyedb.OidArray oid_arr = new org.eyedb.OidArray();
	q.execute(oid_arr);
	System.out.println(str + " : count " + oid_arr.getCount());
	for (int i = 0; i < oid_arr.getCount(); i++) {
	    System.out.println("oid #" + i + " " + oid_arr.getOid(i));
	}			       

	q = new org.eyedb.OQL(db, str);
	org.eyedb.ValueArray val_arr = new org.eyedb.ValueArray();
	q.execute(val_arr);
	System.out.println(str + " : count " + val_arr.getCount());
	for (int i = 0; i < val_arr.getCount(); i++) {
	    System.out.println("val #" + i + " " + val_arr.getValue(i).toString());
	}			       

    }
}
