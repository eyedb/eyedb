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

public class ObjectLocation {

    public static class Info {
	int size;
	int slot_start_num;
	int slot_end_num;
	int dat_start_pagenum;
	int dat_end_pagenum;
	int omp_start_pagenum;
	int omp_end_pagenum;
	int dmp_start_pagenum;
	int dmp_end_pagenum;
    };
    
    private Database db;
    private Oid oid;
    private short dspid, datid;
    private Info info;

    ObjectLocation(Database db, Oid oid, short dspid, short datid, Info info) {
	this.db = db;
	this.oid = oid;
	this.dspid = dspid;
	this.datid = datid;
	this.info = info;
    }
		   
    public Database getDatabase() {return db;}
    public Oid getOid() {return oid;}
    public short getDspid() {return dspid;}
    public short getDatid() {return datid;}
    public int getSize() {return info.size;}
    public Info getInfo() {return info;}

    public String toString() {
	String indent = "  ";

	return "Object Location " + oid + "\n" +
	    indent + "dspid: " + dspid + "\n" +
	    indent + "datid: " + datid + "\n" +
	    indent + "size: " + info.size + "\n" +
	    indent + "slot_start_num: " + info.slot_start_num + "\n" +
	    indent + "slot_end_num: " + info.slot_end_num + "\n" +
	    indent + "dat_start_pagenum: " + info.dat_start_pagenum + "\n" +
	    indent + "dat_end_pagenum: " + info.dat_end_pagenum + "\n" +
	    indent + "omp_start_pagenum: " + info.omp_start_pagenum + "\n" +
	    indent + "omp_end_pagenum: " + info.omp_end_pagenum + "\n" +
	    indent + "dmp_start_pagenum: " + info.dmp_start_pagenum + "\n" +
	    indent + "dmp_end_pagenum: " + info.dmp_end_pagenum;
    }
}
