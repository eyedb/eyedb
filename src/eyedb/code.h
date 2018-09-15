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


#ifndef _EYEDB_CODE_H
#define _EYEDB_CODE_H

#define E_XDR

namespace eyedb {

  extern void
  char_code(Data *, Offset *, Size *, const char *),
    int16_code(Data *, Offset *, Size *, const eyedblib::int16 *),
    int32_code(Data *, Offset *, Size *, const eyedblib::int32 *),
    int64_code(Data *, Offset *, Size *, const eyedblib::int64 *),
    double_code(Data *, Offset *, Size *, const double *),
    string_code(Data *, Offset *, Size *, const char *),
    bound_string_code(Data *, Offset *, Size *, unsigned int,
		      const char *),
    buffer_code(Data *, Offset *, Size *, Data, Size);

#ifdef XDR_TRACE_OID
#define oid_code(DATA, OFFSET, SIZE, OID) \
 do { \
   fprintf(stdout, "Called from %s:%d\n", __FILE__, __LINE__); \
  _oid_code(DATA, OFFSET, SIZE, OID); \
   fprintf(stdout, "\n"); \
 } while(0)
  extern void
  _oid_code(Data *, Offset *, Size *, const eyedbsm::Oid *);
#else
  extern void
  oid_code(Data *, Offset *, Size *, const eyedbsm::Oid *);

#endif

  extern void
  char_decode(Data, Offset *, char *),
    int16_decode(Data, Offset *, eyedblib::int16 *),
    int32_decode(Data, Offset *, eyedblib::int32 *),
    int64_decode(Data, Offset *, eyedblib::int64 *),
    double_decode(Data, Offset *, double *),
    string_decode(Data, Offset *, char **),
    bound_string_decode(Data, Offset *, unsigned int, char **),
    buffer_decode(Data, Offset *, Data, Size);

  extern void
  oid_code(Data out_data, const Data in_data),
    int16_code(Data out_data, const Data in_data),
    int32_code(Data out_data, const Data in_data),
    int64_code(Data out_data, const Data in_data),
    double_code(Data out_data, const Data in_data);

#ifdef XDR_TRACE_OID
#define oid_decode(DATA, OFFSET, OID) \
 do { \
   fprintf(stdout, "Called from %s:%d\n", __FILE__, __LINE__); \
  _oid_decode(DATA, OFFSET, OID); \
   fprintf(stdout, "\n"); \
 } while(0)
  extern void
  _oid_decode(Data, Offset *, eyedbsm::Oid *);
#else
  extern void
  oid_decode(Data, Offset *, eyedbsm::Oid *);
#endif

  extern int
  object_header_code(Data *, Offset *, Size *, const ObjectHeader *),
    object_header_code_head(Data, const ObjectHeader *),
    object_header_decode(Data, Offset *, ObjectHeader *),
    object_header_decode_head(Data, ObjectHeader *);

  /* XDR */
#ifdef E_XDR
  extern void
  xdr_char_code(Data *, Offset *, Size *, const char *),
    xdr_int16_code(Data *, Offset *, Size *, const eyedblib::int16 *),
    xdr_int32_code(Data *, Offset *, Size *, const eyedblib::int32 *),
    xdr_int64_code(Data *, Offset *, Size *, const eyedblib::int64 *),
    xdr_double_code(Data *, Offset *, Size *, const double *),
    xdr_string_code(Data *, Offset *, Size *, const char *),
    xdr_bound_string_code(Data *, Offset *, Size *, unsigned int,
			  const char *),
    xdr_buffer_code(Data *, Offset *, Size *, Data, Size),
    xdr_oid_code(Data *, Offset *, Size *, const eyedbsm::Oid *);

  extern void
  xdr_char_decode(Data, Offset *, char *),
    xdr_int16_decode(Data, Offset *, eyedblib::int16 *),
    xdr_int32_decode(Data, Offset *, eyedblib::int32 *),
    xdr_int64_decode(Data, Offset *, eyedblib::int64 *),
    xdr_double_decode(Data, Offset *, double *),
    xdr_string_decode(Data, Offset *, char **),
    xdr_bound_string_decode(Data, Offset *, unsigned int, char **),
    xdr_buffer_decode(Data, Offset *, Data, Size),
    xdr_oid_decode(Data, Offset *, eyedbsm::Oid *);

  extern int
  xdr_object_header_code(Data *, Offset *, Size *, const ObjectHeader *),
    xdr_object_header_code_head(Data, const ObjectHeader *),
    xdr_object_header_decode(Data, Offset *, ObjectHeader *),
    xdr_object_header_decode_head(Data, ObjectHeader *);
#endif

  extern void br_code(const char *msg);
}

#endif
