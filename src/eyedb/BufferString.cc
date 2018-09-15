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


#include "BufferString.h"
#include <stdlib.h>
#include <string.h>

namespace eyedb {

BufferString::BufferString(int len)
{
  buff_alloc = len + 32;
  buff = (char *)malloc(buff_alloc);
  buff_len = len;
  buff[buff_len] = 0;
}

void BufferString::append(const char *str)
{
  int len = strlen(str);
  if (buff_len + len >= buff_alloc)
    {
      buff_alloc = buff_len + len + 32;
      buff = (char *)realloc(buff, buff_alloc);
    }
  strcat(buff, str);
  buff_len = strlen(buff);
}

int BufferString::length() const
{
  return buff_len;
}

const char *BufferString::getString() const
{
  return buff;
}

BufferString::~BufferString()
{
  free(buff);
}
}
