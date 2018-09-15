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

//
// ValueStruct class
//

package org.eyedb;

public class ValueStruct {
  int attr_cnt;
  ValueStructAttr attrs[];

  public ValueStruct() {attr_cnt = 0; attrs = null;}

  public ValueStruct(int attr_cnt) {
    this.attr_cnt = attr_cnt;
    this.attrs = new ValueStructAttr[attr_cnt];
  }

  public String toString() {
    String s = "struct(";
    for (int ii = 0; ii < attr_cnt; ii++)
      {
	if (ii != 0) s += ", ";
	s += attrs[ii].name + ": " + attrs[ii].value.toString();
      }
    return s + ")";
  }

  public boolean equals(java.lang.Object o) {
    if (!(o instanceof ValueStruct))
      return false;

    ValueStruct stru = (ValueStruct)o;
    if (attr_cnt != stru.attr_cnt)
      return false;

    for (int i = 0; i < attr_cnt; i++)
      {
	if (!attrs[i].name.equals(stru.attrs[i].name))
	  return false;
	if (!attrs[i].value.equals(stru.attrs[i].value))
	  return false;
      }

    return true;
  }
}
