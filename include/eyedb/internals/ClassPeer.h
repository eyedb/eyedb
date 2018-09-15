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


#ifndef _EYEDB_INTERNALS_CLASS_PEER_H
#define _EYEDB_INTERNALS_CLASS_PEER_H

namespace eyedb {

  class ClassPeer {

  public:
    static Status makeColls(Database *, Class *, Data,
			    Bool make_extent = False);
    static Status makeColls(Database *, Class *, Data,
			    const Oid *);
    static Status makeColl(Database *db, Collection **pcoll,
			   Data idr, Offset offset);
    static void newObjRealize(Class *, Object *);

    static void setMType(Class *, Class::MType);

    static void setParent(Class *, const Class *);

    static void setParent(Class *, Class *);
  };

}

#endif
