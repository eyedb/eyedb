
#include "schema.h"
#include <assert.h>

using namespace eyedb;
using namespace std;

static void
test_leaks(Database &db)
{
  printf("purify1> ");
  getchar();
  B *b = new B(&db);
  b->release();

  printf("purify2> ");
  getchar();
  A *a = new A(&db);
  B *b1 = new B(&db);
  b1->addToLsetA4Coll(a);
  b1->store(RecMode::FullRecurs);
  Oid b1_oid = b1->getOid();
  a->release();
  b1->release();

  //  Log::setLog("stdout");
  //  Log::setLogMask("object:refcount+object:garbage");
  printf("purify3> ");
  getchar();
  B *newb = 0;
  db.loadObject(b1_oid, (Object *&)newb);
  newb->trace();
  newb->release();
  //Log::setLog(0);
  printf("end> ");
  getchar();
}

static void
test_N_1(Database &db)
{
  B *b = new B(&db);
  A *a1 = new A(&db);
  A *a2 = new A(&db);
  a1->setAId(100);
  a1->setAS("hello");
  a2->setAId(2000);
  a2->setAS("the");

  b->addToOsetA4Coll(a1);
  b->addToLsetA4Coll(a2);

  A *a3 = new A(&db);
  b->addToOsetA5Coll(a1);
  b->addToOsetA5Coll(a2);
  b->addToOsetA5Coll(a3);
  b->addToLsetA5Coll(a1);
  b->addToLsetA5Coll(a2);
  b->addToLsetA5Coll(a3);

  b->trace();
  b->store(RecMode::FullRecurs);

  printf("Stored -> %s\n", b->getOid().toString());

  db.transactionCommit();
  db.transactionBegin();

  printf("doit2> ");
  getchar();
  B* newb = 0;
  db.loadObject(b->getOid(), (Object *&)newb);
  printf("have load\n");
  newb->trace();
  newb->release();
  printf("doit3> ");
  getchar();

#if 0
  if (getenv("DOIT1"))
    printf("count = %d\n", newb->getOsetA4Count());

  if (getenv("DOIT2"))
    {
      printf("doit2> ");
      getchar();
      newb->trace();
      newb->release();
      printf("doit3> ");
      getchar();
    }

  if (getenv("DOIT3"))
    for (int i = 0; i < newb->getOsetA4Count(); i++)
      newb->getOsetA4At(i)->trace();
  /*
  printf("count = %d\n", newb->getLsetA4Count());
  newb->trace();
  printf("1> ");
  */

  //  newb->getLsetA4Coll()->trace();
  if (getenv("DOIT4"))
    newb->getLsetA4Coll();

  /*
  // les fuites memoires sont la:
  newb->getLsetA4Coll()->trace();
  for (i = 0; i < newb->getLsetA4Count(); i++)
    newb->getLsetA4At(i)->trace();
  // ...
  */

  /*
  printf("2> ");
  newb->getLsetA4Coll()->trace();
  A *a4 = newb->getLsetA4At(0);
  printf("3> ");
  a4->trace();
  printf("3> ");
  newb->getLsetA4Coll()->trace();
  */
#endif

  /*
  newb->rmvFromLsetA4Coll(a4);
  newb->store();
  */

  b->release();
  a1->release();
  a2->release();
  a3->release();
  if (!getenv("DOIT2"))
    newb->release();
}

#define ATTR setLb6
//#define ATTR setOb6

static void
test_1_Na(Database &db)
{
  A *a3 = new A(&db);
  B *b1 = new B(&db);
  a3->ATTR(b1);
  //  a3->setLb6(b1);

  a3->store(RecMode::FullRecurs);

  db.transactionCommit();
  db.transactionBegin();

  B* newb;
  db.loadObject(b1->getOid(), (Object *&)newb);
  newb->trace();

  /*
  db.transactionCommit();
  db.transactionBegin();

  a3->setLb6Oid(Oid::nullOid);
  a3->store();
  db.loadObject(b1->getOid(), (Object *&)newb);
  newb->trace();
  */

  printf("b1 %s\n", b1->getOid().toString());
  printf("a3 %s\n", a3->getOid().toString());
}

static void
test_1_Nb(Database &db)
{
  A *a3 = new A(&db);
  B *b1 = new B(&db);
  //  a3->setOb6(b1);
  a3->ATTR(b1);

  printf("before a3 store\n");
  a3->store(RecMode::FullRecurs);
  printf("after a3 store\n");

  db.transactionCommit();
  db.transactionBegin();

  B* newb;
  db.loadObject(b1->getOid(), (Object *&)newb);
  //newb->trace();

  db.transactionCommit();
  db.transactionBegin();

  A *a4 = new A(&db);
  a4->ATTR(b1);
  printf("before a4 store\n");
  a4->store();
  printf("before a4 store\n");
  db.loadObject(b1->getOid(), (Object *&)newb);
  //newb->trace();
  printf("b1 %s\n", b1->getOid().toString());
  printf("a3 %s\n", a3->getOid().toString());
  printf("a4 %s\n", a4->getOid().toString());
}

static void
test_1_Nc(Database &db)
{
  A *a3 = new A(&db);
  B *b1 = new B(&db);
  B *b2 = new B(&db);
  //  a3->setOb6(b1);
  a3->ATTR(b1);

  printf("before a3 store\n");
  a3->store(RecMode::FullRecurs);
  printf("after a3 store\n");

  db.transactionCommit();
  db.transactionBegin();

  B* newb;
  db.loadObject(b1->getOid(), (Object *&)newb);
  //newb->trace();

  db.transactionCommit();
  db.transactionBegin();

  b2->store();
  a3->ATTR(b2);

  a3->store();
  printf("b1 %s\n", b1->getOid().toString());
  printf("b2 %s\n", b2->getOid().toString());
  printf("a3 %s\n", a3->getOid().toString());
}

static void
realize(Database &db)
{
  test_leaks(db);
  /*
  printf("purify> ");
  getchar();
  test_N_1(db);
  printf("end> ");
  getchar();
  */
  test_1_Na(db);
  test_1_Nb(db);
  test_1_Nc(db);
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
    schemaDatabase db(dbname);

    conn.open();

    if (getenv("EYEDBWAIT"))
      {
	printf("go> ");
	getchar();
      }

    db.open(&conn, (getenv("EYEDBLOCAL") ?
		    Database::DBRWLocal :
		    Database::DBRW));
    
    db.transactionBegin();

    //gbxContext gbx_ctx;
    realize(db);

    db.transactionCommit();
  }

  catch(Exception &e) {
    cerr << argv[0] << ": " << e;
    return 1;
  }

  return 0;
}
