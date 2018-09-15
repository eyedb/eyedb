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
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace eyedb;
using namespace std;

#define mWRITE 0x1000
#define mREAD  0x2000

enum mMode {
  mGetDefDsp = mREAD | 1,
  mSetDefDsp = mWRITE | 2,

  mGetDefIdxDsp = mREAD | 3,
  mSetDefIdxDsp = mWRITE | 4,

  mGetDefInstDsp = mREAD | 5,
  mSetDefInstDsp = mWRITE | 6,

  mGetDefAttrDsp = mREAD | 7,
  mSetDefAttrDsp = mWRITE | 8,

  mGetDefCollDsp = mREAD | 9,
  mSetDefCollDsp = mWRITE | 10,

  mGetIdxLoca = mREAD | 11,
  mMvIdx = mWRITE | 12,

  mGetInstLoca = mREAD | 13,
  mMvInst = mWRITE | 14,

  mGetObjLoca = mREAD | 15,
  mMvObj = mWRITE | 16,

  mGetAttrLoca = mREAD | 17,
  mMvAttr = mWRITE | 18,

  mGetCollLoca = mREAD | 19,
  mMvColl = mWRITE | 20
};

static const unsigned int LocaOpt = 1;
static const unsigned int StatsOpt = 2;

static const char *prog;
static mMode mode;
static Bool complete = False;
static Bool eyedb_prefix = False;
static Connection *conn;
static Database *db;
static int local = (getenv("EYEDBLOCAL") ? _DBOpenLocal : 0);

#define PROG "eyedbloca"

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
    fprintf(pipe, "usage: eyedb");
  else
    fprintf(pipe, "usage: ");

  if (!mode || mode == mGetDefDsp)
    fprintf(pipe,
	    "%sgetdefdsp <dbname>\n",
	    str());

  if (!mode || mode == mSetDefDsp)
    fprintf(pipe,
	    "%ssetdefdsp <dbname> <dataspace>\n",
	    str());

  if (!mode || mode == mGetDefIdxDsp)
    fprintf(pipe,
	    "%sgetdefidxdsp <dbname> <idx name>\n", str());

  if (!mode || mode == mSetDefIdxDsp)
    fprintf(pipe,
	    "%ssetdefidxdsp <dbname> <idx name> <dest dataspace>\n",
	    str());

  if (!mode || mode == mGetDefInstDsp)
    fprintf(pipe,
	    "%sgetdefinstdsp <dbname> <class name>\n",
	    str());

  if (!mode || mode == mSetDefInstDsp)
    fprintf(pipe,
	    "%ssetdefinstdsp <dbname> <class name> <dest dataspace>\n",
	    str());
 
  if (!mode || mode == mGetDefAttrDsp)
    fprintf(pipe,
	    "%sgetdefattrdsp <dbname> <class>::<attr>\n",
	    str());

  if (!mode || mode == mSetDefAttrDsp)
    fprintf(pipe,
	    "%ssetdefattrdsp <dbname> <class>::<attr> <dest dataspace>\n",
	    str());
  
  if (!mode || mode == mGetDefCollDsp)
    fprintf(pipe,
	    "%sgetdefcolldsp <dbname> <name>|<oid>\n",
	    str());

  if (!mode || mode == mSetDefCollDsp)
    fprintf(pipe,
	    "%ssetdefcolldsp <dbname> <name>|<oid> <dest dataspace>\n",
	    str());
  
  if (!mode || mode == mGetIdxLoca)
    fprintf(pipe,
	    "%sgetidxloca --stats|--loca|--all <dbname> <idx name>\n",
	    str());

  if (!mode || mode == mMvIdx)
    fprintf(pipe,
	    "%smvidx <dbname> <idx name> <dest dataspace>\n",
	    str());

  if (!mode || mode == mGetInstLoca)
    fprintf(pipe,
	    "%sgetinstloca --stats|--loca|--all [--subclasses] <dbname> <class name>\n",
	    str());

  if (!mode || mode == mMvInst)
    fprintf(pipe,
	    "%smvinst <dbname> [--subclasses] <class name> <dest dataspace>\n",
	    str());

  if (!mode || mode == mGetObjLoca)
    fprintf(pipe,
	    "%sgetobjloca --stats|--loca|--all <dbname> <oql construct>\n",
	    str());

  if (!mode || mode == mMvObj)
    fprintf(pipe,
	    "%smvobj <dbname> <oql construct> <dest dataspace>\n",
	    str());

  if (!mode || mode == mGetAttrLoca)
    fprintf(pipe,
	    "%sgetattrloca <dbname> --stats|--loca|--all <class>::<attr>\n",
	    str());

  if (!mode || mode == mMvAttr)
    fprintf(pipe,
	    "%smvattr <dbname> <class>::<attr> <dest dataspace>\n",
	    str());

  if (!mode || mode == mGetCollLoca)
    fprintf(pipe,
	    "%sgetcollloca <dbname> --stats|--loca|--all <name>|<oid>\n",
	    str());

  if (!mode || mode == mMvColl)
    fprintf(pipe,
	    "%smvattr <dbname> <name>|<oid> <dest dataspace>\n",
	    str());

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

  if (!strcmp(prog, PROG))
    {
      complete = True;

      if (argc < 2)
	return usage(prog);

      const char *cmd = argv[1];

      if (!strcmp(cmd, "getdefidxdsp"))
	mode = mGetDefIdxDsp;
      else if (!strcmp(cmd, "setdefidxdsp"))
	mode = mSetDefIdxDsp;
      else if (!strcmp(cmd, "getdefinstdsp"))
	mode = mGetDefInstDsp;
      else if (!strcmp(cmd, "setdefinstdsp"))
	mode = mSetDefInstDsp;
      else if (!strcmp(cmd, "getidxloca"))
	mode = mGetIdxLoca;
      else if (!strcmp(cmd, "mvidx"))
	mode = mMvIdx;
      else if (!strcmp(cmd, "getinstloca"))
	mode = mGetInstLoca;
      else if (!strcmp(cmd, "mvinst"))
	mode = mMvInst;
      else if (!strcmp(cmd, "getobjloca"))
	mode = mGetObjLoca;
      else if (!strcmp(cmd, "mvobj"))
	mode = mMvObj;
      else if (!strcmp(cmd, "getdefattrdsp"))
	mode = mGetDefAttrDsp;
      else if (!strcmp(cmd, "setdefattrdsp"))
	mode = mSetDefAttrDsp;
      else if (!strcmp(cmd, "getdefcolldsp"))
	mode = mGetDefCollDsp;
      else if (!strcmp(cmd, "setdefcolldsp"))
	mode = mSetDefCollDsp;
      else if (!strcmp(cmd, "getattrloca"))
	mode = mGetAttrLoca;
      else if (!strcmp(cmd, "mvattr"))
	mode = mMvAttr;
      else if (!strcmp(cmd, "getcollloca"))
	mode = mGetAttrLoca;
      else if (!strcmp(cmd, "mvcoll"))
	mode = mMvAttr;
      else if (!strcmp(cmd, "getdefdsp"))
	mode = mGetDefDsp;
      else if (!strcmp(cmd, "setdefdsp"))
	mode = mSetDefDsp;
      else if (!strcmp(cmd, "getdefidxdsp"))
	mode = mGetDefIdxDsp;
      else
	return usage(prog);
      start = 2;
    }
  else if (!strcmp(prog, "eyedbgetdefidxdsp") || !strcmp(prog, "getdefidxdsp"))
    mode = mGetDefIdxDsp;
  else if (!strcmp(prog, "eyedbsetdefidxdsp") || !strcmp(prog, "setdefidxdsp"))
    mode = mSetDefIdxDsp;
  else if (!strcmp(prog, "eyedbgetdefinstdsp") || !strcmp(prog, "getdefinstdsp"))
    mode = mGetDefInstDsp;
  else if (!strcmp(prog, "eyedbsetdefinstdsp") || !strcmp(prog, "setdefinstdsp"))
    mode = mSetDefInstDsp;
  else if (!strcmp(prog, "eyedbgetidxloca") || !strcmp(prog, "getidxloca"))
    mode = mGetIdxLoca;
  else if (!strcmp(prog, "eyedbmvidx") || !strcmp(prog, "mvidx"))
    mode = mMvIdx;
  else if (!strcmp(prog, "eyedbgetinstloca") || !strcmp(prog, "getinstloca"))
    mode = mGetInstLoca;
  else if (!strcmp(prog, "eyedbmvinst") || !strcmp(prog, "mvinst"))
    mode = mMvInst;
  else if (!strcmp(prog, "eyedbgetobjloca") || !strcmp(prog, "getobjloca"))
    mode = mGetObjLoca;
  else if (!strcmp(prog, "eyedbmvobj") || !strcmp(prog, "mvobj"))
    mode = mMvObj;
  else if (!strcmp(prog, "eyedbgetdefattrdsp") || !strcmp(prog, "getdefattrdsp"))
    mode = mGetDefAttrDsp;
  else if (!strcmp(prog, "eyedbsetdefattrdsp") || !strcmp(prog, "setdefattrdsp"))
    mode = mSetDefAttrDsp;
  else if (!strcmp(prog, "eyedbgetdefcolldsp") || !strcmp(prog, "getdefcolldsp"))
    mode = mGetDefCollDsp;
  else if (!strcmp(prog, "eyedbsetdefcolldsp") || !strcmp(prog, "setdefcolldsp"))
    mode = mSetDefCollDsp;
  else if (!strcmp(prog, "eyedbgetattrloca") || !strcmp(prog, "getattrloca"))
    mode = mGetAttrLoca;
  else if (!strcmp(prog, "eyedbmvattr") || !strcmp(prog, "mvattr"))
    mode = mMvAttr;
  else if (!strcmp(prog, "eyedbgetcollloca") || !strcmp(prog, "getcollloca"))
    mode = mGetCollLoca;
  else if (!strcmp(prog, "eyedbmvcoll") || !strcmp(prog, "mvcoll"))
    mode = mMvColl;
  else if (!strcmp(prog, "eyedbgetdefdsp") || !strcmp(prog, "getdefdsp"))
    mode = mGetDefDsp;
  else if (!strcmp(prog, "eyedbsetdefdsp") || !strcmp(prog, "setdefdsp"))
    mode = mSetDefDsp;
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

/*
static Status
x_transaction_begin(Database *db)
{
  TransactionParams params = Database::getGlobalDefaultTransactionParams();
  params.lockmode = DatabaseX;
  params.wait_timeout = 1;

  Status s = db->transactionBegin(params);
  if (s) {
    if (s->getStatus() == SE_LOCK_TIMEOUT) {
      fprintf(stderr,
	      "cannot acquire exclusive lock on database %s\n",
	      db->getName());
      exit(1);
    }
  }

  return s;
}
*/

static int
init(int argc, char *argv[])
{
  if (!argc) return usage(prog);
  conn = new Connection();
  Status s = conn->open();
  if (s)
    {
      print_prog();
      s->print();
      return 1;
    }

  int i;

  for (i = 0; i < argc; i++)
    if (*argv[i] != '-') {
      db = new Database(argv[i]);
      break;
    }

  if (i == argc) return usage(prog);

  s = db->open(conn, (Database::OpenFlag)((mode & mWRITE) ? Database::DBRW|local : Database::DBSRead|local));
  if (s)
    {
      print_prog();
      s->print();
      return 1;
    }

  if (mode == mSetDefDsp)
    db->transactionBeginExclusive();
  else
    db->transactionBegin();

  return 0;
} 

const char *
get_op(const char *s, int &offset)
{
  if (s[0] == '~')
    {
      if (s[1] == '~')
	{
	  offset = 2;
	  return "~~";
	}

      offset = 1;
      return "~";
    }

  if (s[0] == '!')
    {
      if (s[1] == '~')
	{
	  if (s[2] == '~')
	    {
	      offset = 3;
	      return "!~~";
	    }

	  offset = 2;
	  return "!~";
	}
    }

  offset = 0;
  return "=";
}

#define CHECK(S) \
  if (S) { \
   print_prog(); \
   (S)->print(); \
   return 1; \
   }

static int
get_idxs(const char *attrpath, ObjectArray &obj_arr)
{
  int offset;
  const char *op = get_op(attrpath, offset);
  OQL q(db, "select index.attrpath %s \"%s\"", op, &attrpath[offset]);
  Status s = q.execute(obj_arr);
  CHECK(s);

  if (!obj_arr.getCount()) {
    print_prog();
    fprintf(stderr, "no index %s found\n", attrpath);
    return 1;
  }

  return 0;
}

static int
get_cls(const char *name, ObjectArray &obj_arr)
{
  int offset;
  const char *op = get_op(name, offset);
  OQL q(db, "select class.name %s \"%s\"", op, &name[offset]);
  Status s = q.execute(obj_arr);
  CHECK(s);

  if (!obj_arr.getCount()) {
    print_prog();
    fprintf(stderr, "no class %s found\n", name);
    return 1;
  }

  return 0;
}

static int
get_attr(const char *attrpath, Attribute *&attr)
{
  const char *r = strchr(attrpath, ':');
  if (!r || r[1] != ':') {
    print_prog();
    fprintf(stderr, "attribute must be under the form classname::attrpath\n");
    return 1;
  }

  char *classname = new char[r-attrpath+1];
  strncpy(classname, attrpath, r-attrpath);
  classname[r-attrpath] = 0;
  const Class *cls = db->getSchema()->getClass(classname);
  if (!cls) {
    print_prog();
    fprintf(stderr, "class %s not found\n", classname);
    return 1;
  }

  attr = (Attribute *)cls->getAttribute(r+2);
  if (!attr) {
    print_prog();
    fprintf(stderr, "attribute %s::%s not found\n", classname, r+2);
    return 1;
  }

  return 0;
}

static int
get_colls(const char *name, ObjectArray &obj_arr)
{
  Oid oid(name);
  if (oid.isValid()) {
    Object *o;
    Status s = db->loadObject(oid, o);
    CHECK(s);
    if (!o->asCollection()) {
      print_prog();
      fprintf(stderr, "%s is not a collection\n", oid.toString());
      return 1;
    }
    Object **xo = new Object *[1];
    xo[0] = o;
    obj_arr.set(xo, 1);
    return 0;
  }

  int offset;
  const char *op = get_op(name, offset);
  OQL q(db, "select collection.name %s \"%s\"", op, &name[offset]);
  Status s = q.execute(obj_arr);
  CHECK(s);

  if (!obj_arr.getCount()) {
    print_prog();
    fprintf(stderr, "no collection %s found\n", name);
    return 1;
  }

  return 0;
}

static int
get_objs(const char *oql, OidArray &oid_arr)
{
  OQL q(db, oql);
  Status s = q.execute(oid_arr);
  CHECK(s);
  return 0;
}

static void
print(const Dataspace *dataspace, Bool def = False)
{
  if (!dataspace) {
    Status s = db->getDefaultDataspace(dataspace);
    if (s) {print_prog(); s->print(); return;}
    //    printf("Default ");
    print(dataspace, True);
    return;
  }
  printf("  Dataspace #%d", dataspace->getId());
  if (def) printf(" (default)");
  printf("\n");
  printf("   Name %s\n", dataspace->getName());
  unsigned int datafile_cnt;
  const Datafile **datafiles = dataspace->getDatafiles(datafile_cnt);
  printf("   Composed of {\n", datafile_cnt);
  for (int i = 0; i < datafile_cnt; i++) {
    printf("     Datafile #%d\n", datafiles[i]->getId());
    if (*datafiles[i]->getName())
      printf("       Name %s\n", datafiles[i]->getName());
    printf("       File %s\n", datafiles[i]->getFile());
  }
  printf("   }\n");
}

static int
getdefdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 1) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDefaultDataspace(dataspace);
  CHECK(s);

  printf("Default dataspace:\n");
  print(dataspace);

  return 0;
}

static int
setdefdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 2) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[1], dataspace);
  CHECK(s);

  s = db->setDefaultDataspace(dataspace);
  CHECK(s);

  return 0;
}

static int
getdefidxdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 2) return usage(prog);

  ObjectArray obj_arr;
  if (get_idxs(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = idx->getDefaultDataspace(dataspace);
    CHECK(s);
    if (i) printf("\n");
    printf("Default dataspace for index '%s':\n", idx->getAttrpath().c_str());
    print(dataspace);
  }

  db->transactionCommit();
  return 0;
}

static int
setdefidxdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  ObjectArray obj_arr;
  if (get_idxs(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    Status s = idx->setDefaultDataspace(dataspace);
    CHECK(s);
  }

  db->transactionCommit();
  return 0;
}

static int
getdefinstdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 1 && argc != 2) return usage(prog);

  ObjectArray obj_arr;
  const char *str = (argc == 2 ? argv[1] : "~");
  if (get_cls(str, obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = cls->getDefaultInstanceDataspace(dataspace);
    CHECK(s);
    if (i) printf("\n");
    printf("Default dataspace for instances of class '%s':\n", cls->getName());
    print(dataspace);
  }

  db->transactionCommit();
  return 0;
}

static int
setdefinstdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  ObjectArray obj_arr;
  if (get_cls(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    Status s = cls->setDefaultInstanceDataspace(dataspace);
    CHECK(s);
  }

  db->transactionCommit();
  return 0;
}

static int
getdefattrdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 2) return usage(prog);

  Attribute *attr;
  if (get_attr(argv[1], attr)) return 1;

  const Dataspace *dataspace;
  Status s = attr->getDefaultDataspace(dataspace);
  CHECK(s);
  printf("Default dataspace for attribute '%s':\n", argv[1]);
  print(dataspace);
  db->transactionCommit();
  return 0;
}

static int
setdefattrdsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  Attribute *attr;
  if (get_attr(argv[1], attr)) return 1;

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  s = attr->setDefaultDataspace(dataspace);
  CHECK(s);

  db->transactionCommit();
  return 0;
}

static int
getdefcolldsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 2) return usage(prog);

  ObjectArray obj_arr;
  if (get_colls(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    const Dataspace *dataspace;
    Status s = coll->getDefaultDataspace(dataspace);
    CHECK(s);
    if (i) printf("\n");
    printf("Default dataspace for collection ");
    if (*coll->getName()) printf("'%s' ", coll->getName());
    printf("{%s}\n", coll->getOid().toString());
    print(dataspace);
  }

  db->transactionCommit();
  return 0;
}

static int
setdefcolldsp_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  ObjectArray obj_arr;
  if (get_colls(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Collection *coll = (Collection *)obj_arr[i];
    Status s = coll->setDefaultDataspace(dataspace);
    CHECK(s);
  }

  db->transactionCommit();
  return 0;
}

static int
getloca_opt(int argc, char *argv[], unsigned int &lopt)
{
  if (!strcmp(argv[0], "--stats")) {
    lopt = StatsOpt;
    return 0;
  }

  if (!strcmp(argv[0], "--loca")) {
    lopt = LocaOpt;
    return 0;
  }

  if (!strcmp(argv[0], "--all")) {
    lopt = StatsOpt|LocaOpt;
    return 0;
  }

  return usage(prog);
}

static int
getidxloca_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  unsigned int lopt;
  if (getloca_opt(argc, argv, lopt)) return 1;

  ObjectArray obj_arr;
  if (get_idxs(argv[2], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    ObjectLocationArray locarr;
    Status s = idx->getObjectLocations(locarr);
    CHECK(s);

    if (lopt & LocaOpt)
      cout << locarr(db) << endl;

    if (lopt & StatsOpt) {
      PageStats *pgs = locarr.computePageStats(db);
      cout << *pgs;
      delete pgs;
    }
  }

  return 0;
}

static int
mvidx_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  ObjectArray obj_arr;
  if (get_idxs(argv[1], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Index *idx = (Index *)obj_arr[i];
    Status s = idx->move(dataspace);
    CHECK(s);
  }

  db->transactionCommit();
  return 0;
}

static int
getsubcls_opt(int argc, char *argv[], int i, Bool &subclasses)
{
  if (argc == 4) {
    if (!strcmp(argv[i], "--subclasses")) {
      subclasses = True;
      return 0;
    }

    return usage(prog);
  }

  subclasses = False;
  return 0;
}

static int
getinstloca_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3 && argc != 4) return usage(prog);

  unsigned int lopt;
  if (getloca_opt(argc, argv, lopt)) return 1;

  Bool subclasses;
  if (getsubcls_opt(argc, argv, 1, subclasses)) return 1;

  ObjectArray obj_arr;
  if (get_cls(argv[subclasses ? 3 : 2], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    ObjectLocationArray locarr;
    Status s = cls->getInstanceLocations(locarr, subclasses);
    CHECK(s);
    if (lopt & LocaOpt)
      cout << locarr(db) << endl;

    if (lopt & StatsOpt) {
      PageStats *pgs = locarr.computePageStats(db);
      cout << *pgs;
      delete pgs;
    }
  }

  return 0;
}

static int
mvinst_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3 && argc != 4) return usage(prog);

  Bool subclasses;
  if (getsubcls_opt(argc, argv, 0, subclasses)) return 1;

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[(subclasses ? 3 : 2)], dataspace);
  CHECK(s);

  ObjectArray obj_arr;
  if (get_cls(argv[(subclasses ? 2 : 1)], obj_arr)) return 1;

  for (int i = 0; i < obj_arr.getCount(); i++) {
    Class *cls = (Class *)obj_arr[i];
    Status s = cls->moveInstances(dataspace, subclasses);
    CHECK(s);
  }

  db->transactionCommit();
  return 0;
}

static int
getobjloca_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  unsigned int lopt;
  if (getloca_opt(argc, argv, lopt)) return 1;

  OidArray oid_arr;
  if (get_objs(argv[2], oid_arr)) return 1;

  ObjectLocationArray locarr;
  Status s = db->getObjectLocations(oid_arr, locarr);
  CHECK(s);

  if (lopt & LocaOpt)
    cout << locarr(db) << endl;

  if (lopt & StatsOpt) {
    PageStats *pgs = locarr.computePageStats(db);
    cout << *pgs;
    delete pgs;
  }

  return 0;
}

static int
mvobj_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  if (argc != 3) return usage(prog);

  const Dataspace *dataspace;
  Status s = db->getDataspace(argv[2], dataspace);
  CHECK(s);

  OidArray oid_arr;
  if (get_objs(argv[1], oid_arr)) return 1;

  s = db->moveObjects(oid_arr, dataspace);
  CHECK(s);

  db->transactionCommit();
  return 0;
}

static int
getattrloca_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  return usage(prog);
}

static int
mvattr_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  return usage(prog);
}

static int
getcollloca_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  return usage(prog);
}

static int
mvcoll_realize(int argc, char *argv[])
{
  if (::init(argc, argv)) return 1;
  return usage(prog);
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

  if (start < argc && (!strcmp(argv[start], "--help")
		       || !strcmp(argv[start], "-h"))) {
    usage(prog);
    return 0;
  }

  eyedb::init(argc, argv);

  switch(mode) {

  case mGetDefDsp:
    return getdefdsp_realize(argc - start, &argv[start]);

  case mSetDefDsp:
    return setdefdsp_realize(argc - start, &argv[start]);

  case mGetDefIdxDsp:
    return getdefidxdsp_realize(argc - start, &argv[start]);

  case mSetDefIdxDsp:
    return setdefidxdsp_realize(argc - start, &argv[start]);

  case mGetDefInstDsp:
    return getdefinstdsp_realize(argc - start, &argv[start]);

  case mSetDefInstDsp:
    return setdefinstdsp_realize(argc - start, &argv[start]);

  case mGetIdxLoca:
    return getidxloca_realize(argc - start, &argv[start]);

  case mMvIdx:
    return mvidx_realize(argc - start, &argv[start]);

  case mGetInstLoca:
    return getinstloca_realize(argc - start, &argv[start]);

  case mMvInst:
    return mvinst_realize(argc - start, &argv[start]);

  case mGetObjLoca:
    return getobjloca_realize(argc - start, &argv[start]);

  case mMvObj:
    return mvobj_realize(argc - start, &argv[start]);

  case mGetDefAttrDsp:
    return getdefattrdsp_realize(argc - start, &argv[start]);

  case mSetDefAttrDsp:
    return setdefattrdsp_realize(argc - start, &argv[start]);

  case mGetDefCollDsp:
    return getdefcolldsp_realize(argc - start, &argv[start]);

  case mSetDefCollDsp:
    return setdefcolldsp_realize(argc - start, &argv[start]);

  case mGetAttrLoca:
    return getattrloca_realize(argc - start, &argv[start]);

  case mMvAttr:
    return mvattr_realize(argc - start, &argv[start]);

  case mGetCollLoca:
    return getcollloca_realize(argc - start, &argv[start]);

  case mMvColl:
    return mvcoll_realize(argc - start, &argv[start]);

  default:
    return usage(prog);
  }
}
