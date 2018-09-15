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


#include "eyedb_p.h"
#include "odl.h"

#define NEW_ALLOC_POLICY

//
// Argument
//

namespace eyedb {

  static ArgType *types[BYTE_TYPE+1];
  static ArgType *arrtypes[BYTE_TYPE+1];

  inline static void
  make_type(ArgType *types[], ArgType_Type idx)
  {
    types[idx] = new ArgType();
    types[idx]->setType(idx, False);
    types[idx]->lock();

    arrtypes[idx] = new ArgType();
    arrtypes[idx]->setType((ArgType_Type)(idx|ARRAY_TYPE), False);
    arrtypes[idx]->lock();
  }

  static void
  init_types()
  {
    make_type(types, ANY_TYPE);
    make_type(types, INT16_TYPE);
    make_type(types, INT32_TYPE);
    make_type(types, INT64_TYPE);
    make_type(types, STRING_TYPE);
    make_type(types, CHAR_TYPE);
    make_type(types, FLOAT_TYPE);
    make_type(types, OID_TYPE);
    make_type(types, RAW_TYPE);
    make_type(types, BYTE_TYPE);

    types[OBJ_TYPE] = 0;
  }

  static ArgType *
  getType(ArgType_Type _type)
  {
    if (!types[0]) init_types();
    ArgType *type = (_type & ARRAY_TYPE) ? arrtypes[_type&~ARRAY_TYPE] :
      types[_type];
    if (type) {
      assert(!(type->getType() & INOUT_ARG_TYPE));
      return type;
    }
    type = new ArgType();
    type->setType(_type, False);
    return type;
  }

  void Argument::init(ArgType_Type _type)
  {
#ifdef NEW_ALLOC_POLICY
    type = eyedb::getType(_type);
#else
    type = new ArgType();
    type->setType(_type, False);
#endif
    str = (char *)0;
    policy = Argument::NoGarbage;
    db = (Database *)0;
    //  printf("Argument::init(%p, %p)\n", this, type);
  }

  //
  // Constructors
  //

  // basic types : without copy and without garbage

  Argument::Argument()
  {
    init(VOID_TYPE);
  }

  Argument::Argument(eyedblib::int16 _i)
  {
    init(VOID_TYPE);
    set(_i);
  }

  Argument::Argument(eyedblib::int32 _i)
  {
    init(VOID_TYPE);
    set(_i);
  }

  Argument::Argument(eyedblib::int64 _i)
  {
    init(VOID_TYPE);
    set(_i);
  }

  Argument::Argument(const char *_s)
  {
    init(VOID_TYPE);
    set(_s);
  }

  Argument::Argument(char _c)
  {
    init(VOID_TYPE);
    set(_c);
  }

  Argument::Argument(unsigned char _by)
  {
    init(VOID_TYPE);
    set(_by);
  }

  Argument::Argument(double _d)
  {
    init(VOID_TYPE);
    set(_d);
  }

  Argument::Argument(const Oid &_oid, Database *_db)
  {
    init(VOID_TYPE);
    set(_oid, _db);
  }

  Argument::Argument(const Object *_o)
  {
    init(VOID_TYPE);
    set(_o);
  }

  Argument::Argument(const unsigned char *_raw, int size)
  {
    init(VOID_TYPE);
    set(_raw, size);
  }

  Argument::Argument(void *_x)
  {
    init(VOID_TYPE);
    set(_x);
  }

  Argument::Argument(const ArgArray *_array)
  {
    init(VOID_TYPE);
    set(_array);
  }

  // basic types : without copy and with garbage according to garbage policy

  Argument::Argument(char *_s, Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_s, _policy);
  }

  Argument::Argument(unsigned char *_raw, int size,
		     Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_raw, _policy);
  }

  Argument::Argument(Object *_o, Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_o, _policy);
  }

  Argument::Argument(ArgArray *_array,
		     Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_array, _policy);
  }

  // const array types : without copy and without garbage
  Argument::Argument(const int *_i, int cnt)
  {
    init(VOID_TYPE);
    set(_i, cnt);
  }

  Argument::Argument(const char *_c, int cnt)
  {
    init(VOID_TYPE);
    set(_c, cnt);
  }

  Argument::Argument(char **_s, int cnt)
  {
    init(VOID_TYPE);
    set(_s, cnt);
  }

  Argument::Argument(const double *_d, int cnt)
  {
    init(VOID_TYPE);
    set(_d, cnt);
  }

  Argument::Argument(const Oid *_oid, int cnt, Database *_db)
  {
    init(VOID_TYPE);
    set(_oid, cnt, _db);
  }

  Argument::Argument(const Object **_o, int cnt)
  {
    init(VOID_TYPE);
    set(_o, cnt);
  }

  // array types : without copy and with garbage according to garbage policy
  Argument::Argument(int *_i, int cnt, Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_i, cnt, _policy);
  }

  Argument::Argument(char *_c, int cnt, Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_c, cnt, _policy);
  }

  Argument::Argument(char **_s, int cnt,
		     Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_s, cnt, _policy);
  }

  Argument::Argument(double *_d, int cnt, Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_d, cnt, _policy);
  }

  Argument::Argument(Oid *_oid, int cnt,
		     Argument::GarbagePolicy _policy, Database *_db)
  {
    init(VOID_TYPE);
    set(_oid, cnt, _policy, _db);
  }

  Argument::Argument(Object **_o, int cnt,
		     Argument::GarbagePolicy _policy)
  {
    init(VOID_TYPE);
    set(_o, cnt, _policy);
  }

  //
  // Set methods
  //

  void Argument::set(eyedblib::int16 _i)
  {
    garbage();
    init(INT16_TYPE);
    u.i16 = _i;
  }

  void Argument::set(eyedblib::int32 _i)
  {
    garbage();
    init(INT32_TYPE);
    u.i32 = _i;
  }

  void Argument::set(eyedblib::int64 _i)
  {
    garbage();
    init(INT64_TYPE);
    u.i64 = _i;
  }

  void Argument::set(const char *_s)
  {
    garbage();
    init(STRING_TYPE);
    u.s = (char *)_s;
  }

  void Argument::set(char _c)
  {
    garbage();
    init(CHAR_TYPE);
    u.c = _c;
  }

  void Argument::set(unsigned char _by)
  {
    garbage();
    init(BYTE_TYPE);
    u.by = _by;
  }

  void Argument::set(double _d)
  {
    garbage();
    init(FLOAT_TYPE);
    u.d = _d;
  }

  void Argument::set(const Oid &_oid, Database *_db)
  {
    garbage();
    init(OID_TYPE);
    u.oid = new Oid(_oid.getOid());
    db = _db;
  }

  void Argument::set(const Object *_o)
  {
    garbage();
    init(OBJ_TYPE);
    u.o = (Object *)_o;
    if (u.o)
      type->setClname(u.o->getClass()->getName());
    db = (u.o ? u.o->getDatabase() : NULL);
  }

  void Argument::set(const unsigned char *_raw, int size)
  {
    garbage();
    init(RAW_TYPE);
    u.raw.data = (unsigned char *)_raw;
    u.raw.size = size;
  }

  void Argument::set(void *_x)
  {
    garbage();
    init(ANY_TYPE);
    u.x = _x;
  }

  void Argument::set(const ArgArray *_array)
  {
    garbage();
    init(ARRAY_TYPE);
    u.array = (ArgArray *)_array;
  }

  // basic types : without copy and with garbage according to garbage policy
  void Argument::set(Object *_o, Argument::GarbagePolicy _policy)
  {
    garbage();
    init(OBJ_TYPE);
    u.o = _o;
    if (u.o)
      type->setClname(u.o->getClass()->getName());

    policy = _policy;
    db = (u.o ? u.o->getDatabase() : NULL);
  }

  void Argument::set(char *_s, Argument::GarbagePolicy _policy)
  {
    garbage();
    init(STRING_TYPE);
    u.s = _s;
    policy = _policy;
  }

  void Argument::set(unsigned char *_raw, int size,
		     Argument::GarbagePolicy _policy)
  {
    garbage();
    init(RAW_TYPE);
    u.raw.data = _raw;
    u.raw.size = size;
    policy = _policy;
  }

  void Argument::set(ArgArray *_array, Argument::GarbagePolicy _policy)
  {
    garbage();
    init(ARRAY_TYPE);
    u.array = _array;
    policy = _policy;
  }

  // array types : without copy and with garbage according to garbage policy

  void Argument::set(const eyedblib::int16 *_i, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT16_TYPE));
    u.arr_i16.i = (eyedblib::int16 *)_i;
    u.arr_i16.cnt = cnt;
  }

  void Argument::set(const eyedblib::int32 *_i, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT32_TYPE));
    u.arr_i32.i = (eyedblib::int32 *)_i;
    u.arr_i32.cnt = cnt;
  }

  void Argument::set(const eyedblib::int64 *_i, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT64_TYPE));
    u.arr_i64.i = (eyedblib::int64 *)_i;
    u.arr_i64.cnt = cnt;
  }

  void Argument::set(const char *_c, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | CHAR_TYPE));
    u.arr_c.c = (char *)_c;
    u.arr_c.cnt = cnt;
  }

  void Argument::set(char **_s, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | STRING_TYPE));
    u.arr_s.s = (char **)_s;
    u.arr_s.cnt = cnt;
  }

  void Argument::set(const double *_d, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | FLOAT_TYPE));
    u.arr_d.d = (double *)_d;
    u.arr_d.cnt = cnt;
  }

  void Argument::set(const Oid *_oid, int cnt, Database *_db)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | OID_TYPE));
    u.arr_oid.oid = (Oid *)_oid;
    u.arr_oid.cnt = cnt;
    db = _db;
  }

  void Argument::set(const Object **_o, int cnt)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | OBJ_TYPE));
    u.arr_o.o = (Object **)_o;
    u.arr_o.cnt = cnt;
  }

  void Argument::set(eyedblib::int16 *_i, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT16_TYPE));
    u.arr_i16.i = _i;
    u.arr_i16.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(eyedblib::int32 *_i, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT32_TYPE));
    u.arr_i32.i = _i;
    u.arr_i32.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(eyedblib::int64 *_i, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | INT64_TYPE));
    u.arr_i64.i = _i;
    u.arr_i64.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(char *_c, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | CHAR_TYPE));
    u.arr_c.c = _c;
    u.arr_c.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(char **_s, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | STRING_TYPE));
    u.arr_s.s = _s;
    u.arr_s.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(double *_d, int cnt, Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | FLOAT_TYPE));
    u.arr_d.d = _d;
    u.arr_d.cnt = cnt;
    policy = _policy;
  }

  void Argument::set(Oid *_oid, int cnt,
		     Argument::GarbagePolicy _policy, Database *_db)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | OID_TYPE));
    u.arr_oid.oid = _oid;
    u.arr_oid.cnt = cnt;
    policy = _policy;
    db = _db;
  }

  void Argument::set(const Argument &arg)
  {
    *this = arg;
  }

  void Argument::set(Object **_o, int cnt,
		     Argument::GarbagePolicy _policy)
  {
    garbage();
    init((ArgType_Type)(ARRAY_TYPE | OBJ_TYPE));
    u.arr_o.o = _o;
    u.arr_o.cnt = cnt;
    policy = _policy;
  }


  //
  // accessor methods
  //

  eyedblib::int64 Argument::getInteger() const
  {
    int t = type->getType();

    if (t == INT16_TYPE)
      return u.i16;

    if (t == INT32_TYPE)
      return u.i32;

    if (t == INT64_TYPE)
      return u.i64;

    return -1; // should send an exception!
  }

  const Object *Argument::getObject() const
  {
    if (type->getType() == OBJ_TYPE)
      return u.o;

    if (type->getType() == OID_TYPE && db) {
      Object *xo = 0;
      db->transactionBegin();
      db->loadObject(*u.oid, xo);
      db->transactionCommit();
      return xo;
    }

    return NULL;
  }

  Object *Argument::getObject()
  {
    return (Object *)((const Argument *)this)->getObject();
  }

  const eyedblib::int16 *Argument::getIntegers16(int &cnt) const
  {
    if (type->getType() != (INT16_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_i16.cnt;
    return u.arr_i16.i;
  }

  const eyedblib::int32 *Argument::getIntegers32(int &cnt) const
  {
    if (type->getType() != (INT32_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_i32.cnt;
    return u.arr_i32.i;
  }

  const eyedblib::int64 *Argument::getIntegers64(int &cnt) const
  {
    if (type->getType() != (INT64_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_i64.cnt;
    return u.arr_i64.i;
  }

  const char *Argument::getChars(int &cnt) const
  {
    if (type->getType() != (CHAR_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_c.cnt;
    return u.arr_c.c;
  }

  const double *Argument::getFloats(int &cnt) const
  {
    if (type->getType() != (FLOAT_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_d.cnt;
    return u.arr_d.d;
  }

  const Oid *Argument::getOids(int &cnt) const
  {
    if (type->getType() != (OID_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_oid.cnt;
    return u.arr_oid.oid;
  }

  char **Argument::getStrings(int &cnt) const
  {
    if (type->getType() != (STRING_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_s.cnt;
    return (char **)u.arr_s.s;
  }

  Object **Argument::getObjects(int &cnt) const
  {
    if (type->getType() != (OBJ_TYPE|ARRAY_TYPE))
      return NULL;

    cnt = u.arr_o.cnt;
    return (Object **)u.arr_o.o;
  }

  Argument &Argument::operator=(const Argument &arg)
  {
    int _type = arg.type->getType();

    if (_type == INT16_TYPE)
      set(arg.u.i16);

    else if (_type == INT32_TYPE)
      set(arg.u.i32);

    else if (_type == INT64_TYPE)
      set(arg.u.i64);

    else if (_type == STRING_TYPE)
      set(const_cast<const char *>(arg.u.s));

    else if (_type == CHAR_TYPE)
      set(arg.u.c);

    else if (_type == BYTE_TYPE)
      set(arg.u.by);

    else if (_type == FLOAT_TYPE)
      set(arg.u.d);

    else if (_type == OID_TYPE)
      set(*arg.u.oid);

    else if (_type == OBJ_TYPE)
      set(const_cast<Object *>(arg.u.o));

    else if (_type == ARRAY_TYPE)
      set(arg.u.array);

    else if (_type == RAW_TYPE)
      set((const unsigned char *)arg.u.raw.data, arg.u.raw.size);

    else if (_type == ANY_TYPE)
      set(arg.u.x);

    else if (_type == (ARRAY_TYPE|INT16_TYPE))
      set(const_cast<const eyedblib::int16 *>(arg.u.arr_i16.i), arg.u.arr_i16.cnt);

    else if (_type == (ARRAY_TYPE|INT32_TYPE))
      set(const_cast<const eyedblib::int32 *>(arg.u.arr_i32.i), arg.u.arr_i32.cnt);

    else if (_type == (ARRAY_TYPE|INT64_TYPE))
      set(const_cast<const eyedblib::int64 *>(arg.u.arr_i64.i), arg.u.arr_i64.cnt);

    else if (_type == (ARRAY_TYPE|CHAR_TYPE))
      set(const_cast<const char *>(arg.u.arr_c.c), arg.u.arr_c.cnt);

    else if (_type == (ARRAY_TYPE|FLOAT_TYPE))
      set(const_cast<const double *>(arg.u.arr_d.d), arg.u.arr_d.cnt);

    else if (_type == (ARRAY_TYPE|OID_TYPE))
      set(const_cast<const Oid *>(arg.u.arr_oid.oid), arg.u.arr_oid.cnt);

    else if (_type == (ARRAY_TYPE|STRING_TYPE))
      set(const_cast<char **>(arg.u.arr_s.s), arg.u.arr_s.cnt);

    else if (_type == (ARRAY_TYPE|OBJ_TYPE))
      set(const_cast<const Object **>(arg.u.arr_o.o), arg.u.arr_o.cnt);

    else if (_type == VOID_TYPE)
      {
	garbage();
	init(VOID_TYPE);
      }

    else
      abort();

    return *this;
  }

  Argument::Argument(const Argument &arg)
  {
    init(VOID_TYPE);
    *this = arg;
  }

  const char *
  Argument::getArgTypeStr(const ArgType *argtype, Bool printref)
  {
#define RTMX 4
    static int _rettype_idx;
    static char _rettype[RTMX][128];
    char *rettype;
    const char *array;
    int type = argtype->getType();

    if (_rettype_idx >= RTMX)
      _rettype_idx = 0;

    rettype = &_rettype[_rettype_idx++][0];

    if ((type & INOUT_ARG_TYPE) == INOUT_ARG_TYPE)
      {
	if (printref)
	  strcpy(rettype, "inout ");
	else
	  strcpy(rettype, "_INOUT_");
      }
    else if (type & IN_ARG_TYPE)
      {
	if (printref)
	  strcpy(rettype, "in ");
	else
	  strcpy(rettype, "_IN_");
      }
    else if (type & OUT_ARG_TYPE)
      {
	if (printref)
	  strcpy(rettype, "out ");
	else
	  strcpy(rettype, "_OUT_");
      }
    else
      *rettype = 0;

    if (type & ARRAY_TYPE)
      {
	if (printref)
	  array = "[]";
	else
	  array = "_ARRAY_";
      }
    else
      array = "";

    type &= ~(INOUT_ARG_TYPE | ARRAY_TYPE);

    if (type == ANY_TYPE)
      return strcat(strcat(rettype, "any"), array);

    if (type == VOID_TYPE)
      return strcat(strcat(rettype, "void"), array);

    if (type == INT16_TYPE)
      return strcat(strcat(rettype, Int16_Class->getName()), array);

    if (type == INT32_TYPE)
      {
	std::string clname_str = argtype->getClname();
	const char *clname = clname_str.c_str();
	if (clname && *clname)
	  return strcat(strcat(rettype, clname), array);

	return strcat(strcat(rettype, Int32_Class->getName()), array);
      }

    if (type == INT64_TYPE)
      return strcat(strcat(rettype, Int64_Class->getName()), array);

    if (type == STRING_TYPE)
      return strcat(strcat(rettype, "string"), array);

    if (type == CHAR_TYPE)
      return strcat(strcat(rettype, char_class_name), array);

    if (type == BYTE_TYPE)
      return strcat(strcat(rettype, "byte"), array);

    if (type == RAW_TYPE)
      return strcat(strcat(rettype, "rawdata"), array);

    if (type == FLOAT_TYPE)
      return strcat(strcat(rettype, "float"), array);

    if (type == OID_TYPE)
      return strcat(strcat(rettype, "oid"), array);

    if (type == OBJ_TYPE)
      {
	static char s[1024];
	sprintf(s, "%s%s", (*argtype->getClname().c_str() ? argtype->getClname().c_str() :
			    "<unknown class>"), (printref ? "*" : "_REF_"));
	return strcat(strcat(rettype, s), array);
      }

    return "";
  }

  const char *Argument::toString() const
  {
    if (str)
      return str;

    char buf[1024];

    int t = type->getType();

    if (t == INT16_TYPE)
      {
	sprintf(buf, "%d", u.i16);
	return (((Argument *)this)->str = strdup(buf));
      }

    if (t == INT32_TYPE)
      {
	sprintf(buf, "%ld", u.i32);
	return (((Argument *)this)->str = strdup(buf));
      }

    if (t == INT64_TYPE)
      {
	sprintf(buf, "%lld", u.i64);
	return (((Argument *)this)->str = strdup(buf));
      }

    if (t == STRING_TYPE)
      return u.s;

    if (t == CHAR_TYPE)
      {
	sprintf(buf, "'%c'", u.c);
	return (((Argument *)this)->str = strdup(buf));
      }
  
    if (t == BYTE_TYPE)
      {
	sprintf(buf, "'\\%3o'", u.by);
	return (((Argument *)this)->str = strdup(buf));
      }
  
    if (t == FLOAT_TYPE)
      {
	sprintf(buf, "%f", u.d);
	return (((Argument *)this)->str = strdup(buf));
      }
  
    if (t == OID_TYPE)
      return u.oid->getString();
  
    if (t == OBJ_TYPE)
      {
	sprintf(buf, "%p:%s", u.o, type->getClname().c_str());
	return (((Argument *)this)->str = strdup(buf));
      }
  
    if (t == ARRAY_TYPE)
      return (((Argument *)this)->str = strdup(u.array->toString()));

    return "<type not supported>";
  }

  //
  // garbage
  //

  void Argument::garbage()
  {
    int t = type->getType();

    if (t == OID_TYPE)
      delete u.oid;
    else if (policy != NoGarbage) {
      int _i;
      if (t == OBJ_TYPE) {
	if (u.o && !gbxAutoGarb::isObjectDeleted(u.o))
	  u.o->release();
      }
      else if (t == STRING_TYPE)
	::free(u.s);
      else if (t == RAW_TYPE)
	::free(u.raw.data);
      else if (t == ARRAY_TYPE) {
	if (u.array && !gbxAutoGarb::isObjectDeleted(u.array))
	  u.array->release();
      }
      else if (t == (ARRAY_TYPE|INT16_TYPE))
	::free(u.arr_i16.i);
      else if (t == (ARRAY_TYPE|INT32_TYPE))
	::free(u.arr_i32.i);
      else if (t == (ARRAY_TYPE|INT64_TYPE))
	::free(u.arr_i64.i);
      else if (t == (ARRAY_TYPE|CHAR_TYPE))
	::free(u.arr_c.c);
      else if (t == (ARRAY_TYPE|STRING_TYPE)) {
	if (policy == AutoFullGarbage)
	  for (_i = 0; _i < u.arr_s.cnt; _i++)
	    ::free(u.arr_s.s[_i]);
	::free(u.arr_s.s);
      }
      else if (t == (ARRAY_TYPE|OBJ_TYPE)) {
	if (policy == AutoFullGarbage)
	  for (_i = 0; _i < u.arr_o.cnt; _i++)
	    if (u.arr_o.o[_i] && !gbxAutoGarb::isObjectDeleted(u.arr_o.o[_i]))
	      u.arr_o.o[_i]->release();
	::free(u.arr_o.o);
      }
      else if (t == (ARRAY_TYPE|OID_TYPE))
	::free(u.arr_oid.oid);
    }

    ::free(str);
    str = NULL;

    if (!gbxAutoGarb::isObjectDeleted(type))
      type->release();

    type = NULL;
  }

#define COPY_ARRAY(TYPE, X, SZ) \
  TYPE *__x = (TYPE *)malloc((SZ) * sizeof(TYPE)); \
  memcpy(__x, X, (SZ) * sizeof(TYPE)); \
  return __x

#define ALLOC_ARRAY(TYPE, X, SZ) \
  if (!(X)) \
     return (TYPE *)calloc((SZ), sizeof(TYPE)); \
  return (TYPE *)realloc(X, (SZ) * sizeof(TYPE))

  char *Argument::alloc(unsigned int sz, char *x)
  {
    ALLOC_ARRAY(char, x, sz);
  }

  unsigned char *Argument::alloc(unsigned int sz, unsigned char *x)
  {
    ALLOC_ARRAY(unsigned char, x, sz);
  }

  eyedblib::int16 *Argument::alloc(unsigned int sz, eyedblib::int16 *x)
  {
    ALLOC_ARRAY(eyedblib::int16, x, sz);
  }

  eyedblib::int32 *Argument::alloc(unsigned int sz, eyedblib::int32 *x)
  {
    ALLOC_ARRAY(eyedblib::int32, x, sz);
  }

  eyedblib::int64 *Argument::alloc(unsigned int sz, eyedblib::int64 *x)
  {
    ALLOC_ARRAY(eyedblib::int64, x, sz);
  }

  double *Argument::alloc(unsigned int sz, double *x)
  {
    ALLOC_ARRAY(double, x, sz);
  }

  Oid *Argument::alloc(unsigned int sz, Oid *x)
  {
    ALLOC_ARRAY(Oid, x, sz);
  }

  char **Argument::alloc(unsigned int sz, char **x)
  {
    ALLOC_ARRAY(char *, x, sz);
  }

  Object **Argument::alloc(unsigned int sz, Object **x)
  {
    ALLOC_ARRAY(Object *, x, sz);
  }

  char *Argument::dup(const char *s)
  {
    return strdup(s);
  }

  unsigned char *Argument::dup(const unsigned char *x, int sz)
  {
    COPY_ARRAY(unsigned char, x, sz);
  }

  eyedblib::int16 *Argument::dup(const eyedblib::int16 *x, int cnt)
  {
    COPY_ARRAY(eyedblib::int16, x, cnt);
  }

  eyedblib::int32 *Argument::dup(const eyedblib::int32 *x, int cnt)
  {
    COPY_ARRAY(eyedblib::int32, x, cnt);
  }

  eyedblib::int64 *Argument::dup(const eyedblib::int64 *x, int cnt)
  {
    COPY_ARRAY(eyedblib::int64, x, cnt);
  }

  double *Argument::dup(const double *x, int cnt)
  {
    COPY_ARRAY(double, x, cnt);
  }

  Oid *Argument::dup(const Oid *x, int cnt)
  {
    COPY_ARRAY(Oid, x, cnt);
  }

  char **Argument::dup(char **x, int cnt)
  {
    char **_x = (char **)malloc(cnt * sizeof(char *));
    for (int i = 0; i < cnt; i++)
      _x[i] = strdup(x[i]);
    return _x;
  }

  Object **Argument::dup(Object **x, int cnt)
  {
    COPY_ARRAY(Object *, x, cnt);
  }

  void Argument::free(char *x)
  {
    ::free(x);
  }

  void Argument::free(unsigned char *x)
  {
    ::free(x);
  }

  void Argument::free(eyedblib::int16 *x)
  {
    ::free(x);
  }

  void Argument::free(eyedblib::int32 *x)
  {
    ::free(x);
  }

  void Argument::free(eyedblib::int64 *x)
  {
    ::free(x);
  }

  void Argument::free(double *x)
  {
    ::free(x);
  }

  void Argument::free(Oid *x)
  {
    ::free(x);
  }

  void Argument::free(char **x, int cnt)
  {
    for (int i = 0; i < cnt; i++)
      ::free(x[i]);
    ::free(x);
  }

  void Argument::free(Object *x)
  {
    if (x)
      x->release();
  }

  void Argument::free(Object **x, int cnt)
  {
    for (int i = 0; i < cnt; i++)
      if (x[i])
	x[i]->release();
    ::free(x);
  }

  Argument::~Argument()
  {
    garbageRealize();
  }

  //
  // ArgArray
  //

  ArgArray::ArgArray()
    : cnt(0), args(NULL), str(NULL), policy(Argument::NoGarbage)
  {
  }

  ArgArray::ArgArray(const Argument **_args, int _cnt)
    : cnt(_cnt), args((Argument **)_args), policy(Argument::NoGarbage),
      str(NULL)
  {
  }

  ArgArray::ArgArray(Argument **_args, int _cnt,
		     Argument::GarbagePolicy _policy)
    : cnt(_cnt), args(_args), policy(_policy), str(NULL)
  {
  }

  // 15/01/99: set the default policy to full garbage
  //ArgArray::ArgArray(int _cnt) : str(NULL), policy(Argument::NoGarbage)
  ArgArray::ArgArray(int _cnt, Argument::GarbagePolicy _policy) :
    str(NULL), policy(_policy)
  {
    cnt = _cnt;

    args = (Argument **)malloc(cnt * sizeof(Argument *));

    for (int i = 0; i < cnt; i++)
      args[i] = new Argument();
  }

  void
  ArgArray::set(Argument **_args, int _cnt,
		Argument::GarbagePolicy _policy)
  {
    garbage();
    str = NULL;
    cnt = _cnt;
    args = _args;
    policy = _policy;
  }

  ArgType_Type
  ArgArray::getType() const
  {
    ArgType_Type type = (ArgType_Type)-1;
    for (int i = 0; i < cnt; i++)
      {
	ArgType_Type argtyp = args[i]->type->getType();

	if (type >= 0 && argtyp != type)
	  return ANY_TYPE;
	type = argtyp;
      }

    return type >= 0 ? type : ANY_TYPE;
  }

  const char *
  ArgArray::toString() const
  {
    ::free(str);

    char *r = strdup("{");

    for (int ii = 0; ii < cnt; ii++)
      {
	const char *rs = args[ii]->toString();
	r = (char *)realloc(r, strlen(r) + strlen(rs) + (ii ? 3 : 1));
	if (ii)
	  strcat(r, ", ");
	strcat(r, rs);
      }
  
    r = (char *)realloc(r, strlen(r) + strlen("}") + 1);
    strcat(r, "}");
    return ((ArgArray *)this)->str = r;
  }

  void ArgArray::garbage()
  {
    ::free(str);
    str = NULL;

    if (policy == Argument::NoGarbage)
      return;

    for (int i = 0; i < cnt; i++)
      args[i]->release();

    if (policy == Argument::AutoFullGarbage)
      ::free(args);
  }

  ArgArray::~ArgArray()
  {
    garbageRealize();
  }

  //
  // ArgType
  //

  Bool ArgType::operator==(const ArgType &argtype) const
  {
    ArgType_Type _type = getType();

    if (argtype.getType() != _type)
      return False;

    if (_type != OBJ_TYPE)
      return True;

    return !strcmp(getClname().c_str(), argtype.getClname().c_str()) ? True : False;
  }

  Bool ArgType::operator!=(const ArgType &argtype) const
  {
    return (argtype == *this ? False : True);
  }

  int
  ArgType::getBasicType(const char *s)
  {
    if (!strcmp(s, "any"))
      return ANY_TYPE;

    if (!strcmp(s, "void"))
      return VOID_TYPE;

    if (!strcmp(s, "short") || !strcmp(s, int16_class_name))
      return INT16_TYPE;

    if (!strcmp(s, "int") || !strcmp(s, int32_class_name))
      return INT32_TYPE;
      
    if (!strcmp(s, "long") || !strcmp(s, int64_class_name))
      return INT64_TYPE;

    if (!strcmp(s, "string"))
      return STRING_TYPE;

    if (!strcmp(s, char_class_name))
      return CHAR_TYPE;

    if (!strcmp(s, "byte"))
      return BYTE_TYPE;

    if (!strcmp(s, "float") || !strcmp(s, "double"))
      return FLOAT_TYPE;

    if (!strcmp(s, "oid"))
      return OID_TYPE;

    if (!strcmp(s, "raw") || !strcmp(s, "rawdata"))
      return RAW_TYPE;

    return -1;
  }

  ArgType *
  ArgType::make(Schema *m, const char *_s)
  {
    ArgType *type;
    int basic_type;
    static char x[128];
    int mod;
    int len = strlen(_s);
    const char *enum_class = 0;

    strcpy(x, _s);

    if (len > 2 && !strcmp(&x[len-2], "[]")) {
      mod = ARRAY_TYPE;
      x[len-2] = 0;
    }
    else
      mod = 0;
      
    basic_type = getBasicType(x);

    if (x[strlen(x)-1] == '*')
      x[strlen(x)-1] = 0;

    if (basic_type < 0) {
      const Class *cl = m->getClass(x);
      if (cl && cl->asEnumClass()) {
	basic_type = INT32_TYPE;
	enum_class = cl->getAliasName();
      }
    }

    if (basic_type >= 0) {
      // 16/01/99: do not support array of raw
      if (basic_type == RAW_TYPE && mod == ARRAY_TYPE)
	return 0;
      type = new ArgType();
      type->setType((ArgType_Type)(basic_type | mod), False);
      if (enum_class)
	type->setClname(enum_class);
      return type;
    }
  
    if (!strcmp(x, "int"))
      strcpy(x, int32_class_name);

    if (!strcmp(x, "short"))
      strcpy(x, int16_class_name);

    if (!strcmp(x, "long"))
      strcpy(x, int64_class_name);

    const Class *cls = m->getClass(x);
    if (!cls)
      return 0;
  
    type = new ArgType();

    type->setType((ArgType_Type)(OBJ_TYPE | mod), False);
    type->setClname(x);

    return type;
  }

#define PURGE(T) ((T) & ~(INOUT_ARG_TYPE|ARRAY_TYPE))

  extern Bool odl_class_enums;

  const char *
  ArgType::getCType(Schema *m) const
  {
    int _type = PURGE(getType());

    if (_type == OBJ_TYPE)
      {
	static char tok[512];
	sprintf(tok, "%s *", m->getClass(getClname().c_str())->getCName(True));
	return tok;
      }
    if (_type == ANY_TYPE)
      return "Argument";
    if (_type == INT16_TYPE)
      return Int16_Class->getCName();
    if (_type == INT32_TYPE)
      {
	const char *clname = getClname().c_str();
	if (clname && *clname)
	  {
	    static char tok[512];
	    std::string clsname = getClname();
	    sprintf(tok, "%s", m->getClass(clsname.c_str())->getCName(True));
	    if (odl_class_enums && !Class::isBoolClass(clsname.c_str()))
	      strcat(tok, "::Type");
	    return tok;
	  }
	return Int32_Class->getCName();
      }
    if (_type == INT64_TYPE)
      return Int64_Class->getCName();
    if (_type == CHAR_TYPE)
      return char_class_name;
    if (_type == BYTE_TYPE)
      return "unsigned char";
    if (_type == FLOAT_TYPE)
      return "double";
    if (_type == STRING_TYPE)
      return "char *";
    if (_type == OID_TYPE)
      return "Oid";
    if (_type == RAW_TYPE)
      return "unsigned char *";

    return "";
  }

#define CONST(T) \
((((T) & OUT_ARG_TYPE) || \
  (((T) & ARRAY_TYPE) && \
   (PURGE(T) == STRING_TYPE) || \
   (PURGE(T) == OBJ_TYPE))) \
 ? "" : "const ")

  void
  ArgType::getCPrefix(FILE *fd, Schema *m, const char *prefix,
		      const char *name, Bool fullcast) const
  {
    int _type = PURGE(getType());
 
    int isArray = (getType() & ARRAY_TYPE);
    const char *s = (isArray ? "s" : "");
    const char *r = (isArray ? "" : "*");

    if (_type == INT16_TYPE)
      fprintf(fd, "%s%sgetInteger%s16(", r, prefix, s);
    else if (_type == INT32_TYPE)
      {
	fprintf(fd, r);
	fprintf(fd, "%sgetInteger%s32(", prefix, s);
      }
    else if (_type == INT64_TYPE)
      fprintf(fd, "%s%sgetInteger%s64(", r, prefix, s);
    else if (_type == CHAR_TYPE)
      fprintf(fd, "%s%sgetChar%s(", r, prefix, s);
    else if (_type == BYTE_TYPE)
      fprintf(fd, "%s%sgetByte%s(", r, prefix, s);
    else if (_type == FLOAT_TYPE)
      fprintf(fd, "%s%sgetFloat%s(", r, prefix, s);
    else if (_type == STRING_TYPE)
      fprintf(fd, "%sgetString%s(", prefix, s);
    else if (_type == OID_TYPE)
      fprintf(fd, "%s%sgetOid%s(", r, prefix, s);
    else if (_type == OBJ_TYPE)
      {
	if (fullcast)
	  fprintf(fd, "(%s%s *%s)%sgetObject%s(", 
		  //(*s ? "const " : ""),
		  CONST(_type),
		  m->getClass(getClname().c_str())->getCName(True),
		  (*s ? " *" : ""), prefix, s);
	else
	  fprintf(fd, "%sgetObject%s(", 
		  prefix, s);
      }
    else if (_type == RAW_TYPE)
      fprintf(fd, "%sgetRaw(", prefix);
    else if (_type == ANY_TYPE)
      fprintf(fd, "%sgetArgument(", prefix);

    if (isArray)
      fprintf(fd, "%s_cnt", name);
    else if (_type == RAW_TYPE)
      fprintf(fd, "%s_size", name);

    fprintf(fd, ")");
  }

  void ArgType::declare(FILE *fd, Schema *m, const char *name)
  {
    int _type = getType();

    const char *_ref;
    const char *_const;

    if ((_type & (OUT_ARG_TYPE|ARRAY_TYPE)) ==
	(OUT_ARG_TYPE|ARRAY_TYPE))
      _ref = "* &";
    else if (_type & OUT_ARG_TYPE)
      _ref = "&";
    else if (_type & ARRAY_TYPE)
      _ref = "*";
    else
      _ref = "";

    _const = CONST(_type);
  
    char _array[1024];
    if (_type & ARRAY_TYPE)
      sprintf(_array, ", int %s%s_cnt", (_type & OUT_ARG_TYPE ? "&" : ""),
	      name);
    else if (PURGE(_type) == RAW_TYPE)
      sprintf(_array, ", int %s%s_size", (_type & OUT_ARG_TYPE ? "&" : ""),
	      name);
    else
      *_array = 0;

    fprintf(fd, "%s%s %s%s%s", _const, getCType(m), _ref, name, _array);
  }

  void ArgType::init(FILE *fd, Schema *m, const char *prefix,
		     const char *name, const char *indent)
  {
    int _type = getType();

    if (_type & ARRAY_TYPE)
      fprintf(fd, "%sint %s_cnt = 0;\n", indent, name);
    else if (PURGE(_type) == RAW_TYPE)
      fprintf(fd, "%sint %s_size = 0;\n", indent, name);

    const char *_const = CONST(_type);

    fprintf(fd, "%s%s%s %s%s = ", indent,
	    _const,
	    getCType(m), (_type & ARRAY_TYPE) ? "*" : "",
	    name);

    if ((_type & INOUT_ARG_TYPE) == INOUT_ARG_TYPE)
      {
	if (_type & ARRAY_TYPE)
	  fprintf(fd, "(%s *)", getCType(m));
	else if (PURGE(_type) == RAW_TYPE)
	  fprintf(fd, "(%sunsigned char *)", _const);
      }

    if (PURGE(_type) == INT32_TYPE && *getClname().c_str()) {
      std::string clsname = getClname();
      fprintf(fd, "(%s%s%s)",  m->getClass(clsname.c_str())->getCName(True),
	      (odl_class_enums && !Class::isBoolClass(clsname.c_str()) ? "::Type" : ""),
	      (_type & ARRAY_TYPE) ? " *" : "");
    }

    if (_type & IN_ARG_TYPE)
      getCPrefix(fd, m, prefix, name, True);
    else if (PURGE(_type) == OID_TYPE && !(_type & ARRAY_TYPE))
      fprintf(fd, "Oid::nullOid");
    else
      fprintf(fd, "0");
  }

  void ArgType::ret(FILE *fd, Schema *m, const char *prefix, const char *name)
  {
    int _type = PURGE(getType());
 
    fprintf(fd, "%s = (%s%s)", name, getCType(m),
	    (getType() & ARRAY_TYPE ? " *" : ""));

    getCPrefix(fd, m, prefix, name, False);

    if (getType() & ARRAY_TYPE)
      {
	fprintf(fd, ";\n");
	if (PURGE(getType()) == OBJ_TYPE)
	  {
	    const char *clname = getClname().c_str();
	    if (clname && *clname)
	      fprintf(fd, "  %s = (%s **)eyedb::Argument::dup((Object **)%s, "
		      "%s_cnt)", name, clname, name, name);
	    else
	      fprintf(fd, "  %s = eyedb::Argument::dup((Object **)%s, %s_cnt)",
		      name, name, name);
	  }
	else if (PURGE(getType()) == INT32_TYPE && *getClname().c_str())
	  fprintf(fd, "  %s = (%s *)eyedb::Argument::dup((eyedblib::int32 *)%s, %s_cnt)",
		  name, getClname().c_str(), name, name);
	else
	  fprintf(fd, "  %s = eyedb::Argument::dup(%s, %s_cnt)", name, name, name);
      }
    else if (_type == STRING_TYPE)
      fprintf(fd, ";\n  %s = eyedb::Argument::dup(%s)", name, name);
    else if (_type == RAW_TYPE)
      fprintf(fd, ";\n  %s = eyedb::Argument::dup(%s, %s_size)", name, name, name);
  }

  //
  // Signature
  //

  Bool Signature::operator==(const Signature &sign) const
  {
    if (*getRettype() != *sign.getRettype() ||
	getNargs() != sign.getNargs())
      return False;

    int nargs = getNargs();
    for (int i = 0; i < nargs; i++)
      if (*getTypes(i) != *sign.getTypes(i))
	return False;

    return True;
  }

  Bool Signature::operator!=(const Signature &sign) const
  {
    return (sign == *this ? False : True);
  }

  static const char parg[]    = "arg";
  static const char pretarg[] = "retarg";
  static const char ppretarg[] = "_retarg";

  const char *
  Signature::getArg(int i)
  {
    static char sarg[512];
    if (getUserData())
      {
	char **names = ((odlSignUserData *)getUserData())->names;

	if (names && names[i])
	  return names[i];
      }

    sprintf(sarg, "%s%d", parg, i+1);
    return sarg;
  }

  const char *
  Signature::getPrefix(const char *prefix, int i)
  {
    static char sprefix[512];
    sprintf(sprefix, prefix, i);
    return sprefix;
  }

  Bool
  Signature::isVoid(const ArgType *type)
  {
    return (PURGE(type->getType()) == VOID_TYPE) ?
      True : False;
  }

  void Signature::listArgs(FILE *fd, Schema *m)
  {
    int nargs = getNargs();
    for (int i = 0; i < nargs; i++)
      {
	ArgType *arg = getTypes(i);
	if (i)
	  fprintf(fd, ", ");
	fprintf(fd, getArg(i));
	if (arg->getType() & ARRAY_TYPE)
	  fprintf(fd, ", %s_cnt", getArg(i));
	else if (PURGE(arg->getType()) == RAW_TYPE)
	  fprintf(fd, ", %s_size", getArg(i));
      }

    if (isVoid(getRettype()))
      return;

    if (nargs)
      fprintf(fd, ", ");
    fprintf(fd, "%s", ppretarg);
    if (getRettype()->getType() & ARRAY_TYPE)
      fprintf(fd, ", %s_cnt", ppretarg);
    else if (PURGE(getRettype()->getType()) == RAW_TYPE)
      fprintf(fd, ", %s_size", ppretarg);
  }

  void Signature::declArgs(FILE *fd, Schema *m)
  {
    int nargs = getNargs();
    for (int i = 0, n = 0; i < nargs; i++)
      {
	ArgType *arg = getTypes(i);
	if (i)
	  fprintf(fd, ", ");
	arg->declare(fd, m, getArg(i));
      }

    if (isVoid(getRettype()))
      return;

    if (nargs)
      fprintf(fd, ", ");
    getRettype()->declare(fd, m, pretarg);
  }

  void Signature::initArgs(FILE *fd, Schema *m, const char *prefix,
			   const char *preret, const char *indent)
  {
    int nargs = getNargs();
    for (int i = 0, n = 0; i < nargs; i++)
      {
	ArgType *arg = getTypes(i);
	arg->init(fd, m, getPrefix(prefix, i), getArg(i), indent);
	fprintf(fd, ";\n");
      }

    if (isVoid(getRettype()))
      return;

    getRettype()->init(fd, m, 0, preret, indent);
    fprintf(fd, ";\n");
  }

  static const char *
  getCast(int type, const char *clname, int isout)
  {
    if ((type & (ARRAY_TYPE|OBJ_TYPE)) == (ARRAY_TYPE|OBJ_TYPE))
      return isout ? "(Object **)" : "(const Object **)";
    else if (*clname && (type & (ARRAY_TYPE|INT32_TYPE)) ==
	     (ARRAY_TYPE|INT32_TYPE))
      return isout ? "(eyedblib::int32 *)" : "(const eyedblib::int32 *)";

    return "";
  }

  void Signature::setArgs(FILE *fd, Schema *m, int _type,
			  const char *prefix, const char *preret,
			  const char *indent)
  {
    int nargs = getNargs();

    // 19/02/99 ->
    const char *polstr = (_type & OUT_ARG_TYPE) ?
      ", eyedb::Argument::AutoFullGarbage" : ", eyedb::Argument::NoGarbage";

    //const char *polstr = ", Argument::NoGarbage";

    for (int i = 0; i < nargs; i++)
      {
	ArgType *arg = getTypes(i);
	if ((arg->getType() & _type) == _type)
	  {
	    fprintf(fd, "%s%sset(%s%s", indent, getPrefix(prefix, i),
		    getCast(arg->getType(), arg->getClname().c_str(), (arg->getType() & OUT_ARG_TYPE)),
		    getArg(i));
	    if (arg->getType() & ARRAY_TYPE)
	      fprintf(fd, ", %s_cnt%s", getArg(i),
		      !(arg->getType() & OUT_ARG_TYPE) ? "" : polstr);
	    else if (PURGE(arg->getType()) == RAW_TYPE)
	      fprintf(fd, ", %s_size%s", getArg(i),
		      !(arg->getType() & OUT_ARG_TYPE) ? "" : polstr);
	    else if ((_type & OUT_ARG_TYPE) &&
		     (PURGE(arg->getType()) == STRING_TYPE ||
		      PURGE(arg->getType()) == OBJ_TYPE))
	      fprintf(fd, "%s", polstr);
	    fprintf(fd, ");\n");
	  }
      }

    if (isVoid(getRettype()))
      return;

    if (_type & OUT_ARG_TYPE)
      {
	ArgType *rettype = getRettype();
	fprintf(fd, "%s%sset(%s%s", indent, preret,
		getCast(rettype->getType(), rettype->getClname().c_str(), 1), ppretarg);
	if (rettype->getType() & ARRAY_TYPE)
	  fprintf(fd, ", %s_cnt%s", ppretarg, polstr);
	else if (PURGE(rettype->getType()) == RAW_TYPE)
	  fprintf(fd, ", %s_size%s", ppretarg, polstr);
	else if (PURGE(rettype->getType()) == STRING_TYPE ||
		 PURGE(rettype->getType()) == OBJ_TYPE)
	  fprintf(fd, "%s", polstr);
	fprintf(fd, ");\n");
      }
  }

  void Signature::retArgs(FILE *fd, Schema *m, const char *prefix,
			  const char *preret,
			  const char *indent)
  {
    int nargs = getNargs();
    for (int i = 0, n = 0; i < nargs; i++)
      {
	ArgType *arg = getTypes(i);
	if (arg->getType() & OUT_ARG_TYPE)
	  {
	    fprintf(fd, indent);
	    arg->ret(fd, m, getPrefix(prefix, i), getArg(i));
	    fprintf(fd, ";\n");
	  }
      }

    if (isVoid(getRettype()))
      return;

    fprintf(fd, indent);
    getRettype()->ret(fd, m, preret, pretarg);
    fprintf(fd, ";\n");
  }
}
