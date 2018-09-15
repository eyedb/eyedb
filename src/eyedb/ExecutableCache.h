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


#ifndef _EYEDB_EXECUTABLE_CACHE_H
#define _EYEDB_EXECUTABLE_CACHE_H

namespace eyedb {

  class Database;
  class Signature;
  class Argument;
  class ArgArray;

  class ExecutableCache;

  class ExecutableItem {

    friend class ExecutableCache;
  public:

    ExecutableItem(Database *db, const char *intname,
		   const char *name,
		   int exec_type, int isStaticExec, const Oid &cloid, 
		   const char *extref,
		   Signature *signature,
		   const Oid& oid);

    Status check();
    Status execute(Object *, ArgArray *argarray, Argument *retarg);
    ~ExecutableItem();

  private:
    Database *db;
    int exec_type;
    char *intname;
    char *extref;
    Object *exec;
    void *dl;
    void *csym;
  };

  class ExecutableCache {
  
  public:
    ExecutableCache();

    void insert(ExecutableItem *item);
    void remove(const char *intname);
    void remove(ExecutableItem *);

    ExecutableItem *get(const char *intname);

    ~ExecutableCache();

  private:
    unsigned int nkeys;
    unsigned int mask;
    LinkedList **lists;
    unsigned int get_key(const char *);
  };
}

#endif

  
