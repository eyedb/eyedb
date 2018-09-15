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

public class DatabaseInfoDescription {

    public static final int MAX_DATAFILES = 512;
    public static final int MAX_DATASPACES = 512;
    public static final int L_NAME = 31;
    public static final int L_FILENAME = 256;
    public static final int MAX_DAT_PER_DSP = 32;

    public static class DatafileInfo {
	public String file; // char file[L_FILENAME];
	public String name; // char name[L_NAME+1];
	public long maxsize;
	public short mtype;
	public int sizeslot;
	public short dspid;
	public int extflags;
	public short dtype;
    };

    public static class DataspaceInfo {
	public String name; // char name[L_NAME+1];
	public int ndat;
	public short datid[]; // short datid[MAX_DAT_PER_DSP];
    };

    public String dbfile;
    public int dbid;
    public int nbobjs;
    public int ndat;
    public DatafileInfo dat[]; // Datafile dat[MAX_DATAFILES];
    public int ndsp;
    public DataspaceInfo dsp[]; // Dataspace dsp[MAX_DATASPACES];
    public long dbsfilesize;
    public long dbsfileblksize;
    public long ompfilesize;
    public long ompfileblksize;
    public long shmfilesize;
    public long shmfileblksize;

    public void trace(java.io.PrintStream out) {
	out.println("DatabaseInfoDescription {");
	out.println("\tdbfile: " + dbfile);
	out.println("\tdbid: " + dbid);
	out.println("\tnbobjs: " + nbobjs);
	out.println("\tndat: " + ndat);

	out.println("\tdatafiles {");
	for (int n = 0; n < ndat; n++) {
	    DatafileInfo d = dat[n];
	    out.println("\t\tdatafiles[" + n + "] {");
	    out.println("\t\t\tfile: " + d.file);
	    out.println("\t\t\tname: " + d.name);
	    out.println("\t\t\tmaxsize: " + d.maxsize);
	    out.println("\t\t\tmtype: " + d.mtype);
	    out.println("\t\t\tsizeslot: " + d.sizeslot);
	    out.println("\t\t\tdspid: " + d.dspid);
	    out.println("\t\t\textflags: " + d.extflags);
	    out.println("\t\t\tdtype: " + d.dtype);
	    out.println("\t\t}");
	}

	out.println("\t}");

	out.println("\tndsp: " + ndsp);
	out.println("\tdataspaces {");
	for (int n = 0; n < ndsp; n++) {
	    DataspaceInfo d = dsp[n];
	    out.println("\t\tdataspaces[" + n + "] {");
	    out.println("\t\t\tname: " + d.name);
	    out.println("\t\t\tndat: " + d.ndat);
	    out.print("\t\t\tdatid: [");
	    for (int j = 0; j < d.ndat; j++)
		out.print((j > 0 ? ", " : "") + d.datid[j]);
	    out.println("]");
	    out.println("\t\t}");
	}

	out.println("\t}");

	out.println("\tdbsfilesize: " + dbsfilesize);
	out.println("\tdbsfileblksize: " + dbsfileblksize);
	out.println("\timpfilesize: " + ompfilesize);
	out.println("\timpfileblksize: " + ompfileblksize);
	out.println("\tshmfilesize: " + shmfilesize);
	out.println("\tshmfileblksize: " + shmfileblksize);
	out.println("}");
    }
}
