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

public abstract class AbstractIterator {

  public Value nextValue() {
    if (cur < val_arr.getCount())
      return val_arr.getValue(cur++);

    return null;
  }

  public Oid nextOid() {
    for (;;)
      {
	Value v = nextValue();
	if (v == null) return null;

	if (v.getType() == Value.OID)
	  return v.sgetOid();
      }
  }

  public Object nextObject() throws Exception {
    return nextObject(RecMode.NoRecurs);
  }

  public Object nextObject(RecMode rcm) throws Exception {
    if (db == null) throw new LoadObjectException
		      (new Status(Status.IDB_DATABASE_LOAD_OBJECT_ERROR,
				     "AbstractIterator.nextObject: " +
				     "database reference is null"));
    Oid oid = nextOid();
    if (oid == null) return null;
    return db.loadObject(oid, rcm);
  }

  protected ValueArray val_arr;
  protected int cur;
  protected Database db;
}
