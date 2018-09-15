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


#ifndef _EYEDB_OQLBE_H
#define _EYEDB_OQLBE_H

namespace eyedb {
  class OQLBEIterator;
  class SchemaInfo;

  class OQLBE {
    Status status;
    int qid;
    OQLBEIterator *qiter;
    SchemaInfo *schema_info;

  public:
    OQLBE(Database *, DbHandle *, const char *);

    Status getStatus() const;
    Status getResult(Value *);
    int getQid() const {return qid;}

    SchemaInfo *getSchemaInfo();

    static LinkedList *getMclList() {return def_cl_list;}
    static void setMclList(LinkedList *cl_list) {def_cl_list = cl_list;}

    ~OQLBE();

  private:
    static LinkedList *def_cl_list;
  };

}

#endif
