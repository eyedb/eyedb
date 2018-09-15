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


#ifndef _EYEDB_COLL_LIST_CLASS_H
#define _EYEDB_COLL_LIST_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class CollListClass : public CollectionClass {

    // ----------------------------------------------------------------------
    // CollListClass Interface
    // ----------------------------------------------------------------------
  public:
    static CollectionClass *make(Class *, Bool, int, Status &);

    /**
       Not yet documented
       @param coll_class
       @param isref
    */
    CollListClass(Class *coll_class, Bool isref);

    /**
       Not yet documented
       @param coll_class
       @param dim
    */
    CollListClass(Class *coll_class, int dim);

    /**
       Not yet documented
       @param cl
    */
    CollListClass(const CollListClass &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    CollListClass& operator=(const CollListClass &cl);

    /**
       Not yet documented
       @return
    */
    virtual const char *getPrefix() const {return "list";}

    /**
       Not yet documented
       @return
    */
    virtual const char *getCSuffix() const {return "CollList";}

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new CollListClass(*this);}

    /**
       Not yet documented
       @return
    */
    virtual CollListClass *asCollListClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const CollListClass *asCollListClass() const {return this;}

    /**
       Not yet documented
       @param db
       @return
    */
    Object *newObj(Database *db = NULL) const;

    /**
       Not yet documented
       @param data
       @param copy
       @return
    */
    Object *newObj(Data data, Bool copy = True) const;

    // ----------------------------------------------------------------------
    // CollListClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    CollListClass(const Oid&, const char *);
  };

  class CollListClassPtr : public CollectionClassPtr {

  public:
    CollListClassPtr(CollListClass *o = 0) : CollectionClassPtr(o) { }

    CollListClass *getCollListClass() {return dynamic_cast<CollListClass *>(o);}
    const CollListClass *getCollListClass() const {return dynamic_cast<CollListClass *>(o);}

    CollListClass *operator->() {return dynamic_cast<CollListClass *>(o);}
    const CollListClass *operator->() const {return dynamic_cast<CollListClass *>(o);}
  };

  typedef std::vector<CollListClassPtr> CollListClassPtrVector;

  /**
     @}
  */

}

#endif
  
