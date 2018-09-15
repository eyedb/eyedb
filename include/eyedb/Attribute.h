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


#ifndef _EYEDB_ATTRIBUTE_H
#define _EYEDB_ATTRIBUTE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  // forward definitions
  class ObjectHeader;
  class AgregatClass;
  class GenContext;
  class Index;
  class Class;
  class Schema;
  class ClassSequence;
  class ClassSeqItem;
  class AttrStack;
  class CardinalityConstraint;
  class UniqueConstraint;
  class NotNullConstraint;
  class ClassConversion;
  class AttributeComponent;
  class AttributeComponentSet;
  class CollAttrImpl;
  class Dataspace;

  class AttrIdxContext;
  class MultiIndex;

  /**
     Not yet documented.
  */
  class TypeModifier {

    // ----------------------------------------------------------------------
    // TypeModifier Interface
    // ----------------------------------------------------------------------
  public:
    enum {_Indirect = 0x1, _VarDim = 0x2};

    enum _mode {
      Direct         = 0,
      Indirect       = _Indirect,
      VarDim         = _VarDim,
      IndirectVarDim = _Indirect|_VarDim
    } mode;

    eyedblib::int16 ndims;
    eyedblib::int32 *dims;
    eyedblib::int32 pdims;
    eyedblib::int32 maxdims;

    /**
       Not yet documented
       @param isRef
       @param ndims
       @param dims
       @return
    */
    static TypeModifier make(Bool isRef, int ndims, int *dims);

    /**
       Not yet documented
       @param data
       @param offset
       @param alloc_size
       @return
    */
    Status codeIDR(Data* data, Offset* offset, Size* alloc_size);

    /**
       Not yet documented
       @param data
       @param offset
       @return
    */
    Status decodeIDR(Data data, Offset *offset);

    /**
       Not yet documented
       @param tmod
       @return
    */
    Bool compare(const TypeModifier *tmod) const;

    /**
       Not yet documented
    */
    TypeModifier();

    /**
       Not yet documented
       @param typmod
    */
    TypeModifier(const TypeModifier &typmod);

    /**
       Not yet documented
       @param typmod
       @return
    */
    TypeModifier &operator=(const TypeModifier &typmod);

    /**
       Not yet documented
    */
    ~TypeModifier();
  };

  /**
     Not yet documented.
  */
  class Attribute {

    // ----------------------------------------------------------------------
    // Attribute Interface
    // ----------------------------------------------------------------------
  public:
    enum {
      defaultSize  = -1,
      directAccess = -2,
      wholeData = -3
    };

    /**
       Not yet documented
       @return
    */
    const char *getName() const;

    /**
       Not yet documented
       @return
    */
    CardinalityConstraint *getCardinalityConstraint() const;

    /**
       Not yet documented
       @param db
       @param ind
       @param maxind
       @param sz
       @param idx_ctx
       @param idx
       @param se_idx
       @return
    */
    Status getIdx(Database *db, int ind, int& maxind, Size &sz,
		  const AttrIdxContext &idx_ctx, Index *&idx,
		  eyedbsm::Idx *&se_idx) const;

    /**
       Not yet documented
       @return
    */
    unsigned int getMagOrder() const {return magorder;}

    /**
       Not yet documented
       @param card
       @return
    */
    Status setCardinalityConstraint(CardinalityConstraint *card);

    /**
       Not yet documented
       @return
    */
    virtual Bool isVarDim() const;

    /**
       Not yet documented
       @return
    */
    virtual Bool isIndirect() const;

    /**
       Not yet documented
       @return
    */
    virtual Bool isNative() const {return False;}

    /**
       Not yet documented
       @return
    */
    virtual Bool isString() const {return IDBBOOL(is_string);}

    /**
       Not yet documented
       @return
    */
    virtual Bool isBasicOrEnum() const {return IDBBOOL(is_basic_enum);}

    /**
       Not yet documented
       @return
    */
    virtual Status check() const;

    /**
       Not yet documented
       @param ma
       @param offset
       @param size
       @param inisize
       @return
    */
    virtual Status compile_perst(const AgregatClass *ma, int *offset, int *size, int *inisize);

    /**
       Not yet documented
       @param ma
       @param offset
       @param size
       @return
    */
    virtual Status compile_volat(const AgregatClass *ma, int *offset, int *size);

    /**
       Not yet documented
       @param idr
       @param size
       @return
    */
    virtual Status getSize(Data idr, Size& size) const;

    /**
       Not yet documented
       @param agr
       @param size
       @return
    */
    virtual Status getSize(const Object *agr, Size& size) const;

    /**
       Not yet documented
       @param db
       @param data_oid
       @param size
       @return
    */
    virtual Status getSize(Database *db, const Oid *data_oid, Size& size) const;

    /**
       Not yet documented
       @param agr
       @param size
       @return
    */
    virtual Status setSize(Object *agr, Size size) const;

    /**
       Not yet documented
       @param agr
       @param oid
       @param ??
       @param ??
       @param ??
       @return
    */
    virtual Status setOid(Object *agr, const Oid *oid, int=1, int=0, Bool=True) const;

    /**
       Not yet documented
       @param agr
       @param oid
       @param ??
       @param ??
       @return
    */
    virtual Status getOid(const Object *agr, Oid *oid, int=1, int=0) const;

    /**
       Not yet documented
       @param agr
       @param data
       @param nb
       @param from
       @param check_class
       @return
    */
    virtual Status setValue(Object *agr, Data data, int nb, int from, Bool check_class=True) const;

    /**
       Not yet documented
       @param agr
       @param data
       @param nb
       @param from
       @param isnull
       @return
    */
    virtual Status getValue(const Object *agr, Data *data, int nb, int from, Bool *isnull = 0) const;

    /**
       Not yet documented
       @param db
       @param objoid
       @param data
       @param nb
       @param from
       @param isnull
       @param rnb
       @param poffset
       @return
    */
    virtual Status getTValue(Database *db, const Oid &objoid,
			     Data *data, int nb = 1, int from = 0,
			     Bool *isnull = 0, Size *rnb = 0, Offset poffset= 0) const;

    /**
       Not yet documented
       @param nuser_data
       @return
    */
    void *setUserData(void *nuser_data);

    /**
       Not yet documented
       @return
    */
    void *getUserData(void) {return user_data;}

    /**
       Not yet documented
       @return
    */
    const void *getUserData(void) const {return user_data;}

    // deprecated
    virtual Status getVal(Database *, const Oid *, Data,
			  int, int, int, Bool * = 0) const;

    /*
      virtual Status getField(Database *, const Oid *, Oid *, int *,
      int, int) const;
    */

    /**
       Not yet documented
       @param db
       @param cloid
       @param objoid
       @param agr
       @param idx_ctx
       @return
    */
    virtual Status update(Database *db,
			  const Oid& cloid,
			  const Oid& objoid,
			  Object *agr,
			  AttrIdxContext &idx_ctx) const;

    /**
       Not yet documented
       @param db
       @param agr
       @param cloid
       @param objoid
       @param idx_ctx
       @param rcm
       @return
    */
    virtual Status realize(Database *db, Object *agr,
			   const Oid& cloid,
			   const Oid& objoid,
			   AttrIdxContext &idx_ctx,
			   const RecMode *rcm) const; 

    /**
       Not yet documented
       @param db
       @param agr
       @param cloid
       @param objoid
       @param idx_ctx
       @param rcm
       @return
    */
    virtual Status remove(Database *db, Object *agr,
			  const Oid& cloid,
			  const Oid& objoid,
			  AttrIdxContext &idx_ctx,
			  const RecMode *rcm) const; 

    /**
       Not yet documented
       @param db
       @param agr
       @param cloid
       @param lockmode
       @param idx_ctx
       @param rcm
       @param force
       @return
    */
    virtual Status load(Database *db,
			Object *agr,
			const Oid &cloid,
			LockMode lockmode,
			AttrIdxContext &idx_ctx,
			const RecMode *rcm, 
			Bool force = False) const; 

    /**
       Not yet documented
       @param agr
       @param fd
       @param indent
       @param flags
       @param rcm
       @return
    */
    virtual Status trace(const Object *agr, FILE *fd, int *indent, unsigned int flags,
			 const RecMode *rcm) const;

    /**
       Not yet documented
       @return
    */
    const TypeModifier &getTypeModifier() const;

    /**
       Not yet documented
       @param tp
       @param s
       @param isref
       @param ndims
       @param dims
    */
    Attribute(Class *tp, const char *s, Bool isref=False,
	      int ndims=0, int *dims=0);

    /**
       Not yet documented
       @param tp
       @param s
       @param dim
    */
    Attribute(Class *tp, const char *s, int dim);

    /**
       Not yet documented
       @param attr
       @param cls
       @param class_owner
       @param dyn_class_owner
       @param n
    */
    Attribute(const Attribute*attr, const Class *cls, const Class *class_owner,
	      const Class *dyn_class_owner, int n);

    /**
       Not yet documented
       @param p_off
       @param item_p_sz
       @param p_sz
       @param item_ini_sz
    */
    void getPersistentIDR(Offset& p_off, Size& item_p_sz, Size& p_sz, Size& item_ini_sz) const;

    /**
       Not yet documented
       @return
    */
    Offset getPersistentIDR() const {return idr_poff;}

    /**
       Not yet documented
       @param v_off
       @param item_v_sz
       @param v_sz
    */
    void getVolatileIDR(Offset& v_off, Size& item_v_sz, Size& v_sz) const;

    /**
       Not yet documented
       @return
    */
    const Class *getClass() const {return cls;}

    /**
       Not yet documented
       @return
    */
    const Class *getClassOwner() const {return class_owner;}

    /**
       Not yet documented
       @return
    */
    const Class *getDynClassOwner() const {return dyn_class_owner;}

    /**
       Not yet documented
       @return
    */
    int getNum() const {return num;}

    /**
       Not yet documented
       @param db
       @return
    */
    virtual Attribute *clone(Database *db = 0) const;

    /**
       Not yet documented
       @return
    */
    virtual Bool isFlat() const {return False;}

    /**
       Not yet documented
       @param db
       @param data
       @param offset
       @param alloc_size
       @return
    */
    Status codeIDR(Database *db, Data* data, Offset* offset, Size* alloc_size);

    /**
       Not yet documented
       @param data
       @param offset
       @return
    */
    Status decodeIDR(Data data, Offset *offset);

    /**
       Not yet documented
       @param db
       @param item
       @return
    */
    Bool compare(Database *db, const Attribute *item) const;

    /**
       Not yet documented
       @param ??
       @return
    */
    Status setInverse(const Attribute *);

    /**
       Not yet documented
       @param ??
       @param ??
       @return
    */
    Status setInverse(const char *, const char *);

    /**
       Not yet documented
       @param ??
       @param ??
       @param ??
       @return
    */
    void getInverse(const char **, const char **, const Attribute **) const;

    /**
       Not yet documented
       @return
    */
    Bool hasInverse() const;

    /**
       Not yet documented
       @param dataspace
       @return
    */
    virtual Status getDefaultDataspace(const Dataspace *&dataspace) const;

    /**
       Not yet documented
       @param dataspace
       @return
    */
    virtual Status setDefaultDataspace(const Dataspace *dataspace);

    // BE
    static unsigned char idxNull, idxNotNull;

    /**
       Not yet documented
       @param db
       @param idx_ctx
       @param idx
       @param create
       @return
    */
    Status indexPrologue(Database *db, const AttrIdxContext &idx_ctx,
			 Index *&idx, Bool create);

    /**
       Not yet documented
       @param db
       @param idx_ctx
       @param idx
       @return
    */
    Status createDeferredIndex_realize(Database *db,
				       const AttrIdxContext &idx_ctx,
				       Index *idx);

    /**
       Not yet documented
       @param db 
       @param idx
       @return
    */
    Status destroyIndex(Database *db, Index *idx) const;

    /**
       Not yet documented
       @param db
       @param attrpath
       @param idx
       @return
    */
    static Status getIndex(Database *db, const char *attrpath,
			   Index *&idx);

    /**
       Not yet documented
       @param db
       @param attrpath
       @param unique
       @return
    */
    static Status getUniqueConstraint(Database *db, const char *attrpath,
				      UniqueConstraint *&unique);

    /**
       Not yet documented
       @param db
       @param attrpath
       @param notnull
       @return
    */
    static Status getNotNullConstraint(Database *db, const char *attrpath,
				       NotNullConstraint *&notnull);

    /**
       Not yet documented
       @param db
       @param attrpath
       @param collimpl
       @return
    */
    static Status getCollAttrImpl(Database *db, const char *attrpath,
				  CollAttrImpl *&collimpl);

    /**
       Not yet documented
       @param db
       @param idx_ctx
       @return
    */
    static Status updateIndexEntries(Database *db,
				     AttrIdxContext &idx_ctx);

    /**
       Not yet documented
       @param db
       @param idr
       @param oid
       @param cloid
       @param offset
       @param novd
       @param idx_ctx
       @param count
       @param size
       @return
    */
    Status createIndexEntry_realize(Database *db, Data idr, const Oid *oid,
				    const Oid *cloid, int offset, Bool novd,
				    AttrIdxContext &idx_ctx,
				    int count = 0, int size = -1);

    /**
       Not yet documented
       @param db
       @param idr
       @param oid
       @param cloid
       @param offset
       @param novd
       @param data_oid
       @param idx_ctx
       @param count
       @return
    */
    Status updateIndexEntry_realize(Database *db, Data idr, const Oid *oid,
				    const Oid *cloid, int offset, Bool novd,
				    const Oid *data_oid,
				    AttrIdxContext &idx_ctx,
				    int count = 0);

    /**
       Not yet documented
       @param db
       @param idr
       @param oid
       @param cloid
       @param offset
       @param novd
       @param data_oid
       @param idx_ctx
       @param count
       @return
    */
    Status removeIndexEntry_realize(Database *db, Data idr, const Oid *oid,
				    const Oid *cloid, int offset, Bool novd,
				    const Oid *data_oid,
				    AttrIdxContext &idx_ctx,
				    int count = 0);
  
    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @param cloid
       @param offset
       @param count
       @param ??
       @param ??
       @param ??
       @param idx_ctx
       @return
    */
    Status createIndexEntry(Database *db, Data pdata, const Oid *oid,
			    const Oid *cloid, int offset, int count, int,
			    Size, Bool,
			    AttrIdxContext &idx_ctx);
  
    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @param cloid
       @param offset
       @param data_oid
       @param count
       @param varsize
       @param novd
       @param idx_ctx
       @return
    */
    Status updateIndexEntry(Database *db, Data pdata, const Oid *oid,
			    const Oid *cloid, int offset, const Oid *data_oid, int count,
			    Size varsize, Bool novd,
			    AttrIdxContext &idx_ctx);

    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @param cloid
       @param offset
       @param data_oid
       @param count
       @param varsize
       @param novd
       @param idx_ctx
       @return
    */
    Status removeIndexEntry(Database *db, Data pdata, const Oid *oid,
			    const Oid *cloid, int offset, const Oid *data_oid, int count,
			    Size varsize, Bool novd,
			    AttrIdxContext &idx_ctx);

    /**
       Not yet documented
       @param db
       @param fmt_error
       @param data_oid
       @param offset
       @param varsize
       @param novd
       @param sz
       @param inisize
       @param oinisize
       @param skipRemove
       @param skipInsert
       @return
    */
    Status sizesCompute(Database *db, const char fmt_error[],
			const Oid *data_oid, int &offset,
			Size varsize, Bool novd, int &sz,
			int inisize, int &oinisize,
			Bool &skipRemove, Bool &skipInsert);

    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @return
    */
    virtual Status createInverse_realize(Database *db, Data pdata, const Oid *oid) const;

    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @return
    */
    virtual Status updateInverse_realize(Database *db, Data pdata, const Oid *oid) const;

    /**
       Not yet documented
       @param db
       @param pdata
       @param oid
       @return
    */
    virtual Status removeInverse_realize(Database *db, Data pdata, const Oid *oid) const;

#ifdef GBX_NEW_CYCLE
    virtual void decrRefCountPropag(Object *, int) const;
#endif

    /**
       Not yet documented
       @param ??
       @param ??
    */
    virtual void garbage(Object *, int) const;

    /**
       Not yet documented
       @param db
       @param comp
       @return
    */
    Status addComponent(Database *db, AttributeComponent *comp) const;

    /**
       Not yet documented
       @param db
       @param comp
       @return
    */
    Status rmvComponent(Database *db, AttributeComponent *comp) const;

    /**
       Not yet documented
       @param inidata
       @param tmod
       @return
    */
    static Bool isNull(Data inidata, const TypeModifier *tmod);

    /**
       Not yet documented
       @param inidata
       @param nb
       @param from
       @return
    */
    static Bool isNull(Data inidata, int nb, int from);

    /**
       Not yet documented
       @param m
       @param rcls
       @param attr
       @param attrpath
       @param idx_ctx
       @param just_check_attr
       @return
    */
    static Status checkAttrPath(Schema *m, const Class *&rcls,
				const Attribute *&attr,
				const char *attrpath,
				AttrIdxContext * idx_ctx = 0,
				Bool just_check_attr = False);

    /**
       Not yet documented
    */
    virtual ~Attribute();

    static int composedMode;

    // ----------------------------------------------------------------------
    // Attribute Protected Part
    // ---------------------------------------------------------------------- 
  protected:
    const Dataspace *dataspace;
    short dspid;

    void setItem(Class *, const char *, Bool isRef,
		 int ndims, int *dims,
		 char _is_basic_enum = -1,
		 char _is_string = -1);

    void _setCSDROffset(Offset, Size);
    Status checkTypes(Data, Size, int ) const;
    virtual Status checkRange(int, int&) const;
    virtual Status checkVarRange(const Object *, int, int&, Size*) const;
    virtual Status checkVarRange(int from, int nb, Size size) const;
    Status setValue(Object *, Data, Data, Size, Size,
		    int, int, Data, Bool, Data = 0, Bool = True) const;

    Status getValue(Database *, Data, Data*, Size, int, int,
		    Data, Bool *) const;

    Attribute(Database *, Data, Offset *, const Class *, int);
    Offset endoff;

    unsigned int magorder;

    Oid attr_comp_set_oid;
    AttributeComponentSet *attr_comp_set;

    // C++ generation methods
    virtual Status
    generateCollInsertClassMethod_C(Class *,
				    GenContext *,
				    const GenCodeHints &hints,
				    Bool);

    virtual Status
    generateCollSuppressClassMethod_C(Class *,
				      GenContext *,
				      const GenCodeHints &hints,
				      Bool);
    virtual Status
    generateCollRealizeClassMethod_C(Class *,
				     GenContext *,
				     const GenCodeHints &hints,
				     Bool,
				     int acctype);

    virtual Status
    generateCode_C(Class*, const GenCodeHints &,
		   GenContext *, GenContext *);
    virtual Status generateClassDesc_C(GenContext *);
    virtual Status generateBody_C(Class *, GenContext *,
				  const GenCodeHints &hints);
    virtual Status generateGetMethod_C(Class *, GenContext *,
				       Bool isoid,
				       const GenCodeHints &hints,
				       const char *_const);
    virtual Status generateCollGetMethod_C(Class *, GenContext *,
					   Bool isoid,
					   const GenCodeHints &hints,
					   const char *_const);
    virtual Status generateSetMethod_C(Class *, GenContext *,
				       const GenCodeHints &hints);

    virtual Status generateSetMethod_C(Class *, GenContext *,
				       Bool,
				       const GenCodeHints &hints);

    virtual void genAttrCacheDecl(GenContext *ctx);

    virtual void genAttrCacheEmpty(GenContext *ctx);

    virtual void genAttrCacheGarbage(GenContext *ctx);

    virtual void genAttrCacheGetPrologue(GenContext *ctx, int optype,
					 Bool is_string = False);

    virtual void genAttrCacheGetEpilogue(GenContext *ctx, int optype,
					 Bool is_string = False);

    virtual void genAttrCacheSetPrologue(GenContext *ctx, int optype,
					 Bool is_string = False);

    virtual void genAttrCacheSetEpilogue(GenContext *ctx, int optype,
					 Bool is_string = False);

    // Java generation methods

    virtual Status generateCollGetMethod_Java(Class *own,
					      GenContext *ctx,
					      Bool isoid,
					      const GenCodeHints &hints,
					      const char *_const);

    virtual Status generateCollInsertClassMethod_Java(Class *,
						      GenContext *,
						      const GenCodeHints &,
						      Bool);
    virtual Status
    generateCollSuppressClassMethod_Java(Class *,
					 GenContext *,
					 const GenCodeHints &,
					 Bool);
    virtual Status
    generateCollRealizeClassMethod_Java(Class *,
					GenContext *,
					const GenCodeHints &hints,
					Bool,
					int acctype);
    virtual Status generateCode_Java(Class*,
				     GenContext *,
				     const GenCodeHints &,
				     const char *);
    virtual Status generateClassDesc_Java(GenContext *);
    virtual Status generateBody_Java(Class *, GenContext *,
				     const GenCodeHints &,
				     const char *prefix);
    virtual Status generateGetMethod_Java(Class *, GenContext *,
					  Bool isoid,
					  const GenCodeHints &hints,
					  const char *, const char *);
    virtual Status generateSetMethod_Java(Class *, GenContext *,
					  Bool,
					  const GenCodeHints &);

    virtual Status generateSetMethod_Java(Class *, GenContext *,
					  const GenCodeHints &);


    virtual Status copy(Object*, Bool) const;
    Status incrRefCount(Object *, Data, int) const;
    void manageCycle(Object *, Data, int, gbxCycleContext &r) const;
    void garbage(Data, int) const;
#ifdef GBX_NEW_CYCLE
    void decrRefCountPropag(Data, int) const;
#endif
    Status add(Database *db, ClassConversion *conv,
	       Data in_idr, Size in_size) const;
    const char *name;
    eyedblib::int16 num;
    eyedblib::int16 code;
    const Class *cls;
    const Class *class_owner;
    const Class *dyn_class_owner;
    TypeModifier typmod;

    char is_basic_enum;
    char is_string;

    // IDR
    // persistent
    Offset idr_poff;
    Size idr_item_psize;
    Size idr_psize;
    Offset idr_inisize;

    // volatile
    Offset idr_voff;
    Size idr_item_vsize;
    Size idr_vsize;

    Oid oid_cl;
    Oid oid_cl_own;
    virtual void getVarDimOid(const Object *, Oid *) const;
    virtual int iniCompute(const Database *, int, Data &, Data&) const;
 
    CardinalityConstraint *card;

    void setCollHints(Object *o, const Oid& oid,
		      CardinalityConstraint *card_to_set) const;
    Status setCollImpl(Database *db, Object *o,
		       const AttrIdxContext &idx_ctx) const;
    Status cardManage(Database *, Object *, int) const;

    //Status inverseSet(Object *, const Oid&) const;
    Status inverseManage(Database *, Object *, int) const;
    Status inverseManage(Database *, Object *, Object *) const;

    // ----------------------------------------------------------------------
    // Attribute Private Part
    // ----------------------------------------------------------------------
  private:
    friend class AgregatClass;
    friend class Agregat;
    friend class Class;
    void *user_data;

    friend Status
    agregatClassLoad(Database *, const Oid *, Object **,
		     const RecMode *, const ObjectHeader *);
    friend Status
    agregatLoad(Database *, const Oid *, Object **,
		const RecMode *, const ObjectHeader *);

    Bool indexPrologue(Database *db, Data _idr, Bool novd,
		       int &count, Data &pdata, Size &varsize,
		       Bool create);

    Status getClassOid(Database *db, const Class *cls,
		       const Oid &oidcls, Oid &oid);
    Status getAttrComponents(Database *db, const Class *,
			     LinkedList &);
    void codeClassOid(Data, Offset *);
    virtual void newObjRealize(Object *) const;
    virtual int getBound(Database *, Data);
    virtual void getData(const Database *, Data, Data&, Data&) const;
    virtual void getData(const Object *, Data&, Data&) const;
    virtual void getVarDimOid(Data, Oid *) const;
    ClassComponent *getComp(Class::CompIdx idx,
			    Bool (*pred)(const ClassComponent *,
					 void *),
			    void *client_data) const;

    virtual void manageCycle(Database *db, Object *o, gbxCycleContext &r) const;

    Status loadComponentSet(Database *db, Bool create) const;
    static Status getAttrComp(Database *db, const char *clsname,
			      const char *attrpath, Object *&);

  protected:
    struct invSpec {
      Oid oid_cl;
      eyedblib::int16 num;
      const Attribute *item;
      char *clsname, *fname;

      invSpec() {
	oid_cl.invalidate();
	num = 0;
	item = 0;
	clsname = fname = 0;
      }

      invSpec(const invSpec &inv) {
	*this = inv;
      }

      invSpec& operator=(const invSpec &inv) {
	oid_cl = inv.oid_cl;
	num = inv.num;
	item = inv.item;
	clsname = (inv.clsname ? strdup(inv.clsname) : 0);
	fname = (inv.fname ? strdup(inv.fname) : 0);
	return *this;
      }

      ~invSpec() {
	free(clsname);
	free(fname);
      }
    } inv_spec;

    struct InvCtx {
      Oid oid;
      Data idr;

      InvCtx() : idr(0) { }
      InvCtx(const Oid &_oid, Data _idr) : oid(_oid), idr(_idr) { }
    };

    Status constraintPrologue(Database *db,
			      const AttrIdxContext &idx_ctx,
			      Bool &notnull_comp, Bool &notnull,
			      Bool &unique_comp, Bool &unique) const;
    Status collimplPrologue(Database *db,
			    const AttrIdxContext &idx_ctx,
			    CollAttrImpl *&) const;

    // ----------------------------------------------------------------------
    // Attribute Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public: 
    enum InvObjOp {
      invObjCreate = 1,
      invObjUpdate,
      invObjRemove
    };

    Status inverse_coll_perform(Database *, InvObjOp,
				const Oid &, const Oid &) const;

    void setNum(int _num) {num = _num;}
    void revert(Bool);
    static Status createEntries(Database *db, const Oid &oid,
				Object *o,
				AttrIdxContext &,
				Attribute *attrs[],
				int depth, int last,
				unsigned char entry[], Index *);
    static Status createEntries_realize(Database *db,
					Attribute *attr,
					const Oid &oid,
					Object *o,
					AttrIdxContext &idx_ctx,
					unsigned char entry[],
					Index *idx);
    Status completeInverse(Schema *);
    void setMagOrder(unsigned int _magorder) {magorder = _magorder;}
    Status updateIndexForInverse(Database *db, const Oid &obj_oid,
				 const Oid &new_oid) const;
    static const char *template_name;
    Oid getAttrCompSetOid() const {return attr_comp_set_oid;}
    void setAttrCompSetOid(Oid _attr_comp_set_oid) {
      attr_comp_set_oid = _attr_comp_set_oid;
    }

    Bool compare(Database *, const Attribute *,
		 Bool compClassOwner,
		 Bool compNum,
		 Bool compName,
		 Bool inDepth) const;

    static Status openMultiIndexRealize(Database *db, Index *);

    const char *dumpData(Data);
    static const char *log_item_entry_fmt;
    static const char *log_comp_entry_fmt;
    virtual Status convert(Database *db, ClassConversion *,
			   Data in_idr, Size in_size) const;

    Status clean(Database *db);
    virtual void reportAttrCompSetOid(Offset *offset, Data idr) const;

    void pre_release();

  private:
    Status clean_realize(Schema *, const Class *&);
    Status createComponentSet(Database *);

    Status hasIndex(Database *db, bool &has_index, std::string &idx_str) const;

    void completeInverseItem(Schema *);
    Status completeInverse(Database *);
    Status checkInverse(const Attribute * = 0) const;

    Status inverse_1_1(Database *, InvObjOp,
		       const Attribute *, const Oid &,
		       const Oid &,
		       const InvCtx &) const;
    Status inverse_1_N(Database *, InvObjOp,
		       const Attribute *, const Oid &,
		       const Oid &,
		       const InvCtx &) const;
    Status inverse_N_1(Database *,  InvObjOp,
		       const Attribute *, const Oid &,
		       const Oid &,
		       const InvCtx &) const;
    Status inverse_N_N(Database *,  InvObjOp,
		       const Attribute *, const Oid &,
		       const Oid &,
		       const InvCtx &) const;
    Status inverse_update(Database *,
			  const Attribute *, const Oid &,
			  const Oid &) const;
    Status inverse_realize(Database *, InvObjOp, Data, 
			   const Oid &) const;

    Status inverse_get_collection(Database *db,
				  const Oid &inv_obj_oid,
				  Collection*& coll) const;

    Oid inverse_get_inv_obj_oid(Data) const;

    static Status inverse_read_oid(Database *db, const Attribute *item,
				   const Oid &obj_oid,
				   Oid &old_obj_oid);
    static Status inverse_write_oid(Database *db, const Attribute *item,
				    const Oid &obj_oid,
				    const Oid &new_obj_oid,
				    const InvCtx &);

    Status inverse_create_collection(Database *db,
				     const Attribute *inv_item,
				     const Oid &obj_oid,
				     Bool is_N_N,
				     const Oid &master_oid,
				     Collection *&coll) const;

    Status inverse_get_inv_collection(Database *db,
				      const Attribute *inv_item,
				      const Oid &inv_obj_oid, 
				      const Oid &obj_oid, 
				      Bool is_N_N,
				      Collection *&coll) const;
    Status inverse_coll_perform(Database *db, InvObjOp op,
				const Attribute *inv_item,
				Collection *coll,
				const Oid &obj_oid) const;

    Status inverse_coll_perform_N_1(Database *, InvObjOp,
				    const Oid &, const Oid &) const;

    Status inverse_coll_perform_N_N(Database *, InvObjOp,
				    const Oid &, const Oid &) const;

    Status makeClassSequence(Database *db, Index *idx,
			     ClassSequence& seq);

    Status realizeIndexes(ClassSequence& seq, int, Bool);


    void directFind(Class *rootcl, Class *cl, ClassSeqItem *clitem,
		    ClassSequence& seq, AttrStack&);

    void makeClassSequenceRealize(Class *cl, ClassSequence& seq);
  };

  extern
  Attribute *makeAttribute(const Attribute *, const Class *,
			   const Class *, const Class *, int);
  extern
  Attribute *makeAttribute(Database *db, Data, Offset *,
			   const Class *, int);

  /**
     @}
  */

}

#endif
