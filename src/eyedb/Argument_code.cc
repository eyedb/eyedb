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


#include <assert.h>
#include "eyedb_p.h"
#include <eyedblib/rpc_be.h>

#define NO_DIRECT_SET // added the 22/09/98

// ---------------------------------------------------------------------------
//
// private functions
//
// ---------------------------------------------------------------------------

namespace eyedb {

#define UNKNOWN_TYPE(T, X) \
{ \
  fprintf(stderr, #X ": unknown argument type : %d\n", T); \
  abort(); \
}

static unsigned int
getSize(const ArgArray *);

static unsigned int
getSize(const Argument *arg)
{
  int type = arg->type->getType();
  unsigned int size = sizeof(int); // for arg type

  if (type == INT16_TYPE)
    size += sizeof(eyedblib::int16);
  else if (type == INT32_TYPE)
    size += sizeof(eyedblib::int32);
  else if (type == INT64_TYPE)
    size += sizeof(eyedblib::int64);
  else if (type == CHAR_TYPE)
    size += sizeof(char);
  else if (type == STRING_TYPE)
    size += sizeof(int) + strlen(arg->u.s)+1;
  else if (type == FLOAT_TYPE)
    size += sizeof(double);
  else if (type == OID_TYPE)
    size += sizeof(Oid);
  else if (type == OBJ_TYPE)
    size += sizeof(int) + (arg->u.o ? 2 * sizeof(Oid) +
			   arg->u.o->getIDRSize() : 0);
  else if (type == ARRAY_TYPE)
    size += getSize(arg->u.array);
  else if (type == RAW_TYPE)
    size += sizeof(int) + arg->u.raw.size;
  else if (type == (ARRAY_TYPE|INT16_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(eyedblib::int16) * arg->u.arr_i16.cnt;
    }
  else if (type == (ARRAY_TYPE|INT32_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(eyedblib::int32) * arg->u.arr_i32.cnt;
    }
  else if (type == (ARRAY_TYPE|INT64_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(eyedblib::int64) * arg->u.arr_i64.cnt;
    }
  else if (type == (ARRAY_TYPE|CHAR_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(char) * arg->u.arr_c.cnt;
    }
  else if (type == (ARRAY_TYPE|FLOAT_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(double) * arg->u.arr_d.cnt;
    }
  else if (type == (ARRAY_TYPE|STRING_TYPE))
    {
      size += sizeof(int); // cnt
      for (int i = 0; i < arg->u.arr_s.cnt; i++)
	size += sizeof(int) + strlen(arg->u.arr_s.s[i]) + 1;
    }
  else if (type == (ARRAY_TYPE|OBJ_TYPE))
    {
      size += sizeof(int); // cnt
      for (int i = 0; i < arg->u.arr_o.cnt; i++)
	size += sizeof(int) + (arg->u.arr_o.o[i] ? 2 * sizeof(Oid) +
			       arg->u.arr_o.o[i]->getIDRSize() : 0);
    }
  else if (type == (ARRAY_TYPE|OID_TYPE))
    {
      size += sizeof(int); // cnt
      size += sizeof(Oid) * arg->u.arr_oid.cnt;
    }
  else if (type == VOID_TYPE)
    ;
  else
    UNKNOWN_TYPE(type, getSize())

  return size;
}

static unsigned int
getSize(const ArgArray *array)
{
  int size = sizeof(int); // n
  int cnt = array->getCount();
  for (int i = 0; i < cnt; i++)
    size += getSize((*array)[i]);

  return size;
}

static void
code_arg(const ArgArray *array, char * &);

static void
code_arg(const char *s, char *&p)
{
  int len = strlen(s)+1;
  mcp(p, &len, sizeof(int));
  p += sizeof(int);
  mcp(p, s, len);
  p += len;
}

static void
code_arg(const Object *o, char *&p)
{
  int len = (o ? o->getIDRSize() : 0);
  mcp(p, &len, sizeof(int));
  p += sizeof(int);
  if (o)
    {
      Oid oid = o->getOid();
      mcp(p, &oid, sizeof(Oid));
      p += sizeof(Oid);
      oid = o->getClass()->getOid();
      mcp(p, &oid, sizeof(Oid));
      p += sizeof(Oid);
      mcp(p, o->getIDR(), o->getIDRSize());
      p += o->getIDRSize();
    }
}

#define CODE_SIMPLE_ARRAY(PTR, F, TYPE, P) \
{ \
  int len = sizeof(TYPE) * PTR.cnt; \
  mcp(P, &PTR.cnt, sizeof(int)); \
  P += sizeof(int); \
  mcp(P, PTR.F, len); \
  P += len; \
}

#define DECODE_SIMPLE_ARRAY(ARG, TYPE, P) \
{ \
  int cnt; \
  mcp(&cnt, P, sizeof(int)); \
  P += sizeof(int); \
 \
  TYPE *x = (TYPE *)malloc(sizeof(TYPE) * cnt); \
  for (int i = 0; i < cnt; i++) \
    { \
      mcp(&x[i], P, sizeof(TYPE)); \
      P += sizeof(TYPE); \
    } \
  ARG->set(x, cnt, Argument::AutoFullGarbage); \
}

static void
code_arg(const Argument *arg, char * &p)
{
  int type = arg->type->getType();
  mcp(p, &type, sizeof(int));

  p += sizeof(int);

  if (type == INT16_TYPE)
    {
      mcp(p, &arg->u.i16, sizeof(eyedblib::int16));
      p += sizeof(eyedblib::int16);
    }
  else if (type == INT32_TYPE)
    {
      mcp(p, &arg->u.i32, sizeof(eyedblib::int32));
      p += sizeof(eyedblib::int32);
    }
  else if (type == INT64_TYPE)
    {
      mcp(p, &arg->u.i64, sizeof(eyedblib::int64));
      p += sizeof(eyedblib::int64);
    }
  else if (type == CHAR_TYPE)
    {
      mcp(p, &arg->u.c, sizeof(char));
      p += sizeof(char);
    }
  else if (type == STRING_TYPE)
    code_arg(arg->u.s, p);
  else if (type == RAW_TYPE)
    {
      mcp(p, &arg->u.raw.size, sizeof(int));
      p += sizeof(int);
      mcp(p, arg->u.raw.data, arg->u.raw.size);
      p += arg->u.raw.size;
    }
  else if (type == FLOAT_TYPE)
    {
      mcp(p, &arg->u.d, sizeof(double));
      p += sizeof(double);
    }
  else if (type == OID_TYPE)
    {
      mcp(p, arg->u.oid, sizeof(Oid));
      p += sizeof(Oid);
    }
  else if (type == OBJ_TYPE)
    code_arg(arg->u.o, p);
  else if (type == ARRAY_TYPE)
    code_arg(arg->u.array, p);
  else if (type == (ARRAY_TYPE|INT16_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_i16, i, eyedblib::int16, p);
    }
  else if (type == (ARRAY_TYPE|INT32_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_i32, i, eyedblib::int32, p);
    }
  else if (type == (ARRAY_TYPE|INT64_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_i64, i, eyedblib::int64, p);
    }
  else if (type == (ARRAY_TYPE|CHAR_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_c, c, char, p);
    }
  else if (type == (ARRAY_TYPE|FLOAT_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_d, d, double, p);
    }
  else if (type == (ARRAY_TYPE|OID_TYPE))
    {
      CODE_SIMPLE_ARRAY(arg->u.arr_oid, oid, Oid, p);
    }
  else if (type == (ARRAY_TYPE|STRING_TYPE))
    {
      mcp(p, &arg->u.arr_s.cnt, sizeof(int));
      p += sizeof(int);

      for (int i = 0; i < arg->u.arr_s.cnt; i++)
	code_arg(arg->u.arr_s.s[i], p);
    }
  else if (type == (ARRAY_TYPE|OBJ_TYPE))
    {
      mcp(p, &arg->u.arr_o.cnt, sizeof(int));
      p += sizeof(int);

      for (int i = 0; i < arg->u.arr_o.cnt; i++)
	code_arg(arg->u.arr_o.o[i], p);
    }
  else if (type == VOID_TYPE)
    ;
  else
    UNKNOWN_TYPE(type, code_arg)
}

static void
code_arg(const ArgArray *array, char * &p)
{
  int n = array->getCount();
  mcp(p, &n, sizeof(int));
  p += sizeof(int);

  for (int i = 0; i < n; i++)
    code_arg((*array)[i], p);
}

static void
decode_arg(Database *, ArgArray *array, const char * &);

static void
decode_arg(Database *db, Object *&o, const Class *&cls,
	   const char *&p)
{
  int len;
  mcp(&len, p, sizeof(int));
  p += sizeof(int);

  if (!len)
    {
      o = NULL;
      return;
    }

  Oid o_oid, cl_oid;
  mcp(&o_oid, p, sizeof(Oid));
  p += sizeof(Oid);
  mcp(&cl_oid, p, sizeof(Oid));
  p += sizeof(Oid);

  p += len;

  cls = db->getSchema()->getClass(cl_oid);

  if (!cls)
    {
      o = NULL;
      return;
    }

  o = cls->newObj((Data)p + IDB_OBJ_HEAD_SIZE);

  if (o)
    ObjectPeer::setOid(o, o_oid);
}

static void
decode_arg(Database *db, Argument *arg, const char * &p)
{
  int type;
  
  mcp(&type, p, sizeof(int));
  p += sizeof(int);
  
  //  arg->type->setType(type);
  
  if (type == INT16_TYPE)
    {
      eyedblib::int16 i;
      mcp(&i, p, sizeof(eyedblib::int16));
      arg->set(i);
      p += sizeof(eyedblib::int16);
    }
  else if (type == INT32_TYPE)
    {
      eyedblib::int32 i;
      mcp(&i, p, sizeof(eyedblib::int32));
      arg->set(i);
      p += sizeof(eyedblib::int32);
    }
  else if (type == INT64_TYPE)
    {
      eyedblib::int64 i;
      mcp(&i, p, sizeof(eyedblib::int64));
      arg->set(i);
      p += sizeof(eyedblib::int64);
    }
  else if (type == CHAR_TYPE)
    {
      char c;
      mcp(&c, p, sizeof(char));
      arg->set(c);
      p += sizeof(char);
    }
  else if (type == STRING_TYPE)
    {
      int len;
      mcp(&len, p, sizeof(int));
      p += sizeof(int);
      //arg->set((char *)p);
      //changed the 15/01/99: added strdup
      arg->set(strdup((char *)p), Argument::AutoFullGarbage);
      p += len;
    }
  else if (type == FLOAT_TYPE)
    {
      double d;
      mcp(&d, p, sizeof(double));
      arg->set(d);
      p += sizeof(double);
    }
  else if (type == OID_TYPE)
    {
      Oid oid;
      mcp(&oid, p, sizeof(Oid));
      arg->set(oid, db);
      p += sizeof(Oid);
    }
  else if (type == RAW_TYPE)
    {
      int size;
      mcp(&size, p, sizeof(int));
      p += sizeof(int);
      // see changed 15/01/99: should do the same thing, i.e. copy
      // of the raw buffer!
      arg->set(Argument::dup((const unsigned char *)p, size), size,
	       Argument::AutoFullGarbage);
      p += size;
    }
  else if (type == OBJ_TYPE)
    {
      Object *o = NULL;
      const Class *cls = NULL;
      decode_arg(db, o, cls, p);
      arg->set(o, Argument::AutoFullGarbage);
      if (cls)
	arg->type->setClname(cls->getName());
    }
  else if (type == (OBJ_TYPE|ARRAY_TYPE))
    {
      int cnt;
      mcp(&cnt, p, sizeof(int));
      p += sizeof(int);
      Object **o = (Object **)malloc(sizeof(Object *) * cnt);

      const Class *cls = NULL;
      for (int i = 0; i < cnt; i++)
	decode_arg(db, o[i], cls, p);

      arg->set(o, cnt, Argument::AutoFullGarbage);
    }
  else if (type == (INT16_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, eyedblib::int16, p);
    }
  else if (type == (INT32_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, eyedblib::int32, p);
    }
  else if (type == (INT64_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, eyedblib::int64, p);
    }
  else if (type == (CHAR_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, char, p);
    }
  else if (type == (STRING_TYPE|ARRAY_TYPE))
    {
      int cnt;
      mcp(&cnt, p, sizeof(int));
      p += sizeof(int);

      char **s = (char **)malloc(sizeof(char *) * cnt);
      for (int i = 0; i < cnt; i++)
	{
	  int len;
	  mcp(&len, p, sizeof(int));
	  p += sizeof(int);
	  s[i] = strdup((char *)p);
	  p += len;
	}
      arg->set(s, cnt, Argument::AutoFullGarbage);
    }
  else if (type == (FLOAT_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, double, p);
    }
  else if (type == (OID_TYPE|ARRAY_TYPE))
    {
      DECODE_SIMPLE_ARRAY(arg, Oid, p);
    }
  else if (type == ARRAY_TYPE)
    {
      int n;
      mcp(&n, p, sizeof(int));
      p += sizeof(int);
      ArgArray *array = new ArgArray(n, Argument::AutoFullGarbage);
      arg->set(array, Argument::AutoFullGarbage);
      decode_arg(db, (ArgArray *)array, p);
    }
  else if (type == VOID_TYPE)
    ;
  else
    UNKNOWN_TYPE(type, decode_arg)
}

static void
decode_arg(Database *db, ArgArray *array, const char * &p)
{
  int cnt = array->getCount();
  for (int i = 0; i < cnt; i++)
    decode_arg(db, (Argument *)(*array)[i], p);
}

// ---------------------------------------------------------------------------
//
// exported functions
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//
// argument array coding
//
// ---------------------------------------------------------------------------

void
code_arg_array(void *xdata, const void *xargarray)
{
  rpc_Data *data = (rpc_Data *)xdata;
  const ArgArray *argarray = (const ArgArray *)xargarray;
  unsigned int size = sizeof(int);

  int cnt = argarray->getCount();
  int i;
  for (i = 0; i < cnt; i++)
    {
      const Argument *arg = (*argarray)[i];
      size += getSize(arg);
    }

  data->size = size;
  data->data = malloc(data->size);
  data->status = rpc_TempDataUsed;

  char *p = (char *)data->data;
  mcp(p, &cnt, sizeof(int));

  p += sizeof(int);

  for (i = 0; i < cnt; i++)
    {
      const Argument *arg = (*argarray)[i];
      code_arg(arg, p);
    }
}

int
decode_arg_array(void *xdb, const void *xdata, void **pargarray,
		     Bool allocate)
{
  Database *db = (Database *)xdb;
  rpc_Data *data = (rpc_Data *)xdata;
  const char *p = (const char *)data->data;
  int cnt;
  mcp(&cnt, p, sizeof(int));
  p += sizeof(int);

  ArgArray *argarray;
  if (allocate)
    {
/*
      Argument *args = new Argument[cnt];
      argarray = new ArgArray(args, cnt, Argument::FullGarbage);
*/
      argarray = new ArgArray(cnt, Argument::AutoFullGarbage);
      *(ArgArray **)pargarray = argarray;
    }
  else
    argarray = (ArgArray *)pargarray;

  for (int i = 0; i < cnt; i++)
    {
      Argument *arg = (*argarray)[i];
      decode_arg(db, arg, p);
    }

  //  return (data->size - (int)(p - (char *)data->data));
  return (int)(p - (char *)data->data);
}

// ---------------------------------------------------------------------------
//
// argument coding
//
// ---------------------------------------------------------------------------

void
code_argument(void *xdata, const void *xarg)
{
  rpc_Data *data = (rpc_Data *)xdata;
  const Argument *arg = (const Argument *)xarg;
  int osize = data->size;

  data->size += getSize(arg);
  data->data = realloc(data->data, data->size);
  data->status = rpc_TempDataUsed;
  char *p = ((char *)data->data) + osize;

  code_arg(arg, p);
}

void
decode_argument(void *xdb, const void *xdata, void *xarg, int offset)
{
  Database *db = (Database *)xdb;
  rpc_Data *data = (rpc_Data *)xdata;

  Argument *arg = (Argument *)xarg;
  const char *p = ((const char *)data->data) + offset;

  decode_arg(db, arg, p);
}


// ---------------------------------------------------------------------------
//
// signature coding
//
// ---------------------------------------------------------------------------

void
code_signature(void *xdata, const void *xsign)
{
  rpc_Data *data = (rpc_Data *)xdata;
  const Signature *sign = (const Signature *)xsign;
  int cnt = sign->getNargs();

  data->size = sizeof(int) * (cnt + 2);
  data->data = malloc(data->size);

  char *p = (char *)data->data;
  int type = sign->getRettype()->getType();

  mcp(p, &type, sizeof(int));
  p += sizeof(int);
  mcp(p, &cnt, sizeof(int));
  p += sizeof(int);

  for (int i = 0; i < cnt; i++)
    {
      type = sign->getTypes(i)->getType();
      mcp(p, &type, sizeof(int));
      p += sizeof(int);
      if (type == OBJ_TYPE)
	{
	  const char *s = sign->getTypes(i)->getClname().c_str();
	  int len = strlen(sign->getTypes(i)->getClname().c_str());
	  mcp(p, &len, sizeof(int));
	  p += sizeof(int);
	  mcp(p, s, len);
	  p += len;
	}
    }

  assert(data->size == (int)(p - (char *)data->data));
}

void
decode_signature(const void *xdata, void *xsign)
{
  rpc_Data *data = (rpc_Data *)xdata;
  const char *p = (const char *)data->data;
  Signature *sign = (Signature *)xsign;
  ArgType *type;

  int _type;
  mcp(&_type, p, sizeof(int));
  p += sizeof(int);
#ifdef NO_DIRECT_SET
  type = sign->getRettype();
#else
  type = new ArgType();
#endif

  type->setType((ArgType_Type)_type, False);

#ifndef NO_DIRECT_SET
  sign->setRettype(type);
#endif

  int cnt;
  mcp(&cnt, p, sizeof(int));
  p += sizeof(int);
  sign->setNargs(cnt);

#ifdef NO_DIRECT_SET
  sign->setTypesCount(cnt);
#endif

  for (int i = 0; i < cnt; i++)
    {
      mcp(&_type, p, sizeof(int));
      p += sizeof(int);
#ifdef NO_DIRECT_SET
      type = sign->getTypes(i);
#else
      type = new ArgType();
#endif
      type->setType((ArgType_Type)_type, False);
      if (_type == OBJ_TYPE)
	{
	  int len;
	  mcp(&len, p, sizeof(int));
	  p += sizeof(int);
	  char *s = (char *)malloc(len+1);
	  mcp(s, p, len);
	  p += len;
	  type->setClname(s);
	  free(s);
	}

#ifndef NO_DIRECT_SET
      sign->setTypes(i, type);
#endif
    }

  assert(data->size == (int)(p - (char *)data->data));
}
}
