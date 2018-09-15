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

public class RecMode {

  static final int RecMode_NoRecurs   = 0;
  static final int RecMode_FullRecurs = 1;
  static final int RecMode_FieldNames = 2;
  static final int RecMode_Predicat   = 3;

  static public RecMode FullRecurs = new RecMode(RecMode_FullRecurs);
  static public RecMode NoRecurs   = new RecMode(RecMode_NoRecurs);

  public RecMode(int type) {
    this.type = type;
  }

  public RecMode(String s) {
    this.type = RecMode_FieldNames;
  }

  public RecMode(boolean b, String s) {
    this.type = RecMode_FieldNames;
  }

  public int getType() {return type;}

  public boolean isAgregRecurs(Attribute agr) throws Exception {
    return isAgregRecurs(agr, -1, null);
  }

  public boolean isAgregRecurs(Attribute agr, int j) throws Exception {
    return isAgregRecurs(agr, j, null);
  }

  public boolean isAgregRecurs(Attribute agr, int j, Object o)
    throws Exception {

    if (type == RecMode_FullRecurs)
      return true;

    if (type == RecMode_NoRecurs)
      return false;

    if (type == RecMode_Predicat)
	throw new Exception("INTERNAL_ERROR",
			       "RecMode_Predicat not implemented");

    if (type == RecMode_FieldNames)
	throw new Exception("INTERNAL_ERROR",
			       "RecMode_FieldNames not implemented");

    return false;
  }

  private int type;
}
