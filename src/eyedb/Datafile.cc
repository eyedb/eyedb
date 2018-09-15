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


#include <assert.h>
#include <eyedb_p.h>

using namespace std;

namespace eyedb {

  Status
  Datafile::remove() const
  {
    RPCStatus rpc_status = deleteDatafile(db->getDbHandle(), id);
    return StatusMake(rpc_status);
  }

  Status
  Datafile::resize(unsigned int size) const
  {
    RPCStatus rpc_status =
      resizeDatafile(db->getDbHandle(), id, size);
    return StatusMake(rpc_status);
  }

  Status
  Datafile::getInfo(DatafileInfo &datinfo) const
  {
    DatafileInfo::Info info;
    RPCStatus rpc_status =
      getDatafileInfo(db->getDbHandle(), id, &info);
    if (!rpc_status) datinfo.set(this, info);
    return StatusMake(rpc_status);
  }

  Status
  Datafile::rename(const char *newname) const
  {
    RPCStatus rpc_status =
      renameDatafile(db->getDbHandle(), id, newname);
    return StatusMake(rpc_status);
  }

  Status
  Datafile::defragment() const
  {
    RPCStatus rpc_status = defragmentDatafile(db->getDbHandle(), id);
    return StatusMake(rpc_status);
  }

  Status
  Datafile::move(const char *filedir, const char *filename) const
  {
    std::string s;
    if (filedir)
      s = std::string(filedir) + "/" + filename;
    else
      s = filename;
    RPCStatus rpc_status = moveDatafile(db->getDbHandle(), id, s.c_str());
    return StatusMake(rpc_status);
  }

#define ONE_K 1024
#define ONE_M (ONE_K*ONE_K)
#define ONE_G (ONE_M*ONE_K)

  void
  display_datsize(std::ostream &os, unsigned long long _sz)
  {
    unsigned long long sz1, sz2;
    unsigned long long sz;

    sz = _sz;
    os << sz << "b";

    sz = _sz / ONE_K;
    if (sz)
      {
	os << ", ~" << sz << "Kb";

	sz = _sz / ONE_M;
	if (sz)
	  {
	    sz1 = sz * ONE_M;
	    sz2 = (sz+1) * ONE_M;
	    if ((sz2 - _sz) < (_sz - sz1))
	      sz = sz+1;
	    os << ", ~" << sz << "MB";

	    sz = _sz / ONE_G;
	    if (sz) {
	      sz1 = sz * ONE_G;
	      sz2 = (sz+1) * ONE_G;
	      if ((sz2 - _sz) < (_sz - sz1))
		sz = sz+1;
	      os << ", ~" << sz << "GB";
	      /*
		printf("\n{_sz %llu, sz %llu, sz1 %llu, sz2 %llu, "
		"(sz2 - _sz) %llu, (sz1 - _sz) %llu}\n",
		_sz, sz, sz1, sz2, (sz2 - _sz), (_sz - sz1));
	      */
	    }
	  }
      }

    os << '\n';
  }

  std::ostream& operator<<(std::ostream &os, const Datafile &dat)
  {
    os << "Datafile #" << dat.getId() << '\n';

    if (!dat.isValid()) {
      os << "  Invalid datafile\n";
      return os;
    }

    os << "  Name      " << (*dat.getName() ? dat.getName() : "<unnamed>") <<
      '\n';
    if (dat.getDataspace())
      os << "  Dataspace #" << dat.getDataspace()->getId() << " " <<
	dat.getDataspace()->getName() << '\n';
    os << "  File      " << dat.getFile() << '\n';
    os << "  Maxsize   ";
    display_datsize(os, (eyedblib::uint64)dat.getMaxsize()*1024);

    if (dat.getMaptype() == eyedbsm::BitmapType)
      os << "  Slotsize  " << dat.getSlotsize() << '\n';
    else
      os << "  Linkmap allocator\n";

    os << "  Oid Type  " << (dat.isPhysical() ? "Physical" : "Logical") << '\n';
    return os;
  }

  void
  DatafileInfo::set(const Datafile *_datafile,
		    DatafileInfo::Info &_info)
  {
    datafile = _datafile;
    info = _info;
  }

  std::ostream& operator<<(std::ostream& os, const DatafileInfo &datinfo)
  {
    if (!datinfo.getDatafile()) {
      os << "Null Datafile";
      return os;
    }
    const DatafileInfo::Info &info = datinfo.getInfo();

    os << *datinfo.getDatafile();
    os << '\n';
    os << "  Object Number        " << info.objcnt << '\n';
    os << "  Total Busy Size      ";
    display_datsize(os, info.totalsize);
    os << "  Average Size         ";
    display_datsize(os, info.avgsize);
    os << '\n';
    os << "  Slot Count           " << info.slotcnt << '\n';
    os << "  Busy Slot Count      " << info.busyslotcnt << '\n';
    os << "  Last Busy Slot       " << info.lastbusyslot << '\n';
    os << "  Last Slot            " << info.lastslot << '\n';
    os << "  Busy Slot Size       ";
    display_datsize(os, info.busyslotsize);

    os << "  .dat File Size       ";
    display_datsize(os, info.datfilesize);
    os << "  .dat File Block Size ";
    display_datsize(os, info.datfileblksize);

    os << "  .dmp File Size       ";
    display_datsize(os, info.dmpfilesize);
    os << "  .dmp File Block Size ";
    display_datsize(os, info.dmpfileblksize);

    os << "  Current Slot         " << info.curslot << '\n';
    os << "  Defragmentable Size  ";
    display_datsize(os, info.defragmentablesize);

    char buf[16];
    sprintf(buf, "%2.2f",
	    (info.lastbusyslot ? (double)(100.*info.slotfragcnt)/info.lastbusyslot : 0));

    os << "  Slot Fragmentation   " << info.slotfragcnt << "/" <<
      info.lastbusyslot << " slots [" << buf << "%]\n";

    sprintf(buf, "%2.2f", info.used);
    os << "  Used                 " << buf << "%\n";

    return os;
  }

}
  
