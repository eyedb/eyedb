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


#ifndef	_EYEDB_STRUCT_CLASS_H
#define _EYEDB_STRUCT_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Struct;

  /**
     Not yet documented.
  */
  class StructClass : public AgregatClass {
    friend class Struct;

    // ----------------------------------------------------------------------
    // StructClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param s
       @param p
    */
    StructClass(const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param s
       @param poid
    */
    StructClass(const char *s, const Oid *poid);

    /**
       Not yet documented
       @param db
       @param s
       @param p
    */
    StructClass(Database *db, const char *s, Class *p = NULL);

    /**
       Not yet documented
       @param db
       @param s
       @param poid
    */
    StructClass(Database *db, const char *s, const Oid *poid);

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
    virtual StructClass *asStructClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const StructClass *asStructClass() const {return this;}


    // ----------------------------------------------------------------------
    // StructClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    StructClass(const Oid&, const char *);
    virtual Status loadComplete(const Class *);
  };

  class StructClassPtr : public AgregatClassPtr {

  public:
    StructClassPtr(StructClass *o = 0) : AgregatClassPtr(o) { }

    StructClass *getStructClass() {return dynamic_cast<StructClass *>(o);}
    const StructClass *getStructClass() const {return dynamic_cast<StructClass *>(o);}

    StructClass *operator->() {return dynamic_cast<StructClass *>(o);}
    const StructClass *operator->() const {return dynamic_cast<StructClass *>(o);}
  };

  typedef std::vector<StructClassPtr> StructClassPtrVector;

  /**
     @}
  */

}

#endif
