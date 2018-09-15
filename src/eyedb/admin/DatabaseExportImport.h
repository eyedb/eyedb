/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-1999,2004-2006 SYSRA
   
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
   Author: Francois Dechelle <francois@dechelle.net>
*/

#ifndef _EYEDB_ADMIN_DATABASEEXPORTIMPORT_H_
#define _EYEDB_ADMIN_DATABASEEXPORTIMPORT_H_

namespace eyedb {
  extern int databaseExport( Connection &conn, const char *dbname, const char *file);
  extern int databaseImport( Connection &conn, const char *dbname, const char *file, const char *filedir = 0, const char *mthdir = 0, bool listOnly = false);
}

#endif
