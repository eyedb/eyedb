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
// oqmlColl operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace eyedb {

oqmlColl::oqmlColl(oqml_List * _list, oqmlTYPE _type) : oqmlNode(_type)
{
  list = _list;
}

oqmlColl::~oqmlColl()
{
  delete list;
}

oqmlStatus *oqmlColl::compile(Database *db, oqmlContext *ctx)
{
  if (!list)
    return oqmlSuccess;

  oqml_Link *l = list->first;

  while (l)
    {
      oqmlStatus *s;

      if ((s = l->ql->compile(db, ctx)) != oqmlSuccess)
	return s;

      l = l->next;
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlColl::eval(Database *db, oqmlContext *ctx,
			 oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (!list)
    {
      *alist = new oqmlAtomList(makeAtomColl(new oqmlAtomList()));
      return oqmlSuccess;
    }

  oqml_Link *l = list->first;
  oqmlAtomList *al = new oqmlAtomList();

  while (l)
    {
      oqml_Link *next = l->next;
      oqmlAtomList *tal;
      oqmlStatus *s;

      tal = 0;
      if ((s = l->ql->eval(db, ctx, &tal)) != oqmlSuccess)
	return s;

      al->append(tal);

      l = next;
    }

  *alist = new oqmlAtomList(makeAtomColl(al));
  return oqmlSuccess;
}

void oqmlColl::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlColl::isConstant() const
{
  return oqml_False;
}

std::string
oqmlColl::toString(void) const
{
  std::string s = std::string(getTypeName()) + "(";

  if (list)
    {
      oqml_Link *l = list->first;

      for (int n = 0; l; n++)
	{
	  if (n) s += ",";
	  s += l->ql->toString();
	  l = l->next;
	}
    }

  return s + ")" + oqml_isstat();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlListColl operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlListColl::oqmlListColl(oqml_List * _list) : oqmlColl(_list, oqmlLISTCOLL)
{
  eval_type.type = oqmlATOM_LIST;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlSetColl operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlSetColl::oqmlSetColl(oqml_List * _list) : oqmlColl(_list, oqmlSETCOLL)
{
  eval_type.type = oqmlATOM_SET;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlBagColl operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlBagColl::oqmlBagColl(oqml_List * _list) : oqmlColl(_list, oqmlBAGCOLL)
{
  eval_type.type = oqmlATOM_BAG;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlArrayColl operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlArrayColl::oqmlArrayColl(oqml_List * _list) : oqmlColl(_list, oqmlARRAYCOLL)
{
  eval_type.type = oqmlATOM_ARRAY;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlStruct operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlStruct::oqmlStruct(oqml_IdentList *_list) : oqmlNode(oqmlSTRUCT)
{
  list = _list;
}

oqmlStatus *oqmlStruct::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  oqml_IdentLink *l = list->first;

  while (l)
    {
      s = l->ql->compile(db, ctx);
      if (s) return s;
      l = l->next;
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlStruct::eval(Database *db, oqmlContext *ctx,
			     oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqml_StructAttr *attr = new oqml_StructAttr[list->cnt];
  oqml_IdentLink *l = list->first;
  for (int n = 0; l; n++)
    {
      oqmlAtomList *al;
      s = l->ql->eval(db, ctx, &al);
      if (s)
	{
	  delete[] attr;
	  return s;
	}

      if (l->left->getType() != oqmlIDENT)
	{
	  delete[] attr;
	  return oqmlStatus::expected(this, "identifier",
				      l->left->toString().c_str());
	}

      attr[n].set(((oqmlIdent *)l->left)->getName(), al->first);
      l = l->next;
    }

  *alist = new oqmlAtomList(new oqmlAtom_struct(attr, list->cnt));
  return oqmlSuccess;
}

void oqmlStruct::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_STRUCT;
  at->cls = 0;
  at->comp = oqml_False;
}

oqmlBool oqmlStruct::isConstant() const
{
  return oqml_False;
}

oqmlBool
oqmlStruct::hasIdent(const char *ident)
{
  oqml_IdentLink *ln = list->first;

  while (ln)
    {
      if (ln->ql->hasIdent(ident))
	return oqml_True;
      ln = ln->next;
    }

  return oqml_False;
}

void oqmlStruct::lock()
{
  oqmlNode::lock();
  if (list) list->lock();
}

void oqmlStruct::unlock()
{
  oqmlNode::unlock();
  if (list) list->unlock();
}

std::string
oqmlStruct::toString(void) const
{
  std::string s = "struct(";

  oqml_IdentLink *l = list->first;
  for (int n = 0; l; n++)
    {
      if (n) s += ", ";
      s += l->left->toString() + ": " + l->ql->toString();
      l = l->next;
    }

  return s + ")" + oqml_isstat();
}

oqmlStruct::~oqmlStruct()
{
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary structof operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlStructOf::oqmlStructOf(oqmlNode * _ql) : oqmlNode(oqmlSTRUCTOF)
{
  ql = _ql;
  eval_type.type = oqmlATOM_STRING;
  eval_type.comp = oqml_True;
}

oqmlStructOf::~oqmlStructOf()
{
}

oqmlStatus *oqmlStructOf::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlStructOf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (!al->cnt)
    return oqmlStatus::expected(this, "struct", "nil");

  oqmlAtom *r = al->first;
  if (!OQML_IS_STRUCT(r))
    return oqmlStatus::expected(this, "struct", r->type.getString());

  oqmlAtom_struct *as = r->as_struct();

  oqmlAtomList *list = new oqmlAtomList();

  for (int i = 0; i < as->attr_cnt; i++)
    list->append(new oqmlAtom_string(as->attr[i].name));

  *alist = new oqmlAtomList(new oqmlAtom_list(list));

  return oqmlSuccess;
}

void oqmlStructOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlStructOf::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlStructOf::toString(void) const
{
  return oqml_unop_string(ql, "structof ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlFlatten operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void
oqml_flatten_realize(oqmlAtom *a, int flat, oqmlAtomList *alist)
{
  if (a->type.type == oqmlATOM_LIST && flat) {
    oqmlAtom *ta = ((oqmlAtom_list *)a)->list->first;
    while (ta) {
      oqmlAtom *next = ta->next;
      oqml_flatten_realize(ta, flat, alist);
      ta = next;
    }
  }
  else
    alist->append(a);
}

oqmlFlatten::oqmlFlatten(oqml_List * _list, int _flat) : oqmlNode(oqmlFLATTEN)
{
  list = _list;
  flat = _flat;
}

oqmlFlatten::~oqmlFlatten()
{
  delete list;
}

oqmlStatus *oqmlFlatten::compile(Database *db, oqmlContext *ctx)
{
  if (!list)
    return oqmlSuccess;

  oqml_Link *l = list->first;

  oqmlBool first = oqml_True;
  oqmlBool do_ev_type = oqml_True;

  while (l)
    {
      oqmlStatus *s;

      if ((s = l->ql->compile(db, ctx)) != oqmlSuccess)
	return s;

      if (do_ev_type)
	{
	  oqmlAtomType at;
	  l->ql->evalType(db, ctx, &at);
	  
	  if (first)
	    {
	      first = oqml_False;
	      eval_type = at;
	    }
	  else if (eval_type.type != at.type)
	    {
	      eval_type.type = oqmlATOM_UNKNOWN_TYPE;
	      eval_type.cls = 0;
	      do_ev_type = oqml_False;
	    }
	}

      l = l->next;
    }

  // kludge added the 8/02/99
  if (eval_type.type == oqmlATOM_LIST)
    {
      eval_type.type = oqmlATOM_UNKNOWN_TYPE;
      eval_type.cls = 0;
      do_ev_type = oqml_False;
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlFlatten::eval(Database *db, oqmlContext *ctx,
			      oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  *alist = new oqmlAtomList();

  if (!list)
    return oqmlSuccess;

  oqmlStatus *s;
  oqml_Link *l = list->first;

  while (l)
    {
      oqmlAtomList *tal;
      tal = 0;
      if ((s = l->ql->eval(db, ctx, &tal)) != oqmlSuccess)
	{
	  delete tal;
	  delete *alist;
	  return s;
	}

      oqmlAtom *a = tal->first;
  
      while (a)
	{
	  oqmlAtom *next = a->next;
	  oqml_flatten_realize(a, flat, *alist);
	  a = next;
	}

      l = l->next;
    }
  return oqmlSuccess;
}

void oqmlFlatten::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlFlatten::isConstant() const
{
  return oqml_False;
}

std::string
oqmlFlatten::toString(void) const
{
  std::string s = std::string("flatten(");
  oqml_Link *l = list->first;

  for (int n = 0; l; n++)
    {
      if (n) s += ",";
      s += l->ql->toString();
      l = l->next;
    }

  return s + ")" + oqml_isstat();
}

void
oqmlFlatten::lock()
{
  oqmlNode::lock();
  if (!list)
    return;

  list->lock();
}

void
oqmlFlatten::unlock()
{
  oqmlNode::unlock();
  if (!list)
    return;

  list->unlock();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCount operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlStatus *oqmlCount::eval(Database *db, oqmlContext *ctx,
			  oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  s = ql->eval(db, ctx, &al);

  if (s)
    return s;

  *alist = new oqmlAtomList(new oqmlAtom_int(al->cnt));

  return oqmlSuccess;
}

void oqmlCount::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  at->type = oqmlATOM_INT;
  at->cls = 0;
  at->comp = oqml_False;
}

oqmlBool oqmlCount::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

oqmlCount::oqmlCount(oqmlNode * _ql) : oqmlNode(oqmlCOUNT)
{
  ql = _ql;
}

oqmlCount::~oqmlCount()
{
}

oqmlStatus *oqmlCount::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  s = ql->compile(db, ctx);

  if (s)
    return s;

  return oqmlSuccess;
}

std::string
oqmlCount::toString(void) const
{
  return std::string("count(") + ql->toString() + ")" + oqml_isstat();
}
}
