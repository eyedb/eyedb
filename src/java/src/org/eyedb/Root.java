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

public class Root {

    public static String host;
    public static String port;
    public static String dbm;
    public static String user;
    public static String passwd;
    public static String progname;

    public static final int DefaultTRMode = 0;
    public static final int ReadWriteSharedTRMode = 0x10;
    public static final int WriteExclusiveTRMode = 0x20;
    public static final int ReadWriteExclusiveTRMode = 0x80;

    private static final String defHost = "localhost";
    private static final String defPort = "6240";
    private static final String defDbm  = "default";
    private static final String defUser = "";
    private static final String defPasswd = "";

    public static final int DefaultWRMode = 0;
    public static final int WriteOnCommit = 0x400;
    public static final int WriteImmediate = 0x800;

    public static final int ReturnStatus = 0;
    public static final int AbortTransaction = 1;

    public static final short DefaultDspid = 0x7fff;
    static final int COLL_LITERAL_VERSION = 20609;
    static final int MAGORDER_VERSION = 20609;

    private static void check(String x, String s) throws Exception
    {
	if (x == null)
	    throw new Exception("Exception",
				   progname + ": " + s + " is not set");
    }

    /*
    private static void checkConfig(String prefix, String suffix)
	throws Exception
    {
	check(host,   prefix + "host"   + suffix);
	check(port,   prefix + "port"   + suffix);
	check(user,   prefix + "user"   + suffix);
	check(passwd, prefix + "passwd" + suffix);
	check(dbm,    prefix + "dbm"    + suffix);
    }
    */

    static void init_p(String prg)
    {
	progname = prg;

	host = defHost;
	port = defPort;
	dbm = defDbm;
	user = defPasswd;
	passwd = defPasswd;

	try {
	    org.eyedb.syscls.Database.init();
	    org.eyedb.utils.Database.init();
	}
	catch(Exception e) {
	    System.err.println("Error in Root.init()");
	}
    }

    public static void init(String prg, DefaultConfig conf) {
	init_p(prg);

	if (conf.host != null)
	    host = conf.host;
	if (conf.port != null)
	    port = conf.port;
	if (conf.dbm != null)
	    dbm = conf.dbm;
	if (conf.user != null)
	    user = conf.user;
	if (conf.passwd != null)
	    passwd = conf.passwd;

	//checkConfig(prg + " DefaultConfig field `", "'");
    }

public static void init(String prg) {
	init_p(prg);
	//checkConfig(prg + " DefaultConfig `", "'");
    }

    public static void init(java.applet.Applet applet)
    {
	init("Applet");
	String s;

	if ((s = applet.getParameter("eyedbhost")) != null)
	    host = s;
	if ((s = applet.getParameter("eyedbport")) != null)
	    port = s;
	if ((s = applet.getParameter("eyedbdbm")) != null)
	    dbm = s;
	if ((s = applet.getParameter("eyedbuser")) != null)
	    user = s;
	if ((s = applet.getParameter("eyedbpasswd")) != null)
	    passwd = s;

	//checkConfig("Applet Parameter `eyedb", "'");
    }

    public static String [] init(String prg, String args_in[])
    {
	init_p("java " + prg);

	int i, j;

	String args_out[] = args_in;

	for (i = 0, j = 0; i < args_in.length; ) {
	    String s = args_in[i];

	    if (s == null) {
		i++;
		continue;
	    }

	    int idx = s.indexOf("=");
	    if (s.charAt(0) == '-' && s.charAt(1) == '-' &&
		(idx = s.indexOf("=")) > 0) {
		String opt = s.substring(0, idx);
		if (opt.equals("--host")) {
		    host = s.substring(idx+1);
		    i++;
		}
		else if (opt.equals("--port")) {
		    port = s.substring(idx+1);
		    i++;
		}
		else if (opt.equals("--dbm")) 	{
		    dbm = s.substring(idx+1);
		    i++;
		}
		else if (opt.equals("--user")) {
		    user = s.substring(idx+1);
		    i++;
		}
		else if (opt.equals("--passwd")) {
		    passwd = s.substring(idx+1);
		    i++;
		}
		else {
		    args_out[j++] = s;
		    i++;
		}
	    }
	    else {
		args_out[j++] = s;
		i++;
	    }
	}

	//checkConfig("Command line option `-eyedb", "'");

	if (j == args_out.length)
	return args_out;

	String [] args_xout = new String[j];

	for (i = 0 ; i < j; i++)
	args_xout[i] = args_out[i];

	return args_xout;
    }

    public static String getNullString() {return "NULL";}

    public static void printNull(java.io.PrintStream out) {
	out.print(getNullString());
    }
}
