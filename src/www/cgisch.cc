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

int
idbWSchemaGenRealize(idbWProcess *p, int fd)
{
  free(idbW_ctx->etc);
  idbW_ctx->etc = (char **)malloc(sizeof(char *));
  idbW_ctx->etc[idbW_ctx->etc_cnt++] = strdup("select schema");
  return idbWDumpGenRealize(p, fd);
}

static void
idbWRowClass(const Class *cls, Bool th)
{
  if (th)
    idbW_dest->flush("<td>");
  if (cls && !cls->isSystem())
    {
      int len;
      char *tag = idbWMakeTag(cls->getOid(), cls->getName(), idbWTextHTML,
			      len, True, True);
      idbW_dest->flush("&nbsp;");
      idbW_dest->flush(tag);
    }
  else
    idbW_dest->flush("<center>-</center>");

  if (th)
    idbW_dest->flush("</td>");
}

static void
idbWSchemaStatsHead()
{
  idbW_dest->flush("<tr>");
  idbW_dest->flush("<th><strong><em>Class</em></strong></th>\n");
  idbW_dest->flush("<th><strong><em>SuperClass</em></strong></th>\n");
  idbW_dest->flush("<th><strong><em>SubClasses</em></strong></th>\n");
  idbW_dest->flush("<th><strong><em>Strict Instance Number</em></strong></th>\n");
  idbW_dest->flush("<th><strong><em>Instance Number</em></strong></th>\n");
  idbW_dest->flush("<th><strong><em>Description</em></strong></th>\n");

  idbW_dest->flush("</tr>\n");
}

static int
cmp(const void *cl1, const void *cl2)
{
  return strcmp((*(Class **)cl1)->getName(),
		(*(Class **)cl2)->getName());
}

int
idbWSchemaStatsRealize(idbWProcess *p, int fd)
{
  idbW_dest->flush("<center><table align=center border=%d bgcolor=%s>"
		   "<caption align=top><i>Schema Statistics</i>"
		   "</caption>\n",
		   idbW_db_config->look_config.border,
		   idbW_db_config->look_config.tablebgcolor);
  
  idbWSchemaStatsHead();

  Class *objclass = idbW_ctx->db->getSchema()->getClass("object");
  Class **objclasses;
  unsigned int objclass_cnt;
  Status s = objclass->getSubClasses(objclasses, objclass_cnt);
  if (s) s->print();

  qsort(objclasses, objclass_cnt, sizeof(Class *), cmp);

  int i;
  int total = 0;
  for (i = 0; i < objclass_cnt; i++)
    {
      Class *cls = objclasses[i];
      if (cls->isSystem() || !cls->asAgregatClass())
	continue;

      idbW_dest->flush("<tr>");
      idbWRowClass(cls, True);
      idbWRowClass(cls->getParent(), True);

      Class **classes;
      unsigned int cnt;
      cls->getSubClasses(classes, cnt);
      idbW_dest->flush("<td>");
      int nn = 0;
      for (int n = 0; n < cnt; n++)
	if (classes[n] != cls)
	  {
	    if (n) idbW_dest->cflush("<br>");
	    idbWRowClass(classes[n], False);
	    nn++;
	  }

      if (!nn)
	idbW_dest->flush("<center>-</center>");

      idbW_dest->flush("</td>");

      Collection *extent;
      cls->getExtent(extent, True);
      int count = extent->getCount();
      total += count;
      idbW_dest->flush("<th>%d</th>", count);
      idbW_dest->flush("<th>%d</th>", idbW_get_instance_number(cls, True));
      const char *desc = idbW_get_description(cls);
      idbW_dest->flush("<td>%s</td>", (desc && *desc ? desc : "<center>-</center>"));
      idbW_dest->flush("</tr>\n");
    }

  idbW_dest->flush("<tr><th>Total</th><th>&nbsp;</th><th>&nbsp</th>"
		   "<th>%d</th><th>&nbsp</th><th>&nbsp</th></tr>", total);
  idbW_dest->flush("</table>");
  idbW_dest->flush("<br><br>\n\n");
  idbWBackTo();
  return 0;
}

static void
display_result(const char *file, const std::string &cmd, int r)
{
  /*
  if (r)
    {
      idbWPrintError((std::string("an error ocurred while executing "
			       "the shell command: '") + cmd + "'").c_str());
      return;
    }
  */

  int fd = open(file, O_RDONLY);
  if (fd < 0)
    return;

  idbW_dest->flush("<pre>");
  char buf[1024];
  int n;
  while (n = read(fd, buf, sizeof(buf)-1))
    {
      buf[n] = 0;
      idbW_dest->flush(buf);
    }
  idbW_dest->flush("</pre>");
  close(fd);
}

int
idbWODLGenRealize(idbWProcess *p, int fd)
{
  char *tmpfile = tmpnam(0);
  std::string cmd = std::string("eyedbodl -gencode ODL -package \"") +
    idbW_ctx->db->getSchema()->getName() +
    "\" -d " + idbW_ctx->db->getName() + " -o " + tmpfile +
    " -eyedbuser " + idbW_ctx->user +
    " -eyedbpasswd " + idbW_ctx->passwd;

  display_result(tmpfile, cmd, system(cmd.c_str()));
  unlink(tmpfile);
  return 1;
}

int
idbWIDLGenRealize(idbWProcess *p, int fd)
{
  char *tmpfile = tmpnam(0);
  std::string cmd = std::string("eyedbodl -gencode IDL -package \"") +
    idbW_ctx->db->getSchema()->getName() +
    "\" -db " + idbW_ctx->db->getName() +
    " -idl-module " +  idbW_ctx->db->getName() + " -o " + tmpfile +
    " -eyedbuser " + idbW_ctx->user +
    " -eyedbpasswd " + idbW_ctx->passwd;

  display_result(tmpfile, cmd, system(cmd.c_str()));
  unlink(tmpfile);
  return 1;
}

