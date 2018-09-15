/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
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


#include <eyedbconfig.h>
#include <eyedblib/machtypes.h>
#include <eyedb/eyedb.h>
#include <eyedb/eyedb_p.h>
#include <string>
#include <time.h>

#include <sys/types.h>
#include <list>
#include <regex.h>

// POINTER_INT_TYPE is defined by eyedbconfig.h
typedef POINTER_INT_TYPE pointer_int_t;

#define OQML_MAX_CPS 6

namespace eyedb {

  enum oqmlTYPE {

    __oqml__first__ = 0,

    /* atoms */
    oqmlTRUE,
    oqmlFALSE,
    oqmlCHAR,
    oqmlINT,
    oqmlFLOAT,
    oqmlIDENT,
    oqmlNULL,
    oqmlOID,
    oqmlOBJECT,
    oqmlSTRING,
    oqmlNIL,

    /* standard operators */
    oqmlAAND,
    oqmlADD,
    oqmlAOR,
    oqmlARRAY,
    oqmlASSIGN,
    oqmlCOMMA,
    oqmlDIFF,
    oqmlDIV,
    oqmlINF,
    oqmlINFEQ,
    oqmlLAND,
    oqmlLNOT,
    oqmlLOR,
    oqmlMOD,
    oqmlMUL,
    oqmlREGCMP,
    oqmlREGDIFF,
    oqmlREGICMP,
    oqmlREGIDIFF,
    oqmlSHL,
    oqmlSHR,
    oqmlSUB,
    oqmlSUP,
    oqmlSUPEQ,
    oqmlTILDE,
    oqmlEQUAL,
    oqmlBETWEEN,
    oqmlNOTBETWEEN,
    oqmlRANGE,
    oqmlDOT,
    oqmlXOR,

    /* extended operators */
    oqmlISSET,
    oqmlSET,
    oqmlUNSET,
    oqmlTYPEOF,
    oqmlCLASSOF,
    oqmlEVAL,
    oqmlTHROW,
    oqmlUNVAL,
    oqmlREFOF,
    oqmlVALOF,
    oqmlIMPORT,
    oqmlRETURN,

    oqmlPRINT,

    /* set operators */
    oqmlUNION,
    oqmlINTERSECT,
    oqmlEXCEPT,

    /* cast operators */
    oqmlSTRINGOP,
    oqmlINTOP,
    oqmlCHAROP,
    oqmlFLOATOP,
    oqmlOIDOP,
    oqmlIDENTOP,

    /* literal */
    oqmlSTRUCT,

    /* list management */
    oqmlLISTCOLL,
    oqmlBAGCOLL,
    oqmlARRAYCOLL,
    oqmlSETCOLL,

    oqmlCOUNT,
    oqmlFLATTEN,

    oqmlBODYOF,
    oqmlSCOPEOF,
    oqmlSTRUCTOF,

    /* functions and methods */
    oqmlFUNCTION,
    oqmlCALL,
    oqmlMTHCALL,

    /* flow controls */
    oqmlIF,
    oqmlFOREACH,
    oqmlWHILE,
    oqmlDOWHILE,
    oqmlBREAK,
    oqmlFORDO,

    /* collection management */
    oqmlCOLLECTION,
    oqmlCONTENTS,
    oqmlSUPPRESSFROM,
    oqmlADDTO,
    oqmlAPPEND,

    oqmlELEMENT,
    oqmlEMPTY,
    oqmlIN,
    oqmlEXISTS,
    oqmlFOR,

    /* obsolete */
    oqmlSETINAT,
    oqmlELEMENTAT,
    oqmlUNSETINAT,

    /* query operators */
    oqmlSELECT,
    oqmlAND,
    oqmlOR,

    /* ordonnancing operators */
    oqmlSORT,
    oqmlISORT,
    oqmlPSORT,

    /* constructor/destructor */
    oqmlNEW,
    oqmlDELETE,

    /* time functions */
    oqmlTIMEFORMAT,

    /* misc */
    oqmlCOMPOUND_STATEMENT,
    oqmlDATABASE,

    /* unused */
    oqmlCAST,
    oqmlCASTIDENT,
    oqmlPUSH,
    oqmlPOP,

    __oqml__last__
  };

  enum oqmlBool {
    oqml_False = 0,
    oqml_True = 1
  };

  /*
   * Forward Declarations
   */

  class gbLink;
  class oqmlStatus;
  class oqmlNode;
  class oqmlSymbolEntry;
  class oqmlFunctionEntry;
  class oqmlDot;
  class oqmlIdent;
  class oqmlDotContext;
  class oqml_ParamList;
  class oqml_DeclList;
  class oqmlComp;
  class oqmlIterator;
  class oqml_DeclList;
  class oqml_DeclItem;
  class oqmlAtomList;
  class oqml_Cardinality;
  class oqml_EnumList;
  class oqmlAtom_nil;
  class oqmlAtom_null;
  class oqmlAtom_bool;
  class oqmlAtom_oid;
  class oqmlAtom_obj;
  class oqmlAtom_int;
  class oqmlAtom_char;
  class oqmlAtom_double;
  class oqmlAtom_string;
  class oqmlAtom_ident;
  class oqmlAtom_range;
  class oqmlAtom_coll;
  class oqmlAtom_list;
  class oqmlAtom_bag;
  class oqmlAtom_set;
  class oqmlAtom_array;
  class oqmlAtom_struct;
  class oqmlAtom_node;
  class oqmlAtom_select;

#define OQMLBOOL(X) ((X) ? oqml_True : oqml_False)
#define oqml_ESTIM_MIN    ((unsigned int)0)
#define oqml_ESTIM_MIDDLE ((unsigned int)1)
#define oqml_ESTIM_MAX    ((unsigned int)2)
#define oqml_INFINITE     ((unsigned int)~0)

  /*
   * Atom Declarations
   */

  enum oqmlATOMTYPE {
    oqmlATOM_UNKNOWN_TYPE = 0,
    oqmlATOM_NIL,
    oqmlATOM_NULL,
    oqmlATOM_BOOL,
    oqmlATOM_OID,
    oqmlATOM_OBJ,
    oqmlATOM_INT,
    oqmlATOM_RANGE,
    oqmlATOM_CHAR,
    oqmlATOM_DOUBLE,
    oqmlATOM_STRING,
    oqmlATOM_IDENT,
    oqmlATOM_LIST,
    oqmlATOM_BAG,
    oqmlATOM_SET,
    oqmlATOM_ARRAY,
    oqmlATOM_STRUCT,
    oqmlATOM_NODE,
    oqmlATOM_SELECT
  };

  class oqmlAtomType {

  public:
    oqmlATOMTYPE type;
    Class *cls;
    oqmlBool comp;

    oqmlAtomType() {type = oqmlATOM_UNKNOWN_TYPE; cls = 0; comp = oqml_False;}

    oqmlAtomType(oqmlATOMTYPE _type, Class *_cls = 0) {
      type = _type;
      cls = _cls;
      comp = (type == oqmlATOM_STRING ? oqml_True : oqml_False);
    }	    

    oqmlBool cmp(const oqmlAtomType &, Bool autocast = False);
    const char *getString() const;
  };
  
  class oqmlAtom {

  public:
    oqmlAtomType type;

    int refcnt;
    oqmlBool recurs;
    gbLink *link;
    oqmlAtom *next;
    char *string;

    oqmlAtom();
    oqmlAtom(const oqmlAtom&);
    oqmlAtom &operator =(const oqmlAtom &);

    virtual oqmlNode *toNode() = 0;
    void print(FILE *fd = stdout);
    char *getString() const;

    virtual char *makeString(FILE *) const = 0;
    virtual oqmlBool getData(unsigned char[], Data *, Size&, int&,
			     const Class * = 0) const = 0;
    virtual int getSize() const = 0;
    virtual oqmlBool makeEntry(int, unsigned char *, int,
			       const Class * = 0) const;
    virtual oqmlAtom *copy() {
      return NULL;
    }
    virtual oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual oqmlBool isEqualTo(oqmlAtom &);

    virtual oqmlAtom_nil    *as_nil()    {return 0;}
    virtual oqmlAtom_null   *as_null()   {return 0;}
    virtual oqmlAtom_bool   *as_bool()   {return 0;}
    virtual oqmlAtom_oid    *as_oid()    {return 0;}
    virtual oqmlAtom_obj    *as_obj()    {return 0;}
    virtual oqmlAtom_int    *as_int()    {return 0;}
    virtual oqmlAtom_char   *as_char()   {return 0;}
    virtual oqmlAtom_double *as_double() {return 0;}
    virtual oqmlAtom_string *as_string() {return 0;}
    virtual oqmlAtom_ident  *as_ident()  {return 0;}
    virtual oqmlAtom_range  *as_range()  {return 0;}

    virtual oqmlAtom_coll   *as_coll()   {return 0;}
    virtual oqmlAtom_list   *as_list()   {return 0;}
    virtual oqmlAtom_bag    *as_bag()    {return 0;}
    virtual oqmlAtom_set    *as_set()    {return 0;}
    virtual oqmlAtom_array  *as_array()  {return 0;}

    virtual oqmlAtom_node   *as_node()   {return 0;}
    virtual oqmlAtom_select *as_select() {return 0;}
    virtual oqmlAtom_struct *as_struct() {return 0;}

    virtual Value *toValue() const;

    static oqmlAtom *make_atom(const IteratorAtom &atom, Class *cls);
    static oqmlAtom *make_atom(Data data, oqmlATOMTYPE type,
			       const Class *cls);
  
    virtual ~oqmlAtom();
  };

  class oqmlAtom_null : public oqmlAtom {

  public:

    oqmlAtom_null();

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    virtual oqmlAtom_null   *as_null()   {return this;}
  };

  class oqmlAtom_nil : public oqmlAtom {

  public:

    oqmlAtom_nil();

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual oqmlAtom_nil    *as_nil()    {return this;}
  };

  class oqmlAtom_oid : public oqmlAtom {

  public:
    Oid oid;

    oqmlAtom_oid(const Oid &, Class * = NULL);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    virtual oqmlAtom_oid    *as_oid()    {return this;}
  };

  class oqmlSharedString {

  public:
    int refcnt;
    char *s;
    int len;

    oqmlSharedString(const char *_s);

    void set(const char *);
    int getLen() const;

    ~oqmlSharedString();
  };

  class oqmlAtom_ident : public oqmlAtom {

  public:
    oqmlSharedString *shstr;
    oqmlSymbolEntry *entry;

    oqmlAtom_ident(const char *, oqmlSymbolEntry * = 0);
    oqmlAtom_ident(oqmlSharedString *, oqmlSymbolEntry * = 0);

    void releaseEntry();

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    ~oqmlAtom_ident();
    virtual oqmlAtom_ident  *as_ident()  {return this;}
  };

  class oqmlAtom_obj : public oqmlAtom {

    Object *o;

  public:
    pointer_int_t idx;

    oqmlAtom_obj(Object *, pointer_int_t, const Class * = 0);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();

    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    ~oqmlAtom_obj();

    Object *getObject();
    oqmlStatus *checkObject();

    virtual oqmlAtom_obj    *as_obj()    {return this;}
  };

#define OQL_CHECK_OBJ(A) \
{ \
    oqmlStatus *_s = (A)->as_obj()->checkObject(); \
    if (_s) \
      return _s; \
}

  class oqmlAtom_bool : public oqmlAtom {

  public:
    oqmlBool b;

    oqmlAtom_bool(oqmlBool);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    virtual Value *toValue() const;
    oqmlAtom *copy();
    virtual oqmlAtom_bool   *as_bool()   {return this;}
  };

  class oqmlAtom_int : public oqmlAtom {

  public:
    eyedblib::int64 i;

    oqmlAtom_int(eyedblib::int64);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    virtual oqmlAtom_int    *as_int()    {return this;}
  };

  class oqmlAtom_range : public oqmlAtom {

  public:
    oqmlATOMTYPE rtype;
    oqmlAtom *from, *to;
    oqmlBool from_incl, to_incl;

    oqmlAtom_range(oqmlAtom *from, oqmlBool, oqmlAtom *to, oqmlBool);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    virtual oqmlAtom_range    *as_range()    {return this;}
  };

  class oqmlAtom_char : public oqmlAtom {

  public:
    char c;

    oqmlAtom_char(char);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    virtual oqmlAtom_char   *as_char()   {return this;}
  };

  class oqmlAtom_string : public oqmlAtom {

  public:

    oqmlSharedString *shstr;
    oqmlAtom_string(const char *);
    oqmlAtom_string(oqmlSharedString *);

    int getLen() const;

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlBool makeEntry(int, unsigned char *, int,
		       const Class * = 0) const;
    oqmlAtom *copy();
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual Value *toValue() const;
    ~oqmlAtom_string();
    virtual oqmlAtom_string *as_string() {return this;}
    void set(const char *);

    oqmlStatus *setAtom(oqmlAtom *, int idx, oqmlNode *);
  };

  class oqmlAtom_double : public oqmlAtom {

  public:
    double d;

    oqmlAtom_double(double);

    virtual oqmlBool isEqualTo(oqmlAtom &);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlAtom *copy();
    virtual Value *toValue() const;
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual oqmlAtom_double *as_double() {return this;}
  };

  class oqml_StructAttr {

  public:
    char *name;
    oqmlAtom *value;

    oqml_StructAttr() {
      name = 0;
      value = 0;
    }

    oqml_StructAttr(const char *_name, oqmlAtom *_value) {
      set(_name, _value);
    }

    void set(const char *_name, oqmlAtom *_value) {
      name = strdup(_name);
      value = _value;
    }

    ~oqml_StructAttr() {
      free(name);
    }

    oqml_StructAttr(const oqml_StructAttr &x) {
      *this = x;
    }

    oqml_StructAttr& operator=(const oqml_StructAttr &x) {
      name = strdup(x.name);
      value = (x.value ? x.value->copy() : 0);
      return *this;
    }

  private:
    friend class oqmlAtom_struct;
  };

  class oqmlAtom_struct : public oqmlAtom {

  public:
    char *name;
    int attr_cnt;
    oqml_StructAttr *attr;
    oqmlNode *toNode();
    oqmlAtom_struct();
    oqmlAtom_struct(oqml_StructAttr *, int cnt);
    char *makeString(FILE *) const;
    oqmlAtom *copy();
    oqmlAtom *getAtom(const char *name, int &);
    oqmlAtom *getAtom(unsigned int);
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    virtual oqmlBool isEqualTo(oqmlAtom &);
    int getSize() const;
    virtual oqmlAtom_struct *as_struct() {return this;}
    oqmlStatus *setAtom(oqmlAtom *, int idx, oqmlNode *);
    virtual Value *toValue() const;

    ~oqmlAtom_struct();
  };

  class oqmlAtom_node : public oqmlAtom {

  public:
    oqmlNode *node;
    //  oqmlAtomList *list;
    oqmlBool evaluated;

    oqmlAtom_node(oqmlNode *);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlAtom *copy();
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual oqmlAtom_node *as_node() {return this;}
    ~oqmlAtom_node();
  };

  class oqmlContext;

  class oqmlAtom_select : public oqmlAtom {

  public:
    oqmlNode *node;
    oqmlNode *node_orig;
    oqmlAtomList *list;
    oqmlBool evaluated;
    oqmlBool indexed;
    oqmlAtom_coll *collatom;
    int cpcnt;
    oqmlAtomList *cplists[OQML_MAX_CPS];

    oqmlAtom_select(oqmlNode *, oqmlNode *);
    oqmlNode *toNode();
    char *makeString(FILE *) const;
    oqmlAtom *copy();
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    oqmlBool isPopulated () const {return OQMLBOOL(list);}
    oqmlBool isIndexed () const {return indexed;}
    oqmlBool compare(unsigned char *, int, Bool, oqmlTYPE) const;
    virtual oqmlAtom_select *as_select() {return this;}
    void appendCP(oqmlContext *);
    void setCP(oqmlContext *);
    ~oqmlAtom_select();
  };

  class oqmlAtom_coll : public oqmlAtom {

  public:
    oqmlAtomList *list;

    oqmlAtom_coll(oqmlAtomList *);
    char *makeString(FILE *) const;
    oqmlBool getData(unsigned char[], Data *, Size&, int&,
		     const Class * = 0) const;
    int getSize() const;
    virtual oqmlBool isEqualTo(oqmlAtom &);
    virtual oqmlAtom_coll   *as_coll()   {return this;}
    oqmlStatus *setAtom(oqmlAtom *, int idx, oqmlNode *);
    virtual Value::Type getValueType() const;
    virtual Value *toValue() const;
    virtual const char *getTypeName() const = 0;
    oqmlAtom_coll *suppressDoubles();
  };

  class oqmlAtom_list : public oqmlAtom_coll {
  public:
    oqmlAtom_list(oqmlAtomList *);
    oqmlNode *toNode();
    virtual oqmlAtom_list   *as_list()   {return this;}
    virtual Value::Type getValueType() const;
    oqmlAtom *copy();
    const char *getTypeName() const {return "list";}
  };

  class oqmlAtom_bag : public oqmlAtom_coll {
  public:
    oqmlAtom_bag(oqmlAtomList *);
    oqmlNode *toNode();
    virtual oqmlAtom_bag   *as_bag()   {return this;}
    virtual Value::Type getValueType() const;
    const char *getTypeName() const {return "bag";}
    oqmlAtom *copy();
  };

  class oqmlAtom_set : public oqmlAtom_coll {
  public:
    oqmlAtom_set(oqmlAtomList *, oqmlBool suppressDoubles = oqml_True);
    oqmlNode *toNode();
    const char *getTypeName() const {return "set";}
    virtual oqmlAtom_set   *as_set()   {return this;}
    virtual Value::Type getValueType() const;
    oqmlAtom *copy();
  };

  class oqmlAtom_array : public oqmlAtom_coll {
  public:
    oqmlAtom_array(oqmlAtomList *);
    oqmlNode *toNode();
    virtual oqmlAtom_array   *as_array()   {return this;}
    virtual Value::Type getValueType() const;
    const char *getTypeName() const {return "array";}
    oqmlAtom *copy();
  };

  class oqmlContext;

  class oqmlAtomList {

  public:
    unsigned int cnt;
    oqmlAtom *first;
    oqmlAtom *last;

    int refcnt;
    oqmlBool recurs;
    gbLink *link;
    char *string;

    oqmlAtomList();
    oqmlAtomList(oqmlAtomList *);
    oqmlAtomList(oqmlAtom *);

    oqmlAtomList *copy();
    oqmlAtom **toArray();

    void print(FILE *fd = stdout);
    char *getString() const;

    static oqmlAtomList *andOids(oqmlAtomList *, oqmlAtomList *);
    oqmlAtom *getAtom(unsigned int idx);
    oqmlStatus *setAtom(oqmlAtom *, int idx, oqmlNode *);
    int getFlattenCount() const;

    bool append(oqmlAtom *, bool incref = true, bool detect_cycle = false);
    oqmlBool isIn(oqmlAtom *);
    oqmlStatus *suppress(oqmlAtom *);
    void empty();

    bool append(oqmlAtomList *, oqmlBool = oqml_True, bool detect_cycle = false);
    void suppressDoubles();
    oqmlStatus *compare(oqmlNode *node, oqmlContext *ctx, oqmlAtomList *, const char *, oqmlTYPE, oqmlBool &) const;
    oqmlBool isEqualTo(oqmlAtomList &);
    void convert(IteratorAtom *, int&);
    ~oqmlAtomList();
  };

#define OQML_ATOM_STRVAL(X)   ((X)->as_string()->shstr->s)
#define OQML_ATOM_STRLEN(X)   ((X)->as_string()->getLen())
#define OQML_ATOM_INTVAL(X)   ((X)->as_int()->i)
#define OQML_ATOM_BOOLVAL(X)  ((X)->as_bool()->b)
#define OQML_ATOM_CHARVAL(X)  ((X)->as_char()->c)
#define OQML_ATOM_DBLVAL(X)   ((X)->as_double()->d)
#define OQML_ATOM_OIDVAL(X)   ((X)->as_oid()->oid)
#define OQML_ATOM_COLLVAL(X)  ((X)->as_coll()->list)
#define OQML_ATOM_LISTVAL(X)  ((X)->as_list()->list)
#define OQML_ATOM_SETVAL(X)   ((X)->as_set()->list)
#define OQML_ATOM_BAGVAL(X)   ((X)->as_bag()->list)
#define OQML_ATOM_ARRVAL(X)   ((X)->as_array()->list)
#define OQML_ATOM_IDENTVAL(X) ((X)->as_ident()->shstr->s)
#define OQML_ATOM_OBJVAL(X)   ((X)->as_obj()->getObject())

#define OQML_IS(X, AS) ((X)->AS())

#define OQML_IS_STR(X)    OQML_IS(X, as_string)
#define OQML_IS_INT(X)    OQML_IS(X, as_int)
#define OQML_IS_CHAR(X)   OQML_IS(X, as_char)
#define OQML_IS_BOOL(X)   OQML_IS(X, as_bool)
#define OQML_IS_DBL(X)    OQML_IS(X, as_double)
#define OQML_IS_OID(X)    OQML_IS(X, as_oid)
#define OQML_IS_OBJ(X)    OQML_IS(X, as_obj)
#define OQML_IS_OBJECT(X) (OQML_IS_OBJ(X) || OQML_IS_OID(X))
#define OQML_IS_COLL(X)   OQML_IS(X, as_coll)
#define OQML_IS_LIST(X)   OQML_IS(X, as_list)
#define OQML_IS_BAG(X)    OQML_IS(X, as_bag)
#define OQML_IS_SET(X)    OQML_IS(X, as_set)
#define OQML_IS_ARRAY(X)  OQML_IS(X, as_array)
#define OQML_IS_NODE(X)   OQML_IS(X, as_node)
#define OQML_IS_STRUCT(X) OQML_IS(X, as_struct)
#define OQML_IS_IDENT(X)  OQML_IS(X, as_ident)
#define OQML_IS_NULL(X)   OQML_IS(X, as_null)

#define OQML_IS_STRING OQML_IS_STR
#define OQML_IS_DOUBLE OQML_IS_DBL

  /* end atom declarations */

  struct gbContext {
    gbLink *link;
    gbContext(gbLink *link) : link(link) {}
  };

  class oqmlGarbManager {
    static gbLink *first, *last;
    static gbLink *add_realize(gbLink *);
    static oqmlBool garbaging;
    static unsigned int count;
    static LinkedList str_list;
    static std::list<gbContext *> ctx_l;

  public:
    static void garbage();
    static gbLink *add(oqmlAtom *);
    static gbLink *add(oqmlAtomList *);
    static void remove(gbLink *);
    static void add(char *);
    static unsigned int getCount() {return count;}

    static void garbage(gbLink *l, bool full = false);
    static gbContext *peek();
    static void garbage(gbContext *ctx);
  };

  class oqml_Location {

  public:
    int from, to;
  };

  /* status */
  class oqmlStatus {

  public:
    char *msg;
    oqmlAtom *returnAtom;
    oqmlStatus(const char *, ...);
    oqmlStatus(Status);
    oqmlStatus(oqmlNode *, const char *, ...);
    oqmlStatus(oqmlNode *, Status);
    static oqmlStatus *expected(oqmlNode *node, const char *expected,
				const char *got);
    static oqmlStatus *expected(oqmlNode *node, oqmlAtomType *expected,
				oqmlAtomType *got);
    static void purge();
    void set();

    ~oqmlStatus();
  };

#define oqmlSuccess ((oqmlStatus *)0)

  /* syntax list */
  class oqml_Link {

  public:
    oqmlNode *ql;
    oqml_Link *next;

    oqml_Link(oqmlNode *);
    ~oqml_Link();
  };

  class oqml_List {

  public:
    int cnt;
    oqml_Link *first;
    oqml_Link *last;

    oqml_List();
    ~oqml_List();

    void lock();
    void unlock();

    std::string toString() const;

    oqmlBool hasIdent(const char *);

    void add(oqmlNode *);
  };

  class oqml_IdentLink {

  public:
    char *ident; 
    oqmlNode *left;
    oqmlNode *ql;
    oqmlBool skipIdent;
    oqmlBool requalified;
    const Class *cls;
    oqml_IdentLink *next;
    oqml_IdentLink(const char *, oqmlNode *);
    oqml_IdentLink(oqmlNode *, oqmlNode *);
    void lock();
    void unlock();
    ~oqml_IdentLink();
  };

  class oqml_IdentList {

  public:
    int cnt;
    oqml_IdentLink *first;
    oqml_IdentLink *last;

    oqml_IdentList();
    ~oqml_IdentList();
    void lock();
    void unlock();
    void add(const char *, oqmlNode *);
    void add(oqmlNode *, oqmlNode *);
  };

  /* context */

  class oqmlSymbolEntry {

  public:
    char *ident;
    oqmlAtomType type;
    oqmlAtom *at;
    oqmlBool global;
    oqmlSymbolEntry *prev, *next;
    oqmlBool system;
    oqmlAtomList *list;
    int level;
    oqmlBool popped;

    oqmlSymbolEntry(oqmlContext *, const char *_ident, oqmlAtomType *_type,
		    oqmlAtom *_at, oqmlBool _global, oqmlBool _system);

    void set(oqmlAtomType *_type, oqmlAtom *_at, oqmlBool force = oqml_False,
	     oqmlBool tofree = oqml_False);

    void addEntry(oqmlAtom_ident *ident);

    void releaseEntries();

    ~oqmlSymbolEntry();
  };

  class oqmlSymbolTable {

  public:
    oqmlSymbolEntry *sfirst, *slast;
    oqmlFunctionEntry *ffirst, *flast;
    oqmlSymbolTable() {sfirst = slast = 0; ffirst = flast = 0;}
  };

  class oqmlSelect;

  class oqmlContext {
    oqmlSymbolTable *symtab;
    oqmlDotContext *dot_ctx;
    oqmlAtomList *and_list_ctx;
    int lastTempSymb;
    int or_ctx, and_ctx, preval_ctx, where_ctx;
    unsigned int maxatoms;
    oqmlBool overMaxAtoms;
    oqmlBool oneatom;
    int local_cnt;
    int arg_level;
    int local_alloc;
    oqmlBool is_popping;
    LinkedList **locals;
    int select_ctx_cnt;
    int hidden_select_ctx_cnt;
    oqmlSelect *select_ctx[64];
    oqmlSelect *hidden_select_ctx[64];
    // cp for Cartesian Product
    int cpatom_cnt;
    oqmlAtom *cpatoms[OQML_MAX_CPS];

    oqmlStatus *pushSymbolRealize(const char *, oqmlAtomType *, oqmlAtom *,
				  oqmlBool global, oqmlBool system);
    oqmlStatus *setSymbolRealize(const char *, oqmlAtomType *, oqmlAtom *,
				 oqmlBool global, oqmlBool system,
				 oqmlBool to_free);
    oqmlStatus *popSymbolRealize(const char *, oqmlBool global);
    static std::string makeTempSymb(int idx);

  public:
    static oqmlSymbolTable stsymtab;

    oqmlContext(oqmlSymbolTable * = 0);
    oqmlStatus *pushSymbol(const char *, oqmlAtomType *, oqmlAtom * = 0,
			   oqmlBool global = oqml_False,
			   oqmlBool system = oqml_False);
    oqmlStatus *setSymbol(const char *, oqmlAtomType *, oqmlAtom * = 0,
			  oqmlBool global = oqml_False,
			  oqmlBool system = oqml_False);
    oqmlStatus *popSymbol(const char *, oqmlBool global = oqml_False);

    oqmlStatus *pushCPAtom(oqmlNode *, oqmlAtom *);
    oqmlStatus *popCPAtom(oqmlNode *);
    int getCPAtomCount() const {return cpatom_cnt;}
    oqmlAtom **getCPAtoms() {return cpatoms;}

    LinkedList *getLocalTable() {return local_cnt > 0 ? locals[local_cnt-1] : 0;}

    int getLocalDepth() const {return local_cnt;}

    void push() {local_cnt++;}
    void pop() {local_cnt--;}

    void pushArgLevel() {arg_level++;}
    void popArgLevel() {arg_level--;}

    int getArgLevel() const {return arg_level;}

    oqmlStatus *pushLocalTable();
    oqmlStatus *popLocalTable();

    void displaySymbols();

    oqmlBool getSymbol(const char *, oqmlAtomType * = 0, oqmlAtom ** = 0,
		       oqmlBool *global = 0, oqmlBool *system = 0);

    oqmlSymbolEntry *getSymbolEntry(const char *);

    oqmlStatus *setFunction(const char * , oqml_ParamList *, oqmlNode *);
    int getFunction(const char *, oqmlFunctionEntry **);
    void popFunction(const char *);

    void setDotContext(oqmlDotContext *);
    oqmlDotContext *getDotContext();
    void setAndContext(oqmlAtomList *);
    oqmlAtomList *getAndContext();
    oqmlBool isSelectContext() {return select_ctx_cnt ? oqml_True : oqml_False;}

    oqmlSelect *getSelectContext() {
      return select_ctx_cnt > 0 ? select_ctx[select_ctx_cnt-1] : (oqmlSelect *)0;
    }

    oqmlSelect *getHiddenSelectContext() {
      return hidden_select_ctx_cnt > 0 ? hidden_select_ctx[hidden_select_ctx_cnt-1] : (oqmlSelect *)0;
    }

    void incrSelectContext(oqmlSelect *);
    void decrSelectContext();
    void incrHiddenSelectContext(oqmlSelect *);
    void decrHiddenSelectContext();

    int getHiddenSelectContextCount() const {return hidden_select_ctx_cnt;}
    int getSelectContextCount() const {return select_ctx_cnt;}

    int setSelectContextCount(int _select_ctx_cnt) {
      int o_select_ctx_cnt = select_ctx_cnt;
      select_ctx_cnt = _select_ctx_cnt;
      return o_select_ctx_cnt;
    }

    void setMaxAtoms(unsigned int _maxatoms) {maxatoms = _maxatoms;}
    unsigned int getMaxAtoms() const {return maxatoms;}
    void setOneAtom() {oneatom = oqml_True;}
    void setOneAtom(oqmlBool _oneatom) {oneatom = _oneatom;}
    oqmlBool isOneAtom() const {return oneatom;}
    void setOverMaxAtoms() {overMaxAtoms = oqml_True;}
    oqmlBool isOverMaxAtoms() const {return overMaxAtoms;}

    oqmlBool isOrContext() {return or_ctx ? oqml_True : oqml_False;}
    void incrOrContext() {or_ctx++;}
    void decrOrContext() {or_ctx--;}

    oqmlBool isAndContext() {return and_ctx ? oqml_True : oqml_False;}
    void incrAndContext() {and_ctx++;}
    void decrAndContext() {and_ctx--;}

    oqmlBool isPrevalContext() {return preval_ctx ? oqml_True : oqml_False;}
    void incrPrevalContext() {preval_ctx++;}
    void decrPrevalContext() {preval_ctx--;}
    oqmlBool isWhereContext() {return where_ctx ? oqml_True : oqml_False;}
    void incrWhereContext() {where_ctx++;}
    void decrWhereContext() {where_ctx--;}

    int setWhereContext(int _where_ctx) {
      int owhere_ctx = where_ctx;
      where_ctx = _where_ctx;
      return owhere_ctx;
    }

    int setPrevalContext(int _preval_ctx) {
      int opreval_ctx = preval_ctx;
      preval_ctx = _preval_ctx;
      return opreval_ctx;
    }

    std::string getTempSymb();
    ~oqmlContext();
  };

#define OQML_OBJECT_MANAGER

  class oqmlObjectManager {

    static void addToFreeList(Object *o);
    static LinkedList freeList;
  public:

    static oqmlStatus *getObject(oqmlNode *, Database *, oqmlAtom *,
				 Object *&, oqmlBool addToFreeList,
				 oqmlBool errorIfNull = oqml_True);
    static oqmlStatus *getObject(oqmlNode *, Database *db, const Oid &,
				 Object *&, oqmlBool addToFreeList,
				 oqmlBool errorIfNull = oqml_True);
    static oqmlStatus *getObject(oqmlNode *, Database *db, const Oid *,
				 Object *&, oqmlBool addToFreeList,
				 oqmlBool errorIfNull = oqml_True);
    static oqmlStatus *getObject(oqmlNode *, const char *, Object *&,
				 pointer_int_t &);
    static void releaseObject(Object *, oqmlBool = oqml_False);
    static oqmlStatus *getIndex(oqmlNode *node, const Object *o,
				pointer_int_t &idx);

    static oqmlAtom *registerObject(Object *);
    static oqmlStatus *unregisterObject(oqmlNode *node, Object *o, bool force = false);
    static oqmlBool isRegistered(const Object *, pointer_int_t &);

    static void garbageObjects();

    static ObjCache *objCacheIdx;
    static ObjCache *objCacheObj;
  };

  enum oqmlBinopType {
    oqmlIntOK    = 0x0,
    oqmlDoubleOK = 0x1,
    oqmlConcatOK = 0x2,
    oqmlDoubleConcatOK = oqmlDoubleOK|oqmlConcatOK
  };

  extern void
  oqmlLock(oqmlAtom *a, oqmlBool lock, oqmlBool rm = oqml_False);

  extern void
  oqmlLock(oqmlAtomList *l, oqmlBool lock, oqmlBool rm = oqml_False);

  extern oqmlStatus *
  oqml_check_logical(oqmlNode *node, oqmlAtomList *al, oqmlBool &b,
		     bool strict = false);
  /* base node */
  class oqmlNode {
  protected:
    oqmlTYPE type;
    oqmlAtomType eval_type;
    oqmlAtomList *cst_list;
    oqml_Location loc;
    static LinkedList node_list;
    oqmlBool locked;
    static void init();
    oqmlStatus *binopCompile(Database *, oqmlContext *,
			     const char *opstr, oqmlNode *, oqmlNode *,
			     oqmlAtomType &, oqmlBinopType, oqmlBool &);

    oqmlStatus *binopEval(Database *, oqmlContext *,
			  const char *opstr, const oqmlAtomType &,
			  oqmlNode *, oqmlNode *, oqmlBinopType,
			  oqmlAtomList **, oqmlAtomList **);

    static void swap(oqmlComp *comp, oqmlNode *&qleft, oqmlNode *&qright);
    oqmlStatus *compCompile(Database *, oqmlContext *, const char *,
			    oqmlNode *&, oqmlNode *&, oqmlComp *, oqmlAtom **,
			    oqmlAtomType *);

    oqmlStatus *compEval(Database *, oqmlContext *, const char *, oqmlNode *,
			 oqmlNode *, oqmlAtomList **, oqmlComp *, oqmlAtom *);

    oqmlStatus *requalify_node(Database *, oqmlContext *, oqmlNode *&ql, const char *ident, oqmlNode *node, oqmlBool &done);
    oqmlStatus *requalify_node_back(Database *, oqmlContext *, oqmlNode *&ql);
    oqmlStatus *requalify_node(Database *, oqmlContext *, oqmlNode *&ql, const Attribute **attrs,
			       int attr_cnt, const char *ident);

  public:
    oqmlNode(oqmlTYPE);
    oqmlBool is_statement;
    virtual ~oqmlNode();

    virtual oqmlStatus *compile(Database *db, oqmlContext *ctx) = 0;
    virtual oqmlStatus *eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp * = NULL, oqmlAtom * = NULL) = 0;
    virtual void evalType(Database *db, oqmlContext *ctx, oqmlAtomType *) = 0;
    virtual oqmlBool   isConstant() const = 0;

    virtual oqmlBool mayBeRequalified() const {return oqml_True;}
    virtual oqmlStatus *requalify(Database *, oqmlContext *, const char *, oqmlNode *, oqmlBool &done);
    virtual oqmlStatus *requalify(Database *, oqmlContext *, const Attribute **, int attr_cnt,
				  const char *ident);
    virtual oqmlStatus *requalify_back(Database *, oqmlContext *);

    virtual std::string toString() const {return "";}

    inline oqmlTYPE getType() const {return type;}
    virtual oqmlComp *asComp() {return (oqmlComp *)0;}

    virtual oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &r) {
      r = oqml_ESTIM_MAX;
      return oqmlSuccess;
    }

    oqmlStatus *realize(Database *db, oqmlAtomList **);
    oqmlStatus *compEval(Database *db, oqmlContext *ctx, oqmlAtomType *);
    virtual void lock();
    virtual void unlock();
    oqmlBool isLocked() const {return locked;}
    virtual oqmlStatus *evalLeft(Database *db, oqmlContext *ctx,
				 oqmlAtom **a, int &idx);
    static void garbageNodes();
    void locate(int, int);
    oqmlNode *back;

    virtual oqmlBool hasIdent(const char *) {return oqml_False;}

    virtual oqmlStatus *preEvalSelect(Database *db, oqmlContext *ctx,
				      const char *ident, oqmlBool &done,
				      unsigned int &cnt, oqmlBool firstPass = oqml_True) {
      done = oqml_False;
      cnt = 0;
      return oqmlSuccess;
    }

    oqmlBool equals(const oqmlNode *node) const {
      return OQMLBOOL(node && type == node->getType() &&
		      !strcmp(toString().c_str(), node->toString().c_str()));
    }

    virtual oqmlDot   *asDot() {return (oqmlDot *)0;}
    virtual oqmlIdent *asIdent() {return (oqmlIdent *)0;}
    std::string getOperationName() const;
    void requalifyType(oqmlTYPE);
    static void registerNode(oqmlNode *);
  };

#define OQML_STD_REQUAL_2() \
  virtual oqmlStatus *requalify(Database *db, oqmlContext *ctx, const char *_ident, oqmlNode *_node, oqmlBool &done) { \
    oqmlStatus *s = requalify_node(db, ctx, qleft, _ident, _node, done); \
    if (s) return s; \
    return requalify_node(db, ctx, qright, _ident, _node, done); \
  } \
  virtual oqmlStatus *requalify(Database *db, oqmlContext *ctx, const Attribute **attrs, int attr_cnt, \
				const char *_ident) { \
    oqmlStatus *s = requalify_node(db, ctx, qleft, attrs, attr_cnt, _ident); \
    if (s) return s; \
    return requalify_node(db, ctx, qright, attrs, attr_cnt, _ident); \
  } \
  virtual oqmlStatus *requalify_back(Database *db, oqmlContext *ctx) { \
    oqmlStatus *s = requalify_node_back(db, ctx, qleft); \
    if (s) return s; \
    return requalify_node_back(db, ctx, qright); \
  }

#define OQML_STD_REQUAL_1() \
  virtual oqmlStatus *requalify(Database *db, oqmlContext *ctx, const char *_ident, oqmlNode *node, oqmlBool &done) { \
    return requalify_node(db, ctx, ql, _ident, node, done); \
  } \
  virtual oqmlStatus *requalify(Database *db, oqmlContext *ctx, const Attribute **attrs, int attr_cnt, \
				const char *_ident) { \
    return requalify_node(db, ctx, ql, attrs, attr_cnt, _ident); \
  }  \
  virtual oqmlStatus *requalify_back(Database *db, oqmlContext *ctx) { \
    return requalify_node_back(db, ctx, ql); \
  }

#define OQML_STD_REQUAL_0() \
  virtual oqmlStatus *requalify(Database *, oqmlContext *, const char *_ident, oqmlNode *node, oqmlBool &) { \
    return oqmlSuccess; \
  } \
  virtual oqmlStatus *requalify(Database *, oqmlContext *, const Attribute **attrs, int attr_cnt, \
				const char *_ident) { \
    return oqmlSuccess; \
  } \
  virtual oqmlStatus *requalify_back(Database *, oqmlContext *) { \
    return oqmlSuccess; \
  } \

  class oqmlComp : public oqmlNode {
  protected:
    oqmlIterator *iter;
    oqmlAtom *cst_atom;
    oqmlNode * qleft;
    oqmlNode * qright;
    char *opstr;
    oqmlStatus *optimize(Database *db, oqmlContext *ctx);

  public:
    oqmlComp(oqmlTYPE, oqmlNode *, oqmlNode *, const char *opstr);
    virtual oqmlStatus *makeIterator(Database *, oqmlDotContext *,
				     oqmlAtom *) = 0;
    virtual oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    virtual std::string toString() const;
    oqmlIterator *getIterator() {return iter;}
    virtual oqmlStatus *compile(Database *db, oqmlContext *ctx);
    virtual oqmlStatus *eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp * = NULL, oqmlAtom * = NULL);
    virtual void evalType(Database *db, oqmlContext *ctx, oqmlAtomType *);
    virtual oqmlBool   isConstant() const;
    virtual oqmlStatus *compare(Database *db, oqmlContext *ctx, oqmlAtomList *,
				oqmlAtomList *, oqmlAtomList **);
    oqmlStatus *preEvalSelect(Database *db, oqmlContext *ctx,
			      const char *ident, oqmlBool &done,
			      unsigned int &cnt, oqmlBool = oqml_True);
    oqmlComp *asComp() {return this;}
    oqmlBool appearsMoreOftenThan(oqmlComp *) const;
    oqmlBool hasDotIdent(const char *);
    oqmlStatus *reinit(Database *db, oqmlContext *ctx);
    oqmlNode *getLeft() {return qleft;}
    oqmlNode *getRight() {return qright;}

    OQML_STD_REQUAL_2()

      oqmlStatus *preEvalSelectRealize(Database *db, oqmlContext *ctx,
				       const char *ident, oqmlBool &done,
				       oqmlAtomList **alist, oqmlBool firstPass);

    oqmlBool evalDone;
    oqmlBool needReinit;
    void lock();
    void unlock();
    ~oqmlComp();

    virtual oqmlStatus *complete(Database *, oqmlContext *ctx, oqmlAtom *) {
      return oqmlSuccess;
    }

    oqmlBool hasIdent(const char *_ident) {
      return OQMLBOOL(qleft->hasIdent(_ident) || qright->hasIdent(_ident));
    }
  };


#define OQML_STD_DECL(X) \
  ~X(); \
  virtual std::string toString() const; \
  oqmlStatus *compile(Database *db, oqmlContext *ctx); \
  oqmlStatus *eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, \
		  oqmlComp * = NULL, oqmlAtom * = NULL); \
  void evalType(Database *db, oqmlContext *ctx, oqmlAtomType *); \
  oqmlBool   isConstant() const

#define OQML_STD_DECL_0(X) \
  X(); \
  OQML_STD_REQUAL_0() \
  OQML_STD_DECL(X)

#define OQML_STD_DECL_1(X) \
  X(oqmlNode *); \
  void lock() {oqmlNode::lock(); if (ql) ql->lock();} \
  void unlock() {oqmlNode::unlock(); if (ql) ql->unlock();} \
  oqmlBool hasIdent(const char *_ident) { \
    return OQMLBOOL(ql && ql->hasIdent(_ident)); \
  } \
  OQML_STD_REQUAL_1() \
  OQML_STD_DECL(X)

#define OQML_STD_DECL_2(X) \
  X(oqmlNode *, oqmlNode *); \
  void lock() { \
     oqmlNode::lock(); if (qleft) qleft->lock(); if (qright) qright->lock(); \
  } \
  void unlock() { \
     oqmlNode::unlock(); if (qleft) qleft->unlock(); if (qright) qright->unlock(); \
  } \
  oqmlBool hasIdent(const char *_ident) { \
     return OQMLBOOL((qleft && qleft->hasIdent(_ident)) || \
                     (qright && qright->hasIdent(_ident))); \
  } \
  OQML_STD_REQUAL_2() \
  OQML_STD_DECL(X)

  /* Class.hierarchy */
  class oqmlIdent : public oqmlNode {
    char * name;
    Class *cls, *__class;
    oqmlAtom *cst_atom;
    oqmlStatus *evalQuery(Database *db, oqmlContext *ctx,
			  oqmlAtomList **alist);
  public:
    oqmlIdent(const char *);
    OQML_STD_DECL(oqmlIdent);
    oqmlStatus *evalLeft(Database *db, oqmlContext *ctx,
			 oqmlAtom **a, int &idx);
    const char *getName() const;
    oqmlBool hasIdent(const char *_name) {
      return _name && !strcmp(_name, name) ? oqml_True : oqml_False;
    }

    oqmlIdent *asIdent() {return this;}
    static void initEnumValues(Database *, oqmlContext *ctx);
  };

  class oqmlString : public oqmlNode {
    char * s;

  public:
    oqmlString(const char *);
    OQML_STD_DECL(oqmlString);
    OQML_STD_REQUAL_0()
      };

  class oqmlInt : public oqmlNode {
    eyedblib::int64 i;
    oqmlNode *ql;

  public:
    oqmlInt(eyedblib::int64);
    OQML_STD_DECL_1(oqmlInt);
  };

  class oqmlChar : public oqmlNode {
    char c;

  public:
    oqmlChar(char);
    OQML_STD_DECL(oqmlChar);
    OQML_STD_REQUAL_0()
      };

  class oqmlFloat : public oqmlNode {
    double f;

  public:
    oqmlFloat(double);
    OQML_STD_DECL(oqmlFloat);
    OQML_STD_REQUAL_0()
      };

  class oqmlOid : public oqmlNode {
    Oid oid;
    Class *cls;
    oqmlNode *ql;

  public:
    oqmlOid(const Oid&);
    OQML_STD_DECL_1(oqmlOid);
  };

  class oqmlObject : public oqmlNode {
    Object *o;
    char *s;
    pointer_int_t idx;
    oqmlNode *ql;

  public:
    oqmlObject(const char *);
    oqmlObject(Object *, unsigned int);
    OQML_STD_DECL_1(oqmlObject);
  };

  class oqmlNil : public oqmlNode {

  public:
    OQML_STD_DECL_0(oqmlNil);
  };

  class oqmlNull : public oqmlNode {

  public:
    OQML_STD_DECL_0(oqmlNull);
  };

  class oqmlTrue : public oqmlNode {

  public:
    OQML_STD_DECL_0(oqmlTrue);
  };

  class oqmlFalse : public oqmlNode {

  public:
    OQML_STD_DECL_0(oqmlFalse);
  };

  class oqmlRange : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlBool left_incl, right_incl;
    oqmlBool is_between;

  public:
    oqmlRange(oqmlNode *, oqmlBool, oqmlNode *, oqmlBool, oqmlBool = oqml_False);
    OQML_STD_DECL_2(oqmlRange);
  };

  class oqmlStringOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlStringOp);
  };

  class oqmlIntOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlIntOp);
  };

  class oqmlCharOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlCharOp);
  };

  class oqmlFloatOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlFloatOp);
  };

  class oqmlOidOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlOidOp);
  };

  class oqmlIdentOp : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlIdentOp);
  };

  class oqmlMod : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlMod);
  };

  class oqmlAbs : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlAbs);
  };

  class oqmlUnion : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlUnion);
  };

  class oqmlIntersect : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlIntersect);
  };

  class oqmlExcept : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlExcept);
  };

  class oqml_ParamLink {

  public:
    char *ident;
    oqmlNode *node;
    oqmlBool unval;
    oqml_ParamLink *next;
    oqml_ParamLink(const char *, oqmlNode * = 0);
    ~oqml_ParamLink();
  };

  class oqml_ParamList {

  public:
    int cnt;
    int min_cnt;
    oqml_ParamLink *first;
    oqml_ParamLink *last;
    oqml_ParamList(const char *, oqmlNode * = 0);
    void add(const char *, oqmlNode * = 0);
    void lock();
    void unlock();
    std::string toString() const;
    ~oqml_ParamList();
  };

  class oqmlFunction : public oqmlNode {
    char *name;
    oqmlNode *ql;
    oqml_ParamList *param_list;

  public:
    oqmlFunction(const char *, oqml_ParamList *, oqmlNode *);
    OQML_STD_DECL(oqmlFunction);
    OQML_STD_REQUAL_0()
      void lock();
    void unlock();
  };

  class oqmlCall : public oqmlNode {
    char *name;
    oqmlFunctionEntry *entry;
    oqml_List *list;
    oqmlNode *ql;
    oqmlNode *qlbuiltin; // context argument
    oqmlNode *last_builtin;
    oqmlFunctionEntry *last_entry;
    oqmlBool deferredEval;
    oqmlBool checkBuiltIn(Database *, oqmlNode *, const char *, int);
    oqmlBool compiling;
    oqmlStatus *realize(Database *db, oqmlContext *, oqmlAtomList **);

  public:
    oqmlCall(const char *, oqml_List *);
    oqmlCall(const char *, oqmlNode *);
    oqmlCall(oqmlNode *, oqml_List *);

    OQML_STD_DECL(oqmlCall);
    oqmlBool hasIdent(const char *);

    oqmlNode *getBuiltIn() {return qlbuiltin;}
    oqmlStatus *preCompile(Database *, oqmlContext *);
    oqmlStatus *postCompile(Database *, oqmlContext *, oqmlBool);
    void lock();
    void unlock();

    oqmlStatus *requalify(Database *, oqmlContext *, const char *_ident, oqmlNode *node, oqmlBool &done);
    oqmlStatus *requalify(Database *, oqmlContext *, const Attribute **attrs, int attr_cnt, 
			  const char *_ident);
    oqmlStatus *requalify_back(Database *, oqmlContext *);

    static oqmlStatus *realizePostAction(Database *db, oqmlContext *ctx,
					 const char *name,
					 oqmlFunctionEntry *entry, 
					 oqmlAtom_string *rs,
					 oqmlAtom *ra,
					 oqmlAtomList **alist);

    static oqmlStatus *realizeCall(Database *db, oqmlContext *,
				   oqmlFunctionEntry *entry,
				   oqmlAtomList **);
    static oqmlBool getBuiltIn(Database *, oqmlNode *, const char *name, int, oqmlNode ** = 0, oqml_List * = 0);

    const char *getName() const {return name;}
    oqml_List *getList() {return list;}
  };

  class oqmlMethodCall : public oqmlNode {
    char *clsname;
    char *mthname;
    oqml_List * list;
    oqmlBool deleteList;
    Method *mth;
    const Class *cls;
    oqmlBool is_compiled;
    oqmlNode *call;
    struct {
      const Class *cls;
      Bool isStatic;
      Method *xmth;
    } last;
  public:
    enum Match {
      no_match = 0,
      exact_match,
      match
    };
  private:
    void init(const char *, const char *, oqml_List *);
    oqmlStatus *checkStaticMethod();
    oqmlStatus *findStaticMethod(Database *db, oqmlContext *);
    oqmlStatus *evalList(Database *db, oqmlContext *ctx);
    const char *getSignature(oqmlContext *);
    oqmlStatus *checkArguments(Database *db, oqmlContext *ctx,
			       const Method *, Match * = 0);
    oqmlStatus *noMethod(Bool isStatic, oqmlContext *,
			 const Method **mths = 0,
			 int mth_cnt = 0);
    oqmlStatus *resolveMethod(Database *db, oqmlContext *ctx,
			      Bool isStatic, Object *, Method *&xmth);
    unsigned int getArgCount(const Argument *arg, ArgType_Type odl_type);
    oqmlStatus *makeArg(Argument &tmp, ArgType_Type odl_type,
			const Argument *arg, int j);
    oqmlStatus *buildArgArray(Argument *arg, oqmlAtomList *list,
			      ArgType_Type odl_type, int n);
    oqmlStatus *fillArgArray(Signature *, Argument *arg, Argument tmp,
			     ArgType_Type odl_type, int j);
    oqmlStatus *atomToArg(Database *db, oqmlContext *ctx,
			  Signature *sign, oqmlAtom *,
			  ArgType *, ArgType_Type, Argument *, int);
    oqmlStatus *atomsToArgs(Database *db, oqmlContext *ctx,
			    Method *xmth, ArgArray &);
    oqmlStatus *argToAtom(const Argument *arg, int n, oqmlAtomType &at,
			  oqmlAtom *&);
    oqmlStatus *argsToAtoms(Database *db, oqmlContext *ctx,
			    Method *xmth, const ArgArray &,
			    const Argument &retarg, oqmlAtom *&retatom);
    oqmlStatus *applyC(Database *db, oqmlContext *ctx,
		       Method *xmth, oqmlAtomList **alist,
		       Object * = 0, const Oid *oid = 0);
    oqmlStatus *applyOQL(Database *db, oqmlContext *ctx,
			 Method *xmth, oqmlAtomList **alist,
			 Object * = 0, const Oid *oid = 0);
    static Match compareType(oqmlContext *ctx, int n, oqmlAtom *x,
			     int odl_type, Bool strict);
    oqmlStatus *typeMismatch(const Signature *sign, oqmlAtomType &at, int n);
    oqmlStatus *typeMismatch(ArgType *type, Object *o, int n);
    oqmlBool compareAtomTypes();
    oqmlAtom **atoms;
    oqmlAtom **tmp_atoms;
    oqmlBool noParenthesis;

  public:
    oqmlMethodCall(const char *, oqml_List *, oqmlBool = oqml_False);
    oqmlMethodCall(const char *, const char *, oqml_List *,
		   oqmlNode * = 0);

    OQML_STD_DECL(oqmlMethodCall);
    oqmlBool hasIdent(const char *);
    static oqmlStatus *applyTrigger(Database *db, Trigger *trig,
				    Object *o, const Oid *oid);


    void lock();
    void unlock();

    oqmlStatus *perform(Database *db, oqmlContext *ctx,
			Object *, const Oid &oid, const Class *cls,
			oqmlAtomList **alist);
    oqmlBool isCompiled() const {return is_compiled;}
  };

  class oqmlAdd : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlBool unary;

  public:
    oqmlAdd(oqmlNode *, oqmlNode *, oqmlBool unary);
    OQML_STD_DECL_2(oqmlAdd);
  };

  class oqmlSub : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlBool unary;

  public:
    oqmlSub(oqmlNode *, oqmlNode *, oqmlBool unary);
    OQML_STD_DECL_2(oqmlSub);
  };

  class oqmlMul : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlMul);
  };

  class oqmlDiv : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlDiv);
  };

  class oqmlShr : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlShr);
  };

  class oqmlShl : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlShl);
  };

  class oqmlXor : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlXor);
  };

  class oqmlTilde : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlTilde);
  };

  class oqmlUnset : public oqmlNode {
    oqmlNode * ql;
    char *ident;

  public:
    OQML_STD_DECL_1(oqmlUnset);
  };

  class oqmlIsset : public oqmlNode {
    oqmlNode * ql;
    char *ident;

  public:
    OQML_STD_DECL_1(oqmlIsset);
  };

  class oqmlSet : public oqmlNode {
    char *ident;
    oqmlNode *ql;

  public:
    oqmlSet(const char *, oqmlNode *);
    OQML_STD_DECL(oqmlSet);

    void lock() {oqmlNode::lock(); ql->lock();}
    void unlock() {oqmlNode::unlock(); ql->unlock();}
  };

  class oqmlTypeOf : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlTypeOf);
  };

  class oqmlBreak : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlBreak);
  };

  class oqmlEval : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlEval);
  };

  class oqmlUnval : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlUnval);
  };

  class oqmlPrint : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlPrint);
  };

  class oqmlImport : public oqmlNode {
    oqmlNode *ql;
    static const char *get_next_path(const char *oqmlpath, int &idx);
    oqmlStatus *file_to_buf(const char *file, int fd, char *&);
    oqmlStatus *import_realize(Database *db, oqmlAtomList **alist,
			       const char *file, const char *dir,
			       oqmlBool &check);

  public:
    OQML_STD_DECL_1(oqmlImport);
  };

  class oqmlValRefOf : public oqmlNode {

  public:
    oqmlValRefOf(oqmlNode *, oqmlTYPE, const char *);
    void lock() {oqmlNode::lock(); if (ql) ql->lock();}
    void unlock() {oqmlNode::unlock(); if (ql) ql->unlock();}
    ~oqmlValRefOf();

  protected:
    oqmlNode *ql;
    char *ident; // context argument
    char *opstr;
    static char *makeIdent(oqmlContext *ctx, const char *ident);
    oqmlStatus *realizeIdent(Database *db, oqmlContext *ctx);
  };

  class oqmlRefOf : public oqmlValRefOf {

  public:
    oqmlRefOf(oqmlNode *);
    oqmlStatus *evalLeft(Database *db, oqmlContext *ctx, oqmlAtom **a,
			 int &idx);
    OQML_STD_DECL(oqmlRefOf);
  };

  class oqmlValOf : public oqmlValRefOf {

  public:
    oqmlValOf(oqmlNode *);
    oqmlStatus *evalLeft(Database *db, oqmlContext *ctx, oqmlAtom **a,
			 int &idx);
    OQML_STD_DECL(oqmlValOf);
  };

  class oqmlThrow : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlThrow);
  };

  class oqmlReturn : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlReturn);
  };

  class oqmlWhile : public oqmlNode {
    oqmlNode * qleft, * qright;

  public:
    OQML_STD_DECL_2(oqmlWhile);
  };

  class oqmlDoWhile : public oqmlNode {
    oqmlNode * qleft, * qright;

  public:
    OQML_STD_DECL_2(oqmlDoWhile);
  };

  class oqmlForDo : public oqmlNode {
    oqmlNode *start, *cond, *next, *body;
    char *ident;

  public:
    OQML_STD_DECL(oqmlForDo);
    oqmlForDo(oqmlNode *, oqmlNode *, oqmlNode *, oqmlNode *);
    oqmlForDo(const char *, oqmlNode *, oqmlNode *, oqmlNode *, oqmlNode *);
    oqmlBool hasIdent(const char *);
    void lock();
    void unlock();
  };

  class oqmlStruct : public oqmlNode {
    oqml_IdentList *list;

  public:
    OQML_STD_DECL(oqmlStruct);
    oqmlBool hasIdent(const char *);
    oqmlStruct(oqml_IdentList *);
    void lock();
    void unlock();
  };

  class oqmlAAnd : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlAAnd);
  };

  class oqmlAOr : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlAOr);
  };

  class oqmlEqual : public oqmlComp {

  public:
    oqmlEqual(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlEqual();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlDiff : public oqmlComp {

  public:
    oqmlDiff(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlDiff();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlInf : public oqmlComp {

  public:
    oqmlInf(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlInf();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlInfEq : public oqmlComp {

  public:
    oqmlInfEq(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlInfEq();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlSup : public oqmlComp {

  public:
    oqmlSup(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlSup();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlSupEq : public oqmlComp {

  public:
    oqmlSupEq(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlSupEq();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlBetween : public oqmlComp {

  public:
    oqmlBetween(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlBetween();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
    oqmlNode *requalifyNot();
  };

  class oqmlNotBetween : public oqmlComp {

  public:
    oqmlNotBetween(oqmlNode *, oqmlNode *);
    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    ~oqmlNotBetween();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlRegex : public oqmlComp {

    oqmlStatus *eval_realize(oqmlAtom *, oqmlAtomList **);

  protected:
#ifdef USE_LIBGEN
    char *re;
#endif
    regex_t *re;

  public:
    oqmlRegex(oqmlTYPE t, oqmlNode *_qleft, oqmlNode *_qright, const char *_opstr) :
      oqmlComp(t, _qleft, _qright, _opstr), re(0) {
    }

    oqmlStatus *estimate(Database *, oqmlContext *, unsigned int &);
    oqmlStatus *compile(Database *db, oqmlContext *ctx);
    oqmlStatus *eval(Database *db, oqmlContext *ctx, oqmlAtomList **alist, oqmlComp * = NULL, oqmlAtom * = NULL);
    ~oqmlRegex()
    {
#ifdef USE_LIBGEN
      free(re);
#endif
      if (re)
	regfree( re);
    }

    oqmlStatus *complete(Database *, oqmlContext *ctx, oqmlAtom *);

  protected:
    oqmlIterator *makeIter(Database *, oqmlDotContext *,
			   oqmlAtom *, bool iregcmp);
  };

  class oqmlRegCmp : public oqmlRegex {

  public:
    oqmlRegCmp(oqmlNode *, oqmlNode *);
    ~oqmlRegCmp();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlRegICmp : public oqmlRegex {

  public:
    oqmlRegICmp(oqmlNode *, oqmlNode *);
    ~oqmlRegICmp();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlRegDiff : public oqmlRegex {

  public:
    oqmlRegDiff(oqmlNode *, oqmlNode *);
    ~oqmlRegDiff();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlRegIDiff : public oqmlRegex {

  public:
    oqmlRegIDiff(oqmlNode *, oqmlNode *);
    ~oqmlRegIDiff();
    oqmlStatus *makeIterator(Database *, oqmlDotContext *, oqmlAtom *);
  };

  class oqmlIf : public oqmlNode {
    oqmlNode *qcond, *qthen, *qelse;
    oqmlBool qthen_compiled, qelse_compiled;
    oqmlBool is_cond_expr;

  public:
    oqmlIf(oqmlNode *, oqmlNode *, oqmlNode *, oqmlBool);
    OQML_STD_DECL(oqmlIf);
    oqmlBool hasIdent(const char *);

    void lock() {
      oqmlNode::lock(); 
      qcond->lock();
      qthen->lock();
      if (qelse) qelse->lock();
    }

    void unlock() {
      oqmlNode::unlock(); 
      qcond->unlock();
      qthen->unlock();
      if (qelse) qelse->unlock();
    }
  };

  class oqmlLNot : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlLNot);
  };

  class oqmlLinkItem;

  class oqml_ArrayLink {

  public:
    oqmlBool wholeRange;
    oqmlNode *qleft, *qright;
    oqml_ArrayLink *next;
    oqml_ArrayLink(oqmlBool);
    oqml_ArrayLink(oqmlNode *, oqmlNode * = NULL);
    oqmlStatus *compile(Database *db, oqmlContext *ctx);
    oqmlStatus *eval(oqmlNode *, Database *db, oqmlContext *ctx, oqmlLinkItem &item);
    oqmlBool hasIdent(const char *);
    virtual std::string toString() const;
    void lock();
    void unlock();
    oqmlBool wholeCount;
    oqmlBool isGetCount() const;

    ~oqml_ArrayLink();
  };

  class oqml_ArrayList {

  public:
    int count;
    oqmlBool wholeRange;
    oqml_ArrayLink *first;
    oqml_ArrayLink *last;
    oqml_ArrayList();
    oqmlBool is_getcount;
    oqmlBool is_wholecount;
    oqmlStatus *eval(oqmlNode *, Database *, oqmlContext *, const char *,
		     const char *, const TypeModifier*, int *, int *, oqmlBool);
    void add(oqml_ArrayLink *);
    oqmlStatus *compile(Database *db, oqmlContext *ctx);
    oqmlStatus *eval(oqmlNode *, Database *db, oqmlContext *ctx, oqmlLinkItem *&item,
		     int &item_cnt);
    virtual std::string toString() const;
    oqmlBool hasIdent(const char *);
    void lock();
    void unlock();
    oqmlStatus *checkCollArray(oqmlNode *node, const Class *cls,
			       const char *attrname);
    oqmlStatus *evalCollArray(oqmlNode *node, Database *db,
			      oqmlContext *ctx,
			      const TypeModifier *tmod, int &ind);

    ~oqml_ArrayList();
  };

  class oqmlArray : public oqmlNode {
    oqmlNode *ql;
    oqml_ArrayList *list;
    oqmlStatus *evalRealize(Database *db, oqmlContext *ctx, 
			    oqmlAtomList **alist, oqmlAtom **a,
			    int *ridx);
    oqmlStatus *evalRealize_1(Database *db, oqmlContext *ctx, 
			      oqmlAtom *x, oqmlAtomList **alist, oqmlAtom **a,
			      int *ridx);
    oqmlStatus *evalRealize_2(Database *db, oqmlContext *ctx, 
			      oqmlAtom *x, oqmlAtomList **alist, oqmlAtom **a,
			      int *ridx);
    oqmlStatus *checkRange(oqmlLinkItem *items, int dim, int idx, int len,
			   oqmlBool &stop, const char *msg);

    oqmlBool delegationArray, returnStruct;
  public:
    OQML_STD_DECL(oqmlArray);
    oqmlArray(oqmlNode *);
    oqmlArray(oqmlNode *, oqml_ArrayList *, oqmlBool);
    oqmlBool hasIdent(const char *);

    void lock();
    void unlock();

    static oqmlStatus *evalMake(Database *db, oqmlContext *ctx,
				Object *o, oqml_ArrayList *list,
				oqmlBool, oqmlAtomList **);

    oqmlStatus *evalLeft(Database *db, oqmlContext *ctx, oqmlAtom **a,
			 int &idx);

    void add(oqml_ArrayLink *);
    oqmlNode *getLeft();
    oqml_ArrayList *getArrayList();
  };

  class oqmlCast : public oqmlNode {
    char * ident;
    oqmlNode * qright;

  public:
    oqmlCast(const char *, oqmlNode *);
    OQML_STD_DECL(oqmlCast);
  };

  class oqmlAnd : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    int xc;
    oqmlStatus *check(Database *db, oqmlContext *ctx);
    oqmlStatus *eval_0(Database *db, oqmlContext *ctx, oqmlAtomList **alist);
    oqmlStatus *eval_1(Database *db, oqmlContext *ctx, oqmlAtomList **alist);
    oqmlNode *optimize_realize(Database *, oqmlContext *, oqmlNode *, int &);
    void optimize(Database *, oqmlContext *);

  public:
    OQML_STD_DECL_2(oqmlAnd);
  };

  class oqmlOr : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlOr);
  };

  class oqmlLAnd : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlNode * node;
    oqmlBool isLiteral;
    oqmlNode *intersect_pred;
    oqmlBool must_intersect;
    oqmlBool estimated;
    oqmlBool requalified;
    unsigned int r_first, r_second;

    oqmlStatus *preEvalSelect_optim(Database *db, oqmlContext *ctx,
				    const char *ident, oqmlBool &done,
				    unsigned int &cnt, oqmlBool firstPass);
    oqmlStatus *preEvalSelect_nooptim(Database *db, oqmlContext *ctx,
				      const char *ident, oqmlBool &done,
				      unsigned int &cnt, oqmlBool firstPass);
    oqmlNode *requalifyRange();
    oqmlStatus *estimateLAnd(Database *db, oqmlContext *ctx);
    static oqmlBool isAndOptim(oqmlContext *);

  public:
    OQML_STD_DECL(oqmlLAnd);
    oqmlLAnd(oqmlNode *, oqmlNode *, oqmlBool, oqmlNode *must_intersect = 0);

    void lock();
    void unlock();

    oqmlBool hasIdent(const char *_ident) {
      return (node ? node->hasIdent(_ident) :
	      OQMLBOOL(qleft->hasIdent(_ident) ||
		       qright->hasIdent(_ident)));
    }

    oqmlStatus *preEvalSelect(Database *db, oqmlContext *ctx,
			      const char *ident, oqmlBool &done,
			      unsigned int &cnt, oqmlBool firstPass = oqml_True);
    oqmlStatus *requalify(Database *, oqmlContext *, const char *_ident, oqmlNode *_node, oqmlBool &done);
    oqmlStatus *requalify(Database *db, oqmlContext *ctx, const Attribute **attrs, int attr_cnt,
			  const char *_ident) {
      oqmlStatus *s = requalify_node(db, ctx, qleft, attrs, attr_cnt, _ident);
      if (s) return s;
      return requalify_node(db, ctx, qright, attrs, attr_cnt, _ident);
    }

    oqmlStatus *requalify_back(Database *, oqmlContext *);
    static const char *getDefaultRule();
  };

  class oqmlLOr : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlNode * node;
    oqmlBool isLiteral;

  public:
    OQML_STD_DECL(oqmlLOr);
    oqmlLOr(oqmlNode *, oqmlNode *, oqmlBool);

    OQML_STD_REQUAL_2()

      void lock();
    void unlock();

    oqmlBool hasIdent(const char *_ident) {
      return (node ? node->hasIdent(_ident) :
	      OQMLBOOL(qleft->hasIdent(_ident) ||
		       qright->hasIdent(_ident)));
    }

    oqmlStatus *preEvalSelect(Database *db, oqmlContext *ctx,
			      const char *ident, oqmlBool &done,
			      unsigned int &cnt, oqmlBool firstPass = oqml_True);
  };

  class oqmlComma : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlComma);
    oqmlComma(oqmlNode *, oqmlNode *, oqmlBool);
  };

  class oqmlNew : public oqmlNode {

    oqmlStatus *makeAtom(Database *db, oqmlContext *ctx, Object *o,
			 oqmlAtom *&);

    oqmlStatus *compileNode(Database *db, oqmlContext *ctx,
			    const Class *cls);
    oqmlStatus *compileIdent(Database *db, oqmlContext *ctx,
			     const Class *cls, oqmlNode *left,
			     int n, int &ndims);
    oqmlStatus *compileDot(Database *db, oqmlContext *ctx,
			   const Class *cls, oqmlNode *left,
			   int n, int &ndims);
    oqmlStatus *compileArray(Database *db, oqmlContext *ctx,
			     const Class *cls, oqmlNode *left,
			     int n, int &ndims);
    oqmlStatus *evalNode(Database *db, oqmlContext *ctx,
			 oqmlAtomList **alist);

    oqmlStatus *evalItem(Database *db, oqmlContext *ctx,
			 Agregat *o, oqml_IdentLink *l, int n,
			 oqmlBool &, oqmlAtomList **alist);

  public:
    char *ident;
    oqml_IdentList *ident_list;
    oqmlNode *ql;
    oqmlAtom *cst_atom;
    Class *_class;
    oqmlNode *location;
    Database *newdb;
    char *quoted_odl;

    class newCompile {

    public:
      Class *cls;
      int cnt;
      Attribute **attr;
      char **attrname;
      oqml_ArrayList **list;
      oqmlDotContext **dot_ctx;
    
      newCompile(Class *, int);
      ~newCompile();
    } *comp;

  public:
    oqmlNew(oqmlNode *, const char *, oqml_IdentList *);
    oqmlNew(oqmlNode *, const char *, oqmlNode *);
    oqmlNew(oqmlNode *, const char *);
    OQML_STD_DECL(oqmlNew);
    void lock();
    void unlock();
  };

  class oqml_CollSpec {

  public:
    char *coll_type;
    char *type_spec;
    char *ident;
    Bool isref;
    Bool ishash;
    char *impl_hints;
    oqml_CollSpec *coll_spec;

    oqml_CollSpec(const char *coll_type, const char *type_spec,
		  oqmlBool isref = oqml_True, const char *name = "",
		  oqmlBool ishash = oqml_True, const char *impl_hints = 0);
    oqml_CollSpec(const char *coll_type, oqml_CollSpec *type_spec,
		  oqmlBool isref = oqml_True, const char *name = "",
		  oqmlBool ishash = oqml_True, const char *impl_hints = 0);
    std::string toString() const;
    ~oqml_CollSpec();
  };

  class oqmlCollection : public oqmlNode {
    oqmlNode *location;
    Database *newdb;
    oqmlNode *ql;
    oqml_CollSpec *coll_spec;
    oqmlAtomType atref;

    Class *cls;
    enum {
      collset,
      collbag,
      colllist,
      collarray
    } what;

  public:
    oqmlCollection(oqmlNode *location, oqml_CollSpec *, oqmlNode *);
    OQML_STD_DECL(oqmlCollection);
    void lock();
    void unlock();
  };

  class oqmlColl : public oqmlNode {

    oqml_List *list;

  public:
    oqmlColl(oqml_List *, oqmlTYPE type);
    OQML_STD_DECL(oqmlColl);
    void lock()   {oqmlNode::lock();   if (list) list->lock();}
    void unlock() {oqmlNode::unlock(); if (list) list->unlock();}
    virtual const char *getTypeName() const = 0;
    virtual oqmlAtom_coll *makeAtomColl(oqmlAtomList *l) = 0;
    oqml_List *getList() {return list;}
  };

  class oqmlListColl : public oqmlColl {

  public:
    oqmlListColl(oqml_List *);
    const char *getTypeName() const {return "list";}
    oqmlAtom_coll *makeAtomColl(oqmlAtomList *l) {
      return new oqmlAtom_list(l);
    }
  };

  class oqmlSetColl : public oqmlColl {

  public:
    oqmlSetColl(oqml_List *);
    const char *getTypeName() const {return "set";}
    oqmlAtom_coll *makeAtomColl(oqmlAtomList *l) {
      return new oqmlAtom_set(l);
    }
  };

  class oqmlBagColl : public oqmlColl {

  public:
    oqmlBagColl(oqml_List *);
    const char *getTypeName() const {return "bag";}
    oqmlAtom_coll *makeAtomColl(oqmlAtomList *l) {
      return new oqmlAtom_bag(l);
    }
  };

  class oqmlArrayColl : public oqmlColl {

  public:
    oqmlArrayColl(oqml_List *);
    const char *getTypeName() const {return "array";}
    oqmlAtom_coll *makeAtomColl(oqmlAtomList *l) {
      return new oqmlAtom_array(l);
    }
  };

  class oqml_ClassSpec {

  public:
    char *classname;
    char *parentname;

    oqml_ClassSpec(char *, char *);
    ~oqml_ClassSpec();
  };

  class oqmlDelete : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlDelete);
  };

  class oqmlClassOf : public oqmlNode {
    oqmlNode *ql;

  public:
    OQML_STD_DECL_1(oqmlClassOf);
  };

  class oqmlCastIdent : public oqmlNode {
    char *name;
    char *modname;

  public:
    oqmlCastIdent(const char *, const char *);
    OQML_STD_DECL(oqmlCastIdent);
    const char *getName(const char **) const;
  };

  class oqmlDot : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlNode * qlmth;
    oqmlBool isArrow;
    oqmlDotContext *dot_ctx;
    oqmlStatus *getAttrRealize(const Class *, const char *,
			       const Attribute **);
    oqmlStatus * getAttr(Database *db, oqmlContext *ctx,
			 const Class *cls, oqmlAtom *,
			 const char *name, const Attribute **attr,
			 oqmlAtom **curatom);
    oqmlStatus *oqmlDot_left(Database *db,
			     oqmlContext *ctx,
			     const Class *cls,
			     oqmlAtom *curatom,
			     Attribute **attr,
			     oqmlAtom **rcuratom,
			     Class **castcls,
			     char **attrname);

    oqmlStatus *complete(Database *db, oqmlContext *ctx);
    oqmlStatus *eval_realize_list(Database *db, oqmlContext *ctx,
				  oqmlAtomList *,
				  oqmlAtom *value, oqmlAtomList **, int level);
    static void makeUnion(oqmlContext *, oqmlAtom_select *, oqmlAtomList *list);
    static void makeIntersect(oqmlContext *, oqmlAtom_select *, oqmlAtomList *list);
    static void makeSet(oqmlContext *, oqmlAtom_select *, oqmlAtomList *list);

  public:
    OQML_STD_DECL(oqmlDot);
    oqmlBool hasIdent(const char *);
    oqmlDot(oqmlNode *, oqmlNode *, oqmlBool isArrow);
    oqmlStatus *isScope(Database *db, const char* x, const char*& attrname,
			const Class *&cls, const Attribute **attr = 0);
    oqmlStatus *populate(Database *db, oqmlContext *ctx, oqmlAtomList *,
			 oqmlBool makeUnion);
    void lock();
    void unlock();

    void setIndexHint(oqmlContext *, oqmlBool);
    oqmlStatus *hasIndex(Database *db, oqmlContext *ctx, oqmlBool &);
    oqmlStatus *reinit(Database *db, oqmlContext *ctx,
		       oqmlBool compile = oqml_True);
    const char *getLeftIdent() const;
    void replaceLeftIdent(const char *ident, oqmlNode *node, oqmlBool &done);
    oqmlBool constructed;
    oqmlBool populated;
    oqmlStatus *construct(Database *, oqmlContext *, const Class *,
			  oqmlAtom *, oqmlDotContext **);
    oqmlStatus *set(Database *db, oqmlContext *ctx, oqmlAtom *value,
		    oqmlAtomList **alist);
    oqmlStatus *compile_start(Database *db, oqmlContext *ctx);
    oqmlStatus *compile_continue(Database *db, oqmlContext *ctx,
				 oqmlDotContext *);
    oqmlStatus *eval_realize(Database *db, oqmlContext *ctx,
			     const Class *cls,
			     oqmlAtom *atom, oqmlAtom *value, oqmlAtomList **);
    oqmlStatus *eval_perform(Database *db, oqmlContext *ctx,
			     oqmlAtom *value, oqmlAtomList **alist);
    virtual oqmlStatus *requalify_back(Database *db, oqmlContext *ctx);

    oqmlArray *make_right_array();
    oqmlDot *make_right_dot(const char *, oqmlBool);
    oqmlDot *make_right_call(oqml_List *);
    oqmlDotContext *getDotContext();
    oqmlStatus *check(Database *, oqmlDotContext *);
    Bool boolean_dot;
    Bool boolean_node;
    char *requal_ident;
    oqmlDot *asDot() {return this;}
  };

  class oqmlXSort : public oqmlNode {

  protected:
    oqml_List * xlist;
    oqmlBool reverse;
    oqmlNode * tosort;

    oqmlXSort(oqmlTYPE _type, oqml_List *_xlist, oqmlBool _reverse) :
      oqmlNode(_type) {
      xlist = _xlist;
      reverse = _reverse;
      tosort = 0;
    }
  };

  class oqmlSort : public oqmlXSort {

  public:
    oqmlSort(oqml_List *, oqmlBool);
    OQML_STD_DECL(oqmlSort);
  };

  class oqmlISort : public oqmlXSort {
    oqmlNode *idxnode;

  public:
    oqmlISort(oqml_List *, oqmlBool);
    OQML_STD_DECL(oqmlISort);
  };

  class oqmlPSort : public oqmlXSort {
    oqmlNode *pred;
    oqmlNode *predarg;

  public:
    oqmlPSort(oqml_List *, oqmlBool);
    OQML_STD_DECL(oqmlPSort);
  };

  class oqmlCompoundStatement : public oqmlNode {
    oqmlNode *node;

  public:
    OQML_STD_DECL(oqmlCompoundStatement);
    oqmlCompoundStatement(oqmlNode *);

    void lock() {oqmlNode::lock(); if (node) node->lock();}
    void unlock() {oqmlNode::unlock(); if (node) node->unlock();}

    oqmlBool hasIdent(const char *_ident) {
      return OQMLBOOL(node && node->hasIdent(_ident));
    }
  };

  class oqmlAssign : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    char *ident; // context argument

  public:
    OQML_STD_DECL_2(oqmlAssign);
  };

  class oqmlSelfIncr : public oqmlNode {
    oqmlNode * ql;
    oqmlNode * qlassign;
    int incr;
    oqmlBool post;

  public:
    oqmlSelfIncr(oqmlNode *, int incr, oqmlBool post);
    OQML_STD_DECL(oqmlSelfIncr);
    void lock() {oqmlNode::lock(); ql->lock(); qlassign->lock();}
    void unlock() {oqmlNode::unlock(); ql->unlock(); qlassign->unlock();}
  };

  class oqmlFlatten : public oqmlNode {
    oqml_List *list;
    int flat;

  public:
    oqmlFlatten(oqml_List *, int flat = 1);
    OQML_STD_DECL(oqmlFlatten);
    void lock();
    void unlock();
    void resetList() {list = NULL;}
  };

  class oqmlElement : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlElement);
  };

  class oqmlElementAt : public oqmlNode {
    oqmlNode * qleft, * qright;

  public:
    OQML_STD_DECL_2(oqmlElementAt);
  };

  class oqmlSetInAt : public oqmlNode {
    oqmlNode * qleft, * qright, * ql3;

  public:
    oqmlSetInAt(oqmlNode *, oqmlNode *, oqmlNode *);
    OQML_STD_DECL(oqmlSetInAt);
    void lock() {oqmlNode::lock(); qleft->lock(); qright->lock(); ql3->lock();}
    void unlock() {oqmlNode::unlock(); qleft->unlock(); qright->unlock(); ql3->unlock();}
  };

  class oqmlUnsetInAt : public oqmlNode {
    oqmlNode * qleft, * qright;

  public:
    OQML_STD_DECL_2(oqmlUnsetInAt);
  };

  class oqmlIn : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;
    oqmlBool hasDotIdent(const char *);
    oqmlBool evalDone;

  public:
    OQML_STD_DECL_2(oqmlIn);
    oqmlStatus *preEvalSelect(Database *db, oqmlContext *ctx,
			      const char *ident, oqmlBool &done,
			      unsigned int &cnt, oqmlBool firstPass = oqml_True);
  };

  class oqml_Interval {

    oqmlStatus *evalNode(Database *db, oqmlContext *ctx,
			 oqmlNode *node, unsigned int &base);
    unsigned int from_base, to_base;
    oqmlNode *from, *to;

  public:
    enum Status {
      Error,
      Success,
      Continue
    };

    oqml_Interval(oqmlNode *_from, oqmlNode *_to);

    oqml_Interval::Status isIn(unsigned int count, unsigned int max) const;
    oqmlBool isAll() const;
    oqmlStatus *compile(Database *db, oqmlContext *ctx);
    oqmlStatus *evalNodes(Database *db, oqmlContext *ctx);

    void lock() {if (from) from->lock(); if (to) to->lock();}
    void unlock() {if (from) from->unlock(); if (to) to->unlock();}

    std::string toString() const;
  };

  class oqmlFor : public oqmlNode {
    oqmlNode * qcoll;
    oqmlNode * qpred;
    oqml_Interval * interval;
    char * ident;
    oqmlBool exists;

  public:
    oqmlFor(oqml_Interval *, const char *ident, oqmlNode *qcoll,
	    oqmlNode *qpred, oqmlBool);
    OQML_STD_DECL(oqmlFor);

    void lock();
    void unlock();
  };

  class oqmlAddTo : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlAddTo);
  };

  class oqmlSuppressFrom : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlSuppressFrom);
  };

  class oqmlAppend : public oqmlNode {
    oqmlNode * qleft;
    oqmlNode * qright;

  public:
    OQML_STD_DECL_2(oqmlAppend);
  };

  class oqmlEmpty : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlEmpty);
  };

  class oqmlDatabase : public oqmlNode {
    char *dbname;
    char *mode;
    oqmlNode * ql;
    Database *cdb;

  public:
    oqmlDatabase(const char *, const char *, oqmlNode *);
    OQML_STD_DECL(oqmlDatabase);
  };

  class oqml_SelectOrder {

  public:
    oqml_List *list;
    oqmlBool asc;
    oqml_SelectOrder(oqml_List *_list, oqmlBool _asc) :
      list(_list), asc(_asc) { }
  };

  class oqmlSelect : public oqmlNode {
    oqmlBool distinct;
    oqmlBool one;
    int logged[64];
    //Database *newdb;
    Database *dbs[64];
    int dbs_cnt;
    oqmlNode *location;
    oqmlNode *projection;
    oqml_IdentList * ident_from_list;
    oqmlNode *where, *group, *having;
    oqml_SelectOrder *order;
    oqml_IdentLink **idents;
    oqmlBool calledFromEval;
    int ident_cnt;
    oqmlBool databaseStatement;
  
    oqmlBool makeIdents();
    oqmlStatus *processRequalification_1(Database *db, oqmlContext *);
    oqmlStatus *processRequalification_2(Database *db, oqmlContext *);
    oqmlStatus *processFromListRequalification(Database *db, oqmlContext *);
    oqmlStatus *processMissingIdentsRequalification(Database *db, oqmlContext *);
    oqmlStatus *evalRequalified(Database *db, oqmlContext *ctx);

    oqmlStatus *processMissingProjRequalification(Database *db, oqmlContext *);

    oqmlStatus *evalCartProdRealize(Database *db, oqmlContext *ctx,
				    oqmlAtomList *rlist, int n,
				    const char * = 0,
				    oqmlAtom ** = 0, int = 0);

    oqmlStatus *evalCartProd(Database *db, oqmlContext *ctx,
			     oqmlAtomList **alist);

    int *list_order;
    oqmlStatus *check_order();
    oqmlStatus *check_order_coll(oqmlNode *);
    oqmlStatus *check_order_simple();
    oqmlStatus *realize_order(oqmlAtomList **);

  public:
    oqmlSelect(oqmlNode *location,
	       oqmlBool distinct,
	       oqmlBool one,
	       oqmlNode *projection,
	       oqml_IdentList * = 0,
	       oqmlNode *where = 0,
	       oqmlNode *group = 0,
	       oqmlNode *having = 0,
	       oqml_SelectOrder *order = 0);

    void setDatabaseStatement() {databaseStatement = oqml_True;}
    OQML_STD_DECL(oqmlSelect);

    oqmlBool usesFromIdent(oqmlNode *);
    void lock();
    void unlock();
    oqmlBool mayBeRequalified() const {return oqml_False;}
  };

  class oqmlCount : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlCount);
  };

  class oqmlGetIdent : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlGetIdent);
  };

  class oqmlTimeFormat : public oqmlNode {
    oqml_List *list;
    oqmlNode *time;
    oqmlNode *format;

  public:
    OQML_STD_DECL(oqmlTimeFormat);
    oqmlTimeFormat(oqml_List *);
  };

  class oqmlContents : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlContents);
  };

  class oqmlBodyOf : public oqmlNode {
    oqmlNode * ql;
    char *ident;

  public:
    OQML_STD_DECL_1(oqmlBodyOf);
  };

  class oqmlScopeOf : public oqmlNode {
    oqmlNode * ql;
    char *ident;

  public:
    OQML_STD_DECL_1(oqmlScopeOf);
  };

  class oqmlStructOf : public oqmlNode {
    oqmlNode * ql;

  public:
    OQML_STD_DECL_1(oqmlStructOf);
  };

  class oqmlPush : public oqmlNode {
    char *ident;

  public:
    oqmlPush(const char *);
    OQML_STD_DECL(oqmlPush);
  };

  class oqmlPop : public oqmlNode {
    char *ident;

  public:
    oqmlPop(const char *);
    OQML_STD_DECL(oqmlPop);
  };

  class oqmlForEach : public oqmlNode {
    char * ident;
    oqmlNode *in;
    oqmlNode *action;
    oqmlAtomType sym_type;

  public:
    oqmlForEach(const char *, oqmlNode *, oqmlNode *);
    OQML_STD_DECL(oqmlForEach);
    oqmlBool hasIdent(const char *);

    void lock();
    void unlock();

    void setAction(oqmlNode *);
  };

#define oqmlMAXDESC 16
  class oqmlIterCursor;
  class oqmlDotDesc {

  public:
    const Class *cls;
    const Class *cls_orig;
    const Attribute *attr;
    char *attrname;
    oqmlAtom *curatom;
    const TypeModifier *mod;
    oqml_ArrayList *array;
    int idx_cnt;
    Index **idxs;
    eyedbsm::Idx **idxse;
    Bool isref;
    int sz_item;
    int mode;
    int key_len;
    Bool is_union;
    eyedbsm::Idx::Key *key;
    unsigned char *s_data, *e_data;
    oqmlNode *qlmth;
    oqmlIterCursor *icurs;
    oqmlDotContext *dctx;
    oqmlBool is_coll;

    oqmlStatus *evalInd(oqmlNode *, Database *, oqmlContext *,
			int &, int &, oqmlBool, oqmlBool);
    oqmlStatus *make(Database *, oqmlContext *, oqmlDot *dot,
		     const Attribute *xattr,
		     oqml_ArrayList *, const char *attrname,
		     const Class *castcls);

    oqmlStatus *getIdx(Database *db, oqmlContext *, oqmlDot *dot);

    void make_key(int);
  
    oqmlDotDesc();
    ~oqmlDotDesc();
  };

  class oqmlDotContext {
    void init(oqmlDot *p);
    oqmlStatus *eval_middle(Database *db, oqmlContext *ctx, Object *o,
			    oqmlAtom *value, int n, oqmlAtomList **alist);
    oqmlStatus *eval_terminal(Database *db, oqmlContext *ctx, Object *o,
			      oqmlAtom *value, int n, oqmlAtomList **alist);
    oqmlStatus *eval_terminal(Database *db, oqmlContext *ctx,
			      const Oid &oid, int n, oqmlAtomList **alist);
    oqmlStatus *eval_middle(Database *db, oqmlContext *ctx, Oid *,
			    int offset, Bool, int n, oqmlAtomList **alist);
    oqmlStatus *eval_terminal(Database *db, oqmlContext *ctx, Oid *,
			      int offset, Bool, int n, oqmlAtomList **alist);

    oqmlStatus *eval_object(Database *db, oqmlContext *ctx, 
			    oqmlAtom *, oqmlAtom *value, int from,
			    oqmlAtomList **alist);

    oqmlStatus *eval_struct(Database *db, oqmlContext *ctx, 
			    oqmlAtom_struct *astruct, oqmlAtom *value,
			    int from, oqmlAtomList **alist);

  public:
    oqmlDot *dot;
    int count;
    const char *varname;
    oqmlNode *oqml;
    oqmlDotContext *tctx;
    oqmlAtomList *tlist;
    Bool ident_mode;
    Bool iscoll;

    oqmlDotDesc *desc;
    oqmlAtomType dot_type;
    oqmlDotContext(oqmlDot *p, const Class *);
    oqmlDotContext(oqmlDot *p, oqmlAtom *);
    oqmlDotContext(oqmlDot *p, const char *);
    oqmlDotContext(oqmlDot *p, oqmlNode *);
    oqmlStatus *setAttrName(Database *db, const char *);

    oqmlStatus *eval(Database *db, oqmlContext *ctx, oqmlAtom *atom,
		     oqmlAtom *value, oqmlAtomList **alist);
    oqmlStatus *eval_perform(Database *db, oqmlContext *ctx, Object *o,
			     oqmlAtom *value, int n, oqmlAtomList **alist);
    oqmlStatus *eval_perform(Database *db, oqmlContext *ctx, Oid *,
			     int offset, Bool, int n, oqmlAtomList **alist);
    void setIdentMode(Bool);
    oqmlStatus *add(Database *, oqmlContext *, const Attribute *, oqml_ArrayList *,
		    char *, oqmlAtom *, Class *, oqmlNode *qlmth);
    void print();
    ~oqmlDotContext();
  };

  class oqml_Array {

  public:
    int oqml_cnt;
    int oqml_alloc;
    oqmlNode **oqml;

    oqml_Array(oqmlNode *);
    void add(oqmlNode *);
  };

  // external declarations.

  extern void oqml_capstring(char *);
  extern const char *oqml_error(oqml_Location * = NULL);
  extern char *oqml_make_error();
  extern char *oqml_make_error(oqml_Location *);
  extern oqmlStatus *oqml_get_class(Database *db, const Oid &oid,
				    Class **cls);
  extern oqmlStatus *oqml_scan(oqmlContext *ctx, Iterator *q, Class *cls,
			       oqmlAtomList *alist,
			       const oqmlATOMTYPE t = (oqmlATOMTYPE)0);

  extern std::string oqml_binop_string(oqmlNode *qleft, oqmlNode *qright,
				       const char *opstr, oqmlBool);

  extern std::string oqml_unop_string(oqmlNode *ql, const char *opstr,
				      oqmlBool);

  extern oqmlStatus *oqml_realize(Database *, char *, oqmlAtomList **alist,
				  oqmlBool compiling = oqml_False);

  extern Status oqml_realize(Database *, char *,
			     Value *&, LinkedList *,
			     oqmlBool compiling = oqml_False);

  extern oqmlNode *oqmlMakeSelect(int, oqmlNode *, oqml_IdentList *, oqmlNode *);

#define OQML_CHECK_INTR() \
if (isBackendInterrupted()) \
{ \
    setBackendInterrupt(False); \
    return new oqmlStatus(Exception::make(IDB_OQL_INTERRUPTED, "")); \
}

  extern oqmlStatus *oqmlstatus;
  extern int oqmlLoopLevel, oqmlBreakLevel;
  extern const char oqml_global_scope[];
  extern const int oqml_global_scope_len;
  extern Status
  oqml_check_vardim(Database *db, const Attribute *attr, Oid *oid,
		    oqmlBool& enough, int from, int nb = 1);

  extern Status
  oqml_check_vardim(const Attribute *attr, Object *o, oqmlBool set,
		    oqmlBool& enough, int from, int &nb, int pdims,
		    oqmlBool isComplete);

#define OQMLCOMPLETE(V) ((V) && (V)->as_string() ? oqml_True : oqml_False)

#define OQML_TMP_RLIST(RLIST) tmp_##RLIST

#define OQML_MAKE_RLIST(XALIST, RLIST) \
  oqmlAtomList * RLIST = new oqmlAtomList(); \
  oqmlAtom_list * OQML_TMP_RLIST(RLIST) = new oqmlAtom_list(RLIST); \
  (*XALIST) = new oqmlAtomList(OQML_TMP_RLIST(RLIST))

#define OQML_MAKE_RBAG(XALIST, RLIST) \
  oqmlAtomList * RLIST = new oqmlAtomList(); \
  oqmlAtom_bag * OQML_TMP_RLIST(RLIST) = new oqmlAtom_bag(RLIST); \
  (*XALIST) = new oqmlAtomList(OQML_TMP_RLIST(RLIST))

#define NO_KW
#define SKIP_ERRS

  extern const char oqml_uninit_fmt[];

  extern oqmlAtom_list *oqml_variables;
  extern oqmlAtom_list *oqml_functions;
  extern oqmlAtom_string *oqml_status;
  extern oqmlBool oqml_suppress(oqmlAtomList *, const char *);
  extern void oqml_append(oqmlAtomList *, const char *);

  extern oqmlStatus *
  oqml_manage_postactions(Database *db, oqmlStatus *s, oqmlAtomList **alist);

  extern oqmlStatus *
  oqml_realize_postaction(Database *db, oqmlContext *, const char *ident,
			  oqmlAtom_string *rs, oqmlAtom *ra,
			  oqmlAtomList **alist);

  extern void oqml_initialize(Database *db);
  extern oqmlBool oqml_auto_persist;
  class OqlCtbDatabase;
  extern OqlCtbDatabase *oqml_default_db;
  extern oqmlStatus *oqml_get_location(Database *&, oqmlContext *,
				       oqmlNode *location, oqmlBool * = 0);
  extern oqmlStatus *oqml_get_locations(Database *db, oqmlContext *ctx,
					oqmlNode *location, Database *xdb[],
					int &xdb_cnt);

  extern oqmlBool oqml_is_getcount(oqml_ArrayList *);
  extern oqmlBool oqml_is_wholecount(oqml_ArrayList *);

  extern oqmlStatus *oqmlIndexIter(Database *db, oqmlContext *ctx,
				   oqmlNode *node,
				   oqmlDotContext *dctx, oqmlDotDesc *d,
				   oqmlAtomList **alist);

  extern void
  oqml_sort_simple(oqmlAtomList *ilist, oqmlBool reverse,
		   oqmlATOMTYPE atom_type, oqmlAtomList **alist);
  extern void
  oqml_sort_list(oqmlAtomList *ilist, oqmlBool reverse, int idx,
		 oqmlATOMTYPE atom_type, oqmlAtomList **alist);

  extern oqmlStatus *
  oqml_check_sort_type(oqmlNode *node, oqmlAtomType &type, const char *msg = 0);

  struct oqmlIterCursor {
    int s_ind, e_ind;
    oqmlIterCursor(int _s_ind, int _e_ind) : s_ind(_s_ind), e_ind(_e_ind) {}
    ~oqmlIterCursor() { }
  };

  extern oqmlStatus *
  oqml_opident_compile(oqmlNode *, Database *db, oqmlContext *ctx,
		       oqmlNode *ql, char *&ident);

  extern oqmlStatus *
  oqml_opident_preeval(oqmlNode *, Database *db, oqmlContext *ctx,
		       oqmlNode *ql, char *&ident);

#define oqml_isstat() (is_statement ? "; " : "")

#define OQML_NEW_HINT() (oqml_auto_persist ? new oqmlIdent("oql$db") : (oqmlNode *)0)

  extern oqmlAtom *
  oqml_make_struct_array(Database *db, oqmlContext *ctx, int idx, oqmlAtom *);

#include <eyedb/oqlinline.h>
#include <eyedb/oqliter.h>

#define OQML_CHECK_MAX_ATOMS(LIST, CTX, GARB) \
do { \
  if ((LIST)->cnt && (CTX)->isOneAtom()) \
    { \
      GARB; \
      return oqmlSuccess; \
    } \
 \
  if ((LIST)->cnt >= (CTX)->getMaxAtoms()) \
    { \
      (CTX)->setOverMaxAtoms(); \
      GARB; \
      return oqmlSuccess; \
    } \
} while (0)

#define SYNC_GARB

#define OQL_DELETE(X) do {if ((X) && !(X)->refcnt) delete (X);} while(0)

  extern oqmlBool
  oqml_is_symbol(oqmlContext *ctx, const char *sym);

}
