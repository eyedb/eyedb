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


#ifndef _EYEDB_BASIC_CLASS_H
#define _EYEDB_BASIC_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  // BasicClass
  /**
     Not yet documented.
  */
  class BasicClass : public Class {

    // -----------------------------------------------------------------------
    // BasicClass Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param s
    */
    BasicClass(const char *s);

    /**
       Not yet documented
       @param db
       @param s
    */
    BasicClass(Database *db, const char *s);

    /**
       Not yet documented
       @param cl
    */
    BasicClass(const BasicClass &cl);

    /**
       Not yet documented
       @param cl
    */
    BasicClass& operator=(const BasicClass &cl);

    /**
       Not yet documented
       @param ??
       @return
    */
    Status setValue(Data);

    /**
       Not yet documented
       @param ??
       @return
    */
    Status getValue(Data*) const;

    /**
       Not yet documented
       @return
    */
    Status attrsComplete();

    /**
       Not yet documented
       @return
    */
    int getCode() const;

    /**
       Not yet documented
       @param useAsRef
       @return
    */
    const char *getCName(Bool useAsRef = False) const;

    /**
       Not yet documented
       @param recmode
       @return
    */
    Status remove(const RecMode* recmode= RecMode::NoRecurs);

    /**
       Not yet documented
       @param fd
       @param indent
       @param inidata
       @param data
       @param tmod
       @return
    */
    virtual Status traceData(FILE *fd, int indent, Data inidata,
			     Data data, TypeModifier *tmod = NULL) const = 0; 

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE* fd = stdout, unsigned int flags = 0, const RecMode * recmode= RecMode::FullRecurs) const;

    /**
       Not yet documented
       @return
    */
    virtual BasicClass *asBasicClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const BasicClass *asBasicClass() const {return this;}

    /**
       Not yet documented
       @param cl
       @return
    */
    virtual Status loadComplete(const Class *cl);

    // -----------------------------------------------------------------------
    // BasicClass Protected Part
    // -----------------------------------------------------------------------
  protected:
    eyedblib::int16 code;
    Status create();
    Status update();
    char Cname[32];

    // -----------------------------------------------------------------------
    // BasicClass Private Part
    // -----------------------------------------------------------------------
  private:
    void copy_realize(const BasicClass &);
  };

  class BasicClassPtr : public ClassPtr {

  public:
    BasicClassPtr(BasicClass *o = 0) : ClassPtr(o) { }

    BasicClass *getBasicClass() {return dynamic_cast<BasicClass *>(o);}
    const BasicClass *getBasicClass() const {return dynamic_cast<BasicClass *>(o);}

    BasicClass *operator->() {return dynamic_cast<BasicClass *>(o);}
    const BasicClass *operator->() const {return dynamic_cast<BasicClass *>(o);}
  };

  typedef std::vector<BasicClassPtr> BasicClassPtrVector;

  /**
     @}
  */

}

#endif
