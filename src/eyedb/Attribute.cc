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
#include "CollectionBE.h"
#include "Attribute_p.h"
#include "eyedb/gbxcyctx.h"
#include "eyedblib/strutils.h"

static void stop_imm2() { }

//#define IDX_CTX_SHORTCUT

#define ATTR_COMP_SET_OID_OFFSET 40
#define CODE_ENDOFF
//#define CODE_ENDOFF_TRACE
#define NEW_RELEASE
//#define TRACE_IDX

//#define TRACK_BUG1

#define _64BITS_PORTAGE

#ifdef _64BITS_PORTAGE
#define SIZEOFOBJECT 8
#define SIZEOFDATA   8
#else
#define SIZEOFOBJECT sizeof(Object *)
#define SIZEOFDATA sizeof(Data)
#endif

//#define MULTI_IDX_OPTIM1
#define MULTI_IDX_CACHE_OPTIM
#define MULTI_IDX_BAG
#define MAGORDER_VERSION 20409

#define VARS_INISZ 3
#define VARS_SZ   24
#define VARS_TSZ  (VARS_INISZ+VARS_SZ)

static bool eyedb_support_stack = getenv("EYEDB_SUPPORT_STACK") ? true : false;

//#define E_XDR_TRACE

namespace eyedbsm {
  extern int hidx_gccnt;
}


namespace eyedb {

#define E_XDR_1

#define getDB(O) (Database *)(O)->getClass()->getDatabase()

  //#define VARS_COND(DB) IDBBOOL(isString())
#define VARS_COND(DB) is_string
#define VARS_OFFSET (sizeof(Size) + sizeof(Oid))
#define VARS_DATA(IDR) ((IDR) + idr_poff + VARS_OFFSET)

  unsigned char Attribute::idxNull    = '0';
  unsigned char Attribute::idxNotNull = '1';

  int Attribute::composedMode = -1;
  static AttrIdxContext null_idx_ctx;

#define CHECK_OBJ(O)							\
  do {									\
    CHK_OBJ(O);								\
    if ((O)->isRemoved())						\
      return Exception::make(IDB_ERROR, "object '%s' is removed.",	\
			     (O)->getOid().toString());			\
  } while(0)

#define CHECK_OID(OID)							\
  do {									\
    Bool isremoved;							\
    Status s = db->isRemoved(OID, isremoved);				\
    if (!s && isremoved)						\
      return Exception::make(IDB_ERROR, "object '%s' is removed.",	\
 			     OID.toString());				\
  } while(0)

#define CHECK_RTTYPE(O)							\
  do {									\
    if (dyn_class_owner &&						\
	(O)->getClass()->getOid() != dyn_class_owner->getOid() &&	\
	dyn_class_owner->getOid().isValid() && (O)->getClass()->getOid().isValid()) \
      return Exception::make(IDB_ERROR, "runtime type error: object is of " \
			     "type '%s' not of type '%s'",		\
			     (O)->getClass()->getName(),		\
			     dyn_class_owner->getName());		\
  } while(0)

  const char *Attribute::template_name = "<template>";

  const char *Attribute::log_item_entry_fmt = 
    "index=%s oid=%s, data='%s', index=%d (%s)\n";

  const char *Attribute::log_comp_entry_fmt = 
    "index=%s oid=%s, data='%s' (%s)\n";

  //#define FRONT_END
#include "misc.h"
#include <iostream>
#include <assert.h>

#define ATTR_COMPLETE()				\
  if (!cls)					\
    ((Attribute *)this)->cls =			\
      db->getSchema()->getClass(oid_cl);	\
  if (!class_owner)				\
    ((Attribute *)this)->class_owner =		\
      db->getSchema()->getClass(oid_cl_own)

#define check_range(from, nb)			\
  {						\
    Status status = checkRange(from, nb);	\
    if (status) return status;			\
  }

#define check_var_range(o, from, nb, psize)	\
  {						\
    Status status;				\
    status = checkVarRange(o, from, nb, psize); \
    if (status != Success)			\
      return status;				\
  }

#define IS_BASIC_ENUM(CL) ((CL)->asBasicClass() || (CL)->asEnumClass())
#define IS_STRING(CL)     ((CL)->asCharClass() && !isIndirect() &&	\
			   typmod.ndims == 1 && typmod.dims[0])

#define PRINT_NULL(FD) fprintf(FD, NullString)
  /*
    #define ATTRNAME(IDX) ((IDX) ? (IDX)->getAttrname() : \
    (const char *)(std::string(class_owner->getName()) + "::" + name))
  */

#define ATTRPATH(IDX) ((IDX) ? (IDX)->getAttrpath().c_str() :		\
		       (std::string(class_owner->getName()) + "::" + name).c_str())

  static int inline iniSize(int sz)
  {
    if (!sz)
      return 0;
    return ((sz-1) >> 3) + 1;
  }

  static int inline revSize(int sz)
  {
    return (sz << 3)/9;
  }

#define SHIFT(X) ((X) >> 3)
#define UNSHIFT(X) ((X) << 3)
#define MASK(FROM, SFROM) (1 << (7 - (FROM - (UNSHIFT(SFROM)))))
#define SMASK(FROM, SFROM) (0xff >> (FROM - (UNSHIFT(SFROM))))
#define EMASK(TO, STO)     (0xff << (7 - (TO - (UNSHIFT(STO)))))

#define fpos(P, OFF) ((char const *)(P) + (OFF))

  //#define NEW_NOTNULL_TRACE

  static eyedbsm::Boolean
  idx_precmp(void const * p, void const * q, eyedbsm::Idx::KeyType const * type,
	     int & r)
  {
    if (type->offset > 0) {
#ifdef NEW_NOTNULL_TRACE
      printf("comparing null char %d vs. %d\n", *(unsigned char *)p,
	     *(unsigned char *)q);
#endif
      r = (*(unsigned char *)p - *(unsigned char *)q);
      if (r)
	return eyedbsm::True;
    }
  
    if (type->offset == (1 + sizeof(eyedblib::int32))) {
      int pind, qind;
      eyedblib_mcp(&pind, fpos(p, 1), sizeof(eyedblib::int32));
      eyedblib_mcp(&qind, fpos(q, 1), sizeof(eyedblib::int32));
#ifdef NEW_NOTNULL_TRACE
      printf("comparing index %d vs. %d\n", pind, qind);
#endif
      r = pind - qind;
      if (r)
	return eyedbsm::True;
    }

    return eyedbsm::False;
  }

  Bool
  Attribute::isNull(Data inidata, int nb, int from)
  {
    if (!nb)
      return True;

    int sfrom = SHIFT(from);
    if (nb == 1)
      {
	unsigned char *s = inidata + sfrom;
	unsigned mask = MASK(from, sfrom);
	if (*s & mask)
	  return False;
	return True;
      }

    int to = from+nb-1;
    int sto = SHIFT(to);

    unsigned char *s = inidata + sfrom;
    unsigned char *e = inidata + sto;

    unsigned int smask = SMASK(from, sfrom);
    unsigned int emask = EMASK(to, sto);

    if (*s & smask)
      return False;

    if (*e & emask)
      return False;

    for (unsigned char *p = s+1; p < e; p++)
      if (*p)
	return False;

    return True;
  }

  Bool
  Attribute::isNull(Data inidata, const TypeModifier *tmod)
  {
    return isNull(inidata, tmod->pdims, 0);
  }

  static inline void
  setInit(Data inidata, int nb, int from, int val)
  {
    if (!nb)
      return;

    int sfrom = SHIFT(from);
    if (nb == 1)
      {
	unsigned char *s = inidata + sfrom;
	unsigned mask = MASK(from, sfrom);
	if (val)
	  *s |= mask;
	else
	  *s &= ~mask;
	return;
      }
    int to = from+nb-1;
    int sto = SHIFT(to);

    unsigned char *s = inidata + sfrom;
    unsigned char *e = inidata + sto;

    unsigned int smask = SMASK(from, sfrom);
    unsigned int emask = EMASK(to, sto);

    if (val)
      {
	*s |= smask;
	*e |= emask;

	for (unsigned char *p = s+1; p < e; p++)
	  *p = 0xff;
      }
    else
      {
	*s &= ~smask;
	*e &= ~emask;

	for (unsigned char *p = s+1; p < e; p++)
	  *p = 0;
      }
  }

  int Attribute::iniCompute(const Database *, int sz, Data &pdata, Data &inidata) const
  {
    if (isIndirect())
      {
	inidata = 0;
	return 0;
      }

    inidata = pdata;
    pdata += idr_inisize;
    return idr_inisize;
  }

  int AttrVarDim::iniCompute(const Database *db, int sz, Data &pdata, Data &inidata) const
  {
    if (is_basic_enum)
      {
	int size = iniSize(sz);
	inidata = pdata;
	pdata += size;
	return size;
      }

    inidata = 0;
    return 0;
  }

#define IS_LOADED_MASK        (unsigned int)0x40000000
#define SZ_MASK               (unsigned int)0x80000000

#define IS_LOADED(X)          (((X) & IS_LOADED_MASK) ? True : False)
#define SET_IS_LOADED(X, B)   ((B) ? (((X) | IS_LOADED_MASK)) : ((X) & ~IS_LOADED_MASK))

#define IS_SIZE_CHANGED(X)     (((X) & SZ_MASK) ? True : False)
#define SET_SIZE_CHANGED(X, B) ((B) ? (((X) | SZ_MASK)) : ((X) & ~SZ_MASK))

#define CLEAN_SIZE(X)          ((X) & ~(SZ_MASK|IS_LOADED_MASK))

  static inline int
  set_oid_realize(Data pdata, const Oid *oid, int nb, int from)
  {
    // needs XDR ? yes !
#ifdef E_XDR_1
    for (int n = 0; n < nb; n++) {
      if (eyedbsm::cmp_oid(pdata + ((from+n) * sizeof(Oid)), oid[n].getOid())) {
	for (int m = 0; m < nb; m++)
	  eyedbsm::h2x_oid(pdata + ((from+m) * sizeof(Oid)), oid[m].getOid());
	return 1;
      }
    }
#else
    if (memcmp(pdata + (from * sizeof(Oid)), oid,  nb * sizeof(Oid))) {
      memcpy(pdata + (from * sizeof(Oid)), oid,  nb * sizeof(Oid));
      return 1;
    }
#endif

    return 0;
  }

  static const char incomp_fmt[] = "incomplete type '%s' for attribute '%s' #%d in agregat class '%s'";

  inline static Class *
  get_class(Schema *sch, const Class *cls)
  {
    if (!cls)
      return 0;
    return sch->getClass(cls->getOid());
  }

  Attribute *makeAttribute(const Attribute *agreg)
  {
    return makeAttribute(agreg, agreg->getClass(), agreg->getClassOwner(),
			 agreg->getDynClassOwner(), agreg->getNum());
  }

  Attribute *makeAttribute(const Attribute *agreg,
			   const Class *_cls,
			   const Class *_class_owner,
			   const Class *_dyn_class_owner,
			   int num)
  {
    if (agreg->isNative())
      return new AttrNative((AttrNative *)agreg, _cls, _class_owner,
			    _dyn_class_owner, num);

    Bool is_vardim = agreg->isVarDim(), is_indirect = agreg->isIndirect();

    if (!is_vardim && !is_indirect)
      return new AttrDirect(agreg, _cls, _class_owner, _dyn_class_owner, num);

    if (!is_vardim && is_indirect)
      return new AttrIndirect(agreg, _cls, _class_owner, _dyn_class_owner, num);

    if (is_vardim && !is_indirect)
      return new AttrVarDim(agreg, _cls, _class_owner, _dyn_class_owner, num);

    if (is_vardim && is_indirect)
      return new AttrIndirectVarDim(agreg, _cls, _class_owner, _dyn_class_owner, num);

    return 0;
  }

  Attribute *Attribute::clone(Database *db) const
  {
    if (!db)
      return eyedb::makeAttribute(this);

    Schema *sch = db->getSchema();
    return makeAttribute(this,
			 get_class(sch, getClass()),
			 get_class(sch, getClassOwner()),
			 get_class(sch, getDynClassOwner()),
			 num);
  }

  Attribute *makeAttribute(Database *db,
			   Data data, Offset *offset,
			   const Class *_dyn_class_owner,
			   int num)
  {
    eyedblib::int16 code;

#ifdef CODE_ENDOFF
    eyedblib::int32 endoff;
    int32_decode(data, offset, &endoff);
#endif
    int16_decode(data, offset, &code);

    *offset -= sizeof(eyedblib::int16);
#ifdef CODE_ENDOFF
    *offset -= sizeof(eyedblib::int32);
#endif

    switch(code)
      {
      case AttrDirect_Code:
	return new AttrDirect(db, data, offset, _dyn_class_owner, num);

      case AttrIndirect_Code:
	return new AttrIndirect(db, data, offset, _dyn_class_owner, num);

      case AttrVarDim_Code:
	return new AttrVarDim(db, data, offset, _dyn_class_owner, num);

      case AttrIndirectVarDim_Code:
	return new AttrIndirectVarDim(db, data, offset, _dyn_class_owner, num);

      default:
	fprintf(stderr, "unknown attribute code %d\n", code);
	assert(0);
	return 0;
      }
  }

  TypeModifier::TypeModifier()
  {
    ndims = 0;
    maxdims = 0;
    pdims = 0;
    mode = (TypeModifier::_mode)0;
    dims = NULL;
  }

  TypeModifier::TypeModifier(const TypeModifier &typmod)
  {
    *this = typmod;
  }

  TypeModifier& TypeModifier::operator=(const TypeModifier &typmod)
  {
    ndims = typmod.ndims;
    pdims = typmod.pdims;
    maxdims = typmod.maxdims;
    mode = typmod.mode;

    if (ndims) {
      dims = (int *)malloc(sizeof(int) * ndims);
      memcpy(dims, typmod.dims, sizeof(int) * ndims);
    }
    else
      dims = NULL;

    return *this;
  }

  TypeModifier TypeModifier::make(Bool isRef, int ndims, int *dims)
  {
    TypeModifier typmod;

    unsigned short md = (isRef ? TypeModifier::_Indirect : 0);

    if (ndims)
      {
	typmod.ndims   = ndims;
	typmod.dims    = (int *)malloc(sizeof(int) * ndims);

	memcpy(typmod.dims, dims, sizeof(int)*ndims);

	typmod.pdims = 1;
	typmod.maxdims = 1;

	for (int i = 0; i < ndims; i++)
	  {
	    if (dims[i] < 0)
	      {
		md |= TypeModifier::_VarDim;
		typmod.maxdims *= -dims[i];
	      }
	    else
	      {
		typmod.pdims *= dims[i];
		typmod.maxdims *= dims[i];
	      }
	  }
      }
    else
      {
	typmod.ndims   = 0;
	typmod.dims    = 0;
	typmod.pdims   = 1;
	typmod.maxdims = 1;
      }

    typmod.mode = (TypeModifier::_mode)md;

    return typmod;
  }

  Status TypeModifier::codeIDR(Data* data, Offset *offset,
			       Size *alloc_size)
  {
    //  : mode
    eyedblib::int16 k = (eyedblib::int16)mode;
    int16_code(data, offset, alloc_size, &k);
    //  : pdims
    int32_code(data, offset, alloc_size, &pdims);
    //  : maxdims
    int32_code(data, offset, alloc_size, &maxdims);
    //  : ndims
    int16_code(data, offset, alloc_size, &ndims);

    //  : dims[]
    for (int i = 0; i < ndims; i++)
      int32_code(data, offset, alloc_size, &dims[i]);

    return Success;
  }

  Status TypeModifier::decodeIDR(Data data, Offset *offset)
  {
    memset(&ndims, 0, sizeof(eyedblib::int32)); // strange, isn't it?

    //  : mode
    eyedblib::int16 k;

    int16_decode(data, offset, &k);
    mode = (enum _mode)k;
    //  : pdims
    int32_decode(data, offset, &pdims);
    //  : maxdims
    int32_decode(data, offset, &maxdims);
    //  : ndims
    int16_decode(data, offset, &ndims);

    dims = (int *)malloc(sizeof(int) * ndims);

    for (int i = 0; i < ndims; i++)
      int32_decode(data, offset,  &dims[i]);
    return Success;
  }

  Bool
  TypeModifier::compare(const TypeModifier *tmod) const
  {
    if (ndims != tmod->ndims)
      return False;

    if (pdims != tmod->pdims)
      return False;

    if (maxdims != tmod->maxdims)
      return False;

    if (mode != tmod->mode)
      return False;

    for (int i = 0; i < ndims; i++)
      if (dims[i] != tmod->dims[i])
	return False;

    return True;
  }

  TypeModifier::~TypeModifier()
  {
    free(dims);
  }

  //
  // Attribute methods
  //

  Status Attribute::setCardinalityConstraint(CardinalityConstraint *_card)
  {
#ifdef CARD_TRACE
    printf("Attribute::setCardinalityConstraint %s\n", name);
#endif
    if (cls && !cls->asCollectionClass())
      return Exception::make(IDB_ERROR,
			     "cannot set a cardinality constraint on item %s::%s (not a collection)", class_owner->getName(), name);

    card = _card;
    return Success;
  }

  Status
  Attribute::getClassOid(Database *db, const Class *ocls,
			 const Oid &cls_oid, Oid &oid)
  {
    oid = ocls->getOid();
    if (oid.isValid())
      return Success;

    if (cls_oid.isValid()) {
      oid = cls_oid;
      return Success;
    }

    Class *xcls = db->getSchema()->getClass(ocls->getName());
    if (!xcls)
      return Success;
    /*
      return Exception::make(IDB_ATTRIBUTE_ERROR,
      "attribute %s::%s: class %s is unknown",
      class_owner->getName(), name, ocls->getName());
    */

    oid = xcls->getOid();

    /*
      if (!oid.isValid())
      return Exception::make(IDB_ATTRIBUTE_ERROR,
      "attribute %s::%s: class %s has no valid oid",
      class_owner->getName(), name, ocls->getName());
    */

    return Success;
  }

  Status Attribute::codeIDR(Database *db, Data* data,
			    Offset *offset, Size *alloc_size)
  {
    if (isNative())
      return Success;

#ifdef CODE_ENDOFF
    Offset soffset = *offset;
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&endoff);
#endif
    // code
    int16_code(data, offset, alloc_size, &code);

    Oid oid;
    Status s = getClassOid(db, cls, oid_cl, oid);
    if (s) return s;
    oid_code(data, offset, alloc_size, oid.getOid());

    /*
      printf("coding classowner %p %s::%s -> %s vs. %s\n", this,
      (class_owner ? class_owner->getName() : "<unknown>"), name,
      oid_cl_own.getString(), class_owner->getOid().toString());
    */

    // oid de la cls owner
    s = getClassOid(db, class_owner, oid_cl_own, oid);
    if (s) return s;
    oid_code(data, offset, alloc_size, oid.getOid());

    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&magorder);

    // this has been added the 15/02/99 for class actual update.
    if (!inv_spec.oid_cl.isValid() && inv_spec.item)
      {
	inv_spec.oid_cl = inv_spec.item->getClassOwner()->getOid();
	inv_spec.num    = inv_spec.item->getNum();
      }

    // oid de la cls inverse
    oid_code(data, offset, alloc_size, inv_spec.oid_cl.getOid());
    // num de l'attribute inverse
    int16_code(data, offset, alloc_size, &inv_spec.num);
    // is_basic_enum
    char_code(data, offset, alloc_size, &is_basic_enum);
    // is_string
    char_code(data, offset, alloc_size, &is_string);
    // dspid
    int16_code(data, offset, alloc_size, &dspid);

    /*
      printf("coding component set %p %s::%s -> %s\n", this,
      (class_owner ? class_owner->getName() : "<unknown>"), name,
      attr_comp_set_oid.getString());
    */

#ifdef CODE_ENDOFF_TRACE
    printf("ATTRIBUTE OFFSET %d [%d]\n", *offset, *offset - soffset);
#endif
    assert(*offset - soffset == ATTR_COMP_SET_OID_OFFSET);
    oid_code(data, offset, alloc_size, attr_comp_set_oid.getOid());

    // name
    string_code(data, offset, alloc_size, name);

    // idr_inisize
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_inisize);
    // idr_poff
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_poff);
    // idr_item_psize
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_item_psize);
    // idr_psize
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_psize);
    // idr_voff
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_voff);
    // idr_item_vsize
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_item_vsize);
    // idr_vsize
    int32_code(data, offset, alloc_size, (eyedblib::int32 *)&idr_vsize);

    typmod.codeIDR(data, offset, alloc_size);

    endoff = *offset;
#ifdef CODE_ENDOFF
    int32_code(data, &soffset, alloc_size, (eyedblib::int32 *)&endoff);
#ifdef CODE_ENDOFF_TRACE
    printf("NAME %s -> endof %d\n", name, endoff);
#endif
#endif

    return Success;
  }

  void Attribute::reportAttrCompSetOid(Offset *offset, Data idr) const
  {
    Data oidr = idr;
    Offset endo;
    Offset soffset = *offset;
    int32_decode(idr, &soffset, (eyedblib::int32 *)&endo);
#ifdef CODE_ENDOFF_TRACE
    printf("reportAttrCompSetOid(%d, %p, %s, %s)\n", endo, idr, attr_comp_set_oid.toString(), name);
#endif
    soffset = *offset + ATTR_COMP_SET_OID_OFFSET;
    Size alloc_size = soffset + sizeof(eyedbsm::Oid);
    oid_code(&idr, &soffset, &alloc_size, attr_comp_set_oid.getOid());
    assert(idr == oidr);
    *offset = endo;
  }

  void Attribute::codeClassOid(Data data, Offset *offset)
  {
    *offset += sizeof(eyedblib::int16);
    Size alloc_size = endoff; //  *offset + 2 * sizeof(Oid);
    oid_code(&data, offset, &alloc_size, cls->getOid().getOid());
    oid_code(&data, offset, &alloc_size, class_owner->getOid().getOid());

    /*
      printf("have coded oid %s::%s -> %s %s class_owner=%p\n",
      class_owner->getName(), name,
      class_owner->getOid().toString(), cls->getOid().toString(),
      class_owner);
    */

    *offset = endoff;
  }

  Attribute::Attribute(Database *db, Data data, Offset *offset,
		       const Class *_dyn_class_owner, int n)
  {
    // MIND: must decode class_owner!!
    // must decode code
#ifdef CODE_ENDOFF
    int32_decode(data, offset, (eyedblib::int32 *)&endoff);
#endif
    int16_decode(data, offset, &code);

    // oid de la class
    cls = 0;
    eyedbsm::Oid _oid;
    oid_decode(data, offset, &_oid);
    oid_cl.setOid(_oid);
    class_owner = 0;
    dyn_class_owner = _dyn_class_owner;
    oid_decode(data, offset, &_oid);
    oid_cl_own.setOid(_oid);

    int32_decode(data, offset, (int *)&magorder);

    // oid de la cls inverse
    //  mset(&inv_spec, 0, sizeof(inv_spec));
    oid_decode(data, offset, &_oid);
    inv_spec.oid_cl.setOid(_oid);
    // num de l'attribute inverse
    int16_decode(data, offset, &inv_spec.num);
    // is_basic_enum
    char_decode(data, offset, &is_basic_enum);
    // is_string
    char_decode(data, offset, &is_string);
    // dspid
    int16_decode(data, offset, &dspid);
    
    oid_decode(data, offset, &_oid);
    attr_comp_set_oid.setOid(_oid);

    // name
    char *s;
    string_decode(data, offset, &s);
    name = strdup(s);

#ifdef CODE_ENDOFF_TRACE
    printf("Decoding endof = %d %s\n", endoff, name);
#endif
    /*
      printf("CLASS %s %p class=%s class_owner=%s\n",
      name, dyn_class_owner, oid_cl.toString(),
      oid_cl_own.toString());
    */

    /*
      printf("decoding component set %p %s::%s -> %s\n", this,
      (class_owner ? class_owner->getName() : "<unknown>"), name,
      attr_comp_set_oid.getString());
    */
    // idr_inisize
    int32_decode(data, offset, (eyedblib::int32 *)&idr_inisize);
    // idr_poff
    int32_decode(data, offset, (eyedblib::int32 *)&idr_poff);
    // idr_item_psize
    int32_decode(data, offset, (eyedblib::int32 *)&idr_item_psize);
    // idr_psize
    int32_decode(data, offset, (eyedblib::int32 *)&idr_psize);
    // idr_voff
    int32_decode(data, offset, (eyedblib::int32 *)&idr_voff);
    // idr_item_vsize
    int32_decode(data, offset, (eyedblib::int32 *)&idr_item_vsize);
    // idr_vsize
    int32_decode(data, offset, (eyedblib::int32 *)&idr_vsize);

    typmod.decodeIDR(data, offset);

    num = n;
    attr_comp_set = 0;

    card = NULL;
    user_data = 0;
    dataspace = 0;
  }

  void Attribute::setItem(Class *tp, const char *s,
			  Bool isRef, int ndims, int *dims,
			  char _is_basic_enum,
			  char _is_string)
  {
    //mset(&inv_spec, 0, sizeof(inv_spec));
    cls = tp;
    assert(cls || _is_basic_enum >= 0);
    if (cls)
      is_basic_enum = IS_BASIC_ENUM(cls);
    else
      is_basic_enum = _is_basic_enum;

    name = strdup(s);

    typmod = TypeModifier::make(isRef, ndims, dims);
    assert(cls || _is_string >= 0);
    if (cls)
      is_string = IS_STRING(cls);
    else
      is_string = _is_string;

    attr_comp_set = 0;
    attr_comp_set_oid.invalidate();

    magorder = 0;
    oid_cl.invalidate();
    oid_cl_own.invalidate();
    card = NULL;
    user_data = 0;
    dataspace = 0;
    dspid = Dataspace::DefaultDspid;
  }

  void
  Attribute::revert(Bool)
  {
  }

  Bool
  Attribute::isVarDim() const
  {
    return (typmod.mode == TypeModifier::VarDim ||
	    typmod.mode == TypeModifier::IndirectVarDim) ? True : False;
  }

  Bool
  Attribute::isIndirect() const
  {
    return (typmod.mode == TypeModifier::Indirect ||
	    typmod.mode == TypeModifier::IndirectVarDim) ? True : False;
  }

  /*
    int
    Attribute::isString() const
    {
    if (!getClass()->asCharClass() || isIndirect())
    return 0;

    if (typmod.ndims != 1)
    return 0;

    return typmod.dims[0];
    }
  */

  const TypeModifier &Attribute::getTypeModifier() const
  {
    return typmod;
  }

  Bool
  Attribute::compare(Database *db, const Attribute *item) const
  {
    if (num != item->num)
      return False;

    if (strcmp(name, item->name))
      return False;

    if (!typmod.compare(&item->typmod))
      return False;
  
    ATTR_COMPLETE();

    if (!cls || !item->cls)
      return False; // should raise an exception ??

    if (!cls->compare(item->cls))
      return False;

    return True;
  }

  Bool
  Attribute::compare(Database *db, const Attribute *item,
		     Bool compClassOwner,
		     Bool compNum,
		     Bool compName,
		     Bool inDepth) const
  {
    if (compNum && num != item->num)
      return False;

    if (compName && strcmp(name, item->name))
      return False;

    if (!typmod.compare(&item->typmod))
      return False;
  
    ATTR_COMPLETE();

    if (!inDepth && isIndirect()) {
      if (!cls->compare_l(item->cls))
	return False;
    }
    else {
      if (!cls->compare(item->cls, compClassOwner, compNum, compName, inDepth))
	return False;
    }

    if (compClassOwner) {
      if (!inDepth) {
	if (!class_owner->compare_l(item->class_owner))
	  return False;
      }
      else {
	if (!class_owner->compare(item->class_owner, compClassOwner, compNum,
				  compName, inDepth))
	  return False;
      }
    }

    return True;
  }

  Attribute::Attribute(Class *tp, const char *s,
		       Bool isRef, int ndims, int *dims)
  {
    setItem(tp, s, isRef, ndims, dims);
  }

  Attribute::Attribute(Class *tp, const char *s, int dim)
  {
    setItem(tp, s, False, (dim ? 1 : 0), &dim);
  }

  const char *Attribute::getName() const
  {
    return name;
  }

  Attribute::Attribute(const Attribute *attr,
		       const Class *_cls,
		       const Class *_class_owner,
		       const Class *_dyn_class_owner,
		       int n)
  {
    // 10/10/00
    // l'appel à setItem est-il vraiment indispensable ???
    // je ne pense pas!!
#if 1
    name = strdup(attr->name);
    typmod = TypeModifier::make(attr->isIndirect(), attr->typmod.ndims,
				attr->typmod.dims);
    cls = (Class *)(_cls ? _cls : attr->cls);
#else
    setItem((Class *)(_cls ? _cls : attr->cls),
	    attr->name, attr->isIndirect(),
	    attr->typmod.ndims, attr->typmod.dims,
	    attr->mode,
	    attr->is_basic_enum,
	    attr->is_string);
#endif
    num = n;
    class_owner = _class_owner;
    dyn_class_owner = _dyn_class_owner;

    oid_cl       = attr->oid_cl;
    oid_cl_own   = attr->oid_cl_own;

    attr_comp_set    = attr->attr_comp_set;
    attr_comp_set_oid = attr->attr_comp_set_oid;

    is_basic_enum  = attr->is_basic_enum;
    is_string      = attr->is_string;
    idr_poff       = attr->idr_poff;
    idr_item_psize = attr->idr_item_psize;
    idr_psize      = attr->idr_psize;
    idr_inisize    = attr->idr_inisize;
    idr_voff       = attr->idr_voff;
    idr_item_vsize = attr->idr_item_vsize;
    idr_vsize      = attr->idr_vsize;

    inv_spec      = attr->inv_spec;
    card          = attr->card;
    magorder      = attr->magorder;
    user_data     = attr->user_data;
    dataspace     = attr->dataspace;
    dspid         = attr->dspid;
  }

  Status
  Attribute::clean_realize(Schema *m, const Class *&xcls)
  {
    if (xcls && !m->checkClass(xcls)) {
      std::string str = xcls->getName();
      xcls = m->getClass(xcls->getName());
      if (!xcls)
	return Exception::make(IDB_ATTRIBUTE_ERROR, "clean() error for "
			       "attribute %s::%s (%s)",
			       (class_owner ? class_owner->getName() :
				"<unknown>"),
			       name,
			       str.c_str());
    }

    return Success;
  }

  Status
  Attribute::clean(Database *db)
  {
    ATTR_COMPLETE();
    Schema *m = db->getSchema();
    Status s = clean_realize(m, cls);
    if (s) return s;
    s = clean_realize(m, class_owner);
    if (s) return s;
    return clean_realize(m, dyn_class_owner);
  }

  Status
  Attribute::checkAttrPath(Schema *m, const Class *&rcls,
			   const Attribute *&attr,
			   const char *attrpath, AttrIdxContext *idx_ctx,
			   Bool just_check_attr)

  {
    char *s = strdup(attrpath);
    char *q = strchr(s, '.');

    if (!q) {
      free(s);
      return Exception::make("attribute path '%s' should be under the form "
			     "'class.attrname[.attrname]'", attrpath);
    }

    *q = 0;
    const Class *cls = m->getClass(s);

    Status status;
    if (!cls) {
      status = Exception::make("class '%s' not found", s);
      free(s);
      return status;
    }

    rcls = cls;
    if (idx_ctx)
      idx_ctx->set(cls, (Attribute *)0);

    char *p = q+1;
    for (;;) {
      char *x = strchr(p, '.');
      if (x)
	*x = 0;
      attr = cls->getAttribute(p);

      if (!attr) {
	//      if (just_check_attr && !x) {
	if (just_check_attr) {
	  free(s);
	  return Success;
	}

	status = Exception::make("attribute '%s' not found in class '%s'",
				 p, cls->getName());
	free(s);
	return status;
      }

      if (idx_ctx)
	idx_ctx->push(attr);

      if (!x)
	break;

      if (attr->isIndirect()) {
	status = Exception::make("attribute '%s' in class '%s' is not "
				 "litteral", p, cls->getName());
	free(s);
	return status;
      }

      p = x+1;
      cls = attr->getClass();
    }

    free(s);
    return Success;
  }

  Status Attribute::checkTypes(Data data, Size item_size, int nb) const
  {
    return Success;
  }

  static void
  compile_update(const AgregatClass *ma, int isz, int *offset, int *size)
  {
    if (ma->asUnionClass())
      {
	if (isz > *size)
	  *size = isz;
      }
    else
      {
	*offset += isz;
	*size = isz;
      }
  }

  void
  get_prefix(const Object *agr, const Class *class_owner,
	     char prefix[], int len)
  {
    const char *name = class_owner->getName();
    if (strcmp(agr->getClass()->getName(), name))
      {
	if (strlen(name) < len - 3)
	  sprintf(prefix, "%s::", name);
	else
	  {
	    strncpy(prefix, name, len - 4);
	    prefix[len-3] = 0;
	    strcat(prefix, "::");
	  }
      }
    else
      prefix[0] = 0;
  }

  Status Attribute::checkRange(int from, int &nb) const
  {
    const TypeModifier *tmod = &typmod;

    if (from < 0)
      return Exception::make(IDB_ATTRIBUTE_ERROR, "invalid negative offset [%d] for attribute '%s' in agregat class '%s'", from, name, class_owner->getName());

    if (from >= tmod->pdims)
      return Exception::make(IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR, "offset [%d] too large for attribute '%s' in agregat class '%s'", from, name, class_owner->getName());

    if (nb == defaultSize)
      nb = tmod->pdims - from;
    else if (nb != directAccess && from + nb > tmod->pdims)
      return Exception::make(IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR, "range [%d, %d[ too large for attribute '%s' in agregat class '%s'", from, from+nb, name, class_owner->getName());

    /* todo : blindage sur nb >= 0 && nb != directAccess si !cls->asBasicClass() */
    return Success;
  }

  Status
  Attribute::checkVarRange(const Object *agr, int from, int &nb,
			   Size *psize) const
  {
    const TypeModifier *tmod = &typmod;
    Size size;

    getSize(agr, size);

    if (psize)
      *psize = size;

    if (from < 0)
      return Exception::make(IDB_ATTRIBUTE_ERROR, "invalid negative offset [%d] for attribute '%s' in agregat class '%s'", from, name, class_owner->getName());

    if (nb != directAccess && size < (from+nb)/tmod->pdims)
      return Exception::make(IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR, "range [%d, %d[ too large for attribute '%s' in agregat class '%s'", from, from+nb, name, class_owner->getName());

    return Success;
  }

  Status
  Attribute::checkVarRange(int from, int nb, Size size) const
  {
    if (from < 0)
      return Exception::make(IDB_ATTRIBUTE_ERROR, "invalid negative offset [%d] for attribute '%s' in agregat class '%s'", from, name, class_owner->getName());

    if (nb != wholeData && size < (from+nb)/typmod.pdims)
      return Exception::make(IDB_OUT_OF_RANGE_ATTRIBUTE_ERROR, "range [%d, %d[ too large for attribute '%s' in agregat class '%s'", from, from+nb, name,
			     class_owner->getName());

    return Success;
  }

  static Status
  check_type(const Class *cls, Object *o)
  {
    Bool is;
    Status status;

    status = cls->isObjectOfClass(o, &is, True);

    if (status)
      return status;

    if (!is)
      return Exception::make(IDB_ATTRIBUTE_ERROR, 
			     "waiting for object of class '%s', got object of class '%s'",
			     cls->getName(), (o->getClass() ?
					      o->getClass()->getName() : 
					      "<unknown>"));

    return Success;
  }

  void
  Attribute::setCollHints(Object *o, const Oid& oid,
			  CardinalityConstraint *card_to_set) const
  {
    if (!o || !o->asCollection())
      return;

#ifdef CARD_TRACE
    printf("setCollHints(%s) card_to_set %p\n", name, card_to_set);
#endif
    //if (card_to_set)
    o->asCollection()->setCardinalityConstraint(card_to_set);

    o->asCollection()->setInverse(oid, num); 
    //  o->asCollection()->setMagOrder(magorder);
  }

  Status
  Attribute::setCollImpl(Database *db, Object *o,
			 const AttrIdxContext &idx_ctx) const
  {
    if (!o || !o->asCollection() || !o->asCollection()->isLiteral())
      return Success;

    if (o->asCollection()->getOidC().isValid()) {
      /*
	printf("setCollImpl: %s => does not changed implementation\n",
	o->asCollection()->getOidC().toString());
      */
      return Success;
    }

    CollAttrImpl *collattrimpl;
    Status s = collimplPrologue(db, idx_ctx, collattrimpl);
    /*
      printf("setCollImplementation(%s, collimpl=%p, %s)\n", name, collimpl,
      o->asCollection()->getOidC().toString());
    */
    if (s || !collattrimpl) return s;
    const IndexImpl *idximpl = 0; 
    const CollImpl *collimpl = 0;
    s = collattrimpl->getImplementation(db, collimpl);
    if (s) return s;

    /*
      printf("setCollImplementation(%s, setting implementation %s)\n",
      name, (const char *)idximpl->getHintsString());
    */
    o->asCollection()->setImplementation(collimpl);

    // 3/10/05
    /*
      if (idximpl)
      const_cast<IndexImpl *>(idximpl)->release();
    */

    return Success;
  }

  Status
  Attribute::convert(Database *db, ClassConversion *,
		     Data in_idr, Size in_size) const
  {
    return Exception::make(IDB_ERROR, "attribute %s::%s conversion error",
			   class_owner->getName(), name);
  }

  static Oid getOid(Object *o)
  {
    if (!o)
      return Oid::nullOid;

    if (o->getOid().isValid())
      return o->getOid();

    Collection *coll = dynamic_cast<Collection *>(o);
    if (!coll)
      return Oid::nullOid;

    return coll->getLiteralOid();
  }

  static bool isAutoAffect(Object *old_o, Object *o)
  {
    if (old_o && o && old_o == o)
      return true;

    Oid o_oid = eyedb::getOid(o);
    Oid old_o_oid = eyedb::getOid(old_o);
    return o_oid.isValid() && o_oid == old_o_oid;
  }

  static bool is_persistent(Object *o)
  {
    return (bool)getOid(o).isValid();
  }

  Status
  Attribute::setValue(Object *agr,
		      Data pvdata, // pdata or vdata
		      Data data, Size incsize,
		      Size,
		      int nb, int from, Data inidata,
		      Bool is_indirect, Data vdata,
		      Bool check_class) const
  {
    Status status;

    CHECK_OBJ(agr);

    // EV 26/01/07: just for test
    assert(isIndirect() == is_indirect);

    if (!is_indirect) {
      // pvdata is pdata
      if (!pvdata) {
	setInit(inidata, nb, from, 0);
	return Success;
      }

      if (cls->asBasicClass()) {
#ifdef E_XDR_TRACE
	printf("%s: Attribute::setValue() -> XDR\n", name);
#endif
#ifdef E_XDR
	if (cls->cmp((char *)(pvdata + (from * incsize)),
		     (const char *)data, incsize, nb) ||
	    isNull(inidata, nb, from)) {
	  cls->encode((char *)(pvdata + (from * incsize)),
		      (const char *)data, incsize, nb);
	  agr->touch();
	}
#else
	if (memcmp((char *)(pvdata + (from * incsize)),
		   (const char *)data, nb * incsize) ||
	    isNull(inidata, nb, from)) {
	  memcpy((char *)(pvdata + (from * incsize)),
		 (const char *)data, nb * incsize);
	  agr->touch();
	}
#endif

	setInit(inidata, nb, from, 1);
	return Success;
      }

      else if (cls->asEnumClass()) {
	Bool modify;

	status = ((EnumClass *)cls)->setRawData
	  ((Data)(pvdata + (from * incsize)), (Data)data, nb,
	   modify, check_class);
      
	if (!status && (modify || isNull(inidata, nb, from)))
	  agr->touch();

	if (!status)
	  setInit(inidata, nb, from, 1);
	return status;
      }
    }

    for (int n = 0; n < nb; n++, data += sizeof(Object *)) {
      Object *o = 0;
      Object *old_o = 0;

      if (is_indirect) {
	// pvdata is vdata
	mcp(&old_o, pvdata + ((from+n) * SIZEOFOBJECT), sizeof(Object *));

	mcp(&o, data, sizeof(Object *));

	if (old_o == o)
	  continue;

	if (o && o->isOnStack()) {
	  if (!eyedb_support_stack)
	    return Exception::make(IDB_ERROR,
				   "setting attribute '%s::%s': "
				   "cannot set a stack allocated object",
				   class_owner->getName(), name);
	}

	setCollHints(old_o, Oid::nullOid, NULL);
      
	if (old_o)
	  old_o->release();

	if (o) {
	  if (check_class) {
	    status = check_type(cls, o);
	    if (status) {
	      if (old_o)
		ObjectPeer::incrRefCount(old_o);
	      return status;
	    }
	  }

	  o->gbxObject::incrRefCount();

	  Oid oid = o->getOid();

	  if (oid.isValid()) {
	    if (isVarDim())
	      setOid(agr, &oid, 1, from+n);
	    else {
#ifdef E_XDR_TRACE
	      printf("%s: setValue -> set_oid_realize\n", name);
#endif
	      set_oid_realize(agr->getIDR() + idr_poff, &oid, 1, from+n);
	    }
	  }

	  setCollHints(o, agr->getOid(), card);
	}
      
	agr->touch();
	// no XDR
#ifdef _64BITS_PORTAGE
	memcpy(pvdata + ((from+n) * SIZEOFOBJECT), data, sizeof(Object *));
#else
	memcpy(pvdata + ((from+n) * SIZEOFOBJECT), data, incsize);
#endif
      }
      else {
	// pvdata is pdata

	memcpy(&o, data, sizeof(Object *)); // no XDR

	bool should_release_masterobj = false;

	bool auto_affect = false;

	if (vdata) {
	  // no XDR
	  memcpy(&old_o, vdata + (from+n) * SIZEOFOBJECT, sizeof(Object *));

	  auto_affect = isAutoAffect(old_o, o);
	  if (!auto_affect) {
	    
#define SUPPORT_SET_LITERAL
#ifdef SUPPORT_SET_LITERAL
	    bool has_index = false;
	    std::string idx_str;

	    status = hasIndex(getDB(agr), has_index, idx_str);
	    if (status)
	      return status;
	    
	    if (has_index) {
	      //if (is_persistent(old_o) && is_persistent(o) && has_index) {
	      return Exception::make
		(IDB_ATTRIBUTE_ERROR,
		 "setting attribute value '%s': "
		 "cannot replace an indexed literal attribute '%s', must remove indexes first: %s",
		 name, old_o->getClass()->getName(), idx_str.c_str());
	    }
	    
	    should_release_masterobj = true;
#else
	    if (old_o != o && is_persistent(old_o))
	      return Exception::make
		(IDB_ATTRIBUTE_ERROR,
		 "setting attribute value '%s': "
		 "cannot replace already stored literal attribute '%s'",
		 name, old_o->getClass()->getName());
#endif
	  }
	}

	if (o) {
	  if (check_class) {
	    status = check_type(cls, o);
	    if (status)
	      return status;
	  }

	  // needs XDR ? no...
#ifdef E_XDR_TRACE
	  printf("%s: Attribute::setValue : cmp\n", name);
#endif
	  if (!o->getIDR()) {
	    return Exception::make
	      ("setting attribute value '%s': object of class '%s' "
	       "invalid null IDR", name, o->getClass()->getName());
	  }

	  // 28/01/00: put the following code outside the 'if' statement
	  // WARNING: the true test must be:
	  /*
	    if (o->getMasterObject() && (o->getMasterObject() != agr ||
	    o->getMasterObjectAttribute() != this))
	  */
	  if (o->getMasterObject(false) && o->getMasterObject(false) != agr) {
	    return Exception::make
	      (IDB_ATTRIBUTE_ERROR,
	       "setting attribute value '%s': object of class '%s' "
	       "cannot be shared between several objects.", name,
	       o->getClass()->getName());
	  }
		      
	  /*if (!auto_affect)*/ {
	    status = o->setMasterObject(agr);
	    if (status)
	      return status;
	  }

	  if (memcmp(pvdata + ((from+n) * incsize),
		     o->getIDR() + IDB_OBJ_HEAD_SIZE, incsize)) {
#ifdef E_XDR_TRACE
	    printf("%s: Attribute::setValue -> needs XDR ? no\n", name);
#endif
	    memcpy(pvdata + ((from+n) * incsize),
		   o->getIDR() + IDB_OBJ_HEAD_SIZE, incsize);

	    agr->touch();
	  }

	  /* WARNING: should be also:
	     o->setMasterObjectAttribute(agr, this);
	  */
	}
	else
	  memset(pvdata + ((from+n) * incsize), 0, incsize);

	if (old_o) {
	  if (should_release_masterobj) {
	    status = old_o->releaseMasterObject();
	    if (status)
	      return status;
	  }
	  if (o != old_o)
	    old_o->release_r();
	}

	if (old_o == o)
	  continue;

	if (o)
	  o->gbxObject::incrRefCount();

	agr->touch();
	// no XDR
	memcpy(vdata + ((from+n) * SIZEOFOBJECT), &o, sizeof(Object *));
      }
    }

    return Success;
  }

  Status Attribute::getValue(Database *db,
			     Data pvdata, // pdata or vdata
			     Data *data, Size incsize,
			     int nb, int from, Data inidata, Bool *isnull) const
  {
    if (isnull)
      *isnull = False;

    if (cls->asBasicClass()) {
      // pvdata is pdata
#ifdef E_XDR_TRACE
      printf("%s: Attribute::getValue() -> XDR\n", name);
#endif
      if (nb == directAccess)
	*data = (pvdata ? pvdata + (from * incsize) : 0);
#ifdef E_XDR
      else
	cls->decode(data, pvdata + (from * incsize), incsize, nb);
#else
      else
	memcpy(data, pvdata + (from * incsize), nb * incsize);
#endif

      if (isnull) {
	if (inidata)
	  *isnull = isNull(inidata, (nb == directAccess ?
				     typmod.pdims - from : nb), from);
	else
	  *isnull = True;
      }
    }
    else if (cls->asEnumClass()) {
      // pvdata is pdata
      if (isnull) {
	if (inidata)
	  *isnull = isNull(inidata, nb, from);
	else
	  *isnull = True;
      }
    
      return ((EnumClass *)cls)->getRawData
	((Data)data, pvdata + (from * incsize), nb);
    }
    else {
      // pvdata is vdata
      // no XDR
      for (int n = 0; n < nb; n++, data += SIZEOFOBJECT) // was incsize
	memcpy(data, pvdata + (from+n) * SIZEOFOBJECT,
	       sizeof(Object *));
    }

    return Success;
  }

  Status Attribute::incrRefCount(Object *agr, Data _idr, int n) const
  {
    for (int i = 0; i < n; i++, _idr += SIZEOFOBJECT) {
      Object *o;
      mcp(&o, _idr, sizeof(Object *));
      if (o) {
	o->incrRefCount();
	if (!isIndirect()) {
	  Status s = o->setMasterObject(agr);
	  if (s)
	    return s;
	}
      }
    }

    return Success;
  }

  void Attribute::manageCycle(Object *mo, Data _idr, int n,
			      gbxCycleContext &r) const
  {
    //printf("BEGIN Attribute::manageCycle(o=%p, %s, %p, count=%d)\n", mo, name, _idr, n);

    if (r.isCycle()) {
#ifdef MANAGE_CYCLE_TRACE
      printf("found #2 premature cycle in %s\n", name);
#endif
      return;
    }

    for (int i = 0; i < n; i++, _idr += SIZEOFOBJECT) {
      Object *o;
      mcp(&o, _idr, sizeof(Object *));

      if (o && !gbxAutoGarb::isObjectDeleted(o)) {
	o->manageCycle(r);
	if (r.mustClean(o)) {
	  if (Object::getReleaseCycleDetection()) {
	    throw *Exception::make
	      (IDB_ERROR,
	       "attribute %s::%s in object %p: attempt to release "
	       "object %p of class %s",
	       (getClassOwner() ? getClassOwner()->getName() :
		"<unknown class>"), name, mo, o, o->getClass()->getName());
	  }

	  //printf("%s: mustclean %p in object %p at #%d\n", name, o, mo, i);
	  mset(_idr, 0, sizeof(Object *));
	  /*
	    printf("%s::%s %s setting damaged %p\n", getClassOwner()->getName(),
	    name, (isIndirect() ? "indirect" : "direct"), mo);
	  */

	  // EV : 04/01/07: as cycle are not managed correctly, I disconnect this code:
#if 0
	  if (!isIndirect())
	    mo->setDamaged(this);
#endif
	}

	if (r.isCycle()) {
	  //#ifdef MANAGE_CYCLE_TRACE
	  //printf("found #3 premature cycle in %s\n", name);
	  //#endif
	  return;
	}
      }
    }
    //printf("END Attribute::manageCycle(o=%p, %s, %p, count=%d)\n", mo, name, _idr, n);
  }

  Status Attribute::setValue(Object*, unsigned char*, int, int,
			     Bool) const
  {
    return Success;
  }

  Status Attribute::getOid(const Object*, Oid*, int nb, int from) const
  {
    return Success;
  }

  Status Attribute::getValue(const Object*, unsigned char**, int, int,
			     Bool *) const
  {
    return Success;
  }

  Status Attribute::trace(const Object*, FILE*, int*, unsigned int flags, const RecMode *rcm) const
  {
    return Success;
  }

  Status Attribute::compile_volat(const AgregatClass*, int*, int*)
  {
    return Success;
  }

  Status Attribute::setOid(Object*, const Oid*, int, int, Bool) const
  {
    return Success;
  }

  Status Attribute::compile_perst(const AgregatClass*, int*, int*, int *)
  {
    return Success;
  }

  Status Attribute::setSize(Object *agr, Size size) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot set size of non vardim item '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status Attribute::getSize(Data, Size&) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot get size of non vardim item '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status Attribute::getSize(const Object *agr, Size& size) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot get size of non vardim item '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status Attribute::getSize(Database *db, const Oid *data_oid,
			    Size& size) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot get size of non vardim item '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status Attribute::update(Database *db,
			   const Oid& cloid,
			   const Oid& objoid,
			   Object *agr,
			   AttrIdxContext &idx_ctx) const
  {
    return Success;
  }

  Status
  Attribute::load(Database *db,
		  Object *agr,
		  const Oid &cloid,
		  LockMode lockmode,
		  AttrIdxContext &idx_ctx,
		  const RecMode *rcm, Bool force) const
  {
    return Success;
  }

  Status Attribute::realize(Database *db, Object *agr,
			    const Oid& cloid,
			    const Oid& objoid,
			    AttrIdxContext &idx_ctx,
			    const RecMode *rcm) const
  {
    return Success;
  }

  Status Attribute::remove(Database *db, Object *agr,
			   const Oid& cloid,
			   const Oid& objoid,
			   AttrIdxContext &idx_ctx,
			   const RecMode *rcm) const
  {
    return Success;
  }

  void Attribute::getPersistentIDR(Offset& p_off, Size& item_p_sz,
				   Size& p_sz, Size& item_ini_sz) const
  {
    p_off = idr_poff;
    item_p_sz = idr_item_psize;
    p_sz = idr_psize;
    item_ini_sz = idr_inisize;
  }

  void Attribute::getVolatileIDR(Offset& v_off, Size& item_v_sz,
				 Size& v_sz) const
  {
    v_off = idr_voff;
    item_v_sz = idr_item_vsize;
    v_sz = idr_vsize;
  }

  static Bool
  check_name(const ClassComponent *comp, void *xname)
  {
    return False;
  }

  CardinalityConstraint *Attribute::getCardinalityConstraint() const
  {
    return card;
  }

#define mutable_attr const_cast<Attribute *>(this)

  Status
  Attribute::getIdx(Database *db, int ind, int& maxind, Size &sz,
		    const AttrIdxContext &idx_ctx, Index *&idx,
		    eyedbsm::Idx *&se_idx) const
  {
    ATTR_COMPLETE();
    Status s;

    idx = 0;
    se_idx = 0;

    if (s = const_cast<Attribute *>(this)->indexPrologue
	(const_cast<Database *>(cls->getDatabase()), idx_ctx, idx, False))
      return s;

    if (ind == Attribute::composedMode)
      {
	maxind = Attribute::composedMode;
	sz = idr_item_psize * typmod.maxdims;

	/*
	  if (s = getMultiIndex(cls->getDatabase(), idx_ctx, idx, False))
	  return s;
	*/
	se_idx = (idx ? idx->idx : 0);
	return Success;
      }

    maxind = typmod.maxdims;
    sz = idr_item_psize;

    /*
      if ((mode & indexItemMode) != indexItemMode)
      return Success;

      if (s = getMultiIndex(cls->getDatabase(), idx_ctx, idx, False))
      return s;
    */
    se_idx = (idx ? idx->idx : 0);
    return Success;
  }


#define NEW_ARGARR
  // FE
  eyedbsm::Status
  hash_key(const void *key, unsigned int len, void *hash_data, unsigned int &x)
  {
    BEMethod_C *mth = (BEMethod_C *)hash_data;
#ifdef NEW_ARGARR
    static ArgArray *array = new ArgArray(2, Argument::NoGarbage);

    Argument retarg;

    (*array)[0]->set((const unsigned char *)key, len);
    (*array)[1]->set((int)len);

    Status s = mth->applyTo(mth->getDatabase(), (Object *)0, *array,
			    retarg, False);
#else
    ArgArray array(2, Argument::NoGarbage);
    Argument retarg;

    array[0]->set((const unsigned char *)key, len);
    array[1]->set((int)len);

    Status s = mth->applyTo(mth->getDatabase(), (Object *)0, array,
			    retarg, False);
    array[0]->release();
    array[1]->release();
#endif

    if (s)
      return eyedbsm::statusMake(eyedbsm::ERROR, "while applying hash function %s: %s",
				 mth->getName().c_str(), s->getString());
    x = retarg.getInteger();
    return eyedbsm::Success;
  }

  //#define INDEX_PRINT_STATS

  Status
  Attribute::openMultiIndexRealize(Database *db, Index *idx)
  {
    static const char fmt_error[] = "storage manager error '%s' "
      "reported when opening index '%s' of class '%s'";

    if (idx->idx)
      return Success;

    eyedbsm::Idx *se_idx = 0;
    const eyedbsm::Oid *idx_oid = idx->getIdxOid().getOid();

    eyedbsm::HIdx::hash_key_t hash_key = 0;
    void *hash_data = 0;

    if (idx->asHashIndex() &&
	idx->asHashIndex()->getHashMethod())
      {
	hash_key = hash_key;
	hash_data = idx->asHashIndex()->getHashMethod();
      }

    if (idx->asBTreeIndex() && !idx->getIsString())
      {
	se_idx = new eyedbsm::BIdx(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh), *idx_oid, idx_precmp);

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx->getAttrpath().c_str(),
				 idx->getClassOwner()->getName());
      }

    if (idx->asHashIndex() && !idx->getIsString())
      {

	se_idx = new eyedbsm::HIdx(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh), idx_oid, hash_key, hash_data, idx_precmp);

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx->getAttrpath().c_str(),
				 idx->getClassOwner()->getName());
      }

    if (idx->asBTreeIndex() && idx->getIsString())
      {
	se_idx = new eyedbsm::BIdx(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh), *idx_oid, idx_precmp);

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx->getAttrpath().c_str(),
				 idx->getClassOwner()->getName());
      }

    if (idx->asHashIndex() && idx->getIsString())
      {
	se_idx = new eyedbsm::HIdx(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh), idx_oid, hash_key, hash_data, idx_precmp);

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx->getAttrpath().c_str(),
				 idx->getClassOwner()->getName());
      }

    idx->idx = se_idx;
    return Success;
  }

  ClassComponent *
  Attribute::getComp(Class::CompIdx idx,
		     Bool (*pred)(const ClassComponent *, void *),
		     void *client_data) const
  {
    const Class *cl = getClassOwner();
    if (!cl)
      return NULL;

    const LinkedList *complist = cl->getCompList(idx);

    if (!complist)
      {
	Status s = ((Class *)cl)->setup(True);
	if (s)
	  return NULL;

	if (!(complist = cl->getCompList(idx)))
	  return NULL;
      }

    LinkedListCursor c(complist);

    ClassComponent *comp;
    while (c.getNext((void *&)comp))
      if (pred(comp, client_data))
	return comp;

    return NULL;
  }

#define NEWIDXTYPE2
#define NEWIDXTYPE3

#ifdef NEWIDXTYPE3

  static void
  ktypes_dump(const Attribute *attr, eyedbsm::Idx::KeyType &ktypes, unsigned int count)
  {
    printf("Attribute %s::%s\n", attr->getClassOwner() ?
	   attr->getClassOwner()->getName() : "<unknown>", attr->getName());
    printf("\tktypes.type = %d\n", ktypes.type);
    printf("\tktypes.count = %d\n", ktypes.count);
    printf("\tktypes.offset = %d\n", ktypes.offset);
    printf("\toldcount = %d\n\n", count);
  }

  static Status
  get_index_type(const Attribute *attr, Bool isComp,
		 eyedbsm::Idx::KeyType &ktypes, Bool isHash,
		 Bool isCollLit = False)
  {
    if (isHash && isComp && attr->isString()) {
      ktypes.type = eyedbsm::Idx::tString;
      if (attr->isVarDim()) {
	ktypes.count = eyedbsm::HIdx::VarSize;
	ktypes.offset = 0;
      }
      else {
	/*
	  ktypes.count = sizeof(char) + attr->getTypeModifier().pdims;
	  ktypes.offset = 0;
	*/
	ktypes.count = attr->getTypeModifier().pdims;
	ktypes.offset = 1;
      }
      return Success;
    }

    const Class *cls = attr->getClass();

    if (isCollLit || attr->isIndirect() || cls->asOidClass())
      ktypes.type = eyedbsm::Idx::tOid;
    else if (cls->asCharClass())
      ktypes.type = eyedbsm::Idx::tChar;
    else if (cls->asInt16Class())
      ktypes.type = eyedbsm::Idx::tInt16;
    else if (cls->asInt32Class() || cls->asEnumClass())
      ktypes.type = eyedbsm::Idx::tInt32;
    else if (cls->asInt64Class())
      ktypes.type = eyedbsm::Idx::tInt64;
    else if (cls->asFloatClass())
      ktypes.type = eyedbsm::Idx::tFloat64;
    else if (cls->asByteClass())
      ktypes.type = eyedbsm::Idx::tUnsignedChar;
    else
      return Exception::make(IDB_ATTRIBUTE_ERROR,
			     "cannot create btree index on %s::%s of type %s",
			     attr->getClassOwner()->getName(), attr->getName(),
			     cls->getName());
    if (isComp) {
      if (attr->isVarDim())
	ktypes.count = eyedbsm::HIdx::VarSize; // should be eyedbsm::Idx::varSize
      else
	ktypes.count = attr->getTypeModifier().pdims;

      ktypes.offset = sizeof(char);

      assert(!isHash);
    }
    else {
      ktypes.count = 1;
      ktypes.offset = (isCollLit ? 0 : (sizeof(char) + sizeof(eyedblib::int32)));
#ifdef IDB_UNDEF_ENUM_HINT
      if (cls->asEnumClass())
	ktypes.offset += sizeof(char);
#endif
    }

    return Success;
  }
#else
  static Status
  get_index_type(const Attribute *attr, eyedbsm::Idx::Type &type,
		 unsigned int &count, Bool isCollLit = False)
  {
#ifdef NEWIDXTYPE2
    if (isCollLit || attr->isIndirect()) {
      count = 1;
      type = eyedbsm::Idx::tOid;
      return Success;
    }

    /*
      if (attr->isString()) {
      count = 1;
      type = eyedbsm::Idx::tString;
      return Success;
      }
    */
#else
    if (isCollLit) {
      count = sizeof(eyedbsm::Oid);
      type = eyedbsm::Idx::tUnsignedChar;
      assert(0);
      return Success;
    }

    if (attr->isIndirect()) {
      count = sizeof(eyedbsm::Oid);
      type = eyedbsm::Idx::tUnsignedChar;
      assert(0);
      return Success;
    }
#endif

    // IN FACT, I THINK THAT INDEX ON ARRAY ARE NOT WELL SUPPORTED!
    count = (isComp ? attr->getTypeModifier().pdims : 1);
    ntypes = (isComp ? 1 : attr->getTypeModifier().pdims);

    const Class *cls = attr->getClass();
    if (cls->asCharClass()) {
      type = eyedbsm::Idx::tChar;
      return Success;
    }

    if (cls->asInt16Class()) {
      type = eyedbsm::Idx::tInt16;
      return Success;
    }

    if (cls->asInt32Class() || cls->asEnumClass()) {
      type = eyedbsm::Idx::tInt32;
      return Success;
    }

    if (cls->asInt64Class()) {
      type = eyedbsm::Idx::tInt64;
      return Success;
    }

    if (cls->asFloatClass()) {
      type = eyedbsm::Idx::tFloat64;
      return Success;
    }

    if (cls->asOidClass()) {
      count = sizeof(eyedbsm::Oid);
      type = eyedbsm::Idx::tUnsignedChar;
      assert(0);
      return Success;
    }

    if (cls->asByteClass()) {
      type = eyedbsm::Idx::tUnsignedChar;
      return Success;
    }

#ifdef NEWIDXTYPE2
    if (cls->asOidClass()) {
      type = eyedbsm::Idx::tOid;
      return Success;
    }
#endif
    return Exception::make(IDB_ATTRIBUTE_ERROR,
			   "cannot create btree index on %s::%s of type %s",
			   attr->getClassOwner()->getName(), attr->getName(),
			   cls->getName());
  }
#endif

  static void
  display(const eyedbsm::Idx::KeyType &ktypes)
  {
    printf("ktypes.type %d\n", ktypes.type);
    printf("ktypes.count %d\n", ktypes.count);
    printf("ktypes.offset %d\n", ktypes.offset);
  }


  void
  resynchronize_indexes(Database *db, Index *idx)
  {
    idx->trace();
    idx->idx = 0;
    Index *new_idx = 0;
    if (!db->reloadObject(idx->getOid(), (Object *&)new_idx))
      {
	IDB_LOG(IDB_LOG_IDX_CREATE,
		("resynchronize indexes %s %s\n",
		 idx->getIdxOid().toString(),
		 new_idx->getIdxOid().toString()));
	idx->setIdxOid(new_idx->getIdxOid());
	new_idx->release();
	return;
      }
  
    idx->setIdxOid(Oid::nullOid);
  }

  void
  idx_purge_test(void *xidx)
  {
    Index *idx = (Index *)xidx;
    idx->setIdxOid(Oid::nullOid);
    idx->idx = 0;
  }

  static void
  make_hash_hints(HashIndex *idx, int impl_hints[])
  {
    memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
    int cnt = idx->getImplHintsCount();
    for (int i = 0; i < cnt; i++)
      impl_hints[i] = idx->getImplHints(i);
  }

  char index_backend[] = "eyedb:index:backend";

  Status
  Attribute::createDeferredIndex_realize(Database *db,
					 const AttrIdxContext &idx_ctx,
					 Index *idx)
  {
    // ATTENTION à l'aspect concurrence lors de cette creation différé
    static const char fmt_error[] = "storage manager error '%s' "
      "reported when opening index in attribute '%s' of class '%s'";
    //int hash_entries = 0;
    Oid idx_oid;
    //int mag_order = IDB_CLEAN_MAGORDER(idx_ctx.getMagOrder());
    Status status;

    /*
      if (!mag_order)
      return Exception::make(IDB_ATTRIBUTE_ERROR,
      "invalid null magorder in %s::%s",
      class_owner->getName(), name);
    */

    IDB_LOG(IDB_LOG_IDX_CREATE,
	    ("Attribute::createDeferredIndex_realize(%s)\n",
	     idx->getAttrpath().c_str()));

#ifdef TRACE_IDX
    printf("creating deferred index this=%p, name=%s, attrname=%s\n",
	   this, name, idx_ctx.getAttrName().c_str());
#endif

    /*
      if (idx->asHashIndex())
      hash_entries = idx->asHashIndex()->getKeyCount();
    */

    eyedbsm::HIdx::hash_key_t hash_key = 0;
    void *hash_data = 0;

    if (idx->asHashIndex() &&
	idx->asHashIndex()->getHashMethod())
      {
	hash_key = hash_key;
	hash_data = idx->asHashIndex()->getHashMethod();
      }

    idx_oid = idx->getIdxOid();

    if (idx_oid.isValid())
      return Success;

    eyedbsm::Idx::KeyType ktypes;
    Idx *se_idx = 0;

    Bool isCollLit = IDBBOOL(!isIndirect() && cls->asCollectionClass());
    if (idx->asBTreeIndex() && !is_string)
      {
	/*
	  printf("CREATING BTREE #3 %s::%s\n", class_owner->getName(), name);
	  display(ktypes);
	*/
#ifdef NEWIDXTYPE3
	Status s = get_index_type(this, False, ktypes, False, isCollLit);
	if (s) return s;

#elif defined(NEWIDXTYPE)
	Status s = get_index_type(this, ktypes.type, ktypes.count, isCollLit);
	if (s) return s;
	if (isCollLit)
	  ktypes.offset = 0;
	else
	  ktypes.offset = sizeof(char) + sizeof(eyedblib::int32);

#endif
	se_idx = new eyedbsm::BIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   sizeof(Oid), // was 2*sizeof(Oid),
	   &ktypes,
	   idx->get_dspid(),
	   idx->asBTreeIndex()->getDegree());

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx_ctx.getAttrName().c_str(),
				 class_owner->getName());

	se_idx->asBIdx()->open(idx_precmp);
	idx->report(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
		    Oid(se_idx->oid()));
      }

    int impl_hints[eyedbsm::HIdxImplHintsCount];
    if (idx->asHashIndex())
      make_hash_hints(idx->asHashIndex(), impl_hints);

    if (idx->asHashIndex() && !is_string)
      {
	int count;
	if (isCollLit) count = idr_item_psize;
	else           count = sizeof(char) + sizeof(eyedblib::int32) + idr_item_psize;

	Status s = get_index_type(this, False, ktypes, True, isCollLit);
	if (s) return s;

#ifdef NEW_NOTNULL_TRACE
	ktypes_dump(this, ktypes, count);
#endif

	/*
	  printf("MAGORDER %s -> %d %s -> %d\n",
	  class_owner->getName(),
	  class_owner->getMagorder(),
	  dyn_class_owner->getName(),
	  dyn_class_owner->getMagorder());
	*/
	se_idx = new eyedbsm::HIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   ktypes,
	   sizeof(Oid),
	   idx->get_dspid(),
	   dyn_class_owner->getMagorder(),
	   idx->asHashIndex()->getKeyCount(),
	   impl_hints, eyedbsm::HIdxImplHintsCount);

	if (se_idx->status() != eyedbsm::Success)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx_ctx.getAttrName().c_str(),
				 class_owner->getName());
	se_idx->asHIdx()->open(hash_key, hash_data, idx_precmp);

	idx->report(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
		    Oid(se_idx->oid()));
      }

    if (idx->asBTreeIndex() && is_string)
      {
	/*
	  printf("CREATING BTREE #4 %s::%s\n", class_owner->getName(), name);
	  display(ktypes);
	*/
#ifdef NEWIDXTYPE3
	Status s = get_index_type(this, True, ktypes, False);
	if (s) return s;
#else
	Status s = get_index_type(this, ktypes.type, ktypes.count);
	if (s) return s;
	ktypes.offset = sizeof(char);
#endif
	se_idx = new eyedbsm::BIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   sizeof(Oid), // was 2*sizeof(Oid)
	   &ktypes,
	   idx->get_dspid(),
	   idx->asBTreeIndex()->getDegree());

	if (se_idx->status())
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx_ctx.getAttrName().c_str(),
				 class_owner->getName());
	se_idx->asBIdx()->open(idx_precmp);
	idx->report(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
		    Oid(se_idx->oid()));
      }

    if (idx->asHashIndex() && is_string)
      {
	Status s = get_index_type(this, True, ktypes, True);
	if (s) return s;
#ifdef NEW_NOTNULL_TRACE
	ktypes_dump(this, ktypes,
		    (isVarDim() ? eyedbsm::HIdx::varSize : sizeof(char) + idr_item_psize * typmod.maxdims));
#endif
	/*
	  printf("MAGORDER %s -> %d %s -> %d\n",
	  class_owner->getName(),
	  class_owner->getMagorder(),
	  dyn_class_owner->getName(),
	  dyn_class_owner->getMagorder());

	*/
	se_idx = new eyedbsm::HIdx
	  (get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
	   ktypes,
	   sizeof(Oid), // was 2*sizeof(Oid)
	   idx->get_dspid(),
	   dyn_class_owner->getMagorder(),
	   idx->asHashIndex()->getKeyCount(),
	   impl_hints, eyedbsm::HIdxImplHintsCount);

	if (se_idx->status())
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(se_idx->status()),
				 idx_ctx.getAttrName().c_str(),
				 class_owner->getName());
	se_idx->asHIdx()->open(hash_key, hash_data, idx_precmp);

	idx->report(get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh),
		    Oid(se_idx->oid()));
      }

    if (se_idx) {
      idx->idx = se_idx;
      idx->setIdxOid(se_idx->oid());
      void *ud = idx->setUserData(index_backend, AnyUserData);
      status = idx->store();
      idx->setUserData(index_backend, ud);
      db->addPurgeActionOnAbort(idx_purge_test, idx);
      if (status) {
	resynchronize_indexes(db, idx);
	return status;
      }
    }

    return Success;
  }

  int Attribute::getBound(Database *db, Data _idr)
  {
    return typmod.pdims;
  }

  static inline
  Bool checkNull(Data data, unsigned int size)
  {
    unsigned char *p = data;

    for (int i = 0; i < size; i++, p++)
      if (*p)
	return False;

    return True;
  }

  static void
  check_notnull(const char *msg, const char *name,
		Data pdata, Data inidata, int count, Bool indirect,
		const TypeModifier &typmod, unsigned int incsize)
  {
    Bool isnull;

    // 1) check not null for the whole data
    if (indirect)
      isnull = checkNull(pdata, sizeof(eyedbsm::Oid) * count);
    else
      isnull = Attribute::isNull(inidata, &typmod);

    Data xdata = pdata;
    for (int j = 0; j < count; j++, xdata += incsize)
      {
	if (indirect)
	  isnull = checkNull(xdata, sizeof(eyedbsm::Oid));
	else
	  isnull = Attribute::isNull(inidata, 1, j);
      }
  }

  static Bool
  check_notnull_comp(Data pdata, Data inidata, int count, Bool indirect,
		     const TypeModifier &typmod, unsigned int incsize)
  {

    if (indirect)
      return checkNull(pdata, sizeof(eyedbsm::Oid) * count);

    // added the 21/01/00 to correct a bug of notnull check when
    // vardim attribute has no index!
    if (!count) // means that there is no data!
      return True;
    // ....
    return Attribute::isNull(inidata, &typmod);
  }

  static Bool
  check_notnull(Data pdata, Data inidata, int count, Bool indirect,
		const TypeModifier &typmod, unsigned int incsize)
  {
    if (indirect)
      {
	for (int j = 0; j < count; j++, pdata += incsize)
	  if (checkNull(pdata, sizeof(eyedbsm::Oid)))
	    return True;
	return False;
      }

    for (int j = 0; j < count; j++, pdata += incsize)
      if (Attribute::isNull(inidata, 1, j))
	return True;

    return False;
  }

  Status Attribute::check() const
  {
    return Success;
  }
#define CHECK_NOTNULL_BASE(comp, COMP)					\
  if (notnull##comp)							\
    {									\
      if (check_notnull##comp(pdata, inidata, count, isIndirect(),	\
			      typmod, idr_item_psize))			\
	return Exception::make(IDB_NOTNULL##COMP##_CONSTRAINT_ERROR,	\
			       const_error, idx_ctx.getAttrName().c_str()); \
    } 

#define CHECK_NOTNULL() CHECK_NOTNULL_BASE(, )

#define CHECK_NOTNULL_COMP() CHECK_NOTNULL_BASE(_comp, _COMP)

#define NULL_STR(X) ((X) ? "null data" : "not null data")

  /*
    #define IS_NULL(ISNULL, DATA, INIDATA) \
    Bool ISNULL; \
    if (notnull) \
    ISNULL = False; \
    else if (indirect) \
    ISNULL = checkNull(DATA, sizeof(eyedbsm::Oid)); \
    else \
    ISNULL = isNull(INIDATA, 1, n)
  */

#define IS_NULL(ISNULL, DATA, INIDATA)			\
  Bool ISNULL;						\
  if (indirect)						\
    ISNULL = checkNull(DATA, sizeof(eyedbsm::Oid));	\
  else							\
    ISNULL = isNull(INIDATA, 1, n)

#define NEW_NOTNULL
#define NEW_NOTNULL2

  // MIND: 12/12/01
  // added ISNULL_2 macro:

#ifdef NEW_NOTNULL
  /*
    #define IS_NULL_2(ISNULL, DATA) \
    Bool ISNULL; \
    if (notnull) \
    ISNULL = False; \
    else if (indirect) \
    ISNULL = checkNull(DATA, sizeof(eyedbsm::Oid)); \
    else \
    ISNULL = checkNull(DATA, idr_item_psize)
  */
#define IS_NULL_2(ISNULL, DATA)				\
  Bool ISNULL;						\
  if (indirect)						\
    ISNULL = checkNull(DATA, sizeof(eyedbsm::Oid));	\
  else							\
    ISNULL = checkNull(DATA, idr_item_psize)
#endif

  /*
    #define IS_COMP_NULL(ISNULL, PDATA, INIDATA) \
    Bool ISNULL; \
    if (notnull_comp) \
    ISNULL = False; \
    else \
    ISNULL = check_notnull_comp(PDATA, INIDATA, count, isIndirect(), \
    typmod, idr_item_psize)
  */
#define IS_COMP_NULL(ISNULL, PDATA, INIDATA)				\
  Bool ISNULL;								\
  ISNULL = check_notnull_comp(PDATA, INIDATA, count, isIndirect(),	\
			      typmod, idr_item_psize)

#define CHECK_UNIQUE(ISNULL, F1, F2)					\
  do {									\
    if (unique)								\
      {									\
	if (!ISNULL)							\
	  {								\
	    eyedbsm::Boolean found;					\
	    if (status = idx_item->searchAny(e, &found))		\
	      {								\
		free(F1); free(F2);					\
		return Exception::make(IDB_INDEX_ERROR, fmt_error,	\
				       eyedbsm::statusGet(status),	\
				       idx_ctx.getAttrName().c_str(),	\
				       class_owner->getName());		\
	      }								\
	    if (found)							\
	      {								\
		free(F1); free(F2);					\
		return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,	\
				       const_error,			\
				       idx_ctx.getAttrName().c_str());	\
	      }								\
	  }								\
      }									\
  } while(0)

#define CHECK_UNIQUE_COMP(ISNULL, F1, F2)				\
  do {									\
    if (unique_comp)							\
      {									\
	if (!ISNULL)							\
	  {								\
	    eyedbsm::Boolean found;					\
	    if (status = idx_comp->searchAny(data, &found))		\
	      {								\
		free(F1); free(F2);					\
		return Exception::make(IDB_INDEX_ERROR, fmt_error,	\
				       eyedbsm::statusGet(status),	\
				       idx_ctx.getAttrName().c_str(),	\
				       class_owner->getName());		\
	      }								\
	    if (found)							\
	      {								\
		free(F1); free(F2);					\
		return Exception::make(IDB_UNIQUE_COMP_CONSTRAINT_ERROR, \
				       const_error,			\
				       idx_ctx.getAttrName().c_str());	\
	      }								\
	  }								\
      }									\
  } while(0)

  static const char const_error[] = "attribute path '%s'";

  static void
  dump_data(unsigned char *e, int sz)
  {
    printf("IDX insert ->\n");
    for (int i = 0; i < sz; i++)
      printf("%03o ", e[i]);
    printf("\n");
  }

  // backend method

  Bool
  Attribute::indexPrologue(Database *db, Data _idr, Bool novd,
			   int &count, Data &pdata, Size &varsize,
			   Bool create)
  {
    varsize = 0;
    if (isVarDim() && novd)
      {
	if (!VARS_COND(db))
	  return True;

	getSize(_idr, varsize);

	// ATTENTION: ce varsize est la nouvelle taille
	count = varsize;
	pdata = _idr + idr_poff + VARS_OFFSET;
	if (create && varsize > VARS_SZ)
	  return True;
	return False;
      }

    if (isVarDim())
      {
	pdata = _idr;
	return False;
      }

    Data vdata;
    getData(db, _idr, pdata, vdata);
    count = typmod.pdims;
    return False;
  }

  Status
  Attribute::createIndexEntry_realize(Database *db,
				      Data _idr,
				      const Oid *_oid,
				      const Oid *cloid,
				      int offset,
				      Bool novd,
				      AttrIdxContext &idx_ctx,
				      int count,
				      int size)
  {
    // all should be factorize
    Data pdata;
    Size varsize;

    if (indexPrologue(db, _idr, novd, count, pdata, varsize, True))
      return Success;

    Status status;
    idx_ctx.push(db, *cloid, this);
    status = createIndexEntry(db, pdata, _oid, cloid, offset, count, size,
			      varsize, novd, idx_ctx);
    idx_ctx.pop();
    return status;
  }

  Status
  Attribute::collimplPrologue(Database *db,
			      const AttrIdxContext &idx_ctx,
			      CollAttrImpl *&collattrimpl) const
  {
#ifdef IDX_CTX_SHORTCUT
    collattrimpl = 0;
#else
    std::string attrpath = idx_ctx.getAttrName();
    Status s = loadComponentSet(db, False);
    if (s) return s;

    if (attr_comp_set) {
      s = attr_comp_set->find(attrpath.c_str(), collattrimpl);
      if (s) return s;
    }
    else
      collattrimpl = 0;
#endif

    return Success;
  }

  Status
  Attribute::constraintPrologue(Database *db,
				const AttrIdxContext &idx_ctx,
				Bool &notnull_comp, Bool &notnull,
				Bool &unique_comp, Bool &unique) const
  {
#ifdef IDX_CTX_SHORTCUT
    notnull_comp = notnull = False;
    unique_comp = unique = False;
#else
    std::string attrpath = idx_ctx.getAttrName();
    Status s = loadComponentSet(db, False);
    if (s) return s;

    NotNullConstraint *notnull_component = 0;
    UniqueConstraint *unique_component = 0;
    if (attr_comp_set) {
      s = attr_comp_set->find(attrpath.c_str(), notnull_component);
      if (s) return s;
      s = attr_comp_set->find(attrpath.c_str(), unique_component);
      if (s) return s;

#ifdef TRACE_IDX
      if (notnull_component)
	printf("found a notnull for %s\n", attrpath.c_str());
      if (unique_component)
	printf("found a unique for %s\n", attrpath.c_str());
#endif
    }

    if (notnull_component) {
      if (is_string) {
	notnull_comp = True;
	notnull = False;
      }
      else {
	notnull = True;
	notnull_comp = False;
      }
    }
    else
      notnull_comp = notnull = False;

    if (unique_component) {
      if (is_string) {
	unique_comp = True;
	unique = False;
      }
      else {
	unique = True;
	unique_comp = False;
      }
    }
    else
      unique_comp = unique = False;

#endif
    return Success;
  }

  Status
  Attribute::getAttrComponents(Database *db, const Class *xcls,
			       LinkedList &list)
  {
    if (!cls) {
      //printf("WARNING getAttrComponents %s -> cls is null\n", xcls->getName());
      return Success;
    }
    
    if (!isIndirect() && !is_basic_enum && !cls->asCollectionClass()) {
      ATTR_COMPLETE();
      unsigned int attr_cnt;
      const Attribute **attrs = cls->getAttributes(attr_cnt);
      for (int i = 0; i < attr_cnt; i++) {
	Status s = const_cast<Attribute *>(attrs[i])->getAttrComponents
	  (db, xcls, list);
	if (s) return s;
      }

      return Success;
    }

    Status s = loadComponentSet(db, False);
    if (s) return s;
    if (attr_comp_set)
      return attr_comp_set->getAttrComponents(xcls, list);

    return Success;
  }

  Status
  Attribute::indexPrologue(Database *db,
			   const AttrIdxContext &idx_ctx,
			   Index *&idx, Bool create)
  {
#ifdef IDX_CTX_SHORTCUT
    idx = 0;
#else
    std::string attrpath = idx_ctx.getAttrName();
    Status s = loadComponentSet(db, False);
    if (s) return s;

    if (attr_comp_set) {
      s = attr_comp_set->find(attrpath.c_str(), idx);
      if (s) return s;
    
      if (idx) {
	Idx *se_idx = idx->idx; // volatile idx

	if (!se_idx) {
	  if (!idx->getIdxOid().isValid()) {
	    if (!create)
	      return Success;

	    s = createDeferredIndex_realize(db, idx_ctx, idx);
	    if (s) return s;
	    assert(idx->idx);
	    delete idx->idx;
	    idx->idx = 0;
	  }
	}

	s = openMultiIndexRealize(db, idx);
	if (s) return s;
      }
    }
    else
      idx = 0;
#endif

    return Success;
  }

  Status
  Attribute::createIndexEntry(Database *db, Data pdata,
			      const Oid *_oid,
			      const Oid *cloid,
			      int offset, int count, int,
			      Size, Bool,
			      AttrIdxContext &idx_ctx) 
  {
    ATTR_COMPLETE();
    eyedbsm::Status status;
    static const char fmt_error[] = "storage manager error '%s' reported "
      "when creating index entry in attribute '%s' in agregat class '%s'";

    Oid xoid;
    Data inidata;
    int inisize = iniCompute(db, count, pdata, inidata);

#ifdef TRACE_IDX
    printf("creating index entry this=%p, name=%s, attrname=%s\n",
	   this, name,  idx_ctx.getAttrName().c_str());
#endif
    IDB_LOG(IDB_LOG_IDX_CREATE,
	    ("Attribute::createIndexEntry(%s)\n",
	     idx_ctx.getAttrName().c_str()));

    Bool notnull, notnull_comp, unique, unique_comp;
    Status s = constraintPrologue(db, idx_ctx, notnull_comp, notnull,
				  unique_comp, unique);
    if (s) return s;

    Index *idx_test;
    s = indexPrologue(db, idx_ctx, idx_test, True);
    if (s) return s;

    CHECK_NOTNULL();

    Oid soid[2];
    soid[0] = *_oid;
    soid[1] = *cloid;

    Idx *idx_item = 0, *idx_comp = 0;
    if (idx_test && !is_string)
      {
	// should be in a variable!
	if (cls && cls->asCollectionClass() && !isIndirect())
	  return Success;

	idx_item = idx_test->idx;

	unsigned int size_of_e = sizeof(char) + sizeof(eyedblib::int32) +
	  idr_item_psize;
	unsigned char *e = (unsigned char *)malloc(size_of_e);
	Data data = pdata;
	Bool indirect = isIndirect();
	for (int n = 0; n < count; n++, data += idr_item_psize) {
	  IS_NULL(isnull, data, inidata);
	  e[0] = (isnull ? idxNull : idxNotNull);

	  h2x_32_cpy(e+sizeof(char), &n); // must code n because the index does not perform the swap
	  if (indirect) {
	    eyedbsm::Oid toid;
	    eyedbsm::x2h_oid(&toid, data); // 8/09/06: data contains XDR oid : must be decoded to put in index because index perform the swap
	    mcp(e+sizeof(char)+sizeof(eyedblib::int32), &toid, sizeof(eyedbsm::Oid));
	  }
	  else
	    cls->decode(e+sizeof(char)+sizeof(eyedblib::int32), data, idr_item_psize);

	  CHECK_UNIQUE(isnull, e, 0);

	  IDB_LOG(IDB_LOG_IDX_INSERT,
		  (log_item_entry_fmt,
		   ATTRPATH(idx_test), soid[0].toString(),
		   dumpData(data), n, NULL_STR(isnull)));

	  idx_ctx.addIdxOP(this, AttrIdxContext::IdxInsert,
			   idx_test, idx_item, e, size_of_e, soid);
	}
	free(e);
      }
    else if (unique)
      return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,
			     "when creating index entry on '%s' : "
			     "unique constraint missing index",
			     idx_ctx.getAttrName().c_str());

    CHECK_NOTNULL_COMP();

    if (idx_test && is_string)
      {
	idx_comp = idx_test->idx;

	IS_COMP_NULL(isnull, pdata, inidata);

	int sz;
	if (isVarDim())
	  sz = count;
	else
	  sz = typmod.pdims * idr_item_psize;

	unsigned int size_of_data = sizeof(char) + sz;
	Data data = (unsigned char *)malloc(size_of_data);
	data[0] = (isnull ? idxNull : idxNotNull);
	memcpy(&data[1], pdata, sz);

	CHECK_UNIQUE_COMP(isnull, data, 0);

	IDB_LOG(IDB_LOG_IDX_INSERT,
		(log_comp_entry_fmt,
		 ATTRPATH(idx_test), soid[0].toString(), pdata,
		 NULL_STR(isnull)));

	idx_ctx.addIdxOP(this, AttrIdxContext::IdxInsert,
			 idx_test, idx_comp, data, size_of_data, soid);
	free(data);
      }
    else if (unique_comp)
      return Exception::make(IDB_UNIQUE_COMP_CONSTRAINT_ERROR,
			     "when creating index entry on '%s' : "
			     "unique constraint missing index",
			     idx_ctx.getAttrName().c_str());

    if (cls && cls->asAgregatClass() && !isIndirect() && count)
      {
	Status status;

	offset += idr_poff - IDB_OBJ_HEAD_SIZE;

	Data data = pdata;

	GBX_SUSPEND();

	for (int j = 0; j < count; j++, data += idr_item_psize)
	  {
	    Object *o = cls->newObj(data);
									  
	    status = ((AgregatClass *)cls)->
	      createIndexEntries_realize(db, o->getIDR(), _oid, idx_ctx, cloid,
					 offset + (j * idr_item_psize),
					 True);
	    o->release();
	    if (status)
	      return status;
	  }
      }

    return Success;
  }

#define SK()								\
  (skipRemove ? "true" : "false"), (skipInsert ? "true" : "false")

  // backend method
  Status
  Attribute::sizesCompute(Database *db, const char fmt_error[],
			  const Oid *data_oid, int &offset,
			  Size varsize, Bool novd, int &osz,
			  int inisize, int &oinisize, Bool &skipRemove,
			  Bool &skipInsert)
  {
    skipRemove = False;
    skipInsert = False;

    if (isVarDim())
      {
	if (novd)
	  {
	    assert(VARS_COND(db));

	    if (varsize > VARS_SZ) // was: '>= VARS_SZ' 6/02/01
	      skipInsert = True;

	    eyedbsm::Status status;
	    char buf[sizeof(Size)+sizeof(Oid)];

	    if ((status = eyedbsm::objectRead(get_eyedbsm_DbHandle
					      ((DbHandle *)db->getDbHandle()->
					       u.dbh),
					      idr_poff + offset,
					      sizeof buf,
					      buf,
					      eyedbsm::DefaultLock,
					      0, 0,
					      data_oid->getOid())))
	      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				     eyedbsm::statusGet(status),
				     name,
				     class_owner->getName());
	  
	    Oid varoid;
	    // needs XDR ?
#ifdef E_XDR_TRACE
	    printf("%s: sizesCompute -> XDR\n", name);
#endif
#ifdef E_XDR
	    eyedbsm::x2h_oid(varoid.getOid(), buf+sizeof(Size));
#else
	    mcp(&varoid, buf+sizeof(Size), sizeof(varoid));
#endif
	    if (!varoid.isValid()) {
	      offset += VARS_OFFSET;
#ifdef E_XDR
	      x2h_32_cpy(&osz, buf);
#else
	      mcp(&osz, buf, sizeof(osz));
#endif
	      osz = CLEAN_SIZE(osz);
	      oinisize = iniSize(osz);
	      return Success;
	    }

	    skipRemove = True;
	    osz = 0;
	    oinisize = 0;
	    return Success;
	  }

	size_t xsz; // V3_API_size_t
	eyedbsm::objectSizeGet(get_eyedbsm_DbHandle((DbHandle *)db->
						    getDbHandle()->u.dbh),
			       &xsz, eyedbsm::DefaultLock, data_oid->getOid());
	osz = revSize(xsz);
	oinisize = xsz - osz;
	return Success;
      }

    osz = idr_item_psize * typmod.maxdims;
    oinisize = inisize;
    return Success;
  }

  Status
  Attribute::updateIndexEntry(Database *db, Data pdata,
			      const Oid *_oid,
			      const Oid *cloid,
			      int offset, const Oid *data_oid,
			      int count,
			      Size varsize, Bool novd,
			      AttrIdxContext &idx_ctx)
  {
    ATTR_COMPLETE();
    eyedbsm::Status status;
    static const char fmt_error[] = "storage manager error '%s' reported when updating index entry in attribute '%s' in agregat class '%s'";

    if (!data_oid)
      data_oid = _oid;

#ifdef TRACE_IDX
    printf("updating index entry this=%p, name=%s, attrname=%s\n",
	   this, name,  idx_ctx.getAttrName().c_str());
#endif

    Data inidata;
    int inisize = iniCompute(db, count, pdata, inidata);

    Bool notnull, notnull_comp, unique, unique_comp;
    Status s = constraintPrologue(db, idx_ctx, notnull_comp, notnull,
				  unique_comp, unique);
    if (s) return s;

    Index *idx_test;
    s = indexPrologue(db, idx_ctx, idx_test, True);
    if (s) return s;

    CHECK_NOTNULL();

    Oid soid[2];
    soid[0] = *_oid;
    soid[1] = *cloid;

    //Index *idx = 0;
    Idx *idx_item = 0, *idx_comp = 0;

    if (idx_test && !is_string)
      {
	if (cls && cls->asCollectionClass() && !isIndirect())
	  return Success;

	idx_item = idx_test->idx;

	Data data = pdata;
	eyedbsm::DbHandle *se_dbh = get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh);
	unsigned char *sinidata = 0;
	if (inisize) {
	  sinidata = (unsigned char *)malloc(inisize);
	  if ((status = eyedbsm::objectRead(se_dbh,
					    idr_poff + offset,
					    inisize,
					    sinidata,
					    eyedbsm::DefaultLock,
					    0, 0,
					    data_oid->getOid()))) {
	    free(sinidata);
	    return Exception::make(IDB_INDEX_ERROR, fmt_error,
				   eyedbsm::statusGet(status),
				   idx_ctx.getAttrName().c_str(),
				   class_owner->getName());
	  }
	}

	unsigned char *s = (unsigned char *)malloc((idr_item_psize + inisize)
						   * sizeof(unsigned char));
	unsigned int size_of_e = sizeof(char) + sizeof(eyedblib::int32) +
	  idr_item_psize;
	unsigned char *e = (unsigned char *)malloc(size_of_e);

	unsigned char *sx = s;
	Bool indirect = isIndirect();
	for (int n = 0; n < count; n++, data += idr_item_psize)
	  {
	    if ((status = eyedbsm::objectRead(se_dbh,
					      idr_poff + n * idr_item_psize + offset + inisize,
					      idr_item_psize, s,
					      eyedbsm::DefaultLock,
					      0, 0,
					      data_oid->getOid()))) {
	      free(s);
	      free(e);
	      free(sinidata);
	      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				     eyedbsm::statusGet(status),
				     idx_ctx.getAttrName().c_str(),
				     class_owner->getName());
	    }
	  
	    IS_NULL(isnull_s, s, sinidata);
	    IS_NULL(isnull_data, data, inidata);

	    if (isnull_s != isnull_data ||
		memcmp(data, sx, idr_item_psize))
	      {
		if (unique) {
		  e[0] = (isnull_data ? idxNull : idxNotNull);
		  h2x_32_cpy(e+sizeof(char), &n);
		  if (indirect) {
		    eyedbsm::Oid toid;
		    eyedbsm::x2h_oid(&toid, data);
		    mcp(e+sizeof(char)+sizeof(eyedblib::int32), &toid, sizeof(eyedbsm::Oid));
		  }
		  else
		    cls->decode(e+sizeof(char)+sizeof(eyedblib::int32), data, idr_item_psize);

		  CHECK_UNIQUE(isnull_data, e, s);
		}

		e[0] = (isnull_s ? idxNull : idxNotNull);
		h2x_32_cpy(e+sizeof(char), &n);
		if (indirect) {
		  eyedbsm::Oid toid;
		  eyedbsm::x2h_oid(&toid, sx);
		  mcp(e+sizeof(char)+sizeof(eyedblib::int32), &toid, sizeof(eyedbsm::Oid));
		}
		else
		  cls->decode(e+sizeof(char)+sizeof(eyedblib::int32), sx, idr_item_psize);

		eyedbsm::Boolean found;
	      
		IDB_LOG(IDB_LOG_IDX_SUPPRESS,
			(log_item_entry_fmt,
			 ATTRPATH(idx_test), soid[0].toString(),
			 dumpData(sx), n, NULL_STR(isnull_s)));

		idx_ctx.addIdxOP(this, AttrIdxContext::IdxRemove,
				 idx_test, idx_item, e, size_of_e, soid);
	      
		e[0] = (isnull_data ? idxNull : idxNotNull);
		h2x_32_cpy(e+sizeof(char), &n);
		if (indirect) {
		  eyedbsm::Oid toid;
		  eyedbsm::x2h_oid(&toid, data);
		  mcp(e+sizeof(char)+sizeof(eyedblib::int32), &toid, sizeof(eyedbsm::Oid));
		}
		else
		  cls->decode(e+sizeof(char)+sizeof(eyedblib::int32), data, idr_item_psize);

		IDB_LOG(IDB_LOG_IDX_INSERT,
			(log_item_entry_fmt,
			 ATTRPATH(idx_test), soid[0].toString(),
			 dumpData(data), n, NULL_STR(isnull_data)));
	      
		idx_ctx.addIdxOP(this, AttrIdxContext::IdxInsert,
				 idx_test, idx_item, e, size_of_e, soid);
	      }
	  }

	free(sinidata);
	free(e);
	free(s);
      }
    else if (unique)
      return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,
			     "when updating index entry on '%s' : "
			     "unique constraint missing index",
			     idx_ctx.getAttrName().c_str());
 
    CHECK_NOTNULL_COMP();

    if (idx_test && is_string)
      {
	idx_comp = idx_test->idx;

	int osz, oinisize;

	Bool skipRemove, skipInsert;

	s = sizesCompute(db, fmt_error, data_oid, offset, varsize, novd, osz,
			 inisize, oinisize, skipRemove, skipInsert);
	if (s) return s;

	if (skipInsert && skipRemove)
	  return Success;

	int nsz;
	if (isVarDim())
	  nsz = count;
	else
	  nsz = typmod.pdims * idr_item_psize;

	Data data = pdata;
	unsigned char *s;
	Bool isnull_s, isnull_data;

	if (!skipRemove) {
	  if (osz + oinisize) {
	    s = (unsigned char *)malloc((osz + oinisize));
	
	    if ((status = eyedbsm::objectRead(get_eyedbsm_DbHandle
					      ((DbHandle *)db->getDbHandle()->u.dbh),
					      idr_poff + offset, osz + oinisize, s,
					      eyedbsm::DefaultLock,
					      0, 0,
					      data_oid->getOid()))) {
	      free(s);
	      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				     eyedbsm::statusGet(status),
				     idx_ctx.getAttrName().c_str(),
				     class_owner->getName());
	    }
	  
	    IS_COMP_NULL(isnull_sx, s+oinisize, s);
	    isnull_s = isnull_sx;
	  }
	  else {
	    s = (unsigned char *)malloc(1);
	    s[0] = 0;
	    isnull_s = True;
	  }
	}
	else {
	  s = 0;
	  isnull_s = False;
	}

	if (!skipInsert)
	  {
	    IS_COMP_NULL(isnull_data_x, data, inidata);
	    isnull_data = isnull_data_x;
	  }
	else
	  isnull_data = False;

	// 29/01/01: suppressed 'novd' in the following test, because
	// I think that this test does not make any sense!
	if (/*novd || */ isnull_s != isnull_data || nsz != osz ||
	    memcmp(data, s+oinisize, osz)) {
	  if (!skipInsert)
	    CHECK_UNIQUE_COMP(isnull_data, 0, 0);

	  unsigned char *e = 0;
	  unsigned int size_of_e = sizeof(char) + osz;
	  if (!skipRemove) {
	    e = (unsigned char *)malloc(size_of_e);

	    eyedbsm::Boolean found;

	    e[0] = (isnull_s ? idxNull : idxNotNull);
	      
	    memcpy(e+sizeof(char), s+oinisize, osz);
	      
	    IDB_LOG(IDB_LOG_IDX_SUPPRESS, 
		    (log_comp_entry_fmt,
		     ATTRPATH(idx_test), soid[0].toString(),
		     s+oinisize, NULL_STR(isnull_s)));
	      
	    idx_ctx.addIdxOP(this, AttrIdxContext::IdxRemove,
			     idx_test, idx_comp, e, size_of_e, soid);
	  }

	  if (!skipInsert) {
	    if (!e || nsz > osz) {
	      free(e);
	      size_of_e = sizeof(char) + nsz;
	      e = (unsigned char *)malloc(size_of_e);
	    }

	    e[0] = (isnull_data ? idxNull : idxNotNull);
	    memcpy(e+sizeof(char), data, nsz);
	      
	    IDB_LOG(IDB_LOG_IDX_INSERT, 
		    (log_comp_entry_fmt,
		     ATTRPATH(idx_test), soid[0].toString(), data,
		     NULL_STR(isnull_data)));
	      
	    idx_ctx.addIdxOP(this, AttrIdxContext::IdxInsert,
			     idx_test, idx_comp, e, size_of_e, soid);
	  }

	  free(e);
	}
	free(s);
      }
    else if (unique_comp)
      return Exception::make(IDB_UNIQUE_COMP_CONSTRAINT_ERROR,
			     "when updating index entry on '%s' : "
			     "unique constraint missing index",
			     idx_ctx.getAttrName().c_str());

    if (cls->asAgregatClass() && !isIndirect() && count)
      {
	Status status;

	offset += idr_poff - IDB_OBJ_HEAD_SIZE;

	Data data = pdata;

	GBX_SUSPEND();

	for (int j = 0; j < count; j++, data += idr_item_psize)
	  {
	    Object *o = cls->newObj(data);
	    Status status = ((AgregatClass *)cls)->
	      updateIndexEntries_realize(db, o->getIDR(), _oid, idx_ctx, cloid,
					 offset + (j * idr_item_psize),
					 True, data_oid);
	    o->release();
	    if (status)
	      return status;
	  }
      }

    return Success;
  }

  Status Attribute::updateIndexEntry_realize(Database *db,
					     Data _idr,
					     const Oid *_oid,
					     const Oid *cloid,
					     int offset,
					     Bool novd,
					     const Oid *data_oid,
					     AttrIdxContext &idx_ctx,
					     int count)
  {
    Data pdata;
    Size varsize;

    if (indexPrologue(db, _idr, novd, count, pdata, varsize, False))
      return Success;

    Status status;
    idx_ctx.push(db, *cloid, this);
    status = updateIndexEntry(db, pdata, _oid, cloid, offset, data_oid, count,
			      varsize, novd, idx_ctx);
    idx_ctx.pop();
    return status;
  }

  Status Attribute::removeIndexEntry_realize(Database *db,
					     Data _idr,
					     const Oid *_oid,
					     const Oid *cloid,
					     int offset,
					     Bool novd,
					     const Oid *data_oid,
					     AttrIdxContext &idx_ctx,
					     int count)

  {
    Data pdata;
    Size varsize;

    if (indexPrologue(db, _idr, novd, count, pdata, varsize, False))
      return Success;

    Status status;
    idx_ctx.push(db, *cloid, this);
    status = removeIndexEntry(db, pdata, _oid, cloid, offset, data_oid,
			      count, varsize, novd, idx_ctx);
    idx_ctx.pop();
    return status;
  }

  Status Attribute::removeIndexEntry(Database *db, Data pdata,
				     const Oid *_oid,
				     const Oid *cloid,
				     int offset, const Oid *data_oid,
				     int count,
				     Size varsize, Bool novd,
				     AttrIdxContext &idx_ctx)
  {
    ATTR_COMPLETE();
    eyedbsm::Status status;
    static const char fmt_error[] = "storage manager error '%s' reported when removing index entry in attribute '%s' in agregat class '%s'";

    if (!data_oid)
      data_oid = _oid;

    Data inidata;
    int inisize = iniCompute(db, count, pdata, inidata);

    Oid soid[2];
    soid[0] = *_oid;
    soid[1] = *cloid;

    Status s = Success;
    //Index *idx = 0;

    Index *idx_test;
    s = indexPrologue(db, idx_ctx, idx_test, True);
    if (s) return s;

    Idx *idx_item = 0, *idx_comp = 0;

    if (idx_test && !is_string)
      //if ((mode & indexItemMode) == indexItemMode)
      {
	if (cls && cls->asCollectionClass() && !isIndirect())
	  return Success;

	/*
	  s = getMultiIndex(db, idx_ctx, idx);
	  if (s) return s;
	  if (!idx)
	  return Exception::make(IDB_ATTRIBUTE_ERROR,
	  "cannot open index '%s'",
	  (const char *)idx_ctx.getAttrName());
	  idx_item = idx->idx;
	*/
	idx_item = idx_test->idx;

	eyedbsm::DbHandle *se_dbh = get_eyedbsm_DbHandle((DbHandle *)db->getDbHandle()->u.dbh);
	unsigned char *sinidata = 0;
	if (inisize) {
	  sinidata = (unsigned char *)malloc(inisize);
	  if ((status = eyedbsm::objectRead(se_dbh,
					    idr_poff + offset,
					    inisize,
					    sinidata,
					    eyedbsm::DefaultLock,
					    0, 0,
					    data_oid->getOid()))) {
	    free(sinidata);
	    return Exception::make(IDB_INDEX_ERROR, fmt_error,
				   eyedbsm::statusGet(status),
				   idx_ctx.getAttrName().c_str(),
				   class_owner->getName());
	  }
	}

	unsigned char *s = (unsigned char *)malloc(idr_item_psize + inisize);
	unsigned int size_of_e = sizeof(char) + sizeof(eyedblib::int32) +
	  idr_item_psize;
	unsigned char *e = (unsigned char *)malloc(size_of_e);
	unsigned char *sx = s;
	Data data = pdata;

	Bool indirect = isIndirect();
	for (int n = 0; n < count; n++, data += idr_item_psize)
	  {
	    if ((status = eyedbsm::objectRead(se_dbh,
					      idr_poff + n * idr_item_psize + offset + inisize,
					      idr_item_psize,
					      s, eyedbsm::DefaultLock, 0, 0, data_oid->getOid())))
	      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				     eyedbsm::statusGet(status),
				     idx_ctx.getAttrName().c_str(),
				     class_owner->getName());

	    IS_NULL(isnull_s, s, sinidata);
	    e[0] = (isnull_s ? idxNull : idxNotNull);
	    h2x_32_cpy(e+sizeof(char), &n);
	    if (indirect) {
	      eyedbsm::Oid toid;
	      eyedbsm::x2h_oid(&toid, sx);
	      mcp(e+sizeof(char)+sizeof(eyedblib::int32), &toid, sizeof(eyedbsm::Oid));
	    }
	    else
	      cls->decode(e+sizeof(char)+sizeof(eyedblib::int32), sx, idr_item_psize);

	    IDB_LOG(IDB_LOG_IDX_SUPPRESS,
		    (log_item_entry_fmt,
		     ATTRPATH(idx_test), soid[0].toString(),
		     dumpData(sx), n, NULL_STR(isnull_s)));

	    idx_ctx.addIdxOP(this, AttrIdxContext::IdxRemove,
			     idx_test, idx_item, e, size_of_e, soid);
	  }

	free(sinidata);
	free(e);
	free(s);
      }

    if (idx_test && is_string)
      //if ((mode & indexCompMode) == indexCompMode)
      {
	/*
	  s = getMultiIndex(db, idx_ctx, idx);
	  if (s) return s;
	  if (!idx)
	  return Exception::make(IDB_ATTRIBUTE_ERROR,
	  "cannot open index '%s'",
	  (const char *)idx_ctx.getAttrName());
	  idx_comp = idx->idx;
	*/
	idx_comp = idx_test->idx;

	int osz, oinisize;

	Bool skipRemove, dummy;

	s = sizesCompute(db, fmt_error, data_oid, offset, varsize, novd, osz,
			 inisize, oinisize, skipRemove, dummy);
	if (s) return s;

	if (skipRemove)
	  return Success;

	unsigned char *s;
	Bool isnull_s;
	if (osz + oinisize) { // test added the 16/03/02
	  s =  (unsigned char *)malloc(osz + oinisize);
      
	  if ((status = eyedbsm::objectRead(get_eyedbsm_DbHandle
					    ((DbHandle *)db->getDbHandle()->u.dbh),
					    idr_poff + offset, osz + oinisize, s,
					    eyedbsm::DefaultLock, 0, 0,
					    data_oid->getOid())))
	    {
	      free(s);
	      return Exception::make(IDB_INDEX_ERROR, fmt_error,
				     eyedbsm::statusGet(status),
				     idx_ctx.getAttrName().c_str(),
				     class_owner->getName());
	    }
	
	  IS_COMP_NULL(isnull_sx, s+oinisize, s);
	  isnull_s = isnull_sx;
	}
	else {
	  s = (unsigned char *)malloc(1);
	  s[0] = 0;
	  isnull_s = True;
	}

	eyedbsm::Boolean found;
	unsigned int size_of_e = sizeof(char) + osz;
	unsigned char *e = (unsigned char *)malloc(size_of_e);
	e[0] = (isnull_s ? idxNull : idxNotNull);
	memcpy(e+sizeof(char), s+oinisize, osz);
      
	IDB_LOG(IDB_LOG_IDX_SUPPRESS, 
		(log_comp_entry_fmt,
		 ATTRPATH(idx_test), soid[0].toString(), s+oinisize,
		 NULL_STR(isnull_s)));

	idx_ctx.addIdxOP(this, AttrIdxContext::IdxRemove,
			 idx_test, idx_comp, e, size_of_e, soid);
	free(e);
	free(s);
      }

    if (cls->asAgregatClass() && !isIndirect() && count)
      {
	Status status;

	offset += idr_poff - IDB_OBJ_HEAD_SIZE;

	GBX_SUSPEND();

	Data data = pdata;
	for (int j = 0; j < count; j++, data += idr_item_psize)
	  {
	    Object *o = (Object *)cls->newObj(data);
	    Status status = ((AgregatClass *)cls)->
	      removeIndexEntries_realize(db, o->getIDR(), _oid, idx_ctx, cloid,
					 offset + (j * idr_item_psize),
					 True, data_oid);
	    o->release();
	    if (status)
	      return status;
	  }
      }

    return Success;
  }

  Status
  Attribute::updateIndexForInverse(Database *db, const Oid &_oid,
				   const Oid &new_oid) const
  {
    Status status;
    static const char fmt_error[] = "storage manager error '%s' reported when updating index inverse entry in attribute '%s' in agregat class '%s'";

    assert(isIndirect());

    static const eyedblib::int32 zero = 0;
    Class *obj_cls;
    status = db->getObjectClass(_oid, obj_cls);
    if (status) return status;

    AttrIdxContext idx_ctx(obj_cls, this);

    Idx *se_idx;

    // ?? Hope that there is no problem with polymorphism ??
    //printf("UpdateIndexForInverse(%s::%s)\n", obj_cls->getName(), name);
    Index *idx = 0;
    status = const_cast<Attribute *>(this)->
      indexPrologue(db, idx_ctx, idx, True);
    if (!idx) return Success;

    if (status) return status;
    if (!idx)
      return Exception::make(IDB_ATTRIBUTE_ERROR,
			     "cannot open index '%s'",
			     idx_ctx.getAttrName().c_str());
    se_idx = idx->idx;

    Oid soid[2];
    soid[0] = _oid;
    soid[1] = obj_cls->getOid();
    
    eyedbsm::Oid toid;
    RPCStatus rpc_status =
      dataRead(db->getDbHandle(), idr_poff, sizeof(eyedbsm::Oid),
	       (Data)&toid, 0, _oid.getOid());
    Oid old_oid;
    eyedbsm::x2h_oid(old_oid.getOid(), &toid);

    if (rpc_status)
      return StatusMake(rpc_status);

    if (old_oid == new_oid)
      return Success;

    //printf("creating inverse index entry\n");
    unsigned char s[1+sizeof(eyedblib::int32)+sizeof(eyedbsm::Oid)];

    s[0] = old_oid.isValid() ? idxNotNull : idxNull;
    mcp(s+1, &zero, sizeof(eyedblib::int32));
    mcp(s+1+sizeof(eyedblib::int32), old_oid.getOid(), sizeof(eyedbsm::Oid));

    eyedbsm::Boolean found;
    eyedbsm::Status se_status;

    IDB_LOG(IDB_LOG_IDX_SUPPRESS,
	    (log_item_entry_fmt,
	     ATTRPATH(idx), soid[0].toString(),
	     old_oid.toString(), 0, NULL_STR(!old_oid.isValid())));

    if ((se_status = se_idx->remove(s, soid, &found)))
      return Exception::make(IDB_INDEX_ERROR, fmt_error,
			     eyedbsm::statusGet(se_status),
			     idx_ctx.getAttrName().c_str(),
			     class_owner->getName());

    if (!found)
      return Exception::make(IDB_INDEX_ERROR, fmt_error,
			     "index entry not found",
			     idx_ctx.getAttrName().c_str(),
			     class_owner->getName());



    s[0] = new_oid.isValid() ? idxNotNull : idxNull;
    mcp(s+1, &zero, sizeof(eyedblib::int32));
    mcp(s+1+sizeof(eyedblib::int32), new_oid.getOid(), sizeof(eyedbsm::Oid));

    IDB_LOG(IDB_LOG_IDX_INSERT,
	    (log_item_entry_fmt,
	     ATTRPATH(idx), soid[0].toString(),
	     new_oid.toString(), 0, NULL_STR(!new_oid.isValid())));

    if ((se_status = se_idx->insert(s, soid)))
      return Exception::make(IDB_INDEX_ERROR, fmt_error,
			     eyedbsm::statusGet(se_status),
			     idx_ctx.getAttrName().c_str(),
			     class_owner->getName());

    return Success;
  }

  void
  Attribute::newObjRealize(Object *) const
  {
  }

  void Attribute::getData(const Database *, Data _idr, Data& pdata, Data& vdata) const
  {
    pdata = _idr + idr_poff;
    vdata = _idr + idr_voff;
  }

  void Attribute::getData(const Object *agr, Data& pdata,
			  Data& vdata) const
  {
    getData(getDB(agr), agr->getIDR(), pdata, vdata);
  }

  void Attribute::getVarDimOid(Data _idr, Oid *oid) const
  {
    oid->invalidate();
  }

  void Attribute::getVarDimOid(const Object *, Oid *oid) const
  {
    oid->invalidate();
  }

  /*
    Status Attribute::getField(Database *db, const Oid *data_oid,
    Oid *doid, int *roffset, int offset,
    int from) const
    {
    return Success;
    }
  */

  Status Attribute::getVal(Database *db, const Oid *data_oid,
			   Data data, int offset,
			   int nb, int from, Bool *) const
  {
    return Success;
  }

  Status
  Attribute::getTValue(Database *db, const Oid &objoid,
		       Data *data, int nb, int from,
		       Bool *isnull, Size *rnb, Offset poffset) const
  {
    return Success;
  }

  Status Attribute::copy(Object *agr, Bool share) const
  {
    Data _idr = agr->getIDR();
    assert(_idr);
    if (!isIndirect() && is_basic_enum)
      return Success;

    return Attribute::incrRefCount(agr, _idr + idr_voff, typmod.pdims);
  }

#ifdef GBX_NEW_CYCLE
  void Attribute::decrRefCountPropag(Object *, int) const
  {
  }
#endif

  void Attribute::garbage(Object *, int) const
  {
  }

  void Attribute::manageCycle(Database *db, Object *o, gbxCycleContext &) const
  {
  }

  Status
  Attribute::add(Database *db, ClassConversion *conv,
		 Data in_idr, Size in_size) const
  {
    assert(0);
    Offset offset = conv->getOffsetN();
    Data start = in_idr + offset;
    Size size = conv->getSizeN();
    printf("add %s::%s idr_poff=%d offsetN=%d idr_psize=%d sizeN=%d "
	   "in_size=%d sizemoved=%d %s==%s?\n",
	   class_owner->getName(), name, idr_poff, offset, idr_psize, size,
	   in_size, in_size - size - offset, name, conv->getAttrname().c_str());
    assert(idr_poff == offset);
    assert(idr_psize == size);

    memmove(start + size, start, in_size - size - offset);
    memset(start, 0, size);
    return Success;
  }

#ifdef GBX_NEW_CYCLE
  void Attribute::decrRefCountPropag(Data vdata, int nb) const
  {
    Data data = vdata;

    for (int n = 0; n < nb; n++, data += SIZEOFOBJECT) {
      Object *o;
      mcp(&o, data, sizeof(Object *));
      if (o)
	o->decrRefCount();
    }
  }
#endif

  inline void Attribute::garbage(Data vdata, int nb) const
  {
    Data data = vdata;

    for (int n = 0; n < nb; n++, data += SIZEOFOBJECT)
      {
	Object *o;
	mcp(&o, data, sizeof(Object *));
	if (o && !gbxAutoGarb::isObjectDeleted(o))
	  {
	    unsigned int refcnt = o->getRefCount();
	    o->release_r();
	    if (!refcnt)
	      mset(data, 0, sizeof(Object *));
	  }
      }
  }

  //
  // AttrDirect methods
  //

  AttrDirect::AttrDirect(const Attribute *attr,
			 const Class *_cls,
			 const Class *_class_owner,
			 const Class *_dyn_class_owner, int n) :
    Attribute(attr, _cls, _class_owner, _dyn_class_owner, n)
  {
    code = AttrDirect_Code;
  }

  AttrDirect::AttrDirect(Database *db,
			 Data data, Offset *offset,
			 const Class *_dyn_class_owner,
			 int n) :
    Attribute(db, data, offset, _dyn_class_owner, n)
  {
    code = AttrDirect_Code;
  }

  static char *TADDR;

  static void get_o1()
  {
    if (!TADDR) return;
    Object *o;
    memcpy(&o, TADDR, sizeof(Object *));
    printf("o = %p\n", o);
  }

  void
  AttrDirect::newObjRealize(Object *o) const
  {
    if (is_basic_enum)
      return;

    Data pdata = o->getIDR() + idr_poff;
    Data vdata = o->getIDR() + idr_voff;

    GBX_SUSPEND();

    for (int j = 0; j < typmod.pdims; j++) {
      Object *oo;
      // no XDR
      memcpy(&oo, vdata + (j * idr_item_vsize), sizeof(Object *));
      if (!oo) {
	oo = (Object *)cls->newObj(pdata + (j * idr_item_psize));
	oo->setMustRelease(false); // SMART_PTR
	memcpy(vdata + (j * idr_item_vsize), &oo, sizeof(Object *));
      }
      // added the 5/11/99
      Status s = oo->setMasterObject(o);
      if (s)
	throw *s;
    }
  }

  Status
  AttrDirect::load(Database *db,
		   Object *agr,
		   const Oid &cloid,
		   LockMode lockmode,
		   AttrIdxContext &idx_ctx,
		   const RecMode *rcm,
		   Bool force) const
  {
    if (is_basic_enum)
      return Success;

    Data pdata = agr->getIDR() + idr_poff;

    idx_ctx.push(db, cloid, this);

    pdata += idr_inisize;
    for (int j = 0; j < typmod.pdims; j++) {
      Status status;
      Object *o;

      memcpy(&o, agr->getIDR() + idr_voff + (j * idr_item_vsize),
	     sizeof(Object *));
    
      // needs XDR ? no...
#ifdef E_XDR_TRACE
      printf("%s: AttrDirect::load -> needs XDR ? no\n", name);
#endif
      memcpy(o->getIDR() + IDB_OBJ_HEAD_SIZE,
	     pdata +  (j * idr_item_psize), idr_item_psize);

      status = o->setDatabase(db);

      if (status != Success)
	return status;

      status = o->loadPerform(cloid, lockmode, idx_ctx, rcm);

      if (status != Success)
	return status;

      // needs XDR ? no
      memcpy(pdata +  (j * idr_item_psize),
	     o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);
    }

    idx_ctx.pop();
    return Success;
  }

  Status AttrDirect::realize(Database *db, Object *agr,
			     const Oid& cloid,
			     const Oid& objoid,
			     AttrIdxContext &idx_ctx,
			     const RecMode *rcm) const
  {
    if (is_basic_enum)
      return Success;

    /*
      printf("AttrDirect::realize(%s);\n", name);
    */

    idx_ctx.push(db, cloid, this);

    for (int j = 0; j < typmod.pdims; j++) {
      Object *o;

      // no XDR
      memcpy(&o, agr->getIDR() + idr_voff + (j * idr_item_vsize),
	     sizeof(Object *));

      if (o) {
	Status status;

	status = o->setDatabase(db);
	if (status) return status;

	setCollHints(o, objoid, card);
	status = setCollImpl(db, o, idx_ctx);
	if (status) return status;

	idx_ctx.pushOff(idr_poff +  (j * idr_item_psize));
	/*
	  printf("%s: null_vd_oid: (null) %s %d\n", name,
	  o->getClass()->getName(), IDB_OBJ_HEAD_SIZE);

	*/
	status = o->realizePerform(cloid, objoid, idx_ctx, rcm);

	if (status)
	  return status;

	// needs XDR ? no...
#ifdef E_XDR_TRACE
	printf("%s: AttrDirect::realize : cmp\n", name);
#endif
	/*
	  printf("comparing: %s.%s[%d] %d agroid %s objoid %s [idr_size %d]\n",
	  agr->getClass()->getName(), name, j, idr_poff,
	  agr->getOid().toString(), objoid.toString(),
	  agr->getIDRSize());
	*/
	if (memcmp(agr->getIDR() + idr_poff +  (j * idr_item_psize),
		   o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize)) {
#ifdef E_XDR_TRACE
	  printf("%s: AttrDirect::realize : needs XDR ? no\n", name);
#endif
	  memcpy(agr->getIDR() + idr_poff +  (j * idr_item_psize),
		 o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);

	  // 27/05/06: suppressed test
	  //if (!o->asCollection()) // changed the 9/11/99
	  agr->touch();
	  
	  Bool mustTouch;
	  status = o->postRealizePerform(cloid, objoid,
					 idx_ctx, mustTouch, rcm);
	  if (status)
	    return status;

	  if (mustTouch)
	    agr->touch();
	}

	idx_ctx.popOff();
      }
    }

    idx_ctx.pop();
    return Success;
  }

  Status AttrDirect::remove(Database *db, Object *agr,
			    const Oid& cloid,
			    const Oid& objoid,
			    AttrIdxContext &idx_ctx,
			    const RecMode *rcm) const
  {
    if (is_basic_enum)
      return Success;

    idx_ctx.push(db, cloid, this);
    for (int j = 0; j < typmod.pdims; j++)
      {
	Object *o;

	// no XDR
	memcpy(&o, agr->getIDR() + idr_voff + (j * idr_item_vsize),
	       sizeof(Object *));

	if (o)
	  {
	    Status status;

	    status = o->setDatabase(db);
	    if (status != Success)
	      return status;

	    status = o->removePerform(cloid, objoid, idx_ctx, rcm);

	    if (status != Success)
	      return status;

	    // needs XDR ?
#ifdef E_XDR_TRACE
	    printf("%s: AttrDirect::remove\n", name);
#endif
	    memcpy(agr->getIDR() + idr_poff +  (j * idr_item_psize),
		   o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);
	  }
      }

    idx_ctx.pop();
    return Success;
  }

  Status AttrDirect::check() const
  {
    return Attribute::check();
  }

  Status AttrDirect::compile_perst(const AgregatClass *ma, int *offset, int *size, int *inisize)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    if (status = check())
      return status;

    idr_poff = *offset;
    if (is_basic_enum)
      idr_inisize = iniSize(tmod->pdims);
    else
      idr_inisize = 0;

    if (!cls->getIDRObjectSize(&idr_item_psize))
      return Exception::make(IDB_ATTRIBUTE_ERROR, incomp_fmt, cls->getName(), name, num, class_owner->getName());

    idr_item_psize -= IDB_OBJ_HEAD_SIZE;
    idr_psize = idr_item_psize * tmod->pdims + idr_inisize;
    *inisize = idr_inisize;
  
    compile_update(ma, idr_psize, offset, size);

    return Success;
  }

  Status AttrDirect::compile_volat(const AgregatClass *ma, int *offset, int *size)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    idr_voff = *offset;

    if (!is_basic_enum)
      {
	idr_item_vsize = SIZEOFOBJECT;
	idr_vsize = idr_item_vsize * tmod->pdims;
      }
    else
      {
	idr_item_vsize = 0;
	idr_vsize = 0;
      }

    compile_update(ma, idr_vsize, offset, size);

    return Success;
  }

  Status AttrDirect::setOid(Object *, const Oid *, int, int, Bool) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot set oid for direct attribute '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status AttrDirect::getOid(const Object *agr, Oid *oid, int nb, int from) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot get oid for direct attribute '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status AttrDirect::setValue(Object *agr, Data data,
			      int nb, int from, Bool check_class) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);
    Status status;

    check_range(from, nb);

    Data pdata = agr->getIDR() + idr_poff;
    Data vdata;

    Data inidata;
    if (is_basic_enum) {
      vdata = 0;
      inidata = pdata;
      pdata += idr_inisize;
    }
    else {
      vdata = agr->getIDR() + idr_voff;
      inidata = 0;
    }

    return Attribute::setValue(agr, pdata, data, idr_item_psize,
			       idr_item_psize + IDB_OBJ_HEAD_SIZE, nb,
			       from, inidata, False, vdata, check_class);
  }

  Status AttrDirect::getValue(const Object *agr, Data *data, int nb, int from, Bool *isnull) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);

    check_range(from, nb);

    Data vdata;

    Data inidata;
    if (is_basic_enum) {
      inidata = agr->getIDR() + idr_poff;
      vdata = inidata + idr_inisize;
    }
    else {
      vdata = agr->getIDR() + idr_voff;
      inidata = 0;
    }

    return Attribute::getValue((Database *)agr->getDatabase(), vdata, data,
			       idr_item_psize, nb, from, inidata, isnull);
  }

  /*
    Status AttrDirect::getField(Database *db, const Oid *data_oid,
    Oid *doid, int *roffset, int offset,
    int from) const
    {
    int nb = 1;
    check_range(from, nb);

    *roffset = idr_poff + (from * idr_item_psize) + idr_inisize;
    *doid = *data_oid;

    return Success;
    }
  */

  Status AttrDirect::getVal(Database *db, const Oid *data_oid,
			    Data data, int offset,
			    int nb, int from, Bool *isnull) const
  {
    check_range(from, nb);

    if (idr_inisize && isnull)
      {
	RPCStatus rpc_status;

	if (!from)
	  {
	    int size = idr_inisize + nb * idr_item_psize;
	    Data inidata = (unsigned char *)malloc(size);

	    rpc_status = dataRead(db->getDbHandle(),
				  idr_poff + offset,
				  size, inidata, 0,
				  data_oid->getOid());


	    // needs XDR ?
#ifdef E_XDR_TRACE
	    printf("%s: AttrDirect::getVal -> needs XDR\n", name);
#endif
	    memcpy(data, inidata+idr_inisize, size - idr_inisize);
	    *isnull = isNull(inidata, nb, from);
	    free(inidata);
	    return StatusMake(rpc_status);
	  }
	else
	  {
	    rpc_status = dataRead(db->getDbHandle(),
				  idr_inisize + idr_poff +
				  (from * idr_item_psize) + offset,
				  nb * idr_item_psize, data, 0,
				  data_oid->getOid());

	    if (rpc_status)
	      return StatusMake(rpc_status);

	    Data inidata = (unsigned char *)malloc(idr_inisize);

	    rpc_status = dataRead(db->getDbHandle(),
				  idr_poff + offset, idr_inisize, inidata,
				  0, data_oid->getOid());
	  
	  
	    *isnull = isNull(inidata, nb, from);
	    free(inidata);
	    return StatusMake(rpc_status);
	  }
      }
    else
      return StatusMake(dataRead(db->getDbHandle(),
				 idr_inisize + idr_poff +
				 (from * idr_item_psize) + offset,
				 nb * idr_item_psize, data,
				 0, data_oid->getOid()));
  }

  static const char tvalue_fmt[] = "cannot use the method "
    "Attribute::getTValue() for the non-basic type attribute '%s::%s'";

  static Status
  tvalue_prologue(Database *&db, const Oid &objoid)
  {
    if (objoid.getDbid() != db->getDbid()) {
      Database *xdb;
      Status status = Database::getOpenedDB(objoid.getDbid(), db, xdb);
      if (status) return status;
      if (!xdb) 
	return Exception::make(IDB_ERROR,
			       "cannot get any value of object %s: "
			       "database ID #%d must be manually "
			       "opened by the client",
			       objoid.toString(), objoid.getDbid());
      db = xdb;
    }

    // object conversion
    ObjectHeader hdr;
    RPCStatus rpc_status =
      objectHeaderRead(db->getDbHandle(), objoid.getOid(), &hdr);
    if (rpc_status) return StatusMake(rpc_status);

    if (!db->getSchema()->getClass(hdr.oid_cl)) {
      if (!db->writeBackConvertedObjects())
	return Exception::make(IDB_ERROR, "object %s cannot be converted "
			       "on the fly", objoid.toString());
      /*
	printf("getTValue:: object %s needs a conversion\n",
	objoid.toString());
      */
      Data idr = 0;
      rpc_status = objectRead(db->getDbHandle(), 0, &idr, 0, objoid.getOid(),
			      0, DefaultLock, 0);
      if (rpc_status) return StatusMake(rpc_status);
      free(idr);
    }
    return Success;
  }

  Status
  AttrDirect::getTValue(Database *db, const Oid &objoid,
			Data *data, int nb, int from,
			Bool *isnull, Size *rnb, Offset poffset) const
  {
    if (!is_basic_enum)
      return Exception::make(IDB_ERROR, tvalue_fmt,
			     getClassOwner()->getName(), name);

    check_range(from, nb);

    Status status = tvalue_prologue(db, objoid);
    if (status) return status;

    if (rnb)
      *rnb = nb;

    if (!idr_inisize || !isnull) {
      status = StatusMake(dataRead(db->getDbHandle(),
				   poffset + idr_inisize + idr_poff +
				   (from * idr_item_psize),
				   nb * idr_item_psize, (Data)data,
				   0, objoid.getOid()));
      if (status)
	CHECK_OID(objoid);
      return status;
    }

    RPCStatus rpc_status;

    if (!from) {
      int size = idr_inisize + nb * idr_item_psize;
      Data inidata = (unsigned char *)malloc(size);
    
      rpc_status = dataRead(db->getDbHandle(), poffset + idr_poff, size,
			    inidata, 0, objoid.getOid());


      if (rpc_status) {
	CHECK_OID(objoid);
	return StatusMake(rpc_status);
      }

      *isnull = isNull(inidata, nb, from);

#ifdef E_XDR_TRACE
      printf("%s: AttrDirect::getTValue -> XDR\n", name);
#endif
      if (cls->asEnumClass()) // needs XDR !
	cls->asEnumClass()->getRawData((Data)data, inidata+idr_inisize, nb);
#ifdef E_XDR
      else
	//      cls->decode(data, inidata+idr_inisize, size-idr_inisize);
	cls->decode(data, inidata+idr_inisize, idr_item_psize, nb);
#else
      else
	memcpy(data, inidata+idr_inisize, size-idr_inisize);
#endif

      free(inidata);
      return Success;
    }

    Data xdata = (unsigned char *)malloc(nb * idr_item_psize);

    rpc_status = dataRead(db->getDbHandle(),
			  poffset + idr_inisize + idr_poff +
			  (from * idr_item_psize),
			  nb * idr_item_psize, xdata,
			  0, objoid.getOid());

    if (rpc_status) {
      free(xdata);
      CHECK_OID(objoid);
      return StatusMake(rpc_status);
    }

    if (cls->asEnumClass()) {
      cls->asEnumClass()->getRawData((Data)data, xdata, nb);
    }
    else {
      cls->decode(data, xdata, idr_item_psize, nb);
    }

    free(xdata);

    Data inidata = (unsigned char *)malloc(idr_inisize);

    rpc_status = dataRead(db->getDbHandle(),
			  poffset + idr_poff, idr_inisize, inidata,
			  0, objoid.getOid());
	  
    *isnull = isNull(inidata, nb, from);
    free(inidata);

    if (rpc_status) {
      CHECK_OID(objoid);
      return StatusMake(rpc_status);
    }

    return Success;
  }

#ifdef GBX_NEW_CYCLE
  void AttrDirect::decrRefCountPropag(Object *agr, int) const
  {
    if (is_basic_enum)
      return;

    Attribute::decrRefCountPropag(agr->getIDR() + idr_voff, typmod.maxdims);
  }
#endif

  void AttrDirect::garbage(Object *agr, int) const
  {
    if (is_basic_enum)
      return;

    Attribute::garbage(agr->getIDR() + idr_voff, typmod.maxdims);
  }

  void AttrDirect::manageCycle(Database *db, Object *o, gbxCycleContext &r) const
  {
    if (is_basic_enum)
      return;

    Attribute::manageCycle(o, o->getIDR() + idr_voff, typmod.maxdims, r);
  }

  Status
  AttrDirect::convert(Database *db, ClassConversion *conv,
		      Data in_idr, Size in_size) const
  {
    if (conv->getUpdtype() == ADD_ATTR)
      return add(db, conv, in_idr, in_size);

    return Exception::make(IDB_ERROR, "attribute %s::%s conversion not yet "
			   "implemented", class_owner->getName(), name);
  }

  Status AttrDirect::trace(const Object *agr, FILE *fd, int *indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    char *indent_str = make_indent(*indent);
    TypeModifier tmod = typmod;
    Data pdata = agr->getIDR() + idr_poff;
    char prefix[64];
    Status status = Success;

    get_prefix(agr, class_owner, prefix, sizeof(prefix));

    Data inidata = pdata;
    pdata += idr_inisize;

    if (is_basic_enum)
      {
	fprintf(fd, "%s%s%s = ", indent_str, prefix, name);

	//int len = strlen(indent_str) + strlen(prefix) + strlen(name) + 3;
	int len = *indent + INDENT_INC;
	if (cls->asBasicClass())
	  {
	    if (isNull(inidata, &tmod))
	      PRINT_NULL(fd);
	    else
	      status = ((BasicClass *)cls)->traceData(fd, len, inidata, pdata, &tmod);
	  }
	else
	  {
	    if (isNull(inidata, &tmod))
	      PRINT_NULL(fd);
	    else
	      status = ((EnumClass *)cls)->traceData(fd, len, inidata, pdata, &tmod);
	  }
      
	if (status)
	  goto error;
	fprintf(fd, ";\n");
      }
    else
      for (int j = 0; j < typmod.pdims; j++)
	{
	  Object *o;

	  // no XDR
	  memcpy(&o, agr->getIDR() + idr_voff + (j * idr_item_vsize),
		 sizeof(Object *));

	  if (!tmod.ndims)
	    fprintf(fd, "%s%s%s %s = ", indent_str, prefix, name,
		    cls->getName());
	  else
	    fprintf(fd, "%s%s%s[%d] %s = ", indent_str, prefix, name, j,
		    cls->getName());
	  status = ObjectPeer::trace_realize(o, fd, *indent + INDENT_INC, flags, rcm);
	  if (status)
	    goto error;
	}

  error:
    delete_indent(indent_str);
    return status;
  }

  //
  // AttrIndirect methods
  //

  AttrIndirect::AttrIndirect(const Attribute *agreg,
			     const Class *_cls,
			     const Class *_class_owner,
			     const Class *_dyn_class_owner,
			     int n) :
    Attribute(agreg, _cls, _class_owner, _dyn_class_owner, n)
  {
    code = AttrIndirect_Code;
    idr_inisize = 0;
  }

  AttrIndirect::AttrIndirect(Database *db,
			     Data data, Offset *offset,
			     const Class *_dyn_class_owner,
			     int n) :
    Attribute(db, data, offset, _dyn_class_owner, n)
  {
    code = AttrIndirect_Code;
    idr_inisize = 0;
  }

  Status AttrIndirect::check() const
  {
    return Attribute::check();
  }

  Status
  AttrIndirect::load(Database *db,
		     Object *agr,
		     const Oid &cloid,
		     LockMode lockmode,
		     AttrIdxContext &idx_ctx,
		     const RecMode *rcm, Bool force) const
  {
    Data vdata = agr->getIDR() + idr_voff;

    for (int j = 0; j < typmod.pdims; j++)
      if (rcm->isAgregRecurs(this, j))
	{
	  Oid oid;
	
	  getOid(agr, &oid, 1, j);
	
	  if (oid.isValid())
	    {
	      Status status;
	      Object *o;

	      status = db->loadObject_realize(&oid, &o, lockmode, rcm);
	
	      if (status != Success)
		return status;

	      o->setMustRelease(false); // SMART_PTR
	      status = setValue(agr, (Data)&o, 1, j, False);

	      if (status != Success)
		return status;
	    }
	}

    return Success;
  }

  Status
  Attribute::cardManage(Database *db, Object *agr, int j) const
  {
    Object *o;
    Status status;

    Oid colloid;
    status = ((Agregat *)agr)->getItemOid(this, &colloid, 1, j);

    if (status)
      return status;

#ifdef CARD_TRACE
    printf("Attribute::cardManage(%s -> %s)\n", name, colloid.toString());
#endif
    if (!colloid.isValid())
      return Success;

    status = db->loadObject(&colloid, &o);

    if (status)
      return status;

    Collection *coll = (Collection *)o;

    if (!card->getCardDesc()->compare(coll->getCardinalityConstraint()))
      {
	coll->setCardinalityConstraint(card);

	status = coll->checkCardinality();
	if (status)
	  {
	    o->release();
	    return status;
	  }

	status = coll->realizeCardinality();
	if (status)
	  {
	    o->release();
	    return status;
	  }
      }

    o->release();
    return Success;
  }

  Status
  Attribute::inverseManage(Database *db, Object *agr, int j) const
  {
    if (!cls->asCollectionClass())
      return Success;

    Object *o;
    Status status;

    Oid colloid;

    if (status = agr->asAgregat()->getItemOid(this, &colloid, 1, j))
      return status;

    if (!colloid.isValid())
      return Success;

    if (status = db->loadObject(&colloid, &o))
      return status;

    Collection *coll = (Collection *)o;

    if (status = coll->realizeInverse(agr->getOid(), num))
      {
	o->release();
	return status;
      }
  
    o->release();
    return Success;
  }

  Status
  Attribute::inverseManage(Database *db, Object *agr,
			   Object *coll) const
  {
    if (!cls->asCollectionClass())
      return Success;

    return coll->asCollection()->realizeInverse(agr->getOid(), num);
  }

  std::string
  getAttrCollDefName(const Attribute *attr, const Oid &oid)
  {
    return std::string(attr->getClassOwner()->getName()) + "::" +
      attr->getName() + "[" + oid.toString() + "]";
  }
			     
  static void
  attrCollManage(const Attribute *attr, Collection *coll, Object *o)
  {
    if (!coll->getName() || !*coll->getName())
      coll->setName(getAttrCollDefName(attr, o->getOid()).c_str());
  }

  Status AttrIndirect::realize(Database *db, Object *_o,
			       const Oid& cloid,
			       const Oid& objoid,
			       AttrIdxContext &idx_ctx,
			       const RecMode *rcm) const
  {
    Agregat *agr = (Agregat *)_o;
    Data vdata = agr->getIDR() + idr_voff;

    for (int j = 0; j < typmod.pdims; j++)
      {
	Status status;
	Object *o;

	mcp(&o, vdata + (j * idr_item_vsize), sizeof(Object *));

#ifdef CARD_TRACE
	Oid old_oid;
	status = agr->getItemOid(this, &old_oid, 1, j);
	if (status)
	  return status;
	printf("AttrIndirect::realize(%s, %s, new_o=%s)\n",
	       name, old_oid.toString(), o ? o->getOid().toString() : "NULL");
#endif
	if (o && rcm->isAgregRecurs(this, j, o))
	  {
	    if (!o->getRefCount())
	      abort(); // added the 23/08/99
	    status = o->setDatabase(db);
	    if (status != Success)
	      return status;

	    // ----- adding collection name?? -> 1/07/99 ----
	    if (o->asCollection())
	      attrCollManage(this, o->asCollection(), _o);
	    // ----------------------------------------------

	    status = o->realize(rcm);
	  
	    if (status == Success)
	      status = agr->setItemOid(this, &o->getOid(), 1, j, False);

	    if (status)
	      return status;

	    if (inv_spec.oid_cl.isValid())
	      {
		status = inverseManage(db, agr, o);
		if (status)
		  return status;
	      }
	  }
	else if (!o)
	  {
	    if (card)
	      {
		status = cardManage(db, agr, j);
		if (status)
		  return status;
	      }

	    if (inv_spec.oid_cl.isValid())
	      {
		status = inverseManage(db, agr, j);
		if (status)
		  return status;
	      }
	  }
      }

    return Success;
  }

  Status AttrIndirect::remove(Database *db, Object *_o,
			      const Oid& cloid,
			      const Oid& objoid,
			      AttrIdxContext &idx_ctx,
			      const RecMode *rcm) const
  {
    Agregat *agr = (Agregat *)_o;
    Data vdata = agr->getIDR() + idr_voff;

    for (int j = 0; j < typmod.pdims; j++)
      {
	Status status;
	Object *o;

	// no XDR
	memcpy(&o, vdata + (j * idr_item_vsize), sizeof(Object *));

	if (o && rcm->isAgregRecurs(this, j, o))
	  {
	    status = o->setDatabase(db);
	    if (status != Success)
	      return status;
	    status = o->remove(rcm);

	    if (status == Success ||
		status->getStatus() == IDB_OBJECT_NOT_CREATED)
	      status = agr->setItemOid(this, &Oid::nullOid, 1, j);

	    if (status)
	      return status;
	  }
      }

    return Success;
  }

  Status AttrIndirect::compile_perst(const AgregatClass *ma, int *offset, int *size, int *inisize)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    if (status = check())
      return status;

    idr_inisize = 0;
    idr_poff = *offset;

    idr_item_psize = sizeof(Oid);
    idr_psize = idr_item_psize * tmod->pdims;
    *inisize = idr_inisize;

    compile_update(ma, idr_psize, offset, size);

    return Success;
  }

  Status AttrIndirect::compile_volat(const AgregatClass *ma, int *offset, int *size)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    idr_voff = *offset;

    idr_item_vsize = SIZEOFOBJECT;
    idr_vsize = idr_item_vsize * tmod->pdims;

    compile_update(ma, idr_vsize, offset, size);

    return Success;
  }

  Status AttrIndirect::setOid(Object *agr, const Oid *oid, int nb, int from, Bool check_class) const
  {
    CHECK_OBJ(agr);
    check_range(from, nb);

    if (oid->isValid() && check_class) {
      Class *o_class;
      Status status;
      Bool is;
    
      status = cls->isObjectOfClass(oid, &is, True, &o_class);
      
      if (status)
	return status;
      
      if (!is)
	return Exception::make(IDB_ATTRIBUTE_ERROR,
			       "waiting for object of class '%s', got object of class '%s'",
			       cls->getName(), o_class->getName());
    }
    
#ifdef CARD_TRACE
    printf("AttributeIndirect::setValue(%s, %s, is %sa collection class)\n",
	   name, oid->toString(), cls->asCollectionClass() ? "" : "NOT ");
    Oid old_oid;
    Status status = getOid(agr, &old_oid, nb, from);
    if (status) return status;
    printf("setOid(%s -> old_oid = %s\n", name, old_oid.toString());
#endif

#ifdef E_XDR_TRACE
    printf("%s: setOid -> set_oid_realize\n", name);
#endif
    if (set_oid_realize(agr->getIDR() + idr_poff, oid, nb, from))
      agr->touch();

    return Success;
  }

  Status AttrIndirect::getOid(const Object *agr, Oid *oid, int nb, int from) const
  {
    CHECK_OBJ(agr);
    check_range(from, nb);

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirect::getOid() -> XDR\n", name);
#endif
#ifdef E_XDR_1
    for (int n = 0; n < nb; n++)
      eyedbsm::x2h_oid(oid[n].getOid(), agr->getIDR() + idr_poff +
		       ((from+n) * sizeof(Oid)));
#else
    memcpy(oid, agr->getIDR() + idr_poff + (from * sizeof(Oid)),
	   nb * sizeof(Oid));
#endif
    return Success;
  }

  Status AttrIndirect::setValue(Object *agr, Data data, int nb,
				int from, Bool check_class) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);
    Status status;
    Data vdata = agr->getIDR() + idr_voff;

    check_range(from, nb);

    return Attribute::setValue(agr, vdata, data,
			       SIZEOFOBJECT, sizeof(Object *),
			       nb, from, 0, True, 0, check_class);
  }

  Status AttrIndirect::getValue(const Object *agr, Data *data, int nb, int from, Bool *) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);

    Data vdata = agr->getIDR() + idr_voff;

    check_range(from, nb);

    mcp(data, vdata + from * SIZEOFOBJECT, nb * sizeof(Object *));
    return Success;
  }

  /*
    Status AttrIndirect::getField(Database *db, const Oid *data_oid,
    Oid *doid, int *roffset, int offset,
    int from) const
    {
    int nb = 1;
    check_range(from, nb);

    *roffset = 0;
    return StatusMake
    (dataRead(db->getDbHandle(),
    idr_poff + (from * idr_item_psize) + offset,
    idr_item_psize, (Data)doid, 0, data_oid->getOid()));
    }
  */

  Status AttrIndirect::getVal(Database *db, const Oid *data_oid,
			      Data data, int offset,
			      int nb, int from, Bool *) const
  {
    check_range(from, nb);

    assert(0);
    return StatusMake
      (dataRead(db->getDbHandle(),
		idr_poff + (from * idr_item_psize) + offset,
		nb * idr_item_psize, data, 0, data_oid->getOid()));
  }

  Status
  AttrIndirect::getTValue(Database *db, const Oid &objoid,
			  Data *data, int nb, int from,
			  Bool *isnull, Size *rnb, Offset poffset) const
  {
    check_range(from, nb);

    Status status = tvalue_prologue(db, objoid);
    if (status) return status;

    status = StatusMake
      (dataRead(db->getDbHandle(),
		poffset + idr_poff + (from * idr_item_psize),
		nb * idr_item_psize, (Data)data, 0, objoid.getOid()));

    if (rnb)
      *rnb = nb;

    if (status)
      CHECK_OID(objoid);

    if (status)
      return status;

#ifdef E_XDR_1
    for (int i = 0; i < nb; i++) {
      Oid tmp_oid;
      eyedbsm::x2h_oid(tmp_oid.getOid(), data+i*sizeof(Oid));
      memcpy(data+i*sizeof(Oid), tmp_oid.getOid(), sizeof(Oid));
    }
#endif

    if (!isnull) return status;

    *isnull = True;

    for (int i = 0; i < nb; i++) {
      Oid tmp_oid;
      mcp(&tmp_oid, data+i*sizeof(Oid), sizeof(Oid));
      if (tmp_oid.isValid()) {
	*isnull = False;
	break;
      }
    }

    return Success;
  }

#ifdef GBX_NEW_CYCLE
  void AttrIndirect::decrRefCountPropag(Object *agr, int) const
  {
    Attribute::decrRefCountPropag(agr->getIDR() + idr_voff, typmod.maxdims);
  }
#endif

  void AttrIndirect::garbage(Object *agr, int) const
  {
    Attribute::garbage(agr->getIDR() + idr_voff, typmod.maxdims);
  }

  void AttrIndirect::manageCycle(Database *db, Object *o, gbxCycleContext &r) const
  {
    Attribute::manageCycle(o, o->getIDR() + idr_voff, typmod.maxdims, r);
  }

  Status
  AttrIndirect::convert(Database *db, ClassConversion *conv,
			Data in_idr, Size in_size) const
  {
    return Exception::make(IDB_ERROR, "attribute %s::%s conversion error",
			   class_owner->getName(), name);
  }

  Status AttrIndirect::trace(const Object *agr, FILE *fd, int *indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    char *indent_str = make_indent(*indent);
    TypeModifier tmod = typmod;
    Data vdata = agr->getIDR() + idr_voff;
    char prefix[64];
    Status status = Success;

    get_prefix(agr, class_owner, prefix, sizeof(prefix));

    for (int j = 0; j < typmod.pdims; j++)
      {
	Object *o;
	Oid oid;

	// no XDR
	memcpy(&o, vdata + (j * idr_item_vsize), sizeof(Object *));
	getOid(agr, &oid, 1, j);

	if (o)
	  {
	    if (!tmod.ndims)
	      fprintf(fd, "%s*%s%s %s {%s} = ", indent_str, prefix, name,
		      o->getClass()->getName(), oid.getString());
	    else
	      fprintf(fd, "%s*%s%s[%d] %s {%s} = ", indent_str, prefix, name, j,
		      o->getClass()->getName(), oid.getString());
	    status = ObjectPeer::trace_realize(o, fd, *indent + INDENT_INC, flags, rcm);
	    if (status) break;
	  }
	else
	  {
	    if (!tmod.ndims)
	      fprintf(fd, "%s*%s%s = {%s};\n", indent_str, prefix, name,
		      oid.getString());
	    else
	      fprintf(fd, "%s*%s%s[%d] = {%s};\n", indent_str, prefix, name, j,
		      oid.getString());
	  }
      }

    delete_indent(indent_str);

    return status;
  }

  //
  // AttrVD methods
  //

  AttrVD::AttrVD(const Attribute *agreg,
		 const Class *_cls,
		 const Class *_class_owner,
		 const Class *_dyn_class_owner,
		 int n) :
    Attribute(agreg, _cls, _class_owner, _dyn_class_owner, n)
  {
  }

  AttrVD::AttrVD(Database *db, Data data, Offset *offset,
		 const Class *_dyn_class_owner,
		 int n):
    Attribute(db, data, offset, _dyn_class_owner, n)
  {
  }

  Status
  AttrVD::getDefaultDataspace(const Dataspace *&_dataspace) const
  {
    if (dataspace) {
      _dataspace = dataspace;
      return Success;
    }

    if (dspid == Dataspace::DefaultDspid) {
      _dataspace = 0;
      return Success;
    }

    if (!cls) return Exception::make(IDB_ERROR, "attribute %s is not completed", name);
    Status s = const_cast<Database *>(cls->getDatabase())->getDataspace(dspid, _dataspace);
    if (s) return s;
    const_cast<AttrVD *>(this)->dataspace = _dataspace;
    return Success;
  }

  Status
  AttrVD::setDefaultDataspace(const Dataspace *_dataspace)
  {
    if (!dataspace && dspid != Dataspace::DefaultDspid) {
      if (!cls) return Exception::make(IDB_ERROR, "attribute %s is not completed", name);
      Status s = const_cast<Database *>(cls->getDatabase())->getDataspace(dspid, dataspace);
      if (s) return s;
    }

    if (dataspace != _dataspace) {
      if (!dyn_class_owner) return Exception::make(IDB_ERROR, "attribute %s is not completed", name);
      dataspace = _dataspace;
      dspid = (dataspace ? dataspace->getId() : Dataspace::DefaultDspid);
      const_cast<Class *>(dyn_class_owner)->touch();
      return const_cast<Class *>(dyn_class_owner)->store();
    }

    return Success;
  }

  /*
    Status AttrVD::getField(Database *db, const Oid *data_oid,
    Oid *doid, int *roffset, int offset,
    int from) const
    {
    int nb = 1;
    check_range(from, nb);

    *roffset = from * idr_item_psize;

    return StatusMake
    (dataRead(db->getDbHandle(),
    idr_poff + sizeof(Size),
    sizeof(eyedbsm::Oid), (Data)doid, 0, data_oid->getOid()));
    }
  */

  Status AttrVD::getVal(Database *db, const Oid *data_oid,
			Data data, int offset,
			int nb, int from, Bool *) const
  {
    check_range(from, nb);

    return StatusMake
      (dataRead(db->getDbHandle(),
		idr_poff + (from * idr_item_psize) +
		offset, nb * idr_item_psize,
		data, 0, data_oid->getOid()));
  }

  Status AttrVD::getSize(Data _idr, Size& size) const
  {
#ifdef E_XDR_TRACE
    printf("%s: AttrVD::getSize -> XDR\n", name);
#endif

#ifdef E_XDR
    x2h_32_cpy(&size, _idr + idr_poff);
#else
    mcp(&size, _idr + idr_poff, sizeof(Size));
#endif
    size = CLEAN_SIZE(size);
    return Success;
  }

  Status AttrVD::getSize(const Object *agr, Size& size) const
  {
    return getSize(agr->getIDR(), size);
  }

  Status
  AttrVD::getSize(Database *db, const Oid *data_oid,
		  Size& size) const
  {
    RPCStatus rpc_status;

    rpc_status = dataRead(db->getDbHandle(),
			  idr_poff, sizeof(Size), (Data)&size,
			  0, data_oid->getOid());
#ifdef E_XDR
    Size hsize;
    x2h_32_cpy(&hsize, &size);
    size = CLEAN_SIZE(hsize);
#else
    size = CLEAN_SIZE(size);
#endif
    return Success;
  }

  //
  // AttrVarDim methods
  //

  AttrVarDim::AttrVarDim(const Attribute *agreg,
			 const Class *_cls,
			 const Class *_class_owner,
			 const Class *_dyn_class_owner,
			 int n) :
    AttrVD(agreg, _cls, _class_owner, _dyn_class_owner, n)
  {
    code = AttrVarDim_Code;
    idr_inisize = 0;
  }

  AttrVarDim::AttrVarDim(Database *db, Data data, Offset *offset,
			 const Class *_dyn_class_owner,
			 int n) :
    AttrVD(db, data, offset, _dyn_class_owner, n)
  {
    code = AttrVarDim_Code;
    idr_inisize = 0;
  }

  Status AttrVarDim::check() const
  {
    Status s = Attribute::check();
    if (s) return s;
    const TypeModifier *tmod = &typmod;

    for (int i = 0; i < tmod->ndims; i++)
      if (tmod->dims[i] < 0 && i != 0)
	return Exception::make(IDB_ATTRIBUTE_ERROR, "only left dimension is allowed to be variable for attribute '%s' in agregat class '%s'", name, class_owner->getName());

    return Success;
  }

  int AttrVarDim::getBound(Database *db, Data _idr)
  {
    if (!is_basic_enum)
      {
	Size size;
#ifdef E_XDR_TRACE
	printf("%s: AttrVD::getBound -> XDR\n", name);
#endif
#ifdef E_XDR
	x2h_32_cpy(&size, _idr + idr_poff);
#else
	mcp(&size, _idr + idr_poff, sizeof(Size));
#endif
	size = CLEAN_SIZE(size);
	return (int)size;
      }
    return 0;
  }

  Status AttrVarDim::compile_perst(const AgregatClass *ma, int *offset, int *size, int *inisize)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    if (status = check())
      return status;

    idr_poff = *offset;
    idr_inisize = 0;

    if (!cls->getIDRObjectSize(&idr_item_psize))
      return Exception::make(IDB_ATTRIBUTE_ERROR, incomp_fmt, cls->getName(), name, num, class_owner->getName());

    idr_item_psize -= IDB_OBJ_HEAD_SIZE;

    if (VARS_COND(ma->getDatabase()))
      idr_psize = VARS_OFFSET + VARS_TSZ;
    else
      idr_psize = sizeof(Size) + sizeof(Oid);

    *inisize = idr_inisize;
  
    compile_update(ma, idr_psize, offset, size);

    return Success;
  }

  Status AttrVarDim::compile_volat(const AgregatClass *ma, int *offset, int *size)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    idr_voff = *offset;

    if (!is_basic_enum)
      {
	idr_item_vsize = SIZEOFOBJECT;
	idr_vsize = SIZEOFDATA + SIZEOFDATA;
      }
    else
      {
	idr_item_vsize = 0;
	idr_vsize = SIZEOFDATA;
      }

    compile_update(ma, idr_vsize, offset, size);

    return Success;
  }

  Status AttrVarDim::setOid(Object *, const Oid *, int, int, Bool) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot set oid for direct attribute '%s' in agregat class '%s'", name, class_owner->getName());
  }

  Status AttrVarDim::getOid(const Object *agr, Oid *oid, int nb, int from) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot get oid for direct attribute '%s' in agregat class '%s'", name, class_owner->getName());
  }

  void AttrVarDim::setVarDimOid(Object *agr, const Oid *oid) const
  {
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::setVarDimOid() -> XDR\n", name);
#endif
#ifdef E_XDR
    if (eyedbsm::cmp_oid(agr->getIDR() + idr_poff + sizeof(Size), oid->getOid())) {
      eyedbsm::h2x_oid(agr->getIDR() + idr_poff + sizeof(Size), oid->getOid());
      agr->touch();
    }
#else
    if (memcmp(agr->getIDR() + idr_poff + sizeof(Size), oid,
	       sizeof(Oid))) {
      memcpy(agr->getIDR() + idr_poff + sizeof(Size), oid, sizeof(Oid));
      agr->touch();
    }
#endif
  }

  void AttrVarDim::getVarDimOid(Data _idr, Oid *oid) const
  {
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::getVarDimOid() -> XDR\n", name);
#endif
#ifdef E_XDR
    eyedbsm::x2h_oid(oid->getOid(), _idr + idr_poff + sizeof(Size));
#else
    mcp(oid, _idr + idr_poff + sizeof(Size), sizeof(Oid));
#endif
  }

  void AttrVarDim::getVarDimOid(const Object *agr, Oid *oid) const
  {
    getVarDimOid(agr->getIDR(), oid);
  }

  void AttrVarDim::setSizeChanged(Object *agr, Bool changed) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::setSizeChanged -> XDR\n", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, agr->getIDR() + idr_poff);
    size = SET_SIZE_CHANGED(size, changed);
    h2x_32_cpy(agr->getIDR() + idr_poff, &size);
#else
    mcp(&size, agr->getIDR() + idr_poff, sizeof(Size));
    size = SET_SIZE_CHANGED(size, changed);
    mcp(agr->getIDR() + idr_poff, &size, sizeof(Size));
#endif
  }

  Bool AttrVarDim::isSizeChanged(const Object *agr) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::isSizeChanged -> XDR\n", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, agr->getIDR() + idr_poff);
#else
    mcp(&size, agr->getIDR() + idr_poff, sizeof(Size));
#endif
    return IS_SIZE_CHANGED(size);
  }

  void AttrVarDim::getData(const Database *db, Data _idr, Data& pdata,
			   Data& vdata) const
  {
    if (VARS_COND(db))
      {
	Size size;
	getSize(_idr, size);
	if (size <= VARS_SZ)
	  {
	    pdata = VARS_DATA(_idr);
	    vdata = 0;
	    return;
	  }
      }

    // needs XDR ? guess no because vdata !
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::getData -> needs XDR ? no\n", name);
#endif
    mcp(&pdata, _idr + idr_voff, sizeof(Data));

    if (is_basic_enum)
      vdata = 0;
    else
      mcp(&vdata, _idr + idr_voff + SIZEOFDATA, sizeof(Data));
  }

  Bool AttrVarDim::getIsLoaded(Data _idr) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::getIsLoaded -> XDR\n", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, _idr + idr_poff);
#else
    mcp(&size, _idr + idr_poff, sizeof(Size));
#endif
#ifdef TRACK_BUG1
    printf("%s::getIsLoaded() %d %p\n", name, size, size);
#endif
    if (VARS_COND(cls->getDatabase()) && size <= VARS_SZ)
      return True;
    return IS_LOADED(size);
  }

  void AttrVarDim::setIsLoaded(Data _idr, Bool changed) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::setIsLoaded -> XDR\n", name);
#endif

#ifdef E_XDR
    x2h_32_cpy(&size, _idr + idr_poff);
#else
    mcp(&size, _idr + idr_poff, sizeof(Size));
#endif

    if (VARS_COND(cls->getDatabase()) && size <= VARS_SZ)
      return;

    size = SET_IS_LOADED(size, changed);
#ifdef E_XDR
    h2x_32_cpy(_idr + idr_poff, &size);
#else
    mcp(_idr + idr_poff, &size, sizeof(Size));
#endif
  }

  Bool AttrVarDim::getIsLoaded(const Object *agr) const
  {
    return getIsLoaded(agr->getIDR());
  }

  void AttrVarDim::setIsLoaded(const Object *agr, Bool changed) const
  {
    setIsLoaded(agr->getIDR(), changed);
  }

  void AttrVarDim::getData(const Object *agr, Data& pdata,
			   Data& vdata) const
  {
    getData(getDB(agr), agr->getIDR(), pdata, vdata);
  }

  void AttrVarDim::setData(const Database *db, Data _idr, Data pdata,
			   Data vdata) const
  {
#ifdef TRACK_BUG1
    printf("%s: setting data pdata = %p vdata = %p\n", name, pdata, vdata);
#endif
    Size size;
    getSize(_idr, size);

    //printf("%s: setting string '%s' size=%d pdata=%p\n",
    //name, (pdata ? (char *)pdata : "NULL"), size, pdata);

    // needs XDR ? guess no because vdata !
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::setData -> needs XDR ? no\n", name);
#endif
    mcp(_idr + idr_voff, &pdata, sizeof(Data));

    if (!is_basic_enum)
      mcp(_idr + idr_voff + SIZEOFDATA, &vdata, sizeof(Data));
  }

  void AttrVarDim::setData(const Object *agr, Data pdata,
			   Data vdata) const
  {
    setData(agr->getDatabase(), agr->getIDR(), pdata, vdata);
  }

  void AttrVarDim::getInfo(const Object *agr, Data& pdata,
			   Data& vdata, Size& wpsize,
			   Size& wvsize, Oid &oid) const
  {
    Size size;

    getSize(agr, size);
    getVarDimOid(agr, &oid);
    //printf("getInfo(%s)\n", name);
    getData(agr, pdata, vdata);

    wpsize = size * idr_item_psize * typmod.pdims;
    wvsize = size * idr_item_vsize * typmod.pdims;
  }

  Status
  AttrVarDim::setSize_realize(Object *agr, Data _idr, Size nsize,
			      Bool make, Bool noGarbage) const
  {
    Database *db = getDB(agr);
    if (VARS_COND(db) && nsize <= VARS_SZ) {
      Data opdata, ovdata;
      getData(db, _idr, opdata, ovdata);
      if (opdata != VARS_DATA(_idr))
	free(opdata);

#ifdef E_XDR_TRACE
      printf("%s: AttrVarDim::setSize_realize #1 -> XDR\n", name);
#endif
#ifdef E_XDR
      h2x_32_cpy(_idr + idr_poff, &nsize);
#else
      mcp(_idr + idr_poff, &nsize, sizeof(Size));
#endif
      return Success;
    }

    Size rsize;
    Data opdata, ovdata, npdata, nvdata;

    int wpsize;
    Size osize;
    int oinisize, rinisize, ninisize;
      
    rinisize = oinisize = ninisize = 0;

    Bool isLoaded = getIsLoaded(_idr);

    if (make) {
#ifdef E_XDR_TRACE
      printf("%s: AttrVarDim::setSize_realize #2 -> XDR\n", name);
#endif
#ifdef E_XDR
      x2h_32_cpy(&osize, _idr + idr_poff);
#else
      mcp(&osize, _idr + idr_poff, sizeof(Size));
#endif
      osize = CLEAN_SIZE(osize);
      rsize = ((osize < nsize) ? osize : nsize);
    }
    else
      osize = 0;

    getData(db, _idr, opdata, ovdata);

    if (is_basic_enum) {
      oinisize = iniSize(osize);
      ninisize = iniSize(nsize);
      rinisize = ((oinisize < ninisize) ? oinisize : ninisize);
    }

#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::setSize_realize #3 -> XDR\n", name);
#endif
#ifdef E_XDR
    h2x_32_cpy(_idr + idr_poff, &nsize);
#else
    mcp(_idr + idr_poff, &nsize, sizeof(Size));
#endif
  
    setIsLoaded(_idr, isLoaded);

    if (nsize == 0) {
      setData(db, _idr, 0, 0);
      if (opdata != VARS_DATA(_idr))
	free(opdata);
      if (is_basic_enum) // added this test the 3/09/01
	free(ovdata);
    }
    else {
      wpsize = idr_item_psize * nsize * typmod.pdims;
      npdata = (unsigned char *)malloc(wpsize + ninisize);

      if (make) {
	Size wrsize = idr_item_psize * rsize * typmod.pdims;

	mcp(npdata, opdata, rinisize);
	mset(npdata + rinisize, 0, ninisize - rinisize);

	// needs XDR ? no...
#ifdef E_XDR_TRACE
	printf("%s: AttrVarDim::setSize_realize #4 -> needs XDR ? no\n", name);
#endif
	memcpy(npdata + ninisize, opdata + oinisize, wrsize);
	memset(npdata + ninisize + wrsize, 0, wpsize - wrsize);
      }
      else
	memset(npdata, 0, wpsize + ninisize);
  
      if (opdata != VARS_DATA(_idr))
	free(opdata);
    
      if (idr_item_vsize)	{
	int wvsize = idr_item_vsize * nsize * typmod.pdims;
	nvdata = (unsigned char *)malloc(wvsize);
      
	if (make) {
	  // needs XDR ? no...
#ifdef E_XDR_TRACE
	  printf("%s: AttrVarDim::setSize_realize #5 -> needs XDR ? no\n", name);
#endif
	  Size wrsize = idr_item_vsize * rsize * typmod.pdims;
	  memcpy(nvdata, ovdata, wrsize);
	  memset(nvdata + wrsize, 0, wvsize - wrsize);
	}
	else
	  memset(nvdata, 0, wvsize);
      
	if (is_basic_enum) // added this test the 3/09/01
	  free(ovdata);
      }
      else
	nvdata = 0;
    
      setData(db, _idr, npdata, nvdata);
    }
  
    if (is_basic_enum)
      return Success;

    GBX_SUSPEND();

    if (!noGarbage)
      for (int j = nsize; j < osize; j++) {
	Object *oo;
	// no XDR
	memcpy(&oo, ovdata + (j * idr_item_vsize), sizeof(Object *));
      
	if (oo)
	  oo->release();
      }

    free(ovdata);

    for (int j = osize; j < nsize; j++) // 20/09/01: was for (int j = 0; 
      {
	Object *oo;
	// no XDR
	memcpy(&oo, nvdata + (j * idr_item_vsize), sizeof(Object *));

	if (!oo) {
	  oo = (Object *)cls->newObj(npdata + (j * idr_item_psize));
	  oo->setMustRelease(false); // SMART_PTR
	  // no XDR
	  memcpy(nvdata + (j * idr_item_vsize), &oo, sizeof(Object *));
	}

	// added the 5/11/99
	Status s = oo->setMasterObject(agr);
	if (s)
	  return s;
      }

    return Success;
  }

#define AUTOLOAD(DB, AGR)						\
  do {									\
    if ((DB) && !getIsLoaded(AGR))					\
      {									\
	Status status = load(DB, AGR, (AGR)->getClass()->getOid(), DefaultLock, \
			     null_idx_ctx, NoRecurs, True);		\
	if (status) return status;					\
      }									\
  } while(0)

  Status AttrVarDim::setSize(Object *agr, Size nsize) const
  {
    Size osize;
    Status status;

    getSize(agr, osize);

    if (nsize == osize)
      return Success;

    AUTOLOAD(agr->getDatabase(), agr);

    status = setSize_realize(agr, agr->getIDR(), nsize, True);

    if (!status) {
      agr->touch();
      setSizeChanged(agr, True);
    }

    return status;
  }

  Status AttrVarDim::setValue(Object *agr, Data data, int nb, int from, Bool check_class) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);
    Size size;
    check_var_range(agr, from, nb, &size);

    if (from != 0 || nb != size)
      AUTOLOAD(agr->getDatabase(), agr);

    setIsLoaded(agr, True);

    Data pdata, vdata, inidata;

    //printf("setValue(%s)\n", name);
    getData(agr, pdata, vdata);

    if (is_basic_enum)
      iniCompute(agr->getDatabase(), size, pdata, inidata);
    else
      inidata = 0;

    Status s = Attribute::setValue(agr, pdata, data, idr_item_psize, 0, nb, from,
				   inidata, False, vdata, True);
    //printf("%s: setValue -> %s pdata=%p, inidata=%p\n",
    //name, data, pdata, inidata);
    /*
      if (VARS_COND(agr->getDatabase()))
      {
      Data zdata = 0;
      getValue(agr, &zdata, directAccess, 0, 0);
      printf("----> getvalue returns %s\n", zdata);
      }
    */

    return s;
  }

  Status AttrVarDim::getValue(const Object *agr, Data *data, int nb, int from, Bool *isnull) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);

    Size sz;
    check_var_range(agr, from, nb, &sz);

    AUTOLOAD((Database *)agr->getDatabase(), (Object *)agr);

    Data pdata, vdata, xdata, inidata;
    //printf("getValue(%s)\n", name);
    getData(agr, pdata, vdata);

    Size size;

    if (is_basic_enum) {
      xdata = pdata;
      size = idr_item_psize;
      iniCompute(agr->getDatabase(), sz, xdata, inidata);
    }
    else {
      xdata = vdata;
      size = SIZEOFOBJECT;
      inidata = 0;
    }

    return Attribute::getValue((Database *)agr->getDatabase(), xdata,
			       data, size, nb, from, inidata, isnull);
  }

  Status
  AttrVarDim::getTValue(Database *db, const Oid &objoid,
			Data *data, int nb, int from,
			Bool *isnull, Size *rnb, Offset poffset) const
  {
    if (!is_basic_enum)
      return Exception::make(IDB_ERROR, tvalue_fmt,
			     getClassOwner()->getName(), name);

    if (cls->asEnumClass())
      return Exception::make(IDB_ERROR,
			     "variable dimension array for enums is "
			     "not yet implemented for getTValue()");

    Status status = tvalue_prologue(db, objoid);
    if (status) return status;

    unsigned char infodata[sizeof(Size)+sizeof(Oid)];
    if (status = StatusMake
	(dataRead(db->getDbHandle(),
		  poffset + idr_poff,
		  sizeof(Size)+sizeof(Oid), infodata,
		  0, objoid.getOid()))) {
      CHECK_OID(objoid);
      return status;
    }

    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::getTValue #1 -> XDR\n", name);
#endif

#ifdef E_XDR
    x2h_32_cpy(&size, infodata);
#else
    mcp(&size, infodata, sizeof(Size));
#endif
    size = CLEAN_SIZE(size);

    if (status = checkVarRange(from, nb, size))
      return status;

    if (rnb)
      *rnb = size;

    Oid data_oid;
    Size inisize;
    Offset offset;
    if (VARS_COND(db) && size <= VARS_SZ) {
      data_oid = objoid;
      offset = poffset + idr_poff + VARS_OFFSET;
    }
    else {
#ifdef E_XDR_TRACE
      printf("%s: AttrVarDim::getTValue #2 -> XDR", name);
#endif
#ifdef E_XDR
      eyedbsm::x2h_oid(data_oid.getOid(), infodata+sizeof(Size));
#else
      mcp(&data_oid, infodata+sizeof(Size), sizeof(Oid));
#endif
      if (!data_oid.isValid()) {
	if (isnull)
	  *isnull = True;
	return Success;
      }
      offset = 0;
    }
  
    inisize = iniSize(size);
    Bool toAlloc;
    Data pdata;

    if (nb == wholeData) {
      pdata = 0;
      toAlloc = True;
      nb = size;
    }
    else {
      pdata = (Data)data;
      toAlloc = False;
    }

    // added the 15/05/00
    if (!nb && !from) {
      if (toAlloc)
	*data = pdata = (unsigned char *)malloc(1);
      *pdata = 0;
    
      if (isnull)
	*isnull = True;
      return Success;
    }
    // ...

    if (!inisize || !isnull) {
      if (toAlloc)
	*data = pdata = (unsigned char *)malloc(nb * idr_item_psize);
      if (status = StatusMake
	  (dataRead(db->getDbHandle(),
		    inisize + (from * idr_item_psize) + offset,
		    nb * idr_item_psize, pdata, 0, data_oid.getOid()))) {
	if (toAlloc)
	  free(*data);
	CHECK_OID(objoid);
	return status;
      }

      return Success;
    }

    // 17/09/04: this seems to be thought as an optimisation !?
    // I think, this is not needed !
#if 0
    if (0) {
      unsigned char *xdata = (unsigned char *)malloc(nb * idr_item_psize +
						     inisize);
      if (status = StatusMake
	  (dataRead(db->getDbHandle(),
		    offset,
		    inisize + nb * idr_item_psize, xdata,
		    0, data_oid.getOid()))) {
	free(xdata);
	CHECK_OID(objoid);
	return status;
      }

      *isnull = isNull(xdata, nb, from);
      if (toAlloc)
	*data = pdata = (unsigned char *)malloc(nb * idr_item_psize);
#ifdef E_XDR_TRACE
      printf("%s: AttrVarDim::getTValue -> XDR [nb=%d]\n", name, nb);
#endif
#ifdef E_XDR
      cls->decode(pdata, xdata+inisize, idr_item_psize, nb);
#else
      memcpy(pdata, xdata+inisize, nb * idr_item_psize);
#endif
    
      free(xdata);
      return Success;
    }
#endif

    unsigned char *inidata = (unsigned char *)malloc(inisize);
    if (status = StatusMake
	(dataRead(db->getDbHandle(),
		  offset,
		  inisize, inidata, 0, data_oid.getOid()))) {
      free(inidata);
      CHECK_OID(objoid);
      return status;
    }
  
    *isnull = isNull(inidata, nb, from);
    if (toAlloc)
      *data = pdata = new unsigned char[nb * idr_item_psize];
#ifdef E_XDR
    unsigned char *xdata = (unsigned char *)malloc(nb * idr_item_psize);
#endif

    if (status = StatusMake
	(dataRead(db->getDbHandle(),
		  inisize + (from * idr_item_psize) + offset,
		  nb * idr_item_psize,
#ifdef E_XDR
		  xdata,
#else
		  pdata,
#endif
		  0, data_oid.getOid()))) {
      if (toAlloc)
	free(*data);
#ifdef E_XDR
      free(xdata);
#endif
      CHECK_OID(objoid);
      return status;
    }

#ifdef E_XDR
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::getTValue -> XDR [nb=%d]\n", name, nb);
#endif
    cls->decode(pdata, xdata, idr_item_psize, nb);
    free(xdata);
#endif
    return Success;
  }

#ifdef GBX_NEW_CYCLE
  void AttrVarDim::decrRefCountPropag(Object *agr, int) const
  {
    if (!is_basic_enum) {
      Data pdata, vdata;
      Size size;

      getData(agr, pdata, vdata);

      if (vdata) {
	getSize(agr, size);
	Attribute::decrRefCountPropag(vdata, size);
      }
    }
  }
#endif

  void AttrVarDim::garbage(Object *agr, int refcnt) const
  {
    if (!is_basic_enum)
      {
	Data pdata, vdata;
	Size size;

	getData(agr, pdata, vdata);

	if (vdata)
	  {
	    getSize(agr, size);
	    Attribute::garbage(vdata, size);
	  }
      }

    if (refcnt <= 1) // was refcnt == 1 12/10/00
      {
	// changed the 16/05/00
#if 1
	//setSize_realize(agr, agr->getIDR(), 0, False);
	// changed the 12/10/00
	setSize_realize(agr, agr->getIDR(), 0, True, True); // added last argument the 3/09/01
#else
	setIsLoaded(agr, True);
	setSize(agr, 0);
#endif
      }
  }

  void AttrVarDim::manageCycle(Database *db, Object *o, gbxCycleContext &r) const
  {
    if (is_basic_enum)
      return;

    Data pdata, vdata;
    Size size;

    //printf("ManageCycle(%s)\n", name);
    getData(o, pdata, vdata);

    if (vdata)
      {
	getSize(o, size);
	Attribute::manageCycle(o, vdata, size, r);
      }
  }

  void
  AttrVarDim::newObjRealize(Object *o) const
  {
    if (is_basic_enum)
      {
	Data _idr = o->getIDR();
	Size size;
	getSize(_idr, size);

	//      printf("%s: newObjRealize(size=%d)\n", name, size);
	// QUESTION:
	// pourquoi faire un setSize_realize() dans la base is_basic_enum
	// et pas dans l'autre cas?
	// (voir aussi ref ##1):

	// 13/2/2: ATTENTION ce test a disparu avec le nouveau système
	// d'index: mais est-il toujours utile du fait que ce qui est
	// signalé plus bas ("Special case: data is NULL and ...") 
	// semblent impliquer les varstring NULL uniquement: ce qui
	// n'est plus un problème puisque les varstring NULL ont
	// de toute façon une longueur <= VARS_SZ et donc il n'est
	// pas nécessaire de créer une VDATA

	if (VARS_COND(o->getDatabase()) && size <= VARS_SZ)
	  return;
	else if (size)
	  {
	    Data pdata, vdata;
	    getData(o->getDatabase(), _idr, pdata, vdata);
	    if (!pdata) // not that this test is new!
	      setSize_realize(o, _idr, size, False);
	  }
      }

    setIsLoaded(o, True);
  }

  Status AttrVarDim::copy(Object *agr, Bool share) const
  {
    Data _idr = agr->getIDR();
    Size size;
    getSize(_idr, size);

    if (size)
      {
	AUTOLOAD(agr->getDatabase(), agr);
	Data pdata, vdata;
	//printf("copy(%s)\n", name);
	getData(getDB(agr), _idr, pdata, vdata);

	if (!share)
	  {
	    Data npdata, nvdata;
	    int wpsize = size * idr_item_psize * typmod.pdims +
	      (is_basic_enum ? iniSize(size) : 0);

	    // needs XDR ?
#ifdef E_XDR_TRACE
	    printf("%s: AttrVarDim::copy -> needs XDR ? no\n", name);
#endif

	    // added this test 12/10/00 because of memory leaks
	    if (pdata == VARS_DATA(_idr))
	      npdata = pdata;
	    else
	      {
		npdata = (unsigned char *)malloc(wpsize);
		// needs XDR ? no
		memcpy(npdata, pdata, wpsize);
	      }	  

	    if (vdata)
	      {
		int wvsize = size * idr_item_vsize * typmod.pdims;
		nvdata = (unsigned char *)malloc(wvsize);
		// needs XDR ? no
		memcpy(nvdata, vdata, wvsize);
		Status s = Attribute::incrRefCount(agr, nvdata, size);
		if (s)
		  return s;
	      }
	    else
	      nvdata = 0;

	    setData(agr->getDatabase(), _idr, npdata, nvdata);
	  }
	else if (vdata) {
	  Status s = Attribute::incrRefCount(agr, vdata, size);
	  if (s)
	    return s;
	}
      }

    return Success;
  }

  Status AttrVD::update_realize(Database *db, Object *agr,
				const Oid& cloid,
				const Oid& objoid,
				int count, Size wpsize, Data pdata,
				Oid &oid,
				AttrIdxContext &idx_ctx) const
  {
    // added the 23/01/00
    // before this patch, notnull constraints on vardim with no index did
    // not work at all
    Bool notnull, notnull_comp, unique, unique_comp;
    Status s = constraintPrologue(db, idx_ctx, notnull_comp, notnull,
				  unique_comp, unique);
    if (!wpsize && (notnull || notnull_comp))
      return Exception::make
	((notnull ? IDB_NOTNULL_CONSTRAINT_ERROR :
	  IDB_NOTNULL_COMP_CONSTRAINT_ERROR),
	 const_error, idx_ctx.getAttrName().c_str());

    //printf("wpsize(%s) = %d\n", name, wpsize);
    if (oid.isValid() || wpsize)
      {
	RPCStatus rpc_status;
	const eyedbsm::Oid *cl_oid = agr->getClass()->getOid().getOid();
	const eyedbsm::Oid *actual_cl_oid = cloid.getOid();
	const eyedbsm::Oid *agr_oid = objoid.getOid();

	//      if (isSizeChanged(agr))
	// changed the 17/05/99 because of a bug found!
	if (isSizeChanged(agr) || !oid.isValid())
	  {
	    Size idx_size;
	    Data idx_data = idx_ctx.code(idx_size);
	    if (oid.isValid())
	      {
		rpc_status = VDdataDelete(db->getDbHandle(), actual_cl_oid,
					  cl_oid, num,
					  agr_oid, oid.getOid(),
					  idx_data, idx_size);

		if (rpc_status)
		  {
		    free(idx_data);
		    return StatusMake(rpc_status);
		  }

		agr->setDirty(True);
		setVarDimOid(agr, &Oid::nullOid);
	      }

	    if (wpsize && !(VARS_COND(db) && wpsize <= VARS_TSZ))
	      {
		if ((rpc_status = VDdataCreate(db->getDbHandle(),
					       dspid,
					       actual_cl_oid,
					       cl_oid, num, count,
					       wpsize, pdata,
					       agr_oid, oid.getOid(),
					       idx_data, idx_size)) ==
		    RPCSuccess)
		  {
		    setVarDimOid(agr, &oid);
		    agr->setDirty(True);
		  }
	      
		if (rpc_status)
		  {
		    setVarDimOid(agr, &Oid::nullOid);
		    free(idx_data);
		    return StatusMake(rpc_status);
		  }
	      }
	    else if (!VARS_COND(db))
	      setVarDimOid(agr, &Oid::nullOid);

	    setSizeChanged(agr, False);
	    free(idx_data);
	    return Success;
	  }
	else if (agr->isModify() && !(VARS_COND(db) && wpsize <= VARS_TSZ))
	  {
	    Size idx_size;
	    Data idx_data = idx_ctx.code(idx_size);
	    rpc_status = VDdataWrite(db->getDbHandle(),
				     actual_cl_oid,
				     cl_oid, num, count,
				     wpsize, pdata, agr_oid, oid.getOid(),
				     idx_data, idx_size);
	  
	    free(idx_data);

	    if (rpc_status)
	      return StatusMake(rpc_status);

	    agr->setDirty(True);
	    return Success;
	  }
      }

    agr->setDirty(True);
    return Success;
  }

  Status AttrVD::remove_realize(Database *db,
				const Oid& actual_cl_oid,
				const Oid& objoid,
				Object *agr,
				AttrIdxContext &idx_ctx) const
  {
    Oid oid;

    getVarDimOid(agr, &oid);

    if (oid.isValid())
      {
	RPCStatus rpc_status;
	const eyedbsm::Oid *cl_oid = agr->getClass()->getOid().getOid();
	const eyedbsm::Oid *agr_oid = objoid.getOid();
	Size idx_size;
	Data idx_data = idx_ctx.code(idx_size);

	rpc_status = VDdataDelete(db->getDbHandle(),
				  actual_cl_oid.getOid(), cl_oid, num,
				  agr_oid, oid.getOid(),
				  idx_data, idx_size);
	if (!rpc_status)
	  agr->setDirty(True);

	return StatusMake(rpc_status);
      }

    return Success;
  }

  Status AttrVarDim::update(Database *db,
			    const Oid& cloid,
			    const Oid& objoid,
			    Object *agr,
			    AttrIdxContext &idx_ctx) const
  {
    if (!getIsLoaded(agr))
      return Success;

    Size wpsize, wvsize, size;
    Data pdata, vdata;
    Oid oid;

    getInfo(agr, pdata, vdata, wpsize, wvsize, oid);
    getSize(agr, size);

    if (is_basic_enum)
      wpsize += iniSize(size);

    Status s = update_realize(db, agr, cloid, objoid, size, wpsize, pdata, oid,
			      idx_ctx);
    getInfo(agr, pdata, vdata, wpsize, wvsize, oid);
    return s;
  }

  Status AttrVarDim::realize(Database *db, Object *agr,
			     const Oid& cloid,
			     const Oid& objoid,
			     AttrIdxContext &idx_ctx,
			     const RecMode *rcm) const
  {
    if (!getIsLoaded(agr))
      return Success;

    idx_ctx.push(db, cloid, this);

    Status status;
    //printf("realize(%s)\n", name);
    if (is_basic_enum) {
      status = update(db, cloid, objoid, agr, idx_ctx);
      idx_ctx.pop();
      return status;
    }

    Size size;
    Data pdata, vdata;
    int dd;

    getSize(agr, size);
    getData(agr, pdata, vdata);
    Oid vd_oid;
    getVarDimOid(agr, &vd_oid);

    dd = typmod.pdims * size;

    //#define TRACE_OFF

#ifdef TRACE_OFF
    Offset OFFSET;
    eyedbsm::Oid xoid, hoid;
#endif

    for (int j = 0; j < dd; j++) {
      Object *o;

      mcp(&o, vdata + (j * SIZEOFOBJECT),  sizeof(Object *));

      if (o) {
	status = o->setDatabase(db);
	if (status != Success)
	  return status;

	//idx_ctx.pushOff(idr_poff +  (j * idr_item_psize), vd_oid);
#ifdef TRACE_OFF
	getVarDimOid(agr, &vd_oid);
	printf("#1 %s vd_oid %s\n", name, vd_oid.toString());
#endif

	idx_ctx.pushOff(j * idr_item_psize, vd_oid);

#ifdef TRACE_OFF
	memcpy(&xoid, pdata +  + (j * idr_item_psize), sizeof(xoid));
	eyedbsm::x2h_oid(&hoid, &xoid);
	printf("#1 %s pdata %s at %d\n", name, Oid(hoid).toString(),  + (j * idr_item_psize));
#endif

	/*
	  printf("%s: vd_oid: %s %s %d\n", name, vd_oid.toString(),
	  o->getClass()->getName(), IDB_OBJ_HEAD_SIZE);
	*/
	status = o->realizePerform(cloid, objoid, idx_ctx, rcm);
	  
	if (status)
	  return status;

#ifdef TRACE_OFF
	getVarDimOid(agr, &vd_oid);
	printf("#2 %s vd_oid %s\n", name, vd_oid.toString());

	memcpy(&xoid, pdata +  + (j * idr_item_psize), sizeof(xoid));
	eyedbsm::x2h_oid(&hoid, &xoid);
	printf("#2 %s pdata %s at %d\n", name, Oid(hoid).toString(),  + (j * idr_item_psize));
#endif

	// needs XDR ? no
#ifdef E_XDR_TRACE
	printf("%s: AttrVarDim::realize\n", name);
#endif
	if (memcmp(pdata +  (j * idr_item_psize),
		   o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize)) {
	  memcpy(pdata +  (j * idr_item_psize),
		 o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);
	
	  // added 27/05/06
	  agr->touch();

	  Bool mustTouch;
	  status = o->postRealizePerform(cloid, objoid,
					 idx_ctx, mustTouch, rcm);
	  if (status)
	    return status;
	
	  if (mustTouch)
	    agr->touch();
	}
      
#ifdef TRACE_OFF
	OFFSET = idx_ctx.getOff();
#endif

	idx_ctx.popOff();
      }
    }

    status = update(db, cloid, objoid, agr, idx_ctx);

#ifdef TRACE_OFF
    getVarDimOid(agr, &vd_oid);
    printf("#3 %s vd_oid %s\n", name, vd_oid.toString());
    if (pdata) {
      memcpy(&xoid, pdata, sizeof(xoid));
      eyedbsm::x2h_oid(&hoid, &xoid);
      printf("#3 %s pdata %s at %d\n", name, Oid(hoid).toString(), 0);
    }
    dataRead(db->getDbHandle(), OFFSET,
	     sizeof(eyedbsm::Oid),
	     (Data)&xoid, 0,
	     vd_oid.getOid());
    eyedbsm::x2h_oid(&hoid, &xoid);
    printf("#3 %s DB pdata %s\n", name, Oid(hoid).toString());
    printf("\n");
#endif

    idx_ctx.pop();
    return status;
  }

  Status AttrVarDim::remove(Database *db, Object *agr,
			    const Oid& cloid,
			    const Oid& objoid,
			    AttrIdxContext &idx_ctx,
			    const RecMode *rcm) const
  {
    Status status;

    idx_ctx.push(db, cloid, this);

    if (is_basic_enum)
      {
	status = remove_realize(db, cloid, objoid, agr, idx_ctx);
	idx_ctx.pop();
	return status;
      }

    AUTOLOAD(db, agr);

    Size size;
    Data pdata, vdata;
    int dd;

    getSize(agr, size);
    getData(agr, pdata, vdata);

    dd = typmod.pdims * size;

    for (int j = 0; j < dd; j++)
      {
	Object *o;

	// no XDR
	memcpy(&o, vdata + (j * SIZEOFOBJECT),  sizeof(Object *));

	if (o)
	  {
	    status = o->setDatabase(db);
	    if (status != Success)
	      return status;

	    status = o->removePerform(cloid, objoid, idx_ctx, rcm);

	    if (status != Success)
	      return status;

	    // needs XDR ?
	    memcpy(pdata +  (j * idr_item_psize),
		   o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);
	  }
      }

    status = remove_realize(db, cloid, objoid, agr, idx_ctx);
    idx_ctx.pop();
    return status;
  }

  Status
  AttrVarDim::load(Database *db,
		   Object *agr,
		   const Oid &cloid,
		   LockMode lockmode,
		   AttrIdxContext &idx_ctx,
		   const RecMode *rcm, Bool force) const
  {
    /*
      Size size;
      getSize(agr, size);
    */

    //printf("AttrVarDim::load(%s)\n", name);
    if (!force)
      {
	setIsLoaded(agr, False); // MIND: added on the 15/09/98
	// ref ##1: je pense que les 2 ligne suivantes ne sont pas
	// necessaire, dans la mesure ou on a fait un 
	// setSize_realize dans newObjRealize.
	// Attention cependant, ce setSize_realize n'a ete fait
	// que pour les is_basic_enum, je crois!

	/* disconnected the 16/5/00 */
	/*
	  setSize_realize(agr, agr->getIDR(), size, False);
	  setSizeChanged(agr, False);
	*/
	// added the 3/2/00
	if (rcm->getType() == RecMode_NoRecurs)
	  return Success;
      }

    Size size;
    getSize(agr, size);

    setIsLoaded(agr, True);

    Size wpsize, wvsize;
    Data pdata, vdata;
    Oid oid;

    getInfo(agr, pdata, vdata, wpsize, wvsize, oid);

    if (!oid.isValid())
      return Success;

    if (is_basic_enum)
      wpsize += iniSize(size);

    if (wpsize)
      {
	// added the 24/05/00
	if (!pdata)
	  {
	    setSize_realize(agr, agr->getIDR(), size, False);
	    setSizeChanged(agr, False);
	    getData(db, agr->getIDR(), pdata, vdata);
	  }

	RPCStatus rpc_status;

	rpc_status = dataRead(db->getDbHandle(), 0, wpsize, pdata,
			      0, oid.getOid());

	if (rpc_status != RPCSuccess)
	  return StatusMake(rpc_status);
      }

    if (is_basic_enum)
      return Success;

    int dd;

    dd = typmod.pdims * size;

    idx_ctx.push(db, cloid, this);
#ifdef E_XDR_TRACE
    printf("%s: AttrVarDim::load -> needs XDR ? no\n", name);
#endif
    for (int j = 0; j < dd; j++)
      {
	Status status;
	Object *o;

	// no XDR
	memcpy(&o, vdata + (j * SIZEOFOBJECT), sizeof(Object *));

	// needs XDR ? no
	memcpy(o->getIDR() + IDB_OBJ_HEAD_SIZE,
	       pdata +  (j * idr_item_psize), idr_item_psize);

	status = o->setDatabase(db);

	if (status != Success)
	  return status;

	status = o->loadPerform(cloid, lockmode, idx_ctx, rcm);

	if (status != Success)
	  return status;

	memcpy(pdata +  (j * idr_item_psize),
	       o->getIDR() + IDB_OBJ_HEAD_SIZE, idr_item_psize);
      }

    idx_ctx.pop();
    return Success;
  }

  Status
  AttrVarDim::convert(Database *db, ClassConversion *conv,
		      Data in_idr, Size in_size) const
  {
    if (conv->getUpdtype() == ADD_ATTR)
      return add(db, conv, in_idr, in_size);

    return Exception::make(IDB_ERROR, "attribute %s::%s conversion error",
			   class_owner->getName(), name);
  }

  Status AttrVarDim::trace(const Object *agr, FILE *fd, int *indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    char *indent_str = make_indent(*indent);
    TypeModifier tmod = typmod;
    Size size;
    Data pdata, vdata;
    char prefix[64];
    Status status = Success;

    AUTOLOAD((Database *)agr->getDatabase(), (Object *)agr);

    get_prefix(agr, class_owner, prefix, sizeof(prefix));

    getSize(agr, size);
    getData(agr, pdata, vdata);
#ifdef TRACK_BUG1
    printf("%s::trace(): %s pdata = %p size = %d\n", name,
	   agr->getOid().toString(), pdata, size);
#endif

    tmod.pdims *= size;

    if (is_basic_enum)
      {
	Data inidata = pdata;
	pdata += iniSize(size);
	fprintf(fd, "%s%s%s = ", indent_str, prefix, name);

	//int len = strlen(indent_str) + strlen(prefix) + strlen(name) + 3;
	int len = *indent + INDENT_INC;
	if (cls->asBasicClass())
	  {
	    if (isNull(inidata, &tmod))
	      PRINT_NULL(fd);
	    else
	      status = ((BasicClass *)cls)->traceData(fd, len, inidata, pdata, &tmod);
	  }
	else
	  {
	    if (isNull(inidata, &tmod))
	      PRINT_NULL(fd);
	    else
	      status = ((EnumClass *)cls)->traceData(fd, len, inidata, pdata, &tmod);
	  }
	fprintf(fd, ";\n");
      }
    else
      for (int j = 0; j < tmod.pdims; j++)
	{
	  Object *o;

#ifdef TRACK_BUG1
	  printf("%s::trace() -> #%d\n", name, j);
#endif
	  // no XDR
	  memcpy(&o, vdata + (j * SIZEOFOBJECT), sizeof(Object *));

	  if (!tmod.ndims)
	    fprintf(fd, "%s%s%s %s = ", indent_str, prefix, name,
		    cls->getName());
	  else
	    fprintf(fd, "%s%s%s[%d] %s = ", indent_str, prefix, name, j,
		    cls->getName());
	  status = ObjectPeer::trace_realize(o, fd, *indent + INDENT_INC, flags, rcm);
	  if (status) break;
	}

    delete_indent(indent_str);
    return status;
  }

  //
  // AttrIndirectVarDim methods
  //

  AttrIndirectVarDim::AttrIndirectVarDim(const Attribute *agreg,
					 const Class *_cls,
					 const Class *_class_owner,
					 const Class *_dyn_class_owner,
					 int n) :
    AttrVD(agreg, _cls, _class_owner, _dyn_class_owner, n)
  {
    code = AttrIndirectVarDim_Code;
    idr_inisize = 0;
  }

  AttrIndirectVarDim::AttrIndirectVarDim(Database *db,
					 Data data, Offset *offset,
					 const Class *_dyn_class_owner,
					 int n) :
    AttrVD(db, data, offset, _dyn_class_owner, n)
  {
    code = AttrIndirectVarDim_Code;
    idr_inisize = 0;
  }

  Status AttrIndirectVarDim::check() const
  {
    Status s = Attribute::check();
    if (s) return s;
    const TypeModifier *tmod = &typmod;

    for (int i = 0; i < tmod->ndims; i++)
      if (tmod->dims[i] < 0 && i != 0)
	return Exception::make(IDB_ATTRIBUTE_ERROR, "only left dimension is allowed to be variable in attribute '%s' in agregat class '%s'",
			       name, class_owner->getName());

    return Success;
  }

  Status AttrIndirectVarDim::setSize(Object *agr, Size nsize) const
  {
    Size osize;

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setSize #1 -> XDR\n", name);
#endif

#ifdef E_XDR
    x2h_32_cpy(&osize, agr->getIDR() + idr_poff);
#else
    mcp(&osize, agr->getIDR() + idr_poff, sizeof(Size));
#endif
    osize = CLEAN_SIZE(osize);

    if (osize == nsize)
      return Success;

    Size rsize = ((osize < nsize) ? osize : nsize);

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setSize #2 -> XDR\n", name);
#endif

#ifdef E_XDR
    h2x_32_cpy(agr->getIDR() + idr_poff, &nsize);
#else
    mcp(agr->getIDR() + idr_poff, &nsize, sizeof(Size));
#endif

    Data odata, ndata, dummy;
    Size wrsize;

    getData(agr, odata, dummy);
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setSize()\n", name);
#endif
    if (nsize == 0)
      ndata = 0;
    else {
      Size wsize = idr_item_vsize * nsize * typmod.pdims;
      ndata = (unsigned char *)malloc(wsize);
      wrsize = idr_item_vsize * rsize * typmod.pdims;

      // needs XDR ? no
      memcpy(ndata, odata, wrsize);
      memset(ndata + wrsize, 0, wsize - wrsize);
    }

    setData(agr, ndata);
    free(odata);

    getDataOids(agr, odata);

    if (nsize == 0)
      ndata = 0;
    else {
      Size wsize = sizeof(Oid) * nsize * typmod.pdims;
      ndata = (unsigned char *)malloc(wsize);
      wrsize = sizeof(Oid) * rsize * typmod.pdims;
    
      // needs XDR ? no
      memcpy(ndata, odata, wrsize);
      memset(ndata + wrsize, 0, wsize - wrsize);
    }

    free(odata);
    setDataOids(agr, ndata);

    setSizeChanged(agr, True);
    agr->touch();
    return Success;
  }

  void AttrIndirectVarDim::setSizeChanged(Object *agr, Bool changed) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setSizeChanged -> XDR\n", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, agr->getIDR() + idr_poff);
    size = SET_SIZE_CHANGED(size, changed);
    h2x_32_cpy(agr->getIDR() + idr_poff, &size);
#else
    mcp(&size, agr->getIDR() + idr_poff, sizeof(Size));
    size = SET_SIZE_CHANGED(size, changed);
    mcp(agr->getIDR() + idr_poff, &size, sizeof(Size));
#endif
  }

  Bool AttrIndirectVarDim::isSizeChanged(const Object *agr) const
  {
    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::isSizeChanged -> XDR\n", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, agr->getIDR() + idr_poff);
#else
    mcp(&size, agr->getIDR() + idr_poff, sizeof(Size));
#endif
    return IS_SIZE_CHANGED(size);
  }

  void AttrIndirectVarDim::getData(const Database *, Data _idr, Data& data, Data& dummy) const
  {
    // needs XDR ? guess no
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::getData -> needs XDR ? no\n", name);
#endif
    mcp(&data, _idr + idr_voff, sizeof(Data));
  }

  void AttrIndirectVarDim::getData(const Object *agr, Data& data, Data& dummy) const
  {
    // needs XDR ? guess no
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::getData -> needs XDR ? no\n", name);
#endif
    mcp(&data, agr->getIDR() + idr_voff, sizeof(Data));
  }

  void AttrIndirectVarDim::setData(const Object *agr, Data data) const
  {
    // needs XDR ? guess no
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setData -> needs XDR ? no\n", name);
#endif
    mcp(agr->getIDR() + idr_voff, &data, sizeof(Data));
  }

  void AttrIndirectVarDim::setData(const Database *, Data _idr, Data data) const
  {
    // needs XDR ? guess no
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setData -> needs XDR ? no\n", name);
#endif
    mcp(_idr + idr_voff, &data, sizeof(Data));
  }

  void AttrIndirectVarDim::getDataOids(Data _idr, Data& data) const
  {
    mcp(&data, _idr + idr_voff + SIZEOFDATA, sizeof(Data));
  }

  void AttrIndirectVarDim::getDataOids(const Object *agr, Data& data) const
  {
    mcp(&data, agr->getIDR() + idr_voff + SIZEOFDATA, sizeof(Data));
  }

  void AttrIndirectVarDim::setDataOids(Data _idr, Data data) const
  {
    mcp(_idr + idr_voff + SIZEOFDATA, &data, sizeof(Data));
  }

  void AttrIndirectVarDim::setDataOids(Object *agr, Data data) const
  {
    mcp(agr->getIDR() + idr_voff + SIZEOFDATA, &data, sizeof(Data));
  }

  void AttrIndirectVarDim::getInfo(const Object *agr, Size &wsize, Data &data, Oid &oid) const
  {
    Size size;

    getSize(agr, size);
    getVarDimOid(agr, &oid);
    Data dummy;
    getData(agr, data, dummy);

    wsize = size * idr_item_vsize * typmod.pdims;
  }

  void AttrIndirectVarDim::getInfoOids(const Object *agr, Size &wsize, Data &data, Oid &oid) const
  {
    Size size;

    getSize(agr, size);
    getVarDimOid(agr, &oid);
    getDataOids(agr, data);

    wsize = size * sizeof(Oid) * typmod.pdims;
  }

  Status AttrIndirectVarDim::compile_perst(const AgregatClass *ma, int *offset, int *size, int *inisize)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    if (status = check())
      return status;

    idr_poff = *offset;
    idr_inisize = 0;

    idr_item_psize = sizeof(Oid);
    idr_psize = sizeof(Size) + sizeof(Oid);
    *inisize = idr_inisize;
  
    compile_update(ma, idr_psize, offset, size);

    return Success;
  }

  Status AttrIndirectVarDim::compile_volat(const AgregatClass *ma, int *offset, int *size)
  {
    Status status;
    const TypeModifier *tmod = &typmod;

    idr_voff = *offset;

    idr_item_vsize = SIZEOFOBJECT;
    idr_vsize = SIZEOFDATA + SIZEOFDATA;

    compile_update(ma, idr_vsize, offset, size);

    return Success;
  }

  void AttrIndirectVarDim::setVarDimOid(Object *agr, const Oid *oid) const
  {
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setVarDimOid -> XDR\n", name);
#endif
#ifdef E_XDR
    if (eyedbsm::cmp_oid(agr->getIDR() + idr_poff + sizeof(Size), oid->getOid())) {
      eyedbsm::h2x_oid(agr->getIDR() + idr_poff + sizeof(Size), oid->getOid());
      agr->touch();
    }
#else
    if (memcmp(agr->getIDR() + idr_poff + sizeof(Size), oid,
	       sizeof(Oid))) {
      mcp(agr->getIDR() + idr_poff + sizeof(Size), oid, sizeof(Oid));
      agr->touch();
    }
#endif
    // 17/09/04: seems to be a tests => disconnected 
#if 0
    else {
      Oid tmp;
      dataRead(agr->getDatabase()->getDbHandle(),
	       idr_poff + sizeof(Size), sizeof(Oid),
	       (const Data)&tmp,
	       0, agr->getOid().getOid());
      if (tmp != *oid)
	printf("HORROR differences %s vs. %s\n",
	       tmp.getString(), oid->getString());
    }
#endif
  }

  void AttrIndirectVarDim::getVarDimOid(Data _idr, Oid *oid) const
  {
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::getVarDimOid -> XDR\n", name);
#endif
#ifdef E_XDR
    eyedbsm::x2h_oid(oid->getOid(), _idr + idr_poff + sizeof(Size));
#else
    mcp(oid, _idr + idr_poff + sizeof(Size), sizeof(Oid));
#endif
  }

  void AttrIndirectVarDim::getVarDimOid(const Object *agr, Oid *oid) const
  {
    getVarDimOid(agr->getIDR(), oid);
  }

  Status AttrIndirectVarDim::setOid(Object *agr, const Oid *oid, int nb, int from, Bool check_class) const
  {
    CHECK_OBJ(agr);
    check_var_range(agr, from, nb, 0);

    if (oid->isValid() && check_class) {
      Class *o_class;
      Status status;
      Bool is;

      status = cls->isObjectOfClass(oid, &is, True, &o_class);
      
      if (status) {
	return status;
      }

      if (!is) {
	return Exception::make(IDB_ATTRIBUTE_ERROR,
			       "waiting for object of class '%s', got object of class '%s'",
			       cls->getName(), o_class->getName());
      }
    }

    
    Data data;
    getDataOids(agr, data);

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::setOid -> XDR\n", name);
#endif
    // changed 17/09/04
    set_oid_realize(data, oid, nb, from);
    /*
      if (memcmp(data + (from * sizeof(Oid)), oid, nb * sizeof(Oid)))
      {
      mcp(data + (from * sizeof(Oid)), oid, nb * sizeof(Oid)); 
      agr->touch();
      }
    */

    return Success;
  }

  Status AttrIndirectVarDim::getOid(const Object *agr, Oid *oid, int nb, int from) const
  {
    CHECK_OBJ(agr);
    check_var_range(agr, from, nb, 0);

    Data data;
    getDataOids(agr, data);

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::getOid -> XDR\n", name);
#endif

#ifdef E_XDR_1
    for (int n = 0; n < nb; n++)
      eyedbsm::x2h_oid(oid[n].getOid(), data + (from + n) * sizeof(Oid)); 
#else
    mcp(oid, data + (from * sizeof(Oid)), nb * sizeof(Oid)); 
#endif
    return Success;
  }

  Status AttrIndirectVarDim::setValue(Object *agr, Data data,
				      int nb, int from,
				      Bool check_class) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);
    check_var_range(agr, from, nb, 0);

    Data vdata, dummy;
    getData(agr, vdata, dummy);

    return Attribute::setValue(agr, vdata, data, SIZEOFOBJECT,
			       sizeof(Object *), nb, from, 0, True, 0,
			       check_class);
  }

  Status AttrIndirectVarDim::getValue(const Object *agr, Data *data, int nb, int from, Bool *isnull) const
  {
    CHECK_OBJ(agr);
    CHECK_RTTYPE(agr);

    check_var_range(agr, from, nb, 0);

    Data vdata, dummy;
    getData(agr, vdata, dummy);

    mcp(data, vdata + from * SIZEOFOBJECT, nb * sizeof(Object *));
    return Success;
  }

  Status
  AttrIndirectVarDim::getTValue(Database *db, const Oid &objoid,
				Data *data, int nb, int from,
				Bool *isnull, Size *rnb, Offset poffset) const
  {
    Status status = tvalue_prologue(db, objoid);
    if (status) return status;

    unsigned char infodata[sizeof(Size)+sizeof(Oid)];
    if (status = StatusMake
	(dataRead(db->getDbHandle(),
		  poffset + idr_poff,
		  sizeof(Size)+sizeof(Oid), infodata,
		  0, objoid.getOid())))
      {
	CHECK_OID(objoid);
	return status;
      }

    Size size;
#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim::getTValue -> XDR", name);
#endif
#ifdef E_XDR
    x2h_32_cpy(&size, infodata);
#else
    mcp(&size, infodata, sizeof(Size));
#endif
    size = CLEAN_SIZE(size);

    if (status = checkVarRange(from, nb, size))
      return status;

    if (rnb)
      *rnb = size;

    Oid data_oid;
#ifdef E_XDR
    eyedbsm::x2h_oid(data_oid.getOid(), infodata+sizeof(Size));
#else
    mcp(&data_oid, infodata+sizeof(Size), sizeof(Oid));
#endif

    if (!data_oid.isValid()) {
      if (isnull)
	*isnull = True;
      return Success;
    }

    Bool toAlloc;
    Data pdata;

    if (nb == wholeData) {
      pdata = 0;
      toAlloc = True;
      nb = size;
    }
    else
      {
	pdata = (Data)data;
	toAlloc = False;
      }

    if (toAlloc)
      *data = pdata = (unsigned char *)malloc(nb * idr_item_psize);

    if (status = StatusMake
	(dataRead(db->getDbHandle(),
		  from * idr_item_psize,
		  nb * idr_item_psize, pdata, 0, data_oid.getOid())))
      {
	if (toAlloc)
	  free(*data);
	CHECK_OID(objoid);
	return status;
      }
  
#ifdef E_XDR_1
    //printf("IVD::getTValue() ??\n");
    for (int i = 0; i < nb; i++) {
      Oid tmp_oid;
      eyedbsm::x2h_oid(tmp_oid.getOid(), pdata+i*sizeof(Oid));
      memcpy(pdata+i*sizeof(Oid), tmp_oid.getOid(), sizeof(Oid));
    }
#endif

    if (!isnull)
      return Success;

    *isnull = True;
    for (int i = 0; i < nb; i++) {
      Oid tmp_oid;
      mcp(&tmp_oid, pdata+i*sizeof(Oid), sizeof(Oid));
      if (tmp_oid.isValid()) {
	*isnull = False;
	break;
      }
    }

    return Success;
  }

  Status AttrIndirectVarDim::copy(Object *agr, Bool share) const
  {
    Data _idr = agr->getIDR();
    Size size;
    getSize(_idr, size);

#ifdef E_XDR_TRACE
    printf("%s: AttrIndirectVarDim\n", name);
#endif
    if (size)
      {
	Data pdata, vdata, dummy;
	getData(getDB(agr), _idr, vdata, dummy);

	if (!share)
	  {
	    getDataOids(_idr, pdata);
	  
	    int wpsize = size * idr_item_psize * typmod.pdims;
	    Data npdata = (unsigned char *)malloc(wpsize);
	    // needs XDR ? no
	    memcpy(npdata, pdata, wpsize);

	    int wvsize = size * idr_item_vsize * typmod.pdims;
	    Data nvdata = (unsigned char *)malloc(wvsize);
	    // needs XDR ? no
	    memcpy(nvdata, vdata, wvsize);
	  
	    Status s = Attribute::incrRefCount(agr, nvdata, size);
	    if (s)
	      return s;
	  
	    setData(agr->getDatabase(), _idr, nvdata);
	    setDataOids(_idr, npdata);
	  }
	else if (vdata) {
	  Status s = Attribute::incrRefCount(agr, vdata, size);
	  if (s)
	    return s;
	}
      }

    return Success;
  }

#ifdef GBX_NEW_CYCLE
  void AttrIndirectVarDim::decrRefCountPropag(Object *agr, int) const
  {
    Data vdata, dummy;
    getData(agr, vdata, dummy);

    if (vdata) {
      Size size;
      getSize(agr, size);
      Attribute::decrRefCountPropag(vdata, typmod.pdims * size);
    }
  }
#endif

  void AttrIndirectVarDim::garbage(Object *agr, int refcnt) const
  {
    Data vdata, dummy;
    getData(agr, vdata, dummy);

    if (vdata)
      {
	Size size;
	getSize(agr, size);
	Attribute::garbage(vdata, typmod.pdims * size);
	if (refcnt <= 1) // was refcnt == 1 12/10/00
	  setSize(agr, 0);
      }
  }

  void AttrIndirectVarDim::manageCycle(Database *db, Object *o, gbxCycleContext &r) const
  {
    Data vdata, dummy;
    getData(o, vdata, dummy);
    if (vdata)
      {
	Size size;
	getSize(o, size);
	Attribute::manageCycle(o, vdata, typmod.pdims * size, r);
      }
  }

  Status AttrIndirectVarDim::trace(const Object *agr, FILE *fd, int *indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    char *indent_str = make_indent(*indent);
    TypeModifier tmod = typmod;
    Size size;
    Data vdata;
    char prefix[64];
    Status status = Success;

    get_prefix(agr, class_owner, prefix, sizeof(prefix));

    getSize(agr, size);
    Data dummy;
    getData(agr, vdata, dummy);

    tmod.pdims *= size;

    for (int j = 0; j < tmod.pdims; j++)
      {
	Object *o;
	Oid oid;

	mcp(&o, vdata + (j * idr_item_vsize), sizeof(Object *));
	getOid(agr, &oid, 1, j);

	if (o) {
	  fprintf(fd, "%s%s%s[%d] %s {%s} = ", indent_str, prefix, name, j,
		  o->getClass()->getName(), oid.getString());
	  status = ObjectPeer::trace_realize(o, fd, *indent + INDENT_INC, flags, rcm);
	  if (status) break;
	}
	else
	  fprintf(fd, "%s%s%s[%d] = {%s};\n", indent_str, prefix, name, j,
		  oid.getString());
      }

    delete_indent(indent_str);
    return status;
  }

  Status
  AttrIndirectVarDim::realize(Database *db, Object *_o,
			      const Oid& cloid,
			      const Oid& objoid,
			      AttrIdxContext &idx_ctx,
			      const RecMode *rcm) const
  {
    Size size;
    int dd;
    Agregat *agr = (Agregat *)_o;

    getSize(agr, size);
    Data vdata, dummy;
    getData(agr, vdata, dummy);

    dd = typmod.pdims * size;

    for (int j = 0; j < dd; j++) {
      Status status;
      Object *o;

      mcp(&o, vdata + (j * idr_item_vsize), sizeof(Object *));

      if (o && rcm->isAgregRecurs(this, j, o)) {
	status = o->setDatabase(db);
	if (status) {
	  return status;
	}

	if (o->asCollection()) {
	  attrCollManage(this, o->asCollection(), _o);
	}

	status = o->realize(rcm);

	if (status == Success) {
	  status = agr->setItemOid(this, &o->getOid(), 1, j, False);
	}

	if (status) {
	  return status;
	}
      }
      else if (o && o->getOid().isValid()) {
	status = agr->setItemOid(this, &o->getOid(), 1, j, False);
	
	if (status) {
	  return status;
	}
      }
      else if (!o) {
	if (card) {
	  status = cardManage(db, agr, j);
	  if (status)
	    return status;
	}
	
	if (inv_spec.oid_cl.isValid()) {
	  status = inverseManage(db, agr, j);
	  if (status) {
	    return status;
	  }
	}
      }
    }

    return update(db, cloid, objoid, agr, idx_ctx);
  }

  Status
  AttrIndirectVarDim::remove(Database *db, Object *_o,
			     const Oid& cloid,
			     const Oid& objoid,
			     AttrIdxContext &idx_ctx,
			     const RecMode *rcm) const
  {
    Size size;
    Data vdata;
    int dd;
    Agregat *agr = (Agregat *)_o;

    getSize(agr, size);
    Data dummy;
    getData(agr, vdata, dummy);

    dd = typmod.pdims * size;

    for (int j = 0; j < dd; j++)
      {
	Status status;
	Object *o;

	mcp(&o, vdata + (j * idr_item_vsize), sizeof(Object *));

	if (o && rcm->isAgregRecurs(this, j, o))
	  {
	    status = o->setDatabase(db);
	    if (status != Success)
	      return status;
	    status = o->remove(rcm);

	    if (status == Success ||
		status->getStatus() == IDB_OBJECT_NOT_CREATED)
	      status = agr->setItemOid(this, &Oid::nullOid, 1, j);

	    if (status)
	      return status;
	  }
      }

    return remove_realize(db, cloid, objoid, agr, idx_ctx);
  }

  Status AttrIndirectVarDim::update(Database *db,
				    const Oid& cloid,
				    const Oid& objoid,
				    Object *agr,
				    AttrIdxContext &idx_ctx) const
  {
    Size wsize, size;
    Data data;
    Oid oid;

    getInfoOids(agr, wsize, data, oid);
    getSize(agr, size);

    return update_realize(db, agr, cloid, objoid, size, wsize, data, oid, idx_ctx);
  }

  Status
  AttrIndirectVarDim::convert(Database *db, ClassConversion *conv,
			      Data in_idr, Size in_size) const
  {
    return Exception::make(IDB_ERROR, "attribute %s::%s conversion error",
			   class_owner->getName(), name);
  }

  Status
  AttrIndirectVarDim::load(Database *db,
			   Object *agr,
			   const Oid &cloid,
			   LockMode lockmode, 
			   AttrIdxContext &idx_ctx,
			   const RecMode *rcm, Bool force) const
  {
    Size wsize;
    Data data;
    Oid oid;

    getInfoOids(agr, wsize, data, oid);

    if (wsize)
      {
	RPCStatus rpc_status;

	free(data);
	data = (unsigned char *)malloc(wsize);

	if ((rpc_status = dataRead(db->getDbHandle(), 0, wsize, data, 0, oid.getOid()))
	    == RPCSuccess)
	  {
	    setDataOids(agr, data);
	    getInfo(agr, wsize, data, oid);
	    data = (unsigned char *)malloc(wsize);
	    memset(data, 0, wsize);
	    setData(agr, data);
	  }
	else
	  free(data);

	if (rpc_status != RPCSuccess)
	  return StatusMake(rpc_status);
      }

    Size size;
    getSize(agr, size);

    setSizeChanged(agr, False);

    int dd = size * typmod.pdims;

    for (int j = 0; j < dd; j++)
      if (rcm->isAgregRecurs(this, j))
	{
	  Oid oid;
	
	  getOid(agr, &oid, 1, j);
	
	  if (oid.isValid())
	    {
	      Status status;
	      Object *o;

	      status = db->loadObject_realize(&oid, &o, lockmode, rcm);
	
	      if (status != Success)
		return status;

	      o->setMustRelease(false); // SMART_PTR

	      status = setValue(agr, (Data)&o, 1, j, False);

	      if (status != Success)
		return status;
	    }
	}

    return Success;
  }

  void Attribute::pre_release()
  {
    if (attr_comp_set)
      attr_comp_set->release();

    attr_comp_set = 0;
  }

  Attribute::~Attribute()
  {
    free((void *)name);
  }

  //  s = std::string((TYP)i);

#define MAKE_STR(TYP)				\
  do {						\
    TYP i;					\
    mcp(&i, data, sizeof(i));			\
    s = str_convert((TYP)i);			\
    return s.c_str();				\
  } while(0)

  const char *
  Attribute::dumpData(Data data)
  {
    static std::string s;

    if (isIndirect()) {
      Oid oid;
      mcp(&oid, data, sizeof(Oid));
      s = oid.toString();
      return s.c_str();
    }
  
    if (cls->asInt32Class())
      MAKE_STR(eyedblib::int32);

    if (cls->asInt64Class())
      MAKE_STR(eyedblib::int64);

    if (cls->asInt16Class())
      MAKE_STR(eyedblib::int16);

    if (cls->asFloatClass())
      MAKE_STR(double);

    if (cls->asCharClass())
      MAKE_STR(char);

    if (cls->asOidClass()) {
      //MAKE_STR(Oid);
      Oid x;
      mcp(&x, data, sizeof(x));
      s = x.toString();
      return s.c_str();
    }

    s = "";
    for (int i = 0; i < idr_item_psize; i++)
      {
	char tok[16];
	sprintf(tok, "%02x", data[i]);
	s += tok;
      }
    return s.c_str();
  }

  Status
  Attribute::getDefaultDataspace(const Dataspace *&) const
  {
    return Exception::make(IDB_ERROR,
			   "attribute %s: can get or set default dataspace"
			   " only on variable dimension attributes", name);
  }

  Status
  Attribute::setDefaultDataspace(const Dataspace *)
  {
    return Exception::make(IDB_ERROR,
			   "attribute %s: can get or set default dataspace"
			   " only on variable dimension attributes",
			   name);
  }

  void *Attribute::setUserData(void *nuser_data)
  {
    void *x = user_data;
    user_data = nuser_data;
    return x;
  }

  //
  // AttrIdxContext methods
  //

  AttrIdxContext::AttrIdxContext(const Data data, Size size)
  {
    init();

    if (!size)
      return;

    attrpath_computed = False;
    Offset offset = 0;
    char *s;
    string_decode(data, &offset, &s);
    class_owner = *s ? strdup(s) : 0;
    int dummy;
    int32_decode(data, &offset, &dummy);
    int16_decode(data, &offset, &attr_cnt);
    toFree = True;
    for (int i = 0; i < attr_cnt; i++) {
      string_decode(data, &offset, (char **)&s);
      attrs[i] = s;
    }
  }

  AttrIdxContext::AttrIdxContext(AttrIdxContext *idx_ctx)
  {
    idx_ctx_root = idx_ctx;
    attr_cnt = idx_ctx->attr_cnt;
    attr_off_cnt = idx_ctx->attr_off_cnt;
    class_owner = idx_ctx->class_owner;
    toFree = False;

    for (int n = 0; n < attr_cnt; n++)
      attrs[n] = idx_ctx->attrs[n];

    for (int n = 0; n < attr_off_cnt; n++)
      attr_off[n] = idx_ctx->attr_off[n];

    attrpath_computed = False;

    idx_ops_alloc = idx_ops_cnt = 0;
    idx_ops = 0;
  }

  std::string
  AttrIdxContext::getString() const
  {
    return getAttrName();
  }

  std::string
  AttrIdxContext::getAttrName(Bool ignore_class_owner) const
  {
    if (attrpath_computed && attrpath_ignore_class_owner == ignore_class_owner)
      return attrpath;

    attrpath_computed = True;
    attrpath_ignore_class_owner = ignore_class_owner;

    if (!ignore_class_owner) {
      if (!class_owner) {
	*attrpath = 0;
	return attrpath;
      }

      strcpy(attrpath, class_owner);

      for (int i = 0; i < attr_cnt; i++) {
	strcat(attrpath, ".");
	strcat(attrpath, attrs[i].c_str());
      }

      return attrpath;
    }

    *attrpath = 0;
    for (int i = 0; i < attr_cnt; i++) {
      if (i) strcat(attrpath, ".");
      strcat(attrpath, attrs[i].c_str());
    }

    return attrpath;
  }

  Data
  AttrIdxContext::code(Size &size) const
  {
    Data data = 0;
    Offset offset = 0;
    Size alloc_size = 0;
    string_code(&data, &offset, &alloc_size,
		(class_owner ? class_owner : ""));
    int dummy = 0;
    int32_code(&data, &offset, &alloc_size, &dummy);
    int16_code(&data, &offset, &alloc_size, &attr_cnt);

    for (int i = 0; i < attr_cnt; i++)
      string_code(&data, &offset, &alloc_size, attrs[i].c_str());

    size = offset;
    return data;
  }

  int
  AttrIdxContext::operator==(const AttrIdxContext &idx_ctx) const
  {
    if (strcmp(class_owner, idx_ctx.class_owner))
      return 0;

    if (attr_cnt != idx_ctx.attr_cnt)
      return 0;

    for (int i = 0; i < attr_cnt; i++)
      if (strcmp(attrs[i].c_str(), idx_ctx.attrs[i].c_str()))
	return 0;

    return 1;
  }

  Attribute *
  AttrIdxContext::getAttribute(const Class *cls) const
  {
    const Attribute *attr = 0;
    for (int i = 0; i < attr_cnt; i++) {
      attr = cls->getAttribute(attrs[i].c_str());
      assert(attr);
      cls = attr->getClass();
      assert(cls);
    }

    return const_cast<Attribute *>(attr);
  }

  void AttrIdxContext::addIdxOP(const Attribute *attr, IdxOP op,
				Index *idx_t, eyedbsm::Idx *idx,
				unsigned char *data,
				unsigned int sz, Oid data_oid[2])
  {
    if (idx_ctx_root) {
      idx_ctx_root->addIdxOP(attr, op, idx_t, idx, data, sz, data_oid);
      return;
    }

    eyedblib::MutexLocker mtl(mut);
    if (idx_ops_cnt >= idx_ops_alloc) {
      idx_ops_alloc += 8;
      idx_ops = (IdxOperation *)realloc(idx_ops, sizeof(IdxOperation)*
					idx_ops_alloc);
    }

    IdxOperation *xop = &idx_ops[idx_ops_cnt++];
    xop->attr = attr;
    xop->op = op;
    xop->idx_t = idx_t;
    xop->idx = idx;
    xop->data = (unsigned char *)malloc(sz+1);
    memcpy(xop->data, data, sz);
    xop->data[sz] = 0;

    xop->data_oid[0] = data_oid[0];
    xop->data_oid[1] = data_oid[1];

#ifdef NEW_NOTNULL_TRACE
    printf("add_%s_op : %s::%s, sz=%d, oid=%s",
	   (op == IdxInsert ? "insert" : "remove"),
	   attr->getClassOwner() ? attr->getClassOwner()->getName() : "<NULL>",
	   attr->getName(),
	   sz,
	   data_oid[0].toString());
  
    if (attr->isString())
      printf(", data='%s'", data);
    else {
      printf("data {\n");
      for (int i = 0; i < sz; i++)
	printf("%s%d", (i > 0 ? ", " : ""), data[i]);
      printf("}\n");
    }

    printf("\n");
#endif
  }

  Status
  AttrIdxContext::realizeIdxOP(Bool ok)
  {
    static const char fmt_error[] = "storage manager error '%s' reported when updating index entry in attribute '%s' in agregat class '%s'";

    assert(!idx_ctx_root);
    int ops_cnt = idx_ops_cnt;
    idx_ops_cnt = 0;
    for (int i = 0; i < ops_cnt; i++) {
      IdxOperation *xop = &idx_ops[i];
      if (ok) {
	eyedbsm::Status s;
#ifdef TRACE_IDX
	printf("xop->op %s %s::%s\n", xop->op == IdxInsert ? "insert" : "remove", 
	       xop->attr->getClassOwner() ? xop->attr->getClassOwner()->getName() : "<NULL>",
	       xop->idx_t->getAttrpath().c_str());
#endif
	if (xop->op == IdxInsert) {
	  s = xop->idx->insert(xop->data, xop->data_oid);
	  if (eyedbsm::hidx_gccnt > 20) {
	    fprintf(stdout, "index %s getcell -> %d\n", xop->idx_t->getAttrpath().c_str(), eyedbsm::hidx_gccnt);
	    fflush(stdout);
	  }
	  eyedbsm::hidx_gccnt = 0;
	}
	else {
	  eyedbsm::Boolean found;
	  s = xop->idx->remove(xop->data, xop->data_oid, &found);
	  if (!s && !found)
	    return Exception::make(IDB_INDEX_ERROR, fmt_error,
				   "index entry not found",
				   xop->idx_t->getAttrpath().c_str(),
				   xop->attr->getClassOwner()->getName());
	}

	if (s)
	  return Exception::make(IDB_INDEX_ERROR, fmt_error,
				 eyedbsm::statusGet(s),
				 //xop->attr->getName(),
				 xop->idx_t->getAttrpath().c_str(),
				 xop->attr->getClassOwner()->getName());
      }

      free(xop->data);
      xop->data = 0;
    }

    return Success;
  }

  void
  AttrIdxContext::pushOff(int off)
  {
    pushOff(off, Oid::nullOid);
  }

  void
  AttrIdxContext::pushOff(int off, const Oid &data_oid)
  {
    // 27/05/06: no more useful
#ifdef TRACE_OFF
    printf("pushing [%d] %d %s\n", attr_off_cnt, off, data_oid.toString());
#endif
    attr_off[attr_off_cnt].off = off;  
    attr_off[attr_off_cnt].data_oid = data_oid;
    attr_off_cnt++;
  }

#undef TRACE_OFF

  void
  AttrIdxContext::popOff()
  {
    attr_off_cnt--;
#ifdef TRACE_OFF
    printf("popOff [%d]\n", attr_off_cnt);
#endif
  }

  int
  AttrIdxContext::getOff()
  {
    int off = 0;

#ifdef TRACE_OFF
    printf("getOff(%d)\n", attr_off_cnt);
#endif
    for (int n = attr_off_cnt-1; n >= 0; n--) {
      off += attr_off[n].off;
      if (attr_off[n].data_oid.isValid()) {
	break;
      }
    }

    off -= (attr_off_cnt - 1) * IDB_OBJ_HEAD_SIZE;
    //printf("getOff n=%d off=%d\n", attr_off_cnt, off);
    return off;
  }

  Oid
  AttrIdxContext::getDataOid()
  {
    for (int n = attr_off_cnt-1; n >= 0; n--) {
      if (attr_off[n].data_oid.isValid())
	return attr_off[n].data_oid;
    }

    return Oid::nullOid;
  }

  void
  AttrIdxContext::push(const Attribute *attr)
  {
    attrs[attr_cnt++] = attr->getName();
    attrpath_computed = False;
  }

  void
  AttrIdxContext::push(const char *attrname)
  {
    attrs[attr_cnt++] = attrname;
    attrpath_computed = False;
  }

  void
  AttrIdxContext::pop()
  {
    --attr_cnt;
    /*
      if (!attr_cnt)
      class_owner = 0;
    */
    attrpath_computed = False;
  }

  void
  AttrIdxContext::set(const Class *_class_owner)
  {
    class_owner = (char *)_class_owner->getName();
    attrpath_computed = False;
  }

  void
  AttrIdxContext::garbage(Bool all)
  {
    if (all) {
      assert(!idx_ops_cnt);
      free(idx_ops);
    }

    if (toFree) {
      free(class_owner);
    }
  }

  //
  // AttributeComponentSet related method
  //

  Status
  Attribute::createComponentSet(Database *db)
  {
    /*
      printf("creating component set %p %s::%s\n", this,
      (class_owner ? class_owner->getName() : "<unknown>"), name);
    */
  
    if (class_owner &&
	!strcmp(class_owner->getName(), "attribute_component_set"))
      return Success;

    if (attr_comp_set_oid.isValid())
      return Success;

    Status s;
    assert(dyn_class_owner);
    assert(class_owner);

    if (!dyn_class_owner->compare(class_owner)) {
      Attribute *attr = (Attribute *)class_owner->getAttribute(name);
      assert(attr != this);
      if (!attr->attr_comp_set_oid.isValid()) {
	s = attr->createComponentSet(db);
	if (s) return s;
      }

      attr_comp_set_oid = attr->attr_comp_set_oid;
      assert(attr_comp_set_oid.isValid());
      s = loadComponentSet(db, False);
      if (s) return s;
      assert(attr_comp_set);
      Class *xclass_owner = const_cast<Class *>(dyn_class_owner);
      /*
	printf("create #1 %s cls=%p\n", attr_comp_set_oid.toString(), xclass_owner);
      */
      assert(db->getSchema()->checkClass(xclass_owner));
      xclass_owner->touch();
      return xclass_owner->store();
    }

    attr_comp_set = new AttributeComponentSet(db);
    attr_comp_set->keep();

    Class *xclass_owner = const_cast<Class *>(class_owner);
    attr_comp_set->setAttrname((std::string(xclass_owner->getName()) + "." + name).c_str());
    attr_comp_set->setClassOwner(xclass_owner);
  
    s = attr_comp_set->store();
    if (s) return s;
    attr_comp_set_oid = attr_comp_set->getOid();
    //printf("create #2 %s cls=%p\n", attr_comp_set_oid.toString(), xclass_owner);
    assert(db->getSchema()->checkClass(xclass_owner));
  
    xclass_owner->touch();
    return xclass_owner->store();
  }

  Status
  Attribute::hasIndex(Database *db, bool &has_index, std::string &idx_str) const
  {
    Status s = loadComponentSet(db, False);
    if (s)
      return s;

    if (attr_comp_set)
      return attr_comp_set->hasIndex(has_index, idx_str);

    has_index = false;
    return Success;
  }

  Status
  Attribute::loadComponentSet(Database *db, Bool create) const
  {
    /*
      printf("loadComponentSet: %p %s %p %s\n", this, name, attr_comp_set,
      attr_comp_set_oid.getString());
    */
    if (attr_comp_set) {
      if (attr_comp_set->isRemoved()) {
	printf("REMOVED loadComponentSet: %p %s %p %s\n", this, name, attr_comp_set,
	       attr_comp_set_oid.getString());
      }
      return Success;
    }

#ifdef ATTRNAT_TRACE
    if (isNative()) {
      printf("native loading component set %p %s::%s -> %s\n", this,
	     (dyn_class_owner ? dyn_class_owner->getName() : "<unknown>"), name,
	     attr_comp_set_oid.getString());
    }
    else
      printf("loading component set %p %s::%s -> %s\n", this,
	     (class_owner ? class_owner->getName() : "<unknown>"), name,
	     attr_comp_set_oid.getString());
#endif

    if (attr_comp_set_oid.isValid()) {
      /*
	printf("loading component set %p %s::%s -> %s\n", this,
	(class_owner ? class_owner->getName() : "<unknown>"), name,
	attr_comp_set_oid.getString());
      */
      Status s = db->loadObject(attr_comp_set_oid,
				(Object *&)attr_comp_set);
      if (s) return s;
      attr_comp_set->keep();
      if (attr_comp_set->isRemoved()) {
	printf("REMOVED2 loadComponentSet: %p %s %p %s\n", this, name, attr_comp_set,
	       attr_comp_set_oid.getString());
      }
      return Success;
    }

    if (create)
      return const_cast<Attribute *>(this)->createComponentSet(db);

    return Success;
  }

  Status
  Attribute::addComponent(Database *db, AttributeComponent *comp) const
  {
    Status s;
    if (!attr_comp_set) {
      s = loadComponentSet(db, True);
      if (s) return s;
    }

    /*printf("%s::%s: adding component %s %s\n", dyn_class_owner->getName(),
      name, comp->getName(), comp->getOid().toString());
    */
    s = attr_comp_set->addToCompsColl(comp);
    if (s) return s;
    attr_comp_set->invalidateCache();
    Class *clsown = attr_comp_set->getClassOwner();
    if (clsown) clsown->unmakeAttrCompList();
    return attr_comp_set->store();
  }

  Status
  Attribute::rmvComponent(Database *db, AttributeComponent *comp) const
  {
    Status s;
    if (!attr_comp_set) {
      s = loadComponentSet(db, False);
      if (s) return s;
    }

    /*
      printf("%s::%s: removing component %s %s\n", dyn_class_owner->getName(),
      name, comp->getName(), comp->getOid().toString());
    */

    if (!attr_comp_set)
      return Exception::make(IDB_INTERNAL_ERROR,
			     "no attribute component set tied to attribute "
			     "%s::%s", getClassOwner()->getName(), name);
    s = attr_comp_set->rmvFromCompsColl(comp);
    if (s) return s;
    attr_comp_set->invalidateCache();
    /*
      Class *clsown = attr_comp_set->getClassOwner();

      printf("rmvComponent: comp=%s clsown=%s class_owner=%s "
      "dyn_class_owner=%s\n",
      comp->getOid().toString(),
      (clsown ? clsown->getName() : "nil"),
      class_owner->getName(),
      dyn_class_owner->getName());
    */

    //if (clsown) clsown->unmakeAttrCompList();
    const_cast<Class *>(dyn_class_owner)->unmakeAttrCompList();

    return attr_comp_set->store();
  }

  //
  // AttributeComponentSet methods
  //

  void
  AttributeComponentSet::userInitialize()
  {
    index_cache = 0;
    unique_cache = 0;
    notnull_cache = 0;
    card_cache = 0;
    collimpl_cache = 0;
  }

  void
  AttributeComponentSet::userCopy(const Object &)
  {
    index_cache = 0;
    unique_cache = 0;
    notnull_cache = 0;
    card_cache = 0;
    collimpl_cache = 0;
  }

  void
  AttributeComponentSet::userGarbage()
  {
    delete index_cache;
    delete unique_cache;
    delete notnull_cache;
    delete card_cache;
    delete collimpl_cache;
  }

  AttributeComponentSet::Cache::Cache()
  {
    comp_count = 0;
    comp_alloc = 0;
    comps = 0;
  }

  AttributeComponentSet::Cache::Comp::Comp()
  {
  }

  AttributeComponentSet::Cache::Comp::~Comp()
  {
    abort();
    if (comp)
      comp->release();
    free(attrpath);
  }

  void
  AttributeComponentSet::Cache::add(AttributeComponent *comp)
  {
    if (comp_count >= comp_alloc) {
      comp_alloc += 4;
      comps = (Comp *)realloc(comps, comp_alloc * sizeof(Comp));
    }

    comps[comp_count].attrpath = strdup(comp->getAttrpath().c_str());
    comps[comp_count].comp = comp;
    comp_count++;
  }

  AttributeComponentSet::Cache::~Cache()
  {
    for (int n = 0; n < comp_count; n++) {
      free(comps[n].attrpath);
      if (comps[n].comp)
	comps[n].comp->release();
    }

    free(comps);
  }

  void
  AttributeComponentSet::Cache::getComponents(const char *prefix,
					      int len, LinkedList &list)
  {
    for (int i = 0; i < comp_count; i++) {
      if (!strncmp(comps[i].comp->getAttrpath().c_str(), prefix, len)) {
	if (list.getPos(comps[i].comp) < 0)
	  list.insertObject(comps[i].comp);
      }
    }
  }

  AttributeComponent *
  AttributeComponentSet::Cache::find(const char *attrpath)
  {
    for (int i = 0; i < comp_count; i++)
      if (!strcmp(attrpath, comps[i].attrpath))
	return comps[i].comp;

    return 0;
  }

  void
  AttributeComponentSet::invalidateCache()
  {
    delete index_cache;
    delete unique_cache;
    delete notnull_cache;
    delete collimpl_cache;
    delete card_cache;

    index_cache = 0;
    unique_cache = 0;
    notnull_cache = 0;
    collimpl_cache = 0;
    card_cache = 0;
  }

  Status
  AttributeComponentSet::makeCache()
  {
    invalidateCache();

    index_cache = new Cache();
    unique_cache = new Cache();
    notnull_cache = new Cache();
    collimpl_cache = new Cache();
    card_cache = new Cache();

    Status rs = 0;
    Collection *comps = getCompsColl(0, &rs);
    if (rs)
      return rs;
    Iterator iter(comps);
    ObjectArray obj_arr;
    Status s = iter.scan(obj_arr);
    if (s) return s;
    int cnt = obj_arr.getCount();

    for (int n = 0; n < cnt; n++) {
      AttributeComponent *comp = (AttributeComponent *)obj_arr[n];
      comp->keep();
      const char *clsname = comp->getClass()->getName();

      if (!strcmp(clsname, "notnull_constraint"))
	notnull_cache->add(comp);
      else if (!strcmp(clsname, "unique_constraint"))
	unique_cache->add(comp);
      else if (!strcmp(clsname, "cardinality_constraint_test"))
	card_cache->add(comp);
      else if (!strcmp(clsname, "collection_attribute_implementation"))
	collimpl_cache->add(comp);
      else if (!strcmp(clsname, "hashindex") || !strcmp(clsname, "btreeindex"))
	index_cache->add(comp);
      else
	assert(0);
    }

    return Success;
  }

  Status
  AttributeComponentSet::getAttrComponents(const Class *xcls,
					   LinkedList &list)
  {
    /*
      static const char skipReentrant[] = "eyedb:get_attr_comp_set";

      if (xcls->getUserData(skipReentrant)) return Success;
      const_cast<Class *>(xcls)->setUserData(skipReentrant, AnyUserData);
    */

    if (!index_cache) {
      Status s = makeCache();
      if (s) {
	//const_cast<Class *>(xcls)->setUserData(skipReentrant, 0);
	return s;
      }
    }

    std::string prefix = std::string(xcls->getName()) + ".";
    int len = strlen(prefix.c_str());
    index_cache->getComponents(prefix.c_str(), len, list);
    unique_cache->getComponents(prefix.c_str(), len, list);
    notnull_cache->getComponents(prefix.c_str(), len, list);
    card_cache->getComponents(prefix.c_str(), len, list);
    collimpl_cache->getComponents(prefix.c_str(), len, list);

    //const_cast<Class *>(xcls)->setUserData(skipReentrant, 0);
    return Success;
  }

  Status
  AttributeComponentSet::find(const char *attrpath, Index *&index_comp)
  {
    Status s;
    if (!index_cache) {
      s = makeCache();
      if (s) return s;
    }

    index_comp = (Index *)index_cache->find(attrpath);
    return Success;
  }

  Status
  AttributeComponentSet::hasIndex(bool &has_index, std::string &idx_str)
  {
    Status s;
    if (!index_cache) {
      s = makeCache();
      if (s)
	return s;
    }

    if (index_cache) {
      has_index = index_cache->comp_count > 0;
      for (int n = 0; n < index_cache->comp_count; n++) {
	if (n > 0)
	  idx_str += ", ";
	idx_str += index_cache->comps[n].attrpath;
      }
    }
    else
      has_index = false;
    return Success;
  }


  Status
  AttributeComponentSet::find(const char *attrpath, NotNullConstraint *&notnull_comp)
  {
    Status s;
    if (!notnull_cache) {
      s = makeCache();
      if (s) return s;
    }

    notnull_comp = (NotNullConstraint *)notnull_cache->find(attrpath);
    return Success;
  }

  Status
  AttributeComponentSet::find(const char *attrpath, UniqueConstraint *&unique_comp)
  {
    Status s;
    if (!unique_cache) {
      s = makeCache();
      if (s) return s;
    }

    unique_comp = (UniqueConstraint *)unique_cache->find(attrpath);
    return Success;
  }

  Status
  AttributeComponentSet::find(const char *attrpath, CardinalityConstraint_Test *&card_comp)
  {
    Status s;
    if (!card_cache) {
      s = makeCache();
      if (s) return s;
    }

    card_comp = (CardinalityConstraint_Test *)card_cache->find(attrpath);
    return Success;
  }

  Status
  AttributeComponentSet::find(const char *attrpath,
			      CollAttrImpl *&collimpl_comp)
  {
    Status s;
    if (!collimpl_cache) {
      s = makeCache();
      if (s) return s;
    }

    collimpl_comp = (CollAttrImpl *)collimpl_cache->find(attrpath);
    return Success;
  }

  Status
  Attribute::destroyIndex(Database *db, Index *idx) const
  {
    IDB_LOG(IDB_LOG_IDX_REMOVE,
	    ("Remove Index (%s::%s, "
	     "index=%s)\n",
	     (class_owner ? class_owner->getName() : "<unknown>"),
	     name, idx->getAttrpath().c_str()));
	  
    /*
      printf("destroying index idx=%p %s\n", idx->idx,
      idx->getIdxOid().toString());
    */

    if (!idx->idx && idx->getIdxOid().isValid()) {
      Status s = openMultiIndexRealize(db, idx);
      if (s) return s;
    }

    assert(idx->idx || !idx->getIdxOid().isValid());

    if (!idx->idx)
      return Success;

    //printf("destroying index oid=%s\n", Oid(idx->idx->oid()).toString());
    eyedbsm::Status s = idx->idx->destroy();
    if (s)
      return Exception::make(IDB_INDEX_ERROR, eyedbsm::statusGet(s));

    return Success;
  }

  Status
  Attribute::getAttrComp(Database *db, const char *clsname,
			 const char *attrpath, Object *&o)
  {
    OQL q(db, "select %s.attrpath = \"%s\"", clsname, attrpath);
    ObjectArray obj_arr;
    Status s = q.execute(obj_arr);
    if (s) return s;
    if (!obj_arr.getCount()) {
      o = 0;
      return Success;
    }

    if (obj_arr.getCount() > 1)
      return Exception::make(IDB_ATTRIBUTE_ERROR, "multiple index with attrpath '%s'", attrpath);

    o = const_cast<Object *>(obj_arr[0]);
    return Success;
  }

  Status
  Attribute::getIndex(Database *db, const char *attrpath,
		      Index *&idx)
  {
    return getAttrComp(db, "index", attrpath, (Object *&)idx);
  }
  
  Status
  Attribute::getUniqueConstraint(Database *db, const char *attrpath,
				 UniqueConstraint *&unique)
  {
    return getAttrComp(db, "unique_constraint", attrpath, (Object *&)unique);
  }
  
  Status
  Attribute::getNotNullConstraint(Database *db, const char *attrpath,
				  NotNullConstraint *&notnull)
  {
    return getAttrComp(db, "notnull_constraint", attrpath, (Object *&)notnull);
  }
  
  Status
  Attribute::getCollAttrImpl(Database *db, const char *attrpath,
			     CollAttrImpl *&collimpl)
  {
    return getAttrComp(db, "collection_attribute_implementation", attrpath, (Object *&)collimpl);
  }
}
