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

namespace eyedb {

  void
  decode_atom_array(rpc_ClientData *data, void *_atom_array,
			int count)
  {
    IteratorAtom *atom_array = (IteratorAtom *)_atom_array;
    Offset offset = 0;
    Data idr = (Data)data->data;

    IteratorAtom *p = atom_array;

    for (int c = 0; c < count; c++, p++)
      p->decode(idr, &offset);

    free(idr);
  }

  void
  IteratorAtom::code(Data *idr, Offset *offset, Size *alloc_size)
  {
    eyedblib::int16 t = (eyedblib::int16)type;
    int16_code(idr, offset, alloc_size, &t);

    switch(type)
      {
      case IteratorAtom_INT16:
	int16_code(idr, offset, alloc_size, &i16);
	break;
      
      case IteratorAtom_INT32:
	int32_code(idr, offset, alloc_size, &i32);
	break;
      
      case IteratorAtom_INT64:
	int64_code(idr, offset, alloc_size, &i64);
	break;
      
      case IteratorAtom_CHAR:
	char_code(idr, offset, alloc_size, &c);
	break;
      
      case IteratorAtom_DOUBLE:
	double_code(idr, offset, alloc_size, &d);
	break;
      
      case IteratorAtom_STRING:
	string_code(idr, offset, alloc_size, str);
	break;
      
      case IteratorAtom_OID:
	oid_code(idr, offset, alloc_size, &oid);
	break;
      
      case IteratorAtom_IDR:
	{
	  eyedblib::int32 size = (eyedblib::int32)data.size;
	  int32_code(idr, offset, alloc_size, &size);
	  buffer_code(idr, offset, alloc_size, data.idr, data.size);
	  break;
	}      

#ifndef NEW_ITER_ATOM
      case IteratorAtom_BOOL:
	{
	  char tc = (char)b;
	  char_code(idr, offset, alloc_size, &tc);
	  break;
	}
      
      case IteratorAtom_IDENT:
	string_code(idr, offset, alloc_size, ident);
	break;
      
      case IteratorAtom_LISTSEP:
	{
	  char cc = (char)open;
	  char_code(idr, offset, alloc_size, &cc);
	}
	break;
#endif

      default:
	assert(0);
	break;
      }
  }

  void
  IteratorAtom::decode(Data idr, Offset *offset)
  {
    memset(this, 0, sizeof(IteratorAtom));
    eyedblib::int16 t;
    int16_decode(idr, offset, &t);
    type = (IteratorAtomType)t;

    switch(type)
      {
      case IteratorAtom_INT16:
	int16_decode(idr, offset, &i16);
	break;
      
      case IteratorAtom_INT32:
	int32_decode(idr, offset, &i32);
	break;
      
      case IteratorAtom_INT64:
	int64_decode(idr, offset, &i64);
	break;
      
      case IteratorAtom_CHAR:
	char_decode(idr, offset, &c);
	break;
      
      case IteratorAtom_DOUBLE:
	double_decode(idr, offset, &d);
	break;
      
      case IteratorAtom_OID:
	oid_decode(idr, offset, &oid);;
	break;
      
      case IteratorAtom_STRING:
	{
	  char *ts;
	  string_decode(idr, offset, &ts);
	  str = strdup(ts);
	}
	break;
      
      case IteratorAtom_IDR:
	{
	  eyedblib::int32 size;
	  int32_decode(idr, offset, &size);
	  data.size = size;
	  data.idr = (unsigned char *)malloc(data.size);
	  buffer_decode(idr, offset, data.idr, data.size);
	}
	break;
      
#ifndef NEW_ITER_ATOM
      case IteratorAtom_BOOL:
	{
	  char tc;
	  char_decode(idr, offset, &tc);
	  b = (Bool)tc;
	  break;
	}
      
      case IteratorAtom_IDENT:
	{
	  char *ts;
	  string_decode(idr, offset, &ts);
	  ident = strdup(ts);
	}
	break;
      
      case IteratorAtom_LISTSEP:
	{
	  char cc;
	  char_decode(idr, offset, &cc);
	  open = (Bool)cc;
	}      
	break;
#endif

      default:
	assert(0);
	break;
      }
  }

  IteratorAtom::IteratorAtom()
  {
    memset(this, 0, sizeof(IteratorAtom));
  }

  IteratorAtom::IteratorAtom(const IteratorAtom &atom)
  {
    *this = atom;
  }

  void IteratorAtom::print(FILE *fd)
  {
    fprintf(fd, "%s", getString());
      
  }

  char *IteratorAtom::getString()
  {
    if (fmt_str)
      return fmt_str;

    switch(type)
      {
      case IteratorAtom_INT16:
	{
	  char tok[16];
	  sprintf(tok, "%d", i16);
	  fmt_str = strdup(tok);
	  break;
	}

      case IteratorAtom_INT32:
	{
	  char tok[16];
	  sprintf(tok, "%ld", i32);
	  fmt_str = strdup(tok);
	  break;
	}

      case IteratorAtom_INT64:
	{
	  char tok[16];
	  sprintf(tok, "%lld", i64);
	  fmt_str = strdup(tok);
	  break;
	}

      case IteratorAtom_CHAR:
	{
	  char tok[8];
	  sprintf(tok, "'%c'", c);
	  fmt_str = strdup(tok);
	  break;
	}

      case IteratorAtom_DOUBLE:
	{
	  char tok[16];
	  sprintf(tok, "%f", d);
	  fmt_str = strdup(tok);
	  break;
	}

      case IteratorAtom_STRING:
	{
	  char *tok = (char *)malloc(strlen(str)+3);
	  sprintf(tok, "\"%s\"", str);
	  fmt_str = tok;
	  break;
	}

      case IteratorAtom_OID:
	fmt_str = strdup(OidGetString(&oid));
	break;

      case IteratorAtom_IDR:
	{
	  char tok[64];
	  sprintf(tok, "buffer 0x%x, size %d", data.idr, data.size);
	  fmt_str = strdup(tok);
	  break;
	}

#ifndef NEW_ITER_ATOM
      case IteratorAtom_NULL:
	fmt_str = strdup(NullString);
	break;

      case IteratorAtom_NIL:
	fmt_str = strdup(NilString);
	break;

      case IteratorAtom_BOOL:
	fmt_str = (b ? strdup("true") : strdup("false"));
	break;

      case IteratorAtom_IDENT:
	fmt_str = strdup(ident);
	break;

      case IteratorAtom_LISTSEP:
	fmt_str = strdup(open ? "(" : ")");
	break;
#endif

      default:
	assert(0);
	return 0;
      }

    return fmt_str;
  }

  int IteratorAtom::getSize() const
  {
    switch(type)
      {
      case IteratorAtom_INT16:
	return sizeof(eyedblib::int16);

      case IteratorAtom_INT32:
	return sizeof(eyedblib::int32);

      case IteratorAtom_INT64:
	return sizeof(eyedblib::int64);

      case IteratorAtom_CHAR:
	return sizeof(char);

      case IteratorAtom_DOUBLE:
	return sizeof(double);

      case IteratorAtom_STRING:
	return sizeof(eyedblib::int32) + strlen(str) + 1;

      case IteratorAtom_OID:
	return sizeof(Oid);

      case IteratorAtom_IDR:
	return sizeof(eyedblib::int32) + data.size;

#ifndef NEW_ITER_ATOM
      case IteratorAtom_NULL:
	return 0;

      case IteratorAtom_NIL:
	return 0;

      case IteratorAtom_BOOL:
	return sizeof(char);

      case IteratorAtom_IDENT:
	return sizeof(eyedblib::int32) + strlen(ident) + 1;

      case IteratorAtom_LISTSEP:
	return sizeof(char);
#endif
      case 0:
	return 0;

      default:
	assert(0);
	return 0;
      }
  }

  IteratorAtom & IteratorAtom::operator =(const IteratorAtom &atom)
  {
    garbage();

    memcpy(this, &atom, sizeof(IteratorAtom));

    fmt_str = 0;

    if (type == IteratorAtom_STRING)
      str = strdup(atom.str);
#ifndef NEW_ITER_ATOM
    else if (type == IteratorAtom_IDENT)
      ident = strdup(atom.ident);
#endif
    else if (type == IteratorAtom_IDR) {
      data.size = atom.data.size;
      data.idr = (unsigned char *)malloc(data.size);
      memcpy(data.idr, atom.data.idr, data.size);
    }

    return *this;
  }

  Value *
  IteratorAtom::toValue() const
  {
    Value *value = NULL;

    switch(type)
      {
      case IteratorAtom_INT16:
	value = new Value(i16);
	break;
      
      case IteratorAtom_INT32:
	value = new Value(i32);
	break;
      
      case IteratorAtom_INT64:
	value = new Value(i64);
	break;
      
      case IteratorAtom_CHAR:
	value = new Value(c);
	break;
      
      case IteratorAtom_DOUBLE:
	value = new Value(d);
	break;
      
      case IteratorAtom_STRING:
	value = new Value(str);
	break;
      
      case IteratorAtom_OID:
	value = new Value(oid);
	break;
      
      case IteratorAtom_IDR:
	{
	  // MIND : memory leaks!!!!
	  Data idr = (unsigned char *)malloc(data.size);
	  memcpy(idr, data.idr, data.size);
	  value = new Value(idr, data.size);
	}
	break;

#ifndef NEW_ITER_ATOM
      case IteratorAtom_NULL:
	value = new Value(True, True);
	break;
      
      case IteratorAtom_NIL:
	value = new Value();
	break;
      
      case IteratorAtom_BOOL:
	value = new Value(b);
	break;
      
      case IteratorAtom_IDENT:
	value = new Value(str, True);
	break;
      
      case IteratorAtom_LISTSEP:
	break;
#endif

      default:
	assert(0);
	break;
      }

    return value;
  }

  void IteratorAtom::garbage()
  {
    free(fmt_str);
    if (type == IteratorAtom_STRING)
      free(str);
#ifndef NEW_ITER_ATOM
    else if (type == IteratorAtom_IDENT)
      free(ident);
#endif
    else if (type == IteratorAtom_IDR)
      free(data.idr);

    fmt_str = 0;
    type = (IteratorAtomType)0;
  }

  IteratorAtom::~IteratorAtom()
  {
    garbage();
  }
}
