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

// global variables
Connection *idbW_conn;
idbWContext idbW_context;
char *idbW_progname;
std::string idbW_cmd;
LinkedList idbWDBContext::db_ctx_list;
int idbWDBContext::db_tstamp = 2000;
idbWGenConfig *idbW_db_config;;
int idbW_tstamp = 1000;
idbWDBContext *idbW_dbctx;
idbWContext *idbW_ctx = &idbW_context;
idbWDest *idbW_dest;
int idbW_nullCount;
Status idbW_status;
const char *idbW_accessfile;
const char *idbW_cgidir, *idbW_alias, *idbW_wdir;

// macro declarations

static void
remove_tmp_files()
{
  for (int i = idbW_ostart_tmp; i < idbW_start_tmp; i++)
    {
      unlink((std::string(idbW_ctx->rootdir) + "/" + idbW_get_tmp_file(".jpg", i)).c_str());
      unlink((std::string(idbW_ctx->rootdir) + "/" + idbW_get_tmp_file(".gif", i)).c_str());
    }
}

idbWContext::idbWContext()
{
  action = (idbWAction)0;
  glb_textmod = (idbWTextMode)0;
  nohtml = False;
  etc_cnt = 0;
  etc_alloc = 0;
  etc = 0;
  cp_query = False;
  db = 0;
  user = 0;
  passwd = 0;
  cookie = 0;
  raddr = 0;
  rhost = 0;
  cgidir = 0;
  dir = 0;
  referer = 0;
  als = 0;
  _class = 0;
  dbname = 0;
  confname = 0;
  cgibasedir = 0;
  rootdir = 0;
  imgdir = 0;
  verbmod = (idbWVerboseMode)0;
  db_ctx = 0;
  reopen = False;
  mode = (Database::OpenFlag)0;
  local = 0;
  set_oid_str = 0;
  mcl_oid_str = 0;
  item_name = 0;
  start_str = 0;
  totaldumpedcount_str = 0;
  orig.dbname = 0;
  orig.cookie = 0;
}

void
idbWResetOptions()
{
  int i;
  for (i = 0; i < idbW_ctx->etc_cnt; i++)
    free(idbW_ctx->etc[i]);

  free(idbW_ctx->etc);
  free(idbW_ctx->user);
  free(idbW_ctx->passwd);
  free(idbW_ctx->cookie);
  free(idbW_ctx->raddr);
  free(idbW_ctx->rhost);
  free(idbW_ctx->cgidir);
  free(idbW_ctx->dir);
  free(idbW_ctx->referer);
  free(idbW_ctx->als);
  free(idbW_ctx->_class);
  free(idbW_ctx->dbname);
  free(idbW_ctx->confname);
  free(idbW_ctx->cgibasedir);
  free(idbW_ctx->set_oid_str);
  free(idbW_ctx->mcl_oid_str);
  free(idbW_ctx->item_name);
  free(idbW_ctx->start_str);
  free(idbW_ctx->totaldumpedcount_str);
  free(idbW_ctx->orig.dbname);
  free(idbW_ctx->orig.cookie);

  free(idbW_progname);
  idbW_progname = strdup("eyedbcgi.sh");

  idbW_ctx->exp_ctx.release();
  idbW_ctx->dump_ctx.path.release();

  remove_tmp_files();

  idbW_ctx->release();
  memset(idbW_ctx, 0, sizeof(*idbW_ctx));

  idbW_ostart_tmp = idbW_start_tmp;
  idbW_nullCount = 0;
  idbW_ctx->cp_query = True; // added the 9/10/99
  idbW_ctx->reopen = False;
  idbW_ctx->verbmod = idbWVerboseRestricted;
  idbW_ctx->action = idbW_NOACTION;
  idbW_ctx->glb_textmod = idbWTextHTML;
  idbW_ctx->nohtml = False;
  idbW_ctx->mode = Database::DBSRead;

  if (idbW_alias)
    {
      idbW_ctx->cgidir = (char *)
	malloc(strlen(idbW_alias) +
	       strlen(idbW_cgidir) + 2);

      strcpy(idbW_ctx->cgidir, idbW_alias);
      strcat(idbW_ctx->cgidir, "/");
      strcat(idbW_ctx->cgidir, idbW_cgidir);
    }
}

void
idbWGarbage()
{
  idbWResetOptions();
}

void
idbWInit(int fd)
{
  idbW_tstamp++;

  delete idbW_dest;
  idbW_dest = new idbWDest(fd, idbW_db_config->display_config.indent);
}

void
idbWInit()
{
  static int init;

  if (init) return;

  init = 1;
  idbWInitConf();
  idbWMethodInit();
}
