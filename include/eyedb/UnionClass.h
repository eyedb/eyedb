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


#ifndef _EYEDB_UNION_CLASS_H
#define _EYEDB_UNION_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Union;

  /**
     Not yet documented.
  */
  class UnionClass : public AgregatClass {
    friend class Union;

    // ----------------------------------------------------------------------
    // UnionClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param s
       @param p
    */
    UnionClass(const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param s
       @param poid
    */
    UnionClass(const char *s, const Oid *poid);

    /**
       Not yet documented
       @param db
       @param s
       @param p
    */
    UnionClass(Database *db, const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param db
       @param s
       @param poid
    */
    UnionClass(Database *db, const char *s, const Oid *poid);

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

    /**
       Not yet documented
       @return
    */
    virtual UnionClass *asUnionClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const UnionClass *asUnionClass() const {return this;}


    virtual Status loadComplete(const Class *);
  };

  class UnionClassPtr : public AgregatClassPtr {

  public:
    UnionClassPtr(UnionClass *o = 0) : AgregatClassPtr(o) { }

    UnionClass *getUnionClass() {return dynamic_cast<UnionClass *>(o);}
    const UnionClass *getUnionClass() const {return dynamic_cast<UnionClass *>(o);}

    UnionClass *operator->() {return dynamic_cast<UnionClass *>(o);}
    const UnionClass *operator->() const {return dynamic_cast<UnionClass *>(o);}
  };

  typedef std::vector<UnionClassPtr> UnionClassPtrVector;

  /**
     @}
  */

}

#endif
