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

#ifndef _EYEDB_GENCONTEXT_H
#define _EYEDB_GENCONTEXT_H

namespace eyedb {

  class GenContext {
  public:
    GenContext(FILE *, const char *package = 0, const char *rootclass = 0);
    void push();
    void pop();
    const char *get() const;
    void print();
    FILE *getFile();
    const char *getPackage() const {return package;}
    const char *getRootclass() const {return rootclass;}
    ~GenContext();

  private:
    char *buff;
    int buff_len;
    int buff_alloc;
    FILE *fd;
    char *package;
    char *rootclass;
  };

}

#endif