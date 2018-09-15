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


#ifndef _eyedb_idbbasep_
#define _eyedb_idbbasep_

#include <eyedb/base.h>
#include <eyedb/Error.h>
#include <eyedb/Oid.h>

typedef unsigned int TransactionId;

#define RPCSuccess ((RPCStatus)0)
#define eyedb_clear(X) memset(&(X), 0, sizeof(X))
#define eyedb_is_type(h, t) (((h).type & (t)) == (t))

#endif
