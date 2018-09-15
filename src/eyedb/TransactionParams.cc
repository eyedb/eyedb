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


#include <eyedb/eyedb.h>

namespace eyedb {

  TransactionParams TransactionParams::global_def_params = 
  TransactionParams(TransactionOn, ReadNWriteSX, RecoveryFull,
		    0, 0, 30);

  TransactionParams::TransactionParams(TransactionMode trsmode,
				       TransactionLockMode lockmode,
				       RecoveryMode recovmode,
				       unsigned int magorder,
				       unsigned int ratioalrt,
				       unsigned int wait_timeout)
  {
    this->trsmode = trsmode;
    this->lockmode = lockmode;
    this->recovmode = recovmode;
    this->magorder = magorder;
    this->ratioalrt = ratioalrt;
    this->wait_timeout = wait_timeout;
  }

  bool TransactionParams::operator==(const TransactionParams &params) const
  {
    return params.trsmode == trsmode &&
      params.lockmode == lockmode &&
      params.recovmode == recovmode &&
      params.magorder == magorder &&
      params.ratioalrt == ratioalrt &&
      params.wait_timeout == wait_timeout;
  }

  TransactionParams::TransactionParams()
  {
    *this = global_def_params;
  }

  Status TransactionParams::setGlobalDefaultTransactionParams
  (const TransactionParams &params)
  {
    Status s;

    if (s = Transaction::checkParams(params))
      return s;

    global_def_params = params;
    return Success;
  }

  TransactionParams TransactionParams::getGlobalDefaultTransactionParams()
  {
    return global_def_params;
  }

  void
  TransactionParams::setGlobalDefaultMagOrder(unsigned int magorder)
  {
    global_def_params.magorder = magorder;
  }

  unsigned int
  TransactionParams::getGlobalDefaultMagOrder()
  {
    return global_def_params.magorder;
  }
}
