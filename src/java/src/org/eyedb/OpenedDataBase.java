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

import java.util.*;

class OpenedDataBase {

  static Hashtable hash = new Hashtable(128);
  Connection conn;
  int dbid;
  int flags;
  String userauth, passwdauth;

  OpenedDataBase(Connection conn, int dbid, int flags,
		    String userauth, String passwdauth)
  {
    this.conn = conn;
    this.dbid = dbid;
    this.flags = flags;
    this.userauth = userauth;
    this.passwdauth = passwdauth;
  }

  static Database get(Connection conn, int dbid, int flags,
			 String userauth, String passwdauth)
  {
    return (Database)hash.get(new OpenedDataBase(conn, dbid,
						       flags, userauth,
						       passwdauth));
  }

  static void add(Database db) {
    hash.put(new OpenedDataBase(db.conn, db.dbid, db.flags, db.userauth,
				   db.passwdauth), db);
  }

  public boolean equals(java.lang.Object obj) {
    OpenedDataBase db = (OpenedDataBase)obj;
    return dbid == db.dbid &&
      flags == db.flags &&
      userauth.equals(db.userauth) &&
      passwdauth.equals(db.passwdauth);
  }

  public int hashCode() {
    return dbid;
  }
};
