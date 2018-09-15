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
#include <eyedblib/butils.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace eyedb;
using namespace std;

#define mWRITE 0x1000
#define mREAD  0x2000
#define PROPAG_ON "propagate=on"
#define PROPAG_OFF "propagate=off"

enum mMode {
  mUpdate = mWRITE | 1,
  mSimulate = mWRITE | 2,
  mList = mREAD | 3,
  mStats = mREAD | 4,
  mSetDef = mWRITE | 5,
  mGetDef = mREAD | 6
};

#define ROOTPROG "collimpl"
#define PROG "eyedb" ROOTPROG "admin"

static const char *prog;
static mMode mode;
static Bool complete = False;
static Bool eyedb_prefix = False;
static Connection *conn;
static Database *db;
static const Attribute *attr;
static const Class *cls;
static int local = (getenv("EYEDBLOCAL") ? _DBOpenLocal : 0);

#define CHECK(S) \
if (S) { \
  print_prog(); \
  (S)->print(); \
  return 1; \
}

static const char *
str()
{
  if (!mode)
    return "\n" PROG " ";

  return "";
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

  if (!mode || mode == mUpdate)
    fprintf(pipe,
	    "%supdate <dbname> {<collections> hash|btree [<hints>|\"\"]}\n",
	    str());

  if (!mode || mode == mSimulate)
    fprintf(pipe,
	    "%ssimulate <dbname> [--full] {[--fmt=<fmt>] <collections> hash|btree [<hints>|\"\"]}\n",
	    str());

  if (!mode || mode == mList)
    fprintf(pipe,
	    "%slist <dbname> {<collections>}\n",
	    str());

  if (!mode || mode == mStats)
    fprintf(pipe,
	    "%sstats <dbname> [--full] [--fmt=<fmt>] {<collections>}\n",
	    str());

  if (!mode || mode == mSetDef)
    fprintf(pipe,
	    "%ssetdef <dbname> {<attrpath> hash|btree [<hints>|\"\"] [" PROPAG_ON "|"
	    PROPAG_OFF "|\"\"]]}\n",
	    str());

  if (!mode || mode == mGetDef)
    fprintf(pipe,
	    "%sgetdef <dbname> {[<classname>|<attrpath>]}\n",
	    str());

  if (!mode || mode == mUpdate || mode == mSimulate || mode == mList ||
      mode == mStats) {
    fprintf(pipe, "\nWhere <collections> is under one of the following forms:\n");
    fprintf(pipe, "     a collection name: <string> \n");
    fprintf(pipe, "     a collection oid:  <oid>\n");
    fprintf(pipe, "     an OQL query:      <query>:oql\n");
    
    if (!mode || mode == mStats) {
      fprintf(pipe, "\nFor instance:\n");
      fprintf(pipe, "     " PROG " stats foo X::extent\n");
      fprintf(pipe, "     " PROG " stats foo 2834:8:314562:oid\n");
      fprintf(pipe, "     " PROG " stats foo \"(select class.name = \\\"X\\\").extent\":oql\n");
    }
  }

  if (!mode || mode == mStats || mode == mSimulate) {

    fprintf(pipe, "  The --fmt option indicates an output format for hash index stat entries.\n");
    fprintf(pipe, HashIndexStats::fmt_help);
  }

  fflush(pipe);

  print_use_help(cerr);

  if (pipe != stderr)
    pclose(pipe);

  return 1;
}

static int
get_mode(int argc, char *argv[], int &start)
{
  start = 1;

  if (!strcmp(prog, PROG) || !strcmp(prog, ROOTPROG "admin")) {
    complete = True;

    if (argc < 2)
      return usage(prog);

    const char *cmd = argv[1];

    if (!strcmp(cmd, "update"))
      mode = mUpdate;
    else if (!strcmp(cmd, "list"))
      mode = mList;
    else if (!strcmp(cmd, "stats"))
      mode = mStats;
    else if (!strcmp(cmd, "simulate"))
      mode = mSimulate;
    else if (!strcmp(cmd, "setdef"))
      mode = mSetDef;
    else if (!strcmp(cmd, "getdef"))
      mode = mGetDef;
    else
      return usage(prog);
    start = 2;
  }
  else if (!strcmp(prog, "eyedbcollimplupdate") || !strcmp(prog, "collimplupdate"))
    mode = mUpdate;
  else if (!strcmp(prog, "eyedbcollimplsimulate") || !strcmp(prog, "collimplsimulate"))
    mode = mSimulate;
  else if (!strcmp(prog, "eyedbcollimpllist") || !strcmp(prog, "collimpllist"))
    mode = mList;
  else if (!strcmp(prog, "eyedbcollimplstats") || !strcmp(prog, "collimplstats"))
    mode = mStats;
  else if (!strcmp(prog, "eyedbcollimplsetdef") || !strcmp(prog, "collimplsetdef"))
    mode = mSetDef;
  else if (!strcmp(prog, "eyedbcollimplgetdef") || !strcmp(prog, "collimplgetdef"))
    mode = mGetDef;
  else
    return usage(prog);

  if (!strncmp(prog, "eyedb", strlen("eyedb")))
    eyedb_prefix = True;

  return 0;
}

static void
print_prog()
{
  fprintf(stderr, "%s: ", prog);
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

  if (mode != mList && mode != mStats)
    s = db->transactionBeginExclusive();
  else
    s = db->transactionBegin();

  CHECK(s);
  return 0;
} 

static int
get_collimpl(const char *attrpath, CollAttrImpl *&collimpl)
{
  Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr, attrpath);
  CHECK(s);
  s = Attribute::getCollAttrImpl(db, attrpath, collimpl);
  CHECK(s);
  if (!attr->getClass()->asCollectionClass() || attr->isIndirect()) {
    fprintf(stderr, "attribute path %s: "
	    "a collection implementation can be tied "
	    "only to a literal collection attribute\n",
	    attrpath);
    return 1;
  }
  return 0;
}

static int
get_collimpls(const Class *cls, LinkedList &list)
{
  const LinkedList *cllist;
  Status s = const_cast<Class *>(cls)->getAttrCompList(Class::CollectionImpl_C, cllist);
  CHECK(s);
    
  LinkedListCursor c(cllist);
  void *o;
  while (c.getNext(o))
    list.insertObject(o);
  return 0;
}

static int
get_collimpls(const char *info, LinkedList &list)
{
  Status s;
  if (info) {
    Status s;
    if (strchr(info, '.')) {
      s = Attribute::checkAttrPath(db->getSchema(), cls, attr, info);
      CHECK(s);
      CollAttrImpl *collimpl;
      if (get_collimpl(info, collimpl))
	return 1;
      if (collimpl) {
	list.insertObject(collimpl);
	return 0;
      }
      return 0;
    }
    
    const Class *cls = db->getSchema()->getClass(info);
    if (!cls) {
      print_prog();
      fprintf(stderr, "class '%s' not found\n", info);
      return 1;
    }
    
    return get_collimpls(cls, list);
  }

  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (get_collimpls(cls, list))
      return 1;
  }
  
  return 0;
}
static int
get_collections(const char *str, LinkedList &list)
{
  Status s;
  std::string query;

  int len = strlen(str);
  if (len > 4 && !strcmp(&str[len-4], ":oid")) {
    Oid oid(str);
    Object *o;
    s = db->loadObject(oid, o);
    CHECK(s);
    list.insertObject(o);
    return 0;
  }

  Bool collname;
  if (len > 4 && !strcmp(&str[len-4], ":oql")) {
    char *x = new char[len-3];
    strncpy(x, str, len-4);
    x[len-4] = 0;
    query = x;
    free(x);
    
    collname = False;
  }
  else {
    query = std::string("select collection.name = \"") + str + "\"";
    collname = True;
  }

  OQL oql(db, query.c_str());
  ObjectArray obj_arr;
  s = oql.execute(obj_arr);
  CHECK(s);
  if (!obj_arr.getCount() && collname) {
    fprintf(stderr, "No collection named '%s'\n", str);
    return 1;
  }

  for (int n = 0; n < obj_arr.getCount(); n++)
    if (obj_arr[n]->asCollection())
      list.insertObject(const_cast<Object *>(obj_arr[n]));
  return 0;
}

static void
collection_trace(Collection *coll)
{
  fprintf(stdout, "Collection %s:\n",
	  coll->getOid().toString());
  fprintf(stdout, "  Name: '%s'\n", coll->getName());
  fprintf(stdout, "  Collection of: %s *\n", coll->getClass()->asCollectionClass()->getCollClass()->getName());
}

static int
collimpl_update_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc - 1 < 2) return usage(prog);

  for (int n = 1; n < argc; n += 3) {
    if (n != 1)
      db->transactionBegin();
    LinkedList list;
    if (get_collections(argv[n], list))
      return 1;

    IndexImpl::Type type;
    const char *stype = argv[n+1];
    if (!strcmp(stype, "hash"))
      type = IndexImpl::Hash;
    else if (!strcmp(stype, "btree"))
      type = IndexImpl::BTree;
    else
      return usage(prog);

    IndexImpl *idximpl;
    const char *hints = (argc > n+2 ? argv[n+2] : "");
    Status s = IndexImpl::make(db, type, hints, idximpl);
    CHECK(s);

    LinkedListCursor c(list);
    Collection *coll;
    while (c.getNext((void *&)coll)) {
      coll->setImplementation(idximpl);
      s = coll->store();
      CHECK(s);
    }
    
    db->transactionCommit();
  }

  return 0;
}

static int
collimpl_list_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc - 1 < 1) return usage(prog);

  for (int n = 1; n < argc; n++) {
    LinkedList list;
    if (get_collections(argv[n], list))
      return 1;

    LinkedListCursor c(list);
    Collection *coll;
    for (int n = 0; c.getNext((void *&)coll); n++) {
      IndexImpl *idximpl;
      Status s = coll->getImplementation(idximpl, True);
      CHECK(s);
      if (n) fprintf(stdout, "\n");
      collection_trace(coll);
      fprintf(stdout, idximpl->toString("  ").c_str());
    }
  }

  return 0;
}

static const char fmt_opt[] = "--fmt=";

static int
collimpl_simulate_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  Bool full;
  int start;
  if (argc > 1 && !strcmp(argv[1], "--full")) {
    full = True;
    start = 2;
  }
  else {
    full = False;
    start = 1;
  }

  const char *fmt;
  if (argc > start+1 && !strncmp(argv[start], fmt_opt, strlen(fmt_opt))) {
    fmt = &argv[start+1][strlen(fmt_opt)];
    start++;
    //start += 2;
  }
  else {
    fmt = 0;
  }

  int nn = 0;
  for (int n = start; n < argc; n += 3) {
    if (argc - start < 2) return usage(prog);
    IndexImpl::Type type;
    const char *stype = argv[n+1];
    if (!strcmp(stype, "hash"))
      type = IndexImpl::Hash;
    else if (!strcmp(stype, "btree"))
      type = IndexImpl::BTree;
    else
      return usage(prog);

    IndexImpl *idximpl;
    const char *hints = (argc > n+2 ? argv[n+2] : "");
    Status s = IndexImpl::make(db, type, hints, idximpl);
    CHECK(s);
    
    LinkedList list;
    if (get_collections(argv[n], list))
      return 1;

    LinkedListCursor c(list);
    Collection *coll;
    for (; c.getNext((void *&)coll); nn++) {
      if (fmt && idximpl->getType() == IndexImpl::Hash) {
	IndexStats *stats1, *stats2 = 0;
	if (coll->asCollArray())
	  s = coll->asCollArray()->simulate(*idximpl, stats1, stats2);
	else
	  s = coll->simulate(*idximpl, stats1);
	CHECK(s);
	if (nn) fprintf(stdout, "\n");
	s = stats1->asHashIndexStats()->printEntries(fmt);
	CHECK(s);
	delete stats1;
	if (stats2) {
	  s = stats2->asHashIndexStats()->printEntries(fmt);
	  CHECK(s);
	  delete stats2;
	}
      }
      else {
	std::string stats1, stats2;
	if (coll->asCollArray())
	  s = coll->asCollArray()->simulate(*idximpl, stats1, stats2,
					    True, full, "  ");
	else
	  s = coll->simulate(*idximpl, stats1, True, full, "  ");
	CHECK(s);
	if (nn) fprintf(stdout, "\n");
	collection_trace(coll);
	fprintf(stdout, stats1.c_str());
	if (stats2.size())
	  fprintf(stdout, stats2.c_str());
      }
    }
  }

  return 0;
}

static int
collimpl_stats_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  Bool full;
  int start;
  if (argc > 1 && !strcmp(argv[1], "--full")) {
    full = True;
    start = 2;
  }
  else {
    full = False;
    start = 1;
  }

  const char *fmt;
  if (argc > start+1 && !strncmp(argv[start], fmt_opt, strlen(fmt_opt))) {
    fmt = &argv[start+1][strlen(fmt_opt)];
    start++;
  }
  else {
    fmt = 0;
  }

  if (argc == start)
    argc++;

  int nn = 0;
  for (int n = start; n < argc; n++) {
    Status s;
    LinkedList list;
    if (get_collections(argv[n], list))
      return 1;
    
    LinkedListCursor c(list);
    Collection *coll;
    for (; c.getNext((void *&)coll); nn++) {
      IndexImpl *_idximpl;
      s = coll->getImplementation(_idximpl);
      CHECK(s);
      if (fmt && _idximpl->getType() == IndexImpl::Hash) {
	IndexStats *stats1, *stats2 = 0;
	if (coll->asCollArray())
	  s = coll->asCollArray()->getImplStats(stats1, stats2);
	else
	  s = coll->getImplStats(stats1);
	CHECK(s);
	if (nn) fprintf(stdout, "\n");
	s = stats1->asHashIndexStats()->printEntries(fmt);
	CHECK(s);
	delete stats1;
	if (stats2) {
	  s = stats2->asHashIndexStats()->printEntries(fmt);
	  CHECK(s);
	  delete stats2;
	}
      }
      else {
	std::string stats1, stats2;
	if (coll->asCollArray())
	  s = coll->asCollArray()->getImplStats(stats1, stats2,
						True, full, "  ");
	else
	  s = coll->getImplStats(stats1, True, full, "  ");
	CHECK(s);
	if (nn) fprintf(stdout, "\n");
	collection_trace(coll);
	fprintf(stdout, stats1.c_str());
	if (stats2.size())
	  fprintf(stdout, stats2.c_str());
      }
      if (_idximpl)
	_idximpl->release();
    }
  }

  return 0;
}

static int
collimpl_getdef_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  if (argc == 1)
    argc++;
  int nn = 0;
  for (int n = 1; n < argc; n++) {
    const char *info = argv[n];
    LinkedList list;
    if (get_collimpls(info, list))
      return 1;

    if (list.getCount()) {
      LinkedListCursor c(list);
      CollAttrImpl *collimpl;
      
      for (; c.getNext((void *&)collimpl); nn++) {
	if (nn) fprintf(stdout, "\n");
	fprintf(stdout, "Default implementation on %s:\n",
		collimpl->getAttrpath().c_str());
	const IndexImpl *idximpl;
	Status s = collimpl->getImplementation(db, idximpl);
	CHECK(s);
	fprintf(stdout, idximpl->toString("  ").c_str());
      }
    }
    else if (strchr(info, '.')) {
      fprintf(stdout, "Default implementation on %s:\n", info);
      fprintf(stdout, "  System default\n");
    }
  }

  return 0;
}

static int
collimpl_setdef_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  for (int n = 1; n < argc; n += 4) {
    if (argc - 1 < 2) return usage(prog);
    if (n != 1)
      db->transactionBegin();
    const char *attrpath = argv[n];
    Status s;
    const Attribute *attr;
    const Class *cls;
    s = Attribute::checkAttrPath(db->getSchema(), cls, attr, attrpath);
    CHECK(s);

    IndexImpl::Type type;
    const char *stype = argv[n+1];
    if (!strcmp(stype, "hash"))
      type = IndexImpl::Hash;
    else if (!strcmp(stype, "btree"))
      type = IndexImpl::BTree;
    else
      return usage(prog);
    
    Bool propag;
    
    const char *propagate = (argc > n+3 ? argv[n+3] : 0);
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
    
    IndexImpl *idximpl;
    const char *hints = (argc > n+2 ? argv[n+2] : "");
    s = IndexImpl::make(db, type, hints, idximpl);
    CHECK(s);

    CollAttrImpl *collimpl;
    if (get_collimpl(attrpath, collimpl))
      return 1;

    if (collimpl) {
      s = collimpl->remove();
      CHECK(s);
    }

    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
    collimpl = new CollAttrImpl
      (db, const_cast<Class *>(cls), attrpath,
       propag, idximpl->getDataspace(),
       idximpl->getType(),
       idximpl->getType() == IndexImpl::Hash ?
       idximpl->getKeycount() : idximpl->getDegree(),
       idximpl->getHashMethod(),
       impl_hints, impl_hints_cnt);
    
    s = collimpl->store();
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

  /*
  if (argc < 3)
    return usage(prog);
  */

  eyedb::init(argc, argv);

  switch(mode) {

  case mUpdate:
    return collimpl_update_realize(argc - start, &argv[start]);

  case mList:
    return collimpl_list_realize(argc - start, &argv[start]);

  case mStats:
    return collimpl_stats_realize(argc - start, &argv[start]);

  case mSimulate:
    return collimpl_simulate_realize(argc - start, &argv[start]);

  case mGetDef:
    return collimpl_getdef_realize(argc - start, &argv[start]);

  case mSetDef:
    return collimpl_setdef_realize(argc - start, &argv[start]);

  default:
    return usage(prog);
  }
}
