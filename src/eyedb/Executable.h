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


#ifndef _EYEDB_EXECUTABLE_H
#define _EYEDB_EXECUTABLE_H

namespace eyedb {

  extern Status
  eyedb_CHECKObjArrayType(Database *db, Argument &, const char *);

  extern Status
  eyedb_CHECKObjRefType(Database *db, Argument &arg, const char *);

  extern Status
  eyedb_CHECKObjType(Database *db, Argument &arg, const char *);

  extern Status
  eyedb_CHECKArgument(Database *db, ArgType t1, Argument &arg,
		      const char *typname, const char *name, const char *which,
		      int inout = 0);

  extern Status
  eyedb_CHECKArguments(Database *db, const Signature *sign,
		       ArgArray &array, const char *typname,
		       const char *name, int inout);

  extern Status MethodLaunch
  (Status (*method)(Database *, Method *, Object *,
		    ArgArray &, Argument &),
   Database *db,
   Method *mth,
   Object *o,
   ArgArray &argarray,
   Argument &retarg);

}

#endif
