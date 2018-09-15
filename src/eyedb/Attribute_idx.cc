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
#include "Attribute_p.h"
#include <assert.h>

#define ATTR_COMPLETE(DB, X) \
  if (!(X)->cls) \
     const_cast<Attribute *>(X)->cls = \
                      (DB)->getSchema()->getClass((X)->oid_cl); \
  if (!(X)->class_owner) \
     const_cast<Attribute *>(X)->class_owner = \
                      (DB)->getSchema()->getClass((X)->oid_cl_own)

namespace eyedb {

static const char fmt_error[] = "storage manager error '%s' reported when creating index in attribute '%s' in agregat class '%s'";

static Status
literalCollUpdate(Database *db, Object *o, Attribute *attr,
		  Index *idx, int from, const Oid &oid)
{
  Status s;
  eyedbsm::Status se_s;
  Collection *coll = 0;
  Bool isnull;
  eyedbsm::Idx *se_idx = idx->idx;

  if (!o) {
    s = db->loadObject(oid, o);
    if (s) return s;
  }
  if (s = attr->getValue
      (o, (Data *)&coll, 1, from, &isnull))
    return s;

  Iterator iter(coll);
  for (;;)
    {
      Bool found;
      Oid elem_oid;
      s = iter.scanNext(found, elem_oid);
      if (s) return s;

      if (!found)
	break;

      IDB_LOG(IDB_LOG_IDX_INSERT,
	      (Attribute::log_item_entry_fmt,
	       idx->getAttrpath().c_str(),
	       oid.toString(), attr->dumpData((Data)&elem_oid), from,
	       (isnull ? "null data" : "not null data")));
      
      if (se_s = se_idx->insert(&elem_oid, &oid))
	return Exception::make(IDB_INDEX_ERROR, fmt_error,
				  eyedbsm::statusGet(se_s),
				  attr->getName(),
				  attr->getClassOwner()->getName());
    }
  
  return Success;
}

Status
Attribute::createEntries(Database *db, const Oid &oid,
			    Object *o,
			    AttrIdxContext &idx_ctx,
			    Attribute *attrs[],
			    int depth, int last,
			    unsigned char entry[],
			    Index *idx)
{
  Status s;
  Attribute *attr = attrs[depth];
  assert(attr);
  Size sz = 0;

  if (depth != last) {
    Bool was_null;
    if (!o) {
      s = db->loadObject(oid, o);
      if (s) return s;
      was_null = True;
    }
    else
      was_null = False;

    if (attr->isVarDim()) {
      s = attr->getSize(o, sz);
      if (s) return s;
    }
    else
      sz = attr->getTypeModifier().pdims;

    for (int from = 0; from < sz; from++) {
      Object *x = 0;
      if (s = attr->getValue(o, (Data *)&x, 1, from, 0)) {
	if (was_null) o->release();
	return s;
      }

      if (s = createEntries(db, oid, x, idx_ctx, attrs,
			    depth+1, last, entry, idx)) {
	if (was_null) o->release();
	return s;
      }
    }

    if (was_null)
      o->release();

    return Success;
  }

  return createEntries_realize(db, attr, oid, o, idx_ctx, entry, idx);
}

Status
Attribute::createEntries_realize(Database *db,
				    Attribute *attr,
				    const Oid &oid,
				    Object *o,
				    AttrIdxContext &idx_ctx,
				    unsigned char entry[],
				    Index *idx)
{
  Status s;
  Bool isnull;
  eyedbsm::Status se_s;
  Size sz;
  eyedbsm::Idx *se_idx = idx->idx;
  assert(se_idx);

  if (!attr->isVarDim())
    sz = attr->getTypeModifier().pdims;

  if (attr->isString()) {
    if (o) {
      //printf("STRING VIA A LITERAL!\n");
      if (attr->isVarDim()) {
	s = attr->getSize(o, sz);
	if (s) return s;
	entry = !sz ? new unsigned char[2] : new unsigned char[sz+1];
      }

      /*
      if (!sz) {
	entry[1] = 0;
	isnull = True;
      }
      else*/ if (s = attr->getValue(o, (Data *)&entry[1], sz, 0, &isnull)) {
	if (attr->isVarDim())
	  delete [] entry;
	return s;
      }
    }
    else {
      //printf("STRING DIRECT\n");
      int nb;
      unsigned char *data = 0;
      if (attr->isVarDim()) {
	nb = Attribute::wholeData;
	s = attr->getTValue(db, oid, &data, nb, 0, &isnull, &sz);
      } else {
	nb = attr->getTypeModifier().pdims;
	s = attr->getTValue(db, oid, (Data *)&entry[1], nb, 0,
			    &isnull, &sz);
      }
      
      if (s) return s;

      if (attr->isVarDim()) {
	if (sz) {
	  entry = new unsigned char[sz+1];
	  memcpy(&entry[1], data, sz);
	  delete [] data;
	} else {
	  entry = new unsigned char[2];
	  entry[1] = 0;
	}
      }
    }

    IDB_LOG(IDB_LOG_IDX_INSERT,
	    (Attribute::log_comp_entry_fmt,
	     idx->getAttrpath().c_str(),
	     oid.toString(), &entry[1],
	     (isnull ? "null data" : "not null data")));

    entry[0] = (isnull ? idxNull : idxNotNull);

    se_s = se_idx->insert(entry, &oid);
    
    if (attr->isVarDim())
      delete[] entry;

    if (se_s)
      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				eyedbsm::statusGet(se_s),
				attr->name, attr->class_owner->getName());
    return Success;
  }

  //assert(!attr->isIndirect());

  if (!attr->isString()) {
    if (attr->getClass()->asCollectionClass()) {
      for (int from = 0; from < sz; from++) {
	s = literalCollUpdate(db, o, attr, idx, from, oid);
	if (s) return s;
      }
    }
    else {
      for (int from = 0; from < sz; from++) {
	if (o) {
	  //printf("NON STRING VIA A LITERAL\n");
	  s = attr->getValue
	    (o, (Data *)&entry[sizeof(char)+sizeof(eyedblib::int32)],
	     1, from, &isnull);
	  if (s) return s;
	} else {
	  unsigned int dummy;
	  s = attr->getTValue(db, oid,
			      (Data *)&entry[sizeof(char)+sizeof(eyedblib::int32)],
			      1, from, &isnull, &dummy);
	  //printf("NON STRING DIRECT sz=%d\n", sz);
	  if (s) return s;
	}

	entry[0] = (isnull ? idxNull : idxNotNull);
	memcpy(&entry[1], &from, sizeof(eyedblib::int32));

	IDB_LOG(IDB_LOG_IDX_INSERT,
		(Attribute::log_item_entry_fmt,
		 idx->getAttrpath().c_str(),
		 oid.toString(), attr->dumpData(&entry[5]), from,
		 (isnull ? "null data" : "not null data")));
      
	if (se_s = se_idx->insert(entry, &oid))
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				    eyedbsm::statusGet(se_s),
				    attr->name, attr->class_owner->getName());
      }
    }
  }

  return Success;
}

Status
Attribute::updateIndexEntries(Database *db, AttrIdxContext &idx_ctx)
{
  Status s;
  ClassSeqItem *classSeqItem;

  Class *cl = db->getSchema()->getClass(idx_ctx.getClassOwner());
  //printf("class_owner = '%s'\n", idx_ctx.getClassOwner());
  assert(cl);
  Attribute **attrs = new Attribute*[idx_ctx.getAttrCount()];
  Class *tcl = cl;
  for (int i = 0; i < idx_ctx.getAttrCount(); i++) {
    attrs[i] = (Attribute *)tcl->getAttribute(idx_ctx.getAttrName(i));
    tcl = (Class *)attrs[i]->getClass();
  }

  Attribute *attr = attrs[idx_ctx.getAttrCount()-1];

  Index *idx;
  s = attr->indexPrologue(db, idx_ctx, idx, True);
  if (s) return s;
  assert(idx);

  unsigned char *entry;

  if (attr->isString()) {
    if (attr->isVarDim())
      entry = 0;
    else
      entry = new unsigned char[1 + attr->getTypeModifier().pdims];
  }
  else {
    Offset dummy_off;
    Size idr_item_psize, dummy_size;
    attr->getPersistentIDR(dummy_off, idr_item_psize, dummy_size, dummy_size);

    entry = new unsigned char[sizeof(char) + sizeof(eyedblib::int32) +
			     idr_item_psize];
    //printf("allocating entry %d\n", idr_item_psize);
  }

  Iterator q(cl, False);

  for (;;) {
    Oid oid;
    Bool found;

    if (s = q.scanNext(found, oid)) {
      delete [] entry;
      delete [] attrs;
      return s;
    }

    if (!found)
      break;

    s = createEntries(db, oid, 0, idx_ctx,
		      attrs, 0, idx_ctx.getAttrCount()-1, entry, idx);
      
    if (s) {
      delete [] entry;
      delete [] attrs;
      return s;
    }

    //o->release();
  }

  delete [] entry;
  delete [] attrs;
  return Success;
}
}
