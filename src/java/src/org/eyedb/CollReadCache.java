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

public class CollReadCache {

  ValueArray val_arr;
  OidArray oid_arr;
  ObjectArray obj_arr;
  RecMode rcm;
  boolean index;
  private boolean valid_val;
  private boolean valid_oid;
  private boolean valid_obj;

  CollReadCache() {
    val_arr = new ValueArray();
    oid_arr = new OidArray();
    obj_arr = new ObjectArray();
    invalidate();
  }

  boolean isValidObj(boolean index, RecMode rcm) {
    return valid_obj && this.index == index && this.rcm == rcm;
  }

  void invalidateObj() {
    valid_obj = false;
  }

  void validateObj(boolean index, RecMode rcm) {
    this.index = index;
    this.rcm = rcm;
    valid_obj = true;
  }

  boolean isValidVal(boolean index) {
    return valid_val && this.index == index;
  }

  void invalidateVal() {
    valid_val = false;
  }

  void validateVal(boolean index) {
    this.index = index;
    valid_val = true;
  }

  boolean isValidOid(boolean index) {
    return valid_oid && this.index == index;
  }

  void invalidateOid() {
    valid_oid = false;
  }

  void validateOid(boolean index) {
    this.index = index;
    valid_oid = true;
  }

  void invalidate() {
    rcm = null;
    index = false;
    valid_obj = false;
    valid_oid = false;
    valid_val = false;
  }
}

