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
#include "CollectionBE.h"
#include <eyedb/eyedb_p.h>
#include "IteratorBEEngine.h"
#include "kernel.h"
#include <assert.h>
#include <eyedblib/m_malloc.h>

#define SUPPORT_BASIC_COLL

// This variable (SKIP_INVALID_OID) has been introduced and
// disconnected on the 08/01/99: because
// there are no reasons to skip invalid oids in a query:
// this could lead to inconsistant behaviours!
// #define SKIP_INVALID_OID

namespace eyedb {

  Status IteratorBEEngine::getStatus() const
  {
    return status;
  }

  IteratorBEEngine::~IteratorBEEngine()
  {
  }

  // iterator attribute

  IteratorBEEngineAttribute::IteratorBEEngineAttribute
  (Database *_db,
   const Attribute *_agr, int _ind,
   Data start, Data end, Bool sexcl, Bool eexcl, int x_size)
  {
    eyedbsm::Idx *idx;

    state = False;

    db = _db;
    dbh = NULL;
    agr = _agr;
    ind = _ind;
  
    int maxind;

    assert(0);
    //status = ((Attribute *)agr)->getIdx(ind, maxind, size, idx);

    if (status)
      return;

    if (!idx) {
      // in fact, this is not an error: must use them class object
      // collection!
      status = Exception::make(IDB_ITERATOR_ATTRIBUTE_NO_IDX);
      curs = 0;
      return;
    }

    if (x_size != size) {
      status = Exception::make(IDB_ITERATOR_ATTRIBUTE_INVALID_SIZE, "size `%d' expected, got `%d'", size, x_size);
      curs = 0;
      return;
    }

    if (ind != Attribute::composedMode && (ind < 0 || ind > maxind)) {
      status = Exception::make(IDB_ITERATOR_ATTRIBUTE_INVALID_INDICE, "indice in [0, %d] expected, got `%d'", maxind, ind);
      curs = 0;
      return;
    }

    if (ind == Attribute::composedMode)
      curs = new eyedbsm::HIdxCursor(idx->asHIdx(),
				     (void const *)start, (void const *)end,
				     (eyedbsm::Boolean)sexcl, (eyedbsm::Boolean)eexcl);
    else {
      unsigned char *es = (unsigned char *)malloc(sizeof(eyedblib::int32) + x_size);
      memcpy(es, &ind, sizeof(eyedblib::int32));
      memcpy(es+sizeof(eyedblib::int32), start, x_size);

      unsigned char *ee = (unsigned char *)malloc(sizeof(eyedblib::int32) + x_size);
      memcpy(ee, &ind, sizeof(eyedblib::int32));
      memcpy(ee+sizeof(eyedblib::int32), end, x_size);

      curs = new eyedbsm::BIdxCursor(idx->asBIdx(), (void const *)es,
				     (void const *)ee,
				     (eyedbsm::Boolean)sexcl, (eyedbsm::Boolean)eexcl);
      free(ee);
      free(es);
    }

    state = True;
    status = Success;
  }

  Status IteratorBEEngineAttribute::scanNext(int wanted, int *found, IteratorAtom *atom_array)
  {
    int n = 0;

    if (state && curs) {
      eyedbsm::Boolean sefound = eyedbsm::False;
      for (; n < wanted; n++) {
	Oid oid;
	eyedbsm::Status se_status = curs->next(&sefound, &oid);

	if (se_status)
	  return Exception::make(se_status->err, se_status->err_msg);

	if (!sefound) {
	  state = False;
	  break;
	}

	std::cerr << "IteratorBEEngineAttribute::scanNext: oid must be swapped"
		  << std::endl;

	atom_array[n].type = IteratorAtom_OID;
	atom_array[n].oid = *oid.getOid();
      }
    }

    *found = n;
    return Success;
  }

  IteratorBEEngineAttribute::~IteratorBEEngineAttribute()
  {
    delete curs;
  }

  // iterator collection

  IteratorBEEngineCollection::IteratorBEEngineCollection(CollectionBE *_collbe, Bool _index)
  {
    eyedbsm::Idx *idx;

    collbe = _collbe;
    state = False;

    index = _index;
    db = collbe->getDatabase();
    dbh = collbe->getDbHandle();

    eyedblib::int16 item_size = collbe->getItemSize();
    //dim = 0;
    eyedbsm::Idx *idx1, *idx2;
    collbe->getIdx(&idx1, &idx2);

    if (idx2) {
      idx = idx2;
      data = new unsigned char[item_size];
      buff = new eyedbsm::Idx::Key(sizeof(int));
    }
    else {
      idx = idx1;
      data = 0;
      buff = new eyedbsm::Idx::Key(item_size);
    }

    if (!idx)
      {
	status = Exception::make(IDB_ERROR, "no index found in collection BE");
	curs = 0;

	return;
      }

    //if (collbe->isBIdx())
    if (idx->asBIdx()) {
      assert(idx->asBIdx());
      curs = new eyedbsm::BIdxCursor(idx->asBIdx(), 0, 0, eyedbsm::False, eyedbsm::False);
    }
    else {
      assert(idx->asHIdx());
      curs = new eyedbsm::HIdxCursor(idx->asHIdx(), 0, 0, eyedbsm::False, eyedbsm::False);
    }

    state = True;
    status = Success;
  }

  Status IteratorBEEngineCollection::scanNext(int wanted, int *found, IteratorAtom *atom_array)
  {
    if (status)
      return status;

    int n = 0;

    if (state && curs) {
      eyedbsm::Boolean sefound = eyedbsm::False;
      IteratorAtom *atom = atom_array;
      for (; n < wanted; ) {
	unsigned int ind;
	eyedbsm::Status se_status;

	if (data) {
	  se_status = curs->next(&sefound, data, buff);
	  memcpy(&ind, buff->getKey(), sizeof(int));
	}
	else {
	  se_status = curs->next(&sefound, &ind, buff);
	}

	if (se_status)
	  return Exception::make(se_status->err, se_status->err_msg);

	if (!sefound) {
	  state = False;
	  break;
	}

	if (index) {
	  atom->type = IteratorAtom_INT32;
	  atom->i32 = ind;
	  atom++;
	  n++;
	  if (n == wanted)
	    break;
	}

	void *k = (data ? data : buff->getKey());

	collbe->decode(k, *atom);
	    
	n++;
	atom++;
      }
    }

    *found = n;
    return Success;
  }

  IteratorBEEngineCollection::~IteratorBEEngineCollection()
  {
    delete curs;
    delete buff;
    delete [] data;
  }
}
