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

public class Dataspace {

    Dataspace(Database db, short id, String name,
		 Datafile datafiles[]) {
	this.db = db;
	this.id = id;
	this.name = name;
	this.datafiles = datafiles;
    }

    public Database getDatabase() {return db;}

    public String getName() {return name;}
    public short getId() {return id;}
    public Datafile[] getDatafiles() {return datafiles;}

    Datafile getCurrentDatafile() {return cur_datafile;}
    void setCurrentDatafile(Datafile cur_datafile) {
	this.cur_datafile = cur_datafile;
    }

    private Database db;
    private short id;
    private String name;
    private Datafile datafiles[];
    private Datafile cur_datafile;

    static void notImpl(String msg) {
	System.err.println("Dataspace functionality: " + msg +
			   " is not implemented");
    }

    private String datidString() {
	String str = "";
	for (int j = 0; j < datafiles.length; j++)
	    str += (j > 0 ? ", " : "") + datafiles[j].getId();
	return str;
    }

    public String toString() {
	String indent = "  ";
	return "Dataspace #" + id + "\n" +
	    indent + "name: " + name  + "\n" +
	    indent + "ndat: " + datafiles.length  + "\n" +
	    indent + "datid: " + datidString();
    }
}

