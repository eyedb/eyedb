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


#ifndef _EYEDB_RECMODE_H
#define _EYEDB_RECMODE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  enum RecModeType {
    RecMode_NoRecurs,
    RecMode_FullRecurs,
    RecMode_FieldNames,
    RecMode_Predicat
  };

  struct Attribute;
  class Object;
  class ObjectPtr;

  /**
     Not yet documented.
  */
  class RecMode {

    // ----------------------------------------------------------------------
    // RecMode Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param typ
    */
    RecMode(RecModeType typ);

    /**
       Not yet documented
       @param p
    */
    RecMode(Bool (*p)(const Object *, const Attribute *, int));

    /**
       Not yet documented
       @param first
    */
    RecMode(const char *first, ...);

    /**
       Not yet documented
       @param no
       @param first
    */
    RecMode(Bool no, const char *first, ...);

    /**
       Not yet documented
       @param agreg
       @param n
       @param o
       @return
    */
    Bool isAgregRecurs(const Attribute *agreg, int n = 0, const Object *o = NULL) const;

    /**
       Not yet documented
       @return
    */
    RecModeType getType() const {return type;}


    static const RecMode *FullRecurs;
    static const RecMode *NoRecurs;

    // ----------------------------------------------------------------------
    // RecMode Private Part
    // ----------------------------------------------------------------------
  private:
    RecModeType type;

    union {
      struct {
	Bool _not;
	int fnm_cnt;
	char **fnm;
      } fnm;
      Bool (*pred)(const Object *, const Attribute *, int);
    } u;

    // ----------------------------------------------------------------------
    // RecMode Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init(void);
    static void _release(void);
  };

  extern const RecMode *FullRecurs;
  extern const RecMode *NoRecurs;

  /**
     @}
  */

}

#endif
