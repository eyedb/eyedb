
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


#include "eyedbcgiP.h"
#include "eyedblib/rpc_lib.h"
#include "eyedblib/connman.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>
#include <limits.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "../eyedb/GetOpt.h"

static char *input_buf;
static int input_size;
static int input_fd;
static fd_set fds;
static idbWContext *ctx = &idbW_context;
static int maxservers    = 6;
static int sv_timeout = 300;
static int ck_timeout = INT_MAX;
static Status status;
static int sv_port;
static idbWProcess *proc;
static struct sockaddr_in sock_in_name;
static Bool nod = False;

// forward declarations
static void epilogue(int fd);
static void send_exiting(int);
static int port_open(int port);
static void main_child_loop(idbWProcess *p);


static const char *
spaces(char *buf, int size)
{
  char *p = buf;

  for (int i = 0; i < size; i++, p++)
    if (*p == 0)
      *p = ' ';

  if (p)
    *p = 0;
  return buf ? buf : "";
}

static void
sig_h(int sig)
{
  if (sig == SIGSEGV || sig == SIGBUS)
    {
      char buf[4096];
      sprintf(buf, "\n<hr>\n"
	      "EyeDB WWW server has received an abnormal signal\n"
	      "please contact your server administrator and\n"
	      "gives him the following hint : \n\n<i>%s</i>\n\n<hr>\n",
	      spaces(input_buf, input_size));
      IDB_LOG(IDB_LOG_WWW, ("PID %d : %s", getpid(), buf));
      idbWPrintError(buf);
      epilogue(input_fd);
      send_exiting(idbW_root_proc->pipe_c2p[1]);
      return;
    }

  IDB_LOG(IDB_LOG_WWW, ("receives signal %d\n", sig));
}

static int
usage(const char *prog)
{
  fprintf(stderr, "usage: %s [--alias=ALIAS] [--wdir=DIRECTORY] "
	 "[--cgidir=DIRECTORY] "
	 "[--server-timeout=TIMEOUT] "
	 "[--max-servers=MAXSERVERS] "
	 "[--cookie-timeout=TIMEOUT [--access-file=ACCESS_FILE]\n",
	 prog);
  fflush(stderr);
  //cerr << "\n\nSTANDARD HELP TBD\\n\n";
  //print_standard_usage(getopt, "", std::cerr, true);
  cerr << "\nCommon Options:\n";
  print_common_help(cerr);
  cerr << "\n";
  //[-imgdir <image-temp-directory>] 
  return 1;
}

static char *
load_file(const char *file)
{
  int fd;
  struct stat st;
  if (stat(file, &st) < 0 || (fd = open(file, O_RDONLY)) < 0)
    {
      printf("eyedbcgid: cannot open file '%s' for reading\n", file);
      exit(1);
    }

  char *q = (char *)malloc(st.st_size+1);
  if (read(fd, q, st.st_size) != st.st_size)
    {
      printf("eyedbwwwd: cannot open read file '%s'\n", file);
      exit(1);
    }

  q[st.st_size] = 0;
  return q;
}

static int
getopts(int argc, char *argv[])
{
  for (int n = 1; n < argc; n++) {
    char *s = argv[n];
    string value;
    if (GetOpt::parseLongOpt(s, "server-timeout", &value)) {
      sv_timeout = atoi(value.c_str());
      if (!sv_timeout)
	return usage(argv[0]);
    }
    else if (GetOpt::parseLongOpt(s, "cookie-timeout", &value)) {
      ck_timeout = atoi(value.c_str());
      if (!ck_timeout)
	return usage(argv[0]);
    }
    else if (GetOpt::parseLongOpt(s, "cgidir", &value)) {
      idbW_cgidir = strdup(value.c_str());
    }
    else if (GetOpt::parseLongOpt(s, "alias", &value)) {
      idbW_alias = strdup(value.c_str());
    }
    else if (GetOpt::parseLongOpt(s, "wdir", &value)) {
      idbW_wdir = strdup(value.c_str());
    }
    else if (GetOpt::parseLongOpt(s, "max-servers", &value)) {
      maxservers = atoi(value.c_str());
      if (maxservers <= 0)
	return usage(argv[0]);
    }
    else if (GetOpt::parseLongOpt(s, "access-file", &value)) {
      idbW_accessfile = strdup(value.c_str());
    }
    else if (GetOpt::parseLongOpt(s, "nod")) {
      nod = True;
    }
    else if (!strcmp(s, "-h") || !strcmp(s, "--help"))
      return usage(argv[0]);
    else
      return usage(argv[0]);
  }

  if (!idbW_cgidir)
    idbW_cgidir = "cgi-bin";

  return 0;
}

static void
clear(idbWProcess *p)
{
  close(p->pipe_c2p[0]);
  close(p->pipe_c2p[1]);
  close(p->pipe_p2c[0]);
  close(p->pipe_p2c[1]);
  FD_CLR(p->pipe_c2p[0], &fds);
  FD_CLR(p->pipe_c2p[1], &fds);
  FD_CLR(p->pipe_p2c[0], &fds);
  FD_CLR(p->pipe_p2c[1], &fds);
  p->pipe_c2p[0] = -1;
  p->pipe_c2p[1] = -1;
  p->pipe_p2c[0] = -1;
  p->pipe_p2c[1] = -1;
}

static void
send_ok(int fd)
{
  int msg = IDBW_CHILD_READY;
  write(fd, &msg, sizeof(msg));
}

static void
send_exiting(int fd)
{
  int msg = IDBW_CHILD_EXITING;
  write(fd, &msg, sizeof(msg));
  idbWGarbage();
}

static int
get_port()
{
  static int port_inc = 0;

  if (port_inc == 20)
    port_inc = 0;

  return sv_port + ++port_inc;
}
 
static void
launch(idbWProcess *p, int n)
{
  pipe(p->pipe_c2p);
  pipe(p->pipe_p2c);

  p->port = get_port();

  if ((p->pid = fork()) == 0)
    {
      IDB_LOG(IDB_LOG_WWW, ("launching child pid=%d port=%d\n",
			    getpid(), p->port));

      if (getenv("EYEDBSLEEP"))
	{
	  int sec = atoi(getenv("EYEDBSLEEP"));
	  IDB_LOG(IDB_LOG_WWW, ("sleeping for %d seconds...\n", sec));
	  sleep(sec);
	  IDB_LOG(IDB_LOG_WWW, ("wakes up\n"));
	}	  

      if ((p->fd = port_open(p->port)) < 0)
	{
	  send_exiting(p->pipe_c2p[1]);
	  IDB_LOG(IDB_LOG_WWW, ("launch child pid=%d port=%d "
				"failed: cannot open port\n", 
				getpid(), p->port));
	}
      else
	{
	  if (!idbWOpenConnection())
	    {
	      send_exiting(p->pipe_c2p[1]);
	      IDB_LOG(IDB_LOG_WWW, ("launch child pid=%d port=%d "				    "failed: cannot open connection\n", 
				    getpid(), p->port));
	    }
	  else
	    {
	      send_ok(p->pipe_c2p[1]);
	      IDB_LOG(IDB_LOG_WWW, ("launch child pid=%d port=%d "
				    "sucessful\n", 
				    getpid(), p->port));
	      main_child_loop(p);
	    }
	}

      exit(1);
    }
}

static void
epilogue(int fd)
{
  if (ctx->action != idbW_GETCOOKIE)
    idbWTail();

  char zero = 0;
  write(fd, &zero, 1);
}

static void
get_command_line(const char *input_buf, int argc, char *argv[], int &n)
{
  n = 0;
  argv[n++] = (char*)"eyedbwwwd"; /*@@@@warning cast*/
  argv[n++] = (char *)input_buf;

  char *q = (char *)input_buf;
  std::string s = argv[0];
  s += std::string(" ") + input_buf;

  for (int i = 1; i < argc; i++, n++)
    {
      q += strlen(q) + 1;
      argv[n] = q;
      if (!strcmp(argv[n-1], "-p"))
	s += " #########";
      else
	s += std::string(" ") + q;
    }
      
  idbW_cmd = s;
  IDB_LOG(IDB_LOG_WWW, ("cmd: '%s'\n", s.c_str()));
}

static void
main_child_loop(idbWProcess *p)
{
  idbW_root_proc = p;

  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(p->fd, &fds);

  for (;;)
    {
      int r;
      struct timeval tv;
      tv.tv_sec = sv_timeout;
      tv.tv_usec = 0;

      r = select(p->fd+1, &fds, 0, 0, &tv);
      if (r < 0)
	{
	  if (errno == EINTR)
	    continue;
	  perror("select");
	  return;
	}
      else if (!r)
	{
	  shutdown(p->fd, 2);
	  close(p->fd);
	  close(input_fd);
	  send_exiting(p->pipe_c2p[1]);
	  break;
	}

      struct sockaddr *sock_addr;
//#ifdef LINUX
       socklen_t length;
//#else
      //int length;
//#endif      
      sock_addr = (struct sockaddr *)&sock_in_name;
      length = sizeof(sock_in_name);
      
      if ((input_fd = accept(p->fd, sock_addr, &length)) < 0)
	{
	  perror("accept");
	  return;
	}
      
      idbWProtHeader hd;
      if (rpc_socketRead(input_fd, &hd, sizeof(hd)) != sizeof(hd))
	{
	  perror("read");
	  close(input_fd);
	  return;
	}
      
      if (hd.magic != idbWPROTMAGIC)
	{
	  close(input_fd);
	  return;
	}
      
      int argc = hd.argc;
      input_buf = (char *)malloc(hd.size);
      input_size = hd.size;
      
      if (rpc_socketReadTimeout(input_fd, input_buf, hd.size, 5) != hd.size)
	{
	  perror("read");
	  close(input_fd);
	  return;
	}

      char **argv = (char **)malloc(sizeof(char *) * (argc+8));
      int n;
      get_command_line(input_buf, argc, argv, n);

      idbWInit(input_fd);

      if (!idbWGetOpts(n, argv))
	{
	  free(argv);
	  free(input_buf);
	  if (idbWOpenDatabase(p, input_fd))
	    {
	      epilogue(input_fd);

	      close(input_fd);
	      send_ok(p->pipe_c2p[1]);
	      continue;
	    }

	  idbW_context.anchor = 1;

	  Database *db = idbW_context.db;
	  if (db)
	    {
	      Status s = db->transactionBegin();
	      if (s) s->print();
	    }

	  if (ctx->action != idbW_GETCOOKIE && ctx->action != idbW_DBDLGGEN)
	    idbWHead();

	  if (ctx->action == idbW_GETCOOKIE)
	    idbWGetCookieRealize(p, input_fd);

	  else if (ctx->action == idbW_DUMPGEN)
	    idbWDumpGenRealize(p, input_fd);

	  else if (ctx->action == idbW_DUMPNEXTGEN ||
		   ctx->action == idbW_DUMPPREVGEN)
	    idbWDumpContextRealize(p, input_fd);

	  else if (ctx->action == idbW_DBDLGGEN)
	    idbWDBDialogGenRealize(p, input_fd);

	  else if (ctx->action == idbW_SCHGEN)
	    idbWSchemaGenRealize(p, input_fd);

	  else if (ctx->action == idbW_SCHSTATS)
	    idbWSchemaStatsRealize(p, input_fd);

	  else if (ctx->action == idbW_ODLGEN)
	    idbWODLGenRealize(p, input_fd);

	  else if (ctx->action == idbW_IDLGEN)
	    idbWIDLGenRealize(p, input_fd);

	  else if (ctx->action == idbW_DUMPCONF)
	    idbWDumpConfRealize(p, input_fd);

	  else if (ctx->action == idbW_LOADCONF)
	    idbWLoadConfRealize(p, input_fd);

	  else if (ctx->action == idbW_REQDLGGEN)
	    idbWRequestDialogGenRealize(p, input_fd);

	  if (db)
	    db->transactionCommit();

	  epilogue(input_fd);
	}
      else
	{
	  free(input_buf);
	  free(argv);
	}

      close(input_fd);
      send_ok(p->pipe_c2p[1]);
    }
}

static void
find_port(int sock_fd)
{
  int n;
  for (n = 0; n < maxservers; n++)
    {
      idbWProcess *p = &proc[n];
      if (p->state == FREE)
	{
	  p->state = BUSY;
	  rpc_socketWrite(sock_fd, &p->port, sizeof(p->port));
	  return;
	}
    }

  for (int i = 0; i < 10; i++)
    {
      for (n = 0; n < maxservers; n++)
	{
	  idbWProcess *p = &proc[n];
	  if (p->state == NOTLAUNCHED)
	    {
	      launch(p, n);

	      int msg;
	      idbWREAD_C2P(p, &msg, sizeof(msg));
	      if (msg == IDBW_CHILD_READY)
		{
		  p->state = BUSY;
		  rpc_socketWrite(sock_fd, &p->port, sizeof(p->port));
		  return;
		}

	      if (msg == IDBW_CHILD_EXITING)
		{
		  p->port = 0;
		  rpc_socketWrite(sock_fd, &p->port, sizeof(p->port));
		  clear(p);
		  return;
		}

	      clear(p);
	    }
	}
    }

  int msg = 0;
  rpc_socketWrite(sock_fd, &msg, sizeof(msg));
}

static int
init_fds(int sock_fd, fd_set &fds, int max_fd)
{
  if (sock_fd > max_fd)
    max_fd = sock_fd;

  FD_SET(sock_fd, &fds);

  for (int n = 0; n < maxservers; n++)
    {
      idbWProcess *p = &proc[n];
      if (p->state != NOTLAUNCHED)
	{
	  FD_SET(p->pipe_c2p[0], &fds);
	  if (p->pipe_c2p[0] > max_fd)
	    max_fd = p->pipe_c2p[0];
	}
    }

  return max_fd;
}

static long
gen_rand()
{
  struct timeval tp;
  unsigned short x[3];
  time_t t;

  gettimeofday(&tp, NULL);
  x[0] = tp.tv_usec;
  x[1] = tp.tv_usec % 11111;
  time(&t);
  x[2] = t;

  seed48(x);

  return lrand48();
}


static const char *
gen_cookie()
{
  static char cookie[idbWCOOKIE_SIZE];
  memset(cookie, 0, sizeof(cookie));

  sprintf(cookie, "%ld", gen_rand());
  char tok[32];
  sprintf(tok, "%ld", gen_rand());
  strcat(cookie, tok);
  return cookie;
}

static void
gen_cookie(idbWProcess *p)
{
  char dbname[idbWSTR_LEN], user[idbWSTR_LEN], passwd[idbWSTR_LEN],
    confname[idbWSTR_LEN];

  idbWREAD_C2P(p, dbname, idbWSTR_LEN);

  idbWREAD_C2P(p, user,   idbWSTR_LEN);
	
  idbWREAD_C2P(p, passwd, idbWSTR_LEN);

  idbWREAD_C2P(p, confname, idbWSTR_LEN);

  Database::OpenFlag mode;
  idbWREAD_C2P(p, &mode, sizeof(mode));

  const char *cookie = gen_cookie();

  idbWDBContext::setup(cookie, dbname, user, passwd, mode, confname,
		       (Database *)0);

  idbWWRITE_P2C(p, cookie, idbWCOOKIE_SIZE);
}

static void
cookie_reopen(idbWProcess *p)
{
  char cookie[idbWCOOKIE_SIZE];
  char dbname[idbWSTR_LEN];
  int msg;

  idbWREAD_C2P(p, cookie, idbWCOOKIE_SIZE);
  idbWREAD_C2P(p, dbname, idbWSTR_LEN);

  idbWDBContext *db_ctx = idbWDBContext::lookup(cookie, dbname, NULL,
						NULL, NULL, 0);
  if (!db_ctx)
    msg = IDBW_ERROR;
  else
    msg = IDBW_OK;

  idbWWRITE_P2C(p, &msg, sizeof(msg));

  if (db_ctx)
    {
      for (int n = 0; n < maxservers; n++)
	{
	  idbWProcess *pp = &proc[n];
	  if (pp != p)
	    db_ctx->setReOpen(pp);
	}
    }
}

static void
check_reopen(idbWProcess *p)
{
  char cookie[idbWCOOKIE_SIZE];
  char dbname[idbWSTR_LEN];
  int msg;

  idbWREAD_C2P(p, cookie, idbWCOOKIE_SIZE);
  idbWREAD_C2P(p, dbname, idbWSTR_LEN);

  idbWDBContext *db_ctx = idbWDBContext::lookup(cookie, dbname, NULL,
						NULL, NULL, 0);
  if (!db_ctx)
    msg = IDBW_ERROR;
  else if (db_ctx->mustReOpen(p))
    {
      db_ctx->unsetReOpen(p);
      msg = IDBW_REOPEN;
    }
  else
    msg = IDBW_OK;

  idbWWRITE_P2C(p, &msg, sizeof(msg));
}

static void
check_cookie(idbWProcess *p)
{
  char cookie[idbWCOOKIE_SIZE];
  char dbname[idbWSTR_LEN];
  int msg;

  idbWREAD_C2P(p, cookie, idbWCOOKIE_SIZE);
  idbWREAD_C2P(p, dbname, idbWSTR_LEN);
	      
  idbWDBContext *db_ctx = idbWDBContext::lookup(cookie, dbname, NULL,
						NULL, NULL, 0);
  
  if (!db_ctx)
    msg = IDBW_ERROR;
  else
    msg = IDBW_OK;

  idbWWRITE_P2C(p, &msg, sizeof(msg));

  if (msg == IDBW_OK)
    {
      idbWWRITE_P2C(p, db_ctx->dbname, idbWSTR_LEN);
      idbWWRITE_P2C(p, db_ctx->userauth, idbWSTR_LEN);
      idbWWRITE_P2C(p, db_ctx->passwdauth, idbWSTR_LEN);
      idbWWRITE_P2C(p, db_ctx->confname, idbWSTR_LEN);
      idbWWRITE_P2C(p, &db_ctx->mode, sizeof(db_ctx->mode));
    }
}

static int
get_child_message(int fd)
{
  for (int n = 0; n < maxservers; n++)
    {
      idbWProcess *p = &proc[n];

      if (p->pipe_c2p[0] == fd && p->state != NOTLAUNCHED)
	{
	  int msg;
	  idbWREAD_C2P(p, &msg, sizeof(msg));

	  if (msg == IDBW_CHILD_READY)
	    p->state = FREE;
	  else if (msg == IDBW_CHILD_EXITING)
	    {
	      clear(p);
	      close(p->fd);
	      p->state = NOTLAUNCHED;
	      IDB_LOG(IDB_LOG_WWW, ("child pid=%d port=%d exiting\n", 
				    p->pid, p->port));
	    }
	  else if (msg == IDBW_GEN_COOKIE)
	    gen_cookie(p);
	  else if (msg == IDBW_CHECK_COOKIE)
	    check_cookie(p);
	  else if (msg == IDBW_COOKIE_REOPEN)
	    cookie_reopen(p);
	  else if (msg == IDBW_CHECK_REOPEN)
	    check_reopen(p);
	  else
	    {
	      IDB_LOG(IDB_LOG_WWW,
		      ("invalid child message=%d pid=%d port=%d exiting\n", 
		       msg, p->pid, p->port));
	      clear(p);
	      close(p->fd);
	      p->state = NOTLAUNCHED;
	      return 0;
	    }
	  return 1;
	}
    }

  return 0;
}

static void
net_main_loop(int sock_fd)
{
  fd_set fst;
  idbWProcess *p;
  int msg, n, max_fd = 0;

  FD_ZERO(&fds);

  for (;;)
    {
      max_fd = init_fds(sock_fd, fds, max_fd);
      fst = fds;

      if (select (max_fd+1, &fst, 0, 0, 0) < 0)
	{
	  perror("select");
	  continue;
	}

      for (int fd = 0; fd <= max_fd; fd++)
	if (FD_ISSET(fd, &fst))
	  {
	    if (fd == sock_fd)
	      {
		struct sockaddr *sock_addr;
//#ifdef LINUX
		socklen_t length;
//#else
		//int length;
//#endif
		int input_fd;

		sock_addr = (struct sockaddr *)&sock_in_name;
		length = sizeof(sock_in_name);

		if ((input_fd = accept(fd, sock_addr, &length)) < 0)
		  {
		    perror("accept");
		    continue;
		  }

		FD_SET(input_fd, &fds);
		if (input_fd > max_fd)
		  max_fd = input_fd;
	      }
	    else if (!get_child_message(fd))
	      {
		if (rpc_socketRead(fd, &msg, sizeof(msg)) <= 0)
		  {
		    FD_CLR(fd, &fds);
		    close(fd);
		  }
		else if (msg == IDBW_GET_PORT)
		  find_port(fd);
	      }
	  }
    }
}

static int
port_open(int port)
{
  int v;
  int sockin_fd;

  if ((sockin_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf("unable to create unix socket port `%d'\n", port);
      return -1;
    }
  
  v = 1;

  if (setsockopt(sockin_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&v, sizeof(v)) <
      0)
    {
      printf("setsockopt reuseaddr\n");
      return -1;
    }

  sock_in_name.sin_family = AF_INET;
  sock_in_name.sin_port   = htons(port);

  if (bind(sockin_fd, (struct sockaddr *)&sock_in_name,
	   sizeof(sock_in_name)) < 0 )
    {
      printf("bind (naming the socket) failed port `%d'\n",
	      port);
      perror("bind");
      exit(1);
    }

  if (sockin_fd >= 0 && listen(sockin_fd, 2) < 0 )
    {
      printf( "listen for inix socket port `%d'\n", port);
      return -1;
    }

  return sockin_fd;
}

int
realize(int argc, char *argv[])
{
  int sock_fd;
  idbWProcess *p;
  int n;

  std::string listen;
  eyedb::init(argc, argv, &listen, true);

  if (getopts(argc, argv))
    return 1;

  sv_port = atoi(listen.c_str());

  if (!sv_port)
    return 1;

  idbWInit();

  signal(SIGPIPE, SIG_IGN);

  signal(SIGSEGV, sig_h);
  signal(SIGBUS, sig_h);

  struct sigaction act;
  
  act.sa_handler = 0;
  act.sa_sigaction = 0;
  act.sa_flags = SA_SIGINFO | SA_NOCLDWAIT;
  memset(&act.sa_mask, 0, sizeof(act.sa_mask));
  sigaction(SIGCHLD, &act, 0);

  sock_fd = port_open(sv_port);

  if (!nod)
    {
      close(0);
      close(1);
      close(2);
    }

  proc = new idbWProcess[maxservers];

  for (n = 0; n < maxservers; n++)
    proc[n].state = NOTLAUNCHED;

  net_main_loop(sock_fd);

  return 0;
}

int
main(int argc, char *argv[])
{
  if (fork() == 0)
    return realize(argc, argv);

  return 0;
}
