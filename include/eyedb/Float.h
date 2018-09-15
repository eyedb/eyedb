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


#ifndef _EYEDB_FLOAT_H
#define _EYEDB_FLOAT_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Float : public Basic {

    // -----------------------------------------------------------------------
    // Float Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param d
    */
    Float(double d =0.0);

    /**
       Not yet documented
       @param db
       @param d
       @param dataspace
    */
    Float(Database *db, double d = 0.0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Float(const Float *o);

    /**
       Not yet documented
       @param o
    */
    Float(const Float &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Float(*this);}

    /**
       Not yet documented
       @param o
       @return
    */
    Float& operator=(const Float &o);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE *fd = stdout, unsigned int flags = 0, const RecMode *recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented
       @param data
       @return
    */
    Status setValue(Data data);

    /**
       Not yet documented
       @param data
       @return
    */
    Status getValue(Data *data) const;

    /**
       Not yet documented
       @return
    */
    virtual Float *asFloat() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Float *asFloat() const {return this;}

    // -----------------------------------------------------------------------
    // Float Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    double val;
    Status create();
    Status update();
  };

  class FloatPtr : public BasicPtr {

  public:
    FloatPtr(Float *o = 0) : BasicPtr(o) { }

    Float *getFloat() {return dynamic_cast<Float *>(o);}
    const Float *getFloat() const {return dynamic_cast<Float *>(o);}

    Float *operator->() {return dynamic_cast<Float *>(o);}
    const Float *operator->() const {return dynamic_cast<Float *>(o);}
  };

  typedef std::vector<FloatPtr> FloatPtrVector;

  extern FloatClass   *Float_Class;
  extern const char float_class_name[];

  /**
     @}
  */

}

#endif
