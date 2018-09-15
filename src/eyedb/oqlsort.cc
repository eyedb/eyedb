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

#define RS(r) (r ? "r" : "")
#define CHECK_TYPE(X) oqml_check_sort_type(node, (X)->type)

// -----------------------------------------------------------------------
//
// oqmlSort utility functions
//
// -----------------------------------------------------------------------

namespace eyedb {

oqmlStatus *
oqml_check_sort_type(oqmlNode *node, oqmlAtomType &type, const char *msg)
{
  if (type.type != oqmlATOM_INT &&
      type.type != oqmlATOM_STRING &&
      type.type != oqmlATOM_DOUBLE &&
      type.type != oqmlATOM_CHAR &&
      type.type != oqmlATOM_NULL)
    return new oqmlStatus(node, (msg ? msg :
				 "a collection of integer, string, "
				 "double or character is expected, got %s"),
			  type.getString());

  return oqmlSuccess;
}

static void
oqml_sort_realize(oqmlAtom **array, int array_cnt, oqmlBool reverse,
		  oqmlAtomList **alist)
{
  *alist = new oqmlAtomList();

  if (reverse)
    {
      for (int n = array_cnt - 1; n >= 0; n--)
	(*alist)->append(array[n]);

      free(array);
      return;
    }  

  for (int n = 0; n < array_cnt; n++)
    (*alist)->append(array[n]);

  free(array);
}

#define CHECK_NULL(LEFT, RIGHT) \
if ((LEFT)->as_null() && (RIGHT)->as_null()) \
  return 0; \
if ((LEFT)->as_null())\
  return -1;\
if ((RIGHT)->as_null())\
  return 1

static int
oqml_sort_int(const void *x1, const void *x2)
{
  oqmlAtom *aleft = *(oqmlAtom **)x1;
  oqmlAtom *aright = *(oqmlAtom **)x2;

  CHECK_NULL(aleft, aright);
  return (OQML_ATOM_INTVAL(aleft) - OQML_ATOM_INTVAL(aright));
}

static int
oqml_sort_string(const void *x1, const void *x2)
{
  oqmlAtom *aleft = *(oqmlAtom **)x1;
  oqmlAtom *aright = *(oqmlAtom **)x2;

  CHECK_NULL(aleft, aright);
  return strcmp(OQML_ATOM_STRVAL(aleft), OQML_ATOM_STRVAL(aright));
}

static int
oqml_sort_double(const void *x1, const void *x2)
{
  oqmlAtom *aleft = *(oqmlAtom **)x1;
  oqmlAtom *aright = *(oqmlAtom **)x2;

  CHECK_NULL(aleft, aright);
  return (OQML_ATOM_DBLVAL(aleft) - OQML_ATOM_DBLVAL(aright));
}

static int
oqml_sort_char(const void *x1, const void *x2)
{
  oqmlAtom *aleft = *(oqmlAtom **)x1;
  oqmlAtom *aright = *(oqmlAtom **)x2;

  CHECK_NULL(aleft, aright);
  return (OQML_ATOM_CHARVAL(aleft) - OQML_ATOM_CHARVAL(aright));
}

oqmlAtom **
oqml_make_array(oqmlAtomList *list, int &cnt)
{
  oqmlAtom **array = idbNewVect(oqmlAtom *, list->cnt);
  oqmlAtom *atom = list->first;

  for (cnt = 0; atom; cnt++)
    {
      array[cnt] = atom;
      atom = atom->next;
    }

  return array;
}

static oqmlStatus *
checkTypes(oqmlNode *node, oqmlAtomList *list, oqmlATOMTYPE &atom_type)
{
  oqmlAtom *atom = OQML_ATOM_COLLVAL(list->first)->first;

  if (!atom)
    return oqmlSuccess;

  atom_type = (oqmlATOMTYPE)0;

  while (atom)
    {
      CHECK_TYPE(atom);

      if (!atom_type)
	{
	  if (atom->type.type != oqmlATOM_NULL)
	    atom_type = atom->type.type;
	}
      else if (atom->type.type != oqmlATOM_NULL &&
	       atom_type != atom->type.type)
	return new oqmlStatus(node, "atom types in collection "
			      "are no homogeneous");
      atom = atom->next;
    }

  if (atom_type)
    {
      oqmlAtomType at(atom_type);
      oqmlStatus *s = oqml_check_sort_type(node, at);
      if (s) return s;
    }

  return oqmlSuccess;
}

void
oqml_sort_simple(oqmlAtomList *ilist, oqmlBool reverse,
		 oqmlATOMTYPE atom_type, oqmlAtomList **alist)
{
  oqmlAtom **array;
  int array_cnt, n;

  array = oqml_make_array(ilist, array_cnt);

  if (atom_type == oqmlATOM_INT)
    qsort(array, array_cnt, sizeof(oqmlAtom *), oqml_sort_int);
  else if (atom_type == oqmlATOM_STRING)
    qsort(array, array_cnt, sizeof(oqmlAtom *), oqml_sort_string);
  else if (atom_type == oqmlATOM_DOUBLE)
    qsort(array, array_cnt, sizeof(oqmlAtom *), oqml_sort_double);
  else if (atom_type == oqmlATOM_CHAR)
    qsort(array, array_cnt, sizeof(oqmlAtom *), oqml_sort_char);
  /* else : all is NULL */
      
  oqml_sort_realize(array, array_cnt, reverse, alist);
}

// -----------------------------------------------------------------------
//
// oqmlSort public methods
//
// -----------------------------------------------------------------------

oqmlSort::oqmlSort(oqml_List *_list, oqmlBool _reverse) :
  oqmlXSort(oqmlSORT, _list, _reverse)
{
}

oqmlSort::~oqmlSort()
{
}

oqmlStatus *oqmlSort::compile(Database *db, oqmlContext *ctx)
{
  if (!xlist || xlist->cnt < 1)
    return new oqmlStatus(this, "one argument expected");

  tosort = xlist->first->ql;

  if (xlist->cnt != 1) return new oqmlStatus(this, "one argument expected");

  return tosort->compile(db, ctx);
}

oqmlStatus *oqmlSort::eval(Database *db, oqmlContext *ctx,
			   oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *list;
  int n;

  s = tosort->eval(db, ctx, &list);

  if (s) return s;

  if (list->cnt != 1 || !OQML_IS_COLL(list->first))
    return new oqmlStatus(this, "collection expected");

  oqmlATOMTYPE atom_type;
  s = checkTypes(this, list, atom_type);
  if (s) return s;

  oqmlAtomList *ylist;
  oqml_sort_simple(OQML_ATOM_COLLVAL(list->first), reverse, atom_type, &ylist);
  *alist = new oqmlAtomList(new oqmlAtom_list(ylist));
  return oqmlSuccess;
}

void oqmlSort::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlSort::isConstant() const
{
  return tosort->isConstant();
}

std::string
oqmlSort::toString(void) const
{
  return (reverse ? std::string("rsort(") : std::string("sort(")) +
    (tosort ? tosort->toString() : std::string("")) + ")" + oqml_isstat();
}

// -----------------------------------------------------------------------
//
// oqmlISort utility functions
//
// -----------------------------------------------------------------------

struct oqmlAtomPair {
  oqmlAtom *list;
  oqmlAtom *atom;
};

static int
oqml_isort_int(const void *x1, const void *x2)
{
  oqmlAtomPair *ap1 = (oqmlAtomPair *)x1;
  oqmlAtomPair *ap2 = (oqmlAtomPair *)x2;

  CHECK_NULL(ap1->atom, ap2->atom);
  return (OQML_ATOM_INTVAL(ap1->atom) - OQML_ATOM_INTVAL(ap2->atom));
}

static int
oqml_isort_string(const void *x1, const void *x2)
{
  oqmlAtomPair *ap1 = (oqmlAtomPair *)x1;
  oqmlAtomPair *ap2 = (oqmlAtomPair *)x2;

  CHECK_NULL(ap1->atom, ap2->atom);
  return strcmp(OQML_ATOM_STRVAL(ap1->atom), OQML_ATOM_STRVAL(ap2->atom));
}

static int
oqml_isort_double(const void *x1, const void *x2)
{
  oqmlAtomPair *ap1 = (oqmlAtomPair *)x1;
  oqmlAtomPair *ap2 = (oqmlAtomPair *)x2;

  CHECK_NULL(ap1->atom, ap2->atom);
  return (OQML_ATOM_DBLVAL(ap1->atom) - OQML_ATOM_DBLVAL(ap2->atom));
}

static int
oqml_isort_char(const void *x1, const void *x2)
{
  oqmlAtomPair *ap1 = (oqmlAtomPair *)x1;
  oqmlAtomPair *ap2 = (oqmlAtomPair *)x2;

  CHECK_NULL(ap1->atom, ap2->atom);
  return (OQML_ATOM_CHARVAL(ap1->atom) - OQML_ATOM_CHARVAL(ap2->atom));
}

static oqmlAtom *
oqml_get_atom(oqmlAtom *atom_list, int idx)
{
  oqmlAtomList *list = OQML_ATOM_COLLVAL(atom_list);
  oqmlAtom *atom = list->first;

  for (int i = 0; atom; i++)
    {
      if (i == idx)
	return atom;
      atom = atom->next;
    }

  return 0;
}

oqmlAtomPair *
oqml_make_array(oqmlAtomList *list, int &cnt, int idx)
{
  oqmlAtomPair *array = idbNewVect(oqmlAtomPair, list->cnt);
  oqmlAtom *atom = list->first;

  for (cnt = 0; atom; cnt++)
    {
      if (!OQML_IS_COLL(atom))
	break;
      array[cnt].list = atom;
      array[cnt].atom = oqml_get_atom(atom, idx);
      atom = atom->next;
    }

  return array;
}

static void
oqml_sort_realize(oqmlAtomPair *array, int array_cnt, oqmlBool reverse,
		  oqmlAtomList **alist)
{
  *alist = new oqmlAtomList();

  if (reverse)
    {
      for (int n = array_cnt - 1; n >= 0; n--)
	(*alist)->append(array[n].list);

      free(array);
      return;
    }  

  for (int n = 0; n < array_cnt; n++)
    (*alist)->append(array[n].list);

  free(array);
}

void
oqml_sort_list(oqmlAtomList *ilist, oqmlBool reverse, int idx,
	       oqmlATOMTYPE atom_type, oqmlAtomList **alist)
{
  oqmlAtomPair *array;
  int array_cnt, n;

  array = oqml_make_array(ilist, array_cnt, idx);

  if (atom_type == oqmlATOM_INT)
    qsort(array, array_cnt, sizeof(oqmlAtomPair), oqml_isort_int);
  else if (atom_type == oqmlATOM_STRING)
    qsort(array, array_cnt, sizeof(oqmlAtomPair), oqml_isort_string);
  else if (atom_type == oqmlATOM_DOUBLE)
    qsort(array, array_cnt, sizeof(oqmlAtomPair), oqml_isort_double);
  else if (atom_type == oqmlATOM_CHAR)
    qsort(array, array_cnt, sizeof(oqmlAtomPair), oqml_isort_char);
  else
    abort();
  
  oqml_sort_realize(array, array_cnt, reverse, alist);
}

static oqmlStatus *
checkTypes(oqmlNode *node, oqmlAtomList *list, int idx, oqmlATOMTYPE &atom_type)
{
  oqmlAtom *atom = OQML_ATOM_COLLVAL(list->first)->first;

  if (!atom)
    return oqmlSuccess;

  atom_type = (oqmlATOMTYPE)0;

  while (atom)
    {
      if (!OQML_IS_LIST(atom) && !OQML_IS_ARRAY(atom))
	return new oqmlStatus(node, "a collection of lists or arrays "
			      "was expected");

      if (OQML_ATOM_COLLVAL(atom)->cnt <= idx)
	return new oqmlStatus(node, "index '%d' too large "
			      "for collection", idx);

      oqmlAtom *x = OQML_ATOM_COLLVAL(atom)->first;

      for (int i = 0; x; x = x->next, i++)
	if (i == idx)
	  {
	    CHECK_TYPE(x);
	    if (!atom_type)
	      {
		atom_type = x->type.type;
		break;
	      }
	    
	    if (atom_type != x->type.type)
	      return new oqmlStatus(node, "atom types at index %d "
				    "in list or array are not homogeneous",
				    idx);
	  }

      atom = atom->next;
    }

  if (atom_type)
    {
      oqmlAtomType at(atom_type);
      oqmlStatus *s = oqml_check_sort_type(node, at);
      if (s) return s;
    }

  return oqmlSuccess;
}

// -----------------------------------------------------------------------
//
// oqmlISort public methods
//
// -----------------------------------------------------------------------

oqmlISort::oqmlISort(oqml_List *_list, oqmlBool _reverse) :
  oqmlXSort(oqmlISORT, _list, _reverse)
{
  idxnode = 0;
}

oqmlISort::~oqmlISort()
{
}

oqmlStatus *oqmlISort::compile(Database *db, oqmlContext *ctx)
{
  if (!xlist || xlist->cnt < 1)
    return new oqmlStatus(this, "two arguments expected");

  tosort = xlist->first->ql;

  if (xlist->cnt < 2) return new oqmlStatus(this, "two arguments expected");
  idxnode = xlist->first->next->ql;

  if (xlist->cnt != 2) return new oqmlStatus(this, "two arguments expected");

  oqmlStatus *s = tosort->compile(db, ctx);
  if (s) return s;

  return idxnode->compile(db, ctx);
}

oqmlStatus *oqmlISort::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *list;

  s = tosort->eval(db, ctx, &list);
  if (s) return s;

  if (list->cnt != 1 || !OQML_IS_COLL(list->first))
    return new oqmlStatus(this, "first argument: collection expected");

  oqmlAtomList *idxlist;
  s = idxnode->eval(db, ctx, &idxlist);
  if (s) return s;

  if (idxlist->cnt != 1 || !OQML_IS_INT(idxlist->first))
    return new oqmlStatus(this, "second argument: integer expected");

  int idx = OQML_ATOM_INTVAL(idxlist->first);

  if (idx < 0)
    return new oqmlStatus(this, "second argument: "
			  "positive integer expected");

  oqmlATOMTYPE atom_type;
  s = checkTypes(this, list, idx, atom_type);
  if (s) return s;

  oqmlAtomList *ylist;
  oqml_sort_list(OQML_ATOM_COLLVAL(list->first), reverse, idx,
		 atom_type, &ylist);
  *alist = new oqmlAtomList(new oqmlAtom_list(ylist));
  return oqmlSuccess;

  return oqmlSuccess;
}

void oqmlISort::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlISort::isConstant() const
{
  return tosort->isConstant();
}

std::string
oqmlISort::toString(void) const
{
  return (reverse ? std::string("risort(") : std::string("isort(")) +
    (tosort ? tosort->toString() : std::string("")) +
    (idxnode ? std::string(",") + idxnode->toString() : std::string("")) 
    + ")" + oqml_isstat();
}

// -----------------------------------------------------------------------
//
// oqmlPSort public methods
//
// -----------------------------------------------------------------------

// -------------------------- NOT YET IMPLEMENTED ------------------------

#if 0
static int
oqml_sort_pred_realize(const void *x1, const void *x2)
{
  oqmlAtom *aleft = *(oqmlAtom **)x1;
  oqmlAtom *aright = *(oqmlAtom **)x2;
  oqmlAtomList *input = new oqmlAtomList();
  input->append(aleft);
  input->append(aright);
  if (oqml_predarg)
    input->append(oqml_predarg);

  oqmlAtom *r;
  oqmlCall::realizeCall(oqml_db, oqml_ctx, oqml_entry, input,
			&r);
}

static void
oqml_sort_pred(oqmlAtomList *al, oqmlFunctionEntry *entry,
	       oqmlAtomList *predarglist,
	       oqmlBool reverse, oqmlAtomList **alist)
{
  oqmlAtom **array;
  int nn;

  array = oqml_make_array(al, nn);

  qsort(array, nn, sizeof(oqmlAtom *), oqml_sort_pred_realize);
      
  *alist = new oqmlAtomList();
  
  if (reverse)
    for (int i = nn-1; i >= 0; i--)
      (*alist)->append(array[i]);
  else
    for (int i = 0; i < nn; i++)
      (*alist)->append(array[i]);
  
  free(array);
}

oqmlPSort::oqmlPSort(oqml_List *_list, oqmlBool _reverse) :
  oqmlXSort(oqmlPSORT, _list, _reverse)
{
}

oqmlPSort::~oqmlPSort()
{
}

oqmlStatus *oqmlPSort::compile(Database *db, oqmlContext *ctx)
{
  if (list->cnt < 2)
    return usage();

  tosort = list->first->ql;
  pred = list->first->next->ql;

  oqmlStatus *s = tosort->compile(db, ctx);
  if (s) return s;

  s = pred->compile(db, ctx);
  if (s) return s;

  predarg = list->first->next ? list->first->next->ql : 0;
  if (predarg)
    retun predarg->compile(db, ctx);

  return oqmlSuccess;
}

oqmlStatus *oqmlPSort::usage()
{
  return new oqmlStatus(this, "usage: %spsort(collection of anything, "
			"predicat [, predargs])", RS(reverse));
}

oqmlStatus *oqmlPSort::eval(Database *db, oqmlContext *ctx,
			   oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;
  int n;

  s = tosort->eval(db, ctx, &al);
  if (s) return s;

  oqmlAtomList *predlist;
  s = pred->eval(db, ctx, &predlist);
  if (s) return s;
  if (predlist->cnt != 1 || !OQML_IS_IDENT(predlist->first))
    return usage();

  if (!ctx->getFunction(OQML_ATOM_IDENT(predlist->first, &entry)))
    return new oqmlStatus(this, "unknown function '%s'",
			  OQML_ATOM_IDENT(predlist->first));

  oqmlAtomList *predarglist = 0;
  if (predarg)
    {
      s = predarg->eval(db, ctx, &predarglist);
      if (s) return s;
    }

  oqmlAtomList *ylist;
  oqml_sort_pred(al, entry, predarglist, reverse, &ylist);
  *alist = new oqmlAtomList(new oqmlAtom_list(ylist));
  return oqmlSuccess;
}

void oqmlPSort::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlPSort::isConstant() const
{
  return tosort->isConstant();
}

std::string
oqmlPSort::toString(void) const
{
  return (reverse ? std::string("rpsort(") : std::string("psort(")) +
    tosort->toString() + ", " + pred->toString() +
    (predarg ? std::string(", ") + predarg->toString() : std::string("")) +
    ")" + oqml_isstat();
}
#endif
}
