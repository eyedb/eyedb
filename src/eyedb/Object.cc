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
#include <eyedblib/butils.h>
#include "UserDataHT.h"
#include <time.h>
#include <sstream>

using std::ostringstream;

// 31/08/06
#define NO_GARB_REALIZE

using namespace std;

namespace eyedb {

#define IDR_TRACE(MSG) printf("IDR: " MSG " %d => %p\n", idr_sz, idr) 
#undef IDR_TRACE

#ifdef IDR_TRACE

#define FREE_IDR_TRACE()   if (idr) IDR_TRACE("free")

#define ALLOC_IDR_TRACE()  if (idr_sz) IDR_TRACE("allocate")

#define SET_IDR_TRACE()    if (idr_sz) IDR_TRACE("set")

#else

#define FREE_IDR_TRACE()
#define ALLOC_IDR_TRACE()
#define SET_IDR_TRACE()

#endif

  Bool Object::release_cycle_detection = False;

  // force static initialisation order
  static std::string &getObjectTag() {
    static std::string ObjectTag = "eyedb::Object";
    return ObjectTag;
  }

  void *idr_dbg;

  void stop_alloc_dbg() {
  }

  void
  alloc_dbg(void *idr)
  {
    if (idr != 0 && idr == idr_dbg)
      stop_alloc_dbg();
  }

  Object::IDR::IDR(Size _idr_sz)
  {
    refcnt = 1;
    idr_sz = _idr_sz;
    idr = (unsigned char *)(idr_sz ? malloc(idr_sz) : 0);
    ALLOC_IDR_TRACE();
    alloc_dbg(idr);
  }

  Object::IDR::IDR(Size _idr_sz, Data _idr)
  {
    refcnt = 1;
    idr_sz = _idr_sz;
    idr = _idr;
    SET_IDR_TRACE();
  }

  void
  Object::IDR::setIDR(Size sz)
  {
    FREE_IDR_TRACE();
    free(idr);
    idr_sz = sz;
    idr = (unsigned char *)(idr_sz ? malloc(idr_sz) : 0);
    ALLOC_IDR_TRACE();
    alloc_dbg(idr);
  }

  void
  Object::IDR::setIDR(Data _idr)
  {
    if (idr == _idr)
      return;

    FREE_IDR_TRACE();
    free(idr);
    idr = _idr;
    SET_IDR_TRACE();
  }

  void
  Object::IDR::setIDR(Size sz, Data _idr)
  {
    if (idr == _idr)
      {
	assert(idr_sz == sz);
	return;
      }

    FREE_IDR_TRACE();
    free(idr);

    idr = _idr;
    idr_sz = sz;
    SET_IDR_TRACE();
  }

  Object::IDR::~IDR()
  {
    assert(refcnt <= 1);
    FREE_IDR_TRACE();
    free(idr);
  }

  void Object::init(Bool _init)
  {
    state = 0;
    modify = False;
    applyingTrigger = False;
    dirty = False;
    user_data = (void *)0;
    user_data_ht = 0;
    oql_info = (void *)0;
    xinfo = 0;

    dataspace = 0;
    dspid = Dataspace::DefaultDspid;

    removed = False;
    unrealizable = False;
    grt_obj = False;
    c_time = 0;
    m_time = 0;
    master_object = 0;
    damaged_attr = 0;

    if (_init) {
      memset(&oid, 0, sizeof(oid));
      db = 0;
      idr = new IDR(0);
      cls = 0;
      type = 0;
    }
    
    IDB_LOG(IDB_LOG_OBJ_INIT, ("Object::init(o=%p)\n", this));
  }

  void Object::initialize(Database *_db)
  {
    init(True);
    db = _db;
    //  userInitialize();
  }

  Object::Object(Database *_db, const Dataspace *_dataspace) : gbxObject(getObjectTag())
  {
    init(True); // conceptually this 3 calls are: Object::initialize(_db);
    dataspace = _dataspace;
    db = _db;
    if (db) db->addToRegister(this);
  }

  void
  Object::copy(const Object *o, Bool share)
  {
    IDB_LOG(IDB_LOG_OBJ_COPY, ("Object::operator=(o=%p <= %p [share=%s])\n",
			       this, o, share ? "true" : "false"));
    if (!o) {
      init(True);
      return;
    }

    init(False);

    oid = o->oid;
    db = o->db;
    cls = o->cls;
    type = o->type;
    dataspace = o->dataspace;
    dspid = o->dspid;
    c_time = o->c_time;
    m_time = o->m_time;
    master_object = o->master_object;

    if (share) {
      idr = o->idr;
      idr->incrRefCount();
    }
    else {
      idr = new IDR(o->idr->getSize());
      if (idr->getSize())
	memcpy(idr->getIDR(), o->idr->getIDR(), idr->getSize());
    }

    if (cls)
      setTag(cls->getName());
  }

  Object::Object(const Object *o, Bool share) : gbxObject(o)
  {
    idr = 0;
    copy(o, share);
  }


  Object::Object(const Object &o) : gbxObject(o)
  {
    idr = 0;
    copy(&o, False);
  }

  void
  Object::setClass(Class *_cls)
  {
    cls = _cls;
    if (cls)
      setTag(cls->getName());
  }

  void
  Object::setClass(const Class *_cls)
  {
    cls = const_cast<Class *>(_cls);
    if (cls)
      setTag(cls->getName());
  }

  void
  Object::unlock_refcnt()
  {
    gbx_locked = gbxFalse;
    while (getRefCount() > 1)
      decrRefCount();
  }

  Object& Object::operator=(const Object &o)
  {
    if (&o == this)
      return *this;

    unlock_refcnt();

#ifndef NO_GARB_REALIZE
    garbageRealize();
#endif

    *(gbxObject *)this = gbxObject::operator=((const gbxObject &)o);

    copy(&o, False);

    return *this;
  }

  /*
    const Type Object::getType() const
    {
    return type;
    }

    const Oid Object::getOid() const
    {
    return oid;
    }
  */

  void Object::setOid(const Oid& oid_new)
  {
    Oid oid_last = oid;

    if (db && oid_last.isValid() && !oid_last.compare(oid_new))
      db->uncacheObject(this);

    oid = oid_new;
  }

  void *Object::setUserData(void *nuser_data)
  {
    void *x = user_data;
    user_data = nuser_data;
    return x;
  }

  void *Object::setOQLInfo(void *noql_info)
  {
    void *x = oql_info;
    oql_info = noql_info;
    return x;
  }

  void *Object::getOQLInfo()
  {
    return oql_info;
  }

  Status Object::setDatabase(Database *mdb)
  {
    if (unrealizable)
      return Exception::make(IDB_SETDATABASE_ERROR, "object not realizable");

    Database *odb = db;

    if (odb &&
	odb != mdb &&
	oid.isValid() &&
	oid.getDbid() != mdb->getDbid())
      return Exception::make(IDB_SETDATABASE_ERROR,
			     "cannot change dynamically database of "
			     "a persistent object");

    if (cls)
      {
	const char *name = cls->getName();
	cls = mdb->getSchema()->getClass(name);
      
	if (!cls)
	  return Exception::make(IDB_SETDATABASE_ERROR,
				 "class '%s' not found in schema\n", name);
      }

    if (db != mdb)
      {
	if (db)
	  db->rmvFromRegister(this);
	db = mdb;
	if (db)
	  db->addToRegister(this);
      }

    return Success;
  }

  Status Object::store(const RecMode *recmode)
  {
    bool realizeBlock = db && recmode != RecMode::NoRecurs;
    if (realizeBlock) {
      db->beginRealize();
    }

    Status s = realize(recmode);
    if (realizeBlock) {
      db->endRealize();
    }
    return s;
  }

  void Object::setUnrealizable(Bool _unrealizable)
  {
    unrealizable = _unrealizable;
    if (unrealizable) {
      db = 0;
      setOid(*getInvalidOid());
    }
  }

  /*
    Bool Object::isUnrealizable() const
    {
    return unrealizable;
    }

    Database *Object::getDatabase()
    {
    return db;
    }

    const Database *Object::getDatabase() const
    {
    return db;
    }
  */

  void Object::headerCode(eyedblib::int32 t, eyedblib::int32 size, eyedblib::int32 _xinfo)
  {
    ObjectHeader hdr;

    memset(&hdr, 0, sizeof(hdr));

    hdr.type = t;
    hdr.size = size;
    hdr.xinfo = _xinfo;

    if (cls)
      hdr.oid_cl = *cls->getOid().getOid();

    object_header_code_head(idr->getIDR(), &hdr);

    if (eyedb_is_type(hdr, _Class_Type))
      memset(idr->getIDR() + IDB_CLASS_COLL_START, 0,
	     IDB_CLASS_COLLS_CNT * sizeof(eyedbsm::Oid));
  }

  void Object::classOidCode(void)
  {
    Offset offset = IDB_OBJ_HEAD_OID_MCL_INDEX;
    Size alloc_size = IDB_OBJ_HEAD_SIZE;

    Data data = idr->getIDR();
    oid_code(&data, &offset, &alloc_size, cls->getOid().getOid());
  }

  Status Object::realize(const RecMode *rcm)
  {
    if (state & Realizing)
      return Success;

    CHK_OBJ(this);

    if (master_object)
      return master_object->realize(rcm);

    Status status;
    state |= Realizing;

    if (!oid.isValid())
      status = create();
    else /* if (modify) */
      status = update();

    if (!status)
      {
	modify = False;
	applyingTrigger = False;
	dirty = False;
      }
  
    state &= ~Realizing;
    return status;
  }

  Bool Object::traceRemoved(FILE *fd, const char *indent_str) const
  {
    if (removed)
      {
	fprintf(fd, "%s<object removed>\n", indent_str);
	return True;
      }
    return False;
  }

  Status Object::remove(const RecMode *rcm)
  {
    return remove_r(rcm);
  }

  Status Object::remove_r(const RecMode *rcm, unsigned int flags)
  {
    if (removed)
      return Exception::make(IDB_OBJECT_REMOVE_ERROR, "object %s already removed", oid.toString());
    if (!oid.isValid())
      return Exception::make(IDB_OBJECT_REMOVE_ERROR, "invalid object to remove");

    IDB_CHECK_WRITE(db);

    RPCStatus rpc_status;

    rpc_status = objectDelete(db->getDbHandle(), oid.getOid(), flags);

    if (rpc_status == RPCSuccess)
      {
	db->uncacheObject(this);
	removed = True;
      }

    return StatusMake(rpc_status);
  }

#define ObjectMagicDeleted 6666666

  void Object::garbage()
  {
    IDB_LOG(IDB_LOG_OBJ_GARBAGE,
	    ("Object::garbage(o=%p, oid=%s, class=\"%s\", "
	     "idr=%p, idr:refcnt=%d)\n",
	     this, oid.toString(), cls ? cls->getName() : "<unknown>",
	     idr->getIDR(), idr->getRefCount()));

    if (db)
      {
	db->uncacheObject(this);
	db->rmvFromRegister(this);
      }

    if (!idr->decrRefCount())
      {
	delete idr;
	idr = 0;
      }

    delete user_data_ht;
  }


  Object::~Object()
  {
    garbageRealize();
  }

  /*
    Bool Object::isModify() const
    {
    return modify;
    }
  */

  void Object::touch()
  {
    modify = True;
    /*
      if (master_object)
      master_object->touch();
    */
  }

  Status
  Object::setLock(LockMode lockmode)
  {
    return db->setObjectLock(oid, lockmode);
  }

  Status
  Object::setLock(LockMode lockmode, LockMode &alockmode)
  {
    return db->setObjectLock(oid, lockmode, alockmode);
  }

  Status
  Object::getLock(LockMode &lockmode)
  {
    return db->getObjectLock(oid, lockmode);
  }

  eyedbsm::Oid ClassOidDecode(Data idr)
  {
    Offset offset = IDB_OBJ_HEAD_OID_MCL_INDEX;
    eyedbsm::Oid oid;

    oid_decode(idr, &offset, &oid);
    return oid;
  }

  const char *Object::getStringCTime() const
  {
    return eyedblib::setbuftime(c_time);
  }

  const char *Object::getStringMTime() const
  {
    return eyedblib::setbuftime(m_time);
  }

  void
  Object::trace_flags(FILE *fd, unsigned int flags) const
  {
    if (flags & PointerTrace)
      fprintf(fd, "[this = 0x%x] ", this);

    if (oid.isValid())
      {
	if ((flags & CMTimeTrace) == CMTimeTrace)
	  fprintf(fd, "ctime = %s / mtime = %s",
		  getStringCTime(), getStringMTime());
	else if ((flags & CTimeTrace) == CTimeTrace)
	  fprintf(fd, "ctime = %s", getStringCTime());
	else if ((flags & MTimeTrace) == MTimeTrace)
	  fprintf(fd, "mtime = %s", getStringMTime());
      }
  }

  Status 
  Object::apply(Database *_db, Method *mth, ArgArray &argarray,
		Argument &retarg, Bool checkArgs)
  {
    return mth->applyTo(_db, this, argarray, retarg, checkArgs);
  }

  const Object *Object::getMasterObject(bool recurs) const
  {
    return const_cast<Object *>(this)->getMasterObject(recurs);
  }

  Object *Object::getMasterObject(bool recurs)
  {
    if (!recurs || !master_object)
      return master_object;

    assert(recurs && master_object);

    if (master_object->getMasterObject(false)) // has an immediate master object
      return master_object->getMasterObject(true);

    return master_object;
  }

  Status
  Object::setMasterObject(Object *_master_object)
  {
    master_object = _master_object;
    return Success;
  }

  Status
  Object::releaseMasterObject()
  {
    master_object = 0;
    /*
    printf("Release_master_object %d\n", protect_on_release_master);
    if (getOid().isValid() && !protect_on_release_master) {
      printf("removing %s\n", getOid().toString());
      return remove();
    }
    */

    return Success;
  }

  Status
  Object::setProtection(const Oid &prot_oid)
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");
    return db->setObjectProtection(oid, prot_oid);
  }

  Status
  Object::setProtection(Protection *prot)
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");
    return db->setObjectProtection(oid, prot);
  }

  Status
  Object::getProtection(Oid &prot_oid) const
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");
    prot_oid = oid_prot;
    return Success;
    //  return db->getObjectProtection(oid, prot_oid);
  }

  Status
  Object::getProtection(Protection *&prot) const
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");

    if (oid_prot.isValid())
      return db->loadObject(&oid_prot, (Object **)&prot);

    prot = NULL;
    return Success;
    //  return db->getObjectProtection(oid, prot);
  }

  //
  // ObjectArray
  //

#define TRY_GETELEMS_GC

  ObjectArray::ObjectArray(Object **_objs, unsigned int _count)
  {
    objs = 0;
    count = 0;
    auto_garb = False;
    set(_objs, _count);
  }

  ObjectArray::ObjectArray(bool _auto_garb, Object **_objs, unsigned int _count)
  {
    objs = 0;
    count = 0;
    auto_garb = _auto_garb;
    set(_objs, _count);
  }

  ObjectArray::ObjectArray(const Collection *coll, bool _auto_garb)
  {
    objs = 0;
    count = 0;
    auto_garb = _auto_garb;
    coll->getElements(*this);
  }

  void ObjectArray::set(Object **_objs, unsigned int _count)
  {
    // 01/11/06
#ifdef TRY_GETELEMS_GC
    if (auto_garb && _count && !_objs)
      throw *Exception::make(IDB_ERROR,
			     "cannot set an auto-garbaged object array "
			     "for %d objets with no object pointer",
			     _count);

#endif
    if (auto_garb)
      garbage();

    free(objs);

    int sz = _count * sizeof(Object *);
    objs = (Object **)malloc(sz);
    if (_objs)
      memcpy(objs, _objs, sz);
    count = _count;

#ifdef TRY_GETELEMS_GC
    if (auto_garb) {
      for (int n = 0; n < count; n++) {
	if (objs[n]) {
	  objs[n]->incrRefCount();
	}
      }
    }
#endif
  }

  void ObjectArray::setMustRelease(bool must_release)
  {
    for (int i = 0; i < count; i++)
      if (objs[i])
	objs[i]->setMustRelease(must_release);
  }

  ObjectArray::ObjectArray(const ObjectArray &objarr)
  {
    count = 0;
    objs = 0;
    auto_garb = False;
    *this = objarr;
  }

  Status ObjectArray::setObjectAt(unsigned int ind, Object *o)
  {
    if (ind >= count)
      return Exception::make(IDB_ERROR, "invalid range %d (maximun is %d)",
			     ind, count);
    if (objs[ind] == o)
      return Success;

#ifdef TRY_GETELEMS_GC
    if (auto_garb) {

      if (objs[ind])
	objs[ind]->release();

      objs[ind] = o;

      if (objs[ind])
	objs[ind]->incrRefCount();

      return Success;
    }
#endif

    objs[ind] = o;
    return Success;
  }

  ObjectArray::ObjectArray(const ObjectList &list)
  {
    count = 0;
    auto_garb = False;

    int cnt = list.getCount();
    if (!cnt)
      {
	objs = 0;
	return;
      }

    objs = (Object **)malloc(sizeof(Object *) * cnt);

    ObjectListCursor c(list);
    Object *o;

    for (; c.getNext(o); count++) {
      objs[count] = o;
      /*
#ifdef TRY_GETELEMS_GC
      // 31/10/06
      if (auto_garb)
	o->incrRefCount();
#endif
      */
    }
  }

  ObjectArray& ObjectArray::operator=(const ObjectArray &objarr)
  {
    set(objarr.objs, objarr.count);
#ifndef TRY_GETELEMS_GC
    auto_garb = objarr.auto_garb;
#endif
    return *this;
  }

  void ObjectArray::garbage()
  {
    for (int i = 0; i < count; i++)
      if (objs[i]) {
	objs[i]->release();
	objs[i] = 0;
      }
  }

  ObjectList *
  ObjectArray::toList() const
  {
    return new ObjectList(*this);
  }

  void ObjectArray::makeObjectPtrVector(ObjectPtrVector &obj_vect)
  {
    for (int n = 0; n < count; n++)
      obj_vect.push_back(objs[n]);
  }

  ObjectArray::~ObjectArray()
  {
    if (auto_garb)
      garbage();

    free(objs);
  }

  extern ostream& 
  tmpfile2stream(const char *file, ostream& os, FILE *fd);

  ostream& operator<<(ostream& os, const ObjectPtr &o_ptr)
  {
    return os << o_ptr.getObject();
  }

  ostream& operator<<(ostream& os, const Object &o)
  {
    return os << &o;
  }

  ostream& operator<<(ostream& os, const Object *o)
  {
    if (!o)
      return os << "NULL\n";

    Status s = o->trace(get_file());
    if (s) return os << s->getDesc();
    return convert_to_stream(os);
  }

  std::string
  Object::toString(unsigned int flags, const RecMode *rcm,
		   Status *pstatus) const
  {
    Status status = trace(get_file(), flags, rcm);
    if (status) {
      if (pstatus) *pstatus = status;
      return status->getDesc();
    }

    ostringstream ostr;
    convert_to_stream(ostr);
    //ostr << ends;
    //const std::string &str = ostr.str();
    //std::string s(str);
    std::string s(ostr.str());
    if (pstatus) *pstatus = Success;
    return s;
  }

  void
  Object::freeList(LinkedList *list, Bool wipeOut)
  {
    if (!list)
      return;

    if (wipeOut)
      {
	Object *o;
	LinkedListCursor c(list);
  
	while (c.getNext((void *&)o))
	  o->release();
      }
  
    delete list;
  }


  LinkedList *
  Object::copyList(const LinkedList *list, Bool copy)
  {
    if (!list)
      return (LinkedList *)0;

    LinkedList *rlist = new LinkedList();
    Object *o;

    LinkedListCursor c(list);

    while (c.getNext((void *&)o))
      rlist->insertObject(copy && o ? o->clone() : o);
  
    return rlist;
  }

  //
  // object list
  //

  ObjectList::ObjectList()
  {
    list = new LinkedList();
  }

  ObjectList::ObjectList(const ObjectArray &obj_array)
  {
    list = new LinkedList();
    for (int i = 0; i < obj_array.getCount(); i++)
      insertObjectLast(const_cast<Object *>(obj_array[i]));
  }

  int ObjectList::getCount(void) const
  {
    return list->getCount();
  }

  void
  ObjectList::insertObject(Object *o)
  {
    list->insertObject(o);
  }

  void
  ObjectList::insertObjectFirst(Object *o)
  {
    list->insertObjectFirst(o);
  }

  void
  ObjectList::insertObjectLast(Object *o)
  {
    list->insertObjectLast(o);
  }

  Bool
  ObjectList::suppressObject(Object *o)
  {
    return list->deleteObject(o) ? True : False;
  }

  Bool
  ObjectList::exists(const Object *o) const
  {
    return list->getPos((void *)o) >= 0 ? True : False;
  }

  void
  ObjectList::empty()
  {
    list->empty();
  }

  ObjectArray *
  ObjectList::toArray() const
  {
    int cnt = list->getCount();
    if (!cnt)
      return new ObjectArray();
    Object **arr = (Object **)malloc(sizeof(Object *) * cnt);

    LinkedListCursor c(list);
    for (int i = 0; c.getNext((void *&)arr[i]); i++)
      ;
  
    ObjectArray *obj_arr = new ObjectArray(arr, cnt);
    free(arr);
    return obj_arr;
  }

  ObjectList::~ObjectList()
  {
    delete list;
  }

  ObjectListCursor::ObjectListCursor(const ObjectList &oidlist)
  {
    c = new LinkedListCursor(oidlist.list);
  }

  ObjectListCursor::ObjectListCursor(const ObjectList *oidlist)
  {
    c = new LinkedListCursor(oidlist->list);
  }

  Bool
  ObjectListCursor::getNext(Object *&o)
  {
    return c->getNext((void *&)o) ? True : False;
  }

  ObjectListCursor::~ObjectListCursor()
  {
    delete c;
  }

  Status
  Object::loadPerform(const Oid&, LockMode,AttrIdxContext &idx_ctx,
		      const RecMode*)
  {
    return Success;
  }

  Status
  Object::removePerform(const Oid&, const Oid&, AttrIdxContext&, const RecMode*)
  {
    return Success;
  }

  Status
  Object::realizePerform(const Oid&, const Oid&, AttrIdxContext&, const RecMode*)
  {
    CHK_OBJ(this);
    return Success;
  }

  Status
  Object::getDataspace(const Dataspace *&_dataspace,
		       Bool refetch) const
  {
    /*
      if (dataspace) {
      _dataspace = dataspace;
      return Success;
      }

      getDataspaceID();

      if (dspid == Dataspace::DefaultDspid) {
      _dataspace = 0;
      return Success;
      }

      Status s = db->getDataspace(dspid, _dataspace);
      if (s) return s;
      const_cast<Object *>(this)->dataspace = _dataspace;
      return Success;
    */

    _dataspace = 0;

    if (oid.isValid() && ((!dataspace && dspid == Dataspace::DefaultDspid) ||
			  refetch)) {
      ObjectLocation objloc;
      Status s = getLocation(objloc);
      if (s)
	return s;
      const_cast<Object *>(this)->dspid = objloc.getDspid();
      s = db->getDataspace(dspid, _dataspace);
      if (s)
	return s;
      const_cast<Object *>(this)->dataspace = _dataspace;
      return Success;
    }

    if (dataspace) {
      _dataspace = dataspace;
      return Success;
    }

    if (dspid != Dataspace::DefaultDspid) {
      Status s = db->getDataspace(dspid, _dataspace);
      if (s)
	return s;
      const_cast<Object *>(this)->dataspace = _dataspace;
      return Success;
    }

    return Success;
  }

  Status
  Object::getLocation(ObjectLocation &loc) const
  {
    OidArray oid_arr(&oid, 1);
    ObjectLocationArray locarr;
    Status s = db->getObjectLocations(oid_arr, locarr);
    if (s)
      return s;
    loc = locarr.getLocation(0);
    return Success;
  }


  Status
  Object::setDataspace(const Dataspace *_dataspace)
  {
    /*
      if (oid.isValid() && dataspace) {
      if (dataspace->getId() == _dataspace->getId())
      return Success;
      return Exception::make(IDB_ERROR, "use the move method to change the "
      "dataspace [#%d to #%d] on the already "
      "created object %s", dataspace->getId(),
      _dataspace->getId(), oid.toString());

      }
    */

    Status s = getDataspace(dataspace);
    if (s)
      return s;

    if (oid.isValid() && dataspace && dataspace->getId() != _dataspace->getId())
      return Exception::make(IDB_ERROR, "use the move method to change the "
			     "dataspace [#%d to #%d] on the already "
			     "created object %s", dataspace->getId(),
			     _dataspace->getId(), oid.toString());
  

    dataspace = _dataspace;
    dspid = dataspace->getId();
    return Success;
  }

  Status
  Object::move(const Dataspace *_dataspace)
  {
    OidArray oid_arr(&oid, 1);
    return db->moveObjects(oid_arr, _dataspace);
    /*
      return Exception::make(IDB_ERROR, "Object::move() is "
      "not yet implemented");
    */
  }

  short
  Object::getDataspaceID() const
  {
    if (dataspace)
      return dataspace->getId();

    /*
      if (dspid != Dataspace::DefaultDspid)
      return dspid;

      if (cls) {
      const Dataspace *tmp = 0;
      Status s = cls->getDefaultInstanceDataspace(tmp);
      if (s) throw *s;
      if (tmp)
      const_cast<Object *>(this)->dspid = tmp->getId();
      }
    */

    return dspid;
  }

  void
  Object::setDspid(short _dspid)
  {
    dspid = _dspid;
  }

  void *
  Object::setUserData(const char *s, void *x)
  {
    void *r = getUserData(s);

    if (!user_data_ht)
      user_data_ht = new UserDataHT();

    user_data_ht->insert(s, x);
    return r;
  }

  void *
  Object::getUserData(const char *s)
  {
    if (!user_data_ht)
      return (void *)0;

    return user_data_ht->get(s);
  }

  const void *
  Object::getUserData(const char *s) const
  {
    if (!user_data_ht)
      return (void *)0;

    return user_data_ht->get(s);
  }

  void
  Object::getAllUserData(LinkedList *&key_list, LinkedList *&data_list) const
  {
    if (!user_data_ht) {
      key_list = new LinkedList();
      data_list = new LinkedList();
      return;
    }

    user_data_ht->getall(key_list, data_list);
  }

  gbxBool
  Object::grant_release()
  {
    //return master_object ? gbxFalse : gbxTrue;
    return gbxTrue;
  }

  void
  Object::setDamaged(const Attribute *_damaged_attr)
  {
    damaged_attr = _damaged_attr;
    //printf("setting damaged %s to %p\n", damaged_attr->getName(), this);
  }

  Bool
  Object::setReleaseCycleDetection(Bool x)
  {
    Bool o = release_cycle_detection;
    release_cycle_detection = x;
    return o;
  }

  ObjectReleaser::ObjectReleaser(Object *_o)
  {
    o = _o;
    dont_release = False;
  }

  Object *
  ObjectReleaser::dontRelease()
  {
    dont_release = True;
    return o;
  }

  ObjectReleaser::~ObjectReleaser()
  {
    if (!dont_release)
      o->release();
  }

  ObjectListReleaser::ObjectListReleaser()
  {
    dont_release = False;
  }

  void
  ObjectListReleaser::add(Object *o)
  {
    list.insertObject(o);
  }

  void
  ObjectListReleaser::dontRelease()
  {
    dont_release = True;
  }

  ObjectListReleaser::~ObjectListReleaser()
  {
    if (!dont_release) {
      LinkedListCursor c(list);
      Object *o;
      while (c.getNext((void *&)o))
	o->release();
    }
  }

  ObjectObserver::ObjectObserver(const std::string &tag) :
    gbxObserver(tag)
  {
  }

  void ObjectObserver::addObj(gbxObject *o)
  {
    if (o->getPTag() == getObjectTag()) {
      gbxObserver::addObj(o);
    }
  }

  void ObjectObserver::rmvObj(gbxObject *_o)
  {
    /*
    Object *o = dynamic_cast<Object *>(_o);
    if (o) {
    */
    if (_o->getPTag() == getObjectTag()) {
      gbxObserver::rmvObj(_o);
    }
  }

  ObjectObserver::~ObjectObserver() {
  }

  void ObjectObserver::getObjects(std::vector<Object *> &ov) const
  {
    std::map<gbxObject *, bool>::const_iterator begin = obj_map->begin();
    std::map<gbxObject *, bool>::const_iterator end = obj_map->end();

    ov.erase(ov.begin(), ov.end());
    while (begin != end) {
      gbxObject *o = (*begin).first;
      if (o->getPTag() != getObjectTag()) {
	//if (!o) {
	std::cerr << "eyedb::Observer error: " << (void *)(*begin).first <<
	  " is not an eyedb::Object\n";
      }
      else
	ov.push_back((eyedb::Object *)o);
      ++begin;
    }
  }

  bool ObjectObserver::isObjectRegistered(Object *o) const
  {
    return gbxObserver::isObjectRegistered(o);
  }

}
