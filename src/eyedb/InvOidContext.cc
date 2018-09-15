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
#include "InvOidContext.h"
#include <eyedblib/rpc_be.h>

namespace eyedb {

  static LinkedList st_inv_list;
  static LinkedList *inv_list;

  InvOidContext::InvOidContext(const Oid &_objoid,
			       const Attribute *attr,
			       const Oid &_valoid)
  {
    objoid = _objoid;
    valoid = _valoid;;
    attr_offset = attr->getPersistentIDR();
    attr_num = attr->getNum();
#ifdef TRACE
    printf("InvOidContext_1(%s, %d, %d, %s)\n",
	   objoid.toString(), attr_num, attr_offset, valoid.toString());
#endif
  }

  InvOidContext::InvOidContext(const Oid &_objoid,
			       int _attr_num, Offset _attr_offset,
			       const Oid &_valoid)
  {
    objoid = _objoid;
    valoid = _valoid;;
    attr_offset = _attr_offset;
    attr_num = _attr_num;
#ifdef TRACE
    printf("InvOidContext_2(%s, %d, %d, %s)\n",
	   objoid.toString(), attr_num, attr_offset, valoid.toString());
#endif
  }

  void
  InvOidContext::code(Data *data, LinkedList &list, Bool toDel,
		      Size &size)
  {
    LinkedListCursor c(list);
    InvOidContext *ctx;

    size = 0;
    eyedblib::int32 x = list.getCount();
    Offset offset = 0;

    *data = 0;
    int32_code(data, &offset, &size, &x);

    for (int i = 0; c.getNext((void *&)ctx); i++) {
      oid_code(data, &offset, &size, ctx->objoid.getOid());
      x = ctx->attr_num;
      int32_code(data, &offset, &size, &x);
      x = ctx->attr_offset;
      int32_code(data, &offset, &size, &x);
      oid_code(data, &offset, &size, ctx->valoid.getOid());
#ifdef TRACE
      printf("coding[%d] -> %s %d %d %s\n",
	     i, ctx->objoid.toString(), ctx->attr_num, ctx->attr_offset,
	     ctx->valoid.toString());
#endif
      if (toDel)
	delete ctx;
    }
  }

  void
  InvOidContext::decode(Data data, LinkedList &list)
  {
    eyedblib::int32 cnt;
    Offset offset = 0;

    int32_decode(data, &offset, &cnt);

    for (int i = 0; i < cnt; i++) {
      eyedbsm::Oid objoid, valoid;
      eyedblib::int32 attr_num, attr_offset;

      oid_decode(data, &offset, &objoid);
      int32_decode(data, &offset, &attr_num);
      int32_decode(data, &offset, &attr_offset);
      oid_decode(data, &offset, &valoid);

#ifdef TRACE
      printf("decoding[%d] -> %s %d %d %s\n",
	     i, Oid(objoid).toString(), attr_num, attr_offset,
	     Oid(valoid).toString());
#endif
      list.insertObject(new InvOidContext(objoid, attr_num,
					  attr_offset, valoid));
    }
  }

  Bool
  InvOidContext::getContext()
  {
    if (!inv_list) {
      inv_list = &st_inv_list;
      inv_list->empty();
      return True;
    }

    return False;
  }

  void
  InvOidContext::insert(const Oid &objoid, const Attribute *attr,
			const Oid &valoid)
  {
    inv_list->insertObject(new InvOidContext(objoid, attr, valoid));
  }

  void
  InvOidContext::releaseContext(Bool newctx, Data *inv_data, void *xinv_data)
  {
    static LinkedList empty_list;

    Data rdata;
    Size size;

    if (newctx) {
      code(&rdata, *inv_list, True, size);
      inv_list = 0;
    }
    else {
      code(&rdata, empty_list, True, size);
    }

    if (inv_data) {
      *inv_data = rdata;
      return;
    }

    rpc_ServerData *data = (rpc_ServerData *)xinv_data;

    if (size <= data->buff_size) {
      data->status = rpc_BuffUsed;
      memcpy(data->data, rdata, size);
    }
    else {
      data->status = rpc_TempDataUsed;
      data->data = rdata;
    }

    data->size = size;
  }
}
