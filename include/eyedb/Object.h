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


#ifndef _EYEDB_OBJECT_H
#define _EYEDB_OBJECT_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  //
  // forward declarations
  //

  class Database;
  class TypeModifier;
  class RecMode;
  class Attribute;
  class ObjectPeer;
  class Method;
  class ArgArray;
  class Argument;
  class Protection;
  class Instance;
  class Basic;
  class Int16;
  class Int32;
  class Int64;
  class Byte;
  class Char;
  class Float;
  class OidP;
  class Agregat;
  class Struct;
  class Union;
  class Enum;
  class Collection;
  class CollSet;
  class CollArray;
  class CollBag;
  class CollList;
  class Class;
  class AgregatClass;
  class StructClass;
  class UnionClass;
  class BasicClass;
  class BasicClass;
  class Int16Class;
  class Int32Class;
  class Int64Class;
  class ByteClass;
  class CharClass;
  class FloatClass;
  class OidClass;
  class CollectionClass;
  class CollSetClass;
  class CollBagClass;
  class CollArrayClass;
  class CollListClass;
  class EnumClass;
  class Schema;
  class AttrIdxContext;
  class Datafile;
  class Dataspace;
  class UserDataHT;

  /**
     Not yet documented.
  */
  class Object : public gbxObject {
  
    // ----------------------------------------------------------------------
    // Object Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param dataspace
    */
    Object(Database *db = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Object(const Object &o);

    /**
       Not yet documented
       @param o
       @param share
    */
    Object(const Object *o, Bool share = False);

    /**
       Not yet documented
       @param o
       @return
    */
    Object& operator=(const Object &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const = 0;

    /**
       Not yet documented
       @return
    */
    Class *getClass() const {return cls;}

    /**
       Not yet documented
       @param size
       @return
    */
    Data getIDR(Size& size) {
      size = idr->getSize();
      return idr->getIDR();
    }

    /**
       Not yet documented
       @return
    */
    Data getIDR() {
      return idr->getIDR();
    }

    /**
       Not yet documented
       @param size
       @return
    */
    const Data getIDR(Size& size) const {
      size = idr->getSize();
      return idr->getIDR();
    }

    /**
       Not yet documented
       @return
    */
    const Data getIDR() const {
      return idr->getIDR();
    }

    /**
       Not yet documented
       @return
    */
    Size getIDRSize() const {return idr->getSize();}

    /**
       Not yet documented
       @return
    */
    const Type get_Type() const {return type;}

    /**
       Not yet documented
       @return
    */
    const Oid& getOid() const {return oid;}

    /**
       Not yet documented
       @return
    */
    eyedblib::int64 getCTime() const {return c_time;}

    /**
       Not yet documented
       @return
    */
    eyedblib::int64 getMTime() const {return m_time;}

    /**
       Not yet documented
       @return
    */
    const char *getStringCTime() const;

    /**
       Not yet documented
       @return
    */
    const char *getStringMTime() const;

    /**
       Not yet documented
       @param lockmode
       @return
    */
    Status setLock(LockMode lockmode);

    /**
       Not yet documented
       @param lockmode
       @param alockmode
       @return
    */
    Status setLock(LockMode lockmode, LockMode &alockmode);

    /**
       Not yet documented
       @param lockmode
       @return
    */
    Status getLock(LockMode &lockmode);

    /**
       Not yet documented
       @param data
       @return
    */
    void *setUserData(void *data);

    /**
       Not yet documented
       @return
    */
    void *getUserData() {return user_data;}

    /**
       Not yet documented
       @return
    */
    const void *getUserData() const {return user_data;}

    /**
       Not yet documented
       @param key
       @param data
       @return
    */
    void *setUserData(const char *key, void *data);

    /**
       Not yet documented
       @param key
       @return
    */
    void *getUserData(const char *key);

    /**
       Not yet documented
       @param data
       @return
    */
    const void *getUserData(const char *data) const;

    /**
       Not yet documented
       @param key_list
       @param data_list
       @return
    */
    void getAllUserData(LinkedList *&key_list,
			LinkedList *&data_list) const;

    /**
       Not yet documented
       @return
    */
    Bool isModify() const {return modify;}

    /**
       Not yet documented
       @param mdb
       @return
    */
    virtual Status setDatabase(Database *mdb);

    /**
       Not yet documented
       @return
    */
    Database *getDatabase() const {return db;}

    /**
       Not yet documented
       @param recmode
       @return
    */
    virtual Status remove(const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param recmode
       @return
    */
    Status store(const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param db
       @param mth
       @param argarray
       @param retarg
       @param checkArgs
       @return
    */
    Status apply(Database *db, Method *mth, ArgArray &argarray,
		 Argument &retarg, Bool checkArgs);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    virtual Status trace(FILE *fd = stdout, unsigned int flags = 0,
			 const RecMode *recmode = RecMode::FullRecurs) const = 0;

    /**
       Not yet documented
       @param flags
       @param recmode
       @param pstatus
       @return
    */
    std::string toString(unsigned int flags = 0,
			 const RecMode *recmode = RecMode::FullRecurs,
			 Status *pstatus = 0) const;

    /**
       Not yet documented
       @param dataspace
       @param refetch
       @return
    */
    Status getDataspace(const Dataspace *&dataspace, Bool refetch = False) const;

    /**
       Not yet documented
       @param loc
       @return
    */
    Status getLocation(ObjectLocation &loc) const;

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status setDataspace(const Dataspace *dataspace);

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status move(const Dataspace *dataspace);

    /**
       Not yet documented
       @param prot_oid
       @return
    */
    Status setProtection(const Oid &prot_oid);

    /**
       Not yet documented
       @param prot
       @return
    */
    Status setProtection(Protection *prot);

    /**
       Not yet documented
       @param prot_oid
       @return
    */
    Status getProtection(Oid &prot_oid) const;

    /**
       Not yet documented
       @param prot
       @return
    */
    Status getProtection(Protection *&prot) const;

    /**
       Not yet documented
       @param data
       @return
    */
    virtual Status setValue(Data data) = 0;

    /**
       Not yet documented
       @param data
       @return
    */
    virtual Status getValue(Data *data) const = 0;
  
    /**
       Not yet documented
       @return
    */
    Bool isUnrealizable() const {return unrealizable;}

    /**
       Not yet documented
       @return
    */
    Bool isRemoved() const {return removed;}

    /**
       Not yet documented
       @return
    */
    const void *getPtr() const {return (const void *)this;};

    virtual ~Object();

    // ----------------------------------------------------------------------
    // Down Casting Methods
    // ----------------------------------------------------------------------

    // Const Versions
    /**
       Not yet documented
       @return
    */
    virtual const Instance        *asInstance() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Basic           *asBasic() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Int16           *asInt16() const {return 0;}

    /**
       Not yet documented
       @return 
    */
    virtual const Int64           *asInt64() const {return 0;}

    /**
       Not yet documented
       @return 
    */
    virtual const Int32           *asInt32() const {return 0;}

    /**
       Not yet documented
       @return 
    */
    virtual const Byte            *asByte() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Char            *asChar() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Float           *asFloat() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const OidP            *asOidP() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Agregat         *asAgregat() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Struct          *asStruct() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Union           *asUnion() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Enum            *asEnum() const {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual const Collection      *asCollection() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollSet         *asCollSet() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollArray       *asCollArray() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollBag         *asCollBag() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollList        *asCollList() const {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual const Class           *asClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const AgregatClass    *asAgregatClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const StructClass     *asStructClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const UnionClass      *asUnionClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const BasicClass      *asBasicClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Int16Class      *asInt16Class() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Int32Class      *asInt32Class() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Int64Class      *asInt64Class() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const ByteClass       *asByteClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CharClass       *asCharClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const FloatClass      *asFloatClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const OidClass        *asOidClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollectionClass *asCollectionClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollSetClass    *asCollSetClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollBagClass    *asCollBagClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollArrayClass  *asCollArrayClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const CollListClass   *asCollListClass() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const EnumClass       *asEnumClass() const {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual const Schema          *asSchema() const {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual const Database        *asDatabase() const {return 0;}


    // Non Const Versions
    /**
       Not yet documented
       @return
    */
    virtual Instance        *asInstance() {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual Basic           *asBasic() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int16           *asInt16() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int32           *asInt32() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int64           *asInt64() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Byte            *asByte() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Char            *asChar() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Float           *asFloat() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual OidP            *asOidP() {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual Agregat         *asAgregat() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Struct          *asStruct() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Union           *asUnion() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Enum            *asEnum() {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual Collection      *asCollection() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollSet         *asCollSet() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollArray       *asCollArray() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollBag         *asCollBag() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollList        *asCollList() {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual Class           *asClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual AgregatClass    *asAgregatClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual StructClass     *asStructClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual UnionClass      *asUnionClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual BasicClass      *asBasicClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int16Class      *asInt16Class() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int32Class      *asInt32Class() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Int64Class      *asInt64Class() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual ByteClass       *asByteClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CharClass       *asCharClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual FloatClass      *asFloatClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual OidClass        *asOidClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollectionClass *asCollectionClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollSetClass    *asCollSetClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollBagClass    *asCollBagClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollArrayClass  *asCollArrayClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual CollListClass   *asCollListClass() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual EnumClass       *asEnumClass() {return 0;}


    /**
       Not yet documented
       @return
    */
    virtual Schema          *asSchema() {return 0;}

    /**
       Not yet documented
       @return
    */
    virtual Database        *asDatabase() {return 0;}


    static Bool setReleaseCycleDetection(Bool);

    /**
       Not yet documented
       @return
    */
    static Bool getReleaseCycleDetection() {return release_cycle_detection;}


    // ----------------------------------------------------------------------
    //  Object Protected Part
    // ----------------------------------------------------------------------
  protected:
    Bool removed;
    Oid oid;
    Oid oid_prot;
    Database *db;
    Bool unrealizable;
    const Attribute *damaged_attr;
    int xinfo;

    class IDR {

      int refcnt;
      Data idr;
      Size idr_sz;

    public:
      IDR(Size);
      IDR(Size, Data);
      ~IDR();
      Size getSize() const {return idr_sz;}
      const unsigned char *getIDR() const {return idr;}
      Data getIDR() {return idr;}
      int incrRefCount() {return refcnt++;}
      int decrRefCount() {return --refcnt;}
      int getRefCount() const {return refcnt;}
      void setIDR(Data);
      void setIDR(Size);
      void setIDR(Size, Data);
    } *idr;

    void setClass(Class *cls);
    void setClass(const Class *cls);

    Type type;
    unsigned short state;

    enum {Tracing = 0x1, Realizing = 0x2, Removing = 0x4};
    void *user_data;
    UserDataHT *user_data_ht;
    void *oql_info;
    Bool modify;
    Bool applyingTrigger;
    Bool dirty;
    void headerCode(eyedblib::int32, eyedblib::int32, eyedblib::int32 = 0);
    void classOidCode(void);
    virtual void garbage();
    virtual gbxBool grant_release();
    void trace_flags(FILE *fd, unsigned int) const;
    void initialize(Database *);

    virtual void userInitialize() {}
    virtual void userCopy(const Object &) {}
    virtual void userGarbage() {}

    Bool traceRemoved(FILE *, const char *) const;

    eyedblib::int64 c_time;
    eyedblib::int64 m_time;

    static void freeList(LinkedList *, Bool wipeOut);
    static LinkedList* copyList(const LinkedList *, Bool copy);

    // ----------------------------------------------------------------------
    //  Object Private Part
    // ----------------------------------------------------------------------
  private:

    Class *cls;

    static Bool release_cycle_detection;
    friend class ObjectPeer;
    Object *master_object;

    void copy(const Object *, Bool);

    void setUnrealizable(Bool);
    void setOid(const Oid&);
    virtual Status trace_realize(FILE*, int, unsigned int, const RecMode *) const = 0;
    void init(Bool);
    Bool grt_obj;
    short dspid;
    const Dataspace *dataspace;

    // ----------------------------------------------------------------------
    // Object Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    virtual Status create() = 0;
    virtual Status update() = 0;
    virtual Status realize(const RecMode *recmode = RecMode::NoRecurs);

    short getDataspaceID() const;
    virtual void touch();
    Bool isApplyingTrigger() const {return applyingTrigger;}
    void setApplyingTrigger(Bool _applying) {applyingTrigger = _applying;}
    Bool isDirty() const {return dirty;}
    void setDirty(Bool _dirty) {dirty = _dirty;}
    void *setOQLInfo(void *);
    void *getOQLInfo();
    void unlock_refcnt();
    void setDspid(short);
    void setDamaged(const Attribute *);
    const Attribute *getDamaged() const {return damaged_attr;}

    virtual Status remove_r(const RecMode *rcm = RecMode::NoRecurs,
			    unsigned int flags = 0);

    const Object *getMasterObject(bool recurs) const;
    Object *getMasterObject(bool recurs);

    virtual Status setMasterObject(Object *master_object);

    virtual Status releaseMasterObject();

    virtual Status realizePerform(const Oid& cloid,
				  const Oid& objoid,
				  AttrIdxContext &idx_ctx,
				  const RecMode *);
    virtual Status loadPerform(const Oid&,
			       LockMode lockmode,
			       AttrIdxContext &idx_ctx,
			       const RecMode* = RecMode::NoRecurs);
    virtual Status removePerform(const Oid& cloid,
				 const Oid& objoid,
				 AttrIdxContext &idx_ctx,
				 const RecMode *);
    virtual Status postRealizePerform(const Oid& cloid,
				      const Oid& objoid,
				      AttrIdxContext &idx_ctx,
				      Bool &mustTouch,
				      const RecMode *rcm) {
      mustTouch = True;
      return Success;
    }

    void setXInfo(int _xinfo) {xinfo = _xinfo;}
  };

  std::ostream& operator<<(std::ostream&, const Object &);
  std::ostream& operator<<(std::ostream&, const Object *);

  enum TraceFlag {
    MTimeTrace    = 0x001,
    CTimeTrace    = 0x002,
    CMTimeTrace   = MTimeTrace | CTimeTrace,
    PointerTrace  = 0x004,
    CompOidTrace  = 0x008,
    NativeTrace   = 0x010,
    ContentsFlag  = 0x020,
    InhAttrTrace  = 0x040,
    InhExecTrace  = 0x080,
    ExecBodyTrace = 0x100,
    SysExecTrace  = 0x200,
    AttrCompTrace = 0x400,
    AttrCompDetailTrace = 0x800,
    NoScope       = 0x1000
  };

  class ObjectList;

  class ObjectPtr : public gbxObjectPtr {

  public:
    ObjectPtr(Object *o = 0) : gbxObjectPtr(o) { }

    Object *getObject() {return dynamic_cast<Object *>(o);}
    const Object *getObject() const {return dynamic_cast<const Object *>(o);}

    Object *operator->() {return dynamic_cast<Object *>(o);}
    const Object *operator->() const {return dynamic_cast<const Object *>(o);}
  };

  typedef std::vector<ObjectPtr> ObjectPtrVector;

  std::ostream& operator<<(std::ostream&, const ObjectPtr &);

  /**
     Not yet documented.
  */
  class ObjectArray {

    // ----------------------------------------------------------------------
    // ObjectArray Interface
    // ----------------------------------------------------------------------
  public:
    ObjectArray(Object ** = 0, unsigned int count = 0);
    ObjectArray(bool auto_garb, Object ** = 0, unsigned int count = 0);
    ObjectArray(const Collection *, bool auto_garb = False);
    ObjectArray(const ObjectArray &);
    ObjectArray(const ObjectList &);

    ObjectArray& operator=(const ObjectArray &);

    void set(Object **, unsigned int count);
    Status setObjectAt(unsigned int ind, Object *o);

    unsigned int getCount() const {return count;}

    // voluntary not left value !
    Object *operator[](unsigned int ind) {
      return objs[ind];
    }

    const Object *operator[](unsigned int ind) const {
      return objs[ind];
    }

    void setAutoGarbage(bool _auto_garb) {auto_garb = _auto_garb;}
    bool isAutoGarbage() const {return auto_garb;}

    void setMustRelease(bool must_release);

    ObjectList *toList() const;
    void makeObjectPtrVector(ObjectPtrVector &);

    void garbage();

    ~ObjectArray();

    // ----------------------------------------------------------------------
    // ObjectArray Private Part
    // ----------------------------------------------------------------------
  private:
    unsigned int count;
    Object **objs;
    bool auto_garb;
  };

  class ObjectListCursor;

  class ObjectList {

  public:
    ObjectList();
    ObjectList(const ObjectArray &);
    int getCount(void) const;

    void insertObject(Object *);
    void insertObjectLast(Object *);
    void insertObjectFirst(Object *);
    Bool suppressObject(Object *);
    Bool exists(const Object *) const;
    void empty(void);

    ObjectArray *toArray() const;

    ~ObjectList();

  private:
    LinkedList *list;
    friend class ObjectListCursor;
  };

  class ObjectListCursor {

  public:
    ObjectListCursor(const ObjectList &);
    ObjectListCursor(const ObjectList *);

    Bool getNext(Object *&);

    ~ObjectListCursor();

  private:
    LinkedListCursor *c;
  };

  class ObjectReleaser {

  public:
    ObjectReleaser(Object *);

    Object *dontRelease();
    operator Object *() {return o;}

    ~ObjectReleaser();

  private:
    Object *o;
    Bool dont_release;
  };

  class ObjectListReleaser {

  public:
    ObjectListReleaser();
    void add(Object *);
    void dontRelease();
    ~ObjectListReleaser();

  private:
    Bool dont_release;
    LinkedList list;
  };

  class ObjectObserver : public gbxObserver {

  public:
    typedef std::vector<Object *> ObjectVector;
    typedef ObjectVector::iterator ObjectVectorIterator;
    typedef ObjectVector::const_iterator ObjectVectorConstIterator;

    ObjectObserver(const std::string &tag = "");

    virtual void getObjects(ObjectVector &) const;
    virtual bool isObjectRegistered(Object *) const;

    virtual void addObj(gbxObject *o);
    virtual void rmvObj(gbxObject *o);

    ~ObjectObserver();
  };

  /**
     @}
  */

}

#endif
