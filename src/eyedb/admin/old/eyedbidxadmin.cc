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
#include <signal.h>

using namespace eyedb;
using namespace std;

#define mWRITE 0x1000
#define mREAD  0x2000
#define PROPAG_ON "propagate=on"
#define PROPAG_OFF "propagate=off"

enum mMode {
  mCreate = mWRITE | 1,
  mUpdate = mWRITE | 2,
  mSimulate = mWRITE | 3,
  mList = mREAD | 4,
  mStats = mREAD | 5,
  mDelete = mWRITE | 6
};

#define ROOTPROG "idx"
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
do { \
 if (S) { \
   print_prog(); \
   (S)->print(); \
   return 1; \
  } \
} while(0)

#define CHECK_W(S) \
do { \
 if (S) { \
   print_prog(); \
   (S)->print(); \
  } \
} while(0)

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

  if (!mode || mode == mCreate)
    fprintf(pipe,
	    "%screate <dbname> [--check] {<attrpath> [hash|btree [<hints>|\"\" [" PROPAG_ON "|"
	    PROPAG_OFF "|\"\"]]]}\n",
	    str());

  if (!mode || mode == mUpdate)
    fprintf(pipe,
	    "%supdate <dbname> [--check] {<attrpath> [hash|btree [<hints>|\"\"]] [" PROPAG_ON "|"
	    PROPAG_OFF "|\"\"]}\n",
	    str());

  if (!mode || mode == mSimulate)
    fprintf(pipe,
	    "%ssimulate <dbname> [--full] [--fmt=<fmt>] {<attrpath> hash|btree [<hints>|\"\"]}\n",
	    str());

  if (!mode || mode == mList)
    fprintf(pipe,
	    "%slist <dbname> [--full] {[<attrpath>|<classname>|--all]}\n",
	    str());

  if (!mode || mode == mStats) {
    fprintf(pipe,
	    "%sstats <dbname> [--full] [--fmt=<fmt>] {[<attrpath>|<classname>|--all]}\n",
	    str());
  }

  if (!mode || mode == mDelete)
    fprintf(pipe,
	    "%sdelete <dbname> {<attrpath>}\n",
	    str());

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

    if (!strcmp(cmd, "create"))
      mode = mCreate;
    else if (!strcmp(cmd, "delete"))
      mode = mDelete;
    else if (!strcmp(cmd, "list"))
      mode = mList;
    else if (!strcmp(cmd, "stats"))
      mode = mStats;
    else if (!strcmp(cmd, "update"))
      mode = mUpdate;
    else if (!strcmp(cmd, "simulate"))
      mode = mSimulate;
    else
      return usage(prog);
    start = 2;
    }
  else if (!strcmp(prog, "eyedbidxcreate") || !strcmp(prog, "idxcreate"))
    mode = mCreate;
  else if (!strcmp(prog, "eyedbidxupdate") || !strcmp(prog, "idxupdate"))
    mode = mUpdate;
  else if (!strcmp(prog, "eyedbidxsimulate") || !strcmp(prog, "idxsimulate"))
    mode = mSimulate;
  else if (!strcmp(prog, "eyedbidxlist") || !strcmp(prog, "idxlist"))
    mode = mList;
  else if (!strcmp(prog, "eyedbidxstats") || !strcmp(prog, "idxstats"))
    mode = mStats;
  else if (!strcmp(prog, "eyedbidxdelete") || !strcmp(prog, "idxdelete"))
    mode = mDelete;
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
get_propag(const char *propagate, Bool &propag)
{
  if (propagate) {
    if (!*propagate || !strcasecmp(propagate, PROPAG_ON))
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

  return 0;
}

static int
make_index(const char *attrpath, const char *&type, const char *hints,
	   const char *propagate, Index *&idx)
{
  Bool propag;
  if (get_propag(propagate, propag))
    return 1;

  if (!type || !*type) {
    if (attr->isString() || attr->isIndirect() ||
	attr->getClass()->asCollectionClass()) // what about enums
      type = "hash";
    else
      type = "btree";
  }

  if (!strcmp(type, "hash")) {
    HashIndex *hidx;
    Status s = HashIndex::make(db, const_cast<Class *>(cls), attrpath,
				     propag, attr->isString(), hints, hidx);
    CHECK(s);
    idx = hidx;
  }
  else if (!strcmp(type, "btree")) {
    BTreeIndex *bidx;
    Status s = BTreeIndex::make(db, const_cast<Class *>(cls),
				      attrpath, propag, attr->isString(),
				      hints, bidx);
    CHECK(s);
    idx = bidx;
  }
  else {
    print_prog();
    fprintf(stderr, "hash or btree expected, got: %s\n", type);
    return 1;
  }

  return 0;
}

static int
get_index(const Class *cls, LinkedList &idxlist)
{
  const LinkedList *clidxlist;
  Status s = const_cast<Class *>(cls)->getAttrCompList(Class::Index_C, clidxlist);
  CHECK(s);
    
  LinkedListCursor c(clidxlist);
  void *o;
  while (c.getNext(o)) {
    idxlist.insertObject(o);
  }
  return 0;
}

static int
get_index(const char *info, LinkedList &idxlist)
{
  Status s;
  Bool all = False;

  if (info) {
    Status s;
    if (strchr(info, '.')) {
      s = Attribute::checkAttrPath(db->getSchema(), cls, attr, info);
      CHECK(s);
      Index *idx;
      s = Attribute::getIndex(db, info, idx);
      CHECK(s);
      if (idx) {
	idxlist.insertObject(idx);
	return 0;
      }
      print_prog();
      fprintf(stderr, "index '%s' not found\n", info);
      return 1;
    }
    else if (!strcmp(info, "--all"))
      all = True;
    else {
      const Class *cls = db->getSchema()->getClass(info);
      if (!cls) {
	print_prog();
	fprintf(stderr, "class '%s' not found\n", info);
	return 1;
      }
      
      return get_index(cls, idxlist);
    }
  }

  LinkedListCursor c(db->getSchema()->getClassList());
  const Class *cls;
  while (c.getNext((void *&)cls)) {
    if (!cls->isSystem() || all)
      if (get_index(cls, idxlist))
	return 1;
  }
  
  return 0;
}

static void
index_trace(Index *idx, Bool full)
{
  if (!full) {
    printf("%sindex on %s\n", idx->asHashIndex() ? "hash" : "btree",
	   idx->getAttrpath().c_str());
    return;
  }

  printf("Index on %s:\n", idx->getAttrpath().c_str());
  printf("  Propagation: %s\n", idx->getPropagate() ? "on" : "off");
  Bool isnull;
  const Dataspace *dataspace = 0;
  idx->makeDataspace(db, dataspace);
  if (dataspace)
    printf("  Dataspace: %s #%d\n", dataspace->getName(), dataspace->getId());

  if (idx->asHashIndex()) {
    HashIndex *hidx = idx->asHashIndex();
    printf("  Type: Hash\n");
    printf("  Key count: %d\n", hidx->getKeyCount());
    if (hidx->getHashMethod())
      printf("  Hash method: %s::%s;",
	     hidx->getHashMethod()->getClassOwner()->getName(),
	     hidx->getHashMethod()->getEx()->getExname().c_str());
    int cnt = idx->getImplHintsCount();
    for (int n = 0; n < cnt; n++)
      if (idx->getImplHints(n))
	printf("  %s: %d\n", IndexImpl::hashHintToStr(n, True),
	       idx->getImplHints(n));
  }
  else {
    BTreeIndex *bidx = idx->asBTreeIndex();
    printf("  Type: BTree\n");
    printf("  Degree: %d\n", bidx->getDegree());
  }
}

static int
index_create_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  int error = 0;
  Bool check;
  int start;
  if (argc > 1 && !strcmp(argv[1], "--check")) {
    check = True;
    start = 2;
  }
  else {
    check = False;
    start = 1;
  }

  if (start >= argc) return usage(prog);

  for (int n = start; n < argc; n += 4) {
    //    if (argc - n < 2) return usage(prog);
    if (argc - n < 1) return usage(prog);
    if (!check && n != start)
      db->transactionBegin();
    Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr,
					      argv[n]);
    if (s && check) {
      CHECK_W(s);
      error++;
      continue;
    }
    else
      CHECK(s);

    Index *idx;

    const char *type = argc > n+1 ? argv[n+1] : 0;

    if (make_index(argv[n], type,
		   argc > n+2 ? argv[n+2] : 0,
		   argc > n+3 ? argv[n+3] : 0, idx)) {
      if (!check)
	return 1;
      error++;
    }

    if (!idx) continue;

    if (check)
      printf("%sindex on %s\n", type, argv[n]);
    else {
      printf("Creating %sindex on %s\n", type, argv[n]);
      s = idx->store();
      CHECK(s);
      db->transactionCommit();
    }
  }

  return error;
}

static int
index_update_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;

  Bool check;
  int start;
  if (argc > 1 && !strcmp(argv[1], "--check")) {
    check = True;
    start = 2;
  }
  else {
    check = False;
    start = 1;
  }

  if (start >= argc) return usage(prog);

  for (int n = start; n < argc; n += 4) {
    if (argc - n < 2) return usage(prog);
    if (!check && n != start)
      db->transactionBegin();

    LinkedList idxlist;
    if (get_index(argv[n], idxlist))
      return 1;

    Index *idx = (Index *)idxlist.getObject(0);
    IndexImpl::Type type;

    Bool propag;
    Bool only_propag = False;
    const char *stype = argv[n+1];

    if (!strcmp(stype, "hash"))
      type = IndexImpl::Hash;
    else if (!strcmp(stype, "btree"))
      type = IndexImpl::BTree;
    else if (!*stype)
      type = idx->asBTreeIndex() ? IndexImpl::BTree :
	IndexImpl::Hash;
    else if (!get_propag(stype, propag)) {
      only_propag = True;
      stype = idx->asBTreeIndex() ? "btree" : "hash";
    }
    else
      return usage(prog);
  
    IndexImpl *idximpl = 0;
    Status s;
    if (!only_propag) {
      const char *hints = (argc > n+2 ? argv[n+2] : "");
      s = IndexImpl::make(db, type, hints, idximpl, idx->getIsString());
      CHECK(s);

      if (argc > n+3) {
	if (get_propag(argv[n+3], propag))
	  return 1;
	else
	  propag = idx->getPropagate();
      }
    }

    if (check)
      printf("%sindex on %s\n", argv[n+1], argv[n]);
    else {
      printf("Updating %sindex on %s\n", stype, argv[n]);
      idx->setPropagate(propag);
      if (only_propag) {
	s = idx->store();
	CHECK(s);
      }
      else if ((type == IndexImpl::Hash && idx->asBTreeIndex()) ||
	       (type == IndexImpl::BTree && idx->asHashIndex())) {
	Index *idxn;
	s = idx->reimplement(*idximpl, idxn);
	CHECK(s);
      }
      else {
	s = idx->setImplementation(idximpl);
	CHECK(s);
	s = idx->store();
	CHECK(s);
      }
    }
    
    db->transactionCommit();
  }

  return 0;
}

static int
index_list_realize(int argc, char *argv[])
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

  if (argc == start)
    argc++;

  int nn = 0;
  for (int n = start; n < argc; n++) {
    const char *info = argv[n];
    LinkedList idxlist;
    if (get_index(info, idxlist))
      return 1;

    LinkedListCursor c(idxlist);
    Index *idx;
    for (; c.getNext((void *&)idx); nn++) {
      if (nn && full)
	fprintf(stdout, "\n");
      index_trace(idx, full);
    }
  }

  return 0;
}

static const char fmt_opt[] = "--fmt=";

static int
index_simulate_realize(int argc, char *argv[])
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
  /*
  if (argc > start+1 && !strcmp(argv[start], "-fmt")) {
    fmt = argv[start+1];
    start += 2;
  }
  */
  else {
    fmt = 0;
  }

  for (int n = start; n < argc; n += 3) {
    if (argc - n < 2) return usage(prog);
    LinkedList idxlist;
    if (get_index(argv[n], idxlist))
      return 1;

    IndexImpl::Type type;
    const char *stype = argv[n+1];
    if (!strcmp(stype, "hash"))
      type = IndexImpl::Hash;
    else if (!strcmp(stype, "btree"))
      type = IndexImpl::BTree;
    else
      return usage(prog);

    Index *idx = (Index *)idxlist.getObject(0);
    IndexImpl *idximpl;
    const char *hints = (argc > n+2 ? argv[n+2] : "");
    Status s = IndexImpl::make(db, type, hints, idximpl,
				     idx->getIsString());
    CHECK(s);

    if (fmt && idximpl->getType() == IndexImpl::Hash) {
      IndexStats *stats;
      s = idx->simulate(*idximpl, stats);
      CHECK(s);
      s = stats->asHashIndexStats()->printEntries(fmt);
      CHECK(s);
      delete stats;
    }
    else {
      std::string stats;
      s = idx->simulate(*idximpl, stats, True, full, "  ");
      CHECK(s);
      printf("Index on %s:\n", idx->getAttrpath().c_str());
      fprintf(stdout, stats.c_str());
    }

  }

  return 0;
}

static int
index_stats_realize(int argc, char *argv[])
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
  /*
  if (argc > start+1 && !strcmp(argv[start], "-fmt")) {
    fmt = argv[start+1];
    start += 2;
  }
  */
  else {
    fmt = 0;
  }

  if (argc == start)
    argc++;

  int nn = 0;
  for (int n = start; n < argc; n++) {
    const char *info = argv[n];
    LinkedList idxlist;
    if (get_index(info, idxlist))
      return 1;

    LinkedListCursor c(idxlist);
    Index *idx;
    for (; c.getNext((void *&)idx); nn++) {
      if (fmt && idx->asHashIndex()) {
	IndexStats *stats;
	Status s = idx->getStats(stats);
	CHECK(s);
	if (nn)
	  fprintf(stdout, "\n");
	s = stats->asHashIndexStats()->printEntries(fmt);
	delete stats;
	CHECK(s);
      }
      else {
	std::string stats;
	Status s = idx->getStats(stats, False, full, "    ");
	CHECK(s);
	if (nn)
	  fprintf(stdout, "\n");
	index_trace(idx, True);
	fprintf(stdout, "  Statistics:\n");
	fprintf(stdout, stats.c_str());
      }
    }
  }

  return 0;
}

static int
index_delete_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc < 2) return usage(prog);

  for (int n = 1; n < argc; n++) {
    if (n != 1)
      db->transactionBegin();

    Status s = Attribute::checkAttrPath(db->getSchema(), cls, attr,
					      argv[n]);
    CHECK(s);

    Index *idx;
    s = Attribute::getIndex(db, argv[n], idx);
    CHECK(s);
    
    if (!idx) {
      print_prog();
      fprintf(stderr, "index '%s' not found\n", argv[n]);
      return 1;
    }

    printf("Deleting index %s\n", idx->getAttrpath().c_str());
    s = idx->remove();
    CHECK(s);
    
    db->transactionCommit();
  }

  return 0;
}

static void sig_h(int sig)
{
  signal(SIGINT, sig_h);
  if (conn) {
    printf("Interrupted!\n");
    conn->sendInterrupt();  
  }
  exit(0);
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

  if (start < argc &&
      (!strcmp(argv[start], "--help") || !strcmp(argv[start], "-h"))) {
    usage(prog);
    return 0;
  }

  if (argc < 2)
    return usage(prog);

  eyedb::init(argc, argv);

  signal(SIGINT, sig_h);

  switch(mode) {

  case mCreate:
    return index_create_realize(argc - start, &argv[start]);

  case mUpdate:
    return index_update_realize(argc - start, &argv[start]);

  case mList:
    return index_list_realize(argc - start, &argv[start]);

  case mStats:
    return index_stats_realize(argc - start, &argv[start]);

  case mSimulate:
    return index_simulate_realize(argc - start, &argv[start]);

  case mDelete:
    return index_delete_realize(argc - start, &argv[start]);

  default:
    return usage(prog);
  }
}
