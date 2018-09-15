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


#ifndef _EYEDBLIB_LINKLIST_H
#define _EYEDBLIB_LINKLIST_H

#ifndef NO_IDB_LINKED_LIST

namespace eyedb {

  class LinkedListCursor;
  class Link;

  class LinkedList {

    // ----------------------------------------------------------------------
    // LinkedList Interface
    // ----------------------------------------------------------------------
  public:

    /**
       Not yet documented
    */
    LinkedList();

    /**
       Not yet documented
       @param o
       @return
    */
    int insertObject(void *o);

    /**
       Not yet documented
       @param o
       @return
    */
    int insertObjectLast(void *o);

    /**
       Not yet documented
       @param o
       @return
    */
    int insertObjectFirst(void *o);

    /**
       Not yet documented
       @param pos
       @return
    */
    void *getObject(int pos) const;

    /**
       Not yet documented
       @param o
       @return
    */
    int deleteObject(void *o);

    /**
       Not yet documented
       @param pos
       @return
    */
    int deleteObject(int pos);

    /**
       Not yet documented
       @param o
       @return
    */
    int getPos(void *o) const;

    /**
       Not yet documented
       @return
    */
    int getCount() const;

    /**
       Not yet documented
       @return
    */
    void *getFirstObject() const;

    /**
       Not yet documented
       @return
    */
    void *getLastObject() const;

    /**
       Not yet documented
       @return
    */
    LinkedListCursor *startScan() const;

    /**
       Not yet documented
       @param cursor
       @param o
       @return
    */
    int getNextObject(LinkedListCursor *cursor, void* &o) const;

    /**
       Not yet documented
       @param cursor
    */
    void endScan(LinkedListCursor *cursor) const;

    /**
       Not yet documented
       @param f
       @param user_arg
    */
    void applyToObjects(void (*f)(void *, void *), void *user_arg) const;

    /**
       Not yet documented
       @return
    */
    void empty();

    ~LinkedList();

    // ----------------------------------------------------------------------
    // Linked Private Part
    // ----------------------------------------------------------------------
  private:
    int link_cnt;
    Link *f_link, *l_link;
    void delete_realize(Link *);

    friend class LinkedListCursor;
  };

  class LinkedListCursor {

    // ----------------------------------------------------------------------
    // LinkedListCursor Interface
    // ----------------------------------------------------------------------
  public:
    LinkedListCursor(const LinkedList &);
    LinkedListCursor(const LinkedList *);

    int getNext(void* &);

    void restart();

    ~LinkedListCursor() {}

  private:
    friend class LinkedList;
    Link *link;
    LinkedList *list;
  };

}

#endif

#endif
