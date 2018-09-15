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


#include <assert.h>
#include "oql_p.h"

namespace eyedb {

  int oqmlCallLevel;
  static const char OQML_RETURN_MAGIC[] = "$oqml$return$magic$";

#define oqml_is_return(S) ((S) && !strcmp((S)->msg, OQML_RETURN_MAGIC))

  int OQML_EVAL_ARGS;

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // function module
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // -----------------------------------------------------------------------
  //
  // static utility classes and functions
  //
  // -----------------------------------------------------------------------

  struct oqmlOQMLFunctionDesc {
    oqml_ParamList *param_list;
    oqmlNode *body;

    oqmlOQMLFunctionDesc(oqml_ParamList *_param_list,
			 oqmlNode *_body) {
      param_list = _param_list;
      body = _body;
      lock();
    }

    std::string toString(const char *ident) const {
      std::string s = std::string(ident) + "(";
      if (param_list)
	s += param_list->toString();
      s += ") ";

      return s + (body ? body->toString() : std::string(""));
    }

    void lock() {
      if (body)
	body->lock();

      if (param_list)
	param_list->lock();
    }

    void unlock() {
      if (body)
	body->unlock();

      if (param_list)
	param_list->unlock();
    }

    ~oqmlOQMLFunctionDesc() {
      unlock();
      delete param_list;
    }
  };
  
  struct oqmlFunctionEntry {
    char *ident;

    oqmlOQMLFunctionDesc *OQMLdesc;

    oqmlFunctionEntry *prev, *next;

    oqmlFunctionEntry(const char *_ident, oqml_ParamList *_param_list,
		      oqmlNode *_body) {
      ident = strdup(_ident);
      OQMLdesc = new oqmlOQMLFunctionDesc(_param_list, _body);
      prev = next = 0;
      oqml_append(OQML_ATOM_COLLVAL(oqml_functions), ident);
    }

    std::string toString() const {
      return OQMLdesc->toString(ident);
    }

    ~oqmlFunctionEntry() {

      oqml_suppress(OQML_ATOM_COLLVAL(oqml_functions), ident);
      free(ident);
      delete OQMLdesc;
    }
  };

  oqml_ParamLink::oqml_ParamLink(const char *_ident, oqmlNode *_node)
  {
    next = 0;
    node = _node;
    if (_ident && *_ident == '@')
      {
	ident = strdup(&_ident[1]);
	unval = oqml_True;
	return;
      }

    ident = strdup(_ident);
    unval = oqml_False;
  }

  oqml_ParamLink::~oqml_ParamLink()
  {
    free(ident);
  }

  oqml_ParamList::oqml_ParamList(const char *ident, oqmlNode *node)
  {
    first = new oqml_ParamLink(ident, node);
    last = first;
    cnt = 1;
    min_cnt = 0;
  }

  void oqml_ParamList::add(const char *ident, oqmlNode *node)
  {
    oqml_ParamLink *l = new oqml_ParamLink(ident, node);

    last->next = l;
    last = l;
    cnt++;
  }

  void
  oqml_ParamList::lock()
  {
    oqml_ParamLink *pl = first;

    while (pl)
      {
	if (pl->node)
	  pl->node->lock();
	pl = pl->next;
      }
  }

  void
  oqml_ParamList::unlock()
  {
    oqml_ParamLink *pl = first;

    while (pl)
      {
	if (pl->node)
	  pl->node->unlock();
	pl = pl->next;
      }
  }

  std::string
  oqml_ParamList::toString() const
  {
    std::string s;
    oqml_ParamLink *pl = first;

    for (int n = 0; pl; n++)
      {
	if (n) s += ",";
	if (pl->unval)
	  s += std::string("|");
	s += pl->ident;
	if (pl->node)
	  s += std::string("?") + pl->node->toString();
	pl = pl->next;
      }

    return s;
  }

  oqml_ParamList::~oqml_ParamList()
  {
    oqml_ParamLink *l = first;

    while (l)
      {
	oqml_ParamLink *next = l->next;
	delete l;
	l = next;
      }
  }

  void oqmlContext::popFunction(const char *ident)
  {
    oqmlFunctionEntry *entry;

    if (getFunction(ident, &entry))
      {
	if (entry->prev)
	  entry->prev->next = entry->next;
	if (entry->next)
	  entry->next->prev = entry->prev;

	if (entry == symtab->flast)
	  symtab->flast = entry->prev;
	if (entry == symtab->ffirst)
	  symtab->ffirst = entry->next;

	delete entry;
      }
  }

  oqmlStatus *
  oqmlContext::setFunction(const char *ident, oqml_ParamList *param_list,
			   oqmlNode *body)
  {
    oqmlFunctionEntry *entry;
    if (getFunction(ident, &entry) &&
	entry->OQMLdesc->body == body &&
	entry->OQMLdesc->param_list == param_list)
      return oqmlSuccess;

    popFunction(ident);

    entry = new oqmlFunctionEntry(ident, param_list, body);

    if (symtab->flast)
      {
	symtab->flast->next = entry;
	entry->prev = symtab->flast;
      }
    else
      symtab->ffirst = entry;

    symtab->flast = entry;

    return oqmlSuccess;
  }

  int oqmlContext::getFunction(const char *ident, oqmlFunctionEntry **pentry)
  {
    if (!strncmp(ident, oqml_global_scope, oqml_global_scope_len))
      ident = &ident[oqml_global_scope_len];

    oqmlFunctionEntry *entry = symtab->ffirst;

    while (entry)
      {
	if (!strcmp(entry->ident, ident))
	  {
	    *pentry = entry;
	    return 1;
	  }

	entry = entry->next;
      }

    return 0;
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlFunction operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlFunction::oqmlFunction(const char *_name, oqml_ParamList *_param_list,
			     oqmlNode *_ql) : oqmlNode(oqmlFUNCTION)
  {
    name = strdup(_name);
    ql = _ql;
    param_list = _param_list;

    eval_type.type = oqmlATOM_IDENT;
  }

  oqmlFunction::~oqmlFunction()
  {
    free(name);
  }

  oqmlBool
  checkInList(oqml_ParamLink *pl, int nx)
  {
    const char *ident = pl->ident;
    for (int n = 0; pl; pl = pl->next, n++)
      if (n > nx && !strcmp(pl->ident, ident))
	return oqml_True;

    return oqml_False;
  }

  oqmlStatus *oqmlFunction::compile(Database *db, oqmlContext *ctx)
  {
    oqml_ParamLink *pl = param_list ? param_list->first : 0;

    int min_cnt, n;
    for (min_cnt = 0, n = 0; pl; pl = pl->next, n++)
      {
	if (checkInList(pl, n))
	  return new oqmlStatus(this, "duplicate identifier '%s' in parameter "
				"list", pl->ident);

	if (!pl->node)
	  {
	    if (min_cnt != n)
	      return new oqmlStatus(this, "default arguments must be at the end "
				    "of the parameter list");
	    min_cnt++;
	  }
      }

    if (param_list)
      param_list->min_cnt = min_cnt;
    return ctx->setFunction(name, param_list, ql);
  }

  oqmlStatus *oqmlFunction::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;

    *alist = new oqmlAtomList(new oqmlAtom_ident(name));
    return oqmlSuccess;
  }

  void oqmlFunction::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlFunction::isConstant() const
  {
    return oqml_False;
  }

  void
  oqmlFunction::lock()
  {
    oqmlNode::lock();
    if (ql) ql->lock();
    if (param_list) param_list->lock();
  }

  void
  oqmlFunction::unlock()
  {
    oqmlNode::unlock();
    if (ql) ql->unlock();
    if (param_list) param_list->unlock();
  }

  std::string
  oqmlFunction::toString(void) const
  {
    std::string s = is_statement ? std::string("function ") + name + "(" :
      std::string("define ") + name + "(";

    if (param_list)
      s += param_list->toString();

    if (is_statement)
      return s + ") " + ql->toString() + "; ";

    return s + ") as " + ql->toString();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlBodyOf operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define NEWBODYOF

  oqmlBodyOf::oqmlBodyOf(oqmlNode * _ql) : oqmlNode(oqmlBODYOF)
  {
    ql = _ql;
    ident = 0;
    eval_type.type = oqmlATOM_STRING;
  }

  oqmlBodyOf::~oqmlBodyOf()
  {
    free(ident);
  }

  oqmlStatus *oqmlBodyOf::compile(Database *db, oqmlContext *ctx)
  {
#ifdef NEWBODYOF
    return oqml_opident_compile(this, db, ctx, ql, ident);
#else
    oqmlStatus *s = ql->compile(db, ctx);
    if (s) return s;

    oqmlAtomType at;
    ql->evalType(db, ctx, &at);

    if (at.type == oqmlATOM_UNKNOWN_TYPE)
      return oqmlSuccess;

    if (at.type != oqmlATOM_IDENT)
      return oqmlStatus::expected(this, "ident", at.getString());

    return oqmlSuccess;
#endif
  }

  oqmlStatus *oqmlBodyOf::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
#ifdef NEWBODYOF
    oqmlStatus *s = oqml_opident_preeval(this, db, ctx, ql, ident);
    if (s) return s;
#else

    oqmlAtomList *al;
    oqmlStatus *s = ql->eval(db, ctx, &al);
    if (s) return s;

    if (!al->cnt)
      return oqmlStatus::expected(this, "ident", "nil");

    oqmlAtom *r = al->first;

    if (r->type.type != oqmlATOM_IDENT)
      return oqmlStatus::expected(this, "ident", r->type.getString());

    const char *ident = OQML_ATOM_IDENTVAL(r);
#endif
    oqmlFunctionEntry *entry;
    if (!ctx->getFunction(ident, &entry))
      return new oqmlStatus(this, "unknown function '%s'", ident);

    (*alist) = new oqmlAtomList(new oqmlAtom_string(entry->toString().c_str()));
    return oqmlSuccess;
  }

  void oqmlBodyOf::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlBodyOf::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlBodyOf::toString(void) const
  {
    if (is_statement)
      return std::string("bodyof ") + ql->toString() + "; ";
    return std::string("(bodyof ") + ql->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlCall operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /*
    oqmlCall::oqmlCall(const char *_name, oqml_List *_list) : oqmlNode(oqmlCALL)
    {
    name = strdup(_name);
    list = _list;
    ql = 0;
    qlbuiltin = 0;
    }

    oqmlCall::oqmlCall(const char *_name, oqmlNode *_oqml) : oqmlNode(oqmlCALL)
    {
    name = strdup(_name);
    list = new oqml_List();
    if (_oqml)
    list->add(_oqml);
    ql = 0;
    qlbuiltin = 0;
    }
  */

  oqmlCall::oqmlCall(oqmlNode *_ql, oqml_List *_list) : oqmlNode(oqmlCALL)
  {
    name = 0;
    ql = _ql;
    list = _list;
    qlbuiltin = 0;
    compiling = oqml_False;
  }

  oqmlCall::~oqmlCall()
  {
    free(name);
    if (!qlbuiltin)
      delete list;
  }

  oqmlStatus *oqmlCall::preCompile(Database *db, oqmlContext *ctx)
  {
    deferredEval = oqml_False;
    last_entry = 0;
    last_builtin = 0;

    if (!ql)
      return oqmlSuccess;

    free(name);

    if (ql->getType() == oqmlIDENT)
      {
	name = strdup(((oqmlIdent *)ql)->getName());

	if (ctx->getSymbol(name))
	  deferredEval = oqml_True;

	return oqmlSuccess;
      }

    oqmlStatus *s;

    s = ql->compile(db, ctx);

    if (s) return s;

    oqmlAtomList *al;
    s = ql->eval(db, ctx, &al);
    if (s) return s;
  
    if (al->cnt != 1 || al->first->type.type != oqmlATOM_IDENT)
      return new oqmlStatus(this, "invalid function '%s'",
			    ql->toString().c_str());
  
    name = strdup(OQML_ATOM_IDENTVAL(al->first));

    return oqmlSuccess;
  }

  oqmlStatus *oqmlCall::compile(Database *db, oqmlContext *ctx)
  {
    if (compiling)
      return oqmlSuccess;

    compiling = oqml_True;

    oqmlStatus *s;
    int n;
    qlbuiltin = 0;

    //
    // TBD: il faudrait tester que:
    // (1) il n'y a aucun argument identique dans une param list
    // (2) il n'existe pas d'argument en commun entre la param list et
    //     la local param list
    s = preCompile(db, ctx);

    if (s || deferredEval)
      {
	compiling = oqml_False;
	return s;
      }

    s = postCompile(db, ctx, oqml_False);
    compiling = oqml_False;
    return s;
  }

  oqmlStatus *oqmlCall::postCompile(Database *db, oqmlContext *ctx,
				    oqmlBool checkSymbol)
  {
    oqmlStatus *s;

    const char *fname = 0;
    if (checkSymbol)
      {
	oqmlAtom *x;
	if (ctx->getSymbol(name, 0, &x))
	  {
	    if (!x || !OQML_IS_IDENT(x))
	      return new oqmlStatus(this,
				    "identifier '%s': function expected, "
				    "got '%s'",
				    name, x ? x->type.getString() : "nil");
	    fname = OQML_ATOM_IDENTVAL(x);
	  }
      }

    if (!fname)
      fname = name;

    int found = ctx->getFunction(fname, &entry);
  
    if (checkBuiltIn(db, this, fname, found))
      {
	if (qlbuiltin == last_builtin)
	  return oqmlSuccess;

	if (isLocked())
	  qlbuiltin->lock();

	s = qlbuiltin->compile(db, ctx);
	if (s) return s;
	last_builtin = qlbuiltin;
	return oqmlSuccess;
      }

    if (!found)
      return new oqmlStatus(this, "unknown function '%s'", fname);
  
    if (entry == last_entry)
      return oqmlSuccess;

    int param_list_max_cnt = entry->OQMLdesc->param_list ?
      entry->OQMLdesc->param_list->cnt : 0;

    int param_list_min_cnt = entry->OQMLdesc->param_list ?
      entry->OQMLdesc->param_list->min_cnt : 0;

    int list_cnt = list ? list->cnt : 0;

    if (list_cnt > param_list_max_cnt)
      return new oqmlStatus(this, "function %s expects at most %d arguments, "
			    "got %d", fname, param_list_max_cnt, list_cnt);

    if (list_cnt < param_list_min_cnt)
      return new oqmlStatus(this, "function %s expects at least %d arguments, "
			    "got %d", fname, param_list_min_cnt, list_cnt);
      
#ifdef CALL_TRACE
    printf("\n%s: compiling function %s %p ctx=%p level=%d\n",
	   (const char *)toString(), fname, this, ctx, ctx->getLocalDepth());
#endif
    s = ctx->pushLocalTable();
    if (s) return s;

    oqml_ParamLink *pl;
    oqml_Link *l = (list ? list->first : 0);

    pl = entry->OQMLdesc->param_list ? entry->OQMLdesc->param_list->first : 0;

    //  for (int n = 0; l; n++)
    for (int n = 0; pl; n++)
      {
	if (pl->unval)
	  {
	    oqmlAtomType at(oqmlATOM_STRING);
	    ctx->pushSymbol(pl->ident, &at);
	  }
	else
	  {
	    ctx->pushArgLevel();
	    if (l)
	      s = l->ql->compile(db, ctx);
	    else if (pl->node)
	      s = pl->node->compile(db, ctx);
	    else
	      s = new oqmlStatus(this, "mandatory parameter '%s' is missing",
				 pl->ident);

	    ctx->popArgLevel();

	    if (s)
	      {
		ctx->popLocalTable();
		return s;
	      }
	  
	    oqmlAtomType at;
	    if (l)
	      l->ql->evalType(db, ctx, &at);
	    else
	      pl->node->evalType(db, ctx, &at);
	    ctx->pushSymbol(pl->ident, &at);
	  }

	if (l)
	  l = l->next;
	pl = pl->next;
      }

    if (entry->OQMLdesc->body)
      s = entry->OQMLdesc->body->compile(db, ctx);
    else
      s = oqmlSuccess;

    ctx->popLocalTable();

    if (!s)
      last_entry = entry;
    return s;
  }

  oqmlStatus *oqmlCall::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;

    // added the 3/07/01
    int select_ctx_cnt = ctx->setSelectContextCount(0);

    if (deferredEval)
      {
	s = postCompile(db, ctx, oqml_True);
	if (s) return s;
      }

    int level = ++oqmlCallLevel;

    if (qlbuiltin)
      s = qlbuiltin->eval(db, ctx, alist);
    else
      s = realize(db, ctx, alist);

    --oqmlCallLevel;

    if (s && !oqml_is_return(s))
      return s;

    if (oqml_is_return(s)) {
#ifdef SYNC_GARB
      OQL_DELETE(*alist);
#endif
      (*alist) = new oqmlAtomList(s->returnAtom);
      delete s;

      // added the 3/07/01
      ctx->setSelectContextCount(select_ctx_cnt);
      return oqmlSuccess;
    }

    // added the 3/07/01
    ctx->setSelectContextCount(select_ctx_cnt);

    return s;
  }

  oqmlStatus *oqmlCall::realize(Database *db, oqmlContext *ctx,
				oqmlAtomList **alist)
  {
    oqmlStatus *s;
    int n;
    oqmlAtom *a;

    oqml_Link *l;
    oqml_ParamLink *pl;

#ifdef CALL_TRACE
    printf("\n%s: calling function %s %p ctx=%p level=%d\n",
	   (const char *)toString(), entry->ident, this, ctx, ctx->getLocalDepth());
#endif
    s = ctx->pushLocalTable();
    if (s) return s;

    l = list ? list->first : 0;
    pl = entry->OQMLdesc->param_list ? entry->OQMLdesc->param_list->first : 0;
      
    //for (n = 0; l; n++)
    for (n = 0; pl; n++)
      {
	oqmlAtomList *al;

	if (pl->unval)
	  {
	    oqmlAtomType at(oqmlATOM_STRING);
	    if (l)
	      ctx->pushSymbol(pl->ident, &at,
			      new oqmlAtom_string(l->ql->toString().c_str()));
	    else if (pl->node)
	      ctx->pushSymbol(pl->ident, &at,
			      new oqmlAtom_string(pl->node->toString().c_str()));
	  }
	else
	  {
	    ctx->pushArgLevel();
	    if (l)
	      s = l->ql->eval(db, ctx, &al);
	    else if (pl->node)
	      s = pl->node->eval(db, ctx, &al);
	    else
	      s = new oqmlStatus(this, "mandatory parameter '%s' is missing",
				 pl->ident);
	    ctx->popArgLevel();

	    if (s)
	      {
		ctx->popLocalTable();
		return s;
	      }	  
	  
	    a = (al ? al->first : 0);

	    ctx->pushSymbol(pl->ident, (a ? &a->type : 0), a);
#ifdef SYNC_GARB
	    if (al && !al->refcnt) {
	      al->first = 0;
	      OQL_DELETE(al);
	    }
#endif
	  }

	if (l)
	  l = l->next;
	pl = pl->next;
      }
      
    if (entry->OQMLdesc->body)
      s = entry->OQMLdesc->body->eval(db, ctx, alist);
    else
      s = oqmlSuccess;

    ctx->popLocalTable();
      
    return s;
  }
  
  oqmlStatus *oqmlCall::realizePostAction(Database *db, oqmlContext *ctx,
					  const char *name,
					  oqmlFunctionEntry *entry, 
					  oqmlAtom_string *rs,
					  oqmlAtom *ra,
					  oqmlAtomList **alist)
  {
    int cnt = !entry->OQMLdesc->param_list ? cnt :
      entry->OQMLdesc->param_list->cnt;

    if (cnt != 2)
      return new oqmlStatus("postaction function %s: "
			    "expected 2 arguments, got %d",
			    name, cnt);
    oqmlStatus *s;
    oqml_ParamLink *pl;

    pl = entry->OQMLdesc->param_list->first;

    ctx->pushSymbol(pl->ident, &rs->type, rs, oqml_False);
    ctx->pushSymbol(pl->next->ident, (ra ? &ra->type : 0), ra, oqml_False);
      
    if (entry->OQMLdesc->body) {
      s = entry->OQMLdesc->body->compile(db, ctx);
      if (!s)
	s = entry->OQMLdesc->body->eval(db, ctx, alist);
    }
    else
      s = oqmlSuccess;

    pl = entry->OQMLdesc->param_list->first;
    ctx->popSymbol(pl->ident, oqml_False);
    ctx->popSymbol(pl->next->ident, oqml_False);
      
    return s;
  }
  
  oqmlStatus *oqmlCall::realizeCall(Database *db, oqmlContext *ctx,
				    oqmlFunctionEntry *entry, 
				    oqmlAtomList **alist)
  {
    oqmlStatus *s;
    if (entry->OQMLdesc->body) {
      s = entry->OQMLdesc->body->compile(db, ctx);
      if (s) return s;
    }

    int level = ++oqmlCallLevel;

    if (entry->OQMLdesc->body)
      s = entry->OQMLdesc->body->eval(db, ctx, alist);
    else
      s = oqmlSuccess;

    --oqmlCallLevel;

    if (s && !oqml_is_return(s))
      return s;

    if (oqml_is_return(s))
      {
	(*alist) = new oqmlAtomList(s->returnAtom);
	delete s;
	return oqmlSuccess;
      }

    return s;
  }

  void oqmlCall::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlCall::isConstant() const
  {
    return oqml_False;
  }

  oqmlBool
  oqmlCall::checkBuiltIn(Database *db, oqmlNode *node, const char *fname,
			 int found)
  {
    return getBuiltIn(db, node, fname, found, &qlbuiltin, list);
  }

  oqmlBool
  oqmlCall::getBuiltIn(Database *db, oqmlNode *node, const char *name,
		       int found, oqmlNode **pqlbuiltin, oqml_List *list)
  {
    if (!strcmp(name, "list"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlListColl(list);
	return oqml_True;
      }

    if (!strcmp(name, "set"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlSetColl(list);
	return oqml_True;
      }

    if (!strcmp(name, "array"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlArrayColl(list);
	return oqml_True;
      }

    if (!strcmp(name, "bag"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlBagColl(list);
	return oqml_True;
      }

    if (!strcmp(name, "timeformat"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlTimeFormat(list);
	return oqml_True;
      }

    if (!strcmp(name, "sort"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlSort(list, oqml_False);
	return oqml_True;
      }

    if (!strcmp(name, "rsort"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlSort(list, oqml_True);
	return oqml_True;
      }

    if (!strcmp(name, "isort"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlISort(list, oqml_False);
	return oqml_True;
      }

    if (!strcmp(name, "risort"))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlISort(list, oqml_True);
	return oqml_True;
      }
  
    if (!found && db->getSchema()->getClass(name) && (!list || !list->cnt))
      {
	if (pqlbuiltin)
	  *pqlbuiltin = new oqmlNew(OQML_NEW_HINT(), name, (oqmlNode *)0);
	return oqml_True;
      }

    /*
      if (!found)
      {
      if (pqlbuiltin)
      *pqlbuiltin = new oqmlMethodCall("oql", name, list, node);
      return oqml_True;
      }
    */

    // -------------------------- NOT YET IMPLEMENTED ------------------------
    /*
      if (!strcmp(name, "psort"))
      {
      if (pqlbuiltin)
      *pqlbuiltin = new oqmlPSort(list, oqml_False);
      return oqml_True;
      }

      if (!strcmp(name, "rpsort"))
      {
      if (pqlbuiltin)
      *pqlbuiltin = new oqmlPSort(list, oqml_True);
      return oqml_True;
      }
    */

    return oqml_False;
  }

  std::string
  oqmlCall::toString(void) const
  {
    std::string s = (ql ? ql->toString() : std::string(name)) + "(";
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

  oqmlBool
  oqmlCall::hasIdent(const char *_ident)
  {
    return OQMLBOOL(((ql && ql->hasIdent(_ident)) ||
		     (qlbuiltin && qlbuiltin->hasIdent(_ident)) ||
		     list->hasIdent(_ident)));
  }

  oqmlStatus *
  oqmlCall::requalify(Database *db, oqmlContext *ctx,
		      const char *ident, oqmlNode *node, oqmlBool &done)
  {
    if (!list)
      return oqmlSuccess;

    oqml_Link *l = list->first;

    while (l)
      {
	oqmlStatus *s = l->ql->requalify(db, ctx, ident, node, done);
	if (s) return s;
	l = l->next;
      }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlCall::requalify(Database *db, oqmlContext *ctx,
		      const Attribute **attrs, int attr_cnt,
		      const char *ident)
  {
    if (!list)
      return oqmlSuccess;

    oqml_Link *l = list->first;

    while (l)
      {
	oqmlStatus *s = l->ql->requalify(db, ctx, attrs, attr_cnt, ident);
	if (s) return s;
	l = l->next;
      }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlCall::requalify_back(Database *db, oqmlContext *ctx)
  {
    if (!list)
      return oqmlSuccess;

    oqml_Link *l = list->first;

    while (l)
      {
	oqmlStatus *s = l->ql->requalify_back(db, ctx);
	if (s) return s;
	l = l->next;
      }

    return oqmlSuccess;
  }

  void
  oqmlCall::lock()
  {
    oqmlNode::lock();
    if (ql) ql->lock();
    if (qlbuiltin) qlbuiltin->lock();
    if (list) list->lock();
  }

  void
  oqmlCall::unlock()
  {
    oqmlNode::unlock();
    if (ql) ql->unlock();
    if (qlbuiltin) qlbuiltin->unlock();
    if (list) list->unlock();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlReturn operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlReturn::oqmlReturn(oqmlNode * _ql) : oqmlNode(oqmlRETURN)
  {
    ql = _ql;
  }

  oqmlReturn::~oqmlReturn()
  {
  }

  oqmlStatus *oqmlReturn::compile(Database *db, oqmlContext *ctx)
  {
    if (ql)
      return ql->compile(db, ctx);

    return oqmlSuccess;
  }

  oqmlStatus *oqmlReturn::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    if (oqmlCallLevel == 0)
      return new oqmlStatus(this, "return must be performed in a function");

    oqmlStatus *s;
    oqmlAtom *r;

    if (ql)
      {
	oqmlAtomList *al;
      
	s = ql->eval(db, ctx, &al); 

	if (s)
	  return s;

	r = al->first;
      }
    else
      r = 0;

    s = new oqmlStatus(OQML_RETURN_MAGIC);
    s->returnAtom = r;
    oqmlLock(s->returnAtom, oqml_True, oqml_False);
    return s;
  }

  void oqmlReturn::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlReturn::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlReturn::toString(void) const
  {
    return std::string("return") +
      (ql ? std::string(" ") + ql->toString() : std::string("")) + oqml_isstat();
  }

  oqmlStatus *
  oqml_realize_postaction(Database *db, oqmlContext *ctx, const char *ident,
			  oqmlAtom_string *rs, oqmlAtom *ra,
			  oqmlAtomList **alist)
  {
    oqmlFunctionEntry *entry;

    if (!ctx->getFunction(ident, &entry))
      return new oqmlStatus("postactions: unknown function '%s'.", ident);

    /*
      printf("executing postaction %s(%s, %s)\n", ident,
      rs->getString(), ra ? ra->getString() : "");
    */

    return oqmlCall::realizePostAction(db, ctx, ident, entry, rs, ra, alist);
  }

}
  
