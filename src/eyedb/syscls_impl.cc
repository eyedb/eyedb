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


//#define FRONT_END
#include "eyedb_p.h"
#include "oql_p.h"
#include <assert.h>
#include <eyedblib/butils.h>
#include <eyedb/oqlctb.h>


namespace eyedb {

  extern char *attrcomp_delete_ud;

  //
  // etcMake
  //

  void
  sysclsMake(Object **o)
  {
    Object *etc_o = sysclsMakeObject(*o);
    if (etc_o)
      *o = etc_o;
  }

  //
  // oqlctbMake
  //

  void
  oqlctbMake(Object **o)
  {
    Object *oqlctb_o = oqlctbMakeObject(*o);
    if (oqlctb_o)
      *o = oqlctb_o;
  }

  //
  // utilsMake
  //

  void
  utilsMake(Object **o)
  {
    Object *utils_o = utilsMakeObject(*o);
    if (utils_o)
      *o = utils_o;
  }

  //
  // ClassComponent
  // 

  static Status
  comp_exists(ClassComponent *comp, Bool &exists, Oid *comp_oid = 0)
  {
    OQL oql(comp->getDatabase(), "select class_component.name = \"%s\"",
	    comp->getName().c_str());
    OidArray oid_arr;
    Status status = oql.execute(oid_arr);
    if (status) return status;
    exists = IDBBOOL(oid_arr.getCount());
    if (exists && comp_oid)
      *comp_oid = oid_arr[0];
    return Success;
  }

  Status ClassComponent::realize(const RecMode *rcm)
  {
    Status status;
      
    if (!oid.isValid())
      {
	Class* cl = getClassOwner();

	if (!cl) // a 'not null' constraint should be better
	  return Exception::make(IDB_ERROR, "cannot create class component: attribute `class_owner' is not set");

	if (!db)
	  return Exception::make(IDB_ERROR, "no database associated with object");

	if (status = check(cl))
	  return status;
      
#if 0
	Bool exists;
	if (status = comp_exists(this, exists))
	  return status;

	if (exists)
	  return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,
				 "component '%s' already exists "
				 "in database", getName());
#endif

	if (status = Agregat::realize(rcm))
	  return status;
      
	if (status = cl->add(this, False)) {
	  db->removeObject(&getOid());
	  return status;
	}

	return Success;
      }

    if (status = Agregat::realize(rcm))
      return status;

    // must update
    return Success;
  }

  Status ClassComponent::make(Class *)
  {
    if (isRemoved())
      return Exception::make(IDB_ERROR, "index '%s' is removed",
			     oid.toString());
    return Success;
  }

  Status
  ClassComponent_realize_prologue(Database *db, const Class *&cl)
  {
    if (!cl->isRemoved())
      return Success;

    ClassConversion::Context *conv_ctx = 0;
    ClassConversion::Context *cnvctx;
    Status s = ClassConversion::getClass_(db, cl->getOid(), cl, conv_ctx);
    if (s) return s;
    if (cl->isRemoved())
      return Exception::make(IDB_ERROR,
			     "internal error in class component remove for update: "
			     "class %s is removed",
			     cl->getOid().toString());
    return Success;
  }

  Status ClassComponent::remove(const RecMode *rcm)
  {
    if (isRemoved())
      return Exception::make(IDB_ERROR,
			     "class component %s is removed",
			     getOid().toString());

    Class * cl = getClassOwner();
    Status s;

    //printf("ClassComponent::remove(%p, %s, %s)\n", this, oid.toString(), cl->getName());

    incrRefCount();

    s = ClassComponent_realize_prologue(db, (const Class *&)cl);
    if (s) return s;

    s = cl->suppress(this);
    if (s) return s;

    s = Agregat::remove(rcm);

    if (s)
      s = cl->add(this, False);
      
    return s;
  }

  Status ClassComponent::check(Class *cl) const
  {
    return Success;
  }

  int ClassComponent::getInd() const
  {
    return 0;
  }

  Bool ClassComponent::isInherit() const
  {
    return False;
  }

  Status ClassComponent::m_trace(FILE *fd, int, unsigned int, const RecMode*) const
  {
    return Success;
  }

  Status ClassComponent::realize_for_update()
  {
    return realize();
  }

  Status ClassComponent::remove_for_update()
  {
    return remove();
  }

  // added the 5/3/00
  // BUG_ETC_UPDATE
  static inline const char *get_class_name(const Class *cls)
  {
    return cls->getAliasName() ? cls->getAliasName() : cls->getName();
  }

  //
  // ClassVariable
  // 

  Status ClassVariable::check(Class *cl) const
  {
    if (!getVname()[0])
      return Exception::make(IDB_ERROR, "variable name must be set");

    return Success;
  }

  int ClassVariable::getInd() const
  {
    return Class::Variable_C;
  }

  Bool ClassVariable::isInherit() const
  {
    return True;
  }

  Status ClassVariable::m_trace(FILE *fd, int indent, unsigned int flags, const RecMode *rcm) const
  {
    Status s = Success;
    char *indent_str = make_indent(indent);
    const Object *v = getVal();
    if (v)
      fprintf(fd, "variable %s *%s::%s = ",
	      v->getClass()->getName(),
	      getClassOwner()->getName(), getVname().c_str());
    else
      fprintf(fd, "variable %s::%s = ",
	      getClassOwner()->getName(), getVname().c_str());

    Bool tr = False;
    if (v)
      {
	if (rcm->getType() == RecMode_FullRecurs)
	  {
	    fprintf(fd, "%s {%s} = ", v->getOid().getString(), v->getClass()->getName());
	    s = ObjectPeer::trace_realize(v, fd, indent + INDENT_INC, flags, rcm);
	    tr = True;
	  }
	else
	  fprintf(fd, "{%s}", v->getOid().getString());
      }
    else
      fprintf(fd, NullString);

    fprintf(fd, "%s", (tr ? indent_str : ""));
    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, " {%s}", oid.getString());
    delete_indent(indent_str);
    return s;
  }

  //
  // Protection
  //

  Status Protection::realize(const RecMode *rcm)
  {
    return Agregat::realize(rcm);
  }

  Status Protection::remove(const RecMode *rcm)
  {
    return Agregat::remove(rcm);
  }

  //
  // Trigger
  //

  const char *
  getTriggerName(TriggerType ttype)
  {
    if (ttype == TriggerCREATE_BEFORE)
      return "create_before";
    if (ttype == TriggerCREATE_AFTER)
      return "create_after";
    if (ttype == TriggerUPDATE_BEFORE)
      return "update_before";
    if (ttype == TriggerUPDATE_AFTER)
      return "update_after";
    if (ttype == TriggerLOAD_BEFORE)
      return "load_before";
    if (ttype == TriggerLOAD_AFTER)
      return "load_after";
    if (ttype == TriggerREMOVE_BEFORE)
      return "remove_before";
    if (ttype == TriggerREMOVE_AFTER)
      return "remove_after";

    return 0;
  }

  int Trigger::getInd() const
  {
    TriggerType _type = getType();

    if (_type == TriggerCREATE_BEFORE) 
      return Class::TrigCreateBefore_C;
    if (_type == TriggerCREATE_AFTER)  
      return Class::TrigCreateAfter_C;
    if (_type == TriggerUPDATE_BEFORE) 
      return Class::TrigUpdateBefore_C;
    if (_type == TriggerUPDATE_AFTER)  
      return Class::TrigUpdateAfter_C;
    if (_type == TriggerLOAD_BEFORE)   
      return Class::TrigLoadBefore_C;
    if (_type == TriggerLOAD_AFTER)    
      return Class::TrigLoadAfter_C;
    if (_type == TriggerREMOVE_BEFORE) 
      return Class::TrigRemoveBefore_C;
    if (_type == TriggerREMOVE_AFTER)  
      return Class::TrigRemoveAfter_C;
    abort();
  }

  Trigger::Trigger(Database *_db, Class *class_owner,
		   TriggerType _type,
		   ExecutableLang lang,
		   Bool isSystem,
		   const char *suffix,
		   Bool light,
		   const char *extref) :
    AgregatClassExecutable(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    Executable *ex = getEx();
    ex->initExec(suffix, lang, isSystem, BACKEND, 0, class_owner);
    setClassOwner(class_owner);
    setType(_type);
    setSuffix(suffix);
    setName(getCSym());
    setLight(light);
    ex->setExtrefBody(extref);
  }

  Status Trigger::apply(const Oid &_oid, Object *o)
  {
    if (o->isApplyingTrigger())
      return Success;

    if (getEx()->getLang() == OQL_LANG)
      {
	if (!isRTInitialized)
	  return Exception::make(IDB_EXECUTABLE_ERROR,
				 "cannot apply OQL 'trigger<%s> %s::%s'",
				 getTriggerName(getType()),
				 getClassOwner()->getName(), getName().c_str());

	o->setApplyingTrigger(True);
	oqmlStatus *s = oqmlMethodCall::applyTrigger(db, this, o, &_oid);
	o->setApplyingTrigger(False);

	if (s)
	  return Exception::make("applying OQL 'trigger<%s> %s::%s', got: %s",
				 getTriggerName(getType()),
				 getClassOwner()->getName(), getName().c_str(),
				 s->msg);
	return Success;
      }

    if (!csym)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "cannot apply C++ 'trigger<%s> %s::%s: "
			     "runtime pointer function is null",
			     getTriggerName(getType()),
			     getClassOwner()->getName(), getName().c_str());

    o->setApplyingTrigger(True);
    Status s = csym(getType(), db, _oid, o);
    o->setApplyingTrigger(False);

    return s;
  }

  const char *Trigger::getCSym() const
  {
    static char str[256];
    if (db)
      db->transactionBegin();

    if (!getClassOwner())
      return "";

    sprintf(str, "%s_%s_%s",
	    (getClassOwner()->getAliasName() ? getClassOwner()->getAliasName() : 
	     getClassOwner()->getName()),
	    getStrTriggerType(getType()),
	    getSuffix().c_str());

    if (db)
      db->transactionCommit();

    return str;
  }

  const char *Trigger::getStrTriggerType(TriggerType _type)
  {
    if (_type == TriggerCREATE_BEFORE) 
      return "create_before";
    if (_type == TriggerCREATE_AFTER)  
      return "create_after";
    if (_type == TriggerUPDATE_BEFORE) 
      return "update_before";
    if (_type == TriggerUPDATE_AFTER)  
      return "update_after";
    if (_type == TriggerLOAD_BEFORE)   
      return "load_before";
    if (_type == TriggerLOAD_AFTER)    
      return "load_after";
    if (_type == TriggerREMOVE_BEFORE) 
      return "remove_before";
    if (_type == TriggerREMOVE_AFTER)  
      return "remove_after";
    abort();
  }

  const char *
  Trigger::getPrototype(Bool scope) const
  {
    static char s[256];
    sprintf(s, "%strigger<%s> %s::%s()",
	    (getLight() ? "light" : ""),
	    getStrTriggerType(getType()),
	    getClassOwner()->getName(), getSuffix().c_str());
    return s;
  }

  Bool Trigger::isInherit() const
  {
    return True;
  }

  void
  print_oqlexec(FILE *fd, const char *body)
  {
    int len = strlen(body);
    char *x = strdup(body);
    char c = body[len-1];
    x[len-1] = 0;

    if (c == '}')
      fprintf(fd, " %%oql%s%%}", x);
    else
      {
	x[len-2] = 0;
	fprintf(fd, " %%oql%s%%)", x);
      }

    free(x);
  }

  Status Trigger::m_trace(FILE *fd, int indent, unsigned int flags,
			  const RecMode *rcm) const
  {
    Status s = Success;
    Bool istrs;
    if (db && !db->isInTransaction())
      {
	db->transactionBegin();
	istrs = True;
      }
    else
      istrs = False;    

    fprintf(fd, "%strigger<%s> ",
	    (getLight() ? "light" : ""),
	    getStrTriggerType(getType()));

    if (!(flags & NoScope))
      fprintf(fd, "%s::", getClassOwner()->getName());

    fprintf(fd, "%s()", getSuffix().c_str());

    if (flags & ExecBodyTrace)
      {
	const Executable *ex = getEx();
	if (ex->getLang() & C_LANG)
	  fprintf(fd, " C++(\"%s\")", ex->getExtrefBody().c_str());
	else
	  {
	    ((Trigger *)this)->runtimeInit();
	    if (body)
	      print_oqlexec(fd, body);
	  }
      }

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
      }

    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, " {%s}", oid.getString());

    if (istrs)
      db->transactionCommit();
    return s;
  }

  Status Trigger::realize(const RecMode *rcm)
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");

    if (!getSuffix().c_str() || !*getSuffix().c_str())
      return Exception::make(IDB_ERROR, "cannot realize unamed trigger");

    if (!oid.isValid())
      {
	OQL q(db, "select trigger.name = \"%s\"", getCSym());

	ObjectArray obj_arr(true);
	Status s = q.execute(obj_arr);
	if (s) return s;
      
	if (obj_arr.getCount()) {
	  //obj_arr[0]->release();
	  return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,
				 "trigger<%s> %s::%s() already exists "
				 "in database '%s'",
				 Trigger::getStrTriggerType(getType()),
				 getClassOwner()->getName(),
				 getName().c_str(),
				 db->getName());
	}
      }

    return ClassComponent::realize(rcm);
  }

  Status Trigger::remove(const RecMode *rcm)
  {
    return ClassComponent::remove(rcm);
  }

  std::string
  Trigger::makeExtrefBody(const Class *cls, const char *oql,
			  const char *name,
			  std::string &oqlConstruct)
  {
    std::string funcname = std::string("oql$") + cls->getAliasName() + "$" +
      name;

    std::string s = funcname;
    s += ":";

    oqlConstruct = std::string("function ") + funcname + "()" + oql;

    return s + oql;
  }

  Status
  Trigger::runtimeInit()
  {
    if (isRTInitialized)
      return Success;

    std::string str = getEx()->getExtrefBody();
    const char *s = str.c_str();

    tmpbuf = strdup(s);
    char *q = strchr(tmpbuf, ':');

    if (!q)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "invalid internal format '%s'", s);
    *q = 0;
    funcname = tmpbuf;

    body = q+1;
    fullBody = strdup((std::string("function ") + funcname + "()" + body).c_str());
    isRTInitialized = True;
    return Success;
  }

  void
  Trigger::userInitialize()
  {
    isRTInitialized = False;
    body = 0;
    fullBody = 0;
    funcname = 0;
    tmpbuf = 0;
    entry = 0;
  }

  void
  Trigger::userCopy(const Object &o)
  {
    Trigger::userInitialize();
  }

  void
  Trigger::userGarbage()
  {
    free(tmpbuf);
    free(fullBody);
  }

  Status UnreadableObject::trace_realize(FILE*fd, int indent, unsigned int, const RecMode *) const
  {
    char *last_indent_str = make_indent(indent-INDENT_INC);
    char *indent_str = make_indent(indent);

    fprintf(fd, "{\n");
    fprintf(fd, "%s<unreadable object>\n", indent_str);
    fprintf(fd, "%s};\n", last_indent_str);

    delete_indent(last_indent_str);
    delete_indent(indent_str);
    return Success;
  }

  Bool UnreadableObject::isUnreadableObject(const Class *cls)
  {
    if (cls && !strcmp(cls->getName(), "unreadable_object"))
      return True;
    return False;
  }

  Bool UnreadableObject::isUnreadableObject(const Object *o)
  {
    return isUnreadableObject(o->getClass());
  }

  //
  // CardinalityConstraint
  //

  int CardinalityConstraint::maxint = -1;

  CardinalityConstraint::CardinalityConstraint(Database *_db,
					       Class *class_owner,
					       const char *atname,
					       int bottom, int bottom_excl,
					       int top, int top_excl)
    : AgregatClassComponent(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(class_owner);
    setAttrname(atname);
    CardinalityDescription *card = getCardDesc();
    card->setBottom(bottom);
    card->setBottomExcl(bottom_excl);
    card->setTop(top);
    card->setTopExcl(top_excl);
    setName(genName());
  }

  const char *CardinalityConstraint::genName() const
  {
    static char str[256];
    const CardinalityDescription *card = getCardDesc();
    sprintf(str, "card_%s::%s%s%d,%d%s", 
	    get_class_name(getClassOwner()),
	    getAttrname().c_str(),
	    (card->getBottomExcl() ? "]" : "["),
	    card->getBottom(),
	    card->getTop(),
	    (card->getTopExcl() ? "[" : "]"));
    return str;
  }

  Status CardinalityConstraint::make(Class *cl)
  {
    Attribute *item;

    if (!(item = (Attribute *)cl->getAttribute(getAttrname().c_str())))
      return Exception::make(IDB_ERROR, "cardinality constraint: attribute '%s'"
			     " does not exist in class '%s'",
			     getAttrname().c_str(), cl->getName());

    // disconnected the 15/02/99
#if 0
    if (item->getCardinalityConstraint() &&
	!item->getCardinalityConstraint()->getOid().compare(getOid()))
      return Exception::make(IDB_ERROR,
			     "a cardinality constraint already exists for attribute '%s::%s'",
			     cl->getName(), getAttrname());
#endif

    return item->setCardinalityConstraint(this);
  }

  Status CardinalityConstraint::check(Class *cl) const
  {
    std::string atname_s = getAttrname();
    const char *atname = atname_s.c_str();
    if (!atname || !*atname)
      return Exception::make(IDB_ERROR, "attribute name is not set for"
			     " cardinality constraint in class '%s'",
			     cl->getName());
    const Attribute *item;
    if (!(item = cl->getAttribute(atname)))
      return Exception::make(IDB_ERROR, "cardinality constraint: attribute '%s'"
			     " does not exist in class '%s'",
			     atname, cl->getName());

    return Success;
  }

  int CardinalityConstraint::getInd() const
  {
    return 0;
  }

  Bool CardinalityConstraint::isInherit() const
  {
    return True;
  }

  Status CardinalityConstraint::m_trace(FILE *fd, int indent, unsigned int flags, const RecMode* rcm) const
  {
    Status s = Success;
    char *indent_str = make_indent(indent);
    Bool tr = False;

    if (db)
      db->transactionBegin();
    fprintf(fd, "card(");
    if (rcm->getType() == RecMode_FullRecurs)
      {
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
	tr = True;
      }
    else
      fprintf(fd, "%s::%s", getClassOwner()->getName(), getAttrname().c_str());
  
    if (db)
      db->transactionCommit();

    const CardinalityDescription *card = getCardDesc();
    fprintf(fd, "%s", (tr ? indent_str : ""));

    fprintf(fd, " %s)", card->getString(False));

    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, " {%s}", oid.getString());

    delete_indent(indent_str);
    return s;
  }

  const char *CardinalityDescription::getString(Bool isprefix) const
  {
    static char card_str[256];

    int bottom = getBottom();
    int top = getTop();
    int bottom_excl = getBottomExcl();
    int top_excl = getTopExcl();

    int maxint = CardinalityConstraint::maxint;

    const char *prefix;

    if (isprefix)
      prefix = "card ";
    else
      prefix = "";

    if (bottom == top)
      sprintf(card_str, "%s= %d", prefix, bottom);
    else if (bottom && top != maxint)
      sprintf(card_str, "%sin %s%d,%d%s", prefix,
	      bottom_excl ? "]" : "[",
	      bottom,
	      top,
	      top_excl ? "[" : "]");
    else if (top == maxint)
      sprintf(card_str, "%s>%s %d", prefix, bottom_excl ? "" : "=", bottom);
    else if (!bottom)
      sprintf(card_str, "%s<%s %d", prefix, top_excl ? "" : "=", top);

    return card_str;
  }

  Bool
  CardinalityDescription::compare(CardinalityDescription *card)
  {
    if (!card)
      return False;

    return getBottom() == card->getBottom() &&
      getBottomExcl() == card->getBottomExcl() &&
      getTop() == card->getTop() &&
      getTopExcl() == card->getTopExcl() ? True : False;
  }

  Status 
  post_etc_update(Database *db)
  {
    return Success;
  }

  //
  // AttributeComponent
  //

  Status
  AttributeComponent::m_trace(FILE *, int, unsigned int, const RecMode *) const
  {
    return 0;
  }

  Status
  AttributeComponent::checkUnique(const char *clsname, const char *msg)
  {
    OQL oql(db, "select %s.attrpath = \"%s\"", clsname, getAttrpath().c_str());
    OidArray oid_arr;
    Status s = oql.execute(oid_arr);  
    if (s) return s;
    if (oid_arr.getCount())
      return Exception::make(IDB_ERROR, "%s '%s' already "
			     "exist", msg, getAttrpath().c_str());
    return Success;
  }

  std::string
  AttributeComponent::makeAttrpath(const Class *cls)
  {
    std::string attrpath = getAttrpath();
    const char *p = strchr(attrpath.c_str(), '.');
    assert(p);
    return std::string(cls->getName()) + "." + std::string(p+1);
  }

  Status
  AttributeComponent::find(Database *db, const Class *cls,
			   AttributeComponent *&cattr_comp)
  {
    std::string attrpath = makeAttrpath(cls);
    char *name = strdup(getName().c_str());
    char *p = strchr(name, ':');
    assert(p);
    *p = 0;
    std::string newname = std::string(name) + ":" + attrpath;
    free(name);
    cattr_comp = 0;
    Status s = cls->getAttrComp(newname.c_str(), cattr_comp);
    if (s) return s;
    return Success;
  }

  AttributeComponent *
  AttributeComponent::xclone(Database *, const Class *)
  {
    abort();
    return 0;
  }

  //
  // NotNullConstraint
  //

  NotNullConstraint::NotNullConstraint(Database *_db, Class *cls,
				       const char *attrpath,
				       Bool propagate) :
    AttributeComponent(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(attrpath);
    setName(genName());
    setPropagate(propagate);
  }

  static const char *
  getPropagString(const AttributeComponent *comp)
  {
    static std::string s;
    s = ", propagate = ";
    s += comp->getPropagate() ? "on" : "off";
    return s.c_str();
  }

  Status
  NotNullConstraint::m_trace(FILE *fd, int indent, unsigned int flags,
			     const RecMode *rcm) const
  {
    Status s = Success;
    char *indent_str = make_indent(indent);
    Bool tr = False;
  
    fprintf(fd, "constraint<notnull%s> on %s", getPropagString(this), getAttrpath().c_str());

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
	tr = True;
      }
      
    fprintf(fd, "%s", (tr ? indent_str : ""));
    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, " {%s}", oid.getString());
    delete_indent(indent_str);
    return s;
  }

  const char *
  NotNullConstraint::genName() const
  {
    static std::string s;
    s = std::string("notnull") + ":" + getAttrpath();
    return s.c_str();
  }

  Status
  NotNullConstraint::realize(const RecMode *rcm)
  {
    Status s;

    Bool creating = IDBBOOL(!getOid().isValid());
    if (creating) {
      s = checkUnique("notnull_constraint", "notnull constraint");
      if (s) return s;
    }

    s = AttributeComponent::realize(rcm);
    if (s) return s;
    if (creating)
      return StatusMake(constraintCreate(db->getDbHandle(),
					     oid.getOid()));
    return Success;
  }

  Status
  NotNullConstraint::remove(const RecMode *rcm)
  {
    RPCStatus rpc_status =
      constraintDelete(db->getDbHandle(), oid.getOid(),
			   getUserData(attrcomp_delete_ud) ? 1 : 0);

    if (rpc_status)
      return StatusMake(rpc_status);
    return AttributeComponent::remove(rcm);
  }

  int
  NotNullConstraint::getInd() const
  {
    return Class::NotnullConstraint_C;
  }

  AttributeComponent *
  NotNullConstraint::xclone(Database *db, const Class *cls)
  {
    std::string str = makeAttrpath(cls);
    return new NotNullConstraint(db, (Class *)cls, str.c_str(),
				 getPropagate());
  }

  //
  // UniqueConstraint
  //

  UniqueConstraint::UniqueConstraint(Database *_db, Class *cls,
				     const char *attrpath,
				     Bool propagate) :
    AttributeComponent(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(attrpath);
    setName(genName());
    setPropagate(propagate);
  }

  Status
  UniqueConstraint::m_trace(FILE *fd, int indent, unsigned int flags, const RecMode* rcm) const
  {
    Status s = Success;
    char *indent_str = make_indent(indent);
    Bool tr = False;

    if (db)
      db->transactionBegin();
    fprintf(fd, "constraint<unique%s> on %s", getPropagString(this), getAttrpath().c_str());
    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
	tr = True;
      }
  
    if (db)
      db->transactionCommit();

    fprintf(fd, "%s", (tr ? indent_str : ""));
    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, "{%s}", oid.getString());
    delete_indent(indent_str);
    return s;
  }

  const char *
  UniqueConstraint::genName() const
  {
    static std::string s;
    s = std::string("unique") + ":" + getAttrpath();
    return s.c_str();
  }

  Status
  UniqueConstraint::realize(const RecMode *rcm)
  {
    Status s;

    Bool creating = IDBBOOL(!getOid().isValid());
    if (creating) {
      s = checkUnique("unique_constraint", "unique constraint");
      if (s) return s;
    }

    s = AttributeComponent::realize(rcm);
    if (s) return s;
    if (creating)
      return StatusMake(constraintCreate(db->getDbHandle(),
					     oid.getOid()));
    return Success;
  }

  Status
  UniqueConstraint::remove(const RecMode *rcm)
  {
    RPCStatus rpc_status =
      constraintDelete(db->getDbHandle(), oid.getOid(),
			   getUserData(attrcomp_delete_ud) ? 1 : 0);

    if (rpc_status)
      return StatusMake(rpc_status);
    return AttributeComponent::remove(rcm);
  }

  AttributeComponent *
  UniqueConstraint::xclone(Database *db, const Class *cls)
  {
    std::string str = makeAttrpath(cls);
    return new UniqueConstraint(db, (Class *)cls, str.c_str(),
				getPropagate());
  }

  int
  UniqueConstraint::getInd() const
  {
    return Class::UniqueConstraint_C;
  }

  // ---------------------------------------------------------------------------
  //
  // Classes for component propagation tests
  //
  // _Test suffix will disapear
  //
  // ---------------------------------------------------------------------------


  CardinalityConstraint_Test::CardinalityConstraint_Test
  (Database *, Class *, const char *attrname, int, int, int, int)
  {
  }

  const char *
  CardinalityConstraint_Test::genName() const
  {
    return 0;
  }

  Status CardinalityConstraint_Test::m_trace(FILE *, int, unsigned int, const RecMode *) const
  {
    return 0;
  }

  static int maxint;


  Index::Index(Database *, const char *, const char *)
  {
  }

  Status Index::realize(const RecMode*)
  {
    return 0;
  }

  Status
  Index::report(eyedbsm::DbHandle *sedbh, const Oid &idxoid)
  {
    abort();
    return Success;
  }

  Status Index::remove(const RecMode*)
  {
    return 0;
  }

  Status Index::s_trace(FILE *, Bool, unsigned int flags) const
  {
    return 0;
  }

  void Index::userInitialize()
  {
    idx = 0;
  }

  void Index::userCopy(const Object &)
  {
    idx = 0;
  }

  Status
  Index::makeDataspace(Database *_db, const Dataspace *&dataspace) const
  {
    Bool isnull;
    short dspid = getDspid(&isnull);
    if (!isnull)
      return _db->getDataspace(dspid, dataspace);
    dataspace = 0;
    return Success;
  }

  Status 
  Index::getCount(unsigned int &count)
  {
    RPCStatus rpc_status =
      indexGetCount(db->getDbHandle(), oid.getOid(), (int *)&count);

    return StatusMake(rpc_status);
  }

  Status
  Index::getStats(IndexStats *&stats)
  {
    RPCStatus rpc_status =
      indexGetStats(db->getDbHandle(), oid.getOid(), (Data *)&stats);
    if (rpc_status)
      return StatusMake(rpc_status);

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;
    if (dataspace)
      stats->idximpl->setDataspace(dataspace);

    if (asHashIndex() && asHashIndex()->getHashMethod())
      stats->idximpl->setHashMethod(asHashIndex()->getHashMethod());
    return Success;
  }

  Status
  Index::getStats(std::string &xstats, Bool dspImpl, Bool full,
		  const char *indent)
  {
    IndexStats *stats = 0;
    RPCStatus rpc_status =
      indexGetStats(db->getDbHandle(), oid.getOid(), (Data *)&stats);
    if (rpc_status)
      return StatusMake(rpc_status);
    xstats = (stats ? stats->toString(dspImpl, full, indent) : std::string(""));
    delete stats;
    return Success;
  }

  Status
  Index::simulate(const IndexImpl &idximpl, std::string &xstats,
		  Bool dspImpl, Bool full, const char *indent)
  {
    IndexStats *stats = 0;
    Status s = simulate(idximpl, stats);
    if (s) return s;
    xstats = (stats ? stats->toString(dspImpl, full, indent) : std::string(""));
    return Success;
  }

  Status
  Index::simulate(const IndexImpl &idximpl, IndexStats *&stats)
  {
    Data data;
    Offset offset = 0;
    Size size = 0;
    Status s = IndexImpl::code(data, offset, size, &idximpl);
    if (s) return s;

    RPCStatus rpc_status =
      indexSimulStats(db->getDbHandle(), oid.getOid(), data, size, (Data *)&stats);
    free(data);
    return StatusMake(rpc_status);
  }

  void Index::userGarbage()
  {
    //  printf("Index::userGarbage(%p)\n", this);
  }

  void AttributeComponent::userInitialize()
  {
    //printf("AttributeComponent::userInitialize(%p)\n", this);
  }

  void AttributeComponent::userCopy(const Object &)
  {
    //  printf("AttributeComponent::userCopy(%p)\n", this);
  }

  void AttributeComponent::userGarbage()
  {
    //  printf("AttributeComponent::userGarbage(%p)\n", this);
  }

  int
  Index::getInd() const
  {
    return Class::Index_C;
  }

  short
  Index::get_dspid() const
  {
    Bool isnull;
    short dspid = getDspid(&isnull);

    if (isnull)
      return Dataspace::DefaultDspid;

    return dspid;
  }

  inline int
  IDXTYPE(const Index *idx)
  {
    return idx->asHashIndex() ? 1 : 0;
  }

  Status
  Index::getDefaultDataspace(const Dataspace *&dataspace) const
  {
    short dspid = get_dspid();

    if (dspid == Dataspace::DefaultDspid) {
      dataspace = 0;
      return Success;
    }
  
    return db->getDataspace(dspid, dataspace);
  }

#define NEW_INDEX_MOVE

  static const char index_move[] = "eyedb:move";
  extern char index_backend[];

  Status
  Index::setDefaultDataspace(const Dataspace *dataspace)
  {
    Status s = setDspid(dataspace->getId());
    if (s) return s;

#ifdef NEW_INDEX_MOVE
    void *ud;
    if (!getUserData(index_move))
      ud = setUserData(index_backend, AnyUserData); // prevent from reimplementing index
    s = store();
    if (!getUserData(index_move))
      setUserData(index_backend, ud);
#else
    s = store();
#endif
    if (s) return s;

    Oid idxoid = getIdxOid();
    if (!idxoid.isValid()) return Success;

    // warning: the idx oid could have been changed during
    // the store process because of a reimplementation, so we must reload
    // the object and get its idxoid !
#ifdef NEW_INDEX_MOVE
    // not in this case 
#endif

    Index *idx;
    s = db->reloadObject(oid, (Object *&)idx);
    if (s) return s;
#ifdef NEW_INDEX_MOVE
    if (!getUserData(index_move))
      assert(idx->getIdxOid() == getIdxOid());
#endif
    idxoid = idx->getIdxOid();
    idx->release();

    RPCStatus rpc_status =
      setDefaultIndexDataspace(db->getDbHandle(), idxoid.getOid(),
				   IDXTYPE(this), dataspace->getId());

    return StatusMake(rpc_status);
  }

  Status
  Index::getObjectLocations(ObjectLocationArray &locarr)
  {
    const Oid &idxoid = getIdxOid();
    if (!idxoid.isValid()) return Success;

    RPCStatus rpc_status = getIndexLocations(db->getDbHandle(),
						 idxoid.getOid(),
						 IDXTYPE(this),
						 (void *)&locarr);
    return StatusMake(rpc_status);
  }

  Status
  Index::move(const Dataspace *dataspace) const
  {
#ifdef NEW_INDEX_MOVE
    Index *mthis = const_cast<Index *>(this);
    void *ud = mthis->setUserData(index_move, AnyUserData);
    Status s = mthis->setDefaultDataspace(dataspace);
    mthis->setUserData(index_move, ud);
    return s;
#else
    const Oid &idxoid = getIdxOid();
    if (!idxoid.isValid()) return Success;
    RPCStatus rpc_status = moveIndex(db->getDbHandle(), idxoid.getOid(),
					 IDXTYPE(this),
					 dataspace->getId());
    return StatusMake(rpc_status);
#endif
  }

  HashIndex::HashIndex(Database *_db, Class *cls,
		       const char *pathattr,
		       Bool propagate, Bool is_string,
		       const Dataspace *dataspace,
		       int key_count, BEMethod_C *mth,
		       const int *impl_hints, int impl_hints_cnt)
    : Index(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(pathattr);
    setKeyCount(key_count);
    setIsString((Bool)is_string);
    setPropagate(propagate);
    if (dataspace)
      setDspid(dataspace->getId());
    setHashMethod(mth);
    setName(genName());
    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);
  }

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

  HashIndex::HashIndex(Database *_db,
		       Class *cls, const char *attrpath,
		       Bool propagate, Bool is_string,
		       const IndexImpl *idximpl)
    : Index(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(attrpath);
    setKeyCount(idximpl->getKeycount());
    setIsString((Bool)is_string);
    setPropagate(propagate);
    setHashMethod(idximpl->getHashMethod());
    if (idximpl->getDataspace())
      setDspid(idximpl->getDataspace()->getId());
    setName(genName());

    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);

    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);
  }

  Status
  HashIndex::make(Database *db, Class *cls, const char *attrpath,
		  Bool propagate, Bool is_string,
		  const char *hints, HashIndex *&idx)
  {
    idx = 0;
    IndexImpl *idximpl;
    Status s = IndexImpl::make(db, IndexImpl::Hash, hints, idximpl,
			       is_string);
    if (s) return s;

    idx = new HashIndex(db, cls, attrpath, propagate, is_string,
			idximpl);
    return Success;
  }

  Status HashIndex::check(Class *) const
  {
    return 0;
  }

  Status HashIndex::make(Class *)
  {
    return 0;
  }

  //#define REIMPL_TRACE

  Bool
  Index::compareHints(Index *idx)
  {
    unsigned int cnt1 = getImplHintsCount();
    unsigned int cnt2 = idx->getImplHintsCount();
    int cnt = cnt1 < cnt2 ? cnt1 : cnt2;

#ifdef REIMPL_TRACE
    printf("compareHints: cnt %d vs %d\n", cnt1, cnt2);
#endif
    for (unsigned int i = 0; i < cnt; i++) {
#ifdef REIMPL_TRACE
      printf("hints[%d] %d %d\n", i, getImplHints(i), idx->getImplHints(i));
#endif
      int val = getImplHints(i);
      if (val && val != idx->getImplHints(i))
	return False;
      if (!val)
	setImplHints(i, idx->getImplHints(i));
    }
		    
    return True;
  }

  Status
  Index::reimplement(const IndexImpl &idximpl, Index *&idx)
  {
    IndexImpl::Type idxtype = idximpl.getType();

    if ((idxtype == IndexImpl::Hash && asHashIndex()) ||
	(idxtype == IndexImpl::BTree && asBTreeIndex())) {
      setImplementation(&idximpl);
      idx = this;
      return store();
    }

    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl.getImplHints(impl_hints_cnt);

    // getClassOwner() must be called before the new invocation for idx because
    // of an obscure problem (bug?) with the C++ compiler:
    // as getClassOwner() calls the new operator, gbx package "thought"
    // that the created idx is on the stack, because the new operator 
    // for idx is called before the call of the method getClassOwner() !

    Class *clsown = getClassOwner();
    if (idxtype == IndexImpl::Hash) {
      BEMethod_C *mth = idximpl.getHashMethod();
      idx = new HashIndex(db, clsown, getAttrpath().c_str(),
			  getPropagate(), getIsString(),
			  idximpl.getDataspace(),
			  idximpl.getKeycount(), mth,
			  impl_hints, impl_hints_cnt);
    }
    else
      idx = new BTreeIndex(db, clsown, getAttrpath().c_str(),
			   getPropagate(), getIsString(),
			   idximpl.getDataspace(),
			   idximpl.getDegree(),
			   impl_hints, impl_hints_cnt);

    Oid idxoid = getIdxOid();
    setIdxOid(Oid::nullOid);
    setPropagate(False);
    // to inform backend that it is a reimplementation
    setImplHints(IDB_IDX_MAGIC_HINTS, IDB_IDX_MAGIC_HINTS_VALUE); 
    Status s = store();

    if (!s)
      s = remove();

    if (s) {
      idx->release();
      idx = 0;
      return s;
    }

    idx->setIdxOid(idxoid);
    s = idx->store();
    if (s) {
      idx->release();
      idx = 0;
      return s;
    }
    return Success;
  }

  Bool
  HashIndex::compareHashMethod(HashIndex *idx)
  {
    BEMethod_C *mth1 = getHashMethod();
    BEMethod_C *mth2 = idx->getHashMethod();

    if ((!mth1 && mth2) || (mth1 && !mth2))
      return False;

    if (mth1 && mth1->getOid() != mth2->getOid())
      return False;

    return True;
  }

  Status HashIndex::realize(const RecMode*rcm)
  {
    Status s;

    Bool backend_updating = IDBBOOL(getUserData(index_backend));
    Bool creating = IDBBOOL(!backend_updating && !getOid().isValid());
    Bool updating = False;
    bool index_moving = (getUserData(index_move) != 0);

    /*
      printf("HashIndex::realize(%s, %s, key_count%d, hash_method=%p, backend_updating=%d)\n",
      getAttrpath(), getOid().toString(), getKeyCount(), getHashMethod(),
      backend_updating);
    */
    if (creating) {
#ifndef DUP_INDEX
      s = checkUnique("index", "index");
      if (s) return s;
#endif
    }
    else if (!backend_updating) {
      HashIndex *idx;
      s = db->reloadObject(getOid(), (Object *&)idx);
      if (s) return s;
      if ((getKeyCount() && idx->getKeyCount() != getKeyCount()) || 
	  idx->getDspid () != getDspid() ||
	  !compareHints(idx) ||
	  !compareHashMethod(idx)) {
#ifdef REIMPL_TRACE
	printf("Hashindex: should reimplement: "
	       "key_count: %d v.s %d "
	       "dsp_id %d: v.s %d "
	       "compareHints: %d "
	       "compareHashMethod: %d\n",
	       getKeyCount(), idx->getKeyCount(),
	       getDspid(), idx->getDspid(),
	       compareHints(idx),
	       compareHashMethod(idx));
#endif
	updating = True;
      }
#ifdef REIMPL_TRACE
      else
	printf("Hashindex: should NOT reimplement\n");
#endif
      if (!getKeyCount())
	setKeyCount(idx->getKeyCount());
      idx->release();
    }

    BEMethod_C *mth = getHashMethod();
    if (mth)
      setHashMethod(mth);
    else
      setHashMethodOid(Oid::nullOid);

    s = AttributeComponent::realize(rcm);

    if (s) return s;

    if (creating || updating)
      return StatusMake(indexCreate(db->getDbHandle(),
				    index_moving,
				    oid.getOid()));

    return Success;
  }

  Status HashIndex::report(eyedbsm::DbHandle *sedbh, const Oid &idxoid)
  {
    eyedbsm::HIdx hidx(sedbh, idxoid.getOid());
    if (hidx.status())
      return Exception::make(IDB_ERROR, eyedbsm::statusGet(hidx.status()));

    const eyedbsm::HIdx::_Idx &xidx = hidx.getIdx();
    setKeyCount(xidx.key_count);
    setImplHintsCount(eyedbsm::HIdxImplHintsCount);
    for (int n = 0; n < eyedbsm::HIdxImplHintsCount; n++)
      setImplHints(n, xidx.impl_hints[n]);
#ifdef REIMPL_TRACE
    printf("Hashindex::report\n");
#endif
    return Success;
  }

  Status HashIndex::remove(const RecMode*)
  {
    //printf("HashIndex:: -> index remove(%s)\n", oid.toString());
    RPCStatus rpc_status =
      indexRemove(db->getDbHandle(), oid.getOid(),
		      getUserData(attrcomp_delete_ud) ? 1 : 0);

    if (rpc_status)
      return StatusMake(rpc_status);

    //printf("HashIndex:: -> REMOVE(%s)\n", oid.toString());
    return AttributeComponent::remove();
  }

#define PREF(FD, VAR) \
    if (!VAR) { \
      fprintf(FD, ", hints = \""); \
      VAR = True; \
    } \
    else \
      fprintf(FD, " ")

  Status HashIndex::s_trace(FILE *fd, Bool is_string, unsigned int flags) const
  {
    if (!(flags & AttrCompDetailTrace)) {
      fprintf(fd, "index<hash> on %s", getAttrpath().c_str());
      return Success;
    }

    fprintf(fd, "index<type = hash");
    Bool hints = False;

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;
    if (dataspace) {
      PREF(fd, hints);
      fprintf(fd, "dataspace = %s;", dataspace->getName());
    }

    const BEMethod_C *mth = getHashMethod();
    if (mth) {
      PREF(fd, hints);
      fprintf(fd, "key_function = %s::%s;", mth->getClassOwner()->getName(),
	      mth->getEx()->getExname().c_str());
    }

    if (getKeyCount()) {
      PREF(fd, hints);
      fprintf(fd, "key_count = %d;", getKeyCount());
    }

    unsigned int cnt = getImplHintsCount();
    
#ifdef IDB_SKIP_HASH_XCOEF
    if (cnt > eyedbsm::HIdx::XCoef_Hints)
      cnt = eyedbsm::HIdx::XCoef_Hints;
#endif

    for (unsigned int i = 0; i < cnt; i++) {
      if (is_string) {
	if (i == eyedbsm::HIdx::IniObjCnt_Hints)
	  continue;
      }
      else if ((i) == eyedbsm::HIdx::IniSize_Hints &&
	       getImplHints(eyedbsm::HIdx::IniObjCnt_Hints))
	continue;
      int val = getImplHints(i);
      // if (val)
      if (IndexImpl::isHashHintImplemented(i)) {
	PREF(fd, hints);
	fprintf(fd, "%s = %d;", IndexImpl::hashHintToStr(i), val);
      }
    }

    fprintf(fd, "%s%s> on %s", (hints ? "\"" : ""), getPropagString(this),
	    getAttrpath().c_str());

    return Success;
  }

  Status HashIndex::m_trace(FILE *fd, int indent, unsigned int flags,
			    const RecMode *rcm) const
  {
    Status s = Success;
    s = s_trace(fd, IDBBOOL(flags & IDB_ATTR_IS_STRING), flags);
    if (s) return s;

    if ((flags & CompOidTrace) == CompOidTrace ||
	rcm->getType() == RecMode_FullRecurs)
      fprintf(fd, " {%s}", oid.getString());

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
      }
    return s;
  }

  AttributeComponent *
  HashIndex::xclone(Database *db, const Class *cls)
  {
    int impl_hints[eyedbsm::HIdxImplHintsCount];
    memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
    unsigned int impl_hints_cnt = getImplHintsCount();
    for (int i = 0; i < impl_hints_cnt; i++)
      impl_hints[i] = getImplHints(i);
    Bool isnull;

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) throw *s;

    std::string str = makeAttrpath(cls);
    return new HashIndex(db, (Class *)cls, str.c_str(),
			 getPropagate(), getIsString(),
			 dataspace, getKeyCount(), getHashMethod(),
			 impl_hints, impl_hints_cnt);
  }
  
  Status
  HashIndex::setImplementation(const IndexImpl *idximpl)
  {
    if (idximpl->getType() == IndexImpl::BTree)
      return Exception::make(IDB_ERROR,
			     "cannot change dynamically hash index "
			     "implementation to btree index using the "
			     "setImplementation() method: use the "
			     "reimplement() method");
    setKeyCount(idximpl->getKeycount());
    if (idximpl->getHashMethod())
      setHashMethod(idximpl->getHashMethod());
    else
      setHashMethodOid(Oid::nullOid);

    if (idximpl->getDataspace())
      setDspid(idximpl->getDataspace()->getId());

    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
    setImplHintsCount(impl_hints_cnt);
    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);

    return Success;
  }

  Status
  HashIndex::getImplementation(IndexImpl *&idximpl, Bool remote) const
  {
    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;

    if (!remote) {
      unsigned int cnt = getImplHintsCount();
      int *impl_hints = (cnt ? new int [cnt] : 0);
      for (unsigned int i = 0; i < cnt; i++)
	impl_hints[i] = getImplHints(i);
      idximpl =
	new IndexImpl(IndexImpl::Hash, dataspace, getKeyCount(),
		      (BEMethod_C *)getHashMethod(),
		      impl_hints, cnt);
      delete [] impl_hints;
      return Success;
    }
  
    RPCStatus rpc_status =
      indexGetImplementation(db->getDbHandle(),
				 oid.getOid(), (Data *)&idximpl);
    if (rpc_status)
      return StatusMake(rpc_status);

    idximpl->setHashMethod((BEMethod_C *)getHashMethod());
    idximpl->setDataspace(dataspace);
    return Success;
  }

  const char *HashIndex::genName() const
  {
    static std::string s;
    s = std::string("index") + ":" + getAttrpath();
    return s.c_str();
  }

  BTreeIndex::BTreeIndex(Database *_db, Class *cls,
			 const char *pathattr,
			 Bool propagate, Bool is_string,
			 const Dataspace *dataspace, int degree,
			 const int *impl_hints , int impl_hints_cnt) : Index(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(pathattr);
    setIsString((Bool)is_string);
    setPropagate(propagate);
    setName(genName());
    if (dataspace)
      setDspid(dataspace->getId());
    setDegree(degree);
  }

  BTreeIndex::BTreeIndex(Database *_db, Class *cls,
			 const char *pathattr,
			 Bool propagate, Bool is_string,
			 const IndexImpl *idximpl) :
    Index(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(pathattr);
    setIsString((Bool)is_string);
    setPropagate(propagate);
    setName(genName());
    setDegree(idximpl->getDegree());
    if (idximpl->getDataspace())
      setDspid(idximpl->getDataspace()->getId());

    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);

    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);
  }

  Status
  BTreeIndex::make(Database *db, Class *cls, const char *attrpath,
		   Bool propagate, Bool is_string,
		   const char *hints, BTreeIndex *&idx)
  {
    idx = 0;
    IndexImpl *idximpl;
    Status s = IndexImpl::make(db, IndexImpl::BTree, hints, idximpl,
			       is_string);
    if (s) return s;

    idx = new BTreeIndex(db, cls, attrpath, propagate, is_string, idximpl);
    return Success;
  }

  Status BTreeIndex::check(Class *) const
  {
    return 0;
  }

  Status BTreeIndex::realize(const RecMode *rcm)
  {
    Status s;

    Bool backend_updating = IDBBOOL(getUserData(index_backend));
    Bool creating = IDBBOOL(!backend_updating && !getOid().isValid());
    Bool updating = False;
    bool index_moving = (getUserData(index_move) != 0);

    /*
      printf("realizing btreeindex %s %s creating=%d\n",
      getOid().toString(), getName(), creating);
    */
    if (creating) {
#ifndef DUP_INDEX
      s = checkUnique("index", "index");
      if (s) return s;
#endif
    }
    else if (!backend_updating) {
      BTreeIndex *idx;
      s = db->reloadObject(getOid(), (Object *&)idx);
      if (s) return s;
      if ((getDegree() && idx->getDegree() != getDegree()) ||
	  idx->getDspid () != getDspid() ||
	  !compareHints(idx)) {
#ifdef REIMPL_TRACE
	printf("BTreeindex: should reimplement: "
	       "degree: %d %d "
	       "dspid: %d %d\n",
	       getDegree(), idx->getDegree(),
	       getDspid(), idx->getDspid());
#endif
	updating = True;
      }
#ifdef REIMPL_TRACE
      else
	printf("BTreeindex: should NOT reimplement\n");
#endif
      if (!getDegree())
	setDegree(idx->getDegree());
      idx->release();
    }

    s = AttributeComponent::realize(rcm);
    if (s) return s;
    if (creating || updating)
      return StatusMake(indexCreate(db->getDbHandle(),
				    index_moving,
				    oid.getOid()));

    return Success;
  }

  Status BTreeIndex::report(eyedbsm::DbHandle *sedbh, const Oid &idxoid)
  {
    eyedbsm::BIdx bidx(sedbh, *idxoid.getOid());
    if (bidx.status())
      return Exception::make(IDB_ERROR, eyedbsm::statusGet(bidx.status()));

    setDegree(bidx.getDegree());
#ifdef REIMPL_TRACE
    printf("Btreeindex::report ");
    trace();
#endif
    return Success;
  }

  Status BTreeIndex::remove(const RecMode* )
  {
    RPCStatus rpc_status =
      indexRemove(db->getDbHandle(), oid.getOid(),
		      getUserData(attrcomp_delete_ud) ? 1 : 0);

    if (rpc_status)
      return StatusMake(rpc_status);

    return AttributeComponent::remove();
  }

  Status BTreeIndex::s_trace(FILE *fd, Bool is_string, unsigned int flags) const
  {
    if (!(flags & AttrCompDetailTrace)) {
      fprintf(fd, "index<btree> on %s", getAttrpath().c_str());
      return Success;
    }

    fprintf(fd, "index<type = btree");
    Bool hints = False;

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;

    if (dataspace) {
      PREF(fd, hints);
      fprintf(fd, "dataspace = %s;", dataspace->getName());
    }

    if (getDegree()) {
      PREF(fd, hints);
      fprintf(fd, "degree = %d;", getDegree());
    }

    fprintf(fd, "%s%s> on %s", (hints ? "\"" : ""), getPropagString(this),
	    getAttrpath().c_str());

    return Success;
  }

  Status
  BTreeIndex::m_trace(FILE *fd, int indent, unsigned int flags,
		      const RecMode *rcm) const
  {
    Status s = Success;
    s = s_trace(fd, False, flags);
    if (s) return s;
    //fprintf(fd, " on %s::%s", getClassOwner()->getName(), getAttrname());

    if ((flags & CompOidTrace) == CompOidTrace ||
	rcm->getType() == RecMode_FullRecurs)
      fprintf(fd, " {%s}", oid.getString());

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
      }
    return s;
  }

  AttributeComponent *
  BTreeIndex::xclone(Database *db, const Class *cls)
  {
    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) throw *s;

    std::string str = makeAttrpath(cls);
    return new BTreeIndex(db, (Class *)cls, str.c_str(),
			  getPropagate(), getIsString(),
			  dataspace, getDegree(), 0, 0);
  }

  Status
  BTreeIndex::setImplementation(const IndexImpl *idximpl)
  {
    if (idximpl->getType() == IndexImpl::Hash)
      return Exception::make(IDB_ERROR,
			     "cannot change dynamically btree index "
			     "implementation to hash index using the "
			     "setImplementation() method: use the "
			     "reimplement() method");
    setDegree(idximpl->getDegree());
    if (idximpl->getDataspace())
      setDspid(idximpl->getDataspace()->getId());
    unsigned int impl_hints_cnt;
    const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
    setImplHintsCount(impl_hints_cnt);
    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);

    return Success;
  }

  Status
  BTreeIndex::getImplementation(IndexImpl *&idximpl, Bool remote) const
  {
    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;

    if (!remote) {
      unsigned int cnt = getImplHintsCount();
      int *impl_hints = (cnt ? new int [cnt] : 0);

      idximpl = new IndexImpl(IndexImpl::BTree, dataspace, getDegree(),
			      0, impl_hints, cnt);
      delete [] impl_hints;
      return Success;
    }

    RPCStatus rpc_status =
      indexGetImplementation(db->getDbHandle(),
				 oid.getOid(), (Data *)&idximpl);
    if (rpc_status)
      return StatusMake(rpc_status);

    idximpl->setDataspace(dataspace);
    return Success;
  }

  const char *BTreeIndex::genName() const
  {
    static std::string s;
    s = std::string("index") + ":" + getAttrpath();
    return s.c_str();
  }
 

  Status
  CardinalityConstraint_Test::realize(const RecMode *rcm)
  {
    return 0;
  }

  Status
  CardinalityConstraint_Test::remove(const RecMode *rcm)
  {
    return 0;
  }

  int
  CardinalityConstraint_Test::getInd() const
  {
    return Class::CardinalityConstraint_C;
  }

  int
  AttributeComponent::getInd() const
  {
    assert(0);
    return 0;
  }

  CollAttrImpl::CollAttrImpl(Database *_db, Class *cls,
			     const char *attrpath,
			     Bool propagate,
			     const Dataspace *dataspace,
			     CollImpl::Type impl_type,
			     int key_count_or_degree,
			     BEMethod_C *mth,
			     const int *impl_hints,
			     int impl_hints_cnt)
    : AttributeComponent(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(attrpath);
    setImplType(impl_type);
    setKeyCountOrDegree(key_count_or_degree);
    setPropagate(propagate);
    if (dataspace)
      setDspid(dataspace->getId());
    setHashMethod(mth);
    setName(genName());
    for (int i = 0; i < impl_hints_cnt; i++)
      setImplHints(i, impl_hints[i]);
  }

  CollAttrImpl::CollAttrImpl(Database *_db, Class *cls,
			     const char *attrpath,
			     Bool propagate,
			     const IndexImpl *idximpl)
    : AttributeComponent(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    setClassOwner(cls);
    setAttrpath(attrpath);
    if (idximpl) {
      setImplType(idximpl->getType());
      setKeyCountOrDegree(idximpl->getKeycount());
      setHashMethod(idximpl->getHashMethod());
      if (idximpl->getDataspace())
	setDspid(idximpl->getDataspace()->getId());
      unsigned int impl_hints_cnt;
      const int *impl_hints = idximpl->getImplHints(impl_hints_cnt);
      
      for (int i = 0; i < impl_hints_cnt; i++)
	setImplHints(i, impl_hints[i]);
    }
    else {
      setImplType(CollImpl::NoIndex);
    }

    setPropagate(propagate);
    setName(genName());
  }

  Status
  CollAttrImpl::makeDataspace(Database *_db, const Dataspace *&dataspace) const
  {
    Bool isnull;
    short dspid = getDspid(&isnull);
    if (!isnull)
      return _db->getDataspace(dspid, dataspace);
    dataspace = 0;
    return Success;
  }

  Status
  CollAttrImpl::make(Database *db, Class *cls, const char *attrpath,
		     Bool propagate, CollImpl::Type impl_type,
		     const char *hints, CollAttrImpl *&impl)
  {
    impl = 0;
    IndexImpl *idximpl;
    if (impl_type == CollImpl::HashIndex || impl_type == CollImpl::BTreeIndex) {
      Status s = IndexImpl::make(db, (IndexImpl::Type)impl_type, hints, idximpl);
      if (s) return s;
    }
    else {
      if (hints) {
	return Exception::make(IDB_ERROR, "no hints for no indexed collections");
      }
      idximpl = NULL;
    }

    impl = new CollAttrImpl(db, cls, attrpath, propagate, idximpl);
    return Success;
  }

  AttributeComponent *
  CollAttrImpl::xclone(Database *db, const Class *cls)
  {
    unsigned int impl_hints_cnt = getImplHintsCount();
    int impl_hints[eyedbsm::HIdxImplHintsCount];
    memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
    for (int i = 0; i < impl_hints_cnt; i++)
      impl_hints[i] = getImplHints(i);

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) throw *s;

    std::string str = makeAttrpath(cls);
    return new CollAttrImpl(db, (Class *)cls, str.c_str(),
			    getPropagate(),
			    dataspace, (CollImpl::Type)getImplType(),
			    getKeyCountOrDegree(),
			    getHashMethod(), impl_hints, impl_hints_cnt);
  }

  Status
  CollAttrImpl::realize(const RecMode *rcm)
  {
    Status s;

    Bool creating = IDBBOOL(!getOid().isValid());
    //printf("realizing collattrimpl %s\n", getAttrpath());
    if (creating) {
      s = checkUnique("collection_attribute_implementation",
		      "collection attribute implementation");
      if (s) return s;
    }

    s = AttributeComponent::realize(rcm);
    if (s) return s;
    if (creating)
      return StatusMake(constraintCreate(db->getDbHandle(),
					     oid.getOid()));
    return Success;
  }

  Status
  CollAttrImpl::remove(const RecMode *rcm)
  {
    RPCStatus rpc_status =
      constraintDelete(db->getDbHandle(), oid.getOid(),
			   getUserData(attrcomp_delete_ud) ? 1 : 0);

    if (rpc_status)
      return StatusMake(rpc_status);
    return AttributeComponent::remove(rcm);
  }

  Status
  CollAttrImpl::s_trace(FILE *fd, Bool, unsigned int flags) const
  {
    int idxtype = getImplType();
    fprintf(fd, "implementation<type = %s", idxtype == IndexImpl::Hash ?
	    "hash" : "btree");
    Bool hints = False;

    const Dataspace *dataspace;
    Status s = makeDataspace(db, dataspace);
    if (s) return s;

    if (dataspace) {
      PREF(fd, hints);
      fprintf(fd, "dataspace = %s;", dataspace->getName());
    }

    const BEMethod_C *mth = getHashMethod();
    if (mth) {
      PREF(fd, hints);
      fprintf(fd, "key_function = %s::%s;", mth->getClassOwner()->getName(),
	      mth->getEx()->getExname().c_str());
    }

    if (getKeyCountOrDegree()) {
      PREF(fd, hints);
      if (idxtype == IndexImpl::Hash)
	fprintf(fd, "key_count = %d;", getKeyCountOrDegree());
      else
	fprintf(fd, "degree = %d;", getKeyCountOrDegree());
    }

    unsigned int cnt = getImplHintsCount();
    for (unsigned int i = 0; i < cnt; i++) {
      if ((i) == eyedbsm::HIdx::IniSize_Hints &&
	  getImplHints(eyedbsm::HIdx::IniObjCnt_Hints))
	continue;
      int val = getImplHints(i);
      if (val) {
	PREF(fd, hints);
	fprintf(fd, "%s = %d;", IndexImpl::hashHintToStr(i), val);
      }
    }

    fprintf(fd, "%s%s> on %s", (hints ? "\"" : ""), getPropagString(this),
	    getAttrpath().c_str());

    return Success;
  }

  Status
  CollAttrImpl::m_trace(FILE *fd, int indent, unsigned int flags,
			const RecMode *rcm) const
  {
    Status s = Success;
    s = s_trace(fd, False);
    if (s) return s;

    if ((flags & CompOidTrace) == CompOidTrace ||
	rcm->getType() == RecMode_FullRecurs)
      fprintf(fd, " {%s}", oid.getString());

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
      }
    return s;
  }

  const char *
  CollAttrImpl::genName() const
  {
    static std::string s;
    s = std::string("implementation") + ":" + getAttrpath();
    return s.c_str();
  }

  int
  CollAttrImpl::getInd() const
  {
    return Class::CollectionImpl_C;
  }

  Status
  CollAttrImpl::getImplementation(Database *db,
				  const CollImpl *&_collimpl)

  {
    CollImpl::Type impl_type = (CollImpl::Type)getImplType();
    const IndexImpl *idximpl = 0;
    if (!collimpl) {
      if (impl_type == CollImpl::HashIndex || impl_type == CollImpl::BTreeIndex) {
	const Dataspace *dataspace;
	Status s = makeDataspace(db, dataspace);
	if (s) return s;
	
	unsigned int impl_hints_cnt = getImplHintsCount();
	int impl_hints[eyedbsm::HIdxImplHintsCount];
	memset(impl_hints, 0, sizeof(int) * eyedbsm::HIdxImplHintsCount);
	for (int i = 0; i < impl_hints_cnt; i++)
	  impl_hints[i] = getImplHints(i);
	idximpl = new IndexImpl((IndexImpl::Type)impl_type,
				dataspace, getKeyCountOrDegree(),
				getHashMethod(), impl_hints,
				impl_hints_cnt);
	collimpl = new CollImpl(idximpl);
      }
      else {
	collimpl = new CollImpl(impl_type);
      }
    }

    _collimpl = collimpl;
    return Success;
  }

  void CollAttrImpl::userInitialize()
  {
    collimpl = 0;
    dsp = 0;
  }

  void CollAttrImpl::userCopy(const Object &)
  {
    collimpl = 0;
    dsp = 0;
  }

  void CollAttrImpl::userGarbage()
  {
    if (collimpl)
      collimpl->release();
    dsp = 0;
  }
}
