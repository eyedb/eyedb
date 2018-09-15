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


#ifndef _EYEDB_ENUM_H
#define _EYEDB_ENUM_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Enum : public Instance {

    // ----------------------------------------------------------------------
    // Enum Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param dataspace
    */
    Enum(Database *db = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Enum(const Enum &o);

    /**
       Not yet documented
       @param o
       @return
    */
    Enum& operator=(const Enum &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Enum(*this);}

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
       @param v
       @return
    */
    Status setValue(unsigned int v);

    /**
       Not yet documented
       @param v
       @return
    */
    Status getValue(unsigned int *v) const;

    /**
       Not yet documented
       @param name
       @return
    */
    Status setValue(const char *name);

    /**
       Not yet documented
       @param s
       @return
    */
    Status getValue(const char **s) const;

    /**
       Not yet documented
       @param it
       @return
    */
    Status setValue(const EnumItem *it);

    /**
       Not yet documented
       @param pit
       @return
    */
    Status getValue(const EnumItem **pit) const;

    Status create();
    Status update();
    Status remove(const RecMode* = RecMode::NoRecurs);

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
       @return
    */
    virtual Enum *asEnum() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Enum *asEnum() const {return this;}

    virtual ~Enum();

    // ----------------------------------------------------------------------
    // Enum Private Part
    // ----------------------------------------------------------------------
  private:
    const EnumItem *val;
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    virtual void garbage();
  };

  class EnumPtr : public InstancePtr {

  public:
    EnumPtr(Enum *o = 0) : InstancePtr(o) { }

    Enum *getEnum() {return dynamic_cast<Enum *>(o);}
    const Enum *getEnum() const {return dynamic_cast<Enum *>(o);}

    Enum *operator->() {return dynamic_cast<Enum *>(o);}
    const Enum *operator->() const {return dynamic_cast<Enum *>(o);}
  };

  typedef std::vector<EnumPtr> EnumPtrVector;

  /**
     @}
  */

}

#endif
