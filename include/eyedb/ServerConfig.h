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


#ifndef _EYEDB_SERVER_CONFIG_H
#define _EYEDB_SERVER_CONFIG_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     A class storing the configuration variables values for the server.
  */
  class ServerConfig : public Config {

  public:
    /**
       Not yet documented.
    */

    static ServerConfig *getInstance();

    /**
       Not yet documented.
       @param name
    */

    static const char *getSValue(const char *name) {
      return getInstance()->getValue(name);
    }

    /**
       Not yet documented.
       @param file
    */

    static Status setConfigFile(const std::string &file);

    /**
       Not yet documented.
    */

    static void _release();

  private:
    ServerConfig();

    static ServerConfig *instance;

    static std::string config_file;
    void setDefaults();
  };


  /**
     @}
  */
}

#endif
