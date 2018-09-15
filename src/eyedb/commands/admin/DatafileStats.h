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
   Author: Francois Dechelle <francois@dechelle.net>
*/

#ifndef _EYEDB_ADMIN_DATAFILESTATS_H_
#define _EYEDB_ADMIN_DATAFILESTATS_H_

#include <eyedb/eyedb.h>

namespace eyedb {
  class DatafileStats {
  private:
    unsigned int objcnt;
    unsigned int slotcnt;
    unsigned int busyslotcnt;
    unsigned long long maxsize;
    unsigned long long totalsize;
    unsigned long long busyslotsize;
    unsigned long long datafilesize;
    unsigned long long datafileblksize;
    unsigned long long dmpfilesize;
    unsigned long long dmpfileblksize;
    unsigned long long defragmentablesize;
    unsigned int slotfragcnt;
    double used;
    unsigned int cnt;
    DbInfoDescription *dbdesc;

  public:
    DatafileStats(DbInfoDescription *_dbdesc = 0);

    int add(const Datafile *datafile);

    void display();
  };
}

#endif
