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

public class OidArray {

  Oid[] oids;

  public OidArray() {
    oids = null;
  }

  public OidArray(Oid oids[]) {
    set(oids);
  }

  public OidArray(OidArray oid_array) {

    if (oid_array.oids == null)
      {
	oids = null;
	return;
      }

    oids = new Oid[oid_array.oids.length];

    for (int i = 0; i < oids.length; i++)
      oids[i] = oid_array.oids[i];
  }

  public OidArray(java.util.Vector v) {
    set(v);
  }

  public OidArray(java.util.LinkedList list) {
    set(list);
  }

  public void set(java.util.Vector v) {
   int n = 0;
   oids = new Oid[v.size()];
   for (java.util.Enumeration e = v.elements(); e.hasMoreElements();)
       oids[n++] = (Oid)e.nextElement();
  }

  public void set(java.util.LinkedList list) {
      oids = new Oid[list.size()];
      java.util.ListIterator iter = list.listIterator(0);
      int n = 0;
      while (iter.hasNext())
	  oids[n++] = (Oid)iter.next();
  }

  public void set(Oid oids[]) {
    this.oids = oids;
  }

  public void append(Oid oid) {
    int olength = (oids == null ? 0 : oids.length);
    Oid[] noids = new Oid[olength + 1];

    for (int i = 0; i < olength; i++)
      noids[i] = oids[i];

    noids[olength] = oid;
    oids = noids;
  }

  static OidArray[] copyArray(OidArray[] oid_array) {

    if (oid_array == null)
      return null;

    int len = oid_array.length;
    OidArray[] xoid_array = new OidArray[len];

    for (int i = 0; i < len; i++)
      if (oid_array[i] != null)
	xoid_array[i] = new OidArray(oid_array[i].oids);

    return xoid_array;
  }

  public Oid[] getOids() {return oids;}

  public Oid getOid(int idx) {return oids[idx];}

  public int getCount() {return (oids == null ? 0 : oids.length);}

  public java.util.Vector toVector() {
    java.util.Vector v = new java.util.Vector();
    if (oids != null)
	for (int i = 0; i < oids.length; i++)
	    v.add(oids[i]);
    return v;
  }
}
