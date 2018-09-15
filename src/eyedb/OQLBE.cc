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
#include "OQLBEIterator.h"
#include "BEQueue.h"

namespace eyedb {

  LinkedList *OQLBE::def_cl_list;

  OQLBE::OQLBE(Database *db, DbHandle *dbh, const char *qstr)
  {
    qiter = new OQLBEIteratorOQL(db, dbh, qstr);

    status = qiter->getStatus();

    if (status == Success)
      qid = db->getBEQueue()->addOQL(this);

    schema_info = 0;
  }

  Status OQLBE::getStatus() const
  {
    return status;
  }

  SchemaInfo *OQLBE::getSchemaInfo()
  {
    if (!schema_info && qiter)
      schema_info = new SchemaInfo(((OQLBEIteratorOQL *)qiter)->
				   getSchemaInfo());
    return schema_info;
  }

  Status OQLBE::getResult(Value *value)
  {
    IDB_CHECK_INTR();

    if (qiter)
      return qiter->getResult(value);

    return Success;
  }

  OQLBE::~OQLBE()
  {
    delete qiter;
    delete schema_info;
  }
}
