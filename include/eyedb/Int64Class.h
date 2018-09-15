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


#ifndef _EYEDB_INT64_CLASS_H
#define _EYEDB_INT64_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Int64Class : public BasicClass {

    // ----------------------------------------------------------------------
    // Int64Class Interface
    // ----------------------------------------------------------------------

  public:
    /**
       Not yet documented
       @param db
    */
    Int64Class(Database *db = NULL);

    /**
       Not yet documented
       @param cl
    */
    Int64Class(const Int64Class &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    Int64Class& operator=(const Int64Class &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Int64Class(*this);}

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
    virtual Int64Class *asInt64Class() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Int64Class *asInt64Class() const {return this;}

    // ----------------------------------------------------------------------
    // Int64Class Private Part
    // ----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;

    // ----------------------------------------------------------------------
    // Int64Class Restricted Access (conceptually private)
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

  class Int64ClassPtr : public BasicClassPtr {

  public:
    Int64ClassPtr(Int64Class *o = 0) : BasicClassPtr(o) { }

    Int64Class *getInt64Class() {return dynamic_cast<Int64Class *>(o);}
    const Int64Class *getInt64Class() const {return dynamic_cast<Int64Class *>(o);}

    Int64Class *operator->() {return dynamic_cast<Int64Class *>(o);}
    const Int64Class *operator->() const {return dynamic_cast<Int64Class *>(o);}
  };

  typedef std::vector<Int64ClassPtr> Int64ClassPtrVector;

  /**
     @}
  */

}

#endif
