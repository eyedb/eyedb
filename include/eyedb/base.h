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


#ifndef _EYEDB_BASE_H
#define _EYEDB_BASE_H

#include <eyedblib/machtypes.h>
#include <eyedbsm/eyedbsm.h>

class rpcDB_LocalDBContext;

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class DbHandle;
  class ConnHandle;

  typedef unsigned char *Data;
  typedef eyedblib::uint32 Size;
  typedef eyedblib::uint32 Type;
  typedef eyedblib::uint32 Offset;
  typedef void * Any;
  typedef eyedbsm::DatType DatType;

  enum Bool {
    False = 0,
    True = 1
  };

  enum {
    _DBRead       = 0x2,
    _DBRW         = 0x4,
    _DBSRead      = 0x8,

    /* modifier */
    _DBAdmin      = 0x10,
    _DBOpenLocal  = 0x20
  };

  struct DbCreateDescription {
    char dbfile[256];
    eyedbsm::DbCreateDescription sedbdesc;
  };

  typedef DbCreateDescription DbInfoDescription;

  enum LockMode {
    DefaultLock = eyedbsm::DefaultLock,
    LockN = eyedbsm::LockN,   /* no lock */
    LockX = eyedbsm::LockX,   /* exclusive */
    LockSX = eyedbsm::LockSX, /* shared/exclusive */
    LockS = eyedbsm::LockS,   /* shared */
    LockP = eyedbsm::LockP    /* private */
  };

  enum TransactionMode {
    TransactionOff = eyedbsm::TransactionOff,  /* no rollback is possible */
    TransactionOn  = eyedbsm::TransactionOn
  };

  enum MapHints {
    DefaultMap = 0,
    WholeMap = eyedbsm::WholeMap,
    SegmentMap = eyedbsm::SegmentMap
  };

  struct OpenHints {
    MapHints maph;
    unsigned int mapwide; /* in pages */
  };

  /* lockmode: S means shared, X means exclusive, SX means shared/exclusive,
     N means no */
  enum TransactionLockMode {
    ReadSWriteS = eyedbsm::ReadSWriteS, /* read shared; write shared */
    ReadSWriteSX = eyedbsm::ReadSWriteSX, /* read shared; write shared/exclusive */  
    ReadSWriteX = eyedbsm::ReadSWriteX,  /* read shared; write exclusive */  
    ReadSXWriteSX = eyedbsm::ReadSXWriteSX, /* read shared/exclusive; write shared/exclusive */
    ReadSXWriteX = eyedbsm::ReadSXWriteX, /* read shared/exclusive; write exclusive */
    ReadXWriteX = eyedbsm::ReadXWriteX,  /* read exclusive; write exclusive */
    ReadNWriteS = eyedbsm::ReadNWriteS,  /* read no lock; write shared */
    ReadNWriteSX = eyedbsm::ReadNWriteSX, /* read no lock; write shared/exclusive */
    ReadNWriteX = eyedbsm::ReadNWriteX, /* read no lock; write exclusive */
    ReadNWriteN = eyedbsm::ReadNWriteN, /* read no lock; write no lock */
    DatabaseW = eyedbsm::DatabaseW, /* database exclusive for writing */
    DatabaseRW = eyedbsm::DatabaseRW, /* database exclusive for reading and writing */
    DatabaseWtrans = eyedbsm::DatabaseWtrans /* database exclusive for writing transaction */
  };

  enum RecoveryMode {
    RecoveryOff = eyedbsm::RecoveryOff, /* prevents from remote client failure */
    RecoveryPartial = eyedbsm::RecoveryPartial, /* plus prevents from local client and server failure */
    RecoveryFull = eyedbsm::RecoveryFull     /* plus prevents from OS failure */
  };

  extern const char NullString[];
  extern const char NilString[];

#define IDBBOOL(X) ((X) ? True : False)

  /**
     @}
  */

}

#endif
