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


#ifndef _EYEDB_EXCEPTION_H
#define _EYEDB_EXCEPTION_H

#include <stdarg.h>

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  enum SeverityStatus {
    _Warning=1,
    _Error,
    _FatalError
  };

  class Exception;

  typedef const Exception *Status;

  /**
     Not yet documented.
  */
  class Exception {

    // ----------------------------------------------------------------------
    // Exception Interface
    // ----------------------------------------------------------------------
  public:

    /**
       Not yet documented
       @param status
       @param severity
    */
    Exception(int status = 0, SeverityStatus severity = _Error);

    /**
       Not yet documented
       @param parent
       @param status
       @param severity
    */
    Exception(Status parent, int status = 0,
	      SeverityStatus severity = _Error);

    /**
       Not yet documented
       @param e
    */
    Exception(const Exception &e);

    /**
       Not yet documented
       @param e
       @return
    */
    Exception& operator=(const Exception &e);

    /**
       Not yet documented
       @param severity
       @return
    */
    Status setSeverity(SeverityStatus severity);

    /**
       Not yet documented
       @return
    */
    SeverityStatus getSeverity() const;

    /**
       Not yet documented
       @return
    */
    Status getParent() const;

    /**
       Not yet documented
       @param status
    */
    void setStatus( int status);

    /**
       Not yet documented
       @return
    */
    int getStatus() const;

    /**
       Not yet documented
       @param fmt
       @return
    */
    Status setString(const char *fmt, ...);

    /**
       Not yet documented
       @param severity
       @param fmt
       @return
    */
    Status setString(SeverityStatus severity, const char *fmt, ...);

    /**
       Not yet documented
       @param status
       @param severity
       @param fmt
       @return
    */
    Status setString(int status, SeverityStatus severity, const char *fmt, ...);

    /**
       Not yet documented
       @return
    */
    const char *getString() const;

    /**
       Not yet documented
       @return
    */
    const char *getDesc() const;

    /**
       Not yet documented
       @param ud
       @return
    */
    Status setUserData(Any ud);

    /**
       Not yet documented
       @return
    */
    Any getUserData() const;

    /**
       Not yet documented
       @param handler
       @param hud
    */
    static void setHandler(void (*handler)(Status, void *), void *hud = NULL);

    /**
       Not yet documented
       @return
    */
    static void (*getHandler())(Status, void *);

    /**
       Not yet documented
       @return
    */
    static void *getHandlerUserData();

    void applyHandler() const;

    enum Mode {
      ExceptionMode = 32,
      StatusMode    = 64
    };

    /**
       Not yet documented
       @param mode
       @return
    */
    static Exception::Mode setMode(Exception::Mode mode);

    /**
       Not yet documented
       @return
    */
    static Exception::Mode getMode();

    /**
       Not yet documented
       @param fd
       @return
    */
    Status print(FILE *fd = stderr, bool newline = true) const;

    /**
       Not yet documented
       @param err
       @param fmt
       @return
    */
    static Status make(int err, const char *fmt, ...);

    /**
       Not yet documented
       @param err
       @param s
       @return
    */
    static Status make(int err, const std::string &s);

    /**
       Not yet documented
       @param fmt
       @return
    */
    static Status make(const char *fmt, ...);

    /**
       Not yet documented
       @param s
       @return
    */
    static Status make(const std::string &s);

    /**
       Not yet documented
       @param err
       @return
    */
    static Status make(int err);


    /**
       Not yet documented
       @param status
       @return
    */
    static Status make(Status status);

    ~Exception();

    // ----------------------------------------------------------------------
    // Exception Private Part
    // ----------------------------------------------------------------------
  private:
    Exception *parent;
    int status; // will be _Error
    SeverityStatus severity;
    char *string;
    Any user_data;
    static void* handler_user_data;
    static void (*handler)(Status, void *);
    static Exception::Mode mode;

    static struct _desc {
      const char *status_string;
      const char *desc;
    } statusDesc[IDB_N_ERROR];

    static void (*print_method)(Status, FILE*);
    void setPrintMethod(void (*)(Status, FILE*));
    void (*getPrintMethod(void))(Status, FILE*);
  
    void statusMake(Status, int, SeverityStatus);
    void print_realize(FILE*, bool newline) const;

    // ----------------------------------------------------------------------
    // Exception Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init(void);
    static void _release(void);
  };

  std::ostream& operator<<(std::ostream&, const Exception &);
  std::ostream& operator<<(std::ostream&, Status);

  // ----------------------------------------------------------------------
  // Status CPP Macros
  // ----------------------------------------------------------------------

  static const Status Success = (Status)0;

  extern Status _status_;

#define eyedb_CHECK(e) \
	((_status_ = (e)) ? \
	(void)fprintf(stderr, "file %s, line %d : %s\n<<\n  %s\n>>\n", \
	      	      __FILE__, __LINE__, _status_->getDesc(), #e), \
	_status_ : _status_)

#define eyedb_ASSERT(e) \
	if (eyedb_CHECK(e)) \
		abort()

#define eyedb_CHECK_RETURN(e) \
	if ((_status_ = (e))) return _status_

  /**
     @}
  */

}

#endif
