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

#include "eyedbcgiP.h"
#include "eyedblib/connman.h"
#include "eyedb/DBM_Database.h"
#include "eyedb/Iterator.h" // eyedb private include

static int
cmp(const void *s1, const void *s2)
{
  return strcmp(*(const char **)s1, *(const char **)s2);
}

static void
dump_cls(const char *tag)
{
  if (!tag)
    {
      idbW_dest->flush("<td colspan=1>&nbsp;</t>");
      return;
    }

  //  idbW_dest->flush("<td align=left><pre> ");
  idbW_dest->flush("<td align=left>&nbsp;");
  char href[64];
  sprintf(href, "-reqdlggen=%s", tag);
  idbWHrefTo(href, tag);
  //  idbW_dest->flush(" </pre></td>");
  idbW_dest->flush("&nbsp;</td>");
}

idbWDBContext *
idbWDBContext::genDBContext(idbWProcess *p, const char *dbname,
			    const char *user, const char *passwd,
			    Database::OpenFlag mode,
			    const char *confname, Database *db)
{
  if (!p)
    return 0;

  char cookie[idbWCOOKIE_SIZE];
  char buf[idbWSTR_LEN];

  int msg = IDBW_GEN_COOKIE;

  idbWWRITE_C2P(p, &msg, sizeof(msg));

  strcpy(buf, dbname);
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  strcpy(buf, user);
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  strcpy(buf, passwd);
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  if (confname) strcpy(buf, confname);
  else *buf = 0;
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  idbWWRITE_C2P(p, &mode, sizeof(mode));

  idbWREAD_P2C(p, cookie, idbWCOOKIE_SIZE);

  idbW_ctx->cookie = strdup(cookie);
  return idbWDBContext::setup(cookie, dbname, user, passwd, mode,
			      confname, db);
}

int
idbWOpenConnection()
{
  idbW_conn = new Connection();
  IDB_LOG(IDB_LOG_WWW, ("opening first connection\n"));
  Status status = idbW_conn->open();

  if (!status)
    return 1;

  if (idbWHead(True))
    {
      idbW_STPR(status);
      idbWTail();
      delete idbW_conn;
      idbW_conn = 0;
    }
  return 0;
}

static void
close_first_database()
{
  int tstamp = idbWDBContext::db_tstamp;
  LinkedListCursor c(&idbWDBContext::db_ctx_list);

  fflush(stdout);
  idbWDBContext *idbW_dbctx, *choose = NULL;

  while (idbWDBContext::db_ctx_list.getNextObject(&c, (void *&)idbW_dbctx))
    if (idbW_dbctx->tstamp < tstamp)
      {
	choose = idbW_dbctx;
	tstamp = idbW_dbctx->tstamp;
      }

  if (choose)
    {
      fflush(stdout);
      if (choose->db)
	{
	  choose->db->close();
	  choose->db->release();
	}
      idbWDBContext::db_ctx_list.deleteObject(choose);
    }
}

#define NEW_CONNECTION_EACH_TIME

static Status
open_database(const char *dbname, Database *&db,
	      const char *user, const char *passwd,
	      Database::OpenFlag flag, Bool dumphead = False)
{
#ifdef NEW_CONNECTION_EACH_TIME
  static Bool first = True;
#endif
  Status status = Success;
  db = 0;

  IDB_LOG(IDB_LOG_WWW, ("opening database '%s'\n", dbname));

  for (int i = 0; ; i++)
    {
#ifdef NEW_CONNECTION_EACH_TIME
      if (!first)
	{
	  idbW_conn = new Connection();
	  IDB_LOG(IDB_LOG_WWW, ("opening new connection\n"));
	  status = idbW_conn->open();
	  if (status)
	    {
	      if (dumphead)
		idbWHead();
	      idbW_STPR(status);
	      return status;
	    }
	}
      else
	first = False;
#endif
      status =
	Database::open(idbW_conn, dbname,
		       eyedb::ClientConfig::getCValue("dbm"), user, passwd, flag, 0, &db);

      if (status)
	{
#ifndef NEW_CONNECTION_EACH_TIME
	  if (i < 1 && status->getStatus() == SE_MAP_ERROR)
	    {
	      idbW_conn = new Connection();
	      IDB_LOG(IDB_LOG_WWW, ("opening new connection\n"));
	      status = idbW_conn->open();
	    }	      

	  if (status)
#endif
	    {
	      if (dumphead)
		idbWHead();
	      idbW_STPR(status);
	      return status;
	    }
	}
      else
	return Success;
    }

  return status;
}

static void
idbWReOpenTag(const char *dbname)
{
  idbW_dest->flush("<center><i><font size=+0>"
	      "If the Schema or any Web method or variable has been "
	      "modified in\n"
	      "the database '%s', you must reopen it</i></center>\n",
	      dbname);

  idbW_dest->flush("<form method=POST ACTION=\"http://%s/%s?"
	      "-db" idbWSPACE "%s" idbWSPACE
	      "-u" idbWSPACE "%s" idbWSPACE
	      "-p" idbWSPACE "%s" idbWSPACE
	      "-ck" idbWSPACE "%s" idbWSPACE
	      "-dbdlggen" idbWSPACE "-reopen\">"
	      "<center><i><font size=+0>"
	      "<input type=\"SUBMIT\" value=\"ReOpen Database\">"
	      "</i></center></form>",
	      idbW_ctx->cgidir, idbW_progname,
	      idbW_ctx->dbname,
	      idbW_ctx->user, idbW_ctx->passwd, idbW_ctx->cookie);
}

// 28/10/99 this function should be revisited
static int
idbWReOpen(const char *userauth, const char *passwdauth)
{
  int r = (open_database(idbW_dbctx->db->getName(), idbW_dbctx->db, userauth, passwdauth,
			 Database::DBSRead) ? 0 : 1);

  /*
  if (r)
    idbWSetDumpHints(idbW_dbctx->db, idbW_dbctx);
    */

  return r;
}

static int
send_reopen(idbWProcess *p, const char *cookie, const char *dbname)
{
  int msg = IDBW_COOKIE_REOPEN;
  char buf[idbWSTR_LEN];

  idbWWRITE_C2P(p, &msg, sizeof(msg));
  
  idbWWRITE_C2P(p, cookie, idbWCOOKIE_SIZE);

  strcpy(buf, dbname);
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  idbWREAD_P2C(p, &msg, sizeof(msg));
  
  if (msg == IDBW_OK)
    return 0;
  
  return 1;
}

static int
check_cookie(idbWProcess *p, const char *cookie,
	     const char *dbname, const char *userauth,
	     const char *passwdauth)
{
  int msg = IDBW_CHECK_REOPEN;
  char buf[idbWSTR_LEN];

  idbWWRITE_C2P(p, &msg, sizeof(msg));
  
  idbWWRITE_C2P(p, cookie, idbWCOOKIE_SIZE);

  strcpy(buf, dbname);
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  idbWREAD_P2C(p, &msg, sizeof(msg));
  
  if (msg == IDBW_REOPEN)
    {
      idbW_dbctx->db->close();
      if (!idbWReOpen(userauth, passwdauth))
	return 1;
      return 0;
    }

  if (msg == IDBW_TIMEOUT)
    {
      idbWPrintError("Cookie Timeout");
      return 1;
    }

  if (msg == IDBW_OK)
    return 0;
  
  return 1;
}

int
idbWDBContext::checkCookie(idbWProcess *p, const char *cookie,
			   const char *dbname, const char *userauth,
			   const char *passwdauth, const char *_confname,
			   unsigned int _mode)
{
  if (!cookie && (!userauth || !passwdauth))
    return 0;

  if (idbW_dbctx = lookup(cookie, dbname, userauth, passwdauth, _confname,
			  _mode, True))
    {
      if (!idbW_ctx->dbname)
	dbname = idbW_ctx->dbname = strdup(idbW_dbctx->db->getName());

      if (idbW_ctx->reopen)
	{
	  idbW_dbctx->db->close();
	  if (!idbWReOpen(userauth, passwdauth))
	    return 0;

	  if (!cookie || send_reopen(p, cookie, dbname))
	    return 0;
	}
      else if (check_cookie(p, idbW_dbctx->cookie, idbW_ctx->dbname,
			    userauth, passwdauth))
	return 0;

      idbW_ctx->db = idbW_dbctx->db;
      idbW_ctx->make(idbW_dbctx);
      idbW_db_config = &idbW_dbctx->config;

      free((char *)idbW_ctx->cookie);
      idbW_ctx->cookie = strdup(idbW_dbctx->cookie);

      idbWEmptyMethodCache();

      return 1;
    }

  if (!cookie)
    return 0;

  int msg = IDBW_CHECK_COOKIE;
  char buf[idbWSTR_LEN];

  idbWWRITE_C2P(p, &msg, sizeof(msg));
  
  idbWWRITE_C2P(p, cookie, idbWCOOKIE_SIZE);

  strcpy(buf, (dbname ? dbname : ""));
  idbWWRITE_C2P(p, buf, idbWSTR_LEN);

  idbWREAD_P2C(p, &msg, sizeof(msg));
  
  if (msg != IDBW_OK)
    return 0;
  
  char xdbname[idbWSTR_LEN];
  idbWREAD_P2C(p, xdbname, idbWSTR_LEN);

  char user[idbWSTR_LEN];
  idbWREAD_P2C(p, user, idbWSTR_LEN);

  char passwd[idbWSTR_LEN];
  idbWREAD_P2C(p, passwd, idbWSTR_LEN);
  
  char confname[idbWSTR_LEN];
  idbWREAD_P2C(p, confname, idbWSTR_LEN);
  
  Database::OpenFlag mode;
  idbWREAD_P2C(p, &mode, sizeof(mode));

  if (!idbW_ctx->dbname)
    dbname = idbW_ctx->dbname = strdup(xdbname);

  /*
  Database *db = new Database(dbname);

  if (open_database(db, user, passwd, mode))
    {
      db->release();
      return 0;
    }
    */

  Database *db = 0;

  if (open_database(dbname, db, user, passwd, mode))
    {
      if (db) db->release();
      return 0;
    }

  idbW_dbctx = idbWDBContext::setup(cookie, dbname, user, passwd, mode,
				    confname, db);
  idbW_ctx->make(idbW_dbctx);

  idbW_ctx->db = idbW_dbctx->db;

  idbW_db_config = &idbW_dbctx->config;
  Status s = idbWInitConf(idbW_ctx->db, confname, user);
  if (s) 
    {
      idbW_STPR(s);
      return 0;
    }

  return 1;
}

static Bool
idbWCheckUser(rpc_ConnInfo *ci, const char *user)
{
  rpc_TcpIp *tcpip = &ci->tcpip;

  Bool auth = False;

  for (int i = 0; i < tcpip->user_cnt; i++)
    {
      if (!strcmp(user, tcpip->users[i].user))
	{
	  if (tcpip->users[i].mode == rpc_User::ON ||
	      tcpip->users[i].mode == rpc_User::DEF)
	    auth = True;
	  else if (tcpip->users[i].mode == rpc_User::NOT)
	    auth = False;
	}
      else if (tcpip->users[i].mode == rpc_User::ALL)
	auth = True;
    }

  return auth;
}

static int
idbWCheckAccess(int input_fd)
{
  if (!idbW_accessfile || !idbW_ctx->raddr)
    return 0;

  if (rpc_connman_init(idbW_accessfile))
    return 1;

  struct in_addr addr;
  int b1, b2, b3, b4;
  if (sscanf(idbW_ctx->raddr, "%d.%d.%d.%d", &b1, &b2, &b3, &b4) != 4)
    return 1;

#if defined(HAVE_STRUCT_IN_ADDR_S_ADDR)
  addr.s_addr = (b1 << 24) | (b3 << 16) | (b2 << 8) | b1;
#elif defined(HAVE_STRUCT_IN_ADDR__S_UN)
  addr._S_un._S_un_b.s_b1 = b1;
  addr._S_un._S_un_b.s_b2 = b2;
  addr._S_un._S_un_b.s_b3 = b3;
  addr._S_un._S_un_b.s_b4 = b4;
#endif

  rpc_ConnInfo *ci = rpc_check_addr(&addr);

  if (!ci)
    {
      idbWHead();
      idbWPrintError((std::string("Host '") +
		     (idbW_ctx->rhost ? idbW_ctx->rhost : idbW_ctx->raddr) +
		     "' is not authorized for connection").c_str());
      return 1;
    }

  if (!idbWCheckUser(ci, idbW_ctx->user))
    {
      free(ci);
      idbWHead();
      idbWPrintError((std::string("User '") + idbW_ctx->user +
		     "' from Host '" +
		     (idbW_ctx->rhost ? idbW_ctx->rhost : idbW_ctx->raddr) +
		     "' is not authorized for connection").c_str());
      return 1;
    }

  free(ci);
  return 0;
}

static Status
idbWInitOQLEnvironment()
{
  OQL q(idbW_ctx->db,
	   "www$cookie  := \"-ck&\" + \"%s\"; "
	   "www$dbname  := \"%s\"; "
	   "www$url     := \"http://%s/%s?\"; ",
	   idbW_dbctx->cookie,
	   idbW_ctx->db->getName(),
	   idbW_ctx->cgidir, idbW_progname);

  return q.execute();
}

int
idbWOpenDatabase(idbWProcess *p, int input_fd)
{
  if (idbW_ctx->dbname && !strcmp(idbW_ctx->dbname, idbW_LIST_DATABASES))
    return 0;

  if (input_fd >= 0 && idbWCheckAccess(input_fd))
    return 1;

  if (!idbWDBContext::checkCookie(p, idbW_ctx->cookie, idbW_ctx->dbname,
				  idbW_ctx->user, idbW_ctx->passwd,
				  idbW_ctx->confname, idbW_ctx->mode))
    {
      if (idbW_ctx->cookie)
	{
	  fflush(stdout);
	  idbWHead();
	  idbWPrintError((std::string("Invalid Cookie: reconnect to "
				    "Database ") + (idbW_ctx->dbname ?
						    idbW_ctx->dbname : "")).c_str());
	  return 1;
	}

      if (!idbW_ctx->dbname)
	{
	  fflush(stdout);
	  idbWHead();
	  idbWPrintError("Unknown database to open");
	  return 1;
	}

      /*
      idbW_ctx->db = new Database(idbW_ctx->dbname);

      if (open_database(idbW_ctx->db, idbW_ctx->user, idbW_ctx->passwd,
			idbW_ctx->mode, True))
	{
	  idbW_ctx->db->release();
	  return 1;
	}
	*/

      idbW_ctx->db = 0;

      if (open_database(idbW_ctx->dbname, idbW_ctx->db, idbW_ctx->user,
			idbW_ctx->passwd, idbW_ctx->mode, True))
	{
	  if (idbW_ctx->db)
	    idbW_ctx->db->release();
	  return 1;
	}

      idbW_dbctx = idbWDBContext::genDBContext(p, idbW_ctx->dbname,
					       idbW_ctx->user,
					       idbW_ctx->passwd,
					       idbW_ctx->mode,
					       idbW_ctx->confname,
					       idbW_ctx->db);
      idbW_db_config = &idbW_dbctx->config;
      Status s = idbWInitConf(idbW_ctx->db, idbW_ctx->confname,
				 idbW_ctx->user);

      if (s) 
	{
	  idbW_STPR(s);
	  return 1;
	}
    }

  idbWInitOQLEnvironment();
  return 0;
}

static int
idbWChooseClass(Database *db)
{
  if (!idbW_db_config->mainpage_config.classes)
    return 0;

  const LinkedList *mlist = db->getSchema()->getClassList();
  LinkedListCursor c(mlist);
  Class *mcl;
    
  int n = 0;
  const char **tag = new const char *[mlist->getCount()+4];
  int maxlen = 0;

  while (mlist->getNextObject(&c, (void *&)mcl))
    if (!mcl->isSystem() && mcl->asAgregatClass() &&
	idbW_should_link_class(mcl))
      {
	tag[n++] = mcl->getName();
	int len = strlen(mcl->getName());
	if (len > maxlen)
	  maxlen = len;
      }

  if (!n)
    {
      delete [] tag;
      return 0;
    }

  qsort(tag, n, sizeof(char *), cmp);

  tag[n] = 0;
  tag[n+1] = 0;
  tag[n+2] = 0;
  tag[n+3] = 0;

  idbW_dest->flush("<h1><i>Class oriented queries using a form</i></h1>");
  /*
  idbW_dest->flush("<table border=%d bgcolor=%s><caption align=top>"
	      "<i>To perform a query with a specific form, please choose one of the following class</i></caption>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor);
  */
  idbW_dest->flush("<i>To perform a query with a specific form, please choose one of the following classes:</i><br><br>");
  idbW_dest->flush("<table border=%d bgcolor=%s>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor);

  //int n4 = (n+1)/4;
  int n4 = (n+3)/4;

  for (int i = 0; i < n4; i++)
    {
      if (!tag[i])
	break;

      idbW_dest->flush("<tr>");
      dump_cls(tag[i]);
      if (i+n4 < n)
	dump_cls(tag[i+n4]);
      if (i+2*n4 < n)
	dump_cls(tag[i+2*n4]);
      if (i+3*n4 < n)
	dump_cls(tag[i+3*n4]);
      idbW_dest->flush("</tr>\n");
    }

  idbW_dest->flush("</table>\n");
  delete [] tag;
  return 1;
}

static int
idbWOQLWindow()
{
  if (!idbW_db_config->mainpage_config.oqlform)
    return 0;

  idbW_dest->flush("<h1><i>Free query form</i></h1>");
  idbW_dest->flush("<form method=POST ACTION=\"http://%s/%s?"
	      "-ck" idbWSPACE "%s" idbWSPACE
	      "-db" idbWSPACE "%s" idbWSPACE
	      "-dumpgen\">"
	      "</font></font><font size=+0>",
	      idbW_ctx->cgidir, idbW_progname, idbW_dbctx->cookie, idbW_ctx->dbname);

  /*
  idbW_dest->flush("<table border=%d bgcolor=%s><caption align=top>"
	      "<i>Write a valid OQL query construct and submit it</i></caption>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor);
  */

  idbW_dest->flush("<i>Write a valid OQL query construct and submit it:</i><br><br>");
  idbW_dest->flush("<table border=%d bgcolor=%s>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor);

  idbW_dest->flush("<tr><th align=left>");
  idbW_dest->flush("<textarea rows=%d cols=%d name=\"\"></textarea>",
		   idbW_db_config->oqlform_config.rows,
		   idbW_db_config->oqlform_config.cols);

  idbW_dest->flush("</th></tr></table>\n");

  idbW_dest->flush("<br><center><input type=\"SUBMIT\" value=\"SEARCH\"></center>"
	      "</form>\n\n");

  return 1;
}  

static int
idbWClickSchema(Database *db)
{
  if (!idbW_db_config->mainpage_config.schema)
    return 0;

  idbW_dest->flush("<h1><i>Schema display</i></h1>");

  /*
  idbW_dest->flush("<table border=%d bgcolor=%s><caption align=top>"
	      "<i>To display the database schema</i></caption>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor );
  */
  idbW_dest->flush("<i>To display the database schema</i>:<br><br>");
  idbW_dest->flush("<table border=%d bgcolor=%s>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor );

  idbW_dest->flush("<tr><td>&nbsp;");


  idbWHrefTo("-schgen", (std::string(db->getName()) + " schema").c_str());
  idbW_dest->flush("</td></tr>\n");

  if (idbW_db_config->mainpage_config.stats)
    {
      idbW_dest->flush("<tr><td>&nbsp;");
      idbWHrefTo("-schstats", (std::string(db->getName()) + " schema statistics").c_str());
      idbW_dest->flush("</td></tr>\n");
    }

  if (idbW_db_config->mainpage_config.odl)
    {
      idbW_dest->flush("<tr><td>&nbsp;");
      idbWHrefTo("-odlgen", (std::string(db->getName()) + " ODL dump").c_str());
      idbW_dest->flush("</td></tr>\n");
    }

  /*
  if (idbW_db_config->mainpage_config.idl)
    {
      idbW_dest->flush("<tr><th><pre>  ");
      idbWHrefTo("-idlgen", (std::string(db->getName()) + " default IDL dump").c_str());
      idbW_dest->flush("   </pre></th></tr>");
    }
  */

  idbW_dest->flush("</table>\n");

  return 1;
}

static int
idbWDumpConfig(Database *db)
{
  if (!idbW_db_config->mainpage_config.webconf)
    return 0;

  idbW_dest->flush("<h1><i>WEB Configuration</i></h1>");

  /*
  idbW_dest->flush("<table border=%d bgcolor=%s><caption align=top>"
	      "<i>To dump the WEB configuration</i></caption>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor );
  */

  idbW_dest->flush("<i>To dump the WEB configuration:</i><br><br>");
  idbW_dest->flush("<table border=%d bgcolor=%s>",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor );

  /*
  idbW_dest->flush("<tr><th align=left><pre>   ");
  idbWHrefTo("-loadconf", (std::string("reload ") +
	     db->getName() + " WEB configuration").c_str());
  idbW_dest->flush("   </pre></th>");
  */

  idbW_dest->flush("<tr><th align=left>&nbsp;");
  idbWHrefTo("-dumpconf", (std::string("dump ") +
	     db->getName() + " WEB configuration").c_str());

  idbW_dest->flush("&nbsp;</th></tr></table>\n");

  return 1;
}

static int
sort_realize(const void *xdb1, const void *xdb2)
{
  return strcmp((*(DBEntry **)xdb1)->dbname().c_str(),
		(*(DBEntry **)xdb2)->dbname().c_str());
}

static inline int
check_access(DBAccessMode mode, DBAccessMode umode)
{
  return (mode & umode) == umode;
}

DBEntry **
make_dbentries(ObjectArray &obj_array)
{
  DBEntry **dbentries = new DBEntry*[obj_array.getCount()];
  for (int i = 0; i < obj_array.getCount(); i++)
    dbentries[i] = (DBEntry *)obj_array[i];
  return dbentries;
}

static DBM_Database *dbm;

static int
idbWOpenDBM()
{
  dbm = new DBM_Database(eyedb::ClientConfig::getCValue("dbm"));
  Status status = 
    dbm->open(idbW_conn, Database::DBSRead, idbW_ctx->user, idbW_ctx->passwd);
  if (status)
    {
      status->print();
      idbW_STPR(status);
      return 1;
    }

  /*
  if (open_database(dbm, idbW_ctx->user, idbW_ctx->passwd, Database::DBRead))
    {
      dbm->release();
      dbm = 0;
      return 1;
    }

    */
  return 0;
}

char *
idbWGetDBName(int dbid)
{
  if (!dbm && idbWOpenDBM())
    return 0;

  dbm->transactionBegin();
  OQL q(dbm, "(select DBEntry.dbid = %d).dbname", dbid);
  ValueArray val_arr;
  Status s = q.execute(val_arr);
  if (s)
    {
      dbm->transactionAbort();
      idbW_STPR(s);
      return 0;
    }

  dbm->transactionAbort();
  return val_arr.getCount() ? strdup(val_arr[0].str) : 0;
}

static int
idbWDBList(idbWProcess *p, int fd)
{
  static Bool href = False;

  if (!dbm && idbWOpenDBM())
    return 1;

  dbm->transactionBegin();

  UserEntry *user;
  Status s;

  s = dbm->getUser(idbW_ctx->user, user);
  if (s)
    {
      idbW_STPR(s);
      dbm->transactionCommit();
      return 1;
    }

  if (!user)
    {
      idbWPrintError((std::string("unknown user '") + idbW_ctx->user + "'").c_str());
      return 1;
    }

  user->release();

  idbW_dest->flush("<font size=+2>");
  Iterator q(dbm->getSchema()->getClass("database_entry"));

  idbW_dest->flush("<font size=+0>");

  if (q.getStatus())
    {
      idbW_STPR(q.getStatus());
      dbm->transactionCommit();
      return 1;
    }

  ObjectArray obj_array;
  s = q.scan(obj_array);
  if (s)
    {
      idbW_STPR(s);
      dbm->transactionCommit();
      return 1;
    }

  idbW_dest->flush("<br><center><table border=%d>",
		   idbW_db_config->look_config.border);

  idbW_dest->flush("<tr><th bgcolor=%s><table border=0>",
		   idbW_db_config->look_config.tablebgcolor);

  SysUserAccess *sysaccess = 0;
  dbm->get_sys_user_access(idbW_ctx->user, &sysaccess, True, "");
  int count = obj_array.getCount();

  Bool super = IDBBOOL(sysaccess &&
			  (sysaccess->mode() == AdminSysAccessMode ||
			   (sysaccess->mode() == SuperUserSysAccessMode)));
  
  if (sysaccess) sysaccess->release();
  DBAccessMode mode = (idbW_ctx->mode == Database::DBSRead ||
		       idbW_ctx->mode == Database::DBSReadAdmin) ?
    ReadDBAccessMode : ReadWriteDBAccessMode;

  idbW_dest->flush("<tr><th><i>Database</i></th>"
		   "<th><i>Description</i></th></tr>\n");
  DBEntry **dbentries = make_dbentries(obj_array);
  qsort(dbentries, count, sizeof(DBEntry *), sort_realize);

  int cnt = 0;
  for (int j = 0; j < count; j++)
    {
      DBEntry *dbentry = dbentries[j];
      DBUserAccess *dbaccess = 0;
      DBAccessMode defaccess;
      UserEntry *user = 0;
      dbm->get_db_user_access(dbentry->dbname().c_str(), idbW_ctx->user, &user,
			      &dbaccess, &defaccess);
			
      if (user) user->release();

      if (!super && !check_access(dbentry->default_access(), mode)
	  && !(dbaccess && check_access(dbaccess->mode(), mode)))
	{
	  if (dbaccess) dbaccess->release();
	  continue;
	}

      if (dbaccess) dbaccess->release();

      cnt++;
      idbW_dest->cflush("<tr><th align=left><strong>");
      if (href)
	{
	  idbW_dest->cflush("<a href=\"http://%s/%s?"
			    "-db" idbWSPACE "%s" idbWSPACE
			    "-u" idbWSPACE "%s" idbWSPACE
			    "-p" idbWSPACE "%s" idbWSPACE,
			    idbW_ctx->cgidir, idbW_progname,
			    dbentry->dbname().c_str(),
			    idbW_ctx->user, idbW_ctx->passwd);
	  
	  idbW_dest->cflush
	    ("-mode" idbWSPACE "%s" idbWSPACE,
	     (idbW_ctx->mode == Database::DBRead ? "read" :
	      (idbW_ctx->mode == Database::DBSRead ? "sread" :
	       (idbW_ctx->mode == Database::DBRW ? "rw" :
		(idbW_ctx->mode == Database::DBReadAdmin ? "readadmin" :
		 (idbW_ctx->mode == Database::DBSReadAdmin ? "sreadadmin" :
		  (idbW_ctx->mode == Database::DBRWAdmin ? "rwadmin" : 0)))))));

	  if (idbW_ctx->confname)
	    idbW_dest->cflush
	      ("-conf" idbWSPACE "%s" idbWSPACE, idbW_ctx->confname);
	  
	  idbW_dest->cflush("-dbdlggen\">%s</a>", dbentry->dbname().c_str());
	}
      else
	idbW_dest->cflush(dbentry->dbname().c_str());

      idbW_dest->cflush("</strong></th><th>");
      
      if (dbentry->comment().c_str() && *dbentry->comment().c_str())
	idbW_dest->cflush("%s", dbentry->comment().c_str());
      else
	idbW_dest->cflush("-");
      
      idbW_dest->cflush("</th></tr>\n");
    }

  delete[] dbentries;

  if (!cnt)
    idbW_dest->flush("<tr><th>"
		     "Sorry, no databases accessible</th>"
		     "<th>in %s mode</th></tr>\n",
		     Database::getStringFlag(idbW_ctx->mode));

  obj_array.garbage();
  idbW_dest->flush("</th></tr></table>");
  idbW_dest->flush("</table>\n");
  if (cnt)
    idbW_dest->flush("<br><br>"
		     "To connect to one of those databases, please come back "
		     "to the <a href=\"%s\">connection window</a>",
		     ((idbW_ctx->referer ?
		       std::string(idbW_ctx->referer) :
		       std::string("http://") + idbW_ctx->als +
		       "/index.html")).c_str());
  dbm->transactionCommit();

  return 0;
}

static void
idbWDBHead()
{
  const char *x = idbW_db_config->mainpage_config.head;
  if (x && *x) 
    {
      idbW_dest->flush(x);
      return;
    }

  idbW_dest->flush("Welcome to the <big>%s</big> database main page. "
		   "Using the E<small>YE</small>DB browser, you can perform "
		   "query operations, browsing and you can display database "
		   "statistics.<br>", idbW_ctx->db->getName());
}

int
idbWDBDialogGenRealize(idbWProcess *p, int fd)
{
  if (!strcmp(idbW_ctx->dbname, idbW_LIST_DATABASES))
    {
      idbWHead();
      return idbWDBList(p, fd);
    }

  Status s = idbWInitConf(idbW_ctx->db, idbW_ctx->confname, idbW_ctx->user);

  if (s) 
    {
      idbW_STPR(s);
      return 1;
    }

  if (*idbW_db_config->mainpage_config.url)
    {
      idbW_dest->flush("location: %s\n\n", idbW_db_config->mainpage_config.url);
      return 0;
    }

  idbWHead();

  idbW_CHECK(idbW_ctx->db->transactionBegin());

  idbWDBHead();
  idbW_dest->flush("<br><br>");

  if (idbWChooseClass(idbW_ctx->db))
    idbW_dest->flush("<br><br>");

  if (idbWClickSchema(idbW_ctx->db))
    idbW_dest->flush("<br><br>");

  if (idbWOQLWindow())
    idbW_dest->flush("<br><br>");

  if (idbWDumpConfig(idbW_ctx->db))
    idbW_dest->flush("<br><br>");

  //idbWReOpenTag(idbW_ctx->db->getName());

  idbW_CHECK(idbW_ctx->db->transactionCommit());
  return 0;
}

int
idbWGetCookieRealize(idbWProcess *p, int fd)
{
  idbW_dest->flush("Content-type: text/html\r\n\r\n");
  
  idbW_dest->flush(idbW_ctx->cookie);

  return 0;
}

idbWDBContext *idbWDBContext::setup(const char *cookie,
				    const char *dbname,
				    const char *userauth,
				    const char *passwdauth,
				    Database::OpenFlag mode,
				    const char *confname,
				    Database *db)
{
  idbWDBContext *idbW_dbctx =
    new idbWDBContext(cookie, dbname, userauth, passwdauth, mode, confname, db);

  db_ctx_list.insertObjectLast(idbW_dbctx);
  
  return idbW_dbctx;
}

