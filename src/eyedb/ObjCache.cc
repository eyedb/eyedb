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

namespace eyedb {

  static unsigned int COEF = 50;
  static unsigned int RESCALE_COEF = 2; // must be a power of 2
  static unsigned int MAX_LINKS_CNT = 16384;

  struct ObjCacheLink {
    Oid oid;
    void *o;
    unsigned int tstamp;
    unsigned int refcnt;
    ObjCacheLink *next;
    ObjCacheLink(const Oid &, void *, unsigned int);
  };

  ObjCacheLink::ObjCacheLink(const Oid &toid, void *to, unsigned int ts)
  {
    oid = toid;
    o = to;
    tstamp = ts;
    refcnt = 1;
  }

  ObjCache::ObjCache(unsigned int n)
  {
    // assume that n is a power of 2
    /*
      key = 1;

      while (n--)
      key <<= 1;

      --key;
    */
    //printf("ObjCache(%u)\n", n);
    key = n - 1;

    links_cnt = key + 1;
    links = (ObjCacheLink **)malloc(sizeof(ObjCacheLink *) * links_cnt);
    memset(links, 0, sizeof(ObjCacheLink *) * links_cnt);
    tstamp = 0;
    obj_cnt = 0;
    rescaling = false;
  }

  unsigned int ObjCache::getIndex(const Oid &oid)
  {
    return (oid.getNX() & key);
  }

  void ObjCache::rescale() {
    if (rescaling)
      return;

    char *p = (char *)sbrk(0);
    //printf("objcache %p rescaling from %u\n", this, links_cnt);
    rescaling = true;
    ObjCache *obj_cache = new ObjCache(links_cnt * RESCALE_COEF);

    for (unsigned int n = 0; n < links_cnt; n++) {
      ObjCacheLink *link = links[n];

      while (link) {
	ObjCacheLink *next = link->next;
	obj_cache->insertObject(link->oid, link->o, link->refcnt);
	delete link;
	link = next;
      }

      links[n] = 0;
    }

    free(links);

    key = obj_cache->key;
    obj_cache->key = 0;

    links_cnt = obj_cache->links_cnt;
    obj_cache->links_cnt = 0;

    obj_cnt = obj_cache->obj_cnt;
    obj_cache->obj_cnt = 0;

    links = obj_cache->links;
    obj_cache->links = 0;

    tstamp = obj_cache->tstamp;
    obj_cache->tstamp = 0;

    delete obj_cache;
    rescaling = false;
  }

  Bool ObjCache::insertObject(const Oid &oid, void *o, unsigned int _refcnt)
  {
    //  printf("insertObject %s o = %p links %p\n", oid.getString(), o, links);
    if (!_refcnt) {
      if (getObject(oid))
	return False;
    }

    if (links_cnt < MAX_LINKS_CNT && obj_cnt > COEF * links_cnt)
      rescale();

    unsigned int index = getIndex(oid);
    ObjCacheLink *ol = new ObjCacheLink(oid, o, ++tstamp);
    if (_refcnt)
      ol->refcnt = _refcnt;

    ObjCacheLink *link = links[index];

    ol->next = link;
    links[index] = ol;

    obj_cnt++;
    return True;
  }

  Bool ObjCache::deleteObject(const Oid &oid, bool force)
  {
    ObjCacheLink *prev = 0;
    ObjCacheLink *link = links[getIndex(oid)];

    //printf("deleting object\n");
    while (link) {
      if (oid.compare(link->oid)) {
	assert(link->refcnt > 0);
	if (!force) {
	  if (--link->refcnt > 0) {
	    //printf("not really\n");
	    return True;
	  }
	}

	if (prev)
	  prev->next = link->next;
	else
	  links[getIndex(oid)] = link->next;

	delete link;
	obj_cnt--;
	//printf("really\n");
	return True;
      }
      prev = link;
      link = link->next;
    }

    return False;
  }

  void *ObjCache::getObject(const Oid &oid, bool incr)
  {
    ObjCacheLink *link = links[getIndex(oid)];

    //  printf("getObject %s links %p\n", oid.getString(), links);
    while (link) {
      if (oid.compare(link->oid)) {
	//	  printf("returns %p\n", link->o);
	if (incr)
	  link->refcnt++;
	return link->o;
      }
      link = link->next;
    }

    //  printf("returns 0\n");
    return 0;
  }

  ObjectList *
  ObjCache::getObjects()
  {
    ObjectList *obj_list = new ObjectList();
    for (unsigned int n = 0; n < links_cnt; n++) {
      ObjCacheLink *link = links[n];
      while (link) {
	obj_list->insertObjectLast((Object *)link->o);
	link = link->next;
      }
    }

    return obj_list;
  }

  void ObjCache::empty(void)
  {
    for (unsigned int n = 0; n < links_cnt; n++) {
      ObjCacheLink *link = links[n];

      while (link) {
	ObjCacheLink *next = link->next;
	delete link;
	link = next;
      }

      links[n] = 0;
    }

    obj_cnt = 0;
    tstamp = 0;
  }

  ObjCache::~ObjCache()
  {
    empty();
    free(links);
  }
}
