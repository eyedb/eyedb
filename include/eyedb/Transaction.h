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


#ifndef _EYEDB_TRANSACTION_H
#define _EYEDB_TRANSACTION_H

#include <eyedb/ObjCache.h>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Transaction {

    // ----------------------------------------------------------------------
    // Transaction Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @return
    */
    unsigned int getTID() const             {return tid;}

    /**
       Not yet documented
       @return
    */
    const TransactionParams& getParams() const {return params;}

    /**
       Not yet documented
       @return
    */
    Database *getDatabase()     {return db;}


    /**
       Not yet documented
       @param params
       @return
    */
    Status setParams(const TransactionParams &params);

    /**
       Not yet documented
       @param params
       @param strict
       @return
    */
    static Status checkParams(const TransactionParams &params,
			      Bool strict = True);

    // cache management
    /**
       Not yet documented
       @return
    */
    Status cacheInvalidate();

    /**
       Not yet documented
       @param on_off
       @return
    */
    Status setCacheActive(Bool on_off);

    /**
       Not yet documented
       @param oid
       @param o
    */
    void cacheObject(const Oid &oid, Object *o);

    /**
       Not yet documented
       @param o
    */
    void uncacheObject(Object *o);

    /**
       Not yet documented
       @param _oid
    */
    void uncacheObject(const Oid &_oid);

    // ----------------------------------------------------------------------
    // Transaction Private Part
    // ----------------------------------------------------------------------
  private:
    friend class Database;
    Transaction(Database *, const TransactionParams &);

    Status begin();
    Status commit();
    Status abort();
    void cacheInit();

    ~Transaction();

    Database *db;
    unsigned int tid;
    TransactionParams params;
    ObjCache *cache;
    Bool cache_on;
    Bool incoherency;

    void setIncoherency();
    Object *getObject(const Oid &oid) {
      return (cache_on ? (Object *)cache->getObject(oid) : NULL);
    }

    ObjectList *getObjects() {
      return (cache_on ? cache->getObjects() : NULL);
    }
  };

  /**
     @}
  */

}

#endif
