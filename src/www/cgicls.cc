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

static char *
idbWConcatField(const char *s1, const char *s2, const char *s3 = "")
{
  if (!*s1 && !*s3)
    return strdup(s2);

  char *s = (char *)malloc(strlen(s1)+strlen(s2)+strlen(s3)+2);
  if (*s1)
    sprintf(s, "%s.%s%s", s1, s2, s3);
  else
    sprintf(s, "%s%s", s2, s3);

  return s;
}

static void
make(const char *x, const char *prefix, const Attribute *item)
{
  char *nprefix = idbWConcatField(prefix, x);
  if (!item->isIndirect() &&
      !strcmp(item->getClass()->getName(), "char") &&
      item->getTypeModifier().ndims > 0)
    idbW_dest->flush("<option value=\"%s%s\">%s</option>\n",
		idbW_FIELD_STR, nprefix, nprefix);
  else
    idbW_dest->flush("<option value=\"%s\">%s</option>\n", nprefix, nprefix);
}

static void
set_items(const char *s, const char *prefix, const void *xitem)
{
  const Attribute *item = (const Attribute *)xitem;
  char *p = strdup(s);

  char *x;
  for (x = p; ; )
    {
      char *q;
      char *q1 = strchr(x, ' ');
      char *q2 = strchr(x, '\t');
      
      if (q1 && q2)
	q = (q1 < q2 ? q1 : q2);
      else if (q1)
	q = q1;
      else if (q2)
	q = q2;
      else
	break;

      *q = 0;
      
      make(x, prefix, item);
      q++;
      while (*q == ' ' || *q == '\t')
	q++;
      
      if (!*q)
	break;
      
      x = q;
    }
  
  make(x, prefix, item);
  free(p);
}

#if 0
static int
idbWMakeVariable(const idbClass *cls, const char *prefix,
		  const char *mclname, const char *varname,
		  const char *itemname, void (*action)(const char *,
						       const char *,
						       const void *))
{
  const LinkedList *vlist = cls->getCompList(idbClass::idbVarLIST);

  if (!vlist)
    return 0;

  LinkedListCursor c(vlist);
  idbClass *mclvar = idbW_ctx->db->getSchema()->getClass(mclname);

  if (!mclvar)
    return 0;

  idbClassVariable *v;

  while (vlist->getNextObject(&c, (void *&)v))
    {
      idbClass *cls_owner;
      idbObject *val;
      if (!strcmp(v->getVname(), varname) &&
	  (val = v->getVal()) &&
	  (cls_owner = v->getClassOwner()) &&
	  cls_owner->compare(cls) &&
	  val->getClass()->compare(mclvar))
	{
	  const Attribute *item = mclvar->getAttribute(itemname);
	  if (item)
	    {
	      char *s;
	      item->getValue(val, (Data *)&s, Attribute::directAccess,
			     0);
	      if (s)
		action(s, prefix, item);

	      return 1;
	    }
	}
    }
  return 0;
}
#endif

static std::string
idbW_get_typestr(const Attribute *item)
{
  std::string s = "<small><i>";
  if (item->isIndirect())
    s += "oid";
  else if (item->isString())
    s += "string";
  else
    s += item->getClass()->getName();
  return s + "</i></small>    ";
}

static Bool
is_sub_class_of(const Class *cls1, const Class *cls2)
{
  Bool is;
  cls1->isSubClassOf(cls2, &is);
  return is;
}

static void
idbWMakeOptionItemRealize(const Class *cls, 
			  const idbWItemList *root_list,
			  AttrIdxContext &idx_ctx,
			  const char *prefix, int depth, int maxdepth,
			  Bool polymorph);

#define NEWIDX2
static int
has_index(const Attribute *item, AttrIdxContext &idx_ctx)
{
#ifndef NEWIDX2
  idbIndex *idx;
  int r = !item->getIndex((Database *)item->getClass()->getDatabase(),
			  idx_ctx.getString(), idx) && idx;
  printf("attrpath '%s' [%d] -> %d\n", (const char *)idx_ctx.getString(), idx_ctx.getAttrCount(), r);
  return r;
#else
  return 1;
#endif
}

static void
idbWMakeOptionItemRealizeOne(const Class *rootcls, const Class *cls,
			     const idbWItemList *root_list,
			     AttrIdxContext &idx_ctx,
			     const char *prefix, int depth, int maxdepth,
			     Bool polymorph)
{
  unsigned int items_cnt;
  const Attribute **items;

  const idbWItemList *item_list = idbW_get_item_list(cls);

  items = cls->getAttributes(items_cnt);

  for (int i = 0; i < items_cnt; i++)
    {
      const Attribute *item = items[i];
      if (item->isNative() || (depth >= maxdepth && item->isIndirect()) ||
	  (polymorph && !item->getClassOwner()->compare(cls) &&
	   cls->getAttribute(item->getName())))
	continue;

      std::string item_name = (rootcls == cls ? std::string(item->getName()) :
			     std::string(cls->getName()) + "::" + item->getName());
      if (item->getClass()->asCollectionClass())
	{
	  idx_ctx.push(item_name.c_str());
	  /*
	  printf("idx_ctx %p -> %s -> %s [%d]\n", &idx_ctx, (const char *)item_name,
		 (const char *)idx_ctx.getString(), idx_ctx.getAttrCount());
	  */
#if 0
	  if (item->isIndirect() || !item->getIndexMode() || depth > 0)
	    continue;
#else
	  if (item->isIndirect() || !has_index(item, idx_ctx) || depth > 0) {
	    idx_ctx.pop();
	    continue;
	  }
#endif

	  const Class *cls = item->getClass()->asCollectionClass()->
	    getCollClass();

	  if (!cls->asAgregatClass()) {
	    idx_ctx.pop();
	    continue;
	  }

	  char *nprefix = idbWConcatField(prefix, item_name.c_str(), "[?]");
	  idbWMakeOptionItemRealize(cls, root_list, idx_ctx, nprefix, depth+1,
				    maxdepth, polymorph);
	  idx_ctx.pop();
	  continue;
	}

      char *nprefix = idbWConcatField(prefix, item_name.c_str(),
				      (item->getClass()->asAgregatClass()
				       && !item->isIndirect() &&
				       item->getTypeModifier().ndims > 0) ?
				      "[?]" : "");

      idx_ctx.push(item_name.c_str());
      /*
      printf("#2 idx_ctx %p -> %s -> %s [%d]\n", &idx_ctx, (const char *)item_name,
	     (const char *)idx_ctx.getString(), idx_ctx.getAttrCount());
      */
      idbWTBool root_on = idbW_is_in_item_list(root_list, idx_ctx);
      idbWTBool cls_on  = idbW_is_in_item_list(item_list, item_name.c_str());

      /*
      printf("for item %s %s -> root_on=%d, cls_on=%d\n",
	     (const char *)item_name, (const char *)idx_ctx.getAttrName(True));
      */

      if (!(root_on == idbWTFalse ||
	    (root_on == idbWTUndef && 
	     (cls_on == idbWTFalse ||
	      (cls_on == idbWTUndef && root_list->item_cnt)))) &&
	  (!item->getClass()->asAgregatClass() || item->isIndirect()))
	{
	  std::string s;
	  if (idbW_db_config->clsquery_config.attribute_type)
	    s += idbW_get_typestr(item);
	  s += nprefix;
	  s += " ";

#ifndef NEWIDX2
	  if (has_index(item, idx_ctx))
	    s += std::string("  ") +
	      idbW_db_config->clsquery_config.attribute_indexed_string;
#endif
	  
	  if (item->isString())
	    idbW_dest->flush("<option value=\"%s%s\">%s</option>\n",
			     idbW_FIELD_STR, nprefix, s.c_str());
	  else
	    idbW_dest->flush("<option value=\"%s\">%s</option>", nprefix,
			     s.c_str());
	  
	}

      if (item->getClass()->asAgregatClass())
	idbWMakeOptionItemRealize(item->getClass(), root_list, idx_ctx,
				  nprefix, depth+1, maxdepth, polymorph);

      free(nprefix);
      idx_ctx.pop();
    }
}

static void
idbWMakeOptionItemRealize(const Class *cls, 
			  const idbWItemList *root_list,
			  AttrIdxContext &idx_ctx,
			  const char *prefix, int depth, int maxdepth,
			  Bool polymorph)
{
  if (polymorph)
    {
      Class **subclasses;
      unsigned int subclass_cnt;

      cls->getSubClasses(subclasses, subclass_cnt);

      for (int j = 0; j < subclass_cnt; j++)
	idbWMakeOptionItemRealizeOne(cls, subclasses[j], root_list, idx_ctx,
				     prefix, depth, maxdepth, polymorph);
      return;
    }

  idbWMakeOptionItemRealizeOne(cls, cls, root_list, idx_ctx, prefix, depth,
			       maxdepth, polymorph);
}

static void
idbWMakeOptionItem(const Class *cls)
{
  AttrIdxContext idx_ctx(cls);
  /*
  printf("WMakeOptionItem(%p, %s, %s)\n", &idx_ctx, 
	 (cls ? cls->getName() : "<nil>"),
	 (const char *)idx_ctx.getString());
  */
  idbWMakeOptionItemRealize(cls, idbW_get_item_list(cls), idx_ctx, "", 0,
			    idbW_get_clsquery_form_depth(cls),
			    idbW_is_clsquery_form_polymorph(cls));
}

static int
idbWClassHead(const Class *cls)
{ 
  const char *x = idbW_get_clsquery_header(cls);
  if (x && *x)
    {
      idbW_dest->flush(x);
      return 1;
    }

  return 0;
}

static void
idbWDumpRequestDialogHeaderDump(idbWProcess *p, int fd, int nn)
{
  idbW_dest->flush("<tr>");

  if (nn > 1)
    {
      idbW_dest->flush("<th>");
      idbW_dest->flush("(");
      idbW_dest->flush("</th>");
    }

  idbW_dest->flush("<th>");
  idbW_dest->flush("<i>Attribute</i>");
  idbW_dest->flush("</th>");

  idbW_dest->flush("<th>");
  idbW_dest->flush("<i>Operator</i>");
  idbW_dest->flush("</th>");

  idbW_dest->flush("<th>");
  idbW_dest->flush("<i>Value</i>");
  idbW_dest->flush("</th>");

  if (nn > 1)
    {
      idbW_dest->flush("<th>");
      idbW_dest->flush(")");
      idbW_dest->flush("</th>");

      idbW_dest->flush("<th><i>");
      idbW_dest->flush("and | or |END");
      idbW_dest->flush("</i></th>");
    }

  idbW_dest->flush("</tr>\n");
}

const char idbWExtent[] = "_____extent_____";

static void
make_operand()
{
  idbW_dest->flush("<th>");
  idbW_dest->flush("<select name=\"" idbW_OPERAND "\">");

  idbW_OPTION_SELECTED("=", "=");
  idbW_OPTION("!=");
  idbW_OPTION("<");
  idbW_OPTION(">");
  idbW_OPTION("<=");
  idbW_OPTION(">=");
  idbW_OPTION("~");
  idbW_OPTION("~~");
  idbW_OPTION("!~");
  idbW_OPTION("!~~");
  idbW_OPTION("in");
  idbW_OPTION("between");
  idbW_OPTION("not between");
      
  idbW_dest->flush("</select>");
      
  idbW_dest->flush("</th>");
}

static void
make_par_open()
{
  idbW_dest->flush("<th>");
  //  idbW_dest->flush("<select name=\"%s\">", idbW_PAR);
  idbW_dest->flush("<select name=\"\">");
  idbW_OPTION("  ");
  idbW_OPTION_X(idbW_PAR_OPEN, "(");
  idbW_dest->flush("</select>");
  idbW_dest->flush("</th>");
}

static void
make_items(const Class *cls, int i)
{
  idbW_dest->flush("<th>");
  if (!i)
    idbW_dest->flush("<select name=\"%sselect x from %s x where \">",
		     idbW_SELECT, cls->getName());
  else
    idbW_dest->flush("<select name=\" x.\">", cls->getName());
  idbWMakeOptionItem(cls);
  idbW_dest->flush("</select>");
  idbW_dest->flush("</th>");
}

static void
make_value()
{
  idbW_dest->flush("<th>");
  idbW_dest->flush("<input type=\"text\" name=\"" idbW_FIELD "\" size=16>");
  idbW_dest->flush("</th>");
}

static void
make_par_close()
{
  idbW_dest->flush("<th>");
  //  idbW_dest->flush("<select name=\"%s\">", idbW_PAR);
  idbW_dest->flush("<select name=\"\">");
  idbW_OPTION("  ");
  idbW_OPTION_X(idbW_PAR_CLOSE, ")");
  idbW_dest->flush("</select>");
  idbW_dest->flush("</th>");
}

static void
make_logical()
{
  idbW_dest->flush("<th>");
  idbW_dest->flush("<select name=\"" idbW_LOGICAL "\">");
  idbW_dest->flush("<b>");
  idbW_OPTION_SELECTED(idbW_ETC, "END");
  idbW_OPTION_X("and", "and");
  idbW_OPTION_X("or", "or");
  idbW_dest->flush("</b>");
  idbW_dest->flush("</select>");
  idbW_dest->flush("</th>");
}

static void
make_end()
{
  idbW_dest->flush("<th>");
  idbW_dest->flush("<select name=\"" idbW_LOGICAL "\">");
  idbW_dest->flush("<b>");
  //  idbW_OPTION_X("stop", "END");
  idbW_OPTION_X("", "END");
  idbW_dest->flush("</b>");
  idbW_dest->flush("</select>");
  idbW_dest->flush("</th>");
}

static void
make_desc(const Class *cls)
{
  if (!idbW_should_dump_description(cls))
    return;

  idbW_dest->flush("<center>");
  idbW_dest->flush("<table border=%d> <caption align=top>"
		   "<i>%s Description</i></caption>",
		   idbW_db_config->look_config.border,
		   cls->getName());

  idbW_dest->flush("<tr>");
  idbW_dest->flush("<th align=left bgcolor=%s>",
		   idbW_db_config->look_config.tablebgcolor);
  idbW_dest->flush("<pre><br>");
  idbWDumpAgregatClass(idbW_dest, (Class *)cls, idbWTextHTML, idbWHeaderYes,
		       True);
  idbW_dest->flush("</pre>");
  idbW_dest->flush("</th>");
  idbW_dest->flush("</tr>\n");
  idbW_dest->flush("</table><br><br>\n\n");
  idbW_dest->flush("</center>");
}

Collection *
get_first_extent(const Class *cls)
{
  Class **subclasses;
  unsigned int subclass_cnt = 0;
  Status s = cls->getSubClasses(subclasses, subclass_cnt);
  if (s) s->print();

  Collection *extent;
  for (int i = 0; i < subclass_cnt; i++)
    if (subclasses[i] != cls)
      return get_first_extent(subclasses[i]);

  cls->getExtent(extent);
  return extent;
}

static int
make_extent(const Class *cls)
{
  if (!idbW_should_instances_link(cls))
    return idbW_get_instance_number(cls, True);
;
  return idbWMakeContinueContext(idbW_dest, "next",
				 cls, idbWExtent,
				 get_first_extent(cls)->getOid(), 0,
				 0, idbWTextHTML, False);
}

int
idbWCheckMultiDB(idbWProcess *p, const Oid &oid, Bool href)
{
  if (oid.getDbid() == idbW_ctx->db->getDbid())
    return 1;

  char *dbname = idbWGetDBName(oid.getDbid());

  if (!dbname)
    return 0;

  int r = href ? !idbWHrefDB(dbname) : !idbWOpenCrossDB(dbname);
  free(dbname);
  return r;
}

int
idbWRequestDialogGenRealize(idbWProcess *p, int fd)
{
  Oid clsoid(idbW_ctx->_class);
  idbW_CHECK(idbW_ctx->db->transactionBegin());
  const Class *cls;

  if (clsoid.isValid())
    {
      if (!idbWCheckMultiDB(p, clsoid, False))
	return 0;
      cls = idbW_ctx->db->getSchema()->getClass(clsoid);
    }
  else
    cls = idbW_ctx->db->getSchema()->getClass(idbW_ctx->_class);

  if (!cls)
    return 0;

  if (idbWClassHead(cls))
    idbW_dest->flush("<br><br>");

  make_desc(cls);

  int cnt = make_extent(cls);

  idbW_dest->flush("<br>");
  int nn = idbW_get_clsquery_form_count(cls);
  if (cnt && nn)
    {
      idbW_dest->flush("<center>");
      idbW_dest->flush("<br>");
      idbW_dest->flush("<form method=POST ACTION=\"http://%s/%s?"
		  "-ck" idbWSPACE "%s" idbWSPACE
		  "-db" idbWSPACE "%s" idbWSPACE
		  "-xdumpgen" idbWSPACE "\">",
		  idbW_ctx->cgidir, idbW_progname,
		  idbW_dbctx->cookie, idbW_ctx->dbname);

      idbW_dest->flush("<table border=%d bgcolor=%s>"
		       "<caption align=top><i>Build your query "
		       "for class %s</i></caption>",
		       idbW_db_config->look_config.border,
		       idbW_db_config->look_config.tablebgcolor,
		       cls->getName());
      idbW_dest->flush("<tr><th>");
      idbW_dest->flush("<table >");
      idbWDumpRequestDialogHeaderDump(p, fd, nn);

      for (int i = 0; i < nn; i++)
	{
	  idbW_dest->flush("<tr>");
	  if (nn > 1) make_par_open();
	  make_items(cls, i);
	  make_operand();
	  make_value();
	  if (nn > 1) make_par_close();


	  if (i != nn-1 && nn > 1)
	    make_logical();
	  else if (nn > 1)
	    make_end();

	  idbW_dest->flush("</tr>\n");
	}

      idbW_dest->flush("</table>");
      idbW_dest->flush("</th></tr>\n");
      idbW_dest->flush("</table>\n");
      idbW_dest->flush("<pre>");
      idbW_dest->flush("<input type=\"SUBMIT\" value=\"SEARCH\"></strong></form>");

      idbW_dest->flush("</center>");
      idbW_dest->flush("\n\n<center><pre>");
      idbW_dest->flush("</center>");
    }

  idbW_dest->flush("<center>");
  idbW_TABULAR();

  idbW_CHECK(idbW_ctx->db->transactionCommit());

  idbWBackTo();
  idbW_dest->flush("</pre></center>");

  return 0;
}

