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

#ifdef USE_LIBGEN
zoulou !!!
#include <libgen.h>
#endif

#include <sys/types.h>
#include <regex.h>

#include "oql_p.h"
#include "Attribute_p.h"

namespace eyedb {

#define AND(N) ((N) ? ", " : "")

#define ISNULL(K) (((K) == Attribute::idxNull) ? True : False)
#define idxISNULL(ATOM) \
((ATOM)->type.type == oqmlATOM_NULL ? Attribute::idxNull : \
Attribute::idxNotNull)

static char CI = 'I';
static char CS = 'S';

static oqmlBool equal_oid(Data data, Bool isnull,
			 const oqmlAtom *start, const oqmlAtom *, int, void *)
{
  if (isnull)
    return oqml_False;
  Oid oid;
  memcpy(&oid, data, sizeof(Oid));
  return OQMLBOOL(oid.compare(((oqmlAtom_oid *)start)->oid));
}

static int
getThreadCount(oqmlContext *ctx)
{
  oqmlAtom *thread_cnt_a;

  if (ctx->getSymbol("oql$thread$count", 0, &thread_cnt_a)) {
    if (thread_cnt_a && OQML_IS_INT(thread_cnt_a))
      return OQML_ATOM_INTVAL(thread_cnt_a);
  }

  return 0;
}

static oqmlStatus *
oqmlMakeIter(oqmlNode *node,
	     Database *db, oqmlContext *ctx, oqmlDotContext *dctx, int n,
	     oqmlAtomList *alist, oqmlAtom *start, oqmlAtom *end,
	     oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
			     const oqmlAtom *, int, void *),
	     int depth,
	     int thread_cnt = 0,
	     void *user_data = 0,
	     eyedbsm::Boolean sexcl = eyedbsm::False, eyedbsm::Boolean eexcl = eyedbsm::False);

static inline oqmlBool
oqmlCheckType(Database *db, Oid *cloid, const Class *cls)
{
  // optimisation added the 4/9/99
  if (cls->getOid() == *cloid)
    return oqml_True;

  // ici, on pourrait ajouter une methode dans Class:
  // Bool Class::isSuperClassOf(const Oid &)
  Class *rcl;

  rcl = db->getSchema()->getClass(*cloid);
  if (!rcl)
    return oqml_False;

  Bool issuperclassof;
  return OQMLBOOL(((Class *)cls)->isSuperClassOf(rcl, &issuperclassof)
		 == Success && issuperclassof);
}

static const Class *
oqmlIterGetClass(Database *db, oqmlDotContext *dctx, int n, int &pn,
		 oqmlBool &mustCheck)
{
  const Class *cls = NULL;
  oqmlDotDesc *d = &dctx->desc[n];

  mustCheck = (db->getVersionNumber() >= MULTIIDX_VERSION) ? oqml_False :
    oqml_True;

  pn = n;
  if (dctx->ident_mode)
    cls = d->cls;
  else
    {
      // added the 17/11/99
      d = &dctx->desc[pn];
      if (d->array && !d->array->wholeRange)
	mustCheck = oqml_True;
      // ...

      for (; pn >= 1; pn--)
	{
	  d = &dctx->desc[pn-1];
	  if (d->array && !d->array->wholeRange)
	    mustCheck = oqml_True;

	  if (d->isref)
	    {
	      cls = d->cls;
	      break;
	    }
	}

      if (!cls)
	cls = dctx->desc[0].cls;
    }

  if (!cls->getDatabase())
    cls = db->getSchema()->getClass(cls->getName());

  return cls;
}

//
// MIND: Cette fonction est fausse en ce qui concerne la boucle
// d'indice: en effet, elle est appelée depuis makeiter_noidx:
// elle fait une boucle se `s_ind' a `e_ind' puis retourne 
// le derniere valeur de `data' qu'elle a trouvé: c'est totalement
// faux. 22/08/98
//
// En fait, il semble que cela ne marche pas dans les cas suivants:
// X.y[range].z ou `y' est un agregat direct.
//

static Status
oqmlIterPrelim(oqmlNode *node, Database *db, oqmlContext *ctx, Object *o,
	       oqmlDotDesc *d, Data* pdata, const Attribute *&xattr)
{
  oqmlStatus *s;
  Status is;

  if (d->attrname)
    {
      xattr = o->getClass()->getAttribute(d->attrname);

      /*
      if (!xattr)
	printf("WARNING NO ATTRIBUTE %s for class %s\n",
	       d->attrname, o->getClass()->getName());
	       */

      if (d->attr)
	return Success;

      s = d->make(db, ctx, (oqmlDot *)node, xattr, d->array, d->attrname, 0);
      if (s) return Exception::make(IDB_OQL_ERROR, s->msg);
      if (!*pdata)
	*pdata = d->e_data;

      return Success;
    }

  assert(d->qlmth);

  if (!((oqmlMethodCall *)d->qlmth)->isCompiled())
    {
      s = d->qlmth->compile(db, ctx);
      if (s) return Exception::make(IDB_OQL_ERROR, s->msg);
    }
  
  xattr = 0;
  return Success;
}

static Status
oqmlIterGetValue(oqmlNode *node, 
		 Database *db, oqmlContext *ctx, Object *o,
		 oqmlDotContext *dctx, int n, int pn, Data* pdata,
		 Bool *isnull, int nb, int ind,
		 const Attribute *xattr, Size &size);

static Status
oqmlIterGetValueMiddle(oqmlNode *node, 
		       Database *db, oqmlContext *ctx, Object *o,
		       oqmlDotContext *dctx, int n, int pn, Data* pdata,
		       Bool *isnull, int nb, int ind, oqmlDotDesc *d,
		       const Attribute *xattr, Size &size)
{
  oqmlStatus *s;
  int s_ind, e_ind;

  if (!d->icurs)
    {
      s = d->evalInd(node, db, ctx, s_ind, e_ind, oqml_True, oqml_False);

      if (s)
	return Exception::make(s->msg);
      
      if (s_ind != e_ind)
	d->icurs = new oqmlIterCursor(s_ind, e_ind);
    }
  else
    {
      s_ind = d->icurs->s_ind;
      e_ind = d->icurs->e_ind;
    }

  for (int tind = s_ind; tind <= e_ind; tind++)
    {
      IDB_CHECK_INTR();
      Status is = Success;
      Object *o1 = 0;

      oqmlBool must_release_o1;

      if (xattr)
	{
	  is = xattr->getValue((Agregat *)o, (Data *)&o1, 1, tind,
			       isnull); // idem
	  if (is)
	    {
	      delete d->icurs;
	      d->icurs = 0;
	      return is;
	    }
	  must_release_o1 = oqml_False;
	}
      else
	{
	  oqmlAtomList *al = new oqmlAtomList();
	  s = ((oqmlMethodCall *)d->qlmth)->perform(db, ctx,
						    o, o->getOid(),
						    o->getClass(),
						    &al);
	  if (s) return Exception::make(IDB_OQL_ERROR, s->msg);

	  s = oqmlObjectManager::getObject(node, db, al->first, o1,
					   oqml_False, oqml_False);
	  if (s) return Exception::make(IDB_OQL_ERROR, s->msg);
	  must_release_o1 = oqml_True;
	}

      if (o1)
	{
	  o1->setDatabase(db);
	  is = oqmlIterGetValue(node, db, ctx, o1, dctx, n, pn+1, pdata,
				isnull, nb, ind, 0, size);

	  if (!is && nb == Attribute::directAccess)
	    {
	      // changed (back again!?) the 16/11/99
	      /*
		if (*(char **)*pdata)
		*(char **)*pdata = strdup(*(char **)*pdata);
		else
		*(char **)*pdata = strdup("");
		oqmlGarbManager::add(*(char **)*pdata);
		*/
	      if ((char *)*pdata)
		*pdata = (unsigned char *)strdup((char *)*pdata);
	      else
		*pdata = (unsigned char *)strdup("");

	      // BUG CORRECTED THE 11/02/02!!!!
	      //oqmlGarbManager::add((char *)pdata);
	      oqmlGarbManager::add((char *)*pdata);
	    }

	  if (must_release_o1)
	    oqmlObjectManager::releaseObject(o1);
	  
	}

      if (s_ind == e_ind)
	{
	  delete d->icurs;
	  d->icurs = 0;
	}
      else
	d->icurs->s_ind++;

      if (!is || !d->icurs)
	return is;
    }
}

static Status
oqmlIterGetValueTerminal(oqmlNode *node, 
			 Database *db, oqmlContext *ctx, Object *o,
			 oqmlDotContext *dctx, int n, int pn, Data* pdata,
			 Bool *isnull, int nb, int ind, oqmlDotDesc *d,
			 const Attribute *xattr, Size &size)
{
  oqmlStatus *s;
  Status is;

  if (!xattr && !d->qlmth)
    return Success;

  if (!xattr)
    {
      oqmlAtomList *al = new oqmlAtomList();
      s = ((oqmlMethodCall *)d->qlmth)->perform(db, ctx,
						o, o->getOid(),
						o->getClass(),
						&al);

      if (s) return Exception::make(IDB_OQL_ERROR, s->msg);

      oqmlAtom *x = al->first;
      if (!x)
	return Success;

      if ((OQML_IS_OID(x) && !OQML_ATOM_OIDVAL(x).isValid()) ||
	  ((OQML_IS_OBJ(x) && !OQML_ATOM_OBJVAL(x))) ||
	  x->as_null())
	*isnull = True;

      Data val = 0; int len;
      x->getData(*pdata, &val, size, len);

      if (nb == Attribute::directAccess)
	*(char **)*pdata = (char *)val;
      else if (val)
	{
	  int len = strlen((char *)val)+1;
	  if (len > d->key_len)
	    {
	      s = new oqmlStatus(node,
				 "internal query error: string overflow: "
				 "'%s'", val);
	      return Exception::make(IDB_OQL_ERROR, s->msg);
	    }

	  memcpy(*pdata, val, len);
	}

      return Success;
    }

  if (d->isref && !d->is_coll)
    {
      is = xattr->getOid((Agregat *)o, (Oid *)*pdata, 1, ind);
      if (is) return is;
      if (!((Oid *)*pdata)->isValid())
	*isnull = True;

      return Success;
    }
  else if (nb != Attribute::directAccess && oqml_is_getcount(d->array))
    {
      if (oqml_is_wholecount(d->array))
	return Exception::make(IDB_ERROR, "cannot perform this query");

      Size size;
      Status is = xattr->getSize(o, size);
      if (is) return is;
      memcpy(*pdata, &size, sizeof(size));
      *isnull = False;
      return Success;
    }

  if (nb == Attribute::directAccess)
    is = xattr->getValue((Agregat *)o, pdata, nb, ind, isnull);
  else
    is = xattr->getValue((Agregat *)o, (Data *)*pdata, nb, ind, isnull);

  /* MIND: c'est ici que doit se faire la comparaison et l'ajout
	 eventuel dans la liste d'oids! */

  return is;
}

static Status
oqmlIterGetValue(oqmlNode *node, 
		 Database *db, oqmlContext *ctx, Object *o,
		 oqmlDotContext *dctx, int n, int pn, Data* pdata,
		 Bool *isnull, int nb, int ind, const Attribute *xattr,
		 Size &size)
{
  oqmlDotDesc *d = &dctx->desc[pn];
  oqmlStatus *s;
  Status is;

  size = d->key_len;

  *isnull = False;

  if (d->attrname && (is = oqmlIterPrelim(node, db, ctx, o, d, pdata, xattr)))
    return is;

  if (pn != n)
    return oqmlIterGetValueMiddle(node, db, ctx, o, dctx, n, pn,
				  pdata, isnull, nb, ind, d, xattr, size);

  return oqmlIterGetValueTerminal(node, db, ctx, o, dctx, n, pn,
				  pdata, isnull, nb, ind, d, xattr, size);
}

static oqmlStatus *
oqmlCheckValue(Database *db, oqmlDotDesc *d,
	      oqmlContext *ctx, oqmlDotContext *dctx,
	      const Oid &oid,
	      oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
			     const oqmlAtom *, int, void *),
	      Bool isnull,
	      oqmlAtom *start, oqmlAtom *end,
	      void *user_data, Bool &ok)
{
  oqmlAtomList *al = new oqmlAtomList();
  oqmlStatus *s = dctx->eval(db, ctx, new oqmlAtom_oid(oid), NULL, &al);

  if (s)
    return s;

  if (al)
    {
      oqmlAtom *a = al->first;
      unsigned char *key = (unsigned char *)(d->key->getKey());

      while (a)
	{
	  Data val = 0;
	  Size size = d->key_len;
	  int len;
	  if (a->getData(key, &val, size, len, d->cls))
	    {
	      Data data = (val ? val : key);
	      if ((*cmp)(data, (a->type.type == oqmlATOM_NULL ||
				isnull ? True : False),
			 start, end, d->key_len, user_data))
		{
		  ok = True;
		  return oqmlSuccess;
		}
	    }
	  
	  a = a->next;
	}
    }

  ok = False;
  return oqmlSuccess;
}

#define IDX_USER_CMP

#ifdef IDX_USER_CMP
struct CmpArg {
  oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
		  const oqmlAtom *, int, void *);
  oqmlAtom *start, *end;
  int key_len;
  void *user_data;
  CmpArg(oqmlBool (*_cmp)(Data, Bool, const oqmlAtom *,
			  const oqmlAtom *, int, void *),
	 oqmlAtom *_start, oqmlAtom *_end,
	 int _key_len, void *_user_data) {
    cmp = _cmp;
    start = _start;
    end = _end;
    key_len = _key_len;
    user_data = _user_data;
  }
};

static eyedbsm::Boolean
idx_compare(const void *xkey, void *xarg)
{
  CmpArg *arg = (CmpArg *)xarg;
  unsigned char *key = (unsigned char *)xkey;

  return arg->cmp(key + sizeof(char), ISNULL(key[0]),
		  arg->start, arg->end, arg->key_len, arg->user_data) ?
    eyedbsm::True : eyedbsm::False;
}
#endif

static oqmlStatus *
oqmlMakeIter_idx_comp(oqmlNode *node,
		      Database *db, oqmlContext *ctx, oqmlDotContext *dctx,
		      int n, oqmlAtomList *alist, oqmlAtom *start,
		      oqmlAtom *end,
		      oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
				      const oqmlAtom *, int, void *),
		      int depth,
		      int thread_cnt,
		      void *user_data,
		      eyedbsm::Boolean sexcl, eyedbsm::Boolean eexcl)
{
  oqmlDotDesc *d = &dctx->desc[n];
  int ind;
  Data s_val, e_val;
  Size size;
  int len;
  oqmlStatus *s;
  unsigned char *s_data, *e_data;
  int pn;
  oqmlBool mustCheck;
  Class *cls = (Class *)oqmlIterGetClass(db, dctx, n, pn, mustCheck);

  memset(d->s_data, 0, d->key_len+sizeof(char));

  size = d->key_len;
  if (!start)
    s_data = 0;
  else
    {
      d->make_key(start->getSize());
      if (!start->getData(d->s_data+sizeof(char), &s_val, size, len, cls))
	return oqmlSuccess;
      if (s_val)
	{
	  if (strlen((char *)s_val) >= d->key_len)
	    return oqmlSuccess;
	  strcpy((char *)d->s_data+sizeof(char), (char *)s_val);
	}
      s_data = d->s_data;
      d->s_data[0] = idxISNULL(start);
    }

  memset(d->e_data, 0, d->key_len+sizeof(char));

  size = d->key_len;
  if (!end)
    e_data = 0;
  else
    {
      d->make_key(end->getSize());
      if (!end->getData  (d->e_data+sizeof(char), &e_val, size, len, cls))
	return oqmlSuccess;
      if (e_val)
	{
	  if (strlen((char *)e_val) >= d->key_len)
	    return oqmlSuccess;
	  strcpy((char *)d->e_data+sizeof(char), (char *)e_val);
	}
      e_data = d->e_data;
      d->e_data[0] = idxISNULL(end);
    }

  Bool mustCheckType = (db->getVersionNumber() >= MULTIIDX_VERSION) ?
    False : True;

#ifdef IDX_USER_CMP
  CmpArg arg(cmp, start, end, d->key_len, user_data);
#endif

  for (int nidx = 0; nidx < d->idx_cnt; nidx++)
    {
      eyedbsm::Idx *idx = d->idxse[nidx];

      IDB_LOG(IDB_LOG_IDX_SEARCH,
	      ("[%d] Using Comp Index #%d '%s' between '%s' and '%s' "
	       "[%s check value]%s\n", n,
	       nidx, (d->idxs[nidx] ? d->idxs[nidx]->getAttrpath().c_str() : ""),
	       (start ? start->getString() : "null"),
	       (end ? end->getString() : "null"),
	       (mustCheck ? "must" : "does not"),
	       (!idx ? " SE index is null" : "")));
	       
      if (!idx)
	continue;

      eyedbsm::IdxCursor *curs;

#ifdef IDX_USER_CMP
      if (idx->asHIdx())
	curs = new eyedbsm::HIdxCursor(idx->asHIdx(), s_data, e_data, sexcl, eexcl,
				 idx_compare, &arg, thread_cnt);
      else
	curs = new eyedbsm::BIdxCursor(idx->asBIdx(), s_data, e_data, sexcl, eexcl,
				 idx_compare, &arg);
#else
      if (idx->asHIdx())
	curs = new eyedbsm::HIdxCursor(idx->asHIdx(), s_data, e_data, sexcl, eexcl,
				 0, 0, thread_cnt);
      else
	curs = new eyedbsm::BIdxCursor(idx->asBIdx(), s_data, e_data, sexcl, eexcl);
#endif
  
      int nfound;
      for (nfound = 0; ; nfound++)
	{
	  eyedbsm::Boolean found;
	  Oid oid[2];
	  *((char *)d->key->getKey()) = 0;
	  eyedbsm::Status se;
	  OQML_CHECK_INTR();

	  if ((se = curs->next(&found, oid, d->key)) == eyedbsm::Success)
	    {
	      if (!found)
		break;

	      unsigned char *key = (unsigned char *)(d->key->getKey());

	      IDB_LOG(IDB_LOG_IDX_SEARCH_DETAIL,
		      ("[%d] Key Found %s '%s' (%s) => ", n,
		       oid[0].toString(), key+sizeof(char),
		       ISNULL(key[0]) ? "null data" : "not null data"));

#ifndef IDX_USER_CMP
	      if (!((*cmp)(key + sizeof(char), ISNULL(key[0]),
			   start, end, d->key_len, user_data)))
		{
		  IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL, ("don't match\n"));
		  continue;
		}
#endif

	      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL, ("matches\n"));

	      if (mustCheckType)
		{
		  if (!oqmlCheckType(db, &oid[1], cls))
		    continue;
		}

	      if (pn > 1)
		{
		  oqmlAtom *r = new oqmlAtom_oid(oid[0], cls);
		  s = oqmlMakeIter(node, db, ctx, dctx, pn-1, alist, r, r,
				   equal_oid, depth+1, thread_cnt);
		  if (s)
		    {
		      delete curs;
		      return s;
		    }
		}
	      else if (!depth || pn != n)
		{
		  Bool ok;
		  if (mustCheck)
		    {
		      if (s = oqmlCheckValue(db, d, ctx, dctx, oid[0],
					     cmp, ISNULL(key[0]),
					     start, end, user_data, ok))
			{
			  delete curs;
			  return s;
			}
		    }
		  else
		    ok = True;

		  if (ok)
		    {
		      //printf("INSERTING[%d] %s\n", nfound, oid[0].toString());
		      alist->append(new oqmlAtom_oid(oid[0], cls));
		      OQML_CHECK_MAX_ATOMS(alist, ctx, delete curs);
		    }
		}
	      else
		{
		  alist->append(new oqmlAtom_oid(oid[0], cls));
		  OQML_CHECK_MAX_ATOMS(alist, ctx, delete curs);
		}
	    }
	  else
	    {
	      delete curs;
	      return new oqmlStatus(node, eyedbsm::statusGet(se));
	    }
	}

      if (!nfound)
	IDB_LOG(IDB_LOG_IDX_SEARCH_DETAIL, ("[%d] No Key Found\n", n));

      delete curs;
    }

  return oqmlSuccess;
}

static void
computeOffset(oqmlDotDesc *d, int &offset, int &offset_ind)
{
#ifdef IDB_UNDEF_ENUM_HINT
  if (d->cls->asEnumClass())
    {
      offset = sizeof(char) + sizeof(char);
      offset_ind = sizeof(eyedblib::int32);
      return;
    }
#endif

  if (d->is_coll)
    {
      offset = 0;
      offset_ind = 0;
      return;
    }

  offset = sizeof(char);
  offset_ind = sizeof(eyedblib::int32);
}

static void
promote_atom(oqmlDotDesc *d, oqmlAtom *&x)
{
  if (!d->cls || !x) return;

  if (x->as_double())
    {
      if (d->cls->asInt32Class() || d->cls->asInt16Class() || d->cls->asInt64Class())
	x = new oqmlAtom_int(x->as_double()->d);
      else if (d->cls->asCharClass())
	x = new oqmlAtom_char(x->as_double()->d);

      return;
    }

  if (x->as_int())
    {
      if (d->cls->asFloatClass())
	x = new oqmlAtom_double(x->as_int()->i);
      else if (d->cls->asCharClass())
	x = new oqmlAtom_char(x->as_int()->i);

      return;
    }

  if (x->as_char())
    {
      if (d->cls->asInt32Class() || d->cls->asInt16Class() || d->cls->asInt64Class())
	x = new oqmlAtom_char(x->as_char()->c);
      else if (d->cls->asFloatClass())
	x = new oqmlAtom_double(x->as_char()->c);

      return;
    }
}

static int
oqmlMakeEntry(oqmlDotDesc *d, int ind, oqmlAtom *x, int offset,
	      unsigned char *d_data, unsigned char *&data)
{
  if (!x)
    {
      data = 0;
      return 1;
    }

  if (!x->makeEntry(ind, d_data+offset, d->key_len, d->attr->getClass()))
    return 0;

  d_data[0] = idxISNULL(x);
#ifdef IDB_UNDEF_ENUM_HINT
  if (d->cls->asEnumClass())
    {
      if (idxISNULL(x))
	d_data[1] = 0;
      else
	d_data[1] = 1;
    }
#endif

  data = d_data;
  return 1;
}

static int
oqmlMakeEntries(oqmlDotDesc *d, int ind, oqmlAtom *&start, oqmlAtom *&end,
		int offset, unsigned char *&s_data, unsigned char *&e_data)
{
  promote_atom(d, start);
  promote_atom(d, end);

  if (offset)
    {
      if (!oqmlMakeEntry(d, ind, start, offset, d->s_data, s_data))
	return 0;
      if (!oqmlMakeEntry(d, ind, end, offset, d->e_data, e_data))
	return 0;

      return 1;
    }

  int len;
  Data val;
  Size key_len = d->key_len;

  if (!start)
    s_data = 0;
  else if (!start->getData(d->s_data, &val, key_len, len,
			   d->attr->getClass()))
    return 0;

  if (!end)
    e_data = 0;
  else if (!end->getData(d->e_data, &val, key_len, len,
			 d->attr->getClass()))
    return 0;

  s_data = d->s_data;
  e_data = d->e_data;
  return 1;
}

static oqmlStatus *
oqml_add_coll_atom(oqmlNode *node, oqmlContext *ctx, const char *ident, oqmlAtom *atom)
{
  /*
  printf("\n+++++ MUST ADD %s to identifier %s ++++++\n\n",
	 atom->getString(), ident);
	 */

  oqmlAtom *x;
  if (!ctx->getSymbol(ident, 0, &x) || !x->as_select())
    return new oqmlStatus(node, "internal error: expected select atom for %s",
			  ident);

  if (!x->as_select()->collatom)
    x->as_select()->collatom = new oqmlAtom_list(new oqmlAtomList());

  oqmlAtomList *list = x->as_select()->collatom->list;
  if (!list->isIn(atom))
    list->append(atom->copy());
  return oqmlSuccess;
}

static oqmlStatus *
oqmlMakeIter_idx_item(oqmlNode *node, Database *db, oqmlContext *ctx,
		      oqmlDotContext *dctx, int n,
		      oqmlAtomList *alist, oqmlAtom *start, oqmlAtom *end,
		      oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
				      const oqmlAtom *, int, void *),
		      int depth,
		      int thread_cnt,
		      void *user_data, eyedbsm::Boolean sexcl, eyedbsm::Boolean eexcl)
{
  oqmlDotDesc *d = &dctx->desc[n];
  int ind;
  oqmlStatus *s;
  unsigned char *s_data, *e_data;
  int pn;
  oqmlBool mustCheck;
  Class *cls = (Class *)oqmlIterGetClass(db, dctx, n, pn, mustCheck);
  int s_ind, e_ind;

  s = d->evalInd(node, db, ctx, s_ind, e_ind, oqml_True, oqml_False);
  if (s) return s;
  
  int offset;
  int offset_ind;

  computeOffset(d, offset, offset_ind);

  Bool mustCheckType = (db->getVersionNumber() >= MULTIIDX_VERSION) ?
    False : True;

  /*
  printf("dctx->dot %s requal_ident = %s, is_coll = %d\n",
	 (const char *)dctx->dot->toString(),
	 dctx->dot->requal_ident ? dctx->dot->requal_ident : "no requal ident",
	 d->is_coll);
	 */

  const char *requal_ident = (d->is_coll && dctx->dot->requal_ident) ?
    dctx->dot->requal_ident : 0;

  for (ind = s_ind; ind <= e_ind; ind++)
    {
      OQML_CHECK_INTR();

      if (!oqmlMakeEntries(d, ind, start, end, offset, s_data, e_data))
	continue;

      for (int nidx = 0; nidx < d->idx_cnt; nidx++)
	{
	  eyedbsm::Idx *idx = d->idxse[nidx];

	  IDB_LOG(IDB_LOG_IDX_SEARCH,
		  ("[%d] Using Item Index #%d '%s' between '%s' and '%s' "
		   "[%d, %d] [%s check value]%s\n", n,
		   nidx, (d->idxs[nidx] ? d->idxs[nidx]->getAttrpath().c_str() : ""),
		   (start ? start->getString() : "null"),
		   (end ? end->getString() : "null"),
		   s_ind, e_ind,
		   (mustCheck ? "must" : "does not"),
		   (!idx ? " SE index is null" : "")));
	  
	  if (!idx)
	    continue;

	  eyedbsm::IdxCursor *curs;

	  if (idx->asHIdx())
	    curs = new eyedbsm::HIdxCursor(idx->asHIdx(), s_data, e_data,
				     sexcl, eexcl, 0, 0, thread_cnt);
	  else
	    curs = new eyedbsm::BIdxCursor(idx->asBIdx(), s_data, e_data,
				     sexcl, eexcl);

	  int nfound;
	  for (nfound = 0; ; nfound++)
	    {
	      eyedbsm::Boolean found;
	      Oid oid[2];
	      eyedbsm::Status se;
	      OQML_CHECK_INTR();
	      if ((se = curs->next(&found, oid, d->key)) == eyedbsm::Success)
		{
		  if (!found)
		    break;

		  unsigned char *key = (unsigned char *)(d->key->getKey());
	      
		  IDB_LOG(IDB_LOG_IDX_SEARCH_DETAIL,
			  ("[%d] Key Found %s (%s) => ", n,
			   oid[0].toString(),
			   ISNULL(key[0]) ? "null data" : "not null data"));

		  if (!((*cmp)(key + offset_ind + offset,
			       ISNULL(key[0]), start, end, d->key_len, user_data)))
		    {
		      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
				("%s [index #%d]\n",
				 (d->is_coll ? "not in collection" : "don't match"),
				 ind));
		      continue;
		    }

		  IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL, 
			    ("%s [index #%d]\n", 
			     (d->is_coll ? "is in collection" : "matches"),
			     ind));

		  if (mustCheckType && !oqmlCheckType(db, &oid[1], cls))
		    continue;

		  if (requal_ident)
		    {
		      s = oqml_add_coll_atom(node, ctx, requal_ident, start);
		      if (s) return s;
		    }

		  if (pn > 1)
		    {
		      oqmlAtom *r = new oqmlAtom_oid(oid[0], cls);
		      s = oqmlMakeIter(node, db, ctx, dctx, pn-1, alist, r, r,
				       equal_oid, depth+1, thread_cnt);

		      if (s)
			{
			  delete curs;
			  return s;
			}
		    }
		  else if (!depth || pn != n)
		    {
		      Bool ok;
		      if (mustCheck)
			{
			  if (s = oqmlCheckValue(db, d, ctx, dctx, oid[0],
						 cmp, ISNULL(key[0]),
						 start, end,
						 user_data, ok))
			    {
			      delete curs;
			      return s;
			    }
			}
		      else
			ok = True;

		      if (ok)
			{
			  alist->append(new oqmlAtom_oid(oid[0], cls));
			  OQML_CHECK_MAX_ATOMS(alist, ctx, delete curs);
			}
		    }
		  else
		    {
		      alist->append(new oqmlAtom_oid(oid[0], cls));
		      OQML_CHECK_MAX_ATOMS(alist, ctx, delete curs);
		    }
		}
	      else
		{
		  delete curs;
		  return new oqmlStatus(node, eyedbsm::statusGet(se));
		}
	    }

	  if (!nfound)
	    IDB_LOG(IDB_LOG_IDX_SEARCH_DETAIL, ("[%d] No Key Found\n", n));
	  delete curs;
	}
    }

  return oqmlSuccess;
}

static oqmlStatus *
oqmlCollManage(oqmlNode *node, Database *db, oqmlContext *ctx,
	       oqmlDotDesc *d, Object *o, Collection *&coll)
{
  int ind;
  oqmlStatus *s;
  Status is;

  if (d->array)
    {
      s = d->array->evalCollArray(node, db, ctx, &d->attr->getTypeModifier(),
				  ind);
      if (s) return s;
    }
  else
    ind = 0;

  const Attribute *xattr = o->getClass()->getAttributes()[d->attr->getNum()];
  if (xattr->isIndirect())
    {
      Oid colloid;
      is = xattr->getOid(o, &colloid, 1, ind);
      if (is) return new oqmlStatus(node, is);
      s = oqmlObjectManager::getObject(node, db, &colloid, (Object *&)coll,
				       oqml_True, oqml_False);
      if (s) return s;
    }
  else
    {
      is = xattr->getValue(o, (Data *)&coll, 1, ind);
      if (is) return new oqmlStatus(node, is);
    }

  if (coll && !coll->asCollection())
    return new oqmlStatus(node, "internal query error: "
			  "collection expected, got %s",
			  (coll ? coll->getClass()->getName() : "nil"));

  if (coll)
    IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
	      ("collection %s at index #%d%s ",
	       coll->asCollection()->getOidC().toString(), ind,
	       coll->asCollection()->getOidC().isValid() ? " =>" : ""));
  else
    IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
	      ("collection NULL at index #%d ", ind));

  return oqmlSuccess;
}

#define SCHECK(S) \
if (S) \
{ \
    if (o) oqmlObjectManager::releaseObject(o); \
    return S; \
}
  
//  if (omode == Exception::ExceptionMode) throw *IS; \

#define ICHECK(IS) \
if (IS) \
{ \
  if (o) oqmlObjectManager::releaseObject(o); \
  Exception::setMode(omode); \
  if (omode == Exception::ExceptionMode) Exception::make(IDB_OQL_ERROR, (IS)->getDesc()); \
  return new oqmlStatus(IS); \
}

static oqmlStatus *
oqmlMakeIter_noidx(oqmlNode *node,
		   Database *db, oqmlContext *ctx, oqmlDotContext *dctx, int n,
		   oqmlAtomList *alist, oqmlAtom *start, oqmlAtom *end,
		   oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
				   const oqmlAtom *, int, void *),
		   int depth,
		   int thread_cnt,
		   void *user_data, eyedbsm::Boolean sexcl, eyedbsm::Boolean eexcl)
{
  oqmlDotDesc *d = &dctx->desc[n];
  int ind;
  Status is;
  oqmlStatus *s;
  int pn;
  oqmlBool mustCheck;
  Class *cls = (Class *)oqmlIterGetClass(db, dctx, n, pn, mustCheck);
  oqmlDotDesc *dx = &dctx->desc[pn];

  Iterator q((Class *)cls, True);
  Oid cl_oid = cls->getOid();

  if (is = q.getStatus())
    return new oqmlStatus(node, is);

  Oid oid;
  Bool found;
      
  int s_ind, e_ind;
  s = d->evalInd(node, db, ctx, s_ind, e_ind, oqml_True, oqml_False);
  if (s) return s;

  IDB_LOG(IDB_LOG_IDX_SEARCH,
	  ("[%d] Using No Index between '%s' and '%s' [%d, %d]\n", n,
	   (start ? start->getString() : "null"),
	   (end ? end->getString() : "null"),
	   s_ind, e_ind));
  
  const char *requal_ident = (d->is_coll && dctx->dot->requal_ident) ?
    dctx->dot->requal_ident : 0;

  Exception::Mode omode = 
    Exception::setMode(Exception::StatusMode);

  for (;;)
    {
      OQML_CHECK_INTR();
      is = q.scanNext(&found, &oid);

      Object *o = 0;
      ICHECK(is);

      if (!found)
	break;

      IDB_LOG(IDB_LOG_IDX_SEARCH_DETAIL,
	      ("[%d] Oid Found %s => ", n, oid.toString()));
      
      s = oqmlObjectManager::getObject(node, db, oid, o, oqml_False);

      if (s) continue;

      if (dctx->ident_mode)
	{
	  const char *name;
	  if (dctx->iscoll)
	    name = ((Collection *)o)->getName();
	  else
	    name = ((Class *)o)->getName();

	  if ((*cmp)((Data)name, False, start, end, strlen(name),
		     user_data))
	    alist->append(new oqmlAtom_oid(oid, cls));

	  oqmlObjectManager::releaseObject(o);
	  continue;
	}

      Collection *coll = 0;
      if (d->is_coll)
	{
	  s = oqmlCollManage(node, db, ctx, d, o, coll);
	  SCHECK(s);
	  if (!coll || !coll->getOidC().isValid())
	    {
	      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL, ("\n"));
	      continue;
	    }
	}

      int nn = 0;
      for (ind = s_ind; ind <= e_ind; ind++)
	{
	  OQML_CHECK_INTR();
	  int nb;
	  Bool isnull;
	  Data data = 0;
	  is = Success;

	  delete dx->icurs;
	  dx->icurs = 0;

	  int pp;
	  for (pp = 0; ; nn++, pp++)
	    {
	      OQML_CHECK_INTR();
	      const Attribute *xattr = 0;
	      Size size;

	      if (d->mode == Attribute::composedMode)
		{
		  data = 0;
		  is = oqmlIterGetValue(node, db, ctx, o, dctx, n, pn,
					&data,
					&isnull, Attribute::directAccess,
					ind, xattr, size);

		  if (is && is->getStatus() ==
		      IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR)
		    {
		      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
			("%s%s [index #%d] <out of range>\n",
			 AND(nn),
			 (d->is_coll ? "not in collection": "don't match"),
			 ind));
		      nn++;
		      if (!dx->icurs)
			break;
		      continue;
		    }

		  ICHECK(is);

		  if (data)
		    nb = strlen((char *)data) + 1;
		  else
		    nb = 0;
		}
	      else
		{
		  data = d->e_data;
		  is = oqmlIterGetValue(node, db, ctx, o, dctx, n, pn,
					&data, &isnull, 1, ind, xattr,
					size);

		  if (is && is->getStatus() ==
		      IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR)
		    {
		      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
			("%s%s [index #%d] <out of range>\n",
			 AND(nn),
			 (d->is_coll ? "not in collection": "don't match"),
			 ind));
		      nn++;
		      if (!dx->icurs)
			break;
		      continue;
		    }

		  ICHECK(is);
		  nb = size;
		}
		  
	      Bool isin = False;
	      if (d->is_coll && start)
		{
		  Collection::ItemId where;
		  is = coll->isIn(OQML_ATOM_OIDVAL(start), isin, &where);
		  ICHECK(is);
		  if (requal_ident)
		    {
		      s = oqml_add_coll_atom(node, ctx, requal_ident, start);
		      if (s) return s;
		    }
		}

	      if (isin || (*cmp)(data, isnull, start, end, nb, user_data))
		{
		  IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
			    ("%s%s [index #%d]\n", AND(nn),
			     (d->is_coll ? "is in collection" : "matches"),
			     ind));
		  if (pn > 1)
		    {
		      oqmlAtom *r = new oqmlAtom_oid(oid, cls);
		      s = oqmlMakeIter(node, db, ctx, dctx, pn-1, alist, r, r,
				       equal_oid, depth+1, thread_cnt);
		      SCHECK(s);
		    }
		  else
		    {
		      alist->append(new oqmlAtom_oid(oid, cls));
		      OQML_CHECK_MAX_ATOMS(alist, ctx, 0);
		    }
		}
	      else
		IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL,
			  ("%s%s [index #%d]\n",
			   AND(nn),
			   (d->is_coll ? "not in collection": "don't match"),
			   ind));

	      if (!dx->icurs)
		break;
	    }

	  if (is && is->getStatus() == IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR)
	    {
	      if (!pp)
		break;
	      is = Success;
	    }

	  ICHECK(is);
	}

      IDB_LOG_X(IDB_LOG_IDX_SEARCH_DETAIL, ("\n"));
      oqmlObjectManager::releaseObject(o);
    }

  Exception::setMode(omode);
  return oqmlSuccess;
}

static oqmlStatus *
oqmlMakeIter(oqmlNode *node,
	     Database *db, oqmlContext *ctx, oqmlDotContext *dctx, int n,
	     oqmlAtomList *alist, oqmlAtom *start, oqmlAtom *end,
	     oqmlBool (*cmp)(Data, Bool, const oqmlAtom *,
			     const oqmlAtom *, int, void *),
	     int depth,
	     int thread_cnt,
	     void *user_data,
	     eyedbsm::Boolean sexcl, eyedbsm::Boolean eexcl)
{
  if (n < 0)
    return new oqmlStatus(node, "invalid query construct");

  oqmlDotDesc *d = &dctx->desc[n];

  //if (n == dctx->count - 1) {
  if (!depth)
    thread_cnt = getThreadCount(ctx);;

  if (d->idx_cnt)
    {
      if (d->mode == Attribute::composedMode)
	return oqmlMakeIter_idx_comp(node, db, ctx, dctx, n, alist, start, end,
				     cmp, depth, thread_cnt,
				     user_data, sexcl, eexcl);
      else
	return oqmlMakeIter_idx_item(node, db, ctx, dctx, n, alist, start, end,
				     cmp, depth, thread_cnt,
				     user_data, sexcl, eexcl);

    }
  else
    return oqmlMakeIter_noidx(node, db, ctx, dctx, n, alist, start, end,
			      cmp, depth, thread_cnt,
			      user_data, sexcl, eexcl);
}

oqmlStatus *
oqmlIndexIter(Database *db, oqmlContext *ctx, oqmlNode *node,
	      oqmlDotContext *dctx, oqmlDotDesc *d,
	      oqmlAtomList **alist)
{
  const Class *cls = dctx->dot_type.cls ? dctx->dot_type.cls :
    d->attr->getClass();
  oqmlATOMTYPE atom_type = dctx->dot_type.type;

  int offset = sizeof(char);

  if (d->mode != Attribute::composedMode) {
    if (cls->asCollectionClass() && !d->attr->isIndirect())
      offset = 0;
    else {
      offset += sizeof(eyedblib::int32);
#ifdef IDB_UNDEF_ENUM_HINT
      if (cls->asEnumClass())
	offset += sizeof(char);
#endif
    }
  }

  //int thread_cnt = getThreadCount(ctx);
  int thread_cnt = 0;
  for (int n = 0; n < d->idx_cnt; n++)
    {
      eyedbsm::Idx *idx = d->idxse[n];

      IDB_LOG(IDB_LOG_IDX_SEARCH,
	      ("Using Index #%d '%s' for full search%s\n",
	       n, (d->idxs[n] ? d->idxs[n]->getAttrpath().c_str() : ""),
	       (!idx ? " SE index is null" : "")));

      if (!idx)
	continue;

      eyedbsm::IdxCursor *curs;

      if (idx->asHIdx())
	curs = new eyedbsm::HIdxCursor(idx->asHIdx(), 0, 0, eyedbsm::False, eyedbsm::False, 0,
				 0, thread_cnt);
      else
	curs = new eyedbsm::BIdxCursor(idx->asBIdx(), 0, 0, eyedbsm::False, eyedbsm::False);

      for (;;)
	{
	  eyedbsm::Boolean found;
	  Oid oid[2];
	  *((char *)d->key->getKey()) = 0;
	  eyedbsm::Status se;
	  OQML_CHECK_INTR();

	  if ((se = curs->next(&found, oid, d->key)) == eyedbsm::Success)
	    {
	      if (!found)
		break;
	      unsigned char *key = (unsigned char *)(d->key->getKey());
	      if (offset && ISNULL(key[0]))
		(*alist)->append(new oqmlAtom_null());
	      else
		(*alist)->append(oqmlAtom::make_atom(key+offset,
						     atom_type, cls));

	      OQML_CHECK_MAX_ATOMS(*alist, ctx, delete curs);
	    }
	  else
	    {
	      delete curs;
	      return new oqmlStatus(node, eyedbsm::statusGet(se));
	    }
	}

      delete curs;
    }

  return oqmlSuccess;
}

// EQUAL
oqmlEqualIterator::oqmlEqualIterator(Database *_db, oqmlDotContext *_dctx,
				   oqmlAtom *_start, oqmlAtom *_end, void *_ud) :
				   oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool equal_op(Data data, Bool isnull, const oqmlAtom *start,
			 const oqmlAtom *end, int len_data, void *)
{
  if (!start) return oqml_False;
  return start->compare(data, len_data, isnull, oqmlEQUAL);
}

oqmlStatus *
oqmlIterator::begin(oqmlContext *ctx)
{
  s_dctx = dot_ctx;
  s_and_ctx = 0;

  //  if (dot_ctx && dot_ctx->count == 0)
  // changed the 22/04/99
  if (dot_ctx && dot_ctx->count == 0 && dot_ctx->tctx)
    {
      oqmlDotContext *dctx;

      if (!(dctx = dot_ctx->tctx))
	return new oqmlStatus("fatal error in iterator");

      s_and_ctx = ctx->getAndContext();

      ctx->setAndContext(oqmlAtomList::andOids(dot_ctx->tlist, s_and_ctx));

      dot_ctx = dctx;

      return oqmlSuccess;
    }

  return oqmlSuccess;
}
  
void
oqmlIterator::commit(oqmlContext *ctx)
{
  ctx->setAndContext(s_and_ctx);
  dot_ctx = s_dctx;
}

oqmlStatus *
oqmlEqualIterator::eval(oqmlNode *node, oqmlContext *ctx,
			oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al, equal_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start,
		   end, equal_op, 0);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlEqual::makeIterator(Database *db, oqmlDotContext *dctx,
				    oqmlAtom *atom)
{
  delete iter;
  iter = new oqmlEqualIterator(db, dctx, atom, atom);
  return oqmlSuccess;
}

// DIFF
oqmlDiffIterator::oqmlDiffIterator(Database *_db, oqmlDotContext *_dctx,
				   oqmlAtom *_start, oqmlAtom *_end, void *_ud) :
				   oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool diff_op(Data data, Bool isnull, const oqmlAtom *,
			const oqmlAtom *, int len_data, void *ud)
{
  return ((oqmlAtom *)ud)->compare(data, len_data, isnull, oqmlDIFF);
}

oqmlStatus *
oqmlDiffIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al, diff_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist,
		   start, end,
		   diff_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlDiff::makeIterator(Database *db, oqmlDotContext *dctx,
				  oqmlAtom *atom)
{
  delete iter;
  iter = new oqmlDiffIterator(db, dctx, 0, 0, atom);
  return oqmlSuccess;
}

// INF
oqmlInfIterator::oqmlInfIterator(Database *_db, oqmlDotContext *_dctx,
			       oqmlAtom *_start, oqmlAtom *_end, void *_ud) :
			       oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool sup_op(Data data, Bool isnull, const oqmlAtom *start,
		      const oqmlAtom *end, int len_data, void *);

static oqmlBool inf_op(Data data, Bool isnull, const oqmlAtom *start,
		      const oqmlAtom *end, int len_data, void *)
{
  if (!end) return oqml_False;

  return OQMLBOOL(end->compare(data, len_data, isnull, oqmlINF) &&
		 (!start || start->compare(data, len_data, isnull, oqmlSUPEQ)));
}

oqmlStatus *
oqmlInfIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  assert(node->getType() == oqmlINF || node->getType() == oqmlSUP);
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al, 
		  (node->getType() == oqmlINF ? inf_op : sup_op), rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1,
		   rlist, start, end,
		   (node->getType() == oqmlINF ? inf_op : sup_op),
		   0, 0, 0, eyedbsm::False, eyedbsm::True);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlInf::makeIterator(Database *db, oqmlDotContext *dctx,
				oqmlAtom *atom)
{
  delete iter;

  if (type == oqmlINF)
    iter = new oqmlInfIterator(db, dctx, 0, atom);
  else
    iter = new oqmlSupIterator(db, dctx, atom, 0);

  return oqmlSuccess;
}

// INFEQ
oqmlInfEqIterator::oqmlInfEqIterator(Database *_db, oqmlDotContext *_dctx,
			       oqmlAtom *_start, oqmlAtom *_end, void *_ud) :
			       oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool supeq_op(Data data, Bool isnull, const oqmlAtom *start,
		      const oqmlAtom *end, int len_data, void *);
static oqmlBool infeq_op(Data data, Bool isnull, const oqmlAtom *start,
		      const oqmlAtom *end, int len_data, void *)
{
  if (!end) return oqml_False;

  return OQMLBOOL(end->compare(data, len_data, isnull, oqmlINFEQ) &&
		 (!start || start->compare(data, len_data, isnull, oqmlSUPEQ)));
}

oqmlStatus *
oqmlInfEqIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  assert(node->getType() == oqmlINFEQ || node->getType() == oqmlSUPEQ);
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al) {
    s = evalAnd(node, ctx, al,
		(node->getType() == oqmlINFEQ ? infeq_op : supeq_op),
		rlist);
    commit(ctx);
    return s;
  }
  
  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1,
		   rlist, start, end,
		   (node->getType() == oqmlINFEQ ? infeq_op : supeq_op), 0);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlInfEq::makeIterator(Database *db, oqmlDotContext *dctx,
				oqmlAtom *atom)
{
  delete iter;
  if (type == oqmlINFEQ)
    iter = new oqmlInfEqIterator(db, dctx, 0, atom);
  else
    iter = new oqmlSupEqIterator(db, dctx, atom, 0);

  return oqmlSuccess;
}

// SUP
oqmlSupIterator::oqmlSupIterator(Database *_db, oqmlDotContext *_dctx,
			       oqmlAtom *_start, oqmlAtom *_end, void *_ud) :
			       oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool sup_op(Data data, Bool isnull, const oqmlAtom *start,
		      const oqmlAtom *end, int len_data, void *)
{
  if (!start) return oqml_False;

  return OQMLBOOL(start->compare(data, len_data, isnull, oqmlSUP) &&
		 (!end || end->compare(data, len_data, isnull, oqmlINFEQ)));
}

oqmlStatus *
oqmlSupIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  assert(node->getType() == oqmlSUP || node->getType() == oqmlINF);
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al,
		  (node->getType() == oqmlSUP ? sup_op : inf_op), rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1,
		   rlist, start, end,
		   (node->getType() == oqmlSUP ? sup_op : inf_op), 0, 0, 0,
		   eyedbsm::True, eyedbsm::False);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlSup::makeIterator(Database *db, oqmlDotContext *dctx,
				oqmlAtom *atom)
{
  delete iter;

  if (type == oqmlSUP)
    iter = new oqmlSupIterator(db, dctx, atom, 0);
  else
    iter = new oqmlInfIterator(db, dctx, 0, atom);
  
  return oqmlSuccess;
}

// SUPEQ
oqmlSupEqIterator::oqmlSupEqIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end,
				     void *_ud) :
  oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

#define XSTROPT

struct xString {
  xString(const char *_data, bool capstr = true) {
#ifdef XSTROPT
    if (strlen(_data) >= sizeof sdata)
      data = new char[strlen(_data)+1];
    else
      data = sdata;
#else
    data = new char[strlen(_data)+1];
#endif

    if (capstr)
      capstring(data, _data);
    else
      strcpy(data, _data);
  }

  ~xString() {
#ifdef XSTROPT
    if (data != sdata)
#endif
      delete [] data;
  }

  static void capstring(char *dest, const char *src) {
    char c;
    while (c = *src++)
      *dest++ = (c >= 'a' && c <= 'z') ? c + 'A' - 'a' : c;
    *dest = 0;
  }

#ifdef XSTROPT
  char sdata[128];
#endif
  char *data;
};

static oqmlBool supeq_op(Data data, Bool isnull, const oqmlAtom *start,
			 const oqmlAtom *end, int len_data, void *capstr)
{
  if (!start) return oqml_False;

  if (capstr) {
    xString s((const char *)data);
    return OQMLBOOL(start->compare((Data)s.data, len_data, isnull,
				   oqmlSUPEQ) &&
		    (!end || end->compare((Data)s.data, len_data, isnull,
					  oqmlINFEQ)));
  }
  return OQMLBOOL(start->compare(data, len_data, isnull, oqmlSUPEQ) &&
		  (!end || end->compare(data, len_data, isnull, oqmlINFEQ)));
}

oqmlStatus *
oqmlSupEqIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  /*
  assert(node->getType() == oqmlSUPEQ || node->getType() == oqmlINFEQ ||
	 node->getType() == oqmlREGCMP || node->getType() == oqmlREGICMP);
  */
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al) {
    s = evalAnd(node, ctx, al,
		(node->getType() == oqmlSUPEQ ||
		 node->getType() == oqmlREGCMP ? supeq_op : infeq_op),
		rlist);
    commit(ctx);
    return s;
  }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1,
		   rlist, start, end,
		   (node->getType() == oqmlSUPEQ ||
		    node->getType() == oqmlREGCMP ? supeq_op : infeq_op),
		   0, 0, user_data, eyedbsm::False, eyedbsm::True);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlSupEq::makeIterator(Database *db, oqmlDotContext *dctx,
				oqmlAtom *atom)
{
  delete iter;

  if (type == oqmlSUPEQ)
    iter = new oqmlSupEqIterator(db, dctx, atom, 0);
  else
    iter = new oqmlInfEqIterator(db, dctx, 0, atom);

  return oqmlSuccess;
}

// SubStr
oqmlSubStrIterator::oqmlSubStrIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end,
				     void *_ud) :
  oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool substr_op(Data data, Bool isnull, const oqmlAtom *,
			  const oqmlAtom *, int len_data, void *udata)
{
  if (isnull) return oqml_False;

  const char *str = OQML_ATOM_STRVAL((oqmlAtom *)udata);

  if (*str == CI) {
    xString s((const char *)data);
    return OQMLBOOL(strstr((const char *)s.data, str+1));
  }

  return OQMLBOOL(strstr((const char *)data, str+1));
}

oqmlStatus *
oqmlSubStrIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al) {
    s = evalAnd(node, ctx, al, substr_op, rlist);
    commit(ctx);
    return s;
  }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1,
		   rlist, start, end, substr_op,
		   0, 0, user_data, eyedbsm::False, eyedbsm::True);
  commit(ctx);
  return s;
}

// BETWEEN
oqmlBetweenIterator::oqmlBetweenIterator(Database *_db,
					 oqmlDotContext *_dctx,
					 oqmlAtom *_start, oqmlAtom *_end,
					 void *_ud) :
  oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool between_op(Data data, Bool isnull, const oqmlAtom *start,
			   const oqmlAtom *end, int len_data, void *ud)
{
  oqmlAtom *atom = (oqmlAtom *)ud;
  assert(atom->as_range());

  if (!start) return oqml_False;
  return OQMLBOOL(start->compare(data, len_data, isnull,
				 atom->as_range()->from_incl ? oqmlSUPEQ : oqmlSUP) &&
		  end->compare(data, len_data, isnull,
			       atom->as_range()->to_incl ? oqmlINFEQ : oqmlINF));
}

oqmlStatus *
oqmlBetweenIterator::eval(oqmlNode *node, oqmlContext *ctx,
			  oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al, between_op, rlist);
      commit(ctx);
      return s;
    }
  
  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start,
		   end, between_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlBetween::makeIterator(Database *db, oqmlDotContext *dctx,
				      oqmlAtom *atom)
{
  assert(atom->as_range());

  delete iter;
  iter = new oqmlBetweenIterator(db, dctx, atom->as_range()->from,
				 atom->as_range()->to, atom);
  return oqmlSuccess;
}


// NOT BETWEEN
oqmlNotBetweenIterator::oqmlNotBetweenIterator(Database *_db,
					 oqmlDotContext *_dctx,
					 oqmlAtom *_start, oqmlAtom *_end,
					 void *_ud) :
  oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool not_between_op(Data data, Bool isnull,
			       const oqmlAtom *,
			       const oqmlAtom *, int len_data, void *ud)
{
  oqmlAtom *atom = (oqmlAtom *)ud;
  assert(atom->as_range());

  oqmlAtom *start = atom->as_range()->from;
  oqmlAtom *end = atom->as_range()->to;

  return OQMLBOOL(start->compare(data, len_data, isnull,
				 atom->as_range()->from_incl ? oqmlINF : oqmlINFEQ) ||
		  end->compare(data, len_data, isnull,
			       atom->as_range()->to_incl ? oqmlSUP : oqmlSUPEQ));
}

oqmlStatus *
oqmlNotBetweenIterator::eval(oqmlNode *node, oqmlContext *ctx,
			  oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();

  if (al)
    {
      s = evalAnd(node, ctx, al, not_between_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist,
		   start, end,
		   not_between_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlNotBetween::makeIterator(Database *db, oqmlDotContext *dctx,
					 oqmlAtom *atom)
{
  delete iter;
  iter = new oqmlNotBetweenIterator(db, dctx, 0, 0, atom);
  return oqmlSuccess;
}


// REGCMP
oqmlRegCmpIterator::oqmlRegCmpIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end, void *_ud) : oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool regcmp_op(Data data, Bool isnull, const oqmlAtom *start,
			 const oqmlAtom *end, int len_data, void *re)
{
#ifdef USE_LIBGEN
  return OQMLBOOL(regex((char *)re, (char *)data) != 0);
#endif
  return OQMLBOOL( regexec( (regex_t *)re, (char *)data, 0, (regmatch_t *)0, 0) == 0 );
}

static Bool
meta_chars_in(const char *s)
{
  char c;

  while (c = *s++)
    if (c == '[' || c == ']' || c == '*' || c == '.' || c == '(' ||
	c == ')' || c == '\\' || c == '$')
      return True;

  return False;
}

oqmlIterator *
oqmlRegex::makeIter(Database *db, oqmlDotContext *dctx,
		    oqmlAtom *atom, bool capstr)
{
  const char *s = OQML_ATOM_STRVAL(atom);

  if (!meta_chars_in(s)) {
    if (*s == '^') {
      const char *start = &s[1];
      char *end = strdup(start);
      ++end[strlen(end)-1];

      xString xstart(start, capstr);
      xString xend(end, capstr);

      oqmlIterator *i = new oqmlSupEqIterator(db, dctx,
					      new oqmlAtom_string(xstart.data),
					      new oqmlAtom_string(xend.data),
					      (void *)capstr);
      free(end);
      return i;
    }

    xString xs(s, capstr);
    return new oqmlSubStrIterator(db, dctx, 0, 0,
				  new oqmlAtom_string
				  ((str_convert(capstr ? CI : CS) + xs.data).c_str()));
  }

  if (capstr)
    return new oqmlRegICmpIterator(db, dctx, 0, 0, re);

  return new oqmlRegCmpIterator(db, dctx, 0, 0, re);
}

oqmlStatus *
oqmlRegCmpIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al)
    {
      s = evalAnd(node, ctx, al, regcmp_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start, end,
		  (start ? supeq_op : regcmp_op), 0, 0, user_data,
		  eyedbsm::False, (start ? eyedbsm::True : eyedbsm::False));
  commit(ctx);
  return s;
}

oqmlStatus *oqmlRegCmp::makeIterator(Database *db, oqmlDotContext *dctx,
				     oqmlAtom *atom)
{
  delete iter;
  iter = makeIter(db, dctx, atom, false);
  return oqmlSuccess;
}

// REGICMP
oqmlRegICmpIterator::oqmlRegICmpIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end, void *_ud) : oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool
regicmp_op(Data xdata, Bool isnull, const oqmlAtom *start,
	   const oqmlAtom *end, int len_data, void *re)
{
  xString s((const char *)xdata);

#ifdef USE_LIBGEN
  return OQMLBOOL(regex((char *)re, s.data) != 0);
#endif
  return OQMLBOOL( regexec( (regex_t *)re, s.data, 0, (regmatch_t *)0, 0) == 0);
}

oqmlStatus *
oqmlRegICmpIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al)
    {
      s = evalAnd(node, ctx, al, regicmp_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start, end,
		  regicmp_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlRegICmp::makeIterator(Database *db, oqmlDotContext *dctx,
				      oqmlAtom *atom)
{
  delete iter;
  iter = makeIter(db, dctx, atom, true);
  return oqmlSuccess;
}

// REGDIFF
oqmlRegDiffIterator::oqmlRegDiffIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end, void *_ud) : oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool regdiff_op(Data data, Bool isnull, const oqmlAtom *start,
			 const oqmlAtom *end, int len_data, void *re)
{
#ifdef USE_LIBGEN
  return OQMLBOOL(regex((char *)re, (char *)data) == 0);
#endif
  return OQMLBOOL( regexec( (regex_t *)re, (char *)data, 0, (regmatch_t *)0, 0) != 0);
}

oqmlStatus *
oqmlRegDiffIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al)
    {
      s = evalAnd(node, ctx, al, regdiff_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start, end,
		  regdiff_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlRegDiff::makeIterator(Database *db, oqmlDotContext *dctx,
				   oqmlAtom *atom)
{
  delete iter;
  iter = new oqmlRegDiffIterator(db, dctx, 0, 0, re);
  return oqmlSuccess;
}

// REGIDiff
oqmlRegIDiffIterator::oqmlRegIDiffIterator(Database *_db, oqmlDotContext *_dctx,
				     oqmlAtom *_start, oqmlAtom *_end, void *_ud) : oqmlIterator(_db, _dctx, _start, _end, _ud)
{
}

static oqmlBool
regidiff_op(Data xdata, Bool isnull, const oqmlAtom *start,
	    const oqmlAtom *end, int len_data, void *re)
{
  xString s((const char *)xdata);

#ifdef USE_LIBGEN
  return OQMLBOOL(regex((char *)re, s.data) == 0);
#endif
  return OQMLBOOL( regexec( (regex_t *)re, s.data, 0, (regmatch_t *)0, 0) != 0);
}

oqmlStatus *
oqmlRegIDiffIterator::eval(oqmlNode *node, oqmlContext *ctx, oqmlAtomList **xalist)
{
  OQML_MAKE_RBAG(xalist, rlist);

  oqmlStatus *s = begin(ctx);

  if (s)
    return s;

  oqmlAtomList *al = ctx->getAndContext();
  if (al)
    {
      s = evalAnd(node, ctx, al, regidiff_op, rlist);
      commit(ctx);
      return s;
    }

  s = oqmlMakeIter(node, db, ctx, dot_ctx, dot_ctx->count - 1, rlist, start, end,
		  regidiff_op, 0, 0, user_data);
  commit(ctx);
  return s;
}

oqmlStatus *oqmlRegIDiff::makeIterator(Database *db, oqmlDotContext *dctx,
				     oqmlAtom *atom)
{
  delete iter;
  iter = new oqmlRegIDiffIterator(db, dctx, 0, 0, re);
  return oqmlSuccess;
}

// utils

oqmlIterator::oqmlIterator(Database *_db,
			   oqmlDotContext *_dctx, oqmlAtom *_start,
			   oqmlAtom *_end, void *_ud)
{
  db = _db;
  dot_ctx = _dctx;

  start = (_start ? _start->copy() : 0);
  end   = (_end   ? _end->copy()   : 0);
  user_data = _ud;
}

oqmlStatus *
oqmlIterator::getValue(oqmlNode *node,
		       oqmlContext *ctx, const Oid *oid, Data buff,
		       Data &data, int &len, Bool &isnull)
{
  oqmlAtomList *alist;
  Object *o;
  oqmlStatus *s;

  isnull = False;
  s = oqmlObjectManager::getObject(node, db, oid, o, oqml_False);
  if (s) return s;

  alist = new oqmlAtomList();
  s = dot_ctx->eval_perform(db, ctx, o, 0, 1, &alist);

  oqmlObjectManager::releaseObject(o);

  if (s)
    return s;

  if (alist->cnt != 1)
    {
      len = 0;
      data = buff;
      memset(buff, 0, 16);
      return oqmlSuccess;
    }

  oqmlAtom *value = alist->first;
  if (value->as_null())
    {
      isnull = True;
      return oqmlSuccess;
    }

  Data val;
  Size size;
  
  size = 16;

  if (value->getData(buff, &val, size, len))
    data = (val ? val : buff);

  len = size;
  return oqmlSuccess;
}

oqmlStatus *
oqmlIterator::evalAndRealize(oqmlNode *node, oqmlContext *ctx, 
			     oqmlAtom *a,
			     oqmlBool (*cmp)(Data, Bool,
					     const oqmlAtom *,
					     const oqmlAtom *,
					     int, void *),
			     oqmlAtomList *alist)
{
  oqmlStatus *s;

  if (OQML_IS_COLL(a))
    {
      oqmlAtom *x = OQML_ATOM_COLLVAL(a)->first;
      while (x)
	{
	  s = evalAndRealize(node, ctx, x, cmp, alist); 
	  if (s) return s;
	  x = x->next;
	}

      return oqmlSuccess;
    }
  
  if (a->type.type != oqmlATOM_OID)
    return new oqmlStatus(node, "oid expected, got %s",
			  a->type.getString());

  unsigned char buff[16];
  int len_data;
  Data data;
  Oid oid = ((oqmlAtom_oid *)a)->oid;
  Bool isnull;

  s = getValue(node, ctx, &oid, buff, data, len_data, isnull);

  if (s) return s;

  if ((*cmp)(data, isnull, start, end, len_data, user_data))
    alist->append(new oqmlAtom_oid(oid));

  return oqmlSuccess;
}

oqmlStatus *
oqmlIterator::evalAnd(oqmlNode *node,
		      oqmlContext *ctx, oqmlAtomList *al,
		      oqmlBool (*cmp)(Data, Bool, const oqmlAtom *, const oqmlAtom *,
				      int, void *), oqmlAtomList *alist)
{
  return evalAndRealize(node, ctx, al->first, cmp, alist);
  /*
  oqmlStatus *s;
  oqmlAtom *a = al->first;

  while (a)
    {
      oqmlAtom *next = a->next;

      s = evalAndRealize(node, ctx, a, cmp, alist);
      if (s) return s;

      a = next;
    }

  return oqmlSuccess;
  */
}

oqmlStatus *oqmlComp::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  s = compCompile(db, ctx, opstr, qleft, qright, this, &cst_atom, &eval_type);

  if (s)
    return s;

  if (isConstant())
    return eval(db, ctx, &cst_list);

  return oqmlSuccess;
}

oqmlStatus *oqmlComp::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  if (cst_list)
    {
      *alist = new oqmlAtomList(cst_list);
      return oqmlSuccess;
    }

  oqmlStatus *s;

  /*
  printf("evaluating oqmlComp => %s context='%s %s %s' [iterator %d]\n",
	 (const char *)toString(),
	 ctx->isSelectContext() ? "select" : "not select",
	 ctx->isPrevalContext() ? "preval" : "not preval",
	 ctx->isWhereContext() ? "where" : "not where",
	 getIterator());
	 */

  s = compEval(db, ctx, opstr, qleft, qright, alist, this, cst_atom);

  if (s)
    return s;

  return oqmlSuccess;
}

void oqmlComp::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool   oqmlComp::isConstant() const
{
  return OQMLBOOL(qleft->isConstant() && qright->isConstant());
}

oqmlBool oqmlComp::appearsMoreOftenThan(oqmlComp *ql) const
{
  oqmlDotContext *dot_ctx1 = 0, *dot_ctx2 = 0;

  if (qleft->asDot())
    dot_ctx1 = qleft->asDot()->getDotContext();

  if (ql->qleft->asDot())
    dot_ctx2 = ql->qleft->asDot()->getDotContext();

  if (dot_ctx1 && dot_ctx2)
    {
      oqmlBool isidx1 = OQMLBOOL(dot_ctx1->desc[dot_ctx1->count - 1].idx_cnt);
      oqmlBool isidx2 = OQMLBOOL(dot_ctx2->desc[dot_ctx1->count - 1].idx_cnt);

      if (isidx1 && !isidx2)
	return oqml_False;
      if (!isidx1 && isidx2)
	return oqml_True;

      if (type == oqmlEQUAL)
	return oqml_False;

      if (ql->getType() == oqmlEQUAL)
	return oqml_True;
    }

  return oqml_False;
}

oqmlStatus *oqmlRegex::compile(Database *db, oqmlContext *ctx)
{
  return compCompile(db, ctx, opstr, qleft, qright, this, &cst_atom,
		     &eval_type);
}

oqmlStatus *
oqmlRegex::eval_realize(oqmlAtom *a, oqmlAtomList **alist)
{
  char *str;
  oqmlBool isnull;

  if (a->type.type == oqmlATOM_NULL)
    isnull = oqml_True;
  else if (a->type.type == oqmlATOM_STRING) {
    str = strdup(OQML_ATOM_STRVAL(a));
    isnull = oqml_False;
  }
  else
    return oqmlStatus::expected(this, "string", a->type.getString());


  if (!isnull && (type == oqmlREGICMP || type == oqmlREGIDIFF))
    oqml_capstring((char *)str);

  int r;

  if (isnull)
    r = 0;
  else {
#ifdef USE_LIBGEN
    r = (regex((char *)re, str) != 0);
#endif
    r = (regexec( re, (char *)str, 0, (regmatch_t *)0, 0) == 0);
    }

  if (type == oqmlREGDIFF || type == oqmlREGIDIFF)
    r = !r;

  *alist = new oqmlAtomList(new oqmlAtom_bool(OQMLBOOL(r)));

  if (!isnull) free(str);

  return oqmlSuccess;
}

oqmlStatus *
oqmlRegex::complete(Database *db, oqmlContext *ctx, oqmlAtom *atom)
{
  cst_atom = atom;

  if (cst_atom->type.type != oqmlATOM_STRING)
    return new oqmlStatus(this, "invalid operand type %s.", opstr,
			  cst_atom->type.getString());

  int r;

#ifdef USE_LIBGEN
  free(re);
  if (type == oqmlREGCMP || type == oqmlREGDIFF)
    re = regcmp(OQML_ATOM_STRVAL(cst_atom), (char *)0);
  else {
    char *buf = strdup(OQML_ATOM_STRVAL(cst_atom));
    oqml_capstring(buf);
    re = regcmp(buf, (char *)0);
    free(buf);
  }

  r = (re == NULL);
#endif
  re = (regex_t *)malloc( sizeof( regex_t));
  assert( re != 0);

  if (type == oqmlREGCMP || type == oqmlREGDIFF)
    r = regcomp( re, OQML_ATOM_STRVAL(cst_atom), 0);
  else {
    char *buf = strdup(OQML_ATOM_STRVAL(cst_atom));
    oqml_capstring(buf);
    r = regcomp( re, buf, 0);
    free(buf);
  }

  if (r)
    return new oqmlStatus(this, "invalid regular expression '%s'.",
			 opstr, OQML_ATOM_STRVAL(cst_atom));

  return oqmlSuccess;
}

oqmlStatus *oqmlRegex::eval(Database *db, oqmlContext *ctx,
			    oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al_left;

  /*
  printf("evaluating oqmlRegex => %s context='%s %s %s' [iterator %d]\n",
	 (const char *)toString(),
	 ctx->isSelectContext() ? "select" : "not select",
	 ctx->isPrevalContext() ? "preval" : "not preval",
	 ctx->isWhereContext() ? "where" : "not where",
	 getIterator());
	 */

  oqmlAtomList *al;
  s = qright->eval(db, ctx, &al);

  if (s)
    return s;

  if (al->cnt != 1)
    return new oqmlStatus(this, "invalid operand.", opstr);

  cst_atom = al->first;
  s = complete(db, ctx, cst_atom);
  if (s) return s;

  if (!ctx->isSelectContext())
    {
      if (evalDone)
	{
	  *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
	  return oqmlSuccess;
	}

      if (needReinit)
	{
	  s = reinit(db, ctx);
	  if (s) return s;
	  needReinit = oqml_False;
	}

      s = qleft->eval(db, ctx, &al_left);
      if (s) return s;
      return eval_realize(al_left->first, alist);
    }

  s = qleft->eval(db, ctx, &al_left, this, cst_atom);

  if (s)
    return s;

  if (iter)
    {
      s = iter->eval(this, ctx, alist);
      if (s) return s;

      if (ctx->isOverMaxAtoms())
	return oqmlSuccess;

      evalDone = oqml_True;

      if (qleft->asDot())
	{
	  s = qleft->asDot()->populate(db, ctx, *alist, oqml_False);
	  if (s) return s;
	}

      return oqmlSuccess;
    }

  return eval_realize(al_left->first, alist);
}
}
