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


#ifndef _EYEDB_UNION_H
#define _EYEDB_UNION_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Union : public Agregat {

    // ----------------------------------------------------------------------
    // Union Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param _db
       @param _dataspace
    */
    Union(Database *_db = 0, const Dataspace *_dataspace = 0) :
      Agregat(_db, _dataspace) {item = 0;}

    /**
       Not yet documented
       @param o
    */
    Union(const Union &o) : Agregat(o) {}

    /**
       Not yet documented
       @param o
       @param share
    */
    Union(const Union *o, Bool share = False) : Agregat(o, share)
    {item = 0;}

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Union(*this);}


    /**
       Not yet documented
       @param ni
       @return
    */
    const Attribute *setCurrentItem(const Attribute *ni);

    /**
       Not yet documented
       @return
    */
    const Attribute *getCurrentItem() const;

    /**
       Not yet documented
       @return
    */
    const Attribute *decodeCurrentItem();

    /**
       Not yet documented
       @param cls
       @param idr
       @return
    */
    static const Attribute *decodeCurrentItem(const Class *cls, Data idr);


    /**
       Not yet documented
       @return
    */
    virtual Union *asUnion() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Union *asUnion() const {return this;}


    // ----------------------------------------------------------------------
    // Union Protected Part
    // ----------------------------------------------------------------------
  protected:
    void initialize(Database *_db) {
      Agregat::initialize(_db);
    }

    // ----------------------------------------------------------------------
    // Union Private Part
    // ----------------------------------------------------------------------
  private:
    const Attribute *item;
  };

  class UnionPtr : public AgregatPtr {

  public:
    UnionPtr(Union *o = 0) : AgregatPtr(o) { }

    Union *getUnion() {return dynamic_cast<Union *>(o);}
    const Union *getUnion() const {return dynamic_cast<Union *>(o);}

    Union *operator->() {return dynamic_cast<Union *>(o);}
    const Union *operator->() const {return dynamic_cast<Union *>(o);}
  };

  typedef std::vector<UnionPtr> UnionPtrVector;

  /**
     @}
  */

}

#endif
