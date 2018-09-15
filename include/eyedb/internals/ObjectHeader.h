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


#ifndef _EYEDB_INTERNALS_OBJECT_HEADER_H
#define _EYEDB_INTERNALS_OBJECT_HEADER_H

namespace eyedb {

  typedef struct ObjectHeader {
    ::eyedblib::int32 magic;
    ::eyedblib::int32 type;
    ::eyedblib::int32 size;
    ::eyedblib::int64 ctime;
    ::eyedblib::int64 mtime;
    ::eyedblib::int32 xinfo;
    ::eyedbsm::Oid oid_cl;
    ::eyedbsm::Oid oid_prot;
  } ObjectHeader;

}

#endif
