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

public class ValueArray {

  Value[] values;

  public ValueArray() {
    values = null;
  }

  public ValueArray(Value[] values) {
    set(values);
  }

  public ValueArray(java.util.Vector v) {
    set(v);
  }

  public void set(java.util.Vector v) {
   int n = 0;
   values = new Value[v.size()];
   for (java.util.Enumeration e = v.elements(); e.hasMoreElements();)
      values[n++] = (Value)e.nextElement();
  }

  public void set(Value[] values) {
    this.values = values;
  }

  public Value[] getValues() {return values;}
  public Value getValue(int idx) {return values[idx];}

  public void append(ValueArray value_array, int from, int count) {
    Value[] xvalues = value_array.values;

    if (xvalues == null)
      return;

    int bound = Math.min(xvalues.length - from, count);

    int len = (values != null ? values.length : 0);
    Value[] yvalues = new Value[len + bound];

    for (int i = 0; i < len; i++)
      yvalues[i] = values[i];

    for (int i = 0; i < bound; i++)
      yvalues[i+len] = xvalues[i+from];

    this.values = yvalues;
  }

  public int getCount() {return (values == null ? 0 : values.length);}

  public void print() throws Exception {
    if (values == null)
      return;
      
    for (int n = 0; n < values.length; n++) {
	if (n != 0)
	    System.out.print(", ");
	values[n].print();
    }
  }

  public java.util.Vector toVector() {
    java.util.Vector v = new java.util.Vector();
    if (values != null)
	for (int i = 0; i < values.length; i++)
	    v.add(values[i]);
    return v;
  }
}
