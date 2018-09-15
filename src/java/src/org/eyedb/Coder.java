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

public class Coder {

  byte[] data;
  int offset;
  boolean isconst = false;

  public Coder() {
    this.data = null;
    offset    = 0;
  }
 
  Coder(byte[] data) {
    this.data = data;
    offset    = 0;
  }

  Coder(byte[] data, int offset) {
    this.data   = data;
    this.offset = offset;
  }

  Coder(byte[] data, int offset, boolean isconst) {
    this.data    = data;
    this.offset  = offset;
    this.isconst = isconst;
  }

  void code(char c) {
    adapt(CHAR_SIZE);

    data[offset] = (byte)c;

    offset += CHAR_SIZE;
  }

  void code(byte b) {
    adapt(BYTE_SIZE);

    data[offset] = b;

    offset += BYTE_SIZE;
  }

  void code(int x) {
    adapt(INT32_SIZE);

    int ind = SWAP_INT32_START + offset;

    data[ind] = (byte)(x & 0xff);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0xff00)     >>> 8);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0xff0000)   >>> 16);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0xff000000) >>> 24);

    offset += INT32_SIZE;
  }

  void code(short s) {
    adapt(INT16_SIZE);

    int ind = SWAP_INT16_START + offset;

    data[ind] = (byte)(s & 0xff);
    ind += SWAP_INCR;

    data[ind] = (byte)((s & 0xff00)     >>> 8);
    ind += SWAP_INCR;

    offset += INT16_SIZE;
  }

  void code(long x) {
    adapt(INT64_SIZE);

    int ind = offset + SWAP_INT64_START;

    data[ind] = (byte)((x & 0x00000000000000ffL));
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x000000000000ff00L) >>> 8);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x0000000000ff0000L) >>> 16);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x00000000ff000000L) >>> 24);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x000000ff00000000L) >>> 32);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x0000ff0000000000L) >>> 40);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x00ff000000000000L) >>> 48);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0xff00000000000000L) >>> 56);
    ind += SWAP_INCR;

    offset += INT64_SIZE;
  }

  void code(double d) {

    code(Double.doubleToLongBits(d));

    /*
    adapt(DOUBLE_SIZE);

    int ind = offset + SWAP_DOUBLE_START;

    data[ind] = (byte)((x & 0x00000000000000ffL));
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x000000000000ff00L) >>> 8);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x0000000000ff0000L) >>> 16);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x00000000ff000000L) >>> 24);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x000000ff00000000L) >>> 32);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x0000ff0000000000L) >>> 40);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0x00ff000000000000L) >>> 48);
    ind += SWAP_INCR;

    data[ind] = (byte)((x & 0xff00000000000000L) >>> 56);
    ind += SWAP_INCR;
    */
  }

  void code(String s) {
    int len = s.length();

    adapt(INT32_SIZE + len + 1);

    code(len+1);

    s.getBytes(0, len, data, offset);

    offset += len;

    data[offset] = 0;

    offset++;
  }

  // added for version 2.4.0
  void code(String s, int sz) {
    int len = s.length();

    adapt(sz);

    s.getBytes(0, len, data, offset);

    offset += sz;
  }

  void code(String s, boolean b) {
    int len = s.length();

    adapt(len + 1);

    s.getBytes(0, len, data, offset);

    offset += len;

    data[offset] = 0;

    offset++;
  }

  void code(boolean b) {
    code(b ? (char)1 : (char)0);
  }

  void code(byte[] b) {
    code(b, b.length);
  }

  void code(byte[] b, int len) {
    adapt(len);
    System.arraycopy(b, 0, data, offset, len);
    offset += len;
  }

  void code(Oid oid) {
    if (oid == null)
      oid = new Oid();
    code(oid.nx);
    code((oid.dbid << 22) | oid.unique);
  }

  char decodeChar() {
    return (char)data[offset++];
  }

  byte decodeByte() {
    return data[offset++];
  }

  short decodeShort() {
    int ind = offset + SWAP_INT16_START;

    short x = ((short)unsign(data[ind]));

    x += (short)(unsign(data[ind + SWAP_INCR]) << 8);

    offset += INT16_SIZE;
    return x;
  }

  int decodeInt() {
    int ind = offset + SWAP_INT32_START;

    int x = unsign(data[ind]);
    x += unsign(data[ind + SWAP_INCR])   << 8;
    x += unsign(data[ind + 2*SWAP_INCR]) << 16;
    x += unsign(data[ind + 3*SWAP_INCR]) << 24;

    offset += INT32_SIZE;
    return x;
  }

  long decodeLong() {
    int ind = offset + SWAP_INT64_START;

    /*
      this does not work for decodeDouble():
      one must cast in long */

    /*
    long x = unsign(data[ind]);

    x += unsign(data[ind + SWAP_INCR])   <<  8;
    x += unsign(data[ind + 2*SWAP_INCR]) << 16;
    x += unsign(data[ind + 3*SWAP_INCR]) << 24;
    x += unsign(data[ind + 4*SWAP_INCR]) << 32;
    x += unsign(data[ind + 5*SWAP_INCR]) << 40;
    x += unsign(data[ind + 6*SWAP_INCR]) << 48;
    x += unsign(data[ind + 7*SWAP_INCR]) << 56;
    */

    long x = ((long)unsign(data[ind]));

    x += ((long)unsign(data[ind + SWAP_INCR]))   <<  8;
    x += ((long)unsign(data[ind + 2*SWAP_INCR])) << 16;
    x += ((long)unsign(data[ind + 3*SWAP_INCR])) << 24;
    x += ((long)unsign(data[ind + 4*SWAP_INCR])) << 32;
    x += ((long)unsign(data[ind + 5*SWAP_INCR])) << 40;
    x += ((long)unsign(data[ind + 6*SWAP_INCR])) << 48;
    x += ((long)unsign(data[ind + 7*SWAP_INCR])) << 56;

    offset += INT64_SIZE;
    return x;
  }

  double decodeDouble() {
    /*
    int ind = offset + SWAP_DOUBLE_START;

    long x = (long)unsign(data[ind]);

    x += ((long)unsign(data[ind + SWAP_INCR]))   <<  8;
    x += ((long)unsign(data[ind + 2*SWAP_INCR])) << 16;
    x += ((long)unsign(data[ind + 3*SWAP_INCR])) << 24;
    x += ((long)unsign(data[ind + 4*SWAP_INCR])) << 32;
    x += ((long)unsign(data[ind + 5*SWAP_INCR])) << 40;
    x += ((long)unsign(data[ind + 6*SWAP_INCR])) << 48;
    x += ((long)unsign(data[ind + 7*SWAP_INCR])) << 56;

    offset += DOUBLE_SIZE;
    */
    long x = decodeLong();

    return Double.longBitsToDouble(x);
  }

  String decodeString() {
    int len = decodeInt();
    String s = new String(data, 0, offset, len-1);
    offset += len;
    return s;
  }

  /*
  // added for version 2.4.0
  String decodeBoundString(int len) {
    String s = new String(data, 0, offset, len-1);
    offset += len;
    return s;
  }
  */

  String decodeString(boolean b) {
    int l;
    for (l = offset; l < data.length; l++)
      if (data[l] == 0)
	break;

    String s = new String(data, 0, offset, l - offset);
    offset += l;
    return s;
  }

  String decodeString(int len) {
    int l;
    int end = offset + len;
    for (l = offset; l < end; l++)
      if (data[l] == 0)
	break;

    String s = new String(data, 0, offset, l - offset);
    offset += len;
    return s;
  }

  boolean decodeBoolean() {
    char c = decodeChar();
    return (c == 0 ? false : true);
  }

  byte[] decodeBuffer(int len) {
    byte[] b = new byte[len];
    System.arraycopy(data, offset, b, 0, len);
    offset += len;
    return b;
  }

  Oid decodeOid() {
    int x, y;

    x = decodeInt();
    y = decodeInt();

    return new Oid(x, (y & 0xffd00000) >> 22, (y & 0x3fffff));
  }

  int getOffset() {return offset;}

  void setOffset(int offset) {this.offset = offset;}

  byte[] getData() {return data;}

  void reset() {offset = 0;}

  static private final int INCR_SIZE         = 128;
  static private final int SWAP_INT64_START  = 7;
  static private final int SWAP_INT32_START  = 3;
  static private final int SWAP_INT16_START  = 1;
  static private final int SWAP_DOUBLE_START = 7;
  static private final int SWAP_INCR         = -1;

  static public final int BYTE_SIZE         = 1;
  static public final int CHAR_SIZE         = 1;
  static public final int INT64_SIZE        = 8;
  static public final int INT32_SIZE        = 4;
  static public final int INT16_SIZE        = 2;
  static public final int DOUBLE_SIZE       = 8;
  static public final int OID_SIZE          = 8;
  static public final int OBJECT_SIZE       = 4;
  
  public void adapt(int len, boolean copy) {
    int olength = (data != null ? data.length : 0);

    if (offset + len > olength) // was >=
      {
	  if (isconst) {
	      System.err.println("Coder error in adapt(offset = " +
				 offset + ", len = " + len + ", olength = " +
				 olength + ") is const");
	      return;
	  }

	int nlength = Math.max(olength, offset + len) + INCR_SIZE;

	if (copy && data != null)
	  {
	    byte[] b = new byte[nlength];
	    System.arraycopy(data, 0, b, 0, olength);
	    data = b;
	  }
	else
	  data = new byte[nlength];
      }
   }

  public void adapt(int len) {
    adapt(len, true);
  }

  private static int unsign(byte b) {
    int y = (int)b;

    if (y < 0)
      y = 0xff + y + 1;

    return y;
  }

  static void memzero(byte[] b) {
    for (int i = 0; i < b.length; i++)
      b[i] = 0;
  }

  public static void memzero(byte[] b, int offset, int size) {
    int end = size + offset;

    for (int i = offset; i < end; i++)
      b[i] = 0;
  }

  public static boolean memcmp(byte[] b1, int offset1, byte[] b2, int offset2,
			       int size) {

    for (int i = 0, off1 = offset1, off2 = offset2; i < size; i++,
	   off1++, off2++)
      if (b1[off1] != b2[off2])
	return false;

    return true;
  }
}
