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

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// set utility functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static oqmlStatus *
check_bag_set(oqmlNode *node, oqmlAtomList *alist)
{
  if (alist->cnt != 1 ||
      (!OQML_IS_SET(alist->first) && !OQML_IS_BAG(alist->first)))
    return oqmlStatus::expected(node, "set or bag",
				(alist->first ? alist->first->type.getString() :
				 "nothing"));

  return oqmlSuccess;
}

static oqmlStatus *
check_bag_set(oqmlNode *node, oqmlAtom *x)
{
  if (!OQML_IS_SET(x) && !OQML_IS_BAG(x))
    return oqmlStatus::expected(node, "set or bag", x->type.getString());

  return oqmlSuccess;
}

static oqmlBool
is_in(oqmlAtom *x, oqmlAtomList *list)
{
  oqmlAtom *a = list->first;
  std::string sx = x->getString();
  while (a)
    {
      if (sx == std::string(a->getString()))
	return oqml_True;
      a = a->next;
    }

  return oqml_False;
}

static oqmlStatus *
set_make(oqmlAtom *left, oqmlAtom *right, oqmlAtomList *list,
	 oqmlAtomList **alist)
{
  if (OQML_IS_BAG(left) || OQML_IS_BAG(right))
    {
      (*alist) = new oqmlAtomList(new oqmlAtom_bag(list));
      return oqmlSuccess;
    }

  (*alist) = new oqmlAtomList(new oqmlAtom_set(list));
  return oqmlSuccess;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlUnion operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlUnion::oqmlUnion(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlUNION)
{
  qleft = _qleft;
  qright = _qright;
}

oqmlUnion::~oqmlUnion()
{
}

oqmlStatus *oqmlUnion::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s) return s;

  s = qright->compile(db, ctx);
  if (s) return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlUnion::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlAtomList *aleft;
  oqmlStatus *s;

  s = qleft->eval(db, ctx, &aleft);
  if (s) return s;

  s = check_bag_set(this, aleft);
  if (s) return s;

  oqmlAtomList *aright;
  s = qright->eval(db, ctx, &aright);
  if (s) return s;

  s = check_bag_set(this, aright);
  if (s) return s;

  oqmlAtomList *left = OQML_ATOM_COLLVAL(aleft->first);
  oqmlAtomList *right = OQML_ATOM_COLLVAL(aright->first);

  oqmlAtomList *list = new oqmlAtomList(left);
  list->append(right);

  return set_make(aleft->first, aright->first, list, alist);
}

void oqmlUnion::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlUnion::isConstant() const
{
  return oqml_False;
}

std::string
oqmlUnion::toString(void) const
{
  if (is_statement)
    return std::string(qleft->toString()) + " union " + qright->toString() + "; ";
  return std::string("(") + qleft->toString() + " union " + qright->toString() +
  ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlIntersect operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlIntersect::oqmlIntersect(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlINTERSECT)
{
  qleft = _qleft;
  qright = _qright;
}

oqmlIntersect::~oqmlIntersect()
{
}

oqmlStatus *oqmlIntersect::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s) return s;

  s = qright->compile(db, ctx);
  if (s) return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlIntersect::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlAtomList *aleft;
  oqmlStatus *s;

  s = qleft->eval(db, ctx, &aleft);
  if (s) return s;

  s = check_bag_set(this, aleft);
  if (s) return s;

  oqmlAtomList *aright;
  s = qright->eval(db, ctx, &aright);
  if (s) return s;

  s = check_bag_set(this, aright);
  if (s) return s;

  oqmlAtomList *left = OQML_ATOM_COLLVAL(aleft->first);
  oqmlAtomList *right = OQML_ATOM_COLLVAL(aright->first);
  oqmlAtomList *list = new oqmlAtomList();

  oqmlAtom *leftx = left->first;
  while (leftx)
    {
      oqmlAtom *next = leftx->next;
      if (is_in(leftx, right))
	list->append(leftx);
      leftx = next;
    }

  return set_make(aleft->first, aright->first, list, alist);
}

void oqmlIntersect::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlIntersect::isConstant() const
{
  return oqml_False;
}

std::string
oqmlIntersect::toString(void) const
{
  if (is_statement)
    return std::string(qleft->toString()) + " intersect " + qright->toString() + "; ";
  return std::string("(") + qleft->toString() + " intersect " + qright->toString() +
  ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlExcept operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlExcept::oqmlExcept(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlEXCEPT)
{
  qleft = _qleft;
  qright = _qright;
}

oqmlExcept::~oqmlExcept()
{
}

oqmlStatus *oqmlExcept::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s) return s;

  s = qright->compile(db, ctx);
  if (s) return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlExcept::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlAtomList *aleft;
  oqmlStatus *s;

  s = qleft->eval(db, ctx, &aleft);
  if (s) return s;

  s = check_bag_set(this, aleft);
  if (s) return s;

  oqmlAtomList *aright;
  s = qright->eval(db, ctx, &aright);
  if (s) return s;

  s = check_bag_set(this, aright);
  if (s) return s;

  oqmlAtomList *left = OQML_ATOM_COLLVAL(aleft->first);
  oqmlAtomList *right = OQML_ATOM_COLLVAL(aright->first);
  oqmlAtomList *list = new oqmlAtomList();

  oqmlAtom *leftx = left->first;
  while (leftx)
    {
      oqmlAtom *next = leftx->next;
      if (!is_in(leftx, right))
	list->append(leftx);
      leftx = next;
    }

  return set_make(aleft->first, aright->first, list, alist);
}

void oqmlExcept::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlExcept::isConstant() const
{
  return oqml_False;
}

std::string
oqmlExcept::toString(void) const
{
  if (is_statement)
    return std::string(qleft->toString()) + " except " + qright->toString() + "; ";
  return std::string("(") + qleft->toString() + " except " + qright->toString() +
  ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// set comparison functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static oqmlStatus *
oqml_set_inf(oqmlNode *node, oqmlAtomList *left, oqmlAtomList *right,
	     oqmlBool &b)
{
  if (left->cnt >= right->cnt)
    {
      b = oqml_False;
      return oqmlSuccess;
    }

  oqmlAtom *x = left->first;
  while (x)
    {
      if (!is_in(x, right))
	{
	  b = oqml_False;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_True;
  return oqmlSuccess;
}

static oqmlStatus *
oqml_set_infeq(oqmlNode *node, oqmlAtomList *left, oqmlAtomList *right,
	     oqmlBool &b)
{
  oqmlAtom *x = left->first;
  while (x)
    {
      if (!is_in(x, right))
	{
	  b = oqml_False;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_True;
  return oqmlSuccess;
}

static oqmlStatus *
oqml_set_sup(oqmlNode *node, oqmlAtomList *left, oqmlAtomList *right,
	     oqmlBool &b)
{
  if (right->cnt >= left->cnt)
    {
      b = oqml_False;
      return oqmlSuccess;
    }

  oqmlAtom *x = right->first;
  while (x)
    {
      if (!is_in(x, left))
	{
	  b = oqml_False;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_True;
  return oqmlSuccess;
}

static oqmlStatus *
oqml_set_supeq(oqmlNode *node, oqmlAtomList *left, oqmlAtomList *right,
	     oqmlBool &b)
{
  oqmlAtom *x = right->first;
  while (x)
    {
      if (!is_in(x, left))
	{
	  b = oqml_False;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_True;
  return oqmlSuccess;
}

static oqmlStatus *
oqml_set_equal(oqmlNode *node, oqmlAtom *xleft, oqmlAtom *xright,
	       oqmlBool &b)
{
  oqmlAtomList *left  = OQML_ATOM_COLLVAL(xleft);
  oqmlAtomList *right = OQML_ATOM_COLLVAL(xright);

  if (xleft->type.type != xright->type.type || right->cnt != left->cnt)
    {
      b = oqml_False;
      return oqmlSuccess;
    }


  oqmlAtom *x = right->first;
  while (x)
    {
      if (!is_in(x, left))
	{
	  b = oqml_False;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_True;
  return oqmlSuccess;
}

static oqmlStatus *
oqml_set_diff(oqmlNode *node, oqmlAtom *xleft, oqmlAtom *xright,
	      oqmlBool &b)
{
  oqmlAtomList *left  = OQML_ATOM_COLLVAL(xleft);
  oqmlAtomList *right = OQML_ATOM_COLLVAL(xright);

  if (xleft->type.type != xright->type.type || right->cnt != left->cnt)
    {
      b = oqml_True;
      return oqmlSuccess;
    }

  oqmlAtom *x = right->first;
  while (x)
    {
      if (!is_in(x, left))
	{
	  b = oqml_True;
	  return oqmlSuccess;
	}

      x = x->next;
    }

  b = oqml_False;
  return oqmlSuccess;
}

oqmlStatus *
oqml_set_compare(oqmlNode *node, oqmlTYPE type, const char *opstr,
		 oqmlAtom *aleft, oqmlAtom *aright, oqmlBool &b)
{
  oqmlStatus *s;

  s = check_bag_set(node, aleft);
  if (s) return s;

  s = check_bag_set(node, aright);
  if (s) return s;

  if (type == oqmlINF)
    return oqml_set_inf(node, OQML_ATOM_COLLVAL(aleft),
			OQML_ATOM_COLLVAL(aright), b);

  if (type == oqmlINFEQ)
    return oqml_set_infeq(node, OQML_ATOM_COLLVAL(aleft),
			  OQML_ATOM_COLLVAL(aright), b);

  if (type == oqmlSUP)
    return oqml_set_sup(node, OQML_ATOM_COLLVAL(aleft),
			OQML_ATOM_COLLVAL(aright), b);

  if (type == oqmlSUPEQ)
    return oqml_set_supeq(node, OQML_ATOM_COLLVAL(aleft),
			  OQML_ATOM_COLLVAL(aright), b);

  if (type == oqmlEQUAL)
    return oqml_set_equal(node, aleft, aright, b);

  if (type == oqmlDIFF)
    return oqml_set_diff(node, aleft, aright, b);

  return new oqmlStatus(node,
			"operation '%s %s %s' is not valid",
			aleft->type.getString(), opstr,
			aright->type.getString());
}
}
