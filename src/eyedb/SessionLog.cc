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

#include <eyedbconfig.h>

#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "SessionLog.h"
#include <time.h>
#include <eyedbsm/smd.h>
#include <eyedblib/rpc_lib.h>
#include <eyedblib/strutils.h>
#include "comp_time.h"
#include <eyedblib/m_mem.h>
//#include <eyedbsm/eyedbsm_p.h>

#define MAXPORTS 8
#define PORTLEN 127
#define DATDIRLEN 511
#define LOGDEVLEN 511

#define eyedbsm_mutexLock(X, Y) eyedbsm::mutexLock(X, Y)
#define eyedbsm_mutexUnlock(X, Y) eyedbsm::mutexUnlock(X, Y)

//#define TRACE

namespace eyedb {

  struct SessionHead {
    unsigned int magic;
    eyedbsm::MutexP mp;
    char version[32];
    int up;
    time_t start;
    char smdport[PORTLEN+1];
    int nports;
    char hosts[MAXPORTS][PORTLEN+1];
    char ports[MAXPORTS][PORTLEN+1];
    int uid;
    int pid;
    char datdir[DATDIRLEN+1];
    char logdev[LOGDEVLEN+1];
    int loglevel;
    int nconns;
    eyedbsm::XMOffset conn_first;
  };

#define MAXDBS 8

  struct ClientInfo {
    time_t start;
    char hostname[64];
    char portname[64];
    char username[64];
    char progname[128];
    int n_dbs;
    struct {
      char dbname[32];
      char userauth[10];
      int flags;
    } dbs[MAXDBS];
    int prog_pid;
    int backend_pid;
    eyedbsm::XMOffset conn_prev, conn_next;
  };

  SessionLog *SessionLog::sesslog;

#include <pwd.h>

  static const char *
  getUserName(int uid)
  {
    struct passwd *p = getpwuid(uid);
    if (!p)
      return "<unknown>";
    return p->pw_name;
  }

#define SE_CONNLOG_MAGIC ((unsigned int)0x3f1920ab)
//#define CONNLOG_SIZE 0x10000
#define CONNLOG_SIZE (sizeof(SessionHead) + 128 * sizeof(ClientInfo))

#ifdef HAVE_SEMAPHORE_POLICY_SYSV_IPC
  Status
  SessionLog::init_sems()
  {
    vd = (void *)calloc(sizeof(eyedbsm::DbDescription), 1);
    int *semkeys = (int *)((eyedbsm::DbDescription *)vd)->semkeys;
    smdcli_conn_t *conn = smdcli_open(smd_get_port());
    if (!conn)
      return Exception::make(IDB_ERROR, "sessionlog: cannot connect to eyedbsmd ");
    smdcli_init_getsems(conn, ServerConfig::getSValue("default_dbm"), semkeys);

    if (semkeys[0] <= 0)
      return Exception::make(IDB_ERROR, "sessionlog: cannot create semaphores ");
    return Success;
  }
#endif

  SessionLog::SessionLog(const SessionLog &sesslog)
  {
    init(sesslog.host, sesslog.port, sesslog.logdir, True);
  }

  SessionLog::SessionLog(const char *host, const char *port,
			 const char *logdir, Bool writing)
  {
    init(host, port, logdir, writing);
  }

  void SessionLog::init(const char *host, const char *port, const char *logdir,
			Bool writing)
  {
#ifdef HAVE_SEMAPHORE_POLICY_SYSV_IPC
    status = init_sems();
    if (status)
      return;
#else
    //vd = (void *)calloc(sizeof(eyedbsm::DbDescription), 1);
    // fix compilation for now (see issue #2)
    vd = (void *)calloc(10000, 1);
#endif
    islocked = False;
    addr_connlog = 0;
    status = openRealize(host, port, logdir, False, writing);
    if (status)
      return;

    sesslog = this;
  }

  // create constructor
  SessionLog::SessionLog(const char *logdir, const char *version,
			 int nports, const char *hosts[], const char *ports[],
			 const char *datdir,
			 const char *logdev, int loglevel)
  {
    int i;

#ifdef HAVE_SEMAPHORE_POLICY_SYSV_IPC
    status = init_sems();
    if (status)
      return;
#else
    //vd = (void *)calloc(sizeof(eyedbsm::DbDescription), 1);
    // fix compilation for now (see issue #2)
    vd = NULL;
#endif
    islocked = False;
    addr_connlog = 0;
    status = openRealize(hosts[0], ports[0], logdir, True, True);

    if (status)
      return;

    file_cnt = nports;
    if (file_cnt > 1) {
      files = (char **)realloc(files, file_cnt * sizeof(char *));

      for (i = 1; i < file_cnt; i++) {
	files[i] = makeFile(hosts[i], ports[i], logdir);
	symlink(files[0], files[i]);
      }
    }

    sesslog = this;

    memset(&connhead, sizeof(connhead), 0);
    connhead->magic = SE_CONNLOG_MAGIC;
    strcpy(connhead->version, version);
    time(&connhead->start);
    connhead->up = 1;
    strncpy(connhead->smdport, smd_get_port(), PORTLEN);
    connhead->smdport[PORTLEN] = 0;
    connhead->nports = nports;
    for (i = 0; i < nports; i++) {
      strncpy(connhead->hosts[i], hosts[i], PORTLEN);
      connhead->hosts[i][PORTLEN] = 0;
      strncpy(connhead->ports[i], ports[i], PORTLEN);
      connhead->ports[i][PORTLEN] = 0;
    }

    strncpy(connhead->datdir, datdir, DATDIRLEN);
    connhead->datdir[DATDIRLEN] = 0;
    connhead->pid = rpc_getpid();
    connhead->uid = getuid();

    if (logdev) {
      strncpy(connhead->logdev, logdev, LOGDEVLEN);
      connhead->logdev[LOGDEVLEN] = 0;
    }

    connhead->loglevel = loglevel;
    connhead->conn_first = XM_NULLOFFSET;
  }

  static char *truedir(const char *rs)
  {
    char *s, *os;
    s = strdup(rs);

    os = s;
    if (!strchr(s, '.'))
      return s;

    int sx_cnt = 0;
    int sx_alloc = 0;
    char **sx = 0;
    char *p;

    if (*s == '/')
      s++;

    for (;;) {
      int stopafter = 0;
      if (!(p = strchr(s, '/'))) {
	p = s + strlen(s);
	stopafter = 1;
      }

      char *q = (char *)malloc(p-s+1);
      strncpy(q, s, p-s);
      q[p-s] = 0;
      if (!strcmp(q, "..")) {
	sx_cnt--;
	free(sx[sx_cnt]);
	free(q);
      }
      else {
	if (sx_cnt >= sx_alloc) {
	  sx_alloc++;
	  sx = (char **)realloc(sx, sx_alloc*sizeof(char *));
	}
	sx[sx_cnt] = q;
	sx_cnt++;
      }

      if (stopafter)
	break;
      s = p+1;
    }

#if 1
    std::string str;
    for (int i = 0; i < sx_cnt; i++) {
      str += "/";
      str += sx[i];
      free(sx[i]);
    }
    char* r = strdup(str.c_str());
#else
    char *r = (char *)malloc(strlen(s)+1);
    *r = 0;
    for (int i = 0; i < sx_cnt; i++) {
      strcat(r, "/");
      strcat(r, sx[i]);
      free(sx[i]);
    }
#endif

    free(sx);
    free(os);
    return r;
  }

  char *
  SessionLog::makeFile(const char *_host, const char *_port, const char *_logdir)
  {
    static const char conn_prefix[] = ".eyedb_";
    static const char conn_suffix[] = ".con";
    char hostname[256] = {0};
    char *file;

    if (!_logdir || !_port)
      return 0;

    port = strdup(_port);
    host = strdup(_host);

    //char *port_file = truedir((std::string(host) + ":" + _port).c_str());
    std::string host_port = std::string(host) + ":" + _port;
    char *port_file = truedir(host_port.c_str());
      
    char *p = port_file;
    while (p = strchr(p, '/'))
      *p = '_';

    logdir = strdup(_logdir ? _logdir : "/tmp");

    gethostname(hostname, sizeof(hostname)-1);

    file = (char *)malloc(strlen(logdir) +
			  1 + /* / */
			  strlen(conn_prefix) +
			  strlen(port_file) +
			  1 + /* : */
			  strlen(hostname) +
			  strlen(conn_suffix) +
			  1);
  
    sprintf(file, "%s/%s%s_%s%s", logdir, conn_prefix, port_file, hostname,
	    conn_suffix);

    return file;
  }

  Status
  SessionLog::openRealize(const char *host, const char *port,
			  const char *logdir, Bool create,
			  Bool writing)
  {
    Error err = (create ? IDB_SESSION_LOG_CREATION_ERROR :
		 IDB_SESSION_LOG_OPEN_ERROR);
    int fd;
    file_cnt = 1;
    files = (char **)malloc(file_cnt * sizeof(char *));
    files[0] = makeFile(host, port, logdir);

    // 21/03/06: must be writable for pthread_mutex version
    writing = True;

    xm_connlog = 0;

    if (!files[0])
      return Exception::make(err, "eyedb environment error");

    if (create)
      fd = open(files[0], O_CREAT | O_TRUNC | O_RDWR,
		0644 /*SE_DEFAULT_CREATION_MODE*/);
    else {
      if (access(files[0], F_OK) < 0)
	return Exception::make(IDB_CONNECTION_LOG_FILE_ERROR,
			       "cannot access connection file '%s'",
			       files[0]);

      if (access(files[0], R_OK) < 0)
	return Exception::make(IDB_SESSION_LOG_OPEN_ERROR,
			       "cannot open connection file '%s' "
			       "for reading", files[0]);

      if (writing) {
	if (access(files[0], W_OK) < 0)
	  return Exception::make(IDB_SESSION_LOG_OPEN_ERROR,
				 "cannot open connection file '%s' "
				 "for writing", files[0]);
	
	fd = open(files[0], O_RDWR);
      }
      else
	fd = open(files[0], O_RDONLY);
    }
    
    if (fd < 0 && create)
      return Exception::make(err,
			     "cannot %s connection file '%s'",
			     (create ? "create" :
			      "open"), files[0]);
    if (fd < 0)
      return Exception::make(IDB_SESSION_LOG_OPEN_ERROR,
			     "cannot open connection file '%s'",
			     files[0]);

    if (create && ftruncate(fd, CONNLOG_SIZE) < 0) {
      close(fd);
      return Exception::make(err,
			     "cannot create connection file '%s'", files[0]);
    }

    unsigned int prot = PROT_READ;
    if (writing)
      prot |= PROT_WRITE;
    if (!(m_connlog = m_mmap(0, CONNLOG_SIZE, prot,
			     MAP_SHARED, fd, 0, (char **)&addr_connlog,
			     files[0], 0, 0))) {
      close(fd);
      return Exception::make(err,
			     "cannot map connection file '%s' for size %d",
			     files[0], CONNLOG_SIZE);
    }
    
    if (!create) {
      struct stat st;
      if (fstat(fd, &st) < 0) {
	close(fd);
	return Exception::make(err, "cannot stat connection "
			       "log file '%s'", files[0]);
      }

      if (st.st_size < CONNLOG_SIZE && ftruncate(fd, CONNLOG_SIZE) < 0) {
	close(fd);
	return Exception::make(err,
			       "cannot create connection "
			       "log file '%s'", files[0]);
      }
    }

    close(fd);
    m_lock(m_connlog);

    connhead = (SessionHead *)addr_connlog;

    if (create)
      xm_connlog = eyedbsm::XMCreate(addr_connlog + sizeof(SessionHead),
				     CONNLOG_SIZE - sizeof(SessionHead), vd);
    else
      xm_connlog = eyedbsm::XMOpen(addr_connlog + sizeof(SessionHead), vd);

    if (!xm_connlog)
      return Exception::make(err,
			     "cannot map connection file '%s' for size %d",
			     files[0], CONNLOG_SIZE);
    return Success;
  }

  Status
  SessionLog::add(const char *hostname, const char *portname,
		  const char *username,
		  const char *progname, int pid,
		  ClientSessionLog*& clientLog)
  {
    ClientInfo *conninfo =
      (ClientInfo *)XMAlloc(xm_connlog, sizeof(ClientInfo));

    if (!conninfo)
      return Exception::make(IDB_SESSION_LOG_NO_SPACE_LEFT,
			     "no space left on connection file");

    memset(conninfo, 0, sizeof(*conninfo));

    time(&conninfo->start);
    strncpy(conninfo->hostname, hostname, sizeof(conninfo->hostname)-1);
    strncpy(conninfo->portname, portname, sizeof(conninfo->portname)-1);
    strncpy(conninfo->username, username, sizeof(conninfo->username)-1);
    strncpy(conninfo->progname, progname, sizeof(conninfo->progname)-1);

    conninfo->prog_pid = pid;
    conninfo->backend_pid = rpc_getpid();

#ifdef TRACE
    fprintf(stderr, "SessionLog %d tries to locked\n", rpc_getpid());
#endif
    eyedbsm_mutexLock(xm_connlog->mp, 0);
#ifdef TRACE
    fprintf(stderr, "SessionLog %d is locked\n", rpc_getpid());
#endif
    islocked = True;

    conninfo->conn_next = connhead->conn_first;
    conninfo->conn_prev = XM_NULLOFFSET;

    if (connhead->conn_first) {
      ClientInfo *first = (ClientInfo *)
	XM_ADDR(xm_connlog, connhead->conn_first);
      first->conn_prev = XM_OFFSET(xm_connlog, conninfo);
    }

    connhead->conn_first = XM_OFFSET(xm_connlog, conninfo);
    connhead->nconns++;

    eyedbsm_mutexUnlock(xm_connlog->mp, 0);
#ifdef TRACE
    fprintf(stderr, "SessionLog %d is UNlocked\n", rpc_getpid());
#endif
    islocked = False;

    clientLog = new ClientSessionLog(conninfo);

#ifdef TRACE
    fprintf(stderr, "log add (%s, %s, %s, %d, %d)\n",
	    hostname, username, progname, pid, rpc_getpid());
#endif

    return Success;
  }

  void
  SessionLog::suppress(ClientSessionLog *clientLog)
  {
    ClientInfo *conninfo = clientLog->getClientInfo();

#ifdef TRACE
    fprintf(stderr, "SessionLog %d tries to locked\n", rpc_getpid());
#endif
    eyedbsm_mutexLock(xm_connlog->mp, 0);
#ifdef TRACE
    fprintf(stderr, "SessionLog %d is locked\n", rpc_getpid());
#endif
    islocked = True;

    if (conninfo->conn_next)
      ((ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_next))->conn_prev =
	conninfo->conn_prev;

    if (conninfo->conn_prev)
      ((ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_prev))->conn_next =
	conninfo->conn_next;
    else
      connhead->conn_first = conninfo->conn_next;

#ifdef TRACE
    fprintf(stderr, "log suppress (%s, %s, %s, %d, %d)\n",
	    conninfo->hostname, conninfo->username, conninfo->progname,
	    conninfo->prog_pid, conninfo->backend_pid);
#endif

    connhead->nconns--;
    eyedbsm_mutexUnlock(xm_connlog->mp, 0);
#ifdef TRACE
    fprintf(stderr, "SessionLog %d is UNlocked\n", rpc_getpid());
#endif
    islocked = False;

    XMFree(xm_connlog, conninfo);
  }

  static Bool
  check_server(ClientInfo *conninfo)
  {
#ifdef HAVE_SLASH_PROC
    if (access((std::string("/proc/") +	str_convert((long)conninfo->backend_pid)).c_str(), 
	       F_OK) >= 0)
      return True;
    return False;
#else
    return True;
#endif
  }

  int
  SessionLog::get_nb_clients()
  {
    ClientInfo *conninfo =
      (ClientInfo *)XM_ADDR(xm_connlog, connhead->conn_first);

    int nb_clients = 0;
    for (int n = connhead->nconns-1; n >= 1; n--)
      conninfo = (ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_next);

    while(conninfo) {
      if (check_server(conninfo))
	nb_clients++;

      conninfo = (ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_prev);
    }

    return nb_clients;
  }

  void SessionLog::display(FILE *fd, Bool nolock)
  {
    int n;
    ClientInfo *conninfo;

    if (!connhead)
      {
	fprintf(fd, "EyeDB Server %s:%s is down\n", host, port);
	return;
      }

    if (!nolock) {
#ifdef TRACE
      fprintf(stderr, "SessionLog %d tries to locked\n", rpc_getpid());
#endif
      eyedbsm_mutexLock(xm_connlog->mp, 0);
#ifdef TRACE
      fprintf(stderr, "SessionLog %d is locked\n", rpc_getpid());
#endif
      islocked = True;
    }
    else if (sesslog && sesslog->islocked) {
#ifdef TRACE
      fprintf(stderr, "Warning SessionLog is locked by %d\n",
	      sesslog->connhead->pid);
#endif
    }

    if (!connhead->up) {
      fprintf(fd, "EyeDB Server %s:%s is down from %s", host, port,
	      ctime(&connhead->start));
      if (!nolock) {
	eyedbsm_mutexUnlock(xm_connlog->mp, 0);
#ifdef TRACE
	fprintf(stderr, "SessionLog %d is UNlocked\n", rpc_getpid());
#endif
	islocked = False;
      }
      return;
    }

    fprintf(fd, "EyeDB Server running since %s\n",
	    ctime(&connhead->start));

    fprintf(fd, "  Version       V%s\n", connhead->version);
    fprintf(fd, "  Date          %s\n", getCompilationTime());
    fprintf(fd, "  Architecture  %s\n", Architecture::getArchitecture()->getArch());
    fprintf(fd, "  Program Pid   %d\n", connhead->pid);
    fprintf(fd, "  Running Under %s\n\n", getUserName(connhead->uid));

#ifdef HAVE_EYEDBSMD
    fprintf(fd, "  SMD Port      %s\n", connhead->smdport);
#endif
    
    fprintf(fd, "  Listening on  ");

    const char *indent;
    if (connhead->nports > 1) {
      indent =  "\n                ";
    }
    else
      indent = "";

    for (n = 0; n < connhead->nports; n++) {
      fprintf(fd, "%s%s:%s", (n ? indent : ""), connhead->hosts[n],
	      connhead->ports[n]);
    }

    fprintf(fd, "\n  Datafile Directory %s\n", connhead->datdir);

    if (*connhead->logdev) {
      fprintf(fd, "  Log Device '%s'\n", connhead->logdev);
      if (connhead->loglevel)
	fprintf(fd, "  Log Level %d\n", connhead->loglevel);
    }

    fprintf(fd, "\n");

    int nb_clients = get_nb_clients();
    if (!nb_clients) {
      fprintf(fd, "  No Clients connected.\n");
      if (!nolock) {
	eyedbsm_mutexUnlock(xm_connlog->mp, 0);
#ifdef TRACE
	fprintf(stderr, "SessionLog %d is UNlocked\n", rpc_getpid());
#endif
	islocked = False;
      }
      return;
    }

    fprintf(fd, "  %d Client%s connected\n\n", 
	    nb_clients, nb_clients > 1 ? "s" : "");

    conninfo = (ClientInfo *)XM_ADDR(xm_connlog, connhead->conn_first);

    for (n = connhead->nconns-1; n >= 1; n--)
      conninfo = (ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_next);

    for (n = 0; conninfo;
	 conninfo = (ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_prev)) {

      // solaris only?
      if (!check_server(conninfo))
	continue;

      fprintf(fd, "%sClient #%d\n", (n ? "\n" : ""), n);
      fprintf(fd, "  Connected on %s", ctime(&conninfo->start));
      if (*conninfo->hostname && *conninfo->portname)
	fprintf(fd, "  Host:Port    %s:%s\n", conninfo->hostname,
		conninfo->portname);
      if (*conninfo->username)
	fprintf(fd, "  User Name    %s\n", conninfo->username);
      if (*conninfo->progname)
	fprintf(fd, "  Program Name %s\n", conninfo->progname);
      if (conninfo->prog_pid)
	fprintf(fd, "  Client Pid   %d\n",   conninfo->prog_pid);
      if (conninfo->backend_pid)
	fprintf(fd, "  EyeDB Server Pid %d\n",   conninfo->backend_pid);

      if (conninfo->n_dbs) {
	fprintf(fd, "  Open Database%s ", conninfo->n_dbs > 1 ? "s" : "");
	for (int j = 0; j < conninfo->n_dbs; j++) {
	  fprintf(fd, "%s'%s' [mode=%s]", (j?"\n                   ":""),
		  conninfo->dbs[j].dbname,
		  Database::getStringFlag((Database::OpenFlag)conninfo->dbs[j].flags));
	  if (*conninfo->dbs[j].userauth)
	    fprintf(fd, " [userauth=%s]", conninfo->dbs[j].userauth);
	}
	fprintf(fd, "\n");
      }
      else
	fprintf(fd, "  No Opened Databases\n");

      n++;
    }

    if (!nolock) {
      eyedbsm_mutexUnlock(xm_connlog->mp, 0);
#ifdef TRACE
      fprintf(stderr, "SessionLog %d is UNlocked\n", rpc_getpid());
#endif
      islocked = False;
    }
  }

  void SessionLog::remove()
  {
    connhead->up = 0;
    time(&connhead->start);

    for (int i = 0; i < file_cnt; i++)
      unlink(files[i]);
  }

  Bool SessionLog::isUp() const
  {
    return connhead && connhead->up ? True : False;
  }

  Status
  SessionLog::stopServers(Bool force)
  {
    if (!connhead || !xm_connlog)
      return Exception::make("EyeDB Server %s:%s is down", host, port);

    if (!connhead || !connhead->up)
      return Exception::make("EyeDB Server %s:%s is already down from %s",
			     host, port, ctime(&connhead->start));

    int nb_clients = get_nb_clients();
    if (nb_clients && !force) {
      return Exception::make
	(IDB_ERROR, "%d client%s %s connected.\n"
	 "Use the `stop -f' option to force the servers to stop.",
	 nb_clients,
	 (nb_clients > 1 ? "s" : ""),
	 (nb_clients > 1 ? "are" : "is"));
    }

    // eyedbsm_mutexLock(xm_connlog->mp, 0);

    ClientInfo *conninfo =
      (ClientInfo *)XM_ADDR(xm_connlog, connhead->conn_first);

    while (conninfo) {
      if (check_server(conninfo)) {
	fprintf(stderr, "Killing Client Backend Pid %d\n",
		conninfo->backend_pid);
	kill(conninfo->backend_pid, SIGTERM);
      }
      conninfo = (ClientInfo *)XM_ADDR(xm_connlog, conninfo->conn_next);
    }

    fprintf(stderr, "Killing EyeDB Server Pid %d\n", connhead->pid);
    kill(connhead->pid, SIGTERM);

    // eyedbsm_mutexUnlock(xm_connlog->mp, 0);
    return Success;
  }

  void
  SessionLog::release()
  {
    /*
    if (sesslog && sesslog->islocked)
      eyedbsm_mutexUnlock(&sesslog->mp, 0);
    */
  }

  SessionLog::~SessionLog()
  {
    free(vd);
  }

  ClientSessionLog::ClientSessionLog(ClientInfo *_clinfo)
    : clinfo(_clinfo)
  {
  }

  void
  ClientSessionLog::addDatabase(const char *name, const char *userauth,
				int flags)
  {
    if (clinfo->n_dbs < MAXDBS) { 
      if (!userauth)
	userauth = "";

      strncpy(clinfo->dbs[clinfo->n_dbs].dbname, name, sizeof(clinfo->dbs[clinfo->n_dbs].dbname)-1);
      clinfo->dbs[clinfo->n_dbs].dbname[sizeof(clinfo->dbs[clinfo->n_dbs].dbname)] = 0;
      strncpy(clinfo->dbs[clinfo->n_dbs].userauth, userauth, sizeof(clinfo->dbs[clinfo->n_dbs].userauth)-1);
      clinfo->dbs[clinfo->n_dbs].userauth[sizeof(clinfo->dbs[clinfo->n_dbs].userauth)] = 0;
      clinfo->dbs[clinfo->n_dbs].flags = flags;
      clinfo->n_dbs++;
    }
  }

  void
  ClientSessionLog::suppressDatabase(const char *name, const char *userauth,
				     int flags)
  {
    if (!userauth)
      userauth = "";

    for (int i = 0; i < clinfo->n_dbs; i++)
      if (!strcmp(clinfo->dbs[i].dbname, name) &&
	  !strcmp(clinfo->dbs[i].userauth, userauth) &&
	  clinfo->dbs[i].flags == flags) {
	for (int j = i; j < clinfo->n_dbs-1; j++) {
	  strcpy(clinfo->dbs[j].dbname, clinfo->dbs[j+1].dbname);
	  strcpy(clinfo->dbs[j].userauth, clinfo->dbs[j+1].userauth);
	  clinfo->dbs[j].flags = clinfo->dbs[j+1].flags;
	}

	clinfo->n_dbs--;
	return;
      }
  }
}
