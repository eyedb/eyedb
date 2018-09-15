
//#define protected public
//#define private public
#include <iostream>
#include <vector>
#include <eyedb/eyedb.h>
#include <eyedblib/strutils.h>
#include <eyedb/internals/ObjectPeer.h>
#include <eyedb/internals/kern_const.h>

//#include <execinfo.h>

#include "schema.h"

using namespace std;
using namespace eyedb;

//#define COLLECTION CollSet
//#define COLLECTION CollBag
#define COLLECTION CollArray
#define COLLECTION_ARRAY
#define COLLECTION_ARRAY_2

void nop()
{
}

void ppp()
{
  abort();
  /*
  if (Exception::getMode() == Exception::StatusMode)
    fprintf(stderr, "exit(int) function has been called : getting a chance to trap this call\n");
  //sleep(10000);
  std::set_unexpected(nop);
  std::set_terminate(nop);
  Exception::setMode(Exception::ExceptionMode);
  Exception::make(IDB_EXIT_CALLED, "invalid called");
  */
}

namespace eyedb {
  extern void idb_dump_data(Data data, Size size);
}

static void
perform_leaks(Database &db)
{
  printf("PERFORM_leaks\n");
  Class *cls = db.getSchema()->getClass("Person");

  /*
  for (int n = 0; n < 1000; n++) {
    COLLECTION *coll = new COLLECTION(&db, "", cls, True);
    cout << "sbrk1: " << sbrk(0) << endl;
    Person *p = new Person(&db);

    p->setName("john");
    p->setAge(10);
    p->setIi(0, 200);
    p->setIi(1, 100);
    p->setIi(2, 300);
    p->setJj(10002);
    p->store();

#ifdef COLLECTION_ARRAY
    coll->insertAt(n, p);
#else
    coll->insert(p, False);
#endif
    
    coll->store();

    p->release();
    coll->release();
  }
  */

  COLLECTION *coll = new COLLECTION(&db, "", cls, True);

  for (int n = 0; n < 100; n++) {
    Person *p = new Person(&db);

    p->setName("john");
    p->setAge(10);
    p->setIi(0, 200);
    p->setIi(1, 100);
    p->setIi(2, 300);
    p->setJj(10002);
    p->store();
  
#ifdef COLLECTION_ARRAY
    coll->insertAt(n, p);
#else
    coll->insert(p, False);
#endif
    p->release();
  }

  coll->store();

  db.transactionCommit();

  /*
  void * array[25];
  int nSize = backtrace(array, 25);
  char ** symbols = backtrace_symbols(array, nSize);
 
  for (int n = 0; n < nSize; n++) {
    cout << "stack: " << array[n] << ": " << symbols[n] << endl;
  }

  free(symbols);
  return;
  */

  /*
  for (int n = 0; n < 1000; n++) {
    cout << "sbrk2: " << sbrk(0) << endl;
    ValueArray val_arr;
    coll->getElements(val_arr);
  }

  for (int n = 0; n < 1000; n++) {
    cout << "sbrk3: " << sbrk(0) << endl;
    CollectionIterator citer(coll, True);
    Value v;
    int j = 0;
    while (citer.next(v))
      j++;
    cout << "found: " << j << endl;
  }
  */

  for (int n = 0; n < 1000; n++) {
    db.transactionBegin();
    cout << "sbrk4: " << sbrk(0) << endl;
    /*
    cout << "obj_cnt: " << gbxObject::getObjectCount() << " " <<
      gbxObject::getObjMap()->size() << endl;
    //    getchar();
    */
    CollectionIterator citer(coll, True);
    Object *o;
    int j = 0;
    while (citer.next(o)) {
      o->release();
      //cout << "object_count: " << gbxObject::getObjectCount() << endl;
      //cout << "heap_size: " << gbxObject::getHeapSize() << endl;

      j++;
    }
    cout << "found: " << j << endl;
    db.transactionCommit();
  }

  db.transactionBegin();
}

static void
perform_list(Database &db)
{
  printf("PERFORM_list\n");
  Class *cls = db.getSchema()->getClass("Person");
  Size psize, vsize, isize;
  Size asize = cls->getIDRObjectSize(&psize, &vsize, &isize);
  printf("IDRSIZE: %d %d %d %d %d\n", asize, psize, vsize, isize,
	 psize - IDB_OBJ_HEAD_SIZE);
  CollList *coll = new CollList(&db, "", cls, True);

  //printf("ITEM_SIZE %d\n", coll->item_size);
  if (coll->getStatus())
    cerr << "oups 1: " << coll->getStatus() << endl;
  
  vector<Person *> v;
  vector<Collection::ItemId> v_id;
  for (int ii = 0; ii < 10; ii++) {
    Person *p = new Person(&db);
    v.push_back(p);
    p->setName(("john " + str_convert(ii)).c_str());
    p->setAge(10 + ii);
    p->setIi(0, 200 + ii);
    p->setIi(1, 100 + ii);
    p->setIi(2, 300 + ii);
    p->setJj(10002 + ii);
    p->store();
    Collection::ItemId id;
    coll->insertLast(p, &id);
    v_id.push_back(id);
  }

  Person *p = new Person(&db);
  p->setName("mary1");
  p->setAge(10);
  p->setIi(0, 200);
  p->setIi(1, 100);
  p->setIi(2, 300);
  p->setJj(10002);
  p->store();

  coll->insertBefore(v_id[0], p);

  p = new Person(&db);
  p->setName("mary2");
  p->setAge(10);
  p->setIi(0, 200);
  p->setIi(1, 100);
  p->setIi(2, 300);
  p->setJj(10002);
  p->store();

  coll->insertBefore(v_id[1], p);

  coll->store();
  cout << "OID: " << coll->getOid() << endl;

  Bool found;
  coll->isIn(p, found);
  cout << "obj isin: " << found << endl;

  ObjectArray obj_arr;
  printf("object_array:\n");
  coll->getElements(obj_arr);
  for (int n = 0; n < obj_arr.getCount(); n++) {
    obj_arr[n]->trace();
    printf("database: %p\n", obj_arr[n]->getDatabase());
    printf("parent: %p\n", ((Person *)obj_arr[n])->getParent());
  }

  printf("value_array:\n");
  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    val_arr[n].o->trace();
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;
}

static void
perform_err(Database &db)
{
  printf("PERFORM_ERR\n");
  const int DIM = 13;
  Class *cls = db.getSchema()->getClass("int64");
  COLLECTION *coll = new COLLECTION(&db, "", cls, DIM);

  eyedblib::int64 n[DIM];

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 10 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, n[0]);
#else
  coll->insert(n[0], False);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 100 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(1, n[0]);
#else
  coll->insert(n[0], False);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 0;

#ifdef COLLECTION_ARRAY
  coll->insertAt(2, n[0]);
#else
  coll->insert(n[0]);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 10000 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(3, n[0]);
#else
  coll->insert(n[0], False);
#endif

  coll->store();
  cout << "OID: " << coll->getOid() << endl;

  Bool found = False;
  coll->isIn(n[0], found);
  cout << "lit isin: " << found << endl;

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;
}

static void
perform_str(Database &db)
{
  printf("PERFORM_STR\n");
  const int DIM = 30;
  Class *cls = db.getSchema()->getClass("char");
  COLLECTION *coll = new COLLECTION(&db, "", cls, DIM);

  const char *s;

  s = "hello 0";

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, s);
#else
  coll->insert(s, False); 
#endif

  s = "world 0";

#ifdef COLLECTION_ARRAY
  coll->insertAt(1, s);
#else
  coll->insert(s, False);
#endif

  s = "alors quoi, ca marche ?";
#ifdef COLLECTION_ARRAY
  coll->insertAt(2, s);
#else
  coll->insert(s, False);
#endif


  coll->store();
  cout << "OID: " << coll->getOid() << endl;

  Bool found = False;
  coll->isIn(s, found);
  cout << "str1 isin: " << found << endl;

  coll->isIn(s+1, found);
  cout << "str2 isin: " << found << endl;

#ifdef COLLECTION_ARRAY_2
  Value vv;
  coll->retrieveAt(0, vv);
  cout << "at 0: '" << vv << "'" << endl;
  coll->retrieveAt(1, vv);
  cout << "at 1: '" << vv << "'" << endl;
  coll->retrieveAt(2, vv);
  cout << "at 2: '" << vv << "'" << endl;
#endif

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;
}

static void
perform_cache(Database &db)
{
  printf("PERFORM_CACHE\n");
  Class *cls = db.getSchema()->getClass("int64");
  IndexImpl *idximpl = new IndexImpl(IndexImpl::Hash, 0, 10000);

  COLLECTION *coll = new COLLECTION(&db, "", cls, 1, new CollImpl(idximpl));

  eyedblib::int64 n;

  n = 10;

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, n);
#else
  coll->insert(n, False);
  n = 2000;
  coll->insert(n, False);
  coll->insert(n+1, False);
  coll->insert(n+2, False);
  coll->insert(n+3, False);
  coll->insert(n+4, False);
  coll->insert(n+5, False);

#endif

  n = 100;

#ifdef COLLECTION_ARRAY
  coll->insertAt(1, n);
#else
  coll->insert(n, False);
#endif

  n = 0;

#ifdef COLLECTION_ARRAY
  coll->insertAt(2, n);
#else
  coll->insert(n, False);
#endif

  n = 10000;

#ifdef COLLECTION_ARRAY
  coll->insertAt(3, n);
  for (eyedblib::int64 ii = 20; ii >= 4; ii--) {
    eyedblib::int64 jj = -ii * 10;
    coll->insertAt(ii, jj);
  }
#else
  coll->insert(n, False);
#endif

  Bool found = False;
  coll->isIn(n, found);
  cout << "cache #1 " << n << " isin: " << found << endl;

  coll->store();

  db.transactionCommit();
  db.transactionBegin();

  COLLECTION *coll1;
  db.loadObject(coll->getOid(), (Object *&)coll1);
  printf("coll %p %p\n", coll, coll1);
  coll = coll1;

#ifdef COLLECTION_ARRAY_2
  Value vv;
  coll->retrieveAt(0, vv);
  cout << "at 0: '" << vv << "'" << endl;
  coll->retrieveAt(1, vv);
  cout << "at 1: '" << vv << "'" << endl;
  coll->retrieveAt(2, vv);
  cout << "at 2: '" << vv << "'" << endl;
  coll->retrieveAt(3, vv);
  cout << "at 3: '" << vv << "'" << endl;
  /*
  coll->retrieveAt(4, (Data)nn);
  cout << "at 4: '" << nn << "'" << endl;
  */
#endif

  cout << "OID: " << coll->getOid() << endl;

  coll->isIn(n, found);
  cout << "cache #2 " << n << " isin: " << found << endl;

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;

  CollectionIterator citer(coll, True);
  Value v;
  cout << "CollectionIterator:\n";
  for (int n = 0; citer.next(v); n++) {
    if (!(n & 1)) cout << "[" << v << "] = ";
    else cout << v << endl;
  }

  CollectionIterator citer2(coll, False);
  cout << "CollectionIterator2:\n";
  for (int n = 0; citer2.next(v); n++) {
    cout << v << endl;
  }
}

static void
perform_lit(Database &db)
{
  printf("PERFORM_LIT\n");
  const int DIM = 1;
  Class *cls = db.getSchema()->getClass("int64");
  IndexImpl *idximpl = new IndexImpl(IndexImpl::Hash, 0, 10000);

  //IndexImpl *idximpl = new IndexImpl(IndexImpl::BTree);

  COLLECTION *coll = new COLLECTION(&db, "", cls, DIM, new CollImpl(idximpl));

  eyedblib::int64 n[DIM];

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 10 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, n[0]);
#else
  coll->insert(n[0], False);
#endif

  for (int ii = 0; ii < DIM; ii++)    n[ii] = 100 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(1, n[0]);
#else
  coll->insert(n[0], False);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 0;

#ifdef COLLECTION_ARRAY
  coll->insertAt(2, n[0]);
#else
  coll->insert(n[0], False);
  //eyedblib::int16 i16 = 12;
  //printf("trying to insert i16\n");
  //coll->insert(i16, False);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 10000 + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(3, n[0]);
  for (eyedblib::int64 ii = 100; ii >= 4; ii--) {
    eyedblib::int64 jj = -ii * 10;
    coll->insertAt(ii, jj);
  }
#else
  coll->insert(n[0], False);
#endif

  coll->store();

  db.transactionCommit();
  db.transactionBegin();

  COLLECTION *coll1;
  db.loadObject(coll->getOid(), (Object *&)coll1);
  printf("coll %p %p\n", coll, coll1);
  coll = coll1;

#ifdef COLLECTION_ARRAY_2
  Value vv;
  coll->retrieveAt(0, vv);
  cout << "at 0: '" << vv << "'" << endl;
  coll->retrieveAt(1, vv);
  cout << "at 1: '" << vv << "'" << endl;
  coll->retrieveAt(2, vv);
  cout << "at 2: '" << vv << "'" << endl;
  coll->retrieveAt(3, vv);
  cout << "at 3: '" << vv << "'" << endl;
  /*
  coll->retrieveAt(4, (Data)nn);
  cout << "at 4: '" << nn[0] << "'" << endl;
  */
#endif

  cout << "OID: " << coll->getOid() << endl;

  Bool found = False;
  coll->isIn(n[0], found);
  cout << "lit isin: " << found << endl;

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;

  CollectionIterator citer(coll, True);
  Value v;
  cout << "CollectionIterator:\n";
  for (int n = 0; citer.next(v); n++) {
    if (!(n & 1)) cout << "[" << v << "] = ";
    else cout << v << endl;
  }

  CollectionIterator citer2(coll, False);
  cout << "CollectionIterator2:\n";
  for (int n = 0; citer2.next(v); n++) {
    cout << v << endl;
  }
}

static void
perform_odl(Database &db)
{
  printf("PERFORM_OLD\n");
  AColl *c = new AColl(&db);

  Person *p = new Person(&db);

  p->setName("john");
  p->setAge(10);
  p->setIi(0, 200);
  p->setIi(1, 100);
  p->setIi(2, 300);
  p->setJj(10002);

  Person *pp = new Person(&db);
  pp->setName("persist");
  pp->setJj(1029340);
  pp->store();
  p->setParent(pp);

  c->addToLitCLitColl(p);
  p = new Person(&db);

  p->setName("mary");
  p->setAge(8);
  p->setIi(0, 2000);
  p->setIi(1, 1000);
  p->setIi(2, 3000);
  p->setJj(-202);

  pp = new Person(&db);
  pp->setName("persist2");
  pp->setJj(1029340);
  pp->store();
  p->setParent(pp);

  c->addToLitCLitColl(p);

  c->store();

  printf("coid: %s\n", c->getOid().toString());
  Collection *coll = c->getLitCLitColl();
  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << "#" << n << " " << val_arr[n] << endl;
  }

  CollectionIterator iter(coll);

  printf("iterator:\n");
  /*
  Object *o;
  while (iter.next(o)) {
    o->trace();
  }
  */
  Value v;
  for (int n = 0; iter.next(v); n++) {
    cout << "#" << n << " " << v << endl;
  }
}

static void
perform_obj(Database &db)
{
  printf("PERFORM_Obj\n");
  Class *cls = db.getSchema()->getClass("Person");
  Size psize, vsize, isize;
  Size asize = cls->getIDRObjectSize(&psize, &vsize, &isize);
  printf("IDRSIZE: %d %d %d %d %d\n", asize, psize, vsize, isize,
	 psize - IDB_OBJ_HEAD_SIZE);
  COLLECTION *coll = new COLLECTION(&db, "", cls, 1 /*cls->getIDRObjectSize()*/);

  //printf("ITEM_SIZE %d\n", coll->item_size);
  if (coll->getStatus())
    cerr << "oups 1: " << coll->getStatus() << endl;
  
  Person *p = new Person(&db);

  p->setName("john");
  p->setAge(10);
  p->setIi(0, 200);
  p->setIi(1, 100);
  p->setIi(2, 300);
  p->setJj(10002);

  Person *pp = new Person(&db);
  pp->setName("persist");
  pp->setJj(1029340);
  pp->store();
  p->setParent(pp);

  //coll->insert(p->getIDR()+IDB_OBJ_HEAD_SIZE, False);
  //coll->insert(p->getIDR(), False);

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, p);
  coll->insertAt(1, pp);
#else
  coll->insert(p, False);
  coll->insert(pp, False);
#endif

  coll->store();
  cout << "OID: " << coll->getOid() << endl;

  Bool found = False;
  coll->isIn(p, found);
  cout << "obj isin: " << found << endl;

  ObjectArray obj_arr;
  printf("object_array:\n");
  coll->getElements(obj_arr);
  for (int n = 0; n < obj_arr.getCount(); n++) {
    obj_arr[n]->trace();
    printf("database: %p\n", obj_arr[n]->getDatabase());
    printf("parent: %p\n", ((Person *)obj_arr[n])->getParent());
  }

  printf("value_array:\n");
  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    val_arr[n].o->trace();
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;
}

static void
perform_char(Database &db)
{
  printf("PERFORM_char\n");
  const int DIM = 1;
  Class *cls = db.getSchema()->getClass("char");
  COLLECTION *coll = new COLLECTION(&db, "", cls, DIM);

  char n[DIM];

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = 'a' + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(0, n[0]);
#else
  coll->insert(n[0], False);
#endif

  for (int ii = 0; ii < DIM; ii++)
    n[ii] = '0' + ii;

#ifdef COLLECTION_ARRAY
  coll->insertAt(1, n[0]);
#else
  coll->insert(n[0], False);
#endif

  coll->store();
  cout << "OID: " << coll->getOid() << endl;

  Bool found = False;
  coll->isIn(n[0], found);
  cout << "char isin: " << found << endl;

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }

  cout << "coll: " << coll->getOid() << endl;
  cout << "coll_class: " << coll->getClass()->getOid() << " " <<
    coll->getClass()->getName() << endl;
}

static void
perform_oid(Database &db)
{
  printf("PERFORM_OID\n");
  Class *cls_cls = db.getSchema()->getClass("class");
  COLLECTION *coll = new COLLECTION(&db, "", cls_cls, True);

  if (coll->getStatus())
    cerr << "oups: " << coll->getStatus() << endl;

  Class *c = db.getSchema()->getClass("int32");
  //coll->insert((Data)&c->getOid(), False, sizeof(Oid));
#ifdef COLLECTION_ARRAY
  coll->insertAt(0, c->getOid());
#else
  coll->insert(c->getOid());
#endif

  c = db.getSchema()->getClass("int64");
  //coll->insert((Data)&c->getOid(), False, sizeof(Oid));
#ifdef COLLECTION_ARRAY
  coll->insertAt(1, c->getOid());
#else
  coll->insert(c->getOid());
  //eyedblib::int16 i16 = 12;
  //printf("trying to insert i16\n");
  //coll->insert(i16, False);
#endif

  coll->store();

  Bool found = False;
  coll->isIn(c->getOid(), found);
  cout << "isin: " << found << endl;

#ifdef COLLECTION_ARRAY_2
  Oid item_oid;
  coll->retrieveAt(0, item_oid);
  cout << "at 0: '" << item_oid << "'" << endl;
  coll->retrieveAt(1, item_oid);
  cout << "at 1: '" << item_oid << "'" << endl;
#endif

  ValueArray val_arr;
  coll->getElements(val_arr);
  for (int n = 0; n < val_arr.getCount(); n++) {
    cout << val_arr[n].getString() << endl;
    //cout << val_arr[n].getType() << endl;
  }
}

/*
const Class *get(Database *db, const std::type_info &ty)
{
  const LinkedList *l = db->getSchema()->getClassList();
  LinkedListCursor c(l);
  const Class *cls;
  while(c.getNext((void *&)cls)) {
<<<<<<< colltest.cc
    printf("names: %s %s\n", typeid(cls->getName()).name(), ty.name());
    printf("names: %s %s\n", std::type_info(cls->getName()).name(), ty.name());
=======
    printf("names: %s %s\n", type_info(cls->getName()).name(), ty.name());
>>>>>>> 1.17
    if (type_info(cls->getName()) == ty)
      return cls;
  }
  return 0;
}
*/

/*
    const Class *c = get(&db, typeid(Person));
    if (c)
      printf("found = %s\n", c->getName());
*/

static void
perform_gbx(Database &db)
{
   OQL *oql = new OQL(&db, "select class");
  //OQL *oql = new OQL(&db, "select Person");
   //OQL *oql = new OQL(&db, "select class.name = \"byte\"");
  ValueArray val_arr;
  oql->execute(val_arr);

  for (int n = 0; n < 2; n++) {
    printf("time #%d\n", n);
    for (int j = 0; j < val_arr.getCount(); j++) {
      if (val_arr[j].type == Value::tOid) {
	Object *o;
	db.loadObject(*val_arr[j].oid, o);
	printf("TRACE #%d %s\n", j, (o->asClass() ?
				     o->asClass()->getName() : "not a class"));
	Object *o1 = o->clone();
	o1->trace();
	o1->release();
	o->release();
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);
  schema::init();

  if (argc < 2) {
    cerr << "usage: " << argv[0] << " <dbname>" << endl;
    return 1;
  }

  const char *dbname = argv[1];

  Exception::setMode(Exception::ExceptionMode);
  //set_terminate(ppp);
  //atexit(ppp);

  try {
    Connection conn;

    conn.open();

    schemaDatabase db(dbname);
    //Database db(dbname);
    db.open(&conn, (getenv("EYEDBLOCAL") ?
		    Database::DBRWLocal :
		    Database::DBRW));
    
#if 1
    TransactionParams params;

    //params.lockmode = ReadNWriteSX;
    params.trsmode = eyedb::TransactionOff;
    params.recovmode = eyedb::RecoveryOff;
    db.transactionBegin(params);
    OQL oql(&db, argv[2]);
    ObjectArray obj_arr;
    oql.execute(obj_arr);
    for (int i = 0; i < obj_arr.getCount(); i++)
      cout << obj_arr[i] << endl;
    sleep(1000);
#else
    db.transactionBegin();
    
    perform_list(db);
    perform_gbx(db);

    //perform_leaks(db);
    perform_cache(db);
    perform_obj(db);

    // EV : 18/01/06
    // when opening database in local mode, exit() cause server freeze
#if 0
    //sleep(1000);
    exit(1);
#endif
    perform_oid(db);
    perform_lit(db);
    perform_char(db);
    perform_str(db);

    perform_odl(db);

    //perform_err(db);
    db.transactionCommit();
#endif
  }

  catch(Exception &e) {
    fprintf(stderr, "oups\n");
    cerr << argv[0] << ": " << e;
    return 1;
  }

  return 0;
}

