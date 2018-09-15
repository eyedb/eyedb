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

namespace eyedb {

  void
  decode_sch_info(Data data, void *xschinfo)
  {
    Offset offset = 0;
    eyedblib::int32 cnt;
    int32_decode(data, &offset, &cnt);
    SchemaInfo *schinfo = new SchemaInfo(cnt);

    for (int i = 0; i < cnt; i++)
      {
	eyedbsm::Oid *xoid = schinfo->class_oid[i].getOid();
	oid_decode(data, &offset, xoid);
      }

    *((SchemaInfo **)xschinfo) = schinfo;
  }

  void
  code_sch_info(void *xschinfo, Data *data, int *size)
  {
    Offset offset = 0;
    Size alloc_size = 0;
    SchemaInfo *schinfo = (SchemaInfo *)xschinfo;

    int cnt = schinfo->class_cnt;
    int32_code(data, &offset, &alloc_size, &cnt);

    for (int i = 0; i < cnt; i++)
      oid_code(data, &offset, &alloc_size, schinfo->class_oid[i].getOid());

    *size = offset;
  }

  SchemaInfo::SchemaInfo(int cnt)
  {
    class_cnt = cnt;
    //  class_oid = (class_cnt ? new Oid[class_cnt] : NULL);
    class_oid = (class_cnt ? (Oid *)calloc(sizeof(Oid) * class_cnt, 1)
		 : NULL);
  }

  SchemaInfo::SchemaInfo(LinkedList *cl_list)
  {
    class_cnt = cl_list->getCount();
    class_oid = (class_cnt ? (Oid *)calloc(sizeof(Oid) * class_cnt, 1)
		 : NULL);

    if (class_cnt)
      {
	int n = 0;
	LinkedListCursor c(cl_list);
	Class *cls;

	while (cl_list->getNextObject(&c, (void *&)cls))
	  class_oid[n++] = cls->getOid();
      }	     
  }

  SchemaInfo::~SchemaInfo()
  {
    free(class_oid);

    class_oid = NULL;
    class_cnt = 0;
  }

}

  
