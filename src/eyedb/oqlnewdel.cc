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
#include <eyedb/oqlctb.h>

#include "oql_p.h"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// static utility functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace eyedb {

static oqmlStatus *
oqmlGetValidAtomType(oqmlNode *node, const Attribute *attr,
		     int ndims, oqmlAtomType &at)
{
  const Class *cls = attr->getClass();
  int attr_ndims = attr->getTypeModifier().ndims;
  const char *name = cls->getName();

  int mode = attr_ndims - ndims;

  if (mode < 0)
    return new oqmlStatus(node, "invalid assignation for attribute '%s': to many array specifiers", attr->getName());

  if (mode > 1)
    return new oqmlStatus(node, "invalid assignation for attribute '%s': not enough array specifiers", attr->getName());

  if (attr->isIndirect()) {
    at.type = oqmlATOM_OID;
    at.comp = oqml_False;
  }
  else if ((!strcmp(name, char_class_name) || !strcmp(name, "byte")) &&
	   mode) {
    at.type = oqmlATOM_STRING;
    at.comp = oqml_True;
  }
  else {
    if (cls->asInt32Class() || cls->asInt16Class() || cls->asInt64Class())
      at.type = oqmlATOM_INT;
    else if (!strcmp(name, char_class_name) || !strcmp(name, "byte"))
      at.type = oqmlATOM_CHAR;
    else if (!strcmp(name, "float") || !strcmp(name, "double"))
      at.type = oqmlATOM_DOUBLE;
    else if (!strcmp(name, "oid"))
      at.type = oqmlATOM_OID;
    else if (cls->asEnumClass())
      at.type = oqmlATOM_INT;
    else
      at.type = oqmlATOM_OBJ; // changed from OID to OBJ the 28/12/00
    at.comp = oqml_False;
  }
  /*
    return new oqmlStatus(node, "invalid assignation for attribute '%s'",
			  attr->getName());
  */

  return oqmlSuccess;
}

static oqmlStatus *
oqmlCheckType(oqmlNode *node, Database *db,
	      const Attribute *attr,
	      const oqmlAtomType *at, int ndims)
{
  if (at->type == oqmlATOM_UNKNOWN_TYPE)
    return oqmlSuccess;

  oqmlAtomType vat;
  oqmlStatus *s;
  s = oqmlGetValidAtomType(node, attr, ndims, vat);

  if (s)
    return s;

  if (vat.type == at->type)
    return oqmlSuccess;

  /*
  // added the 19/06/01
  if ((vat.type == oqmlATOM_OID && at->type == oqmlATOM_OBJ) ||
      (vat.type == oqmlATOM_OBJ && at->type == oqmlATOM_OID))
    return oqmlSuccess;
  */

  return oqmlStatus::expected(node, &vat, (oqmlAtomType *)at);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlNew methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlNew::oqmlNew(oqmlNode *_location, const char *_ident,
		 oqml_IdentList * _ident_list) : oqmlNode(oqmlNEW)
{
  location = _location;
  ident = strdup(_ident);
  ident_list = _ident_list;
  ql = 0;
  comp = 0;
  if (!location)
    eval_type.type = oqmlATOM_OBJ;
  else
    eval_type.type = oqmlATOM_OID;
  quoted_odl = 0;
}

oqmlNew::oqmlNew(oqmlNode *_location, const char *_ident, oqmlNode *_ql) :
  oqmlNode(oqmlNEW)
{
  location = _location;
  ident = strdup(_ident);
  ident_list = 0;
  ql = _ql;
  comp = 0;
  eval_type.type = oqmlATOM_OID;
  cst_atom = 0;
  quoted_odl = 0;
}

oqmlNew::oqmlNew(oqmlNode *_location, const char *_quoted_odl) :
  oqmlNode(oqmlNEW)
{
  location = _location;
  ident = 0;
  ident_list = 0;
  ql = 0;
  comp = 0;
  eval_type.type = oqmlATOM_LIST;
  cst_atom = 0;
  quoted_odl = strdup(_quoted_odl);
}

void
oqmlNew::lock()
{
  oqmlNode::lock();
  if (location)
    location->lock();
  if (ident_list)
    ident_list->lock();
}

void
oqmlNew::unlock()
{
  oqmlNode::unlock();
  if (location)
    location->unlock();
  if (ident_list)
    ident_list->unlock();
}

oqmlNew::~oqmlNew()
{
  free(ident);
  free(quoted_odl);
  delete ident_list;
  delete comp;
}

#define IS_NUM(AT) ((AT).type == oqmlATOM_DOUBLE || \
		    (AT).type == oqmlATOM_INT || \
		    (AT).type == oqmlATOM_CHAR)
oqmlStatus *
oqmlNew::compileNode(Database *db, oqmlContext *ctx, const Class *cls)
{
  if (!cls->asBasicClass() && !cls->asEnumClass())
    return new oqmlStatus(this, "class '%s' is not a basic class", ident);

  _class = (Class *)cls;
  if (cls->asEnumClass())
    {
      if (ql->getType() == oqmlIDENT)
	{
	  const char *name = ((oqmlIdent *)ql)->getName();
	  const EnumItem *en = cls->asEnumClass()->
	    getEnumItemFromName(name);

	  if (!en)
	    return new oqmlStatus(this,
				  "unknown value '%s' for enum class '%s'",
				  name, ident);
	  cst_atom = new oqmlAtom_int(en->getValue());
	}
    }
      
  oqmlStatus *s = ql->compile(db, ctx);
  if (s) return s;

  oqmlAtomType at;
  ql->evalType(db, ctx, &at);

  if (at.type == oqmlATOM_UNKNOWN_TYPE)
    return oqmlSuccess;

#if 0
  if (cls->asFloatClass())
    {
      if (!IS_NUM(at))
	return oqmlStatus::expected(this, "float", at.getString());
    }
  else if (cls->asByteClass() || cls->asCharClass())
    {
      if (!IS_NUM(at))
	return oqmlStatus::expected(this, "char", at.getString());
    }
  else if (cls->asInt32Class() || cls->asInt64Class() || cls->asInt16Class())
    {
      if (!IS_NUM(at))
	return oqmlStatus::expected(this, "integer", at.getString());
    }
  else if (cls->asOidClass())
    {
      if (at.type != oqmlATOM_OID)
	return oqmlStatus::expected(this, "oid", at.getString());
    }
  else
    return new oqmlStatus(this, "class '%s' not supported", cls->getName());
#endif
  
  if (ql->isConstant())
    {
      oqmlAtomList *al;
      s = ql->eval(db, ctx, &al);
      if (s)
	return s;
      if (al->cnt != 1)
	return new oqmlStatus(this, "constant expected");
      cst_atom = al->first->copy();
    }
  
  return oqmlSuccess;
}

oqmlStatus *
oqmlNew::compileIdent(Database *db, oqmlContext *ctx, const Class *cls,
		      oqmlNode *left, int n, int &ndims)
{
  comp->attr[n] = (Attribute *)cls->getAttribute
    ( ((oqmlIdent *)left)->getName() );
  if (!comp->attr[n])
    return new oqmlStatus(this, "cannot find attribute '%s' in class '%s'",
			  ((oqmlIdent *)left)->getName(),
			  cls->getName());
  comp->attrname[n] = strdup(((oqmlIdent *)left)->getName());
  return oqmlSuccess;
}

oqmlStatus *
oqmlNew::compileDot(Database *db, oqmlContext *ctx, const Class *cls,
		    oqmlNode *left, int n, int &ndims)
{
  if (ctx->getDotContext())
    return new oqmlStatus(this, "internal error #110");
  
  comp->dot_ctx[n] = new oqmlDotContext(0, cls);
  oqmlDotContext *dot_ctx = comp->dot_ctx[n];
  
  ctx->setDotContext(dot_ctx);
	  
  oqmlStatus *s = left->compile(db, ctx);
  if (s) return s;

  s = left->asDot()->check(db, dot_ctx);
  if (s) return s;

  ctx->setDotContext(0);
  oqmlDotDesc *d = &dot_ctx->desc[dot_ctx->count - 1];
  
  comp->attr[n] = (Attribute *)d->attr;

  if (!comp->attr[n])
    return new oqmlStatus(this, "internal error #111");
  
  comp->attrname[n] = strdup(d->attrname);
  ndims = (d->array ? d->array->count : 0);
  return oqmlSuccess;
}

oqmlStatus *
oqmlNew::compileArray(Database *db, oqmlContext *ctx, 
		    const Class *cls, oqmlNode *left, int n, int &ndims)
{
  comp->dot_ctx[n] = new oqmlDotContext(0, cls);
  
  oqmlArray *array = (oqmlArray *)left;
  oqmlNode *xident = array->getLeft();
  
  if (xident->getType() != oqmlIDENT)
    return new oqmlStatus(this, "left part of array is not an ident");
  
  comp->attr[n] = (Attribute *)
    cls->getAttribute
    (((oqmlIdent *)xident)->getName() );

  if (!comp->attr[n])
    return new oqmlStatus(this, "compilation array error in new operator");
  
  comp->attrname[n] = strdup(((oqmlIdent *)xident)->getName());
  comp->list[n] = array->getArrayList();
  ndims = comp->list[n]->count;
  return oqmlSuccess;
}

oqmlStatus *oqmlNew::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s = oqml_get_location(db, ctx, location);
  if (s) return s;
  newdb = db;

  if (!db->isInTransaction())
    return new oqmlStatus(this, "must be done within the scope of "
			  "a transaction in database '%s'", db->getName());

  Class *cls;

  cls = db->getSchema()->getClass(ident);

  if (!cls)
    return new oqmlStatus(this, "invalid class '%s'", ident);

  if (ql)
    return compileNode(db, ctx, cls);

  comp = new newCompile(cls, (ident_list ? ident_list->cnt : 0));

  oqml_IdentLink *l = (ident_list ? ident_list->first : 0);

  if (!location)
    eval_type.type = oqmlATOM_OBJ;
  else
    eval_type.type = oqmlATOM_OID;

  eval_type.cls = cls;

  int n = 0;

  while (l)
    {
      oqmlNode *left = l->left;
      oqmlTYPE xtype = left->getType();
      int ndims = 0;

      if (xtype == oqmlIDENT)
	{
	  s = compileIdent(db, ctx, cls, left, n, ndims);
	  if (s) return s;
	}
      else if (xtype == oqmlDOT)
	{
	  s = compileDot(db, ctx, cls, left, n, ndims);
	  if (s) return s;
	}
      else if (xtype == oqmlARRAY)
	{
	  s = compileArray(db, ctx, cls, left, n, ndims);
	  if (s) return s;
	}
      else
	return new oqmlStatus(this, "left part is not a left value");

      s = l->ql->compile(db, ctx);
      if (s)
	return s;

      oqmlAtomType at;
      l->ql->evalType(db, ctx, &at);

      if (s = oqmlCheckType(this, db, comp->attr[n], &at, ndims))
	return s;

      l = l->next;
      n++;
    }

  return oqmlSuccess;
}

oqmlStatus *
oqmlNew::makeAtom(Database *db, oqmlContext *ctx, Object *o,
		  oqmlAtom *&r)
{
  if (!location)
    {
      r = oqmlObjectManager::registerObject(o);
      return oqmlSuccess;
    }

  o->setDatabase(db);
  Status is = o->realize();
  if (is)
    {
      o->release();
      return new oqmlStatus(this, is);
    }

  r = new oqmlAtom_oid(o->getOid());
  o->release();
  return oqmlSuccess;
}

#define oqmlDEF_GET_NUM(TYP1, TYP2) \
oqmlStatus * \
get##TYP1##Val(oqmlNode *node, oqmlAtom *x, unsigned char buff[]) \
{ \
  TYP2 i; \
  if (x->as_int()) \
    i = OQML_ATOM_INTVAL(x); \
  else if (x->as_double()) \
    i = OQML_ATOM_DBLVAL(x); \
  else if (x->as_char()) \
    i = OQML_ATOM_CHARVAL(x); \
  else \
    return new oqmlStatus(node, "unexpected '%s' atom type", \
			  x->type.getString()); \
  mcp(buff, &i, sizeof(i)); \
  return oqmlSuccess;\
}

#define oqmlGET_VAL(TYP) \
 (_class->as##TYP##Class()) \
  { \
    s = get##TYP##Val(this, atom, buff); \
    if (s) return s; \
  }

oqmlDEF_GET_NUM(Int32, eyedblib::int32)
oqmlDEF_GET_NUM(Int64, eyedblib::int64)
oqmlDEF_GET_NUM(Int16, eyedblib::int16)
oqmlDEF_GET_NUM(Char,  char)
oqmlDEF_GET_NUM(Byte,  char)
oqmlDEF_GET_NUM(Float, double)

oqmlStatus *
getOidVal(oqmlNode *node, oqmlAtom *x, unsigned char buff[])
{
  if (!x->as_oid())
    return new oqmlStatus(node, "unexpected '%s' atom type",
			  x->type.getString());

  mcp(buff, &OQML_ATOM_OIDVAL(x), sizeof(Oid));
  return oqmlSuccess;
}

oqmlStatus *oqmlNew::evalNode(Database *db, oqmlContext *ctx,
			      oqmlAtomList **alist)
{
  oqmlAtom *atom;
  oqmlStatus *s;
  oqmlAtom *r;

  if (cst_atom)
    atom = cst_atom;
  else
    {
      oqmlAtomList *al;
      s = ql->eval(db, ctx, &al);
      if (s)
	return s;
      if (al->cnt != 1)
	return new oqmlStatus(this, "constant expected");
      atom = al->first->copy();
    }

  Object *o = _class->newObj();
  
  unsigned char buff[32];
  
  if oqmlGET_VAL(Int32)
  else if oqmlGET_VAL(Int64)
  else if oqmlGET_VAL(Int16)
  else if oqmlGET_VAL(Float)
  else if oqmlGET_VAL(Char)
  else if oqmlGET_VAL(Byte)
  else if oqmlGET_VAL(Oid)
  else
    return new oqmlStatus(this, "class '%s' not supported", _class->getName());

  Status is = o->setValue(buff);
  if (is)
    {
      o->release();
      return new oqmlStatus(this, is);
    }
      
  s = makeAtom(db, ctx, o, r);
  if (s) return s;
  (*alist)->append(r);

  return oqmlSuccess;
}

oqmlStatus *
oqmlNew::evalItem(Database *db, oqmlContext *ctx,
		  Agregat *o, oqml_IdentLink *l, int n,
		  oqmlBool &stop, oqmlAtomList **alist)
{
  oqmlAtomList *at;
  oqmlStatus *s = l->ql->eval(db, ctx, &at);
  oqmlTYPE xtype = l->left->getType();

  if (s)
    {
      o->release();
      return s;
    }
  
  stop = oqml_False;
  oqmlAtom *value = at->first;
  
  if (xtype == oqmlDOT)
    {
      oqmlAtomList *dummy = new oqmlAtomList;
      s = comp->dot_ctx[n]->eval_perform(db, ctx, o, value, 1, &dummy);
      if (s)
	{
	  o->release();
	  return s;
	}
      return oqmlSuccess;
    }

  Data val;
  unsigned char data[16];
  Size size;
  int len;
  Bool isIndirect = (comp->attr[n]->isIndirect());
	  
  size = sizeof(data);
  if (value && value->getData(data, &val, size, len,
			      comp->attr[n]->getClass()))
    {
      int nb, s_ind, e_ind;
	      
      if (xtype == oqmlIDENT)
	{
	  s_ind = e_ind = 0;
	  nb = len;
	}
      else if (xtype == oqmlARRAY)
	{
	  ctx->setDotContext(comp->dot_ctx[n]);
	  oqml_ArrayList *list = ((oqmlArray *)l->left)->
	    getArrayList();
	  s = list->eval(this, db, ctx,
			 comp->attr[n]->getClassOwner()->getName(),
			 comp->attr[n]->getName(),
			 &comp->attr[n]->getTypeModifier(),
			 &s_ind, &e_ind, oqml_False);
	  if (s)
	    {
	      o->release();
	      return s;
	    }
	  nb = 1;
	  ctx->setDotContext(0);
	}
      else
	assert(0);
	      
      nb = len; // really??

      // added the 2/07/01 because CheckType has not been done during
      // compilation if atom was of unknown type!
      s = oqmlCheckType(this, db, comp->attr[n], &value->type, 0);
      if (s) return s;

      Status is;

      oqmlBool enough;
      if (isIndirect) {
	int tnb = e_ind - s_ind + 1;
	is = oqml_check_vardim(comp->attr[n], o, OQMLBOOL(value),
			       enough,
			       s_ind, tnb,
			       comp->attr[n]->getTypeModifier().pdims,
			       OQMLCOMPLETE(value));
      }
      else
	is = oqml_check_vardim(comp->attr[n], o, OQMLBOOL(value),
			       enough,
			       s_ind, nb,
			       comp->attr[n]->getTypeModifier().pdims,
			       OQMLCOMPLETE(value));

      if (is)
	{
	  o->release();
	  return new oqmlStatus(this, is);
	}

      if (!enough)
	{
	  if (value)
	    return oqmlSuccess;
	  else
	    {
	      stop = oqml_True;
	      return oqmlSuccess;
	    }
	}

      for (int ind = s_ind; ind <= e_ind; ind++)
	{
	  OQML_CHECK_INTR();
	  Status status;
	  if (isIndirect)
	    status = o->setItemOid(comp->attr[n], (Oid *)data,
				   1, ind);
	  else
	    status = o->setItemValue(comp->attr[n], (val ? val : data),
				     nb, ind);
	  if (status)
	    {
	      o->release();
	      return new oqmlStatus(this, status);
	    }
	}
      return oqmlSuccess;
    }

  o->release();
  return new oqmlStatus(this, "error null data");
}

static Agregat *
make_new_obj(Database *db, Class *cls)
{
  // 5/02/01: I think that as there is the consapp mechanism
  // the following code for connection and database classes are
  // not still necessary!

  const char *name = cls->getName();
  char c = name[0];

  if (c == 'c')
    {
      if (!strcmp(name, "connection"))
	return new OqlCtbConnection(db);
    }
  else if (c == 'd')
    {
      if (!strcmp(name, "database"))
	return new OqlCtbDatabase(db);
    }

  // added the 5/02/01
  Database::consapp_t consapp = db->getConsApp(cls);
  if (consapp)
    return (Agregat *)consapp(cls, 0);

  return (Agregat*)cls->newObj();
}

oqmlStatus *oqmlNew::eval(Database *db, oqmlContext *ctx,
			  oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  db = newdb;

  *alist = new oqmlAtomList();

  // added the 3/05/01
  db = newdb;

  if (ql)
    return evalNode(db, ctx, alist);

  oqml_IdentLink *l = (ident_list ? ident_list->first : 0);

  Agregat *o = make_new_obj(db, comp->cls);

  if (!o)
    {
      (*alist)->append(new oqmlAtom_null());
      return oqmlSuccess;
    }

  Status status = o->setDatabase(db);
  if (status)
    {
      o->release();
      return new oqmlStatus(this, status);
    }

  int n = 0;

  while (l)
    {
      oqmlBool stop;
      s = evalItem(db, ctx, o, l, n, stop, alist);
      if (s) return s;
      if (stop) break;

      l = l->next;
      n++;
    }

  oqmlAtom *r;
  s = makeAtom(db, ctx, o, r);
  if (s) return s;
  (*alist)->append(r);
  return oqmlSuccess;
}

void oqmlNew::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlNew::isConstant() const
{
  return oqml_False;
}

std::string
oqmlNew::toString(void) const
{
  std::string s = std::string("new") +
    (location ? std::string("<") + location->toString() + "> " : std::string(" ")) +
    ident + "(";
  
  if (ident_list)
    {
      oqml_IdentLink *l = ident_list->first;
      for (int n = 0; l; n++)
	{
	  if (n) s += ",";
	  s += l->left->toString() + ":" + l->ql->toString();
	  l = l->next;
	}
    }

  return s + ")" + oqml_isstat();
}

oqmlNew::newCompile::newCompile(Class *_class, int n)
{
  cls = _class;
  cnt = n;
  attr  = idbNewVect(Attribute *, n);
  attrname = idbNewVect(char *, n);
  list = idbNewVect(oqml_ArrayList *, n);
  dot_ctx = idbNewVect(oqmlDotContext *, n);
}

oqmlNew::newCompile::~newCompile()
{
  idbFreeIndVect(dot_ctx, cnt);
  for (int i = 0; i < cnt; i++)
    free(attrname[i]);
  free(attrname);

  free(attr);
  free(list);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlDelete methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

oqmlDelete::oqmlDelete(oqmlNode * _ql) : oqmlNode(oqmlDELETE)
{
  ql = _ql;
}

oqmlDelete::~oqmlDelete()
{
}

oqmlStatus *oqmlDelete::compile(Database *db, oqmlContext *ctx)
{
  oqmlStatus *s;
  s = ql->compile(db, ctx);

  if (s)
    return s;

  ql->evalType(db, ctx, &eval_type);

  if (eval_type.type != oqmlATOM_IDENT && eval_type.type != oqmlATOM_OID &&
      eval_type.type != oqmlATOM_UNKNOWN_TYPE)
    return oqmlStatus::expected(this, "oid", eval_type.getString());

  return oqmlSuccess;
}

oqmlStatus *oqmlDelete::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
{
  oqmlStatus *s;
  oqmlAtomList *al;

  s = ql->eval(db, ctx, &al);

  if (s) return s;

  if (al->cnt == 0)
    return oqmlStatus::expected(this, "oid", "nil");

  if (al->cnt > 1)
    return new oqmlStatus(this, "internal error #112");

  oqmlAtom *a = al->first;
  
  if (!OQML_IS_OID(a))
    return oqmlStatus::expected(this, "oid", a->type.getString());

  Status is = db->removeObject(&OQML_ATOM_OIDVAL(a));

  if (is)
    return new oqmlStatus(this, is);

  *alist = new oqmlAtomList(a);

  return oqmlSuccess;
}

void oqmlDelete::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
{
  *at = eval_type;
}

oqmlBool oqmlDelete::isConstant() const
{
  return oqml_False;
}

std::string
oqmlDelete::toString(void) const
{
  if (is_statement)
    return std::string("delete ") + ql->toString() + "; ";
  return std::string("(delete ") + ql->toString() + ")";
}
}
