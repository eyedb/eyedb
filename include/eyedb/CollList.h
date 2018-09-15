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


#ifndef _EYEDB_COLL_LIST_H
#define _EYEDB_COLL_LIST_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class CollList : public Collection {

    // ----------------------------------------------------------------------
    // CollList Interface
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
    CollList(Database *db, const char *n, Class *mc = NULL,
	     Bool isref = True,
	     const CollImpl *collimpl = 0);

    /**
       Not yet documented
       @param db
       @param n
       @param mc
       @param dim
       @param collimpl
    */
    CollList(Database *db, const char *n, Class *mc, int dim,
	     const CollImpl *collimpl = 0);

    /**
       Not yet documented
       @param o
    */
    CollList(const CollList &o);

    /**
       Not yet documented
       @param o
    */
    CollList& operator=(const CollList &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new CollList(*this);}


    /**
       Not yet documented
       @param value
       @param noDup
       @return
    */
    Status insert(const Value &value, Bool noDup = False);

    /**
       Not yet documented
       @param value
       @return
    */
    Status insertFirst(const Value &value, Collection::ItemId *iid = 0);

    /**
       Not yet documented
       @param value
       @return
    */
    Status insertLast(const Value &value, Collection::ItemId *iid = 0);

    /**
       Not yet documented
       @param value
       @return
    */
    Status insertBefore(Collection::ItemId id, const Value &value,
			Collection::ItemId *iid = 0);

    /**
       Not yet documented
       @param value
       @return
    */
    Status insertAfter(Collection::ItemId id, const Value &value,
		       Collection::ItemId *iid = 0);

    /**
       Not yet documented
       @param id
       @param value
       @return
    */
    Status replaceAt(Collection::ItemId id, const Value &value);

    /**
       Not yet documented
       @param id
       @return
    */
    Status suppressAt(Collection::ItemId id);

    /**
       Not yet documented
       @param id
       @param item_oid
       @return
    */
    Status retrieveAt(Collection::ItemId id, Oid &item_oid) const;

    /**
       Not yet documented
       @param id
       @param item_o
       @param recmode
       @return
    */
    Status retrieveAt(Collection::ItemId id, ObjectPtr &item_o,
		      const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @param id
       @param item_o
       @param recmode
       @return
    */
    Status retrieveAt(Collection::ItemId id, Object *&item_o,
		      const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @param id
       @param value
       @return
    */
    Status retrieveAt(Collection::ItemId id, Value &value) const;

    /**
       Not yet documented
       @return
    */
    virtual CollList *asCollList() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const CollList *asCollList() const {return this;}

    // ----------------------------------------------------------------------
    // CollList Private Part
    // ----------------------------------------------------------------------
  private:
    void init();
    const char *getClassName() const;
    CollList(const char *, Class *,
	     const Oid&, const Oid&, int,
	     int, int, const CollImpl *, Object *,
	     Bool, Bool, Data, Size);
    friend class CollectionPeer;

    // ----------------------------------------------------------------------
    // CollList Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    CollList(const char *, Class * = NULL, Bool = True,
	     const CollImpl * = 0);
    CollList(const char *, Class *, int,
	     const CollImpl * = 0);

    Status insert_p(const Oid &item_oid, Bool noDup = False);
    Status insert_p(const Object *item_o, Bool noDup = False);
    Status insert_p(Data val, Bool noDup = False, Size size = defaultSize);
    Status insertFirst_p(const Oid &item_oid, Collection::ItemId *iid = 0);
    Status insertFirst_p(const Object *item_o, Collection::ItemId *iid = 0);
    Status insertLast_p(const Oid &item_oid, Collection::ItemId *iid = 0);
    Status insertLast_p(const Object *item_o, Collection::ItemId *iid = 0);
    Status insertBefore_p(Collection::ItemId id, const Oid &item_oid,
			  Collection::ItemId *iid = 0);
    Status insertBefore_p(Collection::ItemId id, const Object *item_o,
			  Collection::ItemId *iid = 0);
    Status insertAfter_p(Collection::ItemId id, const Oid &item_oid,
			 Collection::ItemId *iid = 0);
    Status insertAfter_p(Collection::ItemId id, const Object *item_o,
			 Collection::ItemId *iid = 0);
    Status replaceAt_p(Collection::ItemId id, const Oid &item_oid);
    Status replaceAt_p(Collection::ItemId id, const Object *item_o);

  };

  class CollListPtr : public CollectionPtr {

  public:
    CollListPtr(CollList *o = 0) : CollectionPtr(o) { }

    CollList *getCollList() {return dynamic_cast<CollList *>(o);}
    const CollList *getCollList() const {return dynamic_cast<CollList *>(o);}

    CollList *operator->() {return dynamic_cast<CollList *>(o);}
    const CollList *operator->() const {return dynamic_cast<CollList *>(o);}
  };

  typedef std::vector<CollListPtr> CollListPtrVector;

  /**
     @}
  */

}

#endif
