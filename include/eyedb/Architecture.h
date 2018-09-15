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


#ifndef _EYEDB_ARCHITECTURE_H
#define _EYEDB_ARCHITECTURE_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Architecture {

  public:
    /**
       Not yet documented.
       @return
    */
    const char *getArch() const {return arch;}

    /**
       Not yet documented.
       @return
    */
    const char *getMach() const {return mach;}

    /**
       Not yet documented.
       @return
    */
    const char *getOS() const {return os;}

    /**
       Not yet documented.
       @return
    */
    const char *getCC() const {return cc;}

    /**
       Not yet documented.
       @return
    */
    bool isBigEndian() const {return is_big_endian;}

    /**
       Not yet documented.
       @return
    */
    static Architecture *getArchitecture();

  private:
    char *arch;
    char *mach;
    char *os;
    char *cc;
    bool is_big_endian;

    Architecture(const Architecture &);
    Architecture& operator=(const Architecture &);

    ~Architecture() {
      free(arch);
      free(os);
      free(mach);
      free(cc);
    }

    friend class Object;

  public: /* restricted */
    static void init();
    static void _release();

    Architecture(const char *_arch, const char *_mach,
		 const char *_os, const char *_cc,
		 bool _is_big_endian) :
      arch(strdup(_arch)), mach(strdup(_mach)), os(strdup(_os)),
      cc(strdup(_cc)), is_big_endian(_is_big_endian) { }
  };

  Architecture *getArchitecture();

  /**
     @}
  */

}

#endif
