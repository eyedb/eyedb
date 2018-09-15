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


  ClassIterator::ClassIterator(const Class *cls, Bool subclass)
  {
    init(cls, subclass);
  }

  ClassIterator::ClassIterator(const ClassPtr &cls_ptr, Bool subclass)
  {
    init(cls_ptr.getClass(), subclass);
  }

  void ClassIterator::init(const Class *cls, Bool subclass)
  {
    q = new Iterator(const_cast<Class*>(cls), subclass);
    status = q->getStatus();
    if (status) {
      delete q;
      q = 0;
      // 19/09/05: cannot throw in a constructor
      //throw *status;
    }
  }

  Bool ClassIterator::next(Oid &oid)
  {
    if (status)
      throw *status;
    Bool found;
    status = q->scanNext(found, oid);
    if (status)
      throw *status;
    return found;
  }

  Bool ClassIterator::next(ObjectPtr &o_ptr, const RecMode *rcm)
  {
    Object *o = 0;
    Bool b = next(o, rcm);
    o_ptr = o;
    return b;
  }

  Bool ClassIterator::next(Object *&o, const RecMode *rcm)
  {
    if (status)
      throw *status;
    Bool found;
    o = 0;
    status = q->scanNext(found, o, rcm);
    if (status)
      throw *status;
    return found;
  }

  Bool ClassIterator::next(Value &v)
  {
    if (status)
      throw *status;
    Bool found;
    status = q->scanNext(found, v);
    if (status) throw *status;
    return found;
  }


  ClassIterator::~ClassIterator()
  {
    delete q;
  }

}
