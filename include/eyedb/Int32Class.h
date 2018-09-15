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


#ifndef _EYEDB_INT32_CLASS_H
#define _EYEDB_INT32_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Int32Class : public BasicClass {

    // ----------------------------------------------------------------------
    // Int32Class Interface
    // ----------------------------------------------------------------------

  public:
    /**
       Not yet documented
       @param db
    */
    Int32Class(Database *db = NULL);

    /**
       Not yet documented
       @param cl
    */
    Int32Class(const Int32Class &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    Int32Class& operator=(const Int32Class &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Int32Class(*this);}

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
    virtual Int32Class *asInt32Class() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Int32Class *asInt32Class() const {return this;}

    // ----------------------------------------------------------------------
    // Int32Class Private Part
    // ----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;

    // ----------------------------------------------------------------------
    // Int32Class Restricted Access (conceptually private)
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

  class Int32ClassPtr : public BasicClassPtr {

  public:
    Int32ClassPtr(Int32Class *o = 0) : BasicClassPtr(o) { }

    Int32Class *getInt32Class() {return dynamic_cast<Int32Class *>(o);}
    const Int32Class *getInt32Class() const {return dynamic_cast<Int32Class *>(o);}

    Int32Class *operator->() {return dynamic_cast<Int32Class *>(o);}
    const Int32Class *operator->() const {return dynamic_cast<Int32Class *>(o);}
  };

  typedef std::vector<Int32ClassPtr> Int32ClassPtrVector;

  /**
     @}
  */

}

#endif
