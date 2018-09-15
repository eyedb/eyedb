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

  const RecMode *FullRecurs;
  const RecMode *NoRecurs;

  const RecMode *RecMode::FullRecurs;
  const RecMode *RecMode::NoRecurs;

  void RecMode::init(void)
  {
    static const RecMode FullRecurs_rec(RecMode_FullRecurs);;
    static const RecMode NoRecurs_rec(RecMode_NoRecurs);;

    eyedb::FullRecurs = &FullRecurs_rec;
    eyedb::NoRecurs = &NoRecurs_rec;

    FullRecurs = eyedb::FullRecurs;
    NoRecurs = eyedb::NoRecurs;
  }

  void RecMode::_release(void)
  {
  }

  RecMode::RecMode(RecModeType typ)
  {
    type = typ;
    u.pred = 0;
    u.fnm.fnm_cnt = 0;
  }

  RecMode::RecMode(Bool (*p)(const Object *,
			     const Attribute *, int))
  {
    type = RecMode_Predicat;
    u.pred = p;
  }

#define SET_FNAMES(FIRST, AP) \
  va_list AP; \
  va_start(AP, FIRST); \
  char *s; \
  u.fnm.fnm_cnt = 1; \
 \
  while (s = va_arg(AP, char *)) \
    { \
      u.fnm.fnm_cnt++; \
    } \
  u.fnm.fnm = (char **)malloc(u.fnm.fnm_cnt * sizeof(char *)); \
  va_start(AP, FIRST); \
  int k = 0; \
  u.fnm.fnm[k++] = strdup(first); \
  while (s = va_arg(AP, char *)) \
    { \
      u.fnm.fnm[k++] = strdup(s); \
    } \
  va_end(AP);

  RecMode::RecMode(const char *first, ...)
  {
    type = RecMode_FieldNames;
    u.fnm._not = False;

    SET_FNAMES(first, ap);
  }

  RecMode::RecMode(Bool _not, const char *first, ...)
  {
    type = RecMode_FieldNames;
    u.fnm._not = _not;

    SET_FNAMES(first, ap);
  }

  Bool RecMode::isAgregRecurs(const Attribute *agreg, int n, const Object *o) const
  {
    if (type == RecMode_NoRecurs)
      return False;

    if (type == RecMode_FullRecurs)
      return True;

    if (type == RecMode_Predicat)
      return u.pred(o, agreg, n);

    if (type == RecMode_FieldNames)
      {
	const char *name = agreg->getName();
	for (int i = 0; i < u.fnm.fnm_cnt; i++)
	  if (!strcmp(name, u.fnm.fnm[i]))
	    return True;
     
	return False;
      }

    return False;
  }

}
