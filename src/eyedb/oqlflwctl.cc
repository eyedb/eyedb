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

#define EXT_LOGIC

namespace eyedb {

  int oqmlLoopLevel, oqmlBreakLevel;
  static const char OQML_BREAK_MAGIC[] = "$oqml$break$magic$";

#define oqml_is_break(S) ((S) && !strcmp((S)->msg, OQML_BREAK_MAGIC))

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlIf operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlIf::oqmlIf(oqmlNode * _qcond, oqmlNode * _qthen, oqmlNode *_qelse,
		 oqmlBool _is_cond_expr) :
    oqmlNode(oqmlIF)
  {
    qcond = _qcond;
    qthen = _qthen;
    qelse = _qelse;
    qthen_compiled = oqml_False;
    qelse_compiled = oqml_False;
    is_cond_expr = _is_cond_expr;
  }

  oqmlIf::~oqmlIf()
  {
  }

  oqmlStatus *oqmlIf::compile(Database *db, oqmlContext *ctx)
  {
    return qcond->compile(db, ctx);
  }

  oqmlStatus *oqmlIf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;
    oqmlAtomList *al = 0;
    oqmlNode *toeval;
    oqmlBool *compiled = 0;

    s = qcond->eval(db, ctx, &al);
    if (s)
      return s;

    if (!al->cnt) {
      toeval = qelse;
      compiled = &qelse_compiled;
    }
    else if (al->cnt == 1) {
      oqmlAtom *a = al->first;
#ifdef EXT_LOGIC
      oqmlBool b;
      s = oqml_check_logical(this, al, b);
      if (s)
	return s;
      
      if (b) {
	toeval = qthen;
	compiled = &qthen_compiled;
      }
      else {
	toeval = qelse;
	compiled = &qelse_compiled;
      }
#else
      if (!OQML_IS_BOOL(a))
	return new oqmlStatus(this, "boolean expected for condition");
      
      if (OQML_ATOM_BOOLVAL(a)) {
	toeval = qthen;
	compiled = &qthen_compiled;
      }
      else {
	toeval = qelse;
	compiled = &qelse_compiled;
      }
#endif
      }
    else
      toeval = qthen;

#ifdef SYNC_GARB
    OQL_DELETE(al);
#endif

    *alist = 0;

    if (toeval) {
      if (!*compiled) {
	s = toeval->compile(db, ctx);
	if (s) return s;
	*compiled = oqml_True;
      }

      s = toeval->eval(db, ctx, alist);
      if (s) return s;
    }
    else
      *alist = new oqmlAtomList();

    if (!is_cond_expr && toeval) {
#ifdef SYNC_GARB
      OQL_DELETE(*alist);
#endif
      *alist = new oqmlAtomList();
    }

    return oqmlSuccess;
  }

  oqmlBool
  oqmlIf::hasIdent(const char *_ident)
  {
    return OQMLBOOL(qcond->hasIdent(_ident) ||
		    qthen->hasIdent(_ident) ||
		    (qelse && qelse->hasIdent(_ident)));
  }

  void oqmlIf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlIf::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlIf::toString(void) const
  {
    if (is_statement)
      return std::string("if (") + qcond->toString() + ") " + qthen->toString() +
	(qelse ? std::string(" else " ) + qelse->toString() : std::string(""));
    //oqml_isstat();
  
    return std::string("(") + qcond->toString() + "?" + qthen->toString() +
      ":" + qelse->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlForEach operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlForEach::oqmlForEach(const char * _ident, oqmlNode * _in,
			   oqmlNode * _action) : oqmlNode(oqmlFOREACH)
  {
    ident = strdup(_ident);
    in = _in;
    action = _action;
  }

  oqmlForEach::~oqmlForEach()
  {
    free(ident);
  }

  void oqmlForEach::setAction(oqmlNode *_action)
  {
    action = _action;
  }

  oqmlStatus *oqmlForEach::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = in->compile(db, ctx);
    if (s) return s;

    if (action)
      {
	oqmlAtomType at;
	ctx->pushSymbol(ident, &at);
	s = action->compile(db, ctx);
	ctx->popSymbol(ident);
      }

    if (s) return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlForEach::eval(Database *db, oqmlContext *ctx,
				oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;
    oqmlAtomList *al;

    s = in->eval(db, ctx, &al);
    if (s) return s;

    oqmlAtom *a = al->first;
    int iter = 0;
    int level = ++oqmlLoopLevel;

    oqmlAtomList *tal;
    if (a && OQML_IS_COLL(a)) {
      oqmlAtomList *list = OQML_ATOM_COLLVAL(a);
      oqmlAtom *x = list->first;
      while(x) {
#ifdef SYNC_GARB
	if (level == 1) {
	  //printf("foreach level #%d, iter #%d\n", level, iter);
	  iter++;
	}
#endif
	oqmlAtom * next = x->next;

	if (action) {
	  gbContext *gbctx = oqmlGarbManager::peek();
	  ctx->pushSymbol(ident, &x->type, x);
	  tal = 0;
	  s = action->eval(db, ctx, &tal);
#ifdef SYNC_GARB
	  OQL_DELETE(tal);
#endif
	  ctx->popSymbol(ident);
	  oqmlGarbManager::garbage(gbctx);
	  if (s)
	    break;
	}
      
	OQML_CHECK_INTR();
      
	x = next;
      }
    }
    else if (action && a) {
      ctx->pushSymbol(ident, &a->type, a);
#ifdef SYNC_GARB
      tal = 0;
      s = action->eval(db, ctx, &tal);
      OQL_DELETE(tal);
#else
      s = action->eval(db, ctx, alist);
#endif
      ctx->popSymbol(ident);
    }

#ifdef SYNC_GARB
    // 8/02/06
    //  delete al;
#endif
    --oqmlLoopLevel;

    *alist = new oqmlAtomList();
    if (s && !oqml_is_break(s))
      return s;

    if (oqml_is_break(s) && oqmlBreakLevel == level) {
      delete s;
      return oqmlSuccess;
    }

    return s;
  }

  void oqmlForEach::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlForEach::isConstant() const
  {
    return oqml_False;
  }

  oqmlBool
  oqmlForEach::hasIdent(const char *_ident)
  {
    return OQMLBOOL(in->hasIdent(_ident) ||
		    (action ? action->hasIdent(_ident) : oqml_False));
  }

  std::string
  oqmlForEach::toString(void) const
  {
    return std::string("for (") + ident + " in " + in->toString() + ") " +
      (action ? action->toString() : std::string("")); // + oqml_isstat();
  }

  void
  oqmlForEach::lock()
  {
    oqmlNode::lock();
    in->lock();
    if (action)
      action->lock();
  }

  void
  oqmlForEach::unlock()
  {
    oqmlNode::unlock();
    in->unlock();
    if (action)
      action->unlock();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlWhile operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlWhile::oqmlWhile(oqmlNode * _qleft, oqmlNode *_qright) : oqmlNode(oqmlWHILE)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlWhile::~oqmlWhile()
  {
  }

  oqmlStatus *oqmlWhile::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = qleft->compile(db, ctx);
    if (s) return s;

    s = qright->compile(db, ctx);
    if (s) return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlWhile::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s = oqmlSuccess;

    int level = ++oqmlLoopLevel;
    for (;;)
      {
	oqmlAtomList *al;

	s = qleft->eval(db, ctx, &al);
	if (s)
	  break;

#ifdef EXT_LOGIC
	oqmlBool b;
	s = oqml_check_logical(this, al, b);
	if (s)
	  break;

	if (!b)
	  break;
#else
	if (al->cnt != 1 || !OQML_IS_BOOL(al->first))
	  return new oqmlStatus(this, "boolean expected for condition");

	if (!OQML_ATOM_BOOLVAL(al->first))
	  break;
#endif

	OQML_CHECK_INTR();

	if (qright) {
	  gbContext *gbctx = oqmlGarbManager::peek();
	  s = qright->eval(db, ctx, &al);
	  oqmlGarbManager::garbage(gbctx);
	  if (s) break;
	}
      }

    --oqmlLoopLevel;

    *alist = new oqmlAtomList();
    if (s && !oqml_is_break(s))
      return s;

    if (oqml_is_break(s) && oqmlBreakLevel == level)
      {
	delete s;
	return oqmlSuccess;
      }

    return s;
  }

  void oqmlWhile::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlWhile::isConstant() const
  {
    return OQMLBOOL(qleft->isConstant() && qright->isConstant());
  }

  std::string
  oqmlWhile::toString(void) const
  {
    return std::string("while (") + qleft->toString() + ") " +
      (qright ? qright->toString() : std::string(""));
    //oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlDoWhile operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlDoWhile::oqmlDoWhile(oqmlNode * _qleft, oqmlNode *_qright) : oqmlNode(oqmlDOWHILE)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlDoWhile::~oqmlDoWhile()
  {
  }

  oqmlStatus *oqmlDoWhile::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = qleft->compile(db, ctx);
    if (s) return s;

    s = qright->compile(db, ctx);
    if (s) return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlDoWhile::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s = oqmlSuccess;
    oqmlBool b;

    int level = ++oqmlLoopLevel;
    do {
      oqmlAtomList *al;

      if (qright) {
	gbContext *gbctx = oqmlGarbManager::peek();
	s = qright->eval(db, ctx, &al);
	oqmlGarbManager::garbage(gbctx);
	if (s) break;
      }

      s = qleft->eval(db, ctx, &al);
      if (s) break;
    
#ifdef EXT_LOGIC
      s = oqml_check_logical(this, al, b);
      if (s)
	break;

      OQML_CHECK_INTR();
#else
      if (al->cnt != 1 || !OQML_IS_BOOL(al->first))
	return new oqmlStatus(this, "boolean expected for condition");

      OQML_CHECK_INTR();

      b = OQML_ATOM_BOOLVAL(al->first);
#endif
    
    } while(b);

    --oqmlLoopLevel;

    *alist = new oqmlAtomList();

    if (s && !oqml_is_break(s))
      return s;

    if (oqml_is_break(s) && oqmlBreakLevel == level)
      {
	delete s;
	return oqmlSuccess;
      }

    return s;
  }

  void oqmlDoWhile::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlDoWhile::isConstant() const
  {
    return OQMLBOOL(qleft->isConstant() && qright->isConstant());
  }

  std::string
  oqmlDoWhile::toString(void) const
  {
    return std::string("do ") +
      (qright ? qright->toString() : std::string(""))
      + " while " + qleft->toString(); // + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlForDo operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlForDo::oqmlForDo(oqmlNode *_start,
		       oqmlNode *_cond,
		       oqmlNode *_next,
		       oqmlNode *_body) : oqmlNode(oqmlFORDO)
  {
    ident = 0;
    start = _start;
    cond = _cond;
    next = _next;
    body = _body;
  }

  oqmlForDo::oqmlForDo(const char *_ident,
		       oqmlNode *_start,
		       oqmlNode *_cond,
		       oqmlNode *_next,
		       oqmlNode *_body) : oqmlNode(oqmlFORDO)
  {
    ident = strdup(_ident);
    start = _start;
    cond = _cond;
    next = _next;
    body = _body;
  }

  oqmlForDo::~oqmlForDo()
  {
    free(ident);
  }

  oqmlStatus *oqmlForDo::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    if (ident)
      {
	oqmlAtomType at;
	ctx->pushSymbol(ident, &at, 0, oqml_False);
      }

    if (start)
      {
	s = start->compile(db, ctx);
	if (s) return s;
      }

    if (cond)
      {
	s = cond->compile(db, ctx);
	if (s) return s;
      }

    if (next)
      {
	s = next->compile(db, ctx);
	if (s) return s;
      }

    if (body)
      {
	s = body->compile(db, ctx);
	if (s) return s;
      }

    if (ident)
      ctx->popSymbol(ident, oqml_False);

    return oqmlSuccess;
  }

  oqmlStatus *oqmlForDo::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s = oqmlSuccess;
    oqmlBool b;

    oqmlAtomList *al;

    if (ident)
      {
	oqmlAtomType at;
	ctx->pushSymbol(ident, &at, 0, oqml_False);
      }

    if (start)
      {
	s = start->eval(db, ctx, &al);
	if (s) return s;
      }
    
    int level = ++oqmlLoopLevel;

    for (;;) {
      if (cond) {
	s = cond->eval(db, ctx, &al);
	if (s)
	  return s;

#ifdef EXT_LOGIC
	oqmlBool b;
	s = oqml_check_logical(this, al, b);
	if (s)
	  break;

	if (!b)
	  break;
#else
	if (al->cnt != 1 || !OQML_IS_BOOL(al->first)) {
	  s = new oqmlStatus(this, "boolean expected for condition");
	  break;
	}

	if (!OQML_ATOM_BOOLVAL(al->first))
	  break;
#endif
      }
    
      OQML_CHECK_INTR();

      if (body) {
	gbContext *gbctx = oqmlGarbManager::peek();
	s = body->eval(db, ctx, &al);
	oqmlGarbManager::garbage(gbctx);
	if (s) break;
      }

      if (next) {
	s = next->eval(db, ctx, &al);
	if (s) break;
      }
      }

    --oqmlLoopLevel;

    if (ident)
      ctx->popSymbol(ident, oqml_False);

    *alist = new oqmlAtomList();
    if (s && !oqml_is_break(s))
      return s;

    if (oqml_is_break(s) && oqmlBreakLevel == level)
      {
	delete s;
	return oqmlSuccess;
      }

    return oqmlSuccess;
  }

  void oqmlForDo::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlForDo::isConstant() const
  {
    return oqml_False;
  }

  oqmlBool
  oqmlForDo::hasIdent(const char *_ident)
  {
    return OQMLBOOL((start ? start->hasIdent(_ident) : oqml_False)||
		    (cond ? cond->hasIdent(_ident) : oqml_False) ||
		    (next ? next->hasIdent(_ident) : oqml_False) ||
		    (body ? body->hasIdent(_ident) : oqml_False));
  }

  void oqmlForDo::lock()
  {
    oqmlNode::lock();
    if (start)
      start->lock();
    if (cond)
      cond->lock();
    if (next)
      next->lock();
    if (body)
      body->lock();
  }

  void oqmlForDo::unlock()
  {
    oqmlNode::unlock();
    if (start)
      start->unlock();
    if (cond)
      cond->unlock();
    if (next)
      next->unlock();
    if (body)
      body->unlock();
  }

  std::string
  oqmlForDo::toString(void) const
  {
    return std::string("for(") +
      (start ? start->toString() : std::string("")) + ";" +
      (cond ? cond->toString() : std::string("")) + ";" +
      (next ? next->toString() : std::string("")) + ") " +
      (body ? body->toString() : std::string(""));
    //+ oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlThrow operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlThrow::oqmlThrow(oqmlNode * _ql) : oqmlNode(oqmlTHROW)
  {
    ql = _ql;
  }

  oqmlThrow::~oqmlThrow()
  {
  }

  oqmlStatus *oqmlThrow::compile(Database *db, oqmlContext *ctx)
  {
    return ql->compile(db, ctx);
  }

  oqmlStatus *oqmlThrow::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;
    oqmlAtomList *al;

    s = ql->eval(db, ctx, &al); 

    if (s) return s;

    if (al->cnt == 1 && al->first->as_string())
      return new oqmlStatus(OQML_ATOM_STRVAL(al->first));

    if (al->cnt == 1)
      return oqmlStatus::expected(this, "string", al->first->type.getString());

    return new oqmlStatus(this, "string argument expected");
  }

  void oqmlThrow::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlThrow::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlThrow::toString(void) const
  {
    return std::string("throw ") + ql->toString() + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlBreak operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlBreak::oqmlBreak(oqmlNode * _ql) : oqmlNode(oqmlBREAK)
  {
    ql = _ql;
  }

  oqmlBreak::~oqmlBreak()
  {
  }

  oqmlStatus *oqmlBreak::compile(Database *db, oqmlContext *ctx)
  {
    if (ql)
      return ql->compile(db, ctx);

    return oqmlSuccess;
  }

  oqmlStatus *oqmlBreak::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    int level;

    if (ql)
      {
	oqmlStatus *s;
	oqmlAtomList *al;
      
	s = ql->eval(db, ctx, &al); 

	if (s) return s;

	if (al->cnt != 1 || !al->first->as_int())
	  return new oqmlStatus(this, "integer expected");

	level = OQML_ATOM_INTVAL(al->first);
      }
    else
      level = 1;

    if (level > oqmlLoopLevel)
      return new oqmlStatus(this, "level %d is too deep", level);

    oqmlBreakLevel = oqmlLoopLevel - level + 1;

    return new oqmlStatus(OQML_BREAK_MAGIC);
  }

  void oqmlBreak::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlBreak::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlBreak::toString(void) const
  {
    return std::string("break") +
      (ql ? std::string(" ") + ql->toString() : std::string("")) + oqml_isstat();
  }
}
