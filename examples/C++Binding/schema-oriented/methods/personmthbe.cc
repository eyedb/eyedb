
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


#include <eyedb/eyedb.h>

#include "person.h"

using namespace eyedb;

/*
 * perform the following operations:
 *  - copy this file to personmthbe.cc
 *  - implement the methods
 *  - run `gnumake -f Makefile.person all'
 *  - run `cp personmthbe-2.4.4 $EYEDBROOT/etc/so/'
 *  - run `chmod a+rx $EYEDBROOT/etc/so/personmthbe-2.4.4'
 */

static bool __init = false;

#define PACK_INIT() if (!__init) {person::init(); __init = true;}

//
// int32 Person::change_address(in string, in string, out string, out string)
//

Status
__method__OUT_int32_change_address_Person__IN_string__IN_string__OUT_string__OUT_string(Database *db, BEMethod_C *m, Object *o, const char * street, const char * town, char * &oldstreet, char * &oldtown, eyedblib::int32 &retarg)
{
  PACK_INIT();

  //
  // User Implementation
  //

  Person *p = (Person *)o;
  oldstreet = Argument::dup((p->getAddr()->getStreet()).c_str());
  oldtown = Argument::dup((p->getAddr()->getTown()).c_str());

  p->getAddr()->setStreet(street);
  p->getAddr()->setTown(town);

  p->store();

  retarg = 1;

  return Success;
}

//
// int32 Person::getPersonCount()
//

Status
__method_static_OUT_int32_getPersonCount_Person(Database *db, BEMethod_C *m, eyedblib::int32 &retarg)
{
  PACK_INIT();

  //
  // User Implementation
  //

  retarg = 1283;
  return Success;
}

