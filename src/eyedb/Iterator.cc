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
// MIND: error sur le `_count'
// la methode getCount() ne marche que pour le queryLangCreate()
// pour les autres types de queries, le _count n'est pas mis a jour.
//
// a corriger!
//

#include "eyedb_p.h"
#include "api_lib.h"
#include <assert.h>
#include "Attribute_p.h"
#include "eyedblib/butils.h"

//#define TRACK_BUG

namespace eyedbsm {
  extern Boolean backend;
}
namespace eyedb {

#define TRY_GETELEMS_GC

  //
  // this method initializes a few OQL built-in functions in the
  // opened database.

  Status
  Iterator::initDatabase(Database *db)
  {
    assert(0);
    return 0;
  }

  void Iterator::init(Database *_db)
  {
    curatom = 0;
    readatom = 0;
    buff_len = 64;
    _count = 0;
    schinfo = 0;
#ifdef IDB_VECT
    buffatom = idbNewVect(IteratorAtom, buff_len);
#else
    assert(0);
    buffatom = new IteratorAtom[buff_len];
#endif
    memset(buffatom, 0, sizeof(IteratorAtom) * buff_len);

    state = True;
    db = _db;
    curqid = 0;
    nqids = 1;
    //  qid = new int[1];
    qid = (int *)calloc(1, sizeof(int));
    *qid = 0;
  }

  Iterator::Iterator(Class *cls, Bool subclass)
  {
    init(cls->getDatabase());

    if (db)
      {
	Collection *extent;
	if (subclass)
	  {
#define SUBCLASS_OPTIM
#ifdef SUBCLASS_OPTIM
	    Class **subclasses;
	    unsigned int subclass_count;

	    if (status = cls->getSubClasses(subclasses, subclass_count))
	      return;

	    free(qid);
	    qid = (int *)calloc(subclass_count, sizeof(int));
	    nqids = 0;

	    for (int i = 0; i < subclass_count; i++)
	      {
		// added True the 2/3/00
		if (status = subclasses[i]->getExtent(extent, True))
		  break;

		// added extent->getCount() the 2/3/00
		if (extent && extent->getCount())
		  {
		    status = StatusMake
		      (queryCollectionCreate(db->getDbHandle(),
					     extent->getOid().getOid(), False, &qid[nqids++]));
		    if (status)
		      break;
		  }
	      }
#else
	    const LinkedList *_class = db->getSchema()->getClassList();
	    free(qid);
	    qid = (int *)calloc(_class->getCount(), sizeof(int));
	    nqids = 0;

	    Class *cl;
	    LinkedListCursor c(_class);

	    while (c.getNext((void *&)cl))
	      {
		Bool is;
		if (status = cls->isSuperClassOf(cl, &is))
		  break;

		if (is)
		  {
		    if (status = cl->getExtent(extent))
		      break;

		    if (extent)
		      {
			status = StatusMake
			  (queryCollectionCreate(db->getDbHandle(),
						 extent->getOid().getOid(), False, &qid[nqids++]));
			if (status)
			  break;
		      }
		  }
	      }
#endif
	  }
	else
	  {
	    if (status = cls->getExtent(extent))
	      return;
	    if (extent)
	      status = StatusMake
		(queryCollectionCreate(db->getDbHandle(),
				       extent->getOid().getOid(), False, &qid[0]));
	    else
	      status = Success;
	  }
      }
    else
      status = Exception::make(IDB_ITERATOR_ERROR,
			       "database is not set for class query on '%s'", cls->getName());
  }

  Iterator::Iterator(const Collection *coll, Bool index)
  {
    init((Database *)coll->getDatabase());

    (void)const_cast<Collection *>(coll)->loadDeferred();

    if (coll->isRemoved())
      {
	status = Exception::make(IDB_OBJECT_REMOVE_ERROR,
				 "object '%s' is removed.",
				 coll->getOid().toString());
	return;
      }

    if (!coll->getOidC().isValid())
      {
	/*
	  status = Exception::make(IDB_ITERATOR_ERROR,
	  "cannot iterate on a non persistent "
	  "collection");
	*/
	status = Success;
	return;
      }

    if (db)
      status = StatusMake
	(queryCollectionCreate
	 (db->getDbHandle(), coll->getOidC().getOid(), index, &qid[0]));
    else
      status = Exception::make(IDB_ITERATOR_ERROR,
			       "database is not set for collection "
			       "query on '%s'", coll->getName());

    if (!status)
      _count = coll->getCount();
  }

  Iterator::Iterator(Database *_db, const Attribute *agritem, int ind,
		     Data start, Data end, Bool sexcl, Bool eexcl,
		     int x_size)
  {
    init(_db);

    if (db)
      status = StatusMake
	(queryAttributeCreate(db->getDbHandle(),
			      agritem->getClassOwner()->getOid().getOid(),
			      agritem->getNum(), ind, start, end,
			      sexcl, eexcl, x_size, &qid[0]));
    else
      status = Exception::make(IDB_ITERATOR_ERROR, "database is not set for attribute query");
  }

  Iterator::Iterator(Database *_db, const Attribute *agritem, int ind,
		     Data start, int x_size)
  {
    init(_db);

    if (db)
      status = StatusMake
	(queryAttributeCreate(db->getDbHandle(),
			      agritem->getClassOwner()->getOid().getOid(),
			      agritem->getNum(), ind, start, start,
			      False, False, x_size, &qid[0]));
    else
      status = Exception::make(IDB_ITERATOR_ERROR, "database is not set for attribute query");
  }

  Status Iterator::getStatus() const
  {
    return status;
  }

  Status Iterator::scanNext(Bool &found, Oid &oid)
  {
    return scanNext(&found, &oid);
  }

  Status Iterator::scanNext(Bool *found, Oid *oid)
  {
    *found = False;

    if (status)
      return status;

    IDB_CHECK_INTR();

    for (;;)
      {
	Status s;
	IteratorAtom atom;
	s = scanNext(found, &atom);
	if (s)
	  return s;

	if (*found)
	  {
	    if (atom.type == IteratorAtom_OID)
	      {
		*oid = atom.oid;
		break;
	      }
	  }
	else
	  break;
      }

    return Success;
  }

  Status Iterator::scanNext(Bool *found, Value *value)
  {
    return scanNext(*found, *value);
  }

  Status Iterator::scanNext(Bool &found, Value &value)
  {
#define MAXDEPTH 64
    Value *lists[MAXDEPTH];
    int depth = 0;

    for (;;) {
      IteratorAtom atom;
      status = scanNext(&found, &atom);

      if (status || !found)
	return status;

#ifndef NEW_ITER_ATOM
      if (atom.type == IteratorAtom_LISTSEP) {
	if (atom.open) {
	  Value *list = new Value(new LinkedList(), Value::tList);
	  if (depth-1 >= 0)
	    lists[depth-1]->list->insertObjectLast(list);
	  lists[depth++] = list;
	  continue;
	}
	else if (!--depth) {
	  value = *lists[0];
	  delete lists[0];
	  return Success;
	}
      }
      else
#endif
	{
	  Value *avalue = atom.toValue();
	  if (depth)
	    lists[depth-1]->list->insertObjectLast(avalue);
	  else {
	    value = *avalue;
	    delete avalue;
	    return Success;
	  }
	}
    }
  }

  Status Iterator::scanNext(Bool *found, IteratorAtom *atom)
  {
    *found = False;

    if (status)
      return status;

    if (!state)
      return Success;

    IDB_CHECK_INTR();

    for (;;) {
      if (curatom >= readatom) {
	if (!qid[curqid]) {
	  state = False;
	  return Success;
	}

	status = StatusMake(queryScanNext(db->getDbHandle(), qid[curqid], buff_len, &readatom, buffatom));
	if (status) {
	  state = False;
	  return status;
	}
	curatom = 0;
      }
    
      assert(readatom <= buff_len);
      if (readatom)
	break;
      else {
	curqid++;
	if (curqid >= nqids) {
	  state = False;
	  return Success;
	}
      }
    }

    *atom = buffatom[curatom];
#ifndef TRACK_BUG
    buffatom[curatom++].garbage();
#else
    curatom++;
#endif
    *found = True;
    return status;
  }

#define INCR 512

  Status Iterator::scan(ValueArray &valarray, unsigned int max,
			unsigned int start)
  {
    int alloc = 0;
    Value *values = 0;
    int count, n;

    for (count = 0, n = 0; count < max; n++) {
      Bool found = False;
      Value avalue;
      /*
	#ifdef TRY_GETELEMS_GC
	avalue.setAutoObjGarbage(valarray.isAutoObjGarbage());
	#endif
      */

      Status s = scanNext(found, avalue);

      if (s)
	return s;

      if (!found)
	break;

      if (n < start)
	continue;

      if (count >= alloc) {
	int nalloc = alloc + INCR;
	Value *v = new Value[nalloc];
	for (int i = 0; i < alloc; i++)
	  v[i] = values[i];

	delete [] values;
	values = v;
	alloc = nalloc;
      }

#ifdef TRY_GETELEMS_GC
      values[count].setAutoObjGarbage(valarray.isAutoObjGarbage());
#endif

      values[count++] = avalue;
    }

    valarray.set(values, count, False);
    return Success;
  }

  Status Iterator::scan(int *count, IteratorAtom **atom_array)
  {
    *count = 0;
    int alloc = 0;
    *atom_array = 0;
    for (;;)
      {
	Bool found = False;
	IteratorAtom qatom;
	Status s = scanNext(&found, &qatom);
	if (s)
	  return s;
	if (!found)
	  break;
	if (*count >= alloc)
	  {
	    int nalloc = alloc + 512;
#ifdef IDB_VECT
	    IteratorAtom *tmp = idbNewVect(IteratorAtom, nalloc);
#else
	    assert(0);
	    IteratorAtom *tmp = new IteratorAtom[nalloc];
#endif
	    if (*atom_array)
	      {
		for (int i = 0; i < alloc; i++)
		  tmp[i] = (*atom_array)[i];

#ifdef IDB_VECT
		idbFreeVect(*atom_array, IteratorAtom, alloc);
#else
		assert(0);
		delete[] (*atom_array);
#endif
	      }
	    *atom_array = tmp;
	    alloc = nalloc;
	  }
	(*atom_array)[(*count)++] = qatom;
	qatom.garbage();
      }

    return Success;
  }

  Status Iterator::scanNext(Bool &found, Object *&o,
			    const RecMode *rcm)
  {
    return scanNext(&found, &o, rcm);
  }

  Status Iterator::scanNext(Bool *found, Object **o, const RecMode *rcm)
  {
    Oid oid;

    *found = False;

    IDB_CHECK_INTR();

    Status s = scanNext(found, &oid);

    if (s)
      return s;

    if (*found)
      return db->loadObject(&oid, o, rcm);

    return Success;
  }

  Status Iterator::scan(ObjectPtrVector &obj_vect, const RecMode *rcm)
  {
    ObjectArray obj_array; // true of false ?
    Status s = scan(obj_array, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status Iterator::scan(ObjectPtrVector &obj_vect, unsigned int max,
			unsigned int start, const RecMode *rcm)
  {
    ObjectArray obj_array; // true of false ?
    Status s = scan(obj_array, max, start, rcm);
    if (s)
      return s;
    obj_array.makeObjectPtrVector(obj_vect);
    return Success;
  }

  Status Iterator::scan(ObjectArray &obj_array, const RecMode *rcm)
  {
    return scan(obj_array, ~0, 0, rcm);
  }

  Status Iterator::scan(ObjectArray &obj_array, unsigned int max,
			unsigned int start, const RecMode *rcm)
  {
    OidArray oid_array;

    status = scan(oid_array, max, start);
    if (status)
      return status;

    int count = oid_array.getCount();
    Object **obj_x = (Object **)malloc(count * sizeof(Object *));
    for (int i = 0; i < count; i++)
      {
	/*if (!oid_array[i].isValid())
	  obj_x[i] = 0;
	  else*/ if (status = db->loadObject(&oid_array[i], &obj_x[i], rcm))
	    {
	      for (int j = 0; j < i; j++)
		obj_x[j]->release();
	      free(obj_x);
	      return status;
	    }
      }

    obj_array.set(obj_x, count);
    free(obj_x);
    return Success;
  }

  Status Iterator::scan(OidArray &oid_array, unsigned int max,
			unsigned int start)
  {
    int alloc = 0;
    Oid *oid_x = 0;
    int count, n;

    for (count = 0, n = 0; count < max; n++)
      {
	Bool found = False;
	Oid _oid;
	Status s = scanNext(&found, &_oid);

	if (s)
	  return s;

	if (!found)
	  break;

	if (n < start)
	  continue;

	if (count >= alloc)
	  {
	    int nalloc = alloc + 512;
	    oid_x = (Oid *)realloc(oid_x, nalloc * sizeof(Oid));
	    alloc = nalloc;
	  }

	oid_x[count++] = _oid;
      }

    if (oid_x)
      {
	oid_array.set(oid_x, count);
	free(oid_x);
      }

    return Success;
  }

  Iterator::~Iterator()
  {
    if (db)
      {
	for (int n = 0; n < nqids; n++)
	  if (qid[n])
	    queryDelete(db->getDbHandle(), qid[n]);
      }

#ifndef TRACK_BUG
    if (buffatom)
      {
#ifdef IDB_VECT
	idbFreeVect(buffatom, IteratorAtom, buff_len);
#else
	assert(0);
	delete [] buffatom;
#endif
      }
    //  delete[] qid;
    free(qid);
#endif

    //
    delete schinfo;
  }
}
