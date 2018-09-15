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


/*
  TODO :
  One must update runtime pointers in the client side by giving back
  the oid modified ; so each time there is a dataWrite() one has to
  fill an object will the following information : oid modified, item offset, 
  value of the data (which is an oid!).
  The client side will get this information and will looksin its cache for
  the modified oids, then will modify their IDR en consequence!
*/

//#define FRONT_END

#include "eyedb_p.h"
#include "Attribute_p.h"
#include "CollectionBE.h"
#include <eyedbsm/eyedbsm.h>
#include "InvOidContext.h"

#include <assert.h>

#include "misc.h"

#define CK(X) do { \
 Status __s__ = (X); \
 if (__s__) return __s__; \
} while(0)

//#define INVDBG

namespace eyedb {

#ifdef INVDBG
  static void
  get_object_class(Database *db, const Oid &oid)
  {
    if (!oid.isValid())
      return;

    Class *objcls = NULL;
    Status status = db->getObjectClass(oid, objcls);

    if (status)
      status->print();

    if (objcls)
      IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("OID %s -> %s\n", oid.toString(), objcls->getName()));
  }
#else
#define get_object_class(x, y)
#endif

  static Status
  inverse_coll_realize(Collection *&coll)
  {
    coll->invalidateInverse();
    Status s = coll->realize();
    coll->validateInverse();
    coll->release(); coll = 0;
    return s;
  }

  static Status
  requalify(Database *db, const Oid &obj_oid,
	    const Attribute *&inv_item)
  {
    if (!obj_oid.isValid())
      return Success;

    Class *objcls = 0;
    Status status = db->getObjectClass(obj_oid, objcls);
    if (status) return status;

    if (objcls->getOid() == inv_item->getClassOwner()->getOid())
      return Success;

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
	    ("Attribute::inverse_realize(requalifying inverse item "
	     "%s::%s for %s\n",
	     inv_item->getClassOwner()->getName(),
	     inv_item->getName(), obj_oid.toString()));
    /*
      printf("Attribute::inverse_realize(requalifying inverse item "
      "%s::%s for %s\n",
      inv_item->getClassOwner()->getName(),
      inv_item->getName(), obj_oid.toString());
    */

    inv_item = objcls->getAttribute(inv_item->getName());

    assert(inv_item);
    return Success;
  }

  Status
  Attribute::checkInverse(const Attribute *item) const
  {
    if (!item)
      item = this;

    if (item->isVarDim())
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "'%s' cannot make inverse on variable "
			     "dimension attribute", item->name);

    if (!item->cls->asCollectionClass() && !item->cls->asAgregatClass())
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR, 
			     "'%s' cannot make inverse on mateagregat "
			     "or collection_class attribute", item->name);

    if (item->typmod.pdims > 1)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "'%s' cannot make inverse on non "
			     "single dimension attribute", item->name);

    return Success;
  }

  void
  Attribute::getInverse(const char **cname, const char **fname,
			const Attribute **item) const
  {
    if (cname)
      *cname = inv_spec.clsname;
    if (fname)
      *fname = inv_spec.fname;
    if (item)
      *item = inv_spec.item;
  }

  Bool Attribute::hasInverse() const
  {
    return inv_spec.item ? True : False;
  }

  Status Attribute::setInverse(const Attribute *item)
  {
    if (inv_spec.item || inv_spec.clsname)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "inverse is already set for '%s'", name);

    Status status = checkInverse();
    if (status)
      return status;

    status = checkInverse(item);
    if (status)
      return status;

    inv_spec.item = item;
    return Success;
  }

  Status Attribute::setInverse(const char *clsname, const char *fname)
  {
    if (inv_spec.item || inv_spec.clsname)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "inverse is already set for '%s'", name);

    Status status = checkInverse();
    if (status)
      return status;

    if (!clsname || !fname)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "'%s' setInverse : invalid null value", name);

    inv_spec.clsname = strdup(clsname);
    inv_spec.fname      = strdup(fname);
    return Success;
  }

  void
  Attribute::completeInverseItem(Schema *m)
  {
    if (!inv_spec.item->cls)
      ((Attribute *)inv_spec.item)->cls =
	m->getClass(inv_spec.item->oid_cl);

    if (!inv_spec.item->class_owner)
      ((Attribute *)inv_spec.item)->class_owner =
	m->getClass(inv_spec.item->oid_cl_own);
  }

  Status
  Attribute::completeInverse(Schema *m)
  {
    if (!inv_spec.clsname || inv_spec.item)
      return Success;

    Class *cl;
    cl = m->getClass(inv_spec.clsname);

    if (!cl)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "attribute '%s' in agregat class '%s': "
			     "cannot find agregat class '%s'",
			     name, class_owner->getName(),
			     inv_spec.clsname);

    if (!cl->asAgregatClass())
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "attribute '%s' in agregat class '%s':"
			     "class '%s' is not a agregat class",
			     name, class_owner->getName(),
			     inv_spec.clsname);

    inv_spec.item = cl->getAttribute(inv_spec.fname);

    if (!inv_spec.item)
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "attribute '%s' in agregat class '%s': "
			     "cannot find item '%s' in agregat class '%s'",
			     name, class_owner->getName(),
			     inv_spec.fname, inv_spec.clsname);

    completeInverseItem(m);

    return checkInverse(inv_spec.item);
  }

  Status Attribute::completeInverse(Database *db)
  {
    Class *cl;
    Schema *m = db->getSchema();

    Status s = completeInverse(m);
    if (s)
      return s;

    if (inv_spec.item) {
      completeInverseItem(m);

      inv_spec.num = inv_spec.item->num;
      cl = (Class *)inv_spec.item->class_owner;

      // added the 10/12/99
      if (!cl)
	return Success;
      // ...

      if (!cl->getOid().isValid()) {
	Status status = cl->create();
	if (status) return status;
      }
    
      inv_spec.oid_cl = cl->getOid();
    }

    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // Inverse computing
  // 
  // ---------------------------------------------------------------------------

  /* 
     obj_idr         : the IDR of the current object.
     obj_oid         : the oid of the current object.
     inv_obj_oid     : (obj_idr::this) the oid of the object (collection or not)
     pointed by 'this' attribute in the IDR.
     old_inv_obj_oid : (obj_oid::this) the oid of the object (collection or not)
     pointed by 'this' attribute in the DB.
     [no meaning if op == objInvCreate]
     relation 1:1
     cur_obj_oid  : (inv_obj_oid::inv_item) the oid of the object pointed by
     inv_obj_oid by 'this->inv_item' attribute in the DB.
     old_obj_oid  : (old_inv_obj_oid::inv_item) the oid of the object pointed
     by old_inv_obj_oid by 'this->inv_item' attribute
     in the DB.
  */

  static const char *
  get_op_name(Attribute::InvObjOp op)
  {
    if (op == Attribute::invObjCreate)
      return "ObjCreate";

    if (op == Attribute::invObjUpdate)
      return "ObjUpdate";

    if (op == Attribute::invObjRemove)
      return "ObjRemove";

    return "invObjUnknown";
  }

  Status
  Attribute::inverse_coll_perform(Database *db, InvObjOp op,
				  const Oid &obj_oid,
				  const Oid &inv_obj_oid) const
  {
    // WARNING: changed the 10/04/00 because of a manue's bug in genetpig
    if (!inv_spec.item)
      return Success;

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
	    ("inverse_coll_perform(%s, %s)\n", name, inv_spec.item->getName()));

    if (inv_spec.item->getClass()->asCollectionClass())
      return inverse_coll_perform_N_N(db, op, obj_oid, inv_obj_oid);

    return inverse_coll_perform_N_1(db, op, obj_oid, inv_obj_oid);
  }

  /*
    obj_oid         : the oid of the current object.
    x_obj_oid       : the oid of the object to insert (op == invObjUpdate) or
    remove (op == invObjRemove) from the collection.
  */

  // introduced the 15/12/99
  // because I do not understand why it is necessary to run all this code
  // when one adds an object!
#define N_N_SIMPLE

  Status
  Attribute::inverse_coll_perform_N_N(Database *db, InvObjOp op,
				      const Oid &obj_oid,
				      const Oid &x_obj_oid) const
  {
    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
	    ("inverse_coll_perform_N_N(%s, inv %s, op = %s, "
	     "obj_oid = %s, x_obj_oid = %s)\n",
	     name, inv_spec.item->getName(), get_op_name(op), obj_oid.toString(),
	     x_obj_oid.toString()));
  
    const Attribute *inv_item = inv_spec.item;
    InvCtx inv_ctx;
    assert(inv_item);

    get_object_class(db, obj_oid);
    get_object_class(db, x_obj_oid);

    if (op == invObjUpdate) // i.e. inserting x_obj_oid
      {
	Oid old_obj_oid;
	CK(inverse_read_oid(db, inv_item, x_obj_oid, old_obj_oid));

	Collection *coll = 0;
	if (old_obj_oid.isValid())
	  CK(inverse_get_collection(db, old_obj_oid, coll));

#ifndef N_N_SIMPLE
	if (coll)
	  {
	    OidArray oid_arr;
	    CK(coll->getElements(oid_arr));

	    for (int i = 0; i < oid_arr.getCount(); i++)
	      {
		Oid coll_oid;
		CK(inverse_read_oid(db, this, oid_arr[i], coll_oid));
		if (coll_oid.isValid())
		  {
		    Collection *inv_coll;
		    CK(inverse_get_collection(db, coll_oid, inv_coll));
		    if (inv_coll)
		      {
			IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
				("suppressing [1] %s from collection %s\n",
				 x_obj_oid.toString(), 
				 inv_coll->getOid().toString()));
			CK(inv_coll->suppress(x_obj_oid, True));
			CK(inverse_coll_realize(inv_coll));
		      }
		  }
	      }
	  }
#endif

	if (!coll)
	  {
	    CK(inverse_create_collection(db, inv_item, x_obj_oid,
					 True, x_obj_oid, coll));
	      
	    CK(inverse_write_oid(db, inv_item, x_obj_oid,
				 coll->getOidC(), inv_ctx));
	  }

#ifndef N_N_SIMPLE
	Collection *obj_coll = 0;
	Oid coll_obj_oid;
	CK(inverse_read_oid(db, this, obj_oid, coll_obj_oid));

	CK(inverse_get_collection(db, coll_obj_oid, obj_coll));

	OidArray oid_arr;

	Iterator q(obj_coll);
	CK(q.getStatus());
	CK(q.scan(oid_arr));
      
	obj_coll->release(); obj_coll = 0;

	for (int i = 0; i < oid_arr.getCount(); i++)
	  {
	    Oid coll_oid;
	    CK(inverse_read_oid(db, inv_item, oid_arr[i], coll_oid));

	    if (coll_oid.isValid())
	      {
		Collection *inv_coll = 0;
		CK(inverse_get_collection(db, coll_oid, inv_coll));
		if (inv_coll)
		  {
		    OidArray inv_oid_arr;
		    CK(inv_coll->getElements(inv_oid_arr));
		    inv_coll->release(); inv_coll = 0;

		    for (int j = 0; j < inv_oid_arr.getCount(); j++)
		      {
			Collection *rinv_coll = 0;
			Oid rinv_coll_oid;
			CK(inverse_read_oid(db, this, inv_oid_arr[j],
					    rinv_coll_oid));
			if (rinv_coll_oid.isValid())
			  {
			    CK(inverse_get_collection(db, rinv_coll_oid,
						      rinv_coll));
		      
			    if (rinv_coll)
			      {
				if (i == 0)
				  {
				    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
					    ("inserting [1] %s to collection %s\n",
					     inv_oid_arr[j].toString(), 
					     coll->getOid().toString()));
				    CK(coll->insert(inv_oid_arr[j], True));
				  }
				IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
					("appending [2] %s to collection %s\n",
					 x_obj_oid.toString(), 
					 rinv_coll->getOid().toString()));
				CK(rinv_coll->insert(x_obj_oid, True));
				CK(inverse_coll_realize(rinv_coll));
			      }
			  }
		      }
		  }
	      }
	  }
#endif

	IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		("appending [3] %s to collection %s\n",
		 obj_oid.toString(), coll->getOid().toString()));
	coll->insert(obj_oid, True);
	CK(inverse_coll_realize(coll));

	return Success;
      }

    if (op == invObjRemove) // removing x_obj_oid from collection
      {
	Oid old_obj_oid;
	CK(inverse_read_oid(db, inv_item, x_obj_oid, old_obj_oid));

	Collection *coll = 0;
	if (old_obj_oid.isValid())
	  CK(inverse_get_collection(db, old_obj_oid, coll));

	Oid cur_coll_oid;
	CK(inverse_read_oid(db, this, obj_oid, cur_coll_oid));

	if (coll)
	  {
	    OidArray oid_arr;
	    CK(coll->getElements(oid_arr));

	    for (int i = 0; i < oid_arr.getCount(); i++)
	      {
		Oid coll_oid;
		CK(inverse_read_oid(db, this, oid_arr[i], coll_oid));
		if (coll_oid.isValid() && coll_oid != cur_coll_oid)
		  {
		    Collection *inv_coll;
		    CK(inverse_get_collection(db, coll_oid, inv_coll));
		    if (inv_coll)
		      {
			IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
				("suppressing [2] %s from collection %s\n",
				 x_obj_oid.toString(), 
				 inv_coll->getOid().toString()));
			CK(inv_coll->suppress(x_obj_oid, True));
			CK(inverse_coll_realize(inv_coll));
		      }
		  }
	      }

	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		    ("suppressing [3] %s from collection %s\n",
		     obj_oid.toString(), 
		     coll->getOid().toString()));
	    coll->suppress(obj_oid, True);

	    CK(inverse_coll_realize(coll));
	  }

	return Success;
      }

    abort();
    return Success;
  }

  /*
    obj_oid         : the oid of the current object.
    x_obj_oid       : the oid of the object to insert (op == invObjUpdate) pr
    remove (op == invObjRemove) from the collection.
  */

  Status
  Attribute::inverse_coll_perform_N_1(Database *db, InvObjOp op,
				      const Oid &obj_oid,
				      const Oid &x_obj_oid) const
  {
    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("inverse_coll_perform_N_1(%s, inv %s, op = %s, "
				      "obj_oid = %s, x_obj_oid = %s)\n",
				      name, inv_spec.item->getName(), get_op_name(op), obj_oid.toString(),
				      x_obj_oid.toString()));

    const Attribute *inv_item = inv_spec.item;
    InvCtx inv_ctx;
    assert(inv_item);

    get_object_class(db, obj_oid);
    get_object_class(db, x_obj_oid);

    if (op == invObjUpdate) // i.e. inserting x_obj_oid
      {
	Oid old_obj_oid;
	CK(inverse_read_oid(db, inv_item, x_obj_oid, old_obj_oid));
	if (old_obj_oid.isValid() && old_obj_oid != obj_oid)
	  {
	    Oid old_inv_obj_oid;
	    CK(inverse_read_oid(db, this, old_obj_oid, old_inv_obj_oid));
	    if (old_inv_obj_oid.isValid())
	      {
		Collection *old_coll;
		CK(inverse_get_collection(db, old_inv_obj_oid, old_coll));
		if (old_coll)
		  {
		    OidArray oid_arr;
		    CK(old_coll->getElements(oid_arr));

		    for (int i = 0; i < oid_arr.getCount(); i++)
		      {
			CK(inverse_write_oid(db, inv_item, oid_arr[i],
					     Oid::nullOid, inv_ctx));
			IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
				("suppressing [4] %s from collection %s\n",
				 oid_arr[i].toString(), 
				 old_coll->getOid().toString()));
			CK(old_coll->suppress(oid_arr[i], True));
		      }

		    CK(inverse_coll_realize(old_coll));
		  }
	      }
	  }
      
	return inverse_write_oid(db, inv_item, x_obj_oid, obj_oid, inv_ctx);
      }

    if (op == invObjRemove)
      return inverse_write_oid(db, inv_item, x_obj_oid, Oid::nullOid,
			       inv_ctx);

    abort();
    return Success;
  }

  static void
  _trace_(const char *mth, Attribute::InvObjOp op,
	  const Attribute *item,
	  const Attribute *inv_item,
	  const Oid &obj_oid, 
	  const Oid &inv_obj_oid)
  {
    if (!(IDB_LOG_RELSHIP_DETAILS & Log::getLogMask()))
      return;

    char buf[256];
    std::string str;
    sprintf(buf, "Attribute::%s\n", mth);
    str += buf;
    sprintf(buf, "\t\top = %s;\n", get_op_name(op));
    str += buf;
    sprintf(buf, "\t\titem = %s, %d;\n", item->getName(), item->getNum());
    str += buf;
    sprintf(buf, "\t\tinv_item = %s, %d;\n", inv_item->getName(), inv_item->getNum());
    str += buf;
    sprintf(buf, "\t\tobj_oid = %s;\n", obj_oid.toString());
    str += buf;
    sprintf(buf, "\t\tinv_obj_oid = %s;\n\n", inv_obj_oid.toString());
    str += buf;

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, (str.c_str()));
  }

  // ---------------------------------------------------------------------------
  //
  // inverse_1_1
  //
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_1_1(Database *db, InvObjOp op,
			 const Attribute *inv_item,
			 const Oid &obj_oid, 
			 const Oid &inv_obj_oid,
			 const InvCtx &inv_ctx) const
  {
    _trace_("inverse_1_1", op, this, inv_item, obj_oid, inv_obj_oid);

    if (op == invObjCreate)
      {
	if (!inv_obj_oid.isValid())
	  return Success;
	Oid cur_obj_oid;
	CK(inverse_read_oid(db, inv_item, inv_obj_oid, cur_obj_oid));

	IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		("inverse 1_1 create: cur_obj_oid %s\n",
		 cur_obj_oid.toString()));

	if (cur_obj_oid != obj_oid) // added the 30/8/99
	  {
	    if (cur_obj_oid.isValid())
	      CK(inverse_write_oid(db, this, cur_obj_oid, Oid::nullOid,
				   inv_ctx));
	    return inverse_write_oid(db, inv_item, inv_obj_oid, obj_oid,
				     inv_ctx);
	  }

	return Success; // added the 10/9/99
      }

    if (op == invObjUpdate)
      {
	Oid old_inv_obj_oid;
	CK(inverse_read_oid(db, this, obj_oid, old_inv_obj_oid));

	IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		("inverse 1_1 update: obj_oid %s, "
		 "old_inv_obj_oid %s, inv_obj_oid %s\n",
		 obj_oid.toString(), old_inv_obj_oid.toString(),
		 inv_obj_oid.toString()));

	if (old_inv_obj_oid.isValid() && old_inv_obj_oid != inv_obj_oid)
	  {
	    Oid old_obj_oid;
	    CK(inverse_read_oid(db, inv_item, old_inv_obj_oid, old_obj_oid));

	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		    ("inverse 1_1 update: old_obj_oid %s\n", old_obj_oid.toString()));
	    if (old_obj_oid == obj_oid)
	      CK(inverse_write_oid(db, inv_item, old_inv_obj_oid,
				   Oid::nullOid, inv_ctx));
	  }

	if (!inv_obj_oid.isValid())
	  return Success;

	Oid cur_obj_oid;
	CK(inverse_read_oid(db, inv_item, inv_obj_oid, cur_obj_oid));

	IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		("inverse 1_1 update: cur_obj_oid %s\n",
		 cur_obj_oid.toString()));

	// changed the 15/9/99
	//      if (cur_obj_oid.isValid())
	if (cur_obj_oid.isValid() && cur_obj_oid != obj_oid)
	  CK(inverse_write_oid(db, this, cur_obj_oid, Oid::nullOid,
			       inv_ctx));
	return inverse_write_oid(db, inv_item, inv_obj_oid, obj_oid, inv_ctx);
      }

    if (op == invObjRemove)
      {
	if (!inv_obj_oid.isValid())
	  return Success;
	return inverse_write_oid(db, inv_item, inv_obj_oid, Oid::nullOid,
				 inv_ctx);
      }

    abort();
    return Success;
  }
  
  // ---------------------------------------------------------------------------
  //
  // inverse_1_N
  //
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_1_N(Database *db, InvObjOp op,
			 const Attribute *inv_item,
			 const Oid &obj_oid,
			 const Oid &inv_obj_oid,
			 const InvCtx &inv_ctx) const
  {
    _trace_("inverse_1_N", op, this, inv_item, obj_oid, inv_obj_oid);

    if (op == invObjUpdate)
      {
	Oid old_inv_obj_oid;
	CK(inverse_read_oid(db, this, obj_oid, old_inv_obj_oid));

	if (old_inv_obj_oid.isValid() && old_inv_obj_oid != inv_obj_oid)
	  {
	    Oid old_obj_oid;
	    CK(inverse_read_oid(db, inv_item, old_inv_obj_oid, old_obj_oid));
	    if (old_obj_oid.isValid())
	      {
		Collection *coll;
		CK(inverse_get_collection(db, old_obj_oid, coll));

		if (coll)
		  {
		    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
			    ("suppressing [5] %s from collection %s\n",
			     obj_oid.toString(), 
			     coll->getOid().toString()));
		    CK(coll->suppress(obj_oid, True));
		    CK(inverse_coll_realize(coll));
		  }
	      }
	  }
      }

    if (op == invObjCreate || op == invObjUpdate)
      {
	if (!inv_obj_oid.isValid())
	  return Success;

	Oid cur_obj_oid;
	CK(inverse_read_oid(db, inv_item, inv_obj_oid, cur_obj_oid));

	Collection *coll = 0;
	if (cur_obj_oid.isValid())
	  CK(inverse_get_collection(db, cur_obj_oid, coll));

	if (!coll)
	  {
	    CK(inverse_create_collection(db, inv_item, Oid::nullOid,
					 False, inv_obj_oid, coll));
	    CK(inverse_write_oid(db, inv_item, inv_obj_oid, coll->getOidC(),
				 inv_ctx));
	  }

	if (coll)
	  {
	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		    ("appending [4] %s to collection %s\n",
		     obj_oid.toString(), coll->getOid().toString()));
	    CK(coll->insert(obj_oid, True));
	    CK(inverse_coll_realize(coll));
	  }

	return Success;
      }

    if (op == invObjRemove)
      {
	if (!inv_obj_oid.isValid())
	  return Success;

	Oid cur_obj_oid;
	CK(inverse_read_oid(db, inv_item, inv_obj_oid, cur_obj_oid));

	if (cur_obj_oid.isValid())
	  {
	    Collection *coll;
	    CK(inverse_get_collection(db, cur_obj_oid, coll));

	    if (!coll)
	      return Success;

	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		    ("suppressing [6] %s from collection %s\n",
		     obj_oid.toString(), 
		     coll->getOid().toString()));
	    CK(coll->suppress(obj_oid, True));
	    CK(inverse_coll_realize(coll));
	    return Success;
	  }

	return Success;
      }

    abort();
    return Success;
  }
  
  // ---------------------------------------------------------------------------
  //
  // inverse_N_1
  //
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_N_1(Database *db, InvObjOp op,
			 const Attribute *inv_item,
			 const Oid &obj_oid,
			 const Oid &inv_obj_oid,
			 const InvCtx &inv_ctx) const
  {
    _trace_("inverse_N_1", op, this, inv_item, obj_oid, inv_obj_oid);
    Status status;

    if (op == invObjUpdate)
      {
	Oid old_inv_obj_oid;
	CK(inverse_read_oid(db, this, obj_oid, old_inv_obj_oid));
	if (old_inv_obj_oid.isValid() && old_inv_obj_oid != inv_obj_oid)
	  {
	    Collection *old_coll = 0;
	    CK(inverse_get_collection(db, old_inv_obj_oid, old_coll));
	    if (old_coll)
	      {
		OidArray oid_arr;
		CK(old_coll->getElements(oid_arr));

		old_coll->release(); old_coll = 0;
		// visiblement, on a un BUG
		for (int i = 0; i < oid_arr.getCount(); i++)
		  CK(inverse_write_oid(db, inv_item, oid_arr[i],
				       Oid::nullOid, inv_ctx));
	      }
	  }
      }

    if (op == invObjCreate || op == invObjUpdate)
      {
	if (!inv_obj_oid.isValid()) return Success;

	Collection *coll = 0;
	CK(inverse_get_collection(db, inv_obj_oid, coll));

	if (coll)
	  {
	    OidArray oid_arr;
	    CK(coll->getElements(oid_arr));
	    coll->release(); coll = 0;
	    for (int i = 0; i < oid_arr.getCount(); i++)
	      {
		Oid old_obj_oid;
		CK(inverse_read_oid(db, inv_item, oid_arr[i], old_obj_oid));
		if (old_obj_oid != obj_oid) // added the 30/8/99
		  {
		    if (old_obj_oid.isValid())
		      CK(inverse_write_oid(db, this, old_obj_oid,
					   Oid::nullOid, inv_ctx));
		    CK(inverse_write_oid(db, inv_item, oid_arr[i], obj_oid,
					 inv_ctx));
		  }
	      }
	  }

	return Success;
      }

    if (op == invObjRemove)
      {
	if (!inv_obj_oid.isValid()) return Success;

	Collection *coll;

	CK(inverse_get_collection(db, inv_obj_oid, coll));
	if (coll)
	  {
	    OidArray oid_arr;
	    CK(coll->getElements(oid_arr));
	    coll->release(); coll = 0;

	    for (int i = 0; i < oid_arr.getCount(); i++)
	      CK(inverse_write_oid(db, inv_item, oid_arr[i], Oid::nullOid,
				   inv_ctx));
	  }

	return Success;
      }

    abort();
    return Success;

    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // inverse_N_N
  //
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_N_N(Database *db,
			 InvObjOp op,
			 const Attribute *inv_item,
			 const Oid &obj_oid,
			 const Oid &inv_obj_oid,
			 const InvCtx &inv_ctx) const
  {
    _trace_("inverse_N_N", op, this, inv_item, obj_oid, inv_obj_oid);

    if (op == invObjUpdate)
      {
	Oid old_inv_obj_oid;
	CK(inverse_read_oid(db, this, obj_oid, old_inv_obj_oid));
	if (old_inv_obj_oid.isValid() && old_inv_obj_oid != inv_obj_oid)
	  {
	    Collection *old_coll;
	    CK(inverse_get_collection(db, old_inv_obj_oid, old_coll));
	    if (old_coll)
	      {
		OidArray oid_arr;
		CK(old_coll->getElements(oid_arr));
		for (int i = 0; i < oid_arr.getCount(); i++)
		  {
		    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
			    ("suppressing [7] %s from collection %s\n",
			     oid_arr[i].toString(), 
			     old_coll->getOid().toString()));
		    CK(old_coll->suppress(oid_arr[i], True));

		    Oid coll_oid;
		    CK(inverse_read_oid(db, inv_item, oid_arr[i], coll_oid));

		    if (coll_oid.isValid())
		      {
			Collection *inv_coll;
			CK(inverse_get_collection(db, coll_oid, inv_coll));
			if (inv_coll)
			  {
			    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
				    ("suppressing [8] %s from collection %s\n",
				     obj_oid.toString(), 
				     inv_coll->getOid().toString()));
			    CK(inv_coll->suppress(obj_oid, True));
			    CK(inverse_coll_realize(inv_coll));
			  }
		      }
		  }

		CK(inverse_coll_realize(old_coll));
	      }
	  }

	// optimisation added the 15/9/99
	if ((old_inv_obj_oid.isValid() && old_inv_obj_oid == inv_obj_oid) ||
	    !inv_obj_oid.isValid())
	  return Success;
      }

    // 15/9/99
    // je pense qu'en cas d'update, il n'y a rien a faire sauf
    // si la nouvelle collection est != de l'ancienne
    // (see above)
    if (op == invObjCreate || op == invObjUpdate)
      {
	if (!inv_obj_oid.isValid()) return Success;

	Collection *coll;
	CK(inverse_get_collection(db, inv_obj_oid, coll));

	if (!coll)
	  return Success;

	OidArray oid_arr;
	CK(coll->getElements(oid_arr));

	for (int i = 0; i < oid_arr.getCount(); i++)
	  {
	    Oid coll_obj_oid;
	    CK(inverse_read_oid(db, inv_item, oid_arr[i], coll_obj_oid));
	    Collection *inv_coll = 0;

	    if (coll_obj_oid.isValid())
	      CK(inverse_get_collection(db, coll_obj_oid, inv_coll));
	  
	    if (!inv_coll)
	      {
		CK(inverse_create_collection(db, inv_item, oid_arr[i],
					     True, oid_arr[i], inv_coll));
	      
		CK(inverse_write_oid(db, inv_item, oid_arr[i],
				     inv_coll->getOidC(), inv_ctx));
	      }

	    // the following line is a gag, I think!
	    // CK(coll->insert(oid_arr[i], True));
	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("appending [5] %s to collection %s\n",
					      obj_oid.toString(), 
					      inv_coll->getOid().toString()));
	    CK(inv_coll->insert(obj_oid, True));
	    CK(inverse_coll_realize(inv_coll));
	  }

	// no more necessary
	// CK(inverse_coll_realize(coll));
	coll->release(); coll = 0;
	return Success;
      }

    if (op == invObjRemove)
      {
	if (!inv_obj_oid.isValid()) return Success;

	Collection *coll;
	CK(inverse_get_collection(db, inv_obj_oid, coll));

	if (!coll)
	  return Success;

	OidArray oid_arr;
	CK(coll->getElements(oid_arr));

	for (int i = 0; i < oid_arr.getCount(); i++)
	  {
	    Oid coll_oid;
	    CK(inverse_read_oid(db, inv_item, oid_arr[i], coll_oid));
	    if (coll_oid.isValid())
	      {
		Collection *inv_coll;
		CK(inverse_get_collection(db, coll_oid, inv_coll));
		IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
			("suppressing [9] %s from collection %s\n",
			 oid_arr[i].toString(), 
			 coll->getOid().toString()));
		CK(coll->suppress(oid_arr[i], True));
		if (inv_coll)
		  {
		    IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
			    ("suppressing [10] %s from collection %s\n",
			     obj_oid.toString(), 
			     inv_coll->getOid().toString()));
		    CK(inv_coll->suppress(obj_oid, True));
		    CK(inverse_coll_realize(inv_coll));
		  }
	      }
	  }
      
	CK(inverse_coll_realize(coll));
	return Success;
      }

    abort();
    return Success;
  }  

  // ---------------------------------------------------------------------------
  //
  // Main routing method: inverse_realize
  // 
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_realize(Database *db, InvObjOp op, Data obj_idr,
			     const Oid &obj_oid) const
  {
    if (!inv_spec.item) return Success;

    InvCtx inv_ctx(obj_oid, obj_idr);
    const Attribute *inv_item = inv_spec.item;
    Oid inv_obj_oid = inverse_get_inv_obj_oid(obj_idr);
    Status status;
    Class *objcls = NULL;

    IDB_LOG(IDB_LOG_RELSHIP, ("Attribute::inverse_realize(name = \"%s::%s\", "
			      "invitem = \"%s::%s\", "
			      "op = %s. obj_oid = %s, inv_obj_oid = %s)\n", 
			      class_owner->getName(), name,
			      inv_item->getClassOwner()->getName(), inv_item->getName(),
			      get_op_name(op),
			      obj_oid.getString(), inv_obj_oid.getString()));

    get_object_class(db, obj_oid);
    get_object_class(db, inv_obj_oid);

    if (cls->asCollectionClass())
      {
	if (inv_item->cls->asCollectionClass())
	  status = inverse_N_N(db, op, inv_item, obj_oid, inv_obj_oid, inv_ctx);
	else
	  status = inverse_N_1(db, op, inv_item, obj_oid, inv_obj_oid, inv_ctx);
      }
    else
      {
	if (inv_item->cls->asCollectionClass())
	  status = inverse_1_N(db, op, inv_item, obj_oid, inv_obj_oid, inv_ctx);
	else
	  status = inverse_1_1(db, op, inv_item, obj_oid, inv_obj_oid, inv_ctx);
      }

    IDB_LOG(IDB_LOG_RELSHIP,
	    ("Attribute::inverse_realize(name = \"%s::%s\") ending with "
	     "status '%s'\n\n", class_owner->getName(), name,
	     (!status ? "success" : status->getDesc())));

    return status;
  }

  // ---------------------------------------------------------------------------
  //
  // Public API Methods
  // 
  // ---------------------------------------------------------------------------

  Status
  Attribute::createInverse_realize(Database *db,
				   Data obj_idr,
				   const Oid *obj_oid) const
  {
    return inverse_realize(db, invObjCreate, obj_idr, *obj_oid);
  }

  Status
  Attribute::updateInverse_realize(Database *db,
				   Data obj_idr,
				   const Oid *obj_oid) const
  {
    return inverse_realize(db, invObjUpdate, obj_idr, *obj_oid);
  }

  Status
  Attribute::removeInverse_realize(Database *db,
				   Data obj_idr,
				   const Oid *obj_oid) const
  {
    return inverse_realize(db, invObjRemove, obj_idr, *obj_oid);
  }

  // ---------------------------------------------------------------------------
  //
  // Low level utilily methods.
  // 
  // ---------------------------------------------------------------------------

  Status
  Attribute::inverse_get_collection(Database *db,
				    const Oid &inv_obj_oid,
				    Collection*& coll) const
  {
    coll = 0;

    Object *o;
    Status status = db->loadObject(&inv_obj_oid, &o);
  
    if (status)
      return status;
  
    if (!o->asCollection())
      {
	o->release();
	return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			       "%s::%s collection expected",
			       class_owner->getName(), name);
      }
  
    if (o->isRemoved())
      {
	o->release();
	return Success;
      }

    coll = o->asCollection();

    return Success;
  }

  extern std::string
  getAttrCollDefName(const Attribute *attr, const Oid &oid);

  Status
  Attribute::inverse_create_collection(Database *db,
				       const Attribute *inv_item,
				       const Oid &obj_oid,
				       Bool is_N_N,
				       const Oid &master_oid,
				       Collection *&coll) const
  {
    Status status = requalify(db, master_oid, inv_item);
    if (status) return status;

    if (!inv_item->isIndirect())
      {
	IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
		("creating literal collection '%s'\n", inv_item->getName()));

	Object *master_obj=0;
	coll = 0;
	status = db->loadObject(master_oid, master_obj);
	if (status) return status;
	assert(master_obj);
	status = inv_item->getValue(master_obj, (Data *)&coll, 1, 0);
	if (status) return status;
	assert(coll);
	if (!coll->getOidC().isValid())
	  {
	    status = coll->create_realize(RecMode::NoRecurs);
	    if (status) return status;
	    //printf("CREATING INV COLLECTION %s\n", coll->getOidC().toString());
	    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, 
		    ("have created literal collection %s for attribute %s::%s\n",
		     coll->getOidC().toString(),
		     inv_item->getClassOwner()->getName(),
		     inv_item->getName()));
	  }
	else
	  IDB_LOG(IDB_LOG_RELSHIP_DETAILS, 
		  ("literal collection %s was already created "
		   "for attribute %s::%s\n",
		   coll->getOidC().toString(),
		   inv_item->getClassOwner()->getName(),
		   inv_item->getName()));

	coll->incrRefCount();
	master_obj->release();
	return Success;
      }

    std::string collname = getAttrCollDefName(inv_item, obj_oid);

    Class *coll_class = const_cast<Class *>
      (inv_item->cls->asCollectionClass()->getCollClass());

    if (inv_item->cls->asCollSetClass())
      coll = new CollSet(db, collname.c_str(), coll_class);

    else if (inv_item->cls->asCollBagClass()) {
      //printf("inverse: creating bag collection\n");
      coll = new CollBag(db, collname.c_str(), coll_class);
    }

    else if (inv_item->cls->asCollArrayClass())
      coll = new CollArray(db, collname.c_str(), coll_class);

    else if (inv_item->cls->asCollListClass())
      coll = new CollList(db, collname.c_str(), coll_class);

    else {
      coll = 0;
      abort();
    }

    if (is_N_N)
      CollectionPeer::setInvOid(coll, obj_oid, inv_item->getNum());

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("creating collection '%s' "
				      "-> magorder %u\n",
				      inv_item->getName(),
				      inv_item->getMagOrder()));

    //printf("WARNING: should set something about collection implementation\n");
    //coll->setMagOrder(inv_item->getMagOrder());
    
    status = coll->realize();
    //Status status = coll->create_realize(RecMode::NoRecurs);

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("have created collection %s for "
				      "attribute %s::%s\n",
				      coll->getOid().toString(),
				      inv_item->getClassOwner()->getName(),
				      inv_item->getName()));

    if (is_N_N)
      IDB_LOG(IDB_LOG_RELSHIP_DETAILS,
	      ("setting inv_oid %s to collection %s\n",
	       obj_oid.toString(), coll->getOidC().toString()));

    return status;
  }

  Oid
  Attribute::inverse_get_inv_obj_oid(Data obj_idr) const
  {
    Oid inv_obj_oid;
    eyedbsm::x2h_oid(inv_obj_oid.getOid(), obj_idr + idr_poff);
    return inv_obj_oid;
  }

  Status
  Attribute::inverse_read_oid(Database *db, const Attribute *item,
			      const Oid &obj_oid,
			      Oid &old_obj_oid)
  {
    if (obj_oid.getDbid() != db->getDbid())
      return Exception::make(IDB_ATTRIBUTE_INVERSE_ERROR,
			     "%s does not belong to database #%d: relationships cannot cross databases", obj_oid.toString(), db->getDbid());

    eyedbsm::Oid toid;
    Status s = StatusMake(dataRead(db->getDbHandle(), item->idr_poff,
				   sizeof(eyedbsm::Oid),
				   (Data)&toid, 0,
				   obj_oid.getOid()));
    eyedbsm::x2h_oid(old_obj_oid.getOid(), &toid);

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("read oid -> item=%s, obj_oid=%s, "
				      "old_obj_oid=%s\n",
				      item->getName(), obj_oid.toString(),
				      old_obj_oid.toString()));
    return s;
  }


  Status
  Attribute::inverse_write_oid(Database *db, const Attribute *item,
			       const Oid &obj_oid,
			       const Oid &new_obj_oid,
			       const InvCtx &inv_ctx)
  {
    // With new indexes, this test is no more useful, but it was an
    // interesting optimisation
    if (item->isIndirect()) {
      Status status = item->updateIndexForInverse(db, obj_oid, new_obj_oid);
      if (status)
	return status;
    }

    IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("write oid -> item=%s, obj_oid=%s, "
				      "new_obj_oid=%s\n",
				      item->getName(), obj_oid.toString(),
				      new_obj_oid.toString()));

    InvOidContext::insert(obj_oid, item, new_obj_oid);

    eyedbsm::Oid toid;
    eyedbsm::h2x_oid(&toid, new_obj_oid.getOid());

    if (obj_oid == inv_ctx.oid) {
      //printf("writing in idr %p\n", inv_ctx.idr);
      //IDB_LOG(IDB_LOG_RELSHIP_DETAILS, ("writing oid on idr %p %p\n",
      //inv_ctx.idr, item->idr_poff));
      mcp(inv_ctx.idr+item->idr_poff, &toid, sizeof(Oid));
    }

    return StatusMake(dataWrite(db->getDbHandle(), item->idr_poff,
				sizeof(eyedbsm::Oid),
				(Data)&toid,
				obj_oid.getOid()));
  }
}
