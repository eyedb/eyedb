/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-1999,2004-2006 SYSRA
   
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

#include <eyedb/eyedb.h>
#include "DatafileStats.h"

using namespace eyedb;
using namespace std;

namespace eyedb {
  extern void display_datsize(std::ostream &, unsigned long long);
}

DatafileStats::DatafileStats(DbInfoDescription *_dbdesc)
{
  objcnt = slotcnt = busyslotcnt = 0;
  maxsize = totalsize = busyslotsize = datafilesize = datafileblksize =
    dmpfilesize = dmpfileblksize = defragmentablesize = 0;
  slotfragcnt = 0;
  used = 0.;
  cnt = 0;
  dbdesc = _dbdesc;
}

int DatafileStats::add(const Datafile *datafile)
{
  if (!datafile->isValid()) 
    return 0;

  DatafileInfo info;

  datafile->getInfo(info);

  const DatafileInfo::Info &in = info.getInfo();
  maxsize += datafile->getMaxsize();
  objcnt += in.objcnt;
  slotcnt += in.slotcnt;
  totalsize += in.totalsize;
  busyslotcnt += in.busyslotcnt;
  busyslotsize += in.busyslotsize;

  datafilesize += in.datfilesize;
  datafileblksize += in.datfileblksize;
  dmpfilesize += in.dmpfilesize;
  dmpfileblksize += in.dmpfileblksize;

  defragmentablesize += in.defragmentablesize;
  slotfragcnt += in.slotfragcnt;
  used += in.used;
  cnt++;

  return 0;
}

void DatafileStats::display() 
{
  if (dbdesc) {
    cout << "Statistics\n";
    cout << "  Maximum Object Number " << dbdesc->sedbdesc.nbobjs << '\n';
  }
  else
    cout << "Datafile Statistics\n";

  cout << "  Object Number         " << objcnt << '\n';
  cout << "  Maximum Slot Count    " << slotcnt << '\n';
  cout << "  Busy Slot Count       " << busyslotcnt << '\n';
  cout << "  Maximum Size          ";
  display_datsize(cout, maxsize*1024);

  if (!dbdesc) {
    cout << "  Busy Size             ";
    display_datsize(cout, totalsize);
  }

  cout << "  Busy Slot Size        ";
  display_datsize(cout, busyslotsize);

  if (dbdesc) {
    cout << "  Disk Size Used        ";
    display_datsize(cout,
		    datafilesize +
		    dmpfilesize + 
		    dbdesc->sedbdesc.dbsfilesize + 
		    dbdesc->sedbdesc.ompfilesize +
		    dbdesc->sedbdesc.shmfilesize);
    cout << "  Disk Block Size Used  ";
    display_datsize(cout,
		    datafileblksize +
		    dmpfileblksize + 
		    dbdesc->sedbdesc.dbsfileblksize + 
		    dbdesc->sedbdesc.ompfileblksize +
		    dbdesc->sedbdesc.shmfileblksize);
  }
  else {
    cout << "  .dat File Size        ";
    display_datsize(cout, datafilesize);
    cout << "  .dat File Block Size  ";
    display_datsize(cout, datafileblksize);
      
    cout << "  .dmp File Size        ";
    display_datsize(cout, dmpfilesize);
    cout << "  .dmp File Block Size  ";
    display_datsize(cout, dmpfileblksize);
    cout << "  Disk Size Used        ";
    display_datsize(cout, datafilesize+dmpfilesize);
    cout << "  Disk Block Size Used  ";
    display_datsize(cout, datafileblksize+dmpfileblksize);
    cout << "  Defragmentable Size   ";
    display_datsize(cout, defragmentablesize);
  }

  char buf[16];
  sprintf(buf, "%2.2f", used/cnt);
  cout << "  Used                  " << buf << "%\n";
}
