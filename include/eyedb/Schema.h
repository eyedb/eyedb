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


#ifndef _EYEDB_SCHEMA_H
#define _EYEDB_SCHEMA_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class SchemaHashList;
  class SchemaHashTable;

  enum ProgLang {
    ProgLang_C = 1,
    ProgLang_Java
  };

  /**
     Not yet documented.
  */
  class Schema : public Instance {

    // ----------------------------------------------------------------------
    // Schema Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
    */
    Schema();

    /**
       Not yet documented
       @param sch
    */
    Schema(const Schema &sch);

    /**
       Not yet documented
       @param sch
       @return
    */
    Schema& operator=(const Schema &sch);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Schema(*this);}

    /**
       Not yet documented
       @return
    */
    const LinkedList *getClassList() const;


    /**
       Not yet documented
       @param poid
       @param perform_load
       @return
    */
    Class *getClass(const Oid &poid, Bool perform_load = False);

    /**
       Not yet documented
       @param name
       @return
    */
    Class *getClass(const char *name);

    /**
       Not yet documented
       @param num
       @return
    */
    Class *getClass(int num);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE *fd = stdout, unsigned int flags = 0,
		 const RecMode *recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented
       @return
    */
    Status update();

    /**
       Not yet documented
       @param recmode
       @return
    */
    Status realize(const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @return
    */
    Status storeName();


    /**
       Not yet documented
       @param data
       @return
    */
    Status setValue(Data data);

    /**
       Not yet documented
       @param data
       @return
    */
    Status getValue(Data *data) const;

    /**
       Not yet documented
       @param database
       @param create
       @return
    */
    Status init(Database *database = NULL, Bool create = False);

    /**
       Not yet documented
       @param mc
       @return
    */
    Status addClass(Class *mc);

    /**
       Not yet documented
       @param mc
       @param atall
       @return
    */
    Status addClass_nocheck(Class *mc, Bool atall = False);

    /**
       Not yet documented
       @param mc
       @return
    */
    Status suppressClass(Class *mc);

    /**
       Not yet documented
       @param lang
       @param package
       @param schname
       @param c_namespace
       @param prefix
       @param db_prefix
       @param hints
       @param _export
       @param superclass
       @param incl_file_list
       @return
    */
    Status generateCode(ProgLang lang,
			const char *package, const char *schname,
			const char *c_namespace,
			const char *prefix,
			const char *db_prefix,
			const GenCodeHints &hints,
			Bool _export,
			Class *superclass,
			LinkedList *incl_file_list = 0);

    /**
       Not yet documented
       @param setup
       @param force
       @return
    */
    Status complete(Bool setup, Bool force = False);

    /**
       Not yet documented
       @param reload
       @return
    */
    Status setup(Bool reload);

    /**
       Not yet documented
       @return
    */
    const char *getName() const {return name;}

    /**
       Not yet documented
       @param _name
    */
    void setName(const char *_name);

    /**
       Not yet documented
       @return
    */
    virtual void garbage();

    Status deferredCollRegisterRealize(DbHandle *);

    /**
       Not yet documented
       @param clname
       @param oid
    */
    void deferredCollRegister(const char *clname, const eyedbsm::Oid *oid);

    /**
       Not yet documented
       @return
    */
    virtual Schema *asSchema() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Schema *asSchema() const {return this;}


    /**
       Not yet documented
       @param fd
       @param flags
    */
    void genODL(FILE *fd, unsigned int flags = 0) const;

    Class
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

    CharClass    *Char_Class;
    ByteClass    *Byte_Class;
    OidClass    *OidP_Class;
    Int16Class   *Int16_Class;
    Int32Class   *Int32_Class;
    Int64Class   *Int64_Class;
    FloatClass   *Float_Class;

    static Status displaySchemaDiff(Database *db, const char *odlfile,
				    const char *package,
				    const char *db_prefix = 0,
				    FILE *fd = stdout,
				    const char *cpp_cmd = 0,
				    const char *cpp_flags = 0);

#ifdef SUPPORT_CORBA
    static Status genIDL(Database *db, const char *odlfile,
			 const char *package,
			 const char *module,
			 const char *schname,
			 const char *prefix,
			 const char *db_prefix,
			 const char *idltarget,
			 const char *imdlfile,
			 Bool no_generic_idl,
			 const char *generic_idl,
			 Bool no_factory,
			 Bool imdl_synchro,
			 GenCodeHints *hints = 0,
			 Bool comments = True,
			 const char *cpp_cmd=0, const char *cpp_flags=0,
			 const char *ofile = 0);

    static Status genORB(const char *orb,
			 Database *db, const char *odlfile,
			 const char *package,
			 const char *module,
			 const char *schname,
			 const char *prefix,
			 const char *db_prefix,
			 const char *idltarget,
			 const char *imdlfile,
			 Bool no_generic_idl,
			 const char *generic_idl,
			 Bool no_factory,
			 Bool imdl_synchro,
			 GenCodeHints *hints = 0,
			 Bool comments = True,
			 const char *cpp_cmd=0, const char *cpp_flags=0);
#endif

    static Status genC_API(Database *db, const char *odlfile,
			   const char *package,
			   const char *schname = 0,
			   const char *c_namespace = 0,
			   const char *prefix = 0,
			   const char *db_prefix = 0,
			   Bool _export = False,
			   GenCodeHints *hints = 0,
			   const char *cpp_cmd=0, const char *cpp_flags=0);

    static Status genJava_API(Database *db, const char *odlfile,
			      const char *package,
			      const char *schname = 0,
			      const char *prefix = 0,
			      const char *db_prefix = 0,
			      Bool _export = False,
			      GenCodeHints *hints = 0,
			      const char *cpp_cmd=0, const char *cpp_flags=0);

    static Status genODL(Database *db,
			 const char *odlfile,
			 const char *package,
			 const char *schname = 0,
			 const char *prefix = 0,
			 const char *db_prefix = 0,
			 const char *ofile = 0,
			 const char *cpp_cmd=0, const char *cpp_flags=0);

    static Status checkODL(const char *odlfile, const char *package,
			   const char *cpp_cmd=0, const char *cpp_flags=0);

    virtual ~Schema();

    // ----------------------------------------------------------------------
    // Schema Private Part
    // ----------------------------------------------------------------------
  private:
    LinkedList *_class;

    SchemaHashTable *hash;
    char *name;
    Bool reversal;
    int class_cnt;
    Class **classes;

    Status create();
    Status remove(const RecMode* = RecMode::NoRecurs);
    void sort_classes();
    void sort_realize(const Class *, LinkedList *);
    void postComplete();

    friend class BasicClass;
    friend class AgregatClass;
    friend Status
    schemaClassLoad(Database *, const Oid *, Object **,
		    const RecMode *, const ObjectHeader *);
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    void class_make(Class **, int, Class *);
    void bool_class_make(Class **);
    LinkedList *deferred_list;
    void hashTableInvalidate();

    const char *generateStubs_C(Bool, Class *, const char *,
				const char *, const GenCodeHints &hints);

    Status generateCode_C(const char *package, const char *schname,
			  const char *c_namespace,
			  const char *prefix,
			  const char *db_prefix,
			  const GenCodeHints &,
			  Bool _export,
			  Class *,
			  LinkedList *incl_file_list = 0);

    const char *generateStubs_Java(Bool, Class *, const char *,
				   const char *);

    Status generateCode_Java(const char *package, const char *schname,
			     const char *prefix,
			     const char *db_prefix,
			     const GenCodeHints &,
			     Bool _export,
			     Class *,
			     LinkedList *incl_file_list = 0);

    //Status completeIndexes();

    // ----------------------------------------------------------------------
    // Schema Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    void purge();
    void computeHashTable();
    void setReversal(Bool on_off);
    Bool isReversalSet() const;
    void revert(Bool rev);
    Status manageClassDeferred(Class *);
    Bool dont_delete_comps;
    Status checkDuplicates();
    Bool checkClass(const Class *cl);
    Status clean(Database *db);
  };

  class SchemaPtr : public InstancePtr {

  public:
    SchemaPtr(Schema *o = 0) : InstancePtr(o) { }

    Schema *getSchema() {return dynamic_cast<Schema *>(o);}
    const Schema *getSchema() const {return dynamic_cast<Schema *>(o);}

    Schema *operator->() {return dynamic_cast<Schema *>(o);}
    const Schema *operator->() const {return dynamic_cast<Schema *>(o);}
  };

  typedef std::vector<SchemaPtr> SchemaPtrVector;

  /**
     @}
  */

}

#endif
