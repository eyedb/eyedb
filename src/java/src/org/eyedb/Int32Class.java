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



public class Int32Class extends BasicClass {

  Int32Class(Database db) {
    super(db, "int32", Coder.INT32_SIZE);
  }

  Int32Class() {
    super(null, "int32", Coder.INT32_SIZE);
  }

  void traceData(java.io.PrintStream out, Coder coder, TypeModifier typmod) {

    if (typmod != null && typmod.pdims > 1)
      {
	out.print("{");
	for (int i = 0; i < typmod.pdims; i++)
	  {
	    if (i != 0)
	      out.print(", ");
	    out.print(coder.decodeInt());
	  }
	out.print("}");
	return;
      }

    out.print(coder.decodeInt());
  }

  Value getValueValue(Coder coder) {
    return new Value(coder.decodeInt());
  }

  void setValueValue(Coder coder, Value value) 
       throws Exception {
    coder.code(value.getInt());
  }
}

