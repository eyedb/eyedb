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
#include "Attribute_p.h"

// flag added the 28/02/00
#define NEW_IDX_CHOICE

// flag added the 28/02/00
#define NEW_SELECT

#define OBJ_SUPPORT

namespace eyedb {

void stop_oql() { }

#define OPTIM_TVALUE
//#define SUPPORT_CSTRING

static inline oqmlBool
oqml_is_subclass(const Class *cls1, const Class *cls2)
{
  if (!cls1 || !cls2)
    return oqml_False;

  Bool is;
  cls1->isSubClassOf(cls2, &is);
  return OQMLBOOL(is);
}

#define DBG_LEVEL 0
//#define SKIP_ATTR_ERR

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// private utility methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Status
oqml_check_vardim(Database *db, const Attribute *attr, Oid *oid,
		  oqmlBool& enough, int from, int &nb)
{
  enough = oqml_True;

  Size size;
  Status is = attr->getSize(db, oid, size);
      
  if (is)
    return is;

  if (from + nb > size)
    enough = oqml_False;

  return Success;
}

Status
oqml_check_vardim(const Attribute *attr, Object *o, oqmlBool set,
		  oqmlBool& enough, int from, int &nb, int pdims,
		  oqmlBool isComplete)
{
  if (nb < 0)
    return Exception::make(IDB_ERROR, "cannot set to NULL");

  enough = oqml_True;

  if (attr->isVarDim())
    {
      Size size;
      Status is = attr->getSize((Agregat *)o, size);

      if (is)
	return is;

      int wanted = from + nb;
      if (set)
	{
	  if ((isComplete && wanted != size * pdims) ||
	      (!isComplete && wanted > size * pdims)) {
	    is = attr->setSize((Agregat *)o, ((wanted-1) / pdims) + 1);
	    if (is)
	      return is;
	  }
	}
      else if (wanted > size * pdims)
	enough = oqml_False;
    }

  return Success;
}

static oqmlStatus *
oqml_getclass(oqmlNode *node, Database *db, oqmlContext *ctx,
	      oqmlAtom *a, Class **cls, const char *varname = 0)
{
  if (a && OQML_IS_OBJECT(a))
    {
#ifdef OPTIM_TVALUE
      if (OQML_IS_OBJ(a))
	{
	  OQL_CHECK_OBJ(a);
	  if (!OQML_ATOM_OBJVAL(a))
	    return new oqmlStatus(node, "invalid null object");
	  *cls = (Class *)OQML_ATOM_OBJVAL(a)->getClass();
	  return oqmlSuccess;
	}

      if (!OQML_ATOM_OIDVAL(a).isValid() && ctx->isWhereContext()) {
	*cls = 0;
	return oqmlSuccess;
      }

      /*
      // 9/07/06
      if (!OQML_ATOM_OIDVAL(a).isValid()) {
	*cls = 0;
	return oqmlSuccess;
      }
      */

      Status is = db->getObjectClass(OQML_ATOM_OIDVAL(a), *cls);
      if (is)
	return new oqmlStatus(node, is);
      return oqmlSuccess;
#else
      Object *o;
      oqmlStatus *s = oqmlObjectManager::getObject(node, db, a, o, oqml_True);
      if (s) return s;
      if (!o)
	return new oqmlStatus(node, "invalid null object");
      *cls = o->getClass();
      return oqmlSuccess;
#endif
    }

  if (a && OQML_IS_STRUCT(a))
    {
      *cls = 0;
      return oqmlSuccess;
    }
 
  // added 28/02/00
  if ((OQML_IS_NULL(a) || a->as_nil()) && ctx->isWhereContext()) {
    *cls = 0;
    return oqmlSuccess;
  }

  /*
  // 9/07/06
  if (OQML_IS_NULL(a) || a->as_nil()) {
    *cls = 0;
    return oqmlSuccess;
  }
  */

  if (a)
    {
      if (varname)
	return new oqmlStatus(node, "value of '%s': oid expected, got %s",
			      varname, a->type.getString());
      return oqmlStatus::expected(node, "oid", a->type.getString());
    }

  if (varname)
    return new oqmlStatus(node, "value of '%s': oid expected",
			  varname);

  return new oqmlStatus(node, "oid expected");
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlDotDesc method
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlStatus *
oqmlDotDesc::evalInd(oqmlNode *node, Database *db, oqmlContext *ctx,
		     int &s_ind, int &e_ind, oqmlBool nocheck,
		     oqmlBool ignoreColl)
{
  if (array)
    {
      oqmlStatus *s;
#if 0
      if (is_coll && ignoreColl)
	{
	  int ind;
	  s = array->evalCollArray(node, db, ctx, mod, ind);
	  s_ind = e_ind = ind;
	  printf("Ignoring Coll => ind = %d\n", s_ind);
	}
#endif

      if (is_coll && !ignoreColl)
	{
	  int ind;
	  s = array->evalCollArray(node, db, ctx, mod, ind);
	  s_ind = e_ind = ind;
	  //printf("NOT Ignoring Coll => ind = %d\n", s_ind);
	  return s;
	}

      s = array->eval(node, db, ctx, attr->getClassOwner()->getName(),
		      attrname, mod, &s_ind, &e_ind, nocheck);
      //printf("Not Call s_ind = %d, e_ind = %d\n", s_ind, e_ind);
      return s;
    }

  s_ind = 0;
  e_ind = 0;

  return oqmlSuccess;
}

void
oqmlDotDesc::make_key(int size)
{
  if (size > key_len)
    {
      free(s_data);
      s_data = (unsigned char *)malloc(size);
      free(e_data);
      e_data = (unsigned char *)malloc(size);

      key_len = size;
    }
}

oqmlDotDesc::oqmlDotDesc()
{
  key = 0;
  attrname = 0;
  s_data = 0;
  e_data = 0;
  icurs = 0;
  idxs = 0;
  idxse = 0;
  qlmth = 0;
}

oqmlDotDesc::~oqmlDotDesc()
{
  // DISCONNECTED the 3/07/01
  // if (qlmth) qlmth->unlock();

  delete key;
  free(attrname);
  free(s_data);
  free(e_data);
  delete icurs;

  // added 2/09/05 ...and disconnected
  /*
  for (int n = 0; n < idx_cnt; n++) {
    delete idxs[n];
    delete idxse[n];
  }
  */

  delete[] idxs;
  delete[] idxse;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlDotContext methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlDotContext::oqmlDotContext(oqmlDot *p, const char *n)
{
  init(p);
  varname = n;
}

oqmlDotContext::oqmlDotContext(oqmlDot *p, const Class *cls)
{
  init(p);
  count = 1;
  desc[0].cls = cls;
  desc[0].cls_orig = cls;
  desc[0].isref = True;
}

oqmlDotContext::oqmlDotContext(oqmlDot *p, oqmlAtom *curatom)
{
  init(p);
  count = 1;
  desc[0].curatom = curatom;
}

oqmlDotContext::oqmlDotContext(oqmlDot *p, oqmlNode *_oqml)
{
  init(p);
  oqml = _oqml;
}

void oqmlDotContext::init(oqmlDot *d)
{
  dot = d;
  count = 0;
  oqml = 0;
  tctx = 0;
  tlist = 0;
  varname = 0;
  ident_mode = False;
  iscoll = False;
  dot_type.type = oqmlATOM_UNKNOWN_TYPE;
  dot_type.cls = 0;
  dot_type.comp = oqml_False;

  desc = idbNewVect(oqmlDotDesc, oqmlMAXDESC);
}

void oqmlDotContext::setIdentMode(Bool _iscoll)
{
  ident_mode = True;
  iscoll = _iscoll;
}

oqmlStatus *
oqmlDotContext::setAttrName(Database *db,
			    const char *_attrname)
{
  const char *attrname;
  const Class *cls;
  oqmlStatus *s = dot->isScope(db, _attrname, attrname, cls);
  if (s) return s;

  oqmlDotDesc *d = &desc[count];

  if (attrname)
    {
      d->attrname = strdup(attrname);
      assert(count > 0);
      assert(cls);
      desc[count-1].cls_orig = desc[count-1].cls = cls;
    }
  else
    d->attrname = (_attrname ? strdup(_attrname) : 0);

  return oqmlSuccess;
}

static const Class *
get_rootclass(oqmlDotContext *dctx)
{
  for (int k = dctx->count-1; k >= 0; k--)
    if (dctx->desc[k].isref)
      return dctx->desc[k].cls;

  //  assert(0);
  return 0;
}

static void
make_idx_ctx(oqmlDotContext *dctx, AttrIdxContext &idx_ctx)
{
  int j;
  for (j = dctx->count; j >= 2; j--)
    if (dctx->desc[j-1].isref)
      break;

  for (int i = j; i <= dctx->count; i++) {
    idx_ctx.push(dctx->desc[i].attrname);
  }
}

oqmlStatus *
oqmlDotDesc::getIdx(Database *db, oqmlContext *ctx, oqmlDot *dot)
{  
  idx_cnt = 0;

  const Class *rootcls = get_rootclass(dctx);

  if (!rootcls)
    return oqmlSuccess;

  unsigned int subclass_cnt;
  Class **subclasses;
  Status status;
  int maxind;
  Size size;

#if DBG_LEVEL > 0
  printf("oqmlDotDesc::getMultiIdx(%s, count = %d) {\n", rootcls->getName(),
	 dctx->count);
#endif
  status = rootcls->getSubClasses(subclasses, subclass_cnt);
  if (status) return new oqmlStatus(dot, status);

  for (int j = 0; j < subclass_cnt; j++)
    {
      AttrIdxContext idx_ctx(subclasses[j]);
      make_idx_ctx(dctx, idx_ctx);

#if DBG_LEVEL > 0
      printf("\tsubclass #%d = %s [%s] (attr.name = %s, count=%d, name=%s)\n",
	     j, subclasses[j]->getName(),
	     (const char *)idx_ctx.getString(), attr->getName(),
	     dctx->count, dctx->desc[1].attrname);
#endif

      Index *idx = 0;
      eyedbsm::Idx *se_idx = 0;
      status = attr->getIdx(db, mode, maxind, size, idx_ctx, idx, se_idx);
      if (status)
	return new oqmlStatus(dot, status);

      if (!idx && !se_idx)
	continue;

      if (!idx_cnt)
	{
	  idxs = new Index*[subclass_cnt];
	  idxse = new eyedbsm::Idx*[subclass_cnt];
	}

      //printf("idxs[%d] = %p idxse[%d] = %p\n", idx_cnt, idx, idx_cnt, se_idx);
      idxs[idx_cnt] = idx;
      idxse[idx_cnt] = se_idx;
      idx_cnt++;
    }

  //printf("subclass_cnt %d %d\n", subclass_cnt, idx_cnt);

  if (idx_cnt && idx_cnt != subclass_cnt) {
    oqmlAtom *a;
    static const char varname[] = "oql$subtree$index$force";
    if (ctx->getSymbol(varname, 0, &a)) {
      if (a && OQML_IS_BOOL(a) && OQML_ATOM_BOOLVAL(a)) {
	//printf("does not use index in this case\n");
	idx_cnt = 0;
	return oqmlSuccess;
      }
    }

    std::string errmsg = std::string("The inheritance subtree ") +
      rootcls->getName() + ".." + attr->getName() +
      " is not index consistent:";
    std::string missing, existing;
    std::string comma = ", ";
    for (int j = 0; j < subclass_cnt; j++) {
      AttrIdxContext idx_ctx(subclasses[j]);
      make_idx_ctx(dctx, idx_ctx);
      
      Index *idx = 0;
      eyedbsm::Idx *se_idx = 0;
      status = attr->getIdx(db, mode, maxind, size, idx_ctx, idx, se_idx);
      if (status)
	return new oqmlStatus(dot, status);
      if (!idx) {
	if (missing.size()) missing += comma;
	missing += std::string(idx_ctx.getAttrName());
      } else {
	if (existing.size()) existing += comma;
	existing += std::string(idx_ctx.getAttrName());
      }
    }

    errmsg += std::string("\nExisting index: ") + existing;
    errmsg += std::string("\nMissing index: ") + missing;
    errmsg += std::string("\nCreate the missing index or set the oql variable '")
      + varname + "' to true";
    return new oqmlStatus(dot, errmsg.c_str());
  }

#if DBG_LEVEL > 0
  printf("}\n");
#endif
  return oqmlSuccess;
}

oqmlStatus *
oqmlDotDesc::make(Database *db, oqmlContext *ctx, oqmlDot *dot,
		  const Attribute *xattr,
		  oqml_ArrayList *_array, const char *_attrname,
		  const Class *castcls)
{
  assert(!attr);

  if (!xattr)
    return new oqmlStatus(dot, "unknown attribute '%s'", _attrname);

  attr = xattr;
  mod = &attr->getTypeModifier();

  oqmlStatus *s = dctx->setAttrName(db, _attrname);
  if (s)
    return s;

  if (oqml_is_getcount(_array)) {
    mode = 0;
    isref = False;
    cls_orig = cls = Int32_Class;
    key_len = sizeof(eyedblib::int32);
    s_data = (unsigned char *)malloc(sizeof(eyedblib::int32));
    e_data = (unsigned char *)malloc(sizeof(eyedblib::int32));
    array = _array;
    return oqmlSuccess;
  }

  array = _array;

  int ndims = mod->ndims;
  int array_ndims = (array ? array->count : 0);

  if ((array_ndims > ndims) || (ndims - array_ndims) > 1) {
    if (attr->getClass()->asCollectionClass() &&
	((array_ndims <= ndims+1) && (ndims+1 - array_ndims) <= 1)) {
      if (s = array->checkCollArray(dot, attr->getClass(),
				    attr->getName()))
	return s;

      is_coll = oqml_True;
      stop_oql();
    }
    else if (!ndims)
      return new oqmlStatus(dot, "attribute '%s' is not an array",
			    attr->getName());
    else
      return new oqmlStatus(dot, "array attribute '%s': "
			    "maximum dimension allowed is %d <got %d>",
			    attr->getName(), ndims, array_ndims);
  }
  
  Size sz_comp, inisize;
  if (is_coll) {
    eyedblib::int16 dim, item_size;
    cls = attr->getClass()->asCollectionClass()->getCollClass
      (&isref, &dim, &item_size);
    cls_orig = (castcls ? castcls : attr->getClass());
    mode = 0;
    sz_item = item_size*dim;
    sz_comp = 0;
  }
  else {
    isref = attr->isIndirect();

    // MIND: test un peu fragile!!
    if (ndims - array_ndims == 1)
      mode = Attribute::composedMode;
    else
      mode = 0;

    cls_orig = cls = (castcls ? castcls : attr->getClass());

    // added the 27/11/00, because in some cases db field was not
    // set in cls/cls_orig
    if (cls && !cls->getDatabase() && db)
      cls_orig = cls = db->getSchema()->getClass(cls->getName());

    // added the 6/12/99
    // suppressed the 20/12/99
    /*
      if (dctx->count > 1)
      dctx->desc[dctx->count-1].cls_orig = 
      dctx->desc[dctx->count-1].cls = attr->getClassOwner();
    */
    // ...

    Size _sz_item;
    Offset off;
    attr->getPersistentIDR(off, _sz_item, sz_comp, inisize);
      
    sz_comp -= inisize;
    sz_item = _sz_item;
  }
      
  // skip object collections for indexation
  if (!(is_coll && attr->isIndirect())) {
    s = getIdx(db, ctx, dot);
    if (s) return s;
  }

  if (attr->isVarDim() && !idx_cnt &&
      (mode == Attribute::composedMode || !cls->asBasicClass()))
    make_key(32);
  else {
    if (attr->isVarDim() && mode == Attribute::composedMode)
      sz_comp = sz_item = 0x200;
    if (mode == Attribute::composedMode)
      key_len = sz_comp;
    else {
      key_len = sz_item;
      // added 6/10/01: in fact, key_len must be at least the size
      // of a pointer in case of a direct non basic attribute
      if (!attr->isIndirect() && !attr->isBasicOrEnum() &&
	  key_len < sizeof(void *))
	key_len = sizeof(void *);
    }
	  
    if (idx_cnt)
      key = new eyedbsm::Idx::Key(sizeof(char) + sizeof(eyedblib::int32) +
				  key_len);
	  
    if (idx_cnt && mode != Attribute::composedMode) {
      s_data = (unsigned char *)
	malloc(sizeof(char) + sizeof(eyedblib::int32) + key_len);
      e_data = (unsigned char *)
	malloc(sizeof(char) + sizeof(eyedblib::int32) + key_len);
    }
    else {
      s_data = (unsigned char *)malloc(sizeof(char) + key_len);
      e_data = (unsigned char *)malloc(sizeof(char) + key_len);
    }
  }

  return oqmlSuccess;
}

oqmlStatus *
oqmlDotContext::add(Database *db, oqmlContext *ctx,
		    const Attribute *attr,
		    oqml_ArrayList *_array,
		    char *attrname, oqmlAtom *curatom, Class *castcls,
		    oqmlNode *qlmth)
{
  if (!attr && !qlmth & !attrname)
    return oqmlSuccess;

  oqmlDotDesc *d = &desc[count];
  d->dctx = this;

  if (qlmth)
    {
      d->qlmth = qlmth;
      d->isref = True; // !!THIS is not shure!!, in fact we do known
      // the return type of the method!!
      d->key_len = 256; // in case of this buffer is not large enough,
      // an error is thrown at runtime
      d->s_data = (unsigned char *)malloc(d->key_len);
      d->e_data = (unsigned char *)malloc(d->key_len);
      count++;
      return oqmlSuccess;
    }

  if (attr)
    {
      oqmlStatus *s = d->make(db, ctx, dot, attr, _array, attrname, castcls);
      if (s) return s;
      count++;
      /*
      if (!strcmp(attr->getClass()->getName(), "ostring"))
	return add(db, 0, 0, "s", 0, 0, 0);
	*/
      return oqmlSuccess;
    }

  if (attrname)
    {
      oqmlStatus *s = setAttrName(db, attrname);
      if (s) return s;
      d->curatom = curatom;
      count++;
      return oqmlSuccess;
    }

  return oqmlSuccess;
}

static const char not_found_fmt[] = "attribute '%s' not found in class '%s'.";

oqmlStatus *
oqmlDotContext::eval_middle(Database *db, oqmlContext *ctx, Object *o, 
			    oqmlAtom *value, int n, oqmlAtomList **alist)
{
  oqmlDotDesc *d = &desc[n];
  Status is;
  oqmlStatus *s;
  int s_ind, e_ind, ind;

  s = d->evalInd(dot, db, ctx, s_ind, e_ind, (value ? oqml_False : oqml_True),
		 oqml_False);
  if (s) return s;
  
  const Attribute *xattr;
  Object *mtho1;
  if (d->qlmth) {
    oqmlAtom *rmth;
    oqmlAtomList *al = new oqmlAtomList();
    oqmlStatus *s = ((oqmlMethodCall *)d->qlmth)->
      perform(db, ctx, o, o->getOid(), o->getClass(), &al);
    if (s) return s;
    xattr = 0;

    s = oqmlObjectManager::getObject(dot, db, al->first, mtho1, oqml_True);
    if (s) return s;
  }
  else {
    xattr = o->getClass()->getAttribute(d->attrname);

    if (!xattr)
      {
	if (n > 0 && oqml_is_subclass(desc[n-1].cls, o->getClass()))
	  return oqmlSuccess;
	return new oqmlStatus(dot, not_found_fmt,
			      d->attrname, o->getClass()->getName());
      }
  }

  Data data = d->e_data;

  for (ind = s_ind; ind <= e_ind; ind++) {
    OQML_CHECK_INTR();
    Object *o1 = 0;
      
    if (d->qlmth)
      o1 = mtho1;
    else if (d->isref) {
      is = xattr->getValue((Agregat *)o, (Data *)data,
			   1, ind);

      if (is)
	return oqmlSuccess;
	  
      mcp(&o1, data, sizeof(Object *));
      if (!o1) {
	is = xattr->getOid((Agregat *)o, (Oid *)data,
			   1, ind);
	if (is)
	  return oqmlSuccess;

	s = oqmlObjectManager::getObject(dot, db, (Oid *)data, o1,
					 oqml_True, oqml_False);
	if (s) return s;
	if (!o1) return oqmlSuccess;
      }
    }
    else if (oqml_is_getcount(d->array))
      return new oqmlStatus(dot, "invalid indirection");
    else if (xattr->isBasicOrEnum())
      return new oqmlStatus(dot, "attribute '%s' in class '%s' is not an "
			    "agregat",
			    xattr->getName(),
			    xattr->getClassOwner()->getName());
    else {
      oqmlBool enough;
      int nb = 1;
      is = oqml_check_vardim(xattr, o, OQMLBOOL(value), enough, ind, nb,
			     xattr->getTypeModifier().pdims,
			     oqml_False); // 23/05/02: changed from OQMLCOMPLETE(value) because non sense in eval_middle

#if DBG_LEVEL > 2
      printf("oqmlDotContext::eval_middle(ind = %d, enough = %d, %p)\n",
	     ind, enough, is);
#endif

      if (is)
	return new oqmlStatus(dot, is);

      if (!enough) {
	if (value)
	  continue;
	else
	  break;
      }

      is = xattr->getValue((Agregat *)o, (Data *)data, 1, ind);

      if (is)
	return new oqmlStatus(dot, is);

      memcpy(&o1, data, sizeof(Object *));
      if (o1)
	o1->setDatabase(db);
    }
      
    oqmlStatus *s;
    if (d->is_coll) {
      if (!o1->asCollection())
	return new oqmlStatus(dot, "collection expected, got %s",
			      o1->getClass()->getName());

      if (o1->asCollArray()) {
	assert(d->array);
	oqmlAtomList *al;

	s = oqmlArray::evalMake(db, ctx, o1, d->array, oqml_False, &al);
	if (s) return s;
	oqmlAtom *x = al->first;
	if (!x)
	  continue;

	if (OQML_IS_COLL(x))
	  x = OQML_ATOM_COLLVAL(x)->first;

	while (x)	{
	  Object *coll;
	  s = oqmlObjectManager::getObject(dot, db, x, coll,
					   oqml_False, oqml_False);
	  if (s) return s;
	  s = eval_perform(db, ctx, coll, value, n+1, alist);
	  oqmlObjectManager::releaseObject(coll, oqml_False);
	  if (s) return s;
	  x = x->next;
	}

	continue;
      }

      ObjectArray obj_arr;
      is = o1->asCollection()->getElements(obj_arr);
      if (is)
	return new oqmlStatus(dot, is);

      for (int i = 0; i < obj_arr.getCount(); i++)  {
	s = eval_perform(db, ctx, const_cast<Object *>(obj_arr[i]), value, n+1, alist);
	if (s) return s;
      }

      continue;
    }

    s = eval_perform(db, ctx, o1, value, n+1, alist);

#if DBG_LEVEL > 2
    printf("returning from eval perform -> count = %d\n",
	   (*alist)->cnt);
#endif

    if (s) return s;

    if (value && !d->isref) {
      oqmlBool enough;
      int nb = 1;
      is = oqml_check_vardim(xattr, o, OQMLBOOL(value), enough, ind, nb,
			     xattr->getTypeModifier().pdims,
			     oqml_False); // 23/05/02: changed from OQMLCOMPLETE(value) because non sense in eval_middle

      if (is) return new oqmlStatus(dot, is);

      if (!enough) {
	if (value)
	  continue;
	else
	  break;
      }

      is = xattr->setValue((Agregat *)o, (Data)&o1, 1, ind);
      if (is) return new oqmlStatus(dot, is);
    }
  }

  return oqmlSuccess;
}

static oqmlStatus *
make_contents(oqmlDotDesc *d, oqmlNode *dot, Database *db, oqmlContext *ctx,
	      oqmlAtom *x, oqmlAtomList *alist)
{
  oqmlStatus *s;

  Object *coll;
  if (OQML_IS_OBJ(x)) {
    OQL_CHECK_OBJ(x);
    coll = OQML_ATOM_OBJVAL(x);
  }
  else if (s = oqmlObjectManager::getObject(dot, db, x, coll, oqml_True,
					    oqml_True))
    return s;

  if (!coll->asCollection())
    return new oqmlStatus(dot, "%s: collection expected, got %s",
			  coll->getOid().toString(),
			  coll->getClass()->getName());

  if (coll->asCollArray()) {
    oqmlAtomList *al;
    s = oqmlArray::evalMake(db, ctx, coll, d->array, oqml_True, &al);
    if (s) return s;
    alist->append(al);
    return oqmlSuccess;
  }

  OidArray oid_arr;
  Status is = coll->asCollection()->getElements(oid_arr);
  if (is)
    return new oqmlStatus(dot, is);

  for (int i = 0; i < oid_arr.getCount(); i++)
    alist->append(new oqmlAtom_oid(oid_arr[i]));

  return oqmlSuccess;
}

oqmlStatus *
oqmlDotContext::eval_terminal(Database *db, oqmlContext *ctx, Object *o, 
			      oqmlAtom *value, int n, oqmlAtomList **alist)
{
  oqmlDotDesc *d = &desc[n];
  Status is = Success;
  oqmlStatus *s;
  int s_ind, e_ind, ind;

  if (d->qlmth)
    return ((oqmlMethodCall *)d->qlmth)->perform(db, ctx,
						 o, o->getOid(), o->getClass(),
						 alist);
  const Attribute *xattr;

  xattr = o->getClass()->getAttribute(d->attrname);

  if (!xattr) {
    if (n > 0 && oqml_is_subclass(desc[n-1].cls, o->getClass()))
      return oqmlSuccess;
    return new oqmlStatus(dot, not_found_fmt,
			  d->attrname, o->getClass()->getName());
  }

  if (!d->attr) {
    s = d->make(db, ctx, dot, xattr, d->array, d->attrname, 0);
    if (s) return s;
    s = dot->check(db, this);
    if (s) return s;
  }

  int nb;

  d->mod = &xattr->getTypeModifier();

  if (d->mode == Attribute::composedMode)
    nb = d->mod->dims[d->mod->ndims-1];
  else
    nb = 1;

  s = d->evalInd(dot, db, ctx, s_ind, e_ind, (value ? oqml_False : oqml_True),
		 (value ? oqml_True : oqml_False));
  if (s) return s;

  Data data = d->e_data;

  oqmlATOMTYPE atom_type = dot_type.type;
  for (ind = s_ind; ind <= e_ind; ind++) {
    OQML_CHECK_INTR();
    memset(data, 0, d->key_len);
    Bool isnull = False;

    if (value)
      {
	Data val;
	unsigned char buff[16];
	Size size;
	int len;
	  
#if 1
	if (value->as_oid() &&
	    !(d->isref || xattr->getClass()->asOidClass() ||
	      xattr->getClass()->asCollectionClass()))
	  return new oqmlStatus(dot, "cannot assign an oid to a "
				"literal attribute");
#endif

	size = sizeof(buff);
	if (value->getData(buff, &val, size, len, d->cls)) {
	  if (val) {
	    if (nb > 0 && strlen((char *)val) >= nb)
	      return new oqmlStatus(dot, "out of bound character array"
				    "'%s': "
				    "maximum allowed is %d <got %d>",
				    val, nb, strlen((char *)val));
	    else {
	      int len = strlen((char *)val);
	      if (len >= d->key_len) {
		d->make_key(len+1);
		data = d->e_data;
	      }

	      strcpy((char *)data, (char *)val);
	    }

	    if (nb < 0)
	      nb = strlen((char *)data) + 1;
	  }
	  else
	    data = buff;
	      
	  oqmlBool enough;
	  is = oqml_check_vardim(xattr, o, OQMLBOOL(value), enough, ind,
				 nb, xattr->getTypeModifier().pdims,
				 OQMLCOMPLETE(value));

	  if (is)
	    return new oqmlStatus(dot, is);

	  if (!enough) {
	    if (value)
	      continue;
	    else
	      break;
	  }

	  if (d->isref && !d->is_coll) {
	    if (!OQML_IS_OID(value)) {
	      is = xattr->setValue((Agregat *)o, data, nb, ind);
	      // added the 17/05/01
	      if (OQML_IS_NULL(value) && !is) {
		Oid xoid;
		is = xattr->setOid((Agregat *)o, (Oid *)&xoid,
				   nb, ind);
	      }
	      atom_type = oqmlATOM_OBJ;
	    }
	    else
	      is = xattr->setOid((Agregat *)o, (Oid *)data, nb, ind);
	  }
	  else if (oqml_is_getcount(d->array)) {
	    if (!xattr->isVarDim())
	      return new oqmlStatus(dot, "cannot set dimension "
				    "on a non variable dimension "
				    "attribute");

	    if (oqml_is_wholecount(d->array))
	      return new oqmlStatus(dot,
				    "cannot set multiple dimension");
	      
	      
	    if (!OQML_IS_INT(value))
	      return oqmlStatus::expected(dot, "integer",
					  value->type.getString());

	    Size size = OQML_ATOM_INTVAL(value);
	    Status is = xattr->setSize(o, size);
	    if (is)
	      return new oqmlStatus(dot, is);
	    // test added the 28/12/00 for non persistent object
	    if (o->getOid().isValid()) {
	      is = o->realize();
	      if (is) return new oqmlStatus(dot, is);
	    }
	    (*alist)->append(new oqmlAtom_int(size));
	    continue;
	  }
	  else if (d->is_coll) {
	    Collection *coll = 0;
	    if (xattr->isIndirect()) {
	      Oid coll_oid;
	      is = xattr->getOid((Agregat *)o, &coll_oid, 1, 0);
	      if (!is)
		is = db->loadObject(coll_oid, (Object *&)coll);
	    }
	    else
	      is = xattr->getValue((Agregat *)o, (Data *)&coll, 1, 0);

	    if (!is) {
	      if (!coll->asCollArray())
		return new oqmlStatus(dot,
				      "array expected, got %s",
				      coll->getClass()->getName());
		      
	      if (value->as_null() ||
		  !OQML_ATOM_OIDVAL(value).isValid())
		is = coll->asCollArray()->suppressAt(ind);
	      else
		is = coll->asCollArray()->insertAt
		  (ind, OQML_ATOM_OIDVAL(value));

	      if (!is && xattr->isIndirect()) {
		// test added the 28/12/00 for non persistent object
		if (coll->getOid().isValid())
		  is = coll->realize();
	      }
	    }

	    if (xattr->isIndirect() && coll)
	      coll->release();
	  }
	  else {
	    if (OQML_IS_NULL(value)) {
	      /*
	      if (xattr->getClass()->asCollectionClass()) {
		Oid null_oid;
		is = xattr->setOid((Agregat *)o, (Oid *)&null_oid, nb, ind);
	      }
	      else
	      */
		return new oqmlStatus(dot, "cannot set to NULL");
	    }
	    else if (value->as_oid()) {
	      Object *vo = 0;
	      Oid voi = OQML_ATOM_OIDVAL(value);
	      s = oqmlObjectManager::getObject(dot, db, &voi, vo, oqml_True, oqml_True);
	      if (s)
		return s;
	      is = xattr->setValue((Agregat *)o, (Data)&vo, nb, ind);
	      atom_type = oqmlATOM_OID;
	    }
	    else
	      is = xattr->setValue((Agregat *)o, data, nb, ind);
	  }
	      
	  if (is)
	    return new oqmlStatus(dot, is);

	  if (desc[n-1].isref) {
	    // test added the 28/12/00 for non persistent object
	    if (o->getOid().isValid()) {
	      is = o->realize();

	      if (is)
		return new oqmlStatus(dot, is);
	    }
	  }
	}
      }
    else if (oqml_is_getcount(d->array)) {
      Size size;
      if (xattr->isVarDim()) {
	Status is = xattr->getSize(o, size);
	if (is)
	  return new oqmlStatus(dot, is);
      }
      else if (!d->mod->ndims)
	size = 0;
      else
	size = d->mod->dims[0];

      if (oqml_is_wholecount(d->array)) {
	oqmlAtomList *l = new oqmlAtomList();
	l->append(new oqmlAtom_int(size));
	for (int i = 1; i < d->mod->ndims; i++)
	  l->append(new oqmlAtom_int(d->mod->dims[i]));

	(*alist)->append(new oqmlAtom_list(l));
      }
      else
	(*alist)->append(new oqmlAtom_int(size));
    }
    else {
      if (d->isref) {
	is = xattr->getValue((Agregat *)o, (Data *)data, nb, ind,
			     &isnull);
	if (!is) {
	  Object *tmp;
	  mcp(&tmp, data, sizeof(Object *));
	  if (tmp)
	    atom_type = oqmlATOM_OBJ;
	  else
	    is = xattr->getOid((Agregat *)o, (Oid *)data,
			       nb, ind);
	}
      }
      else if (xattr->isVarDim() && d->mode == Attribute::composedMode)
	is = xattr->getValue((Agregat *)o, (Data *)&data,
			     Attribute::directAccess, ind, &isnull);
      else
	is = xattr->getValue((Agregat *)o, (Data *)data, nb, ind,
			     &isnull);

      if (is) {
	if (xattr->isVarDim()) {
	  if (value)
	    continue;
	  else
	    break;
	}
      }

      if (is)
	return new oqmlStatus(dot, is);
    }
      
    // && ... added the 16/2/00
    if (!is && !oqml_is_getcount(d->array)) {
      if (d->is_coll && !value) {
	if (!isnull) {
	  s = make_contents(d, dot, db, ctx,
			    oqmlAtom::make_atom
			    (data, atom_type,
			     (dot_type.cls ? dot_type.cls :
			      xattr->getClass())),
			    *alist);
	  if (s) return s;
	}
      }
      else if (isnull)
	(*alist)->append(new oqmlAtom_null);
      else
	(*alist)->append(oqmlAtom::make_atom(data, atom_type,
					     (dot_type.cls ? dot_type.cls :
					      xattr->getClass())));
    }
  }
  
  return oqmlSuccess;
}

#ifdef OPTIM_TVALUE
oqmlStatus *
oqmlDotContext::eval_terminal(Database *db, oqmlContext *ctx,
			      const Oid &oid, int n, oqmlAtomList **alist)
{
  oqmlDotDesc *d = &desc[n];
  Status is;
  oqmlStatus *s;
  int s_ind, e_ind, ind;

  Class *cls;
  is = db->getObjectClass(oid, cls);
  if (is) return new oqmlStatus(dot, is);

  const Attribute *xattr;
  xattr = cls->getAttribute(d->attrname);

  if (!xattr)
    {
      if (n > 0 && oqml_is_subclass(desc[n-1].cls, cls))
	return oqmlSuccess;
      return new oqmlStatus(dot, not_found_fmt,
			    d->attrname, cls->getName());
    }

  if (!d->attr)
    {
      s = d->make(db, ctx, dot, xattr, d->array, d->attrname, 0);
      if (s) return s;
      s = dot->check(db, this);
      if (s) return s;
    }

  int nb;

  d->mod = &xattr->getTypeModifier();

  if (d->mode == Attribute::composedMode)
    nb = d->mod->dims[d->mod->ndims-1];
  else
    nb = 1;

  //s = d->evalInd(dot, db, ctx, s_ind, e_ind, oqml_True, oqml_False);
  s = d->evalInd(dot, db, ctx, s_ind, e_ind, oqml_False, oqml_False);
  if (s) return s;

  Data data = d->e_data;

  oqmlATOMTYPE atom_type = dot_type.type;
  for (ind = s_ind; ind <= e_ind; ind++)
    {
      OQML_CHECK_INTR();
      memset(data, 0, d->key_len);
      Bool isnull = False;

      if (xattr->isVarDim() && d->mode == Attribute::composedMode)
	is = xattr->getTValue(db, oid, (Data *)&data,
			      Attribute::wholeData, ind, &isnull);
      else
	is = xattr->getTValue(db, oid, (Data *)data, nb, ind, &isnull);

      // disconnected the 19/06/01
      /*
      if (is && xattr->isVarDim())
	break;
      */
      
      if (is)
	return new oqmlStatus(dot, is);
      
      if (d->is_coll)
	{
	  if (!isnull)
	    {
	      s = make_contents(d, dot, db, ctx,
				oqmlAtom::make_atom
				(data, atom_type,
				 (dot_type.cls ? dot_type.cls :
				  xattr->getClass())),
				*alist);
	      if (s) return s;
	    }
	}
      else if (isnull)
	(*alist)->append(new oqmlAtom_null);
      else
	{
	  (*alist)->append(oqmlAtom::make_atom(data, atom_type,
					       (dot_type.cls ? dot_type.cls :
						xattr->getClass())));
	  if (xattr->isVarDim() && d->mode == Attribute::composedMode)
	    free(data);
	}
    }
  
  return oqmlSuccess;
}
#endif

oqmlStatus *
oqmlDotContext::eval_perform(Database *db, oqmlContext *ctx, Object *o, 
			     oqmlAtom *value, int n, oqmlAtomList **alist)
{
  if (!o)
    return oqmlSuccess;

  oqmlStatus *s;

  assert(!dot || dot->constructed);

#if DBG_LEVEL > 1
  printf("oqmlDotContext::eval_perform(count = %d, n = %d)\n",
	 count, n);
#endif

  if (n < count - 1)
    {
      s = eval_middle(db, ctx, o, value, n, alist);
      if (s) return s;
      return oqmlSuccess;;
    }

  s = eval_terminal(db, ctx, o, value, n, alist);
  if (s) return s;
#if DBG_LEVEL > 1
  printf("oqmlDotContext::eval_perform -> alist %p\n", *alist);
  fflush(stdout);
#endif
  return oqmlSuccess;
}

oqmlStatus *
oqmlDotContext::eval_object(Database *db, oqmlContext *ctx, 
			    oqmlAtom *atom, oqmlAtom *value, int from,
			    oqmlAtomList **alist)
{
  Object *o;
  if (OQML_IS_OID(atom))
    {
      Oid oid = OQML_ATOM_OIDVAL(atom);

      if (!oid.isValid())
	return oqmlSuccess;
#ifdef OPTIM_TVALUE
      oqmlDotDesc *d = &desc[from];
      
      if (!value && from >= count - 1 && !d->qlmth &&
	  !oqml_is_getcount(d->array) &&
	  (!d->attr || d->attr->isIndirect() ||
	   d->attr->getClass()->asBasicClass() ||
	   d->attr->getClass()->asEnumClass()))
	return eval_terminal(db, ctx, oid, from, alist);
#endif
    }

  oqmlStatus *s = oqmlObjectManager::getObject(dot, db, atom, o, oqml_False);

  if (s) return s;
  if (!o) return oqmlSuccess;

  s = eval_perform(db, ctx, o, value, from, alist);
  if (!s && value)
    {
      //printf("Realizing object '%s'\n", o->getOid().toString());
      // test added the 28/12/00 for non persistent object
      if (o->getOid().isValid())
	{
	  Status is = o->realize();
	  if (is)
	    {
	      oqmlObjectManager::releaseObject(o);
	      return new oqmlStatus(dot, is);
	    }
	}
    }
  oqmlObjectManager::releaseObject(o);
  return s;
}

oqmlStatus *
oqmlDotContext::eval_struct(Database *db, oqmlContext *ctx, 
			    oqmlAtom_struct *astruct, oqmlAtom *value,
			    int from, oqmlAtomList **alist)
{
  oqmlDotDesc *d = &desc[from];
  int idx;
  oqmlAtom *v = astruct->getAtom(d->attrname, idx);

  if (!v)
    return new oqmlStatus(dot, "unknown attribute name '%s' in structure "
			  "'%s'", d->attrname, astruct->getString());
  if (count <= from+1)
    {
      if (value)
	{
	  astruct->setAtom(value, idx, 0);
	  (*alist)->append(value);
	  return oqmlSuccess;
	}
      
      (*alist)->append(v ? v->copy() : (oqmlAtom *)0);
      return oqmlSuccess;
    }
  
  if (!v)
    return oqmlStatus::expected(dot, "oid or struct", "nil");

  if (OQML_IS_OBJECT(v))
    return eval_object(db, ctx, v, value, from+1, alist);

  if (OQML_IS_STRUCT(v))
    return eval_struct(db, ctx, v->as_struct(), value, from+1, alist);

  return oqmlStatus::expected(dot, "oid or struct", v->type.getString());
}

oqmlStatus *
oqmlDotContext::eval(Database *db, oqmlContext *ctx, oqmlAtom *atom,
		     oqmlAtom *value, oqmlAtomList **alist)
{
  assert(atom);
  oqmlStatus *s;

  if (value)
    {
      if (!value->type.cmp(dot_type) &&
	  dot_type.type != oqmlATOM_UNKNOWN_TYPE &&
	  !(dot_type.type == oqmlATOM_OID || dot_type.type == oqmlATOM_OBJ ||
	    value->type.type == oqmlATOM_NULL))
	return new oqmlStatus(dot, "assignation operator: %s expected, got %s.",
			      dot_type.getString(), value->type.getString());
    }

  if (OQML_IS_OBJECT(atom))
    return eval_object(db, ctx, atom, value, 1, alist);

  if (OQML_IS_STRUCT(atom))
    return eval_struct(db, ctx, atom->as_struct(), value, 1, alist);

  // added 28/02/00
  if ((OQML_IS_NULL(atom) || atom->as_nil()) && ctx->isWhereContext())
    return oqmlSuccess;
  // ...

  /*
  // added 9/07/06
  if (OQML_IS_NULL(atom) || atom->as_nil())
    return oqmlSuccess;
  */

  return oqmlStatus::expected(dot, "oid or struct", atom->type.getString());
}

oqmlStatus *
oqmlDotContext::eval_terminal(Database *db, oqmlContext *ctx, 
			     Oid *data_oid, int offset, Bool isref,
			     int n, oqmlAtomList **alist)
{
  oqmlDotDesc *d = &desc[n];
  Data data = d->e_data;
  Status is;
  oqmlStatus *s;
  int s_ind, e_ind, ind;

  s = d->evalInd(dot, db, ctx, s_ind, e_ind, oqml_True, oqml_False);
  if (s) return s;

  const Attribute *xattr = 0;

  if (isref)
    {
      Class *cls;

      is = db->getObjectClass(*data_oid, cls);
      if (is)
	{
	  is->print(utlogFDGet());
	  return oqmlSuccess;
	}

      if (d->attrname)
	{
	  xattr = cls->getAttribute(d->attrname);

	  if (!xattr)
	    {
	      if (n > 0 && oqml_is_subclass(desc[n-1].cls, cls))
		return oqmlSuccess;
	      return new oqmlStatus(dot, not_found_fmt,
				    d->attrname, cls->getName());
	    }
	}
      else if (d->qlmth)
	return ((oqmlMethodCall *)d->qlmth)->perform(db, ctx,
						     0, *data_oid, cls, alist);
    }
  else
    {
      xattr = d->attr;
      if (!xattr && d->qlmth)
	return new oqmlStatus(dot, "cannot perform method call on non object instances");
    }

  d->mod = &xattr->getTypeModifier();
  int nb;

  if (d->mode == Attribute::composedMode)
    nb = d->mod->maxdims;
  else
    nb = 1;
  
#if DBG_LEVEL > 2
  printf("oqmlDotContext::eval_terminal(%s, offset = %d, isref = %d, n = %d)\n",
	 data_oid->getString(), offset, isref, n);
#endif

  Bool isnull = False;
  for (ind = s_ind; ind <= e_ind; ind++)
    {
      OQML_CHECK_INTR();
      printf("!!! oqmlDotContext::getVal !!!\n");
      // could it be replaced by getTValue ??
      is = xattr->getVal(db, data_oid, data, offset, nb, ind, &isnull);

      if (is && xattr->isVarDim())
	continue;

      if (is)
	return new oqmlStatus(dot, is);
    }
  
  if (is == Success)
    {
      if (isnull)
	(*alist)->append(new oqmlAtom_null);
      else
	(*alist)->append(oqmlAtom::make_atom(data, dot_type.type,
					     (dot_type.cls ? dot_type.cls :
					      xattr->getClass())));
    }
  
  return oqmlSuccess;
}

oqmlDotContext::~oqmlDotContext()
{
  idbFreeVect(desc, oqmlDotDesc, oqmlMAXDESC);
  delete tctx;
#ifdef SYNC_GARB
  //delete tlist;
#endif
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlDot methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlDot::oqmlDot(oqmlNode * _qleft, oqmlNode * _qright,
		 oqmlBool _isArrow) : oqmlNode(oqmlDOT)
{
  qleft = _qleft;
  assert(qleft->getType() != oqmlDOT);
  qright = _qright;
  qlmth = 0;
  dot_ctx = 0;
  boolean_dot = False;
  boolean_node = False;
  constructed = oqml_False;
  populated = oqml_False;
  isArrow = _isArrow;
  requal_ident = 0;
}

oqmlStatus *
oqmlDot::hasIndex(Database *db, oqmlContext *ctx, oqmlBool &hasOne)
{
  oqmlBool wasEmpty;
  if (!dot_ctx)
    {
      oqmlStatus *s = complete(db, ctx);
      if (s) return s;
      wasEmpty = oqml_True;
    }
  else
    wasEmpty = oqml_False;

  // WARNING: in some case, dot_ctx->count == 0 that means that
  // one cannot decide whether this has an index or not!
  hasOne = dot_ctx->count > 1 ?
    OQMLBOOL(dot_ctx->desc[dot_ctx->count-1].idx_cnt) : oqml_False;
  return wasEmpty ? reinit(db, ctx) : oqmlSuccess;
}

oqmlDot::~oqmlDot()
{
  delete dot_ctx;
  free(requal_ident);
}

oqmlDotContext *oqmlDot::getDotContext()
{
  return dot_ctx;
}

oqmlStatus *
oqmlDot::getAttrRealize(const Class *cls, const char *name,
			const Attribute **attr)
{
  *attr = (Attribute *)cls->getAttribute(name);
      
  if (*attr)
    return oqmlSuccess;

  unsigned int subclass_cnt, i, j;
  Class **subclasses;
  Status s;
  s = cls->getSubClasses(subclasses, subclass_cnt);
  if (s) return new oqmlStatus(this, s);
  int attr_cnt = 0;
  const Attribute **attrs = (const Attribute **)
    malloc(subclass_cnt*sizeof(const Attribute *));
  
  for (i = 0; i < subclass_cnt; i++)
    {
      const Attribute *xattr = subclasses[i]->getAttribute(name);
      if (xattr && xattr->getClassOwner()->compare(subclasses[i]))
	attrs[attr_cnt++] = xattr;
    }

  if (!attr_cnt)
    {
      free(attrs);
      // disconnected 28/12/00
      //return new oqmlStatus(this, not_found_fmt, name, cls->getName());
      return oqmlSuccess;
    }

  if (attr_cnt == 1)
    {
      *attr = attrs[0];
      free(attrs);
      return oqmlSuccess;
    }

  for (i = 0; i < attr_cnt; i++)
    {
      const Class *class_owner = attrs[i]->getClassOwner();
      for (j = 0; j < attr_cnt; j++)
	{
	  if (i == j) continue;
	  Bool is;
	  s = attrs[j]->getClassOwner()->isSubClassOf(class_owner, &is);
	  if (s) return new oqmlStatus(this, s);
	  if (!is)
	    break;
	}

      if (j == attr_cnt)
	{
	  *attr = attrs[i];
	  break;
	}
    }

  if (!*attr)
    {
      std::string x = std::string("ambiguous attribute '") + name + 
	"' in class '" + cls->getName() + "', candidates are: ";
      for (i = 0; i < attr_cnt; i++)
	x += std::string(i ? ", " : "") +
	  attrs[i]->getClassOwner()->getName() + "::" + attrs[i]->getName();
      free(attrs);
      return new oqmlStatus(this, x.c_str());
    }

  free(attrs);
  return oqmlSuccess;
}

oqmlStatus *
oqmlDot::getAttr(Database *db, oqmlContext *ctx, const Class *cls,
		 oqmlAtom *atom, const char *name, const Attribute **attr,
		 oqmlAtom **rcuratom)
{
  *rcuratom = 0;

  if (atom)
    {
      if (OQML_IS_STRUCT(atom))
	{
	  int idx;
	  oqmlAtom *a = atom->as_struct()->getAtom(name, idx);
	  if (!a)
	    return new oqmlStatus(this,
				  "unknown attribute name '%s' "
				  "in structure '%s'", name,
				  atom->as_struct()->getString());
	  *rcuratom = a;
	}
      else if (OQML_IS_OBJECT(atom))
	{
	  // test added this test 1/03/00
	  if (!(OQML_IS_OID(atom) && !OQML_ATOM_OIDVAL(atom).isValid() &&
		ctx->isWhereContext()))
	    {
	      Object *o;
	      oqmlStatus *s = oqmlObjectManager::getObject(this, db, atom, o,
							   oqml_False);
	      if (s) return s;
	      cls = o->getClass();
	      oqmlObjectManager::releaseObject(o);
	    }
	}
      // added this test 28/02/00
      // added as_nil the 8/3/00
      else if (!(ctx->isWhereContext() &&
		 (OQML_IS_NULL(atom) || atom->as_nil())))
	return new oqmlStatus(this, "invalid item type for left dot part");
    }
  
  const char *attrname;
  const Class *attrcls;
  oqmlStatus *s = isScope(db, name, attrname, attrcls, attr);
  if (s) return s;

  if (attrname)
    {
      if (cls)
	{
	  Status is;
	  Bool isSub;
	  is = attrcls->isSubClassOf(cls, &isSub);
	  if (is) return new oqmlStatus(this, is);
	  if (!isSub)
	    return new oqmlStatus(this, "class '%s' is not a subclass of '%s'",
				  attrcls->getName(), cls->getName());
	}
    }
  else if (cls)
    {
#if DBG_LEVEL > 2
      printf("cls 0x%x [cnt=%d], *attr 0x%x, clname %s, name %s\n",
	     cls, cls->getAttributesCount(), *attr, cls->getName(),
	     name);
#endif
      // WARNING: changed the 15/10/99
#if 1
      return getAttrRealize(cls, name, attr);
#else
      *attr = (Attribute *)cls->getAttribute(name);
      
      if (!*attr)
	return new oqmlStatus(this, not_found_fmt, name, cls->getName());
#endif
    }
  else
    *attr = 0;

  return oqmlSuccess;
}

oqmlStatus *
oqmlDot::isScope(Database *db, const char* x, const char*& attrname,
		 const Class *&cls, const Attribute **attr)
{
  attrname = 0;
  cls = 0;
  if (!x)
    return oqmlSuccess;

  const char *r = strchr(x, ':');
  if (!r)
    return oqmlSuccess;

  const char *s;

  s = strchr(r+1, ':');
  if (!OQMLBOOL(s == r+1))
    return oqmlSuccess;

  static char clsname[128];

  strncpy(clsname, x, r-x);
  clsname[r-x] = 0;
  cls = db->getSchema()->getClass(clsname);

  if (!cls)
    return new oqmlStatus(this, "unknown class '%s'", clsname);

  attrname = s+1;

  const Attribute *xattr = cls->getAttribute(attrname);
  if (!xattr)
    return new oqmlStatus(this, "unknown attribute '%s' in class '%s'",
			  attrname, clsname);

  if (attr)
    *attr = xattr;

  return oqmlSuccess;
}


oqmlStatus *
oqmlDot::oqmlDot_left(Database *db, oqmlContext *ctx,
		      const Class *cls,
		      oqmlAtom *curatom,
		      Attribute **attr,
		      oqmlAtom **rcuratom,
		      Class **castcls,
		      char **attrname)
{
  const char *name;
  *attr = 0;
  oqmlStatus *s;
  oqmlTYPE type = qright->getType();
  *rcuratom = 0;

  /*
  printf("oqmlDot::Dot_left [%s [%s | %s]]\n",
	 (const char *)toString(),
	 (const char *)qleft->toString(), (const char *)qright->toString());
	 */

  if (type == oqmlIDENT) {
    name = ((oqmlIdent *)qright)->getName();
    s = getAttr(db, ctx, cls, curatom, name, (const Attribute **)attr,
		rcuratom);
    if (s) return s;
    *castcls = 0;
    // added 28/12/00
    if (!*attr) {
      // MIND, the code:
      // qlmth = new oqmlMethodCall(name, new oqml_List(), oqml_True);
      // *attrname = 0;
      // has been added on the 28/12/00 to accept constructs like
      // x.y where y is a method with no argument.
      // The problem is that, constructs like:
      // (select one Person).getDatabase().dbname;
      // did not work anymore.
      // So I disconnected this code on the 15/05/01
      *attrname = (char *)name;
    }
    else
      *attrname = (char *)name;
    
  }
  else if (type == oqmlCASTIDENT) {
    const char *modname;
    name = ((oqmlCastIdent *)qright)->getName(&modname);
    *attr = (Attribute *)cls->getAttribute(name);

    *castcls = db->getSchema()->getClass(modname);
    if (!*castcls)
      return new oqmlStatus(this, "class '%s' not found", modname);

    if (!*attr)
      return new oqmlStatus(this, not_found_fmt,
			    name, cls->getName());
    *attrname = (char *)name;
  }
  else if (type == oqmlDOT || type == oqmlARRAY) {
    s = qright->compile(db, ctx);
    if (s)
      return s;
    *attr = 0;
    *castcls = 0;
    *attrname = 0;
  }
  else if (type == oqmlCALL) {
    s = ((oqmlCall *)qright)->preCompile(db, ctx);
    if (s) return s;

    // added the 28/05/01 because of a memory leak detected by purify!
    if (qlmth) qlmth->unlock();

    qlmth = new oqmlMethodCall(((oqmlCall *)qright)->getName(),
			       ((oqmlCall *)qright)->getList());
    if (locked)
      qlmth->lock();
    *attr = 0;
    *castcls = 0;
    *attrname = 0;
  }
  else
    return new oqmlStatus(this, "invalid item type for left dot part");

  return oqmlSuccess;
}

oqmlStatus *
oqmlDot::construct(Database *db, oqmlContext *ctx,
		   const Class *cls,
		   oqmlAtom *curatom,
		   oqmlDotContext **pdctx)
{
  oqmlDotContext *dctx;
  oqmlStatus *s;
  Class *castcls;

#if DBG_LEVEL > 1
  printf("oqmlDot::construct(%p)\n", *pdctx);
#endif
  if (curatom)
    dctx = (*pdctx ? *pdctx : new oqmlDotContext(this, curatom));
  else
    dctx = (*pdctx ? *pdctx : new oqmlDotContext(this, cls));
  ctx->setDotContext(dctx);
	  
  Attribute *attr;
  char *attrname;

  oqmlAtom *rcuratom;
  s = oqmlDot_left(db, ctx, cls, curatom, (Attribute **)&attr, &rcuratom,
		   &castcls, &attrname);
	  
  if (s)
    {
      if (!*pdctx)
	delete dctx;
      return s;
    }
	  
  s = dctx->add(db, ctx, attr, 0, attrname, rcuratom, castcls, qlmth);
	  
  if (s)
    {
      if (!*pdctx)
	delete dctx;
      return s;
    }
	  
  ctx->setDotContext(0);

  if (qlmth && !((oqmlMethodCall *)qlmth)->isCompiled())
    {
      s = qlmth->compile(db, ctx);
      if (s)
	return s;
    }

  *pdctx = dctx;
  constructed = oqml_True;
  return oqmlSuccess;
}

oqmlStatus *
oqmlDot::check(Database *db, oqmlDotContext *dctx)
{
  oqmlDotDesc *d = &dctx->desc[dctx->count-1];
  if (!d->attr) // added 17/02/99
    return oqmlSuccess;

  Class *cls = (Class *)d->attr->getClass();

  eval_type.cls = 0;
  eval_type.comp = OQMLBOOL(d->mode == Attribute::composedMode);

  Schema *m = db->getSchema();
  if (d->isref)
    {
      eval_type.type = oqmlATOM_OID;
      eval_type.cls = cls;
    }
  else if ((cls->asCharClass() || cls->asByteClass())
	   && eval_type.comp)
    {
      eval_type.type = oqmlATOM_STRING;
      eval_type.comp = oqml_True;
    }
  else if (eval_type.comp)
    return new oqmlStatus(this,
			  "array attribute '%s': use the array operator '[]'",
			  d->attr->getName());
  else if (cls->asInt32Class() || cls->asInt16Class() ||
	   cls->asInt64Class() || cls->asEnumClass() ||
	   oqml_is_getcount(d->array))
    eval_type.type = oqmlATOM_INT;
  else if (cls->asCharClass() || cls->asByteClass())
    eval_type.type = oqmlATOM_CHAR;
  else if (!strcmp(cls->getName(), m->Float_Class->getName()))
    eval_type.type = oqmlATOM_DOUBLE;
  else if (!strcmp(cls->getName(), m->OidP_Class->getName()))
    eval_type.type = oqmlATOM_OID;
#ifdef SUPPORT_CSTRING
  else if (!strcmp(cls->getName(), "ostring"))
    {
      eval_type.type = oqmlATOM_STRING;
      eval_type.comp = oqml_True;
    }
#endif
#ifdef OBJ_SUPPORT
  else
    eval_type.type = oqmlATOM_OBJ;
#else
  else
    return new oqmlStatus(this,
			  "cannot deal with the literal non basic attribute '%s'",
			  d->attr->getName());
#endif

  dctx->dot_type = eval_type;
  return oqmlSuccess;
}

oqmlStatus *
oqmlDot::compile_start(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;

  /*
  printf("oqmlDot::compile_start[%s [%s | %s]] -> %p left = %d [%p], right = %d [%p]\n",
	 (const char *)toString(),
	 (const char *)qleft->toString(), (const char *)qright->toString(),
	 this, qleft->getType(), qleft,
	 qright->getType(), qright);
	 */

  if (qleft->getType() != oqmlIDENT)
    {
      if (qleft->getType() != oqmlCALL)
	{
	  s = qleft->compile(db, ctx);
	  if (s)
	    return s;
	}
      dot_ctx = new oqmlDotContext(this, qleft);
      return oqmlSuccess;
    }
  else
    {
      oqmlDotContext *dctx;
      const char *name;
      const Class *cls = 0;
      oqmlAtomType at;
      const Attribute *attr;

      name = ((oqmlIdent *)qleft)->getName();
      
      oqmlAtom *atom;
	  
      if (ctx->getSymbol(name, &at, &atom))
	{
	  if (at.type == oqmlATOM_SELECT)
	    {
#if DBG_LEVEL > 1
	      printf("oqmlDOT #1 '%s' => SYMBOL %s is a select [select_ctx %d]!\n", 
		     (const char *)toString(), name,
		     ctx->isSelectContext());
#endif
	      boolean_dot = False;
	      boolean_node = True;
	      return oqmlSuccess;
	    }
	  else
	    {
	      dot_ctx = new oqmlDotContext(this, name);
#if DBG_LEVEL > 1
	      printf("oqmlDOT #2 '%s' => SYMBOL %s is NOT a select!\n",
		     (const char *)toString(), name);
#endif
	      boolean_dot = True;
	      boolean_node = False;
	      return oqmlSuccess;
	    }
	}
      else if (ctx->isSelectContext())
	{
	  boolean_dot = False;
	  cls = db->getSchema()->getClass(name);
#if DBG_LEVEL > 1
	  printf("oqmlDOT #3 '%s' => SYMBOL %s is a class ?\n",
		     (const char *)toString(), name);
#endif
	}
      else
	{
	  // added the 12/01/00
	  dot_ctx = new oqmlDotContext(this, name);
#if DBG_LEVEL > 1
	  printf("oqmlDOT #4 '%s' => assuming SYMBOL %s is NOT a select\n",
		     (const char *)toString(), name);
#endif
	  boolean_dot = True;
	  boolean_node = False;
	  // ...
	  return oqmlSuccess;
	}

#if DBG_LEVEL > 1
      printf("oqmlDot::compile_start SelectContext %d, cls = %s <<%s>>\n",
	     ctx->isSelectContext(), (cls ? cls->getName() : "NO"),
	     (const char *)toString());
#endif

      if (!cls)
	return new oqmlStatus(this, "unknown class '%s'", name);

      dctx = 0;
      s = construct(db, ctx, cls, 0, &dctx);

      if (s)
	return s;

      dot_ctx = dctx;
      return check(db, dot_ctx);
    }
  
}

oqmlStatus *
oqmlDot::compile_continue(Database *db, oqmlContext *ctx,
			   oqmlDotContext *dctx)
{
  oqmlStatus *s;
  const char *name;
  const Class *cls;
  oqmlAtomType at;
  const Attribute *attr;
  oqmlTYPE t = qleft->getType();
  oqmlDotDesc *d;
  Class *castcls;

  /*
  printf("oqmlDot::compile_continue [%s [%s | %s]]\n",
	 (const char *)toString(),
	 (const char *)qleft->toString(), (const char *)qright->toString());
  */

  if (t == oqmlIDENT || t == oqmlCASTIDENT)
    {
      d = &dctx->desc[dctx->count-1];
      
      cls = d->cls;
      attr = d->attr;

      if (t == oqmlCASTIDENT)
	{
	  const char *modname;
	  name = ((oqmlCastIdent *)qleft)->getName(&modname);
	  castcls = db->getSchema()->getClass(modname);
	  if (!castcls)
	    return new oqmlStatus(this, "unknown class '%s'", modname);
	}
      else
	{
	  name = ((oqmlIdent *)qleft)->getName();
	  castcls = 0;
	}
      
      if (!cls)
	return new oqmlStatus(this, "class is unknown");

      oqmlAtom *rcuratom;
      s = getAttr(db, ctx, cls, d->curatom, name, &attr, &rcuratom);
      if (s) return s;
      // added 28/12/00 because getAttrRealize changed
      if (!attr) {
	return new oqmlStatus(this, not_found_fmt, name, cls ? cls->getName() : "<unknown>");
      }
      // ...

      s = dctx->add(db, ctx, attr, 0, (char *)name, rcuratom, castcls, 0);
      if (s) return s;
    }
  else if (t == oqmlARRAY)
    {
      s = qleft->compile(db, ctx);
      if (s) return s;
    }
  else if (t != oqmlCALL) // added the 19/02/99
    return new oqmlStatus(this, "cannot use a path expression "
			  "on a unidentificable atom.");
  else if (t == oqmlCALL)
    {
      s = ((oqmlCall *)qleft)->preCompile(db, ctx);
      if (s) return s;

      oqmlMethodCall *xmth = new oqmlMethodCall(((oqmlCall *)qleft)->getName(),
						((oqmlCall *)qleft)->getList());
      if (locked)
	xmth->lock();
      s = dctx->add(db, ctx, 0, 0, 0, 0, 0, xmth);
      if (s) return s;
    }

  d = &dctx->desc[dctx->count-1];
  cls = d->cls;
  oqmlAtom *curatom = d->curatom;

  char *attrname;
  oqmlAtom *rcuratom;
  s = oqmlDot_left(db, ctx, cls, curatom, (Attribute **)&attr, &rcuratom,
		   &castcls, &attrname);
  if (s)
    return s;
  
  s = dctx->add(db, ctx, attr, 0, attrname, rcuratom, castcls, qlmth);
  
  if (s)
    return s;

  if (qlmth && !((oqmlMethodCall *)qlmth)->isCompiled())
    {
      oqmlDotContext *dctx = ctx->getDotContext();
      ctx->setDotContext(0);
      s = qlmth->compile(db, ctx);
      ctx->setDotContext(dctx);
      if (s)
	return s;
    }

  return oqmlSuccess;
}

oqmlStatus *
oqml_use_index(Database *db, oqmlContext *ctx, oqmlNode *node,
	       oqmlDotContext *dctx, oqmlAtomList **alist,
	       oqmlBool &indexUsed)
{
  if (db->getVersionNumber() < MULTIIDX_VERSION)
    {
      indexUsed = oqml_False;
      return oqmlSuccess;
    }

  indexUsed = oqml_True;
  
  oqmlDotDesc *d = 0;
  for (int i = 1; i < dctx->count; i++)
    {
      d = &dctx->desc[i];
      if ((i != dctx->count-1 && d->isref) ||
	  (d->array && !d->array->wholeRange))
	{
	  indexUsed = oqml_False;
	  return oqmlSuccess;
	}
    }

  if (!d || !d->idx_cnt)
    {
      indexUsed = oqml_False;
      return oqmlSuccess;
    }

  if (d->attr->getClass()->asCollectionClass() &&
      ((d->attr->isIndirect() && d->is_coll) ||
       (!d->attr->isIndirect() && !d->is_coll)))
    {
      indexUsed = oqml_False;
      return oqmlSuccess;
    }

  return oqmlIndexIter(db, ctx, node, dctx, d, alist);
}

oqmlStatus *
oqmlDot::eval_realize(Database *db, oqmlContext *ctx,
		      const Class *cls,
		      oqmlAtom *atom, oqmlAtom *value, oqmlAtomList **alist)
{
  oqmlDotContext *dctx;
  oqmlStatus *s;

#if DBG_LEVEL > 1
  printf("oqmlDot::eval_realize(cls = %s)\n", cls ? cls->getName() : "NOCLASS");
#endif
	 
  dctx = 0;
  s = construct(db, ctx, cls, (cls ? 0 : atom), &dctx);

  if (s)
    return s;

  s = check(db, dctx);
  if (s)
    return s;

  delete dot_ctx->tctx;
  dot_ctx->tctx = dctx;

#if DBG_LEVEL > 1
  printf("oqmlDot::eval_realize SelectContext %d [%s] atom = '%s'\n",
	 ctx->isSelectContext(), cls->getName(),
	 atom ? atom->getString() : "");
#endif

  if (atom)
    return dctx->eval(db, ctx, atom, value, alist);

  oqmlBool indexUsed;

  if ((s = oqml_use_index(db, ctx, this, dctx, alist, indexUsed)) || indexUsed)
    return s;

  // should use an ClassIterator
  Iterator q((Class *)cls, True);
  
  Status is;
  if ((is = q.getStatus()) == Success)
    {
      IteratorAtom qatom;
      Bool found;
      
      for (;;)
	{
	  OQML_CHECK_INTR(); // must put 'q' for args to delete it
	  is = q.scanNext(&found, &qatom);
	  
	  if (is)
	    return new oqmlStatus(this, is);
	  
	  if (!found)
	    break;
	  
	  s = dctx->eval(db, ctx,
			 oqmlAtom::make_atom(qatom, (Class *)cls),
			 value, alist);
	  if (s) return s;
	  OQML_CHECK_MAX_ATOMS(*alist, ctx, 0);
	}
    }
  else
    return new oqmlStatus(this, is);

  return oqmlSuccess;
}

oqmlStatus *oqmlDot::compile(Database *db, oqmlContext *ctx)
{
  delete dot_ctx;
  // added the 28/05/01 because of a memory leak detected by purify!
  if (qlmth) qlmth->unlock();

  qlmth = 0;
  dot_ctx = 0;
  boolean_dot = False;
  boolean_node = False;
  constructed = oqml_False;
  populated = oqml_False;

  oqmlDotContext *dctx = ctx->getDotContext();

#if DBG_LEVEL > 1
  printf("oqmlDot::compile this = %p %s SelectContext = %d, DotContext = %p\n",
	 this, (const char *)toString(), ctx->isSelectContext(), dctx);
#endif
  oqmlStatus *s;
  if (!dctx)
    s = compile_start(db, ctx);
  else
    s = compile_continue(db, ctx, dctx);

#if DBG_LEVEL > 1
  printf("oqmlDot::compile %s dot_ctx %p count %d\n",
	 (const char *)toString(), dot_ctx, (dot_ctx ? dot_ctx->count : 0));
#endif
  
  return s;
}

oqmlStatus *
oqmlDot::reinit(Database *db, oqmlContext *ctx, oqmlBool mustCompile)
{
  delete dot_ctx;

#if DBG_LEVEL > 1
  printf("oqmlDot::reinit <<%s>>\n", (const char *)toString());
#endif

  // added the 28/05/01 because of a memory leak detected by purify!
  if (qlmth) qlmth->unlock();
  qlmth = 0;
  dot_ctx = 0;
  boolean_dot = False;
  boolean_node = False;
  constructed = oqml_False;
  populated = oqml_False;
  return (mustCompile ? compile(db, ctx) : oqmlSuccess);
}

oqmlStatus *
oqmlDot::complete(Database *db, oqmlContext *ctx)
{
#if DBG_LEVEL > 1
  printf("oqmlDot::complete(this = %p, %s)\n", this,
	 (const char *)toString());
#endif
  if (dot_ctx)
    return oqmlSuccess;

#if DBG_LEVEL > 1
  assert(boolean_node);
  assert(qleft->getType() == oqmlIDENT);
#endif
  const char *name = ((oqmlIdent *)qleft)->getName();
  oqmlAtom *atom;
  oqmlAtomType at;
  oqmlStatus *s;

  if (!ctx->getSymbol(name, &at, &atom))
    return new oqmlStatus(this, oqml_uninit_fmt, name);

  if (!atom)
    return new oqmlStatus(this, "internal select error");

  if (at.type != oqmlATOM_SELECT)
    {
      dot_ctx = new oqmlDotContext(this, name);
      boolean_dot = True;
      boolean_node = False;
      return oqmlSuccess;
    }

  oqmlNode *node = atom->as_select()->node;
#if DBG_LEVEL > 1
  printf("Symbol %s is still a selectContext!\n", name);
#endif

  oqmlNode *node_ident;
  
  /*
  if (!node)
    node_ident = 0;
    else */ if (node && node->getType() == oqmlIDENT)
    node_ident = node;
  else if ((ctx->isOrContext() || ctx->isAndContext()) &&
	   atom->as_select()->node_orig->getType() == oqmlIDENT)
    node_ident = atom->as_select()->node_orig;
  else
    node_ident = 0;

  if (node_ident)
    {
      const Class *cls =
	db->getSchema()->getClass(((oqmlIdent *)node_ident)->getName());
      
      if (!cls)
	return new oqmlStatus(this, "unknown class '%s'",
			      ((oqmlIdent *)node_ident)->getName());
      
      s = construct(db, ctx, cls, 0, &dot_ctx);
      if (s) return s;
      return check(db, dot_ctx);
    }

  if (atom->as_select()->list)
    {
#if DBG_LEVEL > 1
      printf("oqmlDot::complete() WARNING there is a list in atom select\n");
#endif
      dot_ctx = new oqmlDotContext(this, node);
      return oqmlSuccess;
    }

  dot_ctx = new oqmlDotContext(this, node);
  return oqmlSuccess;
}

oqmlStatus *oqmlDot::eval(Database *db, oqmlContext *ctx,
			  oqmlAtomList **alist, oqmlComp *comp,
			  oqmlAtom *atom)
{
  oqmlStatus *s;

#if DBG_LEVEL > 1
  printf("oqmlDot::eval SelectContext = <<%s>>:\n", (const char *)toString());
#endif

  s = complete(db, ctx);
  if (s) return s;

#if DBG_LEVEL > 1
  printf("\tcomp %p\n\tdot_ctx->oqml %p\n\tdot_ctx->varname %p\n\t[%s]\n\ttctx %p\n",
	 comp, dot_ctx->oqml, dot_ctx->varname, dot_ctx->oqml ?
	 (const char *)dot_ctx->oqml->toString() : "",
	 dot_ctx->tctx);
#endif

  if (comp)
    return comp->makeIterator(db, dot_ctx, atom);

  if (dot_ctx->oqml || dot_ctx->varname)
    {
      s = eval_perform(db, ctx, 0, alist);
      if (s) return s;

      if (*alist && (*alist)->cnt > 1)
	*alist = new oqmlAtomList(new oqmlAtom_list(*alist));

      return oqmlSuccess;
    }

  s = eval_perform(db, ctx, 0, alist);
  if (s) return s;

  if (*alist && (*alist)->cnt > 1)
    *alist = new oqmlAtomList(new oqmlAtom_list(*alist));

  return oqmlSuccess;
}

oqmlStatus *oqmlDot::set(Database *db, oqmlContext *ctx, oqmlAtom *value,
			 oqmlAtomList **alist)
{
  return eval_perform(db, ctx, value, alist);
}

oqmlArray *oqmlDot::make_right_array()
{
  if (qright->getType() == oqmlARRAY)
    return (oqmlArray *)qright;

  if (qright->getType() == oqmlIDENT)
    {
      oqmlArray *array = new oqmlArray(qright);
      qright = array;
      return array;
    }

  if (qright->asDot())
    return qright->asDot()->make_right_array();

  return 0;
}

oqmlDot *oqmlDot::make_right_dot(const char *ident, oqmlBool _isArrow)
{
  if (qright->getType() != oqmlDOT)
    {
      qright = new oqmlDot(qright, new oqmlIdent(ident), _isArrow);
      return this;
    }

  qright = qright->asDot()->make_right_dot(ident, _isArrow);
  return this;
}

oqmlDot *oqmlDot::make_right_call(oqml_List *list)
{
  if (qright->getType() != oqmlDOT)
    {
      qright = new oqmlCall(qright, list);
      return this;
    }

  qright = qright->asDot()->make_right_call(list);
  return this;
}

oqmlAtomList *
make_atom_coll_1(oqmlAtom_coll *a, oqmlAtomList *rlist)
{
  if (!a)
    return rlist;

  if (a->as_list())
    return new oqmlAtomList(new oqmlAtom_list(rlist));
  if (a->as_bag())
    return new oqmlAtomList(new oqmlAtom_bag(rlist));
  if (a->as_set())
    return new oqmlAtomList(new oqmlAtom_set(rlist));
  if (a->as_array())
    return new oqmlAtomList(new oqmlAtom_array(rlist));

  return 0;
}

oqmlAtom *
make_atom_coll_2(oqmlAtom_coll *a, oqmlAtomList *rlist)
{
  if (a->as_list())
    return new oqmlAtom_list(rlist);

  if (a->as_bag())
    return new oqmlAtom_bag(rlist);

  if (a->as_set())
    return new oqmlAtom_set(rlist);

  if (a->as_array())
    return new oqmlAtom_array(rlist);

  return 0;
}

oqmlStatus *
oqmlDot::eval_realize_list(Database *db, oqmlContext *ctx,
			   oqmlAtomList *list, oqmlAtom *value,
			   oqmlAtomList **alist, int level)
{
  oqmlStatus *s;
  oqmlAtom *a = list->first;

  while (a)
    {
      oqmlAtom *next = a->next;
      if (a->as_coll())
	{
	  if (!level)
	    {
	      s = eval_realize_list(db, ctx, a->as_coll()->list, value, alist,
				    level+1);
	      if (s) return s;
	    }
	  else
	    {
	      oqmlAtomList *rlist = new oqmlAtomList();
	      s = eval_realize_list(db, ctx, OQML_ATOM_COLLVAL(a), value,
				    &rlist, level+1);
	      if (s) return s;
	      (*alist)->append(make_atom_coll_2(a->as_coll(), rlist));
	    }
	}
      else
	{
	  Class *cls = a->type.cls;
	  s = oqml_getclass(this, db, ctx, a, &cls);
	  if (s) return s;
	  
	  s = eval_realize(db, ctx, cls, a, value, alist);
	  if (s) return s;
	}

      a = next;
    }

  return oqmlSuccess;
}

oqmlStatus *oqmlDot::eval_perform(Database *db, oqmlContext *ctx,
				  oqmlAtom *value, oqmlAtomList **xalist)
{
  oqmlAtomList *rlist = 0;
  oqmlStatus *s;

#if DBG_LEVEL > 1
  printf("oqmlDot::eval_perform -> %d\n", ctx->isSelectContext());
#endif
  rlist = new oqmlAtomList();

  if (dot_ctx->varname)
    {
      const char *varname = dot_ctx->varname;
      
      oqmlAtomType at;
      oqmlAtom *a = 0;
      
      if (ctx->getSymbol(varname, &at, &a) && a) {
#ifdef SYNC_GARB
	//delete dot_ctx->tlist;
#endif
	dot_ctx->tlist = new oqmlAtomList(a->copy());

	Class *cls = at.cls;
#if DBG_LEVEL > 1
	printf("oqmlDot::eval -> getSymbol(%s) -> %s [%s]\n",
		 varname, (a ? a->makeString(0) : "NullAtom"),
	       cls ? cls->getName() : "noclass");
#endif
	if (a->as_coll()) {
	  s = eval_realize_list(db, ctx, OQML_ATOM_COLLVAL(a), value,
				&rlist, 0);
	  if (s) return s;

	  *xalist = make_atom_coll_1(a->as_coll(), rlist);
	  return oqmlSuccess;
	}

	s = oqml_getclass(this, db, ctx, a, &cls, varname);
	if (s) return s;
	  
	s = eval_realize(db, ctx, cls, a, value, &rlist);
	if (s) return s;
	
	*xalist = rlist;
	return oqmlSuccess;
      }

      return new oqmlStatus(this, oqml_uninit_fmt, varname);
    }

  if (dot_ctx->oqml)
    {
      oqmlAtomList *al;

      s = dot_ctx->oqml->compile(db, ctx);
      if (s) return s;

      s = dot_ctx->oqml->eval(db, ctx, &al);
      if (s) return s;

#ifdef SYNC_GARB
      //delete dot_ctx->tlist;
#endif
      dot_ctx->tlist = new oqmlAtomList(al);

      s = eval_realize_list(db, ctx, al, value, &rlist, 0);
      if (s) return s;
      *xalist = make_atom_coll_1(al->first ? al->first->as_coll() : 0, rlist);
      return oqmlSuccess;
    }

  s = eval_realize(db, ctx, dot_ctx->desc[0].cls, 0, value, &rlist);
  if (s) return s;
  *xalist = new oqmlAtomList(new oqmlAtom_bag(rlist));
  return oqmlSuccess;
}

static void
oqml_append_realize(oqmlContext *ctx, oqmlAtom_select *atom_select,
		    oqmlAtomList *list)
{
  oqmlAtomList *alist = atom_select->list;
#if 0
  if (!alist) {
    oqmlDot::makeSet(ctx, atom_select,  list);
    return;
  }
#else
  if (!alist)
    alist = atom_select->list = new oqmlAtomList();
#endif

  oqmlAtom *a = (list ? list->first : 0);

  if (a && a->as_coll())
    a = a->as_coll()->list->first;

  while (a)
    {
      oqmlAtom *next = a->next;
      if (alist->first && alist->first->as_coll())
	alist->first->as_coll()->list->append(a);
      else
	alist->append(a);

      atom_select->appendCP(ctx);
      a = next;
    }
}

#define NEW_POPULATE_POLICY

void
oqmlDot::makeUnion(oqmlContext *ctx, oqmlAtom_select *atom_select, oqmlAtomList *list)
{
#if DBG_LEVEL > 1
  printf("oqmlDot::makeUnion symbol set to %s (should append ?) a->list=%p, "
	 "list=%p\n", atom_select->getString(), atom_select->list, list);
#endif

  oqml_append_realize(ctx, atom_select, list);
}

void
oqmlDot::makeIntersect(oqmlContext *ctx, oqmlAtom_select *atom_select,
		       oqmlAtomList *list)
{
#if DBG_LEVEL > 1
  printf("oqmlDot::makeIntersect(%p, %d) ", list,
	 //(list->first ? list->first->getString() : ""));
	 (list->first ? list->cnt : 0));
#endif
  if (!list->first || !atom_select->as_select()->list ||
      !OQML_IS_COLL(list->first))
    {
      atom_select->list = new oqmlAtomList();
      atom_select->setCP(ctx);
      return;
    }

  oqmlAtomList *rlist = new oqmlAtomList();
  oqmlAtomList *nlist =  atom_select->as_select()->list;

  oqmlAtom *a = ((oqmlAtom_coll *)list->first)->list->first;
  while (a) 
    {
      assert(OQML_IS_OID(a));
      oqmlBool found = oqml_False;
      oqmlAtom *next = a->next;
      Oid oid = OQML_ATOM_OIDVAL(a);
      oqmlAtom *na = nlist->first;
      while (na)
	{
	  assert(OQML_IS_OID(na));
	  if (oid == OQML_ATOM_OIDVAL(na))
	    {
	      found = oqml_True;
	      break;
	    }
	  na = na->next;
	}
      
      if (found) {
	rlist->append(a);
	atom_select->appendCP(ctx);
      }
      a = next;
    }
  
  atom_select->list = rlist;
}

void
oqmlDot::makeSet(oqmlContext *ctx, oqmlAtom_select *atom_select, oqmlAtomList *list)
{
  if (list->first && OQML_IS_COLL(list->first))
    atom_select->list = OQML_ATOM_COLLVAL(list->first);
  else
    atom_select->list = list;

  atom_select->setCP(ctx);
}		    

oqmlStatus *
oqmlDot::populate(Database *db, oqmlContext *ctx, oqmlAtomList *list,
		  oqmlBool _makeUnion)
{
  assert(qleft->getType() == oqmlIDENT);

  const char *name = ((oqmlIdent *)qleft)->getName();

#if DBG_LEVEL > 1
  printf("\noqmlDot::populate dot=%p symbol '%s' "
	 "<<%s>> -> list_cnt=%d (%s) populated=%d, "
	 "boolean_node=%d, and_ctx=%d, or_ctx=%d, makeunion=%d\n",
	 this, name, (const char *)toString(),
	 (list && list->first && OQML_IS_COLL(list->first) ?
	  OQML_ATOM_COLLVAL(list->first)->cnt : 0),
	 (list ? (const char *)list->getString() : ""),
	 populated,
	 boolean_node, ctx->isAndContext(), ctx->isOrContext(),
	 _makeUnion);
#endif
  
  if (!list || !boolean_node || (populated && !_makeUnion))
    return oqmlSuccess;

  oqmlAtom *atom = 0;
  ctx->getSymbol(name, 0, &atom);

  assert(atom && atom->as_select());

  oqmlAtom_select *atom_select = atom->as_select();

  if (ctx->isOrContext() || _makeUnion)
    makeUnion(ctx, atom_select, list);
  else if (ctx->isAndContext())
    makeIntersect(ctx, atom_select, list);
  else
    makeSet(ctx, atom_select, list);

  oqmlStatus *s = ctx->setSymbol(name, &atom_select->type,
				 atom_select, oqml_False);
  if (s) return s;

  atom_select->as_select()->node = 0;
  populated = oqml_True;
  return oqmlSuccess;
}

void oqmlDot::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlDot::isConstant() const
{
  return OQMLBOOL(qleft->isConstant() && qright->isConstant());
}

void
oqmlDot::setIndexHint(oqmlContext *ctx, oqmlBool indexed)
{
  if (qleft->asDot())
    qleft->asDot()->setIndexHint(ctx, indexed);

  if (qleft->getType() == oqmlIDENT)
    {
      oqmlAtom *atom;
	  
      if (ctx->getSymbol(((oqmlIdent *)qleft)->getName(), 0, &atom) &&
	  atom && atom->as_select())
	atom->as_select()->indexed = indexed;
    }
}

oqmlBool
oqmlDot::hasIdent(const char *_ident)
{
  return OQMLBOOL(qleft->hasIdent(_ident) ||
		  (qlmth && qlmth->hasIdent(_ident)));
}

const char *
oqmlDot::getLeftIdent() const
{
  if (qleft->asDot())
    return qleft->asDot()->getLeftIdent();
  if (qleft->asIdent())
    return qleft->asIdent()->getName();

  return (const char *)0;
}

void
oqmlDot::replaceLeftIdent(const char *ident, oqmlNode *node, oqmlBool &done)
{
  /*
  printf("oqmlDot::replaceLeftIdent(%s, %s, %s, dot_context = %p)\n",
	 ident, (const char *)toString(),
	 (const char *)node->toString(),
	 dot_ctx);
	 */
  
  if (qleft->asDot())
    qleft->asDot()->replaceLeftIdent(ident, node, done);
  else if (qleft->asIdent())
    {
      if (!strcmp(qleft->asIdent()->getName(), ident))
	{
	  requal_ident = strdup(ident);
	  done = oqml_True;

	  if (node->asDot())
	    {
	      oqmlNode *nqleft = node->asDot()->qleft;
	      oqmlNode *nqright = new oqmlDot(node->asDot()->qright, qright, oqml_False);
	      nqleft->back = qleft;
	      nqright->back = qright;
	      qleft = nqleft;
	      qright = nqright;

	      if (isLocked())
		qright->lock();
	    }
	  else
	    {
	      node->back = qleft;
	      qleft = node;
	    }

	  if (isLocked())
	    qleft->lock();
	}
    }
  else
    assert(0);
}

oqmlStatus *
oqmlDot::requalify_back(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s = requalify_node_back(db, ctx, qleft);
  if (s) return s;
  return requalify_node_back(db, ctx, qright);
}

void
oqmlDot::lock()
{
  oqmlNode::lock();
  qleft->lock();
  qright->lock();
  if (qlmth) qlmth->lock();
}

void
oqmlDot::unlock()
{
  oqmlNode::unlock();
  qleft->unlock();
  qright->unlock();
  if (qlmth) qlmth->unlock();
}

std::string
oqmlDot::toString(void) const
{
  return qleft->toString() + (isArrow ? "->" : ".") + qright->toString()
    + oqml_isstat();
}
}
