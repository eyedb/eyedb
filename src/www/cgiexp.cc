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


#include "eyedbcgiP.h"
#include <eyedblib/strutils.h>

//#define TRACE

using namespace std;

#define MAXDEPTH 256
const int idbW_member = -1;

//
// class idbWPathDesc
//

idbWPathDesc::idbWPathDesc()
{
  depth = 0;
  items = new Item[MAXDEPTH];
}

void
idbWPathDesc::init()
{
  release();
  items = new Item[MAXDEPTH];
  expand = True;
}

void
idbWPathDesc::release()
{
  delete[] items;
  depth = 0;
  items = 0;
}

Bool
idbWPathDesc::isExpanded() const
{
  if (!depth)
    return expand;

  //  trace();
#ifdef TRACE
  printf("\n");
#endif

  return idbW_is_expanded(items[depth-1].clsnum, items[depth-1].itemnum);
}

void
idbWPathDesc::push(int clsnum, int itemnum, int ind)
{
  assert(depth < MAXDEPTH);
  items[depth++].set(clsnum, itemnum, ind);
}

void
idbWPathDesc::pop()
{
  --depth;
}

int
idbWPathDesc::operator==(const idbWPathDesc &path_desc)
{
  if (depth != path_desc.depth)
    return 0;

  for (int i = 0; i < depth; i++)
    if (items[i].clsnum != path_desc.items[i].clsnum ||
	items[i].itemnum != path_desc.items[i].itemnum ||
	items[i].ind != path_desc.items[i].ind)
      return 0;

  return 1;
}

#define idbWSEP "&"
#define idbWEXP_TRUE  "t"
#define idbWEXP_FALSE "f"

std::string
idbWPathDesc::build() const
{
  std::string s = (expand ? idbWEXP_TRUE idbWSEP : idbWEXP_FALSE idbWSEP);
  s += str_convert(depth);
  
  for (int i = 0; i < depth; i++)
    {
      s += idbWSEP;
      s += str_convert((long)items[i].clsnum);
      s += idbWSEP;
      s += str_convert((long)items[i].itemnum);
      s += idbWSEP;
      s += str_convert(items[i].ind);
    }

  return s + idbWSEP;
}

void
idbWPathDesc::trace() const
{
  /*
  printf(" %s depth=%d", expand ? idbWEXP_TRUE : idbWEXP_FALSE, depth);
  for (int i = 0; i < depth; i++)
    printf(" %s %s %d", items[i].clsname, items[i].item, items[i].ind);
    */
}

idbWPathDesc::~idbWPathDesc()
{
  delete[] items;
}

//
// class idbWPathDesc::Item
//

idbWPathDesc::Item::Item()
{
  clsnum = -1;
  itemnum = -1;
}

void
idbWPathDesc::Item::set(int _clsnum, int _itemnum, int _ind)
{
  clsnum = _clsnum;
  itemnum = _itemnum;
  ind = _ind;
}

idbWPathDesc::Item::~Item()
{
}

//
// class idbWExpandContext
//

idbWExpandContext::idbWExpandContext()
{
  path_cnt = 0;
  paths = 0;
  isBuilt = False;
  built_path = 0;
}

void
idbWExpandContext::release()
{
  for (int i = 0; i < path_cnt; i++)
    paths[i].release();

  delete [] paths;

  free(built_path); built_path = 0;

  paths = 0;
  path_cnt = 0;
  isBuilt = False;
}

int
idbWExpandContext::init(int &i, int argc, char *argv[])
{
  delete [] paths;
  int oi = i;
  isBuilt = False;

  free(built_path); built_path = 0;

  path_cnt = atoi(argv[i++]);
#ifdef TRACE
  printf("expand context init path_cnt=%d\n", path_cnt);
#endif
  paths = new idbWPathDesc[path_cnt];

  int n;
  for (n = 0; n < path_cnt; n++)
    {
      const char *s = argv[i++];
      if (!strcmp(s, idbWEXP_TRUE))
	paths[n].expand = True;
      else
	paths[n].expand = False;
      int depth = atoi(argv[i++]);
#ifdef TRACE
      printf("depth = %d\n", depth);
#endif
      assert(i+3*(depth-1)+2 < argc);
      for (int k = 0; k < depth; k++)
	paths[n].push(atoi(argv[i+3*k]), atoi(argv[i+3*k+1]),
		      atoi(argv[i+3*k+2]));
      i += depth*3;
    }

  path_cnt = n;
  i--;
#ifdef TRACE
  printf("oi %d, i = %d, argc = %d\n", oi, i, argc);
  trace();
#endif
  return 1;
}

void
idbWExpandContext::trace() const
{
  printf("-expand %d", path_cnt);
  for (int j = 0; j < path_cnt; j++)
    paths[j].trace();
  printf("\n");
}

std::string
idbWExpandContext::getBuiltPath()
{
  if (isBuilt)
    return built_path;

  string s = "";
  for (int i = 0; i < path_cnt; i++)
    s += paths[i].build();
  
  free(built_path);
  built_path = strdup(s.c_str());

  isBuilt = True;
  return built_path;
}

#define etc() \
  (idbW_ctx->verbmod == idbWVerboseFull ? idbWSEP "-verbfull" : "")

std::string
idbWExpandContext::build(const idbWDumpContext &dump_ctx, Bool &is_expanded)
{
  if (!dump_ctx.rootobj)
    {
      std::string s = std::string(idbWSEP) +  "-expand" + idbWSEP +
	str_convert(path_cnt) + idbWSEP;
#ifdef TRACE
      printf("no root obj : %s\n", (const char *)s);
#endif
      return s + getBuiltPath();
    }

  std::string s = dump_ctx.rootobj->getOid().toString();
  s += idbWSEP "-expand" idbWSEP;
  int which = -1;
  int cnt;
  if (isExpanded(dump_ctx, &which))
    {
      is_expanded = True;
      if (which >= 0)
	{
	  cnt = path_cnt-1;
	  if (cnt <= 0)
	    {
	      s += "0";
#ifdef TRACE
	      printf("build1 '%s' [%s]\n", (const char *)(s+etc()));
#endif
	      return s + etc();
	    }
	  s += str_convert(cnt);
	}
      else
	{
	  cnt = path_cnt+1;
	  s += str_convert(cnt);
	  s += idbWSEP;
	  Bool expand = dump_ctx.path.expand;
	  const_cast<idbWDumpContext &>(dump_ctx).path.expand = False;
	  s += dump_ctx.path.build();
	  const_cast<idbWDumpContext &>(dump_ctx).path.expand = expand;
	}
    }
  else
    {
      is_expanded = False;
      which = -1;
      cnt = path_cnt+1;
      s += str_convert(cnt);
      s += idbWSEP;
      s += dump_ctx.path.build();
    }

  s += idbWSEP;

  if (which >= 0)
    {
      for (int i = 0; i < path_cnt; i++)
	if (i != which)
	  s += paths[i].build();
    }
  else
    s += getBuiltPath();

#ifdef TRACE
  printf("build '%s\n", (const char *)(s+etc()));
#endif
  return s + etc();
}

Bool
idbWExpandContext::isExpanded(const idbWDumpContext &dump_ctx,
			      int *which) const
{
  // for now
  //return dump_ctx.items[dump_ctx.depth-1]->isIndirect();

#ifdef TRACE
  printf("idbWExpandContext::isExpanded : ");
  dump_ctx.trace();
#endif

  for (int i = 0; i < path_cnt; i++)
    if (paths[i] == dump_ctx.path)
      {
#ifdef TRACE
	printf("%d-eme path is equal to dump_ctx\n", i);
#endif
	if (which)
	  *which = i;
	return paths[i].expand;
      }

  return dump_ctx.path.isExpanded();
}

//
// class idbWDumpContext
//

idbWDumpContext::idbWDumpContext()
{
  rootobj = 0;
}

void idbWDumpContext::init(Object *_rootobj)
{
  rootobj = (_rootobj ? _rootobj->clone() : 0);
  path.init();
}

void idbWDumpContext::trace() const
{
  printf("idbWDumpContext ");
  if (!rootobj)
    {
      printf("nil\n");
      return;
    }

  printf("%s %s {\n",
	 rootobj->getClass()->getName(), rootobj->getOid().toString());
  path.trace();
  printf("\n}\n");
}

void
idbWDumpContext::release()
{
  if (rootobj)
    rootobj->release();
}

idbWDumpContext::~idbWDumpContext()
{
  release();
}
