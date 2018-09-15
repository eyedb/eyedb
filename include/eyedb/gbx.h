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


#ifndef _EYEDB_GBX_H
#define _EYEDB_GBX_H

#include <stdlib.h>
#include <map>
#include <vector>
#include <string>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class LinkedList;

  class gbxTag;
  class gbxCycleContext;

  enum gbxBool {
    gbxFalse = 0,
    gbxTrue  = 1
  };

  /**
     Not yet documented.
  */
  class gbxObject {

    // ----------------------------------------------------------------------
    // gbxObject Interface
    // ----------------------------------------------------------------------
  public:
    gbxObject();
    gbxObject(const std::string &ptag);
    gbxObject(const gbxTag &tag);
    gbxObject(const gbxObject &);
    gbxObject(const gbxObject *);

    gbxObject &operator=(const gbxObject &);

    static void *operator new(size_t);
    static void operator delete(void *);

    int getRefCount() const  {return gbx_refcnt;}
    gbxBool isLocked() const {return gbx_locked;}

    bool mustRelease() const {return gbx_must_release;}

    virtual void incrRefCount();
    virtual void decrRefCount();

    virtual void lock()   {gbx_locked = gbxTrue;}
    virtual void unlock() {gbx_locked = gbxFalse;}

    gbxBool isOnStack() const {return gbx_isonstack;}

    void reserve();

    void release();

    void setTag(const gbxTag &);
    const gbxTag *getTag() const {return gbx_tag;}
    const std::string &getPTag() const {return gbx_ptag;}

    virtual void userGarbage() {}

    void keep();
    void unkeep();

    static int getObjectCount() {return obj_cnt;}
    static int getHeapSize()    {return heap_size;}

    gbxBool isValidObject() const;

    typedef std::map<gbxObject *, bool> Map;
    typedef std::map<gbxObject *, bool>::iterator MapIterator;

    static void setObjMapped(bool obj_mapped, bool reinit_if_exists);
    static bool isObjMapped();
    static gbxObject::Map *getObjMap() {return obj_map;}

    struct OnRelease {
      virtual void perform(gbxObject *) = 0;
      virtual ~OnRelease() {}
    };

    void setOnRelease(OnRelease *on_release) {gbx_on_release = on_release;}
    OnRelease *getOnRelease() {return gbx_on_release;}

    virtual ~gbxObject();

    // ----------------------------------------------------------------------
    // gbxObject Protected Part
    // ----------------------------------------------------------------------
  protected:
    int gbx_refcnt;
    gbxBool gbx_locked;
    gbxBool gbx_isonstack;
    gbxTag *gbx_tag;
    std::string gbx_ptag;
    gbxBool gbx_chgRefCnt;
    bool gbx_must_release;

    void garbageRealize(gbxBool reentrant = gbxFalse, gbxBool remove = gbxTrue);
    virtual void garbage() {}
#ifdef GBX_NEW_CYCLE
    virtual void decrRefCountPropag() {}
#endif
    virtual gbxBool grant_release() {return gbxTrue;}

    // ----------------------------------------------------------------------
    // gbxObject Private Part
    // ----------------------------------------------------------------------
  private:
    void init(const std::string &ptag);
    unsigned int gbx_magic;
    gbxBool gbx_activeDestruction;
    static int obj_cnt;
    static int heap_size;
    static Map *obj_map;
    OnRelease *gbx_on_release;

    int gbx_size;
    void release_realize(gbxBool);

    // ----------------------------------------------------------------------
    // conceptually private
    // ----------------------------------------------------------------------
  public:
    virtual void manageCycle(gbxCycleContext &);
    void release_r();
    void setMustRelease(bool must_release) {gbx_must_release = must_release;}
    gbxBool isChgRefCnt() const {return gbx_chgRefCnt;}
    gbxBool setChgRefCnt(gbxBool chgRefCnt) {
      gbxBool old_chgRefCnt = gbx_chgRefCnt;
      gbx_chgRefCnt = chgRefCnt;
      return old_chgRefCnt;
    }
  };

  class gbxDeleter {

    // ----------------------------------------------------------------------
    // gbxDeleter Interface
    // ----------------------------------------------------------------------

  public:
    gbxDeleter(gbxObject *);

    gbxObject *keep();
    operator gbxObject *() {return o;}

    ~gbxDeleter();

  private:
    gbxBool _keep;
    gbxObject *o;
  };

  class gbxRegObj;

  class gbxAutoGarb {

    // ----------------------------------------------------------------------
    // gbxAutoGarb Interface
    // ----------------------------------------------------------------------
  public:
    enum Type {
      SUSPEND = 1,
      ACTIVE
    };

    static const int default_list_cnt = 512;
    gbxAutoGarb(int list_cnt = default_list_cnt);
    gbxAutoGarb(gbxAutoGarb::Type, int list_cnt = default_list_cnt);
    gbxAutoGarb(const gbxTag &, gbxBool excepted = gbxFalse, int list_cnt = default_list_cnt);
    gbxAutoGarb(gbxAutoGarb *);
    gbxAutoGarb(const gbxTag [], int cnt, gbxBool excepted = gbxFalse);

    gbxAutoGarb::Type suspend();
    void restore(gbxAutoGarb::Type);

    gbxAutoGarb::Type setType(gbxAutoGarb::Type);
    gbxAutoGarb::Type getType();

    void keepObjs();

    virtual ~gbxAutoGarb();

    void addObj(gbxObject *);
    gbxBool isObjRegistered(gbxObject *);
    gbxBool isObjDeleted(gbxObject *);
    gbxBool keepObj(gbxObject *, gbxBool);

    static void addObject(gbxObject *);
    static void keepObject(gbxObject *, gbxBool);
    static gbxBool isObjectRegistered(gbxObject *);
    static gbxBool isObjectDeleted(gbxObject *);
    static gbxAutoGarb *getCurrentAutoGarb() {return current_auto_garb;}

    // ----------------------------------------------------------------------
    // gbxAutoGarb Private Part
    // ----------------------------------------------------------------------
  private:
    gbxTag *tag;
    gbxBool excepted;
    gbxBool keepobjs;
    unsigned int regobjs_cnt;
    unsigned int list_cnt;
    unsigned int mask;
    LinkedList **todelete_lists;
    LinkedList **deleted_lists;
    unsigned int get_key(gbxObject *);
    gbxRegObj *find(gbxObject *o, LinkedList **);
    void wipeLists(LinkedList **);
    unsigned int countLists(LinkedList **, int state);
    unsigned int todelete_cnt;
    unsigned int deleted_cnt;
    void init(int);

    static gbxAutoGarb *current_auto_garb;
    gbxAutoGarb *prev;
    Type type;
    gbxAutoGarb *deleg_auto_garb;
    void garbage();
    gbxAutoGarb *getAutoGarb();

    // ----------------------------------------------------------------------
    // gbxAutoGarb Restrictive Part
    // ----------------------------------------------------------------------
  public: // conceptually implementation level
    static void markObjectDeleted(gbxObject *);
    gbxBool markObjDeleted(gbxObject *);
  };

  class gbxAutoGarbSuspender {

  public:
    gbxAutoGarbSuspender();
    ~gbxAutoGarbSuspender();

  private:
    gbxAutoGarb::Type type;
    gbxAutoGarb *current;
  };

  class gbxTag {

    // ----------------------------------------------------------------------
    // gbxTag Interface
    // ----------------------------------------------------------------------
  public:
    gbxTag();
    gbxTag(const gbxTag &);
    gbxTag(const char *stag);
    gbxTag(int itag);
    gbxTag(void *vtag);

    gbxTag &operator=(const gbxTag &);

    int operator==(const gbxTag &) const;
    int operator!=(const gbxTag &) const;

    const char * getSTag() const {return stag;}
    const int    getITag() const {return itag;}
    void *       getVTag() const {return vtag;}

    virtual ~gbxTag();

    // ----------------------------------------------------------------------
    // gbxTag Private Part
    // ----------------------------------------------------------------------
  private:
    char *stag;
    int  itag;
    void *vtag;
    void init();
  };

  class gbxObserver {

    // ----------------------------------------------------------------------
    // gbxObserver Interface
    // ----------------------------------------------------------------------
  public:
    typedef std::vector<gbxObject *> ObjectVector;
    typedef ObjectVector::iterator ObjectVectorIterator;
    typedef ObjectVector::const_iterator ObjectVectorConstIterator;

    gbxObserver(const std::string &tag = "");

    const std::string &getTag() const {return tag;}

    virtual size_t getObjectCount() const;

    virtual void getObjects(ObjectVector &) const;

    virtual bool isObjectRegistered(gbxObject *) const;

    class ObjectTrigger {

    public:
      virtual void operator()(gbxObject *o) = 0;
      virtual ~ObjectTrigger();
    };

    class AddObjectTrigger : public ObjectTrigger {

    public:
      virtual ~AddObjectTrigger();
    };

    class RemoveObjectTrigger : public ObjectTrigger {

    public:
      virtual ~RemoveObjectTrigger();
    };

    virtual void setAddObjectTrigger(AddObjectTrigger *trigger);
    virtual void setRemoveObjectTrigger(RemoveObjectTrigger *trigger);

    static gbxObserver *getCurrentObserver() {return current_observer;}

    virtual ~gbxObserver();

    // ----------------------------------------------------------------------
    // conceptually private
    // ----------------------------------------------------------------------
    static void addObject(gbxObject *o);
    static void rmvObject(gbxObject *o);

    virtual void addObj(gbxObject *o);
    virtual void rmvObj(gbxObject *o);

    // ----------------------------------------------------------------------
    // gbxObserver Protected Part
    // ----------------------------------------------------------------------
  protected:
    std::map<gbxObject *, bool> *obj_map;

    // ----------------------------------------------------------------------
    // gbxObserver Private Part
    // ----------------------------------------------------------------------
  private:
    std::string tag;
    gbxObserver *prev;
    AddObjectTrigger *addobj_trigger;
    RemoveObjectTrigger *rmvobj_trigger;
    static gbxObserver *current_observer;
  };

  class gbxObjectPtr {

  protected:
    gbxObject *o;

  public:
    gbxObjectPtr(gbxObject *o = 0) {
      this->o = o;
      if (o && !o->mustRelease())
	o->reserve();
    }

    gbxObjectPtr(const gbxObjectPtr &o_ptr) {
      o = 0;
      *this = o_ptr;
    }

    gbxObjectPtr& operator=(const gbxObjectPtr &o_ptr) {
      if (o)
	o->release();
      o = o_ptr.o;
      if (o)
	o->reserve();
      return *this;
    }

    gbxObject *getGBXObject() {return o;}
    const gbxObject *getGBXObject() const {return o;}

    gbxObject *operator->() {return o;}
    const gbxObject *operator->() const {return o;}

    bool operator!() const {return o == 0;}

    virtual ~gbxObjectPtr() {
      if (o)
	o->release();
      o = 0; // secure
    }
  };
  /**
     @}
  */
}

#endif
