/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
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


#ifndef _EYEDB_ODL_H
#define _EYEDB_ODL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eyedb/eyedb.h>

extern "C" {
  extern int odlparse();
}

namespace eyedb {

  extern int odl_error;

  //
  // basic list template
  //

  template <class T> struct odl_temp_link {
    T x;
    odl_temp_link<T> *next;

    odl_temp_link(T _x) : x(_x) {
      next = 0;
    }

    ~odl_temp_link() {}
  };

  template <class T> class odl_temp_list {

  public:
    odl_temp_link<T> *first;
    odl_temp_link<T> *last;

    int count;

    odl_temp_list() {
      first = last = 0;
      count = 0;
    }
    void add(T x) {
      odl_temp_link<T> *l = new odl_temp_link<T>(x);
      if (last)
	last->next = l;
      else
	first = l;
      last = l;
      count++;
    }
    /*
      void add(T _x) {
      T *x = new T;
      *x = _x;
      add(x);
      }
    */

    ~odl_temp_list() {
      odl_temp_link<T> *l = first;
      while (l)
	{
	  odl_temp_link<T> *next = l->next;
	  delete l;
	  l = next;
	}
    }
  };

  enum odlBool {
    odlFalse = 0,
    odlTrue = 1
  };

  extern const char *odl_get_typname(const char *);

  class odlIndexImplSpec;
  class odlCollImplSpec;
  class odlImplementation;

  class odlClassSpec {

  public:
    char *classname, *parentname, *aliasname;
    odlCollImplSpec *coll_impl_spec;

    odlClassSpec(const char *_classname, const char *_parentname,
		 const char *_aliasname, odlCollImplSpec *_coll_impl_spec);

    ~odlClassSpec() {
      free(classname);
      free(parentname);
      free(aliasname);
    }
  };

  //
  // forward declarations
  //

  class odlInverse;
  class odlImplementation;
  class odlIndex;
  class odlCardinality;
  class odlMagOrder;
  class odlImplementation;
  class odlExecSpec;
  class odlDeclItem;
  class odlMethodSpec;
  class odlTriggerSpec;
  class odlQuotedSeq;
  class odlArgSpec;
  class odlAgregatClass;
  class odlEnumClass;
  class odlAttrComponent;
  class odlConstraint;
  class odlUnique;
  class odlNotnull;
  class odlIndexImplSpec;
  class odlIndexImplSpecItem;
  class odlCollImplSpec;
  class odlCollImplSpecItem;

  typedef odl_temp_link<int>           odlArrayItemLink;
  typedef odl_temp_list<int>           odlArrayList;
  typedef odl_temp_link<odlArgSpec *>  odlArgSpecLink;
  typedef odl_temp_list<odlArgSpec *>  odlArgSpecList;
  typedef odl_temp_link<odlExecSpec *> odlExecLink;
  typedef odl_temp_list<odlExecSpec *> odlExecList;

  class odlUpdateHint;

  struct odlDeclRoot {
    virtual odlDeclItem *asDeclItem() {return 0;}
    virtual odlExecSpec *asExecSpec() {return 0;}
    virtual odlQuotedSeq *asQuotedSeq() {return 0;}
    virtual odlAttrComponent *asAttrComponent() {return 0;}
    virtual odlConstraint *asConstraint() {return 0;}
    virtual odlUnique *asUnique() {return 0;}
    virtual odlNotnull *asNotnull() {return 0;}
    virtual odlIndex *asIndex() {return 0;}
    virtual odlCardinality *asCardinality() {return 0;}
    virtual odlImplementation *asImplementation() {return 0;}
    virtual void setUpdateHint(odlUpdateHint *) { }
  };

  struct odlQuotedSeq : public odlDeclRoot {

    char *quoted_seq;

    odlQuotedSeq(const char *_quoted_seq) {
      quoted_seq = strdup(_quoted_seq);
    }

    odlQuotedSeq *asQuotedSeq() {return this;}

    ~odlQuotedSeq() {
      delete quoted_seq;
    }
  };

  struct odlCollSpec {
    char *collname;
    char *typname;
    odlBool isref;
    int dim;
    odlCollSpec *coll_spec;
    odlBool isString;

    odlCollSpec(const char *_collname, const char *_typname, odlBool _isref,
		int _dim) :
      collname(strdup(_collname)), typname(strdup(odl_get_typname(_typname))),
      isref(_isref), dim(_dim), coll_spec(NULL), isString(odlFalse) {}
  
    odlCollSpec(int _dim) : collname(NULL),
			    dim(_dim), typname(NULL), coll_spec(NULL), isref(odlFalse),
			    isString(odlTrue) {}

    odlCollSpec(const char *_collname, odlCollSpec *_coll_spec, odlBool _isref,
		int _dim) :
      collname(strdup(_collname)), typname(NULL),
      coll_spec(_coll_spec), isref(_isref), dim(_dim), isString(odlFalse) {}

    ~odlCollSpec() {
      free(collname);
      free(typname);
    }
  };

  struct odlUpdateHint {

    enum Type {
      UNDEFINED,
      Remove,
      RenameFrom,
      Convert,
      Extend,
      MigrateFrom,
      MigrateTo
    } type;

    char *detail, *detail2, *detail3;
    odlUpdateHint(Type _type, const char *_detail = 0,
		  const char *_detail2 = 0, 
		  const char *_detail3 = 0) :
      type(_type),
      detail(_detail ? strdup(_detail) : 0),
      detail2(_detail2 ? strdup(_detail2) : 0),
      detail3(_detail3 ? strdup(_detail3) : 0) { }
  };

  struct odlInverse {
    char *classname, *attrname;

    odlInverse(const char *_classname, const char *_attrname) {
      classname = (_classname ? strdup(_classname) : NULL);
      attrname = strdup(_attrname);
    }

    ~odlInverse() {
      free(classname);
      free(attrname);
    }
  };

  struct odlDeclItem : odlDeclRoot {
    char *attrname;
    char *typname;
    odlArrayList *array_list;
    odlInverse *inverse;
    odlCollSpec *coll_spec;
    odlBool isref;
    odlUpdateHint *upd_hints;

    odlDeclItem(const char *_attrname,
		const char *_typname, odlBool _isref,
		odlArrayList *_array_list, odlInverse *_inverse) {

      upd_hints = 0;
      attrname   = strdup(_attrname);
      array_list = _array_list;
      inverse  = _inverse;

      if (!strcmp(_typname, "string"))
	{
	  typname = strdup("char");
	  if (!array_list)
	    {
	      array_list = new odlArrayList();
	      array_list->add(-1);
	    }
	  else 
	    {
	      /*
		fprintf(stderr, "eyedbodl: attribute '%s': "
		"arrays of unbounded strings are not allowed. Use class 'ostring' instead of class 'string'.\n", attrname);
		odl_error++;
	      */
	      fprintf(stderr, "eyedbodl: warning attribute '%s': "
		      "arrays of unbounded strings are not allowed.\n"
		      "Using class 'ostring' instead of class 'string'.\n",
		      attrname);
	      free(typname);
	      typname = strdup("ostring");
	    }
	}
      else
	typname = strdup(odl_get_typname(_typname));

      coll_spec  = NULL;
      isref      = _isref;
    }

    odlDeclItem *asDeclItem() {return this;}

    odlDeclItem(const char *_attrname, odlCollSpec *_coll_spec,
		odlBool _isref,
		odlArrayList *_array_list, odlInverse *_inverse) {

      attrname   = strdup(_attrname);
      array_list = _array_list;
      inverse = _inverse;

      if (_coll_spec->isString)
	{
	  typname = strdup("char");
	  if (!array_list)
	    array_list = new odlArrayList();
	  else /*if (_coll_spec->dim < 0)*/
	    {
	      fprintf(stderr, "eyedbodl: attribute '%s': "
		      "array of string are not allowed: use class 'ostring'.\n", attrname);
	      odl_error++;
	    }
	    
	  array_list->add(_coll_spec->dim);
	  coll_spec = 0;
	}
      else
	{
	  coll_spec = _coll_spec;
	  typname = NULL;
	}

      isref      = _isref;
      upd_hints = 0;
    }

    void setUpdateHint(odlUpdateHint *_upd_hints) {
      upd_hints = _upd_hints;
    }

    odlBool hasInverseAttr() const;

    ~odlDeclItem() {
      free(attrname);
      free(typname);
      delete array_list;
    }
  };

  struct odlAttrComponent : public odlDeclRoot {

    char *attrpath;
    odlBool propagate;
    odlBool isCloned;

    odlAttrComponent(const char *_attrpath, odlBool propag) : 
      attrpath(strdup(_attrpath)), propagate(propag), isCloned(odlFalse) {}
    virtual odlAttrComponent  *asAttrComponent()  {return this;}
    AttributeComponent *make(Database *, Schema *m, Class *cls,
			     const Attribute *&);
    virtual AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *) = 0;
    virtual odlBool similar(odlAttrComponent *,
			    const Class *, const Class *);
    virtual odlAttrComponent *clone() {abort(); return 0;}
  };

  struct odlCollImplSpecItem {
    enum Type {
      UndefType,
      HashIndex,
      BTreeIndex,
      NoIndex
    } type;
    char *hints;

    void init() {
      type = UndefType;
      hints = 0;
    }
  };

  struct odlCollImplSpec {
    unsigned int item_alloc, item_cnt;
    odlCollImplSpecItem *items;

    odlCollImplSpec() {
      item_alloc = item_cnt = 0;
      items = 0;
    }

    void add(const odlCollImplSpecItem &item) {
      if (item_cnt >= item_alloc) {
	item_alloc += 4;
	items = (odlCollImplSpecItem *)realloc(items, item_alloc * sizeof(odlCollImplSpecItem));
      }
      items[item_cnt++] = item;
    }

    int make_class_prologue(const char *clsname,
			    odlCollImplSpecItem::Type &type,
			    char *&hints) const;
    int make_attr_prologue(const char *attrpath,
			   odlCollImplSpecItem::Type &type,
			   char *&hints, const Attribute *) const;
    int make_prologue(Bool isclass, const char *attrpath,
		      odlCollImplSpecItem::Type &type,
		      char *&hints, const Attribute *) const;
    ~odlCollImplSpec() {
      free(items);
    }
  };

  struct odlImplementation : public odlAttrComponent {

    odlCollImplSpec *coll_impl_spec;

    odlImplementation(const char *_attrpath, odlCollImplSpec *_coll_impl_spec,
		      odlBool propag = odlTrue) :
      odlAttrComponent(_attrpath, propag), coll_impl_spec(_coll_impl_spec) {}

    virtual odlImplementation *asImplementation() {return this;}
    AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *);
    odlBool similar(odlAttrComponent *,
		    const Class *, const Class *);
    virtual odlAttrComponent *clone() {odlAttrComponent *comp = new odlImplementation(attrpath, coll_impl_spec); comp->isCloned = odlTrue; return comp;}
  };

  struct odlIndexImplSpecItem {
    enum Type {
      UndefType,
      Hash,
      BTree
    } type;
    char *hints;

    void init() {
      type = UndefType;
      hints = 0;
    }
  };

  struct odlIndexImplSpec {
    unsigned int item_alloc, item_cnt;
    odlIndexImplSpecItem *items;

    odlIndexImplSpec() {
      item_alloc = item_cnt = 0;
      items = 0;
    }

    void add(const odlIndexImplSpecItem &item) {
      if (item_cnt >= item_alloc) {
	item_alloc += 4;
	items = (odlIndexImplSpecItem *)realloc(items, item_alloc * sizeof(odlIndexImplSpecItem));
      }
      items[item_cnt++] = item;
    }

    int make_prologue(const char *attrpath,
		      odlIndexImplSpecItem::Type &type,
		      char *&hints, const Attribute *) const;
    ~odlIndexImplSpec() {
      free(items);
    }
  };

  struct odlIndex : public odlAttrComponent {
    odlIndexImplSpec *index_impl_spec;
    odlIndexImplSpec *index_impl_spec_in;

    odlIndex(const char *_attrpath, odlIndexImplSpec *_index_impl_spec = 0,
	     odlBool propag = odlTrue) :
      odlAttrComponent(_attrpath, propag), index_impl_spec(_index_impl_spec),
      index_impl_spec_in(_index_impl_spec) {}

    static Status findIndex(Schema *m, Index *&idx_obj);

    virtual void retype(odlBool isCollOrIsRef) {
    }

    AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *);
    odlBool similar(odlAttrComponent *,
		    const Class *, const Class *);
    virtual odlIndex   *asIndex()   {return this;}
    virtual odlAttrComponent *clone() {odlAttrComponent *comp = new odlIndex(attrpath, index_impl_spec);  comp->isCloned = odlTrue; return comp;}
  };

  struct odlConstraint : public odlAttrComponent {

    odlConstraint(const char *_attrpath, odlBool propag) :
      odlAttrComponent(_attrpath, propag) {}
    virtual odlConstraint  *asConstraint()  {return this;}
  };

  struct odlUnique : public odlConstraint {

    odlUnique(const char *attrpath, odlBool propag = odlTrue) :
      odlConstraint(attrpath, odlTrue) {}
    virtual odlUnique  *asUnique()  {return this;}
    AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *);
    odlBool similar(odlAttrComponent *,
		    const Class *, const Class *);
    virtual odlAttrComponent *clone() {odlAttrComponent *comp = new odlUnique(attrpath); comp->isCloned = odlTrue; return comp;}
  };

  struct odlNotnull : public odlConstraint {

    odlNotnull(const char *attrpath, odlBool propag = odlTrue) :
      odlConstraint(attrpath, propag) {}
    virtual odlNotnull *asNotnull() {return this;}
    AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *);
    odlBool similar(odlAttrComponent *,
		    const Class *, const Class *);
    virtual odlAttrComponent *clone() {odlAttrComponent *comp = new odlNotnull(attrpath); comp->isCloned = odlTrue; return comp;}
  };

  struct odlCardinality : public odlAttrComponent {
    int bottom;
    odlBool bottom_excl;
    int top;
    odlBool top_excl;

    odlCardinality(int _bottom, odlBool _bottom_excl,
		   int _top, odlBool _top_excl) :
      odlAttrComponent("", odlTrue),
      bottom(_bottom), bottom_excl(_bottom_excl),
      top(_top), top_excl(_top_excl)
    { }

    AttributeComponent *make_realize(Database *, Schema *m, Class *cls, const Attribute *);
    odlBool similar(odlAttrComponent *,
		    const Class *, const Class *);
    virtual odlAttrComponent *clone() {odlAttrComponent *comp = new odlCardinality(bottom, bottom_excl, top, top_excl); comp->isCloned = odlTrue; return comp;}
    virtual odlCardinality *asCardinality() {return this;}
  };

  struct odlExecSpec : public odlDeclRoot {
    odlUpdateHint *upd_hints;

    odlExecSpec() {upd_hints = 0;}
    virtual odlMethodSpec *asMethodSpec() {return 0;}
    virtual odlTriggerSpec *asTriggerSpec() {return 0;}
    odlExecSpec *asExecSpec() {return this;}
    void setUpdateHint(odlUpdateHint *_upd_hints) {
      upd_hints = _upd_hints;
    }
  };

  struct odlArgSpec {

    int inout;
    char *typname;
    char *varname;

    odlArgSpec(int _inout, const char *_typname, const char *_varname) :
      inout(_inout), typname(strdup(_typname)),
      varname(_varname ? strdup(_varname) : NULL) {}

    ~odlArgSpec() {
      free(typname);
      free(varname);
    }
  };

  struct odlMethodHints {
    odlBool isClient;
    enum {
      ANY_HINTS, /* default */
      OQL_HINTS,
      C_HINTS,
      JAVA_HINTS
    } calledFrom;
  };

  struct odlSignUserData {
    char **names;
    odlMethodHints *mth_hints;
    odlUpdateHint *upd_hints;
    odlSignUserData(char **_names) {
      names = _names;
      mth_hints = 0;
      upd_hints = 0;
    }
  };

  struct odlMethodSpec : public odlExecSpec {
    char *rettype;
    char *fname;
    odlArgSpecList *arglist;
    char *extref;
    odlMethodHints mth_hints;
    odlBool isClassMethod;
    char *oqlSpec;

    odlMethodSpec(const char *_rettype, const char *_fname,
		  odlArgSpecList *_arglist)
    {
      mth_hints.isClient = odlFalse;
      mth_hints.calledFrom = odlMethodHints::ANY_HINTS;
      isClassMethod = odlFalse;
      rettype = strdup(_rettype);
      fname = strdup(_fname);
      arglist = _arglist;
      extref = 0;
      oqlSpec = 0;
    }

    odlMethodSpec *asMethodSpec() {return this;}

    int getParamNames(char **&typnames, char **&varnames);

    ~odlMethodSpec() {
      free(rettype);
      free(fname);
      free(extref);
      free(oqlSpec);
      delete arglist;
    }
  };

  struct odlTriggerSpec : public odlExecSpec {
    int light;
    char *name;
    char *localisation;
    char *oqlSpec;
    char *extref;

    odlTriggerSpec(int _light, const char *_localisation, const char *_name)
    {
      light = _light;
      name = strdup(_name);
      localisation = strdup(_localisation);
      oqlSpec = 0;
      extref = 0;
    }

    std::string makeOQLBody(const Class *cls) const;

    odlTriggerSpec *asTriggerSpec() {return this;}

    ~odlTriggerSpec() {
      free(oqlSpec);
      free(extref);
      free(name);
      free(localisation);
    }
  };

  struct odlEnumItem {
    char *name;
    char *aliasname;
    int value;
    odlBool novalue;

    odlEnumItem(const char *_name, const char *_aliasname, int _value) {
      name = strdup(_name);
      aliasname = _aliasname ? strdup(_aliasname) : 0;
      value = _value;
      novalue = odlFalse;
    }

    odlEnumItem(const char *_name, const char *_aliasname, odlBool) {
      name = strdup(_name);
      aliasname = _aliasname ? strdup(_aliasname) : strdup(_name);
      value = 0;
      novalue = odlTrue;
    }
  };


  typedef odl_temp_link<odlDeclRoot *> odlDeclRootLink;
  typedef odl_temp_list<odlDeclRoot *> odlDeclRootList;

  typedef odl_temp_link<odlDeclItem *> odlDeclItemLink;
  typedef odl_temp_list<odlDeclItem *> odlDeclList;

  typedef odl_temp_link<odlTriggerSpec *> odlTriggerSpecLink;
  typedef odl_temp_list<odlTriggerSpec *> odlTriggerList;

  typedef odl_temp_link<odlEnumItem *> odlEnumItemLink;
  typedef odl_temp_list<odlEnumItem *> odlEnumList;

  extern LinkedList *odl_decl_list;

  enum odlAgregSpec {
    odl_Struct,
    odl_Union,
    odl_SuperClass,
    odl_RootClass,
    odl_NativeClass,
    odl_Declare,
    odl_Remove
  };

  class odlDeclaration {
  protected:
    char *name;
    Class *ocls;
    Class *cls;
    char *aliasname;
    int check(Schema *, const char *prefix);

  public:
    odlDeclaration(const char *_name, const char *_aliasname) :
      name(_name ? strdup(_name) : 0), aliasname(_aliasname ? strdup(_aliasname) : 0) {
      odl_decl_list->insertObjectLast(this);
      ocls = NULL;
    }
    virtual int record(Database *, Schema *, const char *, const char *) = 0;
    virtual int realize(Database *, Schema *, const char *, const char *, Bool diff) = 0;
    const char *getName() const {return name;}
    const char *getAliasName() const {return aliasname;}

    void addPrefix(const char *prefix) {
      char *s = (char *)malloc(strlen(name)+strlen(prefix)+1);
      strcpy(s, prefix);
      strcat(s, name);
      free(name);
      name = s;
    }

    virtual odlAgregatClass *asAgregatClass() {return 0;}
    virtual odlEnumClass *asEnumClass() {return 0;}
    Class *getClass() {return cls;}
    Class *getOClass() {return ocls;}
    ~odlDeclaration() {free(name); free(aliasname);}
  };

  extern const char *odl_rootclass;

  class odlAgregatClass : public odlDeclaration {
    odlAgregSpec agrspec;
    char *parentname;
    odlDeclRootList *decl_list;
    Class *parent;
    odlCollImplSpec *coll_impl_spec;
    static LinkedList declared_list;

  public:
    odlUpdateHint *upd_hints;

    odlAgregatClass(odlUpdateHint *_upd_hints, odlAgregSpec _agrspec,
		    odlClassSpec *spec, odlDeclRootList *_decl_list) :
      odlDeclaration(spec->classname, spec->aliasname) {
      upd_hints = _upd_hints;
      agrspec = _agrspec;
      parentname = (spec->parentname ? strdup(spec->parentname) :
		    (superclass ? superclass->name : 0));
      coll_impl_spec = spec->coll_impl_spec;
      decl_list = _decl_list;

      class_count++;

      if (agrspec == odl_Declare)
	declared_list.insertObject(spec->classname);

      if (agrspec != odl_SuperClass &&
	  agrspec != odl_RootClass)
	return;

      if (superclass)
	{
	  if (superclass->agrspec == odl_RootClass)
	    fprintf(stderr,
		    "eyedbodl: superclass `%s' must be defined before all other classes\n", spec->classname);
	  else
	    fprintf(stderr,
		    "eyedbodl: can't have 2 superclasses `%s' and `%s'\n", 
		    spec->classname, superclass->name);
	  exit(1);
	}

      //      if (odl_decl_list->getCount() > 1)
      if (class_count > 1)
	{
	  fprintf(stderr,
		  "eyedbodl: superclass `%s' must be defined before all other classes\n", spec->classname);
	  exit(1);
	}

      if (agrspec == odl_RootClass &&
	  decl_list && decl_list->count > 0)
	{
	  fprintf(stderr,
		  "eyedbodl: volatile superclass `%s' cannot contain any attributes\n", spec->classname);
	  exit(1);
	}

      superclass = this;
    }

    odlBool hasSimilarComp(odlAttrComponent *comp, const Class *);
    int propagateComponents(Database *db, Schema *m);
    int record(Database *, Schema *, const char *, const char *);
    int realize(Database *db, Schema *, const char *, const char *, Bool diff);
    int postRealize(Database *db, Schema *, const char *);
    void realize(odlDeclItem *item,
		 Schema *m, const char *prefix,
		 int n, ClassComponent **comp,
		 int &comp_cnt, Attribute **agr);
    void realize(Database *db, odlAttrComponent *comp,
		 Schema *m, const char *prefix);
    void realize(Database *db, Schema *, odlExecSpec *, const char *);

    void addComp(odlAttrComponent *comp);
    int preManage(Schema *m);
    int manageDifferences(Database *db, Schema *m, Bool diff);
    int manageDiffRelationShips(Database *db, Schema *m, Bool diff);

    odlAgregSpec getAgregSpec() const {return agrspec;}

    static int getDeclaredCount() {return declared_list.getCount();}
    static LinkedList &getDeclaredList() {return declared_list;}

    virtual odlAgregatClass *asAgregatClass() {return this;}

    ~odlAgregatClass() {
      free(parentname);
      delete decl_list;
    }
    static odlAgregatClass *superclass;
    static unsigned int class_count;
  };

  class odlEnumClass : public odlDeclaration {
    odlEnumList *enum_list;

  public:
    /*
      odlEnumClass(const char *_name, odlEnumList *_enum_list,
      const char *_aliasname) :
      odlDeclaration(_name, _aliasname) {
      enum_list = _enum_list;
      }
    */

    odlEnumClass(const char *_aliasname, odlEnumList *_enum_list,
		 const char *_name) :
      odlDeclaration((_name ? _name : _aliasname), _aliasname) {
      enum_list = _enum_list;
    }

    int record(Database *, Schema *, const char *, const char *);
    int realize(Database *, Schema *, const char *, const char *, Bool diff);

    odlEnumClass *asEnumClass() {return this;}
    ~odlEnumClass() {
      delete enum_list;
    }
  };

  //
  // schema flexibility
  //

  class odlUpdateComponent;
  class odlAddComponent;
  class odlRemoveComponent;

  class odlUpdateAttribute;
  class odlAddAttribute;
  class odlRemoveAttribute;
  class odlRenameAttribute;
  class odlConvertAttribute;
  class odlReorderAttribute;
  class odlMigrateAttribute;

  class odlUpdateRelationship;
  class odlAddRelationship;
  class odlRemoveRelationship;

  class odlUpdateClass;
  class odlAddClass;
  class odlRemoveClass;
  class odlRenameClass;
  class odlReparentClass;
  class odlConvertClass;

  class odlUpdateEnum;

  class odlUpdateItem {

  public:
    ClassConversion *clsconv;

    odlUpdateItem() : clsconv(0) { }

    virtual void display() = 0;
    virtual void displayDiff(Database *db, const char *odlfile) = 0;
    virtual Status prePerform(Database *, Schema *m) {
      return Success;
    }

    virtual Status postPerform(Database *, Schema *m) {
      return Success;
    }

    static void initDisplay();
    static void initDisplayDiff(Database *, const char * = 0);

    virtual odlUpdateComponent *asUpdateComponent() {return 0;}
    virtual odlAddComponent *asAddComponent() {return 0;}
    virtual odlRemoveComponent *asRemoveComponent() {return 0;}

    virtual odlUpdateAttribute *asUpdateAttribute() {return 0;}
    virtual odlAddAttribute *asAddAttribute() {return 0;}
    virtual odlRemoveAttribute *asRemoveAttribute() {return 0;}
    virtual odlRenameAttribute *asRenameAttribute() {return 0;}
    virtual odlConvertAttribute *asConvertAttribute() {return 0;}
    virtual odlReorderAttribute *asReorderAttribute() {return 0;}
    virtual odlMigrateAttribute *asMigrateAttribute() {return 0;}

    virtual odlUpdateRelationship *asUpdateRelationship() {return 0;}
    virtual odlAddRelationship *asAddRelationship() {return 0;}
    virtual odlRemoveRelationship *asRemoveRelationship() {return 0;}

    virtual odlUpdateClass *asUpdateClass() {return 0;}
    virtual odlAddClass *asAddClass() {return 0;}
    virtual odlRemoveClass *asRemoveClass() {return 0;}
    virtual odlRenameClass *asRenameClass() {return 0;}
    virtual odlReparentClass *asReparentClass() {return 0;}
    virtual odlConvertClass *asConvertClass() {return 0;}
  };

  class odlUpdateComponent : public odlUpdateItem {

  private:
    odlBool updating;
    void setUpdating(Object *o) {
      updating = o->getOid().isValid() ? odlTrue : odlFalse;
    }

  protected:
    void realize(Database *, Schema *);

  public:
    ClassComponent *cls_comp;
    AttributeComponent *attr_comp;

    odlUpdateComponent(ClassComponent *_cls_comp) :
      cls_comp(_cls_comp), attr_comp(0) { setUpdating(cls_comp); }
    odlUpdateComponent(AttributeComponent *_attr_comp) :
      attr_comp(_attr_comp), cls_comp(0) { setUpdating(attr_comp); }
    virtual void display();
    virtual void displayDiff(Database *db, const char *odlfile);
    virtual odlUpdateComponent *asUpdateComponent() {return this;}
  };

  class odlAddComponent : public odlUpdateComponent {

  public:

    odlAddComponent(ClassComponent *);
    odlAddComponent(AttributeComponent *);
    virtual odlAddComponent *asAddComponent() {return this;}
    Status postPerform(Database *, Schema *);
  };

  class odlRemoveComponent : public odlUpdateComponent {

  public:

    odlRemoveComponent(ClassComponent *);
    odlRemoveComponent(AttributeComponent *);
    virtual odlRemoveComponent *asRemoveComponent() {return this;}
    Status postPerform(Database *, Schema *);
    Status prePerform(Database *, Schema *);
  };

  class odlUpdateRelationship : public odlUpdateItem {

  public:
    const Attribute *item, *invitem;

    odlUpdateRelationship(const Attribute *_item,
			  const Attribute *_invitem) :
      item(_item), invitem(_invitem) { }

    virtual odlUpdateRelationship *asUpdateRelationship() {return this;}
    virtual void display();
    virtual void displayDiff(Database *db, const char *odlfile);
  };

  class odlAddRelationship : public odlUpdateRelationship {

  public:
    odlAddRelationship(const Attribute *_item,
		       const Attribute *_invitem) :
      odlUpdateRelationship(_item, _invitem) { }

    virtual odlAddRelationship *asAddRelationship() {return this;}
    Status postPerform(Database *, Schema *);
  };

  class odlRemoveRelationship : public odlUpdateRelationship {

  public:
    odlRemoveRelationship(const Attribute *_item,
			  const Attribute *_invitem) :
      odlUpdateRelationship(_item, _invitem) { }

    virtual odlRemoveRelationship *asRemoveRelationship() {return this;}
    Status postPerform(Database *, Schema *);
  };

  class odlUpdateAttribute : public odlUpdateItem {

    Status check();
    Status check(Database *, const Class *);
  public:
    const Class *cls;
    const Attribute *item;
    Status initClassConv(Database *);

    odlUpdateAttribute(const Class *_cls, const Attribute *_item) :
      cls(_cls), item(_item) { }

    virtual Status postPerform(Database *, Schema *m);

    virtual void display();
    virtual void displayDiff(Database *db, const char *odlfile);

    virtual odlUpdateAttribute *asUpdateAttribute() {return this;}

  protected:
    Status invalidateCollClassOid(Database *, const Class *);
    Status invalidateInverseOid(Database *, const Class *);
    Status reportExtentOid(Database *db, const Class *);
  };

  class odlAddAttribute : public odlUpdateAttribute {

  public:
    odlAddAttribute(const Class *_cls, const Attribute *_item) :
      odlUpdateAttribute(_cls, _item) { }
    virtual odlAddAttribute *asAddAttribute() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlRemoveAttribute : public odlUpdateAttribute {

  public:
    odlRemoveAttribute(const Class *_cls, const Attribute *_item) :
      odlUpdateAttribute(_cls, _item) { }
    virtual odlRemoveAttribute *asRemoveAttribute() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlRenameAttribute : public odlUpdateAttribute {

  public:
    odlUpdateHint *upd_hints;
    odlRenameAttribute(const Class *_cls, const Attribute *_item,
		       odlUpdateHint *_upd_hints) :
      odlUpdateAttribute(_cls, _item), upd_hints(_upd_hints) { }
    virtual odlRenameAttribute *asRenameAttribute() {return this;}
    virtual void display();
    virtual void displayDiff(Database *db, const char *odlfile);
    Status prePerform(Database *, Schema *);
  };

  class odlConvertAttribute : public odlUpdateAttribute {

  public:
    odlUpdateHint *upd_hints;
    const Attribute *oitem;
    odlConvertAttribute(const Class *_cls, const Attribute *_oitem,
			const Attribute *_item, odlUpdateHint *_upd_hints) :
      odlUpdateAttribute(_cls, _item), oitem(_oitem), upd_hints(_upd_hints) {
    }
    virtual odlConvertAttribute *asConvertAttribute() {return this;}
    virtual void display();
    virtual void displayDiff(Database *db, const char *odlfile);
    Status prePerform(Database *, Schema *);
    Status prePerformBasic(Schema *, const Class *ncls,
			   const Class *ocls);
  };

  class odlReorderAttribute : public odlUpdateAttribute {

  public:
    int oldnum, newnum;
    odlReorderAttribute(const Class *_cls, const Attribute *_item,
			int _oldnum, int _newnum) :
      odlUpdateAttribute(_cls, _item), oldnum(_oldnum), newnum(_newnum) { }
    void display();
    void displayDiff(Database *db, const char *odlfile);
    virtual odlReorderAttribute *asReorderAttribute() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlMigrateAttribute : public odlUpdateAttribute {

  public:
    odlUpdateHint *upd_hints;
    odlMigrateAttribute(const Class *_cls, const Attribute *_item,
			odlUpdateHint *_upd_hints) :
      odlUpdateAttribute(_cls, _item), upd_hints(_upd_hints) { }
    virtual odlMigrateAttribute *asMigrateAttribute() {return this;}
    void display();
    void displayDiff(Database *db, const char *odlfile);
    Status prePerform(Database *, Schema *);
  };


  class odlUpdateClass : public odlUpdateItem {

  public:
    const Class *cls;
    odlUpdateClass(const Class *_cls) : cls(_cls) { }
    virtual Status postPerform(Database *, Schema *m);
    void display();
    void displayDiff(Database *db, const char *odlfile);
  };

  class odlAddClass : public odlUpdateClass {

  public:
    odlAddClass(const Class *_cls) : odlUpdateClass(_cls) { }
    virtual odlAddClass *asAddClass() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlRemoveClass : public odlUpdateClass {

    LinkedList *list;
  public:
    odlRemoveClass(Database *db, const Class *_cls, LinkedList *list);
    virtual odlRemoveClass *asRemoveClass() {return this;}
    Status prePerform(Database *, Schema *);
    Status postPerform(Database *, Schema *);
  };

  class odlRenameClass : public odlUpdateClass {

  public:
    char *name;
    odlRenameClass(const Class *_cls, const char *_name) :
      odlUpdateClass(_cls), name(strdup(_name)) { }
    virtual odlRenameClass *asRenameClass() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlReparentClass : public odlUpdateClass {

  public:
    odlReparentClass(const Class *_cls) : odlUpdateClass(_cls) { }
    virtual odlReparentClass *asReparentClass() {return this;}
    Status prePerform(Database *, Schema *);
  };

  class odlConvertClass : public odlUpdateClass {

  public:
    odlConvertClass(const Class *_cls) : odlUpdateClass(_cls) { }
    virtual odlConvertClass *asConvertClass() {return this;}
    Status prePerform(Database *, Schema *);
  };


  //
  // procedural corner
  //

  extern int
  odl_generate_code(Database *, Schema *, ProgLang, LinkedList *,
		    const char *package,
		    const char *schname,  const char *prefix,
		    const char *db_prefix, const GenCodeHints &);
  extern int
  odl_realize(Database *, Schema *m, LinkedList *list,
	      const char *prefix = "", const char *db_prefix = "",
	      const char *package = "",
	      Bool diff = False);

  extern int
  odl_generate(Schema *m, const char *ofile);

  extern void
  odl_prompt_init(FILE *);

  extern void
  odl_prompt(const char *prompt = "> ");

  extern void
  odl_skip_volatiles(Database *db, Schema *m);

  extern LinkedList qseq_list;
  extern ProgLang odl_lang;
  extern int odl_diff;
  extern void odl_add_error(Status s);
  extern void odl_add_error(const char *fmt, ...);
  extern void odl_add_error(const std::string &s);
  extern Status odl_post_update(Database *db);
  extern Bool odl_dynamic_attr;
  extern Bool odl_system_class;
  extern Bool odl_rmv_undef_attrcomp;
  extern Bool odl_update_index;
  extern Bool odl_sch_rm;
  extern LinkedList odl_cls_rm;
  extern void odl_remove_component(Schema *m, ClassComponent *comp);
  extern void odl_add_component(Schema *m, ClassComponent *comp);

  extern char odlMTHLIST[];
  extern char odlGENCOMP[];
  extern char odlGENCODE[];
}

extern FILE *odlin;

#endif
