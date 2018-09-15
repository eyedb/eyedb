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


#ifndef _EYEDB_TRANSACTION_PARAMS_H
#define _EYEDB_TRANSACTION_PARAMS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class TransactionParams {

  public:
    /**
       Not yet documented
       @param params
       @return
    */

    TransactionParams();

    /**
       Not yet documented
       @param params
       @return
    */

    TransactionParams(TransactionMode trsmode, TransactionLockMode lockmode,
		      RecoveryMode recovmode, unsigned int magorder,
		      unsigned int ratiolart, unsigned int wait_timeout);

    /**
       Not yet documented
       @param params
       @return
    */
    static Status setGlobalDefaultTransactionParams(const TransactionParams &params);

    /**
       Not yet documented
       @return
    */
    static TransactionParams getGlobalDefaultTransactionParams();

    /**
       Not yet documented
       @param magorder
    */
    static void setGlobalDefaultMagOrder(unsigned int magorder);

    /**
       Not yet documented
       @return
    */
    static unsigned int getGlobalDefaultMagOrder();

    /**
       Not yet documented
       @param params
       @return
    */
    bool operator==(const TransactionParams &params) const;

  public:
    TransactionMode trsmode;
    TransactionLockMode lockmode;
    RecoveryMode recovmode;
    unsigned int magorder;     /* estimated object cardinality in trans */
    unsigned int ratioalrt;    /* error returned if ratioalrt != 0 &&
				  trans object number > ratioalrt * magorder  */
    unsigned int wait_timeout; /* in seconds */

  private:
    static TransactionParams global_def_params;
  };

  /**
     @}
  */
}

#endif
