/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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


#include "eyedb_p.h"

namespace eyedb {

  OQLIterator::OQLIterator(Database *db, const std::string &s)
  {
    q = new OQL(db, s.c_str());
    toDelete = True;
    status = q->execute(val_arr);
    // 19/09/05: cannot throw in a constructor
    //if (status) throw *status;
    cur = 0;
  }

  OQLIterator::OQLIterator(OQL &_q)
  {
    q = &_q;
    toDelete = False;
    status = q->execute(val_arr);
    // 19/09/05: cannot throw in a constructor
    //if (status) throw *status;
    cur = 0;
  }

  Bool OQLIterator::next(Oid &oid)
  {
    for (;;)
      {
	Value v;
	if (!next(v))
	  return False;

	if (v.getType() == Value::tOid) {
	  oid = *v.oid;
	  return True;
	}
      }
  }

  Bool OQLIterator::next(ObjectPtr &o_ptr, const RecMode *rcm)
  {
    Object *o = 0;
    Bool b = next(o, rcm);
    o_ptr = o;
    return b;
  }

  Bool OQLIterator::next(Object *&o, const RecMode *rcm)
  {
    Oid oid;
    if (!next(oid))
      return False;

    o = 0;
    status = q->getDatabase()->loadObject(oid, o, rcm);
    if (status)
      throw *status;

    return True;
  }

  Bool OQLIterator::next(Value &v)
  {
    if (cur < val_arr.getCount()) {
      v = val_arr[cur++];
      return True;
    }

    return False;
  }

  OQLIterator::~OQLIterator()
  {
    if (toDelete) delete q;
  }
}
