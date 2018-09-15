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


#ifndef _EYEDB_OIDP_H
#define _EYEDB_OIDP_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class OidP : public Basic {

    // -----------------------------------------------------------------------
    // OidP Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param oid
    */
    OidP(const Oid *oid = NULL);

    /**
       Not yet documented
       @param db
       @param oid
       @param dataspace
    */
    OidP(Database *db, const Oid *oid = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    OidP(const OidP *o);

    /**
       Not yet documented
       @param o
    */
    OidP(const OidP &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new OidP(*this);}


    /**
       Not yet documented
       @param o
       @return
    */
    OidP& operator=(const OidP &o);

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
    virtual OidP *asOidP() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const OidP *asOidP() const {return this;}


    // -----------------------------------------------------------------------
    // OidP Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Oid val;
    Status create();
    Status update();
  };

  class OidPPtr : public BasicPtr {

  public:
    OidPPtr(OidP *o = 0) : BasicPtr(o) { }

    OidP *getOidP() {return dynamic_cast<OidP *>(o);}
    const OidP *getOidP() const {return dynamic_cast<OidP *>(o);}

    OidP *operator->() {return dynamic_cast<OidP *>(o);}
    const OidP *operator->() const {return dynamic_cast<OidP *>(o);}
  };

  typedef std::vector<OidPPtr> OidPPtrVector;

  extern OidClass     *OidP_Class;
  extern const char oid_class_name[];

  /**
     @}
  */

}

#endif
  
