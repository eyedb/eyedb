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

public class Data {

  byte[] data;

  public Data() {
    this.data = null;
  }

  public Data(byte[] data) {
    this.data = data;
  }

  public Data(Data data) {
    if (data == null || data.data == null)
      {
	this.data = null;
	return;
      }

    this.data = new byte[data.data.length];
    System.arraycopy(data.data, 0, this.data, 0, data.data.length);
  }

  static Data[] copyArray(Data[] data_array) {
    if (data_array == null)
      return null;

    int len = data_array.length;
    Data[] data = new Data[len];

    // it seems to me that if data_array[i] is null,
    // data[i] should be null too!
    for (int i = 0; i < len; i++)
      //      data[i] = new Data(data_array[i]);
      data[i] = (data_array[i] == null ? null : new Data(data_array[i]));

    return data;
  }

  public Data(int size) {
    data = new byte[size];
  }
}
