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


#include "oql_p.h"

namespace eyedb {

#if 0
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlTimeToString methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlTimeToString::oqmlTimeToString(oqml_List * _list) : oqmlNode(oqmlTIMETOSTRING)
{
  list = _list;
}

oqmlTimeToString::~oqmlTimeToString()
{
}

oqmlStatus *oqmlTimeToString::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  s = ql->compile(db, ctx);

  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlTimeToString::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  if (al->cnt != 1 || al->first->type.type != oqmlATOM_INT)
    return new oqmlStatus("invalid argument for strtime(int)");

  eyedblib::int32 t = ((oqmlAtom_int *)al->first)->i;
  char *str = ctime((time_t *)&t);
  str[strlen(str)-1] = 0;
  *alist = new oqmlAtomList(new oqmlAtom_string(str));
  return oqmlSuccess;
}

void oqmlTimeToString::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_INT;
  at->cls = 0;
  at->comp = oqml_False;
}

oqmlBool oqmlTimeToString::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlTimeToString::toString(void) const
{
  return std::string("strtime(") + ql->toString() + ")" + oqml_isstat();
}
#endif

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlTimeFormat methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlTimeFormat::oqmlTimeFormat(oqml_List * _list) : oqmlNode(oqmlTIMEFORMAT)
{
  list = _list;
}

oqmlTimeFormat::~oqmlTimeFormat()
{
}

static oqmlStatus *
usage()
{
  return new oqmlStatus("usage: timeformat(time [, format])");
}

oqmlStatus *oqmlTimeFormat::compile(Database *db, oqmlContext *ctx)
{
  if (list->cnt != 1 && list->cnt != 2)
    return usage();

  time = list->first->ql;
  format = list->cnt == 2 ? list->first->next->ql : 0;
  
  oqmlStatus *s;
  s = time->compile(db, ctx);
  if (s) return s;

  return format ? format->compile(db, ctx) : oqmlSuccess;
}

oqmlStatus *oqmlTimeFormat::eval(Database *db, oqmlContext *ctx,
			     oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  s = time->eval(db, ctx, &al);

  if (s) return s;

  if (al->cnt != 1 || !OQML_IS_INT(al->first))
    return usage();

  struct tm *t = localtime((time_t *)&(OQML_ATOM_INTVAL(al->first)));

  // must check second argument
  // if no argument => ctime
  const char *fmt = 0;
  if (format)
    {
      oqmlAtomList *fmtlist;
      s = format->eval(db, ctx, &fmtlist);
      if (s) return s;
      if (fmtlist->cnt != 1 || !OQML_IS_STRING(fmtlist->first))
	return usage();
      fmt = OQML_ATOM_STRVAL(fmtlist->first);
    }

  char str[512];
  if (!strftime(str, sizeof(str)-1, fmt, t))
    *alist = new oqmlAtomList(new oqmlAtom_string("<time format error>"));
  else
    *alist = new oqmlAtomList(new oqmlAtom_string(str));

  return oqmlSuccess;
}

void oqmlTimeFormat::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_STRING;
  at->cls = 0;
  at->comp = oqml_True;
}

oqmlBool oqmlTimeFormat::isConstant() const
{
  return OQMLBOOL(time->isConstant());
}

std::string
oqmlTimeFormat::toString(void) const
{
  return std::string("timeformat(") + time->toString() +
    (format ? std::string(", ") + format->toString() : std::string("")) +
    ")" + oqml_isstat();
}
}
