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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <wctype.h>
#include <limits.h>
#include <eyedb/oqlctb.h>
#include <set>
#include <vector>

//#define SUPPORT_OQLRESULT
//#define SUPPORT_POSTACTIONS

// work, but very slow !
//#define SYNC_SYM_GARB
#define SYNC_SYM_GARB_1
#define SYNC_SYM_GARB_2

//#define GARB_TRACE
//#define GARB_TRACE_DETAIL

#define NEW_SUPPRESS_DOUBLES

#include "oql_p.h"

namespace eyedb {

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // static declarations
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  LinkedList oqmlObjectManager::freeList;
  ObjCache *oqmlObjectManager::objCacheIdx = new ObjCache(1024);
  ObjCache *oqmlObjectManager::objCacheObj = new ObjCache(1024);

  gbLink *oqmlGarbManager::first, *oqmlGarbManager::last;
  oqmlBool oqmlGarbManager::garbaging = oqml_False;
  unsigned int oqmlGarbManager::count = 0;
  LinkedList oqmlGarbManager::str_list;
  LinkedList oqmlNode::node_list;

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlGarbManager methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  std::list<gbContext *> oqmlGarbManager::ctx_l;

  gbContext *oqmlGarbManager::peek()
  {
    gbContext *ctx = new gbContext(last);
    ctx_l.push_back(ctx);
    return ctx;
  }

  void oqmlGarbManager::garbage(gbContext *ctx)
  {
    garbage(ctx->link, false);

    delete ctx;

    std::list<gbContext *>::iterator begin = ctx_l.begin();
    std::list<gbContext *>::iterator end = ctx_l.end();
    while (begin != end) {
      if ((*begin) == ctx) {
	ctx_l.erase(begin);
	return;
      }
      ++begin;
    }
  }

  void oqmlGarbManager::garbage(gbLink *l, bool full)
  {
    if (!l)
      return;

    if (!full)
      l = l->next;

    unsigned int cnt_atoms = 0, cnt_lists = 0;
    unsigned int total_cnt_atoms = 0, total_cnt_lists = 0;

#ifdef GARB_TRACE
    printf("GARBAGE %s %u %u\n", (full ? "full" : "partial"),
	   oqmlObjectManager::objCacheIdx->getObjectCount(),
	   oqmlObjectManager::objCacheObj->getObjectCount());
#endif
    garbaging = oqml_True;
    int toDel_cnt = 0;
    gbLink **links = new gbLink*[count];

    while (l) {
      Bool toDel = False;
      gbLink *next = l->next;
      if (l->at) {
	if (!l->at->refcnt) {
	  delete l->at;
	  cnt_atoms++;
	  toDel = True;
	}
	total_cnt_atoms++;
      }

      if (l->atlist) {
	if (!l->atlist->refcnt) {
	  l->atlist->first = 0;
	  delete l->atlist;
	  cnt_lists++;
	  toDel = True;
	}
	total_cnt_lists++;
      }

      if (toDel)
	links[toDel_cnt++] = l;
      l = next;
    }

    garbaging = oqml_False;

    for (int i = 0; i < toDel_cnt; i++)
      remove(links[i]);

    delete [] links;

#ifdef GARB_TRACE
    printf("%u/%u atoms garbaged\n", cnt_atoms, total_cnt_atoms);
    printf("%u/%u lists garbaged\n", cnt_lists, total_cnt_atoms);
#endif
  }

  void oqmlGarbManager::garbage()
  {
    unsigned int cnt_str = 0;
#if 0
    unsigned int cnt_atoms = 0, cnt_lists = 0;
    unsigned int total_cnt_atoms = 0, total_cnt_lists = 0;
    gbLink *l = first;

#ifdef GARB_TRACE
    printf("GARBAGE global %u %u\n",
	   oqmlObjectManager::objCacheIdx->getObjectCount(),
	   oqmlObjectManager::objCacheObj->getObjectCount());
#endif

    garbaging = oqml_True;
    int toDel_cnt = 0;
    gbLink **links = new gbLink*[count];

    while (l) {
      Bool toDel = False;
      gbLink *next = l->next;
      if (l->at) {
	if (!l->at->refcnt) {
	  delete l->at;
	  cnt_atoms++;
	  toDel = True;
	}
	total_cnt_atoms++;
      }

      if (l->atlist) {
	if (!l->atlist->refcnt) {
	  l->atlist->first = 0;
	  delete l->atlist;
	  cnt_lists++;
	  toDel = True;
	}
	total_cnt_lists++;
      }

      if (toDel)
	links[toDel_cnt++] = l;
      l = next;
    }
#else
    garbage(first, true);
    garbaging = oqml_True;
#endif
    if (str_list.getCount()) {
      LinkedListCursor c(str_list);
      char **tabs = new char *[str_list.getCount()];
      char *s;
      while (c.getNext((void* &)s)) {
	free(s);
	tabs[cnt_str++] = s;
      }
	
      for (int i = 0; i < cnt_str; i++)
	str_list.deleteObject(tabs[i]);
      
      delete [] tabs;
    }

    garbaging = oqml_False;

#if 0
    for (int i = 0; i < toDel_cnt; i++)
      remove(links[i]);

    delete [] links;
#endif

#if 0
#ifdef GARB_TRACE
    printf("%u/%u atoms garbaged\n", cnt_atoms, total_cnt_atoms);
    printf("%u/%u lists garbaged\n", cnt_lists, total_cnt_atoms);
    printf("%u strings garbaged\n", cnt_str);
#endif
#endif
    oqmlNode::garbageNodes();
    oqmlObjectManager::garbageObjects();
  }

  const char *
  oqmlAtomType::getString() const
  {
    static char typestr[4][32];
    static int n;

    if (n >= 4)
      n = 0;

    switch(type) {
    case oqmlATOM_BOOL:
      strcpy(typestr[n], "bool");
      break;

    case oqmlATOM_OID:
      strcpy(typestr[n], "oid");
      break;

    case oqmlATOM_INT:
      strcpy(typestr[n], "integer");
      break;

    case oqmlATOM_DOUBLE:
      strcpy(typestr[n], "float");
      break;

    case oqmlATOM_CHAR:
      strcpy(typestr[n], char_class_name);
      break;

    case oqmlATOM_STRING:
      strcpy(typestr[n], "string");
      break;

    case oqmlATOM_IDENT:
      strcpy(typestr[n], "ident");
      break;

    case oqmlATOM_LIST:
      strcpy(typestr[n], "list");
      break;

    case oqmlATOM_BAG:
      strcpy(typestr[n], "bag");
      break;

    case oqmlATOM_SET:
      strcpy(typestr[n], "set");
      break;

    case oqmlATOM_ARRAY:
      strcpy(typestr[n], "array");
      break;

    case oqmlATOM_STRUCT:
      strcpy(typestr[n], "struct");
      break;

    case oqmlATOM_OBJ:
      strcpy(typestr[n], "object");
      break;

    case oqmlATOM_NODE:
      strcpy(typestr[n], "node");
      break;

    case oqmlATOM_SELECT:
      strcpy(typestr[n], "select");
      break;

    case oqmlATOM_NULL:
      strcpy(typestr[n], NullString);
      break;

    case oqmlATOM_NIL:
      strcpy(typestr[n], "nil");
      break;

    default:
      strcpy(typestr[n], "<unknown>");
      break;
    }

    return typestr[n++];
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlNode methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlNode::oqmlNode(oqmlTYPE _type)
  {
    type = _type;
    eval_type.type = (oqmlATOMTYPE)0;
    eval_type.cls = 0;
    eval_type.comp = oqml_False;
    cst_list = 0;
    loc.from = -1;
    loc.to = -1;
    locked = oqml_False;
    is_statement = oqml_False;
    back = 0;
    //  back = this;
    registerNode(this);
  }

  void
  oqmlNode::registerNode(oqmlNode *node)
  {
    node_list.insertObjectLast(node);
  }

  void oqmlNode::locate(int from, int to)
  {
    loc.from = from;
    loc.to = to;
  }

  static int __garb_guardian__;

  void
  oqmlNode::garbageNodes()
  {
    __garb_guardian__ = 1;

    oqmlNode *node;
#if 0
    LinkedListCursor c1(node_list);
    while (c1.getNext((void *&)node)) {
      if (!node->isLocked())
	printf("freeing node '%s'\n", node->toString().c_str());
      else
	printf("node '%s' is locked\n", node->toString().c_str());
    }
#endif

    LinkedListCursor c(node_list);

    while (c.getNext((void *&)node))
      if (!node->isLocked())
	delete node;

    node_list.empty();
    __garb_guardian__ = 0;
  }

  oqmlSymbolTable oqmlContext::stsymtab;

#define CTX_REENTRANT

  oqmlStatus *oqmlNode::realize(Database *db, oqmlAtomList **al)
  {
#ifdef CTX_REENTRANT
    oqmlContext oqml_ctx_st;
    oqmlContext *oqml_ctx = &oqml_ctx_st;
#else
    static oqmlContext *oqml_ctx;

    oqmlContext *ctx_sv = oqml_ctx;
    if (!oqml_ctx)
      oqml_ctx = new oqmlContext();
#endif

    oqmlStatus *s = compile(db, oqml_ctx);

    if (!s)
      s = eval(db, oqml_ctx, al);

#ifndef CTX_REENTRANT
    if (!ctx_sv)
      delete oqml_ctx;

    oqml_ctx = ctx_sv;
#endif
    return s;
  }

  oqmlStatus *
  oqmlNode::evalLeft(Database *db, oqmlContext *ctx, oqmlAtom **a,
		     int &idx)
  {
    oqmlAtomList *alist;
    oqmlStatus *s = eval(db, ctx, &alist);
    if (s) return s;

    idx = -1;

    if (alist->cnt == 1 && alist->first->as_ident()) {
      *a = alist->first->as_ident();
      return oqmlSuccess;
    }

    if (alist->cnt == 1)
      return new oqmlStatus(this, "%s is not a left value.",
			    alist->first->getString());

    return new oqmlStatus(this, "not a left value.");
  }

  void oqmlNode::requalifyType(oqmlTYPE _type)
  {
    //printf("requalify from %d to %d\n", type, _type);
    type = _type;
  }

  oqmlNode::~oqmlNode()
  {
    //printf("deleting node %p\n", this);
    assert(__garb_guardian__);
    //  delete cst_list;
  }

  oqmlStatus *oqmlNode::compEval(Database *db, oqmlContext *ctx,
				 oqmlAtomType *at)
  {
    oqmlStatus *s;
    s = compile(db, ctx);

    if (s)
      return s;

    evalType(db, ctx, at);

    return oqmlSuccess;
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlComp methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlComp::oqmlComp(oqmlTYPE _type, oqmlNode *_qleft, oqmlNode *_qright,
		     const char *_opstr) : oqmlNode(_type)
  {
    iter = 0;
    cst_atom = 0;
    opstr = strdup(_opstr);
    qleft = _qleft;
    qright = _qright;
    locked = oqml_False;
  }

  oqmlComp::~oqmlComp()
  {
    // delete qleft;
    // delete qright;
    delete iter;
    free(opstr);
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqml_List methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqml_List::oqml_List()
  {
    cnt = 0;
    first = last = 0;
  }

  oqml_List::~oqml_List()
  {
    oqml_Link *l = first;
    while (l) {
      oqml_Link *next = l->next;
      delete l;
      l = next;
    }
  }

  oqmlBool
  oqml_List::hasIdent(const char *_ident)
  {
    oqml_Link *l = first;
    while (l) {
      if (l->ql->hasIdent(_ident))
	return oqml_True;
      l = l->next;
    }

    return oqml_False;
  }

  void
  oqmlNode::lock()
  {
    locked = oqml_True;
    oqmlLock(cst_list, oqml_True);
  }

  void
  oqmlNode::unlock()
  {
    locked = oqml_False;
    oqmlLock(cst_list, oqml_False);

    if (node_list.getPos(this) < 0) {
      //printf("registering object %p while unlocking\n", this);
      registerNode(this);
    }
  }

  void
  oqml_List::lock()
  {
    oqml_Link *l = first;
    while (l) {
      l->ql->lock();
      l = l->next;
    }
  }

  void
  oqml_List::unlock()
  {
    oqml_Link *l = first;
    while (l) {
      l->ql->unlock();
      l = l->next;
    }
  }

  std::string
  oqml_List::toString() const
  {
    std::string s = "";
    oqml_Link *l = first;

    for (int n = 0; l; n++) {
      if (n) s += ",";
      s += l->ql->toString();
      l = l->next;
    }

    return s;
  }

  oqml_Link::oqml_Link(oqmlNode *_ql)
  {
    ql = _ql;
    next = 0;
  }

  oqml_Link::~oqml_Link()
  {
    // delete ql;
  }

  void oqml_List::add(oqmlNode *ql)
  {
    oqml_Link *l = new oqml_Link(ql);
    if (last) {
      last->next = l;
      last = l;
    }
    else
      first = last = l;

    cnt++;
  }

  oqml_IdentLink::oqml_IdentLink(const char *_ident, oqmlNode *_ql)
  {
    ident = _ident ? strdup(_ident) : 0;
    ql = _ql;
    left = 0;
    next = 0;
    skipIdent = oqml_False;
    requalified = oqml_False;
    cls = 0;
  }

  void
  oqml_IdentLink::lock()
  {
    if (ql)    ql->lock();
    if (left)  left->lock();
  }

  void
  oqml_IdentLink::unlock()
  {
    if (ql)    ql->unlock();
    if (left)  left->unlock();
  }

  oqml_IdentLink::oqml_IdentLink(oqmlNode *_left, oqmlNode *_right)
  {
    ident = 0;
    left = _left;
    ql = _right;
    next = 0;
  }

  oqml_IdentLink::~oqml_IdentLink()
  {
    free(ident);
  }

  oqml_IdentList::oqml_IdentList()
  {
    cnt = 0;
    first = last = 0;
  }

  void
  oqml_IdentList::lock()
  {
    oqml_IdentLink *l = first;
    while (l) {
      l->lock();
      l = l->next;
    }
  }

  void
  oqml_IdentList::unlock()
  {
    oqml_IdentLink *l = first;
    while (l) {
      l->unlock();
      l = l->next;
    }
  }

  oqml_IdentList::~oqml_IdentList()
  {
    oqml_IdentLink *l = first;
    while (l) {
      oqml_IdentLink *next = l->next;
      delete l;
      l = next;
    }
  }

  void oqml_IdentList::add(const char *ident, oqmlNode *ql)
  {
    oqml_IdentLink *l = new oqml_IdentLink(ident, ql);
    if (last) {
      last->next = l;
      last = l;
    }
    else
      first = last = l;

    cnt++;
  }

  void oqml_IdentList::add(oqmlNode *left, oqmlNode *right)
  {
    oqml_IdentLink *l = new oqml_IdentLink(left, right);
    if (last) {
      last->next = l;
      last = l;
    }
    else
      first = last = l;

    cnt++;
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlAtom methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  oqmlAtom *
  oqmlAtom::make_atom(const IteratorAtom &atom, Class *cls)
  {
    // 12/09/05: for backward compatibility !
    switch(atom.type) {
    case IteratorAtom_INT16:
      return new oqmlAtom_int(atom.i16);
      
    case IteratorAtom_INT32:
      return new oqmlAtom_int(atom.i32);
      
    case IteratorAtom_INT64:
      return new oqmlAtom_int(atom.i64);
      
    case IteratorAtom_STRING:
      return new oqmlAtom_string(atom.str);
      
    case IteratorAtom_OID:
      return new oqmlAtom_oid(atom.oid, cls);
      
    case IteratorAtom_CHAR:
      return new oqmlAtom_char(atom.c);
      
    case IteratorAtom_DOUBLE:
      return new oqmlAtom_double(atom.d);
      
#ifndef NEW_ITER_ATOM
    case IteratorAtom_BOOL:
      return new oqmlAtom_bool((oqmlBool)atom.b);
      
    case IteratorAtom_IDENT:
      return new oqmlAtom_ident(atom.ident);
#endif
      
    case IteratorAtom_IDR:
    default:
      assert(0);
      return 0;
    }
  }

  static void
  incr_ref_count(Object *o)
  {
    if (!o)
      return;

    if (o->getMasterObject(false)) {
      incr_ref_count(o->getMasterObject(false));
      return;
    }

    o->incrRefCount();
  }

  oqmlAtom *
  oqmlAtom::make_atom(Data data, oqmlATOMTYPE type, const Class *cls)
  {
    switch(type) {
    case oqmlATOM_OID: {
      Oid noid;
      memcpy(&noid, data, sizeof(Oid));
      return new oqmlAtom_oid(noid, (Class *)cls);
    }

    case oqmlATOM_OBJ: {
      Object *o;
      mcp(&o, data, sizeof(Object *));
      // added the 5/11/99
      incr_ref_count(o);
      return oqmlObjectManager::registerObject(o);
    }

    case oqmlATOM_STRING:
      return new oqmlAtom_string((char *)data);

    case oqmlATOM_IDENT:
      return new oqmlAtom_ident((char *)data);

    case oqmlATOM_INT: {
      if (!cls || cls->asInt64Class()) {
	eyedblib::int64 ni;
	memcpy(&ni, data, sizeof(eyedblib::int64));
	return new oqmlAtom_int(ni);
      }

      if (cls->asInt32Class() || cls->asEnumClass()) {
	eyedblib::int32 ni;
	memcpy(&ni, data, sizeof(eyedblib::int32));
	return new oqmlAtom_int(ni);
      }

      if (cls->asInt16Class()) {
	eyedblib::int16 ni;
	memcpy(&ni, data, sizeof(eyedblib::int16));
	return new oqmlAtom_int(ni);
      }

      return new oqmlAtom_int(0);
    }

    case oqmlATOM_DOUBLE: {
      double nd;
      memcpy(&nd, data, sizeof(double));
      return new oqmlAtom_double(nd);
    }

    case oqmlATOM_CHAR:
      return new oqmlAtom_char((char)data[0]);

    default:
      assert(0);
    }
  }

  oqmlAtom::oqmlAtom(const oqmlAtom&)
  {
    abort();
  }

  oqmlAtom & oqmlAtom::operator =(const oqmlAtom &atom)
  {
    abort();
    return *this;
  }

  oqmlBool oqmlAtom::makeEntry(int ind, unsigned char *entry, int key_len,
			       const Class *_class) const
  {
    /*
      memset(&entry->data, 0, key_len);
      entry->ind = ind;
    */
    x2h_32_cpy(entry, &ind);
    memset(entry+sizeof(eyedblib::int32), 0, key_len);
    int len;
    Data val;
    Size size = key_len;
    return getData(entry+sizeof(eyedblib::int32), &val, size, len, _class);
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // oqmlAtom derived class methods
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  char *oqmlAtom_oid::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", oid.getString());
      return 0;
    }
    else if (string)
      return string;
    else {
      char buf[64];
      sprintf(buf, "%s", oid.getString());
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_int::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%lld", i);
      return 0;
    }
    else if (string)
      return string;
    else {
      char buf[32];
      sprintf(buf, "%lld", i);
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_double::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%f", d);
      return 0;
    }
    else if (string)
      return string;
    else {
      char buf[16];
      sprintf(buf, "%f", d);
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_bool::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", (b ? "true" : "false"));
      return 0;
    }
    else if (string)
      return string;
    else {
      char buf[16];
      sprintf(buf, "%s", (b ? "true" : "false"));
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_nil::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, NilString);
      return 0;
    }
    else if (string)
      return string;
    else {
      ((oqmlAtom *)this)->string = strdup(NilString);
      return string;
    }
  }

  char *oqmlAtom_null::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, NullString);
      return 0;
    }
    else if (string)
      return string;
    else {
      ((oqmlAtom *)this)->string = strdup(NullString);
      return string;
    }
  }

  char *oqmlAtom_char::makeString(FILE *fd) const
  {
    static const char cfmt[] = "'%c'";
    static const char ofmt[] = "'\\%03o'";

    if (fd) {
      if (iswprint(c)) fprintf(fd, cfmt, c);
      else             fprintf(fd, ofmt, c);
      return 0;
    }
    else if (string)
      return string;
    else {
      char buf[8];
      if (iswprint(c)) sprintf(buf, cfmt, c);
      else             sprintf(buf, ofmt, c);
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_string::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "\"%s\"", shstr->s);
      return 0;
    }
    else if (string)
      return string;
    else {
      char *buf = (char *)malloc(strlen(shstr->s)+3);
      sprintf(buf, "\"%s\"", shstr->s);
      ((oqmlAtom *)this)->string = buf;
      return string;
    }
  }


  char *oqmlAtom_ident::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", shstr->s);
      return 0;
    }
    else if (string)
      return string;
    else {
      ((oqmlAtom *)this)->string = strdup(shstr->s);
      return string;
    }
  }

  char *oqmlAtom_obj::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%lx:obj", idx);
      return 0;
    }

    if (string)
      return string;
    else {
      char buf[12];
      sprintf(buf, "%lx:obj", idx);
      ((oqmlAtom *)this)->string = strdup(buf);
      return string;
    }
  }

  char *oqmlAtom_node::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", node->toString().c_str());
      return 0;
    }

    if (!string)
      ((oqmlAtom *)this)->string = strdup(node->toString().c_str());
    return string;
  }

  oqmlBool
  oqmlAtom_node::getData(unsigned char[], Data *, Size&, int&,
			 const Class *) const
  {
    abort();
    return oqml_False;
  }

  int
  oqmlAtom_node::getSize() const
  {
    abort();
    return 0;
  }

  oqmlBool
  oqmlAtom_node::compare(unsigned char *, int, Bool, oqmlTYPE) const
  {
    abort();
    return oqml_False;
  }

  char *oqmlAtom_select::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", (node ? node->toString() : std::string("")).c_str());
      return 0;
    }

    if (!string)
      ((oqmlAtom *)this)->string = strdup((node ? node->toString() : std::string("")).c_str());
    return string;
  }

  oqmlBool
  oqmlAtom_select::getData(unsigned char[], Data *, Size&, int&,
			   const Class *) const
  {
    abort();
    return oqml_False;
  }

  int
  oqmlAtom_select::getSize() const
  {
    abort();
    return 0;
  }

  oqmlBool
  oqmlAtom_select::compare(unsigned char *, int, Bool, oqmlTYPE) const
  {
    abort();
    return oqml_False;
  }

  char *oqmlAtom_coll::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, getTypeName());
      if (list)
	list->print(fd);
      else
	fprintf(fd, "()");
      return 0;
    }

    if (string)
      return string;

    if (!list) {
      ((oqmlAtom_coll *)this)->string = strdup("()");
      return string;
    }

    char *buf = list->getString();
    ((oqmlAtom_coll *)this)->string = (char *)malloc(strlen(buf)+20);
    sprintf(string, "%s(%s)", getTypeName(), buf);
    return string;
  }

  char *oqmlAtom_struct::makeString(FILE *fd) const
  {
    if (fd) {
      fprintf(fd, "%s", makeString(0));
      return 0;
    }

    if (string)
      return string;

    std::string s = "struct(";

    for (int i = 0; i < attr_cnt; i++) {
      if (i) s += ", ";
      s += attr[i].name;
      s += ": ";
      s += (attr[i].value ? attr[i].value->makeString(0) : "");
    }

    s += ")";
    ((oqmlAtom *)this)->string = strdup(s.c_str());
    return string;
  }

  oqmlBool
  oqmlAtom_struct::getData(unsigned char[], Data *, Size&, int&,
			   const Class *) const
  {
    abort();
    return oqml_False;
  }

  int
  oqmlAtom_struct::getSize() const
  {
    abort();
    return 0;
  }

  oqmlBool
  oqmlAtom_struct::compare(unsigned char *, int, Bool, oqmlTYPE) const
  {
    abort();
    return oqml_False;
  }

  oqmlStatus *
  oqmlAtom_struct::setAtom(oqmlAtom *a, int idx, oqmlNode *)
  {
    assert (idx >= 0 && idx < attr_cnt);
    attr[idx].value = (a ? a->copy() : 0);
    return oqmlSuccess;
  }

  oqmlAtom *
  oqmlAtom_struct::getAtom(const char *name, int &idx)
  {
    for (int i = 0; i < attr_cnt; i++)
      if (!strcmp(attr[i].name, name)) {
	idx = i;
	return attr[i].value;
      }

    idx = -1;
    return 0;
  }

  oqmlAtom *
  oqmlAtom_struct::getAtom(unsigned int idx)
  {
    return (idx < 0 || idx >= attr_cnt) ? 0 : attr[idx].value;
  }

  oqmlBool oqmlAtom_string::makeEntry(int ind, unsigned char *entry, int key_len,
				      const Class *) const
  {
    if (key_len < strlen(shstr->s))
      return oqml_False;

    x2h_32_cpy(entry, &ind);
    memset(entry+sizeof(eyedblib::int32), 0, key_len);
    strcpy((char *)entry+sizeof(eyedblib::int32), shstr->s);
    return oqml_True;
  }

  oqmlBool
  oqmlAtom_int::getData(unsigned char data[], Data *val,
			Size &size, int &len, const Class *cls) const
  {
    unsigned int psize;

    if (!cls || cls->asInt64Class()) {
      psize = sizeof(eyedblib::int64);
      //eyedblib::int64 v = (eyedblib::int64)i;
      eyedblib::int64 v = i;
      memcpy(data, &v, psize);
    }
    else if (cls->asInt32Class() || cls->asEnumClass()) {
      psize = sizeof(eyedblib::int32);
      eyedblib::int32 v;
      if (i >= INT_MAX)
	v = INT_MAX;
      else if (i <= INT_MIN)
	v = INT_MIN;
      else
	v = (eyedblib::int32)i;
      memcpy(data, &v, psize);
    }
    else if (cls->asInt16Class()) {
      psize = sizeof(eyedblib::int16);
      eyedblib::int16 v;
      if (i >= SHRT_MAX)
	v = SHRT_MAX;
      else if (i <= SHRT_MIN)
	v = SHRT_MIN;
      else
	v = (eyedblib::int16)i;

      memcpy(data, &v, psize);
    }
    else
      psize = ~0;

    if (size < psize)
      return oqml_False;

    *val = 0;
    len = 1;
    size = psize;

    return oqml_True;
  }

  int oqmlAtom_int::getSize() const
  {
    return sizeof(eyedblib::int64);
  }

  oqmlBool oqmlAtom_bool::getData(unsigned char data[], Data *val,
				  Size &size, int &len,
				  const Class *) const
  {
    if (size < sizeof(oqmlBool))
      return oqml_False;

    *val = 0;
    size = sizeof(oqmlBool);
    len = 1;
    memcpy(data, &b, size);
    return oqml_True;
  }

  int oqmlAtom_bool::getSize() const
  {
    return sizeof(oqmlBool);
  }

  oqmlBool oqmlAtom_nil::getData(unsigned char data[], Data *val,
				 Size &size, int &len,
				 const Class *) const
  {
    *val = 0;
    memset(data, 0, size);
    len = 1;
    return oqml_True;
  }

  int oqmlAtom_nil::getSize() const
  {
    return 0;
  }

  oqmlBool oqmlAtom_null::getData(unsigned char data[], Data *val,
				  Size &size, int &len,
				  const Class *) const
  {
    *val = 0;
    memset(data, 0, size);
    len = 1;
    return oqml_True;
  }

  int oqmlAtom_null::getSize() const
  {
    return 0;
  }

  oqmlBool oqmlAtom_double::getData(unsigned char data[], Data *val,
				    Size &size, int &len,
				    const Class *) const
  {
    if (size < sizeof(double))
      return oqml_False;

    *val = 0;
    size = sizeof(double);
    len = 1;
    memcpy(data, &d, size);
    return oqml_True;
  }

  int oqmlAtom_double::getSize() const
  {
    return sizeof(double);
  }

  oqmlBool oqmlAtom_char::getData(unsigned char data[], Data *val,
				  Size &size, int &len,
				  const Class *) const
  {
    if (size < sizeof(char))
      return oqml_False;

    *val = 0;
    size = sizeof(char);
    len = 1;
    memcpy(data, &c, size);
    return oqml_True;
  }

  int oqmlAtom_char::getSize() const
  {
    return sizeof(char);
  }

  oqmlBool oqmlAtom_string::getData(unsigned char data[], Data *val,
				    Size &size, int &len,
				    const Class *) const
  {
    size = strlen(shstr->s) + 1;
    len = strlen(shstr->s) + 1;
    *val = (Data)shstr->s;
    return oqml_True;
  }

  int oqmlAtom_string::getSize() const
  {
    return strlen(shstr->s)+1;
  }

  oqmlStatus *
  oqmlAtom_string::setAtom(oqmlAtom *a, int idx, oqmlNode *node)
  {
    if (!a->as_char())
      return new oqmlStatus(node, "invalid right "
			    "operand: character expected, got "
			    "%s.", a->type.getString());

    assert (idx >= 0 && idx < strlen(shstr->s));
    shstr->s[idx] = a->as_char()->c;
    return oqmlSuccess;
  }

  oqmlBool oqmlAtom_ident::getData(unsigned char data[], Data *val,
				   Size &size, int &len,
				   const Class *) const
  {
    size = strlen(shstr->s);
    len = strlen(shstr->s);
    *val = (Data)shstr->s;
    return oqml_True;
  }

  int oqmlAtom_ident::getSize() const
  {
    return strlen(shstr->s)+1;
  }

  oqmlBool
  oqmlAtom_oid::getData(unsigned char data[], Data *val, Size &size,
			int &len, const Class *) const
  {
    if (size < sizeof(Oid))
      return oqml_False;

    *val = 0;
    size = sizeof(Oid);
    len = 1;
    memcpy(data, &oid, size);
    return oqml_True;
  }

  int oqmlAtom_oid::getSize() const
  {
    return sizeof(Oid);
  }

  oqmlBool
  oqmlAtom_obj::getData(unsigned char data[], Data *val, Size &size,
			int &len, const Class *) const
  {
    size = sizeof(Object *);
    len = 1;
    size = sizeof(Object *);
    *val = 0;
    memcpy(data, &o, size);
    return oqml_True;
  }

  int oqmlAtom_obj::getSize() const
  {
    return sizeof(Object *);
  }

  oqmlBool oqmlAtom_coll::getData(unsigned char data[], Data *val,
				  Size &size, int &len,
				  const Class *) const
  {
    *val = 0;
    return oqml_False;
  }

  int oqmlAtom_coll::getSize() const
  {
    return 0;
  }

  oqmlStatus *
  oqmlAtom_coll::setAtom(oqmlAtom *a, int idx, oqmlNode *node)
  {
    assert (idx >= 0 && idx < list->cnt);
    oqmlStatus *s = list->setAtom((a ? a->copy() : 0), idx, node);
    if (!s && as_set())
      suppressDoubles();
    return s;
  }

  oqmlAtom_coll *
  oqmlAtom_coll::suppressDoubles()
  {
    if (!list->cnt)
      return this;

    list->suppressDoubles();
    return this;
  }

#ifdef NEW_SUPPRESS_DOUBLES
  struct oqmlAtom_x {
    oqmlAtom_x(oqmlAtom *a) : a(a) {}
    oqmlAtom *a;
  };

  struct less_atom {
    bool operator() (const oqmlAtom_x& x, const oqmlAtom_x& y) const {
      return strcmp(x.a->getString(), y.a->getString()) < 0;
    }
  };
#endif

  void
  oqmlAtomList::suppressDoubles()
  {
#ifdef NEW_SUPPRESS_DOUBLES
    std::set<oqmlAtom_x, less_atom> set;

    oqmlAtom *a = first;
    while (a) {
      set.insert(a);
      a = a->next;
    }

    if (set.size() == cnt)
      return;

    first = last = 0;
    cnt = 0;
    std::set<oqmlAtom_x, less_atom>::iterator begin = set.begin();
    std::set<oqmlAtom_x, less_atom>::iterator end = set.end();
    while (begin != end) {
      append((*begin).a, false);
      ++begin;
    }
#else
    int n, i, j;
    oqmlBool noDoubles = oqml_True;
    oqmlAtom **arr = new oqmlAtom*[cnt];
    oqmlAtom *a = first;

    n = 0;
    while (a) {
      arr[n++] = a;
      a = a->next;
    }

    for (i = 0; i < cnt; i++)
      for (j = 0; j < i; j++)
	if (arr[j] && arr[i] && arr[j]->isEqualTo(*arr[i])) {
	  arr[i] = 0;
	  noDoubles = oqml_False;
	}

    if (noDoubles) {
      delete [] arr;
      return;
    }

    first = last = 0;
    int xcnt = cnt;
    cnt = 0;

    for (i = 0; i < xcnt; i++)
      if (arr[i])
	append(arr[i], false);

    delete [] arr;
#endif
  }

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  // 13/03/99 ... continue comments ...
  //
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  static oqmlBool
  checkNull(oqmlAtom *aleft, oqmlAtom *aright, oqmlTYPE type)
  {
    if (aleft->type.type == oqmlATOM_NULL) {
      if (aright->type.type == oqmlATOM_OID) {
	if (!((oqmlAtom_oid *)aright)->oid.isValid())
	  return (type == oqmlEQUAL ? oqml_True : oqml_False);
	return (type == oqmlEQUAL ? oqml_False : oqml_True);
      }

      if (aright->type.type == oqmlATOM_NULL)
	return (type == oqmlEQUAL ? oqml_True : oqml_False);

      return (type == oqmlEQUAL ? oqml_False : oqml_True);
    }

    return checkNull(aright, aleft, type);
  }

  static oqmlBool
  compare_double_int(oqmlAtom *a_int, oqmlAtom *a_double, oqmlTYPE type)
  {
    unsigned char data[16];
    Size size;
    int len;
    Data val;
    size = sizeof(data);
  
    if (a_int->getData(data, &val, size, len)) {
      eyedblib::int64 i;
      memcpy(&i, data, sizeof(eyedblib::int64));
      double d = (double)i;
      memcpy(data, &d, sizeof(double));
      return a_double->compare(data, sizeof(double), False, type);
    }
    return oqml_False;
  }

  static oqmlBool
  compare_atoms(oqmlAtom *aleft, oqmlAtom *aright, oqmlTYPE type)
  {
    if (type == oqmlEQUAL)
      return aleft->isEqualTo(*aright);

    unsigned char data[32];
    Size size;
    int len;
    Data val;
    size = sizeof(data);
	  
    if (aleft->getData(data, &val, size, len))
      return aright->compare((val ? val : data), size, False, type);

    return oqml_False;
  }

  oqmlStatus *
  oqmlAtomList::compare(oqmlNode *node, oqmlContext *ctx, oqmlAtomList *al,
			const char *opstr, oqmlTYPE type, oqmlBool &b) const
  {
    oqmlStatus *s;

    if (type == oqmlDIFF) {
      s = compare(node, ctx, al, opstr, oqmlEQUAL, b);
      if (!s) b = (oqmlBool)!b;
      return s;
    }

    if (al->cnt != cnt && type == oqmlEQUAL) {
      b = oqml_False;
      return oqmlSuccess;
    }

    oqmlAtom *aleft = first, *aright = al->first;
    if (!aleft && !aright) {
      b = oqml_True;
      return oqmlSuccess;
    }
    else if (!aleft || !aright) {
      b = oqml_False;
      return oqmlSuccess;
    }

    oqmlATOMTYPE left_type = aleft->type.type;
    oqmlATOMTYPE right_type = aright->type.type;

    if (left_type == oqmlATOM_RANGE)
      left_type = aleft->as_range()->from->type.type;

    if (right_type == oqmlATOM_RANGE)
      right_type = aright->as_range()->from->type.type;

    for (int i = 0; i < cnt && i < al->cnt; i++) {
      if (left_type == oqmlATOM_NIL &&
	  right_type == oqmlATOM_NIL) {
	b = ((type == oqmlEQUAL || type == oqmlINFEQ || type == oqmlSUPEQ) ?
	     oqml_True : oqml_False);
	if (!b)
	  return oqmlSuccess;
      }
      else if (left_type == oqmlATOM_NULL ||
	       right_type == oqmlATOM_NULL) {
	b = checkNull(aleft, aright, type);
	if (!b)
	  return oqmlSuccess;
      }
      else if (OQML_IS_SET(aleft) || OQML_IS_BAG(aleft)) {
	extern oqmlStatus *
	  oqml_set_compare(oqmlNode *node, oqmlTYPE type, const char *,
			   oqmlAtom *aleft, oqmlAtom *aright, oqmlBool &b);
	s = oqml_set_compare(node, type, opstr, aleft, aright, b);
	if (s || !b)
	  return s;
      }
      else if (left_type != right_type) {
	if (left_type == oqmlATOM_DOUBLE &&
	    right_type == oqmlATOM_INT) {
	  if (type == oqmlSUPEQ)
	    type = oqmlINFEQ;
	  else if (type == oqmlSUP)
	    type = oqmlINF;
	  else if (type == oqmlINFEQ)
	    type = oqmlSUPEQ;
	  else if (type == oqmlINF)
	    type = oqmlSUP;

	  b = compare_double_int(aright, aleft, type);
	  if (!b)
	    return oqmlSuccess;
	}

	else if (right_type == oqmlATOM_DOUBLE &&
		 left_type == oqmlATOM_INT) {
	  b = compare_double_int(aleft, aright, type);
	  if (!b)
	    return oqmlSuccess;
	}
#define TRUSS_JOIN
#define JOIN_TRACE
	else if (ctx->isWhereContext() &&
		 (OQML_IS_COLL(aleft) || OQML_IS_COLL(aright))) {
	  // this has been added to support construction in
	  // where clause such as: xxx.y[?].ss == "soso"
	  // 27/09/99
	  b = oqml_False;
	  if (OQML_IS_COLL(aleft)) {
	    oqmlAtom *left = OQML_ATOM_COLLVAL(aleft)->first;
	    while (left) {
	      if (compare_atoms(left, aright, type)) {
		b = oqml_True;
		break;
	      }
	      left = left->next;
	    }

	  }
	  else {
	    oqmlAtom *right = OQML_ATOM_COLLVAL(aright)->first;
	    while (right) {
	      if (compare_atoms(right, aleft, type)) {
		b = oqml_True;
		break;
	      }
	      right = right->next;
	    }
	  }

	  if (!b)
	    return oqmlSuccess;
	}
	else {
	  if (type == oqmlEQUAL) {
	    b = oqml_False;
	    return oqmlSuccess;
	  }

	  if (type != oqmlDIFF)
	    return new oqmlStatus(node,
				  "operation '%s %s %s' is not valid",
				  aleft->type.getString(), opstr,
				  aright->type.getString());
	}
      }
      else if (OQML_IS_LIST(aleft) || OQML_IS_ARRAY(aleft)) {
	oqmlTYPE xtype;
	if (type == oqmlSUP)      xtype = oqmlSUPEQ;
	else if (type == oqmlINF) xtype = oqmlINFEQ;
	else                      xtype = type;

	if (left_type != right_type && type == oqmlDIFF) {
	  b = oqml_True; return oqmlSuccess;
	}

	if (left_type != right_type && type == oqmlEQUAL) {
	  b = oqml_False; return oqmlSuccess;
	}

	s = OQML_ATOM_COLLVAL(aleft)->
	  compare(node, ctx, OQML_ATOM_COLLVAL(aright), opstr, xtype, b);

	if (s || !b)
	  return s;

	if (type == oqmlSUP || type == oqmlINF) {
	  s = OQML_ATOM_COLLVAL(aleft)->
	    compare(node, ctx, OQML_ATOM_COLLVAL(aright), opstr, oqmlDIFF, b);

	  if (s || !b)
	    return s;
	}
      }
      else if (!compare_atoms(aleft, aright, type)) {
	b = oqml_False;
	return oqmlSuccess;
      }
	  
      aleft = aleft->next;
      aright = aright->next;
    }

    if (al->cnt != cnt) {
      int c = al->cnt < cnt;
      int d = (type == oqmlSUP || type == oqmlSUPEQ);

      if ((c && d) || (!c && !d)) {
	b = oqml_True;
	return oqmlSuccess;
      }

      b = oqml_False;
      return oqmlSuccess;
    }

    b = oqml_True;
    return oqmlSuccess;
  }

  oqmlStatus *
  invalidBinop(oqmlNode *node, const char *opstr, const oqmlAtomType *at_left,
	       const oqmlAtomType *at_right)
  {
    return new oqmlStatus(node,
			  "operation '%s %s %s' is not valid.",
			  at_left->getString(), opstr, at_right->getString());
  }

  //
  // TODO: clarifier promotion char -> int
  //

  oqmlStatus *
  oqmlNode::binopCompile(Database *db, oqmlContext *ctx,
			 const char *opstr, oqmlNode *qleft,
			 oqmlNode *qright, oqmlAtomType &_eval_type,
			 oqmlBinopType binopType,
			 oqmlBool &iscts)
  {
    //if (!strcmp(opstr, "%")) opstr = "%%";
    oqmlStatus *s;
    oqmlAtomType at_left, at_right;
    oqmlBool cstleft, cstright;

    s = qleft->compile(db, ctx);
    if (s)
      return s;

    qleft->evalType(db, ctx, &at_left);

    cstleft = qleft->isConstant();
    s = qright->compile(db, ctx);
    if (s)
      return s;

    qright->evalType(db, ctx, &at_right);

    cstright = qright->isConstant();

    oqmlATOMTYPE type = at_left.type;

    _eval_type.type = oqmlATOM_UNKNOWN_TYPE;

    if (type != oqmlATOM_UNKNOWN_TYPE &&
	at_right.type != oqmlATOM_UNKNOWN_TYPE &&
	type != oqmlATOM_IDENT && at_right.type != oqmlATOM_IDENT) {
      if (!(binopType & oqmlDoubleOK) &&
	  (type == oqmlATOM_DOUBLE || at_right.type == oqmlATOM_DOUBLE))
	return invalidBinop(this, opstr, &at_left, &at_right);

      if (!(binopType & oqmlConcatOK) &&
	  (type == oqmlATOM_STRING || at_right.type == oqmlATOM_STRING ||
	   type == oqmlATOM_LIST || at_right.type == oqmlATOM_LIST ||
	   type == oqmlATOM_SET || at_right.type == oqmlATOM_SET ||
	   type == oqmlATOM_BAG || at_right.type == oqmlATOM_BAG ||
	   type == oqmlATOM_ARRAY || at_right.type == oqmlATOM_ARRAY))
	return invalidBinop(this, opstr, &at_left, &at_right);
      
      if (type == oqmlATOM_INT && at_right.type == oqmlATOM_DOUBLE) {
	_eval_type = at_right;
	iscts = OQMLBOOL(cstleft && cstright);
	return oqmlSuccess;
      }
      else if (type == oqmlATOM_DOUBLE &&
	       (at_right.type == oqmlATOM_INT ||
		at_right.type == oqmlATOM_CHAR)) {
	_eval_type = at_left;
	iscts = OQMLBOOL(cstleft && cstright);
	return oqmlSuccess;
      }
      else if (at_right.type == oqmlATOM_DOUBLE &&
	       (type == oqmlATOM_INT ||
		type == oqmlATOM_CHAR)) {
	_eval_type = at_right;
	iscts = OQMLBOOL(cstleft && cstright);
	return oqmlSuccess;
      }
      else if (type == oqmlATOM_INT && at_right.type == oqmlATOM_CHAR) {
	_eval_type = at_left;
	iscts = OQMLBOOL(cstleft && cstright);
	return oqmlSuccess;
      }
      else if (at_right.type == oqmlATOM_INT && type == oqmlATOM_CHAR) {
	_eval_type = at_right;
	iscts = OQMLBOOL(cstleft && cstright);
	return oqmlSuccess;
      }

      else if (type != oqmlATOM_INT && type != oqmlATOM_CHAR &&
	       type != oqmlATOM_DOUBLE &&
	       type != oqmlATOM_STRING &&
	       type != oqmlATOM_LIST &&
	       type != oqmlATOM_BAG &&
	       type != oqmlATOM_SET &&
	       type != oqmlATOM_ARRAY)
	return invalidBinop(this, opstr, &at_left, &at_right);
      _eval_type = at_left;
    }

    iscts = OQMLBOOL(cstleft && cstright);
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlNode::binopEval(Database *db, oqmlContext *ctx, const char *opstr,
		      const oqmlAtomType &_eval_type,
		      oqmlNode *qleft, oqmlNode *qright, oqmlBinopType binopType,
		      oqmlAtomList **al_left, oqmlAtomList **al_right)
  {
    //if (!strcmp(opstr, "%")) opstr = "%%";
    oqmlStatus *s;
    oqmlBool cstleft, cstright;

    s = qleft->eval(db, ctx, al_left);
    if (s)
      return s;

    s = qright->eval(db, ctx, al_right);
    if (s)
      return s;

    /*
      if ((*al_left)->cnt != 1 || (*al_right)->cnt != 1)
      return new oqmlStatus(this, "invalid operand.");
    */

    if (!(*al_left)->cnt)  (*al_left)->append(new oqmlAtom_nil());
    if (!(*al_right)->cnt) (*al_right)->append(new oqmlAtom_nil());

    oqmlAtom *aleft = (*al_left)->first, *aright = (*al_right)->first;

    if (!(binopType & oqmlDoubleOK) &&
	(aleft->type.type == oqmlATOM_DOUBLE ||
	 aright->type.type == oqmlATOM_DOUBLE))
      return invalidBinop(this, opstr, &aleft->type, &aright->type);

    if (!(binopType & oqmlConcatOK) &&
	(aleft->type.type == oqmlATOM_STRING ||
	 aright->type.type == oqmlATOM_STRING ||
	 OQML_IS_COLL(aleft) ||
	 OQML_IS_COLL(aright)))
      return invalidBinop(this, opstr, &aleft->type, &aright->type);

    if ((binopType & oqmlConcatOK) &&
	(OQML_IS_COLL(aleft) || OQML_IS_COLL(aright)))
      return oqmlSuccess;

    if ((aleft->type.type == oqmlATOM_INT &&
	 aright->type.type == oqmlATOM_INT) ||
	(aleft->type.type == oqmlATOM_DOUBLE &&
	 aright->type.type == oqmlATOM_DOUBLE) ||
	(aleft->type.type == oqmlATOM_CHAR &&
	 aright->type.type == oqmlATOM_CHAR) ||
	(aleft->type.type == oqmlATOM_STRING &&
	 aright->type.type == oqmlATOM_STRING) ||
	(OQML_IS_COLL(aleft) &&
	 OQML_IS_COLL(aright)))
      return oqmlSuccess;

    // (int -> double) promotion
    if (OQML_IS_INT(aleft) && OQML_IS_DOUBLE(aright)) {
      (*al_left)->setAtom(new oqmlAtom_double(OQML_ATOM_INTVAL(aleft)), 0, this);
      return oqmlSuccess;
    }
  
    if (OQML_IS_DOUBLE(aleft) && OQML_IS_INT(aright)) {
      (*al_right)->setAtom(new oqmlAtom_double(OQML_ATOM_INTVAL(aright)), 0, this);
      return oqmlSuccess;
    }
  
    // (char -> double) promotion
    if (OQML_IS_DOUBLE(aleft) && OQML_IS_CHAR(aright)) {
      (*al_right)->setAtom(new oqmlAtom_double(OQML_ATOM_CHARVAL(aright)), 0, this);
      return oqmlSuccess;
    }
  
    if (OQML_IS_CHAR(aleft) && OQML_IS_DOUBLE(aright)) {
      (*al_left)->setAtom(new oqmlAtom_double(OQML_ATOM_CHARVAL(aleft)), 0, this);
      return oqmlSuccess;
    }
  
    // (char -> int) promotion
    if (OQML_IS_INT(aleft) && OQML_IS_CHAR(aright)) {
      (*al_right)->setAtom(new oqmlAtom_int(OQML_ATOM_CHARVAL(aright)), 0, this);
      return oqmlSuccess;
    }
  
    if (OQML_IS_CHAR(aleft) && OQML_IS_INT(aright)) {
      (*al_left)->setAtom(new oqmlAtom_int(OQML_ATOM_CHARVAL(aleft)), 0, this);
      return oqmlSuccess;
    }
  
    return invalidBinop(this, opstr, &aleft->type, &aright->type);
  }

  void
  oqmlNode::swap(oqmlComp *comp, oqmlNode *&qleft, oqmlNode *&qright)
  {
    oqmlTYPE comp_type = comp->getType();
    oqmlNode *tql = qleft;
    qleft = qright;
    qright = tql;
  
    //printf("oqmlNode::SWAP\n");
    if (comp_type == oqmlSUP)        comp->requalifyType(oqmlINF);
    else if (comp_type == oqmlSUPEQ) comp->requalifyType(oqmlINFEQ);
    else if (comp_type == oqmlINF)   comp->requalifyType(oqmlSUP);
    else if (comp_type == oqmlINFEQ) comp->requalifyType(oqmlSUPEQ);
  }

  oqmlStatus *
  oqmlNode::compCompile(Database *db, oqmlContext *ctx, const char *opstr,
			oqmlNode *&qleft, oqmlNode *&qright,
			oqmlComp *comp, oqmlAtom **cst_atom,
			oqmlAtomType *_eval_type)
  {
    oqmlStatus *s;

    /*
      printf("compiling oqmlComp => %s context='%s %s %s' [iterator %d]\n",
      (const char *)comp->toString(),
      ctx->isSelectContext() ? "select" : "not select",
      ctx->isPrevalContext() ? "preval" : "not preval",
      ctx->isWhereContext() ? "where" : "not where",
      comp->getIterator());
    */

    comp->evalDone = oqml_False;
    comp->needReinit = oqml_False;

    s = qleft->compile(db, ctx);
    if (s) return s;

    s = qright->compile(db, ctx);
    if (s) return s;

    if (!qleft->asDot() && qright->asDot())
      swap(comp, qleft, qright);

    oqmlSelect *sel = ctx->getHiddenSelectContext();
    if (sel) {
      /*
	printf("WE ARE IN A HIDDEN SELECT CONTEXT left=%d right=%d\n",
	sel->usesFromIdent(qleft),
	sel->usesFromIdent(qright));
      */
      if (sel->usesFromIdent(qright) && !sel->usesFromIdent(qleft))
	swap(comp, qleft, qright);
    }

    oqmlAtomType at_right;
    qright->evalType(db, ctx, &at_right);

    oqmlAtomType at;
    qleft->evalType(db, ctx, &at);

    if (at.type != oqmlATOM_UNKNOWN_TYPE &&
	at_right.type != oqmlATOM_UNKNOWN_TYPE && !at.cmp(at_right, True)) {
      // changed 12/03/99:
      //      if (comp->getType() != oqmlEQUAL && comp->getType() != oqmlDIFF)

      // changed 24/02/00
      if (comp->getType() != oqmlEQUAL && comp->getType() != oqmlDIFF &&
	  comp->getType() != oqmlBETWEEN &&
	  comp->getType() != oqmlNOTBETWEEN)
	return new oqmlStatus(this, "operation '%s %s %s' "
			      "is not valid",
			      at.getString(), opstr, at_right.getString());
    }

    if (qleft->asDot()) {
      oqmlDot *dot = (oqmlDot *)qleft;

      if (dot->boolean_dot)
	_eval_type->type = oqmlATOM_BOOL;
      else {
	if (ctx->isSelectContext()) {
	  _eval_type->type = oqmlATOM_OID;
	  oqmlDotContext *dot_ctx = qleft->asDot()->getDotContext();
	  _eval_type->cls = (dot_ctx ? (Class *)dot_ctx->desc[0].cls : 0);
	}
	else
	  _eval_type->type = oqmlATOM_UNKNOWN_TYPE;
      }
    }
    else
      _eval_type->type = oqmlATOM_BOOL;

    *cst_atom = 0;

    return oqmlSuccess;
  }

  static void
  oqml_append_realize(oqmlAtomList *alist, oqmlAtomList *list)
  {
    oqmlAtom *a = alist->first;

    if (a->as_coll())
      a = a->as_coll()->list->first;

    while (a) {
      oqmlAtom *next = a->next;
      list->append(a);
      a = next;
    }
  }

  oqmlStatus *
  oqmlNode::compEval(Database *db, oqmlContext *ctx, const char *opstr,
		     oqmlNode *qleft, oqmlNode *qright,
		     oqmlAtomList **alist, oqmlComp *comp, oqmlAtom *cst_atom)
  {
    oqmlStatus *s;

    if (!ctx->isSelectContext()) {
      oqmlAtomList *al_left = 0, *al_right = 0;

      if (comp->evalDone) {
	*alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
	return oqmlSuccess;
      }

      if (comp->needReinit) {
	s = comp->reinit(db, ctx);
	if (s) return s;
	comp->needReinit = oqml_False;
      }

      s = qleft->eval(db, ctx, &al_left);
      if (s) return s;

      s = qright->eval(db, ctx, &al_right);
      if (s) return s;

      s = comp->compare(db, ctx, al_left, al_right, alist);
#ifdef SYNC_GARB
      OQL_DELETE(al_left);
      OQL_DELETE(al_right);
#endif
      return s;
    }

    oqmlBool done;
    unsigned int cnt;
    return comp->preEvalSelectRealize(db, ctx, 0, done, alist, oqml_True);
  }

  oqmlBool
  oqmlComp::hasDotIdent(const char *_ident)
  {
    // added the 27/02/00
    if (qleft->asDot() && qright->asDot())
      return OQMLBOOL(qleft->hasIdent(_ident) || qright->hasIdent(_ident));

    if (qleft->asDot())
      return qleft->hasIdent(_ident);

    // added the 27/02/00
    if (qright->asDot())
      return qright->hasIdent(_ident);

    return oqml_False;
  }

  oqmlStatus *
  oqmlComp::reinit(Database *db, oqmlContext *ctx)
  {
    assert(qleft->asDot());
    oqmlStatus *s = qleft->asDot()->reinit(db, ctx);
    if (s) return s;

    if (qright->asDot()) {
      s = qright->asDot()->reinit(db, ctx);
      return s;
    }

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlComp::preEvalSelect(Database *db, oqmlContext *ctx,
			  const char *ident, oqmlBool &done,
			  unsigned int &cnt, oqmlBool firstPass)
  {
    // JE PENSE que ce n'est pas le bon test!
    // le bon test est 'ctx->getSelectContext()->usesFromIdent(this))'
    if (hasDotIdent(ident)) {
      oqmlAtomList *al;
      oqmlStatus *s = preEvalSelectRealize(db, ctx, ident, done, &al,
					   firstPass);
      if (s) return s;
      cnt = (al && al->first &&
	     al->first->as_coll() ?
	     al->first->as_coll()->list->cnt : 0);

      return firstPass ? reinit(db, ctx) : oqmlSuccess;
    }

    done = oqml_False;
    cnt = 0;
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlComp::optimize(Database *db, oqmlContext *ctx)
  {
    oqmlBool idx_right = oqml_False, idx_left = oqml_False;
    oqmlStatus *s;

    if (qleft->asDot()) {
      s = qleft->asDot()->hasIndex(db, ctx, idx_left);
      if (s) return s;
    }

    if (qright->asDot()) {
      s = qright->asDot()->hasIndex(db, ctx, idx_right);
      if (s) return s;
    }

    if (idx_left)
      qleft->asDot()->setIndexHint(ctx, oqml_True);

    if (idx_right)
      qright->asDot()->setIndexHint(ctx, oqml_True);
  
    if (idx_right && !idx_left)
      swap(this, qleft, qright);

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlComp::preEvalSelectRealize(Database *db, oqmlContext *ctx,
				 const char *ident, oqmlBool &done,
				 oqmlAtomList **alist,
				 oqmlBool firstPass)
  {
    assert(ctx->isSelectContext());

#if 0
    oqmlAtom *xx = 0;
    if (ident && ctx->getSymbol(ident, 0, &xx) && xx)
      printf("preEvalSelectRealize symbol %s is of type %s [%s]\n",
	     ident, xx->type.getString(), (const char *)xx->getString());
#endif
    oqmlStatus *s;
    // CE N'EST PAS LE BON TEST!
    // le bon test est:
    // if (qright->hasIdent(one of the from list!) && !needReinit)
    if (!needReinit && ctx->getSelectContext()->usesFromIdent(qright)) {
      if (firstPass) {
	s = optimize(db, ctx);
	if (s) return s;

	*alist = new oqmlAtomList();
	done = oqml_False;
	return oqmlSuccess;
      }

      s = reinit(db, ctx);
      if (s) return s;
      needReinit = oqml_True;
    }

    oqmlAtomList *al;
    oqmlAtomList *al_left = 0;
      
    oqmlNode *qright_x, *qleft_x;

    if (ident && qright->hasIdent(ident)) {
      qleft_x = qright;
      qright_x = qleft;
    }
    else {
      qleft_x = qleft;
      qright_x = qright;
    }

    s = qright_x->eval(db, ctx, &al);
    if (s) return s;
    cst_atom = al->first;
#ifdef SYNC_GARB
    if (al && !al->refcnt) {
      al->first = 0;
      OQL_DELETE(al);
    }
#endif
    s = complete(db, ctx, cst_atom);
    if (s) return s;
    s = qleft_x->eval(db, ctx, &al_left, this, cst_atom);
    if (s) return s;
#ifdef SYNC_GARB
    if (al_left && !al_left->refcnt) {
      al_left->first = 0;
      OQL_DELETE(al_left);
    }
#endif

    if (iter) {
      s = iter->eval(this, ctx, alist);
      if (s) return s;
      
      if (ctx->isOverMaxAtoms())
	return oqmlSuccess;

      if (qleft_x->asDot()) {
	s = qleft_x->asDot()->populate(db, ctx, *alist,
				       OQMLBOOL(!firstPass));

	if (s) return s;
      }

      // 8/3/00: commented the following line!
      if (firstPass) 
	evalDone = oqml_True;
      done = oqml_True;
      return oqmlSuccess;
    }
  
    // the following query come here: select class = "User"
    //assert(0);
    return new oqmlStatus(this, "invalid comparison");
  }

  oqmlStatus *
  oqmlComp::estimate(Database *, oqmlContext *, unsigned int &r)
  {
    r = oqml_ESTIM_MAX;
    return oqmlSuccess;
  }

  oqmlStatus *
  estimate_realize(Database *db, oqmlContext *ctx, oqmlNode *qleft,
		   unsigned int r0, unsigned int &r)
  {
    //printf("making estimation of %s\n", (const char *)qleft->toString());
    if (qleft->asDot()) {
      oqmlBool hasOne;
      oqmlStatus *s = qleft->asDot()->hasIndex(db, ctx, hasOne);
      if (s) return s;
      //printf("has %s index\n", hasOne ? "an" : "NO");
      if (hasOne) {
	r = r0;
	return oqmlSuccess;
      }
    }

    r = oqml_ESTIM_MAX;
    return oqmlSuccess;
  }



  oqmlStatus *
  oqmlInf::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlInfEq::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlSup::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlSupEq::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlDiff::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlRegex::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlEqual::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    /*
      printf("oqmlEqual::estimate(%s, %s)\n",
      (const char *)qleft->toString(),
      (const char *)qright->toString());
    */
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIN, r);
  }

  oqmlStatus *
  oqmlBetween::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIN, r);
  }

  oqmlStatus *
  oqmlNotBetween::estimate(Database *db, oqmlContext *ctx, unsigned int &r)
  {
    return estimate_realize(db, ctx, qleft, oqml_ESTIM_MIDDLE, r);
  }

  oqmlStatus *
  oqmlComp::compare(Database *, oqmlContext *ctx, oqmlAtomList *al_left,
		    oqmlAtomList *al_right, oqmlAtomList **alist)
  {
    oqmlBool b;
    oqmlStatus *s = al_left->compare(this, ctx, al_right, opstr, type, b);

    if (s) return s;

    if (b) {
      *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_True));
      return oqmlSuccess;
    }

    *alist = new oqmlAtomList(new oqmlAtom_bool(oqml_False));
    return oqmlSuccess;
  }

  void
  oqmlComp::lock()
  {
    oqmlNode::lock();
    if (qleft) qleft->lock();
    if (qright) qright->lock();
  }

  void
  oqmlComp::unlock()
  {
    oqmlNode::unlock();
    if (qleft) qleft->unlock();
    if (qright) qright->unlock();
  }

  oqmlStatus *
  oqmlNode::requalify(Database *, oqmlContext *, const char *ident, oqmlNode *node, oqmlBool &done)
  {
    return new oqmlStatus(this, "cannot requalify node for ident '%s', "
			  "node '%s'", ident, node->toString().c_str());
  }

  oqmlStatus *
  oqmlNode::requalify_back(Database *, oqmlContext *)
  {
    return new oqmlStatus(this, "cannot requalify back node '%s'",
			  toString().c_str());
  }

  oqmlStatus *
  oqmlNode::requalify(Database *, oqmlContext *, const Attribute **attrs, int attr_cnt,
		      const char *ident)
  {
    return new oqmlStatus(this, "cannot requalify node for ident '%s'",
			  ident);
  }

  oqmlStatus *
  oqmlNode::requalify_node(Database *db, oqmlContext *ctx,
			   oqmlNode *&ql, const char *ident, oqmlNode *node,
			   oqmlBool &done)
  {
    if (!ql)
      return oqmlSuccess;

    /*
      printf("requalifying ident '%s' to '%s' (%s)\n", ident,
      (const char *)node->toString(), (const char *)ql->toString());
    */

    if (ql->asIdent()) {
      if (!strcmp(ql->asIdent()->getName(), ident)) {
	done = oqml_True;
	node->back = ql;
	ql = node;
	if (isLocked())
	  ql->lock();
      }

      return oqmlSuccess;
    }

    ql->back = ql;

    if (ql->asDot()) {
      const char *dotident = ql->asDot()->getLeftIdent();
      if (dotident && !strcmp(dotident, ident))
	ql->asDot()->replaceLeftIdent(ident, node, done);
      return oqmlSuccess;
    }

    return ql->requalify(db, ctx, ident, node, done);
  }

  oqmlStatus *
  oqmlNode::requalify_node_back(Database *db, oqmlContext *ctx,
				oqmlNode *&ql)
  {
    /*
      if (ql)
      printf("requalifying back node %s to %s\n", (const char *)ql->toString(),
      ql->back ? (const char *)ql->back->toString() : "nil");
    */

    if (!ql || !ql->back)
      return oqmlSuccess;

    ql = ql->back;

    if (ql->asIdent())
      return oqmlSuccess;

    return ql->requalify_back(db, ctx);
  }

  oqmlStatus *
  oqmlNode::requalify_node(Database *db, oqmlContext *ctx,
			   oqmlNode *&ql, const Attribute **attrs,
			   int attr_cnt, const char *ident)
  {
    if (!ql)
      return oqmlSuccess;

    const char *xident = 0;

    if (ql->asIdent())
      xident = ql->asIdent()->getName();
    else if (ql->asDot())
      xident = ql->asDot()->getLeftIdent();
    else
      return ql->requalify(db, ctx, attrs, attr_cnt, ident);

    if (xident)
      for (int i = 0; i < attr_cnt; i++)
	if (!strcmp(xident, attrs[i]->getName())) {
	  ql = new oqmlDot(new oqmlIdent(ident), ql, oqml_False);
	  if (isLocked()) ql->lock();
	  return oqmlSuccess;
	}

    return oqmlSuccess;
  }

  std::string
  oqmlComp::toString(void) const
  {
    return oqml_binop_string(qleft, qright, opstr, is_statement);
  }

  //#define LOCK_TRACE

  void
  oqmlLock(oqmlAtomList *l, oqmlBool lock, oqmlBool rm)
  {
    if (!l || l->recurs)
      return;

    l->recurs = oqml_True;
    bool donotdel = false;
  
#ifdef LOCK_TRACE
    printf("oqmlLock list(%p, %d) -> ", l, lock);
#endif
    if (lock)
      l->refcnt++;
    else {
      if (l->refcnt > 0)
	l->refcnt--;
      else
	donotdel = true;
    }

#ifdef LOCK_TRACE
    printf("refcnt=%d\n", l->refcnt);
#endif

    oqmlAtom *a = l->first;
    while (a) {
      oqmlLock(a, lock, oqml_False);
      a = a->next;
    }
    
    l->recurs = oqml_False;

    // 8/02/06
    if (!donotdel) {
      if (!lock && rm && !l->refcnt) {
	//printf("before deleting LIST\n");
	delete l;
	//printf("after deleting LIST\n");
      }
    }
  }

  void
  oqmlLock(oqmlAtom *a, oqmlBool lock, oqmlBool rm)
  {
    if (!a || a->recurs)
      return;

    bool donotdel = false;

    a->recurs = oqml_True;

#ifdef LOCK_TRACE
    printf("oqmlLock atom(%p, %d) -> ", a, lock);
#endif
    if (lock)
      a->refcnt++;
    else {
      if (a->refcnt > 0)
	a->refcnt--;
      else
	donotdel = true;
    }
  
#ifdef LOCK_TRACE
    printf("refcnt=%d\n", a->refcnt);
#endif

    if (OQML_IS_COLL(a))
      oqmlLock(OQML_ATOM_COLLVAL(a), lock,
	       //rm && !donotdel ? oqml_True : oqml_False);
	       oqml_False);
    else if (OQML_IS_IDENT(a) && a->as_ident()->entry)
      oqmlLock(a->as_ident()->entry->at, lock,
	       //rm && !donotdel ? oqml_True : oqml_False);
	       oqml_False);
    else if (OQML_IS_STRUCT(a)) {
      oqmlAtom_struct *sa = a->as_struct();
      for (int i = 0; i < sa->attr_cnt; i++)
	oqmlLock(sa->attr[i].value, lock,
		 //rm && !donotdel ? oqml_True : oqml_False);
		 oqml_False);
    }

    a->recurs = oqml_False;

    // 8/02/06
    if (!donotdel) {
      if (!lock && rm && !a->refcnt) {
	//printf("deleting atom\n");
	delete a;
      }
    }
  }

  const char oqml_global_scope[] = "::";
  const int oqml_global_scope_len = strlen(oqml_global_scope);

  static oqmlBool
  oqml_is_global_ident(const char *ident)
  {
    return !strncmp(ident, oqml_global_scope, oqml_global_scope_len) ?
      oqml_True : oqml_False;
  }

  static oqmlAtomType st_atom_type;

  oqmlSymbolEntry::oqmlSymbolEntry(oqmlContext *ctx, const char *_ident,
				   oqmlAtomType *_type,
				   oqmlAtom *_at, oqmlBool _global,
				   oqmlBool _system)
  {
    at = 0;
    ident = strdup(_ident);
    global = _global;
    level = global ? 0 : ctx->getLocalDepth();
    set(_type, _at, oqml_True);
    prev = next = 0;
    system = _system;
    if (global && oqml_is_global_ident(ident))
      oqml_append(OQML_ATOM_COLLVAL(oqml_variables), ident);

    if (!global && ctx->getLocalTable())
      ctx->getLocalTable()->insertObjectLast(this);

    list = new oqmlAtomList();
    oqmlLock(list, oqml_True);

    popped = oqml_False;
  }

  //#define SYMB_TRACE
  //#define LOCAL_TRACE

#ifdef SYNC_SYM_GARB
  static void a2vect(oqmlAtom *a, std::vector<oqmlAtom *> &v)
  {
    if (OQML_IS_COLL(a)) {
      oqmlAtom *ta = OQML_ATOM_COLLVAL(a)->first;
      while (ta) {
	oqmlAtom *next = ta->next;
	a2vect(ta, v);
	ta = next;
      }
    }
  else
    v.push_back(a);
  }

  static void checkAutoAssign(oqmlAtom *at1, oqmlAtom *at2)
  {
    if (!at1 || !at2)
      return;

    if (!OQML_IS_COLL(at1) || !OQML_IS_COLL(at2))
      return;

    std::vector<oqmlAtom *> v1;
    std::vector<oqmlAtom *> v2;
    a2vect(at1, v1);
    a2vect(at2, v2);

    std::vector<oqmlAtom *>::iterator b1 = v1.begin();
    std::vector<oqmlAtom *>::iterator e1 = v1.end();
    while (b1 != e1) {
      std::vector<oqmlAtom *>::iterator b2 = v2.begin();
      std::vector<oqmlAtom *>::iterator e2 = v2.end();
      bool found = false;
      while (b2 != e2) {
	if ((*b1) == (*b2)) {
	  assert(!found);
	  oqmlLock(*b1, oqml_True, oqml_False);
	  found = true;
	  // should done a insert
	}
	++b2;
      }
      ++b1;
    }
  }
#endif

#ifdef SYNC_SYM_GARB_1
  static bool possibleAutoAssign(oqmlAtom *at1, oqmlAtom *at2)
  {
    return at1 && at2 && ((OQML_IS_COLL(at1) && OQML_IS_COLL(at2)) ||
			  at1 == at2);
  }
#endif

  void oqmlSymbolEntry::set(oqmlAtomType *_type, oqmlAtom *_at, oqmlBool force,
			    oqmlBool tofree)
  {
#ifdef SYMB_TRACE
    printf("setting symbol '%s' to %p, [old=%p] force %d\n",
	   ident, _at, at, force);
#endif

    type = (_type ? *_type : st_atom_type);
    if (_at || force) {
#ifdef SYNC_SYM_GARB_1
      if (at) {
	if (tofree && possibleAutoAssign(_at, at))
	  tofree = oqml_False;
	oqmlLock(at, oqml_False, tofree); // 8/02/06
      }

#elif defined(SYNC_SYM_GARB)
      if (at) {
	if (tofree)
	  checkAutoAssign(_at, at);
	oqmlLock(at, oqml_False, tofree); // 8/02/06
      }
#else
      if (at) {
	oqmlLock(at, oqml_False);
      }
#endif
      at = _at;
      oqmlLock(at, oqml_True);
    }
  }

  void oqmlSymbolEntry::addEntry(oqmlAtom_ident *x)
  {
    list->append(x);
  }

  void oqmlSymbolEntry::releaseEntries()
  {
    oqmlAtom *x = list->first;
    while(x) {
      x->as_ident()->releaseEntry();
      x = x->next;
    }
  }

  oqmlSymbolEntry::~oqmlSymbolEntry()
  {
#ifdef SYMB_TRACE
    printf("deleting entry %s %p\n", ident, this);
#endif
    if (global && oqml_is_global_ident(ident))
      oqml_suppress(OQML_ATOM_COLLVAL(oqml_variables), ident);
    releaseEntries();

    oqmlLock(list, oqml_False);

#ifdef SYNC_GARB
    OQL_DELETE(list);
#endif
    //    printf("~oqmlSymbolEntry(%s)\n", ident);
    oqmlLock(at, oqml_False);
    free(ident);
  }

  oqmlContext::oqmlContext(oqmlSymbolTable *_symtab)
  {
    symtab = (_symtab ? _symtab : &stsymtab);
    dot_ctx = 0;
    and_list_ctx = 0;
    select_ctx_cnt = 0;
    hidden_select_ctx_cnt = 0;
    and_ctx = 0;
    or_ctx = 0;
    preval_ctx = 0;
    where_ctx = 0;
    lastTempSymb = 0;
    overMaxAtoms = oqml_False;
    maxatoms = oqml_INFINITE;
    oneatom = oqml_False;
    arg_level = 0;
    local_cnt = 0;
    local_alloc = 0;
    locals = 0;
    is_popping = oqml_False;
    cpatom_cnt = 0;
    for (int n = 0; n < OQML_MAX_CPS; n++)
      cpatoms[n] = 0;
  }

  // -----------------------------------------------------------------------
  //
  // private symbol management methods.
  //
  // -----------------------------------------------------------------------

  oqmlStatus *
  oqmlContext::pushLocalTable()
  {
#if defined(SYMB_TRACE) || defined(LOCAL_TRACE)
    printf("pushLocalTable(level=#%d)\n", local_cnt);
#endif
    if (local_cnt >= local_alloc) {
      local_alloc += 64;
      locals = (LinkedList **)
	realloc(locals, local_alloc * sizeof(LinkedList *));
    }

    locals[local_cnt++] = new LinkedList();
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlContext::popLocalTable()
  {
    assert(local_cnt > 0);

    LinkedListCursor c(locals[local_cnt-1]);
    oqmlSymbolEntry *symb;
    oqmlStatus *s;

#if defined(SYMB_TRACE) || defined(LOCAL_TRACE)
    printf("popLocalTable(level=#%d) starting\n", local_cnt-1);
#endif
    is_popping = oqml_True;
    while (c.getNext((void *&)symb)) {
      assert(!symb->global);
      if (symb->popped)
	delete symb;
      else if (symb->ident) {
	s = popSymbolRealize(symb->ident, oqml_False);
	if (s) return s;
      }
    }

    is_popping = oqml_False;
#if defined(SYMB_TRACE) || defined(LOCAL_TRACE)
    printf("popLocalTable(level=#%d) ending\n", local_cnt-1);
#endif
    delete locals[local_cnt-1];
    local_cnt--;
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlContext::setSymbolRealize(const char *ident, oqmlAtomType *type,
				oqmlAtom *at, oqmlBool global,
				oqmlBool system, oqmlBool tofree)
  {
    oqmlSymbolEntry *s = (global ? symtab->sfirst : symtab->slast);

#ifdef SYMB_TRACE
    printf("setSymbolRealize(\"%s\", %s, level=#%d)\n", ident,
	   (global ? "global" : "local"), local_cnt-1);
#endif
    // 29/12/99
    // I guess that this loop should occurs if and only if symbol
    // is global!
    while (s) {
      // changed test the 19/06/01
      /*
	if (!strcmp(s->ident, ident) && s->global == global &&
	s->level == local_cnt)
      */
      if (!strcmp(s->ident, ident) &&
	  (s->global ||
	   (!global && !s->global && s->level == local_cnt))) {
	if (s->system && !system)
	  return new oqmlStatus("'%s' is a system variable: "
				"it cannot be modified.", ident);
	
	s->set(type, at, oqml_True, tofree);

	return oqmlSuccess;
      }
      
      s = (global ? s->next : s->prev);
    }
  
    return pushSymbolRealize(ident, type, at, global, system);
  }

#ifdef SYMB_TRACE
  void oqml_stop_here() { }
#endif

  oqmlStatus *
  oqmlContext::pushSymbolRealize(const char *ident, oqmlAtomType *type,
				 oqmlAtom *at, oqmlBool global,
				 oqmlBool system)
  {
#ifdef SYMB_TRACE
    printf("pushSymbolRealize(\"%s\", %s, type=%d:%d, value=%s, level=#%d)\n",
	   ident,
	   (global ? "global" : "local"), (type ? type->type : -1),
	   (at ? at->type.type : -1), (at ? at->getString() : "<nil>"),
	   local_cnt-1);
    if (at && OQML_IS_OID(at) && OQML_ATOM_OIDVAL(at).getDbid() == 572)
      oqml_stop_here();
#endif
    oqmlSymbolEntry *s = new oqmlSymbolEntry(this, ident, type, at, global, system);

    if (symtab->slast) {
      symtab->slast->next = s;
      s->prev = symtab->slast;
      symtab->slast = s;
    }
    else
      symtab->slast = symtab->sfirst = s;

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlContext::popSymbolRealize(const char *ident, oqmlBool global)
  {
    oqmlSymbolEntry *s = symtab->slast;

#ifdef SYMB_TRACE
    printf("popSymbolRealize(\"%s\", %s, level=#%d, %p)\n", ident,
	   (global ? "global" : "local"), local_cnt-1, this);
#endif

    while (s) {
      if (!strcmp(s->ident, ident) && s->global == global &&
	  (s->global || s->level == local_cnt)) {
#ifdef SYNC_SYM_GARB_2
	oqmlLock(s->at, oqml_False, oqml_True);
	s->at = 0;
#endif
	if (s->prev)
	  s->prev->next = s->next;
	if (s->next)
	  s->next->prev = s->prev;

	if (s == symtab->slast)
	  symtab->slast = s->prev;
	if (s == symtab->sfirst)
	  symtab->sfirst = s->next;

	if (getLocalTable() && !is_popping)
	  s->popped = oqml_True;
	else
	  delete s;
	return oqmlSuccess;
      }
      s = s->prev;
    }

    assert(0);
    return new oqmlStatus(oqml_uninit_fmt, ident);
  }

  // -----------------------------------------------------------------------
  //
  // public symbol management methods.
  //
  // -----------------------------------------------------------------------

  oqmlStatus *
  oqmlContext::setSymbol(const char *ident, oqmlAtomType *type, oqmlAtom *at,
			 oqmlBool global, oqmlBool system)
  {
    oqmlBool tofree = oqml_True;
    if (global) {
      oqmlStatus *s;
      if (oqml_is_global_ident(ident)) {
	s = setSymbolRealize(&ident[oqml_global_scope_len], type, at,
			     oqml_True, system, oqml_True);
	if (s) return s;
	tofree = oqml_False;
      }
      else if (!getLocalTable()) {
	s = setSymbolRealize((std::string(oqml_global_scope) + ident).c_str(), type, at,
			     oqml_True, system, oqml_True);
	if (s) return s;
	tofree = oqml_False;
      }
      else
	global = oqml_False;
    }
    
    return setSymbolRealize(ident, type, at, global, system, oqml_True);
  }

  oqmlStatus *
  oqmlContext::pushSymbol(const char *ident, oqmlAtomType *type, oqmlAtom *at,
			  oqmlBool global, oqmlBool system)
  {
    if (global) {
      oqmlStatus *s;
      if (oqml_is_global_ident(ident)) {
	s = pushSymbolRealize(&ident[oqml_global_scope_len], type, at,
			      oqml_True, system);
	if (s) return s;
      }
      else if (!getLocalTable()) {
	s = pushSymbolRealize((std::string(oqml_global_scope) + ident).c_str(), type, at,
			      oqml_True, system);
	if (s) return s;
      }
      else
	global = oqml_False;
    }

    return pushSymbolRealize(ident, type, at, global, system);
  }

  oqmlStatus *
  oqmlContext::popSymbol(const char *ident, oqmlBool global)
  {
    if (global) {
      oqmlStatus *s;
      if (oqml_is_global_ident(ident)) {
	s = popSymbolRealize(&ident[oqml_global_scope_len], oqml_True);
	if (s) return s;
      }
      else if (!getLocalTable()) {
	s = popSymbolRealize((std::string(oqml_global_scope) + ident).c_str(),
			     oqml_True);
	if (s) return s;
      }
      else
	global = oqml_False;
    }

    return popSymbolRealize(ident, global);
  }

  oqmlBool
  oqmlContext::getSymbol(const char *ident, oqmlAtomType *type, oqmlAtom **at,
			 oqmlBool *global, oqmlBool *system)
  {
    oqmlSymbolEntry *s = getSymbolEntry(ident);

    if (!s)
      return oqml_False;

    if (type)
      *type = s->type;
    if (at)
      *at = s->at;
    if (global)
      *global = s->global;

    return oqml_True;
  }

  oqmlSymbolEntry *
  oqmlContext::getSymbolEntry(const char *ident)
  {
#ifdef SYMB_TRACE  
    printf("getSymbol(%s arg_level %d local_cnt %d)\n",
	   ident, arg_level, local_cnt);
#endif
    oqmlSymbolEntry *s = symtab->slast;

    while (s) {
#ifdef SYMB_TRACE
      printf("s->ident %s global %d level %d\n",
	     s->ident, s->global, s->level);
#endif
      if (!strcmp(s->ident, ident) &&
	  (s->global || s->level == local_cnt ||
	   (arg_level && s->level < local_cnt))) {
#ifdef SYMB_TRACE  
	printf("symbol %s found\n", ident);
#endif
	return s;
      }

      s = s->prev;
    }

#ifdef SYMB_TRACE  
    printf("symbol %s *not* found\n", ident);
#endif
    return 0;
  }

  void
  oqmlContext::displaySymbols()
  {
    oqmlSymbolEntry *s = symtab->slast;

    while (s) {
      printf("%s [%d, value=%s, %s]\n",
	     s->ident, s->type.type, (s->at ? s->at->getString() : ""),
	     s->global ? "global" : "local");
      s = s->prev;
    }
  }

  void oqmlContext::setDotContext(oqmlDotContext *pc)
  {
    dot_ctx = pc;
  }

  oqmlDotContext *oqmlContext::getDotContext()
  {
    return dot_ctx;
  }

  void oqmlContext::setAndContext(oqmlAtomList *al)
  {
    and_list_ctx = al;
  }

  oqmlAtomList *oqmlContext::getAndContext()
  {
    return and_list_ctx;
  }

  std::string
  oqmlContext::makeTempSymb(int idx)
  {
    return std::string("__oqml__tmp__var__") + str_convert((long long)idx) + "__";
  }

  std::string
  oqmlContext::getTempSymb()
  {
    return makeTempSymb(lastTempSymb++);
  }

  // changed incr and decr SelectContext the 9/may/99

  void 
  oqmlContext::incrSelectContext(oqmlSelect *select)
  {
    assert(select_ctx_cnt < sizeof(select_ctx)/sizeof(select_ctx[0]));
    select_ctx[select_ctx_cnt++] = select;
  }

  void
  oqmlContext::decrSelectContext()
  {
    assert(select_ctx_cnt > 0);
    select_ctx_cnt--;
  }

  void 
  oqmlContext::incrHiddenSelectContext(oqmlSelect *select)
  {
    assert(hidden_select_ctx_cnt < sizeof(hidden_select_ctx)/sizeof(hidden_select_ctx[0]));
    hidden_select_ctx[hidden_select_ctx_cnt++] = select;
  }

  void
  oqmlContext::decrHiddenSelectContext()
  {
    assert(hidden_select_ctx_cnt > 0);
    hidden_select_ctx_cnt--;
  }

  oqmlStatus *
  oqmlContext::pushCPAtom(oqmlNode *node, oqmlAtom *a)
  {
    if (cpatom_cnt >= OQML_MAX_CPS)
      return new oqmlStatus(node, "maximum joins (%d) exceeded", OQML_MAX_CPS);

    cpatoms[cpatom_cnt++] = a;
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlContext::popCPAtom(oqmlNode *node)
  {
    if (!cpatom_cnt)
      return new oqmlStatus(node, "internal error: cannot pop joins");
    cpatom_cnt--;
    return oqmlSuccess;
  }

  oqmlContext::~oqmlContext()
  {
    for (int i = 0; i < lastTempSymb; i++)
      popSymbol(makeTempSymb(i).c_str());

    for (int j = 0; j < local_cnt; j++)
      delete locals[j];

    free(locals);
  }

  oqmlAtom *
  oqmlAtomList::getAtom(unsigned int idx)
  {
    oqmlAtom *a = first;

    for (int _cnt = 0; _cnt < idx && a; _cnt++)
      a = a->next;

    return a;
  }

  static oqmlStatus *
  checkRecursion(oqmlNode *node, oqmlAtomList *list, oqmlAtomList *this_list)
  {
    if (list == this_list)
      return new oqmlStatus(node, "invalid recursive assignation in list.");

    oqmlStatus *s;
    oqmlAtom *a = list->first;
    while (a) {
      if (a->as_list() && (s = checkRecursion(node, a->as_list()->list, this_list)))
	return s;
      a = a->next;
    }

    return oqmlSuccess;
  }

  static oqmlBool
  set_atom(oqmlAtom *olda, oqmlAtom *newa)
  {
    if (olda == newa)
      return oqml_True;

    if (olda) {
      for (int i = 0; i < olda->refcnt; i++)
	oqmlLock(newa, oqml_True);

      oqmlLock(olda, oqml_False);
    }

    return oqml_False;
  }

  oqmlStatus *
  oqmlAtomList::setAtom(oqmlAtom *ia, int idx, oqmlNode *node)
  {
    if (ia && ia->as_list()) {
      oqmlStatus *s = checkRecursion(node, ia->as_list()->list, this);
      if (s) return s;
    }

    if (idx == 0) {
      if (!ia) {
	if (last == first)
	  last = first = first->next;
	else
	  first = first->next;
	cnt--;
	return oqmlSuccess;
      }

      if (set_atom(first, ia))
	return oqmlSuccess;

      if (last == first)
	last = ia;

      oqmlAtom *next = first->next;
      first = ia;
      ia->next = next;
      return oqmlSuccess;
    }

    oqmlAtom *a = first;
    for (int i = 0; i < idx-1; i++)
      a = a->next;

    oqmlAtom *next = a->next;

    if (!ia) {
      if (last == next) {
	a->next = 0;
	last = a;
      }
      else
	a->next = next->next;

      cnt--;
      return oqmlSuccess;
    }

    if (set_atom(next, ia))
      return oqmlSuccess;

    a->next = ia;

    if (last == next) {
      ia->next = 0;
      last = ia;
    }
    else
      ia->next = next->next;

    return oqmlSuccess;
  }

  oqmlBool oqmlAtomList::isIn(oqmlAtom *x)
  {
    //const char *sx = x->getString();
    oqmlAtom *a = first;

    while (a) {
      /*
	if (!strcmp(sx, a->getString()))
	return oqml_True;
      */
      if (a->isEqualTo(*x))
	return oqml_True;
      a = a->next;
    }

    return oqml_False;
  }

  oqmlStatus *oqmlAtomList::suppress(oqmlAtom *x)
  {
    const char *sx = x->getString();
    oqmlAtom *a = first, *prev = 0;

    while (a) {
      if (!strcmp(sx, a->getString())) {
	if (prev)
	  prev->next = a->next;
	else
	  first = a->next;
	  
	if (a == last)
	  last = prev;

	return oqmlSuccess;
      }

      prev = a;
      a = a->next;
    }

    return new oqmlStatus("atom %s not found in list", x->getString());
  }

  void oqmlAtomList::empty()
  {
    cnt = 0;
    first = last = 0;
  }

  oqmlAtomList *
  oqmlAtomList::copy()
  {
    oqmlAtomList *list = new oqmlAtomList();
    oqmlAtom *a = first;

    while (a) {
      if (a->as_coll())
	list->append(a->as_coll()->list->copy());
      else
	list->append(a->copy());
      a = a->next;
    }

    return list;
  }

  oqmlAtom **
  oqmlAtomList::toArray()
  {
    oqmlAtom **arr = new oqmlAtom*[cnt];
    oqmlAtom *a = first;
    int n = 0;

    while(a) {
      arr[n++] = a;
      a = a->next;
    }

    return arr;
  }

  // flag set the 2/3/00
#define NEW_AND_OIDS

  oqmlAtomList *oqmlAtomList::andOids(oqmlAtomList *l1, oqmlAtomList *l2)
  {
#ifdef NEW_AND_OIDS
    if (l1 && OQML_IS_COLL(l1->first))
      l1 = OQML_ATOM_COLLVAL(l1->first);

    if (l2 && OQML_IS_COLL(l2->first))
      l2 = OQML_ATOM_COLLVAL(l2->first);

    if (!l1) return l2;
    if (!l2) return l1;

    oqmlAtom *aleft, *aright;
    oqmlAtomList *alist = new oqmlAtomList();

    aleft = l1->first;
    while (aleft) {
      if (OQML_IS_OID(aleft)) {
	Oid oid = OQML_ATOM_OIDVAL(aleft);
	aright = l2->first;
	while (aright) {
	  oqmlAtom *next = aright->next;
	  if (OQML_IS_OID(aright))
	    if (oid.compare(OQML_ATOM_OIDVAL(aright)))
	      alist->append(aright);
	  aright = next;
	}
      }
      aleft = aleft->next;
    }

    return alist;
#else
    if (!l1) return l2 ? OQML_ATOM_COLLVAL(l2->first) : 0;
    if (!l2) return OQML_ATOM_COLLVAL(l1->first);

    oqmlAtomList *ll1 = OQML_ATOM_COLLVAL(l1->first);
    if (!ll1) return OQML_ATOM_COLLVAL(l2->first);

    oqmlAtomList *ll2 = OQML_ATOM_COLLVAL(l2->first);
    if (!ll2) return ll1;

    oqmlAtom *aleft, *aright;
    oqmlAtomList *alist = new oqmlAtomList();

    aleft = ll1->first;
    while (aleft) {
      if (OQML_IS_OID(aleft)) {
	Oid oid = OQML_ATOM_OIDVAL(aleft);
	aright = ll2->first;
	while (aright) {
	  oqmlAtom *next = aright->next;
	  if (OQML_IS_OID(aright))
	    if (oid.compare(OQML_ATOM_OIDVAL(aright)))
	      alist->append(aright);
	  aright = next;
	}
      }
      aleft = aleft->next;
    }

    return alist;
#endif
  }

  oqmlNode *
  oqmlAtom_nil::toNode()
  {
    return new oqmlNil();
  }

  oqmlNode *
  oqmlAtom_null::toNode()
  {
    return new oqmlNull();
  }

  oqmlNode *
  oqmlAtom_bool::toNode()
  {
    if (b)
      return new oqmlTrue();

    return new oqmlFalse();
  }

  oqmlNode *
  oqmlAtom_oid::toNode()
  {
    return new oqmlOid(oid);
  }

  oqmlNode *
  oqmlAtom_obj::toNode()
  {
    return new oqmlObject(o, idx);
  }

  oqmlNode *
  oqmlAtom_int::toNode()
  {
    return new oqmlInt(i);
  }

  oqmlNode *
  oqmlAtom_char::toNode()
  {
    return new oqmlChar(c);
  }

  oqmlNode *
  oqmlAtom_double::toNode()
  {
    return new oqmlFloat(d);
  }

  oqmlNode *
  oqmlAtom_string::toNode()
  {
    return new oqmlString(shstr->s);
  }

  oqmlNode *
  oqmlAtom_ident::toNode()
  {
    return new oqmlIdent(shstr->s);
  }

  static oqml_List *
  make_list(oqmlAtomList *list)
  {
    oqml_List *l = new oqml_List();
    oqmlAtom *a = list->first;
    while (a) {
      l->add(a->toNode());
      a = a->next;
    }
    return l;
  }

  oqmlNode *
  oqmlAtom_list::toNode()
  {
    return new oqmlListColl(make_list(list));
  }

  oqmlNode *
  oqmlAtom_bag::toNode()
  {
    return new oqmlBagColl(make_list(list));
  }

  oqmlNode *
  oqmlAtom_set::toNode()
  {
    return new oqmlSetColl(make_list(list));
  }

  oqmlNode *
  oqmlAtom_array::toNode()
  {
    return new oqmlArrayColl(make_list(list));
  }

  oqmlNode *
  oqmlAtom_struct::toNode()
  {
    return 0;
  }

  oqmlNode *
  oqmlAtom_node::toNode()
  {
    return node;
  }

  oqmlNode *
  oqmlAtom_select::toNode()
  {
    return node;
  }

  void
  oqmlAtom_select::setCP(oqmlContext *ctx)
  {
    cpcnt = ctx->getCPAtomCount();
    if (!cpcnt) return;

    for (int i = 0; i < cpcnt; i++)
      if (!cplists[i]) cplists[i] = new oqmlAtomList();

    oqmlAtom **cpatoms = ctx->getCPAtoms();
    oqmlAtomList *rlist = list;
    if (OQML_IS_COLL(rlist->first))
      rlist = OQML_ATOM_COLLVAL(rlist->first);

    oqmlAtom *a = rlist->first;
    while (a) {
      for (int i = 0; i < cpcnt; i++)
	cplists[i]->append(cpatoms[i]->copy());
      a = a->next;
    }
  }

  void
  oqmlAtom_select::appendCP(oqmlContext *ctx)
  {
    cpcnt = ctx->getCPAtomCount();
    if (!cpcnt) return;

    oqmlAtom **cpatoms = ctx->getCPAtoms();
    for (int i = 0; i < cpcnt; i++) {
      if (!cplists[i]) cplists[i] = new oqmlAtomList();
      cplists[i]->append(cpatoms[i]->copy());
    }
  }

  // range atom
  oqmlAtom_range::oqmlAtom_range(oqmlAtom *_from, oqmlBool _from_incl,
				 oqmlAtom *_to, oqmlBool _to_incl)
  {
    type.type = oqmlATOM_RANGE;
    type.cls = 0;
    from = _from;
    from_incl = _from_incl;
    to = _to;
    to_incl = _to_incl;
    assert(from->type.type == to->type.type);
  }

  oqmlNode *
  oqmlAtom_range::toNode()
  {
    return new oqmlRange(from->toNode(), from_incl, to->toNode(), to_incl);
  }

  char *
  oqmlAtom_range::makeString(FILE *fd) const
  {
    const char *sf = from_incl ? "[" : "]";
    const char *st = to_incl   ? "]" : "[";

    if (fd) {
      fprintf(fd, sf);
      from->makeString(fd);
      fprintf(fd, ",");
      to->makeString(fd);
      fprintf(fd, st);
      return 0;
    }
    else if (string)
      return string;
    else {
      const char *f = from->makeString(fd);
      const char *t = to->makeString(fd);
      char *buf = (char *)malloc(strlen(f) + strlen(t) + 4);
      sprintf(buf, "%s%s,%s%s", sf, f, t, st);
      ((oqmlAtom *)this)->string = buf;
      return string;
    }
  }

  oqmlBool oqmlAtom_range::getData(unsigned char[], Data *val, Size&, int&,
				   const Class *) const
  {
    assert(0);
    *val = 0;
    return oqml_False;
  }

  int oqmlAtom_range::getSize() const
  {
    assert(0);
    return 0;
  }

  oqmlAtom *oqmlAtom_range::copy()
  {
    return new oqmlAtom_range(from, from_incl, to, to_incl);
  }

  oqmlBool oqmlAtom_range::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE type) const
  {
    if (type == oqmlBETWEEN)
      return OQMLBOOL(from->compare(data, len_data, isnull,
				    (from_incl ? oqmlSUPEQ : oqmlSUP)) &&
		      to->compare(data, len_data, isnull,
				  (to_incl ? oqmlINFEQ : oqmlINF)));

    return OQMLBOOL(from->compare(data, len_data, isnull,
				  (from_incl ? oqmlINF : oqmlINFEQ)) ||
		    to->compare(data, len_data, isnull,
				(to_incl ? oqmlSUP : oqmlSUPEQ)));
  }

  Value *oqmlAtom_range::toValue() const
  {
    return 0;
  }

  oqmlAtom_list *oqml_variables;
  oqmlAtom_list *oqml_functions;
  oqmlAtom_string *oqml_status;
#ifdef SUPPORT_OQLRESULT
  oqmlSymbolEntry *oqml_result_entry;
#endif
  oqmlSymbolEntry *oqml_db_entry;
  oqmlBool oqml_auto_persist = oqml_True;
  OqlCtbDatabase *oqml_default_db;

  void oqml_initialize()
  {
    oqml_variables = new oqmlAtom_list(new oqmlAtomList());
    oqmlContext ctx;

    ctx.setSymbol("oql$variables", &oqml_variables->type,
		  oqml_variables, oqml_True, oqml_True);

    oqml_functions = new oqmlAtom_list(new oqmlAtomList());
    ctx.setSymbol("oql$functions", &oqml_functions->type,
		  oqml_functions, oqml_True, oqml_True);

    oqml_status = new oqmlAtom_string("");
    ctx.setSymbol("oql$status", &oqml_status->type,
		  oqml_status, oqml_True, oqml_True);

#ifdef SUPPORT_OQLRESULT
    ctx.setSymbol("oql$result", 0, 0, oqml_True, oqml_True);
    oqml_result_entry = ctx.getSymbolEntry("oql$result");
#endif

    ctx.setSymbol("oql$db", 0, 0, oqml_True, oqml_True);
    oqml_db_entry = ctx.getSymbolEntry("oql$db");

    oqmlAtom *x;

    x = new oqmlAtom_int(oqml_ESTIM_MIN);
    ctx.setSymbol("oql$ESTIM_MIN", &x->type, x, oqml_True, oqml_True);

    x = new oqmlAtom_int(oqml_ESTIM_MIDDLE);
    ctx.setSymbol("oql$ESTIM_MIDDLE", &x->type, x, oqml_True, oqml_True);

    x = new oqmlAtom_int(oqml_ESTIM_MAX);
    ctx.setSymbol("oql$ESTIM_MAX", &x->type, x, oqml_True, oqml_True);

    x = new oqmlAtom_string(oqmlLAnd::getDefaultRule());
    ctx.setSymbol("oql$default_and_rule", &x->type, x, oqml_True, oqml_True);
  }

  static Database *curdb;

  void oqml_reinit(Database *db)
  {
    if (curdb == db)
      curdb = 0;
  }

  void oqml_initialize(Database *db)
  {
    if (!db)
      return;

    if (!db->isOQLInit()) {
      Bool isInTrans = True;
      if (!db->isInTransaction()) {
	db->transactionBegin();
	isInTrans = False;
      }

      db->setOQLInit();
      oqmlContext ctx;
      oqmlIdent::initEnumValues(db, &ctx);
      (void)OQL::initDatabase(db);
      if (!isInTrans)
	db->transactionAbort();
    }

    if (curdb != db) {
      const Class *clsdb = db->getSchema()->getClass("database");
      if (clsdb) {
	OqlCtbDatabase *tdb;

	if (!oqml_default_db) {
	  tdb = new OqlCtbDatabase(db);
	  tdb->setDbname(db->getName());
	  tdb->setDbid(db->getDbid());
	  tdb->setDbmdb(db->getDBMDB());
	  tdb->xdb = db;
	  db->setOQLInfo(tdb);
	}	    
	else
	  tdb = oqml_default_db;

	oqmlAtom *adb = oqmlObjectManager::registerObject(tdb);
	oqml_db_entry->set(&adb->type, adb, oqml_True);
      }

      curdb = db;
    }
  }

  void oqml_release()
  {
  }

  oqmlBool
  oqml_suppress(oqmlAtomList *list, const char *ident)
  {
    oqmlAtom *prev = 0;
    oqmlAtom *r = list->first;

    while (r) {
      if (r->as_ident() && !strcmp(OQML_ATOM_IDENTVAL(r), ident)) {
	if (!prev)
	  list->first = r->next;
	else
	  prev->next = r->next;

	if (list->last == r)
	  list->last = prev;

	//printf("suppressed! %s\n", ident);
	oqmlLock(r, oqml_False);
	return oqml_True;
      }

      prev = r;
      r = r->next;
    }

    return oqml_False;
  }

  void
  oqml_append(oqmlAtomList *list, const char *ident)
  {
    oqmlAtom *x = new oqmlAtom_ident(ident);
    oqmlLock(x, oqml_True);
    list->append(x);
  }

  oqmlStatus *
  oqml_manage_postactions(Database *db, oqmlStatus *s, oqmlAtomList **alist)
  {
    // changed the 21/01/03 :
    // added :
    if (s) return s;

    // suppressed :
    /*
      if (!s)
      oqmlStatus::purge();
    */
    // end of change

    // DISCONNECTED the 19/06/01 because of memory bugs!
    // reconnected the 21/01/03 ... 
#if !defined(SUPPORT_OQLRESULT) && !defined(SUPPORT_POSTACTIONS)
    //  printf("warning oqml_manage_postactions disconnected\n");
    return s;
#endif

    oqmlAtom *ra = s || !*alist ? 0 : (*alist)->first;

    // disconnected the 21/05/01 cecause of an obscur memory bug.
#if 0
    if (!ra)
      oqmlLock(oqml_result_entry->at, oqml_False);
#endif

#ifdef SUPPORT_OQLRESULT
    oqml_result_entry->set((ra ? &ra->type : 0), ra, oqml_True);
#endif

#ifdef SUPPORT_POSTACTIONS
    oqmlBool global;
    oqmlAtom *action_list;
    oqmlContext ctx;

    if (!ctx.getSymbol("oql$postactions", 0, &action_list, &global) || !global)
      return s;

    if (!OQML_IS_LIST(action_list))
      return new oqmlStatus("postactions: list expected in oql$postactions, "
			    "got %s.", action_list->type.getString());

    oqmlAtomList *list = OQML_ATOM_LISTVAL(action_list);
    oqmlAtom *action = list->first;
    oqmlAtom_string *statstr = new oqmlAtom_string(s ? s->msg : "");

    while (action) {
      oqmlAtom *next = action->next;

      if (!OQML_IS_IDENT(action))
	return new oqmlStatus("postactions: ident expected in "
			      "oql$postactions list, "
			      "got %s.", action->type.getString());

      oqmlStatus *rs;
      rs = oqml_realize_postaction(db, &ctx, OQML_ATOM_IDENTVAL(action),
				   statstr, ra, alist);
      if (rs) return rs;
      action = next;
    }

#endif
    return oqmlSuccess;
  }

  void
  oqmlObjectManager::addToFreeList(Object *o)
  {
    freeList.insertObjectFirst(o);
  }

  void
  oqmlObjectManager::releaseObject(Object *o, oqmlBool inFreeList)
  {
    if (!o)
      return;

    if (inFreeList)
      freeList.deleteObject(o);
    o->release();
  }

  void
  oqmlObjectManager::garbageObjects()
  {
    LinkedListCursor c(freeList);
    Object *o;

    while (c.getNext((void *&)o))
      o->release();

    //printf("garbaging freelist -> cnt=%d\n", freeList.getCount());
    freeList.empty();
  }

  oqmlStatus *
  oqmlObjectManager::getObject(oqmlNode *node, Database *db,
			       const Oid *oid, Object *&o,
			       oqmlBool add_to_free_list,
			       oqmlBool errorIfNull)
  {
    if (!oid->isValid()) {
      if (errorIfNull)
	return new oqmlStatus(node, "invalid null oid");
      o = 0;
      return oqmlSuccess;
    }
  
    Status is = db->loadObject(oid, &o);
  
    if (is)
      return new oqmlStatus(node, is);

    if (add_to_free_list)
      addToFreeList(o);
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlObjectManager::getObject(oqmlNode *node, Database *db,
			       const Oid &oid, Object *&o,
			       oqmlBool add_to_free_list,
			       oqmlBool errorIfNull)
  {
    return getObject(node, db, &oid, o, add_to_free_list, errorIfNull);
  }

  static Oid IDX2OID(pointer_int_t idx)
  {
    Oid oid;
    memcpy(&oid, &idx, sizeof(idx));
    //printf("IDX2OID %u.%d.%d %lld\n", oid.getNX(), oid.getDbid(), oid.getUnique(), idx);
    return oid;
  }

  static Oid OBJ2OID(const Object *o)
  {
    Oid oid;
    memcpy(&oid, &o, sizeof(o));
    //printf("OBJ2OID %u.%d.%d %llx\n", oid.getNX(), oid.getDbid(), oid.getUnique(), o);
    return oid;
  }

  oqmlStatus *
  oqmlObjectManager::getObject(oqmlNode *node, const char *s,
			       Object *&o, pointer_int_t &idx)
  {
    if (sscanf(s, "%lx:obj", &idx) != 1)
      return new oqmlStatus(node, "invalid object format '%s'", s);

    if (!idx) {
      o = 0;
      return oqmlSuccess;
    }

    o = (Object *)objCacheIdx->getObject(IDX2OID(idx));
    if (!o)
      return new oqmlStatus(node, "invalid object '%s'", s);
    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlObjectManager::getIndex(oqmlNode *node, const Object *o,
			      pointer_int_t &idx)
  {
    if (!o) {
      idx = 0;
      return oqmlSuccess;
    }

    idx = (pointer_int_t)objCacheObj->getObject(OBJ2OID(o));

    if (!idx)
      return new oqmlStatus(node, "invalid object '0x%lx:obj'", o);

    return oqmlSuccess;
  }

  oqmlStatus *oqmlAtom_obj::checkObject()
  {
    pointer_int_t i;

    if (oqmlObjectManager::isRegistered(o, i)) {
      //printf("check object %o %lld ok\n", o, i);
      return oqmlSuccess;
    }


#if 0
    oqmlStatus *s = new oqmlStatus(0, "object released %lx -> set to null'", o);
    o = 0;
    return s;
#else
    //printf("setting %lx to null\n", o);
    o = 0;
    return oqmlSuccess;
#endif
  }

  oqmlBool
  oqmlObjectManager::isRegistered(const Object *o, pointer_int_t &idx)
  {
    if (!o)
      return oqml_True;

    idx = (pointer_int_t)objCacheObj->getObject(OBJ2OID(o));
    return OQMLBOOL(idx);
  }

  struct OnRelease : public gbxObject::OnRelease {

    static OnRelease *instance;

    static OnRelease *getInstance() {
      if (!instance)
	instance = new OnRelease();
      return instance;
    }

    virtual void perform(gbxObject *o) {
#ifdef GARB_TRACE_DETAIL
      printf("OQL releasing %p %s\n", o, ((Object *)o)->getOid().toString());
#endif
      oqmlStatus *s = oqmlObjectManager::unregisterObject(0, (Object *)o, true);
    }
  };

  OnRelease *OnRelease::instance;

  oqmlAtom *
  oqmlObjectManager::registerObject(Object *o)
  {
    if (!o)
      return new oqmlAtom_obj(o, 0);

    pointer_int_t idx = (pointer_int_t)objCacheObj->getObject(OBJ2OID(o), true);
#ifdef GARB_TRACE_DETAIL
    std::cout << "REGISTER -> " << (void *)o << " " << idx << '\n';
#endif
    if (idx) {
      // to increase reference count
      (void)objCacheIdx->getObject(IDX2OID(idx), true);
      return new oqmlAtom_obj(o, idx, o->getClass());
    }

    static pointer_int_t stidx = 1000;

#ifdef GARB_TRACE_DETAIL
    std::cout << "REGISTER: " << (void *)o << " " << stidx << " " << OBJ2OID(o).getNX() << '\n';
#endif
    objCacheIdx->insertObject(IDX2OID(stidx), o);
    objCacheObj->insertObject(OBJ2OID(o), (void *)stidx);

    o->setOnRelease(OnRelease::getInstance());

    return new oqmlAtom_obj(o, stidx++, o->getClass());
  }

  oqmlStatus *
  oqmlObjectManager::unregisterObject(oqmlNode *node, Object *o, bool force)
  {
    if (!o)
      return oqmlSuccess;

    static const char fmt1[] = "object '%p' is not registered #1";
    static const char fmt2[] = "object '%p' is not registered #2";
    static const char fmt3[] = "object '%p' is not registered #3";
    pointer_int_t idx = (pointer_int_t)objCacheObj->getObject(OBJ2OID(o));

#ifdef GARB_TRACE_DETAIL
    std::cout << "UNREGISTER: " << (void *)o << " " << idx << " " << OBJ2OID(o).getNX() << '\n';
#endif
    if (!idx)
      return new oqmlStatus(node, fmt1, o);

    if (!objCacheObj->deleteObject(OBJ2OID(o), force))
      return new oqmlStatus(node, fmt2, o);

    if (!objCacheIdx->deleteObject(IDX2OID(idx), force))
      return new oqmlStatus(node, fmt3, o);

    return oqmlSuccess;
  }

  oqmlStatus *
  oqmlObjectManager::getObject(oqmlNode *node, Database *db,
			       oqmlAtom *x, Object *&o,
			       oqmlBool add_to_free_list,
			       oqmlBool errorIfNull)
  {
    if (x) {
      if (OQML_IS_OID(x))
	return getObject(node, db, OQML_ATOM_OIDVAL(x), o, add_to_free_list,
			 errorIfNull);

      if (OQML_IS_OBJ(x)) {
	OQL_CHECK_OBJ(x);
	o = OQML_ATOM_OBJVAL(x);
	if (o)
	  o->incrRefCount();
	return oqmlSuccess;
      }
    }
    
    return oqmlStatus::expected(node, "oid or object", x->type.getString());
  }

  void
  OqlCtbDatabase::userInitialize()
  {
    xdb = 0;
  }

  void
  OqlCtbDatabase::userCopy(const Object &)
  {
    xdb = 0;
  }

  void
  OqlCtbConnection::userInitialize()
  {
    conn = 0;
  }

  void
  OqlCtbConnection::userCopy(const Object &)
  {
    conn = 0;
  }

  void
  OqlCtbDatafile::userInitialize()
  {
    xdatfile = 0;
  }

  void
  OqlCtbDatafile::userCopy(const Object &)
  {
    xdatfile = 0;
  }

  void
  OqlCtbDataspace::userInitialize()
  {
    xdataspace = 0;
  }

  void
  OqlCtbDataspace::userCopy(const Object &)
  {
    xdataspace = 0;
  }

  Value *oqmlAtom::toValue() const
  {
    return 0;
  }

  Value *oqmlAtom_null::toValue() const
  {
    return new Value(True, True);
  }

  Value *oqmlAtom_bool::toValue() const
  {
    return new Value((Bool)b);
  }

  Value *oqmlAtom_oid::toValue() const
  {
    return new Value(oid);
  }

  Value *oqmlAtom_obj::toValue() const
  {
    return new Value(const_cast<const Object *>(o), idx);
  }

  Value *oqmlAtom_int::toValue() const
  {
    return new Value(i);
  }

  Value *oqmlAtom_char::toValue() const
  {
    return new Value(c);
  }

  Value *oqmlAtom_double::toValue() const
  {
    return new Value(d);
  }

  Value *oqmlAtom_string::toValue() const
  {
    return new Value(shstr->s);
  }

  Value *oqmlAtom_ident::toValue() const
  {
    return new Value(shstr->s, True);
  }

  Value::Type
  oqmlAtom_coll::getValueType() const
  {
    return Value::tNil;
  }

  Value::Type
  oqmlAtom_list::getValueType() const
  {
    return Value::tList;
  }

  Value::Type
  oqmlAtom_set::getValueType() const
  {
    return Value::tSet;
  }

  Value::Type
  oqmlAtom_array::getValueType() const
  {
    return Value::tArray;
  }

  Value::Type
  oqmlAtom_bag::getValueType() const
  {
    return Value::tBag;
  }

  Value *oqmlAtom_coll::toValue() const
  {
    LinkedList *l = new LinkedList();
    oqmlAtom *x = list->first;

    while (x) {
      l->insertObjectLast(x->toValue());
      x = x->next;
    }

    return new Value(l, getValueType());
  }

  Value *oqmlAtom_struct::toValue() const
  {
    Value::Struct *stru = new Value::Struct(attr_cnt);

    for (int i = 0; i < attr_cnt; i++)
      stru->attrs[i] = new Value::Attr(attr[i].name,
				       attr[i].value->toValue());

    return new Value(stru);
  }

  static oqmlStatus *
  oqml_get_location_db(Database *db, oqmlContext *ctx, oqmlNode *location,
		       oqmlAtom *a, Database *&xdb)
  {
    Object *o = 0;
    oqmlStatus *s = oqmlObjectManager::getObject(location, db, a, o, oqml_True, oqml_True);
    if (s) return s;

    if (strcmp(o->getClass()->getName(), "database"))
      return new oqmlStatus(location, "database object expected, got object "
			    "of class '%s'", o->getClass()->getName());

    OqlCtbDatabase *tdb = (OqlCtbDatabase *)o;

    if (!tdb->xdb)
      return new oqmlStatus(location, "database is not opened");
  
    xdb = tdb->xdb;
    return oqmlSuccess;
  }

  oqmlStatus *oqml_get_location(Database *&db, oqmlContext *ctx,
				oqmlNode *location, oqmlBool *mustDeferred)
  {
    if (mustDeferred) *mustDeferred = oqml_False;

    if (!location)
      return oqmlSuccess;

    oqmlStatus *s;
    oqmlAtomList *al;

    s = location->compile(db, ctx);
    if (s) return s;

    s = location->eval(db, ctx, &al);
    if (s) return s;

    // should be there ?
    if (!al->cnt && mustDeferred) {
      *mustDeferred = oqml_True;
      return oqmlSuccess;
    }

    // or there ?
    if (!al->cnt || !OQML_IS_OBJECT(al->first)) {
      // kludge for old databases!
      if (location->getType() == oqmlIDENT &&
	  !strcmp(((oqmlIdent *)location)->getName(), "oql$db"))
	return oqmlSuccess;
      return new oqmlStatus(location,
			    (std::string("database expected")+
			     (al->first ? std::string(", got ") + al->first->type.getString() : std::string(""))).c_str());
    }
#if 1
    return oqml_get_location_db(db, ctx, location, al->first, db);
#else
    Object *o = 0;
    s = oqmlObjectManager::getObject(location, db, al->first, o,
				     oqml_True, oqml_True);
    if (s) return s;

    if (strcmp(o->getClass()->getName(), "database"))
      return new oqmlStatus(location, "database object expected, got object "
			    "of class '%s'", o->getClass()->getName());

    OqlCtbDatabase *tdb = (OqlCtbDatabase *)o;

    if (!tdb->xdb)
      return new oqmlStatus(location, "database is not opened");
  
    db = tdb->xdb;

    return oqmlSuccess;
#endif
  }

  oqmlStatus *
  oqml_get_locations(Database *db, oqmlContext *ctx,
		     oqmlNode *location, Database *xdb[], int &xdb_cnt)
  {
    xdb_cnt = 0;
    if (!location) {
      xdb[xdb_cnt++] = db;
      return oqmlSuccess;
    }

    oqmlStatus *s;
    oqmlAtomList *al;

    s = location->compile(db, ctx);
    if (s) return s;

    s = location->eval(db, ctx, &al);
    if (s) return s;

    if (!al->cnt)
      return new oqmlStatus(location,
			    (std::string("database expected")+
			     (al->first ? std::string(", got ") + al->first->type.getString() : std::string(""))).c_str());

    // or there ?
    if (OQML_IS_OBJECT(al->first)) {
      oqmlStatus *s = oqml_get_location_db(db, ctx, location, al->first, xdb[xdb_cnt]);
      if (!s) xdb_cnt++;
#ifdef SYNC_GARB
      if (al && !al->refcnt) {
	al->first = 0;
	OQL_DELETE(al);
      }
#endif
      return s;
    }

    if (OQML_IS_COLL(al->first)) {
      oqmlAtom *a = OQML_ATOM_COLLVAL(al->first)->first;
      while (a) {
	oqmlStatus *s = oqml_get_location_db(db, ctx, location, a, xdb[xdb_cnt]);
	if (s) return s;
	xdb_cnt++;
	a = a->next;
      }
#ifdef SYNC_GARB
      if (al && !al->refcnt) {
	al->first = 0;
	OQL_DELETE(al);
      }
#endif
      return oqmlSuccess;
    }

    // kludge for old databases!
    if (location->getType() == oqmlIDENT &&
	!strcmp(((oqmlIdent *)location)->getName(), "oql$db")) {
      return oqmlSuccess;
    }

    return new oqmlStatus(location,
			  (std::string("database expected")+
			   (al->first ? std::string(", got ") + al->first->type.getString() : std::string(""))).c_str());
  }

  void stop_in_1()
  {
  }

  void stop_in_2()
  {
  }

  oqmlBool oqmlAtom::isEqualTo(oqmlAtom &atom)
  {
    assert(0);
    return oqml_False;
  }

  oqmlBool oqmlAtom_nil::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_nil());
  }

  oqmlBool oqmlAtom_null::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_null());
  }

  oqmlBool oqmlAtom_bool::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_bool() && OQML_ATOM_BOOLVAL(&atom) == OQML_ATOM_BOOLVAL(this));
  }

  oqmlBool oqmlAtom_oid::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_oid() && OQML_ATOM_OIDVAL(&atom) == oid);
  }

  oqmlBool oqmlAtom_obj::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_obj() && OQML_ATOM_OBJVAL(&atom) == o);
  }

  oqmlBool oqmlAtom_int::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_int() && OQML_ATOM_INTVAL(&atom) == i);
  }

  oqmlBool oqmlAtom_char::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_char() && OQML_ATOM_CHARVAL(&atom) == c);
  }

  oqmlBool oqmlAtom_double::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_double() && OQML_ATOM_DBLVAL(&atom) == d);
  }

  oqmlBool oqmlAtom_string::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_string() &&
		    !strcmp(OQML_ATOM_STRVAL(&atom), OQML_ATOM_STRVAL(this)));
  }

  oqmlBool oqmlAtom_ident::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_ident() &&
		    !strcmp(OQML_ATOM_IDENTVAL(&atom), OQML_ATOM_IDENTVAL(this)));
  }

  oqmlBool oqmlAtom_range::isEqualTo(oqmlAtom &atom)
  {
    return OQMLBOOL(atom.as_ident() &&
		    !strcmp(OQML_ATOM_IDENTVAL(&atom), OQML_ATOM_IDENTVAL(this)));
  }

  oqmlBool oqmlAtom_coll::isEqualTo(oqmlAtom &atom)
  {
    if (atom.type.type != type.type)
      return oqml_False;
    return list->isEqualTo(*OQML_ATOM_COLLVAL(&atom));
  }

  oqmlBool oqmlAtom_struct::isEqualTo(oqmlAtom &atom)
  {
    if (!atom.as_struct() || atom.as_struct()->attr_cnt != attr_cnt)
      return oqml_False;

    for (int i = 0; i < attr_cnt; i++)
      if (!attr[i].value->isEqualTo(*atom.as_struct()->attr[i].value))
	return oqml_False;

    return oqml_True;
  }

  oqmlBool oqmlAtomList::isEqualTo(oqmlAtomList &list)
  {
    if (list.cnt != cnt)
      return oqml_False;

    oqmlAtom *a = list.first;
    oqmlAtom *x = first;
    while (a) {
      if (!a->isEqualTo(*x))
	return oqml_False;
      a = a->next;
      x = x->next;
    }

    return oqml_True;
  }

  int BR_CND;
  int GB_COUNT;

  oqmlBool
  oqml_is_symbol(oqmlContext *ctx, const char *sym)
  {
    oqmlAtom *x = 0;
    return (ctx->getSymbol(sym, 0, &x) && x && 
	    ((OQML_IS_INT(x) && OQML_ATOM_INTVAL(x)) ||
	     (OQML_IS_BOOL(x) && OQML_ATOM_BOOLVAL(x)))) ? oqml_True : oqml_False;
  }

  void break_now() { }
}
