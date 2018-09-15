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


#include <assert.h>

namespace eyedb {

  class AttrDirect : public Attribute {

    Status check() const;
    void newObjRealize(Object *) const;
    Status varDimCopy(Data, Data) const;
#ifdef GBX_NEW_CYCLE
    void decrRefCountPropag(Object *, int) const;
#endif
    void garbage(Object *, int) const;
    void manageCycle(Database *db, Object *o, gbxCycleContext &r) const;

  public:
    AttrDirect(const Attribute*, const Class *, const Class *, const Class *, int);
    AttrDirect(Database *, Data, Offset *, const Class *, int);

    inline Bool isVarDim() const {return False;}
    inline Bool isIndirect() const {return False;}

    Status compile_perst(const AgregatClass *, int *, int *, int *);
    Status compile_volat(const AgregatClass *, int *, int *);

    Status setOid(Object *, const Oid *, int=1, int=0, Bool=True) const;
    Status getOid(const Object *, Oid *, int=1, int=0) const;
    Status setValue(Object *, Data, int, int, Bool = True) const;
    Status getValue(const Object *, Data *, int, int, Bool * = NULL) const;
    Status getTValue(Database *db, const Oid &objoid,
		     Data *data, int nb = 1, int from = 0,
		     Bool *isnull = 0, Size *rnb = 0, Offset = 0) const;
    // deprecated
    Status getVal(Database *, const Oid *, Data,
		  int, int, int, Bool * = NULL) const;
    /*
      Status getField(Database *, const Oid *, Oid *, int *,
      int, int) const;
    */
    Status realize(Database *, Object *,
		   const Oid&,
		   const Oid&,
		   AttrIdxContext &idx_ctx,
		   const RecMode *) const;
    Status remove(Database *, Object *,
		  const Oid&,
		  const Oid&,
		  AttrIdxContext &idx_ctx,
		  const RecMode *) const; 
    Status load(Database *,
		Object *,
		const Oid &,
		LockMode lockmode,
		AttrIdxContext &,
		const RecMode *, Bool force = False) const; 
    Status trace(const Object *, FILE *, int *, unsigned int, const RecMode *) const;
    Bool isFlat() const {return True;}
    virtual Status convert(Database *db, ClassConversion *,
			   Data in_idr, Size in_size) const;
  };

  class AttrIndirect : public Attribute {

    Status check() const;
#ifdef GBX_NEW_CYCLE
    void decrRefCountPropag(Object *, int) const;
#endif
    void garbage(Object *, int) const;
    void manageCycle(Database *db, Object *o, gbxCycleContext &r) const;

  public:
    AttrIndirect(const Attribute*, const Class *, const Class *, const Class *, int);
    AttrIndirect(Database *, Data, Offset *, const Class *, int);

    inline Bool isVarDim() const {return False;}
    inline Bool isIndirect() const {return True;}

    Status compile_perst(const AgregatClass *, int *, int *, int *);
    Status compile_volat(const AgregatClass *, int *, int *);

    Status setOid(Object *, const Oid *, int=1, int=0, Bool=True) const;
    Status getOid(const Object *, Oid *, int=1, int=0) const;
    Status setValue(Object *, Data, int, int, Bool = True) const;
    Status getValue(const Object *, Data *, int, int, Bool * = NULL) const;
    Status getTValue(Database *db, const Oid &objoid,
		     Data *data, int nb = 1, int from = 0,
		     Bool *isnull = 0, Size *rnb = 0, Offset = 0) const;

    // deprecated
    Status getVal(Database *, const Oid *, Data,
		  int, int, int, Bool * = NULL) const;
    /*
      Status getField(Database *, const Oid *, Oid *, int *,
      int, int) const;
    */
    Status realize(Database *, Object *,
		   const Oid&,
		   const Oid&,
		   AttrIdxContext &idx_ctx,
		   const RecMode *) const;
    Status remove(Database *, Object *,
		  const Oid&,
		  const Oid&,
		  AttrIdxContext &idx_ctx,
		  const RecMode *) const; 
    Status load(Database *,
		Object *,
		const Oid &,
		LockMode lockmode,
		AttrIdxContext &,
		const RecMode *, Bool force = False) const; 
    Status trace(const Object *, FILE *, int *, unsigned int, const RecMode *) const;
    virtual Status convert(Database *db, ClassConversion *,
			   Data in_idr, Size in_size) const;
  };

  class AttrVD : public Attribute {

    virtual void setVarDimOid(Object *, const Oid *) const = 0;
    virtual void setSizeChanged(Object *, Bool) const = 0;
    virtual Bool isSizeChanged(const Object *) const = 0;
    virtual Status getDefaultDataspace(const Dataspace *&) const;
    virtual Status setDefaultDataspace(const Dataspace *);

  protected:
    AttrVD(const Attribute*, const Class *, const Class *, const Class *, int);
    AttrVD(Database *, Data, Offset *, const Class *, int);

    Status update_realize(Database *, Object *, const Oid&, 
			  const Oid&, int, Size, Data,
			  Oid &oid,
			  AttrIdxContext &idx_ctx) const;
    Status remove_realize(Database *,
			  const Oid&, const Oid&, Object *,
			  AttrIdxContext &idx_ctx) const;

    Status getSize(Data, Size&) const;
    Status getSize(const Object *, Size&) const;
    Status getSize(Database *, const Oid *, Size&) const;

    // deprecated
    Status getVal(Database *, const Oid *, Data,
		  int, int, int, Bool * = NULL) const;
    /*
      Status getField(Database *, const Oid *, Oid *, int *,
      int, int) const;
    */
  };

  class AttrVarDim : public AttrVD {

    Status check() const;
    void manageCycle(Database *db, Object *o, gbxCycleContext &r) const;

    void newObjRealize(Object *) const;
    void getData(const Database *, Data, Data&, Data&) const;
    void getData(const Object *, Data&, Data&) const;
    void setData(const Database *, Data, Data, Data) const;
    void setData(const Object *, Data, Data) const;
    void getInfo(const Object *, Data&, Data&, Size&, Size&,
		 Oid&) const;
    int iniCompute(const Database *, int, Data &, Data&) const;
    void setVarDimOid(Object *, const Oid *) const;
    void getVarDimOid(const Object *, Oid *) const;
    void getVarDimOid(Data, Oid *) const;
    Status setSize_realize(Object *, Data, Size, Bool,
			   Bool noGarbage = False) const;
    int getBound(Database *, Data);
    Status copy(Object*, Bool) const;
#ifdef GBX_NEW_CYCLE
    void decrRefCountPropag(Object *, int) const;
#endif
    void garbage(Object *, int) const;
    void setSizeChanged(Object *, Bool) const;
    Bool isSizeChanged(const Object *) const;
    Status update(Database *, const Oid&,
		  const Oid&, Object *,
		  AttrIdxContext &idx_ctx) const;
    Bool getIsLoaded(const Object *agr) const;
    void setIsLoaded(const Object *agr, Bool) const;
    Bool getIsLoaded(Data) const;
    void setIsLoaded(Data, Bool) const;

  public:
    AttrVarDim(const Attribute*, const Class *, const Class *, const Class *, int);
    AttrVarDim(Database *, Data, Offset *, const Class *, int);

    inline Bool isVarDim() const {return True;}
    inline Bool isIndirect() const {return False;}

    Status compile_perst(const AgregatClass *, int *, int *, int *);
    Status compile_volat(const AgregatClass *, int *, int *);

    Status setSize(Object *, Size) const;
    Status setOid(Object *, const Oid *, int=1, int=0, Bool=True) const;
    Status getOid(const Object *, Oid *, int=1, int=0) const;
    Status setValue(Object *, Data, int, int, Bool = True) const;
    Status getValue(const Object *, Data *, int, int, Bool * = NULL) const;
    Status getTValue(Database *db, const Oid &objoid,
		     Data *data, int nb = 1, int from = 0,
		     Bool *isnull = 0, Size *rnb = 0, Offset = 0) const;

    Status realize(Database *, Object *,
		   const Oid&,
		   const Oid&,
		   AttrIdxContext &idx_ctx,
		   const RecMode *) const;
    Status remove(Database *, Object *,
		  const Oid&,
		  const Oid&,
		  AttrIdxContext &idx_ctx,
		  const RecMode *) const; 
    Status load(Database *,
		Object *,
		const Oid &,
		LockMode lockmode,
		AttrIdxContext &,
		const RecMode *, Bool force = False) const; 
    Status trace(const Object *, FILE *, int *, unsigned int, const RecMode *) const;
    virtual Status convert(Database *db, ClassConversion *,
			   Data in_idr, Size in_size) const;
  };

  class AttrIndirectVarDim : public AttrVD {

    Status check() const;
    void manageCycle(Database *db, Object *o, gbxCycleContext &r) const;
    void getData(const Database *, Data, Data&, Data&) const;
    void getData(const Object *, Data&, Data&) const;
    void setData(const Database *, Data, Data) const;
    void setData(const Object *, Data) const;
    void getInfo(const Object *, Size&, Data&, Oid&) const;
    void getInfoOids(const Object *, Size&, Data&, Oid&) const;
    void setVarDimOid(Object *, const Oid *) const;
    void getVarDimOid(const Object *, Oid *) const;
    void getVarDimOid(Data, Oid *) const;
    void setDataOids(Object *, Data) const;
    void setDataOids(Data, Data) const;
    void getDataOids(Data, Data&) const; 
    void getDataOids(const Object *, Data&) const; 
    Status copy(Object*, Bool) const;
#ifdef GBX_NEW_CYCLE
    void decrRefCountPropag(Object *, int) const;
#endif
    void garbage(Object *, int) const;
    void setSizeChanged(Object *, Bool) const;
    Bool isSizeChanged(const Object *) const;
    Status update(Database *, const Oid&,
		  const Oid&, Object *,
		  AttrIdxContext &idx_ctx) const;
  public:
    AttrIndirectVarDim(const Attribute*, const Class *, const Class *, const Class *, int);
    AttrIndirectVarDim(Database *, Data, Offset *, const Class *, int);

    inline Bool isVarDim() const {return True;}
    inline Bool isIndirect() const {return True;}

    Status compile_perst(const AgregatClass *, int *, int *, int *);
    Status compile_volat(const AgregatClass *, int *, int *);

    Status setSize(Object *, Size) const;

    Status setOid(Object *, const Oid *, int=1, int=0, Bool=True) const;
    Status getOid(const Object *, Oid *, int=1, int=0) const;
    Status setValue(Object *, Data, int, int, Bool = True) const;
    Status getValue(const Object *, Data *, int, int, Bool * = NULL) const;
    Status getTValue(Database *db, const Oid &objoid,
		     Data *data, int nb = 1, int from = 0,
		     Bool *isnull = 0, Size *rnb = 0, Offset = 0) const;
    Status realize(Database *, Object *,
		   const Oid&,
		   const Oid&,
		   AttrIdxContext &idx_ctx,
		   const RecMode *) const;
    Status remove(Database *, Object *,
		  const Oid&,
		  const Oid&,
		  AttrIdxContext &idx_ctx,
		  const RecMode *) const; 
    Status load(Database *,
		Object *,
		const Oid &,
		LockMode lockmode,
		AttrIdxContext &,
		const RecMode *, Bool force = False) const; 
    Status trace(const Object *, FILE *, int *, unsigned int, const RecMode *) const;
    virtual Status convert(Database *db, ClassConversion *,
			   Data in_idr, Size in_size) const;
  };

  class AttrIdxContext {

  public:
    AttrIdxContext() {
      init();
    }

    AttrIdxContext(const Class *_class_owner) {
      init();
      set(_class_owner, (Attribute *)0);
    }

    AttrIdxContext(const Class *_class_owner, const Attribute *attr) {
      init();
      set(_class_owner, attr);
    }

    AttrIdxContext(Class *_class_owner, Attribute **_attrs,
		   int _attr_cnt) {
      init();
      if (!_attr_cnt)
	return;
      set(_class_owner, _attrs[0]);
      for (int i = 1; i < _attr_cnt; i++)
	push(_attrs[i]);
    }

    AttrIdxContext(const Data, Size);

    AttrIdxContext(AttrIdxContext *);

    void set(const Class *_class_owner, const Attribute *attr) {
      garbage(False);
      set(_class_owner);
      attr_cnt = 0;
      if (attr)
	push(attr);
    }

    void push(Database *db, const Oid &cloid, const Attribute *attr) {
      if (!class_owner) {
	set(db->getSchema()->getClass(cloid), attr);
	assert(class_owner);
	return;
      }
      push(attr);
    }

    void push(const Class *_class_owner, const Attribute *attr) {
      if (!class_owner) {
	set(_class_owner, attr);
	return;
      }
      push(attr);
    }

    void push(const Attribute *attr);
    void push(const char *attrname);
    void pop();

    int getLevel() const {return attr_cnt;}

    enum IdxOP {
      IdxInsert = 1,
      IdxRemove
    };

    void addIdxOP(const Attribute *attr, IdxOP,
		  Index *, eyedbsm::Idx *, unsigned char *data, unsigned int sz,
		  Oid data_oid[2]);
		
    Status realizeIdxOP(Bool);

    Attribute *getAttribute(const Class *) const;

    Data code(Size &) const;
    std::string getString() const;

    std::string getAttrName(Bool ignore_class_owner = False) const;
    const char *getClassOwner() const {return class_owner;}
    const char *getAttrName(int n) const {return n >= attr_cnt ? 0 : attrs[n].c_str();}
    unsigned int getAttrCount() const {return attr_cnt;}

    void pushOff(int off, const Oid &data_oid);
    void pushOff(int off);
    int getOff();
    void popOff();

    Oid getDataOid();

    int operator==(const AttrIdxContext &) const;
    int operator!=(const AttrIdxContext &idx_ctx) const {
      return !(*this == idx_ctx);
    }

    ~AttrIdxContext() {
      garbage(True);
    }

  private:

    void set(const Class *_class_owner);

    eyedblib::Mutex mut;
    struct IdxOperation {
      const Attribute *attr;
      IdxOP op;
      Index *idx_t;
      eyedbsm::Idx *idx;
      unsigned char *data;
      Oid data_oid[2];
    } *idx_ops;

    int idx_ops_cnt;
    int idx_ops_alloc;
    AttrIdxContext *idx_ctx_root;
    Bool toFree;

    void init() {
      toFree = False;
      attrpath_computed = False;
      attr_off_cnt = 0;
      attr_cnt = 0;
      class_owner = 0;
      idx_ops_cnt = 0;
      idx_ops_alloc = 0;
      idx_ops = 0;
      idx_ctx_root = 0;
    }

    void garbage(Bool);
    char *class_owner;
    std::string attrs[64];
    struct {
      int off;
      Oid data_oid;
    } attr_off[64];
    eyedblib::int16 attr_off_cnt;
    eyedblib::int16 attr_cnt;
    mutable Bool attrpath_ignore_class_owner;
    mutable Bool attrpath_computed;
    mutable char attrpath[512];

  private: // forbidden
    AttrIdxContext(const AttrIdxContext &);
    AttrIdxContext &operator=(const AttrIdxContext &);
  };

  enum {
    AttrDirect_Code         = 0x31,
    AttrIndirect_Code,
    AttrVarDim_Code,
    AttrIndirectVarDim_Code,
    AttrNative_Code
  };

  extern void
  get_prefix(const Object *, const Class *, char prefix[], int);

#define MULTIIDX_VERSION 20413

}

#include <eyedb/AttrNative.h>
