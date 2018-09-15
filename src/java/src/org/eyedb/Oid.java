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

public class Oid {
    int nx;
    int dbid;
    int unique;

    public final static Oid nullOid = new Oid();

    private void init() {
	nx     = 0;
	dbid   = 0;
	unique = 0;
    }

    public Oid() {
	init();
    }

    public Oid(int nx, int dbid, int unique) {
	this.nx     = nx;
	this.dbid   = dbid;
	this.unique = unique;
    }

    public Oid(String s) {
	init();

	int len = s.length();
	char sc[] = new char[len];

	s.getChars(0, len, sc, 0);

	int i, j, which;

	for (i = 0, j = 0, which = 0; i < len; i++)     {
	    char c = sc[i];

	    if (c != '.' && c != ':')
		continue;

	    String x = new String(sc, j, i-j);
	    j = i+1;
	    if (which == 0)
		nx = Integer.parseInt(x);
	    else if (which == 1)
		dbid = Integer.parseInt(x);
	    else if (which == 2)
		unique = Integer.parseInt(x);
	    else
		nx = dbid = unique = 0;

	    if (c == ':')
		break;

	    which++;
	}
    }

    public int getNx()     {return nx;}
    public int getDbid()   {return dbid;}
    public int getUnique() {return unique;}

    public void print() {
	System.out.print(getString());
    }

    public String getString() {
	if (isValid())
	    return nx + "." + dbid + "." + unique + ":oid";
	return Root.getNullString();
    }

    public String toString() {
	return getString();
    }

    public boolean isValid() {
	return (nx == 0 || dbid == 0 || unique == 0) ? false : true;
    }

    public boolean equals(java.lang.Object obj) {
	if (!(obj instanceof Oid))
	    return false;
	Oid oid = (Oid)obj;
	return (nx == oid.nx && dbid == oid.dbid && unique == oid.unique);
    }

    public int hashCode() {
	return nx;
    }
}
