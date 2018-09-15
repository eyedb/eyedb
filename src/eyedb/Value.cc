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


#include <eyedb/eyedb_p.h>
#include <sstream>

using namespace std;
//using std::ostringstream;

#define TRY_GETELEMS_GC

namespace eyedb {

  int
  Value::Struct::operator==(const Struct &stru)
  {
    if (attr_cnt != stru.attr_cnt)
      return 0;

    for (int i = 0; i < attr_cnt; i++)
      {
	if (strcmp(attrs[i]->name, stru.attrs[i]->name))
	  return 0;
	if (*attrs[i]->value != *stru.attrs[i]->value)
	  return 0;
      }

    return 1;
  }

  void
  Value::Struct::print(FILE *fd) const
  {
    fprintf(fd, "struct(");
    for (int ii = 0; ii < attr_cnt; ii++)
      {
	if (ii) fprintf(fd, ", ");
	fprintf(fd, "%s: ", attrs[ii]->name);
	attrs[ii]->value->print(fd);
      }
    fprintf(fd, ")");
  }

  std::string
  Value::Struct::toString() const
  {
    std::string s = "struct(";
    for (int ii = 0; ii < attr_cnt; ii++)
      {
	if (ii) s += ", ";
	s += std::string(attrs[ii]->name) + ": " + attrs[ii]->value->getString();
      }
    return s + ")";
  }

  int Value::operator==(const Value &val) const
  {
    if (type != val.type)
      return 0;

    switch(type)
      {
      case tNil:
	return 1;

      case tNull:
	return 1;

      case tBool:
	return val.b == b;

      case tByte:
	return val.by == by;

      case tChar:
	return val.c == c;

      case tShort:
	return val.s == s;

      case tInt:
	return val.i == i;

      case tLong:
	return val.l == l;

      case tDouble:
	return val.d == d;

      case tIdent:
      case tString:
	return !strcmp(val.str, str);

      case tData:
	return val.data.data == data.data &&
	  val.data.size == data.size;

      case tOid:
	return *val.oid == *oid;

      case tObject:
	return val.o == o;

      case tObjectPtr:
	return val.o_ptr->getObject() == o_ptr->getObject();

      case tList:
      case tBag:
      case tSet:
      case tArray:
	if (list->getCount() != val.list->getCount())
	  return 0;
	return val.list == list;

      case tPobj:
	return val.idx == idx;

      case tStruct:
	return *val.stru == *stru;

      default:
	abort();
      }

    return 0;
  }

  int cmp_objects(const Object *o, const Object *val_o)
  {
    if (o->getOid().isValid() && val_o->getOid().isValid())
      return o->getOid().getNX() < val_o->getOid().getNX();

    Size o_size;
    Data o_idr = o->getIDR(o_size);

    Size val_o_size;
    Data val_o_idr = val_o->getIDR(val_o_size);

    Size size = (o_size < val_o_size ? o_size : val_o_size);

    int r = memcmp(o_idr, val_o_idr, size);
    if (r < 0)
      return 1;
    if (r > 0)
      return 0;
    return o < val_o;
  }

  int Value::operator<(const Value &val) const
  {
    if (type != val.type)
      return (int)type < (int)val.type;

    switch(type) {

    case tNil:
    case tNull:
      return 0;

    case tBool:
      return b < val.b;

    case tByte:
      return by < val.by;

    case tChar:
      return c < val.c;

    case tShort:
      return s < val.s;

    case tInt:
      return i < val.i;

    case tLong:
      return l < val.l;

    case tDouble:
      return d < val.d;

    case tIdent:
    case tString:
      return strcmp(str, val.str) < 0 ? 1 : 0;

    case tData: {
      Size size = data.size;
      if (val.data.size < size)
	size = val.data.size;
      return memcmp(data.data, val.data.data, size) < 0 ? 1 : 0;
    }

    case tOid:
      return oid->getNX() < val.oid->getNX();

#if 1
    case tObject:
      return cmp_objects(o, val.o);

    case tObjectPtr:
      return cmp_objects(o_ptr->getObject(), val.o_ptr->getObject());
#else
    case tObject: {
      if (o->getOid().isValid() && val.o->getOid().isValid())
	return o->getOid().getNX() < val.o->getOid().getNX();

      Size o_size;
      Data o_idr = o->getIDR(o_size);

      Size val_o_size;
      Data val_o_idr = val.o->getIDR(val_o_size);

      Size size = (o_size < val_o_size ? o_size : val_o_size);

      int r = memcmp(o_idr, val_o_idr, size);
      if (r < 0)
	return 1;
      if (r > 0)
	return 0;
      return o < val.o;
    }
#endif

    default:
      return !(*this == val);
    }
  }

  int Value::operator!=(const Value &val) const
  {
    return !(*this == val);
  }

  Value::Value(const Value &val)
  {
    bufstr = NULL;
    type = tNil;
    auto_obj_garb = false;
    *this = val;
  }

  void Value::set(Object *_o)
  {
    type = tObject;
#ifdef TRY_GETELEMS_GC
    if (auto_obj_garb) {
      if (o)
	o->release();
      o = _o;
      if (o)
	o->incrRefCount();

      unvalid();
      return;
    }
#endif
    o = _o;
    unvalid();
  }

  Status
  Value::toOidObjectArray(Database *db, LinkedList &ll, Bool isobj,
			  const RecMode *rcm)
  {
    if (type == tOid) {
      if (isobj) {
	if (db) {
	  Object *x;
	  Status status = db->loadObject(*oid, x, rcm);
	  if (status)
	    return status;
	  ll.insertObjectLast(x);
	}
      }
      else
	ll.insertObjectLast(new Oid(*oid));
    }
    else if (type == tObject) {
      if (isobj) {
#ifdef TRY_GETELEMS_GC
	if (o)
	  o->incrRefCount();
#endif
	ll.insertObjectLast(o);
      }
      else if (o) {
	Oid *xoid = new Oid(o->getOid());
	ll.insertObjectLast(new Oid(*xoid));
      }
    }

    else if (type == tObjectPtr) {
      if (isobj) {
	if (o_ptr->getObject())
	  o_ptr->getObject()->incrRefCount();
	ll.insertObjectLast(o_ptr->getObject());
      }
      else if (o_ptr->getObject()) {
	Oid *xoid = new Oid(o_ptr->getObject()->getOid());
	ll.insertObjectLast(new Oid(*xoid));
      }
    }
    
    else if (type == tList || type == tBag || type == tSet || type == tArray) {
      LinkedListCursor cc(list);
      Value *v;
      Status status;
      while (cc.getNext((void *&)v))
	if (status = v->toOidObjectArray(db, ll, isobj, rcm))
	  return status;
    }
    else if (type == tStruct) {
      Status status;
      for (int ii = 0; ii < stru->attr_cnt; ii++)
	if (status = stru->attrs[ii]->value->toOidObjectArray(db, ll, isobj, rcm))
	  return status;
    }
    
    return Success;
  }

  Status
  Value::toValueArray(LinkedList &ll)
  {
    if (type == tList || type == tBag || type == tSet || type == tArray) {
      LinkedListCursor cc(list);
      Value *v;
      Status status;
      while (cc.getNext((void *&)v))
	if (status = v->toValueArray(ll))
	  return status;
    }
    else if (type == tStruct) {
      Status status;
      for (int ii = 0; ii < stru->attr_cnt; ii++)
	if (status = stru->attrs[ii]->value->toValueArray(ll))
	  return status;
    }
    else
      ll.insertObjectLast(new Value(*this));

    return Success;
  }

  Status
  Value::toArray(Database *db, ObjectPtrVector &obj_vect, const RecMode *rcm)
  {
    ObjectArray obj_array; // true or false ?
    Status s = toArray(db, obj_array, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status
  Value::toArray(Database *db, ObjectArray &objarr, const RecMode *rcm)
  {
    LinkedList ll;
    Status status = toOidObjectArray(db, ll, True, rcm);
    if (status)
      return status;

#ifdef TRY_GETELEMS_GC
    Object **o_arr = new Object*[ll.getCount()];
    LinkedListCursor cc(ll);
    Object *o;
    for (int ii = 0; cc.getNext((void *&)o); ii++)
      o_arr[ii] = o;

    objarr.set(o_arr, ll.getCount());
    delete [] o_arr;

    if (objarr.isAutoGarbage()) {
      // should release objects because auto garb objectarray has
      // increase refcnt
      LinkedListCursor cc2(ll);
      for (int ii = 0; cc2.getNext((void *&)o); ii++) {
	if (o)
	  o->release();
      }
    }
#else
    objarr.set(0, ll.getCount());

    LinkedListCursor cc(ll);
    Object *x;
    for (int ii = 0; cc.getNext((void *&)x); ii++)
      objarr.setObjectAt(ii, x);//objarr[ii] = x;

#endif
    return Success;
  }

  Status
  Value::toArray(OidArray &oidarr)
  {
    LinkedList ll;
    Status status = toOidObjectArray(0, ll, False, 0);
    if (status) return status;

    oidarr.set(0, ll.getCount());

    LinkedListCursor cc(ll);
    Oid *x;
    for (int ii = 0; cc.getNext((void *&)x); ii++) {
      oidarr[ii] = *x;
      delete x;
    }

    return Success;
  }

  Status
  Value::toArray(ValueArray &valarr)
  {
    LinkedList ll;
    Status status = toValueArray(ll);
    if (status)
      return status;

    valarr.set(0, ll.getCount());

    LinkedListCursor cc(ll);

    Value *x;
    for (int ii = 0; cc.getNext((void *&)x); ii++) {
      valarr.setValueAt(ii, *x); // valarr[ii] = *x;
      delete x;
    }

    return Success;
  }

  Value &Value::operator=(const Value &val)
  {
    if (this == &val)
      return *this;

    garbage();

    type = val.type;

    if (type == tString || type == tIdent)
      str = strdup(val.str);
    else if (type == tOid)
      oid = new Oid(*val.oid);
    else if (type == tList || type == tBag || type == tArray || type == tSet) {
      list = new LinkedList();
      LinkedListCursor cursor(val.list);
      Value *value;
      for (int ii = 0; cursor.getNext((void *&)value); ii++)
	if (value) list->insertObjectLast(new Value(*value));
    }
    else if (type == tStruct) {
      stru = new Struct(val.stru->attr_cnt);
      for (int ii = 0; ii < stru->attr_cnt; ii++)
	stru->attrs[ii] = new Attr(val.stru->attrs[ii]->name,
				   val.stru->attrs[ii]->value);
    }
#ifdef TRY_GETELEMS_GC
    else if (type == tObject) {
      o = val.o;
      if (auto_obj_garb && o) {
	o->incrRefCount();
      }
    }
#endif
    else if (type == tObjectPtr) {
      o_ptr = new ObjectPtr(*val.o_ptr);
    }
    else
      memcpy(this, &val, sizeof(*this));

    bufstr = NULL;

    return *this;
  }

  static void
  append(char *& buf, const char *s)
  {
    buf = (char *)realloc(buf, strlen(buf)+strlen(s)+1);
    strcat(buf, s);
  }

  //#define OPTIM_STRLIST

  static std::string
  getStringList(LinkedList *list, const char *head)
  {
#ifdef OPTIM_STRLIST
    int len = 16;
    char *s = (char *)malloc(len);
    *s = 0;

    LinkedListCursor cursor(list);
    Value *value;
    for (int i = 0; cursor.getNext((void *&)value); i++)
      {
	if (isBackendInterrupted())
	  {
	    set_backendInterrupt(False);
	    return "<interrupted>";
	  }

	char *x = value->getString();
	int l = strlen(x);
	if (l + strlen(s) + 4 >= len)
	  {
	    len += l + 200;
	    s = (char *)realloc(s, len);
	  }

	if (i) strcat(s, ", ");
	strcat(s, x);
      }

    if (strlen(s) + 4 >= len)
      {
	len += 4;
	s = (char *)realloc(s, len);
      }

    strcat(s, ")");
    return std::string(s);
#else
    std::string s = std::string(head) + "(";

    LinkedListCursor cursor(list);
    Value *value;
    for (int i = 0; cursor.getNext((void *&)value); i++)
      {
	if (i) s += ", ";
	s += value->getString();
      }

    s += ")";
    return s;
#endif
  }

  static void
  print_list(FILE *fd, LinkedList *list, const char *head)
  {
    fprintf(fd, "%s(", head);

    LinkedListCursor cursor(list);
    Value *value;
    for (int i = 0; cursor.getNext((void *&)value); i++)
      {
	if (i) fprintf(fd, ", ");
	value->print(fd);
      }

    fprintf(fd, ")");
  }

  const char *Value::getString() const
  {
    if (bufstr)
      return bufstr;

    char tok[32];

    *tok = 0;

    switch(type)
      {
      case tNil:
	((Value *)this)->bufstr = strdup(NilString);
	break;

      case tNull:
	((Value *)this)->bufstr = strdup(NullString);
	break;

      case tBool:
	sprintf(tok, "%s", (b ? "true" : "false"));
	break;

      case tByte:
	sprintf(tok, "\\0%d", b);
	break;

      case tChar:
	sprintf(tok, "'%c'", c);
	break;

      case tShort:
	sprintf(tok, "%d", s);
	break;

      case tInt:
	sprintf(tok, "%d", i);
	break;

      case tLong:
	sprintf(tok, "%lld", l);
	break;

      case tDouble:
	sprintf(tok, "%f", d);
	break;

      case tIdent:
	((Value *)this)->bufstr = strdup(str);
	break;

      case tString:
	((Value *)this)->bufstr = (char *)malloc(strlen(str)+3);
	sprintf(((Value *)this)->bufstr, "\"%s\"", str);
	break;

      case tData:
	sprintf(tok, "[0x%x, %u]", data.data, data.size);
	break;

      case tOid:
	((Value *)this)->bufstr = strdup(oid->getString());
	break;

      case tObject:
	{
	  ostringstream ostr;
	  ostr << o;
	  ((Value *)this)->bufstr = strdup(ostr.str().c_str());
	}
	break;

      case tObjectPtr:
	{
	  ostringstream ostr;
	  ostr << o_ptr->getObject();
	  ((Value *)this)->bufstr = strdup(ostr.str().c_str());
	}
	break;

      case tList:
	((Value *)this)->bufstr = strdup(getStringList(list, "list").c_str());
	break;

      case tSet:
	((Value *)this)->bufstr = strdup(getStringList(list, "set").c_str());
	break;

      case tBag:
	((Value *)this)->bufstr = strdup(getStringList(list, "bag").c_str());
	break;

      case tArray:
	((Value *)this)->bufstr = strdup(getStringList(list, "array").c_str());
	break;

      case tPobj:
	{
	  std::string x = str_convert((long)idx, "%x:obj");
	  ((Value *)this)->bufstr = strdup(x.c_str());
	}
	break;

      case tStruct:
	((Value *)this)->bufstr = strdup(stru->toString().c_str());
	break;

      default:
	abort();
      }

    if (*tok)
      ((Value *)this)->bufstr = strdup(tok);

    return bufstr;
  }

  void
  Value::print(FILE *fd) const
  {
    switch(type)
      {
      case tNil:
	fprintf(fd, NilString);
	break;

      case tNull:
	fprintf(fd, NullString);
	break;

      case tBool:
	fprintf(fd, "%s", (b ? "true" : "false"));
	break;

      case tByte:
	fprintf(fd, "\\0%d", b);
	break;

      case tChar:
	fprintf(fd, "'%c'", c);
	break;

      case tShort:
	fprintf(fd, "%d", s);
	break;

      case tInt:
	fprintf(fd, "%d", i);
	break;

      case tLong:
	fprintf(fd, "%lld", l);
	break;

      case tDouble:
	fprintf(fd, "%f", d);
	break;

      case tIdent:
	fprintf(fd, "%s", str);
	break;

      case tString:
	fprintf(fd, "\"%s\"", str);
	break;

      case tData:
	fprintf(fd, "0x%x", data);
	break;

      case tOid:
	fprintf(fd, oid->getString());
	break;

      case tObject:
	o->trace(fd);
	break;

      case tObjectPtr:
	o_ptr->getObject()->trace(fd);
	break;

      case tList:
	print_list(fd, list, "list");
	break;

      case tSet:
	print_list(fd, list, "set");
	break;

      case tBag:
	print_list(fd, list, "bag");
	break;

      case tArray:
	print_list(fd, list, "array");
	break;

      case tPobj:
	fprintf(fd, "%x:obj", idx);
	break;

      case tStruct:
	fprintf(fd, "%s", stru->toString().c_str());
	break;

      default:
	abort();
      }
  }

  const char *
  Value::getAttributeName(const Class *cl, Bool isIndirect)
  {
    if (cl->asCharClass())
      {
	if (isIndirect)
	  return "str";
	return "c";
      }

    if (isIndirect || (!cl->asBasicClass() && !cl->asEnumClass()))
      return "o";

    if (cl->asInt16Class())
      return "s";

    if (cl->asInt32Class())
      return "i";

    if (cl->asInt64Class())
      return "l";

    if (cl->asFloatClass())
      return "d";

    if (cl->asOidClass())
      return "oid";

    if (cl->asByteClass())
      return "by";

    return "<unknown class>";
  }

  ostream& operator<<(ostream& os, const Value &value)
  {
    os << value.getString();
    return os;
  }

  ostream& operator<<(ostream& os, const Value *value)
  {
    os << value->getString();
    return os;
  }

  void
  Value::garbage()
  {
    if (type == tString || type == tIdent)
      free(str);
    else if (type == tOid)
      delete oid;
    else if (type == tList || type == tBag || type == tSet || type == tArray) {
      LinkedListCursor cursor(list);
      Value *value;
      while (cursor.getNext((void *&)value))
	delete value;
      delete list;
    }
    else if (type == tStruct)
      delete stru;
#ifdef TRY_GETELEMS_GC
    else if (type == tObject) {
      if (auto_obj_garb && o) {
	o->release();
      }
    }
#endif
    else if (type == tObjectPtr)
      delete o_ptr;

    free(bufstr);
  }

  const char *
  Value::getStringType() const
  {
    return getStringType(type);
  }

  const char *
  Value::getStringType(Value::Type type)
  {
    switch(type)
      {
      case tNil:
	return "nil";

      case tNull:
	return "null";

      case tBool:
	return "bool";

      case tByte:
	return "byte";

      case tChar:
	return "char";

      case tShort:
	return "int16";

      case tInt:
	return "int32";

      case tLong:
	return "int64";

      case tDouble:
	return "double";

      case tIdent:
	return "ident";

      case tString:
	return "string";

      case tData:
	return "data";

      case tOid:
	return "oid";

      case tObject:
	return "object";

      case tObjectPtr:
	return "object_ptr";

      case tList:
	return "list";

      case tSet:
	return "set";

      case tBag:
	return "bag";

      case tArray:
	return "array";

      case tPobj:
	return "pobject";

      case tStruct:
	return "struct";

      default:
	return "<unknown>";
      }
  }

  Data
  Value::getData(Size *psize) const
  {
    switch(type)
      {
      case tNil:
      case tNull:
	if (psize)
	  *psize = 0;
	return 0;

      case tByte:
	if (psize)
	  *psize = sizeof(by);
	return (Data)&by;;

      case tChar:
	if (psize)
	  *psize = sizeof(c);
	return (Data)&c;;

      case tShort:
	if (psize)
	  *psize = sizeof(s);
	return (Data)&s;;

      case tInt:
	if (psize)
	  *psize = sizeof(i);
	return (Data)&i;

      case tLong:
	if (psize)
	  *psize = sizeof(l);
	return (Data)&l;;

      case tDouble:
	if (psize)
	  *psize = sizeof(d);
	return (Data)&d;;

      case tString:
	if (psize)
	  *psize = strlen(str)+1;
	return (Data)str;

      case tData:
	if (psize)
	  *psize = data.size;
	return data.data;

      case tOid:
	if (psize)
	  *psize = sizeof(oid);
	return (Data)&oid;;

      default:
	assert(0);
	if (psize)
	  *psize = 0;
	return (Data)0;;
      }
  }

  void
  Value::code(Data &idr, Offset &offset, Size &alloc_size) const
  {
    char x = type;
    char_code(&idr, &offset, &alloc_size, &x);

    switch(type)
      {
      case tNil:
      case tNull:
	break;

      case tBool:
	x = b;
	char_code(&idr, &offset, &alloc_size, &x);
	break;

      case tByte:
	char_code(&idr, &offset, &alloc_size, (char *)&by);
	break;

      case tChar:
	char_code(&idr, &offset, &alloc_size, &c);
	break;

      case tShort:
	int16_code(&idr, &offset, &alloc_size, &s);
	break;

      case tInt:
	int32_code(&idr, &offset, &alloc_size, &i);
	break;

      case tLong:
	int64_code(&idr, &offset, &alloc_size, &l);
	break;

      case tDouble:
	double_code(&idr, &offset, &alloc_size, &d);
	break;

      case tIdent:
      case tString:
	string_code(&idr, &offset, &alloc_size, str);
	break;

      case tData:
	throw *eyedb::Exception::make(eyedb::IDB_UNSERIALIZABLE_TYPE_ERROR, "tData");
	break;

      case tOid:
	oid_code(&idr, &offset, &alloc_size, oid->getOid());
	break;

      case tObject:
	throw *eyedb::Exception::make(eyedb::IDB_UNSERIALIZABLE_TYPE_ERROR, "tObject");
	break;

      case tObjectPtr:
	throw *eyedb::Exception::make(eyedb::IDB_UNSERIALIZABLE_TYPE_ERROR, "tObjectPtr");
	break;

      case tList:
      case tSet:
      case tBag:
      case tArray:
	{
	  int cnt = list->getCount();
	  int32_code(&idr, &offset, &alloc_size, &cnt);
	  LinkedListCursor cc(list);
	  Value *v;
	  while (cc.getNext((void *&)v))
	    v->code(idr, offset, alloc_size);
	}
	break;

      case tPobj:
	int32_code(&idr, &offset, &alloc_size, (eyedblib::int32 *)&idx);
	break;

      case tStruct:
	{
	  int32_code(&idr, &offset, &alloc_size, &stru->attr_cnt);
	  for (int ii = 0; ii < stru->attr_cnt; ii++)
	    {
	      string_code(&idr, &offset, &alloc_size, stru->attrs[ii]->name);
	      stru->attrs[ii]->value->code(idr, offset, alloc_size);
	    }

	}
	break;

      default:
	throw *eyedb::Exception::make(eyedb::IDB_INTERNAL_ERROR, "Unknown type in Value code()");
	break;
      }
  }

  void Value::setMustRelease(bool must_release)
  {
    if (type == tObject && o)
      o->setMustRelease(must_release);
  }

  Value::~Value()
  {
    garbage();
  }

  ValueArray::ValueArray(const Collection *coll)
  {
    values = NULL;
    value_cnt = 0;
    coll->getElements(*this);
  }

  ValueArray::ValueArray(const ValueArray &valarr)
  {
    value_cnt = 0;
    values = NULL;
    *this = valarr;
  }
 
  ValueArray::ValueArray(const ValueList &list)
  {
    value_cnt = 0;
    int cnt = list.getCount();
    if (!cnt) {
      values = 0;
      return;
    }

    values = (Value *)malloc(sizeof(Value) * cnt);
    memset(values, 0, sizeof(Value) * cnt);

    ValueListCursor c(list);
    Value value;

    for (; c.getNext(value); value_cnt++)
      values[value_cnt] = value;
  }

  ValueArray &ValueArray::operator=(const ValueArray &valarr)
  {
    set(valarr.values, valarr.value_cnt, True);
    return *this;
  }

  void ValueArray::set(Value *_values, unsigned int _value_cnt, Bool copy)
  {
    if (values)
      delete[] values;

    value_cnt = _value_cnt;

    if (copy) {
      values = new Value[value_cnt];
      
#ifdef TRY_GETELEMS_GC
      for (int i = 0; i < value_cnt; i++)
	values[i].setAutoObjGarbage(auto_obj_garb);
#endif

      if (_values) {
	for (int i = 0; i < value_cnt; i++) {
	  values[i] = _values[i];
	}
      }

      return;
    }
    
    values = _values;
  }

  void ValueArray::setMustRelease(bool must_release)
  {
    for (int i = 0; i < value_cnt; i++)
      values[i].setMustRelease(must_release);
  }

  void ValueArray::setAutoObjGarbage(bool _auto_obj_garb)
  {
    auto_obj_garb = _auto_obj_garb;
#ifdef TRY_GETELEMS_GC
    for (int i = 0; i < value_cnt; i++)
      values[i].setAutoObjGarbage(auto_obj_garb);
#endif
  }

  Status ValueArray::setValueAt(unsigned int ind, const Value &value)
  {
    if (ind >= value_cnt)
      return Exception::make(IDB_ERROR, "invalid range %d (maximun is %d)",
			     ind, value_cnt);

    values[ind] = value;
    return Success;
  }

  ValueList *
  ValueArray::toList() const
  {
    return new ValueList(*this);
  }

  ValueArray::~ValueArray()
  {
    if (values)
      delete [] values;
  }

  void
  Value::decode(Data idr, Offset &offset)
  {
    garbage();

    char x;
    char_decode(idr, &offset, &x);
    type = (Type)x;

    switch(type)
      {
      case tNil:
      case tNull:
	break;

      case tBool:
	char_decode(idr, &offset, &x);
	b = (Bool)x;
	break;

      case tByte:
	char_decode(idr, &offset, (char *)&by);
	break;

      case tChar:
	char_decode(idr, &offset, &c);
	break;

      case tShort:
	int16_decode(idr, &offset, &s);
	break;

      case tInt:
	int32_decode(idr, &offset, &i);
	break;

      case tLong:
	int64_decode(idr, &offset, &l);
	break;

      case tDouble:
	double_decode(idr, &offset, &d);
	break;

      case tIdent:
      case tString:
	{
	  char *y;
	  string_decode(idr, &offset, &y);
	  str = strdup(y);
	}
	break;

      case tData:
	break;

      case tOid:
	{
	  eyedbsm::Oid xoid;
	  oid_decode(idr, &offset, &xoid);
	  oid = new Oid(xoid);
	}
	break;

      case tObject:
	break;

      case tObjectPtr:
	break;

      case tList:
      case tSet:
      case tBag:
      case tArray:
	{
	  int cnt;
	  int32_decode(idr, &offset, &cnt);
	  list = new LinkedList();
	  for (int ii = 0; ii < cnt; ii++)
	    {
	      Value *v = new Value();
	      v->decode(idr, offset);
	      list->insertObjectLast(v);
	    }
	}
	break;

      case tPobj:
	int32_decode(idr, &offset, (eyedblib::int32 *)&idx);
	break;

      case tStruct:
	{
	  int cnt;
	  int32_decode(idr, &offset, &cnt);
	  stru = new Struct(cnt);
	  for (int ii = 0; ii < stru->attr_cnt; ii++)
	    {
	      char *y;
	      string_decode(idr, &offset, &y);
	      Value *v = new Value();
	      v->decode(idr, offset);
	      stru->attrs[ii] = new Attr(y, v);
	    }

	}
	break;

      default:
	abort();
	break;
      }
  }

  void decode_value(void *xdata, void *xvalue)
  {
    Offset offset = 0;
    ((Value *)xvalue)->decode((Data)((rpc_Data *)xdata)->data, offset);
  }

  ValueList::ValueList()
  {
    list = new LinkedList();
  }

  ValueList::ValueList(const ValueArray &value_array)
  {
    list = new LinkedList();
    for (int i = 0; i < value_array.getCount(); i++)
      insertValueLast(value_array[i]);
  }

  int ValueList::getCount() const
  {
    return list->getCount();
  }

  void
  ValueList::insertValue(const Value &value)
  {
    list->insertObject(new Value(value));
  }

  void
  ValueList::insertValueFirst(const Value &value)
  {
    list->insertObjectFirst(new Value(value));
  }

  void
  ValueList::insertValueLast(const Value &value)
  {
    list->insertObjectLast(new Value(value));
  }

  Bool
  ValueList::suppressValue(const Value &value)
  {
    LinkedListCursor c(list);
    Value *xvalue;
    while (c.getNext((void *&)xvalue))
      if (*xvalue == value) {
	list->deleteObject(xvalue);
	return True;
      }

    return False;
  }

  Bool
  ValueList::suppressPairValues(const Value &value1, const Value &value2)
  {
    LinkedListCursor c(list);
    int cnt = list->getCount();

    for (int n = 0; n < cnt; n += 2) {
      Value *xvalue1, *xvalue2;
      assert(c.getNext((void *&)xvalue1));
      assert(c.getNext((void *&)xvalue2));

      if (value1 == *xvalue1 && value2 == *xvalue2) {
	list->deleteObject(xvalue1);
	list->deleteObject(xvalue2);
	return True;
      }
    }

    return False;
  }

  Bool
  ValueList::exists(const Value &value) const
  {
    LinkedListCursor c(list);
    Value *xvalue;
    while (c.getNext((void *&)xvalue))
      if (*xvalue == value)
	return True;
    return False;
  }

  void
  ValueList::empty()
  {
    list->empty();
  }

  ValueArray *
  ValueList::toArray() const
  {
    int cnt = list->getCount();
    if (!cnt)
      return new ValueArray();
    Value *arr = (Value *)malloc(sizeof(Value) * cnt);
    memset(arr, 0, sizeof(Value) * cnt);

    LinkedListCursor c(list);
    Value *xvalue;
    for (int i = 0; c.getNext((void *&)xvalue); i++)
      arr[i] = *xvalue;
  
    ValueArray *value_arr = new ValueArray(arr, cnt);
    free(arr);
    return value_arr;
  }

  ValueList::~ValueList()
  {
    LinkedListCursor c(list);
    Value *xvalue;
    while (c.getNext((void *&)xvalue))
      delete xvalue;
    delete list;
  }

  ValueListCursor::ValueListCursor(const ValueList &valuelist)
  {
    c = new LinkedListCursor(valuelist.list);
  }

  ValueListCursor::ValueListCursor(const ValueList *valuelist)
  {
    c = new LinkedListCursor(valuelist->list);
  }

  Bool
  ValueListCursor::getNext(Value &value)
  {
    Value *xvalue;
    if (c->getNext((void *&)xvalue))
      {
	value = *xvalue;
	return True;
      }

    return False;
  }

  ValueListCursor::~ValueListCursor()
  {
    delete c;
  }
}
