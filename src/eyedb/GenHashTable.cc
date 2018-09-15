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


#include <eyedb/eyedb.h>
#include "eyedb/GenHashTable.h"

namespace eyedb {

  GenHashTable::GenHashTable(int _pref_len, int n) : pref_len(_pref_len)
  {
    nkeys = 1;

    n >>= 3;

    for (;;)
      {
	if (nkeys >= n)
	  break;
	nkeys <<= 1;
      }

    mask = nkeys - 1;

    lists = (LinkedList **)malloc(sizeof(LinkedList *) * nkeys);
    memset(lists, 0, sizeof(LinkedList *)*nkeys);
  }

  struct GLink {
    char *name;
    int ind;

    GLink(const char *_name, int _ind) : name(strdup(_name)), ind(_ind) {}

    ~GLink() {
      free(name);
    }
  };

  inline int GenHashTable::get_key(const char *tok)
  {
    tok += pref_len;
    int len = strlen(tok);
    int k = 0;

    for (int i = 0; i < len; i++)
      k += *tok++;

    return k&mask;
  }

  void GenHashTable::insert(const char *name, int ind)
  {
    GLink *l = new GLink(name, ind);
    int k = get_key(name);
  
    if (!lists[k])
      lists[k] = new LinkedList();

    lists[k]->insertObjectLast(l);
  }

  int GenHashTable::get(const char *name)
  {
    LinkedList *list;

    if (list = lists[get_key(name)])
      {
	GLink *l;
	LinkedListCursor c(list);
	while (c.getNext((void *&)l))
	  if (!strcmp(l->name, name))
	    return l->ind;
      }

    return -1;
  }

  GenHashTable::~GenHashTable()
  {
    for (int i = 0; i < nkeys; i++)
      {
	LinkedList *list = lists[i];
	if (list)
	  {
	    GLink *l;
	    LinkedListCursor c(list);
	    while (c.getNext((void *&)l))
	      delete l;

	    delete list;
	  }
      }

    free(lists);
  }

  void
  GenHashTable::display(FILE *fd)
  {
    fprintf(fd, "HashTable KEYS %d\n", nkeys);
    for (int i = 0; i < nkeys; i++)
      if (lists[i])
	fprintf(fd, "lists[%d] = %d\n", i, lists[i]->getCount());
  }

}
