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


#ifndef _eyedb_dbp_
#define _eyedb_dbp_

#define check_auth_st(u, p, msg, dbname) \
  if (!u) \
    u = Connection::getDefaultUser(); \
\
  if (!p) \
    p = Connection::getDefaultPasswd(); \
\
  if (!u || !p) \
    return Exception::make(IDB_AUTHENTICATION_NOT_SET, msg " %s", dbname);

#define check_auth(u, p, msg) check_auth_st(u, p, msg, name)

#define set_auth(u, p) \
if (_user != (u)) \
{ \
  free(_user); \
  _user = strdup(u); \
} \
 \
if (_passwd != (p)) \
{ \
  free(_passwd); \
  _passwd = strdup(p); \
}

#endif
