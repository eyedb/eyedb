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


#ifndef _EYEDB_CLASS_H
#define _EYEDB_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  typedef eyedbsm::Idx Idx;

  class Instance;
  class Schema;
  class Collection;
  class ClassPeer;
  class ClassComponent;
  class Trigger;
  class GenCodeHints;
  class Signature;
  class ClassVariable;
  class GenContext;
  class AttributeComponent;
  class IndexImpl;

  /**
     Not yet documented.
  */
  class Class : public Object {

    // ----------------------------------------------------------------------
    // Class Interface
    // ----------------------------------------------------------------------
  public:
    enum MType {
      System = 1,
      User = 2
    };

    /**
       Not yet documented
       @param s
       @param p
    */
    Class(const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param s
       @param poid
    */
    Class(const char *s, const Oid *poid);

    /**
       Not yet documented
       @param db
       @param s
       @param p
    */
    Class(Database *db, const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param db
       @param s
       @param poid
    */
    Class(Database *db, const char *s, const Oid *poid);

    /**
       Not yet documented
       @param cl
    */
    Class(const Class &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Class(*this);}

    /**
       Not yet documented
       @param cl
       @return
    */
    Class& operator=(const Class &cl);

    /**
       Not yet documented
       @return
    */
    const char *getName() const {return name;}

    /**
       Not yet documented
       @param useAsRef
       @return
    */
    virtual const char *getCName(Bool useAsRef = False) const;

    /**
       Not yet documented
       @return
    */
    unsigned int getNum() const {return num;};

    /**
       Not yet documented
       @param s
       @return
    */
    virtual Status setName(const char *s);

    /**
       Not yet documented
       @return
    */
    Class *getParent();

    /**
       Not yet documented
       @return
    */
    const Class *getParent() const;

    /**
       Not yet documented
       @param db
       @param rparent
       @return
    */
    Status getParent(Database *db, Class *&rparent);

    /**
       Not yet documented
       @return
    */
    Schema *getSchema();

    /**
       Not yet documented
       @return
    */
    const Schema *getSchema() const;

    /**
       Not yet documented
       @param mdb
       @return
    */
    virtual Status setDatabase(Database *mdb);

    /**
       Not yet documented
       @param extent
       @param reload
       @return
    */
    Status getExtent(Collection *&extent, Bool reload = False) const;

    /**
       Not yet documented
       @param components
       @param reload
       @return
    */
    Status getComponents(Collection *&components, Bool reload = False) const;

    /**
       Not yet documented
       @param idximpl
       @return
    */
    Status setExtentImplementation(const IndexImpl *idximpl);

    /**
       Not yet documented
       @return
    */
    IndexImpl *getExtentImplementation() const;

    /**
       Not yet documented
       @return
    */
    Size getIDRObjectSize() const {return idr_objsz;}

    /**
       Not yet documented
       @param psize
       @param vsize
       @param isize
       @return
    */
    Size getIDRObjectSize(Size *psize, Size *vsize = 0, Size *isize = 0) const;

    /**
       Not yet documented
       @param comp
       @param incrRefCount
       @return
    */
    Status add(ClassComponent *comp, Bool incrRefCount = True);

    /**
       Not yet documented
       @param w
       @param comp
       @param incrRefCount
       @return
    */
    Status add(unsigned int w, ClassComponent * comp, Bool incrRefCount = True);

    /**
       Not yet documented
       @param w
       @param comp
       @return
    */
    Status add(unsigned int w, AttributeComponent *comp);

    /**
       Not yet documented
       @param w
       @param comp
       @return
    */
    Status suppress(unsigned int w, ClassComponent *comp);

    /**
       Not yet documented
       @param w
       @param comp
       @return
    */
    Status suppress(unsigned int w, AttributeComponent *comp);

    /**
       Not yet documented
       @param comp
       @return
    */
    Status suppress(ClassComponent *comp);

    /**
       Not yet documented
       @param instance_dataspace
       @return
    */
    Status getDefaultInstanceDataspace(const Dataspace *&instance_dataspace) const;

    /**
       Not yet documented
       @param instance_dataspace
       @return
    */
    Status setDefaultInstanceDataspace(const Dataspace *instance_dataspace);

    /**
       Not yet documented
       @param locarr
       @param include_subclasses
       @return
    */
    Status getInstanceLocations(ObjectLocationArray &locarr,
				Bool include_subclasses = False);

    /**
       Not yet documented
       @param dataspace
       @param include_subclasses
       @return
    */
    Status moveInstances(const Dataspace *dataspace,
			 Bool include_subclasses = False);

    /**
       Not yet documented
       @return
    */
    Bool isSystem() const {return (m_type == System) ? True : False;}

    /**
       Not yet documented
       @return
    */
    Class::MType getMType() const {return m_type;}

    /**
       Not yet documented
       @return
    */
    virtual Class *asClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Class *asClass() const {return this;}

    Status setValue(Data);
    Status getValue(Data *) const;

    virtual Object * newObj(Database * = NULL) const;
    virtual Object * newObj(Data, Bool _copy = True) const;

    /**
       Not yet documented
       @return
    */
    virtual Status create();

    /**
       Not yet documented
       @return
    */
    virtual Status update();

    virtual Status remove(const RecMode* = RecMode::NoRecurs);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    virtual Status trace(FILE *fd = stdout, unsigned int flags = 0,
			 const RecMode *recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented
       @param db
       @param cl_oid
       @param oid
       @return
    */
    static Bool isClass(Database *db, const Oid& cl_oid, const Oid& oid);

    /**
       Not yet documented
       @param cl
       @return
    */
    Bool compare(const Class *cl) const;

    /**
       Not yet documented
       @param cl
       @return
    */
    Bool compare_l(const Class *cl) const;

    /**
       Not yet documented
       @param cl
       @param is
       @return
    */
    Status isSuperClassOf(const Class *cl, Bool *is) const;

    /**
       Not yet documented
       @param cl
       @param is
       @return
    */
    Status isSubClassOf(const Class *cl, Bool *is) const;

    /**
       Not yet documented
       @param subclasses
       @param subclass_count
       @param sort_down_to_top
       @return
    */
    Status getSubClasses(Class **&subclasses, unsigned int &subclass_count,
			 Bool sort_down_to_top = True) const;

    /**
       Not yet documented
       @param o
       @param is
       @param issub
       @return
    */
    Status isObjectOfClass(const Object *o, Bool *is, Bool issub) const;

    /**
       Not yet documented
       @param o_oid
       @param is
       @param issub
       @param po_class
       @return
    */
    Status isObjectOfClass(const Oid *o_oid, Bool *is, Bool issub,
			   Class **po_class = NULL) const;

    /**
       Not yet documented
       @return
    */
    unsigned int getAttributesCount(void) const;

    /**
       Not yet documented
       @param n
       @return
    */
    const Attribute *getAttribute(unsigned int n) const;

    /**
       Not yet documented
       @param nm
       @return
    */
    const Attribute *getAttribute(const char *nm) const;

    /**
       Not yet documented
       @param agr
       @param base_n
       @return
    */
    virtual Status setAttributes(Attribute **agr, unsigned int base_n);

    /**
       Not yet documented
       @param cnt
       @return
    */
    inline const Attribute **getAttributes(unsigned int& cnt) const {
      cnt = items_cnt;
      return (const Attribute **)items;
    }

    /**
       Not yet documented
       @return
    */
    inline const Attribute **getAttributes() const {
      return (const Attribute **)items;
    }

    /**
       Not yet documented
       @param mcname
       @param comp
       @return
    */
    Status getComp(const char *mcname, ClassComponent *&comp) const;

    /**
       Not yet documented
       @param mth_cnt
       @return
    */
    Method **getMethods(unsigned int& mth_cnt);

    /**
       Not yet documented
       @param mth_cnt
       @return
    */
    const Method **getMethods(unsigned int& mth_cnt) const;

    /**
       Not yet documented
       @param name
       @param mth
       @param sign
       @return
    */
    Status getMethod(const char *name, Method *&mth,
		     Signature *sign = 0);

    /**
       Not yet documented
       @param name
       @param mth
       @param sign
       @return
    */
    Status getMethod(const char *name, const Method *&mth,
		     Signature *sign = 0) const;

    /**
       Not yet documented
       @return
    */
    unsigned int getMethodCount() const;

    /**
       Not yet documented
       @param name
       @param cnt
       @return
    */
    Status getMethodCount(const char *name, unsigned int &cnt) const;

    /**
       Not yet documented
       @param cnt
       @return
    */
    Trigger **getTriggers(unsigned int& cnt);

    /**
       Not yet documented
       @param cnt
       @return
    */
    const Trigger **getTriggers(unsigned int& cnt) const;

    /**
       Not yet documented
       @param cnt
       @return
    */
    ClassVariable **getVariables(unsigned int& cnt);

    /**
       Not yet documented
       @param cnt
       @return
    */
    const ClassVariable **getVariables(unsigned int& cnt) const;

    /**
       Not yet documented
       @param name
       @param rvar
       @return
    */
    Status getVariable(const char *name, ClassVariable *&rvar);

    /**
       Not yet documented
       @param name
       @param rvar
       @return
    */
    Status getVariable(const char *name, const ClassVariable *&rvar) const;

    virtual Status generateCode_C(Schema *,
				  const char *prefix,
				  const GenCodeHints &,
				  const char *stubs,
				  FILE *fdh, FILE *fdc, FILE *, FILE *,
				  FILE *, FILE *);

    virtual Status generateCode_Java(Schema *,
				     const char *prefix,
				     const GenCodeHints &,
				     FILE *);

    virtual Status generateMethodBody_C(Schema *, GenContext *,
					GenContext *, GenContext *,
					GenContext *, GenContext *);

    virtual Status generateMethodBodyBE_C(Schema *, GenContext *,
					  GenContext *, Method *);
  
    virtual Status generateTriggerBody_C(Schema *m, GenContext *,
					 Trigger *);

    virtual Status generateMethodBodyFE_C(Schema *, GenContext *,
					  Method *);

    virtual Status generateClassComponent_C(GenContext *);

    virtual Status generateAttrComponent_C(GenContext *);

    virtual Status checkInverse(const Schema *) const;

    enum CompIdx {
      Variable_C,
      Method_C,
      TrigCreateBefore_C,
      TrigCreateAfter_C,
      TrigUpdateBefore_C,
      TrigUpdateAfter_C,
      TrigLoadBefore_C,
      TrigLoadAfter_C,
      TrigRemoveBefore_C,
      TrigRemoveAfter_C,
      ComponentCount_C
    };

    /**
       Not yet documented
       @return
    */
    const LinkedList *getCompList() const;

    /**
       Not yet documented
       @param idx
       @return
    */
    const LinkedList *getCompList(CompIdx idx) const;

    enum AttrCompIdx {
      UniqueConstraint_C,
      NotnullConstraint_C,
      CardinalityConstraint_C,
      Index_C,
      CollectionImpl_C,
      AttrComponentCount_C
    };

    /**
       Not yet documented
       @param mcname
       @param comp
       @return
    */
    Status getAttrComp(const char *mcname, AttributeComponent *&comp) const;

    /**
       Not yet documented
       @param list
       @return
    */
    Status getAttrCompList(const LinkedList *&list);

    /**
       Not yet documented
       @param idx
       @param list
       @return
    */
    Status getAttrCompList(AttrCompIdx idx, const LinkedList *&list);

    /**
       Not yet documented
       @param fd
       @param m
       @return
    */
    virtual int genODL(FILE *fd, Schema *m) const;

    /**
       Not yet documented
       @return
    */
    Bool isFlatStructure() const;

    /**
       Not yet documented
    */
    virtual ~Class();

    // ----------------------------------------------------------------------
    // Class Protected Part
    // ----------------------------------------------------------------------
  protected:
    Class *parent;
    char *name;
    char *aliasname;
    char *canonname;
    const Dataspace *instance_dataspace;
    short instance_dspid;
    Bool mustCreateComps;
    IndexImpl *idximpl;

    Size idr_psize, idr_vsize, idr_inisize;
    Size idr_objsz;

    Bool attrs_complete;
    Oid parent_oid;
    Oid coll_oid;
    Collection *extent;
    Collection *components;
    LinkedList *complist, *clist[ComponentCount_C];
    LinkedList *attr_complist, *attr_clist[AttrComponentCount_C];

    MType m_type;

    //virtual Bool compare_perform(const Class *) const;
    virtual Bool compare_perform(const Class *cl,
				 Bool compClassOwner,
				 Bool compNum,
				 Bool compName,
				 Bool inDepth) const;
    virtual void garbage();
    Status setNameRealize(const char *);
    Status trace_comps(FILE *, int, unsigned int, const RecMode *) const;
    Status trace_common(FILE *, int, unsigned int, const RecMode *) const;
    Bool items_set;
    unsigned int items_cnt;
    Attribute ** items;
    void free_items();
    virtual Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status check_items(Attribute **, int);
    Bool isFlat;
    Bool isFlatSet;
    void setPName(const char *);
    void codeExtentCompOids(Size);

    // ----------------------------------------------------------------------
    // Class Private Part
    // ----------------------------------------------------------------------
  private:
    Bool is_root;
    char *tied_code;
    Bool sort_down_to_top;
    Bool subclass_set;
    Class **subclasses;
    unsigned int subclass_count;
    Schema *sch;
    unsigned int num;
    Oid extent_oid, comp_oid;
    Bool partially_loaded;
    Bool setup_complete;

    friend class Schema;
    friend class ClassPeer;

    virtual Status realize(const RecMode* = RecMode::NoRecurs);
    void _init(const char *);
    virtual void newObjRealize(Object *) const;
    Status setupInherit();
    Status scanComponents();
    Status triggerManage(Trigger *);
    Status sort(Bool sort_down_to_top) const;
    void setNum(unsigned int _num) {num = _num;};

    void genODL(FILE *fd, Schema *m, Attribute *attr) const;
    Status makeAttrCompList();
    void compAddGenerate(GenContext *ctx, FILE *fd);
    void pre_release();

    // ----------------------------------------------------------------------
    // Class Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init(void);
    static void _release(void);
    static int RemoveInstances;
    Status manageDataspace(short dspid);
    void setInstanceDspid(short dspid);
    short get_instdspid() const;
    void setSchema(Schema *_sch) {sch = _sch;}
    virtual Status compile(void);
    const char *getAliasName() const {return (aliasname ? aliasname : name);}
    const char *getStrictAliasName() const {return aliasname;}
    void setAliasName(const char *_aliasname) {
      free(aliasname);
      aliasname = (_aliasname ? strdup(_aliasname) : 0);
    }
    const char *getCanonicalName() const {return (canonname ? canonname : name);}
    void setCanonicalName(const char *_canonname) {
      free(canonname);
      canonname = (_canonname ? strdup(_canonname) : 0);
    }
    Status setInSubClasses(ClassComponent *, Bool);
    unsigned int getMagorder() const;

    void unmakeAttrCompList();
    static const char *getSCName(const char *);
    static Status makeClass(Database *db, const Oid &oid,
			    int, const char *, Bool &, Class *&cl);
    virtual Status postCreate();
    Bool isRootClass() const {return is_root;}
    void setIsRootClass() {is_root = True;}
    Status setup(Bool, Bool = False);
    static const char *classNameToCName(const char *name);
    virtual void revert(Bool) { }
    void setTiedCode(char *);
    char *getTiedCode();
    virtual Status createComps();
    void setExtentCompOid(const Oid &, const Oid &);
    void setPartiallyLoaded(Bool _partially_loaded) {partially_loaded = _partially_loaded;}
    void setExtentImplementation(const IndexImpl *, Bool);

    static EnumClass *makeBoolClass();
    static Bool isBoolClass(const char *);
    static Bool isBoolClass(const Class *cls) {
      return isBoolClass(cls->getName());
    }
  
    Bool compare(const Class *cl,
		 Bool compClassOwner,
		 Bool compNum,
		 Bool compName,
		 Bool inDepth) const;

    // completion management
    Bool isAttrsComplete() const {return attrs_complete;}
    Bool isSetupComplete() const {return setup_complete;}
    Bool isPartiallyLoaded() const {return partially_loaded;}

    virtual Status attrsComplete();
    Status setupComplete();
    virtual Status loadComplete(const Class *);
    Status wholeComplete();

    void setSetupComplete(Bool _setup_complete) {
      setup_complete = _setup_complete;
    }

    Class(const Oid&, const char *);
    Status clean(Database *db);

    // XDR
    virtual void decode(void * hdata, // to
			const void * xdata, // from
			Size incsize, // temporarely necessary 
			unsigned int nb = 1) const;

    virtual void encode(void * xdata, // to
			const void * hdata, // from
			Size incsize, // temporarely necessary 
			unsigned int nb = 1) const;

    virtual int cmp(const void * xdata,
		    const void * hdata,
		    Size incsize, // temporarely necessary 
		    unsigned int nb = 1) const;
  };

  class ClassPtr : public ObjectPtr {

  public:
    ClassPtr(Class *o = 0) : ObjectPtr(o) { }

    Class *getClass() {return dynamic_cast<Class *>(o);}
    const Class *getClass() const {return dynamic_cast<Class *>(o);}

    Class *operator->() {return dynamic_cast<Class *>(o);}
    const Class *operator->() const {return dynamic_cast<Class *>(o);}
  };

  typedef std::vector<ClassPtr> ClassPtrVector;

  extern Class
  *Object_Class,

    *Class_Class,
    *BasicClass_Class,
    *EnumClass_Class,
    *AgregatClass_Class,
    *StructClass_Class,
    *UnionClass_Class,

    *Instance_Class,
    *Basic_Class,
    *Enum_Class,
    *Agregat_Class,
    *Struct_Class,
    *Union_Class,
    *Schema_Class,
    *Bool_Class,

    *CollectionClass_Class,
    *CollSetClass_Class,
    *CollBagClass_Class,
    *CollListClass_Class,
    *CollArrayClass_Class,

    *Collection_Class,
    *CollSet_Class,
    *CollBag_Class,
    *CollList_Class,
    *CollArray_Class;

  class AttrNative;

  extern struct InstanceInfo {
    const char *name;
    int items_cnt;
    AttrNative **items;
  } class_info[];

  /**
     @}
  */

}

#endif
