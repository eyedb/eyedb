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
#include "eyedblib/butils.h"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// oqml librairy functions
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace eyedb {

  oqmlStatus *
  oqml_get_class(Database *db, const Oid &oid, Class **cls)
  {
    Status is;
    Bool found;

    if (!oid.isValid())
      {
	*cls = 0;
	return new oqmlStatus("NULL oid");
	//return oqmlSuccess;
      }

    is = db->getObjectClass(oid, *cls);
	      
    if (is)
      return new oqmlStatus(is);
	      
    return oqmlSuccess;
  }

  oqmlStatus *
  oqml_scan(oqmlContext *ctx, Iterator *q, Class *cls, oqmlAtomList *alist,
	    const oqmlATOMTYPE t)
  {
    Status is;
    if ((is = q->getStatus()) == Success)
      {
	Oid oid;
	Bool found;
      
	for (;;)
	  {
	    // should use: Value v; q->next(v)
	    IteratorAtom qatom;
	    OQML_CHECK_INTR();
	    is = q->scanNext(&found, &qatom);
	  
	    if (is)
	      {
		delete q;
		return new oqmlStatus(is);
	      }

	    if (!found)
	      break;
	  
	    if (qatom.type == IteratorAtom_IDR)
	      alist->append(oqmlAtom::make_atom(qatom.data.idr, t, cls));
	    else
	      alist->append(oqmlAtom::make_atom(qatom, cls));

	    OQML_CHECK_MAX_ATOMS(alist, ctx, delete q);
	  }

	delete q;
	return oqmlSuccess;
      }

    delete q;
    return new oqmlStatus(is);
  }

  static const char *operationName[__oqml__last__];
  static oqmlBool _init = oqml_False;

  void
  oqmlNode::init()
  {
    operationName[oqmlTRUE] = "true atom";
    operationName[oqmlFALSE] = "false atom";
    operationName[oqmlCHAR] = "character atom";
    operationName[oqmlINT] = "integer atom";
    operationName[oqmlFLOAT] = "float atom";
    operationName[oqmlIDENT] = "identifier atom";
    operationName[oqmlNULL] = "null atom";
    operationName[oqmlOID] = "oid atom";
    operationName[oqmlOBJECT] = "object atom";
    operationName[oqmlSTRING] = "string atom";
    operationName[oqmlNIL] = "nil atom"; // ??

    /* standard operators */
    operationName[oqmlAAND] = "operator &";
    operationName[oqmlADD] = "operator +";
    operationName[oqmlAOR] = "operator |";
    operationName[oqmlARRAY] = "operator []";
    operationName[oqmlASSIGN] = "operator :=";
    operationName[oqmlCOMMA] = "operator ,";
    operationName[oqmlDIFF] = "operator !=";
    operationName[oqmlDIV] = "operator /";
    operationName[oqmlINF] = "operator <";
    operationName[oqmlINFEQ] = "operator <=";
    operationName[oqmlLAND] = "operator &&";
    operationName[oqmlLNOT] = "operator !";
    operationName[oqmlLOR] = "operator ||";
    operationName[oqmlMOD] = "operator %";
    operationName[oqmlMUL] = "operator *";
    operationName[oqmlREGCMP] = "operator ~";
    operationName[oqmlREGDIFF] = "operator !~";
    operationName[oqmlREGICMP] = "operator ~~";
    operationName[oqmlREGIDIFF] = "operator !~~";
    operationName[oqmlSHL] = "operator <<";
    operationName[oqmlSHR] = "operator >>";
    operationName[oqmlSUB] = "operator -";
    operationName[oqmlSUP] = "operator >";
    operationName[oqmlSUPEQ] = "operator >=";
    operationName[oqmlTILDE] = "operator ~";
    operationName[oqmlEQUAL] = "operator ==";
    operationName[oqmlDOT] = "path expression";
    operationName[oqmlXOR] = "operator ^";

    /* set operators */
    operationName[oqmlUNION]     = "union expression";
    operationName[oqmlINTERSECT] = "intersect expression";
    operationName[oqmlEXCEPT]    = "except expression";

    /* extended operators */
    operationName[oqmlISSET] = "isset operator";
    operationName[oqmlSET] = "set operator";
    operationName[oqmlUNSET] = "unset operator";
    operationName[oqmlTYPEOF] = "typeof operator";
    operationName[oqmlCLASSOF] = "classof operator";
    operationName[oqmlEVAL] = "eval operator";
    operationName[oqmlTHROW] = "throw operator";
    operationName[oqmlRETURN] = "return operator";
    operationName[oqmlUNVAL] = "unval operator";
    operationName[oqmlREFOF] = "refof operator";
    operationName[oqmlVALOF] = "valof operator";
    operationName[oqmlIMPORT] = "import operator";
    operationName[oqmlPRINT] = "print operator";

    /* cast operators */
    operationName[oqmlSTRINGOP] = "string operator";
    operationName[oqmlINTOP] = "int operator";
    operationName[oqmlCHAROP] = "char operator";
    operationName[oqmlFLOATOP] = "float operator";
    operationName[oqmlOIDOP] = "oid operator";
    operationName[oqmlIDENTOP] = "ident operator";

    /* list management */
    operationName[oqmlLISTCOLL] = "list function";
    operationName[oqmlSETCOLL] = "set function";
    operationName[oqmlBAGCOLL] = "bag function";
    operationName[oqmlARRAYCOLL] = "array function";
    operationName[oqmlCOUNT] = "count function"; // not yet implemented
    operationName[oqmlFLATTEN] = "flatten function"; // not yet implemented

    /* functions and methods */
    operationName[oqmlFUNCTION] = "function definition";
    operationName[oqmlCALL] = "function call";
    operationName[oqmlMTHCALL] = "method call";

    /* flow controls */
    operationName[oqmlIF] = "if/then operator";
    operationName[oqmlFOREACH] = "for operator";
    operationName[oqmlFORDO] = "for operator";
    operationName[oqmlWHILE] = "while operator";
    operationName[oqmlDOWHILE] = "do/while operator";
    operationName[oqmlBREAK] = "break operator";

    /* collection management */
    operationName[oqmlCOLLECTION] = "collection function";
    operationName[oqmlCONTENTS] = "contents operator";
    operationName[oqmlADDTO] = "add/to operator";
    operationName[oqmlAPPEND] = "append operator";
    operationName[oqmlSUPPRESSFROM] = "suppress operator";
    operationName[oqmlUNSETINAT] = "unset/in/at operator";
    operationName[oqmlSETINAT] = "set/in/at operator";
    operationName[oqmlELEMENT] = "element operator";
    operationName[oqmlELEMENTAT] = "element operator";
    operationName[oqmlEMPTY] = "empty operator";
    operationName[oqmlIN] = "in operator";
    operationName[oqmlEXISTS] = "exists operator";
    operationName[oqmlFOR] = "for operator";

    operationName[oqmlBODYOF] = "bodyof operator";
    operationName[oqmlSCOPEOF] = "scopeof operator";
    operationName[oqmlSTRUCTOF] = "structof operator";
    operationName[oqmlPUSH] = "push operator";
    operationName[oqmlPOP] = "pop operator";
    operationName[oqmlSTRUCT] = "struct constructor";

    /* query operators */
    operationName[oqmlSELECT] = "select operator";
    operationName[oqmlAND] = "and operator";
    operationName[oqmlOR] = "or operator";

    /* range operators */
    operationName[oqmlRANGE] = "range operator";
    operationName[oqmlBETWEEN] = "between operator";
    operationName[oqmlNOTBETWEEN] = "between operator";

    /* ordonnancing operators */
    operationName[oqmlSORT] = "sort function";
    operationName[oqmlISORT] = "isort function";
    operationName[oqmlPSORT] = "psort function";

    /* constructor/destructor */
    operationName[oqmlNEW] = "new operator";
    operationName[oqmlDELETE] = "delete operator";

    /* time functions */
    operationName[oqmlTIMEFORMAT] = "timeformat function";

    operationName[oqmlCOMPOUND_STATEMENT] = "";
    operationName[oqmlCAST] = "";
    operationName[oqmlCASTIDENT] = "";

    /* misc */
    operationName[oqmlDATABASE] = "database operator";
    operationName[oqmlCOMPOUND_STATEMENT] = "";

    /* unused */
    operationName[oqmlCAST] = "";
    operationName[oqmlCASTIDENT] = "";

    _init = oqml_True;
  }

  std::string
  oqmlNode::getOperationName() const
  {
    if (!_init) init();
    const char *s = operationName[type];
    return s ? s : "";
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlStatus methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  void
  oqmlStatus::set()
  {
    oqml_status->set(msg);
    returnAtom = 0;
  }

  /*
  void
  oqmlStatus::purge()
  {
    oqml_status->set("");
  }
  */

  oqmlStatus::oqmlStatus(const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    returnAtom = 0;
    char *errmsg = eyedblib::getFBuffer(fmt, ap);
    vsprintf(errmsg, fmt, aq);
    va_end(ap);

    msg = strdup(errmsg);
    set();
  }

  oqmlStatus::oqmlStatus(Status s)
  {
    msg = strdup(s->getDesc());
    set();
  }

  static inline std::string
  nodePrefix(oqmlNode *node)
  {
    std::string s = node->getOperationName() + " '" + node->toString() + "' : ";
    return s;
  }

  oqmlStatus::oqmlStatus(oqmlNode *node, const char *fmt, ...)
  {
    va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);

    char *errmsg = eyedblib::getFBuffer(fmt, ap);
    vsprintf(errmsg, fmt, aq);
    va_end(ap);

    if (errmsg[strlen(errmsg)-1] != '.')
      strcat(errmsg, ".");

    std::string prefix = node ? nodePrefix(node) : std::string("");
    msg = strdup((prefix + errmsg).c_str());
    set();
  }

  oqmlStatus::oqmlStatus(oqmlNode *node, Status s)
  {
    std::string prefix = node ? nodePrefix(node) : std::string("");
    std::string str = strdup((prefix + s->getDesc()).c_str());
    if (str[strlen(str.c_str())-1] != '.')
      str += std::string(".");
    msg = strdup(str.c_str());
    set();
  }

  oqmlStatus *
  oqmlStatus::expected(oqmlNode *node, const char *expected, const char *got)
  {
    return new oqmlStatus(node,
			  (std::string(expected) + " expected, got " + got).c_str());
  }

  oqmlStatus *
  oqmlStatus::expected(oqmlNode *node, oqmlAtomType *expected, oqmlAtomType *got)
  {
    return oqmlStatus::expected(node, expected->getString(), got->getString());
  }

  oqmlStatus::~oqmlStatus()
  {
    //oqmlLock(returnAtom, oqml_False, oqml_True);
    free(msg);
  }
}
