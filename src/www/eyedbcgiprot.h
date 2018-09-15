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

#define IDBW_GET_PORT      0xef234847
#define IDBW_CHILD_READY   0xfeeda173
#define IDBW_GEN_COOKIE    0x76efeacd
#define IDBW_CHECK_COOKIE  0x1e837fda
#define IDBW_COOKIE_REOPEN 0x01dfe394
#define IDBW_CHECK_REOPEN  0x61728394
#define IDBW_CHILD_EXITING 0x2fa87612

#define IDBW_OK            0xfefa2373
#define IDBW_ERROR         0x1f2e8290
#define IDBW_REOPEN        0x18881200
#define IDBW_TIMEOUT       0x71800192

#define idbWSTART          0x81928371
#define idbWSTOP           0xabc81928

#define idbWPROTMAGIC      0xf2837e14

struct idbWProtHeader {
  unsigned long magic;
  int argc;
  unsigned int size;
};
