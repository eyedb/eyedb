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


#ifndef _EYEDB_COLL_ARRAY_H
#define _EYEDB_COLL_ARRAY_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class CollArray : public Collection {

    // ----------------------------------------------------------------------
    // CollArray Interface
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
    CollArray(Database *db, const char *n, Class *mc = NULL,
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
    CollArray(Database *db, const char *n, Class *mc, int dim,
	      const CollImpl *collimpl = 0);

    /**
       Not yet documented
       @param o
    */
    CollArray(const CollArray &o);

    /**
       Not yet documented
       @param o
    */
    CollArray& operator=(const CollArray &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new CollArray(*this);}
  
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
       @param checkFirst
       @return
    */
    Status suppress(const Value &value, Bool checkFirst = False);

    /**
       Not yet documented
       @param value
       @return
    */
    Status insertAt(Collection::ItemId id, const Value &value);

    /**
       Not yet documented
       @param value
       @return
    */
    Status append(const Value &value);

    /**
       Not yet documented
       @param id
       @return
    */
    Status suppressAt(Collection::ItemId id);

    /**
       Not yet documented
       @param id
       @param value
       @return
    */
    Status retrieveAt(Collection::ItemId id, Value &value) const;

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
       @param o
       @param recmode
       @return
    */
    Status retrieveAt(Collection::ItemId id, ObjectPtr &o,
		      const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @param id
       @param o
       @param recmode
       @return
    */
    Status retrieveAt(Collection::ItemId id, Object *&o,
		      const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @return
    */
    virtual CollArray *asCollArray() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const CollArray *asCollArray() const {return this;}

    Status getImplStats(std::string &, std::string &, Bool dspImpl = True,
			Bool full = False, const char *indent = "");
    Status getImplStats(IndexStats *&, IndexStats *&);

    Status simulate(const CollImpl &, std::string &, std::string &,
		    Bool dspImpl = True, Bool full = False,
		    const char *indent = "");
    Status simulate(const CollImpl &, IndexStats *&, IndexStats *&);

    virtual void garbage();

    // ----------------------------------------------------------------------
    // CollArray Private Part
    // ----------------------------------------------------------------------
  private:
    ValueCache *read_arr_cache;

    void init();
    const char *getClassName() const;
    CollArray(const char *, Class *,
	      const Oid&, const Oid&, int,
	      int, int, const CollImpl *, Object *,
	      Bool, Bool, Data, Size);
    friend class CollectionPeer;

    // ----------------------------------------------------------------------
    // CollArray Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    CollArray(const char *, Class * = NULL, Bool = True,
	      const CollImpl * = 0);
    CollArray(const char *, Class *, int,
	      const CollImpl * = 0);

    Status insert_p(const Oid &item_oid, Bool noDup = False);
    Status insert_p(const Object *item_o, Bool noDup = False);
    Status insert_p(Data val, Bool noDup = False, Size size = defaultSize);
    Status suppress_p(const Oid &item_oid, Bool checkFirst = False);
    Status suppress_p(const Object *item_o, Bool checkFirst = False);
    Status suppress_p(Data data, Bool checkFirst = False, Size size = defaultSize);
    Status insertAt_p(Collection::ItemId id, const Oid &item_oid);
    Status insertAt_p(Collection::ItemId id, const Object *item_o);
    Status insertAt_p(Collection::ItemId id, Data val, Size size = defaultSize);
    Status append_p(const Oid &item_oid, Bool noDup = False);
    Status append_p(const Object *item_o, Bool noDup = False);
    Status append_p(Data data, Bool noDup = False, Size size = defaultSize);
    Status retrieveAt_p(Collection::ItemId id, Data data, Size size = defaultSize) const;

  };

  class CollArrayPtr : public CollectionPtr {

  public:
    CollArrayPtr(CollArray *o = 0) : CollectionPtr(o) { }

    CollArray *getCollArray() {return dynamic_cast<CollArray *>(o);}
    const CollArray *getCollArray() const {return dynamic_cast<CollArray *>(o);}

    CollArray *operator->() {return dynamic_cast<CollArray *>(o);}
    const CollArray *operator->() const {return dynamic_cast<CollArray *>(o);}
  };

  typedef std::vector<CollArrayPtr> CollArrayPtrVector;

  /**
     @}
  */

}

#endif
