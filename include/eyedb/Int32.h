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


#ifndef _EYEDB_INT32_H
#define _EYEDB_INT32_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Int32 : public Basic {

    // -----------------------------------------------------------------------
    // Int32 Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param i
    */
    Int32(eyedblib::int32 i = 0);

    /**
       Not yet documented
       @param db
       @param i
       @param dataspace
    */
    Int32(Database *db, eyedblib::int32 i = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Int32(const Int32 *o);

    /**
       Not yet documented
       @param o
    */
    Int32(const Int32 &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Int32(*this);}

    /**
       Not yet documented
       @param o
       @return
    */
    Int32& operator=(const Int32 &o);

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
    virtual Int32 *asInt32() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Int32 *asInt32() const {return this;}

    // -----------------------------------------------------------------------
    // Int32 Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    eyedblib::int32 val;
    Status create();
    Status update();
  };

  class Int32Ptr : public BasicPtr {

  public:
    Int32Ptr(Int32 *o = 0) : BasicPtr(o) { }

    Int32 *getInt32() {return dynamic_cast<Int32 *>(o);}
    const Int32 *getInt32() const {return dynamic_cast<Int32 *>(o);}

    Int32 *operator->() {return dynamic_cast<Int32 *>(o);}
    const Int32 *operator->() const {return dynamic_cast<Int32 *>(o);}
  };

  typedef std::vector<Int32Ptr> Int32PtrVector;

  extern Int32Class   *Int32_Class;
  extern const char int32_class_name[];

  /**
     @}
  */

}

#endif
