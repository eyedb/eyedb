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



#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "GenContext.h"

#define GENCONTEXT_INC 2

namespace eyedb {

  GenContext::GenContext(FILE *_fd, const char *_package,
			 const char *_rootclass)
  {
    buff_alloc = 32;
    buff = (char *)malloc(buff_alloc);
    buff_len = 0;
    buff[buff_len] = 0;
    fd = _fd;
    package = (_package ? strdup(_package) : 0);
    rootclass = (_rootclass ? strdup(_rootclass) : 0);
  }

  GenContext::~GenContext()
  {
    free(buff);
    free(package);
    free(rootclass);
  }

  void GenContext::push()
  {
    if (buff_len + GENCONTEXT_INC >= buff_alloc)
      {
	buff_alloc = buff_len + GENCONTEXT_INC + 32;
	buff = (char *)realloc(buff, buff_alloc);
      }

    for (int i = 0; i < GENCONTEXT_INC; i++)
      buff[buff_len++] = ' ';

    buff[buff_len] = 0;
  }

  void GenContext::pop()
  {
    buff_len -= GENCONTEXT_INC;

    assert(buff_len >= 0);
    buff[buff_len] = 0;
  }

  void GenContext::print()
  {
    fprintf(fd, "%s", buff);
  }

  FILE *GenContext::getFile()
  {
    return fd;
  }

  const char *GenContext::get() const
  {
    return buff;
  }

}
  
