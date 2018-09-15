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

import java.io.*;
import java.net.*;

public class Connection {

    private Socket main_sock;
    OutputStream main_os;
    InputStream  main_is;
    private RandomAccessFile main_pipe;

    private Socket intr_sock;
    OutputStream intr_os;
    InputStream  intr_is;
    private RandomAccessFile intr_pipe;

    private Socket outband_sock;
    OutputStream outband_os;
    InputStream  outband_is;
    private RandomAccessFile outband_pipe;
    private static int WAIT = 0;

    private String host, port;

    static private final int rpc_NewConnection        = 0x76;
    static private final int rpc_AssociatedConnection = 0x77;
    static private final int msg_size = 12;
  
    static private boolean isNumber(String port) {
	int len = port.length();
	for (int i = 0; i < len; i++)
	    {
		char c = port.charAt(i);
		if (c < '0' || c > '9')
		    return false;
	    }

	return true;
    }

    public Connection() throws Exception {
	this(Root.host, Root.port);
    }

    public Connection(String host, String port) throws Exception {
	this.host = host;
	this.port = port;
	try {
	    int offset;

	    if (isNumber(port)) {
		main_sock = new Socket(host, Integer.parseInt(port));
		main_sock.setTcpNoDelay(true);
		main_os = new StandardOutputStream(main_sock.getOutputStream());
		main_is = new StandardInputStream(main_sock.getInputStream());

		intr_sock = new Socket(host, Integer.parseInt(port));
		intr_sock.setTcpNoDelay(true);
		intr_os = new StandardOutputStream(intr_sock.getOutputStream());
		intr_is = new StandardInputStream(intr_sock.getInputStream());

		outband_sock = new Socket(host, Integer.parseInt(port));
		outband_sock.setTcpNoDelay(true);
		outband_os = new StandardOutputStream(outband_sock.getOutputStream());
		outband_is = new StandardInputStream(outband_sock.getInputStream());
	    }
	    else {
		main_sock = null;

		main_pipe = new RandomAccessFile(port, "r");
		main_pipe.close();
		main_pipe = new RandomAccessFile(port, "rw");
		main_os = new NamedPipeOutputStream(main_pipe);
		main_is = new NamedPipeInputStream(main_pipe);

		intr_sock = null;
		intr_pipe = new RandomAccessFile(port, "rw");
		intr_os = new NamedPipeOutputStream(intr_pipe);
		intr_is = new NamedPipeInputStream(intr_pipe);
	  
		outband_sock = null;
		outband_pipe = new RandomAccessFile(port, "rw");
		outband_os = new NamedPipeOutputStream(outband_pipe);
		outband_is = new NamedPipeInputStream(outband_pipe);
	    }

	    Coder coder = new Coder();
	    coder.code(RPClib.rpc_MMMagic);
	    coder.code(rpc_NewConnection);
	    coder.code(0);

	    main_os.write(coder.getData(), 0, coder.getOffset());

	    byte in[] = new byte[msg_size];

	    RPClib.read(main_is, in, msg_size);
			 
	    coder = new Coder(in);
	    coder.setOffset(8);
	    int xid = coder.decodeInt();

	    coder.reset();

	    coder.code(RPClib.rpc_MMMagic);
	    coder.code(rpc_AssociatedConnection);
	    coder.code(xid);

	    intr_os.write(coder.getData(), 0, coder.getOffset());

	    RPClib.read(intr_is, in, msg_size);

	    // added 24/09/04
	    coder.reset();

	    coder.code(RPClib.rpc_MMMagic);
	    coder.code(rpc_AssociatedConnection);
	    coder.code(xid);

	    outband_os.write(coder.getData(), 0, coder.getOffset());

	    RPClib.read(outband_is, in, msg_size);

	    StringBuffer challenge = new StringBuffer();;

	    Status status = RPC.rpc_SET_CONN_INFO
		(this,
		 InetAddress.getLocalHost().getHostName(),
		 Root.user, Root.progname, 0,
		 Version.getVersionNumber(),
		 challenge);

	    if (!status.isSuccess())
		throw new Exception(status, "connection");

	    if (challenge.length() > 0) {
		String filename = "/tmp/" + challenge.substring( (new String(challenge)).lastIndexOf( ".")+1);
		File file = new File( filename);

		byte[] b = (new String( challenge)).getBytes();
		FileOutputStream out = new FileOutputStream( file);
		out.write( b);
		out.close();

		try {
		    Process p = Runtime.getRuntime().exec( "chmod 0664 " + filename);
		    p.waitFor();
		}
		catch( InterruptedException e) {
		    throw new Exception( "InterruptedException", e.getMessage());
		}

		status = RPC.rpc_check_auth( this, filename);

		if (!status.isSuccess())
		    throw new Exception(status, "connection");

		// truncate the file to 0
		out = new FileOutputStream( file);
		out.write( b, 0, 0);
		out.close();
		file.delete();

		if (WAIT > 0) {
		    try {
			System.out.println("Waiting for " + WAIT +
					   " seconds");
			Thread.sleep(WAIT * 1000);
		    }
		    catch(java.lang.Exception e) {
		    }
		}
	    
	    }
	}
	catch(IOException e) {
	    e.printStackTrace();
	    throw new Exception
		(Status.IDB_CONNECTION_FAILURE, "connection failure",
		 "Cannot connect to eyedb server: host " + host +
		 ", port " + port);
	}
    }

    public String getHost()
    {
	return host;
    }

    public String getPort()
    {
	return port;
    }

    public static void connectionWait(int wait) {
	WAIT = wait;
    }

    public void close()
    {
	try {
	    if (main_sock != null)
		main_sock.close();
	    if (main_pipe != null)
		main_pipe.close();

	    if (intr_sock != null)
		intr_sock.close();
	    if (intr_pipe != null)
		intr_pipe.close();

	    if (outband_sock != null)
		outband_sock.close();
	    if (outband_pipe != null)
		outband_pipe.close();
	}
	catch(IOException e) {
	}
    }
}
