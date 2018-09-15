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


#include "eyedb_p.h"
#include <assert.h>
#define DSPID 0
#include <sstream>
using std::ostringstream;

//
// needs:
//  hash table
// warning to memory leaks!
//

namespace eyedb {

static Bool dont_use_oql = getenv("EYEDBDONTUSEOQL") ? True : False;

static int
cmp(const void *o1, const void *o2)
{
  ClassConversion *c1 = *(ClassConversion **)o1;
  ClassConversion *c2 = *(ClassConversion **)o2;
  ClassUpdateType t1 = c1->getUpdtype();
  ClassUpdateType t2 = c2->getUpdtype();

  if (t1 == RMV_ATTR) {
    if (t2 == ADD_ATTR)
      return -1;
    if (t2 == CNV_ATTR)
      return -1;
    if (t2 == RMV_ATTR)
      return c2->getOffsetN() - c1->getOffsetN();
  }

  if (t1 == ADD_ATTR) {
    if (t2 == RMV_ATTR)
      return 1;
    if (t2 == CNV_ATTR || t2 == ADD_ATTR)
      return c1->getOffsetN() - c2->getOffsetN();
  }

  if (t1 == CNV_ATTR) {
    if (t2 == RMV_ATTR)
      return 1;
    if (t2 == CNV_ATTR || t2 == ADD_ATTR)
      return c1->getOffsetN() - c2->getOffsetN();
  }

  assert(0); // for now
  return 0;
}

static void
sort_array(ObjectArray *obj_arr_pt)
{
  Object **objs = new Object*[obj_arr_pt->getCount()];
  int cnt = obj_arr_pt->getCount();
  for (int i = 0; i < cnt; i++)
    objs[i] = const_cast<Object *>((*obj_arr_pt)[i]);
  qsort(objs, cnt, sizeof(Object *), cmp);
  obj_arr_pt->set(objs, cnt);
}

ClassConversion::Context::Context()
{
  cls = 0;
  next = 0;
}

ClassConversion::Context::~Context()
{
}

#define CLSCNV(X) ((ClassConversion *)(X))

/* static method */
Status
ClassConversion::getClass_(Database *_db, const Oid &ocloid,
			   const Class *&cls,
			   ClassConversion::Context *&conv_ctx,
			   Bool class_to_class)
{
  assert(!_db->isOpeningState());
  assert(ocloid.isValid());

  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("looking for old class %s\n", ocloid.toString()));
  conv_ctx = (Context *)_db->getConvCache().getObject(ocloid);

  if (conv_ctx && conv_ctx->cls) // added && conv_ctx->cls on the 5/06/01
    {
      cls = conv_ctx->cls;
      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("... found in cache %s\n", cls->getName()));
      return Success;
    }

  Status s;
  conv_ctx = new Context();
  ObjectArray *obj_arr_pt = &conv_ctx->obj_arr;

  if (dont_use_oql) {
    ObjectArray tmp_obj_arr;
    Iterator iter(_db->getSchema()->getClass("class_conversion"));
    s = iter.scan(tmp_obj_arr);
    if (s) return s;
    int cnt = tmp_obj_arr.getCount();
    Object **objs = new Object *[cnt];
    int n = 0;
    for (int i = 0; i < cnt; i++) {
      ClassConversion *cls_cnv = (ClassConversion *)tmp_obj_arr[i];
      if (cls_cnv->getOidO() == ocloid)
	objs[n++] = const_cast<Object *>(tmp_obj_arr[i]);
    }

    obj_arr_pt->set(objs, n);
    delete [] objs;
    //printf("dont_use_oql found %d\n", obj_arr_pt->getCount());
  }
  else {
    OQL q(_db, "select class_conversion.oid_o = %s", ocloid.toString());

    s = q.execute(*obj_arr_pt);
    if (s) return s;
    //printf("use_oql found %d\n", obj_arr_pt->getCount());
  }

  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("..... %d object(s) found\n", obj_arr_pt->getCount()));
  if (!obj_arr_pt->getCount())
    {
      delete conv_ctx;
      Class *xcls;
      s = _db->reloadObject(ocloid, (Object *&)xcls);
      if (!s && xcls->isRemoved()) {
	xcls->release();
	return Exception::make(IDB_ERROR, "dynamic schema module "
			       "internal error: class %s is removed",
			       ocloid.toString());
      }

      if (s)
	return s;
      return Exception::make(IDB_ERROR,
			     "dynamic schema module internal error: "
			     "class %s not found", ocloid.toString());
    }

  sort_array(obj_arr_pt);
  _db->getConvCache().insertObject(ocloid, conv_ctx);

  if (eyedblib::log_mask & IDB_LOG_SCHEMA_EVOLVE)
    for (int i = 0; i < obj_arr_pt->getCount(); i++)
      {
	IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("conv[%d] = ", i));
	(*obj_arr_pt)[i]->trace(stdout, CMTimeTrace);
    }

  ClassConversion *conv = CLSCNV((*obj_arr_pt)[0]);
  conv_ctx->cls = cls = _db->getSchema()->getClass(conv->getOidN());
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("new class oid=%s, class=%p\n", conv->getOidN().toString(), cls));

  if (cls || class_to_class)
    return Success;
    
  s = getClass_(_db, conv->getOidN(), cls, conv_ctx->next);
  if (s) return s;
  conv_ctx->cls = cls;
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("...class=%p\n", cls));
  return Success;
}

/* static method */
Size
ClassConversion::computeSize(ClassConversion::Context *conv_ctx,
				Size start_size)
{
  Size cur_size = start_size;
  Size max_size = cur_size;

  while (conv_ctx)
    {
      const ObjectArray *obj_arr_pt = &conv_ctx->obj_arr;
      
      for (int i = 0; i < obj_arr_pt->getCount(); i++)
	{
	  CLSCNV((*obj_arr_pt)[i])->computeSize(cur_size);
	  if (cur_size > max_size)
	    max_size = cur_size;
	}

      conv_ctx = conv_ctx->next;
    }

  return max_size;
}

static Status (*cnv_convert[NIL_CNV+1])(Database *db,
					      ClassConversion *clscnv,
					      Data in_idr,
					      Size &in_size);
static void (*cnv_computeSize[NIL_CNV+1])(ClassConversion *clscnv,
					     Size &cur_size);

void
ClassConversion::computeSize(Size &cur_size)
{
  switch(getUpdtype())
    {
    case ADD_ATTR:
      cur_size += getSizeN();
      break;

    case RMV_ATTR:
      cur_size -= getSizeN();
      break;

    case CNV_ATTR:
      cnv_computeSize[getCnvtype()](this, cur_size);
      break;

    default:
      assert(0);
      break;
    }
}

/* static method */
Status
ClassConversion::convert(Database *_db,
			    const ClassConversion::Context *conv_ctx,
			    Data in_idr, Size in_size)
{
  int n = 0;
  while (conv_ctx)
    {
      const ObjectArray *obj_arr_pt = &conv_ctx->obj_arr;
      
      int cnt = obj_arr_pt->getCount();
      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("converting #%d {\n", n++));
      for (int i = 0; i < cnt; i++)
	{
	  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\tconvert hint #%d\n", i));
	  Status s = CLSCNV((*obj_arr_pt)[i])->convert(_db, in_idr, in_size);
	  if (s) return s;
	}

      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("}\n\n"));
      conv_ctx = conv_ctx->next;
    }

  return Success;
}

/* instance method */
Status
ClassConversion::convert(Database *_db, Data in_idr, Size &in_size)
{
  switch(getUpdtype())
   {
    case ADD_ATTR: {
      Offset offset = getOffsetN();
      Data start = in_idr + offset;
      Size size = getSizeN();

      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tadd attribute %s::%s offsetN=%d sizeN=%d "
	     "in_size=%d sizemoved=%d\n",
	     getClsname().c_str(), getAttrname().c_str(), offset, size,
	     in_size, in_size - size - offset));

      // changed the 6/6/01
      // memmove(start + size, start, in_size - size - offset);
      memmove(start + size, start, in_size - offset);
      memset(start, 0, size);
      in_size += size;
      return Success;
    }

    case RMV_ATTR: {
      Offset offset = getOffsetN();
      Data start = in_idr + offset;
      Size size = getSizeN();
      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\trmv attribute %s::%s offsetN=%d sizeN=%d "
	     "in_size=%d sizemoved=%d\n",
	     getClsname().c_str(), getAttrname().c_str(), offset, size,
	     in_size, in_size - size - offset));

      memmove(start, start + size, in_size - size - offset);
      in_size -= size;
      return Success;
    }

   case CNV_ATTR: {
      Offset offset = getOffsetN();
      Size size = getSizeN();
      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tcnv attribute %s::%s offsetN=%d sizeN=%d "
	     "in_size=%d, srcdim=%d, destdim=%d\n",
	     getClsname().c_str(), getAttrname().c_str(), offset, size,
	     in_size, getSrcDim(), getDestDim()));
      return cnv_convert[getCnvtype()](_db, this, in_idr, in_size);
      //return Exception::make(IDB_ERROR, "converting attribute is not yet implemented");
   }

    default:
      return Exception::make(IDB_ERROR, "conversion %d is not yet implemented", getUpdtype());
   }
}

void
ClassConversion::userInitialize()
{
  cls_ = 0;
  attr_ = 0;
}

void
ClassConversion::userCopy(const Object &)
{
  ClassConversion::userInitialize();
}

void
ClassConversion::userGarbage()
{
}

//
// CNV_ATTR family conversions
//

// !!!!!! WARNING: duplicated code from attr.cc !!!!!!
#define IS_LOADED_MASK        (unsigned int)0x40000000
#define SZ_MASK               (unsigned int)0x80000000

#define IS_LOADED(X)          (((X) & IS_LOADED_MASK) ? True : False)
#define SET_IS_LOADED(X, B)   ((B) ? (((X) | IS_LOADED_MASK)) : ((X) & ~IS_LOADED_MASK))

#define IS_SIZE_CHANGED(X)     (((X) & SZ_MASK) ? True : False)
#define SET_SIZE_CHANGED(X, B) ((B) ? (((X) | SZ_MASK)) : ((X) & ~SZ_MASK))

#define CLEAN_SIZE(X)          ((X) & ~(SZ_MASK|IS_LOADED_MASK))

static int inline iniSize(int dim)
{
  if (!dim)
    return 0;
  return ((dim-1) >> 3) + 1;
}
// !!!!!! END OF WARNING !!!!!!

extern eyedbsm::DbHandle *IDB_get_se_DbHandle(Database *db);

//
// Macro definitions
//

static const char wrerr[] =  "schema flexibility process: to perform "
"some conversions database should be opened in read or read/write mode "
"not in strict read mode";

#define CNV_VD_SRC_DST(SRCTYP, DSTTYP) \
{ \
  Offset offset = clscnv->getOffsetN(); \
  Data start = in_idr + offset; \
 \
  Size count; \
  mcp(&count, start, sizeof(count)); \
  count = CLEAN_SIZE(count); \
 \
  Size inisize_s = iniSize(count); \
  Size wpsize_s = sizeof(SRCTYP) * count * (-dim_s) + inisize_s; \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_SRC_DST: count=%d, inisize_s=%d, wpsize_s=%d\n", count, inisize_s, wpsize_s)); \
  Oid oid_s; \
  mcp(&oid_s, start + sizeof(Size), sizeof(oid_s)); \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_SRC_DST OID=%s\n", oid_s.toString())); \
  unsigned char *pdata_s = new unsigned char[wpsize_s]; \
  eyedbsm::Status se_status; \
  eyedbsm::DbHandle *sedbh = IDB_get_se_DbHandle(db); \
  se_status = eyedbsm::objectRead(sedbh, 0, wpsize_s, pdata_s, eyedbsm::DefaultLock, 0, 0, \
			     oid_s.getOid()); \
 \
  if (se_status) { \
    delete[] pdata_s; \
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status)); \
  }   \
 \
  Size wpsize_d = sizeof(DSTTYP) * count * (-dim_d) + inisize_s; \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_SRC_DST: wpsize_ds=%d\n", wpsize_d)); \
  unsigned char *pdata_d = new unsigned char[wpsize_d]; \
  memcpy(pdata_d, pdata_s, inisize_s); \
   \
  Data start_s = pdata_s + inisize_s; \
  Data start_d = pdata_d + inisize_s; \
 \
  for (int i = 0; i < count; i++) {  \
    SRCTYP data_s; \
    mcp(&data_s, start_s, sizeof(SRCTYP)); \
    DSTTYP data_d = (DSTTYP)data_s; \
    mcp(start_d, &data_d, sizeof(DSTTYP)); \
    start_s += sizeof(SRCTYP); \
    start_d += sizeof(DSTTYP); \
  } \
 \
  delete[] pdata_s; \
  se_status = eyedbsm::objectSizeModify(sedbh, wpsize_d, eyedbsm::True, oid_s.getOid()); \
  if (se_status) { \
    delete[] pdata_d; \
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status)); \
  }   \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_SRC_DST: writing back\n")); \
 \
  if (!db->writeBackConvertedObjects()) \
    return Exception::make(IDB_ERROR, wrerr); \
 \
  se_status = eyedbsm::objectWrite(sedbh, 0, wpsize_d, pdata_d, oid_s.getOid()); \
    delete[] pdata_d; \
  if (se_status) \
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status)); \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_SRC_DST: writing back done\n")); \
  return Success; \
}

static int
get_top(Data start, int dim)
{
  for (int d = dim-1; d >= 0; d--)
    if (!Attribute::isNull(start, 1, d))
      return d+1;

  return 0;
}

#define CNV_VD_DST(SRCTYP, DSTTYP) \
{ \
  Offset offset = clscnv->getOffsetN(); \
  Data start = in_idr + offset; \
 \
  Size inisize_s = iniSize(dim_s); \
  Size wpsize_s = sizeof(SRCTYP) * dim_s + inisize_s; \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_DST: inisize_s=%d, wpsize_s=%d\n", inisize_s, wpsize_s)); \
 \
  eyedbsm::Status se_status; \
  eyedbsm::DbHandle *sedbh = IDB_get_se_DbHandle(db); \
 \
  int ndim_d = get_top(start, dim_s); \
  Oid oid_d; \
  if (ndim_d) { \
    Size inisize_d = iniSize(ndim_d); \
    Size wpsize_d = sizeof(DSTTYP) * ndim_d + inisize_d; \
    IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_DST: ndim_d=%d, wpsize_d=%d\n", ndim_d, wpsize_d)); \
    unsigned char *pdata_d = new unsigned char[wpsize_d]; \
    memcpy(pdata_d, start, inisize_d); \
     \
    Data start_s = start + inisize_s; \
    Data start_d = pdata_d + inisize_d; \
   \
    for (int i = 0; i < ndim_d; i++) {  \
      SRCTYP data_s; \
      mcp(&data_s, start_s, sizeof(SRCTYP)); \
      DSTTYP data_d = (DSTTYP)data_s; \
      IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_DST: DATA_S %f %d\n", data_s, data_d)); \
      mcp(start_d, &data_d, sizeof(DSTTYP)); \
      start_s += sizeof(SRCTYP); \
      start_d += sizeof(DSTTYP); \
    } \
   \
    se_status = eyedbsm::objectCreate(sedbh, pdata_d, wpsize_d, DSPID, oid_d.getOid()); \
    delete[] pdata_d; \
    if (se_status) \
      return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status)); \
   \
  } \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_VD_DST: new oid is %s\n", oid_d.toString())); \
 \
  memmove(start + sizeof(Size) + sizeof(Oid), \
          start + wpsize_s, \
   	  in_size - offset - wpsize_s); \
  \
  mcp(start, &ndim_d, sizeof(Size)); \
  mcp(start + sizeof(Size), oid_d.getOid(), sizeof(eyedbsm::Oid)); \
 \
  in_size += sizeof(Size) + sizeof(Oid) - wpsize_s; \
  return Success; \
}

#define CNV_VD_SRC(x, y) return Success

#define BASIC_CONVERT(SRCTYP, DSTTYP) \
  int dim_s = clscnv->getSrcDim(); \
  int dim_d = clscnv->getDestDim(); \
 \
  if (dim_s == 1 && dim_d == 1) { \
     Offset offset = clscnv->getOffsetN(); \
     Data start = in_idr + offset + 1; \
     SRCTYP data_s; \
     DSTTYP data_d; \
     \
     mcp(&data_s, start, sizeof(SRCTYP)); \
     memmove(start + sizeof(DSTTYP), start + sizeof(SRCTYP), \
             in_size - sizeof(SRCTYP) - offset - 1); \
     data_d = (DSTTYP)data_s; \
     ostringstream ostr; \
     ostr << "\t\tfrom: " << data_s << " to: " << data_d; \
     IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("%s\n", ostr.str().c_str())); \
     mcp(start, &data_d, sizeof(DSTTYP)); \
     in_size += sizeof(DSTTYP) - sizeof(SRCTYP); \
     return Success; \
  } \
 \
  if (dim_s < 0 && dim_d < 0) \
    CNV_VD_SRC_DST(SRCTYP, DSTTYP); \
 \
  if (dim_d < 0) \
    CNV_VD_DST(SRCTYP, DSTTYP); \
 \
  if (dim_s < 0) \
    CNV_VD_SRC(SRCTYP, DSTTYP); \
 \
  Offset offset = clscnv->getOffsetN(); \
  Data start = in_idr + offset; \
  Size inisize_s = iniSize(dim_s); \
  Size inisize_d = iniSize(dim_d); \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tinisizes %d %d\n", inisize_s, inisize_d)); \
  Data start_s = start + inisize_s; \
  Data start_d = start + inisize_d; \
  SRCTYP *pdata_s = new SRCTYP[dim_s]; \
 \
  for (int i = 0; i < dim_s; i++) { \
    mcp(&pdata_s[i], start_s, sizeof(SRCTYP)); \
    start_s += sizeof(SRCTYP); \
  } \
  \
  unsigned char *inidata_s; \
  if (dim_d < dim_s) { \
     inidata_s = new unsigned char[inisize_s]; \
     memcpy(inidata_s, start, inisize_s); \
  } \
 \
  memmove(start_d + dim_d * sizeof(DSTTYP), \
          start + inisize_s + dim_s * sizeof(SRCTYP), \
   	  in_size - dim_s * sizeof(SRCTYP) - offset - inisize_s); \
  \
  if (dim_d < dim_s) { \
    memcpy(start, inidata_s, inisize_s); \
    delete [] inidata_s; \
  } \
 \
  int dim = (dim_s < dim_d ? dim_s : dim_d); \
  for (int i = 0; i < dim; i++) { \
    DSTTYP data_d = (DSTTYP)pdata_s[i]; \
    mcp(start_d, &data_d, sizeof(DSTTYP)); \
    start_d += sizeof(DSTTYP); \
  } \
 \
  delete [] pdata_s; \
 \
  if (dim_d > dim_s) { \
    memset(start + inisize_s, 0, inisize_d - inisize_s); \
    memset(start_d, 0, (dim_d - dim_s) * sizeof(DSTTYP)); \
  } \
 \
  in_size += inisize_d - inisize_s + dim_d * sizeof(DSTTYP) - dim_s * sizeof(SRCTYP); \
  return Success

#define CNV_MAKE_CONVERT(CNVTYP, SRCTYP, DSTTYP) \
static Status \
CNVTYP##_convert(Database *db, ClassConversion *clscnv, Data in_idr, Size &in_size) \
{ \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\t" #CNVTYP " conversion\n")); \
  BASIC_CONVERT(SRCTYP, DSTTYP); \
}

// WANING DID NOT WORK FOR VARIABLE DIMENSION like x[][2]
// should take -dim_d and dim_s into account!!
// but the most simple is to forbid multi dimensionnal array conversion!
#define CNV_MAKE_COMPSIZE(CNVTYP, SRCTYP, DSTTYP) \
static void \
CNVTYP##_computeSize(ClassConversion *clscnv, Size &cur_size) \
{ \
  int dim_d = clscnv->getDestDim(); \
  int dim_s = clscnv->getSrcDim(); \
  if (dim_d > 0 && dim_s > 0) \
    cur_size += iniSize(dim_d) - iniSize(dim_s) + dim_d * sizeof(DSTTYP) - dim_s * sizeof(SRCTYP); \
  else if (dim_d < 0 && dim_s > 0) \
    cur_size += sizeof(Size) + sizeof(Oid) - iniSize(dim_s) - dim_s * sizeof(SRCTYP); \
}

#define CNV_MAKE(CNVTYP, SRCTYP, DSTTYP) \
  CNV_MAKE_CONVERT(CNVTYP, SRCTYP, DSTTYP) \
  CNV_MAKE_COMPSIZE(CNVTYP, SRCTYP, DSTTYP)

//
// CNV_NYI (i.e. Not Yet Implemented) macro family
//

#define CNV_MAKE_NYI_CONVERT(CNVTYP, SRCTYP, DSTTYP) \
static Status \
CNVTYP##_convert(Database *db, ClassConversion *clscnv, Data in_idr, Size &in_size) \
{ \
  return Exception::make(IDB_ERROR, #CNVTYP " conversion not yet implemented"); \
}

#define CNV_MAKE_NYI_COMPSIZE(CNVTYP, SRCTYP, DSTTYP) \
static void \
CNVTYP##_computeSize(ClassConversion *clscnv, Size &cur_size) \
{ \
  cur_size += 0; \
}

#define CNV_MAKE_NYI(CNVTYP, SRCTYP, DSTTYP) \
  CNV_MAKE_NYI_CONVERT(CNVTYP, SRCTYP, DSTTYP) \
  CNV_MAKE_NYI_COMPSIZE(CNVTYP, SRCTYP, DSTTYP)

//
// CNV_2STR macro family
//

#define VARS_INISZ 3
#define VARS_SZ   24
#define VARS_TSZ  (VARS_INISZ+VARS_SZ)
#define VARS_OFFSET (sizeof(Size) + sizeof(Oid))
#define VARS_DATA(IDR) ((IDR) + idr_poff + VARS_OFFSET)

#define CNV_MAKE_2STR_CONVERT(CNVTYP, SRCTYP) \
static Status \
CNVTYP##_convert(Database *db, ClassConversion *clscnv, Data in_idr, Size &in_size) \
{ \
  int dim_s = clscnv->getSrcDim(); \
  Offset offset = clscnv->getOffsetN(); \
  Data start = in_idr + offset; \
 \
  Size inisize_s = iniSize(dim_s); \
  Size wpsize_s = sizeof(SRCTYP) * dim_s + inisize_s; \
 \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_2STR: inisize_s=%d, wpsize_s=%d\n", inisize_s, wpsize_s)); \
 \
  eyedbsm::Status se_status; \
  eyedbsm::DbHandle *sedbh = IDB_get_se_DbHandle(db); \
 \
  int ndim_d = get_top(start, dim_s); \
  Oid oid_d; \
  unsigned char *pdata_d = 0; \
  if (ndim_d) { \
    SRCTYP data_s; \
    mcp(&data_s, start + inisize_s + (ndim_d - 1) * sizeof(SRCTYP), sizeof(SRCTYP)); \
    if (data_s) ndim_d++; \
    mcp(&data_s, start + inisize_s + ndim_d, sizeof(SRCTYP)); \
    Size inisize_d = iniSize(ndim_d); \
    Size wpsize_d = (ndim_d > VARS_SZ ? ndim_d + inisize_d : VARS_TSZ); \
    pdata_d = new unsigned char[wpsize_d]; \
   \
    mcp(pdata_d, start, inisize_d); \
    \
    Data start_s = start + inisize_s; \
    Data start_d = pdata_d + inisize_d; \
   \
    for (int i = 0; i < ndim_d; i++) {  \
      mcp(&data_s, start_s, sizeof(SRCTYP)); \
      *start_d++ = (char)data_s; \
      start_s += sizeof(SRCTYP); \
    } \
   \
    IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_2STR: ndim_d=%d, wpsize_d=%d, string '%s'\n", ndim_d, wpsize_d, pdata_d + inisize_d)); \
     \
    if (ndim_d > VARS_SZ) { \
      se_status = eyedbsm::objectCreate(sedbh, start, wpsize_d, DSPID, oid_d.getOid()); \
      delete [] pdata_d; \
      pdata_d = 0; \
      if (se_status) \
        return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status)); \
     } \
   } \
  else { \
     pdata_d = new unsigned char[VARS_TSZ]; \
     memset(pdata_d, 0, VARS_INISZ); \
  } \
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("\t\tCNV_2STR: new oid is %s\n", oid_d.toString())); \
 \
  memmove(start + VARS_OFFSET + VARS_TSZ, \
          start + wpsize_s, \
   	  in_size - offset - wpsize_s); \
  \
  if (pdata_d) { \
     memcpy(start + VARS_OFFSET, pdata_d, VARS_TSZ); \
     IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("pdata[0] = %d\n", pdata_d[0])); \
     delete [] pdata_d; \
  } \
 \
  mcp(start, &ndim_d, sizeof(Size)); \
  mcp(start + sizeof(Size), oid_d.getOid(), sizeof(eyedbsm::Oid)); \
 \
  in_size += VARS_OFFSET + VARS_TSZ - wpsize_s; \
  return Success; \
}

#define CNV_MAKE_2STR_COMPSIZE(CNVTYP, SRCTYP) \
static void \
CNVTYP##_computeSize(ClassConversion *clscnv, Size &cur_size) \
{ \
  int dim_s = clscnv->getSrcDim(); \
  cur_size += VARS_OFFSET + VARS_TSZ - iniSize(dim_s) - dim_s * sizeof(SRCTYP); \
}

#define CNV_MAKE_2STR(CNVTYP, SRCTYP) \
  CNV_MAKE_2STR_CONVERT(CNVTYP, SRCTYP) \
  CNV_MAKE_2STR_COMPSIZE(CNVTYP, SRCTYP)

//
// Macro instantiations
//

CNV_MAKE(INT16_TO_INT16, eyedblib::int16, eyedblib::int16)
CNV_MAKE(INT16_TO_INT32, eyedblib::int16, eyedblib::int32)
CNV_MAKE(INT16_TO_INT64, eyedblib::int16, eyedblib::int64)
CNV_MAKE(INT16_TO_FLOAT, eyedblib::int16, eyedblib::float64)
CNV_MAKE(INT16_TO_BYTE, eyedblib::int16, eyedblib::uchar)
CNV_MAKE(INT16_TO_CHAR, eyedblib::int16, eyedblib::uchar)

CNV_MAKE(INT32_TO_INT32, eyedblib::int32, eyedblib::int32)
CNV_MAKE(INT32_TO_INT16, eyedblib::int32, eyedblib::int16)
CNV_MAKE(INT32_TO_INT64, eyedblib::int32, eyedblib::int64)
CNV_MAKE(INT32_TO_FLOAT, eyedblib::int32, eyedblib::float64)
CNV_MAKE(INT32_TO_BYTE, eyedblib::int32, eyedblib::uchar)
CNV_MAKE(INT32_TO_CHAR, eyedblib::int32, eyedblib::uchar)

CNV_MAKE(INT64_TO_INT64, eyedblib::int64, eyedblib::int64)
CNV_MAKE(INT64_TO_INT16, eyedblib::int64, eyedblib::int16)
CNV_MAKE(INT64_TO_INT32, eyedblib::int64, eyedblib::int32)
CNV_MAKE(INT64_TO_FLOAT, eyedblib::int64, eyedblib::float64)
CNV_MAKE(INT64_TO_BYTE, eyedblib::int64, eyedblib::uchar)
CNV_MAKE(INT64_TO_CHAR, eyedblib::int64, char)

CNV_MAKE(FLOAT_TO_FLOAT, eyedblib::float64, eyedblib::float64)
CNV_MAKE(FLOAT_TO_INT16, eyedblib::float64, eyedblib::int16)
CNV_MAKE(FLOAT_TO_INT32, eyedblib::float64, eyedblib::int32)
CNV_MAKE(FLOAT_TO_INT64, eyedblib::float64, eyedblib::int64)
CNV_MAKE(FLOAT_TO_BYTE, eyedblib::float64, eyedblib::uchar)
CNV_MAKE(FLOAT_TO_CHAR, eyedblib::float64, char)

CNV_MAKE(CHAR_TO_CHAR, eyedblib::uchar, eyedblib::uchar)
CNV_MAKE(CHAR_TO_INT16, eyedblib::uchar, eyedblib::int16)
CNV_MAKE(CHAR_TO_INT32, eyedblib::uchar, eyedblib::int32)
CNV_MAKE(CHAR_TO_INT64, eyedblib::uchar, eyedblib::int64)
CNV_MAKE(CHAR_TO_FLOAT, eyedblib::uchar, eyedblib::float64)
CNV_MAKE(CHAR_TO_BYTE, eyedblib::uchar, eyedblib::uchar) // could be nil

CNV_MAKE_NYI(BYTE_TO_BYTE, eyedblib::uchar, eyedblib::uchar)
CNV_MAKE(BYTE_TO_INT16, eyedblib::uchar, eyedblib::int16)
CNV_MAKE(BYTE_TO_INT32, eyedblib::uchar, eyedblib::int32)
CNV_MAKE(BYTE_TO_INT64, eyedblib::uchar, eyedblib::int64)
CNV_MAKE(BYTE_TO_FLOAT, eyedblib::uchar, eyedblib::float64)
CNV_MAKE(BYTE_TO_CHAR, eyedblib::uchar, eyedblib::uchar) // could be nil

// ? conversions to enum should be manual ?
CNV_MAKE_NYI(INT16_TO_ENUM, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(INT32_TO_ENUM, eyedblib::int32, eyedblib::int32)
CNV_MAKE_NYI(INT64_TO_ENUM, eyedblib::int64, eyedblib::int32)
CNV_MAKE_NYI(FLOAT_TO_ENUM, float, eyedblib::int32)
CNV_MAKE_NYI(CHAR_TO_ENUM, char, eyedblib::int32)
CNV_MAKE_NYI(BYTE_TO_ENUM, eyedblib::uchar, eyedblib::int32)

// ? conversions from enum should be manual ?
CNV_MAKE_NYI(ENUM_TO_ENUM, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_INT16, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_INT32, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_INT64, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_FLOAT, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_CHAR, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ENUM_TO_BYTE, eyedblib::int16, eyedblib::int32)

// to string conversions
CNV_MAKE_2STR(CHAR_TO_STRING, eyedblib::uchar)
CNV_MAKE_2STR(INT16_TO_STRING, eyedblib::int16)
CNV_MAKE_2STR(INT32_TO_STRING, eyedblib::int32)
CNV_MAKE_2STR(INT64_TO_STRING, eyedblib::int64)
CNV_MAKE_2STR(FLOAT_TO_STRING, eyedblib::float64)
CNV_MAKE_2STR(BYTE_TO_STRING, eyedblib::uchar)

// collection conversions should be manual
CNV_MAKE_NYI(SET_TO_BAG, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(SET_TO_ARRAY, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(SET_TO_LIST, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(BAG_TO_SET, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(BAG_TO_ARRAY, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(BAG_TO_LIST, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ARRAY_TO_BAG, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ARRAY_TO_SET, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(ARRAY_TO_LIST, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(LIST_TO_BAG, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(LIST_TO_ARRAY, eyedblib::int16, eyedblib::int32)
CNV_MAKE_NYI(LIST_TO_SET, eyedblib::int16, eyedblib::int32)

//
// CLASS_TO_CLASS conversions
//

// fixed size arrays: source and destination

static Status
CLASS_TO_CLASS_convert_1(Database *db, ClassConversion *clscnv,
			 ClassConversion::Context *conv_ctx,
			 int dim_s, int dim_d,
			 Data in_idr, Size &in_size)
{
  Size osize = clscnv->getSizeO();
  Size nsize = clscnv->getSizeN();
  Offset offset = clscnv->getOffsetN();
  Data start = in_idr + offset;
  Size nxsize = ClassConversion::computeSize(conv_ctx, osize);

  int ndim_d = (dim_d < dim_s ? dim_d : dim_s);
  unsigned char **pdata = new unsigned char*[ndim_d];
  Data start_s = start;

  Size asize = nxsize + IDB_OBJ_HEAD_SIZE;
  Size bsize = osize + IDB_OBJ_HEAD_SIZE;

  for (int i = 0; i < ndim_d; i++) {
    pdata[i] = new unsigned char[asize];
    memcpy(pdata[i] + IDB_OBJ_HEAD_SIZE, start_s, osize);
    Status s = ClassConversion::convert(db, conv_ctx, pdata[i], bsize);
    if (s) {
      for (int j = 0; j < i; j++)
	delete [] pdata[j];
      delete [] pdata;
      return s;
    }
    start_s += osize;
  }

  memmove(start + dim_d * nsize,
	  start + dim_s * osize,
	  in_size - dim_s * osize - offset);

  Data start_d = start;
  for (int i = 0; i < ndim_d; i++) {
    memcpy(start_d, pdata[i] + IDB_OBJ_HEAD_SIZE, nsize);
    start_d += nsize;
    delete [] pdata[i];
  }

  delete [] pdata;

  for (int i = dim_s; i < dim_d; i++) {
    memset(start_d, 0, nsize);
    start_d += nsize;
  }

  in_size += dim_d * nsize - dim_s * osize;
  return Success;
}

// variable size arrays: source and destination

static Status
CLASS_TO_CLASS_convert_2(Database *db, ClassConversion *clscnv,
			 ClassConversion::Context *conv_ctx,
			 int dim_s, int dim_d,
			 Data in_idr, Size &in_size)
{
  Size osize = clscnv->getSizeO();
  Size nsize = clscnv->getSizeN();
  Offset offset = clscnv->getOffsetN();
  Data start = in_idr + offset;
  Size nxsize = ClassConversion::computeSize(conv_ctx, osize);

  Size count;
  mcp(&count, start, sizeof(count));
  count = CLEAN_SIZE(count);

  if (!count)
    return Success;

  Size wpsize_s = osize * count * (-dim_s);
  Oid oid_s;
  mcp(&oid_s, start + sizeof(Size), sizeof(oid_s));

  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("convert_2 count=%d\n", count));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("convert_2 wpsize_s=%d\n", wpsize_s));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("convert_2 oid_s=%s\n", oid_s.toString()));

  unsigned char *pdata_s = new unsigned char[wpsize_s];
  eyedbsm::Status se_status;
  eyedbsm::DbHandle *sedbh = IDB_get_se_DbHandle(db);
  se_status = eyedbsm::objectRead(sedbh, 0, wpsize_s, pdata_s, eyedbsm::DefaultLock, 0, 0,
			    oid_s.getOid());
  if (se_status) {
    delete[] pdata_s;
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status));
  } 

  int ndim_d = count;
  unsigned char **pdata = new unsigned char*[ndim_d];
  Data start_s = pdata_s;

  Size asize = nxsize + IDB_OBJ_HEAD_SIZE;
  Size bsize = osize + IDB_OBJ_HEAD_SIZE;

  for (int i = 0; i < ndim_d; i++) {
    pdata[i] = new unsigned char[asize];
    memcpy(pdata[i] + IDB_OBJ_HEAD_SIZE, start_s , osize);
    Status s = ClassConversion::convert(db, conv_ctx, pdata[i], bsize);
    if (s) {
      for (int j = 0; j < i; j++)
	delete [] pdata[j];
      delete [] pdata;
      delete[] pdata_s;
      return s;
    }
    start_s += osize;
  }

  Size wpsize_d = nsize * count * (-dim_d);
  unsigned char *pdata_d = new unsigned char[wpsize_d];

  Data start_d = pdata_d;
  for (int i = 0; i < ndim_d; i++) {
    memcpy(start_d, pdata[i] + IDB_OBJ_HEAD_SIZE, nsize);
    start_d += nsize;
    delete [] pdata[i];
  }

  delete[] pdata_s;
  delete [] pdata;

  for (int i = dim_s; i < dim_d; i++) {
    memset(start_d, 0, nsize);
    start_d += nsize;
  }

  if (!db->writeBackConvertedObjects()) {
    delete[] pdata_d;
    return Exception::make(IDB_ERROR, wrerr);
  }

  se_status = eyedbsm::objectSizeModify(sedbh, wpsize_d, eyedbsm::True, oid_s.getOid());
  if (se_status) {
    delete[] pdata_d;
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status));
  }

  se_status = eyedbsm::objectWrite(sedbh, 0, wpsize_d, pdata_d, oid_s.getOid());
  delete[] pdata_d;
  if (se_status)
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status));

  return Success;
}

// fixed size source array and variable size destination array

static Status
CLASS_TO_CLASS_convert_3(Database *db, ClassConversion *clscnv,
			 ClassConversion::Context *conv_ctx,
			 int dim_s, int dim_d,
			 Data in_idr, Size &in_size)
{
  Size osize = clscnv->getSizeO();
  Size nsize = clscnv->getSizeN();
  Offset offset = clscnv->getOffsetN();
  Data start = in_idr + offset;
  Size nxsize = ClassConversion::computeSize(conv_ctx, osize);

  unsigned char **pdata = new unsigned char*[dim_s];
  Data start_s = start;

  Size asize = nxsize + IDB_OBJ_HEAD_SIZE;
  Size bsize = osize + IDB_OBJ_HEAD_SIZE;
  Size wpsize_s = osize * dim_s;

  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("convert_3 offset=%d\n", offset));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("dim_s=%d\n", dim_s));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("dim_d=%d\n", dim_d));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("osize=%d\n", osize));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("nsize=%d\n", nsize));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("nxsize=%d\n", nxsize));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("asize=%d\n", asize));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("bsize=%d\n", bsize));

  for (int i = 0; i < dim_s; i++) {
    pdata[i] = new unsigned char[asize];
    memcpy(pdata[i] + IDB_OBJ_HEAD_SIZE, start_s, osize);
    Status s = ClassConversion::convert(db, conv_ctx, pdata[i], bsize);
    if (s) {
      for (int j = 0; j < i; j++)
	delete [] pdata[j];
      delete [] pdata;
      return s;
    }
    start_s += osize;
  }

  Size wpsize_d = nsize * dim_s;
  unsigned char *pdata_d = new unsigned char[wpsize_d];
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("wpsize_d %d %d\n", wpsize_s, wpsize_d));

  Data start_d = pdata_d;
  for (int i = 0; i < dim_s; i++) {
    memcpy(start_d, pdata[i] + IDB_OBJ_HEAD_SIZE, nsize);
    start_d += nsize;
    delete [] pdata[i];
  }

  delete [] pdata;

  eyedbsm::Status se_status;
  eyedbsm::DbHandle *sedbh = IDB_get_se_DbHandle(db);
  Oid oid_d;
  se_status = eyedbsm::objectCreate(sedbh, pdata_d, wpsize_d, DSPID,
			      oid_d.getOid());
  delete[] pdata_d;
  if (se_status)
    return Exception::make(IDB_ERROR, eyedbsm::statusGet(se_status));

  memmove(start + sizeof(Size) + sizeof(Oid),
          start + wpsize_s,
   	  in_size - offset - wpsize_s);

  mcp(start, &dim_s, sizeof(Size));
  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("oid_d = %s\n", oid_d.toString()));
  mcp(start + sizeof(Size), oid_d.getOid(), sizeof(eyedbsm::Oid));

  in_size += sizeof(Size) + sizeof(Oid) - dim_s * osize;
  return Success;
}

static Status
CLASS_TO_CLASS_convert(Database *db, ClassConversion *clscnv, Data in_idr, Size &in_size)
{
  const Class *cls = 0;
  ClassConversion::Context *conv_ctx = 0;
  Status s = ClassConversion::getClass_(db, clscnv->getRoidO(), cls,
					conv_ctx, True);
  if (s) return s;

  IDB_LOG(IDB_LOG_SCHEMA_EVOLVE, ("CLASS_TO_CLASS = %s\n",
				  (cls ? cls->getName() : "<unknown>")));

  if (!conv_ctx)
    return Exception::make(IDB_ERROR, "internal fatal error: cannot find "
			      "conversion method for class %s",
			      clscnv->getRoidO().toString());

  int dim_s = clscnv->getSrcDim();
  int dim_d = clscnv->getDestDim();

  if (dim_s > 0 && dim_d > 0)
    return CLASS_TO_CLASS_convert_1(db, clscnv, conv_ctx, dim_s, dim_d,
				    in_idr, in_size);

  if (dim_s < 0 && dim_d < 0)
    return CLASS_TO_CLASS_convert_2(db, clscnv, conv_ctx, dim_s, dim_d,
				    in_idr, in_size);

  if (dim_s > 0 && dim_d < 0)
    return CLASS_TO_CLASS_convert_3(db, clscnv, conv_ctx, dim_s, dim_d,
				    in_idr, in_size);

  return Exception::make(IDB_ERROR, "CLASS_TO_CLASS conversion: "
			    "variable size to non variable size is not "
			    "supported");
}

static void
CLASS_TO_CLASS_computeSize(ClassConversion *clscnv, Size &cur_size)
{
  int dim_s = clscnv->getSrcDim();
  int dim_d = clscnv->getDestDim();

  if (dim_s > 0 && dim_d > 0)
    cur_size += dim_d * clscnv->getSizeN() - dim_s * clscnv->getSizeO();
  else if (dim_s < 0 && dim_d < 0)
    ;
  else if (dim_s > 0 && dim_d < 0)
    cur_size += sizeof(Size) + sizeof(Oid) - dim_s * clscnv->getSizeO();
}

// should be manual
CNV_MAKE_NYI(USER_CNV, eyedblib::int16, eyedblib::int32)

static Status
NIL_CNV_convert(Database *db, ClassConversion *clscnv, Data in_idr,
		Size &in_size)
{
  return Success;
}

static void
NIL_CNV_computeSize(ClassConversion *clscnv, Size &cur_size)
{
}

//CNV_MAKE_NYI(NIL_CNV, eyedblib::int16, eyedblib::int32)

void
ClassConversion::init()
{
  static Bool init = False;

  if (init) return;

  cnv_convert[INT16_TO_INT16] = INT16_TO_INT16_convert;
  cnv_convert[INT16_TO_INT32] = INT16_TO_INT32_convert;
  cnv_convert[INT16_TO_INT64] = INT16_TO_INT64_convert;
  cnv_convert[INT16_TO_FLOAT] = INT16_TO_FLOAT_convert;
  cnv_convert[INT16_TO_BYTE] = INT16_TO_BYTE_convert;
  cnv_convert[INT16_TO_CHAR] = INT16_TO_CHAR_convert;
  cnv_convert[INT16_TO_ENUM] = INT16_TO_ENUM_convert;

  cnv_convert[INT32_TO_INT32] = INT32_TO_INT32_convert;
  cnv_convert[INT32_TO_INT16] = INT32_TO_INT16_convert;
  cnv_convert[INT32_TO_INT64] = INT32_TO_INT64_convert;
  cnv_convert[INT32_TO_FLOAT] = INT32_TO_FLOAT_convert;
  cnv_convert[INT32_TO_BYTE] = INT32_TO_BYTE_convert;
  cnv_convert[INT32_TO_CHAR] = INT32_TO_CHAR_convert;
  cnv_convert[INT32_TO_ENUM] = INT32_TO_ENUM_convert;

  cnv_convert[INT64_TO_INT64] = INT64_TO_INT64_convert;
  cnv_convert[INT64_TO_INT16] = INT64_TO_INT16_convert;
  cnv_convert[INT64_TO_INT32] = INT64_TO_INT32_convert;
  cnv_convert[INT64_TO_FLOAT] = INT64_TO_FLOAT_convert;
  cnv_convert[INT64_TO_BYTE] = INT64_TO_BYTE_convert;
  cnv_convert[INT64_TO_CHAR] = INT64_TO_CHAR_convert;
  cnv_convert[INT64_TO_ENUM] = INT64_TO_ENUM_convert;

  cnv_convert[FLOAT_TO_FLOAT] = FLOAT_TO_FLOAT_convert;
  cnv_convert[FLOAT_TO_INT16] = FLOAT_TO_INT16_convert;
  cnv_convert[FLOAT_TO_INT32] = FLOAT_TO_INT32_convert;
  cnv_convert[FLOAT_TO_INT64] = FLOAT_TO_INT64_convert;
  cnv_convert[FLOAT_TO_BYTE] = FLOAT_TO_BYTE_convert;
  cnv_convert[FLOAT_TO_CHAR] = FLOAT_TO_CHAR_convert;
  cnv_convert[FLOAT_TO_ENUM] = FLOAT_TO_ENUM_convert;

  cnv_convert[CHAR_TO_CHAR] = CHAR_TO_CHAR_convert;
  cnv_convert[CHAR_TO_INT16] = CHAR_TO_INT16_convert;
  cnv_convert[CHAR_TO_INT32] = CHAR_TO_INT32_convert;
  cnv_convert[CHAR_TO_INT64] = CHAR_TO_INT64_convert;
  cnv_convert[CHAR_TO_FLOAT] = CHAR_TO_FLOAT_convert;
  cnv_convert[CHAR_TO_BYTE] = CHAR_TO_BYTE_convert;
  cnv_convert[CHAR_TO_ENUM] = CHAR_TO_ENUM_convert;

  cnv_convert[BYTE_TO_BYTE] = BYTE_TO_BYTE_convert;
  cnv_convert[BYTE_TO_INT16] = BYTE_TO_INT16_convert;
  cnv_convert[BYTE_TO_INT32] = BYTE_TO_INT32_convert;
  cnv_convert[BYTE_TO_INT64] = BYTE_TO_INT64_convert;
  cnv_convert[BYTE_TO_FLOAT] = BYTE_TO_FLOAT_convert;
  cnv_convert[BYTE_TO_CHAR] = BYTE_TO_CHAR_convert;
  cnv_convert[BYTE_TO_ENUM] = BYTE_TO_ENUM_convert;

  cnv_convert[ENUM_TO_ENUM] = ENUM_TO_ENUM_convert;
  cnv_convert[ENUM_TO_INT16] = ENUM_TO_INT16_convert;
  cnv_convert[ENUM_TO_INT32] = ENUM_TO_INT32_convert;
  cnv_convert[ENUM_TO_INT64] = ENUM_TO_INT64_convert;
  cnv_convert[ENUM_TO_FLOAT] = ENUM_TO_FLOAT_convert;
  cnv_convert[ENUM_TO_CHAR] = ENUM_TO_CHAR_convert;
  cnv_convert[ENUM_TO_BYTE] = ENUM_TO_BYTE_convert;

  cnv_convert[CHAR_TO_STRING] = CHAR_TO_STRING_convert;
  cnv_convert[INT16_TO_STRING] = INT16_TO_STRING_convert;
  cnv_convert[INT32_TO_STRING] = INT32_TO_STRING_convert;
  cnv_convert[INT64_TO_STRING] = INT64_TO_STRING_convert;
  cnv_convert[FLOAT_TO_STRING] = FLOAT_TO_STRING_convert;
  cnv_convert[BYTE_TO_STRING] = BYTE_TO_STRING_convert;

  cnv_convert[SET_TO_BAG] = SET_TO_BAG_convert;
  cnv_convert[SET_TO_ARRAY] = SET_TO_ARRAY_convert;
  cnv_convert[SET_TO_LIST] = SET_TO_LIST_convert;
  cnv_convert[BAG_TO_SET] = BAG_TO_SET_convert;
  cnv_convert[BAG_TO_ARRAY] = BAG_TO_ARRAY_convert;
  cnv_convert[BAG_TO_LIST] = BAG_TO_LIST_convert;
  cnv_convert[ARRAY_TO_BAG] = ARRAY_TO_BAG_convert;
  cnv_convert[ARRAY_TO_SET] = ARRAY_TO_SET_convert;
  cnv_convert[ARRAY_TO_LIST] = ARRAY_TO_LIST_convert;
  cnv_convert[LIST_TO_BAG] = LIST_TO_BAG_convert;
  cnv_convert[LIST_TO_ARRAY] = LIST_TO_ARRAY_convert;
  cnv_convert[LIST_TO_SET] = LIST_TO_SET_convert;

  cnv_convert[CLASS_TO_CLASS] = CLASS_TO_CLASS_convert;

  cnv_convert[USER_CNV] = USER_CNV_convert;
  cnv_convert[NIL_CNV] = NIL_CNV_convert;

  cnv_computeSize[INT16_TO_INT16] = INT16_TO_INT16_computeSize;
  cnv_computeSize[INT16_TO_INT32] = INT16_TO_INT32_computeSize;
  cnv_computeSize[INT16_TO_INT64] = INT16_TO_INT64_computeSize;
  cnv_computeSize[INT16_TO_FLOAT] = INT16_TO_FLOAT_computeSize;
  cnv_computeSize[INT16_TO_BYTE] = INT16_TO_BYTE_computeSize;
  cnv_computeSize[INT16_TO_CHAR] = INT16_TO_CHAR_computeSize;
  cnv_computeSize[INT16_TO_ENUM] = INT16_TO_ENUM_computeSize;

  cnv_computeSize[INT32_TO_INT32] = INT32_TO_INT32_computeSize;
  cnv_computeSize[INT32_TO_INT16] = INT32_TO_INT16_computeSize;
  cnv_computeSize[INT32_TO_INT64] = INT32_TO_INT64_computeSize;
  cnv_computeSize[INT32_TO_FLOAT] = INT32_TO_FLOAT_computeSize;
  cnv_computeSize[INT32_TO_BYTE] = INT32_TO_BYTE_computeSize;
  cnv_computeSize[INT32_TO_CHAR] = INT32_TO_CHAR_computeSize;
  cnv_computeSize[INT32_TO_ENUM] = INT32_TO_ENUM_computeSize;

  cnv_computeSize[INT64_TO_INT64] = INT64_TO_INT64_computeSize;
  cnv_computeSize[INT64_TO_INT16] = INT64_TO_INT16_computeSize;
  cnv_computeSize[INT64_TO_INT32] = INT64_TO_INT32_computeSize;
  cnv_computeSize[INT64_TO_FLOAT] = INT64_TO_FLOAT_computeSize;
  cnv_computeSize[INT64_TO_BYTE] = INT64_TO_BYTE_computeSize;
  cnv_computeSize[INT64_TO_CHAR] = INT64_TO_CHAR_computeSize;
  cnv_computeSize[INT64_TO_ENUM] = INT64_TO_ENUM_computeSize;

  cnv_computeSize[FLOAT_TO_FLOAT] = FLOAT_TO_FLOAT_computeSize;
  cnv_computeSize[FLOAT_TO_INT16] = FLOAT_TO_INT16_computeSize;
  cnv_computeSize[FLOAT_TO_INT32] = FLOAT_TO_INT32_computeSize;
  cnv_computeSize[FLOAT_TO_INT64] = FLOAT_TO_INT64_computeSize;
  cnv_computeSize[FLOAT_TO_BYTE] = FLOAT_TO_BYTE_computeSize;
  cnv_computeSize[FLOAT_TO_CHAR] = FLOAT_TO_CHAR_computeSize;
  cnv_computeSize[FLOAT_TO_ENUM] = FLOAT_TO_ENUM_computeSize;

  cnv_computeSize[CHAR_TO_CHAR] = CHAR_TO_CHAR_computeSize;
  cnv_computeSize[CHAR_TO_INT16] = CHAR_TO_INT16_computeSize;
  cnv_computeSize[CHAR_TO_INT32] = CHAR_TO_INT32_computeSize;
  cnv_computeSize[CHAR_TO_INT64] = CHAR_TO_INT64_computeSize;
  cnv_computeSize[CHAR_TO_FLOAT] = CHAR_TO_FLOAT_computeSize;
  cnv_computeSize[CHAR_TO_BYTE] = CHAR_TO_BYTE_computeSize;
  cnv_computeSize[CHAR_TO_ENUM] = CHAR_TO_ENUM_computeSize;

  cnv_computeSize[BYTE_TO_BYTE] = BYTE_TO_BYTE_computeSize;
  cnv_computeSize[BYTE_TO_INT16] = BYTE_TO_INT16_computeSize;
  cnv_computeSize[BYTE_TO_INT32] = BYTE_TO_INT32_computeSize;
  cnv_computeSize[BYTE_TO_INT64] = BYTE_TO_INT64_computeSize;
  cnv_computeSize[BYTE_TO_FLOAT] = BYTE_TO_FLOAT_computeSize;
  cnv_computeSize[BYTE_TO_CHAR] = BYTE_TO_CHAR_computeSize;
  cnv_computeSize[BYTE_TO_ENUM] = BYTE_TO_ENUM_computeSize;

  cnv_computeSize[ENUM_TO_ENUM] = ENUM_TO_ENUM_computeSize;
  cnv_computeSize[ENUM_TO_INT16] = ENUM_TO_INT16_computeSize;
  cnv_computeSize[ENUM_TO_INT32] = ENUM_TO_INT32_computeSize;
  cnv_computeSize[ENUM_TO_INT64] = ENUM_TO_INT64_computeSize;
  cnv_computeSize[ENUM_TO_FLOAT] = ENUM_TO_FLOAT_computeSize;
  cnv_computeSize[ENUM_TO_CHAR] = ENUM_TO_CHAR_computeSize;
  cnv_computeSize[ENUM_TO_BYTE] = ENUM_TO_BYTE_computeSize;

  cnv_computeSize[CHAR_TO_STRING] = CHAR_TO_STRING_computeSize;
  cnv_computeSize[INT16_TO_STRING] = INT16_TO_STRING_computeSize;
  cnv_computeSize[INT32_TO_STRING] = INT32_TO_STRING_computeSize;
  cnv_computeSize[INT64_TO_STRING] = INT64_TO_STRING_computeSize;
  cnv_computeSize[FLOAT_TO_STRING] = FLOAT_TO_STRING_computeSize;
  cnv_computeSize[BYTE_TO_STRING] = BYTE_TO_STRING_computeSize;

  cnv_computeSize[SET_TO_BAG] = SET_TO_BAG_computeSize;
  cnv_computeSize[SET_TO_ARRAY] = SET_TO_ARRAY_computeSize;
  cnv_computeSize[SET_TO_LIST] = SET_TO_LIST_computeSize;
  cnv_computeSize[BAG_TO_SET] = BAG_TO_SET_computeSize;
  cnv_computeSize[BAG_TO_ARRAY] = BAG_TO_ARRAY_computeSize;
  cnv_computeSize[BAG_TO_LIST] = BAG_TO_LIST_computeSize;
  cnv_computeSize[ARRAY_TO_BAG] = ARRAY_TO_BAG_computeSize;
  cnv_computeSize[ARRAY_TO_SET] = ARRAY_TO_SET_computeSize;
  cnv_computeSize[ARRAY_TO_LIST] = ARRAY_TO_LIST_computeSize;
  cnv_computeSize[LIST_TO_BAG] = LIST_TO_BAG_computeSize;
  cnv_computeSize[LIST_TO_ARRAY] = LIST_TO_ARRAY_computeSize;
  cnv_computeSize[LIST_TO_SET] = LIST_TO_SET_computeSize;

  cnv_computeSize[CLASS_TO_CLASS] = CLASS_TO_CLASS_computeSize;

  cnv_computeSize[USER_CNV] = USER_CNV_computeSize;
  cnv_computeSize[NIL_CNV] = NIL_CNV_computeSize;

  init = True;
}

void
ClassConversion::_release()
{
}
}
