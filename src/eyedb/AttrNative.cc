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
#include "Attribute_p.h"

#define ADD_PROT

namespace eyedb {

  struct AttrNativeItems {
    Attribute **items;
    int items_cnt;
  };

  AttrNativeItems nat_items[idbITEMS_COUNT];

  static Oid
  get_class_oid(const Object *refo, const Class *cls)
  {
    if (cls->getOid().isValid())
      return cls->getOid();

    if (!refo->getDatabase())
      return Oid::nullOid;

    cls = const_cast<Schema *>(refo->getDatabase()->getSchema())->getClass(cls->getName());

    if (!cls)
      return Oid::nullOid;

    return cls->getOid();
  }

  static Status
  getv_class(const Object *o, Data *data, int nb, int from,
	     Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    const Class *cls = o->getClass();

    if (isGetTValue) {
      Oid xoid = get_class_oid(o, cls);
      memcpy(data, &xoid, sizeof(Oid));
      return Success;
    }

    memcpy(data, &cls, sizeof(Object *));
    return Success;
  }

  static Status
  geto_class(const Object *o, Oid *oid, int nb, int from)
  {
    Oid xoid = get_class_oid(o, o->getClass());
    memcpy(oid, &xoid, sizeof(Oid));
    return Success;
  }

  static Status
  getv_protection(const Object *o, Data *data, int nb, int from,
		  Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;
    Oid prot_oid;
    Status s = o->getProtection(prot_oid);
    if (s) return s;

    if (isGetTValue)
      {
	memcpy(data, &prot_oid, sizeof(Oid));
	return Success;
      }

    if (prot_oid.isValid())
      return ((Object *)o)->getDatabase()->loadObject(&prot_oid, (Object **)data);
    memset(data, 0, sizeof(Object *));
    return Success;
  }

  static Status
  geto_protection(const Object *o, Oid *oid, int nb, int from)
  {
    return o->getProtection(*oid);
  }

  static Status
  setv_protection(Object *o, Data data, int nb, int from)
  {
    const Class *cls = o->getClass();
    memcpy(data, &cls, sizeof(Object *));
    return Success;
  }

  static Status
  seto_protection(Object *o, const Oid *oid, int nb, int from, Bool)
  {
    return o->setProtection(*oid);
  }

  static Status
  getv_parent(const Object *o, Data *data, int nb, int from,
	      Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    Class *parent;
    Status s = const_cast<Class *>(o->asClass())->getParent(const_cast<Database *>(o->getDatabase()), parent);
    if (s) return s;

    if (isGetTValue)
      {
	if (parent) {
	  Oid xoid = get_class_oid(o, parent);
	  memcpy(data, &xoid, sizeof(Oid));
	}
	else
	  memcpy(data, &Oid::nullOid, sizeof(Oid));
	return Success;
      }

    if (parent)
      memcpy(data, &parent, sizeof(Object *));
    else
      memset(data, 0, sizeof(Object *));
    return Success;
  }

  static Status
  geto_parent(const Object *o, Oid *oid, int nb, int from)
  {
    Class *parent;
    Status s = const_cast<Class *>(o->asClass())->getParent(const_cast<Database *>(o->getDatabase()), parent);
    if (s) return s;

    if (parent) {
      Oid xoid = get_class_oid(o, parent);
      memcpy(oid, &xoid, sizeof(Oid));
    }
    else
      oid->invalidate();
    return Success;
  }

  static Status
  getv_name(const Object *o, Data *data, int nb, int from, Bool *isnull,
	    Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    const char *name = ((Class *)o)->getName();
    if (isGetTValue)
      {
	if (nb == Attribute::wholeData)
	  *data = (Data)strdup(name);
	else
	  memcpy(data, name, strlen(name)+1);

	if (rnb) *rnb = strlen(name) + 1;
	return Success;
      }

    if (nb == Attribute::directAccess)
      *data = (Data)name;
    else
      memcpy(data, name, strlen(name)+1);
    return Success;
  }

  static Status
  getv_mtype(const Object *o, Data *data, int nb, int from,
	     Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    const char *str = (((Class *)o)->isSystem() ? "system" : "user");

    if (isGetTValue)
      {
	if (nb == Attribute::wholeData)
	  *data = (Data)strdup(str);
	else
	  memcpy(data, str, strlen(str)+1);

	if (rnb) *rnb = strlen(str) + 1;
	return Success;
      }

    if (nb == Attribute::directAccess)
      *data = (Data)str;
    else
      memcpy(data, str, strlen(str)+1);

    return Success;
  }

  static Status
  getv_components(const Object *o, Data *data, int nb, int from,
		  Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;
    Collection *coll;
    Status status = o->asClass()->getComponents(coll);
    if (status) return status;

    if (isGetTValue)
      {
	memcpy(data, &coll->getOid(), sizeof(Oid));
	return Success;
      }

    memcpy(data, &coll, sizeof(Object *));
    return Success;
  }

  static Status
  geto_components(const Object *o, Oid *oid, int nb, int from)
  {
    Collection *coll;
    Status status = o->asClass()->getComponents(coll);
    if (status) return status;

    if (coll) {
      memcpy(oid, &coll->getOid(), sizeof(Oid));
    }
    else
      oid->invalidate();

    return Success;
  }

  static Status
  getv_extent(const Object *o, Data *data, int nb, int from,
	      Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;
    Collection *coll;
    Status status = o->asClass()->getExtent(coll);
    if (status) return status;

    if (isGetTValue)
      {
	memcpy(data, &coll->getOid(), sizeof(Oid));
	return Success;
      }

    memcpy(data, &coll, sizeof(Object *));
    return Success;
  }

  static Status
  geto_extent(const Object *o, Oid *oid, int nb, int from)
  {
    Collection *coll;
    Status status = o->asClass()->getExtent(coll);
    if (status) return status;

    if (coll)
      memcpy(oid, &coll->getOid(), sizeof(Oid));
    else
      oid->invalidate();

    return Success;
  }

  static Status
  getv_collcls(const Object *o, Data *data, int nb, int from,
	       Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;
    const Class *cl = ((CollectionClass *)(o->getClass()))->getCollClass();

    if (isGetTValue)
      {
	Oid xoid = get_class_oid(o, cl);
	memcpy(data, &xoid, sizeof(Oid));
	return Success;
      }

    memcpy(data, &cl, sizeof(Object *));
    return Success;
  }

  static Status
  geto_collcls(const Object *o, Oid *oid, int nb, int from)
  {
    Oid xoid = get_class_oid(o, ((CollectionClass *)(o->getClass()))->getCollClass());

    memcpy(oid, &xoid, sizeof(Oid));
    return Success;
  }


  static Status
  getv_isref(const Object *o, Data *data, int nb, int from,
	     Bool *isnull, Bool, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    Bool isref;
    (void)((CollectionClass *)(o->getClass()))->getCollClass(&isref);
    eyedblib::int32 i = isref;
    memcpy(data, &i, sizeof(i));
    return Success;
  }

  static Status
  getv_cname(const Object *o, Data *data, int nb, int from,
	     Bool *isnull, Bool isGetTValue, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    const char *name = ((Collection *)o)->getName();
    if (isGetTValue)
      {
	if (nb == Attribute::wholeData)
	  *data = (Data)strdup(name);
	else
	  memcpy(data, name, strlen(name)+1);
	if (rnb) *rnb = strlen(name) + 1;
	return Success;
      }

    if (nb == Attribute::directAccess)
      *data = (Data)name;
    else
      memcpy(data, name, strlen(name)+1);
    return Success;
  }

  static Status
  getv_dim(const Object *o, Data *data, int nb, int from,
	   Bool *isnull, Bool, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    eyedblib::int16 dim;
    (void)((CollectionClass *)(o->getClass()))->getCollClass(0, &dim);
    eyedblib::int32 i = dim;
    memcpy(data, &i, sizeof(i));
    return Success;
  }

  static Status
  getv_magorder(const Object *o, Data *data, int nb, int from,
		Bool *isnull, Bool, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    //int mgo = ((CollectionClass *)(o->getClass()))->getMagOrder();
    int mgo = 0;
    memcpy(data, &mgo, sizeof(mgo));
    return Success;
  }

  static Status
  getv_count(const Object *o, Data *data, int nb, int from,
	     Bool *isnull, Bool, Size *rnb)
  {
    if (isnull)
      *isnull = False;

    int count = ((Collection *)o)->getCount();
    memcpy(data, &count, sizeof(count));
    return Success;
  }


  static inline void make_items(int type, AttrNative **items, int items_cnt)
  {
    class_info[type].items = items;
    class_info[type].items_cnt = items_cnt;
  }

  enum {
    classITEM = 0,
#ifdef ADD_PROT
    protITEM,
#endif
    mtypeITEM,
    nameITEM,
    parentITEM,
    componentsITEM,
    extentITEM,
    collclsITEM,
    isrefITEM,
    dimITEM,
    magorderITEM,
    cnameITEM,
    countITEM,
    ITEMS_CNT
  };

  static void
  make_tmpitems(AttrNative *tmpitems[], CollectionClass **pcollsetobjs)
  {
    tmpitems[classITEM] = new AttrNative(Class_Class,
					 Object_Class,
					 "class", True,
					 getv_class, 0,
					 geto_class, 0);

    tmpitems[protITEM] = new AttrNative(Object_Class,
					Object_Class,
					"protection", True,
					getv_protection, setv_protection,
					geto_protection, seto_protection);

    tmpitems[nameITEM] = new AttrNative(Char_Class,
					Class_Class,
					"name", 128,
					getv_name, 0, 0, 0);


    tmpitems[mtypeITEM] = new AttrNative(Char_Class,
					 Class_Class,
					 "type", 16,
					 getv_mtype, 0, 0, 0);

    tmpitems[parentITEM] = new AttrNative(Class_Class,
					  Class_Class,
					  "parent", True,
					  getv_parent, 0,
					  geto_parent, 0);

    (*pcollsetobjs) = new CollSetClass(Object_Class, True);

    // "components" has been changed to "components" 
    tmpitems[componentsITEM] = new AttrNative(*pcollsetobjs,
					      Class_Class,
					      "components", True,
					      getv_components, 0,
					      geto_components, 0);

    // "extent" has been changed to "extent" 
    tmpitems[extentITEM] = new AttrNative(*pcollsetobjs,
					  Class_Class,
					  "extent", True,
					  getv_extent, 0,
					  geto_extent, 0);

    tmpitems[collclsITEM] = new AttrNative(*pcollsetobjs,
					   Collection_Class,
					   "collcls", True,
					   getv_collcls, 0,
					   geto_collcls, 0);

    tmpitems[isrefITEM] = new AttrNative(Int32_Class,
					 Collection_Class,
					 "isref", 0,
					 getv_isref, 0, 0, 0);

    tmpitems[dimITEM] = new AttrNative(Int32_Class,
				       Collection_Class,
				       "dim", 0,
				       getv_dim, 0, 0, 0);

    tmpitems[cnameITEM] = new AttrNative(Char_Class,
					 Collection_Class,
					 "name", 128,
					 getv_cname, 0, 0, 0);

    tmpitems[countITEM] = new AttrNative(Int32_Class,
					 Collection_Class,
					 "count", 0,
					 getv_count, 0, 0, 0);

    tmpitems[magorderITEM] = new AttrNative(Int32_Class,
					    Collection_Class,
					    "magorder", 0,
					    getv_magorder, 0, 0, 0);
  }

#ifdef ADD_PROT
  const int
  objitemsCOUNT    = 2,
    m_clitemsCOUNT   = 7,
    collitemsCOUNT   = 8;
#else
  const int
  objitemsCOUNT    = 1,
    m_clitemsCOUNT   = 6,
    collitemsCOUNT   = 7;
#endif

  static inline AttrNative **
  make(int count)
  {
    AttrNative **x = (AttrNative **)malloc(count * sizeof(AttrNative *));
    memset(x, 0, count * sizeof(AttrNative *));
    return x;
  }

  static AttrNative *tmpitems[ITEMS_CNT];
  static AttrNative **m_clitems, **objitems, **collitems;
  static  CollectionClass *collsetobjs;

  void AttrNative::init()
  {
    make_tmpitems(tmpitems, &collsetobjs);

    int n;

    objitems    = make(objitemsCOUNT);
    collitems   = make(collitemsCOUNT);
    m_clitems   = make(m_clitemsCOUNT);

    n = 0;
    objitems[n++]    = tmpitems[classITEM];
#ifdef ADD_PROT
    objitems[n++]    = tmpitems[protITEM];
#endif

    n = 0;
    m_clitems[n++]   = tmpitems[classITEM];
#ifdef ADD_PROT
    m_clitems[n++]   = tmpitems[protITEM];
#endif
    m_clitems[n++]   = tmpitems[mtypeITEM];
    m_clitems[n++]   = tmpitems[nameITEM];
    m_clitems[n++]   = tmpitems[parentITEM];
    m_clitems[n++]   = tmpitems[extentITEM];
    m_clitems[n++]   = tmpitems[componentsITEM];

    n = 0;
    collitems[n++]   = tmpitems[classITEM];
#ifdef ADD_PROT
    collitems[n++]   = tmpitems[protITEM];
#endif
    collitems[n++]   = tmpitems[cnameITEM];
    collitems[n++]   = tmpitems[countITEM];
    collitems[n++]   = tmpitems[collclsITEM];
    collitems[n++]   = tmpitems[isrefITEM];
    collitems[n++]   = tmpitems[dimITEM];
    collitems[n++]   = tmpitems[magorderITEM];

#if 0
    make_items(Object_Type, m_clitems, m_clitemsCOUNT);
#else
    make_items(Object_Type, objitems, objitemsCOUNT);
#endif

    make_items(Class_Type,   m_clitems, m_clitemsCOUNT);
    make_items(BasicClass_Type,   m_clitems, m_clitemsCOUNT);
    make_items(EnumClass_Type,    m_clitems, m_clitemsCOUNT);
    make_items(AgregatClass_Type, m_clitems, m_clitemsCOUNT);
    make_items(StructClass_Type,  m_clitems, m_clitemsCOUNT);
    make_items(UnionClass_Type,   m_clitems, m_clitemsCOUNT);

    make_items(Instance_Type,      objitems, objitemsCOUNT);
    make_items(Basic_Type,      objitems, objitemsCOUNT);
    make_items(Enum_Type,       objitems, objitemsCOUNT);
    make_items(Agregat_Type,    objitems, objitemsCOUNT);
    make_items(Struct_Type,     objitems, objitemsCOUNT);
    make_items(Union_Type,      objitems, objitemsCOUNT);
    make_items(Schema_Type, objitems, objitemsCOUNT);
  
    make_items(CollectionClass_Type, m_clitems, m_clitemsCOUNT);
    make_items(CollSetClass_Type,    m_clitems, m_clitemsCOUNT);
    make_items(CollBagClass_Type,    m_clitems, m_clitemsCOUNT);
    make_items(CollListClass_Type,   m_clitems, m_clitemsCOUNT);
    make_items(CollArrayClass_Type,  m_clitems, m_clitemsCOUNT);

    make_items(Collection_Type, collitems, collitemsCOUNT);
    make_items(CollSet_Type,    collitems, collitemsCOUNT);
    make_items(CollBag_Type,    collitems, collitemsCOUNT);
    make_items(CollList_Type,   collitems, collitemsCOUNT);
    make_items(CollArray_Type,  collitems, collitemsCOUNT);

    nat_items[ObjectITEMS].items           = (Attribute **)objitems;
    nat_items[ObjectITEMS].items_cnt       = objitemsCOUNT;
    nat_items[ClassITEMS].items       = (Attribute **)m_clitems;
    nat_items[ClassITEMS].items_cnt   = m_clitemsCOUNT;
    nat_items[CollectionITEMS].items       = (Attribute **)collitems;
    nat_items[CollectionITEMS].items_cnt   = collitemsCOUNT;
    nat_items[CollectionClassITEMS].items  = (Attribute **)m_clitems;
    nat_items[CollectionClassITEMS].items_cnt = m_clitemsCOUNT;

    collsetobjs->setAttributes((Attribute **)collitems, collitemsCOUNT);
  }

  void AttrNative::_release()
  {
    for (int i = 0; i < ITEMS_CNT; i++)
      delete tmpitems[i];

    free(objitems);
    free(collitems);
    free(m_clitems);

    //collsetobjs->setAttributes(NULL, 0);
    collsetobjs->release();
  }


  //
  // AttrNative methods
  //

  AttrNative::AttrNative
  (Class *cl, Class *cl_own, const char *s, Bool isRef,
   Status (*_getv)(const Object *, Data *, int, int, Bool *, Bool, Size *),
   Status (*_setv)(Object *, Data, int, int),
   Status (*_geto)(const Object *, Oid *oid, int nb, int from),
   Status (*_seto)(Object *, const Oid *, int, int, Bool)) :
    Attribute(cl, s, isRef)
  {
    code = AttrNative_Code;
    class_owner = cl_own;
    _getvalue = _getv;
    _setvalue = _setv;
    _getoid = _geto;
    _setoid = _seto;
    idr_item_psize = sizeof(Oid);
    idr_psize = idr_item_psize;
    idr_poff = 0;
    idr_voff = 0;
    idr_inisize = 0;
  }

  AttrNative::AttrNative
  (Class *cl, Class *cl_own, const char *s, int dim,
   Status (*_getv)(const Object *, Data *, int, int, Bool *, Bool, Size *),
   Status (*_setv)(Object *, Data, int, int),
   Status (*_geto)(const Object *, Oid *oid, int nb, int from),
   Status (*_seto)(Object *, const Oid *, int, int, Bool))
    : Attribute(cl, s, dim)
  {
    code = AttrNative_Code;
    class_owner = cl_own;
    _getvalue = _getv;
    _setvalue = _setv;
    _getoid = _geto;
    _setoid = _seto;

    cl->getIDRObjectSize(&idr_item_psize);
    idr_item_psize -= IDB_OBJ_HEAD_SIZE;
    idr_psize = dim * idr_item_psize;
    idr_poff = 0;
    idr_voff = 0;
    idr_inisize = 0;
  }

  void
  AttrNative::reportAttrCompSetOid(Offset *offset, Data idr) const
  {
  }

  Status
  AttrNative::cannot(const char *msg) const
  {
    return Exception::make(IDB_ATTRIBUTE_ERROR, "cannot %s for %s::%s",
			   msg, class_owner->getName(), name);
  }

  Status AttrNative::setOid(Object *o, const Oid *oid, int nb , int from, Bool check_class) const
  {
    if (_setoid)
      return (*_setoid)(o, oid, nb, from, check_class);
    return cannot("set oid");
  }

  Status AttrNative::getOid(const Object *o, Oid *oid, int nb, int from) const
  {
    if (_getoid)
      return (*_getoid)(o, oid, nb, from);
    return cannot("get oid");
  }

  Status AttrNative::setValue(Object *o, Data data,
			      int nb, int from, Bool) const
  {
    if (_setvalue)
      return (*_setvalue)(o, data, nb, from);

    return cannot("set value");
  }

  Status AttrNative::getValue(const Object *o, Data *data, int nb, int from, Bool *isnull) const
  {
    if (_getvalue)
      return (*_getvalue)(o, data, nb, from, isnull, False, 0);
    return cannot("get value");
  }

  Status AttrNative::getTValue(Database *db, const Oid &objoid,
			       Data *data, int nb, int from,
			       Bool *isnull, Size *rnb, Offset poffset) const
  {
    if (_getvalue)
      {
	if (rnb) *rnb = nb;
	Object *o;
	Status s = db->loadObject(objoid, o);
	if (s) return s;
	s = (*_getvalue)(o, data, nb, from, isnull, True, rnb);
	o->release();
	return s;
      }

    return cannot("get value");
  }

  Status AttrNative::trace(const Object *o, FILE *fd, int *indent, unsigned int flags, const RecMode *rcm) const
  {
    char *indent_str = make_indent(*indent);
    char prefix[64];
    Status status = Success;

    get_prefix(o, class_owner, prefix, sizeof(prefix));

    unsigned char data[128];
    status = getValue(o, (Data *)data, 1, 0, 0);

    if (status) return status;

    fprintf(fd, "%snative attribute ", indent_str);
    if (isString())
      fprintf(fd, "string ");
    else
      fprintf(fd, "%s ", cls->getName());

    fprintf(fd, "%s%s = ", (isIndirect() ? "*" : ""), name);

    if (is_basic_enum)
      {
	int len = strlen(indent_str) + strlen(prefix) + strlen(name) + 3;

	if (cls->asBasicClass())
	  status = ((BasicClass *)cls)->traceData(fd, len, data, data, (TypeModifier *)&typmod);

	fprintf(fd, ";\n");
      }
    else
      {
	Object *oo;
	memcpy(&oo, data, sizeof(Object *));
	if (oo)
	  {
	    if (rcm->isAgregRecurs(this, 0, oo))
	      status = ObjectPeer::trace_realize(oo, fd, *indent + INDENT_INC, flags, rcm);
	    else
	      fprintf(fd, "{%s};\n", oo->getOid().getString());
	  }
	else
	  fprintf(fd, "%s;\n", NullString);
      }

    delete_indent(indent_str);
    return status;
  }

  AttrNative::AttrNative(const AttrNative *agr,
			 const Class *_cls,
			 const Class *_class_owner, 
			 const Class *_dyn_class_owner, 
			 int n) :
    Attribute(agr, _cls, _class_owner, _dyn_class_owner, n)
  {
    _getvalue = agr->_getvalue;
    _setvalue = agr->_setvalue;
    _getoid   = agr->_getoid;
    _setoid   = agr->_setoid;

    idr_item_psize = agr->idr_item_psize;
    idr_psize      = agr->idr_psize;
    idr_poff       = agr->idr_poff;
    idr_voff       = agr->idr_voff;
    idr_inisize    = agr->idr_inisize;
  }

  void AttrNative::copy(int w, Attribute ** &items, unsigned int &items_cnt,
			Class *cls)
  {
    items_cnt = nat_items[w].items_cnt;
    items = (Attribute **)malloc(sizeof(Attribute *) * items_cnt);

    for (int i = 0; i < items_cnt; i++)
      items[i] =
	new AttrNative((AttrNative *)nat_items[w].items[i], 0, cls, cls, i);
  }
}
