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

public class Exception extends Throwable {
  
  Status status;
  String exception_name;

  public Exception(Status status) {
    this.status = status;
    this.exception_name = "Exception";
  }

  public Exception(Status status, String exception_name) {
    this.status = status;
    this.exception_name = exception_name;
  }

  public Exception(String exception_name, String detail) {
    this.status = new Status(Status.IDB_ERROR, detail);
    this.exception_name = exception_name;
  }

  public Exception(int status, String exception_name, String detail) {
    this.status = new Status(status, detail);
    this.exception_name = exception_name;
  }

  public Exception(Status status, String exception_name, String detail) {
    if (status.str != null && !status.str.equals(""))
      this.status = new Status(status.status, status.str + " : " + detail);
    else
      this.status = new Status(status.status, detail);
    this.exception_name = exception_name;
  }

  public String getString() {
      return "org.eyedb." + exception_name + ": " + status.getString();
  }

  public String toString() {
      return getString();
  }

  public int getStatus() {return status.status;}

  public void print() {
    System.err.println(getString());
  }

  public void print(String s) {
    System.err.println(s + ": " + getString());
  }
}
