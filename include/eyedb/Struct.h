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


#ifndef _EYEDB_STRUCT_H
#define _EYEDB_STRUCT_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Struct : public Agregat {

    // ----------------------------------------------------------------------
    // Struct Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param _db
       @param _dataspace
    */
    Struct(Database *_db = 0, const Dataspace *_dataspace = 0) :
      Agregat(_db, _dataspace) {}

    /**
       Not yet documented
       @param o
    */
    Struct(const Struct &o) : Agregat(o) {}

    /**
       Not yet documented
       @param o
       @param share
    */
    Struct(const Struct *o, Bool share = False) :
      Agregat(o, share) {}

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Struct(*this);}


    /**
       Not yet documented
       @return
    */
    virtual Struct *asStruct() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Struct *asStruct() const {return this;}


    // ----------------------------------------------------------------------
    // Struct Protected Part
    // ----------------------------------------------------------------------
  protected:
    void initialize(Database *_db) {
      Agregat::initialize(_db);
    }
  };

  class StructPtr : public AgregatPtr {

  public:
    StructPtr(Struct *o = 0) : AgregatPtr(o) { }

    Struct *getStruct() {return dynamic_cast<Struct *>(o);}
    const Struct *getStruct() const {return dynamic_cast<Struct *>(o);}

    Struct *operator->() {return dynamic_cast<Struct *>(o);}
    const Struct *operator->() const {return dynamic_cast<Struct *>(o);}
  };

  typedef std::vector<StructPtr> StructPtrVector;

  /**
     @}
  */

}

#endif
