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


#ifndef _EYEDB_CHAR_H
#define _EYEDB_CHAR_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Char : public Basic {

    // -----------------------------------------------------------------------
    // Char Interface
    // -----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param c
    */
    Char(char c = 0);

    /**
       Not yet documented
       @param db
       @param c
       @param dataspace
    */
    Char(Database *db, char c = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented
       @param o
    */
    Char(const Char *o);

    /**
       Not yet documented
       @param o
    */
    Char(const Char &o);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new Char(*this);}

    /**
       Not yet documented
       @param o
       @return
    */
    Char& operator=(const Char &o);

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
    virtual Char *asChar() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Char *asChar() const {return this;}

    // -----------------------------------------------------------------------
    // Char Private Part
    // -----------------------------------------------------------------------
  private:
    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    char val;
    Status create();
    Status update();
  };

  class CharPtr : public BasicPtr {

  public:
    CharPtr(Char *o = 0) : BasicPtr(o) { }

    Char *getChar() {return dynamic_cast<Char *>(o);}
    const Char *getChar() const {return dynamic_cast<Char *>(o);}

    Char *operator->() {return dynamic_cast<Char *>(o);}
    const Char *operator->() const {return dynamic_cast<Char *>(o);}
  };

  typedef std::vector<CharPtr> CharPtrVector;

  extern CharClass    *Char_Class;
  extern const char char_class_name[];

  /**
     @}
  */

}

#endif
