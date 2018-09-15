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

public class DefaultConfig {

    public static String host;
    public static String port;
    public static String user;
    public static String passwd;
    public static String dbm;

    public DefaultConfig() {
	host = null;
	port = null;
	user = null;
	passwd = null;
	dbm = null;
    }

    public DefaultConfig(String host, String port, String user,
			    String passwd, String dbm) {
	this.host = host;
	this.port = port;
	this.user = user;
	this.passwd = passwd;
	this.dbm = dbm;
    }
}
