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

class Indent {

  String indent;

  static private String INDENT_INCR = "\t";
  static private int INDENT_INCR_LEN = INDENT_INCR.length();

  Indent() {
    indent = "";
  }

  Indent(Indent xindent) {
    indent = new String(xindent.indent);
  }

  void push() {
    indent = indent + INDENT_INCR;
  }

  void pop() {
    int len = indent.length();
    indent = indent.substring(0, len - INDENT_INCR_LEN);
  }

  static String getIndentIncr() {
    return INDENT_INCR;
  }

  static void setIndentIncr(String indent_incr) {
    INDENT_INCR = indent_incr;
    INDENT_INCR_LEN = INDENT_INCR.length();
  }

  public String toString() {
    return indent;
  }
}

