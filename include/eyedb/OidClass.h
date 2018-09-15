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


#ifndef _EYEDB_OID_CLASS_H
#define _EYEDB_OID_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class OidClass : public BasicClass {

    // ----------------------------------------------------------------------
    // OidClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
    */
    OidClass(Database *db = NULL);

    /**
       Not yet documented
       @param cl
    */
    OidClass(const OidClass &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    OidClass& operator=(const OidClass &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new OidClass(*this);}

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
       @param fd
       @param indent
       @param inidata
       @param data
       @param tmod
       @return
    */
    Status traceData(FILE *fd, int indent, Data inidata,
		     Data data, TypeModifier *tmod = NULL) const;

    /**
       Not yet documented
       @return
    */
    virtual OidClass *asOidClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const OidClass *asOidClass() const {return this;}

    // ----------------------------------------------------------------------
    // OidClass Private Part
    // ----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;

    // ----------------------------------------------------------------------
    // OidClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    // E_XDR
    virtual void decode(void * hdata, // to
			const void * xdata, // from
			Size incsize, // temporarely necessary 
			unsigned int nb = 1) const;

    virtual void encode(void * xdata, // to
			const void * hdata, // from
			Size incsize, // temporarely necessary 
			unsigned int nb = 1) const;

    virtual int cmp(const void * xdata,
		    const void * hdata,
		    Size incsize, // temporarely necessary 
		    unsigned int nb = 1) const;
  };

  class OidClassPtr : public BasicClassPtr {

  public:
    OidClassPtr(OidClass *o = 0) : BasicClassPtr(o) { }

    OidClass *getOidClass() {return dynamic_cast<OidClass *>(o);}
    const OidClass *getOidClass() const {return dynamic_cast<OidClass *>(o);}

    OidClass *operator->() {return dynamic_cast<OidClass *>(o);}
    const OidClass *operator->() const {return dynamic_cast<OidClass *>(o);}
  };

  typedef std::vector<OidClassPtr> OidClassPtrVector;

  /**
     @}
  */

}

#endif
  
