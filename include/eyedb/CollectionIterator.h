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


#ifndef _EYEDB_COLLECTION_ITERATOR_H
#define _EYEDB_COLLECTION_ITERATOR_H

namespace eyedb {

  /**
     Not yet documented.
  */
  class CollectionIterator : public AbstractIterator {

  public:
    CollectionIterator(const Collection *, Bool indexed = False);
    CollectionIterator(const Collection &, Bool indexed = False);
    CollectionIterator(const CollectionPtr &, Bool indexed = False);

    Bool next(Oid &);
    Bool next(ObjectPtr &, const RecMode * = RecMode::NoRecurs);
    Bool next(Object *&, const RecMode * = RecMode::NoRecurs);
    Bool next(Value &);

    ~CollectionIterator();

  private:
    std::vector<Object *> objv;
    void init(const Collection *, Bool indexed);
    const Collection *coll;
    Iterator *q;
    ValueArray val_arr;
    unsigned int cur;
  };
}

#endif
