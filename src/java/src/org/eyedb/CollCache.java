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

import java.util.*;

class CollCache {

  Hashtable value_table;
  Hashtable index_table;

  CollCache() {
    empty();
  }

  void insert(CollItem x) {
    value_table.put(x, x);
    index_table.put(new Value(x.getInd()), x);
  }

  void suppress(CollItem x) {
      java.lang.Object r = value_table.remove(x);

    if (r == null)
      System.out.println("suppress in value_table : not found");

    r = index_table.remove(new Value(x.getInd()));

    if (r == null)
      System.out.println("suppress in index_table : not found");
  }

  CollItem get(CollItem x) {
    return (CollItem)value_table.get(x);
  }

  CollItem get(int ind) {
    return (CollItem)index_table.get(new Value(ind));
  }

  void empty() {
    value_table = new Hashtable(1024);
    index_table = new Hashtable(1024);
  }

  int getCount(boolean isArray) {
    int count = 0;
    Enumeration en = (isArray ? index_table : value_table).elements();

    while (en.hasMoreElements())
	count += ((CollItem)en.nextElement()).getCount();
     
    return count;
  }
}
