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

//
// Method cache utils
//

static ArgArray WNullArgs;

struct MethodItem {
  const char *name;
  Class *cls;
  Signature *sign;
  Method *mth;
};

static inline
Method *getMethodInCache(const char *name, Class *cls,
			   Signature *sign)
{
  if (!idbW_dbctx)
    return (Method *)0;

  LinkedListCursor c(&idbW_dbctx->methodList);

  MethodItem *item;
  while (idbW_dbctx->methodList.getNextObject(&c, (void *&)item))
    {
      if (item->name == name &&
	  item->cls == cls &&
	  item->sign == sign)
	return item->mth;
    }

  return (Method *)0;
}

static inline
void insertMethodInCache(Method *mth, const char *name,
			 Class *cls, Signature *sign)
{
  if (mth && idbW_dbctx)
    {
      MethodItem *item = new MethodItem();
      item->name = name;
      item->cls = cls;
      item->sign = sign;
      item->mth = mth;
      idbW_dbctx->methodList.insertObject(item);
    }
}

void
idbWEmptyMethodCache()
{
  LinkedListCursor c(&idbW_dbctx->methodList);

  MethodItem *item;

  while (idbW_dbctx->methodList.getNextObject(&c, (void *&)item))
    delete item;

  idbW_dbctx->methodList.empty();
}

//
// GetMethods
//

static Method *
getMethod(const Class *cls, const char *name, Signature *sign)
{
  Method *mth;
  Status status;

  if (mth = getMethodInCache(name, (Class *)cls, sign))
    return mth;

  if (status = Method::get(idbW_ctx->db, (Class *)cls, name,
			      sign, False, mth))
    {
      idbW_STPR(status);
      return 0;
    }

  insertMethodInCache(mth, name, (Class*)cls, sign);

  return mth;
}

static Signature *WDumpSignature;
static Signature *WGetTagSignature;
static Signature *WSetDumpHintsSignature;

void
idbWMethodInit()
{
  WDumpSignature = new Signature();

  WDumpSignature->setNargs(1);

  WDumpSignature->getRettype()->setType((ArgType_Type)(OUT_ARG_TYPE|VOID_TYPE));

  WDumpSignature->setNargs(1);
  WDumpSignature->setTypesCount(1);
  WDumpSignature->getTypes(0)->setType((ArgType_Type)(IN_ARG_TYPE|ANY_TYPE));

  WGetTagSignature = new Signature();

  WGetTagSignature->getRettype()->setType((ArgType_Type)(OUT_ARG_TYPE|STRING_TYPE));

  WSetDumpHintsSignature = new Signature();

  WSetDumpHintsSignature->setNargs(1);

  WSetDumpHintsSignature->getRettype()->setType((ArgType_Type)(OUT_ARG_TYPE|VOID_TYPE));

  WSetDumpHintsSignature->setNargs(1);
  WSetDumpHintsSignature->setTypesCount(1);
  WSetDumpHintsSignature->getTypes(0)->setType((ArgType_Type)(IN_ARG_TYPE|ANY_TYPE));
}

static Method *
getWDumpMethod(Object *o)
{
  return getMethod(o->getClass(), "WDump", WDumpSignature);
}

Method *
getWGetTagMethod(Object *o)
{
  return getMethod(o->getClass(), "WGetTag", WGetTagSignature);
}

static Method *
getWSetDumpHintsMethod(Database *db)
{
  return getMethod(db->getSchema()->getClass("object"),
		   "WSetDumpHints", WSetDumpHintsSignature);
}

Status
idbWOverLoadDump(Object *o, Bool &found)
{
  Method *mth = getWDumpMethod(o);

  if (!mth)
    {
      found = False;
      return Success;
    }

  found = True;
  ArgArray array(1, Argument::NoGarbage);
  array[0]->set((void *)idbW_dest);

  Argument retarg;

  return mth->applyTo(idbW_ctx->db, o, array, retarg, False);
}

