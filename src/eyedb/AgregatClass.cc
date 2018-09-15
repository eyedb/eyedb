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
#include <assert.h>
#include "Attribute_p.h"
#include "eyedb/gbxcyctx.h"

namespace eyedb {

#define foreach_item(X) \
{ \
   int i; \
 \
   Status status; \
 \
   for (i = 0; i < items_cnt; i++) \
     if (!items[i]->isNative() && ((status = items[i]->X))) \
         return status; \
}

  //
  // AgregatClass
  //

  void AgregatClass::init(void)
  {
  }

  void AgregatClass::_release(void)
  {
  }

  Status AgregatClass::getValue(unsigned char**) const
  {
    return Success;
  }

  Status AgregatClass::setValue(unsigned char*)
  {
    return Success;
  }

  Status
  AgregatClass::setName(const char *s)
  {
    return setNameRealize(s);
  }

  void
  AgregatClass::touch()
  {
    Class::touch();
    LinkedList *cl_list = IteratorBE::getMclList();
    if (cl_list)
      cl_list->insertObjectLast(this);
  }

  Status AgregatClass::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
  {
    return trace_realize(fd, INDENT_INC, flags, rcm);
  }

  void stop_now1() { }

  void AgregatClass::_init(Class *_parent)
  {
    setClass(AgregatClass_Class);
    parent    = (_parent ? _parent : Agregat_Class);

    AttrNative::copy(ObjectITEMS, items, items_cnt, this);
    post_create_offset = 0;
  }

  AgregatClass::AgregatClass(const char *s, Class *p) :
    Class(s, p)
  {
    _init(p);
  }

  AgregatClass::AgregatClass(const char *s, const Oid *poid) :
    Class(s, poid)
  {
    _init(0);
  }

  AgregatClass::AgregatClass(Database *_db, const char *s,
			     Class *p) : Class(_db, s, p)
  {
    _init(p);
  }

  AgregatClass::AgregatClass(Database *_db, const char *s,
			     const Oid *poid) : Class(_db, s, poid)
  {
    parent = 0;
    setClass( AgregatClass_Class);
    post_create_offset = 0;
  }

  AgregatClass::AgregatClass(const AgregatClass &cl)
    : Class(cl)
  {
    _init(0);
  }

  AgregatClass::AgregatClass(const Oid &_oid, const char *_name)
    : Class(_oid, _name)
  {
    post_create_offset = 0;
  }

  AgregatClass &AgregatClass::operator=(const AgregatClass &cl)
  {
    _init(0);
    this->Class::operator=(cl);
    return *this;
  }

  Status AgregatClass::compile(void)
  {
    int n;
    int offset;
    int size, inisize;
    Status status;

    offset = IDB_OBJ_HEAD_SIZE;

    if (asUnionClass())
      offset += sizeof(eyedblib::int16);

    size = 0;

    for (n = 0; n < items_cnt; n++)
      if ((status = items[n]->compile_perst(this, &offset, &size, &inisize)))
	return status;

    if (asUnionClass())
      {
	idr_psize = size + IDB_OBJ_HEAD_SIZE + sizeof(eyedblib::int16);
	offset = idr_psize;
      }
    else
      idr_psize = offset;

    size = 0;
    idr_inisize = inisize;

    for (n = 0; n < items_cnt; n++)
      if ((status = items[n]->compile_volat(this, &offset, &size)))
	return status;

    if (asUnionClass())
      idr_vsize = size;
    else
      idr_vsize = offset - idr_psize;

    idr_objsz = idr_psize + idr_vsize;

    /*
      printf("\n");
      printf("Class %s {\n", name);
      printf("   Persistent Size = %d\n",    idr_psize);
      printf("   Volatile Size   = %d\n",    idr_vsize);
      printf("   Total Size      = %d\n}\n", idr_objsz);
    */

    fflush(stdout);

    return Success;
  }

  void
  AgregatClass::newObjRealize(Object *agr) const
  {
    if (!attrs_complete)
      ((AgregatClass *)this)->attrsComplete();

    for (int n = 0; n < items_cnt; n++)
      items[n]->newObjRealize(agr);
  }

  Status
  AgregatClass::checkInversePath(const Schema *m,
				 const Attribute *item,
				 const Attribute *&invitem,
				 Bool mandatory) const
  {
    const char *cinvname, *finvname;
    invitem = NULL;

    item->getInverse(&cinvname, &finvname, &invitem);

    /*
      printf("checkInversePath -> invname %s, finvname %s [mandatory %d] "
      "invitem %p\n",
      (cinvname ? cinvname : "NULL"),
      (finvname ? finvname : "NULL"), mandatory, invitem);
    */

    if ((!cinvname || !finvname) && !invitem)
      {
	if (mandatory)
	  return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
				 "attribute '%s::%s': "
				 "has no inverse directive",
				 item->getClassOwner()->getName(),
				 item->getName());

	return Success;
      }

    if (invitem)
      return Success;

    Class *cl = ((Schema *)m)->getClass(cinvname);
    if (!cl)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "attribute '%s::%s': "
			     "inverse class '%s' "
			     "does not exist.",
			     item->getClassOwner()->getName(),
			     item->getName(), cinvname);

    invitem = cl->getAttribute(finvname);

    if (!invitem)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "attribute '%s::%s': "
			     "inverse attribute '%s::%s' "
			     "does not exist.",
			     item->getClassOwner()->getName(),
			     item->getName(),
			     cinvname, finvname);

    return Success;
  }

  Status AgregatClass::checkInverse(const Schema *m) const
  {
    Status s;

    for (int i = 0; i < items_cnt; i++)
      {
	const Attribute *item = items[i];
	const Attribute *invitem;

	if (s = checkInversePath(m, item, invitem, False))
	  return s;

	if (!invitem)
	  continue;

	const Attribute *invinvitem;
	if (s = checkInversePath(m, invitem, invinvitem, True))
	  return s;

	if (!invinvitem->compare(db, item))
	  return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
				 "attribute '%s::%s': "
				 "inverse directive attribute '%s::%s' "
				 "does not match.",
				 name,
				 item->getName(),
				 invitem->getClassOwner()->getName(),
				 invitem->getName());
      }

    return Success;
  }

  Status AgregatClass::create()
  {
    if (oid.isValid())
      return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating agregat_class '%s'", name);

    IDB_CHECK_WRITE(db);

#ifdef OPTOPEN_TRACE
    printf("creating class %s this=%p\n", name, this);
#endif

    RPCStatus rpc_status;
    eyedblib::int16 kk;
    Size alloc_size;
    Offset offset, ioff;
    Status status;
  
    alloc_size = 0;
    idr->setIDR((Size)0);
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

    if (parent && !parent->getOid().isValid() && !parent->getDatabase())
      parent = db->getSchema()->getClass(parent->getName());

    if (parent && !parent->getOid().isValid()) {
      Status status = parent->create();
      if (status) return status;
    }

    if (parent)
      oid_code (&data, &offset, &alloc_size, parent->getOid().getOid());
    else
      oid_code (&data, &offset, &alloc_size, getInvalidOid());

    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_psize);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_vsize);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_objsz);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&items_cnt);
  
    ioff = offset;
    post_create_offset = ioff;

    int i;
    for (i = 0; i < items_cnt; i++)
      items[i]->codeIDR(db, &data, &offset, &alloc_size);

    Size idr_sz = offset;
    idr->setIDR(idr_sz, data);
    headerCode((asStructClass() ? _StructClass_Type : _UnionClass_Type),
	       idr_sz, xinfo);

    codeExtentCompOids(alloc_size);

    // added the 21/10/99
    if (oid.isValid())
      rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());
    else
      rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(),
				data, oid.getOid());
  
    if (rpc_status == RPCSuccess) {
      status = ClassPeer::makeColls(db, this, data, &oid);
      if (status)
	return status;
    }

    if (rpc_status == RPCSuccess) {
      offset = ioff;
      for (i = 0; i < items_cnt; i++) {
	Attribute *item = items[i];
	if (item->isNative())
	  continue;
	if (!item->cls->getOid().isValid()) {
	  if (item->cls->isUnrealizable()) {
	    item->cls =
	      db->getSchema()->getClass(item->cls->getName());
	    assert(item->cls);
	  }
	  Status status;
	  status = const_cast<Class *>(item->cls)->setDatabase(db);
	  if (status)
	    return status;

	  status = const_cast<Class *>(item->cls)->create();
	  if (status) {
	    if (status->getStatus() == IDB_OBJECT_ALREADY_CREATED)
	      continue;
	    return status;
	  }
	}
      
	item->codeClassOid(data, &offset);
      
	// inverse
	status = item->completeInverse(db);
	if (status)
	  return status;
      }
    
      offset = ioff;
      for (i = 0; i < items_cnt; i++)
	items[i]->codeIDR(db, &data, &offset, &alloc_size);
    
      rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());
    }

    return StatusMake(rpc_status);
  }

  Status AgregatClass::postCreate()
  {
    if (!post_create_offset)
      return Success;

    RPCStatus rpc_status;
    Size alloc_size;
    Offset offset;
    Status status;
  
    if (!getUserData("eyedb:odl::update") && (status = createIndexes()))
      return status;

    offset = post_create_offset;
    alloc_size = idr->getSize();
    Data data = idr->getIDR();
    //printf("AgregatClass::postCreate(%s, offset=%d)\n", name, post_create_offset);
    for (int i = 0; i < items_cnt; i++)
      items[i]->codeIDR(db, &data, &offset, &alloc_size);

    if (idr->getSize() - post_create_offset > 0)
      rpc_status = dataWrite(db->getDbHandle(),
			     post_create_offset,
			     idr->getSize() - post_create_offset,
			     idr->getIDR()+post_create_offset, oid.getOid());
    else
      rpc_status = RPCSuccess;
  
    post_create_offset = 0;
    mustCreateComps = True;
  
    /*
      if (!rpc_status)
      return createComps();
    */

    return StatusMake(rpc_status);
  }

  Status
  AgregatClass::completeInverse(Schema *m)
  {
    for (int i = 0; i < items_cnt; i++)
      {
	Status s = items[i]->completeInverse(m);
	if (s)
	  return s;
      }

    return Success;
  }

  Status AgregatClass::attrsComplete()
  {
    if (!db)
      return Success;

    int err = 0;
    Status s = Class::attrsComplete();

    if (s)
      err++;

    setSchema(db->getSchema());
    BufferString buf;

    for (int i = 0; i < items_cnt; i++)
      {
	Attribute *item = items[i];
	if (item->isNative())
	  continue;
	Bool inv_error = False;

#if 0
	printf("attribute %s {\n", item->getName());
	printf("\tclass: %s ", item->oid_cl.toString());
	if (item->cls) {
	  printf("vs. %s", item->cls->getOid().toString());
	  //assert(item->oid_cl.compare(item->cls->getOid()));
	  /*
	    if (!item->oid_cl.isValid() ||
	    !item->oid_cl.compare(item->cls->getOid()))
	    item->oid_cl = item->cls->getOid();
	  */
	}
	printf("\n\tclass_owner: %s ", item->oid_cl_own.toString());
	if (item->class_owner) {
	  printf("vs. %s", item->class_owner->getOid().toString());
	  //assert(item->oid_cl_own.compare(item->class_owner->getOid()));
	  /*
	    if (!item->oid_cl_own.isValid() ||
	    !item->oid_cl_own.compare(item->class_owner->getOid()))
	    item->oid_cl_own = item->class_owner->getOid();
	  */
	}
	printf("\n}\n");
#endif
	if (!item->cls)
	  item->cls = getSchema()->getClass(item->oid_cl, True);
	assert(!item->cls || !item->cls->isRemoved());
	if (!item->class_owner)
	  item->class_owner = getSchema()->getClass(item->oid_cl_own, True);
	assert(!item->class_owner || !item->class_owner->isRemoved());
	if (item->inv_spec.oid_cl.isValid())
	  {
	    Class *cl_inv;
	    cl_inv = getSchema()->getClass(item->inv_spec.oid_cl);

	    if (cl_inv)
	      item->inv_spec.item = ((AgregatClass *)cl_inv)->getAttributes()[item->inv_spec.num];
	    else
	      inv_error = True;
	  }

	if (!item->cls || !item->class_owner || inv_error)
	  {
	    if (!buf.length())
	      buf.append((std::string("attributes of agregat_class '") +
			  name + "' are incomplete: ").c_str());
	    else
	      buf.append(", ");

	    buf.append(item->name);
	    if (!item->cls) buf.append(" (class attribute is missing)");
	    else if (!item->class_owner) buf.append(" (class owner is missing)");
	    else if (inv_error) buf.append(" (class of inverse attribute is missing)");
	    err++;
	  }

	Status status = item->completeInverse(db);
	if (status)
	  return status;
      }

    attrs_complete = (err ? False : True);

    if (err)
      return Exception::make(IDB_CLASS_COMPLETION_ERROR, buf.getString());

    return Success;
  }

  Status AgregatClass::setDatabase(Database *mdb)
  {
    Status status = Class::setDatabase(mdb);

    if (status == Success)
      {
	for (int i = 0; i < items_cnt; i++)
	  {
	    Attribute *item = items[i];
	    if (item->cls && !item->cls->getOid().isValid())
	      {
		if (item->cls->isUnrealizable())
		  {
		    item->cls =
		      db->getSchema()->getClass(item->cls->getName());
		    assert(item->cls);
		  }
	      }
	  }
      }
    return status;
  }

  Status AgregatClass::update()
  {
    if (!modify)
      return Success;

    Status status;

    status = wholeComplete();
    if (status)
      return status;

    RPCStatus rpc_status;
    eyedblib::int16 kk;
    Size alloc_size;
    Offset offset, ioff;
  
    alloc_size = idr->getSize();
    Data data = idr->getIDR();
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

    if (parent && !parent->getOid().isValid())
      {
	status = parent->create();
	if (status) return status;
      }

    if (parent)
      oid_code (&data, &offset, &alloc_size, parent->getOid().getOid());
    else
      oid_code (&data, &offset, &alloc_size, getInvalidOid());

    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_psize);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_vsize);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&idr_objsz);
    int32_code  (&data, &offset, &alloc_size, (eyedblib::int32 *)&items_cnt);
  
    ioff = offset;

    int i;
    for (i = 0; i < items_cnt; i++) {
      status = items[i]->completeInverse(db);
      if (status) return status;
    }

    for (i = 0; i < items_cnt; i++) {
      status = items[i]->codeIDR(db, &data, &offset, &alloc_size);
      if (status) return status;
    }

#if 1
    // added the 4/10/99
    Size idr_sz = offset;
    // added the 27/12/00 because of a problem in eyedbodl updating process
    if (!idr->getSize())
      idr->setIDR(idr_sz, data);

    assert(idr_sz == idr->getSize());
    if (!getClass()->getOid().isValid())
      setClass(db->getSchema()->getClass(getClass()->getName()));
    headerCode((asStructClass() ? _StructClass_Type : _UnionClass_Type),
	       idr_sz, xinfo);
#else
    // workaround ...
    ObjectHeader hdr;
    object_header_decode_head(idr->idr, &hdr);
    if (hdr.type != _AgregatClass_Type)
      {
	hdr.type = _AgregatClass_Type;
	object_header_code_head(idr->idr, &hdr);
      }
    // ...
#endif

    offset = ioff;

    for (i = 0; i < items_cnt; i++)
      items[i]->codeIDR(db, &data, &offset, &alloc_size);
  
    unsigned int objsize = 0;
    rpc_status = dataSizeGet(db->getDbHandle(), oid.getOid(), &objsize);
    if (!rpc_status) {
      if (idr->getSize() != objsize) {
	rpc_status = objectSizeModify(db->getDbHandle(), idr->getSize(),
				      oid.getOid());
      }

      if (!rpc_status)
	rpc_status = objectWrite(db->getDbHandle(), data, oid.getOid());
    }

    if (!rpc_status)
      modify = False;

    return StatusMake(rpc_status);
  }

  void
  AgregatClass::revert(Bool rev)
  {
    for (int i = 0; i < items_cnt; i++)
      items[i]->revert(rev);
  }

  Status AgregatClass::remove(const RecMode *rcm)
  {
    return Class::remove(rcm);
  }

  Status AgregatClass::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    return Class::trace_realize(fd, indent, flags, rcm);
  }

  /*
    Bool
    AgregatClass::compare_perform(const Class *cl) const
    {
    if (!cl->asAgregatClass())
    return False;

    const AgregatClass *ma = (AgregatClass *)cl;

    if (asUnionClass() != ma->asUnionClass())
    return False;

    if (items_cnt != ma->items_cnt)
    return False;

    for (int i = 0; i < items_cnt; i++)
    if (!items[i]->compare(db, ma->items[i]))
    return False;

    return True;
    }
  */

  Bool
  AgregatClass::compare_perform(const Class *cl,
				Bool compClassOwner,
				Bool compNum,
				Bool compName,
				Bool inDepth) const
  {
    if (!cl->asAgregatClass())
      return False;

    const AgregatClass *ma = (AgregatClass *)cl;

    if (asUnionClass() != ma->asUnionClass())
      return False;

    if (items_cnt != ma->items_cnt)
      return False;

    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->compare(db, ma->items[i], compClassOwner, compNum,
			     compName, inDepth))
	return False;

    return True;
  }

  Status
  agregatClassMake(Database *db, const Oid *oid, Object **o,
		   const RecMode *rcm, const ObjectHeader *hdr,
		   Data idr, LockMode lockmode, const Class*)
  {
    RPCStatus rpc_status;
    Status status;
    Data temp;

    if (ObjectPeer::isRemoved(*hdr))
      {
	*o = new StructClass("<removed_class>");
	return Success;
      }

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
	eyedblib::int32 cnt;
	char *s;
	AgregatClass *ma;
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

	// WARNING: ADDED THE 23/05/01 for test
	eyedbsm::Oid parent_oid, *poid;
	oid_decode(temp, &offset, &parent_oid);

	if (isOidValid(&parent_oid))
	  poid = &parent_oid;
	else
	  poid = 0;

	Oid ppoid(poid);
	if (hdr->type == _StructClass_Type)
	  ma = new StructClass(s, &ppoid);
	else if (hdr->type == _UnionClass_Type)
	  ma = new UnionClass(s, &ppoid);
	else
	  return Exception::make(IDB_CLASS_READ, "agregat_class '%s' unknown code `%d'", s, code);

	free(s); s = 0;
	ma->setExtentImplementation(idximpl, True);
	if (idximpl)
	  idximpl->release();
	ma->setInstanceDspid(dspid);

	ClassPeer::setMType(ma, (Class::MType)mt);
	int32_decode(temp, &offset, (eyedblib::int32 *)&ma->idr_psize);
	int32_decode(temp, &offset, (eyedblib::int32 *)&ma->idr_vsize);
	int32_decode(temp, &offset, (eyedblib::int32 *)&ma->idr_objsz);
	int32_decode(temp, &offset, &cnt);

	unsigned int native_cnt;
	const Attribute **items_nat = ma->getAttributes(native_cnt);

	ma->items = (Attribute **)malloc(sizeof(Attribute *) * cnt);
	ma->items_cnt = cnt;

	int i;
	/* disconnected again the 30/04/02 */
#if 1 /* WARNING: this code has been restored on the 14/10/97 because
	 of memory leaks found with purify! */
	for (i = 0; i < native_cnt; i++)
	  ma->items[i] = (Attribute *)items_nat[i];
#else
	for (i = 0; i < native_cnt; i++)
	  ma->items[i] = new AttrNative((AttrNative *)items_nat[i],
					items_nat[i]->getClass(),
					items_nat[i]->getClassOwner(),
					ma, i);
#endif

	free(items_nat);

	for (i = native_cnt; i < cnt; i++)
	  ma->items[i] = makeAttribute(db, temp, &offset, ma, i);

	*o = (Object *)ma;

	ObjectPeer::setOid(ma, *oid);
	if (!db->isOpeningState() && !db->isBackEnd()) {
	  status = ma->setDatabase(db);
	  if (status)
	    return status;
	}

	status = ClassPeer::makeColls(db, (Class *)*o, temp);

	if (status != Success) {
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

  // indexes: Front End methods
  Status AgregatClass::createIndexes(void)
  {
    /*
      IDB_LOG(IDB_LOG_IDX_CREATE,
      ("AgregatClass::createIndexes(%s, attr_cnt=%d)\n",
      name, items_cnt));
      //  foreach_item(createIndex(db, this, (const AttrIdxContext *)0, 0));
      foreach_item(createIndex(db, this, (const AttrIdxContext *)0, items[i]->getIndexMode()));
    */
    return Success;
  }


  // indexes: Back End methods

  Status AgregatClass::openIndexes_realize(Database *_db)
  {
    assert(0);
    /*
      if (_db->isBackEnd())
      foreach_item(openIndex_realize(_db));
    */
    return Success;
  }

  Status
  AgregatClass::createNestedIndex(AttrIdxContext &attr_idx_ctx,
				  const AttrIdxContext *tg_idx_ctx,
				  int _mode)
  {
    assert(0);
    /*
      foreach_item(createNestedIndex(attr_idx_ctx, tg_idx_ctx, _mode));
    */
    return Success;
  }

  Status
  AgregatClass::removeNestedIndex(AttrIdxContext &attr_idx_ctx,
				  const AttrIdxContext *tg_idx_ctx,
				  int _mode)
  {
    assert(0);
    /*
      foreach_item(removeNestedIndex(attr_idx_ctx, tg_idx_ctx, _mode));
    */
    return Success;
  }

  static inline Oid
  getClassOid(Data idr)
  {
    eyedbsm::Oid oid;
    Oid roid;
    Offset offset = IDB_OBJ_HEAD_OID_MCL_INDEX;
    oid_decode(idr, &offset, &oid);
    roid.setOid(oid);
    return roid;
  }

  //#define PERF_POOL

#ifdef PERF_POOL
  struct CreateIndexArgs {
    Database *db;
    Data idr;
    const Oid *oid;
    AttrIdxContext *idx_ctx;
    const Oid *cloid;
    int offset;
    Bool novd;
    int count;
    int size;
    CreateIndexArgs(Database *_db, Data _idr,
		    const Oid *_oid, AttrIdxContext *_idx_ctx,
		    const Oid *_cloid, int _offset, Bool _novd,
		    int _count, int _size) :
      db(_db), idr(_idr), oid(_oid), idx_ctx(_idx_ctx),
      cloid(_cloid), offset(_offset), novd(_novd), count(_count),
      size(_size) { }
  };

  PerformerArg
  createIndexEntryWrapper(PerformerArg xarg)
  {
    Attribute *attr = (Attribute *)xarg.data;
    CreateIndexArgs *arg = (CreateIndexArgs *)attr->getUserData();
    AttrIdxContext idx_ctx(arg->idx_ctx);
    Status s = attr->createIndexEntry_realize(arg->db, arg->idr,
					      arg->oid,
					      arg->cloid, arg->offset,
					      arg->novd, idx_ctx,
					      arg->count, arg->size);
    return PerformerArg((void *)s);
  }

  Status
  AgregatClass::createIndexEntries_realize(Database *_db,
					   Data _idr,
					   const Oid *_oid,
					   AttrIdxContext &idx_ctx,
					   const Oid *cloid,
					   int offset, Bool novd,
					   int count, int size)
  {
    Oid stoid;
    if (!cloid) {stoid = getClassOid(_idr); cloid = &stoid; }

    Performer *perfs[100]; // should be item_cnt
    CreateIndexArgs idxargs(_db, _idr, _oid, &idx_ctx, cloid, offset, novd,
			    count, size);
    PerformerPool *perfpool = idbPerformerPoolManager::getPerfPool();
#ifdef POOL_TRACE
    printf("%s: CREATE GETTING performer pool %p #1\n", name, perfpool);
#endif

    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative()) {
	items[i]->setUserData(&idxargs);
	perfs[i] = perfpool->start(createIndexEntryWrapper, items[i]);
      }

#ifdef POOL_TRACE
    printf("%s: CREATE GETTING performer pool %p #2\n", name, perfpool);
#endif
    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative()) {
	PerformerArg arg = perfpool->wait(perfs[i]);
	if (arg.data) {
	  perfpool->waitAll();
	  if (!idx_ctx.getLevel())
	    idx_ctx.realizeIdxOP(False);
	  return (Status)arg.data;
	}
      }	

    if (perfpool->isProfiled()) {
      static int prof_cnt = 0;
      if (!(++prof_cnt % 20)) {
	unsigned int cnt;
	eyedblib::Thread::Profile **profiles = perfpool->getProfiles(cnt);
	cout << profiles << endl;
	//PeformerPool::deleteProfiles(profiles);
	for (int i = 0; i < cnt; i++)
	  delete profiles[i];
	delete[] profiles;
      }
    }
#ifdef POOL_TRACE
    printf("%s: CREATE GETTING performer pool %p #3\n", name, perfpool);
#endif
    return idx_ctx.getLevel() ? Success : idx_ctx.realizeIdxOP(True);
  }

#else
  Status
  AgregatClass::createIndexEntries_realize(Database *_db,
					   Data _idr,
					   const Oid *_oid,
					   AttrIdxContext &idx_ctx,
					   const Oid *cloid,
					   int offset, Bool novd,
					   int count, int size)
  {
    Oid stoid;
    if (!cloid) {stoid = getClassOid(_idr); cloid = &stoid; }

    Status status;
    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative() && ((status = items[i]->createIndexEntry_realize(_db, _idr, _oid, cloid, offset, novd, idx_ctx, count, size))))
	{
	  if (!idx_ctx.getLevel())
	    idx_ctx.realizeIdxOP(False);
	  return status;
	}
	
    return idx_ctx.getLevel() ? Success : idx_ctx.realizeIdxOP(True);
  }
#endif

#ifdef PERF_POOL
  struct UpdateIndexArgs {
    Database *db;
    Data idr;
    const Oid *oid;
    AttrIdxContext *idx_ctx;
    const Oid *cloid;
    int offset;
    Bool novd;
    const Oid *data_oid;
    int count;
    UpdateIndexArgs(Database *_db, Data _idr,
		    const Oid *_oid, AttrIdxContext *_idx_ctx,
		    const Oid *_cloid, int _offset, Bool _novd,
		    const Oid *_data_oid, int _count) :
      db(_db), idr(_idr), oid(_oid), idx_ctx(_idx_ctx),
      cloid(_cloid), offset(_offset), novd(_novd), data_oid(_data_oid),
      count(_count) { }
  };

  PerformerArg
  updateIndexEntryWrapper(PerformerArg xarg)
  {
    Attribute *attr = (Attribute *)xarg.data;
    UpdateIndexArgs *arg = (UpdateIndexArgs *)attr->getUserData();
    AttrIdxContext idx_ctx(arg->idx_ctx);
    Status s = attr->updateIndexEntry_realize(arg->db, arg->idr,
					      arg->oid,
					      arg->cloid, arg->offset,
					      arg->novd, arg->data_oid,
					      idx_ctx,
					      arg->count);
    return PerformerArg((void *)s);
  }

  Status
  AgregatClass::updateIndexEntries_realize(Database *_db,
					   Data _idr,
					   const Oid *_oid,
					   AttrIdxContext &idx_ctx,
					   const Oid *cloid,
					   int offset, Bool novd,
					   const Oid *data_oid,
					   int count)
  {
    Oid stoid;
    if (!cloid) {stoid = getClassOid(_idr); cloid = &stoid; }

    Performer *perfs[100]; // should be item_cnt
    UpdateIndexArgs idxargs(_db, _idr, _oid, &idx_ctx, cloid, offset, novd,
			    data_oid, count);
    PerformerPool *perfpool = idbPerformerPoolManager::getPerfPool();
#ifdef POOL_TRACE
    printf("%s: UPDATE GETTING performer pool %p #1\n", name, perfpool);
#endif

    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative()) {
	items[i]->setUserData(&idxargs);
	perfs[i] = perfpool->start(updateIndexEntryWrapper, items[i]);
      }

#ifdef POOL_TRACE
    printf("%s: UPDATE GETTING performer pool %p #2\n", name, perfpool);
#endif
    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative()) {
	PerformerArg arg = perfpool->wait(perfs[i]);
	if (arg.data) {
	  perfpool->waitAll();
	  if (!idx_ctx.getLevel())
	    idx_ctx.realizeIdxOP(False);
	  return (Status)arg.data;
	}
      }	

#ifdef POOL_TRACE
    printf("%s: UPDATE GETTING performer pool %p #3\n", name, perfpool);
#endif
    return idx_ctx.getLevel() ? Success : idx_ctx.realizeIdxOP(True);
  }
#else
  Status
  AgregatClass::updateIndexEntries_realize(Database *_db,
					   Data _idr,
					   const Oid *_oid,
					   AttrIdxContext &idx_ctx,
					   const Oid *cloid,
					   int offset, Bool novd,
					   const Oid *data_oid,
					   int count)
  {
    Oid stoid;
    if (!cloid) {stoid = getClassOid(_idr); cloid = &stoid; }

    Status status;
    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative() &&
	  ((status = items[i]->updateIndexEntry_realize
	    (_db, _idr, _oid, cloid, offset, novd,
	     data_oid, idx_ctx, count))))
	{
	  if (!idx_ctx.getLevel())
	    idx_ctx.realizeIdxOP(False);
	  return status;
	}

    return idx_ctx.getLevel() ? Success : idx_ctx.realizeIdxOP(True);
  }
#endif

  Status
  AgregatClass::removeIndexEntries_realize(Database *_db,
					   Data _idr,
					   const Oid *_oid,
					   AttrIdxContext &idx_ctx,
					   const Oid *cloid,
					   int offset, Bool novd,
					   const Oid *data_oid,
					   int count)
  {
    Oid stoid;
    if (!cloid) {stoid = getClassOid(_idr); cloid = &stoid; }

    foreach_item(removeIndexEntry_realize(_db, _idr, _oid, cloid, offset, novd,
					  data_oid, idx_ctx, count));

    return idx_ctx.getLevel() ? Success : idx_ctx.realizeIdxOP(True);
  }

  //
  // inverses
  //

  Status AgregatClass::createInverses_realize(Database *_db, Data _idr, const Oid *_oid)
  {
    if (asUnionClass())
      {
	Attribute *item = (Attribute *)Union::decodeCurrentItem(this, _idr);
	if (item)
	  item->createInverse_realize(_db, _idr, _oid);
      }
    else
      foreach_item(createInverse_realize(_db, _idr, _oid));
    return Success;
  }

  Status AgregatClass::updateInverses_realize(Database *_db, Data _idr, const Oid *_oid)
  {
    if (asUnionClass())
      {
	Attribute *item = (Attribute *)Union::decodeCurrentItem(this, _idr);
	if (item)
	  item->updateInverse_realize(_db, _idr, _oid);
      }
    else
      foreach_item(updateInverse_realize(_db, _idr, _oid));
    return Success;
  }

  Status AgregatClass::removeInverses_realize(Database *_db, Data _idr, const Oid *_oid)
  {
    if (asUnionClass())
      {
	Attribute *item = (Attribute *)Union::decodeCurrentItem(this, _idr);
	if (item)
	  item->removeInverse_realize(_db, _idr, _oid);
      }
    else
      foreach_item(removeInverse_realize(_db, _idr, _oid));
    return Success;
  }

  void AgregatClass::garbage()
  {
    Class::garbage();
  }

  AgregatClass::~AgregatClass()
  {
    garbageRealize();
  }
}
