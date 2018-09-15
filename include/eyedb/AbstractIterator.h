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


#ifndef _EYEDB_ABSTRACT_ITERATOR_H
#define _EYEDB_ABSTRACT_ITERATOR_H

namespace eyedb {

  /**
     @addtogroup eyedb 
     @{
  */

  class Oid;
  class Object;
  class Value;
  class ValueArray;
  class Iterator;
  class OQL;

  /**
     The base class of all iterators.
  */
  class AbstractIterator {
  
  public:
    /**
       Constructs an AbstractIterator.
    */
    AbstractIterator();

    /**
       Go to the next element in the iteration. 

       @param oid	a reference to an object id that is used to return the next object
       @return true if the iterator has more elements
    */
    virtual Bool next(Oid &oid) = 0;

    /**
       Go to the next element in the iteration, specifying the recursion mode.

       @param obj	a reference to a pointer to an Object that is used to return the next object
       @param recmode	the recursion mode
       @return true if the iterator has more elements
    */
    virtual Bool next(ObjectPtr &obj,
		      const RecMode *recmode = RecMode::NoRecurs) = 0;

    /**
       Go to the next element in the iteration, specifying the recursion mode.

       @param obj	a reference to a pointer to an Object that is used to return the next object
       @param recmode	the recursion mode
       @return true if the iterator has more elements
    */
    virtual Bool next(Object *&obj,
		      const RecMode *recmode = RecMode::NoRecurs) = 0;

    /**
       Go to the next element in the iteration. 

       @param v a reference to a Value that is used to return the next object
       @return true if the iterator has more elements
    */
    virtual Bool next(Value &v) = 0;

    /**
       Deconstructs an AbstractIterator.
    */
    virtual ~AbstractIterator() = 0;

  protected:
    Status status;
  };

  /**
     @}
  */

}
#endif
