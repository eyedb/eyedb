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


#ifndef _EYEDB_COLL_BAG_H
#define _EYEDB_COLL_BAG_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class CollBag : public Collection {

    // ----------------------------------------------------------------------
    // CollBag Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param n
       @param mc
       @param isref
       @param collimpl
    */
    CollBag(Database *db, const char *n, Class *mc = NULL,
	    Bool isref = True,  const CollImpl *collimpl = 0);

    /**
       Not yet documented
       @param db
       @param n
       @param mc
       @param dim
       @param collimpl
    */
    CollBag(Database *db, const char *n, Class *mc, int dim,
	     const CollImpl *collimpl = 0);

    /**
       Not yet documented
       @param o
    */
    CollBag(const CollBag &o);

    /**
       Not yet documented
       @param o
       @return
    */
    CollBag& operator=(const CollBag &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new CollBag(*this);}


    /**
       Not yet documented
       @return
    */
    virtual CollBag *asCollBag() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const CollBag *asCollBag() const {return this;}

    // ----------------------------------------------------------------------
    // CollBag Private Part
    // ----------------------------------------------------------------------
  private:
    void init();
    const char *getClassName() const;
    CollBag(const char *, Class *,
	    const Oid&, const Oid&, int,
	    int, int, const CollImpl *, Object *, Bool, Bool,
	    Data, Size);
    friend class CollectionPeer;

    // ----------------------------------------------------------------------
    // CollBag Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    CollBag(const char *, Class * = NULL, Bool = True,
	    const CollImpl * = 0);
    CollBag(const char *, Class *, int,
	    const CollImpl * = 0);

    Status insert_p(const Oid &item_oid, Bool noDup = False);
    Status insert_p(const Object *item_o, Bool noDup = False);
    Status insert_p(Data val, Bool noDup = False, Size size = defaultSize);
  };

  class CollBagPtr : public CollectionPtr {

  public:
    CollBagPtr(CollBag *o = 0) : CollectionPtr(o) { }

    CollBag *getCollBag() {return dynamic_cast<CollBag *>(o);}
    const CollBag *getCollBag() const {return dynamic_cast<CollBag *>(o);}

    CollBag *operator->() {return dynamic_cast<CollBag *>(o);}
    const CollBag *operator->() const {return dynamic_cast<CollBag *>(o);}
  };

  typedef std::vector<CollBagPtr> CollBagPtrVector;

  /**
     @}
  */

}

#endif
