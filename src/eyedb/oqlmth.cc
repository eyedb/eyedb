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


#define protected public

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "oql_p.h"

//#define MTH_TRACE

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlMethodCall methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// -----------------------------------------------------------------------
//
// public instance method constructor
//
// -----------------------------------------------------------------------

namespace eyedb {

oqmlMethodCall::oqmlMethodCall(const char *_mthname, oqml_List *_list,
			       oqmlBool _noParenthesis) :
  oqmlNode(oqmlMTHCALL)
{
  deleteList = oqml_False;
  init(0, _mthname, _list);
  noParenthesis = _noParenthesis;
  call = 0;
}

// -----------------------------------------------------------------------
//
// public class method constructor
//
// -----------------------------------------------------------------------

oqmlMethodCall::oqmlMethodCall(const char *_clsname, const char *_mthname,
			       oqml_List *_list, oqmlNode *_call) :
  oqmlNode(oqmlMTHCALL)
{
  deleteList = oqml_True;
  init(_clsname, _mthname, _list);
  noParenthesis = oqml_False;
  call = _call;
}

// -----------------------------------------------------------------------
//
// public isConstant() method -> returns false
//
// -----------------------------------------------------------------------

oqmlBool oqmlMethodCall::isConstant() const
{
  return oqml_False;
}

// -----------------------------------------------------------------------
//
// public compile method
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  if (clsname)
    cls = db->getSchema()->getClass(clsname);

  last.cls = 0;
  last.xmth = 0;

#ifdef MTH_TRACE
  printf("compiling methodCall(%s, %d arguments)\n", mthname, list->cnt);
#endif

  oqml_Link *l = list->first;

  while (l)
    {
      if (l->ql->getType() != oqmlIDENT)
	{
	  s = l->ql->compile(db, ctx);
	  if (s)
	    return s;
	}

      l = l->next;
    }

  return oqmlSuccess;
}

#define TRS_PROLOGUE(X) \
  Bool isInTrs = True; \
  if (X) \
    { \
      if (!db) db = (Database *)(X)->getDatabase(); \
      if (!db->isInTransaction()) \
	{ \
	  Status xs = db->transactionBegin(); \
	  if (xs) return new oqmlStatus(this, xs); \
	  isInTrs = False; \
	} \
    }

#define TRS_EPILOGUE(S) \
  if (!isInTrs) \
    { \
      if (S) {db->transactionAbort(); return (S);}\
      Status _s = db->transactionCommit(); \
      if (_s) return new oqmlStatus(this, _s); \
    }

// -----------------------------------------------------------------------
//
// public eval method
//
// Algorithm:
//  - eval the list and keep results in tmp_atom array
//  - if the method has been resolved at compile time, check arguments
//  - if the method has not been resolved at compile time, find method
//    according to argument type
//  - atomsToArgs
//  - apply method
//  - argsToAtom
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::eval(Database *db, oqmlContext *ctx,
				 oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;

#ifdef MTH_TRACE
  printf("evaluate static method %s::%s\n", clsname, mthname);
#endif

  *alist = new oqmlAtomList();

  s = evalList(db, ctx);
  if (s)
    return s;

  Method *xmth = 0;

  TRS_PROLOGUE(cls);

  if (mth)
    {
      s = checkArguments(db, ctx, mth);
      if (s) return s;
      xmth = mth;
    }
  else
    {
      s = resolveMethod(db, ctx, True, (Object *)0, xmth);
      if (s) return s;
    }

  TRS_EPILOGUE(s);

  if (xmth->getEx()->getLang() & C_LANG)
    s = applyC(db, ctx, xmth, alist);
  else 
    s = applyOQL(db, ctx, xmth, alist);

  return s;
}

// -----------------------------------------------------------------------
//
// public evalType() method
//
// -----------------------------------------------------------------------

void oqmlMethodCall::evalType(Database *db, oqmlContext *ctx,
			      oqmlAtomType *at)
{
  *at = eval_type;

  if (!mth || eval_type.type != oqmlATOM_UNKNOWN_TYPE)
    return;

  ArgType_Type odl_type = mth->getEx()->getSign()->getRettype()->getType();

  if (odl_type == OID_TYPE)
    at->type = oqmlATOM_OID;
  else if (odl_type == OBJ_TYPE)
    at->type = oqmlATOM_OBJ;
  else if (odl_type == INT16_TYPE ||
	   odl_type == INT32_TYPE ||
	   odl_type == INT64_TYPE)
    at->type = oqmlATOM_INT;
  else if ( odl_type == STRING_TYPE)
    at->type = oqmlATOM_STRING;
  else if (odl_type == CHAR_TYPE)
    at->type = oqmlATOM_CHAR;
  else if (odl_type == FLOAT_TYPE)
    at->type = oqmlATOM_DOUBLE;

  eval_type = *at;
}

// -----------------------------------------------------------------------
//
// public perform method
//
// -----------------------------------------------------------------------

// 19/05/05: disconnected
//#define STATIC_POLYMORPH

oqmlStatus *
oqmlMethodCall::perform(Database *db, oqmlContext *ctx,
			Object *o, const Oid &oid, const Class *_cls,
			oqmlAtomList **alist)
{
  if (o->isRemoved())
    return new oqmlStatus(this, "object %s is removed", o->getOid().toString());

#ifdef MTH_TRACE
  printf("trying to perform method call on %s class=[%p, %s] mth=%s, "
	 "%d count\n", oid.toString(), _cls, _cls->getName(), mthname,
	 list->cnt);
#endif

#ifdef STATIC_POLYMORPH
  const Class *xcls;
  if (o->asClass()) {
    _cls = o->asClass();
    xcls = _cls;
  }
#endif

  cls = _cls;
  if (!cls->getDatabase())
    cls = db->getSchema()->getClass(cls->getName());

  oqmlStatus *s;
  s = evalList(db, ctx);
  if (s) return s;

  TRS_PROLOGUE(cls);

  Method *xmth = 0;
#ifdef STATIC_POLYMORPH
  s = resolveMethod(db, ctx, (o->asClass() ? True: False), o, xmth);
#else
  s = resolveMethod(db, ctx, False, o, xmth);
#endif
  if (s) return s;

  TRS_EPILOGUE(s);

  if (xmth->getEx()->getLang() & C_LANG)
    s = applyC(db, ctx, xmth, alist, o, &oid);
  else
    s = applyOQL(db, ctx, xmth, alist, o, &oid);

  return s;
}

// -----------------------------------------------------------------------
//
// public destructor
//
// -----------------------------------------------------------------------

oqmlMethodCall::~oqmlMethodCall()
{
  free(clsname);
  free(mthname);
  free(atoms);
  free(tmp_atoms);
  if (deleteList)
    delete list;
}

// -----------------------------------------------------------------------
//
// private init() method: common initializer
//
// -----------------------------------------------------------------------

void
oqmlMethodCall::init(const char *_clsname, const char *_mthname,
		    oqml_List *_list)
{
#ifdef MTH_TRACE
  printf("oqmlMethodCall(%s, %s)\n", (_clsname ? _clsname : "<>"),
	 _mthname);
#endif
  mthname = strdup(_mthname);
  clsname = (_clsname ? strdup(_clsname) : 0);
  list = (_list ? _list : new oqml_List());
  if (!_list) deleteList = oqml_True;
  mth = 0;
  cls = 0;
  is_compiled = oqml_False;
  eval_type.type = oqmlATOM_UNKNOWN_TYPE;
  atoms     = (oqmlAtom **)calloc(sizeof(oqmlAtom *), list->cnt);
  tmp_atoms = (oqmlAtom **)calloc(sizeof(oqmlAtom *), list->cnt);
  last.cls = 0;
  last.xmth = 0;
}

oqmlStatus *oqmlMethodCall::checkStaticMethod()
{
  if (!(mth->getEx()->getLoc() & STATIC_EXEC))
    return new oqmlStatus(this, "method '%s::%s' is not a static method",
			 cls->getName(), mthname);

  if (mth->getEx()->getSign()->getNargs() != list->cnt)
    return new oqmlStatus(this, "method '%s::%s': %d argument(s) expected, "
			  "got %d",
			  cls->getName(), mthname,
			  mth->getEx()->getSign()->getNargs(),
			  list->cnt);
  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private noMethod()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::noMethod(Bool isStatic, oqmlContext *ctx,
			 const Method **mths, int mth_cnt)
{
  std::string s;

  if (call)
    {
      s = std::string("unknown function '") + mthname + "'";
      return new oqmlStatus(call, s.c_str());
    }

  if (noParenthesis)
    s = std::string("neither attribute ") + mthname + ", nor";
  else
    s = "no";
  
  s += std::string(" ") + (isStatic ? "class" : "instance") +
    " method " + mthname + "(" +  getSignature(ctx) + ") in class " +
    cls->getName();
  
  if (mth_cnt)
    {
      s += std::string(". Candidate") + (mth_cnt > 1 ? "s are: " : " is: ");
      for (int i = 0; i < mth_cnt; i++)
	{
	  if (i)
	    s += "; ";
	  s += mths[i]->getPrototype();
	}
    }

  return new oqmlStatus(this, s.c_str());
}

#if 0
// -----------------------------------------------------------------------
//
// private findStaticMethod()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::findStaticMethod(Database *db, oqmlContext *ctx)
{
  cls = db->getSchema()->getClass(clsname);
  if (!cls)
    return new oqmlStatus(this, "'%s' is not a class name", clsname);

  unsigned int cnt;
  Status xs = cls->getMethodCount(mthname, cnt);
  if (xs) return new oqmlStatus(this, xs);

  if (!cnt)
    return noMethod(True, ctx);

  if (cnt > 1)
    {
      // will be resolved in eval method
      return oqmlSuccess;
    }

  xs = cls->getMethod(mthname, (const Method *&)mth);
  if (xs) return new oqmlStatus(this, xs);

  return checkStaticMethod();
}
#endif


// -----------------------------------------------------------------------
//
// private evalList method
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::evalList(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  oqml_Link *l = list->first;

#ifdef MTH_TRACE
  printf("methodCall(%s, %s)->evalList()\n", cls->getName(), mthname);
#endif

  for (int i = 0; i < list->cnt; i++, l = l->next)
    {
      /*
      if (l->ql->getType() == oqmlIDENT)
	{
	  tmp_atoms[i] = new oqmlAtom_ident(((oqmlIdent *)l->ql)->getName());
	  continue;
	}
	*/

      oqmlAtomList *al = 0;
      s = l->ql->eval(db, ctx, &al, 0, 0);
      if (s)
	return s;

      if (al->cnt > 1)
	return new oqmlStatus(this, "method '%s::%s': argument #%d "
			     "cannot be a flattened list",
			     cls->getName(), mthname, i+1);

      if (al->cnt == 0)
	return new oqmlStatus(this, "method '%s::%s': argument #%d is undefined",
			      (cls ? cls->getName() : ""), mthname, i+1);
      tmp_atoms[i] = al->first;
#ifdef SYNC_GARB
      if (al && !al->refcnt) {
	al->first = 0;
	OQL_DELETE(al);
      }
#endif
    }

  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private compareAtomTypes() method
//
// -----------------------------------------------------------------------

oqmlBool oqmlMethodCall::compareAtomTypes()
{
  for (int i = 0; i < list->cnt; i++)
    if (!atoms[i] || atoms[i]->type.type != tmp_atoms[i]->type.type)
      return oqml_False;

  return oqml_True;
}

// -----------------------------------------------------------------------
//
// private compareType() method
//
// -----------------------------------------------------------------------

oqmlMethodCall::Match
oqmlMethodCall::compareType(oqmlContext *ctx, int n, oqmlAtom *x,
			    int odl_type, Bool strict)
{
  oqmlATOMTYPE _type = x->type.type;

  if (_type == oqmlATOM_IDENT)
    {
      oqmlAtomType at;
      if (!ctx->getSymbol(OQML_ATOM_IDENTVAL(x), &at, &x))
	return (strict ? no_match : match);
      _type = at.type;
    }

  odl_type = (ArgType_Type)(odl_type & ~INOUT_ARG_TYPE);

  switch(_type)
    {
    case oqmlATOM_OID:
    case oqmlATOM_OBJ:
      if (odl_type == OBJ_TYPE || odl_type == OID_TYPE)
	return exact_match;
      return no_match;

    case oqmlATOM_INT:
      if (odl_type == INT16_TYPE ||
	  odl_type == INT32_TYPE ||
	  odl_type == INT64_TYPE)
	return exact_match;
      return no_match;

    case oqmlATOM_CHAR:
      if (odl_type == CHAR_TYPE)
	return exact_match;
      return no_match;

    case oqmlATOM_DOUBLE:
      if (odl_type == FLOAT_TYPE)
	return exact_match;
      return no_match;

    case oqmlATOM_STRING:
      if (odl_type == STRING_TYPE)
	return exact_match;
      return no_match;

    case oqmlATOM_LIST:
    case oqmlATOM_ARRAY:
      if (odl_type & ARRAY_TYPE)
	{
	  oqmlAtom *xx = OQML_ATOM_COLLVAL(x)->first;
	  if (!xx)
	    return match;
	  Match mm = exact_match;
	  while (xx)
	    {
	      Match m = compareType(ctx, n, xx, odl_type&~ARRAY_TYPE,
				    strict);
	      if (m == no_match)
		return no_match;
	      if (m == match)
		mm = match;
	      xx = xx->next;
	    }
	  return mm;
	}
      return no_match;

    case oqmlATOM_NIL:
    case oqmlATOM_NULL:
    case oqmlATOM_BOOL:
      return no_match;

    default:
      return no_match;
    }
  
  return no_match;
}

// -----------------------------------------------------------------------
//
// private typeMismatch()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::typeMismatch(const Signature *sign, oqmlAtomType &at, int n)
{
  return new oqmlStatus(this, "method '%s::%s', argument #%d: %s expected, "
		       "got %s", cls->getName(), mthname, n+1,
		       Argument::getArgTypeStr(sign->getTypes(n)),
		       at.getString());
}

oqmlStatus *
oqmlMethodCall::typeMismatch(ArgType *type, Object *o, int n)
{
  return new oqmlStatus(this, "method '%s::%s', argument #%d: %s expected, "
		       "got %s", cls->getName(), mthname, n+1,
			type->getClname().c_str(), o->getClass()->getName());
}

// -----------------------------------------------------------------------
//
// private checkArguments(Database *)
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::checkArguments(Database *db, oqmlContext *ctx,
			       const Method *xmth, Match *m)
{
  const Signature *sign = xmth->getEx()->getSign();

  int nargs = sign->getNargs();
  if (m) *m = exact_match;
  for (int i = 0; i < nargs; i++)
    {
      ArgType_Type _type = sign->getTypes(i)->getType();
      if (_type & OUT_ARG_TYPE)
	{
	  if (tmp_atoms[i]->type.type != oqmlATOM_IDENT)
	    {
	      if (!m)
		return new oqmlStatus(this, "method '%s::%s': "
				      "out argument #%d must be set "
				      "to a symbol", cls->getName(),
				      mthname, i+1);
	      *m = no_match;
	      return oqmlSuccess;
	    }
	}

      Match mm = compareType(ctx, i, tmp_atoms[i], _type,
			     IDBBOOL(_type & IN_ARG_TYPE));
      if (mm == no_match)
	{
	  if (!m)
	    return typeMismatch(sign, tmp_atoms[i]->type, i);
	  *m = no_match;
	  return oqmlSuccess;
	}

      if (m && mm == match)
	*m = match;
    }
  
  memcpy(atoms, tmp_atoms, list->cnt * sizeof(oqmlAtom *));
  return oqmlSuccess;
}

#define SET(ARG, TYPE, L) \
(ARG)->set((TYPE *)calloc(sizeof(TYPE)*(L)->cnt, 1), (L)->cnt, \
			  Argument::AutoFullGarbage)

// -----------------------------------------------------------------------
//
// private buildArgArray()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::buildArgArray(Argument *arg, oqmlAtomList *_list,
			      ArgType_Type odl_type, int n)
{
  switch(odl_type)
    {
    case INT16_TYPE:
      SET(arg, eyedblib::int16, _list);
      break;

    case INT32_TYPE:
      SET(arg, eyedblib::int32, _list);
      break;

    case INT64_TYPE:
      SET(arg, eyedblib::int64, _list);
      break;

    case STRING_TYPE:
      SET(arg, char *, _list);
      break;

    case CHAR_TYPE:
      SET(arg, char, _list);
      break;

    case FLOAT_TYPE:
      SET(arg, double, _list);
      break;

    case OID_TYPE:
      SET(arg, Oid, _list);
      break;

    case OBJ_TYPE:
      SET(arg, Object *, _list);
      break;

    case VOID_TYPE:
      break;

    default:
      return new oqmlStatus(this, "method '%s::%s', argument #%d: odl type '%p' "
			   "is not supported",
			   cls->getName(), mthname,
			   n+1, odl_type);
    }

  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private fillArgArray()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::fillArgArray(Signature *sign,
			    Argument *arg, Argument tmp,
			    ArgType_Type odl_type, int j)
{
  switch(odl_type)
    {
    case INT16_TYPE:
      arg->u.arr_i16.i[j] = *tmp.getInteger16();
      break;

    case INT32_TYPE:
      arg->u.arr_i32.i[j] = *tmp.getInteger32();
      break;

    case INT64_TYPE:
      arg->u.arr_i64.i[j] = *tmp.getInteger64();
      break;

    case STRING_TYPE:
      arg->u.arr_s.s[j] = strdup(tmp.getString());
      break;

    case CHAR_TYPE:
      arg->u.arr_c.c[j] = *tmp.getChar();
      break;

    case FLOAT_TYPE:
      arg->u.arr_d.d[j] = *tmp.getFloat();
      break;

    case OID_TYPE:
      arg->u.arr_oid.oid[j] = *tmp.getOid();
      break;

    case OBJ_TYPE:
      arg->u.arr_o.o[j] = tmp.getObject();
      break;

    case VOID_TYPE:
      break;

    default:
      return new oqmlStatus(this, "method '%s::%s', argument #%d: odl type '%s' "
			   "is not supported",
			   cls->getName(), mthname,
			   j+1,
			   Argument::getArgTypeStr(sign->getTypes(j)));
    }

  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private makeArg()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::makeArg(Argument &tmp, ArgType_Type odl_type,
			const Argument *arg, int j)
{
  switch(odl_type)
    {
    case INT16_TYPE:
      tmp.set(arg->u.arr_i16.i[j]);
      break;

    case INT32_TYPE:
      tmp.set(arg->u.arr_i32.i[j]);
      break;

    case INT64_TYPE:
      tmp.set(arg->u.arr_i64.i[j]);
      break;

    case STRING_TYPE:
      tmp.set(arg->u.arr_s.s[j]);
      break;

    case CHAR_TYPE:
      tmp.set(arg->u.arr_c.c[j]);
      break;

    case FLOAT_TYPE:
      tmp.set(arg->u.arr_d.d[j]);
      break;

    case OID_TYPE:
      tmp.set(arg->u.arr_oid.oid[j]);
      break;

    case OBJ_TYPE:
      tmp.set(arg->u.arr_o.o[j]);
      break;

    default:
      assert(0);
      break;
    }

  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private getArgCount()
//
// -----------------------------------------------------------------------

unsigned int
oqmlMethodCall::getArgCount(const Argument *arg, ArgType_Type odl_type)
{
  switch(odl_type)
    {
    case INT16_TYPE:
      return arg->u.arr_i16.cnt;

    case INT32_TYPE:
      return arg->u.arr_i32.cnt;

    case INT64_TYPE:
      return arg->u.arr_i64.cnt;

    case STRING_TYPE:
      return arg->u.arr_s.cnt;

    case CHAR_TYPE:
      return arg->u.arr_c.cnt;

    case FLOAT_TYPE:
      return arg->u.arr_d.cnt;

    case OID_TYPE:
      return arg->u.arr_oid.cnt;

    case OBJ_TYPE:
      return arg->u.arr_o.cnt;

    default:
      assert(0);
      return 0;
    }

  return 0;
}

// -----------------------------------------------------------------------
//
// private getSignature()
//
// -----------------------------------------------------------------------

const char *
oqmlMethodCall::getSignature(oqmlContext *ctx)
{
  static char sign[256];
  *sign = 0;

  for (int i = 0; i < list->cnt; i++)
    {
      if (i)
	strcat(sign, ", ");
      if (tmp_atoms[i]->type.type == oqmlATOM_IDENT)
	{
	  oqmlAtomType at;
	  if (ctx->getSymbol(OQML_ATOM_IDENTVAL(tmp_atoms[i]), &at))
	    strcat(sign, at.getString());
	  else
	    strcat(sign, "??");
	}
      else
	strcat(sign, tmp_atoms[i]->type.getString());
    }

  return sign;
}

static oqmlStatus *
ambiguous(oqmlNode *node, Method *m1, Method **m2, int m2_cnt)
{
  std::string s = std::string("cannot resolve ambiguity "
			  "between methods '") +  m1->getPrototype() + "'";
  for (int i = 0; i < m2_cnt; i++)
    s += std::string(" and '") + m2[i]->getPrototype() + "'";

  return new oqmlStatus(node, s.c_str());
}

static bool
isParent(const Class *parent, Object *o)
{
  if (!o || !o->asClass()) return false;

  const Class *cls = o->asClass()->getClass();
#ifdef MTH_TRACE
  printf("isParent: %s %s vs. %s\n", o->asClass()->getName(), cls->getName(),
	 parent->getName());
#endif
  while (cls) {
    if (parent->compare(cls)) return true;
    cls = cls->getParent();
  }

  return false;
}

// -----------------------------------------------------------------------
//
// private resolveMethod()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::resolveMethod(Database *db, oqmlContext *ctx,
					  Bool isStatic, Object *o,
					  Method *&xmth)
{
  if (last.cls == cls && last.isStatic == isStatic && compareAtomTypes()) {
    xmth = last.xmth;
    return oqmlSuccess;
  }

  if (!cls) {
    if (clsname)
      return new oqmlStatus(this, "'%s' is not a class name", clsname);
    return new oqmlStatus(this, "unknown class");
  }

  Status xs = const_cast<Class *>(cls)->wholeComplete();
  if (xs) return new oqmlStatus(this, xs);

  char *rmthname = strrchr(mthname, ':');
  const Class *scopeClass = 0;

  if (rmthname) {
    char *scope = strdup(mthname);
    rmthname++;
    *(strrchr(scope, ':')-1) = 0;
    if (!(scopeClass = db->getSchema()->getClass(scope))) {
      oqmlStatus *s = new oqmlStatus(this, "invalid class '%s'", scope);
      free(scope);
      return s;
    }

    free(scope);
  }
  else
    rmthname = mthname;

  unsigned int cnt;
  xs = cls->getMethodCount(rmthname, cnt);
  if (xs) return new oqmlStatus(this, xs);

  if (!cnt)
    return noMethod(isStatic, ctx);

#ifdef MTH_TRACE
  printf("resolving method '%s' [isStatic %d] [class %s]\n",
	 rmthname, isStatic, cls->getName());
#endif
  unsigned int mth_cnt;
  const Method **mths = cls->getMethods(mth_cnt);
  const Method **cand_mths = (const Method **)
    malloc(mth_cnt * sizeof(Method *));
  Method **xmth_1 = (Method **)malloc(sizeof(Method*)*mth_cnt);

  int xmth_1_cnt = 0;
  xmth = 0;
  int cand_mth_cnt = 0;
  oqmlStatus *s;
  Match xm = no_match;

  for (int i = 0; i < mth_cnt; i++) {
    const Method *mx = mths[i];
    const Executable *ex = mx->getEx();
#ifdef MTH_TRACE
    printf("%s_method %s [%s]\n",
	   (ex->isStaticExec() ? "class" : "instance"),
	   ex->getExname(),
	   mx->getClassOwner()->getName());
#endif
    if (!strcmp(ex->getExname().c_str(), rmthname) &&
	((ex->isStaticExec() && isStatic) ||
	 (!ex->isStaticExec() && !isStatic) ||
	 (isStatic && isParent(mx->getClassOwner(), o)))) {
      /*
	!(ex->getLoc() & idbSTATIC_EXEC) && isStatic &&
	(!strcmp(mx->getClassOwner()->getName(), "object") ||
	!strcmp(mx->getClassOwner()->getName(), "class"))))) {
      */

	if (scopeClass && !mx->getClassOwner()->compare(scopeClass))
	  continue;

#ifdef MTH_TRACE
	printf("chosen!\n");
#endif
	cand_mths[cand_mth_cnt++] = mx;
	if (ex->getSign()->getNargs() != list->cnt)
	  continue;
    }
    else
      continue;

    Match m;
    if (s = checkArguments(db, ctx, mx, &m))
      return s;
    if (m == no_match)
      continue;
    
    if (!xmth) {
      xmth = (Method *)mx;
      xm = m;
      continue;
    }

    if (xm == exact_match && m == exact_match) {
      if (xmth->getClassOwner()->compare(mx->getClassOwner())) {
	oqmlStatus *s = ambiguous(this, xmth, (Method **)&mx, 1);
	free(mths);
	free(cand_mths);
	return s;
      }

      Bool is;
      Status s = xmth->getClassOwner()->isSuperClassOf
	(mx->getClassOwner(), &is);
      if (s) return new oqmlStatus(this, s);
      if (is)
	xmth = (Method *)mx;
    }
    
    if (xm == exact_match)
      continue;
    
    if (m != exact_match)
      xmth_1[xmth_1_cnt++] = xmth;

    xmth = (Method *)mx;
    xm = m;
  }

  if (xmth)
    s = xmth_1_cnt ? ambiguous(this, xmth, xmth_1, xmth_1_cnt) : oqmlSuccess;
  else
    s = noMethod(isStatic, ctx, cand_mths, cand_mth_cnt);

  free(mths);
  free(cand_mths);
  free(xmth_1);

  if (!s && xmth)
    {
      last.cls = cls;
      last.isStatic = isStatic;
      last.xmth = xmth;
    }

  return s;
}

// -----------------------------------------------------------------------
//
// private atomToArg()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::atomToArg(Database *db, oqmlContext *ctx,
				      Signature *sign,
				      oqmlAtom *a,
				      ArgType *type,
				      ArgType_Type odl_type,
				      Argument *arg, int i)
{
  oqmlAtomType at;
  oqmlStatus *s;

  oqmlATOMTYPE _type = a->type.type;

  if (_type == oqmlATOM_IDENT)
    {
      const char *ident = OQML_ATOM_IDENTVAL(a);
      if (!ctx->getSymbol(ident, &at, &a))
	return new oqmlStatus(this, "method '%s::%s', argument #%d: "
			      "symbol '%s' is undefined",
			      cls->getName(), mthname,
			     i+1, ident);
      _type = at.type;
    }
  else
    at.type = _type;
  
  switch(_type)
    {
    case oqmlATOM_OID:
      if (odl_type == OID_TYPE)
	arg->set(OQML_ATOM_OIDVAL(a));
      else if (odl_type == OBJ_TYPE)
	{
	  Object *o;
	  Oid oid_o = OQML_ATOM_OIDVAL(a);
	  if (oid_o.isValid()) {
	    Status is = db->loadObject(oid_o, o);
	    if (is) return new oqmlStatus(this, is);
	    if (strcmp(type->getClname().c_str(), o->getClass()->getName()))
	      return typeMismatch(type, o, i);
	  }
	  else
	    o = 0;

	  arg->set(o);
	}
      else
	return typeMismatch(sign, at, i);

      break;

    case oqmlATOM_OBJ:
      if (odl_type == OBJ_TYPE)
	{
	  OQL_CHECK_OBJ(a);
	  Object *o = OQML_ATOM_OBJVAL(a);
	  if (o) {
	    o->incrRefCount();
	    if (strcmp(type->getClname().c_str(), o->getClass()->getName()))
	      return typeMismatch(type, o, i);
	    arg->set(o);
	  }
	  else
	    arg->set(Oid::nullOid);
	}
      else if (odl_type == OID_TYPE)
	{
	  OQL_CHECK_OBJ(a);
	  Object *o = OQML_ATOM_OBJVAL(a);
	  if (o)
	    arg->set(o->getOid());
	  else
	    arg->set(Oid::nullOid);
	}
      else
	return typeMismatch(sign, at, i);
      break;

    case oqmlATOM_INT:
      {
	eyedblib::int64 ii = ((oqmlAtom_int *)a)->i;
	if (odl_type == INT16_TYPE)
	  arg->set((eyedblib::int16)ii);
	else if (odl_type == INT32_TYPE)
	  arg->set((eyedblib::int32)ii);
	else if (odl_type == INT64_TYPE)
	  arg->set(ii);
	else
	  return typeMismatch(sign, at, i);
	break;
      }

    case oqmlATOM_CHAR:
      if (odl_type == CHAR_TYPE)
	arg->set(((oqmlAtom_char *)a)->c);
      else
	return typeMismatch(sign, at, i);
      break;

    case oqmlATOM_DOUBLE:
      if (odl_type == FLOAT_TYPE)
	arg->set(((oqmlAtom_double *)a)->d);
      else
	return typeMismatch(sign, at, i);
      break;

    case oqmlATOM_STRING:
      if (odl_type == STRING_TYPE)
	arg->set(OQML_ATOM_STRVAL(a));
      else
	return typeMismatch(sign, at, i);

      break;

    case oqmlATOM_LIST:
    case oqmlATOM_ARRAY:
      if (!(odl_type & ARRAY_TYPE))
	return typeMismatch(sign, at, i);
      {
	oqmlAtomList *_list = OQML_ATOM_COLLVAL(a);
	odl_type = (ArgType_Type)(odl_type & ~ARRAY_TYPE);
	s = buildArgArray(arg, _list, odl_type, i);
	if (s)
	  return s;
	oqmlAtom *a = _list->first;
	Argument tmp;
	for (int j = 0; j < _list->cnt; j++, a = a->next)
	  {
	    s = atomToArg(db, ctx, sign, a, type, odl_type, &tmp, i);
	    if (s)
	      return s;

	    s = fillArgArray(sign, arg, tmp, odl_type, j);
	    if (s)
	      return s;
	  }

	return oqmlSuccess;
      }

    default:
      return typeMismatch(sign, at, i);
    }
  
  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private atomsToArgs()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::atomsToArgs(Database *db, oqmlContext *ctx,
				      Method *xmth, ArgArray &arr)
{
  oqmlStatus *s;
  Signature *sign = xmth->getEx()->getSign();
  for (int i = 0; i < list->cnt; i++)
    {
      ArgType *type = sign->getTypes(i);
      ArgType_Type odl_type = type->getType();

      if (!(odl_type & IN_ARG_TYPE))
	continue;

      odl_type = (ArgType_Type)(odl_type & ~INOUT_ARG_TYPE);

      s = atomToArg(db, ctx, sign, tmp_atoms[i], type, odl_type, arr[i], i);
      if (s)
	return s;
    }
  
  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// private argToAtom()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::argToAtom(const Argument *arg, int n,
				      oqmlAtomType &at, oqmlAtom *&a)
{
  oqmlStatus *s;

  at.cls = 0;
  at.comp = oqml_False;

  ArgType_Type odl_type = arg->getType()->getType();
  if (odl_type & ARRAY_TYPE)
    {
      odl_type = (ArgType_Type)(odl_type & ~ARRAY_TYPE);
      at.type = oqmlATOM_LIST;
      int cnt = getArgCount(arg, odl_type);
      oqmlAtomList *_list = new oqmlAtomList();
      a = new oqmlAtom_list(_list);
      for (int j = 0; j < cnt; j++)
	{
	  oqmlAtomType xat;
	  oqmlAtom *x;
	  Argument tmp;
	  s = makeArg(tmp, odl_type, arg, j);
	  if (s)
	    return s;
	  s = argToAtom(&tmp, j, xat, x);
	  if (s)
	    return s;
	  _list->append(x);
	}

      return oqmlSuccess;
    }

  switch(odl_type)
    {
    case OID_TYPE:
      at.type = oqmlATOM_OID;
      a = new oqmlAtom_oid(*arg->getOid());
      break;

    case OBJ_TYPE:
      at.type = oqmlATOM_OBJ;
      a = oqmlObjectManager::registerObject((Object *)arg->getObject());
      if (arg->getObject())
	(const_cast <Object *>(arg->getObject()))->incrRefCount();
      break;

    case INT16_TYPE:
    case INT32_TYPE:
    case INT64_TYPE:
      at.type = oqmlATOM_INT;
      a = new oqmlAtom_int(arg->getInteger());
      break;

    case STRING_TYPE:
      {
	at.type = oqmlATOM_STRING;
	const char *s = arg->getString();
	a = new oqmlAtom_string(s ? s : "");
      }
    break;

    case CHAR_TYPE:
      at.type = oqmlATOM_CHAR;
      a = new oqmlAtom_char(*arg->getChar());
      break;

    case FLOAT_TYPE:
      at.type = oqmlATOM_DOUBLE;
      a = new oqmlAtom_double(*arg->getFloat());
      break;

    case VOID_TYPE:
      break;

    default:
      return new oqmlStatus(this, "method '%s::%s', argument #%d: "
			    "odl type '%s' is not supported",
			   cls->getName(), mthname,
			   n+1,
			   Argument::getArgTypeStr(arg->getType()));
    }

  return oqmlSuccess;
}
  
// -----------------------------------------------------------------------
//
// private argsToAtoms()
//
// -----------------------------------------------------------------------

oqmlStatus *oqmlMethodCall::argsToAtoms(Database *db,  oqmlContext *ctx,
				      Method *xmth,
				      const ArgArray &arr,
				      const Argument &retarg,
				      oqmlAtom *&retatom)
{
  oqmlStatus *s;
  Signature *sign = xmth->getEx()->getSign();
  oqmlAtomType at;

  for (int i = 0; i < list->cnt; i++)
    {
      const Argument *arg = arr[i];
      ArgType_Type odl_type = sign->getTypes(i)->getType();
      if (!(odl_type & OUT_ARG_TYPE))
	continue;
      oqmlAtom *a;
      s = argToAtom(arg, i, at, a);
      if (s)
	return s;

      const char *ident = OQML_ATOM_IDENTVAL(tmp_atoms[i]);
      oqmlBool global;
      if (!ctx->getSymbol(ident, 0, 0, &global) || global ||
	  !strncmp(ident, oqml_global_scope, oqml_global_scope_len))
	s = ctx->setSymbol(ident, &at, a, oqml_True);
      else
	s = ctx->setSymbol(ident, &at, a, oqml_False);

      if (s) return s;
    }

  return argToAtom(&retarg, list->cnt, at, retatom);
}

// -----------------------------------------------------------------------
//
// private applyC()
//
// -----------------------------------------------------------------------

oqmlStatus *
oqmlMethodCall::applyC(Database *db, oqmlContext *ctx,
		       Method *xmth,
		       oqmlAtomList **alist, Object *o, const Oid *oid)
{
  oqmlStatus *s;
  ArgArray arr(list->cnt, Argument::AutoFullGarbage);
  Argument retarg;

#ifdef MTH_TRACE
  printf("applyC methodCall(%s, %d arguments)\n", mthname, list->cnt);
#endif

  s = atomsToArgs(db, ctx, xmth, arr);
  if (s)
    return s;

  Status xs;
  if (oid && !o) {
    xs = db->loadObject(*oid, o);
    if (xs) return new oqmlStatus(this, xs);
  }

  void *x = db->setUserData(IDB_LOCAL_CALL);

  xs = xmth->applyTo(db, o, arr, retarg, False);

  db->setUserData(x);

  if (xs)
    return new oqmlStatus(this, xs);

  oqmlAtom *retatom = 0;
  s = argsToAtoms(db, ctx, xmth, arr, retarg, retatom);

  if (!s && retatom)
    (*alist)->append(retatom);

  return s;
}

// -----------------------------------------------------------------------
//
// private applyOQL()
//
// -----------------------------------------------------------------------

// introduced the 19/06/01
#define NEW_THIS

oqmlStatus *
oqmlMethodCall::applyOQL(Database *db, oqmlContext *ctx,
			 Method *xmth,
			 oqmlAtomList **alist, Object *o, const Oid *oid)
{
  static const char thisvar[] = "this";
#ifndef NEW_THIS
  static const char pthisvar[] = "pthis";
#endif

  // added the 3/07/01
  int select_ctx_cnt = ctx->setSelectContextCount(0);

  oqmlStatus *s = oqmlSuccess;
  oqmlStatus *s1, *s2;
  BEMethod_OQL *oqlmth = xmth->asBEMethod_OQL();

  if (!oqlmth)
    return new oqmlStatus(this, "internal error #243");

  Status is = oqlmth->runtimeInit();
  if (is) return new oqmlStatus(this, is);

  if (!oqlmth->entry)
    {
      oqmlAtomList *al;
      s = oqml_realize(db, oqlmth->fullBody, &al);
      if (s) return s;
      if (!ctx->getFunction(oqlmth->funcname,
			    (oqmlFunctionEntry **)&oqlmth->entry))
	return new oqmlStatus(this, "internal error #244");
    }

#ifdef NEW_THIS
  pointer_int_t idx;
  if (o && oqmlObjectManager::isRegistered(o, idx)) {
    oqmlAtom_obj * obj_x;
    obj_x = new oqmlAtom_obj(o, idx, o->getClass());
    s = ctx->pushSymbol(thisvar, &obj_x->type, obj_x);
  }
  else {
    if (!oid && o)
      oid = &o->getOid();

    if (!oid)
      return new oqmlStatus(this, "invalid null object");

    oqmlAtom_oid *oid_x;
    oid_x = new oqmlAtom_oid(*oid);
    s = ctx->pushSymbol(thisvar, &oid_x->type, oid_x);
    if (s) return s;
  }
#else

  oqmlAtom_oid * oid_x;
  if (oid)
    oid_x = new oqmlAtom_oid(*oid);
  else
    oid_x = new oqmlAtom_oid(Oid::nullOid);

  s = ctx->pushSymbol(pthisvar, &oid_x->type, oid_x);
  if (s) return s;

  oqmlAtom_obj * obj_x;
  pointer_int_t idx;

  if (o && oqmlObjectManager::isRegistered(o, idx))
    obj_x = new oqmlAtom_obj(o, idx, o->getClass());
  else
    obj_x = new oqmlAtom_obj(0, 0);

  s = ctx->pushSymbol(thisvar, &obj_x->type, obj_x);
  if (s) return s;
#endif

  int n;
  oqml_Link *l = list->first;
  for (n = 0; n < oqlmth->param_cnt; n++)
    {
      oqmlAtomList *al;
      s1 = l->ql->eval(db, ctx, &al);
      s2 = ctx->pushSymbol(oqlmth->varnames[n], &al->first->type,
			   al->first);
      if (s1) s = s1;
      if (s2) s = s2;

      l = l->next;
    }

  if (!s)
    {
      oqmlAtomList *al;
      s = oqmlCall::realizeCall(db, ctx, (oqmlFunctionEntry *)oqlmth->entry,
				&al);
      if (!s)
	{
	  (*alist)->first = al->first;
	  (*alist)->cnt = al->cnt;
	}
    }

  for (n = 0; n < oqlmth->param_cnt; n++)
    {
      s1 = ctx->popSymbol(oqlmth->varnames[n]);
      if (s1 && !s) s = s1;
    }

  s1 = ctx->popSymbol(thisvar);
  if (s1 && !s) s = s1;

#ifndef NEW_THIS
  s1 = ctx->popSymbol(pthisvar);
  if (s1 && !s) s = s1;
#endif

  // added the 3/07/01
  ctx->setSelectContextCount(select_ctx_cnt);

  return s;
}

extern int oqmlLevel;

#define OQML_AUTO(P, C, D) \
  struct P##__ { \
    P##__ () {(C);} \
    ~P##__() {(D);} \
  } P##_

oqmlStatus *
oqmlMethodCall::applyTrigger(Database *db, Trigger *trig,
			     Object *o, const Oid *oid)
{
  OQML_AUTO(, oqmlLevel++, --oqmlLevel);

  static const char thisvar[] = "this";
  static const char pthisvar[] = "pthis";

  oqmlStatus *s = oqmlSuccess;
  oqmlStatus *s1, *s2;

  oqmlContext ctx;

  if (!trig->entry)
    {
      oqmlAtomList *al;
      s = oqml_realize(db, trig->fullBody, &al);
      if (s) return s;
      if (!ctx.getFunction(trig->funcname,
			    (oqmlFunctionEntry **)&trig->entry))
	return new oqmlStatus("internal error #244");
    }

  oqmlAtom_oid * oid_x;
  if (oid)
    oid_x = new oqmlAtom_oid(*oid);
  else
    oid_x = new oqmlAtom_oid(Oid::nullOid);

  s = ctx.pushSymbol(pthisvar, &oid_x->type, oid_x);
  if (s) return s;

  oqmlAtom *obj_x = oqmlObjectManager::registerObject(o);

  s = ctx.pushSymbol(thisvar, &obj_x->type, obj_x);
  if (s) return s;

  oqmlAtomList *al;
  s = oqmlCall::realizeCall(db, &ctx, (oqmlFunctionEntry *)trig->entry,
			    &al);
  s1 = ctx.popSymbol(thisvar);
  if (s1 && !s) s = s1;

  s1 = ctx.popSymbol(pthisvar);
  if (s1 && !s) s = s1;

  s1 = oqmlObjectManager::unregisterObject(0, o);
  if (s1 && !s) s = s1;

  return s;
}

oqmlBool
oqmlMethodCall::hasIdent(const char *_ident)
{
  return list->hasIdent(_ident);
}

void
oqmlMethodCall::lock()
{
  oqmlNode::lock();
  if (call) call->lock(); // added the 17/05/01
  if (list) list->lock();
}

void
oqmlMethodCall::unlock()
{
  oqmlNode::unlock();
  if (call) call->unlock(); // added the 17/05/01
  if (list) list->unlock();
}

std::string
oqmlMethodCall::toString(void) const
{
  std::string s = (clsname ? std::string(clsname) + "::" : std::string("")) + mthname + "(";
  oqml_Link *l = list->first;

  for (int i = 0; l; i++)
    {
      if (i) s += ",";
      s += l->ql->toString();
      l = l->next;
    }

  s += std::string(")") + oqml_isstat();
  return s;
}

}
