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


#ifndef _EYEDB_MISC_H
#define _EYEDB_MISC_H

#include <stdarg.h>

namespace eyedb {

  extern Bool isOidValid(const eyedbsm::Oid *);
  extern eyedbsm::Oid *oidInvalidate(eyedbsm::Oid *);
  extern const eyedbsm::Oid *getInvalidOid();

  extern Bool ObjectHeadCompare(const ObjectHeader *,
				const ObjectHeader *);

  extern Bool OidCompare(const eyedbsm::Oid *, const eyedbsm::Oid *);

  extern const char *OidGetString(const eyedbsm::Oid *);
  extern eyedbsm::Oid stringGetOid(const char *);

#define INDENT_INC 8

  extern char *make_indent(int);
  extern void delete_indent(char *);

  extern void dump_data(Data, Size);

  extern const char *makeName(const char *, const char *prefix);

  extern void tr(const char *, const char *, ...);

}

#endif
