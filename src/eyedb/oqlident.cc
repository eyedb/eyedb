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

// added the 7/5/01
#define DELAY_IDENT

namespace eyedb {

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIdent utility functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//
// 25/05/01: changed following code to support constructs such as:
// for (x in oql$functions) s += (bodyof x);
//

#define NEW_OPIDENT_PREEVAL

oqmlStatus *
oqml_opident_compile(oqmlNode *, Database *db, oqmlContext *ctx,
		     oqmlNode *ql, char *&ident)
{
  free(ident); ident = 0;

  oqmlStatus *s = ql->compile(db, ctx);
  if (s) return s;

  if (ql->getType() == oqmlIDENT)
    ident = strdup(((oqmlIdent *)ql)->getName());
      
  return oqmlSuccess;
}

oqmlStatus *
oqml_opident_preeval(oqmlNode *node, Database *db, oqmlContext *ctx,
		     oqmlNode *ql, char *&ident)
{
#ifdef NEW_OPIDENT_PREEVAL
  free(ident); ident = 0;
#else
  if (ident)
    return oqmlSuccess;
#endif

  oqmlAtomList *al;
  oqmlStatus *s = ql->eval(db, ctx, &al);
  if (s) return s;
  if (al->cnt != 1 || !OQML_IS_IDENT(al->first))
    return oqmlStatus::expected(node, "identifier",
				al->cnt ? al->first->type.getString() : 
				"nil");

#ifdef NEW_OPIDENT_PREEVAL
  char *id = OQML_ATOM_IDENTVAL(al->first);
  oqmlAtomType type;
  oqmlAtom *at;
  if (ctx->getSymbol(id, &type, &at, 0, 0)) {
    if (!at || !OQML_IS_IDENT(at))
      return oqmlStatus::expected(node, "identifier", type.getString());
    ident = strdup(OQML_ATOM_IDENTVAL(at));
    return oqmlSuccess;
  }
#endif

  ident = strdup(OQML_ATOM_IDENTVAL(al->first));
  return oqmlSuccess;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIdent methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

const char oqml_uninit_fmt[] = "uninitialized identifier '%s'";

// -----------------------------------------------------------------------
//
// oqmlIdent constructor
//
// -----------------------------------------------------------------------

oqmlIdent::oqmlIdent(const char * _name) : oqmlNode(oqmlIDENT)
{
  name = strdup(_name);
  cls = 0;
  __class = 0;
  cst_atom = 0;
}

// -----------------------------------------------------------------------
//
// oqmlIdent destructor
//
// -----------------------------------------------------------------------

oqmlIdent::~oqmlIdent()
{
  free(name);
}

// -----------------------------------------------------------------------
//
// oqmlIdent compile public method
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlIdent::compile(Database *db, oqmlContext *ctx)
{
  if (ctx->getDotContext())
    return oqmlSuccess;

  oqmlAtom *atom;

  oqmlAtomType at;
      
  if (ctx->getSymbol(name, &at, &atom))
    {
      if (at.type != oqmlATOM_SELECT)
	eval_type = at;
      cls = 0;
      return oqmlSuccess;
    }

  if (ctx->isSelectContext())
    {
      // added the 7/01/00
      const char *xname = name;
      /*
      if (!strcmp(name, "schema"))
	xname = "scheme";
	*/
      // ...

      cls = db->getSchema()->getClass(xname);
      
      if (cls)
	{
	  eval_type.type = oqmlATOM_OID;
	  eval_type.cls = cls;
	}
      else
	// changed the 18/10/99
	// eval_type.type = oqmlATOM_IDENT;
	eval_type.type = oqmlATOM_UNKNOWN_TYPE;
    }
  else
    // changed the 18/10/99
    //eval_type.type = oqmlATOM_IDENT;
    eval_type.type = oqmlATOM_UNKNOWN_TYPE;
  
  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// oqmlIdent eval public method
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlIdent::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist,
		oqmlComp *, oqmlAtom *)
{
  oqmlAtom *at;
  oqmlNode *ql;
  oqmlAtomType t;
  oqmlFunctionEntry *entry;

  if (cst_atom)
    {
      *alist = new oqmlAtomList(cst_atom->copy());
      return oqmlSuccess;
    }

  if (cls)
    return evalQuery(db, ctx, alist);

  if (ctx->getSymbol(name, &t, &at))
    {
      if (!at)
	{
	  *alist = new oqmlAtomList();
	  return oqmlSuccess;
	}

      if (at->type.type == oqmlATOM_SELECT)
	{
	  if (!at->as_select()->list)
	    {
	      oqmlStatus *s = at->as_select()->node->eval(db, ctx, alist);
	      if (s) return s;
	      at->as_select()->list = (*alist)->copy();
	      return oqmlSuccess;
	    }
	  // added the 1/03/00
	  else
	    *alist = at->as_select()->list->copy();
	}
      else {
	if (OQML_IS_OBJ(at)) {
	  OQL_CHECK_OBJ(at);
	}
	*alist = new oqmlAtomList(at->copy());
      }

      return oqmlSuccess;
    }

  /* disconnected the 23/01/00
  if (ctx->getFunction(name, &entry))
    {
      *alist = new oqmlAtomList(new oqmlAtom_ident(name));
      return oqmlSuccess;
    }

  if (oqmlCall::getBuiltIn(db, name))
    {
      *alist = new oqmlAtomList(new oqmlAtom_ident(name));
      return oqmlSuccess;
    }
    */

  if (ctx->getFunction(name, &entry)) // should check here the argument number!
    return oqmlCall::realizeCall(db, ctx, entry, alist);

  return new oqmlStatus(this, oqml_uninit_fmt, name);
}

oqmlStatus *oqmlIdent::evalLeft(Database *db, oqmlContext *ctx,
				oqmlAtom **a, int &idx)
{
  idx = -1;
  *a = new oqmlAtom_ident(name);
  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// oqmlIdent evalType public method
//
// -----------------------------------------------------------------------

void oqmlIdent::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

// -----------------------------------------------------------------------
//
// oqmlIdent evalType public method
//
// -----------------------------------------------------------------------

oqmlBool oqmlIdent::isConstant() const
{
  return oqml_False;
}

// -----------------------------------------------------------------------
//
// oqmlIdent evalType getName method
//
// -----------------------------------------------------------------------

const char *oqmlIdent::getName() const
{
  return name;
}

// -----------------------------------------------------------------------
//
// oqmlIdent toString public method
//
// -----------------------------------------------------------------------

std::string
oqmlIdent::toString(void) const
{
  return std::string(name) + oqml_isstat();
}

// -----------------------------------------------------------------------
//
// oqmlIdent initEnumValues public method
//
// -----------------------------------------------------------------------

void
oqmlIdent::initEnumValues(Database *db, oqmlContext *ctx)
{
  const LinkedList *list = db->getSchema()->getClassList();
  LinkedListCursor c(list);
  Class *cl;
  
  while (c.getNext((void *&)cl))
    if (cl->asEnumClass()) {
      int item_cnt;
      const EnumItem **items = cl->asEnumClass()->getEnumItems(item_cnt);
      for (int i = 0; i < item_cnt; i++) {
	oqmlAtom *atom = new oqmlAtom_int(items[i]->getValue());
	ctx->setSymbol(items[i]->getName(), &atom->type, atom, oqml_True,
		       oqml_True);
      }
    }
}

// -----------------------------------------------------------------------
//
// oqmlIdent evalQuery private method
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlIdent::evalQuery(Database *db, oqmlContext *ctx, oqmlAtomList **xalist)
{
  oqmlAtomList *al;
  OQML_MAKE_RBAG(xalist, rlist);

  if (al = ctx->getAndContext())
    {
      oqmlAtom *a = al->first;
      while (a)
	{
	  oqmlAtom *next = a->next;
	      
	  if (a->type.type != oqmlATOM_OID)
	    return oqmlStatus::expected(this, "oid", a->type.getString());
	      
	  Status status;
	  Bool is;
	  status = cls->isObjectOfClass(&((oqmlAtom_oid *)a)->oid,
					&is, True);
	  if (status)
	    return new oqmlStatus(this, status);
	      
	  if (is)
	    rlist->append(a);
	      
	  a = next;
	}

      return oqmlSuccess;
    }

  if ((void *)cls == db->getSchema()->Schema_Class)
    {
      const LinkedList *_class = db->getSchema()->getClassList();
      LinkedListCursor c(_class);
      Class *cl;
      while (c.getNext((void *&)cl))
	if (!cl->asCollectionClass() && !cl->isSystem())
	  {
	    rlist->append(new oqmlAtom_oid(cl->getOid(), cl));
	    OQML_CHECK_MAX_ATOMS(rlist, ctx, 0);
	  }

      return oqmlSuccess;
    }

  Iterator *q = new Iterator(cls, True);
  if (q->getStatus())
    {
      Status s = q->getStatus();
      delete q;
      return new oqmlStatus(this, s);
    }

  return oqml_scan(ctx, q, cls, rlist);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unset unary operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlUnset::oqmlUnset(oqmlNode * _ql) : oqmlNode(oqmlUNSET)
{
  ql = _ql;
  ident = 0;
}

oqmlUnset::~oqmlUnset()
{
  free(ident);
}

oqmlStatus *oqmlUnset::compile(Database *db, oqmlContext *ctx)
{
  if (ql->getType() == oqmlIDENT)
    {
      ident = strdup(((oqmlIdent *)ql)->getName());
      return oqmlSuccess;
    }

  if (ql->getType() == oqmlARRAY)
    return ql->compile(db, ctx);

  return new oqmlStatus(this, "identifier or array element expected");
}

oqmlStatus *oqmlUnset::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  *alist = new oqmlAtomList();

  if (ident)
    {
      oqmlAtomType at;
      oqmlBool global;

      if (ctx->getSymbol(ident, 0, 0, &global))
	{
	  if (!global)
	    return new oqmlStatus(this, "cannot unset local symbol '%s'",
				  ident);
	  ctx->popSymbol(ident, global);
	}

      return oqmlSuccess;
    }

  return oqmlSuccess;
}

void oqmlUnset::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlUnset::isConstant() const
{
  return oqml_False;
}

std::string
oqmlUnset::toString(void) const
{
  if (is_statement)
    return std::string("unset ") + ql->toString() + "; ";
  return std::string("(unset ") + ql->toString() + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// isset unary operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlIsset::oqmlIsset(oqmlNode *_ql) : oqmlNode(oqmlISSET)
{
  ql = _ql;
  ident = 0;
  eval_type.type = oqmlATOM_BOOL;
}

oqmlIsset::~oqmlIsset()
{
  free(ident);
}

oqmlStatus *oqmlIsset::compile(Database *db, oqmlContext *ctx)
{
  return oqml_opident_compile(this, db, ctx, ql, ident);
}

oqmlStatus *oqmlIsset::eval(Database *db, oqmlContext *ctx,
			  oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s = oqml_opident_preeval(this, db, ctx, ql, ident);
  if (s) return s;

  oqmlAtomType at;

  if (ctx->getSymbol(ident, &at))
    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
  else
    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));

  return oqmlSuccess;
}

void oqmlIsset::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlIsset::isConstant() const
{
  return oqml_False;
}

std::string
oqmlIsset::toString(void) const
{
  const char *sident = (ident ? ident : "<null>");
  if (is_statement)
    return std::string("isset ") + sident + "; ";
  return std::string("(isset ") + sident + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlValRefOf methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char *
oqmlValRefOf::makeIdent(oqmlContext *ctx, const char *ident)
{
  if (strncmp(ident, oqml_global_scope, oqml_global_scope_len))
    {
      oqmlBool global;
      if (!ctx->getSymbol(ident, 0, 0, &global) || global)
	return strdup((std::string(oqml_global_scope) + ident).c_str());
    }

  return strdup(ident);
}

oqmlValRefOf::oqmlValRefOf(oqmlNode * _ql, oqmlTYPE _type,
			   const char *_opstr) : oqmlNode(_type)
{
  ql = _ql;
  ident = 0;
  opstr = strdup(_opstr);
}

oqmlValRefOf::~oqmlValRefOf()
{
  free(ident);
  free(opstr);
}

oqmlStatus *oqmlValRefOf::realizeIdent(Database *db, oqmlContext *ctx)
{
  if (ident)
    return oqmlSuccess;

  oqmlAtom *a;
  int idx;
  oqmlStatus *s;

  if (ql->getType() == oqmlREFOF)
    {
      oqmlRefOf *refof = (oqmlRefOf *)ql;
      ident = refof->ident ? strdup(refof->ident) : 0;

      if (ident)
	return oqmlSuccess;

      oqmlAtomList *al;
      s = ql->eval(db, ctx, &al);
      if (s) return s;
      if (al->cnt != 1)
	return new oqmlStatus(this, "identifier expected.");
      a = al->first;
      idx = 0;
    }
  else
    {
      s = ql->evalLeft(db, ctx, &a, idx);
      if (s) return s;
    }

  if (a->as_ident())
    {
      ident = makeIdent(ctx, OQML_ATOM_IDENTVAL(a));
      return oqmlSuccess;
    }

  if (a->as_list())
    {
      oqmlAtom *x = a->as_list()->list->getAtom(idx);
      if (x->as_ident())
	{
	  char *xident = makeIdent(ctx, OQML_ATOM_IDENTVAL(x));
	  oqmlAtom_ident *tmp = new oqmlAtom_ident(xident);
	  free(xident);

	  const char *tmp_ident = ctx->getTempSymb().c_str();
	  oqmlAtomType tident(oqmlATOM_IDENT);
	  ctx->pushSymbol(tmp_ident, &tident, tmp, oqml_False);

	  ident = strdup(tmp_ident);
	  return oqmlSuccess;
	}
    }

  return new oqmlStatus(this, "identifier expected.");
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlRefOf methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlRefOf::oqmlRefOf(oqmlNode * _ql) : oqmlValRefOf(_ql, oqmlREFOF, "&")
{
}

oqmlRefOf::~oqmlRefOf()
{
}

oqmlStatus *oqmlRefOf::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  free(ident); ident = 0;

  s = ql->compile(db, ctx);
  if (s)
    return s;

#ifndef DELAY_IDENT
  if (ql->getType() == oqmlIDENT)
    ident = makeIdent(ctx, ((oqmlIdent *)ql)->getName());
#endif
      
  return oqmlSuccess;
}

oqmlStatus *oqmlRefOf::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
#ifdef DELAY_IDENT
  if (ql->getType() == oqmlIDENT && !ident)
    ident = makeIdent(ctx, ((oqmlIdent *)ql)->getName());
#endif

  oqmlStatus *s;

  s = realizeIdent(db, ctx);
  if (s) return s;

  *alist = new oqmlAtomList(new oqmlAtom_ident(ident,
					       ctx->getSymbolEntry(ident)));
  return oqmlSuccess;
}

oqmlStatus *oqmlRefOf::evalLeft(Database *db, oqmlContext *ctx,
				oqmlAtom **a, int &idx)
{
  /*
  oqmlStatus *s = realizeIdent(db, ctx);
  if (s) return s;

  idx = -1;
  *a = new oqmlAtom_ident(ident, ctx->getSymbolEntry(ident));
  return oqmlSuccess;
  */

  return new oqmlStatus(this, "not a left value.");
}

void oqmlRefOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlRefOf::isConstant() const
{
  return oqml_False;
}

std::string
oqmlRefOf::toString(void) const
{
  return oqml_unop_string(ql, "&", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlValOf methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlValOf::oqmlValOf(oqmlNode * _ql) : oqmlValRefOf(_ql, oqmlVALOF, "*")
{
}

oqmlValOf::~oqmlValOf()
{
}

oqmlStatus *oqmlValOf::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  free(ident); ident = 0;

  s = ql->compile(db, ctx);
  if (s)
    return s;

#ifndef DELAY_IDENT
  if (ql->getType() == oqmlIDENT)
    ident = makeIdent(ctx, ((oqmlIdent *)ql)->getName());
#endif
      
  return oqmlSuccess;
}

oqmlStatus *oqmlValOf::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
#ifdef DELAY_IDENT
  if (ql->getType() == oqmlIDENT && !ident)
    ident = makeIdent(ctx, ((oqmlIdent *)ql)->getName());
#endif

  oqmlStatus *s;

  s = realizeIdent(db, ctx);
  if (s) return s;

  oqmlAtomType t;
  oqmlAtom *a = 0;

  if (!ctx->getSymbol(ident, &t, &a) || !a)
    return new oqmlStatus(this, oqml_uninit_fmt, ident);

  oqmlAtom *xa = 0;

  if (ql->getType() == oqmlREFOF)
    xa = a;
  else
    {
      if (!a->as_ident())
	return new oqmlStatus(this, "value of '%s': identifier expected, "
			      "got %s", ident, a->type.getString());

      oqmlSymbolEntry *entry = a->as_ident()->entry;

      if (entry)
	xa = entry->at;

      if (!xa && !ctx->getSymbol(OQML_ATOM_IDENTVAL(a), &t, &xa) || !a)
	return new oqmlStatus(this, oqml_uninit_fmt, OQML_ATOM_IDENTVAL(a));
    }

  *alist = new oqmlAtomList(xa);
  return oqmlSuccess;
}

oqmlStatus *oqmlValOf::evalLeft(Database *db, oqmlContext *ctx,
				oqmlAtom **a, int &idx)
{
  oqmlStatus *s = realizeIdent(db, ctx);
  if (s) return s;

  idx = -1;
  oqmlAtomType t;
  oqmlAtom *ta;

  if (!ctx->getSymbol(ident, &t, &ta) || !ta)
    return new oqmlStatus(this, "identifier '%s' is not initialized.", ident);

  if (ql->getType() == oqmlREFOF)
    *a = ta;
  else
    {
      if (!ta->as_ident())
	return new oqmlStatus(this, "value of '%s': identifier expected, "
			      "got %s", ident, ta->type.getString());

      *a = ta->as_ident();
    }

  return oqmlSuccess;
}

void oqmlValOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlValOf::isConstant() const
{
  return oqml_False;
}

std::string
oqmlValOf::toString(void) const
{
  return oqml_unop_string(ql, "*", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlScopeOf operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlScopeOf::oqmlScopeOf(oqmlNode *_ql) : oqmlNode(oqmlSCOPEOF)
{
  ql = _ql;
  ident = 0;
  eval_type.type = oqmlATOM_STRING;
}

oqmlScopeOf::~oqmlScopeOf()
{
  free(ident);
}

oqmlStatus *oqmlScopeOf::compile(Database *db, oqmlContext *ctx)
{
  return oqml_opident_compile(this, db, ctx, ql, ident);
}

oqmlStatus *oqmlScopeOf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s = oqml_opident_preeval(this, db, ctx, ql, ident);
  if (s) return s;

  oqmlBool global;

  if (ctx->getSymbol(ident, 0, 0, &global))
    {
      (*alist) = new oqmlAtomList(new oqmlAtom_string
				  (global ? "global" : "local"));
      return oqmlSuccess;
    }

  return new oqmlStatus(this, oqml_uninit_fmt, ident);
}

void oqmlScopeOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlScopeOf::isConstant() const
{
  return oqml_False;
}

std::string
oqmlScopeOf::toString(void) const
{
  if (is_statement)
    return std::string("scopeof ") + ident + "; ";
  return std::string("(scopeof ") + ident + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlPush operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlPush::oqmlPush(const char *_ident) : oqmlNode(oqmlPUSH)
{
  ident = strdup(_ident);
}

oqmlPush::~oqmlPush()
{
  free(ident);
}

oqmlStatus *oqmlPush::compile(Database *db, oqmlContext *ctx)
{
  return oqmlSuccess;
}

oqmlStatus *oqmlPush::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlAtomType at;
  oqmlStatus *s = ctx->pushSymbol(ident, &at, 0, oqml_False);
  if (s) return s;
  (*alist) = new oqmlAtomList(new oqmlAtom_ident(ident));
  return oqmlSuccess;
}

void oqmlPush::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlPush::isConstant() const
{
  return oqml_False;
}

std::string
oqmlPush::toString(void) const
{
  return std::string("(push ") + ident + ")" + oqml_isstat();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlPop operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlPop::oqmlPop(const char *_ident) : oqmlNode(oqmlPOP)
{
  ident = strdup(_ident);
}

oqmlPop::~oqmlPop()
{
  free(ident);
}

oqmlStatus *oqmlPop::compile(Database *db, oqmlContext *ctx)
{
  return oqmlSuccess;
}

oqmlStatus *oqmlPop::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtom *a = 0;
  oqmlBool global;

  if (!ctx->getSymbol(ident, 0, &a, &global))
    return new oqmlStatus(this, oqml_uninit_fmt, ident);

  if (global)
    return new oqmlStatus(this, "cannot pop global symbol '%s'", ident);

  s = ctx->popSymbol(ident, oqml_False);
  if (s) return s;
  (*alist) = new oqmlAtomList(a);
  return oqmlSuccess;
}

void oqmlPop::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlPop::isConstant() const
{
  return oqml_False;
}

std::string
oqmlPop::toString(void) const
{
  return std::string("(pop ") + ident + ")" + oqml_isstat();
}
}
