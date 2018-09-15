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


#ifndef _EYEDB_INSTANCE_H
#define _EYEDB_INSTANCE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Instance : public Object {

    // ----------------------------------------------------------------------
    // Instance Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param dataspace
    */
    Instance(Database *db = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Instance(const Instance &o) : Object(o) {}

    /**
       Not yet documented
       @param o
       @param share
    */
    Instance(const Instance *o, Bool share = False) :
      Object(o, share) {}

    /**
       Not yet documented
       @return
    */
    Instance *asInstance() {return this;}

    /**
       Not yet documented
       @return
    */
    const Instance *asInstance() const {return this;}

    /**
       Not yet documented
       @return
    */
    virtual void garbage();

    virtual ~Instance();

    // ----------------------------------------------------------------------
    // Instance Protected Part
    // ----------------------------------------------------------------------
  protected:
    void initialize(Database *);
  };

  class InstancePtr : public ObjectPtr {

  public:
    InstancePtr(Instance *o = 0) : ObjectPtr(o) { }

    Instance *getInstance() {return dynamic_cast<Instance *>(o);}
    const Instance *getInstance() const {return dynamic_cast<Instance *>(o);}

    Instance *operator->() {return dynamic_cast<Instance *>(o);}
    const Instance *operator->() const {return dynamic_cast<Instance *>(o);}
  };

  typedef std::vector<InstancePtr> InstancePtrVector;

  /**
     @}
  */

}

#endif
