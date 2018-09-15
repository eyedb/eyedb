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


#ifndef _EYEDB_OBJECT_LOCATION_H
#define _EYEDB_OBJECT_LOCATION_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Database;
  class Datafile;

  /**
     Not yet documented.
  */
  class ObjectLocation {

  public:
    struct Info {
      unsigned int size;
      unsigned int slot_start_num;
      unsigned int slot_end_num;
      unsigned int dat_start_pagenum;
      unsigned int dat_end_pagenum;
      unsigned int omp_start_pagenum;
      unsigned int omp_end_pagenum;
      unsigned int dmp_start_pagenum;
      unsigned int dmp_end_pagenum;
    };

    /**
       Not yet documented
       @return
    */
    const Oid &getOid() const {return oid;}

    /**
       Not yet documented
       @return
    */
    short getDspid() const {return dspid;}

    /**
       Not yet documented
       @return
    */
    short getDatid() const {return datid;}

    /**
       Not yet documented
       @return
    */
    unsigned int getSize() const {return info.size;}

    /**
       Not yet documented
       @return
    */
    const Info & getInfo() const {return info;}


    /**
       Not yet documented
       @return
    */
    unsigned int getSlotStartNum() const {return info.slot_start_num;}

    /**
       Not yet documented
       @return
    */
    unsigned int getSlotEndNum() const {return info.slot_end_num;}

    /**
       Not yet documented
       @return
    */
    unsigned int getDatStartPagenum() const {return info.dat_start_pagenum;}

    /**
       Not yet documented
       @return
    */
    unsigned int getDatEndPagenum() const {return info.dat_end_pagenum;}

    /**
       Not yet documented
       @return
    */
    unsigned int getOmpStartPagenum() const {return info.omp_start_pagenum;}

    /**
       Not yet documented
       @return
    */
    unsigned int getOmpEndPagenum() const {return info.omp_end_pagenum;}

    /**
       Not yet documented
       @return
    */
    unsigned int getDmpStartPagenum() const {return info.dmp_start_pagenum;}

    /**
       Not yet documented
       @return
    */
    unsigned int getDmpEndPagenum() const {return info.dmp_end_pagenum;}


    /**
       Not yet documented
       @param _db
    */
    ObjectLocation(Database *_db = 0) { db = _db; }

    /**
       Not yet documented
       @param _oid
       @param _dspid
       @param _datid
       @param _info
    */
    ObjectLocation(const Oid &_oid, short _dspid, short _datid,
		   const Info &_info) {
      set(_oid, _dspid, _datid, _info);
    }

    /**
       Not yet documented
       @param _db
       @return
    */
    ObjectLocation& operator()(Database *_db) {db = _db; return *this;}

    /**
       Not yet documented
       @param _oid
       @param _dspid
       @param _datid
       @param _info
    */
    void set(const Oid &_oid, short _dspid, short _datid, const Info &_info) {
      oid = _oid;
      dspid = _dspid;
      datid = _datid;
      info = _info;
    }

  private:
    Database *db;
    Oid oid;
    unsigned short dspid;
    unsigned short datid;
    Info info;
    friend std::ostream &operator<<(std::ostream &, const ObjectLocation &);
  };

  std::ostream &operator<<(std::ostream &, const ObjectLocation &);

  class PageStats;

  class ObjectLocationArray {

  public:
    ObjectLocationArray(Database *_db = 0) {db = _db; locs = 0; cnt = 0;}
    ObjectLocationArray(ObjectLocation *, unsigned int);

    void set(ObjectLocation *, unsigned int);
    unsigned int getCount() const {return cnt;}
    const ObjectLocation & getLocation(int idx) const {
      return locs[idx];
    }

    ObjectLocationArray& operator()(Database *_db) {db = _db; return *this;}
    PageStats *computePageStats(Database *db = 0);
    ~ObjectLocationArray();

  private:
    void garbage();
    unsigned int cnt;
    ObjectLocation *locs;
    Database *db;
    friend std::ostream &operator<<(std::ostream &, const ObjectLocationArray &);
  };

  class PageStats
  {
  public:
    PageStats(Database *);
    void add(const ObjectLocation &loc);
    ~PageStats();

    struct PGS {
      const Datafile *datafile;
      unsigned int slotsize;
      unsigned int slotpow2;

      unsigned long long totalsize;
      unsigned long long totalsize_align;

      unsigned int totaldatpages_cnt;
      unsigned int totaldatpages_max;
      char *totaldatpages;

      unsigned int totaldmppages_max;
      unsigned int totaldmppages_cnt;
      char *totaldmppages;

      unsigned int totalomppages_max;
      unsigned int totalomppages_cnt;
      char *totalomppages;

      unsigned int oid_cnt;

      void init(const Datafile *);
      void add(const ObjectLocation &loc);
    };

    const PageStats::PGS *getPGS() const {return pgs;}
  private:
    PGS *pgs;
    const Datafile **datafiles;
    unsigned int datafile_cnt;
    friend std::ostream &operator<<(std::ostream &os, const PageStats &pgs);
  };

  std::ostream &operator<<(std::ostream &, const ObjectLocationArray &);
  std::ostream &operator<<(std::ostream &os, const PageStats &pgs);

  /**
     @}
  */

}

#endif
