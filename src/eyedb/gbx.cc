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


#include <stdio.h>
#include <string.h>
#include <time.h>
#include "eyedb/gbx.h"
#include "eyedb/gbxcyctx.h"
#define _eyedb_se_P_
#include <eyedblib/log.h>
#include <assert.h>
#include <eyedb/linklist.h>
#include <iostream>

#include <typeinfo>

//#include <execinfo.h>

//#define GBX_TRACE
//#define GBX_TRACE2

namespace eyedb {

    /*
      in gbx.h:
    struct lessObject {
      bool operator() (const gbxObject *x, const gbxObject *y) const;
    };

    typedef std::map<gbxObject *, bool, lessObject> Map;
    typedef std::map<gbxObject *, bool, lessObject>::iterator MapIterator;
    */


  /*
  bool gbxObject::lessObject::operator()(const gbxObject *x,
					 const gbxObject *y) const
  {
    return x < y;
  }
  */

  enum gbxObjState {
    ToDelete = 1,
    Deleted,
    Keep
  };

  struct gbxRegObj {
    gbxObjState state;
    gbxObject *o;
  };

  static gbxBool object_isonstack = gbxTrue;
  static int object_size;

  int gbxObject::obj_cnt;
  int gbxObject::heap_size;
  gbxObject::Map *gbxObject::obj_map;

#define ValidObject   (unsigned int)0x76fe12f1
#define DeletedObject (unsigned int)0x1547eef3

  gbxObject::gbxObject()
  {
    init("");
  }

  gbxObject::gbxObject(const gbxTag &_tag)
  {
    init("");
    gbx_tag = new gbxTag(_tag);
  }

  gbxObject::gbxObject(const std::string &ptag)
  {
    init(ptag);
  }

  void gbxObject::init(const std::string &ptag)
  {
    gbx_ptag = ptag;
    gbx_magic     = ValidObject;
    gbx_refcnt    = 1;
    gbx_must_release = true;
    gbx_locked    = gbxFalse;
    gbx_isonstack = object_isonstack;
    gbx_on_release = 0;

    gbx_size      = object_size;
    gbx_tag       = 0;
    gbx_activeDestruction = gbxFalse;
    gbx_chgRefCnt = gbxFalse;

    object_isonstack = gbxTrue;
    object_size = 0;

    if (!gbx_isonstack)
      gbxAutoGarb::addObject(this);

    obj_cnt++;
    gbxObserver::addObject(this);

#if 0
    void * array[100];
    int nSize = backtrace(array, 100);
    char ** symbols = backtrace_symbols(array, nSize);
 
    std::cout << "gbxObject called from:\n";
    for (int n = 0; n < nSize; n++) {
      std::cout << symbols[n] << '\n';
    }
    free(symbols);
#endif


    if (obj_map) {
      if (obj_map->find(this) != obj_map->end())
	std::cerr << "gbxObject::init: " << this << " already in map" << std::endl;

      (*obj_map)[this] = true;
    }

    heap_size += gbx_size;
    IDB_LOG(IDB_LOG_OBJ_GBX,
	    ("gbxObject::gbxObject(o=%p, isonstack=%s, refcnt=1)\n",
	     this, gbx_isonstack ? "true" : "false"));
#ifdef GBX_TRACE
    printf("gbxObject::gbxObject(%p, refcnt=1, stack=%d)\n",
	   this, gbx_isonstack);
#endif
  }

  gbxBool gbxObject::isValidObject() const
  {
    return (gbx_magic == ValidObject ? gbxTrue : gbxFalse);
  }

  gbxObject::gbxObject(const gbxObject &o)
  {
    init(o.gbx_ptag);

    gbx_locked = o.gbx_locked;
    gbx_tag = o.gbx_tag ? new gbxTag(*o.gbx_tag) : 0;
    gbx_must_release = o.gbx_must_release; // ??
  }

  gbxObject::gbxObject(const gbxObject *o)
  {
    init(o ? o->gbx_ptag : std::string(""));

    if (o) {
      gbx_locked  = o->gbx_locked;
      gbx_tag = o->gbx_tag ? new gbxTag(*o->gbx_tag) : 0;
      gbx_must_release = o->gbx_must_release; // ??
    }
  }

  gbxObject &gbxObject::operator=(const gbxObject &o)
  {
    if (&o == this)
      return *this;

    garbageRealize(gbxFalse, gbxFalse);

    gbx_magic = ValidObject;
    gbx_refcnt = 1;
    gbx_locked = o.gbx_locked;
    gbx_tag = o.gbx_tag ? new gbxTag(*o.gbx_tag) : 0;
    gbx_must_release = o.gbx_must_release; // ??
    gbx_on_release = 0;

    gbx_activeDestruction = gbxFalse;

    return *this;
  }

  void *gbxObject::operator new(size_t size)
  {
    object_isonstack = gbxFalse;
    object_size = size;
    return ::operator new(size);
  }

  static bool nodelete = getenv("EYEDB_NODELETE") ? true : false;
  static bool noabort_on_delete = getenv("EYEDB_NOABORT_ON_DELETE") ? true : false;

  void gbxObject::operator delete(void *o)
  {
    if (!o)
      return;

    fprintf(stderr, "gbxObject::delete: tries to use operator delete on an gbxObject * [%p]\n", o);
    fprintf(stderr, "use gbxObject::release method instead\n");
    if (!noabort_on_delete)
      abort();
  }

  void gbxObject::keep()
  {
    gbxAutoGarb::keepObject(this, gbxTrue);
  }

  void gbxObject::unkeep()
  {
    gbxAutoGarb::keepObject(this, gbxFalse);
  }

  void gbxObject::release_realize(gbxBool reentrant)
  {
    if (gbx_activeDestruction)
      return;
    garbageRealize(reentrant, gbxTrue);
    gbx_activeDestruction = gbxFalse;

    /*
    if (!gbx_refcnt && !gbx_locked) {
      obj_cnt--;
      assert(gbx_magic == DeletedObject);
      gbxObserver::rmvObject(this);
    }
    */

    if (!nodelete)
      if (!gbx_isonstack && !gbx_refcnt && !gbx_locked)
	delete ((void *)this);
  }

  void gbxObject::release()
  {
    if (!grant_release()) {
      fprintf(stderr, "gbxObject::release error, object %p is not granted to be released refcnt = %d\n", this, gbx_refcnt);
      abort();
    }
    release_realize(gbxFalse);
  }

  void gbxObject::release_r()
  {
    release_realize(gbxTrue);
  }

  void
  gbxObject::garbageRealize(gbxBool reentrant, gbxBool remove)
  {
#if 0
    if (gbx_isonstack) {
      std::cerr << "---> onstack " << (void *)this << ": " <<
	gbx_refcnt << " " << gbx_locked << " " << reentrant << " " << remove
	       << std::endl;
    }
#endif

    if (gbx_activeDestruction)
      return;

    /*
      if (gbxAutoGarb::isObjectDeleted(this))
      return;
    */

    gbx_activeDestruction = gbxTrue;

#ifdef GBX_TRACE
    printf("gbxObject::garbageRealize(%p, refcnt=%d, locked=%d, stack=%d, "
	   "reentrant=%d)\n",
	   this, gbx_refcnt, gbx_locked, gbx_isonstack, reentrant);
#endif

    if ((reentrant && gbx_refcnt < 0) || (!reentrant && gbx_refcnt < 1)) {
#if 1
	fprintf(stderr, "gbxObject::delete warning, tries to delete a null ref count object `%p', refcnt = %d\n", this, gbx_refcnt);
      gbx_refcnt = 1;
#else
      fprintf(stderr, "gbxObject::delete error, tries to delete a null ref count object `%p', refcnt = %d\n", this, gbx_refcnt);
      abort();
#endif
    }

    if (gbx_magic == DeletedObject) {
      fprintf(stderr, "gbxObject::delete: error, tries to delete an object already deleted `%p'\n", this);
      abort();
      return;
    }

    if (gbx_magic != ValidObject) {
      fprintf(stderr, "gbxObject::delete: try to delete an invalid object `%p'\n", this);
      abort();
      return;
    }

    // added 30/09/05
    if (gbx_locked)
      return;

    gbxObject::decrRefCount();

    IDB_LOG(IDB_LOG_OBJ_GBX,
	    ("gbxObject::garbageRealize(o=%p, refcnt=%d, locked=%d)\n",
	     this, gbx_refcnt, gbx_locked));

    //if (!reentrant && gbx_refcnt == 1) {// added the 18/10/99: '&& gbx_refcnt==1'
    if (!reentrant) {// EV_CYCLE: suppress the 23/11/06: '&& gbx_refcnt==1' for testing
#ifdef MANAGE_CYCLE_TRACE
      struct timeval tp0, tp1;
      gettimeofday(&tp0, NULL);
      printf("gbxObject begin detect cycle for %p\n", this);
#endif

      gbxCycleContext r(this);
      manageCycle(r);

#ifdef MANAGE_CYCLE_TRACE
      gettimeofday(&tp1, NULL);

      int ms = ((tp1.tv_sec - tp0.tv_sec) * 1000) + ((tp1.tv_usec - tp0.tv_usec)/1000);
      printf("gbxObject end detect cycle for %p [cycle=%d] [%d ms]\n", this,
	     r.isCycle(), ms);
#endif

      if (r.isCycle()) {
	gbxObject::decrRefCount();
      }
    }

#if 0
    if (gbx_isonstack) {
      std::cerr << "onstack " << (void *)this << ": " <<
	gbx_refcnt << " " << gbx_locked << std::endl;
    }
#endif

    if (!gbx_refcnt && !gbx_locked) {
      if (!gbx_isonstack)
	gbxAutoGarb::markObjectDeleted(this);
#ifdef GBX_TRACE
      printf("gbxObject garbaging 0x%p\n", this);
#endif
      IDB_LOG(IDB_LOG_OBJ_GBX,
	      ("gbxObject::garbageRealize(o=%p) calling virtual garbage\n",
	       this));

      if (obj_map) {
	MapIterator iter = obj_map->find(this);

	if (iter != obj_map->end())
	  obj_map->erase(iter);
	else {
	  /*
	    std::cerr << "gbxObject::garbage: " << this << " not found";
	    if (gbx_tag) {
	    if (gbx_tag->getSTag())
	    std::cerr << " [" << gbx_tag->getSTag() << "]";
	    else if (gbx_tag->getITag())
	    std::cerr << " [" << gbx_tag->getITag() << "]";
	    std::cerr << std::endl;
	    }
	  */
	}
      }

      if (remove) {
	obj_cnt--;
	gbxObserver::rmvObject(this);
      }

      if (gbx_on_release) {
	//printf("GBX_ON_RELEASE %p\n", gbx_on_release);
	gbx_on_release->perform(this);
      }

      userGarbage();
      garbage();
      delete gbx_tag;
      gbx_tag = 0;
      gbx_magic = DeletedObject;

      heap_size -= gbx_size; 
   }
  }

  void gbxObject::setObjMapped(bool obj_mapped, bool reinit_if_exists)
  {
    if (obj_mapped) {
      if (obj_map) {
	if (!reinit_if_exists)
	  return;
      }

      delete obj_map;
      obj_map = new Map();
      return;
    }

    delete obj_map;
    obj_map = 0;
  }

  void gbxObject::setTag(const gbxTag &_tag)
  {
    delete gbx_tag;
    gbx_tag = new gbxTag(_tag);
  }

  void
  gbxObject::incrRefCount()
  {
    IDB_LOG(IDB_LOG_OBJ_GBX, ("gbxObject::incrRefCount(o=%p, refcnt=%d -> %d)\n", this, gbx_refcnt, gbx_refcnt+1));

    if (!isValidObject()) {
      fprintf(stderr, "gbxObject::incrRefCount: try to increment reference count on an invalid object `%p'\n", this);
      abort();
    }

    if (!gbx_chgRefCnt)
      gbx_refcnt++;
    /*
    else
      printf("should increment refcnt %p\n", this);
    */
  }

  void
  gbxObject::reserve()
  {
    incrRefCount();
  }

  void
  gbxObject::decrRefCount()
  {
    IDB_LOG(IDB_LOG_OBJ_GBX, ("gbxObject::decrRefCount(o=%p, refcnt=%d -> %d)\n", this, gbx_refcnt, gbx_refcnt-1));

    if (!isValidObject()) {
      fprintf(stderr, "gbxObject::incrRefCount: try to increment reference count on an invalid object `%p'\n", this);
      abort();
    }

    if (!gbx_chgRefCnt)
      gbx_refcnt--;
    /*
    else
      printf("should decrement refcnt %p\n", this);
    */
    assert(gbx_refcnt >= 0);
  }

  void
  gbxObject::manageCycle(gbxCycleContext &)
  {
  }

  gbxObject::~gbxObject()
  {
    garbageRealize(gbxFalse, gbxTrue);
    gbx_activeDestruction = gbxFalse;
  }

  //
  // gbxCycleContext
  //

  gbxBool gbxCycleContext::mustClean(gbxObject *_ref)
  {
    //if (this->ref == _ref)
      //printf("mustClean: ref %p refcnt=%d\n", ref, ref->getRefCount());

    //    if (this->ref == _ref && this->ref->getRefCount() == 1)
    // EV_CYCLE
    if (this->ref == _ref)
      return gbxTrue;
    return gbxFalse;
  }

  void gbxCycleContext::manageCycle(gbxObject *_ref)
  {
    if (this->ref == _ref) {
      //printf("manage cycle %p\n", ref);
      cycle = gbxTrue;
    }
  }

  //
  // gbxTag
  //

  void gbxTag::init()
  {
    stag = 0;
    vtag = 0;
    itag = 0;
  }

  gbxTag::gbxTag()
  {
    init();
  }

  gbxTag::gbxTag(const gbxTag &tag)
  {
    init();
    *this = tag;
  }

  gbxTag::gbxTag(const char *_stag)
  {
    init();
    stag = _stag ? strdup(_stag) : 0;
  }

  gbxTag::gbxTag(int _itag)
  {
    init();
    itag = _itag;
  }

  gbxTag::gbxTag(void *_vtag)
  {
    init();
    vtag = _vtag;
  }

  gbxTag &gbxTag::operator=(const gbxTag &tag)
  {
    free(stag);

    stag = tag.stag ? strdup(tag.stag) : 0;
    itag = tag.itag;
    vtag = tag.vtag;

    return *this;
  }

  int gbxTag::operator==(const gbxTag &tag) const
  {
    return (itag == tag.itag && vtag == tag.vtag && !strcmp(stag, tag.stag));
  }

  int gbxTag::operator!=(const gbxTag &tag) const
  {
    return !this->operator==(tag);
  }

  gbxTag::~gbxTag()
  {
    free(stag);
  }

  //
  // gbxDeleter
  //

  //
  // added the 18/02/99: WARNING this does not work with the gbxAutoGarb!
  //

  gbxDeleter::gbxDeleter(gbxObject *_o) : o(_o), _keep(gbxFalse)
  {
  }

  gbxObject *
  gbxDeleter::keep()
  {
    _keep = gbxTrue;
    return o;
  }

  gbxDeleter::~gbxDeleter()
  {
    if (!_keep && o)
      {
	o->release();
	return;
      }
  }

  //
  // gbxAutoGarb
  //

  static gbxBool
  isPower2(unsigned int x)
  {
    int n = 0;
    while(x) {
      if ((x & 1) && ++n > 1)
	return gbxFalse;

      x >>= 1;
    }

    return gbxTrue;
  }

  gbxAutoGarb *gbxAutoGarb::current_auto_garb = 0;

  void gbxAutoGarb::init(int _list_cnt)
  {
#ifdef GBX_TRACE
    //printf("gbxAutoGarb::init();\n");
#endif
    excepted = gbxFalse;
    keepobjs = gbxFalse;
    regobjs_cnt = 0;
    if (!isPower2(_list_cnt))
      throw "gbxAutoGarb::init() power of 2 expected";
    list_cnt = _list_cnt;
    mask = list_cnt - 1;
    todelete_lists = new LinkedList*[list_cnt];
    memset(todelete_lists, 0, sizeof(LinkedList*)*list_cnt);
    deleted_lists = new LinkedList*[list_cnt];
    memset(deleted_lists, 0, sizeof(LinkedList*)*list_cnt);
    deleted_cnt = todelete_cnt = 0;

    prev = current_auto_garb;
    current_auto_garb = this;
    tag  = 0;
    type = (gbxAutoGarb::Type)0;
    deleg_auto_garb = 0;
  }

  gbxAutoGarb::gbxAutoGarb(int _list_cnt)
  {
    init(_list_cnt);
  }

  gbxAutoGarb::gbxAutoGarb(gbxAutoGarb *auto_garb)
  {
    deleg_auto_garb = auto_garb;
    prev = current_auto_garb;
    current_auto_garb = deleg_auto_garb;
  }

  gbxAutoGarb::gbxAutoGarb(gbxAutoGarb::Type _type, int _list_cnt)
  {
    init(_list_cnt);
    type = _type;
  }

  gbxAutoGarb::gbxAutoGarb(const gbxTag &_tag, gbxBool _excepted, int _list_cnt)
  {
    init(_list_cnt);
    tag = new gbxTag(_tag);
    excepted = _excepted;
  }

  gbxAutoGarb *gbxAutoGarb::getAutoGarb()
  {
    return (deleg_auto_garb ? deleg_auto_garb : this);
  }

  void gbxAutoGarb::keepObjs()
  {
    getAutoGarb()->keepobjs = gbxTrue;
  }

  static int true_obj;

  inline unsigned int
  gbxAutoGarb::get_key(gbxObject *o)
  {
    return ((((long)o)>>4) & mask);
  }

  gbxRegObj *
  gbxAutoGarb::find(gbxObject *o, LinkedList **lists)
  {
    unsigned int key = get_key(o);
    if (!lists[key])
      return 0;

    LinkedListCursor c(lists[key]);
    gbxRegObj *reg;
    while(c.getNext((void *&)reg)) 
      if (reg->o == o)
	return reg;

    return 0;
  }

  void gbxAutoGarb::addObj(gbxObject *o)
  {
    if (type == SUSPEND) {
#ifdef GBX_TRACE
      printf("gbxAutoGarb::addObject(%p) AUTO_GARB IS SUSPENDED\n", o);
#endif
      return;
    }

#ifdef GBX_TRACE
    printf("gbxAutoGarb::addObject(%p)\n", o);
#endif

    unsigned int key = get_key(o);
    if (!todelete_lists[key])
      todelete_lists[key] = new LinkedList();
    gbxRegObj *reg = new gbxRegObj();
    reg->state = ToDelete;
    reg->o = o;
    todelete_lists[key]->insertObject(reg);
    regobjs_cnt++;
    todelete_cnt++;
    true_obj = regobjs_cnt;
  }

  gbxBool gbxAutoGarb::keepObj(gbxObject *o, gbxBool keep)
  {
#ifdef GBX_TRACE
    printf("keepObj(%p, o = %p, regobjs_cnt = %d, true_obj = %d)\n",
	   this, o, regobjs_cnt, true_obj);
#endif

    gbxRegObj *reg = find(o, todelete_lists);
    if (reg) {
      if (keep) {
	if (reg->state == ToDelete) {
	  reg->state = Keep;
	  true_obj--;
	  todelete_cnt--;
	}
      }
      else if (reg->state == Keep) {
	reg->state = ToDelete;
	true_obj++;
	todelete_cnt++;
      }
#ifdef GBX_TRACE
      printf("%p -> found\n", o);
#endif
      return gbxTrue;
    }

    return gbxFalse;
  }

  gbxBool gbxAutoGarb::markObjDeleted(gbxObject *o)
  {
#ifdef GBX_TRACE
    printf("gbxAutoGarb::markObjDeleted(%p)\n", o);
#endif
    gbxRegObj *reg = find(o, todelete_lists);
    if (!reg) {
#ifdef GBX_TRACE
      printf(" -> *not* found\n");
#endif
      return gbxFalse;
    }

    unsigned int key = get_key(o);
    todelete_lists[key]->deleteObject(reg);
    if (reg->state == ToDelete)
      todelete_cnt--;

    if (!find(o, deleted_lists)) {
      reg->state = Deleted;
      if (!deleted_lists[key])
	deleted_lists[key] = new LinkedList();
      deleted_lists[key]->insertObject(reg);
      deleted_cnt++;
    }
    else
      delete reg;
#ifdef GBX_TRACE
    printf(" -> found\n");
#endif
    return gbxTrue;
  }

  gbxBool gbxAutoGarb::isObjRegistered(gbxObject *o)
  {
    return find(o, todelete_lists) || find(o, deleted_lists) ? gbxTrue : gbxFalse;
  }

  gbxBool gbxAutoGarb::isObjDeleted(gbxObject *o)
  {
    return find(o, deleted_lists) ? gbxTrue : gbxFalse;
  }

  gbxBool gbxAutoGarb::isObjectRegistered(gbxObject *o)
  {
    return current_auto_garb ? current_auto_garb->isObjRegistered(o) : gbxFalse;
  }

  // 7/05/01: added this flag because this method is called from several
  // points in eyedb and this considerally slow down the system !
  // is this disconnection a good idea !?
  //#define GBX_IS_OBJ_DELETED_DISCONNECTED
  gbxBool gbxAutoGarb::isObjectDeleted(gbxObject *o)
  {
#ifdef GBX_IS_OBJ_DELETED_DISCONNECTED
    return gbxFalse;
#else
    gbxBool r =  current_auto_garb ? current_auto_garb->isObjDeleted(o) : gbxFalse;
#ifdef GBX_TRACE
    if (r)
      printf("gbxAutoGarb::isObjectDeleted: object %p was deleted\n", o);
#endif
    return r;
#endif
  }

  void gbxAutoGarb::addObject(gbxObject *o)
  {
    if (current_auto_garb)
      current_auto_garb->getAutoGarb()->addObj(o);
  }

  void gbxAutoGarb::keepObject(gbxObject *o, gbxBool keep)
  {
    for (gbxAutoGarb *auto_garb = current_auto_garb; auto_garb; auto_garb = auto_garb->prev)
      if (auto_garb->getAutoGarb()->keepObj(o, keep))
	break;
  }

  void gbxAutoGarb::markObjectDeleted(gbxObject *o)
  {
    for (gbxAutoGarb *auto_garb = current_auto_garb; auto_garb; auto_garb = auto_garb->prev)
      if (auto_garb->getAutoGarb()->markObjDeleted(o))
	break;
  }

  gbxAutoGarb::Type
  gbxAutoGarb::suspend()
  {
    return setType(SUSPEND);
  }

  void
  gbxAutoGarb::restore(gbxAutoGarb::Type t)
  {
    (void)setType(t);
  }

  gbxAutoGarb::Type
  gbxAutoGarb::setType(gbxAutoGarb::Type t)
  {
    Type ot = type;
    type = t;
    return ot;
  }

  gbxAutoGarb::Type
  gbxAutoGarb::getType()
  {
    return type;
  }

  void gbxAutoGarb::wipeLists(LinkedList **lists)
  {
    for (int i = 0; i < list_cnt; i++) {
      LinkedList *list = lists[i];
      if (!list) continue;
      LinkedListCursor c(list);
      gbxRegObj *reg;
      while(c.getNext((void *&)reg))
	delete reg;
      delete list;
    }

    delete [] lists;
  }

  unsigned int gbxAutoGarb::countLists(LinkedList **lists, int state)
  {
    unsigned int cnt = 0;
    for (int i = 0; i < list_cnt; i++) {
      LinkedList *list = lists[i];
      if (!list) continue;
      LinkedListCursor c(list);
      gbxRegObj *reg;
      while(c.getNext((void *&)reg)) {
	if (!state || reg->state == state)
	  cnt++;
      }
    }
    return cnt;
  }

  void displayLists(LinkedList **lists, int list_cnt, const char *msg)
  {
    printf(msg);
    for (int i = 0; i < list_cnt; i++) {
      LinkedList *list = lists[i];
      if (!list) continue;
      printf("#%d : %d\n", i, list->getCount());
    }
  }

  void gbxAutoGarb::garbage()
  {
#ifdef GBX_TRACE2
    printf("BEGIN ~gbxAutoGarb(%p, %d, %d, todelete_cnt=%d, deleted_cnt=%d)\n", this, regobjs_cnt, true_obj,
	   todelete_cnt, deleted_cnt);
    struct timeval tp0, tp1;
    gettimeofday(&tp0, NULL);
#endif

    if (!keepobjs) {
      //displayLists(todelete_lists, list_cnt, "ToDeleteList:\n");
      //displayLists(deleted_lists, list_cnt, "DeletedList:\n");
      gbxRegObj **regs = (gbxRegObj **)malloc(todelete_cnt * sizeof(gbxRegObj*));

      int n = 0;
      for (int i = 0; i < list_cnt; i++) {
	LinkedList *list = todelete_lists[i];
	if (!list) continue;
	LinkedListCursor c(list);
	gbxRegObj *reg;
	while(c.getNext((void *&)reg)) {
	  if (reg->state == ToDelete)
	    regs[n++] = reg;
	}
      }

#ifdef GBX_TRACE2
      if (n != todelete_cnt)
	printf("ToDelete %d vs. %d\n", todelete_cnt, n);
#endif

      assert(n == todelete_cnt);
      { // A.W. adding block because of IRIX cc complain on i scope
	for (int i = 0; i < n; i++) {
	  gbxRegObj *reg = regs[i];
	  gbxObject *o = reg->o;
#ifdef GBX_TRACE
	  printf("o=%p, state=%d\n", o, reg->state);
#endif
	  if (!o || reg->state != ToDelete) {
#ifdef GBX_TRACE
	    printf("not to delete reg->o %p %d\n", o, reg->state);
#endif
	    continue;
	  }
	
	  IDB_LOG(IDB_LOG_OBJ_GARBAGE, 
		  ("~gbxAutoGarb(o=%p, refcnt=%d) => ",
		   o, reg->o->getRefCount()));
      
	  if (o->getRefCount()) {
#ifdef GBX_TRACE
	    printf("gbxAutoGarb--garbage(%p)\n", o);
#endif
	    o->release();
	  }
	  else
	    IDB_LOG_X(IDB_LOG_OBJ_GARBAGE, ("not "));
      
	  IDB_LOG_X(IDB_LOG_OBJ_GARBAGE, ("releasing\n"));
	}
      }

      free(regs);
    }

#ifdef GBX_TRACE2
    printf("Object Count : %d\n", regobjs_cnt);
    printf("  ToDelete : %d vs. %d [%d]\n", countLists(todelete_lists, ToDelete),
	   todelete_cnt, countLists(todelete_lists, 0));
    printf("  Keep : %d+%d\n", countLists(todelete_lists, Keep),
	   countLists(deleted_lists, Keep));
    printf("  Deleted : %d vs. %d [%d]\n", countLists(deleted_lists, Deleted),
	   deleted_cnt, countLists(deleted_lists, 0));
#endif

#ifdef GBX_TRACE2
    fflush(stdout);
#endif

    wipeLists(todelete_lists);
    wipeLists(deleted_lists);

    current_auto_garb = prev;
    delete tag;

    regobjs_cnt = 0;
  }

  gbxAutoGarb::~gbxAutoGarb()
  {
    getAutoGarb()->garbage();
  }

  gbxAutoGarbSuspender::gbxAutoGarbSuspender()
  {
    current = gbxAutoGarb::getCurrentAutoGarb();
    if (current)
      type = current->suspend();
  }

  gbxAutoGarbSuspender::~gbxAutoGarbSuspender()
  {
    if (current)
      current->restore(type);
  }

  gbxObserver *gbxObserver::current_observer = 0;

  gbxObserver::gbxObserver(const std::string &tag) : tag(tag)
  {
    prev = current_observer;
    current_observer = this;
    addobj_trigger = 0;
    rmvobj_trigger = 0;
    obj_map = new std::map<gbxObject *, bool>();
  }

  gbxObserver::~gbxObserver()
  {
    delete obj_map;
    current_observer = prev;
  }

  void gbxObserver::addObject(gbxObject *o)
  {
    if (getCurrentObserver())
      getCurrentObserver()->addObj(o);
  }

  void gbxObserver::rmvObject(gbxObject *o)
  {
    if (getCurrentObserver())
      getCurrentObserver()->rmvObj(o);
  }

  void gbxObserver::addObj(gbxObject *o)
  {
    assert(!isObjectRegistered(o));
    (*obj_map)[o] = true;
    if (addobj_trigger)
      (*addobj_trigger)(o);
  }

  void gbxObserver::rmvObj(gbxObject *o)
  {
    assert(isObjectRegistered(o));
    if (isObjectRegistered(o))
      obj_map->erase(obj_map->find(o));
    if (rmvobj_trigger)
      (*rmvobj_trigger)(o);
  }

  bool gbxObserver::isObjectRegistered(gbxObject *o) const
  {
    return obj_map->find(o) != obj_map->end();
  }

  void gbxObserver::getObjects(std::vector<gbxObject *> &v) const
  {
    v.erase(v.begin(), v.end());
    std::map<gbxObject *, bool>::const_iterator begin = obj_map->begin();
    std::map<gbxObject *, bool>::const_iterator end = obj_map->end();

    while (begin != end) {
      v.push_back((*begin).first);
      ++begin;
    }
  }

  size_t gbxObserver::getObjectCount() const
  {
    return obj_map->size();
  }

  void gbxObserver::setAddObjectTrigger(gbxObserver::AddObjectTrigger *_addobj_trigger)
  {
    addobj_trigger = _addobj_trigger;
  }

  void gbxObserver::setRemoveObjectTrigger(gbxObserver::RemoveObjectTrigger *_rmvobj_trigger)
  {
    rmvobj_trigger = _rmvobj_trigger;
  }

  gbxObserver::ObjectTrigger::~ObjectTrigger()
  {
  }

  gbxObserver::AddObjectTrigger::~AddObjectTrigger()
  {
  }

  gbxObserver::RemoveObjectTrigger::~RemoveObjectTrigger()
  {
  }
}
