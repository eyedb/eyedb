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



#ifndef _EYEDB_ITERATOR_BE_ENGINE_H
#define _EYEDB_ITERATOR_BE_ENGINE_H

namespace eyedb {

  class IteratorBEEngine
  {
  protected:
    Status status;
    Database *db;
    DbHandle *dbh;
    Bool state;

  public:
    virtual Status scanNext(int, int *, IteratorAtom *) = 0;
    Status getStatus() const;
    virtual ~IteratorBEEngine() = 0;
  };

  class IteratorBEEngineAttribute : public IteratorBEEngine {
    const Attribute *agr;
    int ind;
    Size size;
    eyedbsm::IdxCursor *curs;

  public:
    IteratorBEEngineAttribute(Database *,
			      const Attribute *, int,
			      Data, Data, Bool, Bool, int);
    Status scanNext(int, int *, IteratorAtom *);
    ~IteratorBEEngineAttribute();
  };

  class IteratorBEEngineCollection : public IteratorBEEngine {
    eyedbsm::IdxCursor *curs;
    eyedbsm::Idx::Key *buff;
    Bool index;
    CollectionBE *collbe;
    unsigned char *data;

  public:
    IteratorBEEngineCollection(CollectionBE *, Bool index);
    Status scanNext(int, int *, IteratorAtom *);
    ~IteratorBEEngineCollection();
  };
}

#endif

  
