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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "eyedblib/rpc_lib.h"
// #include "eyedblib/bstring.h"  ??? FD: a disparu

#include "eyedbcgiprot.h"

static int
usage(const char *prog)
{
  fprintf(stderr, "usage: %s <hostname> <port> <argslist>\n", prog);
  return 1;
}

/*
#ifdef LINUX
extern int gethostname(char *, int);
#else
extern "C" int gethostname(char *, int);
#endif
*/

static struct hostent *
idbWMakeHostEnt(const char *hostname)
{
  char hname[128];

  if (!strcmp(hostname, "localhost"))
    gethostname(hname, sizeof(hname)-1);
  else
    strcpy(hname, hostname);

  return gethostbyname(hname);
}

static int
idbWOpenConn(const char *hostname, int port)
{
  struct sockaddr_in sock_in_name;
  struct sockaddr *sock_addr;
  static struct hostent *hp;
  int sock_fd;

  sock_in_name.sin_family = AF_INET;
  sock_in_name.sin_port = htons(port);

  if (!hp)
    hp = idbWMakeHostEnt(hostname);

  if (!hp)
    {
      fprintf(stderr, "makehostent failed for hostname '%s'\n", hostname);
      return 0;
    }

  memcpy((char *)&sock_in_name.sin_addr, (char *)hp->h_addr, hp->h_length);

  sock_addr = (struct sockaddr *)&sock_in_name;
  int length = sizeof(sock_in_name);

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
    {
      perror("socket");
      return 0;
    }

  if (connect(sock_fd, sock_addr, length) < 0)
    {
      perror("connect");
      return 0;
    }

  return sock_fd;
}


static int
idbWGetServPort(int sock_fd)
{
  int msg = IDBW_GET_PORT;

  if (rpc_socketWrite(sock_fd, &msg, sizeof(msg)) != sizeof(msg))
    {
      fprintf(stderr, "write failed\n");
      return 0;
    }

  int reply;
  if (rpc_socketRead(sock_fd, &reply, sizeof(reply)) != sizeof(reply))
    {
      fprintf(stderr, "write failed\n");
      return 0;
    }

  return reply;
}

static void
idbWRPC(int sock_fd, int argc, char *argv[])
{
  idbWProtHeader hd;
  hd.magic = idbWPROTMAGIC;
  hd.argc = argc;
  hd.size = 0;

  int i;
  for (i = 0; i < argc; i++)
    hd.size += strlen(argv[i])+1;

  char *buf = (char *)malloc(hd.size);
  char *p = buf;
  *p = 0;
  for (i = 0; i < argc; i++)
    {
      strcat(p, argv[i]);
      p += strlen(argv[i])+1;
      *p = 0;
    }

  if (rpc_socketWrite(sock_fd, &hd, sizeof(hd)) != sizeof(hd))
    {
      perror("write 1");
      return;
    }

  if (rpc_socketWrite(sock_fd, buf, hd.size) != hd.size)
    {
      perror("write 2");
      return;
    }

  char z[1024];

  int n;

  int max_fd = sock_fd + 1;
  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(sock_fd, &fds);

  for (;;)
    {
      fd_set fst = fds;

      if (select (max_fd, &fst, 0, 0, 0) < 0)
	{
	  perror("select");
	  break;
	}

      if (FD_ISSET(sock_fd, &fst))
	{
	  if (((n = read(sock_fd, z, sizeof(z)-1))) <= 0)
	    break;

	  write(1, z, n);

	  if (z[n-1] == 0)
	    break;
	}
    }

  close(sock_fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3)
    return usage(argv[0]);

  char *hostname = argv[1];
  int port = atoi(argv[2]);

  if (!port)
    return usage(argv[0]);

  int sock_fd = idbWOpenConn(hostname, port);

  if (!sock_fd)
    return 1;

  int sv_port = idbWGetServPort(sock_fd);

  if (!sv_port)
    return 1;

  close(sock_fd);

  int fd = idbWOpenConn(hostname, sv_port);

  if (fd < 0)
    return 1;

  idbWRPC(fd, argc-3, &argv[3]);

  return 0;
}


