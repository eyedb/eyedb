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
#include "AttrNative.h"
#include "CollectionBE.h"
#include <assert.h>

//#define E_XDR_TRACE

namespace eyedb {

enum {
  CharClass_Code = 10,
  ByteClass_Code,
  OidPClass_Code,
  Int16Class_Code,
  Int32Class_Code,
  Int64Class_Code,
  FloatClass_Code
};

const char char_class_name[]  = "char";
const char byte_class_name[]  = "byte";
const char oid_class_name[]   = "oid";
const char int16_class_name[] = "int16";
const char int32_class_name[] = "int32";
const char int64_class_name[] = "int64";
const char float_class_name[] = "float";

CharClass    *Char_Class;
ByteClass    *Byte_Class;
OidClass    *OidP_Class;
Int16Class   *Int16_Class;
Int32Class   *Int32_Class;
Int64Class   *Int64_Class;
FloatClass   *Float_Class;

static void
fprint_float(FILE *fd, double d);

static int
increment(int n, int dim[], const TypeModifier *tmod)
{
  if (n < tmod->ndims)
    {
      if (dim[n] == tmod->dims[n]-1)
	{
	  dim[n] = 0;
	  return increment(n-1, dim, tmod) + 1;
	}

      dim[n]++;
      return 0;
    }
}

static int
increment(int dim[], const TypeModifier *tmod)
{
  return increment(tmod->ndims-1, dim, tmod);
}

static const char *mk_indent(int indent, int n)
{
#define INCR 4
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
  static char *indstr;
  static int size;

  int q = indent + INCR * n;
  if (!indstr || size < q)
    {
      size = MAX(128, q);
      if (indstr)
	free(indstr);
      indstr = (char *)malloc(size);
      memset(indstr, ' ', size-1);
      indstr[size-1] = 0;
    }

  return &indstr[size-q-1];
}

static void
traceData(FILE *fd, int indent, Data inidata,
	  Data data, TypeModifier *tmod, int size,
	  void (*print_data)(FILE *fd, Data data))
{
  int i, j, k;

  int *dim = (int *)malloc(tmod->ndims * sizeof(int));
  for (j = 0; j < tmod->ndims; j++)
    {
      dim[j] = 0;
      fprintf(fd, "%s{\n", (j ? mk_indent(indent, j) : ""));
    }

  dim[tmod->ndims-1] = -1;
  for (i = 0; i < tmod->pdims; i++)
    {
      int r = increment(dim, tmod);
      for (k = r-1; k >= 0; k--)
	fprintf(fd, "\n%s}", mk_indent(indent, tmod->ndims - r + k));
      if (i)
	fprintf(fd, ",%s", (r ? "" : "\n"));
      for (k = 0; k < r; k++)
	fprintf(fd, "\n%s{", mk_indent(indent, tmod->ndims - r + k));
		  
      fprintf(fd, "%s%s  ", (r ? "\n" : ""),
	      mk_indent(indent, tmod->ndims-1));

      for (j = 0; j < tmod->ndims; j++)
	fprintf(fd, "[%d]", dim[j]);
      
      fprintf(fd, " = ");
      if (Attribute::isNull(inidata, 1, i))
	fprintf(fd, "%s", NullString);
      else
	print_data(fd, data);

      data += size;
    }

  for (j = tmod->ndims-1; j >= 0; j--)
    fprintf(fd, "\n%s}", mk_indent(indent - INDENT_INC, j));
  free(dim);
}

static void
print_int32_data(FILE *fd, Data data)
{
  eyedblib::int32 n;
#ifdef E_XDR
  x2h_32_cpy(&n, data);
#else
  memcpy(&n, data, sizeof(eyedblib::int32));
#endif
  fprintf(fd, "%d", n);
}

static void
print_int16_data(FILE *fd, Data data)
{
  eyedblib::int16 n;
#ifdef E_XDR
  x2h_16_cpy(&n, data);
#else
  memcpy(&n, data, sizeof(eyedblib::int16));
#endif
  fprintf(fd, "%d", n);
}

static void
print_int64_data(FILE *fd, Data data)
{
  eyedblib::int64 n;
#ifdef E_XDR
  x2h_64_cpy(&n, data);
#else
  memcpy(&n, data, sizeof(eyedblib::int64));
#endif
  fprintf(fd, "%lld", n);
}

static void
print_xdr_float_data(FILE *fd, Data data)
{
  double d;
#ifdef E_XDR
  x2h_f64_cpy(&d, data);
#else
  memcpy(&d, data, sizeof(double));
#endif
  fprint_float(fd, d);
}

static void
print_xdr_oid_data(FILE *fd, Data data)
{
  Oid oid;
#ifdef E_XDR
  eyedbsm::x2h_oid(oid.getOid(), data);
#else
  memcpy(oid.getOid(), data, sizeof(Oid));
#endif
  fprintf(fd, "%s", oid.getString());
}

#define MAXPRES 80
#define MINPRES  6

static void
fprint_float(FILE *fd, double d)
{
  double od = 0;
  char tok[128], fmt[32];

  for (int i = MINPRES; i < MAXPRES; i += 4)
    {
      sprintf(fmt, "%%.%dg", i);
      sprintf(tok, fmt, d);
      double cd = atof(tok);
      if (cd == d || (i != MINPRES && od != cd))
	{
	  fprintf(fd, tok);
	  return;
	}

      od = cd;
    }

  sprintf(fmt, "%%.%dg", MAXPRES);
  sprintf(tok, fmt, d);
  fprintf(fd, tok);
}

static void
print_float_data(FILE *fd, Data data)
{
  double d;
  memcpy(&d, data, sizeof(double));
  fprint_float(fd, d);
}

static void
print_byte_data(FILE *fd, Data data)
{
  fprintf(fd, "'\\%03o'", *data);
}

static void
print_oid_data(FILE *fd, Data data)
{
  Oid oid;
  memcpy(&oid, data, sizeof(Oid));
  fprintf(fd, oid.getString());
}

static void
print_char_data(FILE *fd, Data data)
{
  fprintf(fd, "'%c'", *(char *)data);
}

//
// BasicClass
//

BasicClass::BasicClass(const char *s) : Class(s)
{
  parent = Class_Class;
  type = _BasicClass_Type;
  *Cname = 0;

  AttrNative::copy(ClassITEMS, items, items_cnt, this);
}

BasicClass::BasicClass(Database *_db, const char *s) :
Class(_db, s)
{
  parent = Class_Class;
  type = _BasicClass_Type;
  *Cname = 0;

  AttrNative::copy(ClassITEMS, items, items_cnt, this);
}

void
BasicClass::copy_realize(const BasicClass &cl)
{
  code = cl.code;
  strcpy(Cname, cl.Cname);
}

BasicClass::BasicClass(const BasicClass &cl)
  : Class(cl)
{
  *Cname = 0;
  copy_realize(cl);
}

Status
BasicClass::loadComplete(const Class *cl)
{
  assert(cl->asBasicClass());
  Status s = Class::loadComplete(cl);
  if (s) return s;
  copy_realize(*cl->asBasicClass());
  return Success;
}

BasicClass &BasicClass::operator=(const BasicClass &cl)
{
  this->Class::operator=(cl);
  copy_realize(cl);
  return *this;
}

Status BasicClass::attrsComplete()
{
  //  return Success;
  return Class::attrsComplete();
}

const char *
BasicClass::getCName(Bool useAsRef) const
{
  if (useAsRef) {
      static const char prefix[] = "eyedb::";
      static const int prefix_len = strlen(prefix);
      strcpy(const_cast<BasicClass *>(this)->Cname, prefix);
      const_cast<BasicClass *>(this)->Cname[prefix_len] = *name + 'A' - 'a';
      const_cast<BasicClass *>(this)->Cname[prefix_len+1] = 0;
      strcat(const_cast<BasicClass *>(this)->Cname, name+1);
  }
  else {
    strcpy(const_cast<BasicClass *>(this)->Cname, "eyedblib::");
    strcat(const_cast<BasicClass *>(this)->Cname, name);
  }
  return Cname;
}

int BasicClass::getCode() const
{
  return code;
}

Status BasicClass::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s ", oid.getString());
  return ObjectPeer::trace_realize(this, fd, INDENT_INC, flags, rcm);
}

Status BasicClass::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating basic_class '%s'",
			 name);

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  eyedblib::int16 kk;
  Size alloc_size;
  Offset offset;
  Size size;
  Status status;

  alloc_size = 0;
  //idr->idr = 0;
  idr->setIDR((Size)0);
  Data data = 0;

  /*
  offset = IDB_CLASS_MAG_ORDER;
  int32_code (&data, &offset, &alloc_size, (eyedblib::int32 *)&mag_order);
  */
  offset = IDB_CLASS_IMPL_TYPE;
  status = IndexImpl::code(data, offset, alloc_size, idximpl);
  if (status) return status;

  offset = IDB_CLASS_MTYPE;
  eyedblib::int32 mt = m_type;
  int32_code (&data, &offset, &alloc_size, &mt);

  offset = IDB_CLASS_DSPID;
  eyedblib::int16 dspid = get_instdspid();
  int16_code (&data, &offset, &alloc_size, &dspid);

  offset = IDB_CLASS_HEAD_SIZE;
      
  status = class_name_code(db->getDbHandle(), getDataspaceID(), &data,
			       &offset, &alloc_size, name);
  if (status) return status;

  int16_code  (&data, &offset, &alloc_size, &code);

  size = offset;
  idr->setIDR(size, data);
  headerCode(_BasicClass_Type, size);

  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(),
				data, oid.getOid());

  return StatusMake(rpc_status);
}

Status BasicClass::update()
{
  return Success;
}

Status BasicClass::remove(const RecMode*)
{
  return Success;
}

Status BasicClass::setValue(Data)
{
  return Success;
}

Status BasicClass::getValue(Data*) const
{
  return Success;
}

Status
basicClassMake(Database *db, const Oid *oid, Object **o,
		  const RecMode *rcm, const ObjectHeader *hdr,
		  Data idr, LockMode lockmode, const Class*)
{
  RPCStatus rpc_status;
  Data temp;

  if (!idr)
    {
      temp = (unsigned char *)malloc(hdr->size);
      object_header_code_head(temp, hdr);
      rpc_status = objectRead(db->getDbHandle(), temp, 0, 0, oid->getOid(),
				  0, lockmode, 0);
    }
  else
    {
      temp = idr;
      rpc_status = RPCSuccess;
    }

  if (rpc_status == RPCSuccess)
    {
      eyedblib::int16 code;
      Offset offset;
      char *s;

      /*
      eyedblib::int32 mag_order;
      offset = IDB_CLASS_MAG_ORDER;
      int32_decode (temp, &offset, &mag_order);
      */

      IndexImpl *idximpl;
      offset = IDB_CLASS_IMPL_TYPE;
      Status status = IndexImpl::decode(db, temp, offset, idximpl);
      if (status) return status;

      int mt;
      offset = IDB_CLASS_MTYPE;
      int32_decode (temp, &offset, &mt);

      eyedblib::int16 dspid;
      offset = IDB_CLASS_DSPID;
      int16_decode (temp, &offset, &dspid);

      offset = IDB_CLASS_HEAD_SIZE;
      status = class_name_decode(db->getDbHandle(), temp, &offset, &s);
      if (status) return status;
      int16_decode(temp, &offset, &code);

      *o = db->getSchema()->getClass(s);
      free(s); s = 0;
      (*o)->asClass()->setExtentImplementation(idximpl, True);
      if (idximpl)
	idximpl->release();
      (*o)->asClass()->setInstanceDspid(dspid);

      ClassPeer::setMType((Class *)*o, (Class::MType)mt);

      status = ClassPeer::makeColls(db, (Class *)*o, temp);

      if (status)
	{
	  if (!idr)
	    delete temp;
	  return status;
	}
    }

  if (!idr)
    {
      if (!rpc_status)
	ObjectPeer::setIDR(*o, temp, hdr->size);
    }
  return StatusMake(rpc_status);
}

//
// Basic
// 

Status Basic::create()
{
  abort();
  return Success;
}

Status Basic::update()
{
  return Success;
}

Status Basic::remove(const RecMode*)
{
  return Success;
}

Basic::Basic(Database *_db, const Dataspace *_dataspace) :
  Instance(_db, _dataspace)
{
}

Basic::Basic(const Basic *o) : Instance(o)
{
}

Basic::Basic(const Basic &o) : Instance(o)
{
}

void Basic::init(void)
{
  ObjectPeer::setUnrealizable(Char_Class, True);
  ObjectPeer::setUnrealizable(Byte_Class, True);
  ObjectPeer::setUnrealizable(OidP_Class, True);
  ObjectPeer::setUnrealizable(Int16_Class, True);
  ObjectPeer::setUnrealizable(Int32_Class, True);
  ObjectPeer::setUnrealizable(Int64_Class, True);
  ObjectPeer::setUnrealizable(Float_Class, True);
}

void Basic::_release(void)
{
  Char_Class->release();
  Byte_Class->release();
  OidP_Class->release();
  Int16_Class->release();
  Int32_Class->release();
  Int64_Class->release();
  Float_Class->release();
  Bool_Class->release();
}

Status
basicMake(Database *db, const Oid *oid, Object **o,
	      const RecMode *rcm, const ObjectHeader *hdr,
	      Data idr, LockMode lockmode, const Class *_class)
{
  RPCStatus rpc_status;
  BasicClass *cls = (BasicClass *)_class;

  if (!cls)
    cls = (BasicClass *)db->getSchema()->getClass(hdr->oid_cl);

  if (!cls)
    return Exception::make(IDB_CLASS_NOT_FOUND, "basic class '%s'",
			 OidGetString(&hdr->oid_cl));

  if (idr && !ObjectPeer::isRemoved(*hdr))
    *o = cls->newObj(idr + IDB_OBJ_HEAD_SIZE, False);
  else
    *o = cls->newObj();

  Status status = (*o)->setDatabase(db);

  if (status)
    return status;

  if (idr)
    rpc_status = RPCSuccess;
  else
    rpc_status = objectRead(db->getDbHandle(), (*o)->getIDR(), 0, 0,
				oid->getOid(), 0, lockmode, 0);

  return StatusMake(rpc_status);
}

Status
basic_class_trace(FILE *fd, int indent, unsigned int flags, const char *name,
		    const char *oid_str)
{
  char *indent_str = make_indent(indent);
  fprintf(fd, "%s %s : basic_class : class : object {\n", oid_str, name);
  fprintf(fd, "%s<abstract class>;\n};\n", indent_str);
  delete_indent(indent_str);
  return Success;
}

//
// CharClass
//

Status CharClass::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

CharClass::CharClass(const CharClass &cl)
  : BasicClass(cl)
{
}

CharClass &CharClass::operator=(const CharClass &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Status CharClass::traceData(FILE *fd, int indent, Data inidata,
				  Data data, TypeModifier *tmod) const
{
  int i;
  if (data)
    {
      if (tmod)
	{
	  if (tmod->ndims == 1)
	    fprintf(fd, "\"%s\"", (char *)data);
	  else if (tmod->ndims > 1) {
	    eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(char),
			print_char_data);
	  }
	  else
	    fprintf(fd, "'%c'",  *(char *)data);
	}
      else
	fprintf(fd, "'%c'",  *(char *)data);
    }
  else
    fprintf(fd, "''");

  return Success;
}
#define MetaB
CharClass::CharClass(Database *_db) : BasicClass(_db, char_class_name)
{
  code = CharClass_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif

  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(char);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

Object *CharClass::newObj(Database *_db) const
{
  Object *o = new Char(_db);
  ObjectPeer::setClass(o, this);
  return o;
}

Object *CharClass::newObj(Data data, Bool) const
{
  return new Char(* (char *)data);
}

void
CharClass::decode(void * hdata, // to
		     const void * xdata, // from
		     Size incsize, // temporarely necessary 
		     unsigned int nb) const
{
  Class::decode(hdata, xdata, incsize, nb);
}


void
CharClass::encode(void * xdata, // to
		     const void * hdata, // from
		     Size incsize, // temporarely necessary 
		     unsigned int nb) const
{
  Class::encode(xdata, hdata, incsize, nb);
}

int
CharClass::cmp(const void * xdata,
		  const void * hdata,
		  Size incsize, // temporarely necessary 
		  unsigned int nb) const
{
  return Class::cmp(hdata, xdata, incsize, nb);
}

//
// Int16Class
//

Int16Class::Int16Class(Database *_db) : BasicClass(_db, int16_class_name)
{
  setAliasName("short");
  code = Int16Class_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int16);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

Int16Class::Int16Class(const Int16Class &cl)
  : BasicClass(cl)
{
}

Int16Class &Int16Class::operator=(const Int16Class &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Object *Int16Class::newObj(Database *_db) const
{
  return new Int16(_db);
}

Object *Int16Class::newObj(Data data, Bool) const
{
#ifdef E_XDR
  eyedblib::int16 l;
  x2h_16_cpy(&l, data);
  //Offset offset = 0;
  //  xdr_int16_decode(data, &offset, &l);
  return new Int16(l);
#else
  return new Int16(* (eyedblib::int16 *)data);
#endif
}

Status Int16Class::traceData(FILE *fd, int indent, Data inidata,
				   Data data, TypeModifier *tmod) const
{
  int i;
  if (data)
    {
      eyedblib::int16 j;
      if (tmod)
	{
	  if (tmod->pdims > 1)
	    {
	      eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(eyedblib::int16),
			  print_int16_data);
	    }
	  else
	    {
	      /*
	      memcpy(&j, data, sizeof(eyedblib::int16));
	      fprintf(fd, "%d", j);
	      */
	      print_int16_data(fd, data);
	    }
	}
      else
	{
	  /*
	  memcpy(&j, data, sizeof(eyedblib::int16));
	  fprintf(fd, "%d", j);
	  */
	  print_int16_data(fd, data);
	}
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status Int16Class::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
Int16Class::decode(void * hdata, // to
		      const void * xdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int16Class::decode " << name << std::endl;
#endif
  CHECK_INCSIZE("decode", incsize, sizeof(eyedblib::int16));

  if (nb == 1) {
    x2h_16_cpy(hdata, xdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int16))
    x2h_16_cpy((char *)hdata + inc, (char *)xdata + inc);
}


void
Int16Class::encode(void * xdata, // to
		      const void * hdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int16Class::encode " << name << std::endl;
#endif
  CHECK_INCSIZE("encode", incsize, sizeof(eyedblib::int16));

  if (nb == 1) {
    h2x_16_cpy(xdata, hdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int16))
    h2x_16_cpy((char *)xdata + inc, (char *)hdata + inc);
}

int
Int16Class::cmp(const void * xdata,
		   const void * hdata,
		   Size incsize, // temporarely necessary 
		   unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int16Class::cmp " << name;
#endif
  CHECK_INCSIZE("cmp", incsize, sizeof(eyedblib::int16));

  if (nb == 1) {
    eyedblib::int16 l;
    x2h_16_cpy(&l, xdata);
    int r = memcmp(&l, hdata, sizeof(eyedblib::int16));
#ifdef E_XDR_TRACE
    std::cout << " -> " << r << std::endl;
#endif
    return r;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int16)) {
    eyedblib::int16 l;
    x2h_16_cpy(&l, (char *)xdata + inc);
    int r = memcmp(&l, (char *)hdata + inc, sizeof(eyedblib::int16));
    if (r)
      return r;
  }

  return 0;
}

//
// Int32Class
//

Int32Class::Int32Class(Database *_db) : BasicClass(_db, int32_class_name)
{
  setAliasName("int");
  code = Int32Class_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int32);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

Int32Class::Int32Class(const Int32Class &cl)
  : BasicClass(cl)
{
}

Int32Class &Int32Class::operator=(const Int32Class &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Object *Int32Class::newObj(Database *_db) const
{
  return new Int32(_db);
}

Object *Int32Class::newObj(Data data, Bool) const
{
#ifdef E_XDR
  eyedblib::int32 l;
  x2h_32_cpy(&l, data);
  //Offset offset = 0;
  //  xdr_int32_decode(data, &offset, &l);
  return new Int32(l);
#else
  return new Int32(* (eyedblib::int32 *)data);
#endif
}

Status Int32Class::traceData(FILE *fd, int indent, Data inidata,
				   Data data, TypeModifier *tmod) const
{
  int i, j, k;
  if (data)
    {
      eyedblib::int32 n;
      if (tmod)
	{
	  if (tmod->pdims > 1)
	    {
	      eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(eyedblib::int32),
			  print_int32_data);
	    }
	  else
	    {
	      /*
	      memcpy(&n, data, sizeof(eyedblib::int32));
	      fprintf(fd, "%d", n);
	      */
	      print_int32_data(fd, data);
	    }
	}
      else
	{
	  /*
	  memcpy(&n, data, sizeof(eyedblib::int32));
	  fprintf(fd, "%d", n);
	  */
	  print_int16_data(fd, data);
	}
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status Int32Class::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
Int32Class::decode(void * hdata, // to
		      const void * xdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int32Class::decode " << name << std::endl;
#endif
  CHECK_INCSIZE("decode", incsize, sizeof(eyedblib::int32));

  if (nb == 1) {
    x2h_32_cpy(hdata, xdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int32))
    x2h_32_cpy((char *)hdata + inc, (char *)xdata + inc);
}


void
Int32Class::encode(void * xdata, // to
		      const void * hdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int32Class::encode " << name << std::endl;
#endif
  CHECK_INCSIZE("encode", incsize, sizeof(eyedblib::int32));

  if (nb == 1) {
    h2x_32_cpy(xdata, hdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int32))
    h2x_32_cpy((char *)xdata + inc, (char *)hdata + inc);
}

int
Int32Class::cmp(const void * xdata,
		   const void * hdata,
		   Size incsize, // temporarely necessary 
		   unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int32Class::cmp " << name;
#endif
  CHECK_INCSIZE("cmp", incsize, sizeof(eyedblib::int32));

  if (nb == 1) {
    eyedblib::int32 l;
    x2h_32_cpy(&l, xdata);
    int r = memcmp(&l, hdata, sizeof(eyedblib::int32));
#ifdef E_XDR_TRACE
    std::cout << " -> " << r << std::endl;
#endif
    return r;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int32)) {
    eyedblib::int32 l;
    x2h_32_cpy(&l, (char *)xdata + inc);
    int r = memcmp(&l, (char *)hdata + inc, sizeof(eyedblib::int32));
    if (r)
      return r;
  }

  return 0;
}

//
// Int64Class
//

Int64Class::Int64Class(Database *_db) : BasicClass(_db, int64_class_name)
{
  setAliasName("long");

  code = Int64Class_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int64);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

Int64Class::Int64Class(const Int64Class &cl)
  : BasicClass(cl)
{
}

Int64Class &Int64Class::operator=(const Int64Class &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Object *Int64Class::newObj(Database *_db) const
{
  return new Int64(_db);
}

Object *Int64Class::newObj(Data data, Bool) const
{
#ifdef E_XDR
  eyedblib::int64 l;
  // called when loaded from DB, eyedboql:
  // i := new int64(123);
  // \p (i.e. loading)
  // i->toString(); (i.e. loading)

  x2h_64_cpy(&l, data);
  //Offset offset = 0;
  //xdr_int64_decode(pdata, &offset, &l);
  return new Int64(l);
#else
  return new Int64(* (eyedblib::int64 *)data);
#endif
}

Status Int64Class::traceData(FILE *fd, int indent, Data inidata,
				   Data data, TypeModifier *tmod) const
{
  int i, j, k;
  if (data)
    {
      eyedblib::int64 n;
      if (tmod)
	{
	  if (tmod->pdims > 1) {
	    eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(eyedblib::int64),
			print_int64_data);
	  }
	  else {
	    /*
	    memcpy(&n, data, sizeof(eyedblib::int64));
	    fprintf(fd, "%lld", n);
	    */
	    print_int64_data(fd, data);
	  }
	}
      else {
	/*
	memcpy(&n, data, sizeof(eyedblib::int64));
	fprintf(fd, "%lld", n);
	*/
	print_int64_data(fd, data);
      }
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status Int64Class::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
Int64Class::decode(void * hdata, // to
		      const void * xdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int64Class::decode " << name << std::endl;
#endif
  CHECK_INCSIZE("decode", incsize, sizeof(eyedblib::int64));

  if (nb == 1) {
    x2h_64_cpy(hdata, xdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int64))
    x2h_64_cpy((char *)hdata + inc, (char *)xdata + inc);
}


void
Int64Class::encode(void * xdata, // to
		      const void * hdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int64Class::encode " << name << std::endl;
#endif
  CHECK_INCSIZE("encode", incsize, sizeof(eyedblib::int64));

  if (nb == 1) {
    h2x_64_cpy(xdata, hdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int64))
    h2x_64_cpy((char *)xdata + inc, (char *)hdata + inc);
}

int
Int64Class::cmp(const void * xdata,
		   const void * hdata,
		   Size incsize, // temporarely necessary 
		   unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "Int64Class::cmp " << name;
#endif
  CHECK_INCSIZE("cmp", incsize, sizeof(eyedblib::int64));

  if (nb == 1) {
    eyedblib::int64 l;
    x2h_64_cpy(&l, xdata);
    int r = memcmp(&l, hdata, sizeof(eyedblib::int64));
#ifdef E_XDR_TRACE
    std::cout << " -> " << r << std::endl;
#endif
    return r;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::int64)) {
    eyedblib::int64 l;
    x2h_64_cpy(&l, (char *)xdata + inc);
    int r = memcmp(&l, (char *)hdata + inc, sizeof(eyedblib::int64));
    if (r)
      return r;
  }

  return 0;
}

//
// ByteClass
//

ByteClass::ByteClass(Database *_db) : BasicClass(_db, "byte")
{
  code = ByteClass_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(unsigned char);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

Object *ByteClass::newObj(Database *_db) const
{
  return new Byte(_db);
}

Object *ByteClass::newObj(Data data, Bool) const
{
  return new Byte(* (unsigned char *)data);
}

ByteClass::ByteClass(const ByteClass &cl)
  : BasicClass(cl)
{
}

ByteClass &ByteClass::operator=(const ByteClass &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Status ByteClass::traceData(FILE *fd, int indent, Data inidata,
				  Data data, TypeModifier *tmod) const
{
  int i;
  if (data)
    {
      unsigned char j;
      if (tmod)
	{
	  if (tmod->pdims > 1)
	    {
	      eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(unsigned char),
			  print_byte_data);
	    }
	  else
	    {
	      memcpy(&j, data, sizeof(unsigned char));
	      fprintf(fd, "\\%03o", j);
	    }
	}
      else
	{
	  memcpy(&j, data, sizeof(unsigned char));
	  fprintf(fd, "\\%03o", j);
	}
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status ByteClass::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
ByteClass::decode(void * hdata, // to
		     const void * xdata, // from
		     Size incsize, // temporarely necessary 
		     unsigned int nb) const
{
  Class::decode(hdata, xdata, incsize, nb);
}


void
ByteClass::encode(void * xdata, // to
		     const void * hdata, // from
		     Size incsize, // temporarely necessary 
		     unsigned int nb) const
{
  Class::encode(xdata, hdata, incsize, nb);
}

int
ByteClass::cmp(const void * xdata,
		  const void * hdata,
		  Size incsize, // temporarely necessary 
		  unsigned int nb) const
{
  return Class::cmp(hdata, xdata, incsize, nb);
}

//
// FloatClass
//

FloatClass::FloatClass(Database *_db) : BasicClass(_db, "float")
{
  code = FloatClass_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(double);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

FloatClass::FloatClass(const FloatClass &cl)
  : BasicClass(cl)
{
}

FloatClass &FloatClass::operator=(const FloatClass &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Object *FloatClass::newObj(Database *_db) const
{
  return new Float(_db);
}

Object *FloatClass::newObj(Data data, Bool) const
{
#ifdef E_XDR
  double d;
  x2h_f64_cpy(&d, data);
  //Offset offset = 0;
  //xdr_double_decode(data, &offset, &d);
  return new Float(d);
#else
  double d;
  memcpy(&d, data, sizeof(double));
  return new Float(d);
#endif
}

Status FloatClass::traceData(FILE *fd, int indent, Data inidata,
				   Data data, TypeModifier *tmod) const
{
  int i;
  if (data)
    {
      double j;
      if (tmod)
	{
	  if (tmod->pdims > 1)
	    {
	      eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(double),
			  print_float_data);
	    }
	  else
	    {
	      /*
	      memcpy(&j, data, sizeof(double));
	      fprint_float(fd, j);
	      */
	      print_xdr_float_data(fd, data);
	    }
	}
      else {
	/*
	  memcpy(&j, data, sizeof(double));
	  fprint_float(fd, j);
	*/
	print_xdr_float_data(fd, data);
      }
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status FloatClass::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
FloatClass::decode(void * hdata, // to
		      const void * xdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "FloatClass::decode " << name << std::endl;
#endif
  CHECK_INCSIZE("decode", incsize, sizeof(eyedblib::float64));

  if (nb == 1) {
    x2h_f64_cpy(hdata, xdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::float64))
    x2h_f64_cpy((char *)hdata + inc, (char *)xdata + inc);
}


void
FloatClass::encode(void * xdata, // to
		      const void * hdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "FloatClass::encode " << name << std::endl;
#endif
  CHECK_INCSIZE("encode", incsize, sizeof(eyedblib::float64));

  if (nb == 1) {
    h2x_f64_cpy(xdata, hdata);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::float64))
    h2x_f64_cpy((char *)xdata + inc, (char *)hdata + inc);
}

int
FloatClass::cmp(const void * xdata,
		   const void * hdata,
		   Size incsize, // temporarely necessary 
		   unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "FloatClass::cmp " << name;
#endif
  CHECK_INCSIZE("cmp", incsize, sizeof(eyedblib::float64));

  if (nb == 1) {
    eyedblib::float64 l;
    x2h_f64_cpy(&l, xdata);
    int r = memcmp(&l, hdata, sizeof(eyedblib::float64));
#ifdef E_XDR_TRACE
    std::cout << " -> " << r << std::endl;
#endif
    return r;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedblib::float64)) {
    eyedblib::float64 l;
    x2h_f64_cpy(&l, (char *)xdata + inc);
    int r = memcmp(&l, (char *)hdata + inc, sizeof(eyedblib::float64));
    if (r)
      return r;
  }

  return 0;
}

//
// OidClass
//

OidClass::OidClass(Database *_db) : BasicClass(_db, "oid")
{
  code = OidPClass_Code;
#ifdef MetaB
  setClass(BasicClass_Class);
  parent = Basic_Class;
#else
  setClass(Basic_Class);
  parent = Basic_Class;
#endif
  idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(Oid);
  idr_psize = idr_objsz;
  idr_vsize = 0;
}

OidClass::OidClass(const OidClass &cl)
  : BasicClass(cl)
{
}

OidClass &OidClass::operator=(const OidClass &cl)
{
  this->BasicClass::operator=(cl);
  return *this;
}

Object *OidClass::newObj(Database *_db) const
{
  return new OidP(_db);
}

Object *OidClass::newObj(Data data, Bool) const
{
#ifdef E_XDR
  Oid _oid;
  eyedbsm::x2h_oid(_oid.getOid(), data);
  //Offset offset = 0;
  //xdr_oid_decode(data, &offset, _oid.getOid());
  return new OidP(&_oid);
#else
  Oid _oid;
  memcpy(&_oid, data, sizeof(Oid));
  return new OidP(&_oid);
#endif
}

Status OidClass::traceData(FILE *fd, int indent, Data inidata,
				  Data data, TypeModifier *tmod) const
{
  int i;
  if (data)
    {
      Oid j;
      if (tmod)
	{
	  if (tmod->pdims > 1)
	    {
	      eyedb::traceData(fd, indent, inidata, data, tmod, sizeof(Oid),
			  print_oid_data);
	    }
	  else
	    {
	      /*
	      memcpy(&j, data, sizeof(Oid));
	      fprintf(fd, "%s", j.getString());
	      */
	      print_xdr_oid_data(fd, data);
	    }
	}
      else
	{
	  /*
	  memcpy(&j, data, sizeof(Oid));
	  fprintf(fd, "%s", j.getString());
	  */
	  print_xdr_oid_data(fd, data);
	}
    }
  else
    fprintf(fd, "''");

  return Success;
}

Status OidClass::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  return Class::trace_realize(fd, indent, flags, rcm);
}

void
OidClass::decode(void * hdata, // to
		    const void * xdata, // from
		    Size incsize, // temporarely necessary 
		    unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "OidClass::decode " << name << std::endl;
#endif
  CHECK_INCSIZE("decode", incsize, sizeof(Oid));

  if (nb == 1) {
    eyedbsm::Oid hoid;
    eyedbsm::x2h_oid(&hoid, xdata);
    memcpy(hdata, &hoid, sizeof(hoid));
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(Oid)) {
    eyedbsm::Oid hoid;
    eyedbsm::x2h_oid(&hoid, (char *)xdata + inc);
    memcpy((char *)hdata + inc, &hoid, sizeof(hoid));
  }
}


void
OidClass::encode(void * xdata, // to
		      const void * hdata, // from
		      Size incsize, // temporarely necessary 
		      unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "OidClass::encode " << name << std::endl;
#endif
  CHECK_INCSIZE("encode", incsize, sizeof(Oid));

  if (nb == 1) {
    eyedbsm::Oid hoid;
    memcpy(&hoid, hdata, sizeof(hoid));
    eyedbsm::h2x_oid(xdata, &hoid);
    return;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(Oid)) {
    eyedbsm::Oid hoid;
    memcpy(&hoid, (char *)hdata + inc, sizeof(hoid));
    eyedbsm::h2x_oid((char *)xdata + inc, &hoid);
  }
}

int
OidClass::cmp(const void * xdata,
		 const void * hdata,
		 Size incsize, // temporarely necessary 
		 unsigned int nb) const
{
#ifdef E_XDR_TRACE
  std::cout << "OidClass::cmp " << name;
#endif
  CHECK_INCSIZE("cmp", incsize, sizeof(Oid));

  if (nb == 1) {
    eyedbsm::Oid xoid;
    eyedbsm::x2h_oid(&xoid, xdata);
    int r = memcmp(&xoid, hdata, sizeof(xoid));
#ifdef E_XDR_TRACE
    std::cout << " -> " << r << std::endl;
#endif
    return r;
  }

  for (int n = 0, inc = 0; n < nb; n++, inc += sizeof(eyedbsm::Oid)) {
    eyedbsm::Oid xoid;
    eyedbsm::x2h_oid(&xoid, (char *)xdata + inc);
    int r = memcmp(&xoid, (char *)hdata + inc, sizeof(eyedbsm::Oid));
    if (r)
      return r;
  }

  return 0;
}

//
// Char
//

Char::Char(char c) : Basic()
{
  setClass(Char_Class);
  val = c;

  /*
  idr->idr_sz = cls->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  type = Basic_Type;
  idr->setIDR(getClass()->getIDRObjectSize());

  headerCode(_Basic_Type, idr->getSize());
}

Char::Char(Database *_db, char c, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Char_Class);
  val = c;

  /*
  idr->idr_sz = cls->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());
  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Char::Char(const Char *o) : Basic(o)
{
  val = (o ? o->val : 0);
}

Char::Char(const Char &o) : Basic(o)
{
  val = o.val;
}

Char& Char::operator=(const Char& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Char::setValue(Data data)
{
  val = *(char *)data;
  return Success;
}

Status Char::getValue(Data* data) const
{
  *data = (Data)&val;
  return Success;
}

Status Char::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating char");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  char_code(&data, &offset, &alloc_size, &val);

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(),
				data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Char::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating char");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  char_code(&data, &offset, &alloc_size, &val);

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Char::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s char = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Char::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "'%c';\n", val);
  return Success;
}

//
// Byte
//

Byte::Byte(unsigned char c) : Basic()
{
  setClass(Byte_Class);
  val = c;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());
  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Byte::Byte(Database *_db, unsigned char c, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Byte_Class);
  val = c;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());
  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Byte::Byte(const Byte *o) : Basic(o)
{
  val = (o ? o->val : 0);
}

Byte::Byte(const Byte &o) : Basic(o)
{
  val = o.val;
}

Byte& Byte::operator=(const Byte& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Byte::setValue(Data data)
{
  val = *(unsigned char *)data;
  return Success;
}

Status Byte::getValue(Data* data) const
{
  *data = (Data)&val;
  return Success;
}

Status Byte::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating Byte");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;
  Data data = idr->getIDR();
  char_code(&data, &offset, &alloc_size, (char *)&val);

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(),
				data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Byte::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating byte");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  char_code(&data, &offset, &alloc_size, (char *)&val);

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Byte::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s byte = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Byte::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "'\\%03o';\n", val);
  return Success;
}

//
// Int16
//

Int16::Int16(eyedblib::int16 i) : Basic()
{
  setClass(Int16_Class);
  val = i;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int16::Int16(Database *_db, eyedblib::int16 i, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Int16_Class);
  val = i;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int16::Int16(const Int16 *o) : Basic(o)
{
  val = (o ? o->val : 0);
}

Int16::Int16(const Int16 &o) : Basic(o)
{
  val = o.val;
}

Int16& Int16::operator=(const Int16& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Int16::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s int16 = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Int16::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%d;\n", val);
  return Success;
}

Status Int16::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating eyedblib::int16");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_int16_code(&data, &offset, &alloc_size, &val);

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(),
				data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Int16::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating eyedblib::int16");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_int16_code(&data, &offset, &alloc_size, &val);

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Int16::setValue(Data data)
{
  memcpy(&val, data, sizeof(eyedblib::int16));
  return Success;
}

Status Int16::getValue(Data* data) const
{
  *data = (Data)&val;
  return Success;
}

//
// OidP
//

OidP::OidP(const Oid *_oid) : Basic()
{
  setClass(OidP_Class);

  if (_oid)
    val = *_oid;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

OidP::OidP(Database *_db, const Oid *_oid, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(OidP_Class);

  if (_oid)
    val = *_oid;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

OidP::OidP(const OidP *o) : Basic(o)
{
  if (o)
    val = o->val;
}

OidP::OidP(const OidP &o) : Basic(o)
{
  val = o.val;
}

OidP& OidP::operator=(const OidP& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status OidP::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s oid = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status OidP::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s;\n", val.getString());
  return Success;
}

Status OidP::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating OidP");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_oid_code(&data, &offset, &alloc_size, val.getOid());

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());

  return StatusMake(rpc_status);
}

Status OidP::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating OidP");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_oid_code(&data, &offset, &alloc_size, val.getOid());

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status OidP::setValue(Data data)
{
  memcpy(&val, data, sizeof(Oid));
  return Success;
}

Status OidP::getValue(Data* data) const
{
  memcpy(data, &val, sizeof(Oid));
  return Success;
}

//
// Int32
//

Int32::Int32(eyedblib::int32 i) : Basic()
{
  setClass(Int32_Class);
  val = i;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int32::Int32(Database *_db, eyedblib::int32 i, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Int32_Class);
  val = i;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int32::Int32(const Int32 *o) : Basic(o)
{
  val = (o ? o->val : 0);
}

Int32::Int32(const Int32 &o) : Basic(o)
{
  val = o.val;
}

Int32& Int32::operator=(const Int32& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Int32::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s int32 = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Int32::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%d;\n", val);
  return Success;
}

Status Int32::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating eyedblib::int32");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_int32_code(&data, &offset, &alloc_size, &val);

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Int32::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating eyedblib::int32");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_int32_code(&data, &offset, &alloc_size, &val);

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Int32::setValue(Data data)
{
  val = *(eyedblib::int32 *)data;
  return Success;
}

Status Int32::getValue(Data* data) const
{
  *data = (Data)&val;
  return Success;
}

//
// Int64
//

Int64::Int64(eyedblib::int64 i) : Basic()
{
  setClass(Int64_Class);
  val = i;

  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int64::Int64(Database *_db, eyedblib::int64 i, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Int64_Class);
  val = i;

  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Int64::Int64(const Int64 *o) : Basic(o)
{
  val = (o ? o->val : 0);
}

Int64::Int64(const Int64 &o) : Basic(o)
{
  val = o.val;
}

Int64& Int64::operator=(const Int64& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Int64::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s int64 = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Int64::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%lld;\n", val);
  return Success;
}

Status Int64::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating eyedblib::int64");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
#ifdef E_XDR
  xdr_int64_code(&data, &offset, &alloc_size, &val);
#endif

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Int64::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating eyedblib::int64");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
#ifdef E_XDR
  xdr_int64_code(&data, &offset, &alloc_size, &val);
#endif

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Int64::setValue(Data data)
{
  val = *(eyedblib::int64 *)data;
  return Success;
}

Status Int64::getValue(Data* data) const
{
  *data = (Data)&val;
  return Success;
}

//
// Float
//

Float::Float(double d) : Basic()
{
  setClass(Float_Class);
  val = d;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Float::Float(Database *_db, double d, const Dataspace *_dataspace) : Basic(_db, _dataspace)
{
  setClass(Float_Class);
  val = d;

  /*
  idr->idr_sz = getClass()->getIDRObjectSize();
  idr->idr = (unsigned char *)malloc(idr->idr_sz);
  */
  idr->setIDR(getClass()->getIDRObjectSize());

  type = Basic_Type;

  headerCode(_Basic_Type, idr->getSize());
}

Float::Float(const Float *o) : Basic(o)
{
  val = (o ? o->val : 0.);
}

Float::Float(const Float &o) : Basic(o)
{
  val = o.val;
}

Float& Float::operator=(const Float& o)
{
  if (&o == this)
    return *this;

  *(Basic *)this = Basic::operator=((const Basic &)o);

  val = o.val;
  return *this;
}

Status Float::trace(FILE* fd, unsigned int flags, const RecMode *rcm) const
{
  fprintf(fd, "%s float = ", oid.getString());
  return trace_realize(fd, INDENT_INC, flags, rcm);
}

Status Float::trace_realize(FILE* fd, int indent, unsigned int flags, const RecMode *rcm) const
{
  fprint_float(fd, val);
  fprintf(fd, ";\n");
  return Success;
}

Status Float::create()
{
  if (oid.isValid())
    return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating float");

  IDB_CHECK_WRITE(db);

  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_double_code(&data, &offset, &alloc_size, &val);

  classOidCode();
  rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());

  return StatusMake(rpc_status);
}

Status Float::update()
{
  if (!oid.isValid()) /* && modify */
    return Exception::make(IDB_OBJECT_NOT_CREATED, "updating float");

  IDB_CHECK_WRITE(db);
  RPCStatus rpc_status;
  
  Offset offset;
  Size alloc_size;

  alloc_size = idr->getSize();
  offset = IDB_OBJ_HEAD_SIZE;

  Data data = idr->getIDR();
  xdr_double_code(&data, &offset, &alloc_size, &val);

  rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());

  return Success;
}

Status Float::setValue(Data data)
{
  memcpy(&val, data, sizeof(double));
  return Success;
}

Status Float::getValue(Data* data) const
{
  memcpy(data, &val, sizeof(double));
  return Success;
}

}
