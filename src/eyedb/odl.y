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

%{
#include "odl.h"
#include <stdio.h>
#include <assert.h>
#define FOUR_G 4000000000UL

using namespace eyedb;
//#define odlin ::odlin
//using eyedb::odlin;

#define par_check(o, c) \
 if ((o) != (c)) \
   { \
       odl_add_error("syntax error near line %d\n", _line); \
	 return 1; \
	 }

static int c_c;
static char *__sodlbuf, *__odlbuf;
static int _line = 1;
static int errline = 0;
static int errcnt = 0;
static char *_file;
namespace eyedb {
  extern int odl_error;
}

struct Arg {
  char *type;
  char *ident;
  ~Arg() {free(type); free(ident);}
};

static char *
make_arg(char *_arg, odlBool isRef, odlBool isArray)
{
  char *arg = (char *)malloc(strlen(_arg)+4);
  strcpy(arg, _arg);
  if (isRef)
    strcat(arg, "*");
  if (isArray)
    strcat(arg, "[]");
  free(_arg);
  return arg;
}

static Arg *
make_x_arg(char *_arg, odlBool isRef, odlBool isArray, const char *ident)
{
  Arg *a = new Arg();
  char *arg = (char *)malloc(strlen(_arg)+4);
  strcpy(arg, _arg);
  if (isRef)
    strcat(arg, "*");
  if (isArray)
    strcat(arg, "[]");
  free(_arg);
  a->type = arg;
  a->ident = (ident ? strdup(ident) : 0);
  return a;
}

static int
echo_error(const char *msg = 0)
{
  if (errline != _line)
    {
      if (errcnt++ > 5)
        return 0;
      if (!msg)
	odl_add_error("syntax error near line %d", _line); 
      else
	odl_add_error("near line %d", _line); 
      if (_file)
	odl_add_error(", file %s", _file);
      if (msg)
	odl_add_error(": %s", msg);
      odl_add_error("\n");
      if (odl_error++ > 10)
	return 1;
      errline = _line;
    }
  return 0;
}

static int
attr_check_upd_hints(odlUpdateHint *upd_hints)
{
  if (upd_hints && upd_hints->type == odlUpdateHint::Extend)
    return echo_error("@extend update hint is not available behind an attribute or a relationship");
  return 0;
}

static int
class_check_upd_hints(odlUpdateHint *upd_hints)
{
  if (upd_hints && upd_hints->type != odlUpdateHint::Extend &&
      upd_hints->type != odlUpdateHint::Remove &&
      upd_hints->type != odlUpdateHint::RenameFrom)
    return echo_error("only @extend, @remove or @rename_from update hint are available behind a type declaration");
  return 0;
}

void
rootclass_manage(odlAgregSpec agreg_spec)
{
  if (agreg_spec != odl_SuperClass &&
      !odlAgregatClass::superclass && odl_rootclass)
    new odlAgregatClass(0, odl_RootClass, 
			new odlClassSpec(odl_rootclass, 0, 0, 0),
			new odlDeclRootList());
}

/* support for bison */
#define __attribute__(x)

int yylex();

extern void yyerror(const char *);
/* end of support for bison */

%}

%union {
  int i;
  unsigned int ui;
  char *s;
  odlBool b;
  Arg *arg;

  odlAgregSpec agrspec;
  odlDeclaration *decl;

  odlClassSpec *class_spec;

  odlCollSpec *coll_spec;

  odlDeclRoot *decl_item;
  odlDeclRootList *decl_list;

  odlArrayList *array_list;

  odlInverse *inverse;
  odlImplementation *impl;

  odlUpdateHint *upd_hint;

  odlArgSpecList *arg_list;
  odlExecSpec   *exec_item;

  odlEnumItem *enum_item;
  odlEnumList *enum_list;

  odlIndexImplSpec *index_impl_spec;
  odlIndexImplSpecItem index_impl_spec_item;

  odlCollImplSpec *coll_impl_spec;
  odlCollImplSpecItem coll_impl_spec_item;

  odlMethodHints mth_hints;
}

%token <s> IDENT
%token <s> STRING
%token <s> QUOTED_SEQ
%token <i> INT CHAR TRIGGER

%token SUPERCLASS ENUM INVERSE INDEX NOTNULL CARD IN NOTEMPTY CONSTRAINT ON OFF
%token METHOD UNIQUE SCOPE EXTENDS EXTREF ATTRIBUTE CPLUSPLUS RELSHIP PROPAGATE
%token CALLED_FROM_OQL CALLED_FROM_ANY CALLED_FROM_C CALLED_FROM_JAVA
%token SERVER CLIENT OUT INOUT STATIC INSTMETHOD CLASSMETHOD NATIVE
%token BTREE HASH BTREEINDEX HASHINDEX NOINDEX IMPLEMENTATION NIL TYPE HINTS
%token REMOVED RENAMED_FROM CONVERT_METHOD MIGRATED_FROM EXTENDED
%token <s> OQL_QUOTED_SEQ 
/*%token OQL*/

%type <decl>    odl
%type <decl_list> decl_list
%type <decl_item> decl_item decl_item_x
%type <i>       expr_int int_opt
%type <b>       ref_opt ref propagate
%type <mth_hints> mth_hint mth_hint_list mth_hints
%type <agrspec>   agreg_spec
%type <class_spec> class_spec
%type <coll_spec> coll_spec
%type <array_list> array array_opt
%type <inverse> inverse inverse_opt
%type <arg_list>   arg_list
%type <exec_item>   exec_spec trigger_spec method_spec method_spec_base
%type <enum_item> enum_item
%type <enum_list> enum_list
%type <b>         open close method_hints
%type <s>         string ident_opt attr_path
%type <arg>       arg
%type <i> in_out
%type <impl> implementation
%type <index_impl_spec> index_impl_spec
%type <index_impl_spec_item> index_impl_spec_item
%type <coll_impl_spec> coll_impl_spec class_implementation
%type <coll_impl_spec_item> coll_impl_spec_item
%type <upd_hint> update_hint

%token SHIFT_L SHIFT_R SUP_EQ INF_EQ

%left '|'
%left '^'
%left '&'
%left SHIFT_L SHIFT_R
%left '+' '-'
%left '*' '/' '%'
%right UMINUS
%right NOT

%%

odl_prog : odl_prog odl
{
}
| odl_prog ';'
| /* empty */
| error ';'
{
  yyerrok;
  if (echo_error()) return 1;
}
;

odl : agreg_spec class_spec open decl_list close ';' update_hint
{
  par_check($3, $5);
  $$ = new odlAgregatClass($7, $1, $2, $4);
  if (class_check_upd_hints($7)) return 1;
}

| agreg_spec class_spec open close ';' update_hint
{
  par_check($3, $4);
  $$ = new odlAgregatClass($6, $1, $2, new odlDeclRootList());
  if (class_check_upd_hints($6)) return 1;
}
| agreg_spec class_spec open decl_list close ';'
{
  par_check($3, $5);
  $$ = new odlAgregatClass(0, $1, $2, $4);
}

| agreg_spec class_spec open close ';'
{
  par_check($3, $4);
  $$ = new odlAgregatClass(0, $1, $2, new odlDeclRootList());
}
| agreg_spec class_spec ';'
{
  $$ = new odlAgregatClass(0, $1, $2,  new odlDeclRootList());
}
| QUOTED_SEQ
{
  qseq_list.insertObject($1);
}
;

update_hint : REMOVED
{
  $$ = new odlUpdateHint(odlUpdateHint::Remove);
}
| EXTENDED
{
  $$ = new odlUpdateHint(odlUpdateHint::Extend);
}
| REMOVED '(' ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::Remove);
}
| RENAMED_FROM '(' IDENT ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::RenameFrom, $3);
  free($3);
}
| RENAMED_FROM '(' IDENT ',' IDENT ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::RenameFrom, $3, $5);
  free($3);
  free($5);
}
| MIGRATED_FROM '(' IDENT SCOPE IDENT ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::MigrateFrom, $3, $5);
  free($3);
  free($5);
}
| MIGRATED_FROM '(' IDENT SCOPE IDENT ',' IDENT ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::MigrateFrom, $3, $5, $7);
  free($3);
  free($5);
  free($7);
}
| CONVERT_METHOD '(' IDENT ')'
{
  $$ = new odlUpdateHint(odlUpdateHint::Convert, $3);
  free($3);
}
;

agreg_spec : IDENT
{
  if (!strcmp($1, "struct") || !strcmp($1, "class") ||
      !strcmp($1, "interface"))
    $$ = odl_Struct;
  else if (!strcmp($1, "union"))
    $$ = odl_Union;
  else if (!strcmp($1, "superstruct") || !strcmp($1, "superclass"))
    $$ = odl_SuperClass;
  else if (!strcmp($1, "native") || !strcmp($1, "nativeclass"))
    $$ = odl_NativeClass;
  else if (!strcmp($1, "declare"))
    $$ = odl_Declare;
  else
    echo_error();

  rootclass_manage($$);

  free($1);
};

class_spec : IDENT
{
  $$ = new odlClassSpec($1, 0, 0, 0);
  free($1);
}
| IDENT class_implementation
{
  $$ = new odlClassSpec($1, 0, 0, $2);
  free($1);
}
| IDENT class_implementation '[' IDENT ']'
{
  $$ = new odlClassSpec($1, 0, $4, $2);
  free($1);
  free($4);
}
| IDENT '[' IDENT ']' class_implementation
{
  $$ = new odlClassSpec($1, 0, $3, $5);
  free($1);
  free($3);
}
| IDENT '[' IDENT ']'
{
  $$ = new odlClassSpec($1, 0, $3, 0);
  free($1);
  free($3);
}
| IDENT extends IDENT
{
  $$ = new odlClassSpec($1, $3, 0, 0);
  free($1);
  free($3);
}
| IDENT class_implementation '[' IDENT ']' extends IDENT
{
  $$ = new odlClassSpec($1, $7, $4, $2);
  free($1);
  free($4);
  free($7);
}
| IDENT class_implementation extends IDENT
{
  $$ = new odlClassSpec($1, $4, 0, $2);
  free($1);
  free($4);
}
| IDENT '[' IDENT ']' class_implementation extends IDENT
{
  $$ = new odlClassSpec($1, $7, $3, $5);
  free($1);
  free($3);
  free($7);
}
| IDENT '[' IDENT ']' extends IDENT
{
  $$ = new odlClassSpec($1, $6, $3, 0);
  free($1);
  free($6);
  free($3);
}
;

/*
magorder : MAGORDER '<' expr_int '>'
{
  $$ = $3;
}
| MAGORDER '<' expr_int IDENT '>'
{
  if (!strcasecmp($4, "K"))
    $$ = ($3 >= ((unsigned int)~0)/1000) ?  FOUR_G: $3 * 1000;
  else if (!strcasecmp($4, "M"))
    $$ = ($3 >= ((unsigned int)~0)/(1000000)) ? FOUR_G : $3 * 1000000;
  else if (!strcasecmp($4, "G"))
    $$ = ($3 >= 5) ? FOUR_G : $3 * 1000000000;
  else
    yyerror("");

  free($4);
}
;
*/

extends : EXTENDS
{
}
| ':'
{
}

decl_list: decl_list decl_item_x
{
  $$ = $1;
  $$->add($2);
}
| decl_item_x
{
  $$ = new odlDeclRootList();
  $$->add($1);
}
| decl_list sep
{
  $$ = $1;
}
;

decl_item_x : decl_item update_hint
{
  $$ = $1;
  $$->setUpdateHint($2);
  if (attr_check_upd_hints($2)) return 1;
}
| ATTRIBUTE decl_item update_hint
{
  $$ = $2;
  $$->setUpdateHint($3);
  if (attr_check_upd_hints($3)) return 1;
}
| RELSHIP decl_item update_hint
{
  if ($2->asDeclItem() && !$2->asDeclItem()->hasInverseAttr())
    if (echo_error()) return 1;

  $$ = $2;
  $$->setUpdateHint($3);
  if (attr_check_upd_hints($3)) return 1;
}
| decl_item
{
  $$ = $1;
}
| ATTRIBUTE decl_item
{
  $$ = $2;
}
| CONSTRAINT '<' UNIQUE '>' ON attr_path
{
  $$ = new odlUnique($6);
  free($6);
}
| CONSTRAINT '<' UNIQUE ',' propagate '>' ON attr_path
{
  $$ = new odlUnique($8, $5);
  free($8);
}
| CONSTRAINT '<' propagate ',' UNIQUE '>' ON attr_path
{
  $$ = new odlUnique($8, $3);
  free($8);
}
| CONSTRAINT '<' NOTNULL '>' ON attr_path
{
  $$ = new odlNotnull($6);
  free($6);
}
| CONSTRAINT '<' NOTNULL ',' propagate '>' ON attr_path
{
  $$ = new odlNotnull($8, $5);
  free($8);
}
| CONSTRAINT '<' propagate ',' NOTNULL '>' ON attr_path
{
  $$ = new odlNotnull($8, $3);
  free($8);
}
| INDEX ON attr_path
{
  $$ = new odlIndex($3);
  free($3);
}
| INDEX '<' index_impl_spec '>' ON attr_path
{
  $$ = new odlIndex($6, $3);
  free($6);
}
| INDEX '<' propagate '>' ON attr_path
{
  $$ = new odlIndex($6, 0, $3);
  free($6);
}
| INDEX '<' index_impl_spec ',' propagate '>' ON attr_path
{
  $$ = new odlIndex($8, $3, $5);
  free($8);
}
| INDEX '<' propagate ',' index_impl_spec '>' ON attr_path
{
  $$ = new odlIndex($8, $5, $3);
  free($8);
}
| implementation
{
  $$ = $1;
}
| RELSHIP decl_item
{
  if ($2->asDeclItem() && !$2->asDeclItem()->hasInverseAttr())
    if (echo_error()) return 1;

  $$ = $2;
}
| exec_spec update_hint
{
  $$ = $1;
  $$->setUpdateHint($2);
  if ($2 && $2->type != odlUpdateHint::Remove)
    if (echo_error("only @remove update hint is available behind a method or trigger")) return 1;
}
| exec_spec
{
  $$ = $1;
}
;

index_impl_spec : index_impl_spec_item
{
  $$ = new odlIndexImplSpec();
  $$->add($1);
}
| index_impl_spec ',' index_impl_spec_item
{
  $1->add($3);
  $$ = $1;
}
;

index_impl_spec_item : HASH
{
  $$.init();
  $$.type = odlIndexImplSpecItem::Hash;
}
| BTREE
{
  $$.init();
  $$.type = odlIndexImplSpecItem::BTree;
}
| TYPE '=' HASH
{
  $$.init();
  $$.type = odlIndexImplSpecItem::Hash;
}
| TYPE '=' BTREE
{
  $$.init();
  $$.type = odlIndexImplSpecItem::BTree;
}
| HINTS '=' string
{
  $$.init();
  $$.hints = $3;
}
;

class_implementation: '(' IMPLEMENTATION '<' coll_impl_spec '>' ')'
{
  $$ = $4;
}
;

implementation : IMPLEMENTATION '<' coll_impl_spec '>' ON attr_path
{
  $$ = new odlImplementation($6, $3);
  free($6);
}
| IMPLEMENTATION '<' coll_impl_spec ',' propagate '>' ON attr_path
{
  $$ = new odlImplementation($8, $3, $5);
  free($8);
}
| IMPLEMENTATION '<' propagate ',' coll_impl_spec '>' ON attr_path
{
  $$ = new odlImplementation($8, $5, $3);
  free($8);
}
;

coll_impl_spec : coll_impl_spec_item
{
  $$ = new odlCollImplSpec();
  $$->add($1);
}
| coll_impl_spec ',' coll_impl_spec_item
{
  $1->add($3);
  $$ = $1;
}
;

coll_impl_spec_item : HASHINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::HashIndex;
}
| BTREEINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::BTreeIndex;
}
| NOINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::NoIndex;
}
| TYPE '=' HASHINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::HashIndex;
}
| TYPE '=' BTREEINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::BTreeIndex;
}
| TYPE '=' NOINDEX
{
  $$.init();
  $$.type = odlCollImplSpecItem::NoIndex;
}
| TYPE '=' HASH
{
  fprintf(stderr, "collection implementation warning: 'type = hash' is deprecated, use 'type = hashindex'\n");
  $$.init();
  $$.type = odlCollImplSpecItem::HashIndex;
}
| TYPE '=' BTREE
{
  fprintf(stderr, "collection implementation warning: 'type = btree' is deprecated, use 'type = btreeindex'\n");
  $$.init();
  $$.type = odlCollImplSpecItem::BTreeIndex;
}
| HINTS '=' string
{
  $$.init();
  $$.hints = $3;
}
;

propagate : PROPAGATE '=' ON
{
  $$ = odlTrue;
}
| PROPAGATE '=' OFF
{
  $$ = odlFalse;
}
;

attr_path : IDENT
{
  $$ = $1;
}
| attr_path '.' IDENT
{
  std::string s = std::string($1) + "." + $3;
  free($1);
  free($3);
  $$ = strdup(s.c_str());
}
| IDENT SCOPE IDENT
{
  std::string s = std::string($1) + "::" + $3;
  free($1);
  free($3);
  $$ = strdup(s.c_str());
}
;

/* ODMG style */
decl_item: IDENT ':' IDENT ref array inverse sep
{
  $$ = new odlDeclItem($1, $3, $4, $5, $6);
  free($1);
  free($3);
}
| IDENT ':' IDENT ref sep
{
  $$ = new odlDeclItem($1, $3, $4, 0, 0);
  free($1);
  free($3);
}
| IDENT ':' IDENT ref array sep
{
  $$ = new odlDeclItem($1, $3, $4, $5, 0);
  free($1);
  free($3);
}
| IDENT ':' IDENT ref inverse sep
{
  $$ = new odlDeclItem($1, $3, $4, 0, $5);
  free($1);
  free($3);
}
| IDENT ':' IDENT inverse sep
{
  $$ = new odlDeclItem($1, $3, odlFalse, 0, $4);
  free($1);
  free($3);
}
| IDENT ':' IDENT array sep
{
  $$ = new odlDeclItem($1, $3, odlFalse, $4, 0);
  free($1);
  free($3);
}
| IDENT ':' IDENT array inverse sep
{
  $$ = new odlDeclItem($1, $3, odlFalse, $4, $5);
  free($1);
  free($3);
}
| IDENT ':' IDENT sep
{
  $$ = new odlDeclItem($1, $3, odlFalse, 0, 0);
  free($1);
  free($3);
}
| IDENT ':' coll_spec ref_opt array_opt inverse_opt sep
{
  $$ = new odlDeclItem($1, $3, $4, $5, $6);
  free($1);
}
;

coll_spec : IDENT '<' IDENT '[' expr_int ']' '>'
{
  $$ = new odlCollSpec($1, $3, odlFalse, $5);
  free($1);
  free($3);
}
| IDENT '<' IDENT ref_opt '>'
{
  $$ = new odlCollSpec($1, $3, $4, 0);
  free($1);
  free($3);
}
| IDENT '<' coll_spec ref_opt '>'
{
  $$ = new odlCollSpec($1, $3, $4, 0);
  free($1);
}
| IDENT '<' coll_spec '[' expr_int ']' '>'
{
  $$ = new odlCollSpec($1, $3, odlFalse, $5);
  free($1);
}
| IDENT '<' expr_int '>'
{
  if (strcmp($1, "string"))
    yyerror("string keyword expected");
  $$ = new odlCollSpec($3);
  free($1);
}
;

/* C++ style */
decl_item : IDENT ref IDENT array inverse sep
{
  $$ = new odlDeclItem($3, $1, $2, $4, $5);
  free($1);
  free($3);
}
| IDENT ref IDENT sep
{
  $$ = new odlDeclItem($3, $1, $2, 0, 0);
  free($1);
  free($3);
}
| IDENT ref IDENT array sep
{
  $$ = new odlDeclItem($3, $1, $2, $4, 0);
  free($1);
  free($3);
}
| IDENT ref IDENT inverse sep
{
  $$ = new odlDeclItem($3, $1, $2, 0, $4);
  free($1);
  free($3);
}
| IDENT IDENT array sep
{
  $$ = new odlDeclItem($2, $1, odlFalse, $3, 0);
  free($1);
  free($2);
}
| IDENT IDENT array inverse sep
{
  $$ = new odlDeclItem($2, $1, odlFalse, $3, $4);
  free($1);
  free($2);
}
| IDENT IDENT inverse sep
{
  $$ = new odlDeclItem($2, $1, odlFalse, 0, $3);
  free($1);
  free($2);
}
| IDENT IDENT sep
{
  $$ = new odlDeclItem($2, $1, odlFalse, 0, 0);
  free($1);
  free($2);
}
| coll_spec ref_opt IDENT array_opt inverse_opt sep
{
  $$ = new odlDeclItem($3, $1, $2, $4, $5);
  free($3);
}
| QUOTED_SEQ
{
  $$ = new odlQuotedSeq($1);
  free($1);
}
;

ref: '*'
{
  $$ = odlTrue;
}
| '&'
{
  $$ = odlTrue;
}
;

ref_opt: ref
{
  $$ = odlTrue;
}
| /* empty */
{
  $$ = odlFalse;
}
;

array: '[' int_opt ']'
{
  $$ = new odlArrayList;
  $$->add($2);
}
| array '[' int_opt ']'
{
  $$ = $1;
  $$->add($3);
}
;

int_opt : expr_int
{
  $$ = $1;
}
| /* empty */
{
  $$ = -1;
}
;

array_opt: array
{
  $$ = $1;
}
| /* empty */
{
  $$ = 0;
}
;

/*
card : CARD '=' expr_int
{
  $$ = new odlCardinality($3, odlFalse, $3, odlFalse);
}
| CARD '>' expr_int
{
  $$ = new odlCardinality($3, odlTrue, idbCardinalityConstraint::maxint, odlFalse);
}
| CARD '<' expr_int
{
  $$ = new odlCardinality(0, odlFalse, $3, odlTrue);
}
| CARD SUP_EQ expr_int
{
  $$ = new odlCardinality($3, odlFalse, idbCardinalityConstraint::maxint, odlFalse);
}
| CARD INF_EQ expr_int
{
  $$ = new odlCardinality(0, odlFalse, $3, odlFalse);
}
| CARD IN '[' expr_int ',' expr_int ']'
{
  $$ = new odlCardinality($4, odlFalse, $6, odlFalse);
}
| CARD IN '[' expr_int ',' expr_int '['
{
  $$ = new odlCardinality($4, odlFalse, $6, odlTrue);
}
| CARD IN ']' expr_int ',' expr_int ']'
{
  $$ = new odlCardinality($4, odlTrue, $6, odlFalse);
}
| CARD IN ']' expr_int ',' expr_int '['
{
  $$ = new odlCardinality($4, odlTrue, $6, odlTrue);
}
| NOTEMPTY
{
  $$ = new odlCardinality(0, odlTrue, idbCardinalityConstraint::maxint, odlFalse);
}
;
*/

inverse: INVERSE IDENT SCOPE IDENT
{
  $$ = new odlInverse($2, $4);
  free($2);
  free($4);
}
| INVERSE IDENT
{
  $$ = new odlInverse(0, $2);
  free($2);
}
;

inverse_opt : inverse
{
  $$ = $1;
}
| /* empty */
{
  $$ = 0;
}
;

exec_spec : trigger_spec sep
{
  $$ = $1;
}
| trigger_spec EXTREF '(' string ')' sep
{
  $$ = $1;
  $$->asTriggerSpec()->extref = $4;
}
| trigger_spec OQL_QUOTED_SEQ
{
  $$ = $1;
  $$->asTriggerSpec()->oqlSpec = $2;
}
| method_spec sep
{
  $$ = $1;
}
| method_spec EXTREF '(' string ')' sep
{
  $$ = $1;
  $$->asMethodSpec()->extref = $4;
}
| method_spec OQL_QUOTED_SEQ
{
  $$ = $1;
  $$->asMethodSpec()->oqlSpec = $2;
}
;

trigger_spec : TRIGGER '<' IDENT '>' IDENT '(' ')'
{
  $$ = new odlTriggerSpec($1, $3, $5);
  free($3);
  free($5);
}
;

method_hints: METHOD
{
  $$ = odlFalse;
}
| INSTMETHOD
{
  $$ = odlFalse;
}
| CLASSMETHOD
{
  $$ = odlTrue;
}
| STATIC
{
  $$ = odlTrue;
}
;

method_spec : mth_hints method_spec_base
{
  $$ = $2;
  $$->asMethodSpec()->mth_hints = $1;
}
| method_hints mth_hints method_spec_base
{
  $$ = $3;
  $$->asMethodSpec()->isClassMethod = $1;
  $$->asMethodSpec()->mth_hints = $2;
}
| method_hints method_spec_base
{
  $$ = $2;
  $$->asMethodSpec()->isClassMethod = $1;
  $$->asMethodSpec()->mth_hints.isClient = odlFalse;
  $$->asMethodSpec()->mth_hints.calledFrom = odlMethodHints::ANY_HINTS;
}
| method_spec_base
{
  $$ = $1;
}
;

method_spec_base: IDENT ref IDENT '(' arg_list ')'
{
  $$ = new odlMethodSpec(make_arg($1, odlTrue, odlFalse), $3, $5);
  free($3);
}
| IDENT IDENT '(' arg_list ')'
{
  $$ = new odlMethodSpec($1, $2, $4);
  free($1);
  free($2);
}
| IDENT '[' ']' IDENT '(' arg_list ')'
{
  $$ = new odlMethodSpec(make_arg($1, odlFalse, odlTrue), $4, $6);
  free($4);
}
| IDENT ref '[' ']' IDENT '(' arg_list ')'
{
  $$ = new odlMethodSpec(make_arg($1, odlTrue, odlTrue), $5, $7);
  free($5);
}
;

mth_hints: '<' mth_hint_list '>'
{
  $$ = $2;
}

mth_hint_list: mth_hint_list ',' mth_hint
{
  $$ = $1;
  if (!$$.isClient)
    $$.isClient = $3.isClient;
  if ($$.calledFrom == odlMethodHints::ANY_HINTS)
    $$.calledFrom = $3.calledFrom;
}
| mth_hint
{
  $$ = $1;
}
;

mth_hint: CLIENT
{
  $$.isClient = odlTrue;
  $$.calledFrom = odlMethodHints::ANY_HINTS;
}
| SERVER
{
  $$.isClient = odlFalse;
  $$.calledFrom = odlMethodHints::ANY_HINTS;
}
| CALLED_FROM_OQL
{
  $$.isClient = odlFalse;
  $$.calledFrom = odlMethodHints::OQL_HINTS;
}
| CALLED_FROM_C
{
  $$.isClient = odlFalse;
  $$.calledFrom = odlMethodHints::C_HINTS;
}
| CALLED_FROM_JAVA
{
  $$.isClient = odlFalse;
  $$.calledFrom = odlMethodHints::JAVA_HINTS;
}
| CALLED_FROM_ANY
{
  $$.isClient = odlFalse;
  $$.calledFrom = odlMethodHints::ANY_HINTS;
}
;

arg_list : arg_list ',' in_out arg
{
  $$ = $1;
  $$->add(new odlArgSpec($3, $4->type, $4->ident));
  delete $4;
}
| in_out arg
{
  $$ = new odlArgSpecList();
  $$->add(new odlArgSpec($1, $2->type, $2->ident));
  delete $2;
}
| /* empty */
{
  $$ = new odlArgSpecList();
}
;

ident_opt : IDENT
{
  $$ = $1;
}
| /* empty */
{
  $$ = 0;
}
;

in_out : IN
{
  $$ = IN_ARG_TYPE;
}
| OUT
{
  $$ = OUT_ARG_TYPE;
}
| INOUT
{
  $$ = INOUT_ARG_TYPE;
}
;

arg : IDENT ident_opt
{
  $$ = make_x_arg($1, odlFalse, odlFalse, $2);
  free($2);
}
| IDENT ref ident_opt
{
  $$ = make_x_arg($1, odlTrue, odlFalse, $3);
  free($3);
}
| IDENT ident_opt '[' ']'
{
  $$ = make_x_arg($1, odlFalse, odlTrue, $2);
  free($2);
}
| IDENT ref ident_opt '[' ']'
{
  $$ = make_x_arg($1, odlTrue, odlTrue, $3);
  free($3);
}
;

string : string STRING
{
  $$ = (char *)malloc(strlen($1)+strlen($2)+1);

  strcpy($$, $1);
  strcat($$, $2);

  free($1);
  free($2);
}
| STRING
{
  $$ = $1;
}

/*
 * enum class
 */

odl : ENUM IDENT open enum_list sep_opt close
{
  par_check($3, $6);
  $$ = new odlEnumClass($2, $4, 0);
  free($2);
}
;

odl : ENUM IDENT '[' IDENT ']' open enum_list sep_opt close
{
  par_check($6, $9);
  $$ = new odlEnumClass($2, $7, $4);
  free($2);
  free($4);
}
;

enum_item: IDENT ':' expr_int
{
  $$ = new odlEnumItem($1, 0, $3);
  free($1);
}
| IDENT '=' expr_int
{
  $$ = new odlEnumItem($1, 0, $3);
  free($1);
}
| IDENT
{
  $$ = new odlEnumItem($1, 0, odlTrue);
  free($1);
}
| IDENT '[' IDENT ']' ':' expr_int
{
  $$ = new odlEnumItem($1, $3, $6);
  free($1);
}
| IDENT '[' IDENT ']' '=' expr_int
{
  $$ = new odlEnumItem($1, $3, $6);
  free($1);
}
| IDENT '[' IDENT ']'
{
  $$ = new odlEnumItem($1, $3, odlTrue);
  free($1);
}
;

enum_list: enum_list ',' enum_item
{
  $$ = $1;
  $$->add($3);
}
| enum_item
{
  $$ = new odlEnumList;
  $$->add($1);
}
;

expr_int : expr_int '+' expr_int
{
  $$ = $1 + $3;
}
| expr_int '-' expr_int
{
  $$ = $1 - $3;
}
| expr_int '*' expr_int
{
  $$ = $1 * $3;
}
| expr_int '|' expr_int
{
  $$ = $1 | $3;
}
| expr_int '^' expr_int
{
  $$ = $1 ^ $3;
}
| expr_int SHIFT_L expr_int
{
  $$ = $1 << $3;
}
| expr_int SHIFT_R expr_int
{
  $$ = $1 >> $3;
}
| expr_int '%' expr_int
{
  $$ = $1 % $3;
}
| expr_int '&' expr_int
{
  $$ = $1 & $3;
}
| '~' expr_int %prec NOT
{
  $$ = ~$2;
}
| '!' expr_int %prec NOT
{
  $$ = !$2;
}
| expr_int '/' expr_int
{
  $$ = $1 / $3;
}
| '-' expr_int %prec UMINUS
{
  $$ = - $2;
}
| '(' expr_int ')'
{
  $$ = $2;
}
| INT
{
  $$ = $1;
}
| CHAR
{
  $$ = $1;
}
;

/*
open : '('
{
  $$ = odlTrue;
}
|
*/
open: '{'
{
  $$ = odlFalse;
}
;

/*
close : ')'
{
  $$ = odlTrue;
}
| */
close : '}'
{
  $$ = odlFalse;
}
;

/*
sep : ','
| ';'
;
*/

sep : ';'
;

sep_opt : sep
| /* empty */
;

%%

#include "lex.odl.c"
