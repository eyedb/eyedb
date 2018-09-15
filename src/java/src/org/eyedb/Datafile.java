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

public class Datafile {

    public static final int BITMAP_MTYPE = 0;
    public static final int LINKMAP_MTYPE = 1;
    public static final int DEFAULTMAP_MTYPE = BITMAP_MTYPE;

    // from eyedbsm::DatType
    public static final int LOGICAL_OID_DTYPE = 0x100;
    public static final int PHYSICAL_OID_DTYPE = 0x101;

    private Database db;
    private short id;
    private short dspid;
    private Dataspace dataspace;
    private String file;
    private String name;

    // from eyedbsm::MapType
    // MapType
    private int mtype;

    // DatType
    private int dtype;

    private long maxsize;
    private int slotsize;

    public Datafile(Database db, short id, short dspid, String file,
		    String name, long maxsize, int mtype, int slotsize,
		    int dtype) {
	this.db = db;
	this.id = id;
	this.dspid = dspid;
	this.file = file;
	this.name = name;
	this.dtype = dtype;
	this.mtype = mtype;
	this.maxsize = maxsize;
	this.slotsize = slotsize;
    }

    public Database getDatabase() {return db;}

    public String getName() {return name;}
    public int getId() {return id;}
    public int getDspid() {return dspid;}
    public Dataspace getDataspace() {return dataspace;}
    public String getFile() {return file;}

    public int getMaptype() {return mtype;}
    public int getDatType() {return dtype;}
    public boolean isPhysical() {return dtype == PHYSICAL_OID_DTYPE;}
    public boolean isValid() {return file.length() > 0;}

    public long getMaxsize() {return maxsize;}
    public int getSlotsize() {return slotsize;}

    void setDataspace(Dataspace dataspace) {
	this.dataspace = dataspace;
    }

    public String toString() {
	String indent = "  ";

	return "Datafile #" + id + "\n" +
	    indent + "file: " + file  + "\n" +
	    indent + "name: " + name  + "\n" +
	    indent + "maxsize: " + maxsize  + "\n" +
	    indent + "maptype: " + (mtype == BITMAP_MTYPE ? "Bitmap" :
				    "Linkmap")  + "\n" +
	    indent + "sizeslot: " + slotsize  + "\n" +
	    indent + "dspid: " + dspid  + "\n" +
	    indent + "dtype: " + (isPhysical() ? "Physical" : "Logical");
    }
}
