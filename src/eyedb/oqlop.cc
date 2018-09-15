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

// PATCH p2312

namespace eyedb {

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlAdd operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlAdd::oqmlAdd(oqmlNode * _qleft, oqmlNode * _qright, oqmlBool _unary) :
  oqmlNode(oqmlADD)
{
  qleft = _qleft;
  qright = _qright;
  unary = _unary;
}

oqmlAdd::oqmlAdd(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlADD)
{
  qleft = _qleft;
  qright = _qright;
  unary = oqml_False;
}

oqmlAdd::~oqmlAdd()
{
}

oqmlStatus *oqmlAdd::compile(Database *db, oqmlContext *ctx)
{
  oqmlBool iscts;
  oqmlStatus *s;

  s = binopCompile(db, ctx, "+", qleft, qright, eval_type,
		   oqmlDoubleConcatOK, iscts);

  if (s)
    return s;

  if (isConstant() && !cst_list)
    {
      oqmlAtomList *al;
      s = eval(db, ctx, &al);
      if (s) return s;
      cst_list = al->copy();
      if (isLocked())
	oqmlLock(cst_list, oqml_True);
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlAdd::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (cst_list) {
    *alist = new oqmlAtomList(cst_list);
    return oqmlSuccess;
  }

  oqmlAtomList *al_left, *al_right;
  oqmlStatus *s = binopEval(db, ctx, "+", eval_type,
			    qleft, qright, oqmlDoubleConcatOK,
			    &al_left, &al_right);

  if (s)
    return s;

  oqmlAtom *aleft = al_left->first, *aright = al_right->first;

  oqmlATOMTYPE atleft  = aleft->type.type;
  oqmlATOMTYPE atright = aright->type.type;

//#define NEW_ADD_COLL
  if (OQML_IS_COLL(aleft)) {
    oqmlAtomList *al = new oqmlAtomList();

    al->append(OQML_ATOM_COLLVAL(aleft), oqml_False);
#ifdef NEW_ADD_COLL
    if (!aright->as_nil())
      al->append(aright->copy());
#else
    if (OQML_IS_COLL(aright)) {
      if (atleft != atright)
	return oqmlStatus::expected(this, &aleft->type, &aright->type);
      al->append(OQML_ATOM_COLLVAL(aright)->copy(), oqml_False);
    }
    else if (!aright->as_nil())
      al->append(aright->copy());
#endif

    if (OQML_IS_LIST(aleft))
      *alist = new oqmlAtomList(new oqmlAtom_list(al));
    else if (OQML_IS_BAG(aleft))
      *alist = new oqmlAtomList(new oqmlAtom_bag(al));
    else if (OQML_IS_ARRAY(aleft))
      *alist = new oqmlAtomList(new oqmlAtom_array(al));
    else if (OQML_IS_SET(aleft))
      *alist = new oqmlAtomList(new oqmlAtom_set(al));

#ifdef SYNC_GARB
    // NO !!!
    /*
    delete al_left;
    delete al_right;
    */
#endif
    return oqmlSuccess;
  }

  if (OQML_IS_COLL(aright)) {
    oqmlAtomList *al = new oqmlAtomList();

    if (OQML_IS_COLL(aleft)) {
#ifdef NEW_ADD_COLL
      assert(0);
#endif
      if (atleft != atright)
	return oqmlStatus::expected(this, &aright->type, &aleft->type);
      al->append(OQML_ATOM_COLLVAL(aleft), oqml_False);
    }
    else if (!aleft->as_nil())
      al->append(aleft);

    al->append(OQML_ATOM_COLLVAL(aright), oqml_False);

    if (OQML_IS_LIST(aright))
      *alist = new oqmlAtomList(new oqmlAtom_list(al));
    else if (OQML_IS_BAG(aright))
      *alist = new oqmlAtomList(new oqmlAtom_bag(al));
    else if (OQML_IS_ARRAY(aright))
      *alist = new oqmlAtomList(new oqmlAtom_array(al));
    else if (OQML_IS_SET(aright))
      *alist = new oqmlAtomList(new oqmlAtom_set(al));
    
#ifdef SYNC_GARB
    // NO !!!
    /*
    delete al_left;
    delete al_right;
    */
#endif
    return oqmlSuccess;
  }

  if (atleft == oqmlATOM_INT)
    *alist = new oqmlAtomList
      (new oqmlAtom_int(OQML_ATOM_INTVAL(aleft) + OQML_ATOM_INTVAL(aright)));

  else if (atleft == oqmlATOM_CHAR)
    *alist = new oqmlAtomList
      (new oqmlAtom_int(OQML_ATOM_CHARVAL(aleft) + OQML_ATOM_CHARVAL(aright)));

  else if (atleft == oqmlATOM_DOUBLE)
    *alist = new oqmlAtomList
      (new oqmlAtom_double(OQML_ATOM_DBLVAL(aleft) + OQML_ATOM_DBLVAL(aright)));

  else if (atleft == oqmlATOM_STRING) {
    char *s = new char[OQML_ATOM_STRLEN(aleft) + 
		      OQML_ATOM_STRLEN(aright)+1];
    s[0] = 0;
    strcat(s, OQML_ATOM_STRVAL(aleft));
    strcat(s, OQML_ATOM_STRVAL(aright));
    
    *alist = new oqmlAtomList(new oqmlAtom_string(s));
    delete [] s;
  }
  else
    return new oqmlStatus(this, "unexpected type %s", aleft->type.getString());

#ifdef SYNC_GARB
  OQL_DELETE(al_left);
  OQL_DELETE(al_right);
#endif
  return oqmlSuccess;
}

void oqmlAdd::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlAdd::isConstant() const
{
  return OQMLBOOL(qleft->isConstant() && qright->isConstant());
}

std::string
oqmlAdd::toString(void) const
{
  if (unary)
    return oqml_unop_string(qright, "+", is_statement);
  return oqml_binop_string(qleft, qright, "+", is_statement);
}

oqmlSub::oqmlSub(oqmlNode * _qleft, oqmlNode * _qright, oqmlBool _unary) :
  oqmlNode(oqmlSUB)
{
  qleft = _qleft;
  qright = _qright;
  unary = _unary;
}

oqmlSub::oqmlSub(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlSUB)
{
  qleft = _qleft;
  qright = _qright;
  unary = oqml_False;
}

oqmlSub::~oqmlSub()
{
}

oqmlStatus *oqmlSub::compile(Database *db, oqmlContext *ctx)
{
  oqmlBool iscts;
  oqmlStatus *s;
  s = binopCompile(db, ctx, "-", qleft, qright, eval_type, oqmlDoubleOK, iscts);

  if (s)
    return s;

  if (isConstant() && !cst_list)
   {
      oqmlAtomList *al;
      s = eval(db, ctx, &al);
      if (s) return s;
      cst_list = al->copy();
      if (isLocked())
         oqmlLock(cst_list, oqml_True);
   }

  return oqmlSuccess;
}

oqmlStatus *oqmlSub::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (cst_list)
    {
      *alist = new oqmlAtomList(cst_list);
      return oqmlSuccess;
    }

  oqmlAtomList *al_left, *al_right;
  oqmlStatus *s = binopEval(db, ctx, "-", eval_type, qleft, qright, oqmlDoubleOK, &al_left, &al_right);

  if (s)
    return s;

  oqmlAtom *aleft = al_left->first, *aright = al_right->first;

  oqmlATOMTYPE at = aleft->type.type;

  if (at == oqmlATOM_INT)
    *alist = new oqmlAtomList
      (new oqmlAtom_int(OQML_ATOM_INTVAL(aleft) - OQML_ATOM_INTVAL(aright)));
  else if (at == oqmlATOM_CHAR)
    *alist = new oqmlAtomList
      (new oqmlAtom_int(OQML_ATOM_CHARVAL(aleft) - OQML_ATOM_CHARVAL(aright)));
  else if (at == oqmlATOM_DOUBLE)
    *alist = new oqmlAtomList
      (new oqmlAtom_double(OQML_ATOM_DBLVAL(aleft) - OQML_ATOM_DBLVAL(aright)));
  else
    return oqmlStatus::expected(this, "integer, character or double", aleft->type.getString());

#ifdef SYNC_GARB
  OQL_DELETE(al_left);
  OQL_DELETE(al_right);
#endif
  return oqmlSuccess;
}

void oqmlSub::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlSub::isConstant() const
{
  return OQMLBOOL(qleft->isConstant() && qright->isConstant());
}
std::string oqmlSub::toString(void) const
{
  if (unary)
    return oqml_unop_string(qright, "-", is_statement);
  return oqml_binop_string(qleft, qright, "-", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// standard binop operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define OQML_STD_BINOP(X, PARENT, XX, OPSTR) \
X::X(oqmlNode * _qleft, oqmlNode * _qright) : \
  PARENT(XX, _qleft, _qright, OPSTR) { } \
 \
X::~X() { } 

OQML_STD_BINOP(oqmlEqual, oqmlComp, oqmlEQUAL, "==")
OQML_STD_BINOP(oqmlDiff, oqmlComp, oqmlDIFF, "!=")
OQML_STD_BINOP(oqmlInf, oqmlComp, oqmlINF, "<")
OQML_STD_BINOP(oqmlInfEq, oqmlComp, oqmlINFEQ, "<=")
OQML_STD_BINOP(oqmlSup, oqmlComp, oqmlSUP, ">")
OQML_STD_BINOP(oqmlSupEq, oqmlComp, oqmlSUPEQ, ">=")
OQML_STD_BINOP(oqmlBetween, oqmlComp, oqmlBETWEEN, " between ")
OQML_STD_BINOP(oqmlNotBetween, oqmlComp, oqmlNOTBETWEEN, " not between ")

oqmlNode *oqmlBetween::requalifyNot()
{
  oqmlNode *node = new oqmlNotBetween(qleft, qright);
  qleft = qright = 0;
  return node;
}

OQML_STD_BINOP(oqmlRegCmp, oqmlRegex, oqmlREGCMP, "~")
OQML_STD_BINOP(oqmlRegICmp, oqmlRegex, oqmlREGICMP, "~~")
OQML_STD_BINOP(oqmlRegDiff, oqmlRegex, oqmlREGDIFF, "!~")
OQML_STD_BINOP(oqmlRegIDiff, oqmlRegex, oqmlREGIDIFF, "!~~")

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// logical operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static oqmlStatus *
oqml_logical_error(oqmlNode *node, oqmlAtomType *at, bool strict)
{
  std::string msg = (strict ? "bool": "bool, int, char or double");
  if (at)
    return oqmlStatus::expected(node, msg.c_str(), at->getString());

  return new oqmlStatus(node, (msg + " expected").c_str());
}

static oqmlStatus *
oqml_check_type(oqmlNode *node, Database *db, oqmlContext *ctx,
		oqmlNode *ql, const char *name, bool strict = false)
{
  if (ql->getType() == oqmlASSIGN)
    return oqmlSuccess;

  oqmlAtomType at;
  ql->evalType(db, ctx, &at);

  /*
  if (at.type != oqmlATOM_IDENT && at.type != oqmlATOM_INT &&
      at.type != oqmlATOM_DOUBLE && at.type != oqmlATOM_CHAR &&
      at.type != oqmlATOM_BOOL && at.type != oqmlATOM_UNKNOWN_TYPE)
    return oqml_logical_error(node, name, &at);
    */

  if (at.type == oqmlATOM_UNKNOWN_TYPE)
    return oqmlSuccess;

  if (strict) {
    if (at.type != oqmlATOM_BOOL)
      return oqml_logical_error(node, &at, strict);
  }

  if (at.type != oqmlATOM_INT &&
      at.type != oqmlATOM_CHAR &&
      at.type != oqmlATOM_DOUBLE &&
      at.type != oqmlATOM_BOOL)
    return oqml_logical_error(node, &at, strict);

  return oqmlSuccess;
}

oqmlStatus *
oqml_check_logical(oqmlNode *node, oqmlAtomList *al, oqmlBool &b,
		   bool strict)
{
  if (al->cnt != 1)
    return oqml_logical_error(node, 0, strict);

  oqmlAtom *a = al->first;
  if (a->type.type == oqmlATOM_BOOL) {
    b = (((oqmlAtom_bool *)a)->b) ? oqml_True : oqml_False;
    return oqmlSuccess;
  }
  
  if (!strict) {
    if (a->type.type == oqmlATOM_INT) {
      b = (((oqmlAtom_int *)a)->i) ? oqml_True : oqml_False;
      return oqmlSuccess;
    }

    if (a->type.type == oqmlATOM_CHAR) {
      b = (((oqmlAtom_char *)a)->c) ? oqml_True : oqml_False;
      return oqmlSuccess;
    }

    if (a->type.type == oqmlATOM_DOUBLE) {
      b = (((oqmlAtom_double *)a)->d) ? oqml_True : oqml_False;
      return oqmlSuccess;
    }
  }

  return oqml_logical_error(node, &a->type, strict);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// logical OR operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlLOr::oqmlLOr(oqmlNode * _qleft, oqmlNode * _qright,
		 oqmlBool _isLiteral) : oqmlNode(oqmlLOR)
{
  qleft = _qleft;
  qright = _qright;
  eval_type.type = oqmlATOM_BOOL;
  isLiteral = _isLiteral;
  node = 0;
}

oqmlLOr::~oqmlLOr()
{
}

#define STRICT_SELECT

oqmlStatus *oqmlLOr::compile(Database *db, oqmlContext *ctx)
{
  node = 0;

  if (ctx->isSelectContext())
    {
#ifdef STRICT_SELECT
      return new oqmlStatus(this, "this type of query constructs is no more supported: use select/from/where clause");
#else
      node = new oqmlOr(qleft, qright);
      if (isLocked())
	node->lock();
      return node->compile(db, ctx);
#endif
    }

  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s)
    return s;

  s = oqml_check_type(this, db, ctx, qleft, "||");
  if (s)
    return s;

  s = qright->compile(db, ctx);
  if (s)
    return s;

  s = oqml_check_type(this, db, ctx, qright, "||");
  if (s)
    return s;
  return oqmlSuccess;
}

oqmlStatus *oqmlLOr::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (node)
    return node->eval(db, ctx, alist);

  oqmlStatus *s;
  oqmlAtomList *al_left, *al_right;

  *alist = new oqmlAtomList;

  s = qleft->eval(db, ctx, &al_left);
  if (s)
    return s;

  oqmlBool b;

  s = oqml_check_logical(this, al_left, b);
  if (s)
    return s;

  if (b)
    {
      (*alist)->append(new oqmlAtom_bool(oqml_True));
      return oqmlSuccess;
    }

  s = qright->eval(db, ctx, &al_right);
  if (s)
    return s;

  s = oqml_check_logical(this, al_right, b);
  if (s)
    return s;

  if (b)
    (*alist)->append(new oqmlAtom_bool(oqml_True));
  else
    (*alist)->append(new oqmlAtom_bool(oqml_False));

  return oqmlSuccess;
}

void oqmlLOr::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlStatus *
oqmlLOr::preEvalSelect(Database *db, oqmlContext *ctx,
		       const char *ident, oqmlBool &done,
		       unsigned int &cnt, oqmlBool firstPass)
{
  if (node)
    return node->preEvalSelect(db, ctx, ident, done, cnt, firstPass);

  oqmlStatus *s;

  ctx->incrOrContext();
  s = qleft->preEvalSelect(db, ctx, ident, done, cnt, firstPass);

  if (!s)
    {
      unsigned int cnt_r;
      s = qright->preEvalSelect(db, ctx, ident, done, cnt_r, firstPass);
      if (!s) cnt += cnt_r;
    }

  ctx->decrOrContext();
  return s;
}

oqmlBool oqmlLOr::isConstant() const
{
  return oqml_False;
}

void
oqmlLOr::lock()
{
  oqmlNode::lock();
  qleft->lock();
  qright->lock();
  if (node) node->lock();
}

void
oqmlLOr::unlock()
{
  oqmlNode::unlock();
  qleft->unlock();
  qright->unlock();
  if (node) node->unlock();
}

std::string
oqmlLOr::toString(void) const
{
  return (node ? node->toString() :
	  oqml_binop_string(qleft, qright, (isLiteral ? " or " : "||"),
			    is_statement));
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// logical AND operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlLAnd::oqmlLAnd(oqmlNode * _qleft, oqmlNode * _qright,
		   oqmlBool _isLiteral, oqmlNode *_intersect_pred) :
  oqmlNode(oqmlLAND)
{
  qleft = _qleft;
  qright = _qright;
  isLiteral = _isLiteral;
  eval_type.type = oqmlATOM_BOOL;
  node = 0;
  intersect_pred = _intersect_pred;
  must_intersect = oqml_False;
  estimated = oqml_False;
  r_first =  r_second = 0;
}

oqmlLAnd::~oqmlLAnd()
{
}

oqmlStatus *oqmlLAnd::compile(Database *db, oqmlContext *ctx)
{
  node = 0;
  requalified = oqml_False;

  if (ctx->isSelectContext())
    {
#ifdef STRICT_SELECT
      return new oqmlStatus(this, "this type of query constructs is no more supported: use select/from/where clause");
#else
      node = new oqmlAnd(qleft, qright);
      if (isLocked())
	node->lock();
      return node->compile(db, ctx);
#endif
    }

  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s)
    return s;

  s = oqml_check_type(this, db, ctx, qleft, "&&");
  if (s)
    return s;

  s = qright->compile(db, ctx);
  if (s)
    return s;

  s = oqml_check_type(this, db, ctx, qright, "&&");
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlLAnd::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (node)
    return node->eval(db, ctx, alist);

  oqmlStatus *s;
  oqmlAtomList *al_left, *al_right;

  *alist = new oqmlAtomList;

  s = qleft->eval(db, ctx, &al_left);
  if (s)
    return s;

  oqmlBool b;

  s = oqml_check_logical(this, al_left, b);
  if (s)
    return s;

  if (!b)
    {
      (*alist)->append(new oqmlAtom_bool(oqml_False));
      return oqmlSuccess;
    }

  s = qright->eval(db, ctx, &al_right);
  if (s)
    return s;

  s = oqml_check_logical(this, al_right, b);
  if (s)
    return s;

  if (b)
    (*alist)->append(new oqmlAtom_bool(oqml_True));
  else
    (*alist)->append(new oqmlAtom_bool(oqml_False));

  return oqmlSuccess;
}

void oqmlLAnd::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

class OptimContext {

  LinkedList varlist;
  oqmlNode *node;
  Database *db;
  oqmlContext *ctx;

  oqmlStatus *push(const char *ident, oqmlAtom *x) {
    oqmlStatus *s = ctx->pushSymbol(ident, &x->type, x, oqml_True, oqml_True);
    if (s) return s;
    varlist.insertObject((void *)ident);
    return s;
  }

public:
  OptimContext(oqmlNode *_node, Database *_db, oqmlContext *_ctx) :
    node(_node), db(_db), ctx(_ctx) { }

  oqmlStatus *realize(oqmlNode *intersect_pred, oqmlBool done,
		      unsigned int r_first, unsigned int r_second,
		      unsigned int cnt, oqmlBool &make_intersect)
    {
      if (!intersect_pred)
	{
	  make_intersect = oqml_False;
	  return oqmlSuccess;
	}

      oqmlStatus *s = push("oql$done_1", new oqmlAtom_bool(done));
      if (s) return s;

      s = push("oql$estim_1", new oqmlAtom_int(r_first));
      if (s) return s;

      s = push("oql$estim_2", new oqmlAtom_int(r_second));
      if (s) return s;

      s = push("oql$count_1", new oqmlAtom_int(cnt));
      if (s) return s;

      int select_ctx_cnt = ctx->getSelectContextCount();

      s = intersect_pred->compile(db, ctx);
      oqmlAtomList *al = 0;
      if (!s)
	s = intersect_pred->eval(db, ctx, &al);

      ctx->setSelectContextCount(select_ctx_cnt);

      if (s) return s;

      if (al->cnt != 1 || !OQML_IS_BOOL(al->first))
	return new oqmlStatus(node, "and hint: bool expected, got %s",
			      al->first ? al->first->type.getString() :
			      "nothing");

      make_intersect = OQML_ATOM_BOOLVAL(al->first);
      return oqmlSuccess;
    }

  ~OptimContext() {
    LinkedListCursor c(varlist);
    const char *ident;
    while (c.getNext((void *&)ident))
      ctx->popSymbol(ident, oqml_True);
  }

};

const char *
oqmlLAnd::getDefaultRule()
{
  // WARNING: this must match the test in oqmlLAnd::preEvalSelect_optim
  return "!oql$done_1 || (oql$estim_2 != oql$ESTIM_MAX && oql$estim_2 <= oql$estim_1 && oql$count_1 > 100)";
}

oqmlStatus *
oqmlLAnd::estimateLAnd(Database *db, oqmlContext *ctx)
{
  if (estimated)
    return oqmlSuccess;

  unsigned int r_left, r_right;
  oqmlStatus *s;

  s = qleft->estimate(db, ctx, r_left);
  if (s) return s;

  ctx->incrAndContext();
  s = qright->estimate(db, ctx, r_right);
  ctx->decrAndContext();
  if (s) return s;

  estimated = oqml_True;

  if (r_left <= r_right)
    {
      r_first = r_left;
      r_second = r_right;
      return oqmlSuccess;
    }

  oqmlNode *ql = qleft;
  qleft = qright;
  qright = ql;
  r_first = r_right;
  r_second = r_left;
  return oqmlSuccess;
}

oqmlStatus *
oqmlLAnd::preEvalSelect_optim(Database *db, oqmlContext *ctx,
			      const char *ident, oqmlBool &done,
			      unsigned int &cnt, oqmlBool firstPass)
{
  if (node)
    return node->preEvalSelect(db, ctx, ident, done, cnt, firstPass);

  oqmlStatus *s;
  s = estimateLAnd(db, ctx);
  if (s) return s;

  s = qleft->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
  if (s) return s;

  OptimContext optim_ctx(this, db, ctx);
  oqmlBool make_intersect;
  s = optim_ctx.realize(intersect_pred, done, r_first, r_second, cnt,
			make_intersect);
  if (s) return s;

  /*
    default rule:
    !oql$done_1 || (oql$estim_2 != oql$ESTIM_MAX && oql$estim_2 < oql$estim_1 && oql$count_1 > 100)
    */

  if (!must_intersect && intersect_pred && !make_intersect)
    return oqmlSuccess;

  if (firstPass &&
      (must_intersect || make_intersect ||
       // WARNING: the following code must match default rule
       !done || (r_second != oqml_ESTIM_MAX && r_second <= r_first && cnt > 100)))
    {
      unsigned int cnt_r;
      oqmlBool odone = done;
      if (odone) ctx->incrAndContext();
      s = qright->preEvalSelect(db, ctx, ident, done, cnt_r, firstPass);
      if (odone) ctx->decrAndContext();
      return s;
    }

  return oqmlSuccess;
}

oqmlStatus *
oqmlLAnd::preEvalSelect_nooptim(Database *db, oqmlContext *ctx,
				const char *ident, oqmlBool &done,
				unsigned int &cnt, oqmlBool firstPass)
{
  if (node)
    return node->preEvalSelect(db, ctx, ident, done, cnt, firstPass);

  oqmlStatus *s;

  // 29/08/02 : en mode non optimisé, on doit prendre l'ordre du ``and''
  // as is !
#if 1
  s = qleft->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
  if (s || done) return s;
  return qright->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
#else
  unsigned int r_left, r_right;

  s = qleft->estimate(db, ctx, r_left);
  if (s) return s;

  if (r_left != oqml_ESTIM_MIN)
    {
      s = qright->estimate(db, ctx, r_right);
      if (s) return s;
    }
  else
    r_right = oqml_ESTIM_MAX;

  if (r_left <= r_right)
    {
      s = qleft->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
      if (s || done) return s;
      return qright->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
    }

  s = qright->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
  if (s || done) return s;
  return qleft->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
#endif
}

#define REQUALIFY_AND(OPL, OPLEQ, OPR, OPREQ, \
		      getL1, getR1, getL2, getR2) \
  if ((left_type == OPL || left_type == OPLEQ) && \
      (right_type == OPR || right_type == OPREQ))\
    { \
      oqmlComp *cleft = ((oqmlComp *)qleft); \
      oqmlComp *cright = ((oqmlComp *)qright); \
 \
      if (cleft->getL1()->toString() == cright->getR1()->toString()) \
	return new oqmlBetween \
	  (cleft->getL1(), \
	   new oqmlRange(cleft->getL2(), \
			 (left_type == OPL ? oqml_False : oqml_True), \
			 cright->getR2(), \
			 (right_type == OPR ? oqml_False : oqml_True))); \
    }

oqmlNode *
oqmlLAnd::requalifyRange()
{
  oqmlTYPE left_type = qleft->getType();
  oqmlTYPE right_type = qright->getType();

  REQUALIFY_AND(oqmlSUP, oqmlSUPEQ, oqmlINF, oqmlINFEQ,
		getLeft, getLeft, getRight, getRight);

  REQUALIFY_AND(oqmlINF, oqmlINFEQ, oqmlSUP, oqmlSUPEQ,
		getRight, getRight, getLeft, getLeft);
  REQUALIFY_AND(oqmlINF, oqmlINFEQ, oqmlINF, oqmlINFEQ,
		getRight, getLeft, getLeft, getRight);
  REQUALIFY_AND(oqmlSUP, oqmlSUPEQ, oqmlSUP, oqmlSUPEQ,
		getLeft, getRight, getRight, getLeft);

  return (oqmlNode *)0;
}

oqmlStatus *
oqmlLAnd::requalify(Database *db, oqmlContext *ctx,
		    const char *_ident, oqmlNode *_node, oqmlBool& done)
{
  if (requalified)
    return oqmlSuccess;

  //printf("BEFORE #1 REQUALIFICATION %s\n", (const char *)toString());
  oqmlBool done_left = oqml_False, done_right = oqml_False;
  oqmlStatus *s = requalify_node(db, ctx, qleft, _ident, _node, done_left);
  if (s) return s;
  s = requalify_node(db, ctx, qright, _ident, _node, done_right);
  if (s) return s;

  //printf("AFTER #1 REQUALIFICATION %s\n", (const char *)toString());
  oqmlBool and_optim = isAndOptim(ctx);

  if (and_optim) {
    s = estimateLAnd(db, ctx);
    if (s) return s;
  }

  // must requalify back only if done_left and done_right
  //printf("DONES %d %d\n", done_left, done_right);
  if (done_left && done_right)
    {
      s = requalify_back(db, ctx);
      if (s) return s;
    }

  if (and_optim) {
    must_intersect = OQMLBOOL((done_left && !done_right) ||
			      (!done_left && done_right));

  }

  /*
  printf("AFTER #2 REQUALIFICATION %s %d\n", (const char *)toString(),
	 must_intersect);
	 */

  if (OQMLBOOL(done_left || done_right))
    done = oqml_True;

  requalified = oqml_True;
  return oqmlSuccess;
}

oqmlStatus *
oqmlLAnd::requalify_back(Database *db, oqmlContext *ctx)
{
  return requalify_node_back(db, ctx, qright);
}

oqmlBool
oqmlLAnd::isAndOptim(oqmlContext *ctx)
{
  static const char oql_no_and_optim[] = "oql$no_and_optim";
  return oqml_is_symbol(ctx, oql_no_and_optim) ? oqml_False : oqml_True;
}

oqmlStatus *
oqmlLAnd::preEvalSelect(Database *db, oqmlContext *ctx,
			const char *ident, oqmlBool &done,
			unsigned int &cnt, oqmlBool firstPass)
{
  if (isAndOptim(ctx))
    return preEvalSelect_optim(db, ctx, ident, done, cnt, firstPass);
  return preEvalSelect_nooptim(db, ctx, ident, done, cnt, firstPass);
}

oqmlBool oqmlLAnd::isConstant() const
{
  return oqml_False;
}

void
oqmlLAnd::lock()
{
  oqmlNode::lock();
  qleft->lock();
  qright->lock();
  if (node) node->lock();
  if (intersect_pred) intersect_pred->lock();
}

void
oqmlLAnd::unlock()
{
  oqmlNode::unlock();
  qleft->unlock();
  qright->unlock();
  if (node) node->unlock();
  if (intersect_pred) intersect_pred->unlock();
}

std::string
oqmlLAnd::toString(void) const
{
  return (node ? node->toString() :
	  oqml_binop_string(qleft, qright, (isLiteral ? " and " : "&&"),
			    is_statement));
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// logical NOT operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlLNot::oqmlLNot(oqmlNode * _ql) : oqmlNode(oqmlLNOT)
{
  ql = _ql;
  eval_type.type = oqmlATOM_BOOL;
}

oqmlLNot::~oqmlLNot()
{
}

oqmlStatus *oqmlLNot::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  return oqml_check_type(this, db, ctx, ql, "!");
}

oqmlStatus *oqmlLNot::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  *alist = new oqmlAtomList;

  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  oqmlBool b;

  s = oqml_check_logical(this, al, b);
  if (s)
    return s;

  if (b)
    (*alist)->append(new oqmlAtom_bool(oqml_False));
  else
    (*alist)->append(new oqmlAtom_bool(oqml_True));

  return oqmlSuccess;
}

void oqmlLNot::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlLNot::isConstant() const
{
  return ql->isConstant();
}

std::string
oqmlLNot::toString(void) const
{
  return oqml_unop_string(ql, "!", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// comma binary operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlComma::oqmlComma(oqmlNode * _qleft, oqmlNode * _qright, oqmlBool _is_statement) :
  oqmlNode(oqmlCOMMA)
{
  qleft = _qleft;
  qright = _qright;
  is_statement = _is_statement;
}

oqmlComma::~oqmlComma()
{
}

oqmlStatus *oqmlComma::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = qleft->compile(db, ctx);
  if (s)
    return s;

  s = qright->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlComma::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al_left, *al_right;

  *alist = new oqmlAtomList();

  s = qleft->eval(db, ctx, &al_left);
  if (s)
    return s;

#ifdef SYNC_GARB
  OQL_DELETE(al_left);
#endif
  s = qright->eval(db, ctx, &al_right);
  if (s)
    return s;

  (*alist)->append(al_right);
  /*
#ifdef SYNC_GARB
  al_right->first = 0;
  OQL_DELETE(al_right);
#endif
  */

  return oqmlSuccess;
}

void oqmlComma::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  oqmlAtomType at_left, at_right;
  oqmlStatus *s;

  qright->evalType(db, ctx, &at_right);

  *at = at_right;
}

oqmlBool oqmlComma::isConstant() const
{
  return oqml_False;
}

std::string
oqmlComma::toString(void) const
{
  if (!is_statement)
    return oqml_binop_string(qleft, qright, ",", is_statement);

  return qleft->toString() + qright->toString();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// assignement binary operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlAssign::oqmlAssign(oqmlNode * _qleft, oqmlNode * _qright)
  : oqmlNode(oqmlASSIGN)
{
  qleft = _qleft;
  qright = _qright;
  ident = 0;
}

oqmlAssign::~oqmlAssign()
{
  free(ident);
}

oqmlStatus *oqmlAssign::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  free(ident); ident = 0;

  s = qleft->compile(db, ctx);
  if (s) return s;

  s = qright->compile(db, ctx);
  if (s) return s;

  if (qleft->getType() == oqmlIDENT)
    {
      ident = strdup(((oqmlIdent *)qleft)->getName());
      return oqmlSuccess;
    }

  if (qleft->asDot())
    {
      oqmlAtomType at_left, at_right;

      qleft->evalType(db, ctx, &at_left);
      qright->evalType(db, ctx, &at_right);
	  
      if (at_left.type != oqmlATOM_UNKNOWN_TYPE &&
	  at_right.type != oqmlATOM_UNKNOWN_TYPE &&
	  !at_left.cmp(at_right))
	{
	  if (at_left.type == oqmlATOM_OID && at_right.type != oqmlATOM_NULL)
	    return new oqmlStatus(this, "incompatible types for assignation: "
				 "%s expected, got %s.",
				 at_left.getString(), at_right.getString());
	}

      return oqmlSuccess;
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlAssign::eval(Database *db, oqmlContext *ctx,
			     oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlAtomList *al_right = 0;
  oqmlStatus *s;
  oqmlAtom *leftValue = 0;
  oqmlSymbolEntry *entry = 0;
  int idx = -1;

  s = qright->eval(db, ctx, &al_right);
  if (s) return s;

  if (!ident && qleft->getType() != oqmlDOT)
    {
      s = qleft->evalLeft(db, ctx, &leftValue, idx);
      if (s) return s;

      if (!leftValue)
	return new oqmlStatus(this, "not a left value");

      if (leftValue->as_ident())
	{
	  ident = strdup(OQML_ATOM_IDENTVAL(leftValue));
	  entry = leftValue->as_ident()->entry;
	}
    }

  if (ident)
    {
      oqmlAtom *a;
      if (al_right->cnt == 1) {
	a = al_right->first;
#ifdef SYNC_GARB
	if (al_right && !al_right->refcnt) {
	  al_right->first = 0;
	  OQL_DELETE(al_right);
	}
#endif
      }
      else if (al_right->cnt)
	a = new oqmlAtom_list(al_right);
      else
	a = NULL;

      oqmlBool global;

      if (entry)
	{
	  // added the 3/01/00 to set both entries (:: prefixed, and not
	  // prefixed) in the symbol table.
	  if (entry->global)
	    {
	      s = ctx->setSymbol(entry->ident, (a ? &a->type : 0), a, oqml_True);
	      if (s) return s;
	    }
	  // ...
	  else
	    entry->set((a ? &a->type : 0), a);
	}
      else
	{
	  if (!ctx->getSymbol(ident, 0, 0, &global) || global ||
	      !strncmp(ident, oqml_global_scope, oqml_global_scope_len))
	    s = ctx->setSymbol(ident, (a ? &a->type : 0), a, oqml_True);
	  else
	    s = ctx->setSymbol(ident, (a ? &a->type : 0), a, oqml_False);

	  if (s) return s;
	}

      *alist = new oqmlAtomList(a);
      free(ident); ident = 0;
      return oqmlSuccess;
    }

  if (leftValue)
    {
      if (idx < 0)
	return new oqmlStatus(this, "not a left value");

      oqmlAtom *a = 0;

      if (al_right->cnt == 1) {
	a = al_right->first;
#ifdef SYNC_GARB
	if (al_right && !al_right->refcnt) {
	  al_right->first = 0;
	  OQL_DELETE(al_right);
	}
#endif
      }

      if (leftValue->as_string()) {
	s = leftValue->as_string()->setAtom(a, idx, this);
	if (s) return s;
      }
      else if (leftValue->as_list()) {
	s = leftValue->as_list()->setAtom(a, idx, this);
	if (s) return s;
      }
      else if (leftValue->as_array()) {
	s = leftValue->as_array()->setAtom(a, idx, this);
	if (s) return s;
      }
      else if (leftValue->as_struct()) {
	s = leftValue->as_struct()->setAtom(a, idx, this);
	if (s) return s;
      }
      else if (OQML_IS_OBJECT(leftValue)) {
	Object *o = 0;
	s = oqmlObjectManager::getObject(this, db, leftValue, o, oqml_False,
					 oqml_True);
	if (s) return s;
	
	if (!o->asCollArray()) {
	  oqmlObjectManager::releaseObject(o, oqml_False);
	  return new oqmlStatus(this, "not a left value");
	}

	Status is;

	if (a->as_null() || (a->as_oid() && !OQML_ATOM_OIDVAL(a).isValid())
	    || (a->as_obj() && !OQML_ATOM_OBJVAL(a)))
	  is = o->asCollArray()->suppressAt(idx);
	else if (a->as_oid())
	  is = o->asCollArray()->insertAt(idx, OQML_ATOM_OIDVAL(a));
	else if (a->as_obj()) {
	  OQL_CHECK_OBJ(a);
	  is = o->asCollArray()->insertAt(idx, OQML_ATOM_OBJVAL(a));
	}
	else {
	  oqmlObjectManager::releaseObject(o, oqml_False);
	  return new oqmlStatus(this, "left value: only support collection array of objects");
	}

	if (!is)
	  is = o->store();
	oqmlObjectManager::releaseObject(o, oqml_False);
	if (is) return new oqmlStatus(this, is);
      }
      else
	return new oqmlStatus(this, "not a left value");
      
      *alist = new oqmlAtomList(a);
      return oqmlSuccess;
    }

  if (qleft->asDot()) {
    if (al_right->cnt != 1)
      return new oqmlStatus(this, "invalid right part for dot");
    
    return qleft->asDot()->set(db, ctx, al_right->first, alist);
  }

  return new oqmlStatus(this, "not a left value");
}

void oqmlAssign::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlAssign::isConstant() const
{
  return oqml_False;
}

std::string
oqmlAssign::toString(void) const
{
  return oqml_binop_string(qleft, qright, ":=", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlCompoundStatement operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlCompoundStatement::oqmlCompoundStatement(oqmlNode * _node) : oqmlNode(oqmlCOMPOUND_STATEMENT)
{
  node = _node;
}

oqmlCompoundStatement::~oqmlCompoundStatement()
{
}

oqmlStatus *oqmlCompoundStatement::compile(Database *db, oqmlContext *ctx)
{
  if (!node)
    return oqmlSuccess;

  return node->compile(db, ctx);
}

oqmlStatus *oqmlCompoundStatement::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (!node)
    return oqmlSuccess;

  *alist = new oqmlAtomList();
  oqmlAtomList *dlist = 0;
  oqmlStatus *s = node->eval(db, ctx, &dlist);
#ifdef SYNC_GARB
  OQL_DELETE(dlist);
#endif
  return s;
}

void oqmlCompoundStatement::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlCompoundStatement::isConstant() const
{
  return oqml_False;
}

std::string
oqmlCompoundStatement::toString(void) const
{
  return std::string("{ ") + (node ? node->toString() : std::string("")) +
    "}";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary typeof operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlTypeOf::oqmlTypeOf(oqmlNode * _ql) : oqmlNode(oqmlTYPEOF)
{
  ql = _ql;
  eval_type.type = oqmlATOM_STRING;
  eval_type.comp = oqml_True;
}

oqmlTypeOf::~oqmlTypeOf()
{
}

oqmlStatus *oqmlTypeOf::compile(Database *db, oqmlContext *ctx)
{
  return ql->compile(db, ctx);
}

oqmlStatus *oqmlTypeOf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (!al->cnt)
    *alist = new oqmlAtomList(new oqmlAtom_string("nil"));
  else
    *alist = new oqmlAtomList(new oqmlAtom_string(al->first->type.getString()));
  return oqmlSuccess;
}

void oqmlTypeOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlTypeOf::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlTypeOf::toString(void) const
{
  return oqml_unop_string(ql, "typeof ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary unval operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlUnval::oqmlUnval(oqmlNode * _ql) : oqmlNode(oqmlUNVAL)
{
  ql = _ql;
  eval_type.type = oqmlATOM_STRING;
  eval_type.comp = oqml_True;
}

oqmlUnval::~oqmlUnval()
{
}

oqmlStatus *oqmlUnval::compile(Database *db, oqmlContext *ctx)
{
  return oqmlSuccess;
}

oqmlStatus *oqmlUnval::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  *alist = new oqmlAtomList(new oqmlAtom_string
			    ((ql->toString() +
			      (ql->is_statement ? "; " : "")).c_str()));
  return oqmlSuccess;
}

void oqmlUnval::evalType(Database *db, oqmlContext *ctx,
			    oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlUnval::isConstant() const
{
  return oqml_True;
}

std::string
oqmlUnval::toString(void) const
{
  return std::string("unval(") + ql->toString() + std::string(")");
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary eval operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlEval::oqmlEval(oqmlNode * _ql) : oqmlNode(oqmlEVAL)
{
  ql = _ql;
}

oqmlEval::~oqmlEval()
{
}

oqmlStatus *oqmlEval::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlEval::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (al->cnt == 1 && al->first->as_string())
    {
      char *str = strdup((std::string(OQML_ATOM_STRVAL(al->first)) + ";").c_str());
      s = oqml_realize(db, str, alist);
      free(str);
      return s;
    }

  return new oqmlStatus(this, "string expected");
}

void oqmlEval::evalType(Database *db, oqmlContext *ctx,
			oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlEval::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlEval::toString(void) const
{
  return oqml_unop_string(ql, "eval ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary prin operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlPrint::oqmlPrint(oqmlNode * _ql) : oqmlNode(oqmlPRINT)
{
  ql = _ql;
}

oqmlPrint::~oqmlPrint()
{
}

oqmlStatus *oqmlPrint::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlPrint::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (al->cnt == 1 && al->first->as_string())
    {
      /*
      fprintf(stdout, OQML_ATOM_STRVAL(al->first));
      fflush(stdout);
      */
      Connection::setServerMessage(OQML_ATOM_STRVAL(al->first));
      *alist = new oqmlAtomList();
      return oqmlSuccess;
    }

  return new oqmlStatus(this, "string expected");
}

void oqmlPrint::evalType(Database *db, oqmlContext *ctx,
			oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlPrint::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlPrint::toString(void) const
{
  return oqml_unop_string(ql, "print ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary import operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlImport::oqmlImport(oqmlNode * _ql) : oqmlNode(oqmlIMPORT)
{
  ql = _ql;
}

oqmlImport::~oqmlImport()
{
}

oqmlStatus *oqmlImport::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlImport::eval(Database *db, oqmlContext *ctx,
			     oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (al->cnt == 1 && al->first->as_string())
    {
      char *file = OQML_ATOM_STRVAL(al->first);
      const char *oqmlpath = eyedb::ServerConfig::getSValue("oqlpath");

      oqmlBool check;
      if (!oqmlpath || file[0] == '/')
	{
	  check = oqml_False;
	  return import_realize(db, alist, file, 0, check);
	}

      const char *x;
      int idx = 0;
      while (*(x = get_next_path(oqmlpath, idx)))
	{
	  check = oqml_True;
	  s = import_realize(db, alist, file, x, check);
	  if (check || s)
	    return s;
	}

      return new oqmlStatus(this, "cannot find file '%s'", file);
    }

  return new oqmlStatus(this, "string expected");
}

void oqmlImport::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlImport::isConstant() const
{
  return oqml_False;
}

std::string
oqmlImport::toString(void) const
{
  return oqml_unop_string(ql, "import ", is_statement);
}

oqmlStatus *
oqmlImport::import_realize(Database *db,
			   oqmlAtomList **alist,
			   const char *file, const char *dir, oqmlBool &check)
{
  static const char suffix[] = ".oql";
  static const unsigned int suffix_len = strlen(suffix);
  std::string sfile;

  sfile = (dir ? std::string(dir) + "/" + file : std::string(file));
  int len = strlen(file);
  if (len <= suffix_len || strcmp(&file[len-suffix_len], suffix))
    sfile += suffix;
      
  int fd = open(sfile.c_str(), O_RDONLY);

  *alist = new oqmlAtomList();
  (*alist)->append(new oqmlAtom_string(sfile.c_str()));

  if (fd >= 0)
    {
      extern char *oqml_file;
      oqmlAtomList *xal;
      char *str;
      oqmlStatus *s = file_to_buf(sfile.c_str(), fd, str);
      if (s) return s;
      char *lastfile = oqml_file;
      oqml_file = strdup(sfile.c_str());
      s = oqml_realize(db, str, &xal);
      close(fd);
      free(str);
      if (!s) check = oqml_True;
      free(oqml_file);
      oqml_file = lastfile;
      return s;
    }

  if (check)
    {
      check = oqml_False;
      return oqmlSuccess;
    }

  return new oqmlStatus("cannot find file '%s'", sfile.c_str());
}

const char *
oqmlImport::get_next_path(const char *oqmlpath, int &idx)
{
  static char path[512];

  const char *s = strchr(oqmlpath+idx, ':');
  if (!s)
    {
      s = oqmlpath+idx;
      idx += strlen(oqmlpath+idx)+1;
      return s;
    }

  int len = s-oqmlpath-idx;
  strncpy(path, oqmlpath+idx, len);
  path[len] = 0;
  idx += strlen(path)+1;
  return path;
}

oqmlStatus *
oqmlImport::file_to_buf(const char *file, int fd, char *&str)
{
  struct stat st;
  if (fstat(fd, &st) < 0)
    return new oqmlStatus(this, "stat error on file '%s'",
			  file);

  str = (char *)malloc(st.st_size+1);
  if (read(fd, str, st.st_size) != st.st_size)
    {
      free(str);
      return new oqmlStatus(this, "read error on file '%s'",
			    file);
    }

  str[st.st_size] = 0;
  return oqmlSuccess;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// self increment operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlSelfIncr::oqmlSelfIncr(oqmlNode * _ql, int _incr, oqmlBool _post)
  : oqmlNode(oqmlASSIGN)
{
  ql = _ql;
  incr = _incr;
  post = _post;
  if (incr > 0)
    qlassign = new oqmlAssign(ql, new oqmlAdd(ql, new oqmlInt(incr)));
  else
    qlassign = new oqmlAssign(ql, new oqmlSub(ql, new oqmlInt(-incr)));
}

oqmlSelfIncr::~oqmlSelfIncr()
{
}

oqmlStatus *oqmlSelfIncr::compile(Database *db, oqmlContext *ctx)
{
  return qlassign->compile(db, ctx);
}

oqmlStatus *oqmlSelfIncr::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = qlassign->eval(db, ctx, &al);

  if (s)
    return s;
  
  if (!post)
    {
      *alist = al;
      return oqmlSuccess;
    }

  *alist = new oqmlAtomList();
  oqmlAtom *a = al->first;
  while (a)
    {
      oqmlAtom *x;
      if (a->type.type == oqmlATOM_INT)
	x = new oqmlAtom_int(OQML_ATOM_INTVAL(a)-incr);
      else if (a->type.type == oqmlATOM_DOUBLE)
	x = new oqmlAtom_double(OQML_ATOM_DBLVAL(a)-incr);
      else if (a->type.type == oqmlATOM_CHAR)
	x = new oqmlAtom_int(OQML_ATOM_CHARVAL(a)-incr);
      else
	new oqmlStatus(this, "cannot performed operation on '%s'",
		      (incr > 0 ? "++" : "--"), a->type.getString());
      (*alist)->append(x);
      a = a->next;
    }

  return oqmlSuccess;
}

void oqmlSelfIncr::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  qlassign->evalType(db, ctx, at);
}

oqmlBool oqmlSelfIncr::isConstant() const
{
  return qlassign->isConstant();
}

std::string
oqmlSelfIncr::toString(void) const
{
  const char *opstr = (incr > 0 ? "++" : "--");
  if (post)
    {
      if (is_statement)
	return ql->toString() + opstr + "; ";
      return std::string("(") + ql->toString() + opstr + std::string(")");
    }

  return oqml_unop_string(ql, opstr, is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// unary tilde operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlTilde::oqmlTilde(oqmlNode * _ql) : oqmlNode(oqmlTILDE)
{
  ql = _ql;
}

oqmlTilde::~oqmlTilde()
{
}

oqmlStatus *oqmlTilde::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = ql->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlTilde::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  oqmlAtomList *al;
  s = ql->eval(db, ctx, &al);
  if (s)
    return s;

  if (al->cnt > 1)
    return new oqmlStatus(this, "cannot perform operation on"
			 " a non atomic operand");
  if (al->cnt == 0)
    return new oqmlStatus(this, "cannot perform operation on"
			 " a nil operand");

  oqmlAtom *a = al->first;
  if (a->type.type == oqmlATOM_INT)
    *alist = new oqmlAtomList(new oqmlAtom_int(~((oqmlAtom_int *)a)->i));
  else if (a->type.type == oqmlATOM_CHAR)
    *alist = new oqmlAtomList(new oqmlAtom_int(~(int)((oqmlAtom_char *)a)->c));
  else
    return new oqmlStatus(this, "operation '~%s' is not valid",
			 a->type.getString());
  return oqmlSuccess;
}

void oqmlTilde::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlTilde::isConstant() const
{
  return OQMLBOOL(ql->isConstant());
}

std::string
oqmlTilde::toString(void) const
{
  return oqml_unop_string(ql, "~", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// standard binary operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef oqmlStatus * (*oqml_check_fun)(oqmlNode *node, oqmlAtom *, oqmlAtom *a);

static oqmlStatus *
check_right_nonzero(oqmlNode *node, oqmlAtom *, oqmlAtom *a)
{
  oqmlATOMTYPE at = a->type.type;

  if (at == oqmlATOM_INT)
    {
      if (!OQML_ATOM_INTVAL(a))
	return new oqmlStatus(node,
			      "right operand: invalid null integer value");
      return oqmlSuccess;
    }

  if (at == oqmlATOM_DOUBLE)
    {
      if (!OQML_ATOM_DBLVAL(a))
	return new oqmlStatus(node,
			      "right operand: invalid null float value");
      return oqmlSuccess;
    }

  if (at == oqmlATOM_CHAR)
    {
      if (!OQML_ATOM_CHARVAL(a))
	return new oqmlStatus(node,
			      "right operand: invalid null character value");
      return oqmlSuccess;
    }

  return oqmlSuccess;
}

std::string
oqml_binop_string(oqmlNode *qleft, oqmlNode *qright, const char *opstr,
		  oqmlBool is_statement)
{
  if (is_statement)
    return qleft->toString() + opstr + qright->toString() + "; ";

  return std::string("(") + qleft->toString() + opstr + 
    qright->toString() + std::string(")");
}

std::string
oqml_unop_string(oqmlNode *ql, const char *opstr, oqmlBool is_statement)
{
  if (is_statement)
    return std::string(opstr) + ql->toString() + "; ";
  return std::string("(") + opstr + ql->toString() + std::string(")");
}

#define OQML_INT_BINOP(oqml_x, oqml_XX, OP, OPSTR, CHECK_FUN) \
oqml_x::oqml_x(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqml_XX) \
{ \
  qleft = _qleft; \
  qright = _qright; \
} \
 \
oqml_x::~oqml_x() \
{ \
} \
 \
oqmlStatus *oqml_x::compile(Database *db, oqmlContext *ctx) \
{ \
  oqmlBool iscts; \
  oqmlStatus *s; \
  s = binopCompile(db, ctx, OPSTR, qleft, qright, eval_type, oqmlIntOK, iscts); \
 \
  if (s) \
    return s; \
 \
  if (isConstant() && !cst_list) \
   { \
      oqmlAtomList *al; \
      s = eval(db, ctx, &al); \
      if (s) return s; \
      cst_list = al->copy(); \
      if (isLocked()) \
	oqmlLock(cst_list, oqml_True); \
   } \
 \
  return oqmlSuccess; \
} \
 \
oqmlStatus *oqml_x::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *) \
{ \
  if (cst_list) \
    { \
      *alist = new oqmlAtomList(cst_list); \
      return oqmlSuccess; \
    } \
 \
  oqmlAtomList *al_left, *al_right; \
  oqmlStatus *s = binopEval(db, ctx, OPSTR, eval_type, qleft, qright, oqmlIntOK, &al_left, &al_right); \
 \
  if (s) \
    return s; \
 \
  oqmlAtom *aleft = al_left->first, *aright = al_right->first; \
 \
  if (CHECK_FUN) \
    { \
      s = ((oqml_check_fun)CHECK_FUN)(this, aleft, aright); \
      if (s) return s; \
    } \
 \
  oqmlATOMTYPE at = aleft->type.type; \
 \
  if (at == oqmlATOM_INT) \
    *alist = new oqmlAtomList\
      (new oqmlAtom_int(OQML_ATOM_INTVAL(aleft) OP OQML_ATOM_INTVAL(aright))); \
 \
  else if (at == oqmlATOM_CHAR) \
    *alist = new oqmlAtomList\
      (new oqmlAtom_int(OQML_ATOM_CHARVAL(aleft) OP OQML_ATOM_CHARVAL(aright))); \
  else \
    return oqmlStatus::expected(this, "integer or character", aleft->type.getString()); \
\
  OQL_DELETE(al_left); \
  OQL_DELETE(al_right); \
\
  return oqmlSuccess; \
} \
 \
void oqml_x::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at) \
{ \
  *at = eval_type; \
} \
 \
oqmlBool oqml_x::isConstant() const \
{ \
  return OQMLBOOL(qleft->isConstant() && qright->isConstant()); \
} \
 \
std::string oqml_x::toString(void) const \
{ \
  return oqml_binop_string(qleft, qright, OPSTR, is_statement); \
}

#define OQML_BINOP(oqml_x, oqml_XX, OP, OPSTR, CHECK_FUN) \
oqml_x::oqml_x(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqml_XX) \
{ \
  qleft = _qleft; \
  qright = _qright; \
} \
 \
oqml_x::~oqml_x() \
{ \
} \
 \
oqmlStatus *oqml_x::compile(Database *db, oqmlContext *ctx) \
{ \
  oqmlBool iscts; \
  oqmlStatus *s; \
  s = binopCompile(db, ctx, OPSTR, qleft, qright, eval_type, oqmlDoubleOK, iscts); \
 \
  if (s) \
    return s; \
 \
  if (isConstant() && !cst_list) \
   { \
      oqmlAtomList *al; \
      s = eval(db, ctx, &al); \
      if (s) return s; \
      cst_list = al->copy(); \
      if (isLocked()) \
         oqmlLock(cst_list, oqml_True); \
   } \
 \
  return oqmlSuccess; \
} \
 \
oqmlStatus *oqml_x::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *) \
{ \
  if (cst_list) \
    { \
      *alist = new oqmlAtomList(cst_list); \
      return oqmlSuccess; \
    } \
 \
  oqmlAtomList *al_left, *al_right; \
  oqmlStatus *s = binopEval(db, ctx, OPSTR, eval_type, qleft, qright, oqmlDoubleOK, &al_left, &al_right); \
 \
  if (s) \
    return s; \
 \
  oqmlAtom *aleft = al_left->first, *aright = al_right->first; \
 \
  if (CHECK_FUN) \
    { \
      s = ((oqml_check_fun)CHECK_FUN)(this, aleft, aright); \
      if (s) return s; \
    } \
 \
  oqmlATOMTYPE at = aleft->type.type; \
 \
  if (at == oqmlATOM_INT) \
    *alist = new oqmlAtomList \
      (new oqmlAtom_int(OQML_ATOM_INTVAL(aleft) OP OQML_ATOM_INTVAL(aright))); \
 \
  else if (at == oqmlATOM_CHAR) \
    *alist = new oqmlAtomList \
      (new oqmlAtom_int(OQML_ATOM_CHARVAL(aleft) OP OQML_ATOM_CHARVAL(aright))); \
  else if (at == oqmlATOM_DOUBLE) \
    *alist = new oqmlAtomList \
      (new oqmlAtom_double(OQML_ATOM_DBLVAL(aleft) OP OQML_ATOM_DBLVAL(aright))); \
  else \
    return oqmlStatus::expected(this, "integer, character or double", aleft->type.getString()); \
\
  OQL_DELETE(al_left); \
  OQL_DELETE(al_right); \
\
  return oqmlSuccess; \
} \
 \
void oqml_x::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at) \
{ \
  *at = eval_type; \
} \
 \
oqmlBool oqml_x::isConstant() const \
{ \
  return OQMLBOOL(qleft->isConstant() && qright->isConstant()); \
} \
std::string oqml_x::toString(void) const \
{ \
  if (qleft->getType() == oqmlINT)\
    return oqml_unop_string(qright, OPSTR, is_statement); \
  return oqml_binop_string(qleft, qright, OPSTR, is_statement); \
}

OQML_BINOP(oqmlMul, oqmlMUL, *, "*", 0)
OQML_BINOP(oqmlDiv, oqmlDIV, /, "/", check_right_nonzero)

OQML_INT_BINOP(oqmlMod, oqmlMOD, %, "%", check_right_nonzero)
OQML_INT_BINOP(oqmlAAnd, oqmlAAND, &, "&", 0)
OQML_INT_BINOP(oqmlAOr, oqmlAOR, |, "|", 0)
OQML_INT_BINOP(oqmlXor, oqmlXOR, ^, "^", 0)
OQML_INT_BINOP(oqmlShr, oqmlSHR, >>, ">>", 0)
OQML_INT_BINOP(oqmlShl, oqmlSHL, <<, "<<", 0)
  }
