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


#include <assert.h>
#include <eyedb/eyedb.h>
#include "IteratorAtom.h"
#include "IteratorBE.h"
#include "OQLBE.h"
#include "CollectionBE.h"
#include "BEQueue.h"

namespace eyedb {

  struct BELink {
    int id;
    Oid oid;
    void *data;
    void *info;

    BELink(int, void *, void * = 0);
    BELink(const Oid&, void *, void * = 0);
  };

  BELink::BELink(int i, void *d, void *xinfo)
  {
    id = i;
    data = d;
    info = xinfo;
  }

  BELink::BELink(const Oid &o, void *d, void *xinfo)
  {
    oid = o;
    data = d;
    info = xinfo;
  }

  BEQueue::BEQueue()
  {
    mxid = 100;
    iter_queue = new LinkedList();
    coll_queue = new LinkedList();
    oql_queue = new LinkedList();
  }

  /* QueryBE */
  IteratorBE *BEQueue::getIterator(int id)
  {
    LinkedListCursor c(iter_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->id == id)
	return (IteratorBE *)l->data;

    return 0;
  }

  int BEQueue::addIterator(IteratorBE *qbe)
  {
    BELink *l = new BELink(mxid, (void *)qbe);
    iter_queue->insertObject(l);
    return mxid++;
  }

  void BEQueue::removeIterator(int id)
  {
    LinkedListCursor c(iter_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->id == id)
	{
	  iter_queue->deleteObject(l);
	  delete l;
	  return;
	}
    //printf("WARNING removeIterator(%d) -> not found\n", id);
  }

  void BEQueue::removeIterator(IteratorBE *qbe)
  {
    LinkedListCursor c(iter_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->data == (void *)qbe)
	{
	  iter_queue->deleteObject(l);
	  delete l;
	  return;
	}
  }

  /* CollectionBE */
  CollectionBE *BEQueue::getCollection(const Oid *oid, void *info)
  {
    LinkedListCursor c(coll_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->oid.compare(*oid) && l->info == info)
	{
	  if (((CollectionBE *)l->data)->isLocked())
	    return (CollectionBE *)l->data;
	  return 0;
	}

    return 0;
  }

  void BEQueue::addCollection(CollectionBE *collbe, void *info)
  {
    assert(collbe->isLocked());
    BELink *l = new BELink(collbe->getOid(), (void *)collbe, info);
    //printf("BEQUEUE add %p %p `%d'\n", collbe, info, coll_queue->getCount());
    coll_queue->insertObject(l);
  }

  void BEQueue::removeCollection(CollectionBE *collbe, void *info)
  {
    LinkedListCursor c(coll_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->data == (void *)collbe && l->info == info)
	{
	  coll_queue->deleteObject(l);
	  //printf("removeCollection: %p\n", collbe);
	  delete l;
	  return;
	}

    //printf("removeCollection: %p not found\n", collbe);
    //  assert(0);
  }

  /* OQLBE */
  OQLBE *BEQueue::getOQL(int id)
  {
    LinkedListCursor c(oql_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->id == id)
	return (OQLBE *)l->data;

    return 0;
  }

  int BEQueue::addOQL(OQLBE *qbe)
  {
    BELink *l = new BELink(mxid, (void *)qbe);
    oql_queue->insertObject(l);
    return mxid++;
  }

  void BEQueue::removeOQL(int id)
  {
    LinkedListCursor c(oql_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->id == id)
	{
	  oql_queue->deleteObject(l);
	  delete l;
	  return;
	}
  }

  void BEQueue::removeOQL(OQLBE *qbe)
  {
    LinkedListCursor c(oql_queue);

    BELink *l;
    while (c.getNext((void* &)l))
      if (l->data == (void *)qbe)
	{
	  oql_queue->deleteObject(l);
	  delete l;
	  return;
	}
  }

  static void
  purge(LinkedList *list)
  {
    LinkedListCursor c(list);
    BELink *l;

    while (c.getNext((void* &)l))
      delete l;

    delete list;
  }

  BEQueue::~BEQueue()
  {
    purge(coll_queue);
    purge(iter_queue);
    purge(oql_queue);
  }

}
