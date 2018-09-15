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


#include <eyedb/linklist.h>
#include <stdio.h>

namespace eyedb {

  struct Link {
    void *object;

    Link *prev, *next;

    Link(void *);
    ~Link() {}
  };

  LinkedListCursor::LinkedListCursor(const LinkedList &_list)
  {
    list = (LinkedList *)&_list;
    link = list->f_link;
  }

  LinkedListCursor::LinkedListCursor(const LinkedList *_list)
  {
    list = (LinkedList *)_list;
    link = (list ? list->f_link : 0);
  }

  void
  LinkedListCursor::restart()
  {
    link = (list ? list->f_link : 0);
  }

  int LinkedListCursor::getNext(void* &o)
  {
    if (!link)
      return 0;

    o = link->object;
    link = link->next;
    return 1;
  }

  Link::Link(void *o)
  {
    object = o;
    prev = next = 0;
  }

  LinkedList::LinkedList()
  {
    link_cnt = 0;
    f_link = (Link *)0;
    l_link = (Link *)0;
  }

  int LinkedList::insertObject(void *o)
  {
    return insertObjectLast(o);
  }

  int LinkedList::insertObjectLast(void *o)
  {
    Link *l = new Link(o);

    if (l_link)
      {
	l_link->next = l;
	l->prev = l_link;
      }
    else
      f_link = l;

    l_link = l;

    link_cnt++;
    return link_cnt - 1;
  }

  int LinkedList::insertObjectFirst(void *o)
  {
    Link *l = new Link(o);

    if (f_link)
      {
	f_link->prev = l;
	l->next = f_link;
      }
    else
      l_link = l;

    f_link = l;

    link_cnt++;
    return 0;
  }

  void *LinkedList::getObject(int pos) const
  {
    if (pos < 0 || pos >= link_cnt)
      return 0;

    Link *l = f_link;

    for (int j = 0; j < pos; j++)
      l = l->next;

    return l->object;
  }

  void LinkedList::delete_realize(Link *l)
  {
    if (l->prev)
      l->prev->next = l->next;
    else
      f_link = l->next;
  
    if (l->next)
      l->next->prev = l->prev;
    else
      l_link = l->prev;
  
    link_cnt--;
    delete l;
  }

  int LinkedList::deleteObject(void *o)
  {
    int pos = 0;
    Link *l = f_link;

    while (l)
      {
	if (l->object == o)
	  {
	    delete_realize(l);
	    return pos;
	  }
	l = l->next;
	pos++;
      }

    return -1;
  }

  int LinkedList::deleteObject(int pos)
  {
    if (pos < 0 || pos >= link_cnt)
      return -1;

    Link *l = f_link;

    int j;
    for (j = 0; j < pos; j++)
      l = l->next;

    delete_realize(l);
    return j;
  }

  int LinkedList::getPos(void *o) const
  {
    int pos = 0;
    Link *l = f_link;

    while (l) {
      if (l->object == o)
	return pos;
      l = l->next;
      pos++;
    }

    return -1;
  }

  int LinkedList::getCount(void) const
  {
    return link_cnt;
  }


  void* LinkedList::getFirstObject() const
  {
    return (f_link ? f_link->object : 0);
  }

  void* LinkedList::getLastObject() const
  {
    return (l_link ? l_link->object : 0);
  }

  LinkedListCursor *LinkedList::startScan() const
  {
    return new LinkedListCursor(this);
  }

  int LinkedList::getNextObject(LinkedListCursor *cursor, void* &o) const
  {
    if (!cursor->link)
      return 0;

    o = cursor->link->object;
    cursor->link = cursor->link->next;
    return 1;
  }

  void LinkedList::endScan(LinkedListCursor *cursor) const
  {
    delete cursor;
  }

  void LinkedList::applyToObjects(void (*f)(void *, void *), void *user_arg) const
  {
    Link *l;

    l = f_link;
    while (l)
      {
	(*f)(l->object, user_arg);
	l = l->next;
      }
  }


  void LinkedList::empty()
  {
    Link *l, *next;

    l = f_link;
    while (l)
      {
	next = l->next;
	delete l;
	l = next;
      }
    f_link = l_link = 0;
    link_cnt = 0;
  }

  LinkedList::~LinkedList()
  {
    empty();
  }
}
