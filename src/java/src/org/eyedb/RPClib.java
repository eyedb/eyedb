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

public class RPClib {

  static private final int rpc_SyncData  = 13;
  static private final int rpc_ASyncData = 18;
  static private final int HEAD_SIZE = 4*6;
  static final int rpc_Magic   = 0x43f2e341;
  static final int rpc_MMMagic = 0x43f2e341 + 0x11111111;
  static final int COMM_SZ = 2048;
  static final int RPC_MIN_SIZE = 128;
  static final long ANY = 0x10000;

  static private int serial = 1000;

  static private Coder coder_out = new Coder();
  static private Coder coder_in  = new Coder(new byte[RPC_MIN_SIZE]);
  static private CoderData coder_data[] = new CoderData[8];
  static private int coder_data_cnt;
  static private Connection xconn;
  static private byte null_bytes[];

    public static boolean TRACE = false;

  static {
    null_bytes = new byte[RPC_MIN_SIZE];
  }

  //
  // start method
  //

protected static void start(int code) {
  //System.out.println("code : " + Integer.toHexString(code));
  coder_data_cnt = 0;
  coder_out.reset();
  coder_in.reset();

  addArg(rpc_Magic);
  addArg(++serial);
  addArg(code);

  addArg(0);
  addArg(0);
  addArg(0);

  //System.out.println("RPC start -> " + code);
}

  //
  // utility methods
  //

  static int read(InputStream is, byte b[], int len) {
      rpc_read_cnt++;
    try {
      int n = 0;

      //System.out.println("Reading : " + len);
      while (n < len) {
	int p = is.read(b, n, len - n);
	if (p < 0)
	  return p;
	if (p == 0)
	  break;
	n += p;
      }
      
      //System.out.println("returns " + n);
      return n;
    }

    catch(IOException e) {
      System.err.println(e);
      return 0;
    }
  }
		     
  static boolean is_syncdata(int len) {
    return ((coder_out.offset + len) < COMM_SZ - 96);
  }

  //
  // add methods
  //

  static void addArg(String s) {
    coder_out.code(s);
  }

  static void addDataArg(String s) {
    int len = s.length();
    coder_out.code(ANY);
    coder_out.code(len+1);
    coder_out.code(rpc_SyncData);
    byte[] b = new byte[len];
    s.getBytes(0, len, b, 0);
    coder_out.code(b);
    coder_out.code((char)0);
  }

  static void addArg(int x) {
    coder_out.code(x);
  }

  static void addArg(boolean b) {
    coder_out.code(b);
  }

  static void addArg(char c) {
    coder_out.code(c);
  }

  static void addArg(Oid oid) {
    coder_out.code(oid);
  }

  static void addArg(byte b[]) {
    coder_out.code(ANY);
    coder_out.code(b.length);

    if (is_syncdata(b.length))
      {
	coder_out.code(rpc_SyncData);
	coder_out.code(b);
      }
    else
      {
	coder_out.code(rpc_ASyncData);
	coder_data[coder_data_cnt++] = new CoderData(b, b.length);
      }
  }

  static void addArg(byte b[], int len) {
    coder_out.code(ANY);
    coder_out.code(len);

    if (is_syncdata(len))
      {
	coder_out.code(rpc_SyncData);
	coder_out.code(b, len);
      }
    else
      {
	coder_out.code(rpc_ASyncData);
	coder_data[coder_data_cnt++] = new CoderData(b, len);
      }
  }

  //
  // get methods
  //

  static int getIntArg() {
    return coder_in.decodeInt();
  }

  static short getShortArg() {
    return coder_in.decodeShort();
  }

  static String getStringArg() {
    return coder_in.decodeString();
  }

  static void getStatusArg(Connection conn, Status status) {
    int stat = coder_in.decodeInt();
    String s = "";
    if (stat != 0)
      {
	coder_in.reset();
	int n = read(conn.main_is, coder_in.getData(), 8);
	int xstat = coder_in.decodeInt();
	int len = coder_in.decodeInt();
	coder_in.adapt(len+1);
	coder_in.reset();
	n = read(conn.main_is, coder_in.getData(), len+1);
	s = coder_in.decodeString(true);
      }

    status.set(stat, s);
  }

  static byte[] getDataArg() {
    int size   = coder_in.decodeInt();
    int status = coder_in.decodeInt();
    byte b[]   = null;
    int offset = coder_in.decodeInt();

    if (status == rpc_SyncData)
      {
	//System.out.println("getDataArg() " + offset + ", status " + status);
	int o_offset = coder_in.getOffset();
	coder_in.setOffset(o_offset + offset - 4);
	b = coder_in.decodeBuffer(size);

	coder_in.setOffset(o_offset);
      }
    else if (status == rpc_ASyncData)
      {
	//System.out.println("getDataArg() noSyncData : " + size);

	b = new byte[size];

	byte[] bsz = new byte[4];
	int n = 0;

	if ((n = read(xconn.main_is, bsz, bsz.length)) !=
	    bsz.length)
	  System.err.println("error size=" + size + ", n=4");

	Coder coder = new Coder(bsz);
	int sz = coder.decodeInt();
	
	if ((n = read(xconn.main_is, b, sz)) != sz)
	  System.err.println("error size=" + size + ", n=" + n);
      }
    else
      System.err.println("### fatal protocol error #1\n");

    return b;
  }

  static Oid getOidArg() {
    return coder_in.decodeOid();
  }

  //
  // realize method
  //

  static void writeASyncData(Connection conn) {
    if (coder_data_cnt == 0)
      return;
    Coder coder = new Coder();
    for (int i = 0; i < coder_data_cnt; i++)
      coder.code(coder_data[i].len);

    try {
      OutputStream os = conn.main_os;
      rpc_write_cnt++;
      os.write(coder.getData(), 0, coder.getOffset());

      for (int i = 0; i < coder_data_cnt; i++) {
	  rpc_write_cnt++;
	  os.write(coder_data[i].data, 0, coder_data[i].len);
      }
    }

    catch(IOException e) {
      //System.out.println("here in ASyncData!");
      System.out.println(e);
    }
  }

    public static int read_ms, write_ms, write_async_ms, read_cnt, read_async_cnt;
    public static int rpc_write_cnt, rpc_read_cnt;
    

    static boolean realize(Connection conn, Status status) {

	if (TRACE)
	    (new java.lang.Exception()).printStackTrace();

	rpc_write_cnt = 0;
	rpc_read_cnt = 0;
    try {
      OutputStream os = conn.main_os;
      int offset = coder_out.getOffset();
      //System.out.println("writing size " + offset);
      coder_out.setOffset(12);
      coder_out.code(offset < RPC_MIN_SIZE ? RPC_MIN_SIZE : offset); // size
      coder_out.code(coder_data_cnt); // ndata
      coder_out.setOffset(offset);
      rpc_write_cnt++;
      os.write(coder_out.getData(), 0, offset);
      long ms = System.currentTimeMillis();
      int rsize = RPC_MIN_SIZE - offset;
      if (rsize > 0) {
	  rpc_write_cnt++;
	  os.write(null_bytes, 0, rsize);
      }
      long ms2 = System.currentTimeMillis();
      write_ms += ms2 - ms;
      writeASyncData(conn);
      write_async_ms += System.currentTimeMillis() - ms;
    }

    catch(IOException e) {
      System.err.println("RPClib.realize " + e.toString());
      return false;
    }

    int stat = 0;

    InputStream is = conn.main_is;
    
    long ms = System.currentTimeMillis();
    int n = read(is, coder_in.getData(), RPC_MIN_SIZE);
    read_cnt++;
    read_ms += System.currentTimeMillis() - ms;
    
    //System.out.println("has read: " + n);
    if (n != RPC_MIN_SIZE) {
      status.set(1, "rpc failure: n=" + n + " vs. " + RPC_MIN_SIZE);
      return false;
    }

    int magic  = getIntArg();
    if (magic != rpc_Magic)
      {
	System.err.println("### fatal protocol error #2");
	return false;
      }

    int serial = getIntArg();          // serial
    int code   = getIntArg();          // code
    int size   = getIntArg(); // size
    int ndata  = getIntArg();          // ndata
    stat       = getIntArg();          // status

    if (stat != 0)
      status.set(1, "rpc failure: stat=" + stat);

    /*

      System.out.println("magic  = " + magic);
      System.out.println("serial = " + serial);
      System.out.println("code   = " + code);
      System.out.println("size   = " + size);
      System.out.println("ndata  = " + ndata);
      System.out.println("stat   = " + stat);
    */


    int rsize = size - RPC_MIN_SIZE;
    if (rsize > 0)
      {
	byte b[] = new byte[rsize];

	n = read(is, b, rsize);
	read_async_cnt++;

	if (n != rsize)
	  return false;

	coder_in.adapt(size);
	System.arraycopy(b, 0, coder_in.getData(), RPC_MIN_SIZE, rsize);
      }

    xconn = conn;
    /*
    System.out.println("write " + rpc_write_cnt + " read " +
		       rpc_read_cnt);
    */
    return (stat == 0 ? true : false);
  }
}
