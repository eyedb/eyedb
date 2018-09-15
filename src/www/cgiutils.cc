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

static Bool headDisplayed;
static char idbW_tmppre[] = "eyedbIM";

idbWProcess *idbW_root_proc;

Bool
idbWIsURL(idbWDest *dest, const char *s);

static const char *
helpFromAction()
{
  if (idbW_ctx->action == idbW_DBDLGGEN &&
      ((idbW_ctx->dbname && strcmp(idbW_ctx->dbname, idbW_LIST_DATABASES)) || !idbW_ctx->dbname))
    return idbW_HELP_PREFIX "dbdlggen" idbW_HELP_SUFFIX;

  //  if (idbW_ctx->action == idbW_DUMPGEN)
  //    return idbW_HELP_PREFIX "dumpgen" idbW_HELP_SUFFIX;

  if (idbW_ctx->action == idbW_REQDLGGEN)
    return idbW_HELP_PREFIX "reqdlggen" idbW_HELP_SUFFIX;

  return "";
}

char *
idbWFilterArg(const char *tag)
{
  int len = strlen(tag), l;
  char *dst = (char *)malloc(len+1), *q = dst;
  char c;

  for (l = 0; *tag; l++)
    {
      char c = *tag++;
      if (c == '+')
	*dst++ = ' ';
      else if (c == '%' && len > l-2)
	{
	  char s[3];
	  s[0] = *tag++;
	  s[1] = *tag++;
	  s[2] = 0;
	  int cn;
	  sscanf(s, "%X", &cn);
	  if (cn == '\r')
	    cn = '\n';
	  *dst++ = cn;
	}
      else
	*dst++ = c;
    }

  *dst = 0;
  return q;
}

int
idbWHead(Bool nodb)
{
  if (!idbW_dest)
    return 0;

  if (idbW_ctx->nohtml || headDisplayed)
    return 1;

  idbW_dest->flush("Content-type: text/html\r\n\r\n");

  idbW_dest->flush("<html>");

  if (!nodb && idbW_ctx->dbname)
    idbW_dest->flush("<title>%s database</title>", idbW_ctx->dbname);

  const char *help = helpFromAction();
  idbW_dest->flush(idbW_db_config->look_config.head_str, help);
    
  idbW_dest->flush("\n\n\n");
  if (idbW_ctx->dbname && !strcmp(idbW_ctx->dbname, idbW_LIST_DATABASES))
    idbW_dest->flush("<font size=+4><center><strong><i>"
		     "Databases accessible to the user %s</i>"
		     "</strong></center></font><br>", idbW_ctx->user);
  else if (!nodb && idbW_ctx->dbname)
    idbW_dest->flush("<font size=+3><center><strong><i>"
		     "%s database%s</i></strong></center></font><br>",
		     idbW_ctx->dbname,
		     !strcmp(idbW_ctx->dbname, idbW_LIST_DATABASES) ? "s" : "");

  idbW_dest->flush("<FONT SIZE=+0>\n");
  headDisplayed = True;
  return 1;
}

int
idbWTail()
{
  if (!idbW_dest)
    return 0;

  if (idbW_ctx->nohtml || !headDisplayed)
    return 1;

  idbW_dest->flush("</font22></pre>\n");
  idbW_dest->flush(idbW_db_config->look_config.tail_str);
  
  idbW_dest->flush("</html>\n");
  headDisplayed = False;

  return 1;
}

void
idbWHrefTo(const char *opt, const char *label)
{
  idbW_dest->flush("<strong><em><a href=\"http://%s/%s?"
	      "-ck"       idbWSPACE "%s" idbWSPACE
	      "%s"        idbWSPACE
	      "-db"       idbWSPACE "%s" idbWSPACE
	      "\">%s</a></em></strong>",
	      idbW_ctx->cgidir, idbW_progname,
	      idbW_ctx->cookie,
	      opt,
	      idbW_ctx->dbname,
	      label);
}

void
idbWBackTo()
{
  idbW_ctx->restore();

  if (!idbW_ctx->cookie)
    return;

  idbW_dest->flush("<strong><a href=\"http://%s/%s?"
		   "-ck"       idbWSPACE "%s" idbWSPACE
		   "-dbdlggen" idbWSPACE
		   "-db"       idbWSPACE "%s" idbWSPACE
		   "\">%s database main page</a>"
		   "</strong>",
		   idbW_ctx->cgidir, idbW_progname,
		   idbW_ctx->cookie,
		   idbW_ctx->dbname, idbW_ctx->dbname);
}

//
// Multi-Database Facitilies
//

int
idbWOpenCrossDB(const char *dbname)
{
  idbW_ctx->push(dbname);

  if (!idbWDBContext::checkCookie(idbW_root_proc, 0,
				  dbname, idbW_dbctx->userauth,
				  idbW_dbctx->passwdauth, 0, 0))
    {
      if (idbWOpenDatabase(idbW_root_proc, -1))
	return 1;
    }

  idbW_ctx->db->transactionBegin(); // when stopping transaction???
  return 0;
}

int
idbWHrefDB(const char *dbname)
{
  if (idbWOpenCrossDB(dbname))
    return 1;

  idbW_dest->cflush("<small><em><a href=\"http://%s/%s?"
		    "-dbdlggen"  idbWSPACE
		    "-db"       idbWSPACE "%s" idbWSPACE
		    "-ck" idbWSPACE "%s" "\">"
		    "[%s database]</a></em></small> ",
		    idbW_ctx->cgidir, idbW_progname,
		    dbname, idbW_ctx->cookie, dbname);
  return 0;
}

void
idbWPrintError(const char *s)
{
  if (!headDisplayed)
    return;

  idbW_dest->flush("<br><br><br><br><br><strong>");
  idbW_dest->flush("<center><table border=%d cellspacing=5 cellpadding=5><tr><th bgcolor=%s>"
	      "<pre>\n\n",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.errorbgcolor);
  idbW_dest->flush("<i>E<small>YE</small>DB Web Browser Error</i>\n");
  idbWFlushText(idbW_dest, s, False, False, False);
  idbW_dest->flush("  \n\n</th></tr></table></center></strong><br><br><br><br>\n\n");
}

void
idbWHrefRedisplay(const Oid *oid_array, int oid_cnt)
{
  if (!oid_cnt)
    return;

  idbW_dest->flush("<strong><a href=\"http://%s/%s?"
	      "-ck"       idbWSPACE "%s" idbWSPACE
	      "-db"       idbWSPACE "%s" idbWSPACE,
	      idbW_ctx->cgidir, idbW_progname,
	      idbW_ctx->cookie,
	      idbW_ctx->dbname);

  if (idbW_ctx->verbmod == idbWVerboseRestricted)
    idbW_dest->cflush("-verbfull" idbWSPACE);

  idbW_dest->cflush("-dumpgen"  idbWSPACE);

  for (int i = 0; i < oid_cnt; i++)
    idbW_dest->cflush("%s" idbWSPACE, oid_array[i].toString());

  idbWDumpContext dump_ctx;
  Bool dummy;
  idbW_dest->cflush((idbW_ctx->exp_ctx.build(dump_ctx, dummy)).c_str());

  idbW_dest->cflush("\"><em>%s</em></a></strong>",
	       (idbW_ctx->verbmod == idbWVerboseRestricted ? 
		"Full Display" : "Restricted Display"));
}

void
idbWAddTo(Oid * &oid_array, int &oid_cnt, int &oid_alloc, const Oid &oid)
{
  if (oid_cnt >= oid_alloc)
    {
      oid_alloc = oid_cnt + 8;
      oid_array = (Oid *)realloc(oid_array, oid_alloc * sizeof(Oid));
    }

  oid_array[oid_cnt++] = oid;
}

static void
percent_manage(char *s)
{
  char tok[1024];
  char c, *q, *buf = s;
  int percent = 0;

  q = tok;

  while (c = *s)
    {
      if (c == '%')
	{
	  *q++ = '%';
	  percent = 1;
	}
      *q++ = *s++;
    }

  *q = 0;

  if (percent)
    memcpy(buf, tok, strlen(tok)+1);
}

void
idbWFlushText(idbWDest *dest, const char *s, Bool fixed, Bool isnull,
	       Bool put_sep)
{
  const char *sep = (put_sep ? idbW_STRING_SEP : "");

  if (isnull)
    {
      idbW_dest->cflush(idbW_null);
      return;
    }

  if (!s || !*s)
    {
      idbW_dest->cflush("%s%s", sep, sep);
      return;
    }


  Bool isurl = idbWIsURL(dest, s);

  if (isurl)
    sep = "";

  int len = strlen(s) + 1;

  // changed the 14/10/99
  if (1)   // was if (isurl || len < idbW_FL_LEN)

    {
      idbW_dest->cflush(sep);
      idbW_dest->cflush(s, True);
      idbW_dest->cflush(sep);

      if (isurl)
	idbW_dest->cflush("</a>", True);

      return;
    }

  const char *q = s;
  char c;

  while (c = *q++)
    if (c == '\n')
      {
	idbW_dest->cflush("%s%s%s", sep, s, sep);
	return;
      }

  int l = len;
  idbW_dest->push();
  char buf[1024];
  int inc;

#define INC 72
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

  int sep_len = strlen(sep);
  for (int j = 0; j < len; j += inc, l -= inc)
    {
      int inc0 = INC - (j ? 0 : sep_len);
      idbW_NL();
      
      if (fixed)
	inc = INC;
      else
	{
	  int k = j + MIN(inc0, l);
	  char *sp = (char *)strchr(&s[k], ' '), *tab = (char *)strchr(&s[k], '\t');
	  char *p;
	  if (!sp && !tab)
	    inc = inc0;
	  else
	    {
	      if (sp && tab)
		p = (char *)MIN((long)sp, (long)tab);
	      else if (sp)
		p = sp;
	      else
		p = tab;

	      inc = (long)p - (long)&s[j] + 1;
	    }
	}

      strncpy(buf, &s[j], MIN(inc, l));
      buf[inc] = 0;
      percent_manage(buf);

      if (!j)
	idbW_dest->flush("%s%s", sep, buf);
      else
	idbW_dest->flush(buf);
    }

  idbW_dest->cflush(sep);
  if (isurl)
    idbW_dest->cflush("</a>");
  idbW_dest->pop();
}

static struct {
  char *prefix;
  int len;
  Bool img;
} URLProtocol[] = {
  {(char*)"http:",    strlen("http:"),   True}, /*@@@@warning cast*/
  {(char*)"file:",    strlen("file:"),   False}, /*@@@@warning cast*/
  {(char*)"ftp:",     strlen("ftp:"),    False}, /*@@@@warning cast*/
  {(char*)"mailto:",  strlen("mailto:"), False}, /*@@@@warning cast*/
  {(char*)"gopher:",  strlen("gopher:"), False}, /*@@@@warning cast*/
  {(char*)"wais:",    strlen("wais:"),   False} /*@@@@warning cast*/
};

Bool
idbWIsURL(idbWDest *dest, const char *s)
{
  static int len = sizeof(URLProtocol)/sizeof(URLProtocol[0]);

  for (int i = 0; i < len; i++)
    if (!strncmp(s, URLProtocol[i].prefix, URLProtocol[i].len))
      {
	int l = strlen(s);
	if (URLProtocol[i].img &&
	    (!strcmp(&s[l-4], ".gif") || !strcmp(&s[l-4], ".jpg")))
	  idbW_dest->cflush("<image src=\"%s\">", s);

	idbW_dest->cflush("<a href=\"%s\">", s);
	return True;
      }

  return False;
}


Bool
idbWIsString(const Attribute *item)
{
  if (strcmp(item->getClass()->getName(), "char"))
    return False;

  const TypeModifier &tmod = item->getTypeModifier();

  if (tmod.ndims != 1)
    return False;

  if (!item->isVarDim() && tmod.pdims < 2)
    return False;

  return True;
}

char *
idbWGetString(const Attribute *item, Object *o, Bool *isnull)
{
  char *s;

  idbW_status = item->getValue(o, (Data *)&s, Attribute::directAccess, 0,
			       isnull);

  if (idbW_status)
    return 0;

  if (isnull && *isnull)
    idbW_nullCount++;
  return s;
}

unsigned int
idbWGetSize(const Attribute *item, Object *o)
{
  TypeModifier tmod = item->getTypeModifier();

  if (item->isVarDim())
    {
      Size size;
      item->getSize(o, size);
      return (int)size;
    }

  return tmod.pdims;
}

Bool
idbWIsint16(const Attribute *item)
{
  return item->getClass()->asInt16Class() != 0 ? True : False;
}

Bool
idbWIsint32(const Attribute *item)
{
  return item->getClass()->asInt32Class() != 0 ? True : False;
}

Bool
idbWIsint64(const Attribute *item)
{
  return item->getClass()->asInt64Class() != 0 ? True : False;
}

eyedblib::int16
idbWGetInt16(const Attribute *item, Object *o, int from, Bool *isnull)
{
  eyedblib::int16 val;
  Status status = item->getValue(o, (Data *)&val, 1, from, isnull);
  if (isnull && *isnull)
    idbW_nullCount++;
  return val;
}

eyedblib::int32
idbWGetInt32(const Attribute *item, Object *o, int from, Bool *isnull)
{
  eyedblib::int32 val;
  Status status = item->getValue(o, (Data *)&val, 1, from, isnull);
  if (isnull && *isnull)
    idbW_nullCount++;
  return val;
}

eyedblib::int64
idbWGetInt64(const Attribute *item, Object *o, int from, Bool *isnull)
{
  eyedblib::int64 val;
  Status status = item->getValue(o, (Data *)&val, 1, from, isnull);
  if (isnull && *isnull)
    idbW_nullCount++;
  return val;
}

Bool
idbWIsChar(const Attribute *item)
{
  const char *mname = item->getClass()->getName();
  if (!strcmp(mname, "char") || !strcmp(mname, "byte"))
    return True;

  return False;
}

char
idbWGetChar(const Attribute *item, Object *o, int from,
	     Bool *isnull)
{
  char val;
  Status status = item->getValue(o, (Data *)&val, 1, from, isnull);
  if (isnull && *isnull)
    idbW_nullCount++;
  return val;
}

Bool
idbWIsFloat(const Attribute *item)
{
  const char *mname = item->getClass()->getName();
  if (!strcmp(mname, "float") || !strcmp(mname, "double"))
    return True;

  return False;
}

double
idbWGetFloat(const Attribute *item, Object *agr, int from,
	      Bool *isnull)
{
  double fval;
  Status status = item->getValue(agr, (Data *)&fval, 1, from, isnull);
  if (isnull && *isnull)
    idbW_nullCount++;
  return fval;
}

//
// empty functions
//

static Bool
isEmptyItemBasic(const Attribute *item, Object *o, int from,
		 const char * &item_name)
{
  Bool isnull = False;

  if (idbWIsint16(item))
    (void)idbWGetInt16(item, (Agregat *)o, from, &isnull);
  else if (idbWIsint32(item))
    (void)idbWGetInt32(item, (Agregat *)o, from, &isnull);
  else if (idbWIsint64(item))
    (void)idbWGetInt64(item, (Agregat *)o, from, &isnull);
  else if (idbWIsFloat(item))
    (void)idbWGetFloat(item, (Agregat *)o, from, &isnull);
  else if (idbWIsChar(item))
    (void)idbWGetChar(item, (Agregat *)o, from, &isnull);

  if (isnull)
    idbW_nullCount++;
  return isnull;
}

static Bool
isEmptyItem(const Attribute *item, Object *o, const char * &item_name)
{
  const Class *cls = item->getClass();
  Status status;

  item_name = item->getName();

  if (idbWIsString(item))
    {
      Bool isnull;
      (void)idbWGetString(item, (Agregat *)o, &isnull);
      return isnull;
    }

  int dims = idbWGetSize(item, o);

  for (int i = 0; i < dims; i++)
    {
      if (item->isIndirect())
	{
	  Oid oid;
	  if (status = item->getOid((Agregat *)o, &oid, 1, i))
	    {
	      idbW_STPR(status);
	      return False;
	    }

	  if (oid.isValid())
	    return False;
	}
      else if (cls->asBasicClass())
	{
	  if (!isEmptyItemBasic(item, o, i, item_name))
	    return False;
	}
      else if (cls->asAgregatClass())
	{
	  Object *oo;
	  if (status = item->getValue((Agregat *)o, (Data *)&oo, 1, i))
	    {
	      idbW_STPR(status);
	      return False;
	    }

	  if (oo && !idbWIsEmpty(oo, item_name))
	    return False;
	}
      else if (cls->asEnumClass())
	{
	  int val;
	  Bool isnull;
	  if (status = item->getValue(o, (Data *)&val, 1, i, &isnull))
	    {
	      idbW_STPR(status);
	      return False;
	    }

	  if (!isnull && cls->asEnumClass()->getEnumItemFromVal(val))
	    return False;
	}
    }

  idbW_nullCount++;
  return True;
}

Bool
idbWIsEmpty(Object *o, const char * &item_name)
{
  const Class *cls = o->getClass();

  if (cls->asAgregatClass())
    {
      unsigned int items_cnt;
      const Attribute **items =
	((const AgregatClass *)cls)->getAttributes(items_cnt);


      for (int i = 0; i < items_cnt; i++)
	if (!items[i]->isNative() && !isEmptyItem(items[i], o, item_name))
	  return False;
    }
  else if (cls->asCollectionClass())
    return ((Collection *)o)->getCount() ? False : True;
  else if (cls->asEnumClass())
    {
      unsigned int v = 0;
      ((Enum *)o)->getValue(&v);
      item_name = "enum";
      return (v ? False : True);
    }

  idbW_nullCount++;
  return True;
}

const char *
idbW_get_tmp_file(const char *extension, int s)
{
  static char *tmpfile;
  static int pid = getpid();
  char *imgdir = idbW_ctx->imgdir;

  if (!tmpfile)
    tmpfile = (char *)malloc(strlen(imgdir)+strlen(idbW_tmppre)+30);

  sprintf(tmpfile, "%s/%s.%d.%d",
	  imgdir, idbW_tmppre, pid, (s < 0 ? idbW_start_tmp : s));

  strcat(tmpfile, extension);
  if (s < 0)
    idbW_start_tmp++;
  return tmpfile;
}
