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

#define GET_ACTION(X) do { \
 if (idbW_ctx->action != idbW_NOACTION) return usage(); \
 idbW_ctx->action = (X); \
} while(0)

#define GET_ARG(X) do { \
 if (i == argc-1 || (idbW_ctx->X)) return usage(); \
 idbW_ctx->X = idbWFilterArg(argv[++i]); \
} while(0)


#define GET_CLASS() do { \
 if (idbW_ctx->_class) return usage(); \
 GET_ARG(_class); \
} while(0)

#define GET_OPT(S, OPT, VAR) \
if (!strcmp(S, OPT)) GET_ARG(VAR)

#define MAKE_CTX(S) do { \
if (idbW_ctx->etc_cnt >= idbW_ctx->etc_alloc) \
 { \
   idbW_ctx->etc_alloc = idbW_ctx->etc_cnt + 12; \
   idbW_ctx->etc = (char **)realloc(idbW_ctx->etc, sizeof(char *) * idbW_ctx->etc_alloc); \
 } \
idbW_ctx->etc[idbW_ctx->etc_cnt++] = idbWFilterArg(S); \
} while(0)

static char *
concat(char *x, const char *p, Bool space = True)
{
  if (!x)
    return strdup(p);

  char *q = (char *)malloc(strlen(x)+strlen(p)+2);

  strcpy(q, x);
  if (space)
    strcat(q, " ");
  strcat(q, p);

  free(x);

  return q;
}

static int
usage(const char *opt = 0)
{
  idbWHead(True);

  idbWPrintError((std::string("Invalid command: '") + idbW_cmd + "'").c_str());

  idbWTail();

  IDB_LOG(IDB_LOG_WWW, ("Invalid command: '%s'\n", idbW_cmd.c_str()));
  if (opt)
    IDB_LOG(IDB_LOG_WWW, ("Unknown option: '%s'\n", opt));

  return 1;

  fprintf(stderr,
	  "usage: eyedbcgi\n"
	  "          <envopts>] [<prefopts>] -dbdlggen -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -schgen -db <dbname>\n"
	  "          <envopts> [<prefopts>] -schstats -db <dbname>\n"
	  "          <envopts> [<prefopts>] -odlgen -db <dbname>\n"
	  //"          <envopts> [<prefopts>] -idlgen -db <dbname>\n"
	  "          <envopts> [<prefopts>] -loadconf -db <dbname>\n"
	  "          <envopts> [<prefopts>] -dumconf -db <dbname>\n"
	  "          <envopts> [<prefopts>] -reqdlggen <mclname> -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -reqcpdlggen <mclname> -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -dumpgen <oql/oid list> -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -xdumpgen <oql/oid list> -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -dumpnextgen <collctx> -db <dbname> [-local]\n"
	  "          <envopts> [<prefopts>] -dumpprevgen <collctx> -db <dbname> [-local]\n"
	  "\n"
	  " Where <envopts> is:\n"
	  "   -dir   <directory>  [-nohtml]\n"
	  "   -alias <http-alias> [-nohtml]\n"
	  "\n"
	  " Where <prefopts> is:\n"
	  "   -bg <color>\n"
	  "   -head <htmlfile>\n"
	  "   -tail <htmlfile>\n"
	  "   -value-color <color>\n"
	  "   -name-color <color>\n"
	  "   -value-font <font>\n"
	  "   -name-font <font>\n"
	  "\n");
  return 1;
}

int
idbWGetOpts(int argc, char *argv[])
{
  idbWResetOptions();

  for (int i = 1; i < argc; i++)
    {
      //char *s = idbWFilterArg(argv[i]);
      char *s = argv[i];

      if (*s == '-')
	{
	  /* action options */
	  if (!strcmp(s, "-dbdlggen"))
	    GET_ACTION(idbW_DBDLGGEN);
	  else if (!strcmp(s, "-schgen"))
	    GET_ACTION(idbW_SCHGEN);
	  else if (!strcmp(s, "-schstats"))
	    GET_ACTION(idbW_SCHSTATS);
	  else if (!strcmp(s, "-loadconf"))
	    GET_ACTION(idbW_LOADCONF);
	  else if (!strcmp(s, "-dumpconf"))
	    GET_ACTION(idbW_DUMPCONF);
	  /*
	  else if (!strcmp(s, "-idlgen"))
	    GET_ACTION(idbW_IDLGEN);
	  */
	  else if (!strcmp(s, "-odlgen"))
	    GET_ACTION(idbW_ODLGEN);
	  else if (!strcmp(s, "-reqdlggen"))
	    {
	      GET_ACTION(idbW_REQDLGGEN);
	      GET_CLASS();
	    }
	  else if (!strcmp(s, "-reqcpdlggen"))
	    {
	      GET_ACTION(idbW_REQDLGGEN);
	      idbW_ctx->cp_query = True;
	      GET_CLASS();
	    }
	  else if (!strcmp(s, "-reqcpdlggen"))
	    {
	      GET_ACTION(idbW_REQCPDLGGEN);
	      GET_CLASS();
	    }
	  else if (!strcmp(s, "-dumpgen"))
	    GET_ACTION(idbW_DUMPGEN);
	  else if (!strcmp(s, "-xdumpgen"))
	    {
	      GET_ACTION(idbW_DUMPGEN);
	      char *x = 0;
	      Bool getrid = False;
	      int state = 0;
	      int j;
	      Bool select_done = False;
	      Bool par_open = False;
	      Bool op = False;
	      for (j = i+1; j < argc; j++)
		{
		  char *p = argv[j];
		  if (*p == '-')
		    break;

		  if (getrid)
		    continue;

		  if (!strcmp(p, "$"))
		    {
		      getrid = True;
		      continue;
		    }

		  if (op)
		    {
		      if (!strcmp(p, "in"))
			state = 0;
		      op = False;
		    }
		      
		  if (!strncmp(p, idbW_FIELD_STR, idbW_FIELD_STR_LEN))
		    {
		      state = 1;
		      x = concat(x, p+idbW_FIELD_STR_LEN);
		    }
		  else if (!strncmp(p, idbW_SELECT, idbW_SELECT_LEN))
		    {
		      x = concat(x, p+idbW_SELECT_LEN);
		      if (par_open)
			x = concat(x, " ( ");
		      x = concat(x, "x.");
		      select_done;
		    }
		  else if (!strcmp(p, idbW_PAR_OPEN))
		    {
		      if (select_done)
			x = concat(x, "(");
		      else
			par_open = True;
		    }
		  else if (!strcmp(p, idbW_PAR_CLOSE))
		    x = concat(x, ")");
		  else if (strcmp(p, idbW_OPERAND) && strcmp(p, idbW_FIELD) &&
			   strcmp(p, idbW_LOGICAL))
		    {
		      if (state == 1)
			{
			  x = concat(x, p);
			  state = 2;
			}
		      else if (state == 2)
			{
			  if (strncmp(p, idbW_QUOTE, idbW_QUOTE_LEN) &&
			      strcasecmp(p, "null"))
			    {
			      x = concat(x, idbW_QUOTE);
			      x = concat(x, p, False);
			      x = concat(x, idbW_QUOTE, False);
			    }
			  else
			    x = concat(x, p);

			  state = 0;
			}
		      else
			x = concat(x, p);
		    }
		  else if (!strcmp(p, idbW_OPERAND))
		    op = True;
		}

	      if (!x)
		return usage();

	      MAKE_CTX(x);
	      i = j-1;
	    }
	  else if (!strcmp(s, "-dumpnextgen"))
	    {
	      GET_ACTION(idbW_DUMPNEXTGEN);
	      GET_ARG(set_oid_str);
	      GET_ARG(mcl_oid_str);
	      GET_ARG(item_name);
	      GET_ARG(start_str);
	      GET_ARG(totaldumpedcount_str);
	    }
	  else if (!strcmp(s, "-dumpprevgen"))
	    {
	      GET_ACTION(idbW_DUMPPREVGEN);
	      GET_ARG(set_oid_str);
	      GET_ARG(mcl_oid_str);
	      GET_ARG(item_name);
	      GET_ARG(start_str);
	      GET_ARG(totaldumpedcount_str);
	    }
	  else if (!strcmp(s, "-u"))
	    {
	      if (i >= argc - 1)
		return usage();
	      idbW_ctx->user = idbWFilterArg(argv[++i]);

	      if (!strcmp(idbW_ctx->user, "-p"))
		idbW_ctx->user = 0;
	    }
	  else if (!strcmp(s, "-p"))
	    {
	      if (i >= argc - 1)
		return usage();
	      idbW_ctx->passwd = idbWFilterArg(argv[++i]);
	    }
	  else if (!strcmp(s, "-reopen"))
	    idbW_ctx->reopen = True;
	  else if (!strcmp(s, "-get-cookie"))
	    GET_ACTION(idbW_GETCOOKIE);
	  else if (!strcmp(s, "-expand"))
	    {
	      // option used with dumpgen
	      i++;
	      if (!idbW_ctx->exp_ctx.init(i, argc, argv))
		return usage();
	    }
	  else if (!strcmp(s, "-verbfull"))
	    idbW_ctx->verbmod = idbWVerboseFull;
	  /* environment option */
	  else if (!strcmp(s, "-local"))
	    idbW_ctx->local = _DBOpenLocal;
	  else if (!strcmp(s, "-nohtml"))
	    idbW_ctx->nohtml = True;
	  else GET_OPT(s, "-ck", cookie);
	  else GET_OPT(s, "-cgidir", cgibasedir);
	  else GET_OPT(s, "-imgdir", imgdir);
	  else GET_OPT(s, "-rootdir", rootdir);
	  else if (!strcmp(s, "-cgi"))
	    {
	      if (i >= argc - 1)
		return usage();
	      free(idbW_progname);
	      idbW_progname = idbWFilterArg(argv[++i]);
	    }
	  else if (!strcmp(s, "-pad"))
	    ++i;
	  else GET_OPT(s, "-raddr", raddr);
	  else GET_OPT(s, "-rhost", rhost);
	  else if (!strcmp(s, "-conf"))
	    {
	      if (i < argc - 1 && *argv[i+1] != '-')
		idbW_ctx->confname = idbWFilterArg(argv[++i]);
	    }
	  else if (!strcmp(s, "-mode"))
	    {
	      if (i >= argc - 1)
		return usage();
	      char *mode = argv[++i];
	      if (!strcmp(mode, "read"))
		idbW_ctx->mode = Database::DBRead;
	      else if (!strcmp(mode, "sread"))
		idbW_ctx->mode = Database::DBSRead;
	      else if (!strcmp(mode, "rw"))
		idbW_ctx->mode = Database::DBRW;
	      else if (!strcmp(mode, "readadmin"))
		idbW_ctx->mode = Database::DBReadAdmin;
	      else if (!strcmp(mode, "sreadadmin"))
		idbW_ctx->mode = Database::DBSReadAdmin;
	      else if (!strcmp(mode, "rwadmin"))
		idbW_ctx->mode = Database::DBRWAdmin;
	      else
		return usage();
	    }
	  else if (!strcmp(s, "-db"))
	    {
	      if (idbW_ctx->dbname)
		return usage();
	      if (i == argc - 1 || *argv[i+1] == '-')
		idbW_ctx->dbname = strdup(idbW_LIST_DATABASES);
	      else
		idbW_ctx->dbname = idbWFilterArg(argv[++i]);
	    }
          else if (!strcmp(s, "-alias"))
	    {
	      GET_ARG(als);
	      if (!idbW_ctx->cgibasedir && idbW_cgidir)
		idbW_ctx->cgibasedir = strdup(idbW_cgidir);
	      free(idbW_ctx->cgidir);
	      idbW_ctx->cgidir = (char *)
		malloc(strlen(idbW_ctx->als) +
		       strlen(idbW_ctx->cgibasedir) + 2);

	      strcpy(idbW_ctx->cgidir, idbW_ctx->als);
	      strcat(idbW_ctx->cgidir, "/");
	      strcat(idbW_ctx->cgidir, idbW_ctx->cgibasedir);
	    }
	  else GET_OPT(s, "-referer", referer);
	  else GET_OPT(s, "-wdir", dir);
	  else
	    return usage(s);
	}
      else
	MAKE_CTX(s);
    }

  if (idbW_ctx->action == idbW_NOACTION)
    return usage();

  /*
  if (!idbW_ctx->dbname)
    return usage();
    */

  if (!idbW_ctx->als && idbW_alias)
    idbW_ctx->als = strdup(idbW_alias);

  if (idbW_ctx->action == idbW_DUMPGEN && !idbW_ctx->etc_cnt)
    return usage();

  if (!idbW_ctx->nohtml && !idbW_ctx->cgidir)
    return usage();

  if (!idbW_ctx->user)
    idbW_ctx->user = strdup("guest");

  if (!idbW_ctx->passwd)
    idbW_ctx->passwd = strdup("guest");

  if (idbW_ctx->cookie)
    idbW_ctx->cookie = idbW_ctx->cookie;

  if (idbW_ctx->dbname && !strcmp(idbW_ctx->dbname, "-u"))
    {
      idbW_ctx->dbname = NULL;
      return usage();
    }

  return 0;
}
