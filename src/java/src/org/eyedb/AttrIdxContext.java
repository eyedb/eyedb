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

class AttrIdxContext {

  AttrIdxContext() {
    init();
  }

  AttrIdxContext(Class class_owner) {
    init();
    set(class_owner, (Attribute)null);
  }

  AttrIdxContext(Class class_owner, Attribute attr) {
    init();
    set(class_owner, attr);
  }

  AttrIdxContext(Class class_owner, String attrname) {
    init();
    set(class_owner, attrname);
  }

  AttrIdxContext(Class class_owner, Attribute attrs[]) {
    init();
    if (attrs.length == 0)
      return;
    set(class_owner, attrs[0]);
    for (int i = 1; i < attr_cnt; i++)
      push(attrs[i]);
  }

  void set(Class class_owner, Attribute attr) {
    this.class_owner = class_owner.getName();

    attr_cnt = 0;
    if (attr != null)
      attrs[attr_cnt++] = attr.getName();
  }

  void set(Class class_owner, String attrname) {
    this.class_owner = class_owner.getName();

    attr_cnt = 0;
    if (attrname != null)
      attrs[attr_cnt++] = attrname;
  }

  void push(Database db, Oid cloid, Attribute attr) throws Exception {
    if (class_owner == null)
      {
	set(db.getSchema().getClass(cloid), attr);
	return;
      }

    push(attr);
  }

  void push(Class class_owner, Attribute attr) {
    if (this.class_owner == null)
      {
	set(class_owner, attr);
	return;
      }
    push(attr);
  }

  void push(Attribute attr) {
    // warning possible array out of bounds!!!
    attrs[attr_cnt++] = attr.getName();
  }

  void push(String attrname) {
    attrs[attr_cnt++] = attrname;
  }

  void pop() {
    --attr_cnt;
  }

  int getLevel() {return attr_cnt;}

  byte [] code() {
    Coder coder = new Coder();
    coder.code(class_owner != null ? class_owner : "");
    coder.code(magorder);
    coder.code(attr_cnt);

    for (int i = 0; i < attr_cnt; i++)
      coder.code(attrs[i]);

    return coder.getData();
  }

  String getString() {
    if (class_owner == null)
      return "null context";

    String s = class_owner;
    for (int i = 0; i < attr_cnt; i++)
      s += "." + attrs[i];

    return s;
  }

  int getMagOrder() {return magorder;}
  String getClassOwner() {return class_owner;}

  private void init() {
    attr_cnt = 0;
    class_owner = null;
    attrs = new String[64];
  }

  private String class_owner;
  private String attrs[];
  private short attr_cnt;
  private int magorder;
}

