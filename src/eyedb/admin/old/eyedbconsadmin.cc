/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-1999,2004-2006 SYSRA
   
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


#include <eyedb/eyedb.h>

using namespace eyedb;
using namespace std;

#define mWRITE 0x1000
#define mREAD  0x2000
#define PROPAG_ON "propagate=on"
#define PROPAG_OFF "propagate=off"

enum mMode {
  mCreate = mWRITE | 1,
  mDelete = mWRITE | 2,
  mList = mREAD | 3
};

static const char *prog;
static mMode mode;
static Bool complete = False;
static Bool eyedb_prefix = False;
static Connection *conn;
static Database *db;
static const Attribute *attr;
static const Class *cls;
static int local = (getenv("EYEDBLOCAL") ? _DBOpenLocal : 0);

#define ROOTPROG "cons"
#define PROG "eyedb" ROOTPROG "admin"

static const char *
str()
{
  if (!mode)
    return "\n" PROG " ";

  return "";
}
#define CHECK(S) \
  if (S) { \
   print_prog(); \
   (S)->print(); \
   return 1; \
   }

static void
print_prog()
{
  fprintf(stderr, "%s: ", prog);
}

static int
usage(const char *prog)
{
  static Bool displayed = False;
  if (displayed) return 1;
  displayed = True;

  FILE *pipe = NULL;
  
  if (!pipe)
    pipe = stderr;

  if (!mode)
    fprintf(pipe, PROG " usage:\n");
  else if (complete)
    fprintf(pipe, "usage: " PROG " ");
  else if (eyedb_prefix)
    fprintf(pipe, "usage: eyedb" ROOTPROG);
  else
    fprintf(pipe, "usage: " ROOTPROG);

  if (!mode || mode == mCreate)
    fprintf(pipe,
	    "%screate <dbname> {unique|notnull <attrpath> [" PROPAG_ON "|"
	    PROPAG_OFF "|\"\"]]}\n",
	    str());

  if (!mode || mode == mList)
    fprintf(pipe,
	    "%slist <dbname> {unique|notnull [<attrpath>|<classname>]}\n",
	    str());

  if (!mode || mode == mDelete)
    fprintf(pipe,
	    "%sdelete <dbname> {unique|notnull <attrpath>}\n",
	    str());

  fflush(pipe);

  print_use_help(cerr);

  if (pipe != stderr)
    pclose(pipe);

  return 1;
}

#if 0
static int
usage(const char *prog)
{
  FILE *pipe = 0;
  
  if (!pipe)
    pipe = stderr;

  fprintf(pipe,
	  "\nusage:\n"
	  "\n"
	  HPROG " <dbname> create <constraint> <attrpath> [" PROPAG_ON "|"
	  PROPAG_OFF "]\n"
	  HPROG " <dbname> delete <constraint> <attrpath>\n"
	  HPROG " <dbname> list [<attrpath>|<clsname>]\n"
	  "\n"
	  "Where :\n"
	  "  <dbname> is the database name\n"
	  "  <clsname> is the class name.\n"
	  "  <attrpath> is the path attribute.\n"
	  "\n"
	  "Where <constraint> is one of the following:\n"
	  "  unique      : unique constraint on basic types\n"
	  "  notnull     : notnull constraint on basic types\n"
#ifdef CARD_SUPPORT
	  "  <card_spec> : cardinality constraint specification\n"
	  "\n"
	  "Where <card_spec> is under one of the forms:\n"
	  "  card > <int>\n"
	  "  card > <int>\n"
	  "  card >= <int>\n"
	  "  card <= <int>\n"
	  "  card in [<int>, <int>]\n"
	  "  card in ]<int>, <int>]\n"
	  "  card in [<int>, <int>[\n"
	  "  card in ]<int>, <int>[\n"
	  */
	  "\n"
	  "The key words: create, delete, list, unique, notnull and card are case insensitive.\n"
#else
	  "The key words: create, delete, list, unique and notnull are case insensitive.\n"
	  "\n"
#endif
	  "For instance:\n"
	  "  %% " HPROG " foo create notnull Person.father\n"
	  "  %% " HPROG " foo create unique Person.addr.street " PROPAG_ON "\n");
	  
  if (pipe != stderr)
    pclose(pipe);
  return 1;
}
#endif

static void
capstring(char *str)
{
  char c;
  while (c = *str)
    {
      if (c >= 'a' && c <= 'z')
	*str = c + 'A' - 'a';
      str++;
    }
}

enum Token {
  INVALID = 0,
  OPERATOR,
  NUMBER,
  STRING,
  END
};

static Token
getnexttok(const char *&s, char tok[])
{
  while (*s == ' ' || *s == '\t' || *s == '\n')
    s++;

  char c = *s;

  if (!c)
    return END;

  if (c == '[' || c == ']' || c == '>' || c == '<' || c == '=' || c == ',')
    {
      char x[3];
      x[0] = c;

      if ((c == '>' || c == '<') && *(s+1) == '=')
	{
	  x[1] = '=';
	  x[2] = 0;
	  s += 2;
	}
      else
	{
	  x[1] = 0;
	  s++;
	}

      strcpy(tok, x);
      return OPERATOR;
    }

  if (c >= '0' && c <= '9')
    {
      int n = 0;
      while (*s >= '0' && *s <= '9')
	tok[n++] = *s++;

      tok[n] = 0;
      return NUMBER;
    }

  if (c >= 'A' && c <= 'Z')
    {
      int n = 0;
      while (*s >= 'A' && *s <= 'Z')
	tok[n++] = *s++;

      tok[n] = 0;
      return STRING;
    }

  return INVALID;
}

#define WAIT_NUMBER(CONSTRAINT, TOK, BOUND) \
if (getnexttok(CONSTRAINT, TOK) != NUMBER)\
  return False; \
BOUND = atoi(TOK); \
if (getnexttok(CONSTRAINT, TOK) != END) \
  return False

#ifdef CARD_SUPPORT
static Bool
check_card(const char *constraint, int &bottom, int &bottom_excl,
	   int &top, int &top_excl)
{
  char tok[32];
  Token t = getnexttok(constraint, tok);

  if (t != STRING || strcmp(tok, "CARD"))
    return False;

  t = getnexttok(constraint, tok);
  
  if (!strcmp(tok, ">"))
    {
      WAIT_NUMBER(constraint, tok, bottom);
      bottom_excl = 1;
      top = CardinalityConstraint::maxint;
    }
  else if (!strcmp(tok, ">="))
    {
      WAIT_NUMBER(constraint, tok, bottom);
      bottom_excl = 0;
      top = CardinalityConstraint::maxint;
    }
  else if (!strcmp(tok, "<"))
    {
      WAIT_NUMBER(constraint, tok, top);
      top_excl = 1;
      bottom = 0;
      bottom_excl = 0;
    }
  else if (!strcmp(tok, "<="))
    {
      WAIT_NUMBER(constraint, tok, top);
      top_excl = 0;
      bottom = 0;
      bottom_excl = 0;
    }
  else if (!strcmp(tok, "="))
    {
      WAIT_NUMBER(constraint, tok, bottom);
      bottom_excl = top_excl = 0;
      top = bottom;
    }
  else if (!strcmp(tok, "IN"))
    {
      if (getnexttok(constraint, tok) != OPERATOR)
	return False;

      if (!strcmp(tok, "["))
	bottom_excl = 0;
      else if (!strcmp(tok, "]"))
	bottom_excl = 1;
      else
	return False;

      if (getnexttok(constraint, tok) != NUMBER)
	return False;

      bottom = atoi(tok);

      if (getnexttok(constraint, tok) != OPERATOR)
	return False;

      if (strcmp(tok, ","))
	return False;

      if (getnexttok(constraint, tok) != NUMBER)
	return False;

      top = atoi(tok);
	
      if (getnexttok(constraint, tok) != OPERATOR)
	return False;

      if (!strcmp(tok, "["))
	top_excl = 1;
      else if (!strcmp(tok, "]"))
	top_excl = 0;
      else
	return False;

      if (getnexttok(constraint, tok) != END)
	return False;
    }
  else
    return False;

  return True;
}
#endif

static int
get_mode(int argc, char *argv[], int &start)
{
  start = 1;

  if (!strcmp(prog, PROG) || !strcmp(prog, ROOTPROG "admin")) {
    complete = True;

    if (argc < 2)
      return usage(prog);

    const char *cmd = argv[1];

    if (!strcmp(cmd, "create"))
      mode = mCreate;
    else if (!strcmp(cmd, "delete"))
      mode = mDelete;
    else if (!strcmp(cmd, "list"))
      mode = mList;
    else
      return usage(prog);
    start = 2;
    }
  else if (!strcmp(prog, "eyedbconscreate") || !strcmp(prog, "conscreate"))
    mode = mCreate;
  else if (!strcmp(prog, "eyedbconsdelete") || !strcmp(prog, "consdelete"))
    mode = mDelete;
  else if (!strcmp(prog, "eyedbconslist") || !strcmp(prog, "conslist"))
    mode = mList;
  else
    return usage(prog);

  if (!strncmp(prog, "eyedb", strlen("eyedb")))
    eyedb_prefix = True;

  return 0;
}

static int
init(int argc, char *argv[])
{
  Status s;

  if (!argc) return usage(prog);

  conn = new Connection();
  s = conn->open();
  CHECK(s);

  int i;

  for (i = 0; i < argc; i++)
    if (*argv[i] != '-') {
      db = new Database(argv[i]);
      break;
    }

  if (i == argc) return usage(prog);

  s = db->open(conn, (Database::OpenFlag)((mode & mWRITE) ? Database::DBRW|local : Database::DBSRead|local));
  CHECK(s);

  if (mode != mList)
    s = db->transactionBeginExclusive();
  else
    s = db->transactionBegin();

  CHECK(s);
  return 0;
} 

static int
get_cons(const char *constraint, const char *&cnsname,
	 Class::AttrCompIdx &idx)
{
  if (!strcmp(constraint, "notnull")) {
    idx = Class::NotnullConstraint_C;
    cnsname = "notnull";
    return 0;
  }

  if (!strcmp(constraint, "unique")) {
    idx = Class::UniqueConstraint_C;
    cnsname = "unique";
    return 0;
  }

  return usage(prog);
}

static int
cons_create_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  for (int n = 1; n < argc; n += 3) {
    if (argc - 1 < 2) return usage(prog);
    if (n != 1)
      db->transactionBegin();
    const char *attrpath = argv[n+1];
    Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr,
					      attrpath);
    CHECK(s);

    const char *propagate = (argc > n+2 ? argv[n+2] : 0);
    Bool propag;
    if (propagate) {
      if (!strcasecmp(propagate, PROPAG_ON))
	propag = True;
      else if (!strcasecmp(propagate, PROPAG_OFF))
	propag = False;
      else {
	print_prog();
	fprintf(stderr, PROPAG_ON " or " PROPAG_OFF " expected, got: %s\n",
		propagate);
	return 1;
      }
    }
    else
      propag = True;

    Class::AttrCompIdx idx;
    const char *cnsname;
    if (get_cons(argv[n], cnsname, idx))
      return 1;
    
    AttributeComponent *acomp;
    if (idx == Class::NotnullConstraint_C)
      acomp = new NotNullConstraint(db, (Class *)cls, attrpath, propag);
    else
      acomp = new UniqueConstraint(db, (Class *)cls, attrpath, propag);
    
    printf("Creating %s constraint on %s\n", cnsname, attrpath);
    s = acomp->store();
    CHECK(s);
    db->transactionCommit();
  }

  return 0;
}

static int
get_cons(const Class *cls, Class::AttrCompIdx idx,
	 LinkedList &conslist)
{
  const LinkedList *clconslist;
  Status s = const_cast<Class *>(cls)->getAttrCompList(idx, clconslist);
  CHECK(s);
    
  LinkedListCursor c(clconslist);
  void *o;
  while (c.getNext(o))
    conslist.insertObject(o);
  return 0;
}

static int
get_cons(const char *info, Class::AttrCompIdx idx, LinkedList &conslist)
{
  Status s;
  if (info) {
    Status s;
    if (strchr(info, '.')) {
      s = Attribute::checkAttrPath(db->getSchema(), cls, attr, info);
      CHECK(s);
      AttributeComponent *comp;

      if (idx == Class::UniqueConstraint_C)
	s = Attribute::getUniqueConstraint(db, info,
					      (UniqueConstraint *&)comp);
      else if (idx == Class::NotnullConstraint_C)
	s = Attribute::getNotNullConstraint(db, info,
					       (NotNullConstraint *&)comp);
      else
	abort();

      CHECK(s);
      if (comp) {
	conslist.insertObject(comp);
	return 0;
      }
      print_prog();
      fprintf(stderr, "%s constraint on %s not found\n", 
	      (idx == Class::UniqueConstraint_C ? "unique" : "notnull"),
	      info);
      return 1;
    }
    
    const Class *cls = db->getSchema()->getClass(info);
    if (!cls) {
      print_prog();
      fprintf(stderr, "class %s not found\n", info);
      return 1;
    }
    
    return get_cons(cls, idx, conslist);
  }

  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (get_cons(cls, idx, conslist))
      return 1;
  }
  
  return 0;
}

static void
cons_print(int &n, LinkedList &conslist)
{
  LinkedListCursor c(conslist);
  AttributeComponent *attr_comp;
  for (; c.getNext((void *&)attr_comp); n++) {
    if (n)
      fprintf(stdout, "\n");
    fprintf(stdout, "%s constraint on %s:\n",
	    (attr_comp->asNotNullConstraint() ? "Notnull" : "Unique"),
	    attr_comp->getAttrpath().c_str());
    fprintf(stdout, "  Propagation: %s\n", attr_comp->getPropagate() ? "on" : "off");
  }
}

static int
cons_list_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  //if (argc != 1 && argc != 2 && argc != 3) return usage(prog);

  if (argc == 1)
    argc++;
  for (int n = 1; n < argc; n++) {
    LinkedList conslist;
    int nn = 0;
    if (argv[n]) {
      Class::AttrCompIdx idx;
      const char *cnsname;
      if (get_cons(argv[n], cnsname, idx))
	return 1;

      const char *info = (argc > n+1 ? argv[n+1] : 0);
      if (get_cons(info, idx, conslist))
	return 1;

      cons_print(n, conslist);
    }
    else {
      if (get_cons((const char *)0, Class::NotnullConstraint_C, conslist))
	return 1;
      cons_print(nn, conslist);
      conslist.empty();
      if (get_cons((const char *)0, Class::UniqueConstraint_C, conslist))
	return 1;
      cons_print(nn, conslist);
    }
  }

  return 0;
}

static int
cons_delete_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  for (int n = 1; n < argc; n += 2) {
    if (argc - n < 2) return usage(prog);
    if (n != 1)
      db->transactionBegin();
    const char *attrpath = argv[n+1];
    Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr,
					      attrpath);
    CHECK(s);

    Class::AttrCompIdx idx;
    const char *cnsname;
    if (get_cons(argv[n], cnsname, idx))
      return 1;

    AttributeComponent *acomp;
    if (idx == Class::NotnullConstraint_C)
      s = Attribute::getNotNullConstraint(db, attrpath,
					     (NotNullConstraint *&)acomp);
    else
      s = Attribute::getUniqueConstraint(db, attrpath,
					    (UniqueConstraint *&)acomp);
    
    if (!acomp) {
      print_prog();
      fprintf(stderr, "%s constraint on %s not found\n", cnsname, attrpath);
      return 1;
    }

    printf("Deleting %s constraint on %s\n", cnsname, attrpath);
    s = acomp->remove();
    CHECK(s);
    
    db->transactionCommit();
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  char *p;
  int start;

  prog = ((p = strrchr(argv[0], '/')) ? p+1 : argv[0]);

  eyedb::init();

  if (get_mode(argc, argv, start))
    return 1;

  if (start < argc && (!strcmp(argv[start], "--help") ||
		       !strcmp(argv[start], "-h"))) {
    usage(prog);
    return 0;
  }

  if (argc < 3)
    return usage(prog);

  eyedb::init(argc, argv);

  switch(mode) {

  case mCreate:
    return cons_create_realize(argc - start, &argv[start]);

  case mDelete:
    return cons_delete_realize(argc - start, &argv[start]);

  case mList:
    return cons_list_realize(argc - start, &argv[start]);

  default:
    return usage(prog);
  }
}

#if 0

  EyeDB _(argc, argv);

  if (argc != 5 && argc != 6)
    return usage(argv[0]);

  prog = argv[0];
  const char *dbname     = argv[1];
  char *action_str = argv[2];
  char *constraint = argv[3];
  const char *attrpath    = argv[4];
  const char *propagate    = (argc > 5 ? argv[5] : NULL);
  const char *clname = 0;

  capstring(action_str);

  // first pass for check
  if (!strcmp(action_str, "CREATE"))
    action = Create;
  else if (!strcmp(action_str, "DELETE"))
    action = Delete;
  else if (!strcmp(action_str, "LIST"))
    action = List;
  else
    return usage(argv[0]);

  capstring(constraint);

  Status s;

  Connection *conn = new Connection();
  s = conn->open();
  CHECK(s);

  Database *db = new Database(dbname);

  s = db->open(conn, Database::DBRW);
  CHECK(s);
  
  Bool notnull = False, unique = False, card = False;
  int bottom, bottom_excl, top, top_excl;

  if (!strcmp(constraint, "NOTNULL"))
    notnull = True;
  else if (!strcmp(constraint, "UNIQUE"))
    unique = True;
#ifdef CARD_SUPPORT
  else if (check_card(constraint, bottom, bottom_excl, top, top_excl))
    card = True;
#endif
  else
    return usage(prog);

  db->transactionBegin();

  const Attribute *attr;
  const Class *cls;
  if (notnull || unique) {
    s = Attribute::checkAttrPath(db->getSchema(), cls, attr, attrpath);
    CHECK(s);
  }

  if (action == Create) {
    Bool propag;
    if (propagate) {
      if (!strcasecmp(propagate, PROPAG_ON))
	propag = True;
      else if (!strcasecmp(propagate, PROPAG_OFF))
	propag = False;
      else {
	print_prog();
	fprintf(stderr, PROPAG_ON " or " PROPAG_OFF " expected, got: %s\n",
		propagate);
	return 1;
      }
    }
    else
      propag = True;

    AttributeComponent *attr_comp;

    if (notnull) {
      attr_comp = new NotNullConstraint(db, (Class *)cls, attrpath,
					   propag);
    }
    else if (unique) {
      attr_comp = new UniqueConstraint(db, (Class *)cls, attrpath,
					  propag);
    }
#ifdef CARD_SUPPORT
    else if (card) {
      attr_comp = new CardinalityConstraint_Test(db, (Class *)cls, detail, bottom, bottom_excl, top, top_excl);
    }
#endif

    s = attr_comp->store();
    CHECK(s);
    return 0;
  }

  const char *xname = (unique ? "unique" :
		       (notnull ? "notnull" :
			(card ? "cardinality" : "")));
  OQL oql(db, "select %s_constraint.attrpath = \"%s\"",
	     xname, attrpath);

  ObjectArray obj_arr;
  s = oql.execute(obj_arr);
  CHECK(s);

  if (!obj_arr.getCount()) {
    print_prog();
    fprintf(stderr, "no %s constraint '%s' found\n", xname, attrpath);
    return 1;
  }

  for (int i = 0; i < obj_arr.getCount(); i++) {
    if (action == List)
      obj_arr[i]->trace();
    else {
      s = obj_arr[i]->remove();
      CHECK(s);
    }
  }

  db->transactionCommit();

  return 0;
}
#endif
