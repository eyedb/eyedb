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


#ifndef _EYEDB_BEQUEUE_H
#define _EYEDB_BEQUEUE_H

namespace eyedb {

  class CollectionBE;
  class IteratorBE;

  class BEQueue {
    int mxid;
    LinkedList *iter_queue;
    LinkedList *coll_queue;
    LinkedList *oql_queue;

  public:
    BEQueue();

    // the following four methods should disapear
    IteratorBE *getIterator(int);
    void removeIterator(int);
    int addIterator(IteratorBE *);
    void removeIterator(IteratorBE *); 
    unsigned int getIteratorCount() const {return iter_queue->getCount();}

    OQLBE *getOQL(int);
    int addOQL(OQLBE *);
    void removeOQL(int);
    void removeOQL(OQLBE *);
    unsigned int getOQLCount() const {return oql_queue->getCount();}


    CollectionBE *getCollection(const Oid *, void *);
    void addCollection(CollectionBE *, void *);
    void removeCollection(CollectionBE *, void *);
    unsigned int getCollectionCount() const {return coll_queue->getCount();}

    ~BEQueue();
  };

}

#endif
