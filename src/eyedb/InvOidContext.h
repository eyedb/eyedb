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


#ifndef _eyedb_invoidctx_
#define _eyedb_invoidctx_

namespace eyedb {

  class InvOidContext {

  public: // conceptually private: package level

    Oid objoid;
    Offset attr_offset;
    int attr_num;
    Oid valoid;

    InvOidContext(const Oid &_objoid,
		  const Attribute *attr,
		  const Oid &_valoid);

    InvOidContext(const Oid &_objoid,
		  int _attr_num, Offset _attr_offset,
		  const Oid &_valoid);

    static Bool getContext();
    static void insert(const Oid &objoid, const Attribute *attr,
		       const Oid &valoid);
    static void releaseContext(Bool newctx, Data *, void *);

    static void code(Data *data, LinkedList &inv_list, Bool toDel,
		     Size &size);
    static void decode(Data data, LinkedList &);
  };

}

#endif
