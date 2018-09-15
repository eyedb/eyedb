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

//
// EnumClass
//

namespace eyedb {

  EnumClass::EnumClass(const char *s) : Class(s)
  {
    items_cnt = 0;
    items = 0;

    parent = Enum_Class;
    setClass(EnumClass_Class);
#ifdef IDB_UNDEF_ENUM_HINT
    idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(char) + sizeof(eyedblib::int32);
#else
    idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int32);
#endif
    idr_psize = idr_objsz;
    idr_vsize = 0;
    type = _EnumClass_Type;
  }

  EnumClass::EnumClass(Database *_db, const char *s) : Class(_db, s)
  {
    items_cnt = 0;
    items = 0;

    parent = Enum_Class;
    setClass(EnumClass_Class);
#ifdef IDB_UNDEF_ENUM_HINT
    idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(char) + sizeof(eyedblib::int32);
#else
    idr_objsz = IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int32);
#endif
    idr_psize = idr_objsz;
    idr_vsize = 0;
    type = _EnumClass_Type;
  }

  void
  EnumClass::copy_realize(const EnumClass &cl)
  {
    items_cnt = cl.items_cnt;
    items = (EnumItem **)malloc(sizeof(EnumItem*)*items_cnt);
    for (int i = 0; i < items_cnt; i++)
      items[i] = cl.items[i]->clone();
  }

  EnumClass::EnumClass(const EnumClass &cl)
    : Class(cl)
  {
    copy_realize(cl);
  }

  EnumClass::EnumClass(const Oid &_oid, const char *_name)
    : Class(_oid, _name)
  {
    items_cnt = 0;
    items = 0;
    type = _EnumClass_Type;
  }

  Status
  EnumClass::loadComplete(const Class *cl)
  {
    assert(cl->asEnumClass());
    Status s = Class::loadComplete(cl);
    if (s) return s;
    copy_realize(*cl->asEnumClass());
    return Success;
  }

  EnumClass& EnumClass::operator=(const EnumClass &cl)
  {
    this->Class::operator=(cl);
    copy_realize(cl);

    return *this;
  }

  int EnumClass::getEnumItemsCount(void) const
  {
    return items_cnt;
  }

  const EnumItem *EnumClass::getEnumItem(int n) const
  {
    if (n >= 0 && n < items_cnt)
      return items[n];
    else
      return 0;
  }

  const EnumItem *EnumClass::getEnumItemFromName(const char *nm) const
  {
    for (int i = 0; i < items_cnt; i++)
      if (!strcmp(items[i]->name, nm))
	return items[i];

    return (const EnumItem *)0;
  }

  const EnumItem *EnumClass::getEnumItemFromVal(unsigned int val) const
  {
    for (int i = 0; i < items_cnt; i++)
      if (items[i]->value == val)
	return items[i];

    return (const EnumItem *)0;
  }

  void EnumClass::free_items(void)
  {
    if (items_cnt)
      {
	for (int i = 0; i < items_cnt; i++)
	  delete items[i];

	free(items);
      }
  }

  Status EnumClass::setEnumItems(EnumItem **nitems, int cnt)
  {
    free_items();

    items_cnt = cnt;

    if (items_cnt)
      {
	items = (EnumItem**)malloc(sizeof(EnumItem*)*items_cnt);
	for (int i = 0; i < items_cnt; i++)
	  items[i] = new EnumItem(nitems[i], i);
      }

    return Success;
  }

  const EnumItem **EnumClass::getEnumItems(int& cnt) const
  {
    cnt = items_cnt;
    return (const EnumItem **)items;
  }

  Bool
  EnumClass::compare_perform(const Class *cl,
			     Bool compClassOwner,
			     Bool compNum,
			     Bool compName,
			     Bool inDepth) const
  {
    const EnumClass *me = (EnumClass *)cl;
    if (items_cnt != me->items_cnt)
      return False;

    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->compare(me->items[i]))
	return False;

    return True;
  }

  Object *EnumClass::newObj(Database *_db) const
  {
    if (!idr_objsz)
      return 0;

    Enum *t = new Enum(_db);

    ObjectPeer::make(t, this, 0, _Enum_Type, idr_objsz,
		     idr_psize, idr_vsize);
    return t;
  }

  Object *EnumClass::newObj(Data data, Bool _copy) const
  {
    if (!idr_objsz)
      return 0;

    Enum *t = new Enum();

    ObjectPeer::make(t, this, data, _Enum_Type, idr_objsz,
		     idr_psize, idr_vsize, _copy);
    return t;
  }

  void EnumClass::_setCSDRSize(Size, Size)
  {
  }

  Status
  EnumClass::setName(const char *s)
  {
    return setNameRealize(s);
  }

  Status EnumClass::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
  {
    if (const_cast<EnumClass *>(this)->wholeComplete())
      return 0;

    char *indent_str = make_indent(INDENT_INC);
    EnumItem **item;
    int n;

    fprintf(fd, "%s enum %s { ", oid.getString(), name);

    Status status = Success;
    status = trace_common(fd, INDENT_INC, flags, rcm);
    if (status) goto out;

    for (n = 0, item = items; n < items_cnt; n++, item++)
      fprintf(fd, "%s%s = %d%s\n", indent_str, (*item)->name, (*item)->value,
	      ((n != items_cnt - 1) ? "," : ""));

    status = trace_comps(fd, INDENT_INC, flags, rcm);
    if (status) goto out;

    fprintf(fd, "};\n");

  out:
    delete_indent(indent_str);
    return status;
  }

  static int global_check_enum_value =
  getenv("EYEDBNOCHECKENUM") ? False : True;

  Status EnumClass::setRawData(Data xdata, Data hdata, int nb,
			       Bool& mod, Bool check_value)
  {
#ifdef E_XDR_TRACE
    cout << "EnumClass::setRawData " << name << endl;
#endif
    mod = False;
#ifdef IDB_UNDEF_ENUM_HINT
    xdata += sizeof(char);
#endif
#ifdef IDB_UNDEF_ENUM_HINT
    for (int i = 0; i < nb; i++, xdata += sizeof(char) + sizeof(eyedblib::int32),
	   hdata += sizeof(eyedblib::int32))
#else
      for (int i = 0; i < nb; i++, xdata += sizeof(eyedblib::int32),
	     hdata += sizeof(eyedblib::int32)) {
#endif
	eyedblib::int32 l;
	x2h_32_cpy(&l, xdata);
	if (memcmp(&l, hdata, sizeof(eyedblib::int32))) {
	  if (check_value && global_check_enum_value) {
	    eyedblib::int32 val;
	    eyedblib_mcp(&val, hdata, sizeof(eyedblib::int32));
	    if (!getEnumItemFromVal(val))
	      return Exception::make(IDB_ERROR, "invalid value '%d' for "
				     "enum class %s", val, name);
	  }
	  h2x_32_cpy(xdata, hdata);
	  //eyedblib_mcp(xdata, hdata, sizeof(eyedblib::int32));
	  mod = True;
	}
      }

    return Success;
  }

  Status EnumClass::getRawData(Data hdata, Data xdata, int nb) const
  {
#ifdef E_XDR_TRACE
    cout << "EnumClass::getRawData " << name << endl;
#endif
#ifdef IDB_UNDEF_ENUM_HINT
    xdata += sizeof(char);
    for (int i = 0; i < nb; i++, hdata += sizeof(eyedblib::int32),
	   xdata += sizeof(char) + sizeof(eyedblib::int32))
      memcpy(hdata, xdata, sizeof(eyedblib::int32));
#else
    for (int i = 0; i < nb; i++, hdata += sizeof(eyedblib::int32),
	   xdata += sizeof(eyedblib::int32)) {
#ifdef E_XDR
      x2h_32_cpy(hdata, xdata);
#else
      memcpy(hdata, xdata, sizeof(eyedblib::int32));
#endif
    }
#endif

    return Success;
  }

  Status EnumClass::traceData(FILE *fd, int, Data inidata, Data data, TypeModifier *tmod) const
  {
    int i;
    if (data)
      {
#ifdef IDB_UNDEF_ENUM_HINT
	data += sizeof(char);
#endif
	const EnumItem *it;
	eyedblib::int32 j;
	if (tmod)
	  {
	    if (tmod->pdims > 1)
	      {
		fprintf(fd, "{");
		for (i = 0; i < tmod->pdims; i++)
		  {
		    if (i)
		      fprintf(fd, ", ");
#ifdef E_XDR
		    x2h_32_cpy(&j, data);
#else
		    memcpy(&j, data, sizeof(eyedblib::int32));
#endif
		    it = getEnumItemFromVal(j);
		    if (it)
		      fprintf(fd, "%s", it->name);
		    else
		      {
			fprintf(fd, "%d", j);
			if (j)
			  fprintf(fd, " [%XH 0%o]", j, j);
		      }

#ifdef IDB_UNDEF_ENUM_HINT
		    data += sizeof(eyedblib::int32) + sizeof(char);
#else
		    data += sizeof(eyedblib::int32);
#endif
		  }
		fprintf(fd, "}");
	      }
	    else
	      {
#ifdef E_XDR
		x2h_32_cpy(&j, data);
#else
		memcpy(&j, data, sizeof(eyedblib::int32));
#endif
		it = getEnumItemFromVal(j);
		if (it)
		  fprintf(fd, "%s", it->name);
		else
		  {
		    fprintf(fd, "%d", j);
		    if (j)
		      fprintf(fd, " [%XH 0%o]", j, j);
		  }
	      }
	  }
	else
	  {
#ifdef E_XDR
	    x2h_32_cpy(&j, data);
#else
	    memcpy(&j, data, sizeof(eyedblib::int32));
#endif
	    it = getEnumItemFromVal(j);
	    if (it)
	      fprintf(fd, "%s", it->name);
	    else
	      {
		fprintf(fd, "%d", j);
		if (j)
		  fprintf(fd, " [%XH 0%o]", j, j);
	      }
	  }
      }
    else
      fprintf(fd, "null");

    return Success;
  }

  Status EnumClass::setValue(Data)
  {
    return Success;
  }

  Status EnumClass::getValue(Data*) const
  {
    return Success;
  }

  Status EnumClass::create()
  {
    if (oid.isValid())
      return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating enum class '%s'", name);

    IDB_CHECK_WRITE(db);

    Size alloc_size;
    Offset offset;
    Status status;

    alloc_size = 0;
    Data data = 0;
    offset = IDB_CLASS_IMPL_TYPE;
    Status s = IndexImpl::code(data, offset, alloc_size, idximpl);
    if (s) return s;

    offset = IDB_CLASS_MTYPE;
    eyedblib::int32 mt = m_type;
    int32_code (&data, &offset, &alloc_size, &mt);

    offset = IDB_CLASS_DSPID;
    eyedblib::int16 dspid = get_instdspid();
    int16_code (&data, &offset, &alloc_size, &dspid);

    offset = IDB_CLASS_HEAD_SIZE;
  
    status = class_name_code(db->getDbHandle(), getDataspaceID(), &data, &offset,
			     &alloc_size, name);
    if (status) return status;

    int32_code  (&data, &offset, &alloc_size, &items_cnt);
  
    for (int i = 0; i < items_cnt; i++)
      {
	EnumItem *item = items[i];
	string_code (&data, &offset, &alloc_size, item->name);
	int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&item->value);
      }

    int idr_sz = offset;
    idr->setIDR(idr_sz, data);
    headerCode(_EnumClass_Type, idr_sz);

    RPCStatus rpc_status;
    rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data, oid.getOid());
  
    if (rpc_status == RPCSuccess)
      {
	Status status;

	status = ClassPeer::makeColls(db, this, data, &oid);
	if (status != Success)
	  return status;
      }

    if (rpc_status == RPCSuccess)
      gbx_locked = gbxTrue;
    return StatusMake(rpc_status);
  }

  Status EnumClass::update()
  {
    return Success;
  }

  Status EnumClass::remove(const RecMode*)
  {
    return Success;
  }

  Status EnumClass::trace_realize(FILE*, int, unsigned int flags, const RecMode *rcm) const
  {
    return Success;
  }

  int
  EnumClass::genODL(FILE *fd, Schema *) const
  {
    extern Bool odl_system_class;
     
    if (const_cast<EnumClass *>(this)->wholeComplete())
      return 0;

    if (isSystem() && !odl_system_class)
      return 0;

    fprintf(fd, "enum %s {\n", name);

    EnumItem **item;
    int n;

    for (n = 0, item = items; n < items_cnt; n++, item++)
      fprintf(fd, "\t%s = %d%s\n", (*item)->name, (*item)->value,
	      ((n != items_cnt - 1) ? "," : ""));
    fprintf(fd, "};\n");
    return 1;
  }

  void
  EnumClass::touch()
  {
    Class::touch();
    LinkedList *cl_list = IteratorBE::getMclList();
    if (cl_list)
      cl_list->insertObjectLast(this);
  }

  void EnumClass::garbage()
  {
    free_items();
    Class::garbage();
  }

  EnumClass::~EnumClass()
  {
    garbageRealize();
  }

  EnumItem::EnumItem(const char *nm, unsigned int val, int _num)
  {
    name = strdup(nm);
    aliasname = 0;
    value = val;
    num = _num;
  }

  EnumItem::EnumItem(const char *nm, const char *_aliasname,
		     unsigned int val, int _num)
  {
    name = strdup(nm);
    aliasname = _aliasname ? strdup(_aliasname) : 0;
    value = val;
    num = _num;
  }

  EnumItem::EnumItem(const EnumItem *item, int n)
  {
    name = strdup(item->name);
    aliasname = item->aliasname ? strdup(item->aliasname) : 0;
    value = item->value;
    num = n;
  }

  EnumItem *
  EnumItem::clone() const
  {
    return new EnumItem(name, aliasname, value, num);
  }

  Bool
  EnumItem::compare(const EnumItem *item) const
  {
    if (num != item->num)
      return False;
    if (value != item->value)
      return False;
    if (strcmp(name, item->name))
      return False;
    /*
      if (strcmp(getAliasName(), item->getAliasName()))
      return False;
    */

    return True;
  }


  EnumItem::~EnumItem()
  {
    free(name);
    free(aliasname);
  }
}
