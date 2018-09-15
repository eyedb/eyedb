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

public class OQLIterator extends AbstractIterator {

  public OQLIterator(Database db, String query) 
    throws Exception {
      oql = new OQL(db, query);
      val_arr = new ValueArray();
      oql.execute(val_arr);
      this.db = db;
      cur = 0;
  }

  public OQLIterator(OQL oql)
    throws Exception {
      this.oql = oql;
      this.oql.execute(val_arr);
      this.db = oql.getDatabase();
      cur = 0;
  }

  private OQL oql;
}
