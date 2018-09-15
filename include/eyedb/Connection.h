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


#ifndef _EYEDB_CONNECTION_H
#define _EYEDB_CONNECTION_H

class ConnHandle;

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class ConnectionPeer;
  class ServerMessageDisplayer;

  /**
     Not yet documented.
  */
  class Connection {

    // ----------------------------------------------------------------------
    // Connection Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
    */
    Connection();

    /**
       Not yet documented
       @param open
       @param host
       @param port
       @return
    */
    Connection(bool opening, const char *host = DefaultHost,
	       const char *port = DefaultIDBPort);

    /**
       Not yet documented
       @param host
       @param port
       @return
    */
    Status open(const char *host = DefaultHost,
		const char *port = DefaultIDBPort);

    /**
       Not yet documented
       @return
    */
    Status close();

    /**
       Not yet documented
       @return
    */
    const char *getHost() const;

    /**
       Not yet documented
       @return
    */
    const char *getIDBPort() const;

    /**
       Not yet documented
       @return
    */
    int getServerUid() const;

    /**
       Not yet documented
       @return
    */
    int getServerPid() const;

    /**
       Not yet documented
       @return
    */
    Status sendInterrupt();

    /**
       Not yet documented
       @return
    */
    ConnHandle *getConnHandle() {return connh;}

    /**
       Not yet documented
    */
    ~Connection();

    /**
       Not yet documented
       @param host
    */
    static void setDefaultHost(const char *host);

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultHost();

    /**
       Not yet documented
       @param port
    */
    static void setDefaultIDBPort(const char *port);

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultIDBPort();

    /**
       Not yet documented
       @param set_auth_required
    */
    static void setAuthRequired(Bool set_auth_required);

    /**
       Not yet documented
       @param user
    */
    static void setDefaultUser(const char *user);

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultUser();

    /**
       Not yet documented
       @param passwd
    */
    static void setDefaultPasswd(const char *passwd);

    /**
       Not yet documented
       @return
    */
    static const char *getDefaultPasswd();

    /**
       Not yet documented
       @return
    */
    static std::string makeUser(const char *_ser);

    static const char
      LocalHost[],
      DefaultHost[],
      DefaultIDBPort[],
      DefaultIDBPortValue[];

    /**
       Not yet documented
       @param ud
       @return
    */
    void *setUserData(void *ud);

    /**
       Not yet documented
       @return
    */
    void *getUserData(void);

    /**
       Not yet documented
       @return
    */
    const void *getUserData(void) const;

    /**
       Not yet documented
       @param dsp
       @return
    */
    Status echoServerMessages(const ServerMessageDisplayer &dsp) const;

    // could be called from server side only
    /**
       Not yet documented
       @return
    */
    static Bool isBackendInterrupted();

    /**
       Not yet documented
       @param msg
    */
    static void setServerMessage(const char *msg);

    /**
       Not yet documented
       @param type
       @param data
       @param len
    */
    static void setOutOfBandData(unsigned int type, unsigned char *data,
				 unsigned int len);

    // ----------------------------------------------------------------------
    // Connection Private Part
    // ----------------------------------------------------------------------
  private:
    friend class ConnectionPeer;

    char *host;
    char *port;
    int sv_pid, sv_uid;
    void *user_data;
    void *oql_info;

    static char *default_host, *default_port, *default_user, *default_passwd;

    static Bool set_auth_required;

    ConnHandle *connh;

    Status echoServerMessages(unsigned int mask, FILE *fd, std::ostream *os) const;

    // ----------------------------------------------------------------------
    // Connection Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    static void init();
    static void _release();
    void *setOQLInfo(void *);
    void *getOQLInfo();
    struct ServerMessageContext;
  };

  class ServerMessageDisplayer {
  public:
    virtual void display(const char *msg) const = 0;
    virtual ~ServerMessageDisplayer() {}
  };

  class StdServerMessageDisplayer : public ServerMessageDisplayer {

  public:
    StdServerMessageDisplayer(FILE *fd = stdout);
    StdServerMessageDisplayer(std::ostream &);

    virtual void display(const char *msg) const;

    virtual ~StdServerMessageDisplayer() {}

  private:
    FILE *fd;
    std::ostream *os;
  };

  /**
     @}
  */

}

#endif
