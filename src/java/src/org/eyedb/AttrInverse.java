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

public class AttrInverse {

    Oid class_oid;
    short num;
    Attribute attr;
    String class_name;
    String attr_name;
    Database db;

    AttrInverse(Database db, Oid class_oid, short num, Attribute attr) throws Exception {
	this.db = db;
	this.class_oid = class_oid;
	this.num = num;
	this.attr = attr;
    }

    public Oid getClassOid() {return class_oid;}
    public Attribute getAttribute() {return attr;}
    public String getClassName() {return class_name;}
    public String getAttrName() {return attr_name;}
    public short getNum() {return num;}

    boolean isComplete() {
	if (!class_oid.isValid())
	    return true;
	return class_name != null && attr_name != null && attr != null;
    }

    void complete() throws Exception {
	if (db == null || isComplete()) return;
	Class cls = db.getSchema().getClass(class_oid);
	class_name = cls.getName();
	attr_name = cls.getAttributes()[num].getName();
    }

    public boolean isValid() {
	return class_oid.isValid();
    }
}
