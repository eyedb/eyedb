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
#include "oql_p.h"
#include "OQLBEIterator.h"
#include "kernel.h"
#include <assert.h>
#include <eyedblib/m_malloc.h>

namespace eyedb {

  Status OQLBEIterator::getStatus() const
  {
    return status;
  }

  OQLBEIterator::~OQLBEIterator()
  {
  }

  OQLBEIteratorOQL::OQLBEIteratorOQL(Database *_db,
				     DbHandle *_dbh,
				     const char *qlstr)
  {
    db = _db;
    dbh = _dbh;
    value = 0;
    cl_list.empty();

    status = oqml_realize(db, (char *)qlstr, value, &cl_list);
  }

  Status OQLBEIteratorOQL::getResult(Value *_value)
  {
    if (status) return status;

    if (!value)
      value = new Value();

    *_value = *value;
    return Success;
  }

  OQLBEIteratorOQL::~OQLBEIteratorOQL()
  {
    delete value;
  }
}
