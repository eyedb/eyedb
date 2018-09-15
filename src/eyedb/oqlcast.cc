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


#include <unistd.h>
#include <fcntl.h>
#include <wctype.h>
#include <sys/stat.h>

#include "oql_p.h"

namespace eyedb {

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlStringOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlStringOp::oqmlStringOp(oqmlNode * _ql) : oqmlNode(oqmlSTRINGOP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_STRING;
  eval_type.cls = 0;
  eval_type.comp = oqml_True;
}

oqmlStringOp::~oqmlStringOp()
{
}

oqmlStatus *oqmlStringOp::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);

  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlStringOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  if (al->cnt == 1)
    {
      const char *str = al->first->getString();

      if (al->first->as_char())
	{
	  char *x = strdup(&str[1]);
	  x[strlen(x)-1] = 0;
	  *alist = new oqmlAtomList(new oqmlAtom_string(x));
	  free(x);
	}
      else
	*alist = new oqmlAtomList(new oqmlAtom_string(str));
    }
  else // should not
    *alist = new oqmlAtomList(new oqmlAtom_string(al->getString()));

  return oqmlSuccess;
}

void oqmlStringOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlStringOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlStringOp::toString(void) const
{
  return std::string("(string ") + ql->toString() + ")";
}

// ---------------------------------------------------------------------------
//
// STD_CAST_EVAL: macro used by IntOp, CharOp and FloatOp cast operators
//
// ---------------------------------------------------------------------------

#define STD_CAST_EVAL(ATOM_CONSTRUCT, ASC2ATOM) \
 \
  oqmlStatus *s; \
 \
  oqmlAtomList *al; \
  s = ql->eval(db, ctx, &al); \
 \
  if (s) \
    return s; \
 \
  if (al->cnt > 1) \
    return oqmlStatus::expected(this, "integer, character, float or string", \
				al->first->type.getString()); \
 \
  if (!al->cnt) \
    return new oqmlStatus(this, "integer, character, float or string expected"); \
  oqmlAtom *a = al->first; \
 \
  if (a->as_int()) \
    *alist = new oqmlAtomList(new ATOM_CONSTRUCT(OQML_ATOM_INTVAL(a))); \
  else if (a->as_char()) \
    *alist = new oqmlAtomList(new ATOM_CONSTRUCT(OQML_ATOM_CHARVAL(a))); \
  else if (a->as_double()) \
    *alist = new oqmlAtomList(new ATOM_CONSTRUCT(OQML_ATOM_DBLVAL(a))); \
  else if (a->as_string()) \
    *alist = new oqmlAtomList(new ATOM_CONSTRUCT(ASC2ATOM(OQML_ATOM_STRVAL(a)))); \
  else \
    return oqmlStatus::expected(this, "integer, character, float or string", \
				a->type.getString()); \
 \
  return oqmlSuccess

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIntOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlIntOp::oqmlIntOp(oqmlNode * _ql) : oqmlNode(oqmlINTOP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_INT;
  eval_type.cls = 0;
}

oqmlIntOp::~oqmlIntOp()
{
}

oqmlStatus *oqmlIntOp::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlIntOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  STD_CAST_EVAL(oqmlAtom_int, atoi);
}

void oqmlIntOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlIntOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlIntOp::toString(void) const
{
  return std::string("(int ") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCharOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlCharOp::oqmlCharOp(oqmlNode * _ql) : oqmlNode(oqmlCHAROP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_CHAR;
  eval_type.cls = 0;
}

oqmlCharOp::~oqmlCharOp()
{
}

oqmlStatus *oqmlCharOp::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

static char
atoc(const char *s)
{
  return strlen(s) == 1 ? s[0] : 0;
}

oqmlStatus *oqmlCharOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  STD_CAST_EVAL(oqmlAtom_char, atoc);
}

void oqmlCharOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlCharOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlCharOp::toString(void) const
{
  return std::string("(char ") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlFloatOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlFloatOp::oqmlFloatOp(oqmlNode * _ql) : oqmlNode(oqmlFLOATOP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_DOUBLE;
  eval_type.cls = 0;
}

oqmlFloatOp::~oqmlFloatOp()
{
}

oqmlStatus *oqmlFloatOp::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlFloatOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  STD_CAST_EVAL(oqmlAtom_double, atof);
}

void oqmlFloatOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlFloatOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlFloatOp::toString(void) const
{
  return std::string("(float ") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIdentOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlIdentOp::oqmlIdentOp(oqmlNode * _ql) : oqmlNode(oqmlIDENTOP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_IDENT;
  eval_type.cls = 0;
}

oqmlIdentOp::~oqmlIdentOp()
{
}

oqmlStatus *oqmlIdentOp::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlIdentOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  if (al->cnt > 1)
    return oqmlStatus::expected(this, "string",	al->first->type.getString());

  if (!al->cnt)
    return new oqmlStatus(this, "string expected");

  oqmlAtom *a = al->first;

  if (a->as_string())
    *alist = new oqmlAtomList(new oqmlAtom_ident(OQML_ATOM_STRVAL(a)));
  else if (a->as_ident())
    *alist = new oqmlAtomList(a);
  else
    return oqmlStatus::expected(this, "string", a->type.getString());
  return oqmlSuccess;
}

void oqmlIdentOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlIdentOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlIdentOp::toString(void) const
{
  return std::string("(ident ") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIdentOp methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlOidOp::oqmlOidOp(oqmlNode * _ql) : oqmlNode(oqmlOIDOP)
{
  ql = _ql;
  eval_type.type = oqmlATOM_OID;
  eval_type.cls = 0;
}

oqmlOidOp::~oqmlOidOp()
{
}

oqmlStatus *oqmlOidOp::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlOidOp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  if (al->cnt > 1)
    return oqmlStatus::expected(this, "string",	al->first->type.getString());

  if (!al->cnt)
    return new oqmlStatus(this, "string expected");

  oqmlAtom *a = al->first;

  if (a->as_string())
    *alist = new oqmlAtomList(new oqmlAtom_oid(Oid(OQML_ATOM_STRVAL(a))));
  else if (a->as_oid())
    *alist = new oqmlAtomList(a);
  else
    return oqmlStatus::expected(this, "string",	a->type.getString());

  return oqmlSuccess;
}

void oqmlOidOp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlOidOp::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlOidOp::toString(void) const
{
  return std::string("(oid ") + ql->toString() + ")";
}

}
