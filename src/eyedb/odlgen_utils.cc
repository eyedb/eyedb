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


#include "eyedb_p.h"
#include "Attribute_p.h"
#include <string.h>
#include <map>

namespace eyedb {

  static std::map<std::string, bool> map_rw;

  inline void reserve(const std::string &s)
  {
    map_rw[s] = true;
  }

  void init_reserved_words()
  {
    if (map_rw.size())
      return;

    // C++ keywords
    reserve("asm");
    reserve("auto");
    reserve("bool");
    reserve("break");
    reserve("case");
    reserve("catch");
    reserve("char");
    reserve("class");
    reserve("const");
    reserve("const_cast");
    reserve("continue");
    reserve("default");
    reserve("delete");
    reserve("do");
    reserve("double");
    reserve("dynamic_cast");
    reserve("else");
    reserve("enum");
    reserve("explicit");
    reserve("extern");
    reserve("false");
    reserve("float");
    reserve("for");
    reserve("friend");
    reserve("goto");
    reserve("if");
    reserve("inline");
    reserve("int");
    reserve("long");
    reserve("mutable");
    reserve("namespace");
    reserve("new");
    reserve("operator");
    reserve("private");
    reserve("protected");
    reserve("public");
    reserve("register");
    reserve("reinterpret_cast");
    reserve("return");
    reserve("short");
    reserve("signed");
    reserve("sizeof");
    reserve("static");
    reserve("static_cast");
    reserve("struct");
    reserve("switch");
    reserve("template");
    reserve("this");
    reserve("throw");
    reserve("true");
    reserve("try");
    reserve("typedef");
    reserve("typeid");
    reserve("typename");
    reserve("union");
    reserve("unsigned");
    reserve("using");
    reserve("virtual");
    reserve("void");
    reserve("volatile");
    reserve("wchar_t");
    reserve("while");

    // Java keywords
    reserve("final");
    reserve("synchonized");

    // eyedb methods (non exhaustive list)
    reserve("gbxObject");
    reserve("getRefCount");
    reserve("isLocked");
    reserve("incrRefCount");
    reserve("decrRefCount");
    reserve("lock");
    reserve("unlock");
    reserve("isOnStack");
    reserve("release");
    reserve("setTag");
    reserve("getTag");
    reserve("userGarbage");
    reserve("keep");
    reserve("unkeep");
    reserve("getObjectCount");
    reserve("getHeapSize");
    reserve("isValidObject");
    reserve("setObjMapped");
    reserve("isObjMapped");
    reserve("getObjMap");

    reserve("Object");
    //reserve("clone");
    reserve("getClass");
    reserve("getIDR");
    reserve("getIDRSize");
    //reserve("getType");
    reserve("getOid");
    reserve("getCTime");
    reserve("getMTime");
    reserve("getStringCTime");
    reserve("getStringMTime");
    reserve("getLock");
    reserve("setLock");
    reserve("setUserData");
    reserve("getUserData");
    reserve("getAllUserData");
    reserve("isModify");
    reserve("setDatabase");
    reserve("create");
    reserve("update");
    reserve("realize");
    reserve("remove");
    reserve("store");
    reserve("apply");
    reserve("trace");
    reserve("toString");
    reserve("getDataspace");
    reserve("getLocation");
    reserve("setDataspace");
    reserve("move");
    reserve("setProtection");
    reserve("setValue");
    reserve("isUnrealizable");
    reserve("isRemoved");
    reserve("getDatabase");

    reserve("Agregat");
    reserve("getValue");
    reserve("setItemSize");
    reserve("getItemSize");
    reserve("setItemValue");
    reserve("setItemOid");
    reserve("getItemOid");

    reserve("Struct");
    reserve("Union");
  }

  GenCodeHints::GenCodeHints()
  {
    attr_style       = ExplicitAttrStyle; // keep it for now
    style = NULL;
    setStyle("explicit");

    gen_class_stubs  = False;
    class_enums      = False;
    gen_down_casting = True;
    attr_cache       = False;
    error_policy     = StatusErrorPolicy;
    dirname          = strdup(".");
    fileprefix       = NULL;
    stubs            = NULL;
    c_suffix         = strdup(".cc");
    h_suffix         = strdup(".h");
    gen_date         = True;

    init_reserved_words();
  }

  Status
  GenCodeHints::setStyle(const char *file)
  {
    delete style;
    style = new Style(file);
    return style->getStatus();
  }

  static char *upper_case(const char *p, char *s = 0)
  {
    char c, *q;

    s = (!s ? strdup(p) : s);
    q = s;

    while (c = *p) {
      *q = c;
      if (c >= 'a' && c <= 'z')
	*q += 'A' - 'a';

      q++; p++;
    }

    *q = 0; // not needed
    return s;
  }

  static char *lower_case(const char *p, char *s = 0)
  {
    char c, *q;

    s = (!s ? strdup(p) : s);
    q = s;

    while (c = *p) {
      *q = c;
      if (c >= 'A' && c <= 'Z')
	*q += c + 'a' - 'A';

      q++; p++;
    }

    *q = 0; // not needed
    return s;
  }

  static char *capitalize_case(const char *p, char *s = 0)
  {
    char c, *q;
    int state = 0;

    s = (!s ? strdup(p) : s);
    q = s;

    while(c = *p) {
      if (!state || (c == '_' && *(p+1))) {
	*q = (c == '_' ? *++p : *p);
	  
	if (*q >= 'a' && *q <= 'z')
	  *q += 'A' - 'a';
	state = 1;
      }
      else
	*q = *p;
      
      q++;
      p++;
    }

    *q = 0;
    return s;
  }

  static char *
  getStaticString()
  {
#define NN 8
    static int round = 0;
    static char str[NN][256];

    if (round >= NN)
      round = 0;

    return str[round++];
  }

  static const char *capitalize_fun(const char *s)
  {
    return capitalize_case(s, getStaticString());
  }

  static const char *upper_fun(const char *s)
  {
    return upper_case(s, getStaticString());
  }

  static const char *lower_fun(const char *s)
  {
    return lower_case(s, getStaticString());
  }

  static const char *ident_fun(const char *s)
  {
    return s;
  }

  GenCodeHints::Style::Style(const char *file)
  {
    memset(desc, 0, sizeof(desc));

    if (!strcasecmp(file, "implicit")) {
      desc[tGet].fmt = strdup("%IN");
      desc[tSet].fmt = strdup("%IN");
      desc[tGetOid].fmt = strdup("%IN_oid");
      desc[tSetOid].fmt = strdup("%IN_oid");
      desc[tGetCount].fmt = strdup("%IN_cnt");
      desc[tSetCount].fmt = strdup("%IN_cnt");
      desc[tGetColl].fmt = strdup("%IN");
      desc[tSetColl].fmt = strdup("%IN");
      desc[tAddItemToColl].fmt = strdup("addto_%IN");
      desc[tRmvItemFromColl].fmt = strdup("rmvfrom_%IN");
      desc[tSetItemInColl].fmt = strdup("setin_%IN_at");
      desc[tUnsetItemInColl].fmt = strdup("unsetin_%IN_at");
      desc[tGetItemAt].fmt = strdup("%IN_at");
      desc[tGetOidItemAt].fmt = strdup("%IN_oidat");
      desc[tRetrieveItemAt].fmt = strdup("%IN_at");
      desc[tRetrieveOidItemAt].fmt = strdup("%IN_oidat");
      desc[tCast].fmt = strdup("%IP%CN_");
      desc[tSafeCast].fmt = strdup("%IP%CN_c");
    }
    else if (!strcasecmp(file, "explicit")) {
      desc[tGet].fmt = strdup("get%CN");
      desc[tSet].fmt = strdup("set%CN");
      desc[tGetOid].fmt = strdup("get%CNOid");
      desc[tSetOid].fmt = strdup("set%CNOid");
      desc[tGetCount].fmt = strdup("get%CNCount");
      desc[tSetCount].fmt = strdup("set%CNCount");
      desc[tGetColl].fmt = strdup("get%CNColl");
      desc[tSetColl].fmt = strdup("set%CNColl");
      desc[tAddItemToColl].fmt = strdup("addTo%CNColl");
      desc[tRmvItemFromColl].fmt = strdup("rmvFrom%CNColl");
      desc[tSetItemInColl].fmt = strdup("setIn%CNCollAt");
      desc[tUnsetItemInColl].fmt = strdup("unsetIn%CNCollAt");
      desc[tGetItemAt].fmt = strdup("get%CNAt");
      desc[tGetOidItemAt].fmt = strdup("get%CNOidAt");
      desc[tRetrieveItemAt].fmt = strdup("retrieve%CNAt");
      desc[tRetrieveOidItemAt].fmt = strdup("retrieve%CNOidAt");
      desc[tCast].fmt = strdup("%UP%CN_");
      desc[tSafeCast].fmt = strdup("%UP%CN_c");
    }
    else {
      parse_file(file);
      if (status)
	return;
    }
  
    status = compile();
  }
  
  static const char *
  make_fmt(const char *fmt)
  {
    static char sfmt[64];
    char *p = sfmt;
    char c;
    while (c = *fmt) {
      if (c == '%') {
	*p++ = c;
	c = *++fmt;
	if (c == 'n' || c == 'N')
	  *p++ = 'I';
      }
      *p++ = *fmt++;
    }

    *p = 0;
    return sfmt;
  }

  void
  GenCodeHints::Style::parse_file(const char *file)
  {
    FILE *fd = fopen(file, "r");
    if (!fd) {
      status = Exception::make(IDB_ERROR,
			       "cannot open user file style '%s'",
			       file);
      return;
    }

    char line[128];
    int nlines = 0;
  
    while (fgets(line, sizeof(line)-1, fd)) {
      char optype_str[64], fmt_str[64];
      int r = sscanf(line, "%s %s\n", optype_str, fmt_str);
      
      nlines++;

      if (r <= 0 || *optype_str == '#')
	continue;
	  
      if (r != 2) {
	status =
	  Exception::make(IDB_ERROR,
			  "syntax error in user file style '%s' "
			  "at line %d",
			  file, nlines);
	return;
      }
      
      int i;
      for (i = 0; i < tLastOp; i++)
	if (!strcasecmp(optype_str, opTypeStr((OpType)i))) {
	  desc[i].fmt = strdup(make_fmt(fmt_str));
	  desc[i].op = (OpType)i; // added the 10/3/00
	  break;
	}
      
      if (i == tLastOp) {
	status =
	  Exception::make(IDB_ERROR,
			  "syntax error in user file style '%s' "
			  "at line %d",
			  file, nlines);
	return;
      }
    }
  }

  const char *
  GenCodeHints::Style::opTypeStr(OpType op)
  {
    if (op == tGet)
      return "GET";
    if (op == tSet)
      return "SET";
    if (op == tGetOid)
      return "tGetOid";
    if (op == tSetOid)
      return "SETOID";
    if (op == tGetCount)
      return "GETCOUNT";
    if (op == tSetCount)
      return "SETCOUNT";
    if (op == tGetColl)
      return "GETCOLL";
    if (op == tSetColl)
      return "SETCOLL";
    if (op == tAddItemToColl)
      return "ADD_ITEM_TO_COLL";
    if (op == tRmvItemFromColl)
      return "RMV_ITEM_FROM_COLL";
    if (op == tSetItemInColl)
      return "SET_ITEM_IN_COLL";
    if (op == tUnsetItemInColl)
      return "UNSET_ITEM_IN_COLL";
    if (op == tGetItemAt)
      return "GET_ITEM_AT";
    if (op == tGetOidItemAt)
      return "GETOID_ITEM_AT";
    if (op == tRetrieveItemAt)
      return "RETRIEVE_ITEM_AT";
    if (op == tRetrieveOidItemAt)
      return "RETRIEVEOID_ITEM_AT";
    if (op == tCast)
      return "CAST";
    if (op == tSafeCast)
      return "SAFE_CAST";
    abort();
  }

  Status GenCodeHints::Style::compile(Desc *d)
  {
    char c, *p, *q;

    p = d->fmt;
    d->comp.cnt = 0;
    q = d->comp.cfmt = (char *)malloc(strlen(d->fmt)+1);

    while (c =*p) {
      if (c == '%') {
	Desc::CompDesc::Item *item = &d->comp.items[d->comp.cnt++];

	*q++ = *p++;
	c = *p;
	switch (c) {
	case 'c': case 'C':
	  item->fun = capitalize_fun;
	  *q++ = 's'; break;

	case 'u': case 'U':
	  item->fun = upper_fun;
	  *q++ = 's'; break;

	case 'l': case 'L':
	  item->fun = lower_fun;
	  *q++ = 's'; break;

	case 'i': case 'I':
	  item->fun = ident_fun;
	  *q++ = 's'; break;

	default:
	  return Exception::make(IDB_ERROR,
				 "invalid style format '%s'",
				 d->fmt);
	}

	c = *++p;
	switch(c) {
	case 'n': case 'N':
	  item->which = Desc::CompDesc::Item::Name; break;

	case 'p': case 'P':
	  item->which = Desc::CompDesc::Item::Prefix; break;

	default:
	  return Exception::make(IDB_ERROR,
				 "invalid style format '%s'",
				 d->fmt);
	}

	p++;
      }
      else
	*q++ = *p++;
    }

    *q = 0;

    return Success;
  }

  Status GenCodeHints::Style::compile()
  {
    for (int i = 0; i < tLastOp; i++) {
      Desc *d = &desc[i];
      if (!d->fmt)
	return Exception::make(IDB_ERROR,
			       "format is not set for operation '%s'",
			       opTypeStr((OpType)i)); // opTypeStr(d->op));
      Status s = compile(d);
      if (s)
	return s;
    }

    return Success;
  }

  const char *
  GenCodeHints::Style::getString(GenCodeHints::OpType op,
				 const char *name,
				 const char *prefix)
  {
    if (status)
      return "";

    static char str[256];
    Desc *d = &desc[op];
    const char *s[4] = {0};
    for (int i = 0; i < d->comp.cnt; i++) {
      Desc::CompDesc::Item *item = &d->comp.items[i];
      if (item->which == Desc::CompDesc::Item::Name)
	s[i] = item->fun(name);
      else if (item->which == Desc::CompDesc::Item::Prefix)
	s[i] = item->fun(prefix);
      else
	s[i] = "";
    }
  
    sprintf(str, d->comp.cfmt, s[0], s[1], s[2], s[3]);

    for (;;) {
      if (map_rw.find(str) == map_rw.end())
	break;
      strcat(str, "_");
    }
    return str;
  }

  GenCodeHints::Style::~Style()
  {
  }
}
