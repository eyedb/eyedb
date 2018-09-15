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

public class EnumItem {

    public EnumItem(String s, int val, int num) {
	this.s = s;
	this.val = val;
	this.num = num;
    }

    public boolean compare(EnumItem item) {

	if (num != item.num)
	    return false;

	if (val != item.val)
	    return false;

	return s.equals(item.s);
    }

    public String getName() {return s;}
    public int getValue()   {return val;}
    public int getNum()     {return num;}

    String s;
    int val;
    int num;
}

