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


#include "eyedb_p.h"
	
using namespace std;

namespace eyedb {

  const Oid Oid::nullOid;

  /*
    Oid::Oid()
    {
    mset(&oid, 0, sizeof(eyedbsm::Oid));
    }
  */

  Oid::Oid(const eyedbsm::Oid &_oid)
  {
    oid = _oid;
  }

  Oid::Oid(const Oid &_oid)
  {
    oid = _oid.oid;
  }

  Oid::Oid(const eyedbsm::Oid *_oid)
  {
    oid = *_oid;
  }

  Oid::Oid(const char *str)
  {
    oid = stringGetOid(str);
  }

  Oid::Oid(eyedbsm::Oid::NX nx, eyedbsm::Oid::DbID dbid,
	   eyedbsm::Oid::Unique unique)
  {
    oid.setNX(nx);
    eyedbsm::dbidSet(&oid, dbid);
    oid.setUnique(unique);
  }

  const char *Oid::getString() const
  {
    if (isValid())
      return OidGetString(&oid);
    return NullString;
  }

  void Oid::setOid(const eyedbsm::Oid &_oid)
  {
    oid = _oid;
  }

  Oid &Oid::operator=(const Oid &_oid)
  {
    oid = _oid.oid;
    return *this;
  }

  ostream& operator<<(ostream& os, const Oid &oid)
  {
    os << oid.toString();
    return os;
  }

  ostream& operator<<(ostream& os, const Oid *oid)
  {
    if (!oid)
      return os << "NULL";

    os << oid->toString();
    return os;
  }

  OidArray::OidArray(const Oid *_oids, int _count)
  {
    oids = 0;
    count = 0;
    set(_oids, _count);
  }

  OidArray::OidArray(const Collection *coll)
  {
    oids = 0;
    count = 0;
    coll->getElements(*this);
  }

  OidArray::OidArray(const OidArray &oid_arr)
  {
    oids = 0;
    count = 0;
    *this = oid_arr;
  }

  OidArray::OidArray(const OidList &list)
  {
    count = 0;
    int cnt = list.getCount();
    if (!cnt)
      {
	oids = 0;
	return;
      }

    oids = (Oid *)malloc(sizeof(Oid) * cnt);
    memset(oids, 0, sizeof(Oid) * cnt);

    OidListCursor c(list);
    Oid oid;

    for (; c.getNext(oid); count++)
      oids[count] = oid;
  }

  OidArray& OidArray::operator=(const OidArray &oidarr)
  {
    set(oidarr.oids, oidarr.count);
    return *this;
  }

  void OidArray::set(const Oid *_oids, int _count)
  {
    free(oids);
    count = _count;
    if (count)
      {
	int size = count * sizeof(Oid);
	oids = (Oid *)malloc(size);
	if (_oids)
	  memcpy(oids, _oids, size);
      }
    else
      oids = 0;
  }

  OidList *
  OidArray::toList() const
  {
    return new OidList(*this);
  }

  OidArray::~OidArray()
  {
    free(oids);
  }

  OidList::OidList()
  {
    list = new LinkedList();
  }

  OidList::OidList(const OidArray &oid_array)
  {
    list = new LinkedList();
    for (int i = 0; i < oid_array.getCount(); i++)
      insertOidLast(oid_array[i]);
  }

  int OidList::getCount(void) const
  {
    return list->getCount();
  }

  void
  OidList::insertOid(const Oid &oid)
  {
    list->insertObject(new Oid(oid));
  }

  void
  OidList::insertOidFirst(const Oid &oid)
  {
    list->insertObjectFirst(new Oid(oid));
  }

  void
  OidList::insertOidLast(const Oid &oid)
  {
    list->insertObjectLast(new Oid(oid));
  }

  Bool
  OidList::suppressOid(const Oid &oid)
  {
    LinkedListCursor c(list);
    Oid *xoid;
    while (c.getNext((void *&)xoid))
      if (xoid->compare(oid))
	{
	  list->deleteObject(xoid);
	  return True;
	}

    return False;
  }

  Bool
  OidList::exists(const Oid &oid) const
  {
    LinkedListCursor c(list);
    Oid *xoid;
    while (c.getNext((void *&)xoid))
      if (xoid->compare(oid))
	return True;
    return False;
  }

  void
  OidList::empty()
  {
    list->empty();
  }

  OidArray *
  OidList::toArray() const
  {
    int cnt = list->getCount();
    if (!cnt)
      return new OidArray();
    Oid *arr = (Oid *)malloc(sizeof(Oid) * cnt);
    memset(arr, 0, sizeof(Oid) * cnt);

    LinkedListCursor c(list);
    Oid *xoid;
    for (int i = 0; c.getNext((void *&)xoid); i++)
      arr[i] = *xoid;
  
    OidArray *oid_arr = new OidArray(arr, cnt);
    free(arr);
    return oid_arr;
  }

  OidList::~OidList()
  {
    LinkedListCursor c(list);
    Oid *xoid;
    while (c.getNext((void *&)xoid))
      delete xoid;
    delete list;
  }

  OidListCursor::OidListCursor(const OidList &oidlist)
  {
    c = new LinkedListCursor(oidlist.list);
  }

  OidListCursor::OidListCursor(const OidList *oidlist)
  {
    c = new LinkedListCursor(oidlist->list);
  }

  Bool
  OidListCursor::getNext(Oid &oid)
  {
    Oid *xoid;
    if (c->getNext((void *&)xoid))
      {
	oid = *xoid;
	return True;
      }

    return False;
  }

  OidListCursor::~OidListCursor()
  {
    delete c;
  }
}
