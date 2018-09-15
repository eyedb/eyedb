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


#ifndef _EYEDB_DATAFILE_H
#define _EYEDB_DATAFILE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Database;
  class Dataspace;
  class DatafileInfo;

  /**
     Not yet documented.
  */
  class Datafile {

  public:
    /**
       Not yet documented
       @return
    */
    Database *getDatabase() {return db;}

    /**
       Not yet documented
       @return
    */
    const Database *getDatabase() const {return db;}

    /**
       Not yet documented
       @return
    */
    const char *getName() const {return name;}

    /**
       Not yet documented
       @return
    */
    unsigned short getId() const {return id;}

    /**
       Not yet documented
       @return
    */
    unsigned short getDspid() const {return dspid;}

    /**
       Not yet documented
       @return
    */
    const Dataspace *getDataspace() const {return dataspace;}

    /**
       Not yet documented
       @return
    */
    const char *getFile() const {return file;}

    /**
       Not yet documented
       @return
    */
    Bool isValid() const {return *file ? True : False;}

    /**
       Not yet documented
       @return
    */
    Bool isPhysical() const {
      return dtype == eyedbsm::PhysicalOidType ? True : False;
    }

    /**
       Not yet documented
       @return
    */
    unsigned int getMaxsize() const {return maxsize;}

    /**
       Not yet documented
       @return
    */
    eyedbsm::MapType getMaptype() const {return mtype;}

    /**
       Not yet documented
       @return
    */
    unsigned int getSlotsize() const {return slotsize;}

    /**
       Not yet documented
       @return
    */
    Status remove() const;

    /**
       Not yet documented
       @param size
       @return
    */
    Status resize(unsigned int size) const;

    /**
       Not yet documented
       @param info
       @return
    */
    Status getInfo(DatafileInfo &info) const;

    /**
       Not yet documented
       @param newname
       @return
    */
    Status rename(const char *newname) const;

    /**
       Not yet documented
       @return
    */
    Status defragment() const;

    /**
       Not yet documented
       @param filedir
       @param filename
       @return
    */
    Status move(const char *filedir, const char *filename) const;

  private:
    Datafile(Database *_db, unsigned short _id, unsigned short _dspid,
	     const char *_file, const char *_name, int _maxsize,
	     eyedbsm::MapType _mtype, int _slotsize, DatType _dtype) :
      db(_db), id(_id), dspid(_dspid), file(strdup(_file)), name(strdup(_name)),
      mtype(_mtype), maxsize(_maxsize), slotsize(_slotsize), dtype(_dtype) 
      {
	dataspace = 0;
      }

    void setDataspace(const Dataspace *_dataspace) {dataspace = _dataspace;}

    Database *db;
    unsigned short id, dspid;
    const Dataspace *dataspace;
    char *file, *name;
    eyedbsm::MapType mtype;
    unsigned int maxsize, slotsize;
    DatType dtype;

    ~Datafile() {
      free(file);
      free(name);
    }

    friend class Database;
  };

  std::ostream& operator<<(std::ostream&, const Datafile &);

  class DatafileInfo {

  public:
    struct Info {
      unsigned int objcnt;
      unsigned int slotcnt;
      unsigned int busyslotcnt;
      unsigned long long totalsize;
      unsigned int avgsize;
      unsigned int lastbusyslot;
      unsigned int lastslot;
      unsigned long long busyslotsize;
      unsigned long long datfilesize;
      unsigned long long datfileblksize;
      unsigned long long dmpfilesize;
      unsigned long long dmpfileblksize;
      unsigned int curslot;
      unsigned long long defragmentablesize;
      unsigned int slotfragcnt;
      double used;
    };

    DatafileInfo() {datafile = 0;}
    const Datafile *getDatafile() const {return datafile;}
    const DatafileInfo::Info & getInfo() const {return info;}

  private:
    void set(const Datafile *, DatafileInfo::Info &);
    const Datafile *datafile;
    Info info;
    friend class Datafile;
  };

  std::ostream& operator<<(std::ostream&, const DatafileInfo &);

  /**
     @}
  */

}

#endif
