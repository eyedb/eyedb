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


#ifndef _EYEDB_USER_DATA_HT_H
#define _EYEDB_USER_DATA_HT_H

namespace eyedb {

  class UserDataHT {

    unsigned int nkeys;
    unsigned int mask;
    LinkedList **lists;

    int get_key(const char *s) {
      int len = strlen(s);
      int k = 0;

      for (int i = 0; i < len; i++)
	k += *s++;

      return k & mask;
    }

    struct Link {
      char *s;
      void *x;

      Link(const char *_s, void *_x) : s(strdup(_s)), x(_x) {}

      ~Link() {
	free(s);
      }
    };

  public:
    UserDataHT() {
      nkeys = 64;
      mask = nkeys - 1;

      lists = (LinkedList **)malloc(sizeof(LinkedList *) * nkeys);
      memset(lists, 0, sizeof(LinkedList *)*nkeys);
    }

    void insert(const char *s, void *x) {
      Link *l = new Link(s, x);
      int k = get_key(s);
  
      if (!lists[k])
	lists[k] = new LinkedList();

      lists[k]->insertObjectLast(l);
    }

    void getall(LinkedList *&key_list, LinkedList *&data_list) {
      key_list = new LinkedList();
      data_list = new LinkedList();
      for (int i = 0; i < nkeys; i++) {
	LinkedList *list = lists[i];
	if (list) {
	  Link *l;
	  LinkedListCursor c(list);
	  while (c.getNext((void *&)l)) {
	    key_list->insertObjectLast(l->s);
	    data_list->insertObjectLast(l->x);
	  }
	}
      }
    }

    void display(FILE *fd = stdout) {
      fprintf(fd, "HashTable KEYS %d\n", nkeys);
      for (int i = 0; i < nkeys; i++)
	if (lists[i])
	  fprintf(fd, "lists[%d] = %d\n", i, lists[i]->getCount());
    }

    void *get(const char *s) {
      LinkedList *list;

      if (list = lists[get_key(s)])
	{
	  Link *l;
	  LinkedListCursor c(list);
	  while (c.getNext((void *&)l))
	    if (!strcmp(l->s, s))
	      return l->x;
	}
    
      return 0;
    }

    ~UserDataHT() {
      for (int i = 0; i < nkeys; i++)
	{
	  LinkedList *list = lists[i];
	  if (list)
	    {
	      Link *l;
	      LinkedListCursor c(list);
	      while (c.getNext((void *&)l))
		delete l;
	    
	      delete list;
	    }
	}
    
      free(lists);
    }
  };

}

#endif
