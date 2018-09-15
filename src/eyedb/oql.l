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
static int yyskipcomments();
extern double atof(const char *);
static char *yygetquotedseq(char sep = '}');
static char oqml_ccx;
#define ESC(x, y) \
if (!strcmp((const char *)yytext, "'\\" #x "'")) \
    yylval.c = y
static int oqmlGetKeyword(const char *s);
static void yypurgestring(unsigned char *s);
static char *yytokstr();
// useful when compiled with the '-e' option
int yycgid(wchar_t c);
void yywunput(wchar_t wc);

// BUG corrected on the 6/05/01: replaced:
//   char c = *__oqmlbuf++;
//     by
//   char c = *__oqmlbuf; \
//   if (c) ++__oqmlbuf;

#define YY_INPUT(buf, result, max_size)  \
{ \
    char c = *__oqmlbuf; \
    if (c) ++__oqmlbuf; \
    result = (c == 0 ? YY_NULL : (buf[0] = c, 1)); \
}

#undef unput
#define unput(c) abort()
%}

DIGIT   [0-9]
OCTAL   [0-7]
HEXA    [0-9A-Fa-f]
INT     {DIGIT}+
REAL	({INT}\.{INT}?)|({INT}?\.{INT})
EREAL   (e|E)[+-]?{INT}([fF]|[lL])?

ESC_SEQ   (\\\'|\\\"|\\\?|\\\\|\\a|\\b|\\f|\\n|\\r|\\t|\\v)
STRING    \"([^"]|\\\")*\"
CHAR      ([^\'\\]|{ESC_SEQ})
CPPDIRECT [\#]([\ ])?{INT}[\ ]{STRING}([\ ]{INT})*
CPPLINE   [\#]([\ ])?{INT}\n
SYMBOL    [a-zA-Z\$_#][a-zA-Z\$_0-9#]*
SEP       [ \t]
%%

[ \t]+ {}
[\n]+  {__line += yyleng;}
";"    {return TERM;}
"?"    {return QUESTMARK;}
"$"    {return DOLLAR;}
":"    {return DBL_DOT;}
":="   {return ASSIGN;}
"*="   {return MUL_ASSIGN;}
"/="   {return DIV_ASSIGN;}
"%="   {return MOD_ASSIGN;}
"+="   {; return PLUS_ASSIGN;}
"-="   {return MINUS_ASSIGN;}
">>="  {return SHR_ASSIGN;}
"<<="  {return SHL_ASSIGN;}
"&="   {return AND_ASSIGN;}
"|="   {return OR_ASSIGN;}
"^="   {return XOR_ASSIGN;}
"|"    {return OR;}
"&"    {return AND;}
"||"   {yylval.b = oqml_False; return LOGICAL_OR;}
"&&"   {yylval.b = oqml_False; return LOGICAL_AND;}
"or"   {yylval.b = oqml_True;  return LOGICAL_OR;}
"and"  {yylval.b = oqml_True;  return LOGICAL_AND;}
"^"    {return XOR;}
"=="|"=" {return EQUAL;}
"!="   {return DIFF;}
"<"    {return LT;}
">"    {return GT;}
"<="   {return LE;}
">="   {return GE;}
"<<"   {return SHIFT_LEFT;}
">>"   {return SHIFT_RIGHT;}
"+"    {return PLUS;}
"-"    {return MINUS;}
"*"    {return MUL;}
"/"    {return DIV;}
"%"    {return MOD;}
"++"   {return INCR;}
"--"   {return DECR;}
"!"    {return NOT;}
"~"    {return REGCMP;}
"~~"   {return REGICMP;}
"!~"   {return REGDIFF;}
"!~~"  {return REGIDIFF;}
"("    {return PAR_OPEN;}
")"    {return PAR_CLOSE;}
"["    {return BRK_OPEN;}
"]"    {return BRK_CLOSE;}
"{"    {return ACC_OPEN;}
"}"    {return ACC_CLOSE;}
"::"   {return SCOPE;}
"."    {yylval.b = oqml_False; return DOT;}
"->"   {yylval.b = oqml_True; return DOT;}
","    {return COMMA;}
"null"|"NULL" {return NULL_ATOM;}
"distinct" {
     if (oqml_in_select)
       return DISTINCT;
     yylval.s = strdup((const char *)yytext);
     return SYMBOL;
}
"select" {
     oqml_in_select = oqml_True;
     return SELECT;
}
"$"[^\n\$]+"$" {
	yylval.s = strdup((const char *)&yytext[1]);
	yylval.s[strlen(yylval.s)-1] = 0;
	return LANG_SPEC;
	}
"//"[^\n]* {}
\'{ESC_SEQ}\'  {
	ESC(a, '\a');
	else ESC(b, '\b');
	else ESC(f, '\f');
	else ESC(n, '\n');
	else ESC(r, '\r');
	else ESC(t, '\t');
	else ESC(v, '\v');
	return CHAR;
}
\'{CHAR}\'     {sscanf((const char *)yytext, "'%c'", &yylval.c); return CHAR;}
\'\\{OCTAL}+\' {
		int n;
		yytext[yyleng - 1] = 0;
		sscanf((const char *)&yytext[2], "%o", &n);
		yylval.c = n;
		return CHAR;
	       }
\'\\(x|X){HEXA}+\' {
		int n;
		yytext[yyleng - 1] = 0;
	        sscanf((const char *)&yytext[3], "%x", &n);
		yylval.c = n;
	        return CHAR;
	       }
0{OCTAL}+ {sscanf((const char *)&yytext[1], "%llo", &yylval.n); return INT;}
{DIGIT}+  {sscanf((const char *)yytext, "%lld", &yylval.n); return INT;}
("0x"|"0X"){HEXA}+ {sscanf((const char *)&yytext[2], "%llx", &yylval.n); return INT;}
{REAL}|{INT}{EREAL}|{INT}\.{EREAL}|{INT}\.{INT}{EREAL}|\.{INT}{EREAL} {
	   yylval.d = atof((const char *)yytext); return FLOAT;
         }
\\\" {/*oqmlerrok; */ yyerror("");}
\" {
  yylval.s = yytokstr();
  return STRING;
}
{INT}\.{INT}\.{INT}:oid {yylval.s = strdup((const char *)yytext); return OID;}
{INT}\:{INT}\:{INT}:oid {yylval.s = strdup((const char *)yytext); return OID;}
{HEXA}+:obj               {yylval.s = strdup((const char *)yytext); return OBJ;}
{CPPDIRECT} {
  int s = ((yytext[1] == ' ') ? 2 : 1);
  char *p = (char *)strchr((const char *)&yytext[s], ' ');
  if (p)
    {
      *p++ = 0;
      __line = atoi((const char *)&yytext[s]);
      char *q = strchr(p, ' ');
      if (q)
	*q = 0;
      free(oqml_file);
      oqml_file = strdup(p);
    }
}
{CPPLINE} {int s = ((yytext[1] == ' ') ? 2 : 1); __line = atoi((const char *)&yytext[s]);}
^\#ident[ ][^\n]*\n {}
"/*"   {
         if (!yyskipcomments())
	   return 0;
       }
"#"    {return SHARP;}
@{SYMBOL} {yylval.s = strdup((const char *)&yytext[1]); return SYMBOL;}
{SYMBOL} {
   int token;

   if (token = oqmlGetKeyword((const char *)yytext))
      return token;
   yylval.s = strdup((const char *)yytext);
   return SYMBOL;
}
"%"(odl|ODL)"{" {
  char *s = yygetquotedseq();
  yylval.s = strdup(s);
  free(s);
  return ODL_QUOTED_SEQ;
}
. {if (yytext[0] == 0) return 0;}


%%

