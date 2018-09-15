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


#ifndef _EYEDB_DATASPACE_H
#define _EYEDB_DATASPACE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class Database;

  class Dataspace {

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
    bool isValid() const;

    /**
       Not yet documented
       @return
    */
    unsigned short getId() const {return id;}

    /**
       Not yet documented
       @param 
       @return
    */
    const Datafile **getDatafiles(unsigned int &_datafile_cnt) const {
      _datafile_cnt = datafile_cnt;
      return datafiles;
    }

    /**
       Not yet documented
       @param datafile
       @return
    */
    Status getCurrentDatafile(const Datafile *&datafile) const;

    /**
       Not yet documented
       @param datafile
       @return
    */
    Status setCurrentDatafile(const Datafile *datafile);

    /**
       Not yet documented
       @return
    */
    Status remove() const;

    /**
       Not yet documented
       @param newname
       @return
    */
    Status rename(const char *newname) const;

    /**
       Not yet documented
       @param datafiles
       @param datafile_cnt
       @return
    */
    Status update(const Datafile **datafiles, unsigned int datafile_cnt, short flags, short orphan_dspid) const;

    static const short DefaultDspid;

  private:
    Dataspace(Database *_db, unsigned short _id, const char *_name,
	      const Datafile **_datafiles, unsigned int _datafile_cnt) :
      db(_db), id(_id), name(strdup(_name)), datafiles(_datafiles),
      datafile_cnt(_datafile_cnt), cur_datafile(0) { }

    static char **makeDatid(const Datafile **, unsigned int);
    static void freeDatid(char **, unsigned int);

    Database *db;
    unsigned short id;
    char *name;
    const Datafile **datafiles;
    unsigned int datafile_cnt;
    const Datafile *cur_datafile;

    ~Dataspace();

    friend class Database;
  };

  std::ostream& operator<<(std::ostream&, const Dataspace &);

  /**
     @}
  */

}

#endif
