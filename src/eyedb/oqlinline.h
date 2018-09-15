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


//#define DEST_TRACE

struct gbLink {
  oqmlAtom *at;
  oqmlAtomList *atlist;

  gbLink *prev, *next;

  inline gbLink(oqmlAtom *_at) {at = _at; atlist = 0;prev=next=0;}
  inline gbLink(oqmlAtomList *_atlist) {atlist = _atlist; at = 0;prev=next=0;}
};

//#define GB_TRACE

#ifdef GB_TRACE
extern int GB_COUNT;
extern int BR_CND;
extern void break_now();
#endif

inline gbLink *oqmlGarbManager::add_realize(gbLink *link)
{
  if (last) {
    last->next = link;
    link->prev = last;
    last = link;
  }
  else
    first = last = link;

  count++;
#ifdef GB_TRACE
  printf("creating %s link=%p ptr=%p [%d]\n",
	 link->at ? "atom" : "list",
	 link,
	 link->at ? (void *)link->at : (void *)link->atlist,
	 GB_COUNT);
  if (BR_CND && GB_COUNT == BR_CND)
    break_now();
  GB_COUNT++;
#endif
  return link;
}

inline gbLink *oqmlGarbManager::add(oqmlAtom *at)
{
  gbLink *link = new gbLink(at);
  return add_realize(link);
}

inline gbLink *oqmlGarbManager::add(oqmlAtomList *atlist)
{
  gbLink *link = new gbLink(atlist);
  return add_realize(link);
}

inline void oqmlGarbManager::add(char *s)
{
  str_list.insertObject(s);
}

extern void break_now();

inline void oqmlGarbManager::remove(gbLink *link)
{
  if (!link)
    return;

  if (!garbaging) {

    std::list<gbContext *>::iterator begin = ctx_l.begin();
    std::list<gbContext *>::iterator end = ctx_l.end();
    while (begin != end) {
      if ((*begin)->link == link)
	(*begin)->link = link->next;	
      ++begin;
    }

    if (link->prev)
      link->prev->next = link->next;
    if (link->next)
      link->next->prev = link->prev;

    if (last == link)
      last = link->prev;
    if (first == link)
      first = link->next;

    count--;
#ifdef GB_TRACE
    printf("removing %s link %p\n",
	   link->at ? "atom" : "list",
	   link->at ? (void *)link->at : (void *)link->atlist);
#endif
    delete link;
  }
#ifdef GB_TRACE
  else
    printf("NOT removing %s link %p\n",
	   link->at ? "atom" : "list",
	   link->at ? (void *)link->at : (void *)link->atlist);
#endif
}

inline oqmlAtom::oqmlAtom()
{
  type.comp = oqml_False;
  next = 0;
  refcnt = 0;
  recurs = oqml_False;
  link = oqmlGarbManager::add(this);
  string = (char *)0;
#ifdef DEST_TRACE
  printf("new atom %p\n", this);
#endif
}

inline oqmlAtom_oid::oqmlAtom_oid(const Oid &_oid, Class *_class) :
     oqmlAtom()
{
  type.type = oqmlATOM_OID;
  type.cls = _class;
  oid = _oid;
}

inline oqmlAtom_obj::oqmlAtom_obj(Object *_o, pointer_int_t _idx,
				  const Class *_class) : oqmlAtom()
{
  type.type = oqmlATOM_OBJ;
  type.cls = (Class *)_class;
  o = _o;
  idx = _idx;
  //printf("oqmlAtom_obj(%p, %p, %d)\n", this, o, idx);
}

inline oqmlAtom_bool::oqmlAtom_bool(oqmlBool _b) : oqmlAtom()
{
  type.type = oqmlATOM_BOOL;
  type.cls = 0;
  b = _b;
}

inline oqmlAtom_nil::oqmlAtom_nil() : oqmlAtom()
{
  type.type = oqmlATOM_NIL;
  type.cls = 0;
}

inline oqmlAtom_null::oqmlAtom_null() : oqmlAtom()
{
  type.type = oqmlATOM_NULL;
  type.cls = 0;
}

inline oqmlAtom_int::oqmlAtom_int(eyedblib::int64 _i) : oqmlAtom()
{
  type.type = oqmlATOM_INT;
  type.cls = 0;
  i = _i;
}

inline oqmlAtom_char::oqmlAtom_char(char _c) : oqmlAtom()
{
  type.type = oqmlATOM_CHAR;
  type.cls = 0;
  c = _c;
}

inline oqmlAtom_string::oqmlAtom_string(const char *s) : oqmlAtom()
{
  type.type = oqmlATOM_STRING;
  type.cls = 0;
  type.comp = oqml_True;

  shstr = new oqmlSharedString(s);
}

inline
oqmlSharedString::oqmlSharedString(const char *_s)
{
  refcnt = 1;
  s = strdup(_s);
  len = -1;
}

inline void
oqmlSharedString::set(const char *_s)
{
  free(s);
  s = strdup(_s);
  len = -1;
}

inline int
oqmlSharedString::getLen() const
{
  return len >= 0 ? len : (const_cast<oqmlSharedString *>(this)->len = strlen(s));
}

inline
oqmlSharedString::~oqmlSharedString()
{
  free(s);
}

inline int
oqmlAtom_string::getLen() const
{
  return shstr->getLen();
}

inline oqmlAtom_string::oqmlAtom_string(oqmlSharedString *_shstr) :
 oqmlAtom()
{
  type.type = oqmlATOM_STRING;
  type.cls = 0;
  type.comp = oqml_True;

  shstr = _shstr;
  shstr->refcnt++;
}

inline oqmlAtom_struct::oqmlAtom_struct()
{
  attr = 0;
  attr_cnt = 0;
}

inline oqmlAtom_struct::oqmlAtom_struct(oqml_StructAttr *_attr,
					int _attr_cnt)
{
  type.type = oqmlATOM_STRUCT;
  type.cls = 0;
  type.comp = oqml_False;
  attr_cnt = _attr_cnt;
  attr = _attr;
}

inline oqmlAtom_ident::oqmlAtom_ident(const char *ident,
				      oqmlSymbolEntry *_entry) : oqmlAtom()
{
  entry = _entry;
  if (entry) entry->addEntry(this);
  type.type = oqmlATOM_IDENT;
  type.cls = 0;
  type.comp = oqml_False;

  shstr = new oqmlSharedString(ident);
}

inline oqmlAtom_ident::oqmlAtom_ident(oqmlSharedString *_shstr,
				      oqmlSymbolEntry *_entry) : oqmlAtom()
{
  entry = _entry;
  if (entry) entry->addEntry(this);
  type.type = oqmlATOM_IDENT;
  type.cls = 0;
  type.comp = oqml_False;

  shstr = _shstr;
  shstr->refcnt++;
}

inline void oqmlAtom_ident::releaseEntry()
{
  entry = 0;
}

inline oqmlAtom_double::oqmlAtom_double(double _d) : oqmlAtom()
{
  type.type = oqmlATOM_DOUBLE;
  type.cls = 0;
  d = _d;
}

inline oqmlAtom_node::oqmlAtom_node(oqmlNode *_node) : oqmlAtom()
{
  node = _node;
  type.type = oqmlATOM_NODE;
  type.cls = 0;
  evaluated = oqml_False;
//  list = 0;
}

inline oqmlAtom *oqmlAtom_node::copy()
{
  return new oqmlAtom_node(node);
}

inline oqmlAtom_select::oqmlAtom_select(oqmlNode *_node, oqmlNode *_node_orig) : oqmlAtom()
{
  node = _node;
  node_orig = _node_orig;
  type.type = oqmlATOM_SELECT;
  type.cls = 0;
  evaluated = oqml_False;
  indexed = oqml_False;
  collatom = 0;
  list = 0;
  cpcnt = 0;
  for (int i = 0; i < OQML_MAX_CPS; i++)
    cplists[i] = 0;
}

inline oqmlAtom *oqmlAtom_select::copy()
{
  return new oqmlAtom_select(node, node_orig);
}

inline oqmlAtom_coll::oqmlAtom_coll(oqmlAtomList *_list) : oqmlAtom()
{
#ifdef DEST_TRACE
  printf("new coll %p\n", this);
#endif
  list = _list;
}

inline oqmlAtom_list::oqmlAtom_list(oqmlAtomList *_list) : oqmlAtom_coll(_list)
{
  type.type = oqmlATOM_LIST;
  type.cls = 0;
}

inline oqmlAtom_set::oqmlAtom_set(oqmlAtomList *_list,
				  oqmlBool _suppressDoubles) :
  oqmlAtom_coll(_list)
{
  type.type = oqmlATOM_SET;
  type.cls = 0;
  if (_suppressDoubles)
    suppressDoubles();
}

inline oqmlAtom_bag::oqmlAtom_bag(oqmlAtomList *_list) : oqmlAtom_coll(_list)
{
  type.type = oqmlATOM_BAG;
  type.cls = 0;
}

inline oqmlAtom_array::oqmlAtom_array(oqmlAtomList *_list) : oqmlAtom_coll(_list)
{
  type.type = oqmlATOM_ARRAY;
  type.cls = 0;
}

inline oqmlAtom *oqmlAtom_oid::copy()
{
  return new oqmlAtom_oid(oid, type.cls);
}

inline oqmlAtom *oqmlAtom_obj::copy()
{
  //printf("copy obj %p %p %d\n", this, o, idx);
  //return new oqmlAtom_obj(o, idx);
  return oqmlObjectManager::registerObject(o);
}

inline Object *oqmlAtom_obj::getObject()
{
  return o;
}

inline oqmlAtom *oqmlAtom_bool::copy()
{
  return new oqmlAtom_bool(b);
}

inline oqmlAtom *oqmlAtom_nil::copy()
{
  return new oqmlAtom_nil;
}

inline oqmlAtom *oqmlAtom_null::copy()
{
  return new oqmlAtom_null;
}

inline oqmlAtom *oqmlAtom_int::copy()
{
  return new oqmlAtom_int(i);
}

inline oqmlAtom *oqmlAtom_char::copy()
{
  return new oqmlAtom_char(c);
}

#define SHARE

inline oqmlAtom *oqmlAtom_string::copy()
{
#ifdef SHARE
  // 7/04/99: adding True to share the copied string
  return new oqmlAtom_string(shstr);
#else
  return new oqmlAtom_string(s);
#endif
}

inline oqmlAtom *oqmlAtom_ident::copy()
{
#ifdef SHARE
  // 7/04/99: adding True to share the copied string
  return new oqmlAtom_ident(shstr, entry);
#else
  return new oqmlAtom_ident(ident, entry);
#endif
}

inline oqmlAtom *oqmlAtom_double::copy()
{
  return new oqmlAtom_double(d);
}

inline oqmlAtom *oqmlAtom_list::copy()
{
  return new oqmlAtom_list(list);
}

inline oqmlAtom *oqmlAtom_set::copy()
{
  return new oqmlAtom_set(list, oqml_False);
}

inline oqmlAtom *oqmlAtom_bag::copy()
{
  return new oqmlAtom_bag(list);
}

inline oqmlAtom *oqmlAtom_array::copy()
{
  return new oqmlAtom_array(list);
}

inline oqmlAtom *oqmlAtom_struct::copy()
{
  return new oqmlAtom_struct(attr, attr_cnt);
}

#define __CMP__(n, o, op) \
  if (op == oqmlEQUAL) \
    return OQMLBOOL((n) == (o)); \
  if (op == oqmlINF) \
    return OQMLBOOL((n) < (o)); \
  if (op == oqmlINFEQ) \
    return OQMLBOOL((n) <= (o)); \
  if (op == oqmlSUP) \
    return OQMLBOOL((n) > (o)); \
  if (op == oqmlSUPEQ) \
    return OQMLBOOL((n) >= (o)); \
  if (op == oqmlDIFF) \
    return OQMLBOOL((n) != (o)); \
  return oqml_False

inline oqmlBool oqmlAtom_null::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  int n1 = (isnull ? 0 : 1);

  __CMP__(0, n1, op);
}

inline oqmlBool oqmlAtom_nil::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  return oqml_False;
}

inline oqmlBool oqmlAtom_bool::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  if (len_data != sizeof(oqmlBool))
    return oqml_False;

  int n1 = (int)(*(oqmlBool *)data);
  int n = (int)b;

  __CMP__(n, n1, op);
}

inline oqmlBool oqmlAtom_oid::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (len_data < sizeof(Oid))
    return oqml_False;

  Oid noid;
  memcpy(&noid, data, sizeof(Oid));

  eyedbsm::Oid::NX nnx = noid.getNX();
  eyedbsm::Oid::NX nx =  oid.getNX();
  __CMP__(nnx, nx, op);
}

inline oqmlBool oqmlAtom_obj::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (len_data < sizeof(Object *))
    return oqml_False;

  Object *no;
  memcpy(&no, data, sizeof(Object *));

  __CMP__(no, o, op);
}

inline oqmlBool oqmlAtom_int::compare(unsigned char *data, int len_data,
				      Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  if (len_data == sizeof(eyedblib::int32)+1)
    {
      eyedblib::int32 ni;
      memcpy(&ni, data, sizeof(ni));
      __CMP__(ni, i, op);
    }

  if (len_data == sizeof(eyedblib::int16))
    {
      eyedblib::int16 ni;
      memcpy(&ni, data, sizeof(ni));
      __CMP__(ni, i, op);
    }

  if (len_data == sizeof(eyedblib::int32))
    {
      eyedblib::int32 ni;
      memcpy(&ni, data, sizeof(ni));
      __CMP__(ni, i, op);
    }

  if (len_data == sizeof(eyedblib::int64))
    {
      eyedblib::int64 ni;
      memcpy(&ni, data, sizeof(ni));
      __CMP__(ni, i, op);
    }

  if (op == oqmlDIFF)
    return oqml_True;
  return oqml_False;
}

inline oqmlBool oqmlAtom_char::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  if (len_data < sizeof(char))
    return oqml_False;

  char nc = data[0];
  __CMP__(nc, c, op);
}

inline oqmlBool oqmlAtom_string::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  int cmp;
  if (len_data)
    cmp = strcmp((char *)data, shstr->s);
  else if (data)
    cmp = !*shstr->s;
  else
    cmp = !shstr->s;

  __CMP__(cmp, 0, op);
}

inline oqmlBool oqmlAtom_ident::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  if (len_data < strlen(shstr->s))
    return oqml_False;

  int cmp = strcmp((char *)data, shstr->s);
  __CMP__(cmp, 0, op);
}

inline oqmlBool oqmlAtom_double::compare(unsigned char *data, int len_data, Bool isnull, oqmlTYPE op) const
{
  if (isnull)
    {
      if (op == oqmlDIFF)
	return oqml_True;
      return oqml_False;
    }

  if (len_data < sizeof(double))
    return oqml_False;

  double nd;
  memcpy(&nd, data, sizeof(double));

  if (op == oqmlEQUAL)
    {
      double delta = nd - d;
      if (delta < 0.)
	delta = -delta;
      return OQMLBOOL(delta < .0001);
    }
  else
    __CMP__(nd, d, op);
}

inline void oqmlAtom::print(FILE *fd)
{
  (void)makeString(fd);
}

inline char *oqmlAtom::getString() const
{
  return makeString(0);
}

inline oqmlBool oqmlAtom::compare(unsigned char *, int, Bool, oqmlTYPE) const
{
  return oqml_False;
}

#include <assert.h>

inline oqmlAtom::~oqmlAtom()
{
#ifdef DEST_TRACE
  printf("deleting atom %p refcnt=%d\n", this, refcnt);
#endif
  assert(!refcnt);
  if (refcnt)
    return;
  refcnt = 32000;
  oqmlGarbManager::remove(link);
  free(string);
}

inline void oqmlAtom_string::set(const char *s)
{
  shstr->set(s);
}

inline oqmlAtom_string::~oqmlAtom_string()
{
  if (!--shstr->refcnt)
    delete shstr;
}

inline oqmlAtom_ident::~oqmlAtom_ident()
{
  if (!--shstr->refcnt)
    delete shstr;
}

inline oqmlAtom_obj::~oqmlAtom_obj()
{
  //printf("oqmlAtom_obj::~oqmlAtom_obj(%p, %p, %u)\n", this, o, idx);
  oqmlStatus *s = oqmlObjectManager::unregisterObject(0, o);
#if 0
  if (s) {
    fprintf(stderr, "~oqmlAtom_obj error: %s\n", s->msg);
    abort();
  }
#endif
  o = 0;
}

inline oqmlAtom_node::~oqmlAtom_node()
{
}

inline oqmlAtom_select::~oqmlAtom_select()
{
}

inline oqmlAtom_struct::~oqmlAtom_struct()
{
}

inline oqmlBool oqmlAtomType::cmp(const oqmlAtomType &at, Bool autocast)
{
  /*
  if (type == oqmlATOM_OBJ || at.type == oqmlATOM_OBJ)
    return oqml_False;
  */
  if (type == oqmlATOM_OBJ && at.type == oqmlATOM_OBJ) {
    if (cls == 0)
      return (oqmlBool)(at.cls == 0);
    if (at.cls == 0)
      return (oqmlBool)(cls == 0);
    return (oqmlBool)cls->compare(at.cls);
  }

  if (autocast)
    {
      if ((type == oqmlATOM_DOUBLE && at.type == oqmlATOM_INT) ||
	  (at.type == oqmlATOM_DOUBLE && type == oqmlATOM_INT))
	return oqml_True;
    }

  return OQMLBOOL(type == at.type && comp == at.comp);
}

inline oqmlAtomList::oqmlAtomList()
{
  cnt = 0;
  first = last = 0;
  link = oqmlGarbManager::add(this);
  refcnt = 0;
  recurs = oqml_False;
  string = (char *)0;
}

inline oqmlAtomList::oqmlAtomList(oqmlAtomList *alist)
{
  cnt = 0;
  first = last = 0;

  refcnt = 0;
  recurs = oqml_False;
  string = (char *)0;

  link = oqmlGarbManager::add(this);

  if (!alist)
    return;

  oqmlAtom *a = alist->first;
  while (a)
    {
      oqmlAtom *next = a->next;
      append(a->copy());
      a = next;
    }
}

inline oqmlAtomList::oqmlAtomList(oqmlAtom *a)
{
  cnt = (a ? 1 : 0);
  first = last = a;
  refcnt = 0;
  recurs = oqml_False;
  string = (char *)0;
  link = oqmlGarbManager::add(this);

  if (!a)
    return;

  a->next = 0;
}

inline oqmlAtomList::~oqmlAtomList()
{
#ifdef DEST_TRACE
  printf("deleting list %p refcnt=%d\n", this, refcnt);
#endif
  assert(!refcnt);
  if (refcnt)
    return;

  oqmlAtom *a = first;
  while (a) {
    oqmlAtom *next = a->next;
    //printf("a->refcnt %d %s\n", a->refcnt, a->getString());
    if (!a->refcnt)
      delete a;
    a = next;
  }

  cnt = 0;
  oqmlGarbManager::remove(link);
#ifdef DEST_TRACE
  printf("end of deleting list %p\n", this);
#endif
  refcnt = 64000;
  free(string);
}

inline int oqmlAtomList::getFlattenCount() const
{
  int count = 0;

  oqmlAtom *a = first;
  while (a)
    {
      if (a->type.type == oqmlATOM_LIST)
	count += ((oqmlAtom_list *)a)->list->getFlattenCount() + 2;
      else
	count++;
      a = a->next;
    }

  return count;
}

inline void oqmlAtomList::print(FILE *fd)
{
  oqmlAtom *a = first;

  fprintf(fd, "(");
  while (a)
    {
      a->print(fd);
      a = a->next;
      if (a)
	fprintf(fd, ", ");
    }
  fprintf(fd, ")");
}

inline char *oqmlAtomList::getString() const
{
  oqmlAtom *a = first;
  int sz = 1;
  char *b = (char *)malloc(sz);
  b[0] = 0;

  while (a)
    {
      char *d = a->getString();
      sz += strlen(d) + 2;
      b = (char *)realloc(b, sz);
      if (a != first)
	strcat(b, ", ");
      strcat(b, d);
      a = a->next;
    }

  delete string;
  ((oqmlAtomList *)this)->string = b;
  return b;
}

inline bool detect_cycle(oqmlAtom *a1, oqmlAtom *a2)
{
  while (a1) {
    while (a2) {
      bool r = false;

      if (a1 == a2)
	r = true;
      else if (a1->as_coll() && a2->as_coll())
	r = detect_cycle(a1->as_coll()->list->first, a2->as_coll()->list->first);
      else if (a1->as_coll())
	r = detect_cycle(a1->as_coll()->list->first, a2);

      else if (a2->as_coll())
	r = detect_cycle(a1, a2->as_coll()->list->first);

      if (r)
	return true;
      a2 = a2->next;
    }
    a1 = a1->next;
  }

  return false;
}

inline bool oqmlAtomList::append(oqmlAtom *a, bool incref, bool _detect_cycle)
{
  a->next = 0;

  if (_detect_cycle && detect_cycle(first, a))
    return true;

  if (last) {
    last->next = a;
    last = a;
  }
  else
    first = last = a;

  if (incref) {
    if (refcnt)
      oqmlLock(a, oqml_True);
    else if (a->refcnt)
      oqmlLock(this, oqml_True);
  }

  cnt++;

  return false;
}

inline bool oqmlAtomList::append(oqmlAtomList *al, oqmlBool dodel, bool _detect_cycle)
{
  if (!al)
    return false;

  if (_detect_cycle && detect_cycle(first, al->first))
    return true;

  if (refcnt)
    oqmlLock(al, oqml_True);
  else if (al->refcnt)
    oqmlLock(this, oqml_True);

  oqmlAtom *a = al->first;

  if (!a)
    return false;

  if (last)
    last->next = a;
  else
    first = a;

  last = al->last;

  cnt += al->cnt;
  if (dodel && !al->refcnt) {
    al->first = 0;
    al->cnt = 0;
    delete al;
  }

  return false;
}
