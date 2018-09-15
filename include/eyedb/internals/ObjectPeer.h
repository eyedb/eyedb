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


#ifndef _EYEDB_INTERNALS_OBJECT_PEER_H
#define _EYEDB_INTERNALS_OBJECT_PEER_H


namespace eyedb {

  class ObjectHeader;

  class ObjectPeer {
  
  public:
    static void incrRefCount(Object *);
    static void decrRefCount(Object *);
    static void setUnrealizable(Object *, Bool);
    static void setDatabase(Object *o, Database *db) {o->db = db;}
    static void setOid(Object *, const Oid&);
    static void setProtOid(Object *, const Oid&);
    static void setClass(Object *, const Class *);
    static void make(Object *, const Class *, Data,
		     Type, Size, Size, Size,
		     Bool _copy = True);
    static void setModify(Object *, Bool);
    static void setTimes(Object *, const ObjectHeader &);
    static Status trace_realize(const Object *, FILE*, int, unsigned int,
				const RecMode *);
    static void setIDR(Object *, Data=0, Size=0);
    static inline void setGRTObject(Object *o, Bool b) {o->grt_obj = b;}
    static inline Bool isGRTObject(const Object *o) {return o->grt_obj;}

    static Bool isRemoved(const ObjectHeader &hdr);
    static void setRemoved(Object *o);
    static void loadEpilogue(Object *o, const Oid &,
			     const ObjectHeader &, Data o_idr);
  };

}

#endif
