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


#ifndef _EYEDB_AGREGAT_CLASS_H
#define _EYEDB_AGREGAT_CLASS_H

namespace eyedb {

  class AttrIdxContext;

  /**
     @addtogroup eyedb 
     @{
  */
  
  /**
     Not yet documented.
  */
  class AgregatClass : public Class {

    // ----------------------------------------------------------------------
    // AgregatClass Interface
    // ----------------------------------------------------------------------

  public:
    /**
       Not yet documented.
       @param s
       @param p
    */
    AgregatClass(const char *s, Class* p = NULL);

    /**
       Not yet documented.
       @param s
       @param poid
    */
    AgregatClass(const char *s, const Oid *poid);

    /**
       Not yet documented.
       @param db 
       @param s 
       @param p 
    */
    AgregatClass(Database *db, const char *s, Class* p = NULL);

    /**
       Not yet documented.
       @param db 
       @param s 
       @param poid 
    */
    AgregatClass(Database *db, const char *s, const Oid *poid);

    /**
       Not yet documented.
       @param cl 
    */
    AgregatClass(const AgregatClass &cl);

    /**
       Not yet documented.
       @param cl
       @return
    */
    AgregatClass& operator=(const AgregatClass &cl);

    /**
       Not yet documented.
       @return
    */
    virtual Object *clone() const {return new AgregatClass(*this);}

    /**
       Not yet documented.
       @return
    */
    Status attrsComplete();

    /**
       Not yet documented.
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE* fd = stdout, unsigned int flags = 0,
		 const RecMode * recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented.
       @param mdb
       @return
    */
    virtual Status setDatabase(Database *mdb);

    /**
       Not yet documented.
       @param s
       @return
    */
    virtual Status setName(const char *s);

    /**
       Not yet documented.
    */
    void touch();

    /**
       Not yet documented.
       @return
    */
    virtual AgregatClass *asAgregatClass() {return this;}

    /**
       Not yet documented.
       @return
    */
    virtual const AgregatClass *asAgregatClass() const {return this;}

    /**
     */
    virtual ~AgregatClass();

    // ----------------------------------------------------------------------
    // AgregatClass Protected Part
    // ----------------------------------------------------------------------
  protected:
    static const unsigned int IndirectSize;

    void newObjRealize(Object *) const;
    virtual void garbage();

    // ----------------------------------------------------------------------
    // AgregatClass Private Part
    // ----------------------------------------------------------------------
  private:
    Offset post_create_offset;
    friend Status
    agregatClassMake(Database *, const Oid *, Object **,
		     const RecMode *, const ObjectHeader *,
		     Data, LockMode, const Class *);
    Status create();
    Status update();

    Status remove(const RecMode* = RecMode::NoRecurs);
    virtual Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status generateCode_C(Schema *, const char *prefix,
			  const GenCodeHints &,
			  const char *stubs,
			  FILE *, FILE *, FILE *, FILE *, FILE *, FILE *);
    Status generateConstructors_C(GenContext *);
    Status generateConstructors_C(GenContext *, GenContext *);
    Status generateDownCasting_C(GenContext *, Schema *m);
    Status generateClassDesc_C(GenContext *, const char *);

    Status generateMethodDecl_C(Schema *, GenContext *);

    Status generateMethodBodyFE_C(Schema *, GenContext *,
				  Method *);
    Status generateMethodFetch_C(GenContext *, Method *);

    Status generateCode_Java(Schema *, const char *prefix, 
			     const GenCodeHints &, FILE *);
    Status generateConstructors_Java(GenContext *);
    Status generateClassDesc_Java(GenContext *, const char *);
    Status generateClassComponent_Java(GenContext *, GenContext *,
				       GenContext *);

    Status checkInversePath(const Schema *m,
			    const Attribute *item,
			    const Attribute *&invitem, Bool) const;

    void _init(Class *);

    // ----------------------------------------------------------------------
    // AgregatClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init();
    static void _release();
    Status completeInverse(Schema *m);
    void _setCSDRSize(Size, Size);

    Status setValue(Data);
    Status getValue(Data*) const;
    Status postCreate();
    Status compile(void);

    // FE
    Status createIndexes(void);
    //Bool compare_perform(const Class *) const;
    Bool compare_perform(const Class *cl,
			 Bool compClassOwner,
			 Bool compNum,
			 Bool compName,
			 Bool inDepth) const;

    // BE
    Status openIndexes_realize(Database *db);

    Status createIndexes_realize(Database *db);

    Status createNestedIndex(AttrIdxContext &attr_idx_ctx,
			     const AttrIdxContext *tg_idx_ctx, int);
    Status removeNestedIndex(AttrIdxContext &attr_idx_ctx,
			     const AttrIdxContext *tg_idx_ctx, int);

    Status createIndexEntries_realize(Database *db, Data,
				      const Oid *,
				      AttrIdxContext &,
				      const Oid * = NULL,
				      int = 0, Bool = True, int = 0,
				      int = -1);
    Status updateIndexEntries_realize(Database *db, Data,
				      const Oid *,
				      AttrIdxContext &,
				      const Oid * = NULL,
				      int = 0, Bool = True,
				      const Oid * = NULL,
				      int = 0);
    Status removeIndexEntries_realize(Database *db, Data,
				      const Oid *,
				      AttrIdxContext &,
				      const Oid * = NULL,
				      int = 0, Bool = True,
				      const Oid * = NULL, int = 0);

    Status createInverses_realize(Database *, Data, const Oid *);
    Status updateInverses_realize(Database *, Data, const Oid *);
    Status removeInverses_realize(Database *, Data, const Oid *);
    Status checkInverse(const Schema *) const;
    void revert(Bool);

    // ----------------------------------------------------------------------
    // AgregatClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    AgregatClass(const Oid&, const char *);
  };

  class AgregatClassPtr : public ClassPtr {

  public:
    AgregatClassPtr(AgregatClass *o = 0) : ClassPtr(o) { }

    AgregatClass *getAgregatClass() {return dynamic_cast<AgregatClass *>(o);}
    const AgregatClass *getAgregatClass() const {return dynamic_cast<AgregatClass *>(o);}

    AgregatClass *operator->() {return dynamic_cast<AgregatClass *>(o);}
    const AgregatClass *operator->() const {return dynamic_cast<AgregatClass *>(o);}
  };

  typedef std::vector<AgregatClassPtr> AgregatClassPtrVector;

  /**
     @}
  */

}

#endif
