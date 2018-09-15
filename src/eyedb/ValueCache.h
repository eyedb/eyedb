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


#include <map>

namespace eyedb {

  class ValueItem {

  public:
    ValueItem(Object *o, const Value &, Collection::ItemId, int state);

    void setState(int state);
    int getState() const {return state;}

    const Value& getValue() const {return v;}
    Collection::ItemId getId() const {return id;}

    void release();

    void incRef() {refcnt++;}

  private:
    Object *o;
    Value v;
    Collection::ItemId id;
    int state;
    unsigned int refcnt;
    ~ValueItem();
  };

  class ValueCache {

  public:
    static Collection::ItemId DefaultItemID;

    ValueCache(Object *);

    Status insert(const Value &, Collection::ItemId, int state);

    Status suppress(ValueItem *item);

    ValueItem *get(Collection::ItemId);
    ValueItem *get(const Value &);

    void setObject(Object *_o) {o = _o;}

    void empty();

    typedef std::map<Value, ValueItem *> ValueMap;
    typedef std::map<Collection::ItemId, ValueItem *> IdMap;

    typedef std::map<Value, ValueItem *>::iterator ValueMapIterator;
    typedef std::map<Collection::ItemId, ValueItem *>::iterator IdMapIterator;

    unsigned int size() const {return val_map.size();}

    ValueMap &getValueMap() {return val_map;}
    IdMap &getIdMap() {return id_map;}
    void trace();

    // bwc
    Status suppressOid(ValueItem *item) {return suppress(item);}
    Status suppressObject(ValueItem *item) {return suppress(item);}
    Status suppressData(ValueItem *item) {return suppress(item);}
    Status setState(int state);
    ValueItem *get(Data data, Size item_size);
    Status insert(const eyedbsm::Oid *oid, Collection::ItemId id, int state) {
      return insert(Value(*oid), id, state);
    }
    // ...

    ~ValueCache();

  private:
    Object *o;
    //std::map<Value, ValueItem *> val_map;
    //std::map<Collection::ItemId, ValueItem *> id_map;
    ValueMap val_map;
    IdMap id_map;
    Collection::ItemId current_id;
  };
}
