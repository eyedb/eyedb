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


#ifndef _EYEDB_ARGUMENT_CODE_H
#define _EYEDB_ARGUMENT_CODE_H

namespace eyedb {

  extern void code_arg_array(void *data, const void *argarray);
  extern int decode_arg_array(void *db, const void *data,
				  void **argarray, Bool allocate);

  extern void code_argument(void *data, const void *arg);
  extern void decode_argument(void *, const void *data, void *arg,
				  int offset);

  extern void code_signature(void *data, const void *sign);
  extern void decode_signature(const void *data, void *sign);

}

#endif
