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


#ifndef _EYEDB_INT16_H
#define _EYEDB_INT16_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Int16 : public Basic {

    // -----------------------------------------------------------------------
    // Int16 Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param i
    */
    Int16(eyedblib::int16 i = 0);

    /**
       Not yet documented
       @param db
       @param i
       @param dataspace
    */
    Int16(Database *db, eyedblib::int16 i = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Int16(const Int16 *o);

    /**
       Not yet documented
       @param o
    */
    Int16(const Int16 &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Int16(*this);}

    /**
       Not yet documented
       @param o
       @return
    */
    Int16& operator=(const Int16 &o);

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
    virtual Int16 *asInt16() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Int16 *asInt16() const {return this;}

    // -----------------------------------------------------------------------
    // Int16 Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    eyedblib::int16 val;
    Status create();
    Status update();
  };

  class Int16Ptr : public BasicPtr {

  public:
    Int16Ptr(Int16 *o = 0) : BasicPtr(o) { }

    Int16 *getInt16() {return dynamic_cast<Int16 *>(o);}
    const Int16 *getInt16() const {return dynamic_cast<Int16 *>(o);}

    Int16 *operator->() {return dynamic_cast<Int16 *>(o);}
    const Int16 *operator->() const {return dynamic_cast<Int16 *>(o);}
  };

  typedef std::vector<Int16Ptr> Int16PtrVector;

  extern Int16Class   *Int16_Class;
  extern const char int16_class_name[];

  /**
     @}
  */

}

#endif
