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

#ifdef HAVE_REGEX_H
#include <regex.h>
#elif defined(HAVE_LIBGEN_H)
#include <libgen.h>
#else
#error No regular expression implementation available on this platform
#endif

static Config *idbW_configuration;

//
// Forward Declarations
//

static const char *idbW_getTagOQLRealize(const Class *);
static const char *idbW_getItemsRealize(const Class *cls);
static int idbW_getFormCountRealize(const Class *cls);
static int idbW_isFormPolymorphRealize(const Class *cls);
static int idbW_getFormDepthRealize(const Class *cls);

static const char *idbW_default_conf = "default";

static const char *
getParentResource(Class *cls, const char *(*resfun)(const Class *),
		  const char *def = "")
{
  cls = cls->getParent();
  if (cls->isSystem())
    return def;
  const char *x = resfun(cls);
  return x ? x : def;
}

static int
getParentIntResource(Class *cls, int (*resfun)(const Class *),
		     int def)
{
  cls = cls->getParent();
  if (cls->isSystem())
    return def;
  return resfun(cls);
}

static const char *
make_parent_tag(const char *prefix, const Class *cls, Bool generic)
{
  static std::string s;

  cls = cls->getParent();
  if (cls->isSystem())
    {
      if (!generic)
	return 0;
      s = std::string("%") + prefix + "%";
    }
  else
    s = std::string("%") + prefix + "_" + cls->getName() + "%";

  return s.c_str();
}

static void
print_nl(idbWDest *dest, const Attribute **attrs, int attr_cnt)
{
  if (!dest)
    return;

  for (int j = 0; j < attr_cnt; j++)
    {
      const Attribute *attr = attrs[j];
      if (attr->isNative() || (!attr->getClass()->asAgregatClass() &&
			       !attr->getClass()->asCollectionClass()))
	continue;
      dest->flush("\n");
      return;
    }
}

static int
iscolor(const char *color)
{
  if (strlen(color) != 6)
    return 0;

  for (int i = 0; i < 6; i++)
    {
      char c = color[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F')))
	return 0;
    }

  return 1;
}

void
idbW_get_conf_color(char *&color, const char *resname, const char *def,
		    idbWDest *dest, const char *xdef)
{
  const char *x = idbW_configuration->getValue(resname);
  if (!x) x = def;
  if (!iscolor(x))
    {
      IDB_LOG(IDB_LOG_WWW,
	      ("invalid color format '%s' for the resource '%s'. "
	       "Must be under the form: rrggbb", color, resname));
      x = def;
    }

  if (dest)
    {
      if (xdef && x == def)
	dest->flush("%s = %s;\n", resname, xdef);
      else
	dest->flush("%s = \"%s\";\n", resname, x);
    }

  //free(color);
  color = strdup((std::string("#") + x).c_str());
}

static const char *
str_to_text(const char *x)
{
  static int buf_len;
  static char *buf;

  int len = 4*strlen(x)+1; // strict majorent
  if (buf_len <= len)
    {
      buf_len = len;
      free(buf);
      buf = (char *)malloc(buf_len);
    }

  char *p = buf;

  for (int n = 0; *x; x++)
    {
      char c = *x;
      if (c == '"')
	*p++ = '\\';
      *p++ = c;
    }
  *p = 0;
  return buf;
}

static const char *
str_to_html(const char *x, Bool strict)
{
  static int buf_len;
  static char *buf;

  int len = 4*strlen(x)+1; // strict majorent
  if (buf_len <= len)
    {
      buf_len = len;
      free(buf);
      buf = (char *)malloc(buf_len);
    }

  char *p = buf;

  for (; *x; x++)
    {
      char c = *x;
      if (c == '<')
	{
	  *p++ = '&';
	  *p++ = 'l';
	  *p++ = 't';
	  *p++ = ';';
	}
      else if (c == '>')
	{
	  *p++ = '&';
	  *p++ = 'g';
	  *p++ = 't';
	  *p++ = ';';
	}
      else if (strict && c == '"')
	{
	  *p++ = '\\';
	  *p++ = '"';
	}
      else
	*p++ = c;
    }

  *p = 0;
  return buf;
}

void
idbW_get_conf_string(char *&s, const char *resname, const char *def,
		     idbWDest *dest, const char *xdef)
{
  const char *x = idbW_configuration->getValue(resname);
  if (!x) x = def;

  if (dest)
    {
      if (xdef && x == def)
	dest->flush("%s = %s;\n", resname,
		    (dest->isHtml() ? str_to_html(xdef, False) :
		     xdef));
      else
	dest->flush("%s = \"%s\";\n", resname,
		    (dest->isHtml() ? str_to_html(x, True) :
		     str_to_text(x)));
    }

  //free(s);
  s = strdup(x);
}

void
idbW_get_conf_bool(Bool &sb, const char *resname, Bool def,
		   idbWDest *dest, const char *xdef)
{
  const char *x = idbW_configuration->getValue(resname);
  Bool b;

  if (!x) b = def;
  else if (!strcasecmp(x, "yes") || !strcasecmp(x, "on") ||
	   !strcasecmp(x, "true"))
    b = True;
  else
    b = False;

  if (dest)
    {
      if (xdef && !x)
	dest->flush("%s = %s;\n", resname, xdef);
      else
	dest->flush("%s = \"%s\";\n", resname, b ? "yes" : "no");
    }

  sb = b;
}

void
idbW_get_conf_uint(int &sui, const char *resname, unsigned int def,
		   idbWDest *dest, const char *xdef)
{
  const char *x = idbW_configuration->getValue(resname);
  int ui = x ? atoi(x) : def;
  if (ui < 0)
    {
      IDB_LOG(IDB_LOG_WWW,
	      ("invalid number '%s' for the resource '%s'\n", x, resname));
      ui = def;
      assert(ui >= 0);
    }

  if (dest)
    {
      if (xdef && !x)
	dest->flush("%s = %s;\n", resname, xdef);
      else
	dest->flush("%s = %d;\n", resname, ui);
    }

  sui = ui;
}

static void
init_conf_file(const char *cfgfile)
{
  int fd = open(cfgfile, O_RDONLY);
  if (fd >= 0)
    {
      close(fd);
      IDB_LOG(IDB_LOG_WWW, 
	      ("reading config file '%s'\n", (const char *)cfgfile));
      idbW_configuration->add(cfgfile);
    }
  else
    IDB_LOG(IDB_LOG_WWW, 
	    ("warning cannot open config file '%s'\n", (const char *)cfgfile));
}

static inline const char *
getval(const char *val, const char *def)
{
  return val ? val : def;
}

#if 1
#define DEF_BGCOLOR "ffffff"
#define DEF_TEXT    "000000"
#define DEF_LINK    "0000ee"
#define DEF_VLINK   "1000aa"
#define DEF_TABLE_BGCOLOR "eeeeee"
#define DEF_ERR_COLOR "ffaa00"
#define DEF_FIELD_COLOR "888888"
#else
#define DEF_BGCOLOR "000055"
#define DEF_TEXT    "ffffff"
#define DEF_LINK    "ffff00"
#define DEF_VLINK   "ff9900"
#define DEF_TABLE_BGCOLOR "002299"
#define DEF_ERR_COLOR "cc0000"
#define DEF_FIELD_COLOR "bbbbbb"
#endif

void
init_look_config(Database *, idbWDest *dest)
{
  if (dest) dest->flush("#\n# General Look Configuration\n#\n\n");

  idbW_get_conf_color(idbW_db_config->look_config.bgcolor,
		      "look_bgcolor", DEF_BGCOLOR, dest);
  idbW_get_conf_color(idbW_db_config->look_config.textcolor,
		      "look_textcolor", DEF_TEXT, dest);
  idbW_get_conf_color(idbW_db_config->look_config.linkcolor,
		      "look_linkcolor", DEF_LINK, dest);
  idbW_get_conf_color(idbW_db_config->look_config.vlinkcolor,
		      "look_vlinkcolor", DEF_VLINK, dest);
  idbW_get_conf_color(idbW_db_config->look_config.tablebgcolor,
		      "look_tablebgcolor", DEF_TABLE_BGCOLOR, dest);
  idbW_get_conf_color(idbW_db_config->look_config.errorbgcolor,
		      "look_errbgcolor", DEF_ERR_COLOR, dest);
  idbW_get_conf_color(idbW_db_config->look_config.fieldcolor,
		      "look_fieldcolor", DEF_FIELD_COLOR, dest);

  const char *bgcolor = getval(idbW_db_config->look_config.bgcolor, DEF_BGCOLOR);
  const char *text = getval(idbW_db_config->look_config.textcolor, DEF_TEXT);
  const char *link = getval(idbW_db_config->look_config.linkcolor, DEF_LINK);
  const char *vlink = getval(idbW_db_config->look_config.vlinkcolor, DEF_VLINK);

  std::string s = std::string("<body bgcolor=") + bgcolor + 
    " text=" + text + " link=" + link + " vlink=" + vlink + "><br><br>";

  idbW_get_conf_string(idbW_db_config->look_config.head_str,
		       "look_header", s.c_str(), dest,
		       "\"<body "
		       "bgcolor=%"  "look_bgcolor% "
		       "text=%"     "look_textcolor% "
		       "link=%"     "look_linkcolor% "
		       "vlink=%"    "look_vlinkcolor%"
		       "><br><br>\"");

  idbW_get_conf_string(idbW_db_config->look_config.tail_str,
		       "look_tail", "</body>", dest);

  idbW_get_conf_uint(idbW_db_config->look_config.border,
		     "look_border", 1, dest);
}

static void
init_mainpage_config(Database *db, idbWDest *dest)
{
  if (dest) dest->flush("\n#\n# MainPage Configuration\n#\n\n");

  idbW_get_conf_string(idbW_db_config->mainpage_config.url,
		       "mainpage_url", "", dest);

  idbW_get_conf_string(idbW_db_config->mainpage_config.head,
		       "mainpage_header", "", dest);

  idbW_get_conf_bool(idbW_db_config->mainpage_config.schema,
		     "mainpage_schema", True, dest);

  /*
  idbW_get_conf_bool(idbW_db_config->mainpage_config.idl,
		     "mainpage_idl", True, dest);
  */

  idbW_get_conf_bool(idbW_db_config->mainpage_config.odl,
		     "mainpage_odl", True, dest);

  idbW_get_conf_bool(idbW_db_config->mainpage_config.stats,
		     "mainpage_stats", True, dest);

  idbW_get_conf_bool(idbW_db_config->mainpage_config.oqlform,
		     "mainpage_oqlform", True, dest);

  idbW_get_conf_uint(idbW_db_config->oqlform_config.rows,
		     "mainpage_oqlform_rows", 10, dest);
  idbW_get_conf_uint(idbW_db_config->oqlform_config.cols,
		     "mainpage_oqlform_cols", 60, dest);

  idbW_get_conf_bool(idbW_db_config->mainpage_config.webconf,
		     "mainpage_webconf", True, dest);

  idbW_get_conf_bool(idbW_db_config->mainpage_config.classes,
		     "mainpage_classes", True, dest);

  if (!db)
    return;

  if (dest) dest->flush("\n");

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  idbW_db_config->mainpage_config.cls_cnt = list->getCount();

  idbW_db_config->mainpage_config.cls_config =
    new idbWMainPageConfig::Config[list->getCount()];

  LinkedListCursor c(list);
  Class *cls;

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      if (cls->isSystem() || cls->asCollectionClass())
	continue;

      struct idbWMainPageConfig::Config *config =
	&idbW_db_config->mainpage_config.cls_config[n];

      idbW_db_config->mainpage_config.cls_config[n].cls = cls;
      idbW_get_conf_bool(config->link,
			 (std::string("mainpage_link_") + cls->getName()).c_str(),
			 True, dest);
    }
}

static void
init_stats_config(Database *db, idbWDest *dest)
{
  if (!db)
    return;

  if (dest) dest->flush("\n#\n# Statistics Page Configuration\n#\n\n");

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  idbW_db_config->stats_config.cls_cnt = list->getCount();

  idbW_db_config->stats_config.cls_config =
    new idbWStatsConfig::Config[list->getCount()];

  LinkedListCursor c(list);
  Class *cls;

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      if (cls->isSystem() || cls->asCollectionClass())
	continue;

      struct idbWStatsConfig::Config *config =
	&idbW_db_config->stats_config.cls_config[n];

      idbW_db_config->stats_config.cls_config[n].cls = cls;

      idbW_get_conf_string(config->description,
			   (std::string("stats_description_") +
			   cls->getName()).c_str(), "", dest);
    }
}

static void
init_display_attributes_config(Database *db, idbWDest *dest)
{
  if (dest) dest->flush("\n#\n# Object Display General Configuration\n#\n\n");

  idbW_get_conf_uint(idbW_db_config->display_config.border,
		     "display_border", idbW_db_config->look_config.border,
		     dest);
  idbW_get_conf_uint(idbW_db_config->display_config.collelemcount,
		     "display_collelemcount", 40, dest);

  idbW_get_conf_uint(idbW_db_config->display_config.indent,
		     "display_indent", 2, dest);

  char *s = 0;
  idbW_get_conf_string(s,  "display_indent_char",
		       " ", dest);

  idbW_db_config->display_config.indent_char = *s;
  free(s);
  s = 0;

  idbW_get_conf_string(s,  "display_indent_expand_char",
		       " ",
		       dest);

  idbW_db_config->display_config.indent_expand_char = *s;
  free(s);

  idbW_get_conf_string(idbW_db_config->display_config.open,
		       "display_open_string",
		       //"+",
		       "<img src=\"../icons/open.jpg\" border=0 alt=\"Open\">",
		       dest);

  idbW_get_conf_string(idbW_db_config->display_config.close,
		       "display_close_string",
		       //"-",
		       "<img src=\"../icons/close.jpg\" border=0 alt=\"Close\">",
		       dest);

  idbW_get_conf_bool(idbW_db_config->display_config.type_link,
		     "display_type_link",
		     //True,
		     False,
		     dest);
}

static void
init_clsquery_config(Database *db, idbWDest *dest)
{
  if (dest) dest->flush("\n#\n# Class Query Page Configuration\n#\n\n");
  idbW_get_conf_bool(idbW_db_config->clsquery_config.description,
		     "classquery_description", True, dest);

  idbW_get_conf_bool(idbW_db_config->clsquery_config.instances_link,
		     "classquery_instances_link", True, dest);

  idbW_get_conf_uint(idbW_db_config->clsquery_config.form_count,
		     "classquery_form_count", 6, dest);

  idbW_get_conf_uint(idbW_db_config->clsquery_config.form_depth,
		     "classquery_form_depth", 1, dest);

  idbW_get_conf_bool(idbW_db_config->clsquery_config.form_polymorph,
		     "classquery_form_polymorph", False, dest);

  idbW_get_conf_string(idbW_db_config->clsquery_config.attribute_indexed_string,
		       "classquery_form_indexed_string",
		       "<i>(*)</i>", dest);

  idbW_get_conf_bool(idbW_db_config->clsquery_config.attribute_type,
		     "classquery_form_type", True, dest);

  if (!db)
    return;

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  idbW_db_config->clsquery_config.cls_cnt = list->getCount();

  idbW_db_config->clsquery_config.cls_config =
    new idbWClassQueryConfig::Config[list->getCount()];

  LinkedListCursor c(list);
  Class *cls;

  db->getSchema()->computeHashTable();

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      if (cls->isSystem() || cls->asCollectionClass())
	continue;

      if (dest) dest->flush("\n");

      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[n];

      config->cls = cls;

      idbW_get_conf_string(config->head,
			   (std::string("classquery_header_") +
			   cls->getName()).c_str(), "", dest);

      idbW_get_conf_bool(config->description,
			 (std::string("classquery_description_") +
			 cls->getName()).c_str(),
			 idbW_db_config->clsquery_config.description, dest);
      idbW_get_conf_bool(config->instances_link,
			 (std::string("classquery_instances_link_") +
			 cls->getName()).c_str(),
			 idbW_db_config->clsquery_config.instances_link, dest);

      char *items = 0;
      idbW_get_conf_string(items,
			   (std::string("classquery_form_attributes_") +
			   cls->getName()).c_str(),
			   getParentResource(cls, idbW_getItemsRealize),
			   dest,
			   make_parent_tag("classquery_form_attributes", cls,
					   False));

      config->items = new idbWItemList(items);
      free(items);

      idbW_get_conf_uint(config->form_count,
			 (std::string("classquery_form_count_") +
			 cls->getName()).c_str(),
			 getParentIntResource
			 (cls, idbW_getFormCountRealize,
			  idbW_db_config->clsquery_config.form_count),
			 dest,
			 make_parent_tag("classquery_form_count", cls, True));

      idbW_get_conf_uint(config->form_depth,
			 (std::string("classquery_form_depth_") +
			 cls->getName()).c_str(),
			 getParentIntResource
			 (cls, idbW_getFormDepthRealize,
			  idbW_db_config->clsquery_config.form_depth),
			 dest,
			 make_parent_tag("classquery_form_depth", cls, True));

      idbW_get_conf_bool(config->form_polymorph,
			 (std::string("classquery_form_polymorph_") +
			 cls->getName()).c_str(),
			 (Bool)getParentIntResource
			 (cls, idbW_isFormPolymorphRealize,
			  idbW_db_config->clsquery_config.form_polymorph),
			 dest,
			 make_parent_tag("classquery_form_polymorph", cls, True));

    }
}

static void
init_display_attributes_init(Database *db, idbWDest *dest)
{
  if (!db)
    return;

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  idbW_db_config->display_config.cls_cnt = list->getCount();

  idbW_db_config->display_config.cls_config =
    new idbWDisplayConfig::Config[list->getCount()];

  LinkedListCursor c(list);
  Class *cls;

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      /*
      if (cls->isSystem() || cls->asCollectionClass())
	continue;
	*/

      if (cls->asCollectionClass())
	continue;

      struct idbWDisplayConfig::Config *config =
	&idbW_db_config->display_config.cls_config[n];

      config->cls = cls;
      config->expand_cnt = 0;

      unsigned int attr_cnt;
      const Attribute **attrs = cls->getAttributes(attr_cnt);

      config->expands = new idbWDisplayConfig::Config::Expand[2*attr_cnt];
    }
}

static void
init_display_attributes_expand_config(Database *db, idbWDest *dest)
{
  if (!db)
    return;

  if (dest) dest->flush("\n#\n# Object Display Attribute Expansion Configuration\n#\n");

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  LinkedListCursor c(list);
  Class *cls;

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      if (cls->isSystem() || cls->asCollectionClass())
	continue;

      struct idbWDisplayConfig::Config *config =
	&idbW_db_config->display_config.cls_config[n];

      config->cls = cls;
      config->expand_cnt = 0;

      unsigned int attr_cnt;
      const Attribute **attrs = cls->getAttributes(attr_cnt);

      print_nl(dest, attrs, attr_cnt);

      for (int i = 0; i < attr_cnt; i++)
	{
	  const Attribute *attr = attrs[i];
	  if (attr->isNative() || (!attr->getClass()->asAgregatClass() &&
				   !attr->getClass()->asCollectionClass()))
	    continue;

	  Bool b;
	  idbW_get_conf_bool(b,
			     (std::string("display_attributes_expand_") +
			     cls->getName() + "_" + attr->getName()).c_str(),
			     False, dest);
	  
	  config->expands[config->expand_cnt++].set(attr->getNum(), b);
	}
    }
}

static unsigned int
get_extra_count(const char *extra)
{
  int cnt = 0;
  while (extra = strchr(extra, ':'))
    {
      cnt++;
      if (!*++extra)
	break;
    }

  return cnt;
}

static void
make_attr_extra_realize(struct idbWDisplayConfig::Config *config,
			Class *cls, const char *extra)
{
  char *extra_orig = strdup(extra);
  char *x = extra_orig, c;
  char *upx = x + strlen(extra_orig);

  while (x < upx)
    {
      const char *attrname, *value;
      while (*x == ' ' || *x == '\t')
	{
	  x++;
	  if (x >= upx)
	    break;
	}

      char *prevx = x;
      x = strchr(x, ':');
      if (!x) break;
      *x++ = 0;
      if (x >= upx)
	break;

      attrname = prevx;

      if (*x != '@' || (x < upx-1 && *(x+1) != '{'))
	break;
	  
      x += 2;
      if (x >= upx)
	break;

      prevx = x;

      while (c=*x++)
	{
	  if (x >= upx)
	    break;

	  if (c == '}' && *x == '@')
	    {
	      *(x-1) = 0;
	      value = prevx;
	      x += 2;
	      break;
	    }
	}

      config->attrs[config->attr_cnt++].set(config->cls, attrname, value);
    }

  free(extra_orig);
}

static void
make_attr_extra(struct idbWDisplayConfig::Config *config,
		Class *cls, const char *extra)
{
  config->attr_alloc_cnt = cls->getAttributesCount() + get_extra_count(extra);
  config->attrs = new idbWDisplayConfig::Config::Attr[config->attr_alloc_cnt];
  config->attr_cnt = 0;

  unsigned int attr_cnt;
  const Attribute **attrs = cls->getAttributes(attr_cnt);
  for (int i = 0; i < attr_cnt; i++)
    {
      const Attribute *attr = attrs[i];
      if (!attr->isNative())
	config->attrs[config->attr_cnt++].set(attr);
    }


  if (!*extra) return;
  make_attr_extra_realize(config, cls, extra);
  assert(config->attr_cnt < config->attr_alloc_cnt);
}

static void
make_attr_list(struct idbWDisplayConfig::Config *config,
	       Class *cls, const char *list)
{
  if (!*list) return;

  // 1/ transform list into Items with boolean
  idbWItemList items(list);

  // 2/ filter attributes from struct Attr *attrs
  for (int i = 0; i < config->attr_cnt; i++)
    {
      idbWDisplayConfig::Config::Attr *attr = &config->attrs[i];
      if (idbW_is_in_item_list(&items,
			       attr->isExtra ? attr->extra.name :
			       attr->attr->getName()) == idbWTFalse)
	attr->isIn = False;
    }
}

typedef idbWDisplayConfig::Config::Attr WAttr;

struct idbWAttrOrder {
  char *name;
  enum {
    AT,
    BEFORE,
    AFTER
  } op;
  char *name2;
  int pos;
};

WAttr *
get_attr(const char *name, WAttr *attrs, int attr_cnt)
{
  for (int i = 0; i < attr_cnt; i++)
    if (!strcmp(attrs[i].getName(), name))
      return &attrs[i];

  return 0;
}

static void
make_attr_order_num_set(WAttr *attrs, int attr_cnt, idbWAttrOrder *attrs_order,
			int order_cnt)
{
  for (int i = 0; i < order_cnt; i++)
    {
      idbWAttrOrder *order = &attrs_order[i];
      WAttr *attr1 = get_attr(order->name, attrs, attr_cnt);
      if (!attr1)
	continue;

      if (order->op == idbWAttrOrder::AT)
	{
	  attr1->setNum(order->pos);
	  continue;
	}

      WAttr *attr2 = get_attr(order->name2, attrs, attr_cnt);
      if (!attr2)
	continue;

      if (order->op == idbWAttrOrder::BEFORE)
	attr1->setNum(attr2->getNum()-1);
      else
	attr1->setNum(attr2->getNum()+1);
    }
}

static void
make_attr_order_struct(const char *order, idbWAttrOrder *& attrs_order,
		       int &item_cnt, char *&buf)
{
  attrs_order = 0;
  item_cnt = 0;

  buf = strdup(order);
  char *x = buf;

  int item_alloc = 0;
  x = strtok(x, " \t\n");

  int lastpos = -1;
  while(x)
    {
      if (item_cnt >= item_alloc)
	{
	  item_alloc += 12;
	  attrs_order = (idbWAttrOrder *)
	    realloc(attrs_order, sizeof(idbWAttrOrder) * item_alloc);
	}

      char *p;
      if ((p = strchr(x, '>')) || (p = strchr(x, '<')))
	{
	  attrs_order[item_cnt].op = (*p == '<' ? idbWAttrOrder::BEFORE :
				      idbWAttrOrder::AFTER);
	  *p = 0;
	  attrs_order[item_cnt].name = x;
	  attrs_order[item_cnt].name2 = p+1;
	}
      else
	{
	  p = strchr(x, '=');
	  attrs_order[item_cnt].op = idbWAttrOrder::AT;
	  int pos = p && *(p+1) ? atoi(p+1) : lastpos+1;
	  lastpos = pos;
	  attrs_order[item_cnt].pos = pos; 
	  if (p)
	    *p = 0;
	  attrs_order[item_cnt].name = x;
	}

      item_cnt++;
      x = strtok(0, " \t\n");
    }
}

static const Class *idbW_order_cls;
static idbWAttrOrder *idbW_order;
static int idbW_order_cnt;

static idbWAttrOrder *
get_order(const char *name)
{
  for (int i = 0; i < idbW_order_cnt; i++)
    if (!strcmp(idbW_order[i].name, name))
      return &idbW_order[i];

  return (idbWAttrOrder *)0;
}

static idbWAttrOrder *
get_order(const WAttr *a)
{
  return get_order(a->getName());
}

static int
cmp(const void *x1, const void *x2)
{
  return ((const WAttr *)x1)->getNum() - ((const WAttr *)x2)->getNum();
}

static void
trace(struct idbWDisplayConfig::Config *config)
{
  const WAttr *attrs = config->attrs;
  for (int i = 0; i < config->attr_cnt; i++)
    printf("attr[%d] = %s [%s]\n", i, attrs[i].getName(),
	   attrs[i].isExtra ? attrs[i].extra.oql : "normal attr");
  printf("\n");
}

static void
trace(idbWAttrOrder *order, int order_cnt)
{
  for (int i = 0; i < order_cnt; i++)
    printf("order[%d] = %s %s %s\n",
	   i, order[i].name,
	   (order[i].op == idbWAttrOrder::AT ? "=" :
	    (order[i].op == idbWAttrOrder::BEFORE ? "<" : ">")),
	   order[i].name2 ? order[i].name2 : "");
  printf("\n");
}

static void
make_attr_order(struct idbWDisplayConfig::Config *config,
		Class *cls, const char *order)
{
  idbW_order_cnt = 0;
  if (!*order) return;

  char *buf;
  make_attr_order_struct(order, idbW_order, idbW_order_cnt, buf);
  make_attr_order_num_set(config->attrs, config->attr_cnt,
			  idbW_order, idbW_order_cnt);

  //trace(idbW_order, idbW_order_cnt);

  idbW_order_cls = config->cls;
  qsort(config->attrs, config->attr_cnt, sizeof(WAttr), cmp);

  free(buf);
  free(idbW_order);
  idbW_order = 0;

  //trace(config);
}

#define INIT_DISPLAY(NAME, MSG, RESNAME, FUN) \
static void \
NAME(Database *db, idbWDest *dest) \
{ \
  if (!db) \
    return; \
 \
  if (dest) dest->flush(MSG); \
 \
  LinkedListCursor c((LinkedList *)db->getSchema()->getUserData()); \
  Class *cls; \
 \
  for (int n = 0; c.getNext((void *&)cls); n++) \
    { \
      if (!cls->asAgregatClass()) \
	continue; \
 \
      struct idbWDisplayConfig::Config *config = \
	&idbW_db_config->display_config.cls_config[n]; \
 \
      char *list = 0; \
      idbW_get_conf_string(list, \
			   (std::string(RESNAME) + \
			   cls->getName()).c_str(), "", dest); \
 \
      FUN(config, cls, list); \
      free(list); \
    } \
}

INIT_DISPLAY(init_display_attributes_list_config,
	     "\n#\n# Object Display Attribute List Configuration\n#\n\n",
	     "display_attributes_list_",
	     make_attr_list)

INIT_DISPLAY(init_display_attributes_order_config,
	     "\n#\n# Object Display Order Attribute Configuration\n#\n\n",
	     "display_attributes_order_",
	     make_attr_order)

INIT_DISPLAY(init_display_attributes_extra_config,
	     "\n#\n# Object Display Extra Attribute Configuration\n#\n\n",
	     "display_attributes_extra_",
	     make_attr_extra)

static void
init_cls_config(Database *db, idbWDest *dest)
{
  if (!db)
    return;

  if (dest) dest->flush("\n#\n# Object Display Gettag Methods\n#\n\n");

  const LinkedList *list = (LinkedList *)db->getSchema()->getUserData();
  idbW_db_config->cls_config.cls_cnt = list->getCount();

  int cnt = list->getCount();
  idbW_db_config->cls_config.cls_config =
    new idbWClassConfig::Config[cnt];

  LinkedListCursor c(list);
  Class *cls;

  for (int n = 0; c.getNext((void *&)cls); n++)
    {
      if (cls->isSystem() || cls->asCollectionClass())
	continue;

      struct idbWClassConfig::Config *config =
	&idbW_db_config->cls_config.cls_config[n];

      config->cls = cls;

      idbW_get_conf_string(config->gettag,
			   (std::string("class_gettag_") + cls->getName()).c_str(),
			   getParentResource(cls, idbW_getTagOQLRealize),
			   dest,
			   make_parent_tag("class_gettag", cls, False));
    }
}

static void
init()
{
  static idbWGenConfig idbW_config;

  if (!idbW_db_config)
    idbW_db_config = &idbW_config;

  idbW_db_config->release();

  delete idbW_configuration ;
  idbW_configuration = new Config(*ClientConfig::getInstance());
}

static int
is_in(Class *cls, Class **classes, int cnt)
{
  for (int i = 0; i < cnt; i++)
    if (classes[i] == cls)
      return 1;
  return 0;
}

static void
sort_classes(Schema *sch)
{
  const LinkedList *list = sch->getClassList();
  Class **classes = new Class*[list->getCount()];
  LinkedListCursor c(list);
  Class *cls;
  int n, dn, i;

  for (n = 0; c.getNext((void *&)cls); )
    if (!cls->isSystem() && !cls->asCollectionClass())
      classes[n++] = cls;

  Class **dclasses = new Class*[n];

  for (i = 0, dn = 0; i < n; i++)
    if (classes[i]->asEnumClass() || classes[i]->getParent()->isSystem())
      dclasses[dn++] = classes[i];

  while (dn != n)
    for (i = 0; i < n; i++)
      {
	if (!is_in(classes[i], dclasses, dn) &&
	    is_in(classes[i]->getParent(), dclasses, dn))
	  dclasses[dn++] = classes[i];
      }

  LinkedList *nlist = new LinkedList();
  sch->setUserData(nlist);
  for (i = 0; i < dn; i++)
    nlist->insertObjectLast(dclasses[i]);

  delete [] classes;
  delete [] dclasses;
}

static void
conf_realize(Database *db, idbWDest *dest = 0)
{
  if (db && !db->getSchema()->getUserData())
    sort_classes(db->getSchema());
  
  init_look_config(db, dest);
  init_mainpage_config(db, dest);
  init_stats_config(db, dest);
  init_display_attributes_config(db, dest);
  init_cls_config(db, dest);

  init_display_attributes_init(db, dest);
  init_display_attributes_extra_config(db, dest);
  init_display_attributes_list_config(db, dest);
  init_display_attributes_order_config(db, dest);
  init_display_attributes_expand_config(db, dest);

  init_clsquery_config(db, dest);
}

static Status
emergency(Status s)
{
  eyedb::init();
  conf_realize(0);
  idbWHead();
  return s;
}

static Status
init(WConfig *w_config)
{
  if (!w_config)
    return Success;

  char *tmpfile = tmpnam(0);
  int fd = creat(tmpfile, 0666);
  if (fd < 0)
    return emergency(Exception::make(IDB_ERROR,
					"cannot open tmpfile '%s' for "
					"writing", tmpfile));

  w_config->getDatabase()->transactionBegin();
  const char *conf = w_config->getConf().c_str();
  write(fd, conf, strlen(conf));
  close(fd);
  w_config->getDatabase()->transactionAbort();
  
  init_conf_file(tmpfile);
  unlink(tmpfile);

  if (idbW_dbctx)
    {
      if (idbW_dbctx->w_config)
	idbW_dbctx->w_config->release();
      idbW_dbctx->w_config = w_config;
    }

  return Success;
}

void
idbWDisplayConf(Database *db, idbWDest *dest)
{
  conf_realize(db, dest);
}

void
idbWTemplateConf(Database *db, idbWDest *dest)
{
  ::init();
  conf_realize(db, dest);
}

void
idbWExpandConf(Database *db, idbWDest *dest, const char *conffile)
{
  ::init();
  init_conf_file(conffile);
  conf_realize(db, dest);
}

Status
getconf(Database *db, const char *attr, const char *hint,
	WConfig *&w_config)
{
  if (!db->getSchema()->getClass("w_config"))
    {
      w_config = 0;
      return Success;
    }

  Status s;

  s = db->transactionBegin();
  if (s) return emergency(s);

  OQL oql(db, "select w_config.%s = \"%s\"", attr, hint);
  ObjectArray obj_arr;
  s = oql.execute(obj_arr);
  if (s) return emergency(s);
      
  w_config = obj_arr.getCount() ? (WConfig *)obj_arr[0] : 0;

  if (w_config)
    {
      Oid oid = w_config->getOid();
      w_config->release();
      s = db->reloadObject(&oid, (Object **)&w_config);
      if (s) return s;
    }

  return db->transactionAbort();
}

struct idbTR {
  Database *db;
  idbTR(Database *_db) : db(_db) {
    if (db) db->transactionBegin();
  }
  ~idbTR() {
    if (db) db->transactionCommit();
  }
};

Status
idbWInitConf(Database *db, const char *confname, const char *username)
{
  WConfig *w_config = 0;
  Status s;

  idbTR tr(db);

  IDB_LOG(IDB_LOG_WWW,
	  ("INIT config db=%s, confname=%s, user=%s\n",
	   (db ? db->getName() : "nodb"), (confname ? confname : "noconf"),
	   (username ? username : "nouser")));

  if (confname && *confname)
    {
      s = getconf(db, "name", confname, w_config);
      if (s) return s;
      
      if (!w_config)
	return emergency(Exception::make(IDB_ERROR,
					    "cannot find configuration '%s'",
					    confname));
    }
  else if (username)
    {
      s = getconf(db, "user", username, w_config);
      if (s) return s;
      
      if (!w_config)
	{
	  s = getconf(db, "name", idbW_default_conf, w_config);
	  if (s) return s;
	}
    }

  ::init();
  s = init(w_config);
  if (s) return s;
  conf_realize(db);
  return Success;
}

//
// specific utility functions
//

const char *
idbW_get_clsquery_header(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (config->cls != cls)
	continue;

      return config->head;
    }

  return 0;
}

Bool
idbW_is_expanded(int clsnum, int attrnum)
{
  for (int i = 0; i < idbW_db_config->display_config.cls_cnt; i++)
    {
      struct idbWDisplayConfig::Config *config =
	&idbW_db_config->display_config.cls_config[i];

      if (!config->cls || config->cls->getNum() != clsnum)
	continue;

      for (int i = 0; i < config->expand_cnt; i++)
	if (attrnum == config->expands[i].attrnum)
	  return config->expands[i].expand;
    }

  return False;
}

static inline int
compare(const Class *cls1, const Class *cls2)
{
  if (cls1 == cls2)
    return 1;

  if (!cls1 || !cls2)
    return 0;

  return cls1->getOid() == cls2->getOid();
}

const char *
idbW_get_description(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->mainpage_config.cls_cnt; i++)
    {
      struct idbWStatsConfig::Config *config =
	&idbW_db_config->stats_config.cls_config[i];

      if (compare(config->cls, cls))
	return config->description;
    }

  return 0;
}

static int
idbW_getFormCountRealize(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->form_count;
    }

  return 0;
}

static int
idbW_isFormPolymorphRealize(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->form_polymorph;
    }

  return 0;
}

static int
idbW_getFormDepthRealize(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->form_depth;
    }

  return 0;
}

int
idbW_get_clsquery_form_count(const Class *cls)
{
  return idbW_getFormCountRealize(cls);
}

Bool
idbW_is_clsquery_form_polymorph(const Class *cls)
{
  return IDBBOOL(idbW_isFormPolymorphRealize(cls));
}

int
idbW_get_clsquery_form_depth(const Class *cls)
{
  return idbW_getFormDepthRealize(cls);
}

int
idbW_should_dump_description(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->description;
    }

  return idbW_db_config->clsquery_config.instances_link;
}

int
idbW_should_instances_link(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->instances_link;
    }

  return idbW_db_config->clsquery_config.instances_link;
}

const idbWItemList *
idbW_get_item_list(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->items;
    }

  return 0;
}

static const char *
idbW_getItemsRealize(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassQueryConfig::Config *config =
	&idbW_db_config->clsquery_config.cls_config[i];
      if (compare(config->cls, cls))
	return (config->items ? config->items->orig_str : 0);
    }

  return 0;
}

Bool
idbW_should_link_class(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->mainpage_config.cls_cnt; i++)
    {
      struct idbWMainPageConfig::Config *config =
	&idbW_db_config->mainpage_config.cls_config[i];
      if (compare(config->cls, cls))
	return config->link;
    }

  return True;
}

static const char *
idbW_getTagOQLRealize(const Class *cls)
{
  for (int i = 0; i < idbW_db_config->clsquery_config.cls_cnt; i++)
    {
      struct idbWClassConfig::Config *config =
	&idbW_db_config->cls_config.cls_config[i];

      if (compare(config->cls, cls))
	return config->gettag && *config->gettag ? config->gettag : 0;
    }

  return 0;
}

const char *
idbW_getGetTagOQL(Object *o)
{
  return idbW_getTagOQLRealize(o->getClass());
}

WAttr *
make_default_attr_list(const Class *cls, int &attr_cnt)
{
#define DEPTH 12
  static int depth;
  static WAttr *idbW_attrs[DEPTH];
  static unsigned int idbW_attr_cnt[DEPTH];

  if (depth == DEPTH)
    depth = 0;

  if (idbW_attrs[depth])
    {
      memset(idbW_attrs[depth], 0, sizeof(WAttr) * idbW_attr_cnt[depth]);
      delete [] idbW_attrs[depth];
    }

  const Attribute **cls_attrs = cls->getAttributes(idbW_attr_cnt[depth]);
  idbW_attrs[depth] = new WAttr[idbW_attr_cnt[depth]];
  attr_cnt = 0;

  for (int i = 0; i < idbW_attr_cnt[depth]; i++)
    {
      const Attribute *attr = cls_attrs[i];
      if (!attr->isNative())
	idbW_attrs[depth][attr_cnt++].set(attr);
    }

  return idbW_attrs[depth++];
}

WAttr *
idbW_getAttrList(const Class *cls, int &attr_cnt)
{
  for (int i = 0; i < idbW_db_config->display_config.cls_cnt; i++)
    {
      struct idbWDisplayConfig::Config *config =
	&idbW_db_config->display_config.cls_config[i];

      if (compare(config->cls, cls))
	{
	  attr_cnt = config->attr_cnt;
	  return config->attrs;
	}
    }

  return make_default_attr_list(cls, attr_cnt);
}

idbWTBool
idbW_is_in_item_list(const idbWItemList *items, const char *item_name)
{
  if (!items || !items->item_cnt)
    return idbWTUndef;

  idbWTBool on = idbWTUndef;
  for (int i = 0; i < items->item_cnt; i++)
    {
      idbWTBool xon = items->bitems[i].compare(item_name);
      if (xon != idbWTUndef)
	on = xon;
    }

  return on;
}

idbWTBool
idbW_is_in_item_list(const idbWItemList *items, AttrIdxContext &idx_ctx)
{
  return idbW_is_in_item_list(items, idx_ctx.getAttrName(True).c_str());
}

idbWItemList::idbWItemList(const char *s)
{
  item_cnt = 0;
  buf = 0;
  orig_str = 0;
  bitems = 0;

  if (!s || !*s)
    return;

  int item_alloc = 0;
  buf = strdup(s);

  char *x = buf;
  orig_str = strdup(s);

  x = strtok(x, " \t\n");

  while(x)
    {
      if (item_cnt >= item_alloc)
	{
	  item_alloc += 12;
	  bitems = (idbWItemList::BItem *)
	    realloc(bitems, sizeof(idbWItemList::BItem) * item_alloc);
	}

      bitems[item_cnt].on = (*x == '!' ? idbWTFalse : idbWTTrue);
      char *s = (*x == '!' ? x+1 : x);
      if (*s == '~')
	{
	  bitems[item_cnt].isregex = True;
#if defined(HAVE_REGCOMP)
	  regex_t *re = (regex_t *)malloc( sizeof( regex_t));
	  if (!regcomp(re, s+1, 0))
	    bitems[item_cnt].item = (char *)re;
#elif defined(HAVE_REGCMP)
	  bitems[item_cnt].item = regcmp(s+1, (char *)0);
#endif
	}
      else
	{
	  bitems[item_cnt].isregex = False;
	  bitems[item_cnt].item = s;
	}

      item_cnt++;
      x = strtok(0, " \t\n");
    }
}

idbWTBool
idbWItemList::BItem::compare(const char *s) const
{
  if (isregex)
    {
#ifdef HAVE_REGEXEC
      if (regexec((regex_t *)item, s, 0, (regmatch_t *)0, 0))
	return on;
#elif defined(HAVE_REGEX)
      if (regex(item, s, 0))
	return on;
#endif

      return idbWTUndef;
    }

  if (!strcmp(item, s))
    return on;

  return idbWTUndef;
}

void
idbWItemList::BItem::release()
{
}

idbWItemList::~idbWItemList()
{
  free(orig_str);
  free(buf);
  for (int i = 0; i < item_cnt; i++)
    bitems[i].release();
  free(bitems);
}

int
idbWLoadConfRealize(idbWProcess *p, int fd)
{
  idbWInitConf(idbW_ctx->db);
  idbW_dest->flush("<h1><em>WEB Configuration Reloaded</em></h1><center>");
  idbWBackTo();
  return 0;
}

int
idbWDumpConfRealize(idbWProcess *p, int fd)
{
  std::string s;
  if (idbW_dbctx->w_config)
    {
      if (*idbW_dbctx->w_config->getName().c_str())
	s = std::string("'") + idbW_dbctx->w_config->getName() + "'";
      else if (*idbW_dbctx->w_config->getUser().c_str())
	s = std::string("(default for user '") + idbW_dbctx->w_config->getUser() + "')";
    }
  else
    s = "(default)";

  idbW_dest->flush("<h1><em>WEB Configuration %s</em>"
		   "</h1><pre>", s.c_str());

  if (idbW_dbctx->w_config)
    idbW_dest->cflush(str_to_html(idbW_dbctx->w_config->getConf().c_str(), False),
		      True);
  else
    idbWDisplayConf(idbW_ctx->db, idbW_dest);

  idbW_dest->flush("</pre><center><br>");
  idbWBackTo();
  return 0;
}
