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


#ifndef _EYEDB_OPTS_H
#define _EYEDB_OPTS_H

class GetOpt;

#include <vector>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  void print_standard_usage(GetOpt &getopt, const std::string &append = "",
			    std::ostream &os = std::cerr,
			    bool server = false);

  void print_standard_help(GetOpt &getopt,
			   const std::vector<std::string> &options,
			   std::ostream &os = std::cerr,
			   bool server = false);

  void print_common_help(std::ostream &os, bool server = false);

  void print_common_usage(std::ostream &os, bool server = false);

  void print_use_help(std::ostream &os);

  /**
     @}
  */

}

#endif
