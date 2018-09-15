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
#define _YYTEXT_ ((const char *)yytext)
static int yyskipcomments();
static char *yygetquotedseq(char sep = '}');
int yycgid(wchar_t c);
void yywunput(wchar_t wc);
%}

STRING    \"([^"]|\\\")*\"
INTEGER	  [0-9]+
SYMBOL	  [a-zA-Z_]+[a-zA-Z0-9_]*
CPPDIRECT #\ ?{INTEGER}[\ ]{STRING}([\ ]{INTEGER})?
CPPLINE   #\ ?{INTEGER}\n
%%

{STRING} {
	char *s = new char[strlen(_YYTEXT_)+1];
	strcpy(s, &_YYTEXT_[1]); s[strlen(s)-1] = 0;
	yylval.s = s;
	return STRING;
}

[0-9]+		{yylval.i = atoi(_YYTEXT_); return INT;}
0(x|X)[0-9A-Fa-f]+ {sscanf(_YYTEXT_, "0x%x", &yylval.i); return INT;}
\'.\'		{yylval.i = (int)_YYTEXT_[1]; return CHAR;}
"+"		{return '+';}
"-"		{return '-';}
"*"		{return '*';}
"/"		{return '/';}
"("		{return '(';}
")"		{return ')';}
"<<"		{return SHIFT_L;}
">>"		{return SHIFT_R;}
">="            {return SUP_EQ;}
"<="            {return INF_EQ;}
"::"		{return SCOPE;}
"%"[cC]"++{"    {
	          yylval.s = yygetquotedseq();
		  if (odl_lang == ProgLang_C)
		    return QUOTED_SEQ;
		}
"%"(java|JAVA|Java)"{" {
	          yylval.s = yygetquotedseq();
		  if (odl_lang == ProgLang_Java)
		    return QUOTED_SEQ;
		}
"%"(oql|OQL)"{" {
                  char *s = yygetquotedseq();
	          yylval.s = strdup((std::string("{") + s + "}").c_str());
                  free(s);
                  return OQL_QUOTED_SEQ;
		}
"%"(oql|OQL)"(" {
                  char *s = yygetquotedseq(')');
	          yylval.s = strdup((std::string("(") + s + ");").c_str());
                  free(s);
                  return OQL_QUOTED_SEQ;
		}
[ \t]+ { }
{CPPDIRECT} {
	int s = ((_YYTEXT_[1] == ' ') ? 2 : 1);
	char *p = (char *)strchr(&_YYTEXT_[s], ' ');
	if (p)
	  {
	    *p++ = 0;
	    _line = atoi(&_YYTEXT_[s])-1;
	    char *q = strchr(p, ' ');
	    if (q)
	      *q = 0;
            free(_file);
	    _file = strdup(p);
	  }
}
{CPPLINE} {int s = ((_YYTEXT_[1] == ' ') ? 2 : 1); _line = atoi(&_YYTEXT_[s]); }
^\#ident[ ][^\n]*\n {}
[\n]   {_line++; odl_prompt();}
[\\\n] {_line++;}
"//".*[\n] {_line++; odl_prompt();}
"C++"|"c++" {return EXTREF;}
@[ ]*[a-zA-Z_][a-zA-Z_]* {
   const char *ys = strrchr(_YYTEXT_, ' ');

   if (ys) ++ys;
   else ys = &_YYTEXT_[1];

   if (!strcasecmp(ys, "removed"))
     return REMOVED;

   if (!strcasecmp(ys, "renamed_from"))
     return RENAMED_FROM;

   if (!strcasecmp(ys, "convert_method"))
     return CONVERT_METHOD;

   if (!strcasecmp(ys, "migrated_from"))
     return MIGRATED_FROM;

   if (!strcasecmp(ys, "extended"))
     return EXTENDED;

   yyerror((std::string("unknown ODL directive: ") + _YYTEXT_).c_str());
}

called_from[ \t]*=[ \t]*OQL {
  return CALLED_FROM_OQL;
}
called_from[ \t]*=[ \t]*any {
  return CALLED_FROM_ANY;
}
called_from[ \t]*=[ \t]*"C++" {
  return CALLED_FROM_C;
}
called_from[ \t]*=[ \t]*Java {
  return CALLED_FROM_JAVA;
}
[a-zA-Z_][a-zA-Z_0-9]* {
  char c = _YYTEXT_[0];

  if (c == 'a')
    {
      if (!strcmp(_YYTEXT_, "attribute"))
	return ATTRIBUTE;			
    }
  else if (c == 'b')
    {
      if (!strcmp(_YYTEXT_, "backend"))
	return SERVER;
      if (!strcmp(_YYTEXT_, "btreeindex"))
	return BTREEINDEX;
      if (!strcmp(_YYTEXT_, "btree"))
	return BTREE;
    }
  else if (c == 'c')
    {
      if (!strcmp(_YYTEXT_, "card"))
	return CARD;				
      if (!strcmp(_YYTEXT_, "classmethod") ||
	  !strcmp(_YYTEXT_, "class_method"))
	return CLASSMETHOD;			
      if (!strcmp(_YYTEXT_, "client"))
	return CLIENT;
      if (!strcmp(_YYTEXT_, "constraint"))
	return CONSTRAINT;
    }
  else if (c == 'e')
    {
      if (!strcmp(_YYTEXT_, "enum"))
	return ENUM;				
      if (!strcmp(_YYTEXT_, "extends"))
	return EXTENDS;			
      if (!strcmp(_YYTEXT_, "extref"))
	return EXTREF;				
    }
  else if (c == 'f')
    {
      if (!strcmp(_YYTEXT_, "frontend"))
	return CLIENT;
    }
  else if (c == 'h')
    {
      if (!strcmp(_YYTEXT_, "hashindex"))
	return HASHINDEX;
      if (!strcmp(_YYTEXT_, "hash"))
	return HASH;				
      if (!strcmp(_YYTEXT_, "hints"))
	return HINTS;
    }
  else if (c == 'i')
    {
      if (!strcmp(_YYTEXT_, "inverse"))
	return INVERSE;			
      if (!strcmp(_YYTEXT_, "index"))
	return INDEX;				
      if (!strcmp(_YYTEXT_, "in"))
	return IN;				
      if (!strcmp(_YYTEXT_, "inout"))
	return INOUT;				
      if (!strcmp(_YYTEXT_, "instmethod") ||
	  !strcmp(_YYTEXT_, "inst_method") ||
	  !strcmp(_YYTEXT_, "instance_method"))
	return INSTMETHOD;			
      if (!strcmp(_YYTEXT_, "implementation"))
	return IMPLEMENTATION;
    }
  else if (c == 'l')
    {
      if (!strcmp(_YYTEXT_, "lighttrigger"))
	{
	  yylval.i = 1;				
	  return TRIGGER;			
	}
    }
  else if (c == 'm')
    {
      if (!strcmp(_YYTEXT_, "method"))
	return METHOD;				
    }
  else if (c == 'n')
    {
      if (!strcmp(_YYTEXT_, "noindex"))
	return NOINDEX;
      if (!strcmp(_YYTEXT_, "notnull"))
	return NOTNULL;			
      if (!strcmp(_YYTEXT_, "notempty"))
	return NOTEMPTY;			
      if (!strcmp(_YYTEXT_, "nil"))
	return NIL;			
    }
  else if (c == 'o')
    {
      if (!strcmp(_YYTEXT_, "out"))
	return OUT;				
      if (!strcmp(_YYTEXT_, "on"))
	return ON;				
      if (!strcmp(_YYTEXT_, "off"))
	return OFF;				
    }
  else if (c == 'p')
    {
      if (!strcmp(_YYTEXT_, "propagate"))
	return PROPAGATE;
    }
  else if (c == 'r')
    {
      if (!strcmp(_YYTEXT_, "relationship"))
	return RELSHIP;			
    }
  else if (c == 's')
    {
      if (!strcmp(_YYTEXT_, "static"))
	return STATIC;				
      if (!strcmp(_YYTEXT_, "server"))
	return SERVER;
    }
  else if (c == 't')
    {
      if (!strcmp(_YYTEXT_, "trigger"))
	{
	  yylval.i = 0;				
	  return TRIGGER;			
	}
      if (!strcmp(_YYTEXT_, "type"))
	return TYPE;
    }
  else if (c == 'u')
    {
      if (!strcmp(_YYTEXT_, "unique"))
	return UNIQUE;				
    }

  yylval.s = strdup(_YYTEXT_);

  return IDENT;
}
(#|$)[a-zA-Z_][a-zA-Z_0-9]* {
  yylval.s = strdup(&_YYTEXT_[1]);
  return IDENT;
}
"/*"   {
         if (!yyskipcomments())
	   return 0;
       }
.	{ return (int)_YYTEXT_[0];}

%%

int
yywrap()
{
  return 1;
}

void
yyerror(const char *msg)
{
  if (errline == _line)
    return;

 // 10/09/04: suppressed yyerrok call for bison support
 //yyerrok;

  if (errcnt++ > 5)
    return;

  const char *premsg = (!*msg ? "syntax error " : "");

  if (_file)
    odl_add_error("%snear line %d, file %s %s\n",
		   premsg, _line, _file, msg);
  else
    odl_add_error("%snear line %d %s\n", _line, msg); 

  errline = _line;
}

static int
yyskipcomments()
{
  int c;
  while (c = yyinput())
    {
      if (c == '\n')
	_line++;
      else if (c == '*')
	{
	  c = yyinput();
	  if (c == '/')
	    return 1;
	  if (c == '\n')
	    _line++;
	}
    }

  if (c == 0)
    return 0;

  return 1;
}

static void
append(char *& s, int& i, int &len, char c)
{
  if (i >= len)
    {
      len = i + 64;
      s = (char *)realloc(s, len);
    }
  s[i++] = c;
}

static char *
yygetquotedseq(char sep)
{
  char *s = 0;
  int i = 0, len = 0;
  int c;

  while (c = yyinput()) {
    if (c == '\n') {
      _line++;
    }
    else if (c == '%') {
      c = yyinput();
      if (c == sep) {
	append(s, i, len, 0);
	return s;
      }
      append(s, i, len, '%');
      if (c == '\n')
	_line++;
    }
    
    append(s, i, len, c);
  }

  append(s, i, len, 0);
  return s;
}
