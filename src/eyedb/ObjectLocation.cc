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

#include <eyedbconfig.h>

#include "eyedb/eyedb.h"
#include <assert.h>
#include "eyedbsm/eyedbsm.h"

using namespace std;

namespace eyedbsm {
  extern int power2(int);
}

namespace eyedb {

  ObjectLocationArray::ObjectLocationArray(ObjectLocation *_locs,
					   unsigned int _cnt)
  {
    locs = 0;
    set(_locs, _cnt);
  }

  void
  ObjectLocationArray::set(ObjectLocation *_locs, unsigned int _cnt)
  {
    garbage();
    locs = _locs;
    cnt = _cnt;
  }

  ObjectLocationArray::~ObjectLocationArray()
  {
    garbage();
  }

  void
  ObjectLocationArray::garbage()
  {
    delete [] locs;
  }

  PageStats *
  ObjectLocationArray::computePageStats(Database *_db)
  {
    if (_db)
      db = _db;

    if (!db) return 0;

    PageStats *pgs = new PageStats(db);
    for (int i = 0; i < cnt; i++)
      pgs->add(locs[i]);
    return pgs;
  }

  ostream &operator<<(ostream &os, const ObjectLocationArray &locarr)
  {
    os << "Object number: " << locarr.cnt << endl;
    for (int i = 0; i < locarr.cnt; i++) {
      ObjectLocation &loc = const_cast<ObjectLocation &>(locarr.getLocation(i));
      if (i) os << '\n';
      os << loc(locarr.db);
    }

    return os;
  }

  ostream &operator<<(ostream &os, const ObjectLocation &loc)
  {
    os << "Object: " << loc.oid << '\n';
    if (loc.db) {
      os << "  Dspid: #" << loc.dspid << " ";
      const Dataspace *dataspace = 0;
      if (!loc.db->getDataspace(loc.dspid, dataspace))
	os  << dataspace->getName();
      os << '\n';
      os << "  Datid: #" << loc.datid << " ";
      const Datafile *datafile = 0;
      if (!loc.db->getDatafile(loc.dspid, datafile)) {
	if (*datafile->getName())
	  os << datafile->getName();
	else
	  os << datafile->getFile();
      }
      os << '\n';
    }
    else {
      os << "  Dspid: #" << loc.dspid << '\n';
      os << "  Datid: #" << loc.datid << '\n';
    }

    os << "  Size: " << loc.info.size << '\n';
    os << "  SlotStartNum: " << loc.info.slot_start_num << '\n';
    os << "  SlotEndNum: " << loc.info.slot_end_num << '\n';
    os << "  DatStartPagenum: " << loc.info.dat_start_pagenum << '\n';
    os << "  DatEndPagenum: " << loc.info.dat_end_pagenum << '\n';
    if (loc.info.omp_start_pagenum != ~0) {
      os << "  OmpStartPagenum: " << loc.info.omp_start_pagenum << '\n';
      os << "  OmpEndPagenum: " << loc.info.omp_end_pagenum << '\n';
    }
    os << "  DmpStartPagenum: " << loc.info.dmp_start_pagenum << '\n';
    os << "  DmpEndPagenum: " << loc.info.dmp_end_pagenum << '\n';

    return os;
  }


  //
  // PageStats
  //

  PageStats::PageStats(Database *db)
  {
    Status s = db->getDatafiles(datafiles, datafile_cnt);
    if (s) {
      cerr << "Exception catcher in PageStats::PageStats: " << s;
      throw *s;
    }

    pgs = new PGS[datafile_cnt];
    for (int i = 0; i < datafile_cnt; i++)
      pgs[i].init(datafiles[i]);
  }

  void
  PageStats::add(const ObjectLocation &loc)
  {
    pgs[loc.getDatid()].add(loc);
  }

  PageStats::~PageStats()
  {
    delete [] pgs;
  }

  ostream &operator<<(ostream &os, const PageStats::PGS &pgs);

  ostream &operator<<(ostream &os, const PageStats &pgs)
  {
    os << ">>>> Page Statistics <<<<\n";
    for (int i = 0; i < pgs.datafile_cnt; i++)
      if (pgs.pgs[i].oid_cnt)
	os << '\n' << pgs.pgs[i];
    return os;
  }

  //
  // PageStats::PGS
  //

  static int pgsize_pow2 = eyedbsm::power2(getpagesize());

  inline unsigned int
  size2slot(unsigned int sz, unsigned int pow2)
  {
    return ((sz-1) >> pow2) + 1;
  }

  inline unsigned int
  ideal(unsigned int sz)
  {
    return sz ? (((sz-1) >> pgsize_pow2) + 1) :  0;
  }

#define ONE_K 1024

  void
  PageStats::PGS::init(const Datafile *_datafile)
  {
    if (!_datafile->isValid()) {
      oid_cnt = 0;
      datafile = 0;
      return;
    }

    datafile = _datafile;
    slotsize = datafile->getSlotsize();
    slotpow2 = eyedbsm::power2(slotsize);
    unsigned long long maxsize =
      ((unsigned long long)datafile->getMaxsize()) * ONE_K;

    totalsize = 0;
    totalsize_align = 0;

    totaldatpages_max = (maxsize >> pgsize_pow2) + 1;
    totaldatpages = (char *)calloc(totaldatpages_max, 1);
    totaldatpages_cnt = 0;

    totalomppages_max = 0;
    totalomppages = 0;
    totalomppages_cnt = 0;

    totaldmppages_max = ((maxsize / slotsize * 8) >> pgsize_pow2) + 1;
    totaldmppages = (char *)calloc(totaldmppages_max, 1);
    totaldmppages_cnt = 0;

    oid_cnt = 0;
  }

  void
  PageStats::PGS::add(const ObjectLocation &loc)
  {
    const ObjectLocation::Info &info = loc.getInfo();
    unsigned int size = loc.getSize() + sizeof(eyedbsm::ObjectHeader);
    totalsize += size;
    totalsize_align += size2slot(size, slotpow2) * slotsize;

    assert (info.dat_start_pagenum < totaldatpages_max);
    assert (info.dat_end_pagenum < totaldatpages_max);

    for (int n = info.dat_start_pagenum; n <= info.dat_end_pagenum; n++)
      if (!totaldatpages[n]) {
	totaldatpages[n] = 1;
	totaldatpages_cnt++;
      }

    assert (info.dmp_start_pagenum < totaldmppages_max);
    assert (info.dmp_end_pagenum < totaldmppages_max);

    for (int n = info.dmp_start_pagenum; n <= info.dmp_end_pagenum; n++)
      if (!totaldmppages[n]) {
	totaldmppages[n] = 1;
	totaldmppages_cnt++;
      }

    if (info.omp_end_pagenum >= totalomppages_max) {
      totalomppages_max = info.omp_end_pagenum + 64;
      totalomppages = (char *)realloc(totalomppages, totalomppages_max);
    }

    if (info.omp_start_pagenum != ~0)
      for (int n = info.omp_start_pagenum; n <= info.omp_end_pagenum; n++)
	if (!totalomppages[n]) {
	  totalomppages[n] = 1;
	  totalomppages_cnt++;
	}

    oid_cnt++;
  }

  extern void
  display_datsize(ostream &os, unsigned long long _sz);

  /*
    void
    display_datsize(ostream &os, unsigned long long sz)
    {
    os << sz << "b";
    sz /= ONE_K;
    if (sz)
    {
    os << ", " << sz << "Kb";
    sz /= ONE_K;
    if (sz)
    {
    os << ", " << sz << "~Mb";
    sz /= ONE_K;
    if (sz)
    os << ", " << sz << "~Gb";
    }
    }

    os << '\n';
    }
  */

  ostream &operator<<(ostream &os, const PageStats::PGS &pgs)
  {
    os << "Datafile #" << pgs.datafile->getId();
    if (*pgs.datafile->getName())
      os << " " << pgs.datafile->getName();
    else
      os << " File: " << pgs.datafile->getFile();
    os << '\n';
    os << "  Oid Type: " <<
      (pgs.datafile->isPhysical() ? "Physical" : "Logical") << '\n';
    if (pgs.datafile->getDataspace())
      os << "  Dataspace: " << pgs.datafile->getDataspace()->getName() << '\n';
    os << "  Object Count: " << pgs.oid_cnt << '\n';
    os << "  Size: ";
    display_datsize(os, pgs.totalsize);
    os << "  .dat Page Count:\n";
    os << "      Effective: " << pgs.totaldatpages_cnt << '\n';
    os << "      Ideal:  " << ideal(pgs.totalsize) <<
      " (slot based: " << ideal(pgs.totalsize_align) << ")\n";
    if (pgs.totalomppages_cnt) {
      os << "  .omp Page Count:\n";
      os << "      Effective: " << pgs.totalomppages_cnt << '\n';
      os << "      Ideal: " << 
	//	(pgs.oid_cnt ? ((pgs.oid_cnt * OIDLOCSIZE) >> pgsize_pow2) + 1 : 0) <<
	"Unknown (see issue #1)" <<
	'\n';
    }

    os << "  .dmp Page Count:\n";
    os << "      Effective: " << pgs.totaldmppages_cnt << '\n';
    os << "      Ideal: " << 
      (pgs.totalsize_align ?
       ((((pgs.totalsize_align-1)/(pgs.slotsize*8))>>pgsize_pow2)+1) : 0) <<
      '\n';
    return os;
  }

}


