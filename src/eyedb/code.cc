/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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


#include "base_p.h"
#include "eyedb/internals/ObjectHeader.h"
#include "code.h"
#include "eyedb/internals/kern_const.h"
#include <stdlib.h>
#include <eyedblib/rpc_lib.h>
#include <eyedblib/xdr.h>
#include <eyedbsm/xdr.h>

#define mcp(D, S, N) \
{ \
  int __n__ = (N); \
  char *__d__ = (char *)(D), *__s__ = (char *)(S); \
  while(__n__--) \
    *__d__++ = *__s__++; \
}

#define mset(D, V, N) \
{ \
  int __n__ = (N); \
  char *__d__ = (char *)(D); \
  while(__n__--) \
    *__d__++ = V; \
}
/*#define memcpy mcp*/

namespace eyedb {

  static int NUM = 1;
  void br_code(const char *msg)
  {
    fprintf(stdout, "BR_CODE: %s\n", msg);
    fflush(stdout);
  }

  static void
  inc_size(Data* idr, Size wsize, Size *alloc_size)
  {
#define INC 64
    if (!*alloc_size) {
      *alloc_size = wsize + INC;
      *idr = (Data)malloc(*alloc_size);
    }
    else if (*alloc_size < wsize) {
      *alloc_size = wsize + INC;
      *idr = (Data)realloc(*idr, *alloc_size);
    }
  }

#define BIG_TRY

  void
  char_code(Data* idr, Offset* offset, Size* alloc_size, const char *k)
  {
    inc_size(idr, (*offset) + sizeof(char), alloc_size);
    mcp((*idr) + (*offset), k, sizeof(char));
    *offset += sizeof(char);
  }

  void
  int16_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int16 *k)
  {
#ifdef BIG_TRY
    xdr_int16_code(idr, offset, alloc_size, k);
#else
    inc_size(idr, (*offset) + sizeof(eyedblib::int16), alloc_size);
    mcp((*idr) + (*offset), k, sizeof(eyedblib::int16));
    *offset += sizeof(eyedblib::int16);
#endif
  }

  void
  int32_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int32 *k)
  {
#ifdef BIG_TRY
    xdr_int32_code(idr, offset, alloc_size, k);
#else
    inc_size(idr, (*offset) + sizeof(eyedblib::int32), alloc_size);
    mcp((*idr) + (*offset), k, sizeof(eyedblib::int32));
    *offset += sizeof(eyedblib::int32);
#endif
  }

  void
  int64_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int64 *k)
  {
#ifdef BIG_TRY
    xdr_int64_code(idr, offset, alloc_size, k);
#else
    inc_size(idr, (*offset) + sizeof(eyedblib::int64), alloc_size);
    mcp((*idr) + (*offset), k, sizeof(eyedblib::int64));
    *offset += sizeof(eyedblib::int64);
#endif
  }

  void
  double_code(Data* idr, Offset* offset, Size* alloc_size, const double *k)
  {
#ifdef BIG_TRY
    xdr_double_code(idr, offset, alloc_size, k);
#else
    inc_size(idr, (*offset) + sizeof(double), alloc_size);
    mcp((*idr) + (*offset), k, sizeof(double));
    *offset += sizeof(double);
#endif
  }

  void
  string_code(Data* idr, Offset* offset, Size* alloc_size, const char *s)
  {
#ifdef BIG_TRY
    xdr_string_code(idr, offset, alloc_size, s);
#else
    int len = strlen(s)+1;
    int32_code(idr, offset, alloc_size, &len);
    inc_size(idr, (*offset) + len, alloc_size);
    mcp((*idr) + (*offset), s, len);
    *offset += len;
#endif
  }

  void
  bound_string_code(Data* idr, Offset* offset, Size* alloc_size,
		    unsigned int len, const char *s)
  {
#ifdef BIG_TRY
    xdr_bound_string_code(idr, offset, alloc_size, len, s);
#else
    int slen = (s ? strlen(s)+1 : 0);
    inc_size(idr, (*offset) + len, alloc_size);
    mcp((*idr) + (*offset), s, (slen < len ? slen : len));
    *offset += len;
#endif
  }

  void
  buffer_code(Data* idr, Offset* offset, Size* alloc_size,
	      Data data, Size size)
  {
    inc_size(idr, (*offset) + size, alloc_size);
    memcpy((*idr) + (*offset), data, size);
    *offset += size;
  }

#ifdef XDR_TRACE_OID
  void
  _oid_code(Data* idr, Offset* offset, Size* alloc_size, const eyedbsm::Oid *oid)
#else
    void
  oid_code(Data* idr, Offset* offset, Size* alloc_size, const eyedbsm::Oid *oid)
#endif
  {
#ifdef BIG_TRY
    xdr_oid_code(idr, offset, alloc_size, oid);
#else
    inc_size(idr, (*offset) + sizeof(eyedbsm::Oid), alloc_size);
    mcp((*idr) + (*offset), oid, sizeof(eyedbsm::Oid));
    *offset += sizeof(eyedbsm::Oid);
#endif
  }

  void
  oid_code(Data out_data, const Data in_data) {
    eyedbsm::Oid oid;
    memcpy(&oid, in_data, sizeof(oid));

    Offset offset = 0;
    Size alloc_size = sizeof(oid);
    oid_code(&out_data, &offset, &alloc_size, &oid);
  }

  void
  int16_code(Data out_data, const Data in_data) {
    eyedblib::int16 i;
    memcpy(&i, in_data, sizeof(i));

    Offset offset = 0;
    Size alloc_size = sizeof(i);
    int16_code(&out_data, &offset, &alloc_size, &i);
  }

  void
  int32_code(Data out_data, const Data in_data) {
    eyedblib::int32 i;
    memcpy(&i, in_data, sizeof(i));

    Offset offset = 0;
    Size alloc_size = sizeof(i);
    int32_code(&out_data, &offset, &alloc_size, &i);
  }

  void
  int64_code(Data out_data, const Data in_data) {
    eyedblib::int64 i;
    memcpy(&i, in_data, sizeof(i));

    Offset offset = 0;
    Size alloc_size = sizeof(i);
    int64_code(&out_data, &offset, &alloc_size, &i);
  }

  void
  double_code(Data out_data, const Data in_data) {
    eyedblib::float64 d;
    memcpy(&d, in_data, sizeof(d));

    Offset offset = 0;
    Size alloc_size = sizeof(d);
    double_code(&out_data, &offset, &alloc_size, &d);
  }

  int
  object_header_code(Data* idr, Offset* offset, Size* alloc_size,
		     const ObjectHeader *hdr)
  {
    eyedblib::int32 magic = IDB_OBJ_HEAD_MAGIC;

    inc_size(idr, (*offset) + IDB_OBJ_HEAD_SIZE, alloc_size);
    int32_code(idr, offset, alloc_size, &magic);
    int32_code(idr, offset, alloc_size, &hdr->type);
    int32_code(idr, offset, alloc_size, &hdr->size);
    int64_code(idr, offset, alloc_size, &hdr->ctime);
    int64_code(idr, offset, alloc_size, &hdr->mtime);
    int32_code(idr, offset, alloc_size, &hdr->xinfo);
    oid_code(idr, offset, alloc_size, &hdr->oid_cl);
    oid_code(idr, offset, alloc_size, &hdr->oid_prot);
    return 1;
  }

  int
  object_header_code_head(Data idr, const ObjectHeader *hdr)
  {
    Offset offset = 0;
    Size alloc_size = IDB_OBJ_HEAD_SIZE;

    return object_header_code(&idr, &offset, &alloc_size, hdr);
  }

  void
  char_decode(Data idr, Offset* offset, char *k)
  {
    mcp(k, idr + (*offset), sizeof(char));
    *offset += sizeof(char);
  }

  void
  int16_decode(Data idr, Offset* offset, eyedblib::int16 *k)
  {
#ifdef BIG_TRY
    xdr_int16_decode(idr, offset, k);
#else
    mcp(k, idr + (*offset), sizeof(eyedblib::int16));
    *offset += sizeof(eyedblib::int16);
#endif
  }

  void
  int32_decode(Data idr, Offset* offset, eyedblib::int32 *k)
  {
#ifdef BIG_TRY
    xdr_int32_decode(idr, offset, k);
#else
    mcp(k, idr + (*offset), sizeof(eyedblib::int32));
    *offset += sizeof(eyedblib::int32);
#endif
  }

  void
  int64_decode(Data idr, Offset* offset, eyedblib::int64 *k)
  {
#ifdef BIG_TRY
    xdr_int64_decode(idr, offset, k);
#else
    mcp(k, idr + (*offset), sizeof(eyedblib::int64));
    *offset += sizeof(eyedblib::int64);
#endif
  }

  void
  double_decode(Data idr, Offset* offset, double *k)
  {
#ifdef BIG_TRY
    xdr_double_decode(idr, offset, k);
#else
    mcp(k, idr + (*offset), sizeof(double));
    *offset += sizeof(double);
#endif
  }

  void
  string_decode(Data idr, Offset* offset, char **s)
  {
#ifdef BIG_TRY
    xdr_string_decode(idr, offset, s);
#else
    eyedblib::int32 len;
    int32_decode(idr, offset, &len);
    *s = (char *)(idr + *offset);
    *offset += len;
#endif
  }

  void
  bound_string_decode(Data idr, Offset* offset, unsigned int len,
		      char **s)
  {
#ifdef BIG_TRY
    xdr_bound_string_decode(idr, offset, len, s);
#else
    if (s) *s = (char *)(idr + *offset);
    *offset += len;
#endif
  }

  void
  buffer_decode(Data idr, Offset* offset, Data data, Size size)
  {
    memcpy(data, idr + (*offset), size);
    *offset += size;
  }

#ifdef XDR_TRACE_OID
  void
  _oid_decode(Data idr, Offset* offset, eyedbsm::Oid *oid)
#else
    void
  oid_decode(Data idr, Offset* offset, eyedbsm::Oid *oid)
#endif
  {
#ifdef BIG_TRY
    xdr_oid_decode(idr, offset, oid);
#else
    mcp(oid, idr + (*offset), sizeof(eyedbsm::Oid));
    *offset += sizeof(eyedbsm::Oid);
#endif
  }

  int
  object_header_decode(Data idr, Offset* offset, ObjectHeader *hdr)
  {
#ifdef BIG_TRY
    xdr_object_header_decode(idr, offset, hdr);
#else
    int xoffset;

    int32_decode(idr, offset, &hdr->magic);

    if (hdr->magic != IDB_OBJ_HEAD_MAGIC)
      return 0;

    xoffset = *offset;
    mcp(&hdr->type, idr + xoffset, sizeof(eyedblib::int32));
    xoffset += sizeof(eyedblib::int32);
    mcp(&hdr->size, idr + xoffset, sizeof(eyedblib::int32));
    xoffset += sizeof(eyedblib::int32);
    mcp(&hdr->ctime, idr + xoffset, sizeof(eyedblib::int64));
    xoffset += sizeof(eyedblib::int64);
    mcp(&hdr->mtime, idr + xoffset, sizeof(eyedblib::int64));
    xoffset += sizeof(eyedblib::int64);
    mcp(&hdr->xinfo, idr + xoffset, sizeof(eyedblib::int32));
    xoffset += sizeof(eyedblib::int32);
    *offset = xoffset;
    oid_decode(idr, offset, &hdr->oid_cl);
    oid_decode(idr, offset, &hdr->oid_prot);
#endif

    return 1;
  }

  int
  object_header_decode_head(Data idr, ObjectHeader *hdr)
  {
#ifdef BIG_TRY
    return xdr_object_header_decode_head(idr, hdr);
#else
    Offset offset = 0;
    return object_header_decode(idr, &offset, hdr);
#endif
  }

  /* XDR */

#ifdef E_XDR
  void
  xdr_char_code(Data* idr, Offset* offset, Size* alloc_size, const char *k)
  {
    char_code(idr, offset, alloc_size, k);
  }

  void
  xdr_int16_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int16 *k)
  {
    inc_size(idr, (*offset) + sizeof(eyedblib::int16), alloc_size);
    h2x_16_cpy((*idr) + (*offset), k);
    *offset += sizeof(eyedblib::int16);
  }

  void
  xdr_int32_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int32 *k)
  {
    inc_size(idr, (*offset) + sizeof(eyedblib::int32), alloc_size);
    h2x_32_cpy((*idr) + (*offset), k);
    *offset += sizeof(eyedblib::int32);
  }

  void
  xdr_int64_code(Data* idr, Offset* offset, Size* alloc_size, const eyedblib::int64 *k)
  {
    inc_size(idr, (*offset) + sizeof(eyedblib::int64), alloc_size);
    h2x_64_cpy((*idr) + (*offset), k);
    *offset += sizeof(eyedblib::int64);
  }

  void
  xdr_double_code(Data* idr, Offset* offset, Size* alloc_size, const double *k)
  {
    inc_size(idr, (*offset) + sizeof(double), alloc_size);
    h2x_64_cpy((*idr) + (*offset), k);
    *offset += sizeof(double);
  }

  void
  xdr_string_code(Data* idr, Offset* offset, Size* alloc_size, const char *s)
  {
    int len = strlen(s)+1;
    xdr_int32_code(idr, offset, alloc_size, &len);
    inc_size(idr, (*offset) + len, alloc_size);
    mcp((*idr) + (*offset), s, len);
    *offset += len;
  }

  void
  xdr_bound_string_code(Data* idr, Offset* offset, Size* alloc_size,
			unsigned int len, const char *s)
  {
    int slen = (s ? strlen(s)+1 : 0);
    inc_size(idr, (*offset) + len, alloc_size);
    mcp((*idr) + (*offset), s, (slen < len ? slen : len));
    *offset += len;
  }

  void
  xdr_buffer_code(Data* idr, Offset* offset, Size* alloc_size,
		  Data data, Size size)
  {
    buffer_code(idr, offset, alloc_size, data, size);
  }

  void
  xdr_oid_code(Data* idr, Offset* offset, Size* alloc_size, const eyedbsm::Oid *oid)
  {
    inc_size(idr, (*offset) + sizeof(eyedbsm::Oid), alloc_size);
#ifdef XDR_TRACE_OID
    fprintf(stdout, "%d: oid_code %s -> ", NUM++, se_getOidString(oid));
    fflush(stdout);
    if (oid->dbid > 3)
      br_code("oid_code");
#endif
    eyedbsm::h2x_oid((*idr) + (*offset), oid);
    eyedbsm::Oid toid;
    memcpy(&toid, (*idr) + (*offset), sizeof(eyedbsm::Oid));
#ifdef XDR_TRACE_OID
    fprintf(stdout, "[%s]\n", se_getOidString(&toid));
#endif
    *offset += sizeof(eyedbsm::Oid);
  }

#if 0
  int
  xdr_object_header_code(Data* idr, Offset* offset, Size* alloc_size,
			 const ObjectHeader *hdr)
  {
    return object_header_code(idr, offset, alloc_size, hdr);
  }

  int
  xdr_object_header_code_head(Data idr, const ObjectHeader *hdr)
  {
    return object_header_code_head(idr, hdr);
  }
#endif

  /* decoding */
#if 0
  void
  xdr_char_decode(Data idr, Offset* offset, char *k)
  {
    char_decode(idr, offset, k);
  }
#endif

  void
  xdr_int16_decode(Data idr, Offset* offset, eyedblib::int16 *k)
  {
    x2h_16_cpy(k, idr + (*offset));
    *offset += sizeof(eyedblib::int16);
  }

  void
  xdr_int32_decode(Data idr, Offset* offset, eyedblib::int32 *k)
  {
    x2h_32_cpy(k, idr + (*offset));
    *offset += sizeof(eyedblib::int32);
  }

  void
  xdr_int64_decode(Data idr, Offset* offset, eyedblib::int64 *k)
  {
    x2h_64_cpy(k, idr + (*offset));
    *offset += sizeof(eyedblib::int64);
  }

  void
  xdr_double_decode(Data idr, Offset* offset, double *k)
  {
    x2h_f64_cpy(k, idr + (*offset));
    *offset += sizeof(double);
  }

  void
  xdr_string_decode(Data idr, Offset* offset, char **s)
  {
    eyedblib::int32 len;
    xdr_int32_decode(idr, offset, &len);
    *s = (char *)(idr + *offset);
    *offset += len;
  }

  void
  xdr_bound_string_decode(Data idr, Offset* offset, unsigned int len,
			  char **s)
  {
    if (s) *s = (char *)(idr + *offset);
    *offset += len;
  }

  void
  xdr_buffer_decode(Data idr, Offset* offset, Data data, Size size)
  {
    memcpy(data, idr + (*offset), size);
    *offset += size;
  }

  void
  xdr_oid_decode(Data idr, Offset* offset, eyedbsm::Oid *oid)
  {
    eyedbsm::Oid toid;
    memcpy(&toid, idr + (*offset), sizeof(eyedbsm::Oid));
#ifdef XDR_TRACE_OID
    fprintf(stdout, "%d: oid_decode [%s] -> ", NUM++, se_getOidString(&toid));
#endif
    eyedbsm::x2h_oid(oid, idr + (*offset));
#ifdef XDR_TRACE_OID
    fprintf(stdout, "%s\n", se_getOidString(oid));
    fflush(stdout);
    if (oid->dbid > 3)
      br_code("oid_decode");
#endif
    *offset += sizeof(eyedbsm::Oid);
  }

  int
  xdr_object_header_decode(Data idr, Offset* offset, ObjectHeader *hdr)
  {
    int xoffset;
    int32_decode(idr, offset, &hdr->magic);

    if (hdr->magic != IDB_OBJ_HEAD_MAGIC) {
      //fprintf(stdout, "MAGIC HEADER: %x\n", hdr->magic);
      return 0;
    }

    int32_decode(idr, offset, &hdr->type);
    int32_decode(idr, offset, &hdr->size);
    int64_decode(idr, offset, &hdr->ctime);
    int64_decode(idr, offset, &hdr->mtime);
    int32_decode(idr, offset, &hdr->xinfo);
    oid_decode(idr, offset, &hdr->oid_cl);
    oid_decode(idr, offset, &hdr->oid_prot);

    return 1;
  }

  int
  xdr_object_header_decode_head(Data idr, ObjectHeader *hdr)
  {
    Offset offset = 0;
    Size alloc_size = IDB_OBJ_HEAD_SIZE;

    return xdr_object_header_decode(idr, &offset, hdr);
  }

}

#endif

