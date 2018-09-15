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

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCastIdent methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace eyedb {

oqmlCastIdent::oqmlCastIdent(const char * _name, const char *_modname) :
oqmlNode(oqmlCASTIDENT)
{
  name = strdup(_name);
  modname = strdup(_modname);
  printf("castident %s %s\n", name, modname);
}

oqmlCastIdent::~oqmlCastIdent()
{
  free(name);
  free(modname);
}

oqmlStatus *oqmlCastIdent::compile(Database *db, oqmlContext *ctx)
{
  return new oqmlStatus("cannot eval cast ident '%s:%s'.", name, modname);
}

oqmlStatus *oqmlCastIdent::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  return new oqmlStatus("cannot eval cast ident '%s:%s'.", name, modname);
}

void oqmlCastIdent::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlCastIdent::isConstant() const
{
  return oqml_False;
}

const char *oqmlCastIdent::getName(const char **_modname) const
{
  if (_modname)
    *_modname = modname;
  return name;
}

std::string
oqmlCastIdent::toString(void) const
{
  return std::string(name) + "@" + modname;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlClassOf methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlClassOf::oqmlClassOf(oqmlNode *_ql): oqmlNode(oqmlCLASSOF)
{
  ql = _ql;
  eval_type.type = oqmlATOM_STRING;
  eval_type.comp = oqml_True;
}

oqmlClassOf::~oqmlClassOf()
{
}

oqmlStatus *oqmlClassOf::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);

  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlClassOf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  *alist = new oqmlAtomList;

  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  oqmlAtom *a = al->first;
  while (a)
    {
      oqmlAtom *next = a->next;
      const Class *cls = 0;

      if (OQML_IS_OID(a))
	{	
	  Bool found;
	  const Oid &oid = OQML_ATOM_OIDVAL(a);
	  if (!oid.isValid())
	    cls = 0;
	  else
	    {
	      Status status = db->getObjectClass(oid, (Class *&)cls);
	      if (status)
		return new oqmlStatus(this, status);
	    }
	}
      else if (OQML_IS_OBJ(a)) {
	OQL_CHECK_OBJ(a);
	cls = (OQML_ATOM_OBJVAL(a) ? OQML_ATOM_OBJVAL(a)->getClass() : 0);
      }
      else if (a->as_null())
	cls = 0;
      else
	return oqmlStatus::expected(this, "oid or object",
				    a->type.getString());
      if (!cls)
	//	(*alist)->append(new oqmlAtom_oid(Oid()));
	(*alist)->append(new oqmlAtom_string(""));
      else
	(*alist)->append(new oqmlAtom_string((char *)cls->getName()));

      a = next;
    }

  return oqmlSuccess;
}

void oqmlClassOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlClassOf::isConstant() const
{
  return oqml_False;
}

std::string
oqmlClassOf::toString(void) const
{
  return std::string("classof(") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCast methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlCast::oqmlCast(const char * _ident, oqmlNode * _qright) : oqmlNode(oqmlCAST)
{
  ident = strdup(_ident);
  qright = _qright;
}

oqmlCast::~oqmlCast()
{
  delete ident;
}

oqmlStatus *oqmlCast::compile(Database *db, oqmlContext *ctx)
{
  return oqmlSuccess;
}

oqmlStatus *oqmlCast::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  return oqmlSuccess;
}

void oqmlCast::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlCast::isConstant() const
{
  return oqml_False;
}

std::string
oqmlCast::toString(void) const
{
  return "";
}


#if 0
// -----------------------------------------------------------------------
//
// static utility functions
//
// -----------------------------------------------------------------------

static oqmlStatus *
oqml_CMtime_compile(Database *db, oqmlContext *ctx, oqmlNode *ql)
{
  oqmlStatus *s;
  oqmlAtomType at;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  ql->evalType(db, ctx, &at);

  if (at.type != oqmlATOM_UNKNOWN_TYPE && at.type != oqmlATOM_OID)
    return new oqmlStatus("invalid argument for ctime(oid)");

  return oqmlSuccess;
}

static oqmlStatus *
oqml_CMtime_eval(Database *db, oqmlContext *ctx, oqmlNode *ql,
		time_t *ctime, time_t *mtime)
{
  oqmlAtomList *al;
  oqmlStatus *s;

  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (al->cnt != 1 || al->first->type.type != oqmlATOM_OID)
    return new oqmlStatus("invalid argument for ctime(oid)");

  Oid *oid = &((oqmlAtom_oid *)al->first)->oid;

  ObjectHeader hdr;

  RPCStatus rpc_status;

  if ((rpc_status = objectHeaderRead(db->getDbHandle(), oid->getOid(),
					 &hdr)) ==
      RPCSuccess)
    {
      if (ctime)
	*ctime = hdr.ctime;
      if (mtime)
	*mtime = hdr.mtime;
    }

  return oqmlSuccess;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCTime methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlCTime::oqmlCTime(oqmlNode * _ql) : oqmlNode(oqmlCTIME)
{
  ql = _ql;
}

oqmlCTime::~oqmlCTime()
{
}

oqmlStatus *oqmlCTime::compile(Database *db, oqmlContext *ctx)
{
  return oqml_CMtime_compile(db, ctx, ql);
}

oqmlStatus *oqmlCTime::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  Oid oid;
  time_t ctime;
  oqmlStatus *s = oqml_CMtime_eval(db, ctx, ql, &ctime, 0);

  if (s)
    return s;

  *alist = new oqmlAtomList(new oqmlAtom_int(ctime));
  return oqmlSuccess;
}

void oqmlCTime::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_INT;
  at->cls = 0;
  at->comp = oqml_False;
}

oqmlBool oqmlCTime::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlCTime::toString(void) const
{
  return std::string("ctime(") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlMTime methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlMTime::oqmlMTime(oqmlNode * _ql) : oqmlNode(oqmlMTIME)
{
  ql = _ql;
}

oqmlMTime::~oqmlMTime()
{
}

oqmlStatus *oqmlMTime::compile(Database *db, oqmlContext *ctx)
{
  return oqml_CMtime_compile(db, ctx, ql);
}

oqmlStatus *oqmlMTime::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  Oid oid;
  time_t mtime;
  oqmlStatus *s = oqml_CMtime_eval(db, ctx, ql, 0, &mtime);

  if (s)
    return s;

  *alist = new oqmlAtomList(new oqmlAtom_int(mtime));
  return oqmlSuccess;
}

void oqmlMTime::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_INT;
  at->cls = 0;
  at->comp = oqml_False;
}

oqmlBool oqmlMTime::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlMTime::toString(void) const
{
  return std::string("mtime(") + ql->toString() + ")";
}
#endif
}
