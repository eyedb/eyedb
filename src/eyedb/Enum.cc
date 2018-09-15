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
#include "IteratorBE.h"
#include "CollectionBE.h"
#include <iostream>

namespace eyedb {

  //
  // Enum methods
  //

  Enum::Enum(Database *_db, const Dataspace *_dataspace) :
    Instance(_db, _dataspace)
  {
    val = NULL;
  }

  Enum::Enum(const Enum &o) : Instance(o)
  {
    val = o.val;
  }

  Enum& Enum::operator=(const Enum &o)
  {
    if (&o == this)
      return *this;

    //garbageRealize();

    *(Instance *)this = Instance::operator=((const Instance &)o);

    val = o.val;

    return *this;
  }

  Status Enum::setValue(Data data)
  {
    int i;
    memcpy(&i, data, sizeof(eyedblib::int32));
    val = ((EnumClass *)getClass())->getEnumItemFromVal(i);
    printf("Enum::setValue(%d)\n", i);
    return Success;
  }

  Status Enum::getValue(Data *data) const
  {
    printf("Enum::getValue(%p)\n", val);
    if (val) {
      int i = val->getValue();
      memcpy(data, &i, sizeof(eyedblib::int32));
    }
    return Success;
  }

  Status Enum::setValue(unsigned int v)
  {
    const EnumItem *item;
    if ((item = ((EnumClass *)getClass())->getEnumItemFromVal(v)))
      {
	val = item;
	return Success;
      }

    return Exception::make(IDB_ENUM_ERROR, "invalid value `%d' for enum '%s'", v,
			   getClass()->getName());
  }

  Status Enum::getValue(unsigned int *v) const
  {
    if (val)
      {
	*v = val->value;
	return Success;
      }

    *v = 0;
    return Success;
    //  return Exception::make(IDB_ENUM_ERROR, "enum not initialized");
  }

  Status Enum::setValue(const char *name)
  {
    const EnumItem *item;
    if ((item = ((EnumClass *)getClass())->getEnumItemFromName(name)))
      {
	val = item;
	return Success;
      }

    return Exception::make(IDB_ENUM_ERROR, "invalid value '%s' for enum '%s'",
			   name, getClass()->getName());
  }

  Status Enum::getValue(const char **s) const
  {
    if (val)
      {
	*s = val->name;
	return Success;
      }

    *s = 0;
    return Success;
    //  return Exception::make(IDB_ERROR, "enum not initialized");
  }

  Status Enum::setValue(const EnumItem *it)
  {
    val = it;
    return Success;
  }

  Status Enum::getValue(const EnumItem **pit) const
  {
    if (val)
      {
	*pit = val;
	return Success;
      }

    *pit = 0;
    return Success;
    //  return Exception::make(IDB_ERROR, "enum not initialized");
  }

  Status Enum::create()
  {
    if (!getClass())
      return Exception::make(IDB_NO_CLASS);

    IDB_CHECK_WRITE(db);
    if (oid.isValid())
      return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating enum of class '%s'", getClass()->getName());

    if (!getClass()->getOid().isValid())
      return Exception::make(IDB_CLASS_NOT_CREATED, "creating enum of class '%s'", getClass()->getName());

    Offset offset = IDB_OBJ_HEAD_SIZE;
    Size alloc_size = idr->getSize();
    Data data = idr->getIDR();

    char k;

    if (val)
      {
	k = 1;
	char_code(&data, &offset, &alloc_size, &k);
	int32_code(&data, &offset, &alloc_size, &val->num);
      }
    else
      {
	k = 0;
	char_code(&data, &offset, &alloc_size, &k);
      }

    classOidCode();

    RPCStatus rpc_status;
    rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());

    return StatusMake(rpc_status);
  }

  Status Enum::update()
  {
    return Success;
  }

  Status Enum::remove(const RecMode*)
  {
    return Success;
  }

  Status Enum::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
  {
    char *indent_str = make_indent(INDENT_INC);
    EnumItem *item;
    int n;

    fprintf(fd, "%s %s = { ", oid.getString(), getClass()->getName(), val);
    trace_flags(fd, flags);
    fprintf(fd, "\n");

    if (traceRemoved(fd, indent_str))
      goto out;

    if (val)
      fprintf(fd, "%s%s <%d>;\n", indent_str, val->name, val->value);
    else
      fprintf(fd, "%s<uninitialized>;\n", indent_str);
    fprintf(fd, "};\n");

  out:
    delete_indent(indent_str);
    return Success;
  }

  Status Enum::trace_realize(FILE*, int, unsigned int flags, const RecMode *rcm) const
  {
    return Success;
  }

  void Enum::garbage()
  {
  }

  Enum::~Enum()
  {
    garbageRealize();
  }

  //
  // functions
  // 

  Status
  enumClassMake(Database *db, const Oid *oid, Object **o,
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
	eyedblib::int32 cnt;
	char *s;
	EnumClass *me;
	Offset offset;

	/*
	  eyedblib::int32 mag_order;
	  offset = IDB_CLASS_MAG_ORDER;
	  int32_decode (temp, &offset, &mag_order);
	*/
	IndexImpl *idximpl;
	offset = IDB_CLASS_IMPL_TYPE;
	Status status = IndexImpl::decode(db, temp, offset, idximpl);
	if (status) return status;

	eyedblib::int32 mt;
	offset = IDB_CLASS_MTYPE;
	int32_decode (temp, &offset, &mt);

	eyedblib::int16 dspid;
	offset = IDB_CLASS_DSPID;
	int16_decode (temp, &offset, &dspid);

	offset = IDB_CLASS_HEAD_SIZE;

	status = class_name_decode(db->getDbHandle(), temp, &offset, &s);
	if (status) return status;
#if 0
	if (Class::isBoolClass(s) && db->getSchema()) {
	  printf("BOOL LOADING\n");
	  if (!idr)
	    free(temp);
	  me = (EnumClass *)db->getSchema()->getClass(s);
	  *o = (Object *)me;
	  me->setMagOrder(mag_order);
	  me->setExtentImplementation(idximpl, True);
	  if (idximpl)
	    idximpl->release();
	  me->setInstanceDspid(dspid);

	  ClassPeer::setMType(me, (Class::MType)mt);
	  return RPCSuccess;
	}
#endif
	int32_decode(temp, &offset, &cnt);

	me = new EnumClass(s);

	free(s); s = 0;
	me->items = (EnumItem **)malloc(sizeof(EnumItem *) * cnt);
	me->items_cnt = cnt;

	me->setExtentImplementation(idximpl, True);
	if (idximpl)
	  idximpl->release();
	me->setInstanceDspid(dspid);

	ClassPeer::setMType(me, (Class::MType)mt);

	for (int i = 0; i < cnt; i++)
	  {
	    eyedblib::int32 val;
	    string_decode(temp, &offset, &s);
	    int32_decode(temp, &offset, &val);
	    me->items[i] = new EnumItem(s, val, i);
	  }

	*o = (Object *)me;

	status = ClassPeer::makeColls(db, (Class *)*o, temp);

	if (status)
	  {
	    if (!idr)
	      free(temp);
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

  Status
  enumMake(Database *db, const Oid *oid, Object **o,
	       const RecMode *rcm, const ObjectHeader *hdr, Data idr,
	       LockMode lockmode, const Class *_class)
  {
    RPCStatus rpc_status;

    if (!_class)
      _class = db->getSchema()->getClass(hdr->oid_cl, True);

    if (!_class)
      return Exception::make(IDB_CLASS_NOT_FOUND, "enum class '%s'",
			     OidGetString(&hdr->oid_cl));

    Enum *e;

    if (idr && !ObjectPeer::isRemoved(*hdr))
      *o = _class->newObj(idr + IDB_OBJ_HEAD_SIZE, False);
    else
      *o = _class->newObj();

    e = (Enum *)*o;
    Status status = e->setDatabase(db);
    if (status)
      return status;

    if (idr)
      rpc_status = RPCSuccess;
    else
      rpc_status = objectRead(db->getDbHandle(), e->getIDR(), 0, 0,
				  oid->getOid(), 0, lockmode, 0);

    if (rpc_status == RPCSuccess)
      {
	char k;
	Offset offset = IDB_OBJ_HEAD_SIZE;
	Data idr = e->getIDR();
	char_decode(idr, &offset, &k);
	if (k)
	  {
	    eyedblib::int32 k;
	    int32_decode(idr, &offset, &k);
	    int cnt;
	    e->setValue(((EnumClass *)_class)->getEnumItems(cnt)[k]);
	  }
      }

    return StatusMake(rpc_status);
  }
}


  
