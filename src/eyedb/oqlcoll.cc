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
#include <math.h>
#include <assert.h>

#include "oql_p.h"
#define OQML_NEW_COLL

namespace eyedb {

  //
  // TODO:
  // - remplacer les appels aux methodes ::usage par un status adequat.
  // - les functions xx_perform doivent avoir oqmlNode * en premier argument.
  //

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // private utility methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  static oqmlStatus *
  append_perform(oqmlNode *node, Collection *coll, const Oid *oid,
		 Data data, int size, oqmlAtom *a,
		 oqmlAtomList *atom_coll, oqmlAtomList *alist);

  static oqmlStatus *
  oqml_check_coll_type(Collection *coll, oqmlATOMTYPE &t)
  {
    Bool isref;
    eyedblib::int16 dim;
    if (!coll->asCollection() || !coll->getClass()->asCollectionClass())
      return new oqmlStatus("object '%s' is not a collection",
			    coll->getOid().toString());

    const Class *coll_class =
      coll->getClass()->asCollectionClass()->getCollClass(&isref, &dim);
    const char *name = coll_class->getName();
  
    if (isref)
      t = oqmlATOM_OID;
    else if (dim > 1) {
      if (!strcmp(name, char_class_name))
	t = oqmlATOM_STRING;
      else {
	return new oqmlStatus("OQL cannot deal with collection of "
			      "non basic types");
      }
    }
    else if (!strcmp(name, int32_class_name) ||
	     !strcmp(name, int16_class_name) ||
	     !strcmp(name, int64_class_name))
      t = oqmlATOM_INT;
    else if (!strcmp(name, "char"))
      t = oqmlATOM_CHAR;
    else if (!strcmp(name, "float"))
      t = oqmlATOM_DOUBLE;
    else {
      t = oqmlATOM_OBJ;
    }

    return oqmlSuccess;
  }
      
  static oqmlStatus *
  oqml_coll_eval(oqmlNode *node, Database *db, oqmlContext *ctx,
		 oqmlAtom *a, oqmlAtomList *rlist, const Class *&cls,
		 Bool indexed = False)
  {
    oqmlStatus *s;

    if (!a)
      return oqmlStatus::expected(node, "oid or object", "nil");

    if (!OQML_IS_OBJECT(a))
      return oqmlStatus::expected(node, "oid or object", a->type.getString());

    Object *o;
    s = oqmlObjectManager::getObject(node, db, a, o, oqml_False);
    if (s) return s;
    if (!o) {
      return new oqmlStatus(node, "invalid null object");
    }

    cls = o->getClass();
    if (!cls->asCollectionClass()) {
      oqmlObjectManager::releaseObject(o);
      return oqmlStatus::expected(node, "collection", o->getClass()->getName());
    }

    oqmlATOMTYPE t;

    s = oqml_check_coll_type((Collection *)o, t);

    if (s) {
      oqmlObjectManager::releaseObject(o);
      return s;
    }
  
    // 13/09/05
#if 1
    OQML_CHECK_INTR();
#if 1
    Bool is_indexed = (indexed && o->asCollArray() && !ctx->isWhereContext()) ? True : False;
    CollectionIterator iter(o->asCollection(), is_indexed);

    Value v;
    int idx;
    for (int n = 0; iter.next(v); n++) {
      if (is_indexed && !(n & 1)) {
	idx = v.i;
	continue;
      }

      oqmlAtom *x;
      Value::Type type = v.getType();
      if (type == Value::tOid)
	x = new oqmlAtom_oid(*v.oid, (Class *)cls);
      else if (type == Value::tObject) {
	//v.o->trace();
	x = oqmlObjectManager::registerObject(v.o->clone());
      }
      else if (type == Value::tString)
	x = new oqmlAtom_string(v.str);
      else if (type == Value::tInt)
	x = new oqmlAtom_int(v.i);
      else if (type == Value::tLong)
	x = new oqmlAtom_int(v.l);
      else if (type == Value::tChar)
	x = new oqmlAtom_char(v.c);
      else if (type == Value::tDouble)
	x = new oqmlAtom_double(v.d);
      else {
	oqmlObjectManager::releaseObject(o);
	return new oqmlStatus(node, "unsupported value type %s\n",
			      v.getStringType());
      }

      if (is_indexed)
	rlist->append(oqml_make_struct_array(db, ctx, idx, x));
      else
	rlist->append(x);
    }

#else
    ValueArray val_arr;
    Status status = o->asCollection()->getElements(val_arr);
    if (status) {
      oqmlObjectManager::releaseObject(o);
      return new oqmlStatus(node, status);
    }
    OQML_CHECK_INTR();
    int count = val_arr.getCount();
    for (int n = 0; n < count; n++) {
      Value &v = val_arr[n];
      Value::Type type = v.getType();
      if (type == Value::tOid)
	rlist->append(new oqmlAtom_oid(*v.oid, (Class *)cls));
      else if (type == Value::tObject)
	rlist->append(oqmlObjectManager::registerObject(v.o));
      else if (type == Value::tString)
	rlist->append(new oqmlAtom_string(v.str));
      else if (type == Value::tInt)
	rlist->append(new oqmlAtom_int(v.i));
      else if (type == Value::tLong)
	rlist->append(new oqmlAtom_int(v.l));
      else if (type == Value::tChar)
	rlist->append(new oqmlAtom_char(v.c));
      else if (type == Value::tDouble)
	rlist->append(new oqmlAtom_double(v.d));
      else {
	oqmlObjectManager::releaseObject(o);
	return new oqmlStatus(node, "unsupported value type %s\n",
			      v.getStringType());
      }
    }
#endif

    oqmlObjectManager::releaseObject(o);
    return oqmlSuccess;
#else
    Iterator *q = new Iterator(o->asCollection(),
			       (indexed && o->asCollArray() && !ctx->isWhereContext()) ? True : False);
  
    if (q->getStatus()) {
      Status s = q->getStatus();
      delete q;
      return new oqmlStatus(node, s);
    }

    s = oqml_scan(ctx, q,	((CollectionClass *)
				 (o->getClass()))->getCollClass(), rlist, t);

    oqmlObjectManager::releaseObject(o);
    return s;
#endif
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlCollection methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlCollection::oqmlCollection(oqmlNode *_location, oqml_CollSpec *_coll_spec,
				 oqmlNode * _ql) : oqmlNode(oqmlCOLLECTION)
  {
    location = _location;
    coll_spec = _coll_spec;
    ql = _ql;

    if (!location)
      eval_type.type = oqmlATOM_OBJ;
    else
      eval_type.type = oqmlATOM_OID;
  }

  oqmlCollection::~oqmlCollection()
  {
    delete coll_spec;
  }

  oqmlStatus *oqmlCollection::compile(Database *db, oqmlContext *ctx)
  {
    Bool modify = False;
    const char *which = coll_spec->coll_type;

    if (!strcmp(which, "set"))
      what = collset;
    else if (!strcmp(which, "array"))
      what = collarray;
    else if (!strcmp(which, "bag"))
      what = collbag;
    else if (!strcmp(which, "list"))
      return new oqmlStatus(this, "list collection type '%s' is not yet "
			    "implemented");
    else
      return new oqmlStatus(this, "invalid collection type '%s'", which);

    const char *type_spec = coll_spec->type_spec;
    char *ident = coll_spec->ident;

    Bool isref = coll_spec->isref;

    Schema *m = db->getSchema();

    if (type_spec) {
      if (!strcmp(type_spec, "int"))
	cls = m->Int32_Class;
      else
	cls = m->getClass(type_spec);

      if (!cls)
	return new oqmlStatus(this, "unknown class '%s'",
			      type_spec);
    }
  
    if (coll_spec->coll_spec) {
      oqml_CollSpec *cs = coll_spec->coll_spec;
      oqml_CollSpec *coll_spec_arr[64];
      int coll_spec_cnt = 0;

      while (cs) {
	coll_spec_arr[coll_spec_cnt++] = cs;
	cs = cs->coll_spec;
      }

      char coll_typname[256];
      strcpy(coll_typname, coll_spec_arr[coll_spec_cnt-1]->type_spec);

      Class *mcoll = NULL;

      for (int i = coll_spec_cnt-1; i >= 0; i--) {
	cs = coll_spec_arr[i];
	cls = m->getClass(coll_typname);

	if (!cls)
	  return new oqmlStatus(this, "unknown class '%s'",
				type_spec);

	if (!strcmp(cs->coll_type, "set"))
	  mcoll = new CollSetClass(cls, (Bool)cs->isref);
	else if (!strcmp(cs->coll_type, "bag"))
	  mcoll = new CollBagClass(cls, (Bool)cs->isref);
	else if (!strcmp(cs->coll_type, "array"))
	  mcoll = new CollArrayClass(cls, (Bool)cs->isref);
	else if (!strcmp(cs->coll_type, "list"))
	  mcoll = new CollListClass(cls, (Bool)cs->isref);
	else
	  return new oqmlStatus(this, "invalid collection type '%s'",
				cs->coll_type);

	strcpy(coll_typname, mcoll->getName());

	if (!m->getClass(mcoll->getName())) {
	  m->addClass(mcoll);
	  modify = True;
	}
      }

      cls = mcoll;
    }

    atref.cls = 0;
    atref.comp = oqml_False;

    if (isref) {
      atref.type = oqmlATOM_OID;
      atref.cls = cls;
    }
    else if (cls->asInt16Class() || cls->asInt32Class() ||
	     cls->asInt64Class() || cls->asEnumClass())
      atref.type = oqmlATOM_INT;
    else if (!strcmp(cls->getName(), m->Char_Class->getName())||
	     !strcmp(cls->getName(), m->Byte_Class->getName()))
      atref.type = oqmlATOM_CHAR;
    else if (!strcmp(cls->getName(), m->Float_Class->getName()))
      atref.type = oqmlATOM_DOUBLE;
    else if (!strcmp(cls->getName(), m->OidP_Class->getName()))
      atref.type = oqmlATOM_OID;
    else {
      atref.type = oqmlATOM_OBJ;
      atref.cls = cls;
    }
    /*
      return new oqmlStatus(this, "collection type '%s' is neither a reference nor a basic type", type_spec);
    */

    oqmlStatus *s = oqml_get_location(db, ctx, location);
    if (s) return s;
    newdb = db;

    return ql ? ql->compile(db, ctx) : oqmlSuccess;
  }

  oqmlStatus *oqmlCollection::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;
    Collection *coll = 0;

    db = newdb;

    char *ident = coll_spec->ident;
    Bool isref = coll_spec->isref;

    IndexImpl *idximpl = 0;
    CollImpl *collimpl = 0;
    if (coll_spec->impl_hints) {
      CollImpl::Type impl_type = CollImpl::Unknown;
      Status status = IndexImpl::make(db, coll_spec->ishash ?
				      IndexImpl::Hash :
				      IndexImpl::BTree,
				      coll_spec->impl_hints, idximpl);
      if (status) {
	return new oqmlStatus(this, status);
      }
      if (idximpl) {
	impl_type = (CollImpl::Type)idximpl->getType();
      }
      collimpl = new CollImpl(impl_type, idximpl);
    }

    if (what == collset)
      coll = new CollSet(ident, cls, isref, collimpl);
    else if (what == collbag)
      coll = new CollBag(ident, cls, isref, collimpl);
    else if (what == collarray)
      coll = new CollArray(ident, cls, isref, collimpl);
    /*
      else if (what == colllist)
      coll = new CollList(ident, cls, isref, idximpl);
    */

    if (!coll)
      return new oqmlStatus(this, "internal error in collection create");

    Status status = coll->setDatabase(db);
    if (status) {
      coll->release();
      return new oqmlStatus(this, status);
    }

    if (!ql) {
      if (!location) {
	*alist = new oqmlAtomList(oqmlObjectManager::registerObject(coll));
	return oqmlSuccess;
      }

      Status is = coll->realize();

      if (is) {
	coll->release();
	return new oqmlStatus(this, is);
      }
      
      *alist = new oqmlAtomList(new oqmlAtom_oid(coll->getOid()));

      coll->release();
      return oqmlSuccess;
    }

    Status is;
    int ii = 0;

    oqmlAtomList *al;
    s = ql->eval(db, ctx, &al);
    if (s) return s;

    if (!al->cnt)
      return oqmlStatus::expected(this, "atom list", "nil");

    oqmlAtom *a = al->first;

    if (OQML_IS_COLL(a))
      a = OQML_ATOM_COLLVAL(a)->first;
    /*
      return oqmlStatus::expected(this, "collection literal",
      a->type.getString());
    */

    while (a) {
      oqmlAtom *next = a->next;

      if (!atref.cmp(a->type)) {
	coll->release();
	return oqmlStatus::expected(this, &atref, &a->type);
      }
      
      if (isref) {
	if (what == collarray)
	  is = coll->asCollArray()->insertAt(ii, ((oqmlAtom_oid *)a)->oid);
	else
	  is = coll->insert(((oqmlAtom_oid *)a)->oid);
      }
      else if (a->type.type == oqmlATOM_OBJ) {
	OQL_CHECK_OBJ(a);
	if (what == collarray)
	  is = coll->asCollArray()->insertAt(ii, OQML_ATOM_OBJVAL(a));
	else
	  is = coll->insert(OQML_ATOM_OBJVAL(a));
      }
      else {
	  Data val;
	  unsigned char buff[16];
	  Size size;
	  int len;
	      
	  size = sizeof(buff);
	  if (a->getData(buff, &val, size, len, cls)) {
	    Data data = (val ? val : buff);
	    if (what == collarray)
	      is = coll->asCollArray()->insertAt_p(ii, data, size);
	    else
	      is = coll->insert_p(data, False, size);
	  }
	  else
	    is = Success;
	}
	  
      if (is) {
	coll->release();
	return new oqmlStatus(this, is);
      }
      a = next;
      ii++;
    }

    if (!location) {
      *alist = new oqmlAtomList(oqmlObjectManager::registerObject(coll));
      return oqmlSuccess;
    }

    is = coll->realize();

    if (is) {
      coll->release();
      return new oqmlStatus(this, is);
    }

    *alist = new oqmlAtomList(new oqmlAtom_oid(coll->getOid()));

    coll->release();

    return oqmlSuccess;
  }

  void oqmlCollection::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlCollection::isConstant() const
  {
    return oqml_False;
  }

  void
  oqmlCollection::lock()
  {
    oqmlNode::lock();
    if (ql) ql->lock();
    if (location) location->lock();
  }

  void
  oqmlCollection::unlock()
  {
    oqmlNode::unlock();
    if (ql) ql->unlock();
    if (location) location->unlock();
  }

  std::string
  oqmlCollection::toString(void) const
  {
    if (is_statement)
      return std::string("new ") + coll_spec->toString() + "(" +
	(ql ? ql->toString() : std::string("")) + "); ";

    return std::string("(new ") + coll_spec->toString() + "(" +
      (ql ? ql->toString() : std::string("")) + "))";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlContents methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlContents::oqmlContents(oqmlNode * _ql) : oqmlNode(oqmlCONTENTS)
  {
    ql = _ql;
  }

  oqmlContents::~oqmlContents()
  {
  }

  oqmlStatus *oqmlContents::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;
    s = ql->compile(db, ctx);

    if (s)
      return s;

    return oqmlSuccess;
  }

  oqmlAtom *
  oqml_make_struct_array(Database *db, oqmlContext *ctx, int idx, oqmlAtom *x)
  {
    if (ctx->isWhereContext())
      return x;
    oqml_StructAttr *attrs = new oqml_StructAttr[2];
    attrs[0].set("index", new oqmlAtom_int(idx));
    attrs[1].set("value", x);
    return new oqmlAtom_struct(attrs, 2);
  }

  static oqmlAtom *
  make_array_set(Database *db, oqmlContext *ctx, oqmlAtomList *list)
  {
    if (ctx->isWhereContext())
      return new oqmlAtom_set(list);

    oqmlAtomList *rlist = new oqmlAtomList();
    oqmlAtom *x = list->first;

    int idx;
    for (int n = 0; x; n++)
      {
	oqmlAtom *next = x->next;
	if (!(n & 1))
	  idx = OQML_ATOM_INTVAL(x);
	else
	  rlist->append(oqml_make_struct_array(db, ctx, idx, x));

	x = next;
      }

    return new oqmlAtom_set(rlist);
  }

  oqmlStatus *oqmlContents::eval(Database *db, oqmlContext *ctx,
				 oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;
    oqmlAtomList *al;

    s = ql->eval(db, ctx, &al);

    if (s) return s;

    oqmlAtomList *rlist = new oqmlAtomList();

    if (!al->first)
      {
	*alist = new oqmlAtomList();
	return oqmlSuccess;
      }

    const Class *cls;
    s = oqml_coll_eval(this, db, ctx, al->first, rlist, cls, True);

    if (s) return s;

    if (cls->asCollSetClass())
      *alist = new oqmlAtomList(new oqmlAtom_set(rlist, oqml_False)); // added
    // oqml_False the 16/02/01 => optimisation!
    else if (cls->asCollArrayClass())
      //*alist = new oqmlAtomList(make_array_set(db, ctx, rlist));
      *alist = new oqmlAtomList(new oqmlAtom_set(rlist));
    else if (cls->asCollBagClass())
      *alist = new oqmlAtomList(new oqmlAtom_bag(rlist));
    else if (cls->asCollListClass())
      *alist = new oqmlAtomList(new oqmlAtom_list(rlist));
    else
      *alist = new oqmlAtomList(new oqmlAtom_list(rlist));
    return oqmlSuccess;
  }

  void oqmlContents::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlContents::isConstant() const
  {
    return OQMLBOOL(ql->isConstant());
  }

  std::string
  oqmlContents::toString(void) const
  {
    if (is_statement)
      return std::string("contents ") + ql->toString() + "; ";
    return std::string("(contents ") + ql->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlIn methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlIn::oqmlIn(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlIN)
  {
    qleft = _qleft;
    qright = _qright;
    eval_type.type = oqmlATOM_BOOL;
    evalDone = oqml_False;
  }

  oqmlIn::~oqmlIn()
  {
  }

  oqmlStatus *oqmlIn::compile(Database *db, oqmlContext *ctx)
  {
    evalDone = oqml_False;
    oqmlStatus *s;
    s = qleft->compile(db, ctx);

    if (s) return s;

    return qright->compile(db, ctx);
  }

  static oqmlStatus *
  check_coll(oqmlNode *node, Database *db, oqmlAtom *aleft,
	     Collection *&coll, oqmlATOMTYPE &t)
  {
    if (!OQML_IS_OBJECT(aleft))
      return oqmlStatus::expected(node, "oid or object", aleft->type.getString());
  
    oqmlStatus *s;
    s = oqmlObjectManager::getObject(node, db, aleft, (Object *&)coll,
				     oqml_False);

    if (s) return s;

    if (!coll->getClass()->asCollectionClass())
      {
	oqmlObjectManager::releaseObject(coll);
	return oqmlStatus::expected(node, "collection", coll->getClass()->getName());
      }
      
    s = oqml_check_coll_type(coll, t);
  
    if (s)
      {
	oqmlObjectManager::releaseObject(coll);
	return s;
      }

    return oqmlSuccess;
  }

  static oqmlStatus *
  coll_perform(oqmlNode *node, Database *db, oqmlContext *ctx,
	       oqmlAtomList **alist,
	       oqmlNode *qleft, oqmlNode *qright,
	       oqmlStatus *(*perform)(oqmlNode *,Collection *, const Oid *,
				      Data, int, oqmlAtom *,
				      oqmlAtomList *, oqmlAtomList *),
	       oqmlBool write)
  {
    oqmlStatus *s;
    oqmlAtomList *al_left, *al_right;

    *alist = new oqmlAtomList();

    s = qleft->eval(db, ctx, &al_left);
    if (s) return s;

    oqmlAtom *aleft = al_left->first;
    if (!aleft)
      return new oqmlStatus(node, "invalid nil atom: %s",
			    qleft->toString().c_str());

    if (qright) {
      s = qright->eval(db, ctx, &al_right);
      if (s) return s;
    }

#if 0
    if (perform == append_perform) {
      printf("aleft %p aright %p\n", al_left, al_right);
      printf("aleft first %p aright %p\n", al_left->first, al_right->first);
      printf("copying %p %d\n", al_right->first, aleft->as_coll());
      al_right = al_right->copy();
      printf("after copying %p\n", al_right->first);
      /*
      if (aleft->as_coll()) {
	aleft->as_coll()->list->append(al_right, oqml_False);
	return oqmlSuccess;
      }
      */
    }
#endif

    oqmlAtom *aright = (qright ? al_right->first : 0);

    if (aleft->as_coll())
      return (*perform)(node, 0, 0, 0, 0, aright, aleft->as_coll()->list, *alist);

    Collection *coll = 0;
    oqmlATOMTYPE t;

    s = check_coll(node, db, aleft, coll, t);
    if (s) return s;

    if (!qright)
      {
	s = (*perform)(node, coll, 0, 0, 0, 0, 0, *alist);
	if (s)
	  {
	    oqmlObjectManager::releaseObject(coll);
	    return s;
	  }

	return oqmlSuccess;
      }

    if (aright->type.type != t &&
	!(aright->as_null() && t == oqmlATOM_OID))
      {
	oqmlObjectManager::releaseObject(coll);
	return oqmlStatus::expected(node, "oid", 
				    aright->type.getString());
      }
  
    Status status = Success;

    if (t == oqmlATOM_OID)
      {
	Oid oid;
	if (!aright->as_null())
	  oid = OQML_ATOM_OIDVAL(aright);
      
	s = (*perform)(node, coll, &oid, 0, 0,
		       aright, 0, *alist);
	if (s)
	  {
	    oqmlObjectManager::releaseObject(coll);
	    return s;
	  }

	if (write)
	  status = coll->realize();

	oqmlObjectManager::releaseObject(coll);

	if (status)
	  return new oqmlStatus(node, status);

	return oqmlSuccess;
      }

    Data val;
    unsigned char buff[16];
    Size size;
    int len;
  
    size = sizeof(buff);
    oqmlAtomType *at = &aleft->type;
    if (aright->getData(buff, &val, size, len, at->cls))
      {
	s = (*perform)(node, coll, 0, (val ? val : buff), size,
		       aright, 0, *alist);
      
	if (s)
	  {
	    oqmlObjectManager::releaseObject(coll);
	    return s;
	  }
      }
  
    if (write)
      status = coll->realize();

    oqmlObjectManager::releaseObject(coll);

    if (status)
      return new oqmlStatus(node, status);

    return oqmlSuccess;
  }

  static oqmlStatus *
  in_perform(oqmlNode *node, Collection *coll, const Oid *oid,
	     Data data, int size,
	     oqmlAtom *atom, oqmlAtomList *atom_coll, oqmlAtomList *alist)
  {
    Status status;

    if (coll)
      {
	Bool is;

	if (oid)
	  status = coll->isIn(*oid, is);
	else
	  status = coll->isIn_p(data, is);
      
	if (status)
	  return new oqmlStatus(node, status);

	alist->append(new oqmlAtom_bool(OQMLBOOL(is)));
	return oqmlSuccess;
      }

    alist->append(new oqmlAtom_bool(atom_coll->isIn(atom)));
    return oqmlSuccess;
  }

  oqmlStatus *oqmlIn::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    if (evalDone)
      {
	*alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
	return oqmlSuccess;
      }

    return coll_perform(this, db, ctx, alist, qright, qleft, in_perform, oqml_False);
  }

  void oqmlIn::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlIn::isConstant() const
  {
    return oqml_False;
  }

  oqmlBool
  oqmlIn::hasDotIdent(const char *_ident)
  {
    if (qleft->asDot())
      return qleft->hasIdent(_ident);
    return oqml_False;
  }

  static oqmlNode *
  get_invalid_node()
  {
    return (new oqmlAtom_oid("0:0:1:oid"))->toNode();
  }

  oqmlStatus *
  oqmlIn::preEvalSelect(Database *db, oqmlContext *ctx,
			const char *ident, oqmlBool &done,
			unsigned int &cnt, oqmlBool firstPass)
  {
    done = oqml_False;
    cnt = 0;

    if (!hasDotIdent(ident))
      return oqmlSuccess;

    oqmlAtomList *alist;
    oqmlStatus *s = qright->eval(db, ctx, &alist);
    if (s) return s;
      
    oqmlAtomList *list;

    if (!alist->cnt)
      list = 0;
    else if (alist->first->as_coll())
      list = alist->first->as_coll()->list;
    else
      list = alist;

    if (!list || list->cnt <= 1)
      {
	oqmlNode *node = new oqmlEqual
	  (qleft, (list && list->first ? list->first->toNode() :
		   get_invalid_node()));
	s = node->compile(db, ctx);
	if (s) return s;
	s = node->eval(db, ctx, &alist);
	if (!s)
	  {
	    done = oqml_True;
	    evalDone = oqml_True;
	  }
	return s;
      }

    oqmlAtom *x = list->first;
    if (x)
      {
	oqmlNode *nodeOr = new oqmlEqual(qleft, x->toNode());

	x = x->next;
	while (x)
	  {
	    nodeOr = new oqmlLOr
	      (nodeOr, new oqmlEqual(qleft, x->toNode()), oqml_False);
	    x = x->next;
	  }

	s = nodeOr->preEvalSelect(db, ctx, ident, done, cnt, firstPass);
      }

    if (!s)
      evalDone = oqml_True;
    return s;
  }

  std::string
  oqmlIn::toString(void) const
  {
    if (is_statement)
      return std::string("") + qleft->toString() + " in " +
	qright->toString() + "; ";
    return std::string("(") + qleft->toString() + " in " +
      qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlAddTo methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlAddTo::oqmlAddTo(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlADDTO)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlAddTo::~oqmlAddTo()
  {
  }

  oqmlStatus *oqmlAddTo::compile(Database *db, oqmlContext *ctx)
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

  static oqmlStatus *
  insert_perform(oqmlNode *node, Collection *coll, const Oid *oid,
		 Data data, int size, oqmlAtom *a, oqmlAtomList *atom_coll,
		 oqmlAtomList *alist)
  {
    if (coll)
      {
	Status status;
	if (oid)
	  status = coll->insert(*oid);
	else
	  status = coll->insert_p(data, False, size);
      
	if (status)
	  return new oqmlStatus(node, status);
      
	alist->append(a->copy());
	return oqmlSuccess;
      }

    oqmlAtom *x = a->copy();
    atom_coll->append(x);
    alist->append(x);
    return oqmlSuccess;
  }

  oqmlStatus *oqmlAddTo::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    return coll_perform(this, db, ctx, alist, qright, qleft, insert_perform, oqml_True);
  }

  void oqmlAddTo::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlAddTo::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlAddTo::toString(void) const
  {
    if (is_statement)
      return std::string("add ") + qleft->toString() + " to " +
	qright->toString() + "; ";
    return std::string("(add ") + qleft->toString() + " to " +
      qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlSuppressFrom methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlSuppressFrom::oqmlSuppressFrom(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlSUPPRESSFROM)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlSuppressFrom::~oqmlSuppressFrom()
  {
  }

  oqmlStatus *oqmlSuppressFrom::compile(Database *db, oqmlContext *ctx)
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

  static oqmlStatus *
  suppress_perform(oqmlNode *node, Collection *coll, const Oid *oid,
		   Data data, int size, oqmlAtom *a,
		   oqmlAtomList *atom_coll, oqmlAtomList *alist)
  {
    if (coll)
      {
	Status status;
	if (oid)
	  status = coll->suppress(*oid);
	else
	  status = coll->suppress_p(data);
      
	if (status)
	  return new oqmlStatus(node, status);
      
	alist->append(a->copy());
	return oqmlSuccess;
      }

    oqmlStatus *s = atom_coll->suppress(a);
    if (s) return s;
    alist->append(a->copy());
    return oqmlSuccess;
  }

  oqmlStatus *oqmlSuppressFrom::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    return coll_perform(this, db, ctx, alist, qright, qleft, suppress_perform, oqml_True);
  }

  void oqmlSuppressFrom::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlSuppressFrom::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlSuppressFrom::toString(void) const
  {
    if (is_statement)
      return std::string("suppress ") + qleft->toString() + " from " +
	qright->toString() + "; ";
    return std::string("(suppress ") + qleft->toString() + " from " +
      qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlAppend methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlAppend::oqmlAppend(oqmlNode * _qleft, oqmlNode * _qright) : oqmlNode(oqmlAPPEND)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlAppend::~oqmlAppend()
  {
  }

  oqmlStatus *oqmlAppend::compile(Database *db, oqmlContext *ctx)
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

  static oqmlStatus *
  append_perform(oqmlNode *node, Collection *coll, const Oid *oid,
		 Data data, int size, oqmlAtom *a,
		 oqmlAtomList *atom_coll, oqmlAtomList *alist)
  {
    if (coll) {
      if (!coll->asCollArray())
	return new oqmlStatus(node, "cannot append to a non-array collection");
      Status status;
      if (oid)
	status = coll->asCollArray()->append(*oid);
      else
	status = coll->asCollArray()->append_p(data, False, size);
      
      if (status)
	return new oqmlStatus(node, status);
      return oqmlSuccess;
    }

    oqmlAtom *x = a->copy();
    if (atom_coll->append(x, true, x->as_coll() ? true : false))
      return new oqmlStatus(node, "collection cycle detected");

    alist->append(x);
    return oqmlSuccess;
  }

  oqmlStatus *oqmlAppend::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    return coll_perform(this, db, ctx, alist, qright, qleft, append_perform, oqml_True);
  }

  void oqmlAppend::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlAppend::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlAppend::toString(void) const
  {
    if (is_statement)
      return std::string("append ") + qleft->toString() + " to " +
	qright->toString() + "; ";
    return std::string("(append ") + qleft->toString() + " to " +
      qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlEmpty methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlEmpty::oqmlEmpty(oqmlNode * _ql) : oqmlNode(oqmlEMPTY)
  {
    ql = _ql;
  }

  oqmlEmpty::~oqmlEmpty()
  {
  }

  oqmlStatus *oqmlEmpty::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;
    s = ql->compile(db, ctx);

    if (s)
      return s;

    return oqmlSuccess;
  }

  static oqmlStatus *
  empty_perform(oqmlNode *node, Collection *coll, const Oid *,
		Data, int, oqmlAtom *, oqmlAtomList *atom_coll,
		oqmlAtomList *alist)
  {
    if (coll)
      {
	Status status;
	status = coll->empty();
      
	if (status)
	  return new oqmlStatus(node, status);
	return oqmlSuccess;
      }

    atom_coll->empty();
    return oqmlSuccess;
  }

  oqmlStatus *oqmlEmpty::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    return coll_perform(this, db, ctx, alist, ql, 0, empty_perform, oqml_True);
  }

  void oqmlEmpty::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlEmpty::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlEmpty::toString(void) const
  {
    if (is_statement)
      return std::string("empty ") + ql->toString() + "; ";
    return std::string("(empty ") + ql->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlSetInAt methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlSetInAt::oqmlSetInAt(oqmlNode * _qleft, oqmlNode *_qright, oqmlNode *_ql3) : oqmlNode(oqmlSETINAT)
  {
    qleft = _qleft;
    qright = _qright;
    ql3 = _ql3;
  }

  oqmlSetInAt::~oqmlSetInAt()
  {
  }

  oqmlStatus *oqmlSetInAt::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = qleft->compile(db, ctx);

    if (s)
      return s;

    s = qright->compile(db, ctx);

    if (s)
      return s;

    s = ql3->compile(db, ctx);

    if (s)
      return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlSetInAt::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlAtomList *al_left, *al_right, *al3;
    oqmlStatus *s;

    *alist = new oqmlAtomList();

    s = qleft->eval(db, ctx, &al_left);

    if (s)
      return s;

    s = qright->eval(db, ctx, &al_right);

    if (s)
      return s;

    s = ql3->eval(db, ctx, &al3);

    if (s)
      return s;

    oqmlAtom *array = al3->first;

    while (array)
      {
	Status is;
	if (!OQML_IS_OBJECT(array))
	  return oqmlStatus::expected(this, "oid or object",
				      array->type.getString());

	Object *o;
	s = oqmlObjectManager::getObject(this, db, array, o, oqml_False);
	if (s) return s;
	if (!o->getClass()->asCollArrayClass())
	  {
	    oqmlObjectManager::releaseObject(o);
	    return oqmlStatus::expected(this, "array",
					o->getClass()->getName());
	  }

	CollArray *arr = (CollArray *)o;
	oqmlAtom *aind = al_left->first;

	oqmlATOMTYPE t;
	oqml_check_coll_type(arr, t);
      
	while(aind)
	  {
	    oqmlAtom *nextaind = aind->next;
	    if (aind->type.type != oqmlATOM_INT)
	      return oqmlStatus::expected(this, "integer",
					  aind->type.getString());

	    oqmlAtom *e = al_right->first;
	    while (e)
	      {
		oqmlAtom *nexte = e->next;
		if (t == oqmlATOM_OID)
		  {
		    if (e->type.type != oqmlATOM_OID)
		      return oqmlStatus::expected(this, "oid",
						  e->type.getString());

		    is = arr->insertAt(((oqmlAtom_int *)aind)->i, ((oqmlAtom_oid *)e)->oid);
		    if (is)
		      return new oqmlStatus(this, is);
		  }
		else
		  {
		    Data val = 0;
		    unsigned char buff[16];
		    Size size;
		    int len;
		  
		    size = sizeof(buff);
		    if (e->getData(buff, &val, size, len))
		      {
			is = arr->insertAt_p(((oqmlAtom_int *)aind)->i, (val ? val : buff));
			if (is)
			  return new oqmlStatus(this, is);
		      }
		  }

		(*alist)->append(e->copy());
		e = nexte;
	      }
	    aind = nextaind;
	  }

	is = arr->realize();
	if (is)
	  return new oqmlStatus(this, is);

	oqmlObjectManager::releaseObject(o);
	array = array->next;
      }

    return oqmlSuccess;
  }

  void oqmlSetInAt::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlSetInAt::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlSetInAt::toString(void) const
  {
    if (is_statement)
      return std::string("set ") + qleft->toString() + " in " +
	ql3->toString() + " at " + qright->toString() + "; ";
    return std::string("(set ") + qleft->toString() + " in " +
      ql3->toString() + " at " + qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlUnsetInAt methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlUnsetInAt::oqmlUnsetInAt(oqmlNode * _qleft, oqmlNode *_qright) : oqmlNode(oqmlUNSETINAT)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlUnsetInAt::~oqmlUnsetInAt()
  {
  }

  oqmlStatus *oqmlUnsetInAt::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = qleft->compile(db, ctx);

    if (s)
      return s;

    s = qright->compile(db, ctx);

    if (s) return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlUnsetInAt::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlAtomList *al_left, *al_right;
    oqmlStatus *s;

    *alist = new oqmlAtomList();

    s = qleft->eval(db, ctx, &al_left);

    if (s) return s;

    s = qright->eval(db, ctx, &al_right);

    if (s) return s;

    oqmlAtom *array = al_right->first;

    while (array)
      {
	Status is;
	if (!OQML_IS_OBJECT(array))
	  return oqmlStatus::expected(this, "oid or object",
				      array->type.getString());

	Object *o;
	s = oqmlObjectManager::getObject(this, db, array, o, oqml_False);
	if (s) return s;
	if (!o->getClass()->asCollArrayClass())
	  {
	    oqmlObjectManager::releaseObject(o);
	    return oqmlStatus::expected(this, "array",
					o->getClass()->getName());
	  }

	CollArray *arr = (CollArray *)o;
	oqmlAtom *aind = al_left->first;

	while(aind)
	  {
	    if (aind->type.type != oqmlATOM_INT)
	      return oqmlStatus::expected(this, "integer",
					  aind->type.getString());

	    is = arr->suppressAt(((oqmlAtom_int *)aind)->i);
	    if (is)
	      return new oqmlStatus(this, is);

	    aind = aind->next;
	  }

	is = arr->realize();
	if (is)
	  return new oqmlStatus(this, is);

	oqmlObjectManager::releaseObject(o);
	array = array->next;
      }

    return oqmlSuccess;
  }

  void oqmlUnsetInAt::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlUnsetInAt::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlUnsetInAt::toString(void) const
  {
    if (is_statement)
      return std::string("unset in ") + qright->toString() + " at " +
	qleft->toString() + "; ";

    return std::string("(unset in ") + qleft->toString() + " at " +
      qleft->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlElement methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlElement::oqmlElement(oqmlNode * _ql) : oqmlNode(oqmlELEMENT)
  {
    ql = _ql;
  }

  oqmlElement::~oqmlElement()
  {
  }

  oqmlStatus *oqmlElement::compile(Database *db, oqmlContext *ctx)
  {
    return ql->compile(db, ctx);
  }

  oqmlStatus *oqmlElement::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlAtomList *al;
    oqmlStatus *s;

    s = ql->eval(db, ctx, &al);

    if (s) return s;

    if (!al->cnt || !OQML_IS_COLL(al->first))
      return oqmlStatus::expected(this, "collection",
				  (al->first ? al->first->type.getString() :
				   "nil"));

    int cnt = OQML_ATOM_COLLVAL(al->first)->cnt;
    if (cnt != 1)
      return new oqmlStatus(this, "expected collection with one and "
			    "only one element, got %d element%s",
			    cnt, cnt != 1 ? "s" : "");

    *alist = new oqmlAtomList(OQML_ATOM_COLLVAL(al->first)->first);
    return oqmlSuccess;
  }

  void oqmlElement::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlElement::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlElement::toString(void) const
  {
    if (is_statement)
      return std::string("element ") + ql->toString() + "; ";
    return std::string("(element ") + ql->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlElementAt methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlElementAt::oqmlElementAt(oqmlNode * _qleft, oqmlNode *_qright) : oqmlNode(oqmlELEMENTAT)
  {
    qleft = _qleft;
    qright = _qright;
  }

  oqmlElementAt::~oqmlElementAt()
  {
  }

  oqmlStatus *oqmlElementAt::compile(Database *db, oqmlContext *ctx)
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

  oqmlStatus *oqmlElementAt::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlAtomList *al_left, *al_right;
    oqmlStatus *s;

    *alist = new oqmlAtomList();

    s = qleft->eval(db, ctx, &al_left);

    if (s)
      return s;

    s = qright->eval(db, ctx, &al_right);

    if (s)
      return s;

    oqmlAtom *array = al_right->first;

    while (array)
      {
	Status is;
	if (!OQML_IS_OBJECT(array))
	  return oqmlStatus::expected(this, "oid or object",
				      array->type.getString());

	Object *o;
	s = oqmlObjectManager::getObject(this, db, array, o, oqml_False);
	if (s) return s;
	if (!o->getClass()->asCollArrayClass())
	  {
	    oqmlObjectManager::releaseObject(o);
	    return oqmlStatus::expected(this, "array",
					o->getClass()->getName());
	  }

	CollArray *arr = (CollArray *)o;
	oqmlAtom *aind = al_left->first;

	oqmlATOMTYPE t;
	oqml_check_coll_type(arr, t);
      
	while(aind)
	  {
	    oqmlAtom *next = aind->next;
	    if (aind->type.type != oqmlATOM_INT)
	      return oqmlStatus::expected(this, "integer",
					  aind->type.getString());

	    if (t == oqmlATOM_OID)
	      {
		Oid xoid;
		is = arr->retrieveAt(((oqmlAtom_int *)aind)->i, xoid);
		if (is)
		  return new oqmlStatus(this, is);
		(*alist)->append(new oqmlAtom_oid(xoid));
	      }
	    else
	      {
		unsigned char buff[256];
		is = arr->retrieveAt_p(((oqmlAtom_int *)aind)->i, buff);
		if (is)
		  return new oqmlStatus(this, is);
		switch(t)
		  {
		  case oqmlATOM_INT:
		    {
		      int i;
		      memcpy(&i, buff, sizeof(int));
		      (*alist)->append(new oqmlAtom_int(i));
		      break;
		    }
		  case oqmlATOM_DOUBLE:
		    {
		      double d;
		      memcpy(&d, buff, sizeof(double));
		      (*alist)->append(new oqmlAtom_double(d));
		      break;
		    }
		  case oqmlATOM_STRING:
		    {
		      (*alist)->append(new oqmlAtom_string((char *)buff));
		      break;
		    }

		  default:
		    return new oqmlStatus(this, "type #%d not supported", t);
		  }
	      }

	    aind = next;
	  }

	oqmlObjectManager::releaseObject(o);
	array = array->next;
      }

    return oqmlSuccess;
  }

  void oqmlElementAt::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlElementAt::isConstant() const
  {
    return oqml_False;
  }

  std::string
  oqmlElementAt::toString(void) const
  {
    if (is_statement)
      return std::string("element at ") + qleft->toString() + " in " +
	qright->toString() + "; ";
    return std::string("(element at ") + qleft->toString() + " in " +
      qright->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqml_Interval methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqml_Interval::oqml_Interval(oqmlNode *_from, oqmlNode *_to) :
    from(_from), to(_to)
  {
    from_base = 0;
    to_base = 0;
  }

  oqmlStatus *
  oqml_Interval::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    if (from)
      {
	s = from->compile(db, ctx);
	if (s) return s;
      }

    if (to)
      return to->compile(db, ctx);

    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_Interval::evalNode(Database *db, oqmlContext *ctx,
			  oqmlNode *node, unsigned int &base)
  {
    oqmlStatus *s;
    if (!node)
      {
	base = oqml_INFINITE;
	return oqmlSuccess;
      }

    oqmlAtomList *al;
    s = node->eval(db, ctx, &al);
    if (s) return s;

    if (!al->first)
      return new oqmlStatus("interval %s: integer expected, got nil",
			    toString().c_str());

    if (!OQML_IS_INT(al->first))
      return new oqmlStatus("interval %s: integer expected, got %s",
			    toString().c_str(),
			    al->first->type.getString());

    base = OQML_ATOM_INTVAL(al->first);
    return oqmlSuccess;
  }

  oqmlBool
  oqml_Interval::isAll() const
  {
    return OQMLBOOL(from_base == oqml_INFINITE && to_base == oqml_INFINITE);
  }

  oqml_Interval::Status
  oqml_Interval::isIn(unsigned int count, unsigned int max) const
  {
    if (max == oqml_INFINITE)
      {
	if (count >= from_base && count <= to_base)
	  return Success;

	return Error;
      }

    if (count > to_base)
      return Error;

    if (count >= from_base && max <= to_base)
      return Success;

    return Continue;
  }

  oqmlStatus *
  oqml_Interval::evalNodes(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = evalNode(db, ctx, from, from_base);
    if (s) return s;

    return evalNode(db, ctx, to, to_base);
  }

  std::string
  oqml_Interval::toString() const
  {
    if (!from && !to) return "all";

    std::string s = "<";

    if (from) s += from->toString();
    else      s += "$";

    if (to != from)
      {
	s += ":";
	if (to) s += to->toString();
	else    s += "$";
      }

    return s + ">";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlFor methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlFor::oqmlFor(oqml_Interval *_interval, const char *_ident,
		   oqmlNode * _qcoll, oqmlNode * _qpred,
		   oqmlBool _exists) : oqmlNode(_exists ? oqmlEXISTS : oqmlFOR)
  {
    interval = _interval;
    qcoll = _qcoll;
    qpred = _qpred;
    ident = strdup(_ident);
    exists = _exists;
    eval_type.type = oqmlATOM_BOOL;
  }

  oqmlFor::~oqmlFor()
  {
    free(ident);
  }

  oqmlStatus *oqmlFor::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s;

    s = interval->compile(db, ctx);
    if (s) return s;

    s = qcoll->compile(db, ctx);
    if (s) return s;

    oqmlAtomType nodeType;
    ctx->pushSymbol(ident, &nodeType, 0, oqml_False);
    s = qpred->compile(db, ctx);
    ctx->popSymbol(ident, oqml_False);

    if (s) return s;

    return oqmlSuccess;
  }

  oqmlStatus *oqmlFor::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    oqmlStatus *s;

    s = interval->evalNodes(db, ctx);
    if (s) return s;

    oqmlAtomList *al;
    s = qcoll->eval(db, ctx, &al);
    if (s) return s;

    oqmlAtomList *rlist = new oqmlAtomList();
    const Class *cls;
    s = oqml_coll_eval(this, db, ctx, al->first, rlist, cls);
    if (s) return s;

    oqmlBool isAll = interval->isAll();

    unsigned int count = 0;
    unsigned int max = rlist->cnt;
    oqmlAtom *a = rlist->first;
    while(a)
      {
	oqmlAtomList *predList;
	oqmlAtom *next = a->next;

	ctx->pushSymbol(ident, &a->type, a, oqml_False);
	s = qpred->eval(db, ctx, &predList);
	ctx->popSymbol(ident, oqml_False);
	if (s) return s;

	if (!predList->first)
	  return new oqmlStatus(this,
				"condition: boolean expected, got nil");

	if (!OQML_IS_BOOL(predList->first))
	  return new oqmlStatus(this,
				"condition: boolean expected, got %s",
				predList->first->type.getString());

	if (OQML_ATOM_BOOLVAL(predList->first))
	  {
	    oqml_Interval::Status is = interval->isIn(++count, max);
	    if (is == oqml_Interval::Error)
	      {
		*alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));
		return oqmlSuccess;
	      }

	    if (is == oqml_Interval::Success)
	      {
		*alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
		return oqmlSuccess;
	      }
	  }
	else if (isAll)
	  {
	    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));
	    return oqmlSuccess;
	  }

	a = next;
      }

    if (isAll && count == max)
      {
	*alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
	return oqmlSuccess;
      }

    oqml_Interval::Status is = interval->isIn(count, oqml_INFINITE);

    if (is == oqml_Interval::Success)
      *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
    else
      *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));

    return oqmlSuccess;
  }

  void oqmlFor::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlFor::isConstant() const
  {
    return oqml_False;
  }

  void
  oqmlFor::lock()
  {
    oqmlNode::lock();
    interval->lock();
    qcoll->lock();
    qpred->lock();
  }

  void
  oqmlFor::unlock()
  {
    oqmlNode::unlock();
    interval->unlock();
    qcoll->unlock();
    qpred->unlock();
  }

  std::string
  oqmlFor::toString(void) const
  {
    if (exists)
      {
	if (is_statement)
	  return std::string("exists ") + ident + " in " +
	    qcoll->toString() + ": " + qpred->toString() + "; ";
	return std::string("(exists ") + ident + " in " +
	  qcoll->toString() + ": " + qpred->toString() + ")";
      }

    if (is_statement)
      return std::string("for ") + interval->toString() + " " + ident + " in " +
	qcoll->toString() + ": " + qpred->toString() + "; ";
    return std::string("(for ") + interval->toString() + " " + ident + " in " +
      qcoll->toString() + ": " + qpred->toString() + ")";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqml_CollSpec methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqml_CollSpec::oqml_CollSpec(const char *_coll_type, const char *_type_spec, oqmlBool _isref, const char *_ident, oqmlBool _ishash, const char *_impl_hints)
  {
    coll_type = strdup(_coll_type);
    type_spec = strdup(_type_spec);
    isref = (_isref ? True : False);
    ishash = (_ishash ? True : False);
    ident = strdup(_ident);
    impl_hints = _impl_hints ? strdup(_impl_hints) : 0;
    coll_spec = NULL;
  }

  oqml_CollSpec::oqml_CollSpec(const char *_coll_type, oqml_CollSpec *_coll_spec, oqmlBool _isref, const char *_ident, oqmlBool _ishash, const char *_impl_hints)
  {
    coll_type = strdup(_coll_type);
    isref = (_isref ? True : False);
    ishash = (_ishash ? True : False);
    ident = strdup(_ident);
    impl_hints = _impl_hints ? strdup(_impl_hints) : 0;
    type_spec = NULL;
    coll_spec = _coll_spec;
  }

  std::string
  oqml_CollSpec::toString(void) const
  {
    return std::string(coll_type) + "<" + (type_spec ? type_spec : "??") +
      (isref ? "*" : "") +
      (ident ? std::string(", \"") + ident + "\"" : std::string(", \"\"")) +
      (ishash ? ", hash" : ", btree") +
      (impl_hints ? std::string(", \"") + impl_hints + "\"":  std::string(", """)) +
      ">";
  }

  oqml_CollSpec::~oqml_CollSpec()
  {
    free(coll_type);
    free(type_spec);
    free(ident);
    free(impl_hints);
  }
}
