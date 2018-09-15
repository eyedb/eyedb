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
#include <assert.h>
//#include <values.h>
#include <eyedb/oql_p.h>
#include <OQLBE.h>
#include "eyedblib/log.h"
#include "eyedb/Log.h"
#include "eyedb/oqlctb.h"

using namespace eyedb;

int oqlparse();

 namespace eyedb {
   oqmlStatus *oqmlstatus;
   int oqmlLevel;
   char *oqml_file;
 }

static int __line = 1;
static oqmlBool oqml_in_select = oqml_False;

#define INFINITE ((oqmlNode *)~0)

//#define PARSE_TRACE

#define MAXLEVELS 8

static unsigned int __line__[MAXLEVELS];

static char *__soqmlbuf, *__oqmlbuf;
static char *__soqmlbuf__[MAXLEVELS], *__oqmlbuf__[MAXLEVELS];

static oqmlAtomList **oqmllist;
static oqmlBool oqml_compiling;
Database *oqmldb;
static oqmlNode *oqml_assign(oqmlNode *, int, oqmlNode *);
static oqmlNode *oqml_call(const char *, oqmlNode *, Bool);
static oqmlNode *oqml_make_array(oqmlNode *, oqmlNode *, oqmlNode *);
static oqmlNode *oqml_make_call(oqmlNode *, oqml_List *);
static oqmlNode *oqml_forin_realize(char *ident, oqmlNode *from, int op,
				    oqmlNode *to, oqmlNode *action);
/* support for bison */
#define __attribute__(x)

int yylex();

extern void yyerror(const char *);
/* end of support for bison */

%}

%union {
  oqmlNode *node;
  eyedblib::int64 n;
  double d;
  char c;
  char *s;
  oqmlBool b;

  struct {
    oqmlBool b1;
    oqmlBool b2;
  } b2;

  struct {
    char *symb;
    oqmlNode *select;
  } select_ident;

  struct {
    oqmlNode *left;
    oqmlNode *right;
  } new_init_ident;

  oqml_IdentList *ident_list;
  oqml_ParamList *param_list;
  oqml_List *list;
  oqml_Interval *interval;
  oqml_SelectOrder *order;
  oqml_CollSpec *coll_spec;

  oqml_Array *array;

  // warning: this structure is not correct because not recursive
  struct TypeDirectName {
    char *symbol;
    char *type_name;
    oqmlBool type_name_is_ref;
    // instead of the last two attributes, it must be:
    // TypeName *type_name;
    // to allow for recursion.
  } type_direct_name;

  struct TypeName {
    TypeDirectName type_direct_name;
    oqmlBool is_ref;
  } type_name;
}

/* terminals */
%token PAR_OPEN PAR_CLOSE BRK_OPEN BRK_CLOSE ACC_OPEN ACC_CLOSE

%token<n> INT
%token<c> CHAR
%token<d> FLOAT
%token<s> STRING
%token<s> OID
%token<s> OBJ
%token<s> SYMBOL
%token<s> ODL_QUOTED_SEQ
%token TRUE_ FALSE_ NULL_ATOM NIL

%token TERM QUESTMARK DBL_DOT

%token ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN PLUS_ASSIGN MINUS_ASSIGN
%token SHR_ASSIGN SHL_ASSIGN AND_ASSIGN OR_ASSIGN XOR_ASSIGN
%token COMMA SHARP
%token<b> DOT
%token<b> LOGICAL_AND LOGICAL_OR
%token UNSET APPEND SET LIST BAG ARRAY ISSET EVAL UNVAL BREAK IMPORT PRINT
%token STRINGOP INT32OP INT16OP INT64OP CHAROP FLOATOP IDENTOP OIDOP
%token TYPEOF CLASSOF BODYOF SCOPEOF REFOF VALOF STRUCTOF PUSH POP THROW RETURN

%token OR AND XOR INTERSECT EXCEPT BETWEEN
%token EQUAL DIFF LT GT LE GE
%token PLUS MINUS MUL DIV MOD INCR DECR NOT SHIFT_LEFT SHIFT_RIGHT
%token SCOPE NEW DELETE IMPLEMENTATION
%token IF THEN ELSE
%token SELECT DISTINCT FROM IS IN WHERE ONE CONTEXT
%token WHILE DO FOREACH
%token FUNCTION DEFINE AS EXISTS EMPTY ADD TO SUPPRESS ELEMENT AT CONTENTS
%token LIKE REGCMP REGICMP REGDIFF REGIDIFF

%token STRUCT UNION
%token GROUP HAVING ORDER BY ASC DESC

%token INDEX BTREE HASH LANG_SPEC
%token UNIQUE NOTNULL NOTEMPTY CARD INVERSE

%token FOR ALL DOLLAR

/* non terminals */
%type<node> statement compound_statement statement_list selection_statement
iteration_statement assign_statement function_statement jump_statement
%type<node> expr_opt
%type<node> expr
%type<node> assign_expr assign_expr_opt
%type<node> select_expr
%type<node> cast_expr
%type<node> unary_expr
%type<node> decl_expr
%type<node> function_expr
%type<node> function_def
%type<node> cond_expr
%type<node> while_expr
%type<node> logical_or_expr
%type<node> logical_and_expr
%type<node> incl_or_expr
%type<node> excl_or_expr
%type<node> and_expr
%type<node> equal_expr
%type<node> rel_expr
%type<node> shift_expr
%type<node> add_expr
%type<node> mul_expr
%type<node> postfix_expr
%type<node> new_expr
%type<node> delete_expr
%type<node> primary_expr
%type<node> literal
%type<node> interval_item
%type<node> range
%type<interval> interval_hint interval

%type<param_list> param_list_opt
%type<list> expr_list expr_list_opt
%type<s> qualified_class_name
%type<s> symbol ext_symbol param_symbol type_symbol
%type<s> string coll_type coll_impl_opt
%type<n> unary_op assign_op

%type<ident_list> select_ident_list
%type<new_init_ident> new_init_item
%type<ident_list> new_init_list new_init_list_opt
%type<select_ident> select_ident

%type<b> select_order_spec_opt hash_or_btree
%type<b2> distinct_opt
%type<node> select_projection select_where select_group select_having
%type<list> select_order_item_list
%type<order> select_order

%type<coll_spec> coll_spec
%type <b>	   ref_opt
%type <node> new_hint select_hint

%%

translation_unit: translation_unit statement
{
  oqmlNode *node = $2;
  if (node)
    {
      //      oqmlstatus = 0;
      if (!oqml_compiling)
	*oqmllist = 0;
      if (!eyedb::oqmlstatus)
	{
#ifdef PARSE_TRACE
	  //	  if (oqmlLevel == 1)
	    //printf("ok compiling? %d\n", oqml_compiling);
	  //cout << "<< " << node->toString() << " >>" << endl;
#endif
          if (!oqml_compiling)
   	    eyedb::oqmlstatus = node->realize(oqmldb, oqmllist);
	}

#ifdef PARSE_TRACE
      if (eyedb::oqmlstatus)
	cout << "ERROR: " << eyedb::oqmlstatus->msg << ", level: " << oqmlLevel
	     << endl;
#endif
      if (eyedb::oqmlstatus)
	return 0;
    }
}
| /* empty */
{
}
;

compound_statement: ACC_OPEN statement_list ACC_CLOSE
{
  $$ = new oqmlCompoundStatement($2);
}
;

statement_list : statement_list statement
{
  if ($1 && $2) $$ = new oqmlComma($1, $2, oqml_True);
  else if ($1)  $$ = $1;
  else          $$ = $2;
}
| statement
{
  $$ = $1;
} 
;

statement : expr_opt TERM
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| compound_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| selection_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| iteration_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| function_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| jump_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| error TERM
{
  $$ = new oqmlString("<<syntax error>>");;
  yyerrok;
}
;

/*
 * Expression Sub Grammar
 */

expr_opt : expr
{
  $$ = $1;
}
| /* empty */
{
  $$ = 0;
}
;

expr : assign_expr
{
  $$ = $1;
}
| expr COMMA assign_expr
{
  $$ = new oqmlComma($1, $3, oqml_False);
}
;

assign_expr_opt : assign_expr
{
  $$ = $1;
}
|
{
  $$ = 0;
}
;

assign_expr : select_expr
{
  $$ = $1;
}
| unary_expr assign_op assign_expr
{
  $$ = oqml_assign($1, $2, $3);
}
| CONTEXT LT postfix_expr GT assign_expr
{
  $$ = new oqmlSelect($3, oqml_False, oqml_False, $5, 0, 0);
  ((oqmlSelect *)$$)->setDatabaseStatement();
}
;

distinct_opt : DISTINCT
{
  $$.b1 = oqml_True;
  $$.b2 = oqml_False;
  oqml_in_select = oqml_False;
}
| ONE
{
  $$.b1 = oqml_False;
  $$.b2 = oqml_True;
  oqml_in_select = oqml_False;
}
| /* empty */
{
  $$.b1 = oqml_False;
  $$.b2 = oqml_False;
  oqml_in_select = oqml_False;
}
;

select_projection : assign_expr
{
  $$ = $1;
}
| MUL
{
  $$ = 0;
}
;

select_expr : select_hint distinct_opt select_projection
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3);
}
| select_hint distinct_opt select_projection select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, 0, 0, 0, 0, $4);
}
| select_hint distinct_opt select_projection FROM select_ident_list
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_where
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, $6);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_where select_group
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, $6, $7);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, 0, 0, $6);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_where select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, $6, 0, 0, $7);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_where select_group select_having
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, $6, $7, $8);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_where select_group select_having select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, $6, $7, $8, $9);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_group
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, $6);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_group select_having
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, $6, $7);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_group select_having select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, $6, $7, $8);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_having
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, 0, $6);
}
| select_hint distinct_opt select_projection FROM select_ident_list
  select_having select_order
{
  $$ = new oqmlSelect($1, $2.b1, $2.b2, $3, $5, 0, 0, $6, $7);
}
| decl_expr
{
  $$ = $1;
}
;

select_hint : SELECT
{
  $$ = 0;
}
| SELECT LT postfix_expr GT
{
  $$ = $3;
}
;

select_where : WHERE assign_expr
{
  $$ = $2;
}
;

select_group : GROUP BY assign_expr
{
  $$ = $3;
}
;

select_having : HAVING assign_expr
{
  $$ = $2;
}
;

select_order : ORDER BY select_order_item_list select_order_spec_opt
{
  $$ = new oqml_SelectOrder($3, $4);
}
;

select_order_item_list : select_order_item_list COMMA assign_expr
{
  $$ = $1;
  $$->add($3);
}
| assign_expr
{
  $$ = new oqml_List();
  $$->add($1);
}
;

select_order_spec_opt : ASC
{
  $$ = oqml_True;
}
| DESC
{
  $$ = oqml_False;
}
| /* empty */
{
  $$ = oqml_True;
}
;

select_ident: 
symbol IN select_expr
{
  $$.symb = $1;
  $$.select = $3;
}
| select_expr symbol
{
  $$.symb = $2;
  $$.select = $1;
}
| select_expr AS symbol
{
  $$.symb = $3;
  $$.select = $1;
}
| select_expr
{
  $$.symb = 0;
  $$.select = $1;
}
;

select_ident_list : select_ident_list COMMA select_ident
{
  $$ = $1;
  $$->add($3.symb, $3.select);
  free($3.symb);
}
| select_ident
{
  $$ = new oqml_IdentList();
  $$->add($1.symb, $1.select);
  free($1.symb);
}
;

decl_expr : function_expr
{
  $$ = $1;
}
;

function_expr : function_def
{
  $$ = $1;
}
| cond_expr
{
  $$ = $1;
}
;

function_statement : FUNCTION symbol PAR_OPEN param_list_opt PAR_CLOSE compound_statement
{
  $$ = new oqmlFunction($2, $4, $6);
  free($2);
}
| FUNCTION symbol PAR_OPEN param_list_opt PAR_CLOSE ACC_OPEN ACC_CLOSE
{
  $$ = new oqmlFunction($2, $4, 0);
  free($2);
}
;

function_def : DEFINE symbol PAR_OPEN param_list_opt PAR_CLOSE AS expr
{
  $$ = new oqmlFunction($2, $4, $7);
  free($2);
}
| DEFINE symbol AS expr
{
  $$ = new oqmlFunction($2, 0, $4);
  free($2);
}
;

assign_op : ASSIGN
{
  $$ = ASSIGN;
}
| MUL_ASSIGN
{
  $$ = MUL_ASSIGN;
}
| DIV_ASSIGN
{
  $$ = DIV_ASSIGN;
}
| MOD_ASSIGN
{
  $$ = MOD_ASSIGN;
}
| PLUS_ASSIGN
{
  $$ = PLUS_ASSIGN;
}
| MINUS_ASSIGN
{
  $$ = MINUS_ASSIGN;
}
| SHR_ASSIGN
{
  $$ = SHR_ASSIGN;
}
| SHL_ASSIGN
{
  $$ = SHL_ASSIGN;
}
| XOR_ASSIGN
{
  $$ = XOR_ASSIGN;
}
| AND_ASSIGN
{
  $$ = AND_ASSIGN;
}
| OR_ASSIGN
{
  $$ = OR_ASSIGN;
}
;

assign_statement : compound_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| selection_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| iteration_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| jump_statement
{
  $$ = $1;
  if ($$) $$->is_statement = oqml_True;
}
| assign_expr TERM
{
  $$ = $1;
  $$->is_statement = oqml_True;
}
| TERM
{
  $$ = 0;
}
/* added 9/02/06 */
| ACC_OPEN ACC_CLOSE
{
  $$ = 0;
}
;

selection_statement: IF PAR_OPEN logical_or_expr PAR_CLOSE assign_statement ELSE assign_statement
{
  $$ = new oqmlIf($3, $5, $7, oqml_False);
}
| IF PAR_OPEN logical_or_expr PAR_CLOSE assign_statement
{
  $$ = new oqmlIf($3, $5, 0, oqml_False);
}
;

iteration_statement : WHILE PAR_OPEN while_expr PAR_CLOSE assign_statement
{
  $$ = new oqmlWhile($3, $5);
}
| DO assign_statement WHILE PAR_OPEN while_expr PAR_CLOSE TERM
{
  $$ = new oqmlDoWhile($5, $2);
}
| FOR PAR_OPEN symbol IN while_expr PAR_CLOSE assign_statement
{
  $$ = new oqmlForEach($3, $5, $7);
  free($3);
}
| FOR PAR_OPEN assign_expr_opt TERM assign_expr_opt TERM assign_expr_opt PAR_CLOSE assign_statement
{
  $$ = new oqmlForDo($3, $5, $7, $9);
}
| FOR PAR_OPEN symbol IN primary_expr LT primary_expr PAR_CLOSE assign_statement
{
  $$ = oqml_forin_realize($3, $5, LT, $7, $9);
}
| FOR PAR_OPEN symbol IN primary_expr LE primary_expr PAR_CLOSE assign_statement
{
  $$ = oqml_forin_realize($3, $5, LE, $7, $9);
}
| FOR PAR_OPEN symbol IN primary_expr GT primary_expr PAR_CLOSE assign_statement
{
  $$ = oqml_forin_realize($3, $5, GT, $7, $9);
}
| FOR PAR_OPEN symbol IN primary_expr GE primary_expr PAR_CLOSE assign_statement
{
  $$ = oqml_forin_realize($3, $5, GE, $7, $9);
}
;

jump_statement: RETURN TERM
{
  $$ = new oqmlReturn(0);
}
| RETURN assign_expr TERM
{
  $$ = new oqmlReturn($2);
}
| BREAK INT
{
  $$ = new oqmlBreak(new oqmlInt($2));
}
| BREAK
{
  $$ = new oqmlBreak(0);
}
;

cond_expr : while_expr
{
  $$ = $1;
}
| logical_or_expr QUESTMARK expr DBL_DOT cond_expr
{
  $$ = new oqmlIf($1, $3, $5, oqml_True);
}
/*
| EXISTS logical_or_expr IN cond_expr
{
  $$ = new oqmlIn($2, $4);
}
*/
/*
| logical_or_expr IN range
{
  $$ = new oqmlBetween($1, $3);
}
*/
/*
| logical_or_expr NOT IN range
{
  $$ = new oqmlNotBetween($1, $4);
}
*/
| ADD cond_expr TO cond_expr
{
  $$ = new oqmlAddTo($2, $4);
}
| SUPPRESS cond_expr FROM cond_expr
{
  $$ = new oqmlSuppressFrom($2, $4);
}
| APPEND cond_expr TO cond_expr
{
  $$ = new oqmlAppend($2, $4);
}

/*
 * list operators: not implemented
 */

/*
| INSERT cond_expr BEFORE cond_expr FROM cond_expr
| INSERT cond_expr AFTER cond_expr FROM cond_expr
| SUPPRESS BEFORE cond_expr FROM cond_expr
| SUPPRESS AFTER cond_expr FROM cond_expr
*/

/*
 * obsolete array operators *
 */

/*
| ADD cond_expr AT cond_expr TO cond_expr
{
  $$ = new oqmlSetInAt($4, $2, $6);
}
| SUPPRESS AT cond_expr FROM cond_expr
{
  $$ = new oqmlUnsetInAt($3, $5);
}
| ELEMENT AT cond_expr INTO cond_expr
{
  $$ = new oqmlElementAt($3, $5);
}
*/

/*
| ELEMENT cond_expr
{
  $$ = new oqmlElement($2);
}
| EXISTS SYMBOL IN cond_expr DBL_DOT cond_expr
{
  $$ = new oqmlFor(new oqml_Interval(new oqmlInt(1), 0), $2, $4, $6, oqml_True);
  free($2);
}
| FOR interval SYMBOL IN cond_expr DBL_DOT cond_expr
{
  $$ = new oqmlFor($2, $3, $5, $7, oqml_False);
  free($3);
}
*/
;

interval: LT interval_hint GT
{
  $$ = $2;
}
| ALL
{
  $$ = new oqml_Interval(0, 0);
}
;

interval_hint : interval_item
{
  $$ = new oqml_Interval($1, $1);
}
| interval_item DBL_DOT interval_item
{
  $$ = new oqml_Interval($1, $3);
}
;

interval_item : primary_expr
{
  $$ = $1;
}
| DOLLAR
{
  $$ = 0;
}
;

while_expr: logical_or_expr
{
  $$ = $1;
}

assign_expr : THROW assign_expr
{
  $$ = new oqmlThrow($2);
}
| EVAL assign_expr
{
  $$ = new oqmlEval($2);
}
| EMPTY assign_expr
{
  $$ = new oqmlEmpty($2);
}
| PRINT assign_expr
{
  $$ = new oqmlPrint($2);
}
| UNVAL assign_expr
{
  $$ = new oqmlUnval($2);
}
;

unary_expr : TYPEOF unary_expr
{
  $$ = new oqmlTypeOf($2);
}
| CLASSOF unary_expr
{
  $$ = new oqmlClassOf($2);
}
| STRINGOP unary_expr
{
  $$ = new oqmlStringOp($2);
}
| INT32OP unary_expr
{
  $$ = new oqmlIntOp($2);
}
| CHAROP unary_expr
{
  $$ = new oqmlCharOp($2);
}
| FLOATOP unary_expr
{
  $$ = new oqmlFloatOp($2);
}
| OIDOP unary_expr
{
  $$ = new oqmlOidOp($2);
}
| IDENTOP unary_expr
{
  $$ = new oqmlIdentOp($2);
}
| IMPORT unary_expr
{
  $$ = new oqmlImport($2);
}
;

logical_or_expr : logical_and_expr
{
  $$ = $1;
}
| logical_or_expr LOGICAL_OR logical_and_expr
{
  $$ = new oqmlLOr($1, $3, $2);
}
| logical_or_expr UNION logical_and_expr
{
  $$ = new oqmlUnion($1, $3);
}
| logical_or_expr EXCEPT logical_and_expr
{
  $$ = new oqmlExcept($1, $3);
}
;

logical_and_expr : incl_or_expr
{
  $$ = $1;
}
| logical_and_expr LOGICAL_AND incl_or_expr
{
  $$ = new oqmlLAnd($1, $3, $2);
}
/*| logical_and_expr LOGICAL_AND LT INT GT incl_or_expr*/
| logical_and_expr LOGICAL_AND ACC_OPEN expr ACC_CLOSE incl_or_expr
{
  /*  $$ = new oqmlLAnd($1, $6, $2, $4); */
  $$ = new oqmlLAnd($1, $6, $2, $4);
}
| logical_and_expr INTERSECT incl_or_expr
{
  $$ = new oqmlIntersect($1, $3);
}
;

incl_or_expr : excl_or_expr
{
  $$ = $1;
}
| incl_or_expr OR excl_or_expr
{
  $$ = new oqmlAOr($1, $3);
}
;

excl_or_expr : and_expr
{
  $$ = $1;
}
| excl_or_expr XOR and_expr
{
  $$ = new oqmlXor($1, $3);
}
;

and_expr : equal_expr
{
  $$ = $1;
}
| and_expr AND equal_expr
{
  $$ = new oqmlAAnd($1, $3);
}
;

equal_expr : rel_expr
{
  $$ = $1;
}
| equal_expr EQUAL rel_expr
{
  $$ = new oqmlEqual($1, $3);
}
| equal_expr DIFF rel_expr
{
  $$ = new oqmlDiff($1, $3);
}
| equal_expr LIKE rel_expr
{
  // TBD: should transfrom the rel_expr so to use standard regex
  // instead of % driven constructs.
  $$ = new oqmlRegCmp($1, $3);
}
| equal_expr REGCMP rel_expr
{
  $$ = new oqmlRegCmp($1, $3);
}
| equal_expr REGICMP rel_expr
{
  $$ = new oqmlRegICmp($1, $3);
}
| equal_expr REGDIFF rel_expr
{
  $$ = new oqmlRegDiff($1, $3);
}
| equal_expr REGIDIFF rel_expr
{
  $$ = new oqmlRegIDiff($1, $3);
}
| rel_expr BETWEEN rel_expr LOGICAL_AND rel_expr
{
  $$ = new oqmlBetween($1, new oqmlRange($3, oqml_True, $5, oqml_True,
					 oqml_True));
}
| rel_expr NOT BETWEEN rel_expr LOGICAL_AND rel_expr
{
  $$ = new oqmlNotBetween($1, new oqmlRange($4, oqml_True, $6, oqml_True,
					    oqml_True));
}
| rel_expr BETWEEN range
{
  $$ = new oqmlBetween($1, $3);
}
| rel_expr NOT BETWEEN range
{
  $$ = new oqmlNotBetween($1, $4);
}
/* 9/02/06 */
| rel_expr IN rel_expr
{
  $$ = new oqmlIn($1, $3);
}
;

rel_expr : shift_expr
{
  $$ = $1;
}
| rel_expr LT shift_expr
{
  $$ = new oqmlInf($1, $3);
}
| rel_expr GT shift_expr
{
  $$ = new oqmlSup($1, $3);
}
| rel_expr LE shift_expr
{
  $$ = new oqmlInfEq($1, $3);
}
| rel_expr GE shift_expr
{
  $$ = new oqmlSupEq($1, $3);
}
;

shift_expr : add_expr
{
  $$ = $1;
}
| shift_expr SHIFT_LEFT add_expr
{
  $$ = new oqmlShl($1, $3);
}
| shift_expr SHIFT_RIGHT add_expr
{
  $$ = new oqmlShr($1, $3);
}
;

add_expr : mul_expr
{
  $$ = $1;
}
| add_expr PLUS mul_expr
{
  $$ = new oqmlAdd($1, $3);
}
| add_expr MINUS mul_expr
{
  $$ = new oqmlSub($1, $3);
}
;

mul_expr : cast_expr
{
  $$ = $1;
}
| mul_expr MUL cast_expr
{
  $$ = new oqmlMul($1, $3);
}
| mul_expr DIV cast_expr
{
  $$ = new oqmlDiv($1, $3);
}
| mul_expr MOD cast_expr
{
  $$ = new oqmlMod($1, $3);
}
;

cast_expr : unary_expr
{
  $$ = $1;
}
;

unary_expr : postfix_expr
{
  $$ = $1;
}
| INCR unary_expr
{
  $$ = new oqmlSelfIncr($2, 1, oqml_False);
}
| DECR unary_expr
{
  $$ = new oqmlSelfIncr($2, -1, oqml_False);
}
| UNSET unary_expr
{
  $$ = new oqmlUnset($2);
}
| ISSET unary_expr
{
  $$ = new oqmlIsset($2);
}
| AND unary_expr
{
  $$ = new oqmlRefOf($2);
}
| MUL unary_expr
{
  $$ = new oqmlValOf($2);
}
| REFOF unary_expr
{
  $$ = new oqmlRefOf($2);
}
| VALOF unary_expr
{
  $$ = new oqmlValOf($2);
}
| CONTENTS unary_expr
{
  $$ = new oqmlContents($2);
}
| BODYOF unary_expr
{
  $$ = new oqmlBodyOf($2);
}
| SCOPEOF unary_expr
{
  $$ = new oqmlScopeOf($2);
}
| STRUCTOF unary_expr
{
  $$ = new oqmlStructOf($2);
}
| PUSH symbol
{
  $$ = new oqmlPush($2);
  free($2);
}
| POP symbol
{
  $$ = new oqmlPop($2);
  free($2);
}
| unary_op unary_expr
{
  if ($1 == PLUS)
    $$ = new oqmlAdd(new oqmlInt((long long)0), $2, oqml_True);
  else if ($1 == MINUS)
    $$ = new oqmlSub(new oqmlInt((long long)0), $2, oqml_True);
  else if ($1 == NOT)
    {
      if ($2->getType() == oqmlBETWEEN)
	{
	  oqmlBetween *between = ((oqmlBetween *)$2);
	  $$ = between->requalifyNot();
	}
      else
	$$ = new oqmlLNot($2);
    }
  else if ($1 == REGCMP)
    $$ = new oqmlTilde($2);
  else
    $$ = 0;
}
| new_expr
{
  $$ = $1;
}
| delete_expr
{
  $$ = $1;
}
;

unary_op : PLUS
{
  $$ = PLUS;
}
| MINUS
{
  $$ = MINUS;
}
| NOT
{
  $$ = NOT;
}
| REGCMP
{
  $$ = REGCMP;
}
;

range : BRK_OPEN postfix_expr COMMA postfix_expr BRK_CLOSE
{
  $$ = new oqmlRange($2, oqml_True, $4, oqml_True);
}
| BRK_OPEN postfix_expr COMMA postfix_expr BRK_OPEN
{
  $$ = new oqmlRange($2, oqml_True, $4, oqml_False);
}
| BRK_CLOSE postfix_expr COMMA postfix_expr BRK_OPEN
{
  $$ = new oqmlRange($2, oqml_False, $4, oqml_True);
}
| BRK_CLOSE postfix_expr COMMA postfix_expr BRK_CLOSE
{
  $$ = new oqmlRange($2, oqml_False, $4, oqml_False);
}
;

ext_symbol : qualified_class_name
{
  $$ = $1;
}
| type_symbol
{
  $$ = $1;
}
;

type_symbol: INT32OP
{
  $$ = strdup("int32");
}
| INT16OP
{
  $$ = strdup("int16");
}
| INT64OP
{
  $$ = strdup("int64");
}
| CHAROP
{
  $$ = strdup("char");
}
| FLOATOP
{
  $$ = strdup("float");
}
| OIDOP
{
  $$ = strdup("oid");
}
;

new_expr: new_hint qualified_class_name PAR_OPEN new_init_list_opt PAR_CLOSE
{
  $$ = new oqmlNew($1, $2, $4);
  free($2);
}
| new_hint type_symbol PAR_OPEN assign_expr PAR_CLOSE
{
  $$ = new oqmlNew($1, $2, $4);
  free($2);
}
| new_hint type_symbol PAR_OPEN PAR_CLOSE
{
  $$ = new oqmlNew($1, $2, (oqmlNode *)0);
  free($2);
}
| new_hint ODL_QUOTED_SEQ
{
  $$ = new oqmlNew($1, $2);
  free($2);
}
| new_hint coll_spec PAR_OPEN select_expr PAR_CLOSE
{
  $$ = new oqmlCollection($1, $2, $4);
}
| new_hint coll_spec PAR_OPEN PAR_CLOSE
{
  $$ = new oqmlCollection($1, $2, 0);
}
| coll_spec PAR_OPEN select_expr PAR_CLOSE
{
  $$ = new oqmlCollection(OQML_NEW_HINT(), $1, $3);
}
| coll_spec PAR_OPEN PAR_CLOSE
{
  $$ = new oqmlCollection(OQML_NEW_HINT(), $1, 0);
}
;

new_hint: NEW
{
  $$ = (oqml_auto_persist ? new oqmlIdent("oql$db") : (oqmlNode *)0);
}
| NEW LT primary_expr GT
{
  $$ = $3;
}
| NEW LT GT
{
  $$ = (oqmlNode *)0;
}
;

new_init_list_opt : new_init_list
{
  $$ = $1;
}
|
{
  $$ = 0;
}
;

new_init_list : new_init_list COMMA new_init_item
{
  $$ = $1;
  $$->add($3.left, $3.right);
}
| new_init_item
{
  $$ = new oqml_IdentList();
  $$->add($1.left, $1.right);
}
;

new_init_item : postfix_expr DBL_DOT assign_expr
{
  $$.left = $1;
  $$.right = $3;
}
;

ref_opt : MUL
{
  $$ = oqml_True;
}
| /* empty */
{
  $$ = oqml_False;
}
;

coll_impl_opt : COMMA string
{
  $$ = $2;
}
| /* empty */
{
  $$ = 0;
}

coll_type : SET
{
  $$ = strdup("set");
}
| BAG
{
  $$ = strdup("bag");
}
| ARRAY
{
  $$ = strdup("array");
}
| LIST
{
  $$ = strdup("list");
}
;

hash_or_btree : HASH
{
  $$ = oqml_True;
}
| BTREE
{
  $$ = oqml_False;
}
;

/* WARNING disconnected coll_spec recursion */
coll_spec : coll_type LT ext_symbol ref_opt GT
{
  $$ = new oqml_CollSpec($1, $3, $4);
  free($1);
  free($3);
}
| coll_type LT ext_symbol ref_opt COMMA string GT
{
  $$ = new oqml_CollSpec($1, $3, $4, $6);
  free($1);
  free($3);
  free($6);
}
| coll_type LT ext_symbol ref_opt COMMA string COMMA hash_or_btree coll_impl_opt GT
{
  $$ = new oqml_CollSpec($1, $3, $4, $6, $8, $9);
  free($1);
  free($3);
  free($6);
  free($9);
}
|  coll_type LT coll_spec ref_opt GT
{
  $$ = new oqml_CollSpec($1, $3, $4);
  free($1);
  free($3);
}
| coll_type LT coll_spec ref_opt COMMA string GT
{
  $$ = new oqml_CollSpec($1, $3, $4, $6);
  free($1);
  free($3);
  free($6);
}
| coll_type LT coll_spec ref_opt COMMA string COMMA hash_or_btree coll_impl_opt GT
{
  $$ = new oqml_CollSpec($1, $3, $4, $6, $8, $9);
  free($1);
  free($3);
  free($6);
  free($9);
}
;

delete_expr : DELETE unary_expr  
{
  $$ = new oqmlDelete($2);
}
;

postfix_expr : primary_expr
{
  $$ = $1;
}
| postfix_expr BRK_OPEN expr BRK_CLOSE
{
  $$ = oqml_make_array($1, $3, 0);
}
| postfix_expr BRK_OPEN QUESTMARK BRK_CLOSE
{
  $$ = oqml_make_array($1, 0, 0);
}
| postfix_expr BRK_OPEN NOT BRK_CLOSE
{
  $$ = oqml_make_array($1, INFINITE, 0);
}
| postfix_expr BRK_OPEN NOT NOT BRK_CLOSE
{
  $$ = oqml_make_array($1, INFINITE, INFINITE);
}
| postfix_expr BRK_OPEN expr DBL_DOT expr BRK_CLOSE
{
  $$ = oqml_make_array($1, $3, $5);
}
| postfix_expr BRK_OPEN expr DBL_DOT DOLLAR BRK_CLOSE
{
  $$ = oqml_make_array($1, $3, new oqmlIdent("$"));
}
| postfix_expr PAR_OPEN expr_list_opt PAR_CLOSE
{
  $$ = oqml_make_call($1, $3);
}
| coll_type PAR_OPEN expr_list_opt PAR_CLOSE
{
  $$ = oqml_make_call(new oqmlIdent($1), $3);
  free($1);
}
| postfix_expr PAR_OPEN new_init_list PAR_CLOSE
{
  // il faut adapter the constructor de oqmlNew de facon a prendre
  // un node en 1er argument
  if ($1->getType() == oqmlIDENT)
    $$ = new oqmlNew(OQML_NEW_HINT(), ((oqmlIdent *)$1)->getName(), $3);
  else
    $$ = new oqmlString("<<syntax error>>");
}
| postfix_expr DOT qualified_class_name
{
  if ($1->asDot())
    $$ = $1->asDot()->make_right_dot($3, $2);
  else
    $$ = new oqmlDot($1, new oqmlIdent($3), $2);

  free($3);
}
| postfix_expr INCR
{
  $$ = new oqmlSelfIncr($1, 1, oqml_True);
}
| postfix_expr DECR
{
  $$ = new oqmlSelfIncr($1, -1, oqml_True);
}
| STRUCT PAR_OPEN new_init_list PAR_CLOSE
{
  $$ = new oqmlStruct($3);
}
;

expr_list : assign_expr
{
  $$ = new oqml_List();
  $$->add($1);
}
| expr_list COMMA assign_expr
{
  $$ = $1;
  $$->add($3);
}
;

expr_list_opt : expr_list
{
  $$ = $1;
}
| /* empty */
{
  $$ = 0;
}
;

primary_expr : literal
{
  $$ = $1;
}
| qualified_class_name
{
  $$ = new oqmlIdent($1);
  free($1);
}
| PAR_OPEN expr PAR_CLOSE
{
  $$ = $2;
}
;

literal: INT
{
  $$ = new oqmlInt($1);
}
| CHAR
{
  $$ = new oqmlChar($1);
}
| FLOAT
{
  $$ = new oqmlFloat($1);
}
| string
{
  $$ = new oqmlString($1);
  free($1);
}
| OID
{
  $$ = new oqmlOid($1);
  free($1);
}
| OBJ
{
  $$ = new oqmlObject($1);
  free($1);
}
| FALSE_
{
  $$ = new oqmlFalse();
}
| TRUE_
{
  $$ = new oqmlTrue();
}
| NULL_ATOM
{
  $$ = new oqmlNull();
}
| NIL
{
  $$ = new oqmlNil();
}
;

string : STRING
{
  $$ = strdup($1);
  free($1);
}
| string STRING
{
  $$ = (char *)malloc(strlen($1)+strlen($2)+1);
  strcpy($$, $1);
  strcat($$, $2);
  free($1);
  free($2);
}
;

qualified_class_name : symbol
{
  $$ = $1;
}
| symbol SCOPE qualified_class_name
{
  std::string s = std::string($1) + "::" + $3;
  $$ = strdup(s.c_str());
  free($1);
  free($3);
}
| SCOPE qualified_class_name
{
  $$ = strdup((std::string("::") + $2).c_str());
  free($2);
}
;

param_list_opt : param_list_opt COMMA param_symbol
{
  $$ = $1;
  $$->add($3);
  free($3);
}
| param_list_opt COMMA param_symbol assign_or_equal assign_expr
{
  $$ = $1;
  $$->add($3, $5);
  free($3);
}
| param_symbol
{
  $$ = new oqml_ParamList($1);
  free($1);
}
| param_symbol assign_or_equal assign_expr
{
  $$ = new oqml_ParamList($1, $3);
  free($1);
}
| /* empty */
{
  $$ = 0;
}
;

/* syntaxic sugar */
assign_or_equal : QUESTMARK
| ASSIGN
| QUESTMARK ASSIGN
;

param_symbol : symbol
{
  $$ = $1;
}
| OR symbol
{
  $$ = strdup((std::string("@") + $2).c_str());
  free($2);
}
;

symbol : SYMBOL
{
    $$ = $1; 
}
;

%%

#include "lex.oql.c"

/*#define CASEINSENSITIVE*/

static const char stdin_str[] = "stdin";

void
yyerror(const char *msg)
{
  static int lastline;
  static const char *lastfile = stdin_str;

  if (lastline == __line && oqml_file && !strcmp(lastfile, oqml_file))
    return;

  std::string s = std::string("syntax error ") + oqml_make_error();
  if (oqml_file)
    {
      s += std::string(" in file \"") + oqml_file + "\"";
      s += std::string(" at line ") + str_convert((long)__line);
    }

  lastline = __line;
  lastfile = (oqml_file ? oqml_file : stdin_str);

  eyedb::oqmlstatus = new oqmlStatus(s.c_str());
}

static const int nentries = 'z' - 'a' + 1;

struct HashKeyWords {
  struct KeyWord {
    char *key;
    int token;
  };

  int keywords_len[nentries];
  KeyWord *keywords[nentries];

  static int inline makeIdx(char c) {return c - 'a';}

  HashKeyWords() {
    memset(keywords_len, 0, sizeof(keywords_len));
    memset(keywords, 0, sizeof(keywords));
    initKeyWords();
  }

  void insert(const char *key, int token) {
    int idx = makeIdx(key[0]);
    assert(idx >= 0 && idx < nentries);

    keywords[idx] = (KeyWord *)
      realloc(keywords[idx], (keywords_len[idx]+1) * sizeof(KeyWord));
    KeyWord *kw = keywords[idx];
    kw[keywords_len[idx]].key = strdup(key);
    kw[keywords_len[idx]].token = token;
    keywords_len[idx]++;
  }

  int getToken(const char *key) {
#ifdef CASEINSENSITIVE
    char c = key[0];
    int idx = makeIdx(c >= 'A' && c <= 'Z' ?  c + 'a' - 'A' : c);
#else
    int idx = makeIdx(key[0]);
#endif

    if (idx < 0 || idx >= nentries)
      return 0;

    KeyWord *kw = keywords[idx];
    for (int i = 0; i < keywords_len[idx]; i++)
#ifdef CASEINSENSITIVE
      if (!strcasecmp(kw[i].key, key))
        return kw[i].token;
#else
      if (!strcmp(kw[i].key, key))
        return kw[i].token;
#endif

    return 0;
  }

  void initKeyWords() {
    insert("false", FALSE_);
    insert("mod", MOD);
    insert("not", NOT);
    insert("true", TRUE_);
    insert("new", NEW);
    insert("delete", DELETE);
//    insert("at", AT);

    insert("union", UNION);
    insert("intersect", INTERSECT);
    insert("except", EXCEPT);

    insert("between", BETWEEN);
    insert("one", ONE);
    insert("by", BY);
    insert("order", ORDER);
    insert("context", CONTEXT);
    insert("nil", NIL);
    insert("group", GROUP);
    insert("having", HAVING);
    insert("asc", ASC);
    insert("desc", DESC);

    insert("from", FROM);
    insert("in", IN);
//    insert("into", INTO);
    insert("is", IS);
    insert("where", WHERE);
    insert("if", IF);
    insert("else", ELSE);
    insert("then", THEN);
    insert("while", WHILE);
    insert("do", DO);
    insert("for", FOR);
    insert("all", ALL);
    insert("function", FUNCTION);
    insert("implementation", IMPLEMENTATION);
    insert("hash", HASH);
    insert("btree", BTREE);
    insert("define", DEFINE);
    insert("as", AS);
    insert("like", LIKE);
    insert("struct", STRUCT);
    insert("append", APPEND);
    insert("set", SET);
    insert("bag", BAG);
    insert("array", ARRAY);
    insert("list", LIST);
    insert("unset", UNSET);
    insert("isset", ISSET);
    insert("eval", EVAL);
    insert("unval", UNVAL);
    insert("print", PRINT);
    insert("bodyof", BODYOF);
    insert("scopeof", SCOPEOF);
    insert("refof", REFOF);
    insert("valof", VALOF);
    insert("structof", STRUCTOF);
    insert("push", PUSH);
    insert("pop", POP);
    insert("string", STRINGOP);
    insert("int", INT32OP);
    insert("int32", INT32OP);
    insert("int16", INT16OP);
    insert("int64", INT64OP);
    insert("char", CHAROP);
    insert("float", FLOATOP);
    insert("double", FLOATOP);
    insert("oid", OIDOP);
    insert("ident", IDENTOP);
    insert("import", IMPORT);
    insert("break", BREAK);
    insert("typeof", TYPEOF);
    insert("classof", CLASSOF);
    insert("throw", THROW);
    insert("return", RETURN);

    insert("contents", CONTENTS);
    insert("exists", EXISTS);
    insert("empty", EMPTY);
    insert("add", ADD);
    insert("to", TO);
    insert("suppress", SUPPRESS);
    insert("element", ELEMENT);
  }
};

static HashKeyWords hashKeyWords;

static int oqmlGetKeyword(const char *s)
{
  return hashKeyWords.getToken(s);
}

namespace eyedb {
  const char *oqml_error(oqml_Location *loc)
  {
    if (loc)
      {
	if (loc->to >= 0 && loc->from >= 0)
	  {
	    __soqmlbuf[loc->to] = 0;
	    if (loc->to == loc->from)
	      return __soqmlbuf;

	    return &__soqmlbuf[loc->from];
	  }
      
	return "";
      }
    else
      return __soqmlbuf;
  }

  char *oqml_make_error()
  {
    const char prefix[] = "near `";
    const char *err = oqml_error();
    if (err)
      {
	char *buf = (char *)malloc(strlen(prefix)+strlen(err)+6);
	sprintf(buf, "%s%s'", prefix, err);
	return buf;
      }
  
    return strdup("");
  }

  char *oqml_make_error(oqml_Location *loc)
  {
#if 0
    const char prefix[] = "in `";
    const char *err = oqml_error(loc);
    char *buf = new char[strlen(prefix)+strlen(err)+6];
    sprintf(buf, "%s%s'", prefix, err);
    return buf;
#else
    return (char*)""; /*@@@@warning cast*/
#endif
  }
}

#define _ESC_(X, Y) case X: *p++ = Y; break

static void yypurgestring(unsigned char *s)
{
  unsigned char c;
  unsigned char *p = s;
  while (c = *s)
    {
      if (c == '\\')
	{
	  c = *++s;
	  switch(c)
	    {
	      _ESC_('a', '\a');
	      _ESC_('b', '\b');
	      _ESC_('f', '\f');
	      _ESC_('n', '\n');
	      _ESC_('r', '\r');
	      _ESC_('t', '\t');
	      _ESC_('v', '\v');
	      _ESC_('\'', '\'');
	      _ESC_('\"', '"');
	      _ESC_('\\', '\\');

	    default:
	      *p++ = '\\';
	      *p++ = c;
            }
	  s++;
	}
      else
	*p++ = *s++;
    }

  *p = 0;
}

static int yyskipcomments()
{
  int c;
  while ((c = yyinput()) > 0)
    {
      if (c == '\n')
	__line++;
      else if (c == '*')
	{
          for (;;) {
	    c = yyinput();
	    if (c == '/')
	      return 1;
	    if (c == '\n')
	      __line++;
            if (c != '*')
	      break;
	  }
	}
    }

  if (c <= 0)
    return 0;

  return 1;
}

//#define PARSE_TRACE

int oqml_push_buf(int level, char *buf)
{
  __line__[level]     = __line;
  __soqmlbuf__[level] = (__soqmlbuf ? strdup(__soqmlbuf) : 0);
  __oqmlbuf__[level]  = (__oqmlbuf  ? strdup(__oqmlbuf)  : 0);

#ifdef PARSE_TRACE
  printf("oqml_push_buf(%s) level #%d #%d line=%d\n",
	 buf, level, oqmlLevel, __line);
#endif

  if (level)
    __line = 1;

  __soqmlbuf = buf;
  __oqmlbuf  = buf;

  return 0;
}

static void oqml_pop_buf(int level)
{
#ifdef PARSE_TRACE
  printf("oqml_pop_buf() level #%d #%d line=%d, levelline=%d\n",
	 level, oqmlLevel, __line, __line__[level]);
#endif

  if (level)
    __line     = __line__[level];

  __soqmlbuf = __soqmlbuf__[level];
  __oqmlbuf  = __oqmlbuf__[level];
}

static void oqml_set_buf(char *buf)
{
#ifdef PARSE_TRACE
  printf("oqml_set_buf(%s)\n", buf);
#endif

  __oqmlbuf = strdup(buf);
}

static void oqml_empty_bufs()
{
#ifdef PARSE_TRACE
  printf("oqml_empty_bufs()\n");
#endif

  for (int i = 0; i < MAXLEVELS; i++)
    {
      free(__oqmlbuf__[i]);
      __oqmlbuf__[i] = 0;
    }

  __oqmlbuf = 0;
}

int yywrap()
{
  return 1;
}

static void append(char *& s, int& i, int &len, char c)
{
  if (i >= len)
    {
      len = i + 64;
      s = (char *)realloc(s, len);
    }
  s[i++] = c;
}

static char *yygetquotedseq(char sep)
{
  char *s = 0;
  int i = 0, len = 0;
  int c;

  while ((c = yyinput()) > 0)
    {
      if (c == '\n')
	__line++;
      else if (c == '%')
	{
	  c = yyinput();
	  if (c == sep)
	    {
	      append(s, i, len, 0);
	      return s;
	    }
	  append(s, i, len, '%');
	  if (c == '\n')
	    __line++;
	}

      append(s, i, len, c);
    }

  append(s, i, len, 0);
  return s;
}

static char *yytokstr()
{
  int s_size;
  unsigned char *s, *p, c;
  s_size = 32;
  s = (unsigned char *)malloc(s_size);

  int n, backslash;
  for (n = 0, backslash = 0; (c = yyinput()) > 0; n++)
    {
      if (c == '\n')
	__line++;
      else if (c == '\\')
	backslash = 1;
      else if (c == '"' && !backslash)
	break;
      else
	backslash = 0;

      if (n >= s_size)
	{
	  s_size += 32;
	  s = (unsigned char *)realloc(s, s_size);
	}

      s[n] = c;
    }

  if (n >= s_size)
    {
      s_size += 4;
      s = (unsigned char *)realloc(s, s_size);
    }
   
  s[n] = 0;
  yypurgestring(s);
  return (char *)s;
}

/*
  {STRING} {
  yylval.s = strdup((const char *)&yytext[1]);
  yylval.s[strlen(yylval.s)-1] = 0;
  yypurgestring(yylval.s);
  return STRING;
  }

*/

static oqmlNode *oqml_forin_realize(char *ident, oqmlNode *from, int op,
				    oqmlNode *to, oqmlNode *action)
{
  /*
    oqmlPush *push = new oqmlPush(ident);
    oqmlPop *pop = new oqmlPop(ident);
    push->is_statement = oqml_True;
    pop->is_statement = oqml_True;
  */

  oqmlIdent *nident = new oqmlIdent(ident);
  oqmlAssign *assign = new oqmlAssign(nident, from);
  oqmlForDo *fordo;

  switch(op)
    {
    case LT:
      fordo = new oqmlForDo(ident,
			    assign,
			    new oqmlInf(nident, to),
			    new oqmlSelfIncr(nident, 1, oqml_True),
			    action);
      break;

    case GT:
      fordo = new oqmlForDo(ident,
			    assign,
			    new oqmlSup(nident, to),
			    new oqmlSelfIncr(nident, -1, oqml_True),
			    action);
      break;

    case LE:
      fordo = new oqmlForDo(ident,
			    assign,
			    new oqmlInfEq(nident, to),
			    new oqmlSelfIncr(nident, 1, oqml_True),
			    action);
      break;

    case GE:
      fordo = new oqmlForDo(ident,
			    assign,
			    new oqmlSupEq(nident, to),
			    new oqmlSelfIncr(nident, -1, oqml_True),
			    action);
      break;
    }

  free(ident);
  //return new oqmlComma(push, new oqmlComma(fordo, pop, oqml_True), oqml_True);
  return fordo;
}

static oqmlNode *oqml_assign(oqmlNode *left, int op, oqmlNode *right)
{
  switch(op)
    {
    case ASSIGN:
      return new oqmlAssign(left, right);

    case MUL_ASSIGN:
      return new oqmlAssign(left, new oqmlMul(left, right));

    case DIV_ASSIGN:
      return new oqmlAssign(left, new oqmlDiv(left, right));

    case MOD_ASSIGN:
      return new oqmlAssign(left, new oqmlMod(left, right));

    case PLUS_ASSIGN:
      return new oqmlAssign(left, new oqmlAdd(left, right));

    case MINUS_ASSIGN:
      return new oqmlAssign(left, new oqmlSub(left, right));

    case SHR_ASSIGN:
      return new oqmlAssign(left, new oqmlShr(left, right));

    case SHL_ASSIGN:
      return new oqmlAssign(left, new oqmlShl(left, right));

    case XOR_ASSIGN:
      return new oqmlAssign(left, new oqmlXor(left, right));

    case AND_ASSIGN:
      return new oqmlAssign(left, new oqmlAAnd(left, right));

    case OR_ASSIGN:
      return new oqmlAssign(left, new oqmlAOr(left, right));

    default:
      return (oqmlNode *)0;
    }
}

static oqml_ArrayLink *oqml_make_link(oqmlNode *from, oqmlNode *to)
{
  if (from == INFINITE && to == 0)
    return new oqml_ArrayLink(oqml_False);

  if (from == INFINITE && to == INFINITE)
    return new oqml_ArrayLink(oqml_True);

  return new oqml_ArrayLink(from, to);
}

static oqmlNode *oqml_make_array(oqmlNode *postfix_expr, oqmlNode *from, oqmlNode *to)
{
  if (postfix_expr->asDot())
    {
      oqmlArray *array = postfix_expr->asDot()->make_right_array();
      if (array)
	{
	  array->add(oqml_make_link(from, to));
	  return postfix_expr;
	}
    }

  oqmlArray *array = new oqmlArray(postfix_expr);
  array->add(oqml_make_link(from, to));
  return array;
}

static oqmlNode *oqml_make_call(oqmlNode *postfix_expr, oqml_List *list)
{
  if (postfix_expr->asDot())
    return postfix_expr->asDot()->make_right_call(list);

  if (postfix_expr->getType() == oqmlIDENT)
    {
      char *name = strdup(((oqmlIdent *)postfix_expr)->getName());
      char *p = strchr(name, ':');
      if (p)
	{
	  *p = 0;
	  oqmlNode *mth = new oqmlMethodCall(name, p+2, list);
	  free(name);
	  return mth;
	}

      free(name);
    }      

  return new oqmlCall(postfix_expr, list);
}

namespace eyedb {
  Status oqml_realize(Database *db, char *oqmlstr,
		      Value *&v, LinkedList *mcllist,
		      oqmlBool compiling)
  {
    Status status;
    oqmlAtomList *alist = 0;
    LinkedList *mcllist_s = OQLBE::getMclList();

    OQLBE::setMclList(mcllist);

    oqmlStatus *s = oqml_realize(db, oqmlstr, &alist, compiling);
    if (s)
      status = Exception::make(IDB_OQL_ERROR, std::string(s->msg));
    else
      {
	status = Success;
	if (alist && alist->first)
	  v = alist->first->toValue();
	else
	  v = 0;
      }

    OQLBE::setMclList(mcllist_s);
    return status;
  }

  oqmlStatus *oqml_realize(Database *db, char *oqmlstr, oqmlAtomList **alist,
			   oqmlBool compiling)
  {
#ifdef PARSE_TRACE
    printf("\nSTARTING oqml_realize(%d)\n", oqmlLevel);
#endif

    if (oqml_default_db && oqml_default_db->xdb)
      db = oqml_default_db->xdb;

    if (!oqmlLevel)
      {
	delete eyedb::oqmlstatus;
	eyedb::oqmlstatus = 0;
	oqml_initialize(db);
	//IDB_LOG(IDB_LOG_OQL_EXEC, ("before '%s'\n", oqmlstr));
      }

    oqml_compiling = compiling;
    Database *oqmldb_s = oqmldb;
    oqmlAtomList **oqmllist_s = oqmllist;
    oqmlStatus *oqmlstatus_s = eyedb::oqmlstatus;

    oqmldb = db;
    oqmllist = alist;

    if (!oqmlLevel)
      {
	oqmlGarbManager::garbage();
	oqmlLoopLevel = 0;
	oqmlBreakLevel = 0;
	oqml_empty_bufs();
      }
    else if (oqmlLevel >= MAXLEVELS-1)
      return new oqmlStatus("evaluation level is too deep. "
			    "Maximum allowed is %d.", MAXLEVELS);

    oqml_push_buf(oqmlLevel, oqmlstr);
    oqmlLevel++;
    oqlparse();
    oqml_pop_buf(oqmlLevel-1);

    if (oqmlLevel == 1)
      for (int i = MAXLEVELS-1; i >= 0 && !eyedb::oqmlstatus; i--)
	if (__oqmlbuf__[i] && *__oqmlbuf__[i])
	  {       
	    oqml_set_buf(__oqmlbuf__[i]);
	    while (__oqmlbuf && *__oqmlbuf && !eyedb::oqmlstatus)
	      oqlparse();
	  }

    oqmlLevel--;

    oqmlStatus *s = eyedb::oqmlstatus;

    oqmldb = oqmldb_s;
    oqmllist = oqmllist_s;
    eyedb::oqmlstatus = oqmlstatus_s;

    if (oqmlLevel == 0 && !compiling)
      s = oqml_manage_postactions(db, s, alist);

    /*
      if (!oqmlLevel)
      IDB_LOG(IDB_LOG_OQL_EXEC,
      ("'%s' done => %s\n", oqmlstr,
      (!s ? "successful" : s->msg)));
    */

#ifdef PARSE_TRACE
    printf("ENDING oqml_realize(%d)\n\n", oqmlLevel);
#endif

    return s;
  }
}
