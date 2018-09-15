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
#include "OQLBE.h"
#include "IteratorBE.h"
#include "CollectionBE.h"
#include "IteratorBEEngine.h"
#include "BEQueue.h"

namespace eyedb {

LinkedList *IteratorBE::def_cl_list;

IteratorBE::IteratorBE(Database *_db)
{
  collbe = 0;
  schinfo = 0;
  qiter = 0;
}

IteratorBE::IteratorBE(Database *db, const Attribute *agr, int ind,
		       Data start, Data end, Bool sexcl,
		       Bool eexcl, int x_size)
{
  qiter = new IteratorBEEngineAttribute(db, agr, ind, start, end, 
					  sexcl, eexcl, x_size);
  status = qiter->getStatus();
  if (status == Success)
    qid = db->getBEQueue()->addIterator(this);

  collbe = 0;
  schinfo = 0;
}

IteratorBE::IteratorBE(CollectionBE *_collbe, Bool index)
{
  collbe = _collbe;
  qiter = new IteratorBEEngineCollection(collbe, index);

  status = qiter->getStatus();
  if (status == Success)
    qid = collbe->getDatabase()->getBEQueue()->addIterator(this);
  schinfo = 0;
}

Status IteratorBE::getStatus() const
{
  return status;
}

/*
SchemaInfo *IteratorBE::getSchemaInfo()
{
  if (!schinfo && qiter)
    schinfo =
      new SchemaInfo(((IteratorBEEngineOQL *)qiter)->getSchemaInfo());
  return schinfo;
}
*/

Status IteratorBE::scanNext(int wanted, int *found, IteratorAtom *atom_array)
{
  IDB_CHECK_INTR();

  if (qiter)
    return qiter->scanNext(wanted, found, atom_array);

  return Success;
}

int IteratorBE::getQid() const
{
  return qid;
}

IteratorBE::~IteratorBE()
{
  delete qiter;
  if (collbe && !collbe->isLocked())
    delete collbe;
  delete schinfo;
}
}
