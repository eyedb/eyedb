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

// -----------------------------------------------------------------------
//
// utility macros
//
// -----------------------------------------------------------------------

namespace eyedb {

#define oqmlIsPureComp(T) \
  ((T) == oqmlEQUAL || \
   (T) == oqmlDIFF || \
   (T) == oqmlINF || \
   (T) == oqmlINFEQ || \
   (T) == oqmlSUP || \
   (T) == oqmlSUPEQ || \
   (T) == oqmlREGCMP || \
   (T) == oqmlREGICMP || \
   (T) == oqmlREGDIFF || \
   (T) == oqmlREGIDIFF)

#define oqmlIsComp(T) \
  ((T) == oqmlAND || (T) == oqmlLAND || \
   (T) == oqmlOR || (T) == oqmlLOR || \
   oqmlIsPureComp(T))

static oqmlAtomType selectAtomType(oqmlATOM_SELECT);

//#define TRACE

#ifdef TRACE
#define SELECT_TRACE(X) printf X
#else
#define SELECT_TRACE(X)
#endif

class SelectLog {

public:
  enum Control {
    Off = 0,
    On,
    Detail
  };

  static Control oql_select_log_ctl;
  static oqmlAtom *oql_select_log;
  static const char oql_select_log_ctl_var[];
  static const char oql_select_log_var[];
  
  static oqmlStatus *error(oqmlNode *node) {
    return new oqmlStatus(node, "%s must be a string of "
			  "one of the values: on, off or detail",
			  oql_select_log_ctl_var);
  }

  static oqmlStatus *init(oqmlNode *node, oqmlContext *ctx) {
    if (ctx->getHiddenSelectContextCount())
      return oqmlSuccess;;

    oqmlAtom *x = 0;
    if (!ctx->getSymbol(oql_select_log_ctl_var, 0, &x) || !x) {
      oql_select_log_ctl = Off;
      oql_select_log = 0;
      return oqmlSuccess;;
    }

    if (!OQML_IS_STRING(x))
      return error(node);

    const char *val = OQML_ATOM_STRVAL(x);
    if (!strcasecmp(val, "off"))
      oql_select_log_ctl = Off;
    else if (!strcasecmp(val, "on"))
      oql_select_log_ctl = On;
    else if (!strcasecmp(val, "detail"))
      oql_select_log_ctl = Detail;
    else
      return error(node);

    if (oql_select_log_ctl != Off) {
      oql_select_log = new oqmlAtom_string("");
      ctx->setSymbol(oql_select_log_var, &oql_select_log->type, oql_select_log);
    }
    else
      oql_select_log = 0;

    return oqmlSuccess;
  }

  static void append(const std::string &str) {
    oql_select_log->as_string()->set((std::string(OQML_ATOM_STRVAL(oql_select_log)) + str).c_str());
  }
};

oqmlAtom *SelectLog::oql_select_log;
SelectLog::Control SelectLog::oql_select_log_ctl = SelectLog::Off;
const char SelectLog::oql_select_log_ctl_var[] = "oql$select_log_ctl";
const char SelectLog::oql_select_log_var[] = "oql$select_log";

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlAnd methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlAnd::oqmlAnd(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlAND)
{
  qleft = _qleft;
  qright = _qright;
}

oqmlAnd::~oqmlAnd()
{
}

oqmlNode *
oqmlAnd::optimize_realize(Database *db, oqmlContext *ctx, oqmlNode *ql,
			 int &_xc)
{
  if (xc == 4)
    {
      _xc = 4;
      return ql;
    }

  oqmlNode **xql;
      
  oqmlTYPE t1 = qleft->getType();

  if (t1 == oqmlIDENT)
    {
      oqmlNode *tql = qleft;
      qleft = ql;
      return tql;
    }

  oqmlTYPE t2 = qright->getType();

  if (oqmlIsPureComp(t1))
    xql = &qleft;
  else if (oqmlIsPureComp(t2))
    xql = &qright;
  else
    return ql;

  if (((oqmlComp *)(*xql))->appearsMoreOftenThan((oqmlComp *)ql))
    {
      oqmlNode *tql = *xql;
      *xql = ql;
      return tql;
    }

  return ql;
}

void
oqmlAnd::optimize(Database *db, oqmlContext *ctx)
{
  oqmlAtomType at_left;

  qleft->evalType(db, ctx, &at_left);

  eval_type.type = oqmlATOM_OID;
  eval_type.cls = at_left.cls;
}

oqmlStatus *
oqmlAnd::check(Database *db, oqmlContext *ctx)
{
  oqmlAtomType at_left;
  oqmlAtomType at_right;

  qleft->evalType(db, ctx, &at_left);
  qright->evalType(db, ctx, &at_right);

  if (!xc) {
    if (at_left.type != oqmlATOM_BOOL || at_right.type != oqmlATOM_BOOL)
      return new oqmlStatus("and operator cannot deal with non boolean values");
    eval_type.type = oqmlATOM_BOOL;
    return oqmlSuccess;
  }

  if (at_left.type != oqmlATOM_OID)
    return new oqmlStatus("and operator: left identifier is not a class");

  if (at_right.type != oqmlATOM_OID)
    return new oqmlStatus("and operator: right identifier is not a class");
  
  if (!at_left.cls || !at_right.cls)
    return oqmlSuccess;

  Status status;
  Bool issuperclassof;
  
  status = at_left.cls->isSuperClassOf(at_right.cls, &issuperclassof);
  
  if (status)
    return new oqmlStatus(status);
      
  if (issuperclassof) {
    if (qleft->getType() == oqmlIDENT) {
      // swap
      oqmlNode *ql = qleft;
      qleft = qright;
      qright = ql;
    }

    optimize(db, ctx);
    return oqmlSuccess;
  }
  
  status = at_right.cls->isSuperClassOf(at_left.cls, &issuperclassof);
  
  if (status)
    return new oqmlStatus(status);
  
  if (issuperclassof)
    {
      optimize(db, ctx);
      return oqmlSuccess;
    }      
  
  xc = 4;
  eval_type.type = oqmlATOM_OID;
  eval_type.cls = db->getSchema()->getClass("object");

  return oqmlSuccess;
}

oqmlStatus *oqmlAnd::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  oqmlTYPE t1, t2;

  s = qleft->compile(db, ctx);
  if (s)
    return s;

  s = qright->compile(db, ctx);
  if (s)
    return s;

  t1 = qleft->getType();
  t2 = qright->getType();

  if (t1 == oqmlIDENT && t2 == oqmlIDENT)
    xc = 1;
  else if (oqmlIsComp(t1) && t2 == oqmlIDENT)
    xc = 2;
  else if (t1 == oqmlIDENT && oqmlIsComp(t2))
    {
      // swap
      oqmlNode *ql = qleft;
      qleft = qright;
      qright = ql;
      xc = 2;
    }
  else if (oqmlIsComp(t1) && oqmlIsComp(t2))
    xc = 3;
  else
    xc = 0;

  return check(db, ctx);
}

oqmlStatus *
oqmlAnd::eval_0(Database *db, oqmlContext *ctx, oqmlAtomList **alist)
{
  assert(0);
  oqmlStatus *s;
  oqmlAtomList *al_left, *al_right;

  s = qleft->eval(db, ctx, &al_left);
  if (s)
    return s;

  if (al_left->cnt != 1)
    return new oqmlStatus("and operator cannot deal with non atomic values");

  if (al_left->first->type.type != oqmlATOM_BOOL)
    return new oqmlStatus("and operator cannot deal with non boolean values");

  if (!((oqmlAtom_bool *)al_left->first)->b)
    {
      (*alist)->append(new oqmlAtom_bool(oqml_False));
      return oqmlSuccess;
    }

  s = qright->eval(db, ctx, &al_right);
  if (s)
    return s;

  if (al_right->cnt != 1)
    return new oqmlStatus("and operator cannot deal with non atomic values");

  if (al_right->first->type.type != oqmlATOM_BOOL)
    return new oqmlStatus("and operator cannot deal with non boolean values");

  (*alist)->append(new oqmlAtom_bool(((oqmlAtom_bool *)al_right->first)->b));

  return oqmlSuccess;
}

oqmlStatus *
oqmlAnd::eval_1(Database *db, oqmlContext *ctx, oqmlAtomList **alist)
{
  oqmlStatus *s;
  oqmlAtomList *and_ctx = ctx->getAndContext();
  unsigned int r_left, r_right;

  s = qleft->estimate(db, ctx, r_left);
  if (s) return s;

  if (r_left > 0)
    {
      s = qright->estimate(db, ctx, r_right);
      if (s) return s;
    }
  else
    r_right = oqml_ESTIM_MAX;

  oqmlAtomList *al_left, *al_right;
  if (r_left <= r_right)
    {
      s = qleft->eval(db, ctx, &al_left);
      if (s) return s;

      ctx->setAndContext(al_left);

      s = qright->eval(db, ctx, alist);
      if (s) return s;

      ctx->setAndContext(and_ctx);
      return oqmlSuccess;
    }

  s = qright->eval(db, ctx, &al_right);
  if (s) return s;

  ctx->setAndContext(al_right);

  s = qleft->eval(db, ctx, alist);
  if (s) return s;

  ctx->setAndContext(and_ctx);
  return oqmlSuccess;
}

oqmlStatus *oqmlAnd::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  *alist = new oqmlAtomList();

  if (xc == 0)
    return eval_0(db, ctx, alist);
  if (xc == 4)
    return oqmlSuccess;

  return eval_1(db, ctx, alist);
}

void oqmlAnd::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlAnd::isConstant() const
{
  return oqml_False;
}

std::string
oqmlAnd::toString(void) const
{
  return oqml_binop_string(qleft, qright, " and ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlOr methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlOr::oqmlOr(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlOR)
{
  qleft = _qleft;
  qright = _qright;
}

oqmlOr::~oqmlOr()
{
  // delete qleft;
  // delete qright;
}

oqmlStatus *oqmlOr::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  // must eval types before evaluation
  s = qleft->compile(db, ctx);
  if (s)
    return s;

  s = qright->compile(db, ctx);
  if (s)
    return s;

  return oqmlSuccess;
}

oqmlStatus *oqmlOr::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al_left, *al_right;

  oqmlAtomList *rlist = new oqmlAtomList();
  *alist = new oqmlAtomList(new oqmlAtom_list(rlist));

  s = qleft->eval(db, ctx, &al_left);
  if (s)
    return s;

  if (al_left->cnt == 1 && OQML_IS_COLL(al_left->first))
    rlist->append(OQML_ATOM_COLLVAL(al_left->first));

  s = qright->eval(db, ctx, &al_right);
  if (s)
    return s;

  if (al_right->cnt == 1 && OQML_IS_COLL(al_right->first))
    rlist->append(OQML_ATOM_COLLVAL(al_right->first));

  return oqmlSuccess;
}

void oqmlOr::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  oqmlAtomType at_left, at_right;
  oqmlStatus *s;

  qleft->evalType(db, ctx, &at_left);

  qright->evalType(db, ctx, &at_right);

  if (at_left.type == at_right.type)
    *at = at_left;
  else
    {
      at->type = oqmlATOM_UNKNOWN_TYPE;
      at->cls = 0;
      at->comp = oqml_False;
    }
}

oqmlBool oqmlOr::isConstant() const
{
  return oqml_False;
}

std::string
oqmlOr::toString(void) const
{
  return oqml_binop_string(qleft, qright, " or ", is_statement);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlDatabase methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlDatabase::oqmlDatabase(const char *_dbname, const char *_mode,
			   oqmlNode * _ql) : oqmlNode(oqmlDATABASE)
{
  dbname = strdup(_dbname);
  mode = strdup(_mode);
  ql = _ql;
  cdb = 0;
}

oqmlDatabase::~oqmlDatabase()
{
  free(dbname);
  free(mode);
}

oqmlStatus *oqmlDatabase::compile(Database *db, oqmlContext *ctx)
{
  Database::OpenFlag open_flag;

  if (!strcasecmp(mode, "r"))
    open_flag = Database::DBRead;
  else if (!strcasecmp(mode, "sr"))
    open_flag = Database::DBSRead;
  else if (!strcasecmp(mode, "rw"))
    open_flag = Database::DBRW;
  else
    return new oqmlStatus(this, "unknown opening mode: %s", mode);

  if (!strcmp(dbname, db->getName()) && db->getOpenFlag() == open_flag)
    cdb = db;
  else
    {
      if (db->isLocal())
	return new oqmlStatus("cannot use multi database feature"
			      "in local opening mode");

      Status status = Database::open(db->getConnection(), dbname,
					   db->getDBMDB(),
					   db->getUser(),
					   db->getPassword(),
					   open_flag, 0, &cdb);
      
      if (status) return new oqmlStatus(status);
    }

  oqmlStatus *s;

  if (cdb != db) cdb->transactionBegin();
  s = ql->compile(cdb, ctx);
  if (cdb != db) cdb->transactionCommit();

  return s;
}

oqmlStatus *oqmlDatabase::eval(Database *db, oqmlContext *ctx,
			       oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

  if (cdb != db)
    cdb->transactionBegin();

  s = ql->eval(cdb, ctx, alist);

  if (cdb != db)
    cdb->transactionCommit();

  if (s)
    return s;

  return oqmlSuccess;
}

void oqmlDatabase::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlDatabase::isConstant() const
{
  return oqml_False;
}

std::string
oqmlDatabase::toString(void) const
{
  return std::string("(database ") + dbname + ")";
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlSelect methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlSelect::oqmlSelect(oqmlNode *_location,
		       oqmlBool _distinct,
		       oqmlBool _one,
		       oqmlNode *_projection,
		       oqml_IdentList *_ident_from_list,
		       oqmlNode *_where,
		       oqmlNode *_group,
		       oqmlNode *_having,
		       oqml_SelectOrder *_order) : oqmlNode(oqmlSELECT)
{
  location = _location;
  distinct = _distinct;
  one = _one;
  projection = _projection;
  ident_from_list = _ident_from_list;
  where = _where;
  group = _group;
  having = _having;
  order = _order;
  list_order = 0;
  idents = 0;
  ident_cnt = 0;
  calledFromEval = oqml_False;
  databaseStatement = oqml_False;
  memset(&logged, 0, sizeof(logged));
}

oqmlSelect::~oqmlSelect()
{
  delete ident_from_list;
  delete [] list_order;
  delete [] idents;
}

oqmlStatus *oqmlSelect::check_order_coll(oqmlNode *actual_projection)
{
  oqml_List *projlist = ((oqmlColl *)actual_projection)->getList();
  if (!projlist)
    return new oqmlStatus(this, "order clause: '%s' not found in projection",
			  order->list->first->ql->toString().c_str());

  list_order = new int[order->list->cnt];
  oqml_Link *olink, *plink;
  int ocnt, pcnt;
  for (olink = order->list->first, ocnt = 0; olink; olink = olink->next, ocnt++)
    {
      oqmlBool ok = oqml_False;
      for (plink = projlist->first, pcnt = 0; plink; plink = plink->next, pcnt++)
	if (plink->ql->equals(olink->ql))
	  {
	    ok = oqml_True;
	    list_order[ocnt] = pcnt;
	    break;
	  }

      if (!ok)
	return new oqmlStatus(this, "order clause: '%s' not found "
			      "in projection",
			      olink->ql->toString().c_str());
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlSelect::check_order_simple()
{
  for (oqml_Link *link = order->list->first; link; link = link->next)
    if (!projection->equals((link->ql)))
      return new oqmlStatus(this, "order clause: %s not found in projection",
			    link->ql->toString().c_str());
  return oqmlSuccess;
}

oqmlStatus *oqmlSelect::check_order()
{
  if (!order)
    return oqmlSuccess;

  oqmlNode *actual_projection = projection;
  if (projection->getType() == oqmlCALL)
    actual_projection = ((oqmlCall *)projection)->getBuiltIn();

  if (actual_projection &&
      actual_projection->getType() == oqmlLISTCOLL ||
      actual_projection->getType() == oqmlBAGCOLL ||
      actual_projection->getType() == oqmlARRAYCOLL ||
      actual_projection->getType() == oqmlSETCOLL)
    return check_order_coll(actual_projection);

  return check_order_simple();
}

extern oqmlStatus *
oqml_check_sort_type(oqmlNode *, oqmlATOMTYPE);

oqmlStatus *
oqmlSelect::realize_order(oqmlAtomList **alist)
{
  if (!order || !*alist || !(*alist)->cnt || !OQML_IS_COLL((*alist)->first))
    return oqmlSuccess;

  oqmlAtomList *list = OQML_ATOM_COLLVAL((*alist)->first);

  if (!list->cnt) {
    *alist = new oqmlAtomList(new oqmlAtom_list(new oqmlAtomList())); // added the 25/05/01
    return oqmlSuccess;
  }

  oqmlStatus *s;
  if (!list_order)
    {
      oqmlAtomType &atom_type = list->first->type;
      s = oqml_check_sort_type(this, atom_type,
			       (std::string("cannot order using '") +
				order->list->first->ql->toString() + "'").c_str());
      if (s) return s;
      oqml_sort_simple(list, OQMLBOOL(!order->asc), atom_type.type, &list);
      *alist = new oqmlAtomList(new oqmlAtom_list(list));
      return oqmlSuccess;
    }

  // 2/2/00
  // warning: cette algo est faux dès qu'il y a plus d'un list order!
  oqml_Link *olink = order->list->first;
  for (int i = 0; i < order->list->cnt; i++, olink = olink->next)
    {
      oqmlAtomType &atom_type = OQML_ATOM_COLLVAL(list->first)->getAtom(list_order[i])->type;
      s = oqml_check_sort_type(this, atom_type,
			       (std::string("cannot order using '") +
				olink->ql->toString() + "'").c_str());
      if (s) return s;
      oqml_sort_list(list, OQMLBOOL(!order->asc), list_order[i],
		     atom_type.type, &list);
    }

  *alist = new oqmlAtomList(new oqmlAtom_list(list));
  return oqmlSuccess;
}

oqmlBool
oqmlSelect::makeIdents()
{
  if (!ident_from_list)
    return oqml_False;

  oqmlBool missingIdent = oqml_False;

  delete [] idents;
  idents = new oqml_IdentLink*[ident_from_list->cnt];

  oqml_IdentLink *l = ident_from_list->first;

  for (ident_cnt = 0; l; ident_cnt++)
    {
      idents[ident_cnt] = l;
      if (!l->ident)
	missingIdent = oqml_True;
      l = l->next;
    }

  return missingIdent;
}

oqmlStatus *
oqmlSelect::processFromListRequalification(Database *db, oqmlContext *ctx)
{
  for (int i = ident_cnt-1; i >= 0; i--)
    {
      if (!idents[i]->ident)
	continue;

      const char *left_ident = 0;
      if (idents[i]->ql->asIdent())
	left_ident = idents[i]->ql->asIdent()->getName();
      else if (idents[i]->ql->asDot())
	left_ident = idents[i]->ql->asDot()->getLeftIdent();

      if (left_ident)
	{
	  for (int j = i-1; j >= 0; j--)
	    if (idents[j]->ident && !strcmp(left_ident, idents[j]->ident))
	      {
		idents[i]->requalified = oqml_True;
		break;
	      }

	  if (idents[i]->requalified)
	    {
	      oqmlBool done = oqml_False;
	      oqmlStatus *s = where->requalify(db, ctx, idents[i]->ident,
					       idents[i]->ql, done);
	      if (s) return s;
	    }
	}
    }

  return oqmlSuccess;
}

#define ERRFROM "from clause '%s': "

std::string ident_gen()
{
  static int i;
  return std::string("oql$x") + str_convert(i++);
}

oqmlStatus *
oqmlSelect::processMissingIdentsRequalification(Database *db, oqmlContext *ctx)
{
  for (int i = ident_cnt-1; i >= 0; i--)
    {
      if (idents[i]->ident)
	{
	  if (idents[i]->ql->asIdent())
	    idents[i]->cls = db->getSchema()->getClass(idents[i]->ql->asIdent()->getName());
	  continue;
	}

      if (!idents[i]->ql->asIdent())
	return new oqmlStatus(this, ERRFROM "needs identifier",
			      idents[i]->ql->toString().c_str());

      const char *ident = idents[i]->ql->asIdent()->getName();

      for (int j = i-1; j >= 0; j--)
	if (idents[j]->ident && !strcmp(ident, idents[j]->ident))
	  return new oqmlStatus(this, ERRFROM "needs identifier",
				idents[i]->ql->toString().c_str());

      idents[i]->cls = db->getSchema()->getClass(ident);
      if (!idents[i]->cls)
	return new oqmlStatus(this, ERRFROM "not a class",
			      idents[i]->ql->toString().c_str());

      //      idents[i]->ident = strdup(ident_gen());
      idents[i]->ident = strdup(ident);
      unsigned int attr_cnt;
      const Attribute **attrs = idents[i]->cls->getAttributes(attr_cnt);
      oqmlStatus *s;
      if (where)
	{
	  s = where->requalify(db, ctx, attrs, attr_cnt, idents[i]->ident);
	  if (s) return s;
	}

      if (order)
	for (oqml_Link *link = order->list->first; link; link = link->next)
	  {
	    if (link->ql->asIdent())
	      s = requalify_node(db, ctx, link->ql, attrs, attr_cnt,
				 idents[i]->ident);
	    else
	      s = link->ql->requalify(db, ctx, attrs, attr_cnt,
				      idents[i]->ident);
	    if (s) return s;
	  }

      if (projection)
	{
	  if (projection->asIdent())
	    s = requalify_node(db, ctx, projection, attrs, attr_cnt, idents[i]->ident);
	  else
	    s = projection->requalify(db, ctx, attrs, attr_cnt, idents[i]->ident);
	  if (s) return s;
	}
    }

  return oqmlSuccess;
}

static oqmlBool
is_terminal(const Attribute *attr)
{
  if (attr->isNative())
    return oqml_False;

  if (attr->isString())
    return oqml_True;

  if (attr->getTypeModifier().pdims != 1)
    return oqml_False;
    
  if (attr->isIndirect() || attr->getClass()->asBasicClass() ||
      attr->getClass()->asEnumClass())
    return oqml_True;

  return oqml_False;
}

oqmlStatus *
oqmlSelect::processMissingProjRequalification(Database *db, oqmlContext *ctx)
{
  oqml_IdentList *struct_list = new oqml_IdentList();

  for (int j = 0; j < ident_cnt; j++)
    {
      if (!idents[j]->cls)
	{
	  delete struct_list;
	  return new oqmlStatus(this, ERRFROM "not a class",
				idents[j]->ql->toString().c_str());
	}

      unsigned int attr_cnt;
      const Attribute **attrs = idents[j]->cls->getAttributes(attr_cnt);
      for (int k = 0; k < attr_cnt; k++)
	if (is_terminal(attrs[k]))
	  struct_list->add(new oqmlIdent(attrs[k]->getName()),
			   new oqmlDot(new oqmlIdent(idents[j]->ident),
				       new oqmlIdent(attrs[k]->getName()),
				       oqml_False));
    }

  projection = new oqmlStruct(struct_list);

  if (isLocked()) projection->lock();

  return oqmlSuccess;
}

oqmlStatus *
oqmlSelect::processRequalification_1(Database *db, oqmlContext *ctx)
{
  oqmlBool missingIdent = makeIdents();

  if (missingIdent || !projection)
    {
      if (missingIdent && where && !where->mayBeRequalified())
	return new oqmlStatus(this, "this construct needs explicit "
			      "identifiers in the from clause");

      oqmlStatus *s = processMissingIdentsRequalification(db, ctx);
      if (s) return s;
    }

  if (!projection)
    {
      oqmlStatus *s = processMissingProjRequalification(db, ctx);
      if (s) return s;
    }

  return oqmlSuccess;
}

oqmlStatus *
oqmlSelect::processRequalification_2(Database *db, oqmlContext *ctx)
{
  if (where && where->mayBeRequalified())
    return processFromListRequalification(db, ctx);

  return oqmlSuccess;
}

oqmlStatus *
oqmlSelect::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  if (!calledFromEval) {
    memset(&logged, 0, sizeof(logged));
    s = SelectLog::init(this, ctx);
    if (s) return s;
    return oqmlSuccess;
  }

  if (!db->isInTransaction())
    return new oqmlStatus(this, "must be done within the scope of "
			  "a transaction in database '%s'", db->getName());

  s = processRequalification_1(db, ctx);
  if (s) return s;
  
  if (!ident_from_list) {
    if (projection) {
      ctx->incrSelectContext(this);
      s = projection->compile(db, ctx);
      ctx->decrSelectContext();
      if (s) return s;
    }
    s = check_order();
    return s;
  }
    
  oqml_IdentLink *l;
    
  l = ident_from_list->first;
    
  while(l) {
    if (!l->ident)
      return new oqmlStatus(this, "identificator is missing in the from "
			    "clause: '%s'",
			    l->ql->toString().c_str());
    l = l->next;
  }
    
  l = ident_from_list->first;
  oqmlAtomType unknownType;
    
  s = oqmlSuccess;

  if (!s && !where) {
    ctx->incrSelectContext(this);
    while(l) {
      if (!s)
	s = l->ql->compile(db, ctx);
      ctx->pushSymbol(l->ident, &unknownType, 0, oqml_False);
      l = l->next;
    }
    
    if (!s) {
      int select_ctx_cnt = ctx->setSelectContextCount(0);
      s = projection->compile(db, ctx);
      ctx->setSelectContextCount(select_ctx_cnt);
    }

    if (!s)
      s = check_order();

    l = ident_from_list->first;

    while (l)	{
      ctx->popSymbol(l->ident, oqml_False);
      l = l->next;
    }

    ctx->decrSelectContext();
    return s;
  }

  ctx->incrSelectContext(this);

  while (l) {
    if (!s) s = l->ql->compile(db, ctx);
    ctx->pushSymbol(l->ident, &selectAtomType, 0, oqml_False);
    l = l->next;
  }

  ctx->decrSelectContext();

  if (!s && where) {
    int select_ctx_cnt = ctx->setSelectContextCount(0);
    // added 28/08/02
    ctx->incrHiddenSelectContext(this);
    s = where->compile(db, ctx);
    ctx->decrHiddenSelectContext();
    ctx->setSelectContextCount(select_ctx_cnt);
  }

  if (!s) {
    int select_ctx_cnt = ctx->setSelectContextCount(0);
    s = projection->compile(db, ctx);
    ctx->setSelectContextCount(select_ctx_cnt);
  }

  if (!s)
    s = check_order();

  l = ident_from_list->first;

  while (l) {
    ctx->popSymbol(l->ident, oqml_False);
    l = l->next;
  }

  return s;
}

oqmlStatus *
optimize(Database *db, oqmlContext *ctx, oqmlAtom_select *atom,
	 int n, oqml_IdentLink *idents[], oqmlAtomList *&list)
{
  if (atom->list)
    {
      oqmlBool toEval = oqml_False;
      oqmlNode *node = atom->node;

      if (node)
	for (int i = 0; i < n; i++)
	  if (node->hasIdent(idents[i]->ident)) {
	    SELECT_TRACE(("optimize : node has ident %p\n", idents[i]->ident));
	    toEval = oqml_True;
	    break;
	  }

      if (!toEval) {
	list = atom->list;
	return oqmlSuccess;
      }
    }

  if (!atom->node)
    return oqmlSuccess;

  oqmlStatus *s = atom->node->eval(db, ctx, &list);
  if (s) return s;
  atom->list = list;
  return oqmlSuccess;
}

oqmlBool
checkIdent(oqml_IdentLink *idents[], int n, int cnt)
{
  const char *ident = idents[n]->ident;

  for (int i = n+1; i < cnt; i++)
    if (idents[i]->ql->hasIdent(ident))
      return oqml_True;

  return oqml_False;
}

void stop_imm() { }

static oqmlAtomList *
oqml_make_intersect(oqmlAtomList *list1, oqmlAtom *a2)
{
  oqmlAtomList *list = new oqmlAtomList();

  if (!a2 || !list1)
    return list;

  if (OQML_IS_COLL(a2))
    {
      oqmlAtomList *list2 = OQML_ATOM_COLLVAL(a2);
      oqmlAtom *a = list1->first;

      while (a)
	{
	  oqmlAtom *next = a->next;
	  if (list2->isIn(a))
	    list->append(a->copy());
	  a = next;
	}
    }
  else if (list1->isIn(a2))
    list->append(a2);

  /*
  printf("make_intersect between %s and %s is %s\n",
	 list1->getString(), a2->getString(), list->getString());
	 */

  return list;
}

oqmlAtom **
begin_cpatoms(oqmlAtomList **cplists, int cpcnt)
{
  if (!cpcnt)
    return 0;

  oqmlAtom **ratoms = new oqmlAtom *[cpcnt];
  for (int i = 0; i < cpcnt; i++) {
    oqmlAtomList *list = cplists[i];
    if (list && list->first && list->first->as_coll())
      list = OQML_ATOM_COLLVAL(list->first);
    ratoms[i] = (list ? list->first : 0);
  }

  return ratoms;
}

oqmlAtom **
next_cpatoms(oqmlAtom **cpatoms, int cpcnt)
{
  if (!cpcnt)
    return 0;

  oqmlAtom **ratoms = new oqmlAtom *[cpcnt];
  for (int i = 0; i < cpcnt; i++)
    ratoms[i] = cpatoms[i]->next;

  return ratoms;
}

#ifdef TRACE
#define CP1_TRACE() \
  SELECT_TRACE(("EvalCartProdRealize(cpcnt %d, reducing #%d)\n", \
   	        cpcnt, ident_cnt - n - 1)); \
  for (int i = 0; i < cpcnt; i++) \
    SELECT_TRACE(("EvalCartProdRealize(cpatoms[%d]: %s)\n", \
                 i, (const char *)cpatoms[i]->getString()))

#define CP2_TRACE() \
  if (cpcnt_n) { \
    SELECT_TRACE(("\tcp count: %d\n", cpcnt_n)); \
    for (int i = 0; i < cpcnt_n; i++) \
      SELECT_TRACE(("\tcplist[%d]: %s\n", i, (const char *)cplists_n[i]->getString())); \
  }
#else
#define CP1_TRACE()
#define CP2_TRACE()
#endif

oqmlStatus *
oqmlSelect::evalCartProdRealize(Database *db, oqmlContext *ctx,
				oqmlAtomList *rlist, int n,
				const char *preval_ident,
				oqmlAtom **cpatoms, int cpcnt)
{
  oqmlStatus *s = oqmlSuccess;
  oqmlAtomList *list = 0;

  oqmlAtomType at;
  oqmlAtom *atom = 0;
  const char *ident = idents[n]->ident;
  oqmlBool where_optim = oqml_False;
  int cpcnt_n = 0;
  oqmlAtomList **cplists_n = 0;

  SELECT_TRACE(("\nEvalCartProdRealize(n=%d, ident_cnt=%d) `%s'\n",
		n, ident_cnt, (const char *)toString()));

  if (cpcnt) {
    CP1_TRACE();
    list = new oqmlAtomList(cpatoms[ident_cnt - n - 1]);
    where_optim = oqml_True;
  }
  else if (idents[n]->skipIdent || idents[n]->requalified) {
    if (n != ident_cnt - 1) {
      SELECT_TRACE(("\tBefore Requalified EvalCartProdRealize `%s'\n",
		    (const char *)toString()));
      oqmlStatus *s = evalCartProdRealize(db, ctx, rlist, n+1, preval_ident);
      SELECT_TRACE(("\tAfter Requalified EvalCartProdRealize `%s'\n",
		    (const char *)toString()));
      return s;
    }
    ctx->incrWhereContext();
    if (idents[n]->skipIdent)
      {
	ident = "oql$dummy";
	list = new oqmlAtomList(new oqmlAtom_nil());
      }
    else if (ctx->getSymbol(ident, &at, &atom) && atom->as_select() &&
	     atom->as_select()->collatom)
      {
	oqmlAtomList *al;
	s = idents[n]->ql->eval(db, ctx, &al);
	if (s) return s;
	list = oqml_make_intersect(atom->as_select()->collatom->list,
				   al ? al->first : 0);
	SELECT_TRACE(("\tMaking list from make_intersect `%s'\n",
		      (const char *)toString()));
      }
    else
      {
	SELECT_TRACE(("\tMaking list from idents[n]->ql->eval `%s' %s\n",
		      (const char *)toString(),
		      (const char*)idents[n]->ql->toString()));
	s = idents[n]->ql->eval(db, ctx, &list);
	if (s) return s;
      }
  }
  else {
    oqmlBool wasPopulated;

    if (ctx->getSymbol(ident, &at, &atom) && at.type == oqmlATOM_SELECT)
      wasPopulated = OQMLBOOL(atom->as_select()->list);
    else
      assert(0);
      
    ctx->incrWhereContext();
    if (n != ident_cnt - 1 && !wasPopulated &&
	!checkIdent(idents, n, ident_cnt)) {
      ctx->incrSelectContext(this);
      ctx->incrPrevalContext();
      SELECT_TRACE(("\tBefore Prevaling EvalCartProdRealize `%s'\n",
		    (const char *)toString()));
      s = evalCartProdRealize(db, ctx, rlist, n+1, ident);
      SELECT_TRACE(("\tAfter Prevaling EvalCartProdRealize `%s'\n",
		    (const char *)toString()));
      ctx->getSymbol(ident, &at, &atom);
      ctx->decrSelectContext();
      ctx->decrPrevalContext();
      if (s) return s;
    }
      
    where_optim = atom->as_select()->list ? oqml_True : oqml_False;
    cpcnt_n = atom->as_select()->cpcnt;
    cplists_n = atom->as_select()->cplists;
    CP2_TRACE();

    s = optimize(db, ctx, atom->as_select(), n, idents, list);
    SELECT_TRACE(("\tMaking list from optimize `%s' [where_optim %s]\n",
		  (const char *)toString(),
		  where_optim ? "true" : "false"));

    if (s) return s;
  }

  if (list && list->first && list->first->as_coll()) {
#ifdef SYNC_GARB
    //oqmlAtomList *olist = list; 
#endif
    list = OQML_ATOM_COLLVAL(list->first);
#ifdef SYNC_GARB
    //olist->first = 0;
    //delete olist; // bad garbage !
#endif
  }

  oqmlAtom *a = (list ? list->first : 0);
  oqmlAtom **cpatoms_n;

  if (cpcnt) {
    cpatoms_n = cpatoms;
    cpcnt_n = cpcnt;
  }
  else
    cpatoms_n = begin_cpatoms(cplists_n, cpcnt_n);

  oqmlBool cpatom_while = (!cpatoms && cpatoms_n) ? oqml_True : oqml_False;

  if (!a && n != ident_cnt - 1)
    a = new oqmlAtom_nil();

  SELECT_TRACE(("\tLoop start for %d items `%s'\n", list ? list->cnt : 0,
		(const char *)toString()));

  if (SelectLog::oql_select_log_ctl != SelectLog::Off) {
    if (!logged[n] || SelectLog::oql_select_log_ctl == SelectLog::Detail) {
      SelectLog::append((std::string("QUERY : ") + toString() + "\nDEPTH : #" +
			 str_convert((long)n) + "\n").c_str());
      logged[n] = 1;
    }
    if (SelectLog::oql_select_log_ctl == SelectLog::Detail) {
      SelectLog::append((std::string("ITEM COUNT : ") +
			 str_convert((long)(list ? list->cnt : 0)) + "\n").c_str());
    }
  }

#if 0
  printf("LOOP from 0 to %d\n", (list ? list->cnt : 0));
  int nn = 0;

  oqmlAtom *xxx = a;
  while (xxx) {
    oqmlAtom *next = xxx->next;
    printf("previous #%d -> %s %s\n", nn, a->getString(), (next ? next->getString() : "<NIL>"));
    nn++;
    xxx = next;
  }

  nn = 0;
#endif

  while (a) {
    oqmlAtom *next = a->next;
    /*
    printf("#%d -> %s %s\n", nn, a->getString(), (next ? next->getString() : "<NIL>"));
    nn++;
    */
    oqmlAtom **cpatom_next = 0;
    if (cpatom_while)
      cpatom_next = next_cpatoms(cpatoms_n, cpcnt_n);
    OQML_CHECK_INTR();
    if (ident) {
      ctx->pushSymbol(ident, &a->type, a, oqml_False);
	  
      SELECT_TRACE(("\tPushing Symbol %s -> %s\n", ident, a->getString()));

      if (n != ident_cnt - 1) {
	SELECT_TRACE(("\tBefore Calling EvalCartProdRealize "
		      "recursively(%s)\n", (const char *)toString()));
	s = evalCartProdRealize(db, ctx, rlist, n+1, preval_ident,
				cpatoms_n, cpcnt_n);
	SELECT_TRACE(("\tAfter Calling EvalCartProdRealize "
		      "recursively(%s)\n", (const char *)toString()));
      }
      else {
	oqmlAtomList *xlist = 0;
	if (where && ctx->isPrevalContext())
	  {
	    oqmlBool done;
	    unsigned int cnt;
	    SELECT_TRACE(("\tBefore PreEvaluatingSelect Where(%s)\n",
			  (const char *)where->toString()));
	    ctx->pushCPAtom(this, a);
	    s = where->preEvalSelect(db, ctx, preval_ident, done, cnt,
				     oqml_False);
	    ctx->popCPAtom(this);
	    SELECT_TRACE(("\tAfter PreEvaluatingSelect Where(%s)\n",
			  (const char *)where->toString()));
	  }
	else if (where && !(where_optim && where->asComp())) {
	  // added the 4/07/01
	  int select_ctx_cnt = ctx->setSelectContextCount(0);
	  SELECT_TRACE(("\tBefore Evaluating Where(%s)\n",
			(const char *)where->toString()));
	  // added 28/08/02
	  ctx->incrHiddenSelectContext(this);
	  s = where->eval(db, ctx, &xlist);
	  ctx->decrHiddenSelectContext();
	  SELECT_TRACE(("\tAfter Evaluating Where(%s)\n",
			(const char *)where->toString()));

	  if (xlist && !xlist->first)
	    SELECT_TRACE(("\tWhere(%s) returning false\n",
			  (const char *)where->toString()));
	  if (!s && xlist->first) {
	    if (!OQML_IS_BOOL(xlist->first)) {
	      s = new oqmlStatus(this,
				 "where condition: "
				 "boolean expected, got %s",
				 xlist->first->type.getString());
	    }
	    else if (OQML_ATOM_BOOLVAL(xlist->first)) {
	      SELECT_TRACE(("\tWhere(%s) returning true\n",
			    (const char *)where->toString()));
	      oqmlAtomList *ylist;
	      s = projection->eval(db, ctx, &ylist);
	      if (!s && ylist->first)
		{
		  if (!distinct || !rlist->isIn(ylist->first)) {
		    SELECT_TRACE(("\tWhere(%s) appending %s\n",
				  (const char *)where->toString(),
				  (const char *)ylist->first->getString()));
		    rlist->append(ylist->first);
#ifdef SYNC_GARB
		    if (ylist && !ylist->refcnt) {
		      ylist->first = 0;
		      delete ylist;
		    }
#endif
		  }
		}
	    }
	    else
	      SELECT_TRACE(("\tWhere(%s) returning false\n",
			    (const char *)where->toString()));
	  }

	  // added the 4/07/01
	  ctx->setSelectContextCount(select_ctx_cnt);
#ifdef SYNC_GARB
	  OQL_DELETE(xlist);
#endif
	}
	else {
	  // added the 4/07/01
	  oqmlAtomList *ylist;
	  int select_ctx_cnt = ctx->setSelectContextCount(0);
	  s = projection->eval(db, ctx, &ylist);
	  if (!s && ylist->first) {
	    SELECT_TRACE(("\t%sappending %s\n",
			  "Unconditionally ",
			  (const char *)ylist->first->getString()));
	    rlist->append(ylist->first);
#ifdef SYNC_GARB
	    if (ylist && !ylist->refcnt) {
	      ylist->first = 0;
	      delete ylist;
	    }
#endif
	  }
	  // added the 4/07/01
	  ctx->setSelectContextCount(select_ctx_cnt);
	}
      }

      ctx->popSymbol(ident, oqml_False);

      if (s)
	break;
    }

    if (cpatom_while)
      delete [] cpatoms_n;

    cpatoms_n = cpatom_next;
    a = next;
  }

  if (cpatom_while)
    delete [] cpatoms_n;

  SELECT_TRACE(("\tLoop end `%s'\n", (const char *)toString()));

  ctx->decrWhereContext();
#ifdef SYNC_GARB
  //delete list; // really ?
#endif
  return s;
}

static oqmlContext *context;

int identlink_cmp(const void *xi1, const void *xi2)
{
  const oqml_IdentLink *i1 = *(const oqml_IdentLink **)xi1;
  const oqml_IdentLink *i2 = *(const oqml_IdentLink **)xi2;
  oqmlAtom *a1, *a2;

  if (context->getSymbol(i1->ident, 0, &a1) && a1 && a1->as_select() &&
      context->getSymbol(i2->ident, 0, &a2) && a2 && a2->as_select())
    {
      if (a1->as_select()->isPopulated())
	{
	  if (a2->as_select()->isPopulated())
	    return 0;
	  return -1;
	}
      else if (a2->as_select()->isPopulated())
	return 1;

      if (a1->as_select()->indexed)
	{
	  if (a2->as_select()->indexed)
	    return 0;
	  return -1;
	}
      else if (a2->as_select()->indexed)
	return 1;
    }

  return 0;
}

static void
dump_idents(const char *msg, oqml_IdentLink *idents[], int n)
{
  printf("%s: ", msg);
  for (int i = 0; i < n; i++)
    printf("ident[%d] = %s\n", i, idents[i]->ident);
}

oqmlBool
oqmlSelect::usesFromIdent(oqmlNode *node)
{
  if (!ident_from_list)
    return oqml_False;

  oqml_IdentLink *l = ident_from_list->first;

  while (l)
    {
      if (node->hasIdent(l->ident))
	return oqml_True;
      l = l->next;
    }

  return oqml_False;
}

oqmlStatus *
oqmlSelect::evalCartProd(Database *db, oqmlContext *ctx,
			 oqmlAtomList **alist)
{
  oqmlStatus *s = oqmlSuccess;

  if (ident_cnt > 1)
    {
      context = ctx;
      //dump_idents("before", idents, ident_cnt);
      qsort(idents, ident_cnt, sizeof(oqml_IdentLink *), identlink_cmp);
      //dump_idents("after", idents, ident_cnt);
    }

#define OPT_DEAD_IDENTS

#ifdef OPT_DEAD_IDENTS
  // Eliminate ``dead idents''
  //
  // WARNING: this test is not reliable as hasIdent() is not correctly
  // implemented for all classes!!!
  //

  // MUST BE IMPROVED: should check that idents[i] is not used in
  // any constructs of the ident_from_list except this.
  //
  for (int i = 0; i < ident_cnt; i++)
    if (!projection->hasIdent(idents[i]->ident) &&
	(!where || !where->hasIdent(idents[i]->ident)) &&
	!checkIdent(idents, i, ident_cnt))
      idents[i]->skipIdent = oqml_True;
#endif

  oqmlAtomList *rlist = new oqmlAtomList();

  SELECT_TRACE(("\nEvalCartProd Start `%s'\n",
	 (const char *)toString()));

  if (!s)
    s = evalCartProdRealize(db, ctx, rlist, 0);

  SELECT_TRACE(("EvalCartProd End `%s'\n",
	 (const char *)toString()));

  if (s) return s;

  if (one)
    *alist = new oqmlAtomList(rlist->first);
  else
    {
      oqmlAtom_coll *list;
      if (distinct)
	list = new oqmlAtom_set(rlist, oqml_False);
      else
	list = new oqmlAtom_bag(rlist);
      
      //*alist = new oqmlAtomList(distinct ? list->suppressDoubles() : list);
      *alist = new oqmlAtomList(list);
    }

  //return realize_order(alist);
  return oqmlSuccess;
}

//#define NEW_ONEATOM

#ifdef NEW_ONEATOM
static oqmlBool
xalist_make(oqmlBool one, oqmlAtomList *xalist, oqmlAtomList *alist)
#else
static oqmlBool
xalist_make(oqmlContext *ctx, oqmlAtomList *xalist, oqmlAtomList *alist)
#endif
{
  if (!alist)
    return oqml_True;

  oqmlAtom *xa = xalist->first;
  oqmlAtom *a = alist->first;

  if (xa && OQML_IS_COLL(xa)) {
    if (OQML_IS_COLL(a))
      OQML_ATOM_COLLVAL(xa)->append(OQML_ATOM_COLLVAL(a));
    else
      OQML_ATOM_COLLVAL(xa)->append(a);
  }
  else if (a)
    xalist->append(a);

#ifdef NEW_ONEATOM
  if (xalist->cnt && one)
    return oqml_False;
#else
  if (xalist->cnt && ctx->isOneAtom())
    return oqml_False;
#endif

  return oqml_True;
}

oqmlStatus *oqmlSelect::eval(Database *db, oqmlContext *ctx, oqmlAtomList **xalist, oqmlComp *, oqmlAtom *)
{
  if (one && order)
    return new oqmlStatus(this,
			  "cannot use an order by clause within a select one");

  oqmlStatus *s;

  s = oqml_get_locations(db, ctx, location, dbs, dbs_cnt);
  if (s) return s;

  int where_ctx = ctx->setWhereContext(0);
  int preval_ctx = ctx->setPrevalContext(0);

  oqmlBool oldone = ctx->isOneAtom();
#ifdef NEW_ONEATOM
  if (oldone) {
    printf("warning: a `select' expression should not be imbricated in a `select one' expression");
  }
#else
  if (oldone)
    return new oqmlStatus(this, "a `select' expression cannot be imbricated in a `select one' expression");

  if (one && ident_from_list && ident_from_list->cnt > 1)
    return new oqmlStatus(this, "a join cannot be performed in a `select one' expression");
#endif

  if (one) ctx->setOneAtom();

  SELECT_TRACE(("Eval(%s)\n", (const char *)toString()));
  *xalist = new oqmlAtomList(); // leaks

  for (int i = 0; i < dbs_cnt; i++) {
    oqmlAtomList *alist = 0;
    db = dbs[i];
    calledFromEval = oqml_True;
    s = compile(db, ctx);
    calledFromEval = oqml_False;
    if (s) return s;
    //db = newdb;

    if (!ident_from_list)
      {
	ctx->incrSelectContext(this);
	s = projection->eval(db, ctx, &alist);
	ctx->decrSelectContext();

	if (s) return s;

	if (distinct && alist->cnt && OQML_IS_COLL(alist->first))
	  {
	    oqmlAtom_set *list = new oqmlAtom_set
	      (OQML_ATOM_COLLVAL(alist->first));
	    //alist = new oqmlAtomList(list->suppressDoubles());
	  }
	else if (one)
	  {
	    if (alist->cnt && OQML_IS_COLL(alist->first))
	      alist = new oqmlAtomList(OQML_ATOM_COLLVAL(alist->first)->first);
	    else
	      alist = new oqmlAtomList(alist->first);
	  }

	if (s) return s;

#ifdef NEW_ONEATOM
	if (!xalist_make(one, *xalist, alist))
	  break;
#else
	if (!xalist_make(ctx, *xalist, alist))
	  break;
#endif
	continue;
      }

    s = oqmlSuccess;
    char *popIdent = 0;

    oqml_IdentLink *l = ident_from_list->first;
    while (l)
      {
	ctx->pushSymbol(l->ident, &selectAtomType,
			new oqmlAtom_select(l->ql, l->ql), oqml_False); // leaks
	l = l->next;
      }

    s = processRequalification_2(db, ctx);

  /*
  IDB_LOG(IDB_LOG_OQL_EXEC,
	  ("select requalified to '%s'\n", (const char *)toString()));
	  */

  //printf("select is now '%s'\n", (const char *)toString());

    if (!s && where)
      {
	oqml_IdentLink *l = ident_from_list->first;

	while (l)
	  {
	    if (l->ql->getType() == oqmlIDENT)
	      {
		oqmlBool done;
		ctx->incrSelectContext(this);
		ctx->incrPrevalContext();
		unsigned int cnt;
		SELECT_TRACE(("Before Where PreEvalSelect `%s' %s\n",
			      (const char *)toString(),
			      (const char *)where->toString()));
		s = where->preEvalSelect(db, ctx, l->ident, done, cnt);
		SELECT_TRACE(("After Where PreEvalSelect `%s' %s\n",
			      (const char *)toString(),
			      (const char *)where->toString()));
		ctx->decrPrevalContext();
		popIdent = l->ident;
		ctx->decrSelectContext();
		if (s)
		  break;
		oqmlAtom *atom_select = 0;
		ctx->getSymbol(l->ident, 0, &atom_select);
		if (atom_select->as_select()->list)
		  break;
	      }
	    l = l->next;
	  }
      }

    if (!s) {
      s = evalCartProd(db, ctx, &alist);
      //if (!s) s = realize_order(&alist);
    }

    l = ident_from_list->first;
    while (l)
      {
	ctx->popSymbol(l->ident, oqml_False);
	l = l->next;
      }

    if (s) return s;
#ifdef NEW_ONEATOM
    if (!xalist_make(one, *xalist, alist))
      break;
#else
    if (!xalist_make(ctx, *xalist, alist))
      break;
#endif
  }

#ifdef NEW_ONEATOM
  ctx->setOneAtom(oqml_False);
#else
  ctx->setOneAtom(oldone);
#endif
  if (distinct) {
    oqmlAtom *xa = (*xalist)->first;
    if (xa && OQML_IS_COLL(xa))
      *xalist = new oqmlAtomList(new oqmlAtom_set(OQML_ATOM_COLLVAL(xa)));
  }
    
  s = realize_order(xalist);
  (void)ctx->setWhereContext(where_ctx);
  (void)ctx->setPrevalContext(preval_ctx);
  return s;
}

void oqmlSelect::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlSelect::isConstant() const
{
  return oqml_False;
}

std::string
oqmlSelect::toString(void) const
{
  std::string s = (is_statement ? std::string("") : std::string("(")) +
    (databaseStatement ? "context" : "select") + 
    (location ? std::string("<") + location->toString() + "> " :
     std::string(" ")) + (one ? "one " : "") + (distinct ? "distinct " : "") +
    (projection ? projection->toString() : std::string("*"));
  
  if (ident_from_list)
    {
      s += " from ";
      oqml_IdentLink *l = ident_from_list->first;
      for (int n = 0, c = 0; l; n++, l = l->next)
	//if (!l->requalified)
	  {
	    if (c) s += ",";
	    if (l->ql)
	      s += l->ql->toString();
	    if (l->ident)
	      s += std::string(" as ") + l->ident;
	    c++;
	  }
    }

  if (where)
    s += std::string(" where ") + where->toString();

  if (group)
    s += std::string(" group by ") + group->toString();

  if (having)
    s += std::string(" having ") + having->toString();

  if (order)
    {
      s += std::string(" order by ");
      s += order->list->toString();
      s += order->asc ? " asc" : " desc";
    }

  if (is_statement)
    return s + "; ";

  return s + ")";
}

void
oqmlSelect::lock()
{
  oqmlNode::lock();
  projection->lock();
  if (location) location->lock();
  if (where) where->lock();
  if (group) group->lock();
  if (having) having->lock();
  if (order) order->list->lock();
  if (!ident_from_list)
    return;

  oqml_IdentLink *l = ident_from_list->first;
  while (l)
    {
      l->ql->lock();
      l = l->next;
    }
}

void
oqmlSelect::unlock()
{
  oqmlNode::unlock();
  projection->unlock();
  if (location) location->unlock();
  if (where) where->unlock();
  if (group) group->unlock();
  if (having) having->unlock();
  if (order) order->list->unlock();
  if (!ident_from_list)
    return;

  oqml_IdentLink *l = ident_from_list->first;
  while (l)
    {
      l->ql->unlock();
      l = l->next;
    }
}

void
oqml_capstring(char *str)
{
  char c;
  while (c = *str)
    {
      if (c >= 'a' && c <= 'z')
	*str = c + 'A' - 'a';
      str++;
    }
}
}
