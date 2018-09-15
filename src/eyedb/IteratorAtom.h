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


#ifndef _EYEDB_ITERATOR_ATOM_H
#define _EYEDB_ITERATOR_ATOM_H

namespace eyedb {

#define NEW_ITER_ATOM
  enum IteratorAtomType {
#ifdef NEW_ITER_ATOM
    IteratorAtom_INT16 = 1,
    IteratorAtom_INT32,
    IteratorAtom_INT64,
    IteratorAtom_CHAR,
    IteratorAtom_DOUBLE,
    IteratorAtom_STRING,
    IteratorAtom_OID,
    IteratorAtom_IDR
#else
    IteratorAtom_NULL = 1,
    IteratorAtom_NIL,
    IteratorAtom_BOOL,
    IteratorAtom_INT16,
    IteratorAtom_INT32,
    IteratorAtom_INT64,
    IteratorAtom_CHAR,
    IteratorAtom_DOUBLE,
    IteratorAtom_STRING,
    IteratorAtom_IDENT,
    IteratorAtom_OID,
    IteratorAtom_IDR,
    IteratorAtom_LISTSEP
#endif
  };

  struct IteratorAtom {
    IteratorAtomType type;

    char *fmt_str;

    union {
      eyedblib::int16 i16;
      eyedblib::int32 i32;
      eyedblib::int64 i64;
      char c;
      double d;
      char *str;
      eyedbsm::Oid oid;
      struct {
	Size size;
	Data idr;
      } data;
#ifndef NEW_ITER_ATOM
      Bool b;
      char *ident;
      Bool open;
#endif
    };

    IteratorAtom();
    IteratorAtom(const IteratorAtom&);

    Value* toValue() const;

    operator Value *() {
      return toValue();
    }

    void print(FILE * = stdout);
    char *getString();
    int getSize() const;

    IteratorAtom &operator =(const IteratorAtom &);
    void code(Data *, Offset *, Size *);
    void decode(Data, Offset *);

    void garbage();
    IteratorAtom *next;

    static void makeValueArray(IteratorAtom **atoms, int count,
			       ValueArray &valarray);
    ~IteratorAtom();
  };

}

#endif
