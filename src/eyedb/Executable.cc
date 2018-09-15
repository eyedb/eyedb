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


#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include "eyedb_p.h"

namespace eyedb {

  static eyedbsm::Oid nulloid;

  Status
  eyedb_CHECKObjRefType(Database *db, Argument &arg, const char *which)
  {
    if (!arg.u.o->isModify())
      {
	arg.set(arg.u.o->getOid(), db); // requalification
	return Success;
      }

    if (!arg.u.o->getClass()->isFlatStructure())
      return Exception::make(IDB_ERROR,
			     "argument %s : object '%s' of class '%s' "
			     "is not consistent with database "
			     ": cannot be transmitted",
			     which, arg.u.o->getOid().getString(),
			     arg.u.o->getClass()->getName());
    return Success;
  }

  Status
  eyedb_CHECKObjType(Database *db, Argument &arg, const char *which)
  {
    const ArgType *t = arg.type;

    if (t->getType() == ARRAY_TYPE)
      return eyedb_CHECKObjArrayType(db, arg, which);

    if (t->getType() != OBJ_TYPE)
      return Success;

    if (!arg.u.o)
      return Success;

    if (arg.u.o->getOid().isValid())
      return eyedb_CHECKObjRefType(db, arg, which);

    if (!arg.u.o->getClass()->isFlatStructure() &&
	!(db->getOpenFlag() & _DBOpenLocal) &&
	!db->isBackEnd()) // the two && conditions added the 12/06/01
      return Exception::make(IDB_ERROR,
			     "argument %s : non persistent object of "
			     "class '%s' is not a flat structure : "
			     "cannot be transmitted",
			     which, arg.u.o->getClass()->getName());
    return Success;
  }

  Status
  eyedb_CHECKObjArrayType(Database *db, Argument &arg, const char *which)
  {
    int cnt = arg.u.array->getCount();

    for (int i = 0; i < cnt; i++)
      {
	Status s = eyedb_CHECKObjType(db, *((*arg.u.array)[i]), which);
	if (s) return s;
      }

    return Success;
  }

  Status
  eyedb_CHECKArgument(Database *db, ArgType t1, Argument &arg,
		      const char *typname, const char *name, const char *which,
		      int inout)
  {
    int type = t1.getType();

    if (!inout || (type & inout)) {
      ArgType t2 = *arg.type;

      t1.setType((ArgType_Type)(type & ~(INOUT_ARG_TYPE)), False);

      if (!t1.getType()) // means ANY
	return Success;

      Status s = eyedb_CHECKObjType(db, arg, which);
      if (s) return s;

      if (t1 != t2) {
	if (t1.getType() == OBJ_TYPE && t2.getType() == OBJ_TYPE) {
	  if (!arg.u.o) // means nil objects
	    return Success;
	  const Class *m1, *m2;
	  m1 = db->getSchema()->getClass(t1.getClname().c_str());
	  m2 = db->getSchema()->getClass(t2.getClname().c_str());
	  /*
	    printf("check obj argument: m1 '%s' m2 '%s'\n",
	    t1.getClname(), t2.getClname());
	  */
	  Bool is;
	  if (m1 && m2 && !m1->isSuperClassOf(m2, &is) && is)
	    return Success;
	}

	return Exception::make(IDB_EXECUTABLE_ERROR,
			       "%s %s, %s argument %s mismatch, "
			       "expected %s, got %s",
			       typname, name, 
			       (inout ? ((inout & IN_ARG_TYPE) ?
					 "input" : "output") : "return"),
			       which,
			       Argument::getArgTypeStr(&t1),
			       Argument::getArgTypeStr(&t2));
      }
    }
  
    return Success;
  }

  Status
  eyedb_CHECKArguments(Database *db, const Signature *sign,
		       ArgArray &array, const char *typname,
		       const char *name, int inout)
  {
    int cnt = sign->getNargs();
    if (cnt != array.getCount())
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "%s %s, %d arguments expected, got %d",
			     typname, name, cnt,
			     array.getCount());

    for (int i = 0; i < cnt; i++) {
      char which[12];
      sprintf(which, "#%d", i+1);

      Status s = eyedb_CHECKArgument(db, *sign->getTypes(i), *array[i],
				     typname, name, which, inout);
      if (s) return s;
    }

    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // Executable Methods
  //
  // ---------------------------------------------------------------------------

  const char *
  Executable::getSOTag()
  {
    static std::string s = std::string("-") + getVersion();
    return s.c_str();
  }

  /*
    const char *
    Executable::getSOTag(const Compiler *comp)
    {
    static std::string s;

    s = std::string("-") + eyedb::getVersion() +
    (comp ? comp->getSoTag() : eyedb::getDefaultCompiler()->getSoTag());
    return s;
    }
  */

  const char *
  Executable::makeExtRef(const char *extref)
  {
    static std::string s;

    int len = strlen(extref);
    if (len > 3 && !strcmp(&extref[len-3], ".so"))
      return extref;

    //  s = std::string(extref) + getSOTag(comp) + ".so";
    s = std::string(extref) + getSOTag() + ".so";

    return s.c_str();
  }

  static const char *
  get_next_path(const char *sopath, int &idx)
  {
    static char path[512];

    const char *s = strchr(sopath+idx, ':');
    if (!s)
      {
	s = sopath+idx;
	idx += strlen(sopath+idx);
	return s;
      }

    int len = s-sopath-idx;
    strncpy(path, sopath+idx, len);
    path[len] = 0;
    idx += strlen(path)+1;
    return path;
  }

  void *
  Executable::_dlopen(const char *extref)
  {
    const char *s = makeExtRef(extref);
    const char *sopath = ServerConfig::getSValue("sopath");

    if (!sopath)
      return (void *)0;

    const char *x;
    int idx = 0;
    while (*(x = get_next_path(sopath, idx)))
      {
	void *dl = dlopen((std::string(x) + "/" + s).c_str(), RTLD_LAZY);
	if (dl)
	  return dl;
      }

    return (void *)0;
  }

  const char *
  Executable::getSOFile(const char *extref)
  {
    static std::string file;
    //  const char *s = makeExtRef(extref, comp);
    const char *s = makeExtRef(extref);
    const char *sopath = ServerConfig::getSValue("sopath");

    if (!sopath)
      return (const char *)0;

    const char *x;
    int idx = 0;
    while (*(x = get_next_path(sopath, idx)))
      {
	file = std::string(x) + "/" + s;
	int fd = open(file.c_str(), O_RDONLY);
	if (fd >= 0)
	  {
	    close(fd);
	    return file.c_str();
	  }
      }

    return (const char *)0;
  }

  Status
  Executable::checkRealize(const char *extref, const char *intname,
			   void **pdl, void **pcsym)
  {
    if (!extref)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "invalid null external reference for"
			     "function '%s'", intname);
    if (*pdl)
      dlclose(*pdl);

    *pdl = Executable::_dlopen(extref);

    if (!*pdl)
      {
	std::string s = std::string("method `") + intname + "' check failed : " +
	  dlerror();
	return Exception::make(IDB_EXECUTABLE_ERROR, s);
      }

    *pcsym = dlsym(*pdl, intname);

    if (!*pcsym)
      {
	dlclose(*pdl);
	*pdl = 0;
	return Exception::make(IDB_EXECUTABLE_ERROR,
			       "symbol '%s' not found in external reference '%s'",
			       intname, extref);
      }

    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // const char *
  // Executable::makeInternalName(const char *exname,
  //                                 const Signature *sign,
  //				   Bool isClassMethod, const char *clname)
  //
  // Builds the internal name from the executable name, the signature and
  // the optionnal class name
  //
  // ---------------------------------------------------------------------------

  const char *
  Executable::makeInternalName(const char *exname, const Signature *sign,
			       Bool isClassMethod,
			       const char *clname)
  {
    static char intname[512];

    strcpy(intname, "method_");

    if (isClassMethod)
      strcat(intname, "static");

    if (sign)
      strcat(intname, Argument::getArgTypeStr(sign->getRettype(), False));

    int n = (sign ? sign->getNargs() : 0);

    strcat(intname, "_");
    strcat(intname, exname);

    if (clname)
      {
	strcat(intname, "_");
	strcat(intname, clname);
      }

    for (int i = 0; i < n; i++)
      {
	strcat(intname, "_");
	strcat(intname, Argument::getArgTypeStr(sign->getTypes(i), False));
      }

    return intname;
  }

  int Executable::isStaticExec() const
  {
    return (getLoc() & STATIC_EXEC) ? 1 : 0;
  }

  void Executable::initExec(const char *exname,
			    ExecutableLang lang,
			    Bool isSystem,
			    ExecutableLocalisation loc,
			    Signature *sign,
			    Class *_class)
  {
    setExname(exname);
    setLang((ExecutableLang)(lang | (isSystem ? SYSTEM_EXEC : 0)),
	    False);
    setLoc(loc, False);
    if (sign)
      *getSign() = *sign;
    const char *name = makeInternalName
      (exname, (sign ? getSign() : 0),
       ((loc & STATIC_EXEC) ? True : False),
       //     (_class ? _class->getName() : 0));
       // changed the 19/05/99
       (_class ? _class->getAliasName() : 0));
    setIntname(name);
  }

  void Executable::userInitialize()
  {
    dl = (void *)0;
  }

  void Executable::userCopy(const Object &)
  {
    userInitialize();
  }

  Status Executable::checkRealize(const char *extref, const char *mcname,
				  void **pcsym)
  {
    const char *intname = makeInternalName
      (getExname().c_str(), getSign(),
       ((getLoc()&STATIC_EXEC) ? True : False), mcname);

    return checkRealize(extref, intname, &dl, pcsym);
  }

  Status Executable::execCheck()
  {
    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // Method Methods
  //
  // ---------------------------------------------------------------------------

  Status Method::execCheck()
  {
    return Success;
  }

  Status Method::realize(const RecMode *rcm)
  {
    if (!db)
      return Exception::make(IDB_ERROR, "no database associated with object");

    if (!oid.isValid()) {
      OQL q(db, "select method.ex.intname = \"%s\"", getEx()->getIntname().c_str());

      ObjectArray obj_arr(true);
      Status s = q.execute(obj_arr);

      if (obj_arr.getCount()) {
	//obj_arr[0]->release();
	return Exception::make(IDB_UNIQUE_CONSTRAINT_ERROR,
			       "method '%s::%s' already exists in"
			       " database '%s'",
			       getClassOwner()->getName(),
			       getEx()->getIntname().c_str(),
			       db->getName());
      }
    }

    return ClassComponent::realize(rcm);
  }

  Status Method::remove(const RecMode *rcm)
  {
    return ClassComponent::remove(rcm);
  }

  Status Method::check(Class *cl) const
  {
    return Success;
  }

  int Method::getInd() const
  {
    return Class::Method_C;
  }

  const char *
  Method::getPrototype(Bool scope) const
  {
    return getEx()->_getPrototype(getClassOwner(), scope);
  }

  Bool Method::isInherit() const
  {
    return True;
  }

  const char *
  Executable::_getPrototype(const Class *_class,
			    Bool scope) const
  {
    static std::string proto;

    Bool istrs;
    if (db && !db->isInTransaction())
      {
	db->transactionBegin();
	istrs = True;
      }
    else
      istrs = False;    

    //proto.reset();
    proto = "";

    const Signature *sign = getSign();

    ArgType rettype = *sign->getRettype();
    rettype.setType((ArgType_Type)(rettype.getType() & ~INOUT_ARG_TYPE),
		    False);
    //  proto.append(Argument::getArgTypeStr(&rettype));
    proto += Argument::getArgTypeStr(&rettype);

    char tok[256];
    if (_class && scope)
      sprintf(tok, " %s::%s(", _class->getName(), getExname().c_str());
    else
      sprintf(tok, " %s(", getExname().c_str());

    //  proto.append(tok);
    proto += tok;

    int n = sign->getNargs();
    for (int i = 0; i < n; i++)
      {
	sprintf(tok, "%s%s", (i ? ", " : ""),
		Argument::getArgTypeStr(sign->getTypes(i)));
	//proto.append(tok);
	proto += tok;
      }

    //proto.append(")");
    proto += ")";

    if (istrs)
      db->transactionCommit();

    return proto.c_str();
  }

  extern void print_oqlexec(FILE *fd, const char *body);

  Status
  Method::m_trace(FILE *fd, int indent, unsigned int flags,
		  const RecMode* rcm) const
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

    const Executable *ex = getEx();
    const Signature *sign = ex->getSign();
    const Class *_class = getClassOwner();

    ArgType t = *sign->getRettype();
    t.setType((ArgType_Type)(t.getType() & ~OUT_ARG_TYPE), False);
    fprintf(fd, "%s_method <%s> %s ",
	    (ex->isStaticExec() ? "class" : "instance"),
	    (asFEMethod_C() ? "client" : "server"),
	    Argument::getArgTypeStr(&t));

    if (flags & NoScope)
      fprintf(fd, "%s(", ex->getExname().c_str());
    else
      fprintf(fd, "%s::%s(", (_class ? _class->getName() : "??"),
	      ex->getExname().c_str());

    int is_clang = (ex->getLang() & C_LANG);
    int n = sign->getNargs();
    for (int i = 0; i < n; i++)
      {
	fprintf(fd, "%s%s",
		(i ? ", " : ""), Argument::getArgTypeStr(sign->getTypes(i)));
	if (!is_clang && (flags & ExecBodyTrace))
	  fprintf(fd, " x%d", i);
      }

    fprintf(fd, ")");

    if (flags & ExecBodyTrace)
      {
	if (is_clang)
	  fprintf(fd, " C++(\"%s\")", ex->getExtrefBody().c_str());
	else
	  {
	    ((Method *)this)->asBEMethod_OQL()->runtimeInit();
	    if (asBEMethod_OQL()->body)
	      print_oqlexec(fd, asBEMethod_OQL()->body);
	  }
      }

    if (rcm->getType() == RecMode_FullRecurs)
      {
	fprintf(fd, " ");
	s = ObjectPeer::trace_realize(this, fd, indent + INDENT_INC, flags, rcm);
      }

    if ((flags & CompOidTrace) == CompOidTrace)
      fprintf(fd, " %s", oid.getString());

    if (istrs)
      db->transactionCommit();

    return s;
  }

  Status Method::applyTo(Database *_db, Object *o,
			 ArgArray &array, Argument &retarg,
			 Bool checkArgs)
  {
    return Success;
  }

  Status Method::get(Database *db, Class *_class,
		     const char *exname,
		     const Signature *sign, Bool isClassMethod,
		     Method* &mth)
  {
    const char *intname = Executable::makeInternalName
      //    (exname, sign, isClassMethod, _class->getName());
      // changed the 19/5/99
      (exname, sign, isClassMethod, _class->getAliasName());

    mth = 0;

    const LinkedList *mth_list = _class->getCompList(Class::Method_C);
    if (!mth_list || !mth_list->getCount())
      return Success;

    //  Method **pmth = new Method*[mth_list->getCount()];
    Method **pmth = (Method **)malloc(sizeof(Method *) *
				      mth_list->getCount());
    int mth_cnt = 0;
    LinkedListCursor *c = new LinkedListCursor(mth_list);
    Method *tmth;

    while (mth_list->getNextObject(c, (void *&)tmth))
      {
	Executable *ex = tmth->getEx();
	if (!strcmp(ex->getExname().c_str(), exname) && *ex->getSign() == *sign)
	  pmth[mth_cnt++] = tmth;
      }

    mth_list->endScan(c);

    if (mth_cnt == 1)
      mth = pmth[0];
    else if (mth_cnt > 1)
      {
	Class *cl = _class;
	int carryon = 1;
	while (cl && carryon)
	  {
	    for (int i = 0; i < mth_cnt; i++)
	      if (pmth[i]->getClassOwner()->compare(cl))
		{
		  mth = pmth[i];
		  carryon = 0;
		  break;
		}
	    cl = cl->getParent();
	  }
      }

    free(pmth);

    return Success;
  }

  //
  // Signature utilities
  //

#define check_symbol(C) \
(((C) >= 'a' && (C) <= 'z') || ((C) >= 'A' && (C) <= 'Z') || (C) == '_' ||\
 (C) >= '0' && (C) <= '9')

  static const char *
  getSpaceRid(const char *s, unsigned int len)
  {
    static char *mem;
    static int mem_len;

    if (strlen(s) > mem_len)
      {
	mem_len = strlen(s);
	mem = (char *)malloc(mem_len + 1);
      }

    char *p = mem;
    char c;
    unsigned int l = 0;

    if (!len)
      len = ~0;

    while ((c = *s) && l < len)
      {
	if (check_symbol(c) || c == '*' || c == '[' || c == ']')
	  *p++ = *s;
	s++;
	l++;
      }

    *p = 0;
    return mem;
  }

  static char *
  getReturnType(const char *s, const char *& p)
  {
    p = s;
    char c;
    int state = 0;

    while (c = *p)
      {
	if (c != ' ' && c != '\t')
	  break;
	p++;
      }

    while (c = *p)
      {
	if (state && check_symbol(c))
	  {
	    int len = p-s;
	    return strdup(getSpaceRid(s, p-s));
	  }

	if (c == ' ' || c == '\t' || c == '*' || c == ']')
	  state = 1;
	  
	p++;
      }

    return 0;
  }
  
  static char *
  getExecName(const char *s, const char *& p)
  {
    const char *q = p;
    char *name = 0;
    char c;

    while (c = *p)
      {
	if (c == '(')
	  {
	    name = strdup(getSpaceRid(q, p-q));
	    break;
	  }

	if (!check_symbol(c))
	  return 0;
	p++;
      }

    if (!name)
      return 0;

    return name;
  }

  struct Arg {
    int inout;
    char *arg;

    Arg() {
      inout = 0;
      arg = NULL;
    }

    void set(int _inout, const char *_arg) {
      inout = _inout;
      arg = strdup(_arg);
    }

    ~Arg() {
      free(arg);
    }
  };

  static int
  getInOut(const char *t)
  {
    if (!strcasecmp(t, "in"))
      return IN_ARG_TYPE;

    if (!strcasecmp(t, "out"))
      return OUT_ARG_TYPE;

    if (!strcasecmp(t, "inout"))
      return INOUT_ARG_TYPE;	

    return 0;
  }

  static int
  getArgTypes(const char *s, const char *& p, Arg args[], int &nargs,
	      const char *&str)
  {
    static const char missinout[] =
      "missing IN, OUT or INOUT keyword(s) in signature";

    int carryon = 1;
    char c;
    nargs = 0;
    p++;
    str = "";

    for (nargs = 0; carryon; )
      {
	int state = 0;
	const char *q = p;
	int inout = 0;
	while (c = *p)
	  {
	    if (c == ',')
	      {
		args[nargs].set(inout, getSpaceRid(q, p-q));

		if (!inout)
		  {
		    str = missinout;
		    return 0;
		  }

		if (*args[nargs].arg)
		  nargs++;
		else
		  return 0;
		p++;
		break;
	      }

	    if (c == ')')
	      {
		args[nargs].set(inout, getSpaceRid(q, p-q));

		if (!inout && !*args[nargs].arg)
		  return 1;

		if (!inout)
		  {
		    str = missinout;
		    return 0;
		  }

		if (*args[nargs].arg)
		  nargs++;
		else
		  return 0;

		p++;
		carryon = 0;
		break;
	      }

	    if (c == ' ' || c == '\t')
	      {
		if (state == 1)
		  {
		    if (!(inout = getInOut(getSpaceRid(q, p-q))))
		      {
			str = missinout;
			return 0;
		      }

		    state = 2;
		    q = p;
		  }
	      }
	    else if (!state)
	      state = 1;

	    p++;
	  }

	if (!c)
	  break;
      }

    while (c = *p)
      {
	if (c != ' ' && c != '\t')
	  return 0;
	p++;
      }
    return 1;
  }

  Status
  Method::getSignature(Database *db, Class *cls, 
		       const char *s, Signature *&sign, char *&name)
  {
    char *rettype;
    int nargs;
    Arg args[64];
    const char *ctx = 0;
    sign = NULL;

    if (!(rettype = getReturnType(s, ctx)))
      return Exception::make
	(IDB_ERROR, "invalid signature syntax: invalid return type");

    if (!(name = getExecName(s, ctx))) {
      free(rettype);
      return Exception::make
	(IDB_ERROR, "invalid signature syntax: invalid method name");
    }
  
    const char *str;
    if (!getArgTypes(s, ctx, args, nargs, str)) {
      free(rettype);
      free(name);
      return Exception::make(IDB_ERROR, "invalid signature syntax: %s", str);
    }
    
    sign = new Signature(db);
    Schema *m = db->getSchema();
    ArgType *type = ArgType::make(m, rettype);

    free(rettype);

    if (type) {
      ArgType *t = sign->getRettype();
      t->setDatabase(db);
      t->setType((ArgType_Type)(type->getType() | OUT_ARG_TYPE),
		 False);
      if (type->getClname().c_str())
	t->setClname(type->getClname().c_str());
      type->release();
    }
    else
      return Exception::make(IDB_ERROR,
			     "invalid signature syntax: invalid return type");

    sign->setNargs(nargs);
    sign->getClass()->getAttribute("types")->setSize(sign, nargs);

    for (int i = 0; i < nargs; i++) {
      if (type = ArgType::make(m, args[i].arg)) {
	ArgType *t = sign->getTypes(i);
	t->setDatabase(db);
	t->setType((ArgType_Type)(type->getType() | args[i].inout),
		   False);
	if (type->getClname().c_str())
	  t->setClname(type->getClname().c_str());
	type->release();
      }
      else
	return Exception::make(IDB_ERROR, "invalid signature syntax: invalid argument type '%s'", args[i].arg);
    }
  
    return Success;
  }

  Status Method::get(Database *db, Class *_class,
		     const char *sign_str, Bool isClassMethod,
		     Method *&mth)
  {
    Signature *sign = NULL;
    char *fname = NULL;

    if (!_class)
      return Exception::make(IDB_EXECUTABLE_ERROR, "invalid null class");

    Status s = getSignature(db, _class, sign_str, sign, fname);
    if (s) return s;

    s = get(db, _class, fname, sign, isClassMethod, mth);

    free(fname);
    sign->release();

    return s;
  }

  // ---------------------------------------------------------------------------
  //
  // FEMethod_C Methods
  //
  // ---------------------------------------------------------------------------

  FEMethod_C::FEMethod_C(Database *_db, Class *_class,
			 const char *name, Signature *sign,
			 Bool isClassMethod,
			 Bool isSystem, const char *extref)
    : FEMethod(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    db = _db;
    Executable *ex = getEx();
    ex->initExec(name, C_LANG, isSystem,
		 (ExecutableLocalisation)(FRONTEND | (isClassMethod ? STATIC_EXEC : 0)),
		 sign, _class);
    setClassOwner(_class);
    ex->setExtrefBody(extref);
    setName(ex->getIntname());
  }

  void FEMethod_C::userInitialize()
  {
    getEx()->Executable::userInitialize();
    csym = 0;
  }

  void FEMethod_C::userCopy(const Object &)
  {
    userInitialize();
  }

  Status FEMethod_C::applyTo(Database *_db, Object *o,
			     ArgArray &array, Argument &retarg,
			     Bool checkArgs)
  {
    Executable *ex = getEx();
    Status s;

    if (checkArgs) {
      s = eyedb_CHECKArguments(db, ex->getSign(), array, "method",
			       ex->getExname().c_str(), IN_ARG_TYPE);
      if (s) return s;
    }

    if (!csym)
      {
	Bool isTrs = _db->isInTransaction();
	if (!isTrs)
	  _db->transactionBegin();
	const char *mcname = getClassOwner()->getName();
	if (!isTrs)
	  _db->transactionCommit();
	Status s = ex->checkRealize(ex->getExtrefBody().c_str(), mcname,
				    (void **)&csym);
	if (s) return s;
      }

    return csym(_db, this, o, array, retarg);
  }

  Status FEMethod_C::execCheck()
  {
    Executable *ex = getEx();
    return ex->checkRealize(ex->getExtrefBody().c_str(), NULL, (void **)&csym);
  }

  // ---------------------------------------------------------------------------
  //
  // BEMethod_C Methods
  //
  // ---------------------------------------------------------------------------

  BEMethod_C::BEMethod_C(Database *_db, Class *_class,
			 const char *name, Signature *sign,
			 Bool isClassMethod, Bool isSystem,
			 const char *extref)
    : BEMethod(_db, (const Dataspace *)0, 1)
  {
    initialize(_db);
    //db = _db;
    Executable *ex = getEx();
    ex->initExec(name, C_LANG, isSystem,
		 (ExecutableLocalisation)(BACKEND | (isClassMethod ? STATIC_EXEC : 0)),
		 sign, _class);
    setClassOwner(_class);
    ex->setExtrefBody(extref);
    setName(ex->getIntname());
  }

  Status BEMethod_C::execCheck()
  {
    if (!db)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "BEMethod_C: cannot set external "
			     "reference, database is not set");

    Executable *ex = getEx();

    if (db->isBackEnd())
      return ex->checkRealize(ex->getExtrefBody().c_str(), NULL, (void **)&csym);

    RPCStatus rpc_status;

    rpc_status = eyedb::execCheck(db->getDbHandle(),
				  ex->getIntname().c_str(), oid.getOid(),
				  ex->getExtrefBody().c_str());

    return StatusMake(rpc_status);
  }

  Status BEMethod_C::applyTo(Database *_db, Object *o,
			     ArgArray &array, Argument &retarg,
			     Bool checkArgs)
  {
    const eyedbsm::Oid *objoid;
    Oid nulloid;

    if (o && !o->getOid().isValid() &&
	(!o->asCollection() || !o->asCollection()->getOidC().isValid())) {
      //extern eyedbsm::Boolean se_backend;
      //if (!se_backend)
      if (!(db->getOpenFlag() & _DBOpenLocal) &&
	  !db->isBackEnd()) // test added the 12/06/01
	return Exception::make(IDB_EXECUTABLE_ERROR,
			       "cannot apply a backend method on a "
			       "non persistent object");
    }
  
    if (o)
      objoid = (o->asCollection() ? o->asCollection()->getOidC().getOid() :
		o->getOid().getOid());
    else
      objoid = nulloid.getOid();

    Executable *ex = getEx();
    Status s;

    if (checkArgs) {
      s = eyedb_CHECKArguments(db, ex->getSign(), array, "method",
			       ex->getExname().c_str(), IN_ARG_TYPE);
      if (s) return s;
    }

    RPCStatus rpc_status;
    const eyedbsm::Oid *_oid = getOid().getOid();;
  
    rpc_status = execExecute(_db->getDbHandle(), 
				 _db->getUser(),
				 _db->getPassword(),
				 ex->getIntname().c_str(),
				 ex->getExname().c_str(),
				 METHOD_C_TYPE |
				 (getEx()->isStaticExec() ? STATIC_EXEC : 0),
				 getClassOwner()->getOid().getOid(),
				 ex->getExtrefBody().c_str(),
				 ex->getSign(),
				 _oid,
				 objoid,
				 o,
				 (const void *)&array,
				 (void **)&retarg);

    if (rpc_status)
      return StatusMake(rpc_status);

    return Success;
  }

  // ---------------------------------------------------------------------------
  //
  // BEMethod_OQL Methods
  //
  // ---------------------------------------------------------------------------

  BEMethod_OQL::BEMethod_OQL(Database *_db, Class *_class,
			     const char *name, Signature *sign,
			     Bool isClassMethod, Bool isSystem,
			     const char *_body)
  {
    initialize(_db);
    db = _db;
    Executable *ex = getEx();
    ex->initExec(name, OQL_LANG, isSystem,
		 (ExecutableLocalisation)(BACKEND|
					  (isClassMethod ? STATIC_EXEC : 0)),
		 sign, _class);
    setClassOwner(_class);
    ex->setExtrefBody(_body);
    setName(ex->getIntname());
  }

  Status BEMethod_OQL::applyTo(Database *, Object *,
			       ArgArray &, Argument &retarg,
			       Bool checkArgs)
  {
    return Exception::make("cannot use the 'applyTo' method to an OQL method");
  }

  Status BEMethod_OQL::execCheck()
  {
    // for now
    return Success;
  }

  Status BEMethod_OQL::setBody(const char *_body)
  {
    getEx()->setExtrefBody(_body);
    return Success;
  }

  //
  // extref coding
  //
  // paramcnt:varname#1:varname#2:...:funcname:body
  //

  std::string
  BEMethod_OQL::makeExtrefBody(const Class *cls, const char *oql,
			       const char *name,
			       char *typnames[],
			       char *varnames[],
			       unsigned int param_cnt,
			       std::string &oqlConstruct)
  {
    std::string s = str_convert((long)param_cnt);
    int i;

    for (i = 0; i < param_cnt; i++)
      s += std::string(":") + varnames[i];

    s += ":";

    std::string funcname = std::string("oql$") + cls->getAliasName() + "$" +
      name;

    for (i = 0; i < param_cnt; i++)
      funcname += std::string("$") + typnames[i];

    s += funcname;
    s += ":";

    oqlConstruct = std::string("function ") + funcname + " (";

    for (i = 0; i < param_cnt; i++)
      {
	if (i) oqlConstruct += ",";
	oqlConstruct += varnames[i];
      }

    oqlConstruct += ")";
    oqlConstruct += oql;

    return s + oql;
  }

  Status
  BEMethod_OQL::runtimeInit()
  {
    if (isRTInitialized)
      return Success;

    char *r;
    const char *s = getEx()->getExtrefBody().c_str();

    tmpbuf = strdup(s);
    char *q = strchr(tmpbuf, ':');

    if (!q)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "invalid internal format '%s'", s);
    *q = 0;
    param_cnt = atoi(tmpbuf);

    varnames = new char *[param_cnt];
    std::string tmp = "(";
    for (int i = 0; i < param_cnt; i++)
      {
	r = strchr(q+1, ':');
	if (!r)
	  return Exception::make(IDB_EXECUTABLE_ERROR,
				 "invalid internal format '%s'", s);
	*r = 0;
	varnames[i] = q+1;
	if (i) tmp += ",";
	tmp += varnames[i];
	q = r;
      }

    tmp += ")";
    r = strchr(q+1, ':');
    if (!r)
      return Exception::make(IDB_EXECUTABLE_ERROR,
			     "invalid internal format '%s'", s);
    *r = 0;
    funcname = q+1;

    r++;
    body = r;
    fullBody = strdup((std::string("function ") + funcname + tmp + body).c_str());
    isRTInitialized = True;
    return Success;
  }

  void
  BEMethod_OQL::userInitialize()
  {
    isRTInitialized = False;
    varnames = 0;
    param_cnt = 0;
    body = 0;
    fullBody = 0;
    funcname = 0;
    tmpbuf = 0;
    entry = 0;
  }

  void BEMethod_OQL::userCopy(const Object &)
  {
    userInitialize();
  }

  void
  BEMethod_OQL::userGarbage()
  {
    delete[] varnames;
    free(tmpbuf);
    free(fullBody);
  }

}
  
