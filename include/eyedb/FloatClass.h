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


#ifndef _EYEDB_FLOAT_CLASS_H
#define _EYEDB_FLOAT_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class FloatClass : public BasicClass {

    // ----------------------------------------------------------------------
    // FloatClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
    */
    FloatClass(Database *db = NULL);

    /**
       Not yet documented
       @param cl
    */
    FloatClass(const FloatClass &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    FloatClass& operator=(const FloatClass &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new FloatClass(*this);}

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
    virtual FloatClass *asFloatClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const FloatClass *asFloatClass() const {return this;}

    // ----------------------------------------------------------------------
    // FloatClass Private Part
    // ----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;

    // ----------------------------------------------------------------------
    // FloatClass Restricted Access (conceptually private)
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

  class FloatClassPtr : public BasicClassPtr {

  public:
    FloatClassPtr(FloatClass *o = 0) : BasicClassPtr(o) { }

    FloatClass *getFloatClass() {return dynamic_cast<FloatClass *>(o);}
    const FloatClass *getFloatClass() const {return dynamic_cast<FloatClass *>(o);}

    FloatClass *operator->() {return dynamic_cast<FloatClass *>(o);}
    const FloatClass *operator->() const {return dynamic_cast<FloatClass *>(o);}
  };

  typedef std::vector<FloatClassPtr> FloatClassPtrVector;

  /**
     @}
  */

}

#endif
