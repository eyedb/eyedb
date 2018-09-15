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


#ifndef _EYEDB_BASIC_H
#define _EYEDB_BASIC_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Basic : public Instance {

    // ----------------------------------------------------------------------
    // Basic Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param dataspace
    */
    Basic(Database *db = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Basic(const Basic *o);

    /**
       Not yet documented
       @param o
    */
    Basic(const Basic &o);

    /**
       Not yet documented
       @param recmode
       @return
    */
    Status remove(const RecMode* recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @return
    */
    virtual Basic *asBasic() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Basic *asBasic() const {return this;}

    // ----------------------------------------------------------------------
    // Basic Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init(void);
    static void _release(void);
    virtual Status create();
    virtual Status update();
  };

  class BasicPtr : public InstancePtr {

  public:
    BasicPtr(Basic *o = 0) : InstancePtr(o) { }

    Basic *getBasic() {return dynamic_cast<Basic *>(o);}
    const Basic *getBasic() const {return dynamic_cast<Basic *>(o);}

    Basic *operator->() {return dynamic_cast<Basic *>(o);}
    const Basic *operator->() const {return dynamic_cast<Basic *>(o);}
  };

  typedef std::vector<BasicPtr> BasicPtrVector;

  /**
     @}
  */

}

#endif
