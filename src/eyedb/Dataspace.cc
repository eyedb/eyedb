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

namespace eyedb {

  const short Dataspace::DefaultDspid = eyedbsm::DefaultDspid;

  bool Dataspace::isValid() const
  {
    return *getName();
  }

  Status
  Dataspace::remove() const
  {
    RPCStatus rpc_status = deleteDataspace(db->getDbHandle(), id);
    return StatusMake(rpc_status);
  }

  Status
  Dataspace::rename(const char *newname) const
  {
    RPCStatus rpc_status =
      renameDataspace(db->getDbHandle(), id, newname);
    return StatusMake(rpc_status);
  }

  Status
  Dataspace::update(const Datafile **datafiles, unsigned int datafile_cnt, short flags, short orphan_dspid) const
  {
    char **datids = makeDatid(datafiles, datafile_cnt);
    RPCStatus rpc_status = updateDataspace(db->getDbHandle(), id, datids, datafile_cnt, flags, orphan_dspid);
    freeDatid(datids, datafile_cnt);
    return StatusMake(rpc_status);
  }

  Status
  Dataspace::setCurrentDatafile(const Datafile *datafile)
  {
    RPCStatus rpc_status = dataspaceSetCurrentDatafile(db->getDbHandle(), id, datafile->getId());
    if (!rpc_status)
      cur_datafile = datafile;
    return StatusMake(rpc_status);
  }

  Status
  Dataspace::getCurrentDatafile(const Datafile *&datafile) const
  {
    if (!cur_datafile) {
      int datid;
      RPCStatus rpc_status = dataspaceGetCurrentDatafile
	(db->getDbHandle(), id, &datid);

      if (rpc_status)
	return StatusMake(rpc_status);

      for (int i = 0; i < datafile_cnt; i++)
	if (datafiles[i]->getId() == datid) {
	  const_cast<Dataspace *>(this)->cur_datafile = datafiles[i];
	  break;
	}
    }

    datafile = cur_datafile;
    return Success;
  }

  extern void display_datsize(std::ostream &, unsigned long long);

  std::ostream& operator<<(std::ostream& os, const Dataspace &dsp)
  {
    if (!*dsp.getName())
      return os;

    os << "Dataspace #" << dsp.getId() << '\n';
    os << "Name " << dsp.getName() << '\n';

    unsigned int cnt;
    const Datafile **datafiles = dsp.getDatafiles(cnt);
    for (int i = 0; i < cnt; i++) {
      const Datafile *dat = datafiles[i];
      os << "   Datafile #" << dat->getId() << '\n';
      if (*dat->getName())
	os << "     Name     " << dat->getName() << '\n';
      os << "     File     " << dat->getFile() << '\n';
      os << "     Maxsize  ";
      display_datsize(os, (eyedblib::uint64)dat->getMaxsize() * 1024);
      os << "     Slotsize " << dat->getSlotsize() << '\n';
      os << "     Oid Type " << (dat->isPhysical() ? "Physical" : "Logical") <<
	'\n';
    }
    return os;
  }

  char **
  Dataspace::makeDatid(const Datafile **datafiles,
		       unsigned int datafile_cnt)
  {
    char **datids = new char *[datafile_cnt];
    for (int i = 0; i < datafile_cnt; i++)
      datids[i] = strdup(str_convert((long)datafiles[i]->getId()).c_str());
    return datids;
  }

  void
  Dataspace::freeDatid(char **datids, unsigned int datafile_cnt)
  {
    for (int i = 0; i < datafile_cnt; i++)
      free(datids[i]);

    delete [] datids;
  }

  Dataspace::~Dataspace()
  {
    free(name);
    delete [] datafiles;
  }

}
