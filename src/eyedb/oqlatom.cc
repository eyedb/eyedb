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


#include <math.h>
#include <ctype.h>
#include "oql_p.h"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqmlString operator methods
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace eyedb {

  oqmlString::oqmlString(const char * _s) : oqmlNode(oqmlSTRING)
  {
    s = strdup(_s);
    eval_type.type = oqmlATOM_STRING;
    eval_type.cls = 0;
    eval_type.comp = oqml_True;
  }

  oqmlString::~oqmlString()
  {
    free(s);
  }

  oqmlStatus *oqmlString::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlString::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_string(s));
    return oqmlSuccess;
  }

  void oqmlString::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlString::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlString::toString(void) const
  {
    return std::string("\"") + s + "\"" + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlInt operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlInt::oqmlInt(eyedblib::int64 _i) : oqmlNode(oqmlINT)
  {
    i = _i;
    eval_type.type = oqmlATOM_INT;
    ql = 0;
  }

  oqmlInt::oqmlInt(oqmlNode *_ql) : oqmlNode(oqmlINT)
  {
    ql = _ql;
    eval_type.type = oqmlATOM_INT;
    i = 0;
  }

  oqmlInt::~oqmlInt()
  {
  }

  oqmlStatus *oqmlInt::compile(Database *db, oqmlContext *ctx)
  {
    if (ql)
      {
	oqmlStatus *s;

	s = ql->compile(db, ctx);

	if (s)
	  return s;

	oqmlAtomType at;

	ql->evalType(db, ctx, &at);

	if (at.type != oqmlATOM_DOUBLE &&
	    at.type != oqmlATOM_INT &&
	    at.type != oqmlATOM_UNKNOWN_TYPE)
	  return new oqmlStatus("int() function expects a 'float' or an 'int'.");

	return oqmlSuccess;
      }

    return oqmlSuccess;
  }

  oqmlStatus *oqmlInt::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    if (ql)
      {
	oqmlStatus *s;
	oqmlAtomList *al;

	*alist = new oqmlAtomList();

	s = ql->eval(db, ctx, &al);

	if (s)
	  return s;

	if (al->cnt == 1)
	  {
	    if (al->first->type.type == oqmlATOM_INT)
	      {
		*alist = new oqmlAtomList(al->first);
		return oqmlSuccess;
	      }
	  
	    if (al->first->type.type == oqmlATOM_DOUBLE)
	      {
		*alist = new oqmlAtomList
		  (new oqmlAtom_int(floor(((oqmlAtom_double *)al->first)->d)));
		return oqmlSuccess;
	      }
	  
	  }

	return new oqmlStatus("int() function expects a 'float' or an 'int'.");
      }

    *alist = new oqmlAtomList(new oqmlAtom_int(i));
    return oqmlSuccess;
  }

  void oqmlInt::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlInt::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlInt::toString(void) const
  {
    return str_convert(i) + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlChar operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlChar::oqmlChar(char _c) : oqmlNode(oqmlCHAR)
  {
    c = _c;
    eval_type.type = oqmlATOM_CHAR;
  }

  oqmlChar::~oqmlChar()
  {
  }

  oqmlStatus *oqmlChar::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlChar::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_char(c));
    return oqmlSuccess;
  }

  void oqmlChar::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlChar::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlChar::toString(void) const
  {
    if (c && !iscntrl(c))
      return std::string("'") + str_convert(c) + "'" + oqml_isstat();

    return std::string("'") + str_convert(c, "\\%03o") + "'" + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlFloat operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlFloat::oqmlFloat(double _f) : oqmlNode(oqmlFLOAT)
  {
    f = _f;
    eval_type.type = oqmlATOM_DOUBLE;
  }

  oqmlFloat::~oqmlFloat()
  {
  }

  oqmlStatus *oqmlFloat::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlFloat::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_double(f));
    return oqmlSuccess;
  }

  void oqmlFloat::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlFloat::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlFloat::toString(void) const
  {
    return str_convert(f) + + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlOid operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlOid::oqmlOid(const Oid &_oid) : oqmlNode(oqmlOID)
  {
    oid = _oid;
    eval_type.type = oqmlATOM_OID;
    cls = 0;
    ql = 0;
  }

  oqmlOid::oqmlOid(oqmlNode *_ql) : oqmlNode(oqmlOID)
  {
    eval_type.type = oqmlATOM_OID;
    cls = 0;
    ql = _ql;
  }

  oqmlOid::~oqmlOid()
  {
    // delete ql;
  }

  oqmlStatus *oqmlOid::compile(Database *db, oqmlContext *ctx)
  {
    if (ql)
      {
	oqmlStatus *s;

	s = ql->compile(db, ctx);

	if (s)
	  return s;

	oqmlAtomType at;

	ql->evalType(db, ctx, &at);

	if (at.type != oqmlATOM_STRING && at.type != oqmlATOM_UNKNOWN_TYPE)
	  return new oqmlStatus("oid() function expects a 'string'.");

	return oqmlSuccess;
      }

    if (oid.isValid())
      return oqml_get_class(db, oid, &cls);

    return oqmlSuccess;
  }

  oqmlStatus *oqmlOid::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    if (ql)
      {
	oqmlStatus *s;
	oqmlAtomList *al;

	*alist = new oqmlAtomList();

	s = ql->eval(db, ctx, &al);

	if (s)
	  return s;

	if (al->cnt != 1 || al->first->type.type != oqmlATOM_STRING)
	  return new oqmlStatus("oid() function expects a 'string'.");

	oid = Oid(OQML_ATOM_STRVAL(al->first));
	s = oqml_get_class(db, oid, &cls);

	if (s) return s;
	*alist = new oqmlAtomList(new oqmlAtom_oid(oid, cls));

	return oqmlSuccess;
      }
      
    *alist = new oqmlAtomList(new oqmlAtom_oid(oid, cls));
    return oqmlSuccess;
  }

  void oqmlOid::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlOid::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlOid::toString(void) const
  {
    return std::string(oid.toString()) + + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlObject operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlObject::oqmlObject(const char *_s) : oqmlNode(oqmlOBJECT)
  {
    eval_type.type = oqmlATOM_OBJ;
    s = strdup(_s);
    o = 0;
    idx = 0;
  }

  oqmlObject::oqmlObject(Object *_o, unsigned int _idx) : oqmlNode(oqmlOBJECT)
  {
    s = 0;
    o = _o;
    idx = _idx;
  }

  oqmlObject::oqmlObject(oqmlNode *) : oqmlNode(oqmlOBJECT)
  {
    abort();
  }

  oqmlStatus *oqmlObject::compile(Database *db, oqmlContext *ctx)
  {
    if (!o)
      return oqmlObjectManager::getObject(this, s, o, idx);
    return oqmlSuccess;
  }

  oqmlStatus *oqmlObject::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(oqmlObjectManager::registerObject(o));
    return oqmlSuccess;
  }

  void
  oqmlObject::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlObject::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlObject::toString() const
  {
    return (s ? std::string(s) : str_convert((long)idx, "%x") + ":obj") + oqml_isstat();;
  }

  oqmlObject::~oqmlObject()
  {
    free(s);
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlNil operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlNil::oqmlNil() : oqmlNode(oqmlNIL)
  {
  }

  oqmlNil::~oqmlNil()
  {
  }

  oqmlStatus *oqmlNil::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlNil::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    //*alist = new oqmlAtomList(); //WARNING: modified the 13/09/98
    //*alist = new oqmlAtomList(new oqmlAtom_nil());
    *alist = new oqmlAtomList(); //WARNING: remodified the 10/06/99
    return oqmlSuccess;
  }

  void oqmlNil::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlNil::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlNil::toString(void) const
  {
    return std::string("nil") + oqml_isstat();;
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlNull operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlNull::oqmlNull() : oqmlNode(oqmlNULL)
  {
    eval_type.type = oqmlATOM_NULL;
  }

  oqmlNull::~oqmlNull()
  {
  }

  oqmlStatus *oqmlNull::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlNull::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_null);
    return oqmlSuccess;
  }

  void oqmlNull::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlNull::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlNull::toString(void) const
  {
    return std::string("NULL") + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlTrue operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlTrue::oqmlTrue() : oqmlNode(oqmlTRUE)
  {
    eval_type.type = oqmlATOM_BOOL;
  }

  oqmlTrue::~oqmlTrue()
  {
  }

  oqmlStatus *oqmlTrue::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlTrue::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
    return oqmlSuccess;
  }

  void oqmlTrue::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlTrue::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlTrue::toString(void) const
  {
    return std::string("true") + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlFalse operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlFalse::oqmlFalse() : oqmlNode(oqmlFALSE)
  {
    eval_type.type = oqmlATOM_BOOL;
  }

  oqmlFalse::~oqmlFalse()
  {
  }

  oqmlStatus *oqmlFalse::compile(Database *db, oqmlContext *ctx)
  {
    return oqmlSuccess;
  }

  oqmlStatus *oqmlFalse::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));
    return oqmlSuccess;
  }

  void oqmlFalse::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlFalse::isConstant() const
  {
    return oqml_True;
  }

  std::string
  oqmlFalse::toString(void) const
  {
    return std::string("false") + oqml_isstat();
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlRange operator methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlRange::oqmlRange(oqmlNode *_qleft, oqmlBool _left_incl,
		       oqmlNode *_qright, oqmlBool _right_incl,
		       oqmlBool _is_between) :
    oqmlNode(oqmlRANGE)
  {
    qleft = _qleft;
    qright = _qright;
    left_incl = _left_incl;
    right_incl = _right_incl;
    is_between = _is_between;
    eval_type.type = oqmlATOM_RANGE;
  }

  oqmlRange::~oqmlRange()
  {
  }

  oqmlStatus *oqmlRange::compile(Database *db, oqmlContext *ctx)
  {
    oqmlStatus *s = qleft->compile(db, ctx);
    if (s) return s;
    return qright->compile(db, ctx);
  }

  oqmlStatus *oqmlRange::eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp *, oqmlAtom *)
  {
    static const char fmt[] = "invalid %s operand: expected int, float, char or string, got %s";

    oqmlStatus *s;
    oqmlAtomList *aleft, *aright;

    s = qleft->eval(db, ctx, &aleft);
    if (s) return s;

    s = qright->eval(db, ctx, &aright);
    if (s) return s;

    oqmlAtom *left =  (aleft->cnt == 1 ? aleft->first : 0);
    oqmlAtom *right = (aright->cnt == 1 ? aright->first : 0);

    if (!left)
      return new oqmlStatus(this, fmt, "left", "nil");

    if (!right)
      return new oqmlStatus(this, fmt, "right", "nil");

    if (left->type.type != right->type.type)
      return new oqmlStatus(this, "operand types differ");

    if (!OQML_IS_INT(left) &&
	!OQML_IS_DOUBLE(left) &&
	!OQML_IS_CHAR(left) &&
	!OQML_IS_STRING(left))
      return new oqmlStatus(this, fmt, "left", left->type.getString());

    *alist = new oqmlAtomList(new oqmlAtom_range(left, left_incl,
						 right, right_incl));
    return oqmlSuccess;
  }

  void oqmlRange::evalType(Database *db, oqmlContext *ctx, oqmlAtomType *at)
  {
    *at = eval_type;
  }

  oqmlBool oqmlRange::isConstant() const
  {
    return OQMLBOOL(qleft->isConstant() && qright->isConstant());
  }

  std::string
  oqmlRange::toString(void) const
  {
    if (is_between)
      return qleft->toString() + " and " + qright->toString();

    return (left_incl ? std::string("[") : std::string("]")) +
      qleft->toString() + "," + qright->toString() +
      (right_incl ? std::string("]") : std::string("["));
  }
}
