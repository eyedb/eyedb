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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "oql_p.h"

#define MAX_RANGE (int)0x0A000000

namespace eyedb {

  oqmlBool
  oqml_is_getcount(oqml_ArrayList *arr)
  {
    if (!arr)
      return oqml_False;
    return arr->is_getcount;
  }

  oqmlBool
  oqml_is_wholecount(oqml_ArrayList *arr)
  {
    if (!arr)
      return oqml_False;
    return OQMLBOOL(arr->is_getcount && arr->is_wholecount);
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // private utility methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  static oqmlStatus *
  oqml_wait_for_int(oqmlNode *node, Database *db,
		    oqmlContext *ctx, oqmlNode *ql)
  {
    oqmlAtomType type;
    oqmlStatus *s;

    if (ql->getType() != oqmlIDENT)
      {
	s = ql->compile(db, ctx);
	if (s)
	  return s;

	if (ql->isConstant())
	  {
	    ql->evalType(db, ctx, &type);
	    if (type.type != oqmlATOM_INT)
	      return oqmlStatus::expected(node, "integer", type.getString());
	  }
      }

    return oqmlSuccess;
  }


  static oqmlStatus *
  oqml_wait_for_int(oqmlNode *node, Database *db, oqmlContext *ctx,
		    oqmlNode *ql, int *i)
  {
    oqmlStatus *s;
    oqmlAtomList *alist;

    s = ql->eval(db, ctx, &alist);

    if (s)
      return s;

    if (alist->cnt != 1)
      return new oqmlStatus(node, "integer expected");

    if (alist->first->type.type != oqmlATOM_INT)
      return oqmlStatus::expected(node, "integer",
				  alist->first->type.getString());

    *i = ((oqmlAtom_int *)(alist->first))->i;
    return oqmlSuccess;
  }

  static inline int 
  oqml_getinc(const TypeModifier *tmod, int n)
  {
    int inc = 1;
    for (int i = tmod->ndims-1; i > n; i--)
      inc *= tmod->dims[i];

    return inc;
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlArray methods and subclass methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /*
    x := "hello";
    x[0]   -> 'h'
    x[0:1] -> list('h', 'e')
    x[?]   -> list('h', 'e', 'l', 'l', 'o', '\000')

    list(1, 2, 3)[0] -> 1
    list(1, 2, 3)[?] -> list(1, 2, 3)
  
    list(1, list(2, 3), 4)[1] -> list(2, 3)
    list(1, list(2, 3), 4)[1][1] -> 3
  
    list(1, "hello", 2)[1][0] -> 'h'
    list(1, "hello", 2)[1:2][0] -> error
  */

  // -----------------------------------------------------------------------
  //
  // oqml_ArrayLink methods
  //
  // -----------------------------------------------------------------------

  struct oqmlLinkItem {
    int from, to;
    oqmlBool untilEnd;
    oqmlBool wholeRange;
    oqmlLinkItem() {
      from = to = 0;
      untilEnd = wholeRange = oqml_False;
    }
  };

  oqml_ArrayLink::oqml_ArrayLink(oqmlBool _wholeCount)
  {
    qleft = 0;
    qright = 0;
    next = 0;
    wholeCount = _wholeCount;
    wholeRange = oqml_False;
  }

  oqml_ArrayLink::oqml_ArrayLink(oqmlNode *_qleft, oqmlNode *_qright)
  {
    if (_qleft)
      {
	qleft = _qleft;
	qright = _qright;
	wholeRange = oqml_False;
	next = 0;
	return;
      }

    qleft = new oqmlInt((long long)0);
    qright = new oqmlInt(MAX_RANGE);
    wholeRange = oqml_True;
    next = 0;
  }

  oqmlBool
  oqml_ArrayLink::hasIdent(const char *_ident)
  {
    return OQMLBOOL((qleft && qleft->hasIdent(_ident)) ||
		    (qright && qright->hasIdent(_ident)));
  }

  oqmlBool
  oqml_ArrayLink::isGetCount() const
  {
    return OQMLBOOL(!qleft && !qright);
  }

  std::string
  oqml_ArrayLink::toString() const
  {
    if (isGetCount())
      return wholeCount ? "[!!]" : "[!]";

    if (wholeRange)
      return "[?]";

    return std::string("[") +
      (qleft ? qleft->toString() : std::string("")) +
      (qright ? std::string(":") + qright->toString() : std::string("")) + "]";
  }

  oqmlStatus *
  oqml_ArrayLink::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;
    if (qleft)
      {
	s = qleft->compile(db, ctx);
	if (s) return s;
      }

    if (qright)
      return qright->compile(db, ctx);
    return oqmlSuccess;
  }

  static oqmlStatus *
  oqml_eval_link(oqmlNode *node, Database *db, oqmlContext *ctx, oqmlNode *ql, int &val, oqmlBool &untilEnd)
  {
    if (ql->getType() == oqmlIDENT &&
	!strcmp(((oqmlIdent *)ql)->getName(), "$")) {
      val = MAX_RANGE;
      untilEnd = oqml_True;
      return oqmlSuccess;
    }

    oqmlAtomList *al;
    oqmlStatus *s = ql->eval(db, ctx, &al);
    if (s) return s;

    if (al->cnt == 0)
      return new oqmlStatus(node, "invalid right operand: "
			    "integer expected.");

    if (al->cnt != 1 || !al->first->as_int())
      return new oqmlStatus(node, "invalid right operand: "
			    "integer expected, got '%s'.",
			    al->first->type.getString());

    val = OQML_ATOM_INTVAL(al->first);
    untilEnd = oqml_False;
    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_ArrayLink::eval(oqmlNode *node, Database *db, oqmlContext *ctx,
		       oqmlLinkItem &item)
  {
    if (wholeRange) {
      item.from = 0;
      item.to = MAX_RANGE;
      item.wholeRange = oqml_True;
      return oqmlSuccess;
    }

    item.wholeRange = oqml_False;
    oqmlStatus *s;

    oqmlBool dummy;
    s = oqml_eval_link(node, db, ctx, qleft, item.from, dummy);
    if (s) return s;

    if (qright)
      return oqml_eval_link(node, db, ctx, qright, item.to, item.untilEnd);

    item.to = item.from;
    return oqmlSuccess;
  }

  void
  oqml_ArrayLink::lock()
  {
    if (qleft) qleft->lock();
    if (qright) qright->lock();
  }

  void
  oqml_ArrayLink::unlock()
  {
    if (qleft) qleft->unlock();
    if (qright) qright->unlock();
  }

  oqml_ArrayLink::~oqml_ArrayLink()
  {
  }

  // -----------------------------------------------------------------------
  //
  // oqml_ArrayList methods
  //
  // -----------------------------------------------------------------------

  oqml_ArrayList::oqml_ArrayList()
  {
    count = 0;
    first = last = 0;
    is_getcount = oqml_False;
    is_wholecount = oqml_False;
    wholeRange = oqml_True;
  }

  void oqml_ArrayList::add(oqml_ArrayLink *link)
  {
    if (last)
      last->next = link;
    else
      first = link;

    if (!is_getcount)
      {
	is_getcount = link->isGetCount();
	is_wholecount = link->wholeCount;
      }

    if (!link->wholeRange)
      wholeRange = oqml_False;

    last = link;
    count++;
  }

  oqmlBool
  oqml_ArrayList::hasIdent(const char *_ident)
  {
    oqml_ArrayLink *l = first;

    while (l)
      {
	if (l->hasIdent(_ident))
	  return oqml_True;
	l = l->next;
      }

    return oqml_False;
  }

  std::string
  oqml_ArrayList::toString() const
  {
    oqml_ArrayLink *l = first;
    std::string s = "";
    while (l)
      {
	s += l->toString();
	l = l->next;
      }

    return s;
  }

  oqmlStatus *
  oqml_ArrayList::compile(Database *db, oqmlContext *ctx)
  {
    if (is_getcount)
      return oqmlSuccess;

    oqmlStatus *s;
    oqml_ArrayLink *l = first;

    while (l)
      {
	s = l->compile(db, ctx);
	if (s) return s;
	l = l->next;
      }

    return oqmlSuccess;
  }

  oqmlNode *oqmlArray::getLeft()
  {
    return ql;
  }

  oqml_ArrayList *oqmlArray::getArrayList()
  {
    return list;
  }

  oqmlStatus *
  oqml_ArrayList::eval(oqmlNode *node, Database *db, oqmlContext *ctx,
		       oqmlLinkItem *&items, int &item_cnt)
  {
    oqmlStatus *s;
    oqml_ArrayLink *l = first;

    item_cnt = count;
    items = new oqmlLinkItem[count];

    for (int i = 0; l ; i++)
      {
	s = l->eval(node, db, ctx, items[i]);
	if (s) return s;
	l = l->next;
      }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_ArrayList::checkCollArray(oqmlNode *node, const Class *cls,
				 const char *attrname)
  {
    if (!last->wholeRange && !cls->asCollArrayClass())
      return new oqmlStatus(node, "non array collection invalid operator: "
			    "'%s%s'", attrname, toString().c_str());
    oqml_ArrayLink *l = first;
    while (l != last)
      {
	if (l->qright)
	  return new oqmlStatus(node, "collection attribute contents '%s[%s]': "
				"value ranges are not valid "
				"for intermediate dimensions",
				attrname, toString().c_str());

	l = l->next;
      }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_ArrayList::eval(oqmlNode *node, Database *db, oqmlContext *ctx,
		       const char *clname,
		       const char *attrname, const TypeModifier *tmod,
		       int *s_ind, int *e_ind, oqmlBool nocheck)
  {
    *s_ind = 0;
    *e_ind = 0;

    if (is_getcount)
      return oqmlSuccess;

    oqmlStatus *s;
    int i;
    int n = 0;
    oqml_ArrayLink *l = first;

    while (l)
      {
	int incdim = oqml_getinc(tmod, n);
	s = oqml_wait_for_int(node, db, ctx, l->qleft, &i);

	if (s)
	  return s;

	int maxdim = (tmod && tmod->dims ? tmod->dims[n] : 0); // added test on tmod 17/01/01

	if (maxdim > 0 && i >= maxdim)
	  {
	    if (nocheck)
	      i = maxdim-1;
	    else
	      return new oqmlStatus(node, "attribute '%s' in class '%s' : "
				    "out of range dimension #%d: "
				    "maximum allowed is %d <got %d>",
				    attrname, clname,
				    n, maxdim-1, i);
	  }

	*s_ind += i * incdim;

	if (l->qright)
	  {
	    s = oqml_wait_for_int(node, db, ctx, l->qright, &i);

	    if (s)
	      return s;

	    if (i == -1)
	      {
		if (maxdim > 0)
		  i = maxdim-1;
		else
		  i = 10000000;
	      }
	    else if (maxdim > 0 && i >= maxdim)
	      {
		if (nocheck)
		  i = maxdim-1;
		else
		  return new oqmlStatus(node, "attribute '%s' in class '%s' : "
					"out of range dimension #%d: "
					"maximum allowed is %d, got %d)",
					attrname, clname,
					n, maxdim, i);
	      }

	    *e_ind += i * incdim;
	  }
	else
	  *e_ind += i * incdim;

	l = l->next;
	n++;
      }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_ArrayList::evalCollArray(oqmlNode *node, Database *db,
				oqmlContext *ctx,
				const TypeModifier *tmod, int &ind)
  {
    oqmlStatus *s;
    oqml_ArrayLink *l = first;

    ind = 0;

    for (int n = 0; l != last; n++)
      {
	int incdim = oqml_getinc(tmod, n);
	int i;
	s = oqml_wait_for_int(node, db, ctx, l->qleft, &i);

	if (s) return s;

	int maxdim = tmod->dims[n];

	if (maxdim > 0 && i >= maxdim)
	  return new oqmlStatus(node, "out of range dimension #%d: "
				"maximum allowed is %d, got %d",
				n, maxdim, i);

	ind += i * incdim;

	l = l->next;
	n++;
      }

    return oqmlSuccess;
  }

  void
  oqml_ArrayList::lock()
  {
    oqml_ArrayLink *l = first;
    while (l)
      {
	l->lock();
	l = l->next;
      }
  }

  void
  oqml_ArrayList::unlock()
  {
    oqml_ArrayLink *l = first;
    while (l)
      {
	l->unlock();
	l = l->next;
      }
  }

  oqml_ArrayList::~oqml_ArrayList()
  {
    oqml_ArrayLink *l = first;
    while (l)
      {
	oqml_ArrayLink *next = l->next;
	delete l;
	l = next;
      }
  }

  // -----------------------------------------------------------------------
  //
  // oqmlArray methods
  //
  // -----------------------------------------------------------------------

  oqmlArray::oqmlArray(oqmlNode * _ql) : oqmlNode(oqmlARRAY)
  {
    ql = _ql;
    list = new oqml_ArrayList();
    delegationArray = oqml_False;
    returnStruct = oqml_True;
  }

  oqmlArray::oqmlArray(oqmlNode * _ql, oqml_ArrayList *_list,
		       oqmlBool _returnStruct) : oqmlNode(oqmlARRAY)
  {
    ql = _ql;
    list = _list;
    delegationArray = oqml_True;
    returnStruct = _returnStruct;
  }

  oqmlStatus *
  oqmlArray::evalMake(Database *db, oqmlContext *ctx, Object *o,
		      oqml_ArrayList *list, oqmlBool returnStruct, oqmlAtomList **alist)
  {
    oqmlArray *array = new oqmlArray(new oqmlObject(o, 0), list,
				     returnStruct);
    oqmlStatus *s = array->compile(db, ctx);
    if (s) return s;
    return array->eval(db, ctx, alist);
  }

  oqmlArray::~oqmlArray()
  {
    if (!delegationArray)
      delete list;
  }

  void oqmlArray::add(oqml_ArrayLink *link)
  {
    list->add(link);
  }

  oqmlStatus *oqmlArray::compile(Database *db, oqmlContext *ctx)
  {
    oqmlDotContext *dctx = ctx->getDotContext();
    oqmlStatus *s;

    if (!dctx)
      {
	s = ql->compile(db, ctx);
	if (s) return s;
	return list->compile(db, ctx);
      }

    if (ql->getType() == oqmlIDENT)
      {
	const char *name = ((oqmlIdent *)ql)->getName();
	oqmlDotDesc *desc = &dctx->desc[dctx->count-1];
      
	Class *cls = (Class *)desc->cls;

	Attribute *attr = (Attribute *)desc->attr;
	if (cls) // test added the 24/01/00
	  {
	    attr = (Attribute *)cls->getAttribute(name);
	  
	    if (!attr)
	      return new oqmlStatus(this, "attribute '%s' not "
				    "found in class '%s'", name, cls->getName());
	  }
      
	if (!list->is_getcount)
	  {
	    oqml_ArrayLink *l = list->first;

	    while (l)
	      {
		s = oqml_wait_for_int(this, db, ctx, l->qleft);
	      
		if (s)
		  return s;

		if (l->qright)
		  {
		    s = oqml_wait_for_int(this, db, ctx, l->qright);
		  
		    if (s)
		      return s;
		  }
	      
		l = l->next;
	      }
	  }

	s = dctx->add(db, ctx, attr, list, (char *)name, 0, 0, 0);
	if (s)
	  return s;
	eval_type = dctx->dot_type;
	return oqmlSuccess;
      }

    return new oqmlStatus(this, "currently cannot deal with no ident left "
			  "part in array");
  }

#define MAKELEFTVALUE(X) \
do { \
  if (ridx) \
  { \
    if (dim == item_cnt - 1) \
      { \
        if (n != 0) \
  	{ \
  	  delete[] items; \
  	  return new oqmlStatus(this, "invalid left value."); \
  	} \
        *ridx = idx; \
        *ra = x; \
      } \
    else \
      xlist->append(X); \
  } \
  else \
    xlist->append(X); \
} while(0)

  oqmlStatus *
  oqmlArray::checkRange(oqmlLinkItem *items, int dim, int idx, int len,
			oqmlBool &stop, const char *msg)
  {
    if (idx >= len && items[dim].untilEnd)
      {
	stop = oqml_True;
	return oqmlSuccess;
      }

    if (idx < 0 || idx >= len)
      {
	if (!items[dim].wholeRange)
	  {
	    delete[] items;
	    return new oqmlStatus(this, "out of bounds "
				  "value, %d, for %s", idx, msg);
	  }
	stop = oqml_True;
	return oqmlSuccess;
      }

    stop = oqml_False;
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlArray::evalRealize_1(Database *db, oqmlContext *ctx,
			   oqmlAtom *x, oqmlAtomList **alist, oqmlAtom **ra,
			   int *ridx)
  {
    static const char fmt[] = "invalid operand, "
      "string, collection or struct.";

    if (ridx)
      return new oqmlStatus(this, "invalid left operand");

    if (!x->as_string() && !x->as_coll() && !x->as_struct())
      return new oqmlStatus(this, fmt);

    if (x->as_string())
      {
	if (alist)
	  *alist =
	    new oqmlAtomList(new oqmlAtom_int(OQML_ATOM_STRLEN(x)));
	return oqmlSuccess;
      }

    if (x->as_coll())
      {
	if (alist)
	  *alist =
	    new oqmlAtomList(new oqmlAtom_int(OQML_ATOM_COLLVAL(x)->cnt));
	return oqmlSuccess;
      }

    if (x->as_struct())
      {
	if (alist)
	  *alist =
	    new oqmlAtomList(new oqmlAtom_int(x->as_struct()->attr_cnt));
	return oqmlSuccess;
      }

    return new oqmlStatus(this, fmt);
  }


  oqmlStatus *oqmlArray::evalRealize_2(Database *db, oqmlContext *ctx,
				       oqmlAtom *x, oqmlAtomList **alist,
				       oqmlAtom **ra, int *ridx)
  {
    static const char fmt[] = "invalid%soperand, "
      "string, list, array or struct.";

    oqmlLinkItem *items = 0;
    int item_cnt;
    oqmlStatus *s;

    s = list->eval(this, db, ctx, items, item_cnt);

    if (s) return s;

    if (ridx) {
      *ridx = -1;
      *ra = 0;
    }

    if (!x->as_string() && !x->as_list() && !x->as_array() &&
	!x->as_struct() && !OQML_IS_OBJECT(x))
      return new oqmlStatus(this, fmt, ridx ? " left " : " ");

    for (int dim = 0; dim < item_cnt; dim++) {
      oqmlAtomList *xlist = new oqmlAtomList();
      oqmlBool stop;

      if (!x)
	break;

      if (x->as_string()) {
	const char *str = OQML_ATOM_STRVAL(x);
	int len = OQML_ATOM_STRLEN(x) + (ridx ? 0 : 1);
	for (int idx = items[dim].from, n = 0; idx <= items[dim].to; idx++,
	       n++) {
	  // WARNING: the OQL library function 'strlen' is
	  // very penalised with the construct:
	  // 'std::string("string \"") + str + "\"")'
	  // because for each recursion, the construction of this string
	  // is called although it is not necessary.
	  s = checkRange(items, dim, idx, len, stop,
			 //std::string("string \"") + str + "\"");
			 str);
	  if (s) return s;
	  if (stop) break;
	
	  MAKELEFTVALUE(new oqmlAtom_char(str[idx]));
	}
      }
      else if (x->as_list() || x->as_array()) {
	oqmlAtomList *alist = OQML_ATOM_COLLVAL(x);
	for (int idx = items[dim].from, n = 0; idx <= items[dim].to; idx++,
	       n++) {
	  s = checkRange(items, dim, idx, alist->cnt, stop, "list");
	  if (s) return s;
	  if (stop) break;
	
	  MAKELEFTVALUE(alist->getAtom(idx)->copy());
	}
      }
      else if (x->as_struct()) {
	oqmlAtom_struct *astruct = x->as_struct();
	for (int idx = items[dim].from, n = 0; idx <= items[dim].to; idx++,
	       n++) {
	  s = checkRange(items, dim, idx, astruct->attr_cnt, stop, "struct");
	  if (s) return s;
	  if (stop) break;
	
	  MAKELEFTVALUE(astruct->attr[idx].value->copy());
	}
      }
      else if (OQML_IS_OBJECT(x)) {
	OQL_CHECK_OBJ(x);
	Object *o = 0;
	s = oqmlObjectManager::getObject(this, db, x, o, oqml_True,
					 oqml_True);
	if (s) return s;
	if (!o->asCollArray())
	  return new oqmlStatus(this, "only support array collection");
      
	Bool is_ref;
	(void)o->getClass()->asCollectionClass()->getCollClass(&is_ref);
	if (!is_ref)
	  return new oqmlStatus(this, "only support collection array of "
				"objects");
	if (ridx) {
	  if (items[dim].from != items[dim].to)
	    return new oqmlStatus(this, "invalid left value");
	  int n = 0;
	  int idx = items[dim].from;
	  if (OQML_IS_OBJ(x))
	    MAKELEFTVALUE(new oqmlAtom_oid(OQML_ATOM_OBJVAL(x)->getOid()));
	  else
	    MAKELEFTVALUE(x->copy());
	  continue;
	}
      
	Status is = Success;
	for (int idx = items[dim].from, n = 0; idx <= items[dim].to; idx++,
	       n++) {
	  if (n >= o->asCollArray()->getTop())
	    break;
	  if (dim == item_cnt - 1) {
	    Oid obj_oid;
	    is = o->asCollArray()->retrieveAt(idx, obj_oid);
	    if (is) break;

	    if (returnStruct && items[dim].from != items[dim].to)
	      xlist->append(oqml_make_struct_array(db, ctx, idx, new oqmlAtom_oid(obj_oid)));
	    else
	      xlist->append(new oqmlAtom_oid(obj_oid));
	  }
	  else {
	    xlist->append(oqmlObjectManager::registerObject(o));
	  }
	}
      
	if (is) return new oqmlStatus(this, is);
      }
      else {
	delete[] items;
	return new oqmlStatus(this, fmt, ridx ? " left " : " ");
      }
    
      if (xlist->cnt > 1) {
	if (x->as_array())
	  x = new oqmlAtom_array(xlist);
	else
	  x = new oqmlAtom_list(xlist);
      }
      else
	x = xlist->first;
    }
  
    if (alist)
      *alist = new oqmlAtomList(x);

    delete[] items;
    return oqmlSuccess;
  }


  oqmlStatus *oqmlArray::evalRealize(Database *db, oqmlContext *ctx,
				     oqmlAtomList **alist, oqmlAtom **ra,
				     int *ridx)
  {
    oqmlStatus *s;

    oqmlAtomList *alleft;
    s = ql->eval(db, ctx, &alleft);

    if (s)
      return s;

    if (alleft->cnt != 1)
      return new oqmlStatus(this, "invalid left operand.");

    oqmlAtom *x = alleft->first;
    if (list->is_getcount)
      {
	if (list->is_wholecount)
	  return new oqmlStatus(this, "invalid operand");
	return evalRealize_1(db, ctx, x, alist, ra, ridx);
      }

    return evalRealize_2(db, ctx, x, alist, ra, ridx);
  }

  oqmlStatus *oqmlArray::eval(Database *db, oqmlContext *ctx,
			      oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    return evalRealize(db, ctx, alist, 0, 0);
  }

  oqmlStatus *oqmlArray::evalLeft(Database *db, oqmlContext *ctx,
				  oqmlAtom **ra, int &ridx)
  {
    return evalRealize(db, ctx, 0, ra, &ridx);
  }

  void oqmlArray::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlArray::isConstant() const
  {
    return oqml_False;
  }

  oqmlBool
  oqmlArray::hasIdent(const char *_ident)
  {
    return OQMLBOOL(((ql && ql->hasIdent(_ident)) ||
		     (list && list->hasIdent(_ident))));
  }

  std::string
  oqmlArray::toString(void) const
  {
    return ql->toString() + list->toString();
  }

  void
  oqmlArray::lock()
  {
    oqmlNode::lock();
    ql->lock();
    list->lock();
  }

  void
  oqmlArray::unlock()
  {
    oqmlNode::unlock();
    ql->unlock();
    list->unlock();
  }
}
