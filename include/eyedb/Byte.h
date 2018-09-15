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


#ifndef _EYEDB_BYTE_H
#define _EYEDB_BYTE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Byte : public Basic {

    // -----------------------------------------------------------------------
    // Byte Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param c
    */
    Byte(unsigned char c =0);

    /**
       Not yet documented
       @param db
       @param c
       @param dataspace
    */
    Byte(Database *db, unsigned char c = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Byte(const Byte *o);

    /**
       Not yet documented
       @param o
    */
    Byte(const Byte &o);

    /**
       Not yet documented
       @param o
    */
    Byte& operator=(const Byte &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Byte(*this);}

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE* fd = stdout, unsigned int flags = 0, const RecMode * recmode = RecMode::FullRecurs) const;

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
    Status getValue(Data* data) const;

    /**
       Not yet documented
       @return
    */
    virtual Byte *asByte() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Byte *asByte() const {return this;}

    // -----------------------------------------------------------------------
    // Byte Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    unsigned char val;
    Status create();
    Status update();
  };

  class BytePtr : public BasicPtr {

  public:
    BytePtr(Byte *o = 0) : BasicPtr(o) { }

    Byte *getByte() {return dynamic_cast<Byte *>(o);}
    const Byte *getByte() const {return dynamic_cast<Byte *>(o);}

    Byte *operator->() {return dynamic_cast<Byte *>(o);}
    const Byte *operator->() const {return dynamic_cast<Byte *>(o);}
  };

  typedef std::vector<BytePtr> BytePtrVector;

  extern ByteClass    *Byte_Class;
  extern const char byte_class_name[];

  /**
     @}
  */

}

#endif
