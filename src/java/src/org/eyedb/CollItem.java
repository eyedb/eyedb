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

class CollItem {

  private Value value;
  private int state;
  private int ind;
  private int count;

  static final int undefined = 0;
  static final int coherent = 1;
  static final int added    = 2;
  static final int removed  = 3;

  CollItem(Value value) {
    this.value  = value;
    this.state = 0;
    this.ind   = undefined;
    this.count = 1;
  }

  CollItem(Value value, int state) {
    this.value  = value;
    this.state = state;
    this.ind   = 1;
    this.count = 1;
  }

  CollItem(Value value, int state, int ind) {
    this.value  = value;
    this.state = state;
    this.ind   = ind;
    this.count = 1;
  }

  int getState() {return state;}
  void setState(int state) {this.state = state;}
  int getInd() {return ind;}
  Value getValue() {return value;}

  int getCount() {return count;}
  void incrCount() {count++;}

  public int hashCode() {
    return value.hashCode();
  }

  public boolean equals(java.lang.Object obj) {
    CollItem x = (CollItem)obj;
    return x.value.equals(value);
  }
}
