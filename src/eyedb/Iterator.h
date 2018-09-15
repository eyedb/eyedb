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


#ifndef _EYEDB_ITERATOR_H
#define _EYEDB_ITERATOR_H

namespace eyedb {

  // ----------------------------------------------------------------------
  //
  // The Iterator interface is not part of the API. It is used for internal
  // use only.
  // 
  // ----------------------------------------------------------------------

  class IteratorAtom;

  class Iterator {

    // ----------------------------------------------------------------------
    // Iterator Interface
    // ----------------------------------------------------------------------
  public:
    Iterator(Class *, Bool include_subclasses = False); /* class query */
    Iterator(const Collection *, Bool index = False); /* collection query */
    Iterator(Database *, const Attribute *, int, Data, int);
    Iterator(Database *, const Attribute *, int,
	     Data, Data, Bool, Bool, int);

    Status getStatus() const;

    int getCount() const {return _count;}

    Status scanNext(Bool &found, Oid &);
    Status scanNext(Bool &found, ObjectPtr &,
		    const RecMode * = RecMode::NoRecurs);
    Status scanNext(Bool &found, Object *&,
		    const RecMode * = RecMode::NoRecurs);
    Status scanNext(Bool &found, Value &);

    /* same as 3 previous methods : pointer version */
    Status scanNext(Bool *found, Oid *);
    Status scanNext(Bool *found, Object **,
		    const RecMode * = RecMode::NoRecurs);
    Status scanNext(Bool *found, Value *);

    Status scan(ObjectPtrVector &, const RecMode * = RecMode::NoRecurs);
    Status scan(ObjectPtrVector &, unsigned int max,
		unsigned int start = 0, const RecMode * = RecMode::NoRecurs);
    Status scan(ObjectArray &, const RecMode * = RecMode::NoRecurs);
    Status scan(ObjectArray &, unsigned int max,
		unsigned int start = 0, const RecMode * = RecMode::NoRecurs);
    Status scan(OidArray &, unsigned int max = ~0,
		unsigned int start = 0);
    Status scan(ValueArray &, unsigned int max = ~0,
		unsigned int start = 0);
    ~Iterator();

    // ----------------------------------------------------------------------
    // Iterator Private Part
    // ----------------------------------------------------------------------
  private:
    Status status;
    int curqid;
    int nqids;
    int *qid;
    Database *db;
    int curatom, readatom;
    int buff_len;
    IteratorAtom *buffatom;
    Bool state;
    void init(Database *);
    int _count;
    SchemaInfo *schinfo;

  public: /* restricted */
    Status scanNext(Bool *found, IteratorAtom *);
    Status scan(int *count, IteratorAtom **);
    static Status initDatabase(Database *);
  };

  extern void oqml_initialize();
  extern void oqml_release();

}

#endif
