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
#include <dlfcn.h>
#include <assert.h>
#include "AttrNative.h"
#include "Attribute_p.h"

#define XBOOL "bool"

#define DEF_PREFIX "eyedb::"
#define DEF_PREFIX1 "eyedb::"

using namespace std;

//#define COMP_TRACE

namespace eyedb {
  int Class::RemoveInstances = 0x12;

  static inline const IndexImpl *
  getDefaultIndexImpl()
  {
    static IndexImpl *defIndexImpl;
    if (!defIndexImpl)
      defIndexImpl = new IndexImpl(IndexImpl::Hash, 0, 2048, 0, 0, 0);
    return defIndexImpl;
  }

  void Class::_init(const char *s)
  {
    name = NULL;
    num = 0;
    aliasname = NULL;
    canonname = NULL;
    setPName(s);
    mustCreateComps = False;

    subclass_set = False;
    subclass_count = 0;
    subclasses = NULL;
    sch = 0;

    setClass(Class_Class);
    if (!getClass())
      setClass(this);

    idr_objsz = 0;
    idr_psize = 0;
    idr_vsize = 0;
    idr_inisize = 0;
    extent = NULL;
    components = NULL;
    complist = NULL;
    memset(clist, 0, sizeof(LinkedList *)*ComponentCount_C);
    attr_complist = NULL;
    memset(attr_clist, 0, sizeof(LinkedList *)*AttrComponentCount_C);
    complist = new LinkedList();
    type = _Class_Type;
    items_cnt = 0;
    items = 0;
    items_set = False;
    m_type = User;
    isFlat = False;
    isFlatSet = False;
    is_root = False;
    tied_code = 0;
    attrs_complete = False;
    partially_loaded = False;
    setup_complete = False;
    instance_dataspace = 0;
    instance_dspid = Dataspace::DefaultDspid;
    idximpl = getDefaultIndexImpl()->clone();
  }

  Class::Class(const char *s, Class *p) : Object()
  {
    _init(s);

    parent = (p ? p : Instance_Class);
    parent_oid.invalidate();
  }

  Class::Class(const char *s, const Oid *poid) : Object()
  {
    _init(s);

    if (poid)
      parent_oid = *poid;
    else
      parent_oid.invalidate();

    parent = 0;
  }

  Class::Class(Database *_db, const char *s, Class *p) : Object(_db)
  {
    _init(s);

    parent = (p ? p : Instance_Class);
    parent_oid.invalidate();
  }

  Class::Class(Database *_db, const char *s, const Oid *poid) : Object(_db)
  {
    _init(s);

    if (poid)
      parent_oid = *poid;
    else
      parent_oid.invalidate();

    parent = 0;
  }

  Class::Class(const Class &cl) : Object(cl)
  {
    _init(cl.getName());
    *this = cl;
  }

  Class::Class(const Oid &_oid, const char *_name) : Object()
  {
    _init(_name);
    oid = _oid;
    partially_loaded = True;
    parent = 0;
  }

#define STRDUP(X) ((X) ? strdup(X) : 0)

  Status
  Class::loadComplete(const Class *cl)
  {
    //assert(!cl->setup_complete);
    assert(cl->getRefCount());
    assert(this != cl);
    *this = *cl;
      
    return Success;
  }

  static void
  freeCompList(LinkedList *list, Class *cl)
  {
    /*
      if (!list)
      return;

      ClassComponent *comp;
      LinkedListCursor c(list);
  
      while (c.getNext((void *&)comp))
      if (comp->getClassOwner()->getOid() == cl->getOid())
      comp->release();
  
      delete list;
    */
    assert(0);
  }

  Class& Class::operator=(const Class &cl)
  {
    assert(cl.getRefCount());
    assert(getRefCount());
    //garbage(); => will be done in Object::operator=()
    *(Object *)this = (const Object &)cl;

    name = STRDUP(cl.name);
    aliasname = STRDUP(cl.aliasname);
    canonname = STRDUP(cl.canonname);

    parent = cl.parent;
    parent_oid = parent ? parent->getOid() : cl.parent_oid;
    idximpl = cl.idximpl->clone();

    subclass_set = cl.subclass_set;
    subclass_count = cl.subclass_count;
    subclasses = (Class **)malloc(sizeof(Class *) * subclass_count);
    for (int i = 0; i < subclass_count; i++)
      subclasses[i] = cl.subclasses[i];

    sch = cl.sch;

    setClass(cl.getClass());

    idr_objsz = cl.idr_objsz;
    idr_psize = cl.idr_psize;
    idr_vsize = cl.idr_vsize;

    idr_inisize = cl.idr_inisize;

    attrs_complete = cl.attrs_complete;
    extent = (cl.extent ? cl.extent->clone()->asCollection() : 0);
    extent_oid = cl.extent_oid;
    components = (cl.components ? cl.components->clone()->asCollection() : 0);
    comp_oid = cl.comp_oid;
    instance_dspid = cl.instance_dspid;
    instance_dataspace = cl.instance_dataspace;

    Object::freeList(complist, True);
    //freeCompList(complist, this);

    int k;
    for (k = 0; k < ComponentCount_C; k++)
      Object::freeList(clist[k], False);

    complist = Object::copyList(cl.complist, True);

    for (k = 0; k < ComponentCount_C; k++)
      clist[k] = Object::copyList(cl.clist[k], False);

    for (k = 0; k < AttrComponentCount_C; k++)
      Object::freeList(attr_clist[k], False);

    attr_complist = Object::copyList(cl.attr_complist, True);

    for (k = 0; k < AttrComponentCount_C; k++)
      attr_clist[k] = Object::copyList(cl.attr_clist[k], False);

    type = cl.type;
    items_cnt = cl.items_cnt;

    items = (Attribute **)malloc(sizeof(Attribute *) * items_cnt);
    for (int j = 0; j < items_cnt; j++)
      items[j] = cl.items[j]->clone(db);

    items_set = cl.items_set;

    m_type = cl.m_type;
    isFlat = cl.isFlat;
    isFlatSet = cl.isFlatSet;
    is_root = cl.is_root;
    tied_code = STRDUP(cl.tied_code);

    partially_loaded = False;
    setup_complete = False;

    /*
      printf("contruct #2 %s %p [%d]\n",
      name, this, setup_complete);
    */
    return *this;
  }

  void Class::setPName(const char *s)
  {
    free(name);
    name = strdup(s);
  }

  Status
  Class::setName(const char *s)
  {
    if (strcmp(s, name)) {
      if ((!strcmp(s, "short") && !strcmp(name, "int16"))
	  || (!strcmp(s, "int16") && !strcmp(name, "short")))
	return Success;

      if ((!strcmp(s, "long") && !strcmp(name, "int64"))
	  || (!strcmp(s, "int64") && !strcmp(name, "long")))
	return Success;

      if ((!strcmp(s, "int") && !strcmp(name, "int32"))
	  || (!strcmp(s, "int32") && !strcmp(name, "int")))
	return Success;

      return Exception::make(IDB_ERROR,
			     "cannot change name of class '%s' to '%s'",
			     name, s);
    }
    return Success;
  }

  Status
  Class::setNameRealize(const char *s)
  {
    if (strcmp(s, name))
      {
	setPName(s);
	if (db)
	  db->getSchema()->computeHashTable();
      }

    return Success;
  }

  static int
  pre(char *name, const char *pre)
  {
    int len = strlen(pre);
    if (!strncmp(name, pre, len))
      {
	name[len] += 'A' - 'a';
	return 1;
      }
    return 0;
  }

  const char *
  Class::classNameToCName(const char *name)
  {
    if (!strcmp("agregat_class", name))
      return "eyedb::AgregatClass";

    if (!strcmp("agregat", name))
      return "eyedb::Agregat";

    if (!strcmp("array_class", name))
      return "eyedb::CollArrayClass";

    if (!strcmp("array", name))
      return "eyedb::CollArray";

    if (!strcmp("bag_class", name))
      return "eyedb::CollBagClass";

    if (!strcmp("bag", name))
      return "eyedb::CollBag";

    if (!strcmp("basic_class", name))
      return "eyedb::BasicClass";

    if (!strcmp("basic", name))
      return "eyedb::Basic";

    if (!strcmp("class", name))
      return "eyedb::Class";

    if (!strcmp("collection_class", name))
      return "eyedb::CollectionClass";

    if (!strcmp("enum_class", name))
      return "eyedb::EnumClass";

    if (!strcmp("enum", name))
      return "eyedb::Enum";

    if (!strcmp("instance", name))
      return "eyedb::Instance";

    if (!strcmp("object", name))
      return "eyedb::Object";

    if (!strcmp("schema", name))
      return "eyedb::Schema";

    if (!strcmp("set_class", name))
      return "eyedb::CollSetClass";

    if (!strcmp("set", name))
      return "eyedb::CollSet";

    if (!strcmp("struct_class", name))
      return "eyedb::StructClass";

    if (!strcmp("struct", name))
      return "eyedb::Struct";

    return name;
#if 1
    static char sname[64];

    static const char class_suffix[] = "_class";
    static const int class_suffix_len = strlen(class_suffix);
    static const int prefix_len = strlen(DEF_PREFIX);
    static const char coll_prefix[] = "_class";
    static const int coll_prefix_len = strlen(coll_prefix);

    int len = strlen(name);
	
    if (len > class_suffix_len &&
	!strncmp(&name[len-class_suffix_len], "_class", class_suffix_len)) {
      char s[64];
      strncpy(s, name, len-class_suffix_len);
      s[len-class_suffix_len] = 0;
      sprintf(sname, "%s%sClass", DEF_PREFIX, s);
    }
    else
      sprintf(sname, "%s%s", DEF_PREFIX, name);

    sname[prefix_len] += 'A' - 'a';

    if (!strncmp(name, coll_prefix, coll_prefix_len))
      sname[coll_prefix_len] += 'A' - 'a';

    printf("class name : %s -> %s\n", name, sname);
    return sname;
#endif
  }

  // added 18/05/05
  const char *
  classNameToJavaName(const char *name)
  {
    static char sname[64];
    static const char class_suffix[] = "_class";
    static const int class_suffix_len = strlen(class_suffix);
    static const int prefix_len = strlen("org.eyedb.");
    static const char coll_prefix[] = "_class";
    static const int coll_prefix_len = strlen(coll_prefix);

    int len = strlen(name);
	
    if (len > class_suffix_len &&
	!strncmp(&name[len-class_suffix_len], "_class", class_suffix_len))
      {
	char s[64];
	strncpy(s, name, len-class_suffix_len);
	s[len-class_suffix_len] = 0;
	sprintf(sname, "org.eyedb.%sClass", s);
      }
    else
      sprintf(sname, "org.eyedb.%s", name);

    sname[prefix_len] += 'A' - 'a';

    if (!strncmp(name, coll_prefix, coll_prefix_len))
      sname[coll_prefix_len] += 'A' - 'a';

    return sname;
  }

  static std::string
  getSCName_2(const char *name, const std::string &prefix,
	      const char *prefix_sep = 0)
  {
#if 1    
    if (!strcmp(name, "bool"))
      return prefix + "Bool";

    // syscls
    if (!strcmp(name, "attribute_component"))
      return prefix + (prefix_sep ? "syscls." : "") + "AttributeComponent";
    if (!strcmp(name, "attribute_component_set"))
      return prefix + (prefix_sep ? "syscls." : "") + "AttributeComponentSet";
    if (!strcmp(name, "class_component"))
      return prefix + (prefix_sep ? "syscls." : "") + "ClassComponent";
    if (!strcmp(name, "agregat_class_component"))
      return prefix + (prefix_sep ? "syscls." : "") + "AgregatClassComponent";
    if (!strcmp(name, "class_variable"))
      return prefix + (prefix_sep ? "syscls." : "") + "ClassVariable"; 
    if (!strcmp(name, "index"))
      return prefix + (prefix_sep ? "syscls." : "") + "Index";
    if (!strcmp(name, "hashindex"))
      return prefix + (prefix_sep ? "syscls." : "") + "HashIndex";
    if (!strcmp(name, "btreeindex"))
      return prefix + (prefix_sep ? "syscls." : "") + "BTreeIndex";
    if (!strcmp(name, "index_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "IndexType";
    if (!strcmp(name, "collection_attribute_implementation"))
      return prefix + (prefix_sep ? "syscls." : "") + "CollAttrImpl";
    if (!strcmp(name, "executable_lang"))
      return prefix + (prefix_sep ? "syscls." : "") + "ExecutableLang";
    if (!strcmp(name, "argtype_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "ArgType_Type";
    if (!strcmp(name, "argtype"))
      return prefix + (prefix_sep ? "syscls." : "") + "ArgType";
    if (!strcmp(name, "executable_localisation"))
      return prefix + (prefix_sep ? "syscls." : "") + "ExecutableLocalisation";
    if (!strcmp(name, "executable_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "ExecutableType";
    if (!strcmp(name, "trigger_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "TriggerType";
    if (!strcmp(name, "signature"))
      return prefix + (prefix_sep ? "syscls." : "") + "Signature";
    if (!strcmp(name, "executable"))
      return prefix + (prefix_sep ? "syscls." : "") + "Executable";
    if (!strcmp(name, "agregat_class_executable"))
      return prefix + (prefix_sep ? "syscls." : "") + "AgregatClassExecutable";
    if (!strcmp(name, "method"))
      return prefix + (prefix_sep ? "syscls." : "") + "Method";
    if (!strcmp(name, "fe_method"))
      return prefix + (prefix_sep ? "syscls." : "") + "FEMethod";
    if (!strcmp(name, "fe_method_C"))
      return prefix + (prefix_sep ? "syscls." : "") + "FEMethod_C";
    if (!strcmp(name, "be_method"))
      return prefix + (prefix_sep ? "syscls." : "") + "BEMethod";
    if (!strcmp(name, "be_method_C"))
      return prefix + (prefix_sep ? "syscls." : "") + "BEMethod_C";
    if (!strcmp(name, "be_method_OQL"))
      return prefix + (prefix_sep ? "syscls." : "") + "BEMethod_OQL";
    if (!strcmp(name, "trigger"))
      return prefix + (prefix_sep ? "syscls." : "") + "Trigger";
    if (!strcmp(name, "unique_constraint"))
      return prefix + (prefix_sep ? "syscls." : "") + "UniqueConstraint";
    if (!strcmp(name, "notnull_constraint"))
      return prefix + (prefix_sep ? "syscls." : "") + "NotNullConstraint";
    if (!strcmp(name, "cardinality_description"))
      return prefix + (prefix_sep ? "syscls." : "") + "CardinalityDescription";
    if (!strcmp(name, "cardinality_constraint"))
      return prefix + (prefix_sep ? "syscls." : "") + "CardinalityConstraint";
    if (!strcmp(name, "cardinality_constraint_test"))
      return prefix + (prefix_sep ? "syscls." : "") + "CardinalityConstraint_Test";
    if (!strcmp(name, "protection_mode"))
      return prefix + (prefix_sep ? "syscls." : "") + "ProtectionMode";
    if (!strcmp(name, "protection_user"))
      return prefix + (prefix_sep ? "syscls." : "") + "ProtectionUser";
    if (!strcmp(name, "protection"))
      return prefix + (prefix_sep ? "syscls." : "") + "Protection";
    if (!strcmp(name, "unreadable_object"))
      return prefix + (prefix_sep ? "syscls." : "") + "UnreadableObject";
    if (!strcmp(name, "class_update_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "ClassUpdateType";
    if (!strcmp(name, "attribute_convert_type"))
      return prefix + (prefix_sep ? "syscls." : "") + "AttributeConvertType";
    if (!strcmp(name, "class_conversion"))
      return prefix + (prefix_sep ? "syscls." : "") + "ClassConversion";

    // utils
    if (!strcmp(name, "image_type"))
      return prefix + (prefix_sep ? "utils." : "") + "ImageType";
    if (!strcmp(name, "image"))
      return prefix + (prefix_sep ? "utils." : "") + "Image";
    if (!strcmp(name, "URL"))
      return prefix + (prefix_sep ? "utils." : "") + "CURL";
    //if (!strcmp(name, "CURL"))
    //return prefix + (prefix_sep ? "utils." : "") + "CURL";
    if (!strcmp(name, "w_config"))
      return prefix + (prefix_sep ? "utils." : "") + "WConfig";
    if (!strcmp(name, "month"))
      return prefix + (prefix_sep ? "utils." : "") + "Month";
    if (!strcmp(name, "weekday"))
      return prefix + (prefix_sep ? "utils." : "") + "Weekday";
    if (!strcmp(name, "date"))
      return prefix + (prefix_sep ? "utils." : "") + "Date";
    if (!strcmp(name, "time"))
      return prefix + (prefix_sep ? "utils." : "") + "Time";
    if (!strcmp(name, "time_stamp"))
      return prefix + (prefix_sep ? "utils." : "") + "TimeStamp";
    if (!strcmp(name, "time_interval"))
      return prefix + (prefix_sep ? "utils." : "") + "TimeInterval";
    if (!strcmp(name, "ostring"))
      return prefix + (prefix_sep ? "utils." : "") + "OString";

    // oqlctb
    if (!strcmp(name, "database_open_mode"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbDatabaseOpenMode";
    if (!strcmp(name, "lock_mode"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbLockMode";
    if (!strcmp(name, "transaction_mode"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbTransactionMode";
    if (!strcmp(name, "transaction_lockmode"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbTransactionLockMode";
    if (!strcmp(name, "recovery_mode"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbRecoveryMode";
    if (!strcmp(name, "tostring_flags"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbToStringFlags";
    if (!strcmp(name, "MapType"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbMapType";
    if (!strcmp(name, "DatType"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbDatType";
    if (!strcmp(name, "datafile"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbDatafile";
    if (!strcmp(name, "dataspace"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbDataspace";
    if (!strcmp(name, "eyedb"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbEyedb";
    if (!strcmp(name, "connection"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbConnection";
    if (!strcmp(name, "database"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbDatabase";
    if (!strcmp(name, "math"))
      return prefix + (prefix_sep ? "oqlctb." : "") + "OqlCtbMath";
#else
    if (!strcmp(name, "class_component"))
      return "ClassComponent";

    if (!strcmp(name, "agregat_class_component"))
      return "AgregatClassComponent";

    if (!strcmp(name, "class_variable"))
      return "ClassVariable";

    if (!strcmp(name, "attribute_index_mode"))
      return "AttributeIndexMode";

    if (!strcmp(name, "attribute_component"))
      return "AttributeComponent";

    if (!strcmp(name, "attribute_component_set"))
      return "AttributeComponentSet";

    if (!strcmp(name, "index"))
      return DEF_PREFIX1 "Index";

    if (!strcmp(name, "multi_index"))
      return DEF_PREFIX1 "MultiIndex";

    if (!strcmp(name, "hashindex"))
      return DEF_PREFIX1 "HashIndex";

    if (!strcmp(name, "btreeindex"))
      return DEF_PREFIX1 "BTreeIndex";

    if (!strcmp(name, "executable_lang"))
      return DEF_PREFIX1 "ExecutableLang";

    if (!strcmp(name, "argtype_type"))
      return DEF_PREFIX1 "ArgType_Type";

    if (!strcmp(name, "argtype"))
      return DEF_PREFIX1 "ArgType";

    if (!strcmp(name, "executable_localisation"))
      return DEF_PREFIX1 "ExecutableLocalisation";

    if (!strcmp(name, "executable_type"))
      return DEF_PREFIX1 "ExecutableType";

    if (!strcmp(name, "trigger_type"))
      return DEF_PREFIX1 "TriggerType";

    if (!strcmp(name, "signature"))
      return DEF_PREFIX1 "Signature";

    if (!strcmp(name, "executable"))
      return DEF_PREFIX1 "Executable";

    if (!strcmp(name, "agregat_class_executable"))
      return "AgregatClassExecutable";

    if (!strcmp(name, "method"))
      return DEF_PREFIX1 "Method";

    if (!strcmp(name, "fe_method"))
      return DEF_PREFIX1 "FEMethod";

    if (!strcmp(name, "fe_method_C"))
      return DEF_PREFIX1 "FEMethod_C";

    if (!strcmp(name, "be_method"))
      return DEF_PREFIX1 "BEMethod";

    if (!strcmp(name, "be_method_C"))
      return DEF_PREFIX1 "BEMethod_C";

    if (!strcmp(name, "be_method_OQL"))
      return DEF_PREFIX1 "BEMethod_OQL";

    if (!strcmp(name, "trigger"))
      return DEF_PREFIX1 "Trigger";

    if (!strcmp(name, "check_constraint"))
      return DEF_PREFIX1 "CheckConstraint";

    if (!strcmp(name, "unique_constraint"))
      return DEF_PREFIX1 "UniqueConstraint";

    if (!strcmp(name, "notnull_constraint"))
      return DEF_PREFIX1 "NotNullConstraint";

    if (!strcmp(name, "cardinality_description"))
      return DEF_PREFIX1 "CardinalityDescription";

    if (!strcmp(name, "cardinality_constraint"))
      return DEF_PREFIX1 "CardinalityConstraint";

    if (!strcmp(name, "inverse"))
      return DEF_PREFIX1 "Inverse";

    if (!strcmp(name, "protection_mode"))
      return DEF_PREFIX1 "ProtectionMode";

    if (!strcmp(name, "protection_user"))
      return DEF_PREFIX1 "ProtectionUser";

    if (!strcmp(name, "protection"))
      return DEF_PREFIX1 "Protection";

    if (!strcmp(name, "unreadable_object"))
      return DEF_PREFIX1 "UnreadableObject";

    if (!strcmp(name, "class_update_type"))
      return "ClassUpdateType";

    if (!strcmp(name, "attribute_convert_type"))
      return "AttributeConvertType";

    if (!strcmp(name, "class_conversion"))
      return "ClassConversion";

    if (!strcmp(name, XBOOL))
      return DEF_PREFIX1 "Bool";

    if (!strcmp(name, "database_open_mode"))
      return "OqlCtbDatabaseOpenMode";

    if (!strcmp(name, "lock_mode"))
      return DEF_PREFIX1 "OqlCtbLockMode";

    if (!strcmp(name, "transaction_mode"))
      return "OqlCtbTransactionMode";

    if (!strcmp(name, "transaction_lockmode"))
      return "OqlCtbTransactionLockMode";

    if (!strcmp(name, "recovery_mode"))
      return DEF_PREFIX1 "OqlCtbRecoveryMode";

    if (!strcmp(name, "tostring_flags"))
      return DEF_PREFIX1 "OqlCtbToStringFlags";

    if (!strcmp(name, "eyedb"))
      return "OqlCtbeyedb";

    if (!strcmp(name, "connection"))
      return "OqlCtbConnection";

    if (!strcmp(name, "database"))
      return "OqlCtbDatabase";

    if (!strcmp(name, "math"))
      return DEF_PREFIX1 "OqlCtbMath";

    if (!strcmp(name, "image_type"))
      return DEF_PREFIX1 "ImageType";

    if (!strcmp(name, "image"))
      return DEF_PREFIX1 "Image";

    if (!strcmp(name, "datafile"))
      return "OqlCtbDatafile";

    oups
    if (!strcmp(name, "URL"))
      return DEF_PREFIX1 "URL";

    if (!strcmp(name, "w_config"))
      return DEF_PREFIX1 "WConfig";

    if (!strcmp(name, "cstring"))
      return DEF_PREFIX1 "CString";

    if (!strcmp(name, "month"))
      return DEF_PREFIX1 "Month";

    if (!strcmp(name, "weekday"))
      return DEF_PREFIX1 "Weekday";

    if (!strcmp(name, "date"))
      return DEF_PREFIX1 "Date";

    if (!strcmp(name, "time"))
      return DEF_PREFIX1 "Time";

    if (!strcmp(name, "time_stamp"))
      return DEF_PREFIX1 "TimeStamp";

    if (!strcmp(name, "time_interval"))
      return DEF_PREFIX1 "TimeInterval";

    if (!strcmp(name, "ostring"))
      return DEF_PREFIX1 "OString";
#endif

    return "";
  }

  const char *
  Class::getSCName(const char *name)
  {
    std::string str = getSCName_2(name, DEF_PREFIX);
    if (str.length() == 0)
      return 0;
    static char sname[128];
    sprintf(sname, "%s", str.c_str());
    return sname;

  }

  const char *getJavaName(const Class *cls)
  {
    static char *buf = new char[256];
    const char *cname = cls->getCName();
    if (!strncmp(cname, DEF_PREFIX, strlen(DEF_PREFIX))) { // WARNING : does not work if DEF_PREFIX is ""
      strcpy(buf, "org.eyedb.");
      strcat(buf, &cname[strlen(DEF_PREFIX)]);
    }
    else
      strcpy(buf, cname);
    return buf;
  }

  const char *Class::getCName(Bool skip_nmsp) const
  {
    for (int i = 0; i < idbLAST_Type; i++)
      if (!strcmp(name, class_info[i].name))
	return Class::classNameToCName(name);

    const char *sCName = getSCName(name);

    if (!sCName)
      sCName = name;

    return sCName;
  }

  Class *Class::getParent()
  {
    return parent;
  }

  Status
  Class::getParent(Database *db, Class *&rparent)
  {
    if (parent) {
      rparent = parent;
      return Success;
    }

    if (!parent_oid.isValid()) {
      rparent = 0;
      return Success;
    }

    return db->loadObject(parent_oid, (Object *&)rparent);
  }

  const Class *Class::getParent() const
  {
    return parent;
  }

  Schema *Class::getSchema()
  {
    return sch;
  }

  const Schema *Class::getSchema() const
  {
    return sch;
  }

  Status Class::setValue(Data)
  {
    return Success;
  }

  Status Class::getValue(Data*) const
  {
    return Success;
  }

  Status
  Class::clean(Database *db)
  {
    for (int i = 0; i < items_cnt; i++) {
      Status s = items[i]->clean(db);
      if (s) return s;
    }

    return Success;
  }

  void Class::free_items(void)
  {
    for (int i = 0; i < items_cnt; i++)
      delete items[i];

    free(items);
    items = NULL;
  }

  void Class::pre_release(void)
  {
    for (int i = 0; i < items_cnt; i++)
      items[i]->pre_release();
  }

  Status
  Class::check_items(Attribute **agr, int base_n)
  {
    char **items_str = (char **)malloc(base_n * sizeof(char *));
    int i, j;

    int nitems = 0;

    for (i = 0; i < base_n; i++, nitems++)
      {
	// 13/2/2: ce test ne semble plus d'actualité
	items_str[nitems] = (char *)agr[i]->getName();
      }

    for (i = 0; i < base_n; i++)
      for (j = i+1; j < base_n; j++)
	if (!strcmp(items_str[i], items_str[j]))
	  {
	    char *s = items_str[i];
	    free(items_str);
	    return Exception::make(IDB_ATTRIBUTE_ERROR, "duplicate name '%s' in agregat_class '%s' [attribute #%d and #%d]", s, name, i, j);
	  }

    free(items_str);
    return Success;
  }

  Status Class::setAttributes(Attribute **agr, unsigned int base_n)
  {
    if (items_set)
      return Exception::make(IDB_ATTRIBUTE_ERROR, "class '%s' has already its attributes set", name);

    Status status;

    status = check_items(agr, base_n);

    if (status)
      return status;

    const Class *p;

    int native_cnt = items_cnt;
    int n = base_n + native_cnt;

    if ((p = getParent()) && p->asAgregatClass())
      n += p->items_cnt - native_cnt;

    items_cnt = n;

    Attribute **items_new = (Attribute**)malloc(sizeof(Attribute *) *
						items_cnt);

    int i;
#if 0
    printf("setting attribute for %p %s\n", this, name);
    if (!asAgregatClass()) {
      printf("copying native attributes\n");
      for (i = 0; i < native_cnt; i++)
	items_new[i] = new AttrNative((AttrNative *)items[i],
				      items[i]->getClass(),
				      items[i]->getClassOwner(),
				      this, i);
    }
    else
#endif
      for (i = 0; i < native_cnt; i++)
	items_new[i] = items[i];

    free(items);
    items = items_new;

    int nitems = native_cnt;

    if ((p = getParent()) && p->asAgregatClass())
      for (i = native_cnt; i < p->items_cnt; i++, nitems++)
	items[nitems] = makeAttribute(p->items[i],
				      p->items[i]->getClass(),
				      p->items[i]->getClassOwner(),
				      this, nitems);

    for (i = 0; i < base_n; i++, nitems++)
      items[nitems] = makeAttribute(agr[i], agr[i]->getClass(), this, this,
				    nitems);

    items_set = True;
    isFlat = isFlatStructure();
    isFlatSet = True;
    return compile();
  }

  Status Class::compile()
  {
    return Success;
  }

  const Attribute *Class::getAttribute(unsigned int n) const
  {
    if (n >= 0 && n < items_cnt)
      return items[n];
    else
      return 0;
  }

  unsigned int Class::getAttributesCount(void) const
  {
    return items_cnt;
  }

  static int inline
  get_scope(const char *nm)
  {
    for (int idx = 0; *nm; idx++)
      {
	char c = *nm++;
	if (c == ':' && *nm == ':')
	  return idx;
      }

    return -1;
  }

  const Attribute *Class::getAttribute(const char *nm) const
  {
    int idx = get_scope(nm);
    if (idx >= 0)
      {
	if (!db || !db->getSchema())
	  return (const Attribute *)0;

	char *clname = (char *)malloc(idx+1);
	strncpy(clname, nm, idx);
	clname[idx] = 0;

	Class *cl = db->getSchema()->getClass(clname);
	if (!cl)
	  {
	    free(clname);
	    return (const Attribute *)0;
	  }

	char *fname = strdup(&nm[idx+2]);

	const Attribute *item = 0;
	int _items_cnt = cl->items_cnt;
	Attribute **_items = cl->items;

	for (int i = _items_cnt - 1; i >= 0; i--)
	  if (!strcmp(_items[i]->name, fname) &&
	      !strcmp(_items[i]->class_owner->name, clname))
	    {
	      item = _items[i];
	      break;
	    }

	free(clname);
	free(fname);
	return item;
      }

    for (int i = items_cnt - 1; i >= 0; i--)
      if (!strcmp(items[i]->getName(), nm))
	return items[i];

    return (const Attribute *)0;
  }

  InstanceInfo class_info[idbLAST_Type];

  Class
  *Object_Class,

    *Class_Class,
    *BasicClass_Class,
    *EnumClass_Class,
    *AgregatClass_Class,
    *StructClass_Class,
    *UnionClass_Class,

    *Instance_Class,
    *Basic_Class,
    *Enum_Class,
    *Agregat_Class,
    *Struct_Class,
    *Union_Class,
    *Schema_Class,
    *Bool_Class,

    *CollectionClass_Class,
    *CollSetClass_Class,
    *CollBagClass_Class,
    *CollListClass_Class,
    *CollArrayClass_Class,

    *Collection_Class,
    *CollSet_Class,
    *CollBag_Class,
    *CollList_Class,
    *CollArray_Class;

  static void inline class_make(const char *name, Class **cls,
				int type, Class *parent)
  {
    class_info[type].name = name;
    *cls = new Class(class_info[type].name, parent);
    ClassPeer::setMType(*cls, Class::System);
    ObjectPeer::setUnrealizable(*cls, True);
  }

  static void inline class_make(Class *cls, int type)
  {
    cls->setAttributes((Attribute **)class_info[type].items,
		       class_info[type].items_cnt);
  }

  EnumClass *
  Class::makeBoolClass()
  {
    EnumClass *cls = new EnumClass(XBOOL);

    EnumItem *en[2];
    en[0] = new EnumItem("FALSE", "False_", (unsigned int)0);
    en[1] = new EnumItem("TRUE", "True_", (unsigned int)1);

    cls->setEnumItems(en, 2);

    delete en[0];
    delete en[1];

    ClassPeer::setMType(cls, Class::System);
    return cls;
  }

  Bool
  Class::isBoolClass(const char *name)
  {
    return IDBBOOL(!strcmp(name, XBOOL));
  }

  void Class::init(void)
  {
    class_make("object", &Object_Class, Object_Type,
	       (Class *)0);
    Object_Class->parent = (Class *)0;

    class_make("class", &Class_Class, Class_Type,
	       Object_Class);
    Object_Class->setClass(Class_Class);

    class_make("basic_class", &BasicClass_Class, BasicClass_Type,
	       Class_Class);
    class_make("enum_class", &EnumClass_Class, EnumClass_Type,
	       Class_Class);

    class_make("agregat_class", &AgregatClass_Class, AgregatClass_Type,
	       Class_Class);

    class_make("struct_class", &StructClass_Class, StructClass_Type,
	       AgregatClass_Class);

    class_make("union_class", &UnionClass_Class, UnionClass_Type,
	       AgregatClass_Class);

    class_make("instance", &Instance_Class, Instance_Type,
	       Object_Class);

    class_make("basic", &Basic_Class, Basic_Type,
	       Instance_Class);

    class_make("enum", &Enum_Class, Enum_Type,
	       Instance_Class);

    class_make("agregat", &Agregat_Class, Agregat_Type,
	       Instance_Class);

    class_make("struct", &Struct_Class, Struct_Type,
	       Agregat_Class);

    class_make("union", &Union_Class, Union_Type,
	       Agregat_Class);

    class_make("schema", &Schema_Class, Schema_Type,
	       Instance_Class);

    class_make("collection_class", &CollectionClass_Class, CollectionClass_Type,
	       Class_Class);

    class_make("bag_class", &CollBagClass_Class, CollBagClass_Type,
	       CollectionClass_Class);

    class_make("set_class", &CollSetClass_Class, CollSetClass_Type,
	       CollectionClass_Class);

    class_make("list_class", &CollListClass_Class, CollListClass_Type,
	       CollectionClass_Class);

    class_make("array_class", &CollArrayClass_Class, CollArrayClass_Type,
	       CollectionClass_Class);

    class_make("collection", &Collection_Class, Collection_Type,
	       Instance_Class);

    class_make("bag", &CollBag_Class, CollBag_Type,
	       Collection_Class);

    class_make("set", &CollSet_Class, CollSet_Type,
	       Collection_Class);

    class_make("list", &CollList_Class, CollList_Type,
	       Collection_Class);

    class_make("array", &CollArray_Class, CollArray_Type,
	       Collection_Class);

    Char_Class  = new CharClass();
    Byte_Class  = new ByteClass();
    OidP_Class  = new OidClass();
    Int16_Class = new Int16Class();
    Int32_Class = new Int32Class();
    Int64_Class = new Int64Class();
    Float_Class = new FloatClass();

    Bool_Class = Class::makeBoolClass();
    ObjectPeer::setUnrealizable(Bool_Class, True);

    AttrNative::init();

    class_make(Object_Class, Object_Type);
    class_make(Class_Class, Class_Type);
    class_make(BasicClass_Class, BasicClass_Type);
    class_make(EnumClass_Class, EnumClass_Type);
    class_make(AgregatClass_Class, AgregatClass_Type);
    class_make(StructClass_Class, StructClass_Type);
    class_make(UnionClass_Class, UnionClass_Type);
    class_make(Instance_Class, Instance_Type);
    class_make(Basic_Class, Basic_Type);
    class_make(Enum_Class, Enum_Type);
    class_make(Agregat_Class, Agregat_Type);
    class_make(Struct_Class, Struct_Type);
    class_make(Union_Class, Union_Type);
    class_make(Schema_Class, Schema_Type);
    class_make(CollectionClass_Class, CollectionClass_Type);
    class_make(CollBagClass_Class, CollBagClass_Type);
    class_make(CollSetClass_Class, CollSetClass_Type);
    class_make(CollListClass_Class, CollListClass_Type);
    class_make(CollArrayClass_Class, CollArrayClass_Type);
    class_make(Collection_Class, Collection_Type);
    class_make(CollBag_Class, CollBag_Type);
    class_make(CollSet_Class, CollSet_Type);
    class_make(CollList_Class, CollList_Type);
    class_make(CollArray_Class, CollArray_Type);
    class_make(Char_Class, Basic_Type);
    class_make(Byte_Class, Basic_Type);
    class_make(OidP_Class, Basic_Type);
    class_make(Int16_Class, Basic_Type);
    class_make(Int32_Class, Basic_Type);
    class_make(Int64_Class, Basic_Type);
    class_make(Float_Class, Basic_Type);
  }

  void Class::_release(void)
  {
#if 1
    AttrNative::_release();
#endif

    Object_Class->release();
    Class_Class->release();
    BasicClass_Class->release();
    EnumClass_Class->release();
    AgregatClass_Class->release();
    StructClass_Class->release();
    UnionClass_Class->release();
    Instance_Class->release();
    Basic_Class->release();
    Enum_Class->release();
    Agregat_Class->release();
    Struct_Class->release();
    Union_Class->release();
    Schema_Class->release();
    CollectionClass_Class->release();
    CollBagClass_Class->release();
    CollSetClass_Class->release();
    CollListClass_Class->release();
    CollArrayClass_Class->release();
    Collection_Class->release();
    CollBag_Class->release();
    CollSet_Class->release();
    CollList_Class->release();
    CollArray_Class->release();
  }

  // tries to make class not abstract

  Object * Class::newObj(Database *) const
  {
    return 0;
  }

  Object * Class::newObj(Data, Bool) const
  {
    return 0;
  }

  Status
  Class::trace_comps(FILE *fd, int indent,
		     unsigned int flags,
		     const RecMode *rcm) const
  {
    Status s;
    ClassComponent *comp;
    LinkedListCursor c(complist);

    char *indent_str = make_indent(indent);
    Bool nl = False;

    while (c.getNext((void *&)comp)) {
      IDB_CHECK_INTR();

      if ((flags & CompOidTrace) ||
	  ((flags & SysExecTrace) ||
	   !comp->asMethod() ||
	   (comp->asMethod() && 
	    !(comp->asMethod()->getEx()->getLang() &
	      SYSTEM_EXEC)))) {
	if (!nl) {nl = True; fprintf(fd, "\n");}
	fprintf(fd, "%s", indent_str);
	// EV 23/05/01: changed rcm to norecurs
	// because of a fatal and non resolved recursion error
	//Status s = comp->m_trace(fd, indent, flags, rcm);
	s = comp->m_trace(fd, indent, flags&~ContentsFlag, NoRecurs);
	if (s) return s;
	fprintf(fd, ";\n", indent_str);
      }
    }

    s = const_cast<Class *>(this)->makeAttrCompList();
    if (s) return s;

    if ((flags & AttrCompTrace) || (flags & AttrCompDetailTrace)) {
      if (attr_complist && attr_complist->getCount())
	fprintf(fd, "\n");
      LinkedListCursor cx(attr_complist);
      AttributeComponent *attr_comp;
      while (cx.getNext((void *&)attr_comp)) {
	IDB_CHECK_INTR();
	fprintf(fd, "%s", indent_str);
	s = attr_comp->m_trace(fd, indent, flags&~ContentsFlag, NoRecurs);
	if (s) return s;
	fprintf(fd, ";\n", indent_str);
      }
    }

    delete_indent(indent_str);
    return Success;
  }

  Status Class::trace(FILE *fd, unsigned int flags, const RecMode *rcm) const
  {
    return trace_realize(fd, INDENT_INC, flags, rcm);
  }

  Status Class::trace_common(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    /*
      if (flags & NativeTrace)
      fprintf(fd, "ObjectSize = %d ", idr_objsz);
    */

    trace_flags(fd, flags);
    fprintf(fd, "\n");

    //  trace_comps(fd, indent, flags, rcm);
    if ((flags & NativeTrace) == NativeTrace)
      {
	unsigned int _items_cnt;
	const Attribute **_items = getClass()->getAttributes(_items_cnt);

	for (int n = 0; n < _items_cnt; n++)
	  {
	    const Attribute *agreg = _items[n];
	    if (agreg->isNative()) {
	      // EV 23/05/01: changed rcm to norecurs
	      // because of a fatal and non resolved recursion error
	      //agreg->trace(this, fd, &indent, flags|0x100, rcm);
	      Status s = agreg->trace(this, fd, &indent,
				      (flags&~ContentsFlag)|0x100,
				      NoRecurs);
	      if (s) return s;
	    }
	  }	
      }

    return Success;
  }

  /*
    #define ATTR_PR() \
    if (!attr_list) {attr_list = True; fprintf(fd, " (");} \
    else {fprintf(fd, ", ");}
  */

#define ATTR_PR()

  void
  Class::genODL(FILE *fd, Schema *m, Attribute *attr) const
  {
    Bool attr_list = False;
    Index *idx = 0;

    Bool strdim = attr->isString();

    if (idx) {
      ATTR_PR();
      idx->s_trace(fd, strdim);
    }
  
    const Attribute *inv_item = NULL;

    if (attr->inv_spec.oid_cl.isValid() && !attr->inv_spec.item)
      inv_item = m->getClass(attr->inv_spec.oid_cl)->getAttributes()[attr->inv_spec.num];
    else
      inv_item = attr->inv_spec.item;

    if (inv_item)
      {
	ATTR_PR();
	fprintf(fd, " inverse %s::%s", inv_item->class_owner->getName(),
		inv_item->name);
      }

    LinkedListCursor c(complist);
    ClassComponent *comp;

    while (c.getNext((void *&)comp))
      if (comp->asCardinalityConstraint()) {
	if (!strcmp(comp->asCardinalityConstraint()->getAttrname().c_str(),
		    attr->getName())) {
	  ATTR_PR();
	  CardinalityConstraint *card = comp->asCardinalityConstraint();
	  fprintf(fd, card->getCardDesc()->getString());
	}
      }

    /*
      if (attr_list)
      fprintf(fd, ")");
    */
    fprintf(fd, ";\n");
  }

  int
  Class::genODL(FILE *fd, Schema *m) const
  {
    extern Bool odl_system_class;

    Status status = Success;

    if (const_cast<Class *>(this)->wholeComplete())
      return 0;

    if ((isSystem() && !odl_system_class) || isRootClass())
      return 0;

    if (asUnionClass())
      fprintf(fd, "union");
    else if (asAgregatClass())
      fprintf(fd, "class");
    else
      fprintf(fd, "native");
    
    fprintf(fd, " %s%s", (isSystem() ? "@" : ""), name);

    fprintf(fd, " (implementation <%s, hints = \"%s\">)",
	    idximpl->getStringType(),
	    idximpl->getHintsString().c_str());

    const Class *p;

    if (getParent() && !getParent()->isRootClass() &&
	strcmp(getParent()->getName(), "struct"))
      fprintf(fd, " extends %s%s", (isSystem() ? "@" : ""),
	      getParent()->getName());

    fprintf(fd, " {\n");

    for (int n = 0; n < items_cnt; n++)
      {
	Attribute *attr = items[n];

	if (attr->isNative() || !attr->getClassOwner()->compare(this))
	  continue;
      
	Bool strdim = attr->isString();
	if (strdim)
	  {
	    fprintf(fd, "\tattribute string");
	    if (attr->typmod.ndims == 1 &&
		attr->typmod.dims[0] > 0)
	      fprintf(fd, "<%d>", attr->typmod.dims[0]);
	  }
	else
	  {
	    fprintf(fd, "\t%s %s",
		    (attr->inv_spec.oid_cl.isValid() ? "relationship" :
		     "attribute"),
		    attr->cls->getName());

	    if (attr->isIndirect())
	      fprintf(fd, "*");
	  }

	if (strcmp(attr->class_owner->getName(), name))
	  fprintf(fd, " %s::%s", attr->class_owner->getName(), attr->name);
	else
	  fprintf(fd, " %s", attr->name);

	if (!strdim)
	  for (int j = 0; j < attr->typmod.ndims; j++)
	    {
	      if (attr->typmod.dims[j] < 0)
		fprintf(fd, "[]");
	      else
		fprintf(fd, "[%d]", attr->typmod.dims[j]);
	    }

	genODL(fd, m, attr);
      }

    ClassComponent *comp;
    LinkedListCursor c(complist);

    Bool nl = False;

    while (c.getNext((void *&)comp))
      if ((comp->asMethod() || comp->asTrigger())
	  && comp->getClassOwner()->compare(this)) {
	if (!nl) {nl = True; fprintf(fd, "\n");}
	fprintf(fd, "\t");
	Status s = comp->m_trace(fd, 0, NoScope|ExecBodyTrace, NoRecurs);
	if (s) return 0;
	fprintf(fd, ";\n");
      }

    const_cast<Class *>(this)->makeAttrCompList();

    if (attr_complist && attr_complist->getCount())
      fprintf(fd, "\n");
    AttributeComponent *attr_comp;
    LinkedListCursor cx(attr_complist);

    while (cx.getNext((void *&)attr_comp)) {
      fprintf(fd, "\t");
      const Class *xcls;
      const Attribute *xattr;
      Status s = Attribute::checkAttrPath(m, xcls, xattr, attr_comp->getAttrpath().c_str());
      if (s) return 0;
      s = attr_comp->m_trace(fd, 0,
			     (AttrCompDetailTrace
			      |(xattr->isString() ? IDB_ATTR_IS_STRING : 0)),
			     NoRecurs);
      if (s) return 0;
      fprintf(fd, ";\n");
    }

    fprintf(fd, "};\n");
    return 1;
  }

  Status
  Class::wholeComplete()
  {
    if (removed)
      return Exception::make(IDB_ERROR,
			     "class %s is removed", oid.toString());
    Status s;

    if (!parent && !attrs_complete)
      {
	s = Class::attrsComplete();
	//if (s) return s;
      }

    if (parent)
      {
	s = parent->wholeComplete();
	if (s) return s;
      }

    if (!attrs_complete)
      {
	s = attrsComplete();
	//if (s) return s;
      }

    if (isPartiallyLoaded() && db && db->isOpened()) {
      s = db->getSchema()->manageClassDeferred(this);
      if (s) return s;
    }

    if (!setup_complete)
      {
	s = setupComplete();
	if (s) return s;
      }

    return Success;
  }

  static bool debug_trace = getenv("EYEDB_DEBUG_TRACE") ? true : false;

  Status Class::trace_realize(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    IDB_CHECK_INTR();

    Status status = Success;
    char *indent_str = make_indent(indent);
    int n = 0;
    Schema *m = (db ? db->getSchema() : NULL);
    const Class *p = 0;

    if (state & Tracing)
      {
	fprintf(fd, "%s%s;\n", indent_str, oid.getString());
	delete_indent(indent_str);
	return Success;
      }

    char *lastindent_str = make_indent(indent - INDENT_INC);

    status = const_cast<Class *>(this)->wholeComplete();
    if (status) return status;

    const_cast<Class *>(this)->state |= Tracing;

    if (asAgregatClass())
      fprintf(fd, "%s",
	      (((Class *)this)->asStructClass() ? "struct" : "union"));
    else
      fprintf(fd, "class"); // was class

    fprintf(fd, " %s", name);
    fprintf(fd, " {%s}", oid.getString());

    if (flags & NativeTrace)
      fprintf(fd, " (implementation <%s, hints = \"%s\">)",
	      idximpl->getStringType(),
	      idximpl->getHintsString().c_str());
  
    p = getParent();
    while (p)
      {
	fprintf(fd, " : %s", p->getName());
	if (!p->getParent() && p->parent_oid.isValid())
	  ((Class *)p)->attrsComplete();
	p = p->getParent();
      }

    fprintf(fd, " { ");
    if (debug_trace) {
      fprintf(fd, "// psize %d, vsize %d, inisize %d, objsize %d ",
	      idr_psize, idr_vsize, idr_inisize, idr_objsz);
    }

    /*
      if (flags & NativeTrace)
      fprintf(fd, "object sizes = %dp, %dt bytes / ", idr_objsz, idr_psize);
    */

    status = trace_common(fd, indent, flags, rcm);
    if (status) goto out;

    for (n = 0; n < items_cnt; n++)
      {
	Attribute *attr = items[n];

	if (attr->isNative())
	  continue;
	
	Bool strdim = attr->isString();
	if (strdim)
	  {
	    fprintf(fd, "%sattribute string", indent_str);
	    if (attr->typmod.ndims == 1 &&
		attr->typmod.dims[0] > 0)
	      fprintf(fd, "<%d>", attr->typmod.dims[0]);
	  }
	else
	  {
	    fprintf(fd, "%s%s %s", indent_str,
		    (attr->inv_spec.oid_cl.isValid() ? "relationship" :
		     "attribute"),
		    attr->cls->getName());

	    if (attr->isIndirect())
	      fprintf(fd, "*");
	  }

	if (strcmp(attr->class_owner->getName(), name))
	  fprintf(fd, " %s::%s", attr->class_owner->getName(), attr->name);
	else
	  fprintf(fd, " %s", attr->name);

	if (!strdim)
	  for (int j = 0; j < attr->typmod.ndims; j++)
	    {
	      if (attr->typmod.dims[j] < 0)
		fprintf(fd, "[]");
	      else
		fprintf(fd, "[%d]", attr->typmod.dims[j]);
	    }

	Bool attr_list = False;

	const Attribute *inv_item;
	if (m && attr->inv_spec.oid_cl.isValid() && !attr->inv_spec.item)
	  inv_item = m->getClass(attr->inv_spec.oid_cl)->
	    getAttributes()[attr->inv_spec.num];
	else
	  inv_item = attr->inv_spec.item;

	if (inv_item)
	  {
	    ATTR_PR();
	    fprintf(fd, " inverse %s::%s", inv_item->class_owner->getName(),
		    inv_item->name);
	  }

	if (complist && complist->getCount())
	  {
	    LinkedListCursor c(complist);
	    ClassComponent *comp;

	    while (c.getNext((void *&)comp))
	      if (comp->asCardinalityConstraint())
		{
		  if (!strcmp(comp->asCardinalityConstraint()->getAttrname().c_str(),
			      attr->getName()))
		    {
		      ATTR_PR();
		      CardinalityConstraint *card = comp->asCardinalityConstraint();
		      fprintf(fd, card->getCardDesc()->getString());
		    }
		}
	  }

	/*
	  if (attr_list)
	  fprintf(fd, ")");
	*/

#if 0
	if (flags & CompOidTrace)
	  fprintf(fd, " [attr_comp_set_oid = %s]", attr->getAttrCompSetOid().toString());
#endif

	fprintf(fd, ";");
	if (debug_trace) {
	  Offset poff, voff;
	  Size item_psize, psize, inisize, item_vsize, vsize;

	  attr->getPersistentIDR(poff, item_psize, psize, inisize);
	  attr->getVolatileIDR(voff, item_vsize, vsize);
	  fprintf(fd,
		  " // poff %d, item_psize %d, psize %d, inisize %d, "
		  "voff %d, item_vsize %d, vsize %d",
		  poff, item_psize, psize, inisize, voff, item_vsize,
		  vsize);
		  
	}

	fprintf(fd, "\n");
      }

    status = trace_comps(fd, indent, flags, rcm);

  out:
    const_cast<Class *>(this)->state &= ~Tracing;
    fprintf(fd, "%s};\n", lastindent_str);
    delete_indent(indent_str);
    delete_indent(lastindent_str);

    return status;
  }

#define DEF_MAGORDER 100000
  unsigned int
  Class::getMagorder() const
  {
    return idximpl->getMagorder(DEF_MAGORDER);
  }

  void
  Class::codeExtentCompOids(Size alloc_size)
  {
    Offset offset;

    Data data = idr->getIDR();
    if (extent_oid.isValid()) {
      offset = IDB_CLASS_EXTENT;
      //printf("codeExtentOid -> %s\n", extent_oid.toString());
      oid_code (&data, &offset, &alloc_size, extent_oid.getOid());
    }

    if (comp_oid.isValid()) {
      offset = IDB_CLASS_COMPONENTS;
      //printf("codeCompOid -> %s\n", comp_oid.toString());
      oid_code (&data, &offset, &alloc_size, comp_oid.getOid());
    }
  }

  Status
  class_name_code(DbHandle *dbh, short dspid, Data *idr,
		      Offset *offset,
		      Size *alloc_size, const char *name)
  {
    int len = strlen(name);
    if (len >= IDB_CLASS_NAME_LEN)
      {
	eyedbsm::Oid data_oid;
	RPCStatus rpc_status = dataCreate(dbh, dspid, len+1,
					      (Data)name, &data_oid);
	if (rpc_status) return StatusMake(rpc_status);
	char c = IDB_NAME_OUT_PLACE;
	char_code(idr, offset, alloc_size, &c);
	oid_code (idr, offset, alloc_size, &data_oid);
	bound_string_code (idr, offset, alloc_size,
			   IDB_CLASS_NAME_PAD, 0);
	return Success;
      }

    char c = IDB_NAME_IN_PLACE;
    char_code(idr, offset, alloc_size, &c);
    bound_string_code(idr, offset, alloc_size, IDB_CLASS_NAME_LEN,
		      name);
    return Success;
  }

  Status
  class_name_decode(DbHandle *dbh, Data idr, Offset *offset,
			char **name)
  {
    char c;
    char_decode(idr, offset, &c);

    if (c == IDB_NAME_OUT_PLACE)
      {
	eyedbsm::Oid data_oid;
	RPCStatus rpc_status;

	oid_decode (idr, offset, &data_oid);
	unsigned int size;
	rpc_status = dataSizeGet(dbh, &data_oid, &size);
	if (rpc_status) return StatusMake(rpc_status);
	*name = (char *)malloc(size);
	rpc_status = dataRead(dbh, 0, size, (Data)*name, 0, &data_oid);
	if (rpc_status) return StatusMake(rpc_status);
	bound_string_decode (idr, offset, IDB_CLASS_NAME_PAD, 0);
	return Success;
      }

    assert(c == IDB_NAME_IN_PLACE);
    char *s;
    bound_string_decode(idr, offset, IDB_CLASS_NAME_LEN, &s);
    *name = strdup(s);
    return Success;
  }

  Status Class::create()
  {
    if (oid.isValid())
      return Exception::make(IDB_OBJECT_ALREADY_CREATED, "creating class '%s'", name);

    IDB_CHECK_WRITE(db);

    RPCStatus rpc_status;
    Size alloc_size;
    Offset offset;
    ObjectHeader hdr;
    Status s;  

    attrsComplete();

    idr->setIDR((Size)0);
    alloc_size = 0;
    Data data = 0;

    offset = IDB_CLASS_IMPL_TYPE;
    s = IndexImpl::code(data, offset, alloc_size, idximpl);
    if (s) return s;

    offset = IDB_CLASS_MTYPE;
    eyedblib::int32 mt = m_type;
    int32_code (&data, &offset, &alloc_size, &mt);

    offset = IDB_CLASS_DSPID;
    eyedblib::int16 dspid = get_instdspid();
    int16_code (&data, &offset, &alloc_size, &dspid);

    offset = IDB_CLASS_HEAD_SIZE;

    s = class_name_code(db->getDbHandle(), getDataspaceID(), &data, &offset,
			    &alloc_size, name);
    if (s) return s;
  
    int idr_sz = offset;
    idr->setIDR(idr_sz, data);
    headerCode(_Class_Type, idr_sz);

    codeExtentCompOids(alloc_size);

    rpc_status = objectCreate(db->getDbHandle(), getDataspaceID(), data,
				  oid.getOid());
  
    if (rpc_status == RPCSuccess)
      {
	gbx_locked = gbxTrue;

	ClassComponent *comp;
	LinkedListCursor c(complist);

	while (c.getNext((void *&)comp)) {
	  if (!comp->getClassOwner())
	    comp->setClassOwner(this);
	  comp->setClassOwnerOid(comp->getClassOwner()->getOid());
	  /*
	    printf("%s: realizing component %p %s %s class:create %s\n",
	    name, comp, comp->getName(),
	    comp->getOid().toString(),
	    comp->getClassOwner()->getOid().toString());
	  */
	  s = comp->realize();
	  if (s)
	    return s;
	}
      }
 
    return StatusMake(rpc_status);
  }

  void
  Class::setExtentCompOid(const Oid &_extent_oid, const Oid &_comp_oid)
  {
    extent_oid = _extent_oid;
    comp_oid = _comp_oid;
    /*
      printf("class %s setting extent_oid to %s %s\n", name, extent_oid.toString(),
      comp_oid.toString());
    */
  }

  Status Class::update()
  {
    Status status;

    return Success;
  }

  Status
  Class::createComps()
  {
    if (!mustCreateComps)
      return Success;

    //printf("Class::createComps(%p, %s)\n", this, name);
    gbx_locked = gbxTrue;

    ClassComponent *cls_comp;
    LinkedListCursor c(complist);

    Status s;
    while (c.getNext((void *&)cls_comp)) {
      cls_comp->setDatabase(db);

      if (!cls_comp->getClassOwner())
	cls_comp->setClassOwner(this);
      cls_comp->setClassOwnerOid(cls_comp->getClassOwner()->getOid());

      /*
	printf("%s: realizing class component %s %s class:createComps %s\n",
	name, cls_comp->getName(),
	cls_comp->getOid().toString(),
	cls_comp->getClassOwner()->getOid().toString());
      */
    
      s = cls_comp->realize();
      if (s) return s;
    }

    cls_comp = 0;

    s = makeAttrCompList();
    if (s) return s;

    LinkedListCursor cx(attr_complist);
    AttributeComponent *attr_comp;
    while (cx.getNext((void *&)attr_comp)) {
      if (attr_comp->getOid().isValid())
	continue;
      attr_comp->setDatabase(db);
      if (!attr_comp->getClassOwner())
	attr_comp->setClassOwner(this);
      attr_comp->setClassOwnerOid(attr_comp->getClassOwner()->getOid());

      /*
	printf("%s: realizing attribute component %s %s class:createComps %s\n",
	name, attr_comp->getName(),
	attr_comp->getOid().toString(),
	attr_comp->getClassOwner()->getOid().toString());
      */

      s = attr_comp->realize();
      if (s) return s;
    }

    mustCreateComps = False;
    // added the 5/02/02 to force report of attr_comp_set_oid in IDB_classWrite
    // in case of a class creation after the creation of attribute components
#if 1
    touch();
    return update();
#else
    return Success;
#endif
  }

  Status Class::postCreate()
  {
    //return createComps();
    mustCreateComps = True;
    return Success;
  }

  Status Class::realize(const RecMode*rcm)
  {
    return Object::realize(rcm);
  }

  Status Class::remove(const RecMode*)
  {
    return Exception::make(IDB_ERROR, "cannot delete the class '%s' this way",
			   name);
  }

  Size
  Class::getIDRObjectSize(Size *psize, Size *vsize,
			  Size *isize) const
  {
    if (psize)
      *psize = idr_psize;

    if (vsize)
      *vsize = idr_vsize;

    if (isize)
      *isize = idr_inisize;

    return idr_objsz;
  }

  Status Class::attrsComplete()
  {
    if (db && parent_oid.isValid()) {
      parent = db->getSchema()->getClass(parent_oid, True);
      if (!parent)
	return Exception::make(IDB_ERROR, "cannot complete parent '%s'",
			       parent_oid.getString());
    }

    return Success;
  }

  /*
    static Class *
  getClassFromName(const char *name)
  {
    // must improve this function !! hash code
    if (!strcmp("object", name))
      return Object_Class;

    if (!strcmp("class", name))
      return Class_Class;
    if (!strcmp("basic_class", name))
      return BasicClass_Class;
    if (!strcmp("enum_class", name))
      return EnumClass_Class;
    if (!strcmp("agregat_class", name))
      return AgregatClass_Class;
    if (!strcmp("struct_class", name))
      return StructClass_Class;
    if (!strcmp("union_class", name))
      return UnionClass_Class;

    if (!strcmp("instance", name))
      return Instance_Class;
    if (!strcmp("basic", name))
      return Basic_Class;
    if (!strcmp("enum", name))
      return Enum_Class;
    if (!strcmp("agregat", name))
      return Agregat_Class;
    if (!strcmp("struct", name))
      return Struct_Class;
    if (!strcmp("union", name))
      return Union_Class;

    if (!strcmp("schema", name))
      return Schema_Class;

    if (!strcmp(XBOOL, name))
      return Bool_Class;

    if (!strcmp("collection_class", name))
      return CollectionClass_Class;
    if (!strcmp("set_class", name))
      return CollSetClass_Class;
    if (!strcmp("bag_class", name))
      return CollBagClass_Class;
    if (!strcmp("list_class", name))
      return CollListClass_Class;
    if (!strcmp("array_class", name))
      return CollArrayClass_Class;

    if (!strcmp("collection", name))
      return Collection_Class;
    if (!strcmp("set", name))
      return CollSet_Class;
    if (!strcmp("bag", name))
      return CollBag_Class;
    if (!strcmp("list", name))
      return CollList_Class;
    if (!strcmp("array", name))
      return CollArray_Class;

    return 0;
  }
  */

  Status
  classMake(Database *db, const Oid *oid, Object **o,
		const RecMode *rcm, const ObjectHeader *hdr,
		Data idr, LockMode lockmode, const Class*)
  {
    RPCStatus rpc_status;
    Data temp;

    if (!idr)
      {
	temp = (unsigned char *)malloc(hdr->size);
	object_header_code_head(temp, hdr);
	rpc_status = objectRead(db->getDbHandle(), temp, 0, 0, oid->getOid(),
				    0, lockmode, 0);
      }
    else
      {
	temp = idr;
	rpc_status = RPCSuccess;
      }

    if (rpc_status == RPCSuccess)
      {
	Offset offset;
	char *s;

	IndexImpl *idximpl;
	offset = IDB_CLASS_IMPL_TYPE;
	Status status = IndexImpl::decode(db, temp, offset, idximpl);
	if (status) return status;

	eyedblib::int32 mt;
	offset = IDB_CLASS_MTYPE;
	int32_decode (temp, &offset, &mt);

	eyedblib::int16 dspid;
	offset = IDB_CLASS_DSPID;
	int16_decode (temp, &offset, &dspid);

	offset = IDB_CLASS_HEAD_SIZE;

	status = class_name_decode(db->getDbHandle(), temp, &offset, &s);
	if (status) return status;

	*o = db->getSchema()->getClass(s);
	(*o)->incrRefCount(); // added the 17/10/00
	free(s); s = 0;
	(*o)->asClass()->setExtentImplementation(idximpl, True);
	if (idximpl)
	  idximpl->release();
	(*o)->asClass()->setInstanceDspid(dspid);

	ClassPeer::setMType((Class *)*o, (Class::MType)mt);

	status = ClassPeer::makeColls(db, (Class *)*o, temp);

	if (status != Success)
	  {
	    if (!idr)
	      free(temp);
	    return status;
	  }
      }

    if (!idr)
      {
	if (!rpc_status)
	  ObjectPeer::setIDR(*o, temp, hdr->size);
      }
    return StatusMake(rpc_status);
  }

  Status Class::setDatabase(Database *mdb)
  {
    Status status = Object::setDatabase(mdb);

    if (status == Success && parent)
      {
	const char *_name = parent->getName();
	parent = mdb->getSchema()->getClass(_name);
      
	if (!parent)
	  {
	    //assert(0);
	    return Exception::make(IDB_SETDATABASE_ERROR, "class '%s': parent class '%s' not found in schema\n", name, _name);
	  }
      }
    return status;
  }

  Status
  Class::triggerManage(Trigger *trig)
  {
    if (trig->getEx()->getLang() == OQL_LANG)
      {
	Status s = trig->runtimeInit();
	if (db->getOpenFlag() & _DBAdmin)
	  return Success;
	return s;
      }

    if (!db->trig_dl)
      {
	const char *schname = db->getSchema()->getName();
	char file[64];
	sprintf(file, "%smthbe", schname);
	db->trig_dl = Executable::_dlopen(file);
	if (!db->trig_dl)
	  {
	    if (db->getOpenFlag() & _DBAdmin)
	      return Success;
	    std::string s = std::string("class `") + name + 
	      "' : trigger(s) check failed : " + dlerror();
	    return Exception::make(IDB_EXECUTABLE_ERROR, s);
	  }
      }

    trig->csym = (Status (*)(TriggerType, Database *,
			     const Oid &, Object *))
      dlsym(db->trig_dl, trig->getCSym());

    if (!trig->csym)
      {
	if (db->getOpenFlag() & _DBAdmin)
	  return Success;
	return Exception::make(IDB_EXECUTABLE_ERROR,
			       "trigger '%s' not found for database '%s'",
			       trig->getCSym(), db->getName());
      }

    return Success;
  }

  Status
  Class::getComp(const char *mcname, ClassComponent *&rcomp) const
  {
    rcomp = 0;
    const LinkedList *list = getCompList();
    if (!list)
      return Success;

    ClassComponent *comp;
    LinkedListCursor c(complist);
    while (c.getNext((void *&)comp))
      {
	Bool isnull;
	Status s = Success;
	const char *compname = comp->getName(&isnull, &s).c_str();
	if (s) return s;
	if (!strcmp(compname, mcname))
	  {
	    rcomp = comp;
	    return Success;
	  }  
      }

    return Success;
  }

  static ClassComponent **
  make_array(const LinkedList *list, unsigned int &cnt,
	     ClassComponent **prev = 0,
	     unsigned int prev_cnt = 0)
  {
    cnt = (list ? list->getCount() : 0) + prev_cnt;

    if (!cnt)
      return NULL;

    ClassComponent **comp_arr = (ClassComponent **)
      malloc(sizeof(ClassComponent *) * cnt);

    int i;
    for (i = 0; i < prev_cnt; i++)
      comp_arr[i] = prev[i];

    if (prev)
      delete[] prev;

    if (!list)
      return comp_arr;

    LinkedListCursor c(list);
    for (i = prev_cnt; c.getNext((void *&)comp_arr[i]); i++)
      ;

    return comp_arr;
  }

  Method **
  Class::getMethods(unsigned int& cnt)
  {
    return (Method **)make_array(getCompList(Method_C), cnt);
  }

  const Method **
  Class::getMethods(unsigned int& cnt) const
  {
    return (const Method **)make_array(getCompList(Method_C), cnt);
  }

  Status
  Class::getMethod(const char *_name, Method *&rmth, Signature *sign)
  {
    rmth = 0;
    const LinkedList *list = getCompList(Method_C);
    if (!list)
      return Success;

    LinkedListCursor c(list);
    Method *mth;
    while (c.getNext((void *&)mth))
      {
	Bool isnull;
	Status s = Success;
	const char *exname = mth->getEx()->getExname(&isnull, &s).c_str();
	if (s) return s;
	if (!strcmp(exname, _name) &&
	    (!sign || (*sign == *mth->getEx()->getSign())))
	  {
	    rmth = mth;
	    return Success;
	  }
      }

    return Success;
  }

  Status
  Class::getMethod(const char *_name, const Method *&mth,
		   Signature *sign) const
  {
    return const_cast<Class *>(this)->getMethod
      (_name, (const Method *&)mth, sign);
  }

  Status
  Class::getMethodCount(const char *_name, unsigned int &cnt) const
  {
    cnt = 0;
    const LinkedList *list = getCompList(Method_C);
    if (!list)
      return Success;

    LinkedListCursor c(list);
    Method *mth;
    while (c.getNext((void *&)mth))
      {
	Status s = Success;
	Bool isnull;
	const char *mthname = mth->getEx()->getExname(&isnull, &s).c_str();
	if (s) return s;

	if (!strcmp(mth->getEx()->getExname().c_str(), _name))
	  cnt++;
      }

    return Success;
  }

  unsigned int
  Class::getMethodCount() const
  {
    const LinkedList *list = getCompList(Method_C);
    return (list ? list->getCount() : 0);
  }

  Trigger **
  Class::getTriggers(unsigned int& cnt)
  {
    ClassComponent **arr;
    arr = make_array(getCompList(TrigCreateBefore_C), cnt);
    arr = make_array(getCompList(TrigCreateAfter_C), cnt, arr, cnt);
    arr = make_array(getCompList(TrigUpdateBefore_C), cnt, arr, cnt);
    arr = make_array(getCompList(TrigUpdateAfter_C), cnt, arr, cnt);
    arr = make_array(getCompList(TrigLoadBefore_C), cnt, arr, cnt);
    arr = make_array(getCompList(TrigLoadAfter_C), cnt, arr, cnt);
    arr = make_array(getCompList(TrigRemoveBefore_C), cnt, arr, cnt);
    return (Trigger **)
      make_array(getCompList(TrigRemoveAfter_C), cnt, arr, cnt);
  }

  const Trigger **
  Class::getTriggers(unsigned int& cnt) const
  {
    return (const Trigger **)((Class *)this)->getTriggers(cnt);
  }


  ClassVariable **
  Class::getVariables(unsigned int& cnt)
  {
    return (ClassVariable **)make_array(getCompList(Variable_C), cnt);
  }

  const ClassVariable **
  Class::getVariables(unsigned int& cnt) const
  {
    return (const ClassVariable **)make_array(getCompList(Variable_C), cnt);
  }

  Status
  Class::getVariable(const char *_name, ClassVariable *&rvar)
  {
    rvar = 0;
    const LinkedList *list = getCompList(Variable_C);
    if (!list)
      return Success;

    LinkedListCursor c(list);
    ClassVariable *var;
    while (c.getNext((void *&)var))
      {
	Status s = Success;
	Bool isnull;
	const char *vname = var->getVname(&isnull, &s).c_str();
	if (s) return s;
	if (!strcmp(vname, _name))
	  {
	    rvar = var;
	    return Success;
	  }
      }

    return Success;
  }

  Status
  Class::getVariable(const char *_name, const ClassVariable *&rvar) const
  {
    return const_cast<Class *>(this)->getVariable(_name, (const ClassVariable *&)rvar);
  }

  Status Class::add(unsigned int w, AttributeComponent *comp)
  {
    Status s = makeAttrCompList();
    if (s) return s;

    if (attr_complist->getPos(comp) < 0) {
      if (!attr_clist[w])
	attr_clist[w] = new LinkedList();

      attr_clist[w]->insertObject(comp);
      attr_complist->insertObject(comp);
    }

    return Success;
  }

  Status Class::add(unsigned int w, ClassComponent *comp, Bool incrRefCount)
  {
    Status status;

    if (db && ((db->isBackEnd() && !db->isLocal()) ||
	       (!db->isBackEnd() && db->isLocal())) && comp->asTrigger())
      {
	status = triggerManage(comp->asTrigger());
	if (status)
	  return status;
      }

    status = comp->make(this);

    if (status)
      return status;

    if (complist->getPos(comp) < 0)
      {
	if (!clist[w])
	  clist[w] = new LinkedList();

	LinkedList *list = clist[w];

	list->insertObject(comp);
	complist->insertObject(comp);
	if (incrRefCount)
	  ObjectPeer::incrRefCount(comp); // prevent deleting
	//printf("Class::add(this=%p, %s, %p, %s)\n", this, name, comp, comp->getName());
      }
    else if (!clist[w])
      abort();

    touch();
    return Success;
  }

  Status Class::suppress(unsigned int w, ClassComponent *comp)
  {
    if (clist[w])
      clist[w]->deleteObject(comp);
    complist->deleteObject(comp);
    // added the 21/05/01
#if 0
    ObjectPeer::decrRefCount(comp);
#endif
    //printf("Class::suppress(%p => %d)\n", comp, comp->getRefCount());
    assert(comp->getRefCount());
    touch();
    return Success;
  }

  Status Class::suppress(unsigned int w, AttributeComponent *comp)
  {
    if (attr_clist[w])
      attr_clist[w]->deleteObject(comp);
    if (attr_complist)
      attr_complist->deleteObject(comp);
    return Success;
  }

  Status Class::scanComponents()
  {
    /*
      printf("scanComponents %s, components %x %d %s\n", name, components,
      components->getCount(), components->getOid().toString());
    */

    Status status;
    Bool found;
    Iterator *q;
    ClassComponent *comp;
    //  Bool is_trs = db->isInTransaction();
    Bool is_trs = True;

    if (!is_trs)
      db->transactionBegin();

    status = getComponents(components);
    if (status) return status;

    q = new Iterator(components);

    if (q->getStatus())
      {
	status = q->getStatus();
	goto out;
      }      

    for (;;)
      {
	comp = 0;
	if (status = q->scanNext(&found, (Object **)&comp))
	  goto out;

	if (!found)
	  break;
      
	  
	//      if (status = add(comp->getInd(), comp, False))
	if (status = add(comp->getInd(), comp))
	  goto out;
      }

  out:
    delete q;
    if (!is_trs)
      db->transactionCommit();
    return status;
  }

  Status Class::setup(Bool force, Bool rescan)
  {
    // MIND: must add an argument: Bool force, to force
    // the scan even if components->getCount() == 0

    if ((state & Realizing))
      return Success;

    if (setup_complete && !rescan)
      return Success;

    state |= Realizing; // to avoid recursion

    Status s = getComponents(components);
    if (s)
      {
	state &= ~Realizing;
	return s;
      }

#ifdef COMP_TRACE
    printf("Class::setup(this=%p, name=%s, oid=%s, components=%p, count=%d, db=%p, idr=%p, loaded=%d)\n",
	   this, name, oid.toString(), components,
	   (components && components != (Collection*)1 ?
	    components->getCount() : 0), db, idr->getIDR(), partially_loaded);
#endif

    //  if (!db || !components || (!components->getCount() && !force))
    // HACK ADDED THE 21/11/00
    if (!db || !components || components == (Collection *)1 ||
	(!components->getCount() && !force))
      {
	state &= ~Realizing;
	return Success;
      }

    //freeCompList(complist, this);
    Object::freeList(complist, True);

    complist = new LinkedList();

    for (int i = 0; i < ComponentCount_C; i++)
      delete clist[i];

    memset(clist, 0, sizeof(LinkedList *) * ComponentCount_C);

    //Object::freeList(attr_complist, True);

    makeAttrCompList();
    /*
      attr_complist = new LinkedList();

      for (int i = 0; i < AttrComponentCount_C; i++)
      delete attr_clist[i];

      memset(attr_clist, 0, sizeof(LinkedList *) * AttrComponentCount_C);
    */

    s = scanComponents();
    if (s)
      {
	state &= ~Realizing;
	return s;
      }

    s = setupInherit();
    if (s)
      {
	state &= ~Realizing;
	return s;
      }

    setup_complete = True;
    state &= ~Realizing;
    return Success;
  }

  static Bool
  comp_find(LinkedList *list, const Oid &oid)
  {
    if (!list)
      return False;

    LinkedListCursor c(list);
    ClassComponent *comp;

    while (c.getNext((void *&)comp))
      if (comp->isInherit() && comp->getOid().compare(oid))
	return True;

    return False;
  }

  Status Class::setupInherit()
  {
    Class *cl;
    Status status;

    assert(!isRemoved());

#ifdef COMP_TRACE
    printf("Class::setupInherit(this=%p, name=%s)\n", this, name);
#endif
    cl = parent;

    // added the 5/01/00
    if (cl && !cl->setup_complete && !cl->getOid().isValid() && db)
      cl = db->getSchema()->getClass(cl->getName());

    while (cl)
      {
	assert(!cl->isRemoved());

	ClassComponent *comp;

#ifdef COMP_TRACE
	printf("\tClass::setupInherit(this=%p, name=%s, parent=%p, %s, db=%p, comp=%d, oid=%s)\n", 
	       this, name, cl, cl->getName(), cl->getDatabase(), cl->setup_complete, cl->getOid().toString());
#endif

	// added the 5/01/00
	if (!cl->setup_complete && (status = cl->setup(False)))
	  return status;

	LinkedList *list = cl->complist;
	LinkedListCursor c(list);
	while (c.getNext((void *&)comp))
	  {
#ifdef COMP_TRACE
	    printf("\tcomp=%s, inherit=%d oids=%s %s\n",
		   comp->getName(), comp->isInherit(),
		   comp->getClassOwner()->getOid().toString(), cl->getOid().toString());
#endif

	    //oid comparison optimisation added on 11/10/00
	    if (comp->isInherit() && 
		comp->getClassOwner()->getOid() == cl->getOid())
	      {
		int ind = comp->getInd();
#ifdef COMP_TRACE
		printf("\tcompclass=%p %p [%s %s]\n",
		       comp->getClassOwner(),
		       cl, 
		       comp->getClassOwner()->getOid().toString(),
		       cl->getOid().toString());
#endif
		if (!comp_find(clist[ind], comp->getOid()))
		  {
#ifdef COMP_TRACE
		    printf("\tcomponent not find: we addit.\n");
#endif
		    if ((status = add(ind, comp)) || (status = comp->make(this)))
		      return status;
		  }
	      }

	  }
	cl = cl->parent;
      }

    return Success;
  }

  Status
  Class::setupComplete()
  {
#ifdef COMP_TRACE
    printf("Class::setupComplete(this=%p, name=%s, %d)\n",
	   this, name, setup_complete);
#endif

    if (setup_complete)
      return Success;

    Status s;

    s = setup(True);
    if (s) return s;

    return Success;
  }

  const LinkedList *
  Class::getCompList() const
  {
    if (!setup_complete && db && db->isOpened())
      {
	//Status s = const_cast<Class *>(this)->setupComplete();
	Status s = const_cast<Class *>(this)->wholeComplete();
	if (s) throw *s;
      }
    return complist;
  }

  const LinkedList *
  Class::getCompList(CompIdx idx) const
  {
    if (!setup_complete)
      {
	//Status s = const_cast<Class *>(this)->setupComplete();
	Status s = const_cast<Class *>(this)->wholeComplete();
	if (s) throw *s;
      }
    return clist[idx];
  }

  Status
  Class::getAttrCompList(AttrCompIdx idx, const LinkedList *&list)
  {
    Status s = makeAttrCompList();
    if (s) return s;
    list = attr_clist[idx];
    return Success;
  }

  void
  Class::unmakeAttrCompList()
  {
    for (int i = 0; i < AttrComponentCount_C; i++) {
      delete attr_clist[i];
      attr_clist[i] = new LinkedList();
    }

    delete attr_complist;
    attr_complist = 0;
  }

  Status
  Class::makeAttrCompList()
  {
    if (attr_complist)
      return Success;

    attr_complist = new LinkedList();
    for (int i = 0; i < items_cnt; i++) {
      Status s = items[i]->getAttrComponents(db, this, *attr_complist);
      if (s) return s;
    }

    for (int k = 0; k < AttrComponentCount_C; k++) {
      delete attr_clist[k];
      attr_clist[k] = new LinkedList();
    }
  
    LinkedListCursor c(attr_complist);
    AttributeComponent *comp;
    while (c.getNext((void *&)comp)) {
      attr_clist[comp->getInd()]->insertObject(comp);
    }

    return Success;
  }

  Status
  Class::getAttrCompList(const LinkedList *&list)
  {
    Status s = makeAttrCompList();
    if (s) return s;

    list = attr_complist;
    return Success;
  }

  Status
  Class::getAttrComp(const char *mcname, AttributeComponent *&rcomp) const
  {
    Status s = const_cast<Class *>(this)->makeAttrCompList();
    if (s) return s;

    LinkedListCursor c(attr_complist);
    AttributeComponent *comp;
    while (c.getNext((void *&)comp)) {
      // 31/08/05: added !comp->isRemoved()
      if (!comp->isRemoved() && !strcmp(comp->getName().c_str(), mcname)) {
	rcomp = comp;
	return Success;
      }
    }

    rcomp = 0;
    return Success;
  }

  Status
  Class::setInSubClasses(ClassComponent *comp, Bool added)
  {
    int ind = comp->getInd();
    const LinkedList *_class = db->getSchema()->getClassList();
    Status status;
    Class *cl;

    LinkedListCursor c(_class);

    while (c.getNext((void *&)cl))
      if ((!added && !compare(cl)) || (added && cl != this))
	{
	  Bool found;
	  status = isSuperClassOf(cl, &found);
	  if (status)
	    return status;

	  if (found)
	    {
	      if (added)
		status = cl->add(ind, comp);
	      else
		status = cl->suppress(ind, comp);

	      if (status)
		return status;
	    }
	}

    return Success;
  }

  Status Class::add(ClassComponent *comp, Bool incrRefCount)
  {
    Status status;

    // persistance

    status = getComponents(components);
    if (status) return status;

    if (!components)
      return Exception::make(IDB_ERROR, "internal error in class::add: "
			     "no component collection in class %s %s",
			     name, oid.toString());

    if (status = components->insert(comp))
      return status;

    status = add(comp->getInd(), comp, incrRefCount);
    if (status)
      return status;

    if (comp->isInherit())
      {
	if (status = setInSubClasses(comp, True))
	  return status;
      }

    // persistance
    if (status = components->realize(NoRecurs))
      {
#if 1
	return status;
#else
	suppress(comp->getInd(), comp);
	return components->suppress(comp);
#endif
      }

    return Success;
  }

  Status Class::suppress(ClassComponent *comp)
  {
    if (isRemoved())
      return Exception::make(IDB_ERROR, "internal error in class::suppress: "
			     "class %s is removed",
			     oid.toString());

    Status status;

    status = getComponents(components);
    if (status) return status;

    if (!components)
      return Exception::make(IDB_ERROR, "internal error in class::suppress: "
			     "no component collection in class %s %s",
			     name, oid.toString());

    if (status = components->suppress(comp))
      return status;

    suppress(comp->getInd(), comp);

    if (comp->isInherit() && (status = setInSubClasses(comp, False)))
      return components->insert(comp);

    // persistance
    if (status = components->realize(NoRecurs))
      {
#if 1
	return status;
#else

	status = add(comp->getInd(), comp);
	if (status)
	  return status;

	return components->insert(comp);
#endif
      }

#if 0
    // to back end
    RPCstatus rpc_status = compSet(db->getDbHandle(), False,
					   &oid.getOid(),
					   &comp->getOid().getOid());
    if (rpc_status)
      {
	comp->supp(this);
	components->suppress(comp);
	components->realize(NoRecurs);
      }
    return Exception::make(rpc_status);
#else
    return Success;
#endif
  }

  IndexImpl *
  Class::getExtentImplementation() const
  {
    return idximpl->clone();
  }

  Status
  Class::setExtentImplementation(const IndexImpl *_idximpl)
  {
    if (oid.isValid())
      return Exception::make(IDB_ERROR, "class %s: extent implementation "
			     "cannot be set when class is created",
			     name);
    setExtentImplementation(_idximpl, True);
    return Success;
  }

  void
  Class::setExtentImplementation(const IndexImpl *_idximpl, Bool)
  {
    if (!idximpl || !idximpl->compare(_idximpl)) {
      if (idximpl)
	idximpl->release();
      /*
	printf("setting extent implementation %s %p %s\n",
	name, this, (const char *)_idximpl->getHintsString());
      */
      idximpl = _idximpl->clone();
    }
  }

#define GBX_NEW
  Status
  Class::getExtent(Collection*& _extent, Bool reload) const
  {
    if (reload && extent_oid.isValid()) {
      //Oid xoid = extent->getOid();
      if (extent) {
	extent->release();
	((Class *)this)->extent = 0;
      }
      Status status = db->reloadObject(&extent_oid, (Object **)&extent);
      if (status) return status;
    }
    else if (!extent && db && idr->getIDR()) {
      _extent = 0;

      Status status =
	ClassPeer::makeColl(db, (Collection **)&extent, idr->getIDR(),
			    IDB_CLASS_EXTENT);
      if (status) return status;

      if (!extent) {
	(void)dataRead(db->getDbHandle(), IDB_CLASS_EXTENT,
			   sizeof(Oid), idr->getIDR() + IDB_CLASS_EXTENT,
			   0, oid.getOid());
      
	status = ClassPeer::makeColl(db, (Collection **)&extent,
				     idr->getIDR(), IDB_CLASS_EXTENT);
	if (status) return status;
      }
    }

    // 7/05/02
#ifdef GBX_NEW
    if (extent)
      extent->keep();
#endif
    if (extent)
      const_cast<Class *>(this)->extent_oid = extent->getOid();

    _extent = extent;
    return Success;
  }

  Status
  Class::getComponents(Collection *&_components, Bool reload) const
  {
#ifdef COMP_TRACE
    printf("Class::getComponents(%p, name=%s, components=%p, reload=%p)\n",
	   this, name, components, reload);
#endif

    if (reload && comp_oid.isValid()) {
      //Oid xoid = components->getOid();
      if (components) {
	components->release();
	((Class *)this)->components = 0;
      }
      Status status = db->reloadObject(&comp_oid, (Object **)&components);
      if (status) return status;
    }
    else if (!components && db && idr->getIDR()) {
      _components = 0;

      Status status =
	ClassPeer::makeColl(db, (Collection **)&components, idr->getIDR(),
			    IDB_CLASS_COMPONENTS);
      if (status) return status;

      if (!components) { // ouh ouh la: was if (!_components)
	(void)dataRead(db->getDbHandle(), IDB_CLASS_COMPONENTS,
			   sizeof(Oid), idr->getIDR() + IDB_CLASS_COMPONENTS,
			   0, oid.getOid());
	
	status = ClassPeer::makeColl(db, (Collection **)&components,
				     idr->getIDR(), IDB_CLASS_COMPONENTS);
	if (status) return status;
      }
    }
  
    // 7/05/02
#ifdef GBX_NEW
    if (components)
      components->keep();
#endif

    if (components)
      const_cast<Class *>(this)->comp_oid = components->getOid();

    _components = components;
    return Success;
  }

  void
  Class::setTiedCode(char *_tied_code)
  {
    tied_code = _tied_code;
  }

  char *
  Class::getTiedCode()
  {
    return tied_code;
  }

  void Class::garbage()
  {
    free_items();
    free(name);
    free(aliasname);
    free(canonname);
    free(tied_code);
    free(subclasses);

    for (int i = 0; i < ComponentCount_C; i++) {
      delete clist[i];
      clist[i] = 0;
    }

    for (int i = 0; i < AttrComponentCount_C; i++) {
      delete attr_clist[i];
      attr_clist[i] = 0;
    }

    if (complist)
      {
	// disconnected the 19/06/00: components seem to be already 
	// released in Schema::gargabe()

	if (!db || !db->getSchema()->dont_delete_comps) {
	  Object::freeList(complist, True);
	  //Object::freeList(attr_complist, True);
	}
	else {
	  delete complist;
	  delete attr_complist;
	}

	complist = 0;
	attr_complist = 0;
      }

    if (extent)
      extent->release();
    if (components)
      components->release();
    if (idximpl)
      idximpl->release();
    Object::garbage();
  }

  Class::~Class()
  {
    garbageRealize();
  }

  Bool Class::isClass(Database *db, const Oid& cl_oid,
		      const Oid& oid)
  {
    if (cl_oid.compare(oid))
      return True;

    Class *cl = db->getSchema()->getClass(oid);

    while (cl)
      {
	if (cl->getOid().compare(cl_oid))
	  return True;
	cl = cl->getParent();
      }

    return False;
  }

  Bool
  Class::compare_l(const Class *cl) const
  {
    if (strcmp(getAliasName(), cl->getAliasName()))
      return False;

    if (type != cl->get_Type())
      return False;

    return True;
  }

  Bool
  Class::compare(const Class *cl) const
  {
    /*
    if (this == cl)
      return True;

    if (!compare_l(cl))
      return False;

    if (state & Realizing)
      return True;

    ((Class *)this)->state |= Realizing;
    Bool r = compare_perform(cl);
    ((Class *)this)->state &= ~Realizing;
    return r;
    */
    return compare(cl, True, True, True, True);
  }

  Bool Class::compare(const Class *cl,
		      Bool compClassOwner,
		      Bool compNum,
		      Bool compName,
		      Bool inDepth) const
  {
    if (this == cl)
      return True;

    if (strcmp(getAliasName(), cl->getAliasName()))
      return False;

    if (type != cl->get_Type())
      return False;

    if (state & Realizing)
      return True;

    ((Class *)this)->state |= Realizing;
    Bool r = compare_perform(cl, compClassOwner, compNum, compName, inDepth);
    ((Class *)this)->state &= ~Realizing;
    return r;
  }


  /*
  Bool Class::compare_perform(const Class *cl) const
  {
    return True;
  }
  */

  Bool Class::compare_perform(const Class *cl,
			      Bool compClassOwner,
			      Bool compNum,
			      Bool compName,
			      Bool inDepth) const
  {
    return True;
  }

  Bool
  Class::isFlatStructure() const
  {
    if (isFlatSet)
      return isFlat;

    for (int i = 0; i < items_cnt; i++)
      if (!items[i]->isNative() && !items[i]->isFlat())
	return False;

    return True;
  }

  static int
  sort_down_to_top_cmp(const void *xcls1, const void *xcls2)
  {
    Bool is;
    (*(Class **)xcls1)->isSuperClassOf(*(Class **)xcls2, &is);
    if (is) return 1;
    (*(Class **)xcls2)->isSuperClassOf(*(Class **)xcls1, &is);
    if (is) return -1;
    return 0;
  }

  static int
  sort_top_to_down_cmp(const void *xcls1, const void *xcls2)
  {
    Bool is;
    (*(Class **)xcls2)->isSuperClassOf(*(Class **)xcls1, &is);
    if (is) return 1;
    (*(Class **)xcls1)->isSuperClassOf(*(Class **)xcls2, &is);
    if (is) return -1;
    return 0;
  }

  Status
  Class::sort(Bool _sort_down_to_top) const
  {
    ((Class *)this)->sort_down_to_top = _sort_down_to_top;
    ::qsort(subclasses, subclass_count, sizeof(Class *),
	    (sort_down_to_top ? sort_down_to_top_cmp :
	     sort_top_to_down_cmp));
    return Success;
  }

  Status
  Class::getSubClasses(Class **&_subclasses, unsigned int &_subclass_count,
		       Bool _sort_down_to_top) const
  {
    Schema *msch = 0;
    _subclass_count = 0;
    if (!sch)
      {
	if (!db)
	  return Exception::make(IDB_ERROR, "class '%s': cannot get "
				 "subclasses when database is not set", name);
	msch = db->getSchema();
      }
    else
      msch = sch;

    if (subclass_set)
      {
	if (_sort_down_to_top == sort_down_to_top)
	  {
	    _subclass_count = subclass_count;
	    _subclasses = subclasses;
	    return Success;
	  }

	return sort(_sort_down_to_top);
      }

    Class *this_mutable = (Class *)this;
    LinkedListCursor c(msch->getClassList());
    Class *xcls;
    this_mutable->subclasses = 0;
    this_mutable->subclass_count = 0;

    while (c.getNext((void *&)xcls))
      {
	Bool is;
	Status status;

	if (status = isSuperClassOf(xcls, &is))
	  return status;
      
	if (is)
	  {
	    this_mutable->subclasses = (Class **)
	      realloc(subclasses, (subclass_count+1)*sizeof(Class *));

	    this_mutable->subclasses[this_mutable->subclass_count++] = xcls;
	  }
      }

    this_mutable->subclass_set = True;
    _subclasses = subclasses;
    _subclass_count = subclass_count;

    return sort(_sort_down_to_top);
  }

  Status
  Class::isSubClassOf(const Class *cl, Bool *is) const
  {
    return cl->isSuperClassOf(this, is);
  }

  Status
  Class::isSuperClassOf(const Class *cl, Bool *is) const
  {
    *is = False;

    while (cl)
      {
	if (compare(cl))
	  {
	    *is = True;
	    break;
	  }
	cl = cl->parent;
      }

    return Success;
  }

  static const char *NO_CLASS_CHECK = getenv("NO_CLASS_CHECK");

  Status
  Class::isObjectOfClass(const Object *o, Bool *is, Bool issub) const
  {
    if (NO_CLASS_CHECK) {
      *is = True;
      return Success;
    }

    /* if multi-database, be laxist for now! */
#if 0
    if (o->getOid().getDbid() != db->getDbid()) {
      *is = True;
      printf("multi database object %s : class '%s'\n",
	     o->getOid().getString(), o->getClass()->getName());
      return Success;
    }
#endif

    if (UnreadableObject::isUnreadableObject(o)) {
      *is = True;
      return Success;
    }

    if (issub) {
      return isSuperClassOf(o->getClass(), is);
    }

    *is = compare(o->getClass());
    return Success;
  }

  Status
  Class::isObjectOfClass(const Oid *o_oid, Bool *is, Bool issub,
			 Class **po_class) const
  {
    if (NO_CLASS_CHECK) {
      *is = True;
      return Success;
    }

    Status status;
    Class *o_class;

    *is = False;

    if (!db)
      return Exception::make(IDB_IS_OBJECT_OF_CLASS_ERROR, "database is not opened for class '%s', cannot performed isObjectOfClass(%s)", name, o_oid->getString());

    status = db->getObjectClass(*o_oid, o_class);

    if (UnreadableObject::isUnreadableObject(o_class)) {
      *is = True;
      return Success;
    }

    if (status)
      return status;
    if (po_class)
      *po_class = o_class;

    if (issub)
      return isSuperClassOf(o_class, is);

    *is = compare(o_class);
    return Success;
  }

  Status Class::generateCode_Java(Schema *,
				  const char *,
				  const GenCodeHints &,
				  FILE *)
  {
    return Success;
  }

  void Class::newObjRealize(Object *) const
  {
  }

  Status Class::checkInverse(const Schema *) const
  {
    return Success;
  }

  Status
  Class::makeClass(Database *db, const Oid &oid, int hdr_type,
		   const char *name, Bool& newClass, Class *&cl)
  {
#ifdef OPTOPEN_TRACE
    printf("begin Class::makeClass(%s, %s)\n", oid.getString(), name);
#endif
    newClass = True;
    Status s;
    switch(hdr_type)
      {
      case _StructClass_Type:
	cl = new StructClass(oid, name);
	break;

      case _Class_Type:
	cl = db->getSchema()->getClass(name);
	newClass = False;
	cl->setPartiallyLoaded(True);
	break;

      case _BasicClass_Type:
	cl = db->getSchema()->getClass(name);
	newClass = False;
	cl->setPartiallyLoaded(True);
	break;

      case _CollSetClass_Type:
	if (!strcmp(name, "set<object*>"))
	  {
	    cl = new CollSetClass(db->getSchema()->getClass("object"),
				  True);
	    cl->setPartiallyLoaded(True);
	  }
	else
	  cl = new CollSetClass(oid, name);
	break;

      case _CollBagClass_Type:
	cl = new CollBagClass(oid, name);
	break;

      case _CollArrayClass_Type:
	cl = new CollArrayClass(oid, name);
	break;

      case _CollListClass_Type:
	cl = new CollListClass(oid, name);
	break;

      case _EnumClass_Type:
	if (Class::isBoolClass(name)) {
	  cl = db->getSchema()->getClass(name);
	  newClass = False;
	  cl->setPartiallyLoaded(True);
	}
	else
	  cl = new EnumClass(oid, name);
	break;

      case _UnionClass_Type:
	//cl = new UnionClass(oid, name);
	break;

      default:
	assert(0);
      }

    if (!cl->getOid().isValid())
      ObjectPeer::setOid(cl, oid);
#ifdef OPTOPEN_TRACE
    printf("Class::makeClass(%s, %s, class=%p)\n", oid.getString(),
	   cl->getName(), cl);
#endif

    return Success;
  }

  Status
  Class::getDefaultInstanceDataspace(const Dataspace *&_instance_dataspace) const
  {
    if (instance_dataspace) {
      _instance_dataspace = instance_dataspace;
      return Success;
    }

    if (instance_dspid == Dataspace::DefaultDspid) {
      _instance_dataspace = 0;
      return Success;
    }

    Status s = db->getDataspace(instance_dspid, _instance_dataspace);
    if (s) return s;
    const_cast<Class *>(this)->instance_dataspace = _instance_dataspace;
    return Success;
  }

  Status
  Class::setDefaultInstanceDataspace(const Dataspace *_instance_dataspace)
  {
    if (!instance_dataspace && instance_dspid != Dataspace::DefaultDspid) {
      Status s = db->getDataspace(instance_dspid, instance_dataspace);
      if (s) return s;
    }

    if (instance_dataspace != _instance_dataspace) {
      instance_dataspace = _instance_dataspace;
      instance_dspid = (instance_dataspace ? instance_dataspace->getId() : Dataspace::DefaultDspid);
      touch();
      return store();
    }

    return Success;
  }

  Status
  Class::getInstanceLocations(ObjectLocationArray &locarr, Bool inclsub)
  {
    RPCStatus rpc_status =
      getInstanceClassLocations(db->getDbHandle(), oid.getOid(), inclsub,
				    (Data *)&locarr);
    return StatusMake(rpc_status);
  }

  Status
  Class::moveInstances(const Dataspace *dataspace, Bool inclsub)
  {
    RPCStatus rpc_status =
      moveInstanceClass(db->getDbHandle(), oid.getOid(), inclsub,
			    dataspace->getId());
    return StatusMake(rpc_status);
  }

  void
  Class::setInstanceDspid(short _instance_dspid)
  {
    instance_dspid = _instance_dspid;
  }

  Status
  Class::manageDataspace(short dspid)
  {
    if (dspid == Dataspace::DefaultDspid)
      return Success;
    const Dataspace *dataspace;
    Status s;
    s = db->getDataspace(dspid, dataspace);
    if (s) return s;
    return setDefaultInstanceDataspace(dataspace);
  }

  short
  Class::get_instdspid() const
  {
    return (instance_dataspace ? instance_dataspace->getId() : Dataspace::DefaultDspid);
  }


  // XDR
  //#define E_XDR_TRACE

  void
  Class::decode(void * hdata, // to
		const void * xdata, // from
		Size incsize, // temporarely necessary 
		unsigned int nb) const
  {
#ifdef E_XDR_TRACE
    cout << "Class::decode " << name << endl;
#endif
    CHECK_INCSIZE("decode", incsize, idr_psize - IDB_OBJ_HEAD_SIZE);

    memcpy(hdata, xdata, nb * incsize);
  }

  void
  Class::encode(void * xdata, // to
		const void * hdata, // from
		Size incsize, // temporarely necessary 
		unsigned int nb) const
  {
#ifdef E_XDR_TRACE
    cout << "Class::encode " << name << endl;
#endif
    CHECK_INCSIZE("encode", incsize, idr_psize - IDB_OBJ_HEAD_SIZE);

    memcpy(xdata, hdata, nb * incsize);
  }


  int
  Class::cmp(const void * xdata,
	     const void * hdata,
	     Size incsize, // temporarely necessary 
	     unsigned int nb) const
  {
#ifdef E_XDR_TRACE
    cout << "Class::cmp " << name;
#endif
    CHECK_INCSIZE("cmp", incsize, idr_psize - IDB_OBJ_HEAD_SIZE);

    int r = memcmp(xdata, hdata, nb * incsize);
#ifdef E_XDR_TRACE
    cout << " -> " << r << endl;
#endif
    return r;
  }

}
  
