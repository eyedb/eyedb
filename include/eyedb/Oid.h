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


#ifndef _EYEDB_OID_H
#define _EYEDB_OID_H

#include <iostream>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Oid;

  std::ostream& operator<<(std::ostream&, const Oid &);
  std::ostream& operator<<(std::ostream&, const Oid *);

  /**
     Not yet documented.
  */
  class Oid {

    // ----------------------------------------------------------------------
    // Oid Interface
    // ----------------------------------------------------------------------

  public:
    /**
       Not yet documented
    */
    Oid() {
      oid = nullOid.oid;
    }

    /**
       Not yet documented
       @param _oid
    */
    Oid(const Oid &_oid);

    /**
       Not yet documented
       @param str
    */
    Oid(const char *str);

    /**
       Not yet documented
       @param nx
       @param dbid
       @param unique
    */
    Oid(eyedbsm::Oid::NX nx, eyedbsm::Oid::DbID dbid,
	eyedbsm::Oid::Unique unique);

    /**
       Not yet documented
       @return
    */
    eyedbsm::Oid::NX getNX() const     {return oid.getNX();}

    /**
       Not yet documented
       @return
    */
    eyedbsm::Oid::DbID getDbid() const   {return oid.getDbID();}

    /**
       Not yet documented
       @return
    */
    eyedbsm::Oid::Unique getUnique() const {return oid.getUnique();}


    /**
       Not yet documented
       @return
    */
    const char *getString() const;

    /**
       Not yet documented
       @return
    */
    const char *toString()  const {return getString();}


    /**
       Not yet documented
       @param oid 
       @return
    */
    Oid &operator =(const Oid &oid);

    /**
       Not yet documented
       @param oid 
       @return
    */
    bool operator <(const Oid &oid) const {
      if (getDbid() != oid.getDbid())
	return getDbid() < oid.getDbid();
      return getNX() < oid.getNX();
    }

    /**
       Not yet documented
       @return
    */
    bool operator !() const {
      return !isValid() ? true : false;
    }

    /**
       Not yet documented
       @return
    */
    const eyedbsm::Oid *getOid() const {return &oid;}

    /**
       Not yet documented
       @param 
    */
    void setOid(const eyedbsm::Oid&);

    /**
       Not yet documented
       @param 
    */
    void setOid(const eyedbsm::Oid *_oid) {oid = *_oid;}

    /**
       Not yet documented
       @return
    */
    eyedbsm::Oid *getOid() {return &oid;}


    /**
       Not yet documented
       @return
    */
    Bool isValid() const {
      return oid.getNX() ? True : False;
    }    

    /**
       Not yet documented
       @return
    */
    void invalidate() {

      oid = nullOid.oid;
    }

    /**
       Not yet documented
       @param _oid
       @return
    */
    Bool compare(const Oid &_oid) const {
      return IDBBOOL(!memcmp(&oid, &_oid.oid, sizeof(oid)));
    }

    /**
       Not yet documented
       @param _oid
       @return
    */
    Bool operator==(const Oid &_oid) const {
      return IDBBOOL(!memcmp(&oid, &_oid.oid, sizeof(oid)));
    }

    /**
       Not yet documented
       @param _oid
       @return
    */
    Bool operator!=(const Oid &_oid) const {
      return IDBBOOL(memcmp(&oid, &_oid.oid, sizeof(oid)));
    }

    static const Oid nullOid;

    ~Oid() {}

  private:
    eyedbsm::Oid oid;

  public: /* restricted */
    Oid(const eyedbsm::Oid &);
    Oid(const eyedbsm::Oid *);
  };

  class Collection;
  class OidList;
  class LinkedList;
  class LinkedListCursor;

  /**
     Not yet documented.
  */
  class OidArray {

  public:
    /**
       Not yet documented
       @param oids
       @param count
    */
    OidArray(const Oid *oids = NULL, int count = 0);

    /**
       Not yet documented
       @param coll
    */
    OidArray(const Collection *coll);

    /**
       Not yet documented
       @param list 
    */
    OidArray(const OidList &list);

    /**
       Not yet documented
       @param oid_arr
    */
    OidArray(const OidArray &oid_arr);

    /**
       Not yet documented
       @param oid_arr
       @return
    */
    OidArray& operator=(const OidArray &oid_arr);

    /**
       Not yet documented
       @param oids
       @param count
    */
    void set(const Oid *oids, int count);

    /**
       Not yet documented
       @return
    */
    int getCount() const {return count;}


    /**
       Not yet documented
       @param x 
       @return
    */
    Oid & operator[](int x) {
      return oids[x];
    }

    /**
       Not yet documented
       @param x 
       @return
    */
    const Oid &operator[](int x) const {
      return oids[x];
    }

    /**
       Not yet documented
       @return
    */
    OidList *toList() const;

    ~OidArray();

  private:
    int count;
    Oid *oids;
  };

  class OidListCursor;

  /**
     Not yet documented.
  */
  class OidList {

  public:
    /**
       Not yet documented
    */
    OidList();

    /**
       Not yet documented
       @param oid_array
    */
    OidList(const OidArray &oid_array);

    /**
       Not yet documented
       @return
    */
    int getCount() const;

    /**
       Not yet documented
       @param oid
    */
    void insertOid(const Oid &oid);

    /**
       Not yet documented
       @param oid
    */
    void insertOidLast(const Oid &oid);

    /**
       Not yet documented
       @param oid
    */
    void insertOidFirst(const Oid &oid);

    /**
       Not yet documented
       @param oid
       @return
    */
    Bool suppressOid(const Oid &oid);

    /**
       Not yet documented
       @param oid
       @return
    */
    Bool exists(const Oid &oid) const;

    /**
       Not yet documented
    */
    void empty();

    /**
       Not yet documented
       @return
    */
    OidArray *toArray() const;

    ~OidList();

  private:
    LinkedList *list;
    friend class OidListCursor;
  };

  /**
     Not yet documented.
  */
  class OidListCursor {

  public:
    /**
       Not yet documented
       @param oidlist
    */
    OidListCursor(const OidList &oidlist);

    /**
       Not yet documented
       @param oidlist
    */
    OidListCursor(const OidList *oidlist);

    /**
       Not yet documented
       @param oid
       @return
    */
    Bool getNext(Oid &oid);

    ~OidListCursor();

  private:
    LinkedListCursor *c;
  };

  /**
     @}
  */

}

#endif
