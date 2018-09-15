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

#ifndef _EYEDB_CONFIG_H
#define _EYEDB_CONFIG_H

#include <map>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     A class storing the configuration variables values
  */
  class Config {

  public:
    typedef std::map<std::string, bool> VarMap;

    /**
       Not yet documented.
       @param name
    */

    Config(const std::string &name);

    /**
       Not yet documented.
       @param name
       @param var_map
    */

    Config(const std::string &name, const VarMap &var_map);

    /**
       Not yet documented.
       @param config
    */

    Config(const Config &config);

    /**
       Not yet documented.
    */

    std::string getName() const {return name;}

    struct Item {
      char *name;
      char *value;

      Item();
      Item(const char *name, const char *value);
      Item(const Item &item);
      Item& operator=(const Item &item);
      ~Item();
    };

    /**
       Not yet documented.
       @param file
       @param quietFileNotFoundError
    */
    bool add(const char *file, bool quietFileNotFoundError = false);

    /**
       Not yet documented.
       @param configFileName
       @param envVariable
       @param defaultFilename
    */

    void loadConfigFile( const std::string& configFilename, const char* envVariable, const char* defaultFilename);

    /**
       Not yet documented.
       @param name
    */

    const char *getValue(const char *name, bool expand_vars = true) const;

    /**
       Not yet documented.
       @param name
       @param value
    */

    void setValue(const char *name, const char *value);

    /**
       Not yet documented.
       @param item_cnt
    */

    Item *getValues(unsigned int &item_cnt, bool expand_vars = false) const;

    /**
       Not yet documented.
    */

    static void init();

    /**
       Not yet documented.
    */

    static void _release();

    static bool isBuiltinVar(const char *name);

    static bool isUserVar(const char *name);

    ~Config();

  private:
    friend class Object;

    Config();
    Config& operator=(const Config &config);

    VarMap *var_map;

    std::string name;

    void garbage();

    LinkedList list;

    friend std::ostream& operator<< (std::ostream&, const Config&);

    void checkIsIn(const char *name);
  };

  /**
     @}
  */

  std::ostream& operator<< (std::ostream&os, const Config &config);
}

#endif
