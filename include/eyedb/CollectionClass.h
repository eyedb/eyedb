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


#ifndef _EYEDB_COLLECTION_CLASS_H
#define _EYEDB_COLLECTION_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class CollectionClass : public Class {

    // ----------------------------------------------------------------------
    // CollectionClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param useAsRef
       @return
    */
    const char *getCName(Bool useAsRef = False) const;

    /**
       Not yet documented
       @param isref
       @param dim
       @param item_size
       @return
    */
    Class *getCollClass(Bool *isref = NULL, eyedblib::int16 *dim = NULL, eyedblib::int16 *item_size = NULL);

    /**
       Not yet documented
       @param isref
       @param dim
       @param item_size
       @return
    */
    const Class *getCollClass(Bool *isref = NULL, eyedblib::int16 *dim = NULL, eyedblib::int16 *item_size = NULL) const;

    /**
       Not yet documented
       @return
    */
    int getItemSize() const {return item_size;}

    /**
       Not yet documented
       @return
    */
    Status getStatus() const {return _status;}

    /**
       Not yet documented
       @return
    */
    Status create();

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
    Status remove(const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE* fd = stdout, unsigned int flags = 0, const RecMode *recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented
       @param db
       @param cls
       @return
    */
    static Status make(Database *db, Class **cls);

    Status generateCode_C(Schema *, const char *prefix,
			  const GenCodeHints &,
			  const char *stubs,
			  FILE *, FILE *, FILE *, FILE *, FILE *, FILE *);
    Status generateCode_Java(Schema *, const char *prefix, 
			     const GenCodeHints &, FILE *);
    //Bool compare_perform(const Class *) const;
    Bool compare_perform(const Class *cl,
			 Bool compClassOwner,
			 Bool compNum,
			 Bool compName,
			 Bool inDepth) const;

    virtual const char *getPrefix() const {return NULL;}
    virtual const char *getCSuffix() const {return "NULL";}

    /**
       Not yet documented
       @return
    */
    virtual CollectionClass *asCollectionClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const CollectionClass *asCollectionClass() const {return this;}

    /**
       Not yet documented
       @param s
       @return
    */
    virtual Status setName(const char *s);

    // ----------------------------------------------------------------------
    // CollectionClass Protected Part
    // ----------------------------------------------------------------------
  protected:
    Class *coll_class;
    Bool isref;
    eyedblib::int16 dim;
    Oid cl_oid;
    eyedblib::int16 item_size;
    Status _status;

    CollectionClass(Class *, Bool, const char *);
    CollectionClass(Class *, int, const char *);
    CollectionClass(const CollectionClass &);
    CollectionClass& operator=(const CollectionClass &);

    static const char *make_name(const char *, Class*, Bool, int, Bool);
    static CollectionClass *get(const char *, Class *, Bool, int);
    static void set(const char *, Class *, Bool, int,
		    CollectionClass *);

    int genODL(FILE *fd, Schema *) const;

    // ----------------------------------------------------------------------
    // CollectionClass Private Part
    // ----------------------------------------------------------------------
  private:
    static LinkedList *mcoll_list;
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status check(Class *_coll_class, Bool _isref, int _dim);
    void copy_realize(const CollectionClass &);

    // ----------------------------------------------------------------------
    // CollectionClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init();
    static void _release();
    void invalidateCollClassOid();

    // ----------------------------------------------------------------------
    // CollectionClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    CollectionClass(const Oid&, const char *);
    virtual Status loadComplete(const Class *);
  };

  class CollectionClassPtr : public ClassPtr {

  public:
    CollectionClassPtr(CollectionClass *o = 0) : ClassPtr(o) { }

    CollectionClass *getCollectionClass() {return dynamic_cast<CollectionClass *>(o);}
    const CollectionClass *getCollectionClass() const {return dynamic_cast<CollectionClass *>(o);}

    CollectionClass *operator->() {return dynamic_cast<CollectionClass *>(o);}
    const CollectionClass *operator->() const {return dynamic_cast<CollectionClass *>(o);}
  };

  typedef std::vector<CollectionClassPtr> CollectionClassPtrVector;

  /**
     @}
  */

}

#endif
