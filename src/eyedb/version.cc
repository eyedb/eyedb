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
#include "version_p.h"

namespace eyedb {

  const char *
  convertVersionNumber(int version)
  {
#define MAJORCOEF 100000
#define MINORCOEF   1000
    static char s_version[32];
    int major     = version/MAJORCOEF;
    int minor     = (version - major*MAJORCOEF)/MINORCOEF;
    int patch_level = (version - major*MAJORCOEF - minor*MINORCOEF);
    sprintf(s_version, "%d.%d.%d", major, minor, patch_level);
    return s_version;
  }
}
