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


#ifndef _EYEDB_ARGUMENT_H
#define _EYEDB_ARGUMENT_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class ArgArray;
  class ArgType;

  /**
     Not yet documented.
  */
  class Argument : public gbxObject {

    // ----------------------------------------------------------------------
    // Argument Interface
    // ----------------------------------------------------------------------
  public:
    enum GarbagePolicy {
      NoGarbage = 1,
      AutoGarbage,
      AutoFullGarbage
    };

    //
    // Constructors
    //

    // basic types : without copy and without garbage
    /**
       Not yet documented.
    */
    Argument();

    /**
       Not yet documented.
       @param i
    */
    Argument(eyedblib::int16 i);

    /**
       Not yet documented.
       @param i
    */
    Argument(eyedblib::int32 i);

    /**
       Not yet documented.
       @param i
    */
    Argument(eyedblib::int64 i);

    /**
       Not yet documented.
       @param s
    */
    Argument(const char *s);

    /**
       Not yet documented.
       @param c
    */
    Argument(char c);

    /**
       Not yet documented.
       @param by
    */
    Argument(unsigned char by);

    /**
       Not yet documented.
       @param d
    */
    Argument(double d);

    /**
       Not yet documented.
       @param oid
       @param db
    */
    Argument(const Oid &oid, Database *db = NULL);

    /**
       Not yet documented.
       @param o
    */
    Argument(const Object *o);

    /**
       Not yet documented.
       @param raw
       @param size
    */
    Argument(const unsigned char *raw, int size);

    /**
       Not yet documented.
       @param x
    */
    Argument(void *x);

    /**
       Not yet documented.
       @param array
    */
    Argument(const ArgArray *array);

    // basic types : without copy and with garbage according to garbage policy
    /**
       Not yet documented.
       @param s
       @param policy
    */
    Argument(char *s, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param o
       @param policy
    */
    Argument(Object *o, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param raw
       @param size
       @param policy
    */
    Argument(unsigned char *raw, int size, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param array
       @param policy
    */
    Argument(ArgArray *array, Argument::GarbagePolicy policy);

    // const array types : without copy and without garbage
    /**
       Not yet documented.
       @param i
       @param cnt
    */
    Argument(const int *i, int cnt);

    /**
       Not yet documented.
       @param c
       @param cnt
    */
    Argument(const char *c, int cnt);

    /**
       Not yet documented.
       @param s
       @param cnt
    */
    Argument(char **s, int cnt);

    /**
       Not yet documented.
       @param d
       @param cnt
    */
    Argument(const double *d, int cnt);

    /**
       Not yet documented.
       @param oid
       @param cnt
       @param db
    */
    Argument(const Oid *oid, int cnt, Database *db = 0);

    /**
       Not yet documented.
       @param o
       @param cnt
    */
    Argument(const Object **o, int cnt);

    // array types : without copy and with garbage according to garbage policy
    /**
       Not yet documented.
       @param i
       @param cnt
       @param policy
    */
    Argument(int *i, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param c
       @param cnt
       @param policy
    */
    Argument(char *c, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param s
       @param cnt
       @param policy
    */
    Argument(char **s, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param d
       @param cnt
       @param policy
    */
    Argument(double *d, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param oid
       @param cnt
       @param policy
       @param db
    */
    Argument(Oid *oid, int cnt, Argument::GarbagePolicy policy,
	     Database *db = NULL);

    /**
       Not yet documented.
       @param o
       @param cnt
       @param policy
    */
    Argument(Object **o, int cnt, Argument::GarbagePolicy policy);

    // copy constructor
    /**
       Not yet documented.
       @param arg
    */
    Argument(const Argument & arg);

    // assignation operator
    /**
       Not yet documented.
       @param arg
       @return
    */
    Argument &operator=(const Argument &arg);

    //
    // Set methods
    //

    // basic types
    /**
       Not yet documented.
       @param i
    */
    void set(eyedblib::int16 i);

    /**
       Not yet documented.
       @param i
    */
    void set(eyedblib::int32 i);

    /**
       Not yet documented.
       @param i
    */
    void set(eyedblib::int64 i);

    /**
       Not yet documented.
       @param s
    */
    void set(const char *s);

    /**
       Not yet documented.
       @param c
    */
    void set(char c);

    /**
       Not yet documented.
       @param by
    */
    void set(unsigned char by);

    /**
       Not yet documented.
       @param d
    */
    void set(double d);

    /**
       Not yet documented.
       @param oid
       @param db
    */
    void set(const Oid &oid, Database *db = 0);

    /**
       Not yet documented.
       @param o
    */
    void set(const Object *o);

    /**
       Not yet documented.
       @param raw
       @param size
    */
    void set(const unsigned char *raw, int size);

    /**
       Not yet documented.
       @param x
    */
    void set(void *x);

    /**
       Not yet documented.
       @param array
    */
    void set(const ArgArray *array);

    /**
       Not yet documented.
       @param arg
    */
    void set(const Argument &arg);

    // basic types : without copy and with garbage according to garbage policy
    /**
       Not yet documented.
       @param s
       @param policy
    */
    void set(char *s, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param o
       @param policy
    */
    void set(Object *o, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param raw
       @param size
       @param policy
    */
    void set(unsigned char *raw, int size, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param array
       @param policy
    */
    void set(ArgArray *array, Argument::GarbagePolicy policy);

    // array types : without copy and with garbage according to garbage policy
    /**
       Not yet documented.
       @param i
       @param cnt
    */
    void set(const eyedblib::int16 *i, int cnt);

    /**
       Not yet documented.
       @param i
       @param cnt
    */
    void set(const eyedblib::int32 *i, int cnt);

    /**
       Not yet documented.
       @param i
       @param cnt
    */
    void set(const eyedblib::int64 *i, int cnt);

    /**
       Not yet documented.
       @param c
       @param cnt
    */
    void set(const char *c, int cnt);

    /**
       Not yet documented.
       @param s
       @param cnt
    */
    void set(char **s, int cnt);

    /**
       Not yet documented.
       @param d
       @param cnt
    */
    void set(const double *d, int cnt);

    /**
       Not yet documented.
       @param oid
       @param cnt
       @param db
    */
    void set(const Oid *oid, int cnt, Database *db = 0);

    /**
       Not yet documented.
       @param o
       @param cnt
    */
    void set(const Object **o, int cnt);

    // array types : without copy and with garbage according to garbage policy
    /**
       Not yet documented.
       @param i
       @param cnt
       @param policy
    */
    void set(eyedblib::int16 *i, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param i
       @param cnt
       @param policy
    */
    void set(eyedblib::int32 *i, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param i
       @param cnt
       @param policy
    */
    void set(eyedblib::int64 *i, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param c
       @param cnt
       @param policy
    */
    void set(char *c, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param s
       @param cnt
       @param policy
    */
    void set(char **s, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param d
       @param cnt
       @param policy
    */
    void set(double *d, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param oid
       @param cnt
       @param policy
       @param db
    */
    void set(Oid *oid, int cnt, Argument::GarbagePolicy policy,
	     Database *db = NULL);

    /**
       Not yet documented.
       @param o
       @param cnt
       @param policy
    */
    void set(Object **o, int cnt, Argument::GarbagePolicy policy);

    //
    // Accessor methods
    // 

    // basic types
    /**
       Not yet documented.
       @return
    */
    const eyedblib::int16 *getInteger16() const {
      return type->getType() == INT16_TYPE ? &u.i16 : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const eyedblib::int32 *getInteger32() const {
      return type->getType() == INT32_TYPE ? &u.i32 : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const eyedblib::int64 *getInteger64() const {
      return type->getType() == INT64_TYPE ? &u.i64 : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    eyedblib::int64 getInteger() const;

    /**
       Not yet documented.
       @return
    */
    const char *getChar() const {
      return type->getType() == CHAR_TYPE ? &u.c : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const unsigned char *getByte() const {
      return type->getType() == BYTE_TYPE ? &u.by : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const double *getFloat() const  {
      return type->getType() == FLOAT_TYPE ? &u.d : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const Oid *getOid() const {
      return type->getType() == OID_TYPE ? u.oid : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const char *getString() const {
      return type->getType() == STRING_TYPE ? u.s : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const Object *getObject() const;

    /**
       Not yet documented.
       @param size
       @return
    */
    const unsigned char *getRaw(int &size) const {
      if (type->getType() == RAW_TYPE) {
	size = u.raw.size;
	return u.raw.data;
      }
      return NULL;
    }

    /**
       Not yet documented.
       @param size
       @return
    */
    const unsigned char *getBytes(int &size) const {
      return getRaw(size);
    }

    /**
       Not yet documented.
       @return
    */
    const void *getX() const {
      return type->getType() == ANY_TYPE ? u.x : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    char *getString() {
      return type->getType() == STRING_TYPE ? u.s : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    Object *getObject();

    /**
       Not yet documented.
       @return
    */
    void *getX() {
      return type->getType() == ANY_TYPE ? u.x : NULL;
    }

    /**
       Not yet documented.
       @return
    */
    const Argument getArgument() {
      return *this;
    }

    // array types
    /**
       Not yet documented.
       @param cnt
       @return
    */
    const eyedblib::int16 *getIntegers16(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    const eyedblib::int32 *getIntegers32(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    const eyedblib::int64 *getIntegers64(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    const char *getChars(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    const double *getFloats(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    const Oid *getOids(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    char **getStrings(int &cnt) const;

    /**
       Not yet documented.
       @param cnt
       @return
    */
    Object **getObjects(int &cnt) const;

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static char *alloc(unsigned int sz, char *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static unsigned char *alloc(unsigned int sz, unsigned char *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static eyedblib::int16 *alloc(unsigned int sz, eyedblib::int16 *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static eyedblib::int32 *alloc(unsigned int sz, eyedblib::int32 *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static eyedblib::int64 *alloc(unsigned int sz, eyedblib::int64 *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static double *alloc(unsigned int sz, double *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static Oid *alloc(unsigned int sz, Oid *x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static char **alloc(unsigned int sz, char **x);

    /**
       Not yet documented.
       @param sz
       @param x
       @return
    */
    static Object **alloc(unsigned int sz, Object **x);

    /**
       Not yet documented.
       @param s
       @return
    */
    static char *dup(const char *s);

    /**
       Not yet documented.
       @param x
       @param sz
       @return
    */
    static unsigned char *dup(const unsigned char *x, int sz);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static eyedblib::int16 *dup(const eyedblib::int16 *x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static eyedblib::int32 *dup(const eyedblib::int32 *x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static eyedblib::int64 *dup(const eyedblib::int64 *x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static double *dup(const double *x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static Oid *dup(const Oid *x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static char **dup(char **x, int cnt);

    /**
       Not yet documented.
       @param x
       @param cnt
       @return
    */
    static Object **dup(Object **x, int cnt);

    /**
       Not yet documented.
       @param x
    */
    static void free(char *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(unsigned char *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(eyedblib::int16 *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(eyedblib::int32 *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(eyedblib::int64 *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(double *x);

    /**
       Not yet documented.
       @param x
    */
    static void free(Oid *x);

    /**
       Not yet documented.
       @param x
       @param cnt
    */
    static void free(char **x, int cnt);

    /**
       Not yet documented.
       @param x
    */
    static void free(Object *x);

    /**
       Not yet documented.
       @param x
       @param cnt
    */
    static void free(Object **x, int cnt);

    // miscellaneous
    /**
       Not yet documented.
       @return
    */
    const ArgType *getType() const {return type;}

    /**
       Not yet documented.
       @return
    */
    ArgType *getType() {return type;}

    /**
       Not yet documented.
       @return
    */
    const char *toString() const;

    /**
       De-initializes an Argument
    */
    ~Argument();

    /**
       Not yet documented.
       @param argType
       @param printref
       @return
    */
    static const char *
    getArgTypeStr(const ArgType *argType, Bool printref = True);

    // ----------------------------------------------------------------------
    // Argument Private Part
    // ----------------------------------------------------------------------
  private:
    char *str;
    void garbage();
    void init(ArgType_Type);
    GarbagePolicy policy;

    // ----------------------------------------------------------------------
    // Argument Conceptually private
    // ----------------------------------------------------------------------
  public:
    ArgType *type;
    Database *db;

    unsigned char *code(unsigned int &) const;

    union {
      eyedblib::int16 i16;
      eyedblib::int32 i32;
      eyedblib::int64 i64;
      char *s;
      char c;
      unsigned char by;
      double d;
      Oid *oid;
      Object *o;
      void *x;
      ArgArray *array;
      struct {
	int size;
	unsigned char *data;
      } raw;
      struct {
	int cnt;
	eyedblib::int16 *i;
      } arr_i16;
      struct {
	int cnt;
	eyedblib::int32 *i;
      } arr_i32;
      struct {
	int cnt;
	eyedblib::int64 *i;
      } arr_i64;
      struct {
	int cnt;
	char **s;
      } arr_s;
      struct {
	int cnt;
	char *c;
      } arr_c;
      struct {
	int cnt;
	double *d;
      } arr_d;
      struct {
	int cnt;
	Oid *oid;
      } arr_oid;
      struct {
	int cnt;
	Object **o;
      } arr_o;
    } u;
  };

  /**
     Not yet documented.
  */
  class ArgArray : public gbxObject {

    // ----------------------------------------------------------------------
    // ArgArray Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented.
    */
    ArgArray();

    /**
       Not yet documented.
       @param args
       @param cnt
    */
    ArgArray(const Argument **args, int cnt);

    /**
       Not yet documented.
       @param args
       @param cnt
       @param policy
    */
    ArgArray(Argument **args, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param cnt
       @param policy
    */
    ArgArray(int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @param args
       @param cnt
       @param policy
    */
    void set(Argument **args, int cnt, Argument::GarbagePolicy policy);

    /**
       Not yet documented.
       @return
    */
    Argument **getArgs() {return args;}

    /**
       Not yet documented.
       @return
    */
    int getCount() const {return cnt;}
		     
    /**
       Not yet documented.
       @param arg
    */
    ArgArray(const ArgArray &arg);

    /**
       Not yet documented.
       @param arg
       @return 
    */
    ArgArray &operator=(const ArgArray &arg);

    /**
       Not yet documented.
       @param c
       @param n
    */
    ArgArray(unsigned char *c, unsigned int n);

    /**
       Not yet documented.
       @param n
       @return
    */
    const Argument *operator[](int n) const {
      return ((n >= 0 && n < cnt) ? args[n] : (Argument *)0);
    }

    /**
       Not yet documented.
       @param n
       @return
    */
    Argument *operator[](int n) {
      return ((n >= 0 && n < cnt) ? args[n] : (Argument *)0);
    }

    /**
       Not yet documented.
       @return
    */
    ArgType_Type getType() const;

    /**
       Not yet documented.
       @param ??
       @return
    */
    unsigned char *code(unsigned int &) const;

    /**
       Not yet documented.
       @return
    */
    const char *toString() const;

    /**
       Not yet documented.
    */
    ~ArgArray();

    // ----------------------------------------------------------------------
    // ArgArray Private Part
    // ----------------------------------------------------------------------
  private:
    Argument::GarbagePolicy policy;
    void garbage();
    char *str;
    int cnt;
    Argument **args;
  };

  class ArgumentPtr : public gbxObjectPtr {

  public:
    ArgumentPtr(Argument *o = 0) : gbxObjectPtr(o) { }

    Argument *getArgument() {return dynamic_cast<Argument *>(o);}
    const Argument *getArgument() const {return dynamic_cast<Argument *>(o);}

    Argument *operator->() {return dynamic_cast<Argument *>(o);}
    const Argument *operator->() const {return dynamic_cast<Argument *>(o);}
  };

  typedef std::vector<ArgumentPtr> ArgumentPtrVector;

  /**
     @}
  */

}

#endif
