
#include "schema.h"
#include <assert.h>

#define TRY_STORE

using namespace eyedb;
using namespace std;

static void
bug_coll_elements(Database &db)
{
#ifdef TRY_STORE
  //  db.registerObjects(True);
  db.storeOnCommit(True);
#endif
  OidArray oid_arr;
  ObjectArray obj_arr;
  A *a = new A(&db);

  a->addToSetB1Coll(new B(&db));
  a->addToSetB1Coll(new B(&db));

  a->getSetB1Coll()->getElements(obj_arr);
  assert(obj_arr.getCount() == 2);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

  assert(a->addToSetB1Coll(0));

#ifndef TRY_STORE
  a->store(RecMode::FullRecurs);
#endif

  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

  B *b = new B(&db);

  a->addToSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 3);
  assert(a->getSetB1Coll()->getCount() == 3);

  a->rmvFromSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

#ifndef TRY_STORE
  a->store(RecMode::FullRecurs);
#endif

  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

#ifdef TRY_STORE
  //  db.storeObjects();
#endif
}

static void
bug_coll_empty(Database &db)
{
  OidArray oid_arr;
  A *a = new A(&db);
  B *b = new B(&db);

  a->addToSetB1Coll(b);
  a->addToSetB1Coll(new B(&db));
  a->addToSetB1Coll(new B(&db));
  // 3 elements added dans le cache (o != NULL, oid == NULL)
  // 0 element dans le read_cache
  a->getSetB1Coll()->getElements(oid_arr);
  // 3 elements added dans le cache (o != NULL, oid == NULL)
  // 3 element dans le read_cache (o != NULL, oid == NULL)
  assert(oid_arr.getCount() == 3);
  assert(a->getSetB1Coll()->getCount() == 3);

  a->getSetB1Coll()->empty(); 
  // 3 elements suppress dans le cache (o != NULL, oid == NULL)
  // 0 element dans le read_cache

  a->getSetB1Coll()->getElements(oid_arr);
  // 3 elements suppress dans le cache (o != NULL, oid == NULL)
  // 0 element dans le read_cache
  assert(oid_arr.getCount() == 0);
  assert(a->getSetB1Coll()->getCount() == 0);

  printf("continue> ");
  getchar();
  a->addToSetB1Coll(b);
  // 3 elements suppress dans le cache + 1 element added (o != NULL, oid == NULL)
  // 0 element dans le read_cache

  a->getSetB1Coll()->getElements(oid_arr);
  // 3 elements suppress dans le cache + 1 element added
  // 0 element dans le read_cache

  assert(oid_arr.getCount() == 1);
  assert(a->getSetB1Coll()->getCount() == 1);

  b = new B(&db);
  a->addToSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

  a->getSetB1Coll()->store(RecMode::FullRecurs);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 2);
  assert(a->getSetB1Coll()->getCount() == 2);

  a->getSetB1Coll()->empty();
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 0);
  assert(a->getSetB1Coll()->getCount() == 0);

  printf("continue 2> ");
  getchar();
  a->addToSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  assert(oid_arr.getCount() == 1);
  assert(a->getSetB1Coll()->getCount() == 1);
}

static void
bug_coll_cnt_at(Database &db)
{
  OidArray oid_arr;

  A *a = new A(&db);
  B *b = new B(&db);
  a->addToSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  printf("#1 oid_arr count %d\n", oid_arr.getCount());
  a->getSetB1Coll()->getElements(oid_arr);
  printf("#2 oid_arr count %d\n", oid_arr.getCount());
  a->addToSetB1Coll(new B(&db));

  CollectionIterator i1(a->getSetB1Coll());
  Object *b1;
  while (i1.next(b1))
    cout << "##1 " << (void *)b1 << " ";
  /*
  for (int i = 0; i < a->getSetB1Count(); i++)
    cout << "##1 " << (void *)a->getSetB1At(i) << " ";
  */

  cout << endl;

  b = new B(&db);
  a->addToSetB1Coll(b);
  a->getSetB1Coll()->getElements(oid_arr);
  printf("#1 oid_arr count %d\n", oid_arr.getCount());
  a->getSetB1Coll()->store(RecMode::FullRecurs);
  a->getSetB1Coll()->empty();
  a->getSetB1Coll()->getElements(oid_arr);
  printf("#2 oid_arr count %d\n", oid_arr.getCount());
  a->addToSetB1Coll(b);

  CollectionIterator i2(a->getSetB1Coll());
  while (i2.next(b1))
    cout << "##2 " << (void *)b1 << " ";
  /*
  for (int i = 0; i < a->getSetB1Count(); i++)
    cout << "##2 " << (void *)a->getSetB1At(i) << " ";
  */

  cout << endl;
}

static void
display_coll_elements(const char *when, Database &db, A *a)
{
  printf("\n------------- %s bottom %d and top %d array.cnt = %d\n",
	 when, a->getArrB4Coll()->getBottom(),
	 a->getArrB4Coll()->getTop(),
	 a->getArrB4Coll()->getCount());

  //assert(a->getArrB4Coll()->getCount() == 3);

  printf("objat[0] = %p\n", a->retrieveArrB4At(0));
  printf("objat[1] = %p\n", a->retrieveArrB4At(1));
  printf("objat[2] = %p\n", a->retrieveArrB4At(2));
  printf("objat[3] = %p\n", a->retrieveArrB4At(3));
  printf("objat[4] = %p\n", a->retrieveArrB4At(4));
  printf("objat[5] = %p\n", a->retrieveArrB4At(5));
  printf("objat[6] = %p\n", a->retrieveArrB4At(6));

  printf("oidat[0] = %s\n", a->retrieveArrB4OidAt(0).toString());
  printf("oidat[1] = %s\n", a->retrieveArrB4OidAt(1).toString());
  printf("oidat[2] = %s\n", a->retrieveArrB4OidAt(2).toString());
  printf("oidat[3] = %s\n", a->retrieveArrB4OidAt(3).toString());
  printf("oidat[4] = %s\n", a->retrieveArrB4OidAt(4).toString());
  printf("oidat[5] = %s\n", a->retrieveArrB4OidAt(5).toString());
  printf("oidat[6] = %s\n", a->retrieveArrB4OidAt(6).toString());

  // val_arr
  ValueArray val_arr;

  /*
  a->getBagB5Coll()->getElements(val_arr);
  printf("BagB5 val_arr.getCount() == %d\n", val_arr.getCount());
  for (int i = 0; i < val_arr.getCount(); i++) 
    printf("BagB5 valarr[%d] = %s\n", i, val_arr[i].getString());

  a->getArrB4Coll()->getElements(val_arr, True);
  */

  printf("ArrB4 True val_arr.getCount() == %d\n", val_arr.getCount());

  for (int i = 0; i < val_arr.getCount(); i++) 
    printf("ArrB4 True valarr[%d] = %s\n", i, val_arr[i].getString());

  a->getArrB4Coll()->getElements(val_arr, False);

  printf("ArrB4 False val_arr.getCount() == %d\n", val_arr.getCount());

  for (int i = 0; i < val_arr.getCount(); i++) 
    printf("ArrB4 False valarr[%d] = %s\n", i, val_arr[i].getString());

  // oid_arr

  OidArray oid_arr;
  /*
  a->getBagB5Coll()->getElements(oid_arr);
  printf("BagB5 oid_arr.getCount() == %d\n", oid_arr.getCount());
  for (int i = 0; i < oid_arr.getCount(); i++) 
    printf("BagB5 oidarr[%d] = %s\n", i, oid_arr[i].getString());
  */

  a->getArrB4Coll()->getElements(oid_arr);

  printf("ArrB4 oid_arr.getCount() == %d\n", oid_arr.getCount());

  for (int i = 0; i < oid_arr.getCount(); i++) 
    printf("ArrB4 oidarr[%d] = %s\n", i, oid_arr[i].getString());


  for (int i = 0; i < 10; i++) {
    Oid oid;
    Object *o;
    a->getArrB4Coll()->retrieveAt(i, oid);
    printf("item_oid[%d] = %s\n", i, oid.toString());
    a->getArrB4Coll()->retrieveAt(i, o);
    printf("item_obj[%d] = %p\n", i, o);
  }

  printf("Checking %s [%d]\n", a->getArrB4Coll()->getOid().toString(),
	 a->getArrB4Coll()->getCount());
}

static void
bug_coll_array(Database &db)
{
  A *a = new A(&db);
  B *b = new B(&db);
  a->setInArrB4CollAt(0, b);
  a->setInArrB4CollAt(3, b);
  a->setInArrB4CollAt(4, new B(&db));

  a->addToBagB5Coll(b);
  a->addToBagB5Coll(b);
  a->addToBagB5Coll(new B(&db));
  a->addToBagB5Coll(b);
  a->addToBagB5Coll(new B(&db));
  a->addToBagB5Coll(new B(&db));
  a->addToBagB5Coll(new B(&db));
  a->addToBagB5Coll(new B(&db));

  display_coll_elements("first", db, a);
  a->store(RecMode::FullRecurs);
  display_coll_elements("middle 1", db, a);
  a->setInArrB4CollAt(5, b);
  a->setInArrB4CollAt(2, new B(&db));
  a->setInArrB4CollAt(1, b);
  a->unsetInArrB4CollAt(3);
  display_coll_elements("middle 2", db, a);
  a->getArrB4Coll()->empty();
  //a->getBagB5Coll()->empty();
  display_coll_elements("after empty", db, a);
  //printf("before store 1\n");
  //a->getBagB5Coll()->store();
  printf("before store 2\n");
  a->getArrB4Coll()->store();
  printf("after store\n");
  display_coll_elements("after empty/store", db, a);

  db.transactionCommit();

  db.transactionBegin();
  Object *o;
  db.loadObject(a->getOid(), o);
  a = A_c(o);
  display_coll_elements("last", db, a);
}

static void
bug_coll_array2(Database &db)
{
  Supercontig *sctg;
  eyedb::CollSet *set;

  Stage *s = new Stage(&db);
  s->setId(1);

  set = new eyedb::CollSet(&db, "", db.getSchema()->getClass("Supercontig"), eyedb::True);

  for (int n = 0; n < 10; n++) {
    sctg = new Supercontig(&db);
    sctg->setId(n+1);
    set->insert(sctg);
    sctg->release();
  }

  s->setInLinkCounts1CollAt(0, set);
  set->release();

  set = new eyedb::CollSet(&db, "", db.getSchema()->getClass("Supercontig"), eyedb::True);

  for (int n = 0; n < 10; n++) {
    sctg = new Supercontig(&db);
    sctg->setId(n+11);
    set->insert(sctg);
    sctg->release();
  }

  s->setInLinkCounts1CollAt(3, set);
  set->release();
  s->store(RecMode::FullRecurs);
  printf("oid: %s\n", s->getOid().toString());
}

//#define FULL_RECURS

static void
bug_coll_array3(Database &db)
{
  Supercontig *sctg;
  eyedb::CollSet *set;

  Stage *s = new Stage(&db);
  s->setId(1);

  set = new eyedb::CollSet(&db, "", db.getSchema()->getClass("Supercontig"), eyedb::True);

  for (int n = 0; n < 10; n++) {
    sctg = new Supercontig(&db);
    sctg->setId(n+1);
    set->insert(sctg);
#ifndef FULL_RECURS
    sctg->store();
#endif
    sctg->release();
  }

#ifndef FULL_RECURS
  set->store();
#endif

  s->setInLinkCounts1CollAt(0, set);
  set->release();

  set = new eyedb::CollSet(&db, "", db.getSchema()->getClass("Supercontig"), eyedb::True);

  for (int n = 0; n < 10; n++) {
    sctg = new Supercontig(&db);
    sctg->setId(n+11);
    set->insert(sctg);
#ifndef FULL_RECURS
    sctg->store();
#endif
    sctg->release();
  }

#ifndef FULL_RECURS
  set->store();
#endif

  s->setInLinkCounts1CollAt(3, set);
  set->release();
#ifdef FULL_RECURS
  s->store(RecMode::FullRecurs);
#else
  s->store();
#endif
#ifdef FULL_RECURS
  printf("FullRecurs mode\n");
#else
  printf("NoRecurs mode\n");
#endif
  printf("oid: %s\n", s->getOid().toString());
}

int
main(int argc, char *argv[])
{
  // initializing the EyeDB layer
  eyedb::init(argc, argv);

  // initializing the person package
  schema::init();

  if (argc != 2)
    {
      fprintf(stderr, "usage: %s <dbname>\n",
	      argv[0]);
      return 1;
    }

  const char *dbname = argv[1];

  Exception::setMode(Exception::ExceptionMode);

  try {
    Connection conn;

    conn.open();

    schemaDatabase db(dbname);
    db.open(&conn, (getenv("EYEDBLOCAL") ?
		    Database::DBRWLocal :
		    Database::DBRW));
    
    db.transactionBegin();

    /*
    bug_coll_elements(db);
    bug_coll_empty(db);
    bug_coll_cnt_at(db);
    */
    //bug_coll_array(db);
    //    bug_coll_array2(db);
    bug_coll_array3(db);

    db.transactionCommit();
  }

  catch(Exception &e) {
    cerr << argv[0] << ": " << e << endl;
    return 1;
  }

  return 0;
}
