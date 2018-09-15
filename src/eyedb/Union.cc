/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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


//
// Union methods
// 

#include "eyedb_p.h"
#include <iostream>

namespace eyedb {

  const Attribute *Union::setCurrentItem(const Attribute *ni)
  {
    const Attribute *old = item;
    item = ni;

    Offset offset = IDB_OBJ_HEAD_SIZE;
    Size alloc_size = idr->getSize();
    Data data = idr->getIDR();

    if (item)
      {
	eyedblib::int16 num = item->getNum();
	int16_code(&data, &offset, &alloc_size, &num);
      }
    else
      {
	eyedblib::int16 k = -1;
	int16_code(&data, &offset, &alloc_size, &k);
      }

    //  printf("SETCURRENTITEM %s\n", ni->getName());
    return old;
  }

  const Attribute *Union::getCurrentItem() const
  {
    /*
      if (item)
      printf("  GETCURRENTITEM %s\n", item->getName());
    */
    return item;
  }

  const Attribute *Union::decodeCurrentItem(const Class *cls,
					    Data idr)
  {
    eyedblib::int16 num;
    const Attribute *item;

    Offset offset = IDB_OBJ_HEAD_SIZE;
    int16_decode(idr, &offset, &num);

    if (num >= 0)
      item = ((AgregatClass *)cls)->getAttributes()[num];
    else
      item = 0;

    return item;
  }

  const Attribute *Union::decodeCurrentItem()
  {
    item = decodeCurrentItem(getClass(), idr->getIDR());
    return item;
  }
}
