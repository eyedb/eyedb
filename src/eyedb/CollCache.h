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


#ifdef USE_VALUE_CACHE

#include "ValueCache.h"

#else

namespace eyedb {

  class CollItem {

  public:
    CollItem(Collection *_coll, const Oid&, Collection::ItemId, int);
    CollItem(Collection *_coll, const Object *, Collection::ItemId, int);
    CollItem(Collection *_coll, const Value &, Collection::ItemId, int);
    CollItem(Collection *_coll, Data, Collection::ItemId, int);

    void setState(int _state);
    int  getState() const {return state;}

    void setId(Collection::ItemId _id) {id = _id;}
    Collection::ItemId getId() const {return id;}

    void  setOid(const Oid&);
    const Oid& getOid() const;

    void  setObject(const Object *);
    const Object *getObject() const;

    void    setData(Data);
    Data getData() const;

    void  setValue(const Value &);
    const Value& getValue() const;

    ~CollItem() {delete data;}

  private:
    void init(Collection *_coll, Collection::ItemId, int);
    Collection *coll;
    Collection::ItemId id;
    Oid oid;
    const Object *o;
    Value value;
    Data data;
    int state;
    Bool is_oid, is_object, is_value, is_data;

  public:
    void setColl(Collection *_coll) {
      coll = _coll;
    }
  };

  class CollCache {

  public:
    CollCache(Collection *, unsigned int magorder, Size,
	      unsigned int max = 0xffffffff);

    Status insert(const Oid&, Collection::ItemId, int);
    Status insert(const Object *, Collection::ItemId, int);
    Status insert(const Value&, Collection::ItemId, int);
    Status insert(Data, Collection::ItemId, int);

    Status suppressOid(CollItem *item);
    Status suppressObject(CollItem *item);
    Status suppressValue(CollItem *item);
    Status suppressData(CollItem *item);
    Status suppress(CollItem *item);

    CollItem *get(Collection::ItemId);
    CollItem *get(const Object *);
    CollItem *get(const Oid&);
    CollItem *get(const Value &);
    CollItem *get(Data);

    LinkedList *getList();

    void empty(Bool = False);

    ~CollCache();

  private:
    Collection *coll;
    Collection::ItemId mask;
    unsigned int nkeys;
    Size item_size;

    void remove(LinkedList **);

    LinkedList *list;
    LinkedList **oid_list;
    LinkedList **id_list;
    LinkedList **obj_list;
    LinkedList **value_list;
    LinkedList **data_list;

    int nitems;
    unsigned int max_items;
    CollItem *get_realize(int, LinkedList **);
    Status insert_realize(const void *, int, Collection::ItemId, unsigned int, LinkedList **);

    inline unsigned int key_id(Collection::ItemId id) { return id & mask; }
    inline unsigned int key_oid(const Oid &oid)  { return oid.getNX() & mask; }
    inline unsigned int key_obj(const Object *o) { return (((long)o)>>2) & mask; }
    inline unsigned int key_data(Data data)
    {
      Data p = data;
      unsigned int key = 0;
      int r = (item_size < 8 ? item_size : 8);
    
      for (int i = 0; i < r; i++)
	key += (int)*p++;
    
      return key & mask;
    }

    inline int key_value(const Value &value) {
      return 0;
    }

    void init();

  public:
    void setColl(Collection *);
  };
}

#endif
