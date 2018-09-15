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
#include "odl.h"
#include "CollectionBE.h"
#include <eyedblib/butils.h>

namespace eyedb {

  const int IndexImpl::UNKNOWN_IMPL_TYPE = 0; // CollAttrImpl::Unknown
  const int IndexImpl::NOINDEX_IMPL_TYPE = 3; // CollAttrImpl::NoIndex

  struct KeyValue {
    const char *key;
    const char *value;
    void set(const char *_key, const char *_value) {
      key = _key;
      value = _value;
    }
  };

  struct KeyValueArray {
    int kalloc;
    int cnt;
    KeyValue *kvalues;
    KeyValueArray() {
      kalloc = cnt = 0;
      kvalues = 0;
    }
    void add(const char *k, const char *v) {
      if (cnt >= kalloc) {
	kalloc += 8;
	kvalues = (KeyValue *)realloc(kvalues, kalloc * sizeof(KeyValue));
      }
      kvalues[cnt++].set(k, v);
    }
    void trace() {
      for (int i = 0; i < cnt; i++)
	printf("'%s' <-> '%s'\n", kvalues[i].key, (kvalues[i].value ?
						   kvalues[i].value : "NULL"));
    }
    ~KeyValueArray() {
      free(kvalues);
    }
  };

  static char *
  trim(char *p)
  {
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    int len = strlen(p);
    char *q = p + strlen(p) - 1;
    while (*q == ' ' || *q == '\t' || *q == '\n')
      *q-- = 0;
    return p;
  }

  static char *
  make(const char *xhints, KeyValueArray &kvarr)
  {
    char *hints = strdup(xhints);
    char *p = hints;
    char *q = hints;
    char *key = 0, *value = 0, *last = hints;
    for (;;) {
      char c = *p;
      if (c == '=') {
	*p = 0;
	key = trim(last);
	last = p+1;
      }
      else if (c == ';' || c == ',' || !c) {
	*p = 0;
	value = trim(last);
	last = p+1;
	if (!key)
	  kvarr.add(value, 0);
	else
	  kvarr.add(key, value);
	if (!c) break;
	key = 0;
      }
      p++;
    }

    //kvarr.trace();
    return hints;
  }

  const char *IndexImpl::hashHintToStr(unsigned int hints, Bool cap)
  {
    if (hints == eyedbsm::HIdx::IniSize_Hints)
      return cap ? "Initial Size" : "initial_size";

    if (hints == eyedbsm::HIdx::IniObjCnt_Hints)
      return cap ? "Initial Object Count" : "initial_object_count";

    if (hints == eyedbsm::HIdx::XCoef_Hints)
      return cap ? "Extend Coeficient" : "extend_coef";

    if (hints == eyedbsm::HIdx::SzMax_Hints)
      return cap ? "Maximal Hash Object Size" : "size_max";

    if (hints == eyedbsm::HIdx::DataGroupedByKey_Hints)
      return cap ? "Data Grouped by Key" : "data_grouped_by_key";

    return "<unimplemented>";
  }

  bool IndexImpl::isHashHintImplemented(unsigned int hints)
  {
    return hints <= eyedbsm::HIdx::DataGroupedByKey_Hints;
  }

#define MAKE(H) \
 if (!strcasecmp(k, hashHintToStr(eyedbsm::HIdx::H))) { \
   if (!v || !eyedblib::is_number(v)) { \
     if (errmsg.size()) errmsg += "\n"; \
     errmsg += std::string(hashHintToStr(eyedbsm::HIdx::H)) + ": expected a number"; \
   } \
   else \
     impl_hints[eyedbsm::HIdx::H] = atoi(v); \
 }

  static Signature *
  makeSign(Schema *m)
  {
    Signature *sign = new Signature();
    ArgType *type;

    type = sign->getRettype();
    *type = *ArgType::make(m, int32_class_name);

    type->setType((ArgType_Type)(type->getType() | OUT_ARG_TYPE), False);

    sign->setNargs(2);

    sign->setTypesCount(2);
    type = sign->getTypes(0);
    *type = *ArgType::make(m, "rawdata");

    type->setType((ArgType_Type)(type->getType() | IN_ARG_TYPE), False);

    sign->setTypesCount(2);
    type = sign->getTypes(1);
    *type = *ArgType::make(m, int32_class_name);
    type->setType((ArgType_Type)(type->getType() | IN_ARG_TYPE), False);

    return sign;
  }
  static Signature *
  getHashSignature(Schema *m)
  {
    static Signature *sign;

    if (!sign)
      sign = makeSign(m);
 
    return sign;
  }

  static Status
  findMethod(Database *db, const char *key_function, const char *mthname,
	     const Class *xcls, Method *&mth)
  {
    Signature *sign = getHashSignature(db->getSchema());
    Status s;

    LinkedList *mthlist = (LinkedList *)xcls->getUserData(odlMTHLIST);
    if (mthlist) {
      LinkedListCursor c(mthlist);
      while (c.getNext((void *&)mth)) {
	if (!strcmp(mth->getEx()->getExname().c_str(), mthname) &&
	    *sign == *mth->getEx()->getSign())
	  return Success;
      }
    }
	   
    s = const_cast<Class *>(xcls)->getMethod(mthname, mth, sign);
    if (s) return s;

    if (!mth) {
      s = const_cast<Class *>(xcls)->getMethod(mthname, mth);
      if (s) return s;
      if (!mth)
	return Exception::make(IDB_ERROR, "no method '%s' in class '%s'",
			       mthname, xcls->getName());
      return Exception::make(IDB_ERROR, "invalid hash method signature: "
			     "must be classmethod int %s(in rawdata, in int)",
			     key_function);
    }

    return Success;
  }

  Status
  get_key_function(Database *db, const char *key_function,
		   BEMethod_C *&be_mth)
  {
    be_mth = 0;
    if (!key_function || !*key_function)
      return Success;

    if (!db)
      return Exception::make(IDB_ERROR, "database should be set when a "
			     "hash method is specified");

    const Class *xcls = 0;
    Status s;
    const char *q = strchr(key_function, ':');
    const char *mthname;
    if (q) {
      int len = q-key_function;
      char *clsname = new char[len+1];
      strncpy(clsname, key_function, len);
      clsname[len] = 0;
      xcls = db->getSchema()->getClass(clsname);
      if (!xcls) {
	s = Exception::make(IDB_ERROR, "invalid key function '%s': "
			    "cannot find class '%s'", key_function, clsname);
	delete [] clsname;
	return s;
      }
      delete [] clsname;
      mthname = q+2;
    }
    else
      return Exception::make(IDB_ERROR, "key function must be under the form "
			     "'classname::methodname'");

    Method *mth;
    s = findMethod(db, key_function, mthname, xcls, mth);
    if (s) return s;

    be_mth = mth->asBEMethod_C();

    if (!be_mth)
      return Exception::make(IDB_ERROR, "method '%s' in class '%s' is not "
			     "a server method",
			     mthname, xcls->getName());
    return Success;
  }

  Status
  IndexImpl::makeHash(Database *db, const char *hints,
		      IndexImpl *&idximpl, Bool is_string)
  {
    idximpl = 0;
    int key_count = 0;
    std::string dspname;
    int impl_hints_cnt = eyedbsm::HIdxImplHintsCount;
    int impl_hints[eyedbsm::HIdxImplHintsCount];
    memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
    std::string key_function;
    char *x = 0;

    if (hints) {
      std::string errmsg;
      KeyValueArray kvarr;
      x = eyedb::make(hints, kvarr);
      for (int n = 0; n < kvarr.cnt; n++) {
	KeyValue *kv = &kvarr.kvalues[n];
	const char *k = kv->key;
	const char *v = kv->value;
	if (!strcasecmp(k, "key_count")) {
	  if (!v || !eyedblib::is_number(v))
	    errmsg += std::string("key_count expected a number\n");
	  else
	    key_count = atoi(v);
	}
	else if (!strcasecmp(k, "dataspace")) {
	  if (!v)
	    errmsg += std::string("dataspace expected a value\n");
	  dspname = v;
	}
	else MAKE(IniSize_Hints)
	       else MAKE(IniObjCnt_Hints)
		      else MAKE(XCoef_Hints)
			     else MAKE(SzMax_Hints)
				    else MAKE(DataGroupedByKey_Hints)
					   else if (!strcasecmp(k, "key_function"))
					     key_function = v;
	else if (*k || (v && *v)) {
	  if (errmsg.size()) errmsg += "\n";
	  errmsg += std::string("unknown hint: ") + k;
	}
      }

      if (errmsg.size()) {
	errmsg += "\nhash index hints grammar: 'key_count = <intval>; "
	  "initial_size = <intval>; "
	  "initial_object_count = <intval>; "
	  "extend_coef = <intval>; "
	  "size_max = <intval>; "
	  "data_grouped_by_key = <intval>; "
	  "key_function = <class>::<method>; "
	  "dataspace = <name>';";
	return Exception::make(IDB_ERROR, errmsg.c_str());
      }
    }

    free(x);
    Status s;
    const Dataspace *dataspace = 0;
    if (dspname.size()) {
      if (db->isOpened()) {
	s = db->getDataspace(dspname.c_str(), dataspace);
	if (s) return s;
      }
    }

    BEMethod_C *mth;
    s = get_key_function(db, key_function.c_str(), mth);
    if (s) return s;

    if (is_string && impl_hints[eyedbsm::HIdx::IniObjCnt_Hints]) {
      return Exception::make(IDB_ERROR,
			     "one cannot define "
			     "the initial_object_count hint "
			     "on a string index");
    }

    if (impl_hints[eyedbsm::HIdx::IniObjCnt_Hints] &&
	impl_hints[eyedbsm::HIdx::IniSize_Hints]) {
      return Exception::make(IDB_ERROR,
			     "one cannot define "
			     "both the initial_object_count "
			     "and the initial_size hints "
			     "on an index");
    }


    idximpl =
      new IndexImpl(Hash, dataspace, key_count, mth, impl_hints,
		    impl_hints_cnt);
    return Success;
  }

  Status
  IndexImpl::makeBTree(Database *db, const char *hints,
		       IndexImpl *&idximpl, Bool)
  {
    int degree = 0;
    char *x = 0;
    std::string dspname;
    int impl_hints_cnt = eyedbsm::HIdxImplHintsCount;
    int impl_hints[eyedbsm::HIdxImplHintsCount];
    memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);

    if (hints) {
      std::string errmsg;
      KeyValueArray kvarr;
      x = eyedb::make(hints, kvarr);
      for (int n = 0; n < kvarr.cnt; n++) {
	KeyValue *kv = &kvarr.kvalues[n];
	const char *k = kv->key;
	const char *v = kv->value;
	if (!strcasecmp(k, "degree")) {
	  if (!v || !eyedblib::is_number(v))
	    errmsg += std::string("defree expected a number\n");
	  else
	    degree = atoi(v);
	}
	else if (!strcasecmp(k, "dataspace")) {
	  if (!v)
	    errmsg += std::string("dataspace expected a value\n");
	  dspname = v;
	}
	else if (*k || (v && *v)) {
	  if (errmsg.size()) errmsg += "\n";
	  errmsg += std::string("unknown hint: ") + k;
	}
      }

      if (errmsg.size()) {
	errmsg += "\nbtree index hints grammar: 'degree = <intval>; "
	  "dataspace = <name>;'";
	return Exception::make(IDB_ERROR, errmsg.c_str());
      }
    }

    free(x);
    Status s;
    const Dataspace *dataspace = 0;
    if (dspname.size()) {
      if (db->isOpened()) {
	s = db->getDataspace(dspname.c_str(), dataspace);
	if (s) return s;
      }
    }

    idximpl = new IndexImpl(BTree, dataspace, degree, 0,
			    impl_hints, impl_hints_cnt);

    return Success;
  }

  IndexImpl::IndexImpl(Type _type,
		       const Dataspace *_dataspace,
		       unsigned int keycount_or_degree,
		       BEMethod_C *mth,
		       const int _impl_hints[],
		       unsigned int _impl_hints_cnt)
  {
    //printf("indeximpl:: construct %p\n", this);
    int impl_hints_def[eyedbsm::HIdxImplHintsCount];
    type = _type;
    dataspace = _dataspace;

    u.hash.mth = 0;
    u.hash.keycount = 0;
    u.degree = 0;

    if (type == Hash) {
      u.hash.keycount = keycount_or_degree;
      u.hash.mth = mth;
    }
    else {
      u.degree = keycount_or_degree;
    }

    impl_hints_cnt = _impl_hints_cnt;

    if (impl_hints_cnt) {
      impl_hints = new int[impl_hints_cnt];
      memcpy(impl_hints, _impl_hints, impl_hints_cnt * sizeof(impl_hints[0]));
    }
    else
      impl_hints = 0;

    setTag("eyedb::IndexImpl");
  }

  IndexImpl::IndexImpl(const IndexImpl &)
  {
    abort();
  }

  IndexImpl& IndexImpl::operator=(const IndexImpl &)
  {
    abort();
    return *this;
  }

  Status
  IndexImpl::make(Database *db, Type type, const char *hints,
		  IndexImpl *&idximpl, Bool is_string)
  {
    if (type == Hash)
      return makeHash(db, hints, idximpl, is_string);
    if (type == BTree)
      return makeBTree(db, hints, idximpl, is_string);

    return Exception::make(IDB_ERROR,
			   "index implementation type is not valid");
  }

#define CHECK_X(I) \
  if ((I) == eyedbsm::HIdx::IniSize_Hints && impl_hints[eyedbsm::HIdx::IniObjCnt_Hints]) continue;

  std::string
  IndexImpl::getHintsString() const
  {
    std::string hints;
    if (dataspace) {
      const char *dspname = dataspace->getName();
      hints += std::string("dataspace = ") +
	(*dspname ? std::string(dspname) :
	 str_convert((long)dataspace->getId())) + ";";
    }

    if (type == BTree) {
      if (u.degree) {
	if (hints.size()) hints += " ";
	hints += std::string("degree = ") + str_convert((long)u.degree) + ";";
      }
      return hints;
    }

    if (u.hash.keycount) {
      if (hints.size()) hints += " ";
      hints += std::string("key_count = ") + str_convert((long)u.hash.keycount) + ";";
    }

    if (u.hash.mth) {
      if (hints.size()) hints += " ";
      hints += std::string("key_function = ") +
	u.hash.mth->getClassOwner()->getName() + "::" +
	u.hash.mth->getEx()->getExname() + ";";
    }

    int cnt = impl_hints_cnt;
#ifdef IDB_SKIP_HASH_XCOEF
    if (cnt > eyedbsm::HIdx::XCoef_Hints) cnt = eyedbsm::HIdx::XCoef_Hints;
#endif
    for (int i = 0; i < cnt ; i++) {
      CHECK_X(i);
      int val = impl_hints[i];
      if (val) {
	if (hints.size()) hints += " ";
	hints += std::string(hashHintToStr(i)) + " = " + str_convert(val) + ";";
      }
    }

    return hints;
  }

  std::string
  IndexImpl::toString(const char *xindent) const
  {
    std::string indent = xindent;
    std::string hints = indent + "Type: ";
    if (type == BTree) {
      hints += "BTree\n";
      if (dataspace) {
	const char *dspname = dataspace->getName();
	hints += indent + "Dataspace: "  +
	  (*dspname ? std::string(dspname) :
	   str_convert((long)dataspace->getId())) + "\n";
      }
      if (u.degree)
	hints += indent + "Degree: " + str_convert((long)u.degree) + "\n";
      return hints;
    }

    hints += "Hash\n";
    if (dataspace) {
      const char *dspname = dataspace->getName();
      hints += indent + "Dataspace: "  +
	(*dspname ? std::string(dspname) :
	 str_convert((long)dataspace->getId())) + "\n";
    }

    if (u.hash.keycount)
      hints += indent + "Key Count: " + str_convert((long)u.hash.keycount) + "\n";


    if (u.hash.mth) {
      hints += indent + "Key Function: " +
	u.hash.mth->getClassOwner()->getName() + "::" +
	u.hash.mth->getEx()->getExname() + "\n";
    }

    int cnt = impl_hints_cnt;
#ifdef IDB_SKIP_HASH_XCOEF
    if (cnt > eyedbsm::HIdx::XCoef_Hints) cnt = eyedbsm::HIdx::XCoef_Hints;
#endif
    for (int i = 0; i < cnt; i++) {
      CHECK_X(i);
      int val = impl_hints[i];
      if (val)
	hints += indent + hashHintToStr(i, True) + ": " + str_convert(val) + "\n";
    }

    return hints;
  }

  IndexImpl *
  IndexImpl::clone() const
  {
    assert(getRefCount() > 0);
    if (type == Hash)
      return new IndexImpl(type, dataspace, u.hash.keycount, u.hash.mth,
			   impl_hints, impl_hints_cnt);

    return new IndexImpl(type, dataspace, u.degree, 0,
			 impl_hints, impl_hints_cnt);
  }

  Bool
  IndexImpl::compare(const IndexImpl *idximpl) const
  {
    if (idximpl->type != type)
      return False;

    if (type == Hash)
      return IDBBOOL
	(!(idximpl->u.hash.keycount != u.hash.keycount ||
	   (idximpl->u.hash.mth && !u.hash.mth) ||
	   (!idximpl->u.hash.mth && u.hash.mth) ||
	   (u.hash.mth && (idximpl->u.hash.mth->getOid() != u.hash.mth->getOid() ||
			 idximpl->u.hash.mth != u.hash.mth)) ||
	   idximpl->impl_hints_cnt != impl_hints_cnt ||
	   memcmp(idximpl->impl_hints, impl_hints,
		  impl_hints_cnt * sizeof(impl_hints[0]))));

    return IDBBOOL
      (!(idximpl->u.degree != u.degree ||
	 idximpl->impl_hints_cnt != impl_hints_cnt ||
	 memcmp(idximpl->impl_hints, impl_hints,
		impl_hints_cnt * sizeof(impl_hints[0]))));
  }

  unsigned int
  IndexImpl::getMagorder(unsigned int def_magorder) const
  {
    if (type == Hash) {
      if (!u.hash.keycount)
	return def_magorder;
      return eyedbsm::HIdx::getMagOrder(u.hash.keycount);
    }

    if (!u.degree)
      return def_magorder;

    return eyedbsm::BIdx::getMagOrder(u.degree);
  }

  unsigned int
  IndexImpl::estimateHashKeycount(unsigned int magorder)
  {
    return eyedbsm::HIdx::getKeyCount(magorder);
  }

  unsigned int
  IndexImpl::estimateBTreeDegree(unsigned int magorder)
  {
    return eyedbsm::BIdx::getDegree(magorder);
  }

  IndexImpl::~IndexImpl()
  {
    garbageRealize();
  }

  void IndexImpl::garbage()
  {
    //printf("indeximpl:: garbage %p\n", this);
    delete [] impl_hints;
  }


  Status
  IndexImpl::code(Data &data, Offset &offset, Size &alloc_size,
		  const IndexImpl *idximpl, int impl_type)
  {
    if (!idximpl && impl_type != NOINDEX_IMPL_TYPE) {
      return Exception::make(IDB_ERROR, "invalid null index implementation");
    }

    static eyedblib::int32 zero = 0;
    char idxtype = (idximpl ? idximpl->getType() : NOINDEX_IMPL_TYPE);
    char_code (&data, &offset, &alloc_size, &idxtype);
    short dspid = (idximpl && idximpl->dataspace ? idximpl->dataspace->getId() :
		   Dataspace::DefaultDspid);
    int16_code (&data, &offset, &alloc_size, &dspid);

    if (idxtype == IndexImpl::Hash) {
      eyedblib::int32 x = idximpl->getKeycount();
      int32_code (&data, &offset, &alloc_size, &x);
      if (idximpl->getHashMethod()) {
	const Oid &xoid = idximpl->getHashMethod()->getOid();
	if (!xoid.isValid())
	  return Exception::make(IDB_ERROR, "while coding collection, "
				 "non persistent hash method found");
      
	oid_code (&data, &offset, &alloc_size, xoid.getOid());
      }
      else
	oid_code (&data, &offset, &alloc_size, Oid::nullOid.getOid());
    }
    else if (idxtype == IndexImpl::BTree) {
      eyedblib::int32 x = idximpl->getDegree();
      int32_code (&data, &offset, &alloc_size, &x);
      oid_code (&data, &offset, &alloc_size, Oid::nullOid.getOid());
    }
    else if (idxtype == 0) {
      eyedblib::int32 x = 0;
      int32_code (&data, &offset, &alloc_size, &x);
      oid_code (&data, &offset, &alloc_size, Oid::nullOid.getOid());
    }

    unsigned int impl_hints_cnt = 0;
    const int *impl_hints = (idximpl ? idximpl->getImplHints(impl_hints_cnt) : 0);
    assert(impl_hints_cnt <= IDB_MAX_HINTS_CNT);
    for (int i = 0; i < impl_hints_cnt; i++)
      int32_code (&data, &offset, &alloc_size, &impl_hints[i]);
    for (int i = impl_hints_cnt; i < IDB_MAX_HINTS_CNT; i++)
      int32_code (&data, &offset, &alloc_size, &zero);
  
    return Success;
  }

  void
  IndexImpl::setHashMethod(BEMethod_C *mth)
  {
    if (type == Hash)
      u.hash.mth = mth;
  }

  Status
  IndexImpl::decode(Database *db, Data data,
		    Offset &offset, IndexImpl *&idximpl, int *rimpl_type)
  {
    IndexImpl::Type impl_type;
    eyedblib::int32 implinfo;
    eyedblib::int32 impl_hints[IDB_MAX_HINTS_CNT];
    Oid mthoid;
    BEMethod_C *mth = 0;

    char c;
    char_decode (data, &offset, &c);
    impl_type = (IndexImpl::Type)c;
    short dspid;
    int16_decode (data, &offset, &dspid);
    int32_decode (data, &offset, &implinfo);
    oid_decode (data, &offset, mthoid.getOid());

    Status s;
    const Dataspace *dataspace = 0;
    if (dspid != Dataspace::DefaultDspid) {
      s = db->getDataspace(dspid, dataspace);
      if (s) return s;
    }

    if (mthoid.isValid()) {
      s = db->loadObject(mthoid, (Object *&)mth);
      if (s) return s;
    }

    for (int i = 0; i < IDB_MAX_HINTS_CNT; i++)
      int32_decode (data, &offset, &impl_hints[i]);

    if (impl_type == IndexImpl::Hash || impl_type == IndexImpl::BTree) {
      idximpl = new IndexImpl(impl_type, dataspace, implinfo, mth, impl_hints,
			      IDB_MAX_HINTS_CNT);
    }
    else if (impl_type != NOINDEX_IMPL_TYPE) {
      return Exception::make(IDB_ERROR, "unknown index type implementation %d", impl_type);
    }

    if (rimpl_type) {
      *rimpl_type = impl_type;
    }
    return Success;
  }

  IndexStats::IndexStats()
  {
    idximpl = 0;
  }

  IndexStats::~IndexStats()
  {
    if (idximpl)
      idximpl->release();
  }

  HashIndexStats::HashIndexStats()
  {
    //memset(this, 0, sizeof(*this));
    key_count = 0;
    entries = 0;
  }

  HashIndexStats::~HashIndexStats()
  {
    delete [] entries;
  }

#define ONE_K 1024
#define ONE_M (ONE_K*ONE_K)

  static std::string
  get_string_size(unsigned int _sz)
  {
    unsigned int sz1, sz2;
    unsigned int sz;

    sz = _sz;
    std::string s = str_convert((long)sz) + "B";
    sz = _sz / ONE_K;

    if (sz) {
      s += std::string(", ~") + str_convert((long)sz) + "KB";

      sz = _sz / ONE_M;
      if (sz) {
	sz1 = sz * ONE_M;
	sz2 = (sz+1) * ONE_M;
	if ((sz2 - _sz) < (_sz - sz1))
	  sz = sz+1;
	s += std::string(", ~") + str_convert((long)sz) + "MB";
      }
    }

    return s;
  }

  std::string
  HashIndexStats::toString(Bool dspImpl, Bool full, const char *xindent)
  {
    std::string indent = xindent;
    std::string s;
    if (dspImpl)
      s = idximpl ? idximpl->toString(xindent) : std::string("");
    Entry *entry = entries;

    if (dspImpl && !idximpl)
      s += indent + "Key count: " + str_convert((long)key_count) + "\n";

    if (full) {
      for (int i = 0; i < key_count; i++, entry++)
	if (entry->object_count || entry->hash_object_count) {
	  char buf[2048];
	  sprintf(buf,
		  "%sKey #%d {\n "
		  "\t%sObject count: %d\n"
		  "\t%sHash object count: %d\n"
		  "\t%sHash object size: %s\n"
		  "\t%sHash object busy size: %s\n"
		  "\t%sHash object free size: %s\n%s}\n",
		  indent.c_str(), i,
		  indent.c_str(), entry->object_count,

		  indent.c_str(), entry->hash_object_count,
		  indent.c_str(), get_string_size(entry->hash_object_size).c_str(),
		  indent.c_str(), get_string_size(entry->hash_object_busy_size).c_str(),
		  indent.c_str(), get_string_size(entry->hash_object_size - entry->hash_object_busy_size).c_str(),
		  indent.c_str());
	  s += buf;
	}
    }

    s += indent + std::string("Min objects per entry: ") +
      str_convert((long)min_objects_per_entry) + "\n";
    s += indent + std::string("Max objects per entry: ") +
      str_convert((long)max_objects_per_entry) + "\n";
    s += indent + std::string("Total object count: ") +
      str_convert((long)total_object_count) + "\n";
    s += indent + std::string("Total hash object count: ") + 
      str_convert((long)total_hash_object_count) + "\n";
    s += indent + std::string("Total hash object size: ") +
      get_string_size(total_hash_object_size) + "\n";
    s += indent + std::string("Total hash object busy size: ") +
      get_string_size(total_hash_object_busy_size) + "\n";
    s += indent + std::string("Total hash object free size: ") +
      get_string_size(total_hash_object_size - total_hash_object_busy_size) + "\n";
    s += indent + std::string("Busy entry count: ") +
      str_convert((long)busy_key_count) + "\n";
    s += indent + std::string("Free entry count: ") +
      str_convert((long)free_key_count) + "\n";
    return s;
  }

  std::string
  BTreeIndexStats::toString(Bool dspImpl, Bool full, const char *xindent)
  {
    std::string s;
    std::string indent = xindent;

    if (dspImpl) {
      s = indent + "Type: BTree\n";
      s += indent + "Degree: " + str_convert((long)degree) + "\n";
      s += indent + "Data size: " + str_convert((long)dataSize) + "\n";
      s += indent + "Key size: " + str_convert((long)keySize) + "\n";
      s += indent + "Key type: " + eyedbsm::Idx::typeString((eyedbsm::Idx::Type)keyType) + "\n";
      s += indent + "Key offset: " + str_convert((long)keyOffset) + "\n";
    }

    s += indent + "Total object count: " + str_convert((long)total_object_count) + "\n";
    s += indent + "Total btree object count: " + str_convert((long)total_btree_object_count) + "\n";
    s += indent + "Total btree node count: " + str_convert((long)total_btree_node_count) + "\n";
    s += indent + "Btree node size: " + str_convert((long)btree_node_size) + "\n";
    s += indent + "Btree key object size: " +
      get_string_size(btree_key_object_size) + "\n";
    s += indent + "Btree data object size: " +
      get_string_size(btree_data_object_size) + "\n";
    s += indent + "Total btree object size: " +
      get_string_size(total_btree_object_size) + "\n";
    return s;
  }

  BTreeIndexStats::BTreeIndexStats()
  {
  }

  BTreeIndexStats::~BTreeIndexStats()
  {
  }


  class HIdxStatsFormat {

  public:
    enum Type {
      Num,
      ObjCnt,
      HObjCnt,
      HSz,
      BSz,
      FSz,
      LAST
    };

    HIdxStatsFormat(const char *_fmt);

    bool hasFormat() const {
      return fmt != 0;
    }

    const char *getError() const {
      if (errmsg.size())
	return errmsg.c_str();
      return 0;
    }

    void print(int values[], FILE *fd = stdout);

    ~HIdxStatsFormat() {
      delete [] fmt;
    }

  private:
    std::string errmsg;
    static const int maxprms = 7;
    char *fmt;
    int pos[maxprms];
    unsigned int pos_cnt;
  };

  HIdxStatsFormat::HIdxStatsFormat(const char *_fmt)
  {
    fmt = (_fmt ? new char[strlen(_fmt)+1] : 0);
    if (!fmt) return;
    pos_cnt = 0;
    const char *s = _fmt;
    char *p = fmt;
    char c;
    while (c = *s) {
      if (c == '%') {
	*p++ = '%';
	c = *++s;
	if (c == 'n') {
	  pos[pos_cnt++] = Num;
	  *p++ = 'd';
	}
	else if (c == 'O') {
	  pos[pos_cnt++] = ObjCnt;
	  *p++ = 'd';
	}
	else if (c == 'o') {
	  pos[pos_cnt++] = HObjCnt;
	  *p++ = 'd';
	}
	else if (c == 's') {
	  pos[pos_cnt++] = HSz;
	  *p++ = 'd';
	}
	else if (c == 'b') {
	  pos[pos_cnt++] = BSz;
	  *p++ = 'd';
	}
	else if (c == 'f') {
	  pos[pos_cnt++] = FSz;
	  *p++ = 'd';
	}
	else if (c == '%')
	  *p++ = '%';
	else {
	  printf("ERROR !!!!\n");
	  errmsg = std::string("unknown format sequence: %%n, %%o, %%h or %%z expected\n");
	  delete [] fmt;
	  fmt = 0;
	  return;
	}
      }
      else if (c == '\\') {
	c = *++s;
#define _ESC_(X, Y) case X: *p++ = Y; break
	switch(c) {
	  _ESC_('a', '\a');
	  _ESC_('b', '\b');
	  _ESC_('f', '\f');
	  _ESC_('n', '\n');
	  _ESC_('r', '\r');
	  _ESC_('t', '\t');
	  _ESC_('v', '\v');
	  _ESC_('\'', '\'');
	  _ESC_('\"', '"');
	  _ESC_('\\', '\\');

	default:
	  *p++ = '\\';
	  *p++ = c;
	}
      }
      else
	*p++ = c;

      s++;
    }
    *p = 0;
  }

  void
  HIdxStatsFormat::print(int values[], FILE *fd)
  {
    int v[maxprms];
    for (int i = 0; i < pos_cnt; i++)
      v[i] = values[pos[i]];
    fprintf(fd, fmt, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
  }

  Status
  HashIndexStats::printEntries(const char *fmt, FILE *fd)
  {
    HIdxStatsFormat f(fmt);
    if (f.getError())
      return Exception::make(IDB_ERROR, f.getError());

    for (int i = 0; i < key_count; i++) {
      int values[HIdxStatsFormat::LAST];
      HashIndexStats::Entry *e = &entries[i];
      values[HIdxStatsFormat::Num] = i;
      values[HIdxStatsFormat::ObjCnt] = e->object_count;
      values[HIdxStatsFormat::HObjCnt] = e->hash_object_count;
      values[HIdxStatsFormat::HSz] = e->hash_object_size;
      values[HIdxStatsFormat::BSz] = e->hash_object_busy_size;
      values[HIdxStatsFormat::FSz] = e->hash_object_size - e->hash_object_busy_size;
      f.print(values, fd);
    }

    return Success;
  }

  const char HashIndexStats::fmt_help[] =
  "    <fmt> is a printf-like string where:\n"
  "      %%n denotes the number of keys,\n"
  "      %%O denotes the count of object entries for this key,\n"
  "      %%o denotes the count of hash objects for this key,\n"
  "      %%s denotes the size of hash objects for this key,\n"
  "      %%b denotes the busy size of hash objects for this key,\n"
  "      %%f denotes the free size of hash objects for this key.\n"
  "\n  For instance:\n"
  "      --fmt=\"%%n %%O\\n\"\n"
  "      --fmt=\"%%n -> %%O, %%o, %%s\\n\"\n";

}
