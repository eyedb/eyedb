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
#include "Attribute_p.h"
#include <string.h>

namespace eyedb {

  static const char classSuffix[] = "_Class";
  static const char __agritems[] = "getClass(true).getAttributes()";
  static const char downCastPrefix[] = "as";

#define ATTRNAME(NAME, OPTYPE, HINTS) \
  (HINTS).style->getString(GenCodeHints::OPTYPE, NAME)

#define ATTRNAME_1(NAME, OPTYPE, HINTS) \
  (HINTS).style->getString(OPTYPE, NAME)

#define ATTRGET(CL) ((CL)->asCollectionClass() ? \
		     GenCodeHints::tGetColl : GenCodeHints::tGet)

#define ATTRSET(CL) ((CL)->asCollectionClass() ? \
		     GenCodeHints::tSetColl : GenCodeHints::tSet)
  static const char *
  getstr(const char *s)
  {
    if (!strcmp(s, int32_class_name) || !strcmp(s, "int") ||
	!strcmp(s, "eyedblib::int32"))
      return "Int";

    if (!strcmp(s, int16_class_name) || !strcmp(s, "short") ||
	!strcmp(s, "eyedblib::int16"))
      return "Short";

    if (!strcmp(s, int64_class_name) || !strcmp(s, "long") ||
	!strcmp(s, "eyedblib::int64"))
      return "Long";

    if (!strcmp(s, char_class_name))
      return "Char";

    if (!strcmp(s, "byte"))
      return "Byte";

    if (!strcmp(s, "float") || !strcmp(s, "double"))
      return "Double";

    if (!strcmp(s, "oid"))
      return "Oid";

    if (!strcmp(s, "org.eyedb.Oid"))
      return "Oid";

    fprintf(stderr, "eyedbodl: getstr cannot decode type '%s'\n",
	    s);
    abort();
  }

  static const char *
  attrName(const char *name, Bool isGet,
	   GenCodeHints::AttrStyle attr_style)
  {
    if (attr_style == GenCodeHints::ImplicitAttrStyle)
      return name;

    static char sname[256];
    strcpy(sname, (isGet ? "get" : "set"));
    char c = name[0];
    if (c >= 'a' && c <= 'z')
      c += 'A' - 'a';
    int len = strlen(sname);
  
    sname[len] = c;
    sname[len+1] = 0;
    strcat(sname, &name[1]);
  
    return sname;
  }

  static const char *
  make_int(int i)
  {
    static char x[16];
    sprintf(x, "%d", i);
    return x;
  }

  static int
  pre(char *name, const char *pre)
  {
    int len = strlen(pre);
    if (!strncmp(name, pre, len))
      {
	name[len] += 'A' - 'a';
	return 1;
      }
    return 0;
  }

  // 18/05/05 : patches

  const char *
  classNameToJavaName(const char *name);
  const char *getJavaName(const Class *cls);

#if 1
  static const char *className(const Class *cls,
			       Bool makeC = True,
			       Bool makeAlias = False)
  {
    const char *name = (makeAlias ? cls->getAliasName() : cls->getName());

    if (!strncmp(name, "set<", 4))
      return "org.eyedb.CollSet";
  
    if (!strncmp(name, "bag<", 4))
      return "org.eyedb.CollBag";
  
    if (!strncmp(name, "array<", 6))
      return "org.eyedb.CollArray";
  
    if (!strncmp(name, "list<", 5))
      return "org.eyedb.CollList";

    if (!strcmp(name, "image"))
      return "org.eyedb.utils.Image";

    if (!strcmp(name, "URL"))
      return "org.eyedb.utils.URL";

    if (!strcmp(name, "CURL"))
      return "org.eyedb.utils.CURL";

    if (!strcmp(name, "date"))
      return "org.eyedb.utils.Date";

    if (!strcmp(name, "time"))
      return "org.eyedb.utils.Time";

    if (!strcmp(name, "timestamp"))
      return "org.eyedb.utils.TimeStamp";

    if (!strcmp(name, "bool"))
      return "org.eyedb.Bool";

    for (int i = 0; i < idbLAST_Type; i++)
      if (!strcmp(name, class_info[i].name))
	return /*Class::*/classNameToJavaName(name);

    if (makeC) {
      if (!strcmp(name, char_class_name))
	return "org.eyedb.Char";

      if (!strcmp(name, int32_class_name))
	return "org.eyedb.Int32";
      
      if (!strcmp(name, int64_class_name))
	return "org.eyedb.Int64";
      
      if (!strcmp(name, int16_class_name))
	return "org.eyedb.Int16";
      
      if (!strcmp(name, "float"))
	return "org.eyedb.Float";

      if (!strcmp(name, "oid"))
	return "org.eyedb.OidP";

      if (!strcmp(name, "byte"))
	return "org.eyedb.Byte";
    }
    else {
      if (!strcmp(name, int32_class_name))
	return "int";
      
      if (!strcmp(name, int64_class_name))
	return "long";
      
      if (!strcmp(name, int16_class_name))
	return "short";
      
      if (!strcmp(name, "oid"))
	return "org.eyedb.Oid";

      if (!strcmp(name, "byte"))
	return "byte";

      if (!strcmp(name, "float"))
	return "double";
    }

    // 1/10/01: what about enums ??
    const char *sCName = Class::getSCName(name);
    return sCName ? sCName : name;
  }

#else
  static const char *className(const Class *cls,
			       Bool makeC=True,
			       Bool makeAlias=False)
  {
    const char *name = (makeAlias ? cls->getAliasName() : cls->getName());
  
    if (!strncmp(name, "set<", 4))
      return "org.eyedb.CollSet";
  
    if (!strncmp(name, "bag<", 4))
      return "org.eyedb.CollBag";
  
    if (!strncmp(name, "array<", 6))
      return "org.eyedb.CollArray";
  
    if (!strncmp(name, "list<", 5))
      return "CollList";
  
    if (!strcmp(name, "image"))
      return "Image";

    oups
    if (!strcmp(name, "URL"))
      return "URL";

    for (int i = 0; i < idbLAST_Type; i++)
      if (!strcmp(name, class_info[i].name))
	{
	  return /*Class::*/classNameToJavaName(name);
	}
  
    if (makeC)
      {
	if (!strcmp(name, char_class_name))
	  return "org.eyedb.Char";
      
	if (!strcmp(name, int32_class_name))
	  return "org.eyedb.Int32";
      
	if (!strcmp(name, int64_class_name))
	  return "org.eyedb.Int64";
      
	if (!strcmp(name, int16_class_name))
	  return "org.eyedb.Int16";
      
	if (!strcmp(name, "float"))
	  return "org.eyedb.Float";
      
	if (!strcmp(name, "oid"))
	  return "org.eyedb.OidP";
      
	if (!strcmp(name, "byte"))
	  return "org.eyedb.Byte";
      }
    else
      {
	if (!strcmp(name, "oid"))
	  return "org.eyedb.Oid";
      
	if (!strcmp(name, "byte"))
	  return "byte";
      
	if (!strcmp(name, "float"))
	  return "double";

	if (!strcmp(name, int32_class_name))
	  return "int";

	if (!strcmp(name, int16_class_name))
	  return "short";

	if (!strcmp(name, int64_class_name))
	  return "long";
      }
  
    return name;
  }
#endif

  static void dimArgsGen(FILE *fd, int ndims, Bool named = False)
  {
    for (int i = 0; i < ndims; i++)
      {
	if (i)
	  fprintf(fd, ", ");
	fprintf(fd, "int");
	if (named)
	  fprintf(fd, " a%d", i);
      }
  }

  Status Attribute::generateClassDesc_Java(GenContext *ctx)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims;
  
    fprintf(fd, "\n");
    if (ndims)
      {
	fprintf(fd, "%sdims = new int[%d];\n", ctx->get(), ndims);
	for (int i = 0; i < ndims; i++)
	  fprintf(fd, "%sdims[%d] = %d;\n", ctx->get(), i, typmod.dims[i]);
      }
    else
      fprintf(fd, "%sdims = null;\n", ctx->get());
  
    fprintf(fd, "%sattr[%d] = new org.eyedb.Attribute(", ctx->get(), num);
  
    if (cls->asCollectionClass())
      fprintf(fd, "((m != null) ? m.getClass(\"%s\") : %s.idbclass), idbclass, \"%s\", ",
	      cls->getAliasName(), getJavaName(cls), name); // 10/12/05: className(cls) instead of name : disconnected 18/01/06
    else
      fprintf(fd, "((m != null) ? m.getClass(\"%s\") : %s.idbclass), idbclass, \"%s\", ",
	      cls->getAliasName(), className(cls), name); //10/12/05: className(cls) instead of name : disconnected 18/01/06
  
    fprintf(fd, "%d, %s, %d, dims);\n",
	    num, (isIndirect() ? "true" : "false"), ndims);
  
    /* these lines should be present, but no real support for
       schema generation in java is provided */
    /*
      if (inv_spec.clsname)
      fprintf(fd, "%sattr[%d].setInverse(\"%s\", \"%s\");\n", ctx->get(), num,
      inv_spec.clsname, inv_spec.fname);
      else if (inv_spec.item)
      fprintf(fd, "%sattr[%d].setInverse(\"%s\", \"%s\");\n", ctx->get(), num,
      inv_spec.item->getClassOwner()->getName(), inv_spec.item->getName());
    */  
    return Success;
  }

  static void
  get_info(CollectionClass *mcoll, const char *&classname,
	   Bool &ordered)
  {
    if (mcoll->asCollSetClass())
      {
	classname = "org.eyedb.CollSet";
	ordered = False;
      }
    else if (mcoll->asCollBagClass())
      {
	classname = "org.eyedb.CollBag";
	ordered = False;
      }
    else if (mcoll->asCollArrayClass())
      {
	classname = "org.eyedb.CollArray";
	ordered = True;
      }
    else if (mcoll->asCollListClass())
      {
	classname = "CollList";
	ordered = True;
      }
  }

  Status
  Attribute::generateCollRealizeClassMethod_Java(Class *own,
						 GenContext *ctx,
						 const GenCodeHints &hints,
						 Bool isoid,
						 int xacctype)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    const char *comma = (ndims ? ", " : "");
    Bool _isref;
    eyedblib::int16 _dim;
    Class *cl =
      ((CollectionClass *)cls)->getCollClass(&_isref, &_dim);
    GenCodeHints::OpType acctype = (GenCodeHints::OpType)xacctype;
#if 0
    const char *etc = (acctype == GenCodeHints::tAddItemToColl ?
		       ", int mag_order" : "");
#else
    const char *etc = "";
#endif
    const char *accmth = (acctype == GenCodeHints::tAddItemToColl ? "insert" : "suppress");
    const char *oclassname = _isref ?
      className(cl, True) : className(cl, False);
  
    if (isoid && cl->asBasicClass())
      return Success;
  
    const char *cast = (cl->asBasicClass() ? "(org.eyedb.Value)" : "");
    const char *classname;
    Bool ordered;
    get_info(const_cast<CollectionClass *>(cls->asCollectionClass()), classname, ordered);

    const char *At = ((acctype == GenCodeHints::tAddItemToColl ||
		       acctype == GenCodeHints::tRmvItemFromColl)
		      && ordered ? "At" : "");
    const char *etc1 = "";
    const char *etc2 = "";
    Bool rmv_from_array = (acctype == GenCodeHints::tRmvItemFromColl)
      && ordered ? True : False;

    if (rmv_from_array && isoid)
      return Success;

    GenCodeHints::OpType nacctype;
    if (ordered)
      nacctype = (acctype == GenCodeHints::tRmvItemFromColl ?
		  GenCodeHints::tUnsetItemInColl :
		  GenCodeHints::tSetItemInColl);
    else
      nacctype = acctype;

    if (!*At)
      {
	if (acctype == GenCodeHints::tAddItemToColl)
	  {
	    etc1 = ", boolean noDup";
	    etc2 = ", noDup";
	  }
	else if (acctype == GenCodeHints::tRmvItemFromColl)
	  {
	    etc1 = ", boolean checkFirst";
	    etc2 = ", checkFirst";
	  }
      }

    if (_dim == 1)
      {
	// because Java does not support default argument:
	if (*etc1)
	  {
	    fprintf(fd, "%spublic void %s(%s%s", ctx->get(),
		    ATTRNAME_1(name, nacctype, hints),
		    (*At ? "int where" : ""),
		    (*At && (!rmv_from_array || ndims >= 1) ? ", " : ""));
	    dimArgsGen(fd, ndims);
	    if (rmv_from_array)
	      fprintf(fd, ")\n");
	    else if (isoid)
	      fprintf(fd, "%sorg.eyedb.Oid _oid)\n", comma);
	    else
	      fprintf(fd, "%s%s _%s)\n",
		      comma, oclassname, name);
	    fprintf(fd, "  throws org.eyedb.Exception {\n");
	    fprintf(fd, "%s  %s(%s",  ctx->get(),
		    ATTRNAME_1(name, nacctype, hints),
		    (*At ? "where, " : ""));
	    for (i = 0; i < ndims; i++)
	      fprintf(fd, "a%d, ", i);

	    if (isoid)
	      fprintf(fd, "%s_oid%s);\n", ((*comma || *At) ? ", " : ""), ", false");
	    else
	      fprintf(fd, "%s_%s%s);\n", ((*comma || *At) ? ", " : ""), name, ", false");
	    fprintf(fd, "%s}\n\n", ctx->get());
	  }

	fprintf(fd, "%spublic void %s(%s%s", ctx->get(),
		ATTRNAME_1(name, nacctype, hints),
		(*At ? "int where" : ""),
		(*At && (!rmv_from_array || ndims >= 1) ? ", " : ""));
	dimArgsGen(fd, ndims);
	if (rmv_from_array)
	  fprintf(fd, ")\n");
	else if (isoid)
	  fprintf(fd, "%sorg.eyedb.Oid _oid%s%s)\n", comma, etc1, etc);
	else
	  fprintf(fd, "%s%s _%s%s%s)\n", comma, oclassname, name,
		  etc1, etc);
      }
  
    fprintf(fd, "%sthrows org.eyedb.Exception {\n", ctx->get());

    ctx->push();
    fprintf(fd, "%sorg.eyedb.Value __x;\n", ctx->get());
    fprintf(fd, "%s%s _coll;\n", ctx->get(), classname);
    fprintf(fd, "%sboolean _not_set = false;\n", ctx->get());

    if (ndims > 1)
      {
	fprintf(fd, "%sint from = a%d;\n", ctx->get(), ndims-1);
	for (int i = ndims - 2 ; i >= 0; i--)
	  fprintf(fd, "%sfrom += a%d * %d;\n", ctx->get(), i, typmod.dims[i]);
      }
    else
      fprintf(fd, "%sint from = 0;\n", ctx->get());
  
    fprintf(fd, "\n%s__x = %s[%d].getValue(this, from, true);\n",
	    ctx->get(), __agritems, num, name);

    fprintf(fd, "%s_coll = (%s)__x.sgetObject();\n", ctx->get(), classname);
    //  fprintf(fd, "%sif (_coll == null) return;\n", ctx->get());

    // 8/05/01: from C++ version:
    // should create a collection with mag_order given as argument
    // ...
    fprintf(fd, "%sif (_coll == null)", ctx->get());
    ctx->push();
    if (acctype == GenCodeHints::tAddItemToColl)
      {
	fprintf(fd, " {\n");
	fprintf(fd, "%s  _coll = new %s(db, \"\", "
		"db.getSchema().getClass(\"%s\"), %s);\n",
		ctx->get(), classname,
		cl->getAliasName(), (_isref ? "true" : make_int(_dim)));
	fprintf(fd, "%s  _not_set = true;\n", ctx->get());
	//fprintf(fd, "%s   _coll.setMagOrder(mag_order);\n", ctx->get());
	ctx->pop();
	fprintf(fd, "%s}\n", ctx->get());
      }
    else {
      fprintf(fd, "\n%s  throw new org.eyedb.Exception(org.eyedb.Status.IDB_ERROR, \"invalid collection\", \"no valid collection in attribute %s::%s\");\n\n", ctx->get(), class_owner->getName(), name);
      ctx->pop();
    }

    if (rmv_from_array)
      fprintf(fd, "\n%s_coll.suppressAt(where);\n", ctx->get());
    else if (isoid)
      fprintf(fd, "\n%s_coll.%s%s(%s_oid%s);\n", ctx->get(),
	      accmth, At, (*At ? "where, " : ""), etc2);
    else
      fprintf(fd, "\n%s_coll.%s%s(%s%s_%s%s);\n", ctx->get(),
	      accmth, At, (*At ? "where, " : ""),  cast, name,
	      etc2);

    /*
      if (isoid)
      fprintf(fd, "\n%s_set.%s(_oid);\n", ctx->get(), accmth);
      else
      fprintf(fd, "\n%s_set.%s(_%s);\n", ctx->get(), name, accmth);
    */

    fprintf(fd, "%sif (!_not_set)\n%s  return;\n", ctx->get(), ctx->get());
    fprintf(fd, "%s%s[%d].setValue(this, new org.eyedb.Value(_coll), from);\n",
	    ctx->get(), __agritems, num);
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
    return Success;
  }

  Status
  Attribute::generateCollInsertClassMethod_Java(Class *own,
						GenContext *ctx,
						const GenCodeHints &hints,
						Bool isoid)
  {
    return generateCollRealizeClassMethod_Java(own, ctx, hints,
					       isoid,
					       GenCodeHints::tAddItemToColl);
  }

  Status
  Attribute::generateCollSuppressClassMethod_Java(Class *own,
						  GenContext *ctx,
						  const GenCodeHints &hints,
						  Bool isoid)
  {
    return generateCollRealizeClassMethod_Java(own, ctx, hints,
					       isoid, 
					       GenCodeHints::tRmvItemFromColl);
  }

#if 0
  Status
  Attribute_generateCollInsertClassMethod_Java_old(Class *own,
						   GenContext *ctx,
						   const GenCodeHints &hints,
						   Bool isoid)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    const char *comma = (ndims ? ", " : "");
    Bool _isref;
    eyedblib::int16 _dim;
    Class *cl =
      ((CollectionClass *)cls)->getCollClass(&_isref, &_dim);
  
    if (isoid && cl->asBasicClass())
      return Success;
  
    if (_dim == 1)
      {
	fprintf(fd, "%spublic void %s(", ctx->get(),
		attrName(name, False, hints.attr_style));
	dimArgsGen(fd, ndims);
	if (isoid)
	  fprintf(fd, "%sorg.eyedb.Oid _oid)\n", comma);
	else
	  fprintf(fd, "%s%s _%s)\n", comma, className(cl, False), name);
      }
    else if (!strcmp(cl->getName(), char_class_name))
      {
	fprintf(fd, "%s%s(", ctx->get(),
		attrName(name, False, hints.attr_style));
	dimArgsGen(fd, ndims);
	fprintf(fd, "%sString _%s)\n", comma, name);
      }
    else if (!strcmp(cl->getName(), "byte"))
      {
	fprintf(fd, "%s%s(", ctx->get(),
		attrName(name, False, hints.attr_style));
	dimArgsGen(fd, ndims);
	fprintf(fd, "%sbyte _%s[])\n", comma, name);
      }
  
    fprintf(fd, "%sthrows org.eyedb.Exception {\n", ctx->get());

    ctx->push();
    fprintf(fd, "%sorg.eyedb.Value __x;\n", ctx->get());
    fprintf(fd, "%sorg.eyedb.CollSet _set;\n", ctx->get());

    if (ndims > 1)
      {
	fprintf(fd, "%sint from = a%d;\n", ctx->get(), ndims-1);
	for (int i = ndims - 2 ; i >= 0; i--)
	  fprintf(fd, "%sfrom += a%d * %d;\n", ctx->get(), i, typmod.dims[i]);
      }
    else
      fprintf(fd, "%sint from = 0;\n", ctx->get());
  
    fprintf(fd, "\n%s__x = %s[%d].getValue(this, from, true);\n",
	    ctx->get(), __agritems, num, name);

    fprintf(fd, "%s_set = (org.eyedb.CollSet)__x.sgetObject();\n", ctx->get());
    fprintf(fd, "%sif (_set == null) return;\n", ctx->get());

    if (isoid)
      fprintf(fd, "\n%s_set.insert(_oid);\n", ctx->get());
    else
      fprintf(fd, "\n%s_set.insert(_%s);\n", ctx->get(), name);

    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
    return Success;
  }
#endif

  Status
  Attribute::generateSetMethod_Java(Class *own, GenContext *ctx,
				    const GenCodeHints &hints)
  {
    if (cls->asCollectionClass())
      {
	(void)generateCollInsertClassMethod_Java(own, ctx, hints, False);
	(void)generateCollInsertClassMethod_Java(own, ctx, hints, True);

	(void)generateCollSuppressClassMethod_Java(own, ctx, hints, False);
	(void)generateCollSuppressClassMethod_Java(own, ctx, hints, True);
      }
    return Success;
  }

#define NOT_BASIC() \
(isIndirect() || (!cls->asBasicClass() && !cls->asEnumClass()))
  
  Status
  Attribute::generateSetMethod_Java(Class *own, GenContext *ctx,
				    Bool isoid,
				    const GenCodeHints &hints)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    int not_basic = NOT_BASIC();
    const char *ref = (not_basic ? "*" : "");
    const char *comma = (ndims ? ", " : "");
  
    if (isoid)
      fprintf(fd, "%spublic void %s_oid(", ctx->get(),
	      ATTRNAME(name, tSetOid, hints));
    //	    attrName(name, False, hints.attr_style));
    else
      fprintf(fd, "%spublic void %s(", ctx->get(),
	      ATTRNAME_1(name, ATTRSET(cls), hints));
    //	    attrName(name, False, hints.attr_style));
  
    dimArgsGen(fd, ndims, True);
  
    if (isoid)
      fprintf(fd, "%sorg.eyedb.Oid _oid)\n", comma);
    else
      fprintf(fd, "%s%s _%s)\n", comma, (cls->asEnumClass() ? "int" :
					 className(cls, False)), name);
  
    fprintf(fd, "%sthrows org.eyedb.Exception {\n", ctx->get());
    ctx->push();
  
    if (ndims)
      {
	fprintf(fd, "%sint from = a%d;\n", ctx->get(), ndims-1);
	for (int i = ndims - 2 ; i >= 0; i--)
	  fprintf(fd, "%sfrom += a%d * %d;\n", ctx->get(), i, typmod.dims[i]);
      
	if (isVarDim())
	  {
	    fprintf(fd, "%sint size = %s[%d].getSize(this);\n",
		    ctx->get(), __agritems, num);
	    fprintf(fd, "%sif (size <= from)\n", ctx->get());
	    ctx->push();
	    fprintf(fd, "%s%s[%d].setSize(this, from+1);\n",
		    ctx->get(), __agritems, num);
	    ctx->pop();
	  }
      
	/*
	  if (cls->asCollectionClass())
	  printf("MUST DO SPECIAL COLLECTION CHECK for '%s'\n", name);
	*/
      
	if (isoid)
	  fprintf(fd, "%s%s[%d].setOid(this, _oid, from);\n",
		  ctx->get(), __agritems, num);
	else
	  fprintf(fd, "%s%s[%d].setValue(this, new org.eyedb.Value(_%s), from);\n",
		  ctx->get(), __agritems, num, name);
      }
    else if (isoid)
      fprintf(fd, "%s%s[%d].setOid(this, _oid, 0);\n",
	      ctx->get(), __agritems, num);
    else
      fprintf(fd, "%s%s[%d].setValue(this, new org.eyedb.Value(_%s), 0);\n",
	      ctx->get(), __agritems, num, name);
  
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    return Success;
  }

#define _GRT_WRK_

  Status
  Attribute::generateGetMethod_Java(Class *own, GenContext *ctx,
				    Bool isoid, 
				    const GenCodeHints &hints,
				    const char *_const, const char *prefix)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    int not_basic = NOT_BASIC();
    const char *ref = (not_basic ? "*" : "");
  
    if (isoid)
      fprintf(fd, "%spublic org.eyedb.Oid %s_oid(", ctx->get(),
	      ATTRNAME(name, tGetOid, hints));
    //	    attrName(name, True, hints.attr_style));
    else
      fprintf(fd, "%spublic %s %s(", ctx->get(),
	      (cls->asEnumClass() ? "int" : className(cls, False)),
	      ATTRNAME_1(name, ATTRGET(cls), hints));
    //	    attrName(name, True, hints.attr_style));
  
    dimArgsGen(fd, ndims, True);
  
    fprintf(fd, ")\n%sthrows org.eyedb.Exception {\n", ctx->get());

    ctx->push();
    int const_obj = (!isoid && not_basic /* && !cls->asCollectionClass() */);
  
    if (isoid)
      fprintf(fd, "%sorg.eyedb.Oid __x;\n", ctx->get());
    else
      {
	fprintf(fd, "%sorg.eyedb.Value __x;\n", ctx->get());
	fprintf(fd, "%sorg.eyedb.Object __y;\n", ctx->get());
      }

    if (ndims)
      {
	fprintf(fd, "%sint from = a%d;\n", ctx->get(), ndims-1);
	for (int i = ndims - 2 ; i >= 0; i--)
	  fprintf(fd, "%sfrom += a%d * %d;\n", ctx->get(), i, typmod.dims[i]);
      
	if (isoid)
	  fprintf(fd, "\n%s__x = %s[%d].getOid(this, from);\n",
		  ctx->get(), __agritems, num);
	else
	  fprintf(fd, "\n%s__x = %s[%d].getValue(this, from, true);\n",
		  ctx->get(), __agritems, num);
      }
    else if (isoid)
      fprintf(fd, "\n%s__x = %s[%d].getOid(this, 0);\n",
	      ctx->get(), __agritems, num);
    else
      {
	fprintf(fd, "\n%s__x = %s[%d].getValue(this, 0, true);\n",
		ctx->get(), __agritems, num);
	/*
	  if (isIndirect())
	  fprintf(fd, "%sif (__x == null) return null;\n", ctx->get());
	*/
      }
  
    if (const_obj)
      {
	//fprintf(fd, "%stry {\n", ctx->get());
	//ctx->push();
	fprintf(fd, "%s__y = %sDatabase.makeObject(__x.sgetObject(), true);\n",
		ctx->get(), prefix);
	fprintf(fd, "%sif (__y != __x.sgetObject())\n", ctx->get());
	ctx->push();
	fprintf(fd, "%s%s[%d].setValue(this, new org.eyedb.Value(__y), 0);\n",
		ctx->get(), __agritems, num);
	ctx->pop();
	/*
	  ctx->pop();
	  fprintf(fd, "%s}\n", ctx->get());
	  fprintf(fd, "%scatch(idbLoadObjectException e) {\n", ctx->get());
	  ctx->push();
	  fprintf(fd, "%sreturn null;\n", ctx->get());
	  ctx->pop();
	  fprintf(fd, "%s}\n", ctx->get());
	*/
	fprintf(fd, "%sreturn (%s)__y;\n", ctx->get(),
		className(cls, False));
      }
    else if (isoid)
      fprintf(fd, "%sreturn __x;\n", ctx->get(),
	      className(cls, False));
    else if (cls->asEnumClass())
      fprintf(fd, "%sreturn __x.sgetInt();\n", ctx->get());
    else
      fprintf(fd, "%sreturn __x.sget%s();\n", ctx->get(),
	      getstr(className(cls, False)));

    ctx->pop();

    fprintf(fd, "%s}\n\n", ctx->get());
  
    if (isVarDim() && *_const)
      {
	fprintf(fd, "%sint %s_cnt()\n", ctx->get(),
		attrName(name, True, hints.attr_style));
	fprintf(fd, "%s{\n", ctx->get());
	ctx->push();
	fprintf(fd, "%sreturn %s[%d].getSize(this);\n", ctx->get(),
		__agritems, num);
	ctx->pop();
	fprintf(fd, "%s}\n\n", ctx->get());
      }
  
    if (cls->asCollectionClass())
      return generateCollGetMethod_Java(own, ctx, isoid,
					hints, _const);
    return Success;
  }

  Status
  Attribute::generateCollGetMethod_Java(Class *own, GenContext *ctx,
					Bool isoid,
					const GenCodeHints &hints,
					const char *_const)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    Bool _isref;
    eyedblib::int16 _dim;
    Class *cl = const_cast<Class *>(cls->asCollectionClass()->getCollClass(&_isref, &_dim));
    int not_basic = _isref || (!cl->asBasicClass() && !cl->asEnumClass());
    const char *ref = (not_basic ? "*" : "");
    const char *classname = isIndirect() ?
      className(cls, True) : className(cls, False);
    const char *oclassname = _isref ?
      className(cl, True) : className(cl, False);
    const char *comma = (ndims ? ", " : "");

    Bool ordered = (cls->asCollArrayClass() || cls->asCollListClass()) ? True : False;
    if (isoid)
      {
	if (!_isref)
	  return Success;

	fprintf(fd, "  public org.eyedb.Oid %s(int ind%s",
		ATTRNAME_1(name, (ordered ? GenCodeHints::tRetrieveOidItemAt : GenCodeHints::tGetOidItemAt), hints), comma);
	dimArgsGen(fd, ndims, True);
	fprintf(fd, ") throws org.eyedb.Exception {\n");

	fprintf(fd, "%sorg.eyedb.Oid tmp;\n", ctx->get());
	fprintf(fd, "%sorg.eyedb.Collection coll = %s(", ctx->get(),
		ATTRNAME(name, tGetColl, hints));

	for (i = 0; i < ndims; i++)
	  fprintf(fd, "a%d, ", i);

	fprintf(fd, ");\n\n", ctx->get());
	fprintf(fd, "%sif (coll == null)\n", ctx->get());
	fprintf(fd, "%s  return null;\n\n", ctx->get());
      
	if (cls->asCollArrayClass() || cls->asCollListClass())
	  fprintf(fd, "%stmp = ((org.eyedb.CollArray)coll).retrieveOidAt(ind);\n", ctx->get());
	else
	  fprintf(fd, "%stmp = coll.getOidAt(ind);\n", ctx->get());
	fprintf(fd, "%sreturn tmp;\n", ctx->get());
	fprintf(fd, "}\n\n");
	return Success;
      }

    if (!*_const && cl->asBasicClass())
      return Success;

    if (!strcmp(cl->getName(), char_class_name) && _dim > 1)
      {
	fprintf(fd, "String %s(int ind%s",
		ATTRNAME_1(name, (ordered ? GenCodeHints::tRetrieveItemAt : GenCodeHints::tGetItemAt), hints), comma);
      }
    else if (_dim == 1)
      fprintf(fd, "  public %s %s(int ind%s",
	      oclassname,
	      ATTRNAME_1(name, (ordered ? GenCodeHints::tRetrieveItemAt : GenCodeHints::tGetItemAt), hints), comma);
    else
      return Success;

    dimArgsGen(fd, ndims, True);
    fprintf(fd, ")\n%sthrows org.eyedb.Exception {\n", ctx->get());

    ctx->push();
    fprintf(fd, "%sorg.eyedb.Collection coll = %s(", ctx->get(),
	    ATTRNAME(name, tGetColl, hints));

    for (i = 0; i < ndims; i++)
      fprintf(fd, "a%d, ", i);

    fprintf(fd, ");\n\n", ctx->get());
    fprintf(fd, "%sif (coll == null)\n", ctx->get());
    if (not_basic)
      {
	fprintf(fd, "%s  return null;\n\n", ctx->get());
	fprintf(fd, "%s%s tmp;\n", ctx->get(), oclassname);
	if (cls->asCollArrayClass() || cls->asCollListClass())
	  fprintf(fd, "%stmp = (%s)((org.eyedb.CollArray)coll).retrieveObjectAt(ind);\n", ctx->get(), oclassname);
	else
	  fprintf(fd, "%stmp = (%s)coll.getObjectAt(ind);\n", ctx->get(),
		  oclassname);
      }
    else
      {
	fprintf(fd, "%s  return;\n\n", ctx->get(), oclassname);
	fprintf(fd, "%sorg.eyedb.Value tmp;\n", ctx->get());
	if (cls->asCollArrayClass() || cls->asCollListClass())
	  fprintf(fd, "%stmp = ((org.eyedb.CollArray)coll).retrieveValueAt(ind);\n", ctx->get());
	else
	  fprintf(fd, "%stmp = coll.getValueAt(ind);\n", ctx->get());
      }

    if (not_basic)
      fprintf(fd, "%sreturn tmp;\n", ctx->get());
    else
      fprintf(fd, "%sreturn tmp.%s;\n", ctx->get(),
	      Value::getAttributeName(cl, (_isref || (_dim > 1)) ?
				      True : False));
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
    return Success;
  }

  Status Attribute::generateBody_Java(Class *own,
				      GenContext *ctx,
				      const GenCodeHints &hints,
				      const char *prefix)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    int maxdims = typmod.maxdims;
    int not_basic = NOT_BASIC();
    const char *ref = (not_basic ? "*" : "");
    int is_string = (ndims == 1 && !strcmp(cls->getName(), char_class_name));
    int is_raw    = (ndims == 1 && !strcmp(cls->getName(), "byte"));
    const char *typestr, *postfix, *fname;

    // set methods
    if (is_string)
      {
	typestr = "String";
	fname = typestr;
	postfix = "";
      }
    else if (is_raw)
      {
	typestr = "byte";
	fname = "Raw";
	postfix = "[]";
      }

    if (is_string || is_raw)
      {
	fprintf(fd, "%spublic void %s(%s _%s%s)\n",
		ctx->get(),
		attrName(name, False, hints.attr_style), typestr, name, postfix);
      
	fprintf(fd, "%sthrows org.eyedb.Exception {\n", ctx->get());
	ctx->push();
	if (isVarDim())
	  {
	    if (is_string)
	      fprintf(fd, "%sint len = _%s.length() + 1;\n\n", ctx->get(), name);
	    else
	      fprintf(fd, "%sint len = _%s.length;\n\n", ctx->get(), name);

	    fprintf(fd, "%sint size = %s[%d].getSize(this);\n", ctx->get(),
		    __agritems, num);
	    fprintf(fd, "%sif (size < len)\n", ctx->get());
	    ctx->push();
	    fprintf(fd, "%s%s[%d].setSize(this, len);\n", ctx->get(),
		    __agritems, num);
	    ctx->pop();
	    fprintf(fd, "%s%s[%d].set%sValue(this, _%s);\n",
		    ctx->get(), __agritems, num, fname, name);
	  }
	else
	  fprintf(fd, "%s%s[%d].set%sValue(this, _%s);\n",
		  ctx->get(), __agritems, num, fname, name);

	ctx->pop();
	fprintf(fd, "%s}\n\n", ctx->get());
      }
  
    generateSetMethod_Java(own, ctx, False, hints);
  
    // get methods
    if (is_string || is_raw)
      {
	fprintf(fd, "%spublic %s%s %s()\n", ctx->get(),
		typestr, postfix,
		attrName(name, True, hints.attr_style));
	fprintf(fd, "%sthrows org.eyedb.Exception {\n", ctx->get());
	ctx->push();
	fprintf(fd, "%sreturn %s[%d].get%sValue(this);\n",
		ctx->get(), __agritems, num, fname);
	ctx->pop();
	fprintf(fd, "%s}\n\n", ctx->get());
      }
    else
      generateGetMethod_Java(own, ctx, False, hints, "const ", prefix);
  
    if (isIndirect())
      {
	generateSetMethod_Java(own, ctx, True, hints);
	generateGetMethod_Java(own, ctx, True, hints, "", prefix);
      }
  
    generateSetMethod_Java(own, ctx, hints);
  
    return Success;
  }


  Status Attribute::generateCode_Java(Class *own,
				      GenContext *ctx,
				      const GenCodeHints &hints,
				      const char *prefix)
  {
    FILE *fd = ctx->getFile();
    int ndims = typmod.ndims, i;
    int not_basic = NOT_BASIC();
    const char *ref1 = (not_basic ? "*" : "");
    const char *ref2 = (not_basic ? " *" : " ");
    const char *comma = (ndims ? ", " : "");
  
    if (cls->asCollectionClass())
      {
	fprintf(fd, "%spublic int %s(", ctx->get(),
		ATTRNAME(name, tGetCount, hints));
	dimArgsGen(fd, ndims);
	fprintf(fd, ") throws org.eyedb.Exception {\n");
	ctx->push();
	fprintf(fd, "%sorg.eyedb.Collection _coll = %s(", ctx->get(), ATTRNAME(name, tGetColl, hints));
	for (i = 0; i < ndims; i++)
	  fprintf(fd, "a%d, ", i);
	fprintf(fd, ");\n");
	fprintf(fd, "%sreturn (_coll != null ? _coll.getCount() : 0);\n",
		ctx->get());
	ctx->pop();
	fprintf(fd, "%s}\n\n", ctx->get());
      }

    // set method
    generateBody_Java(own, ctx, hints, prefix);
  
    return Success;
  }

  static void const_f0(FILE *fd, Class *parent, const char *var,
		       Bool fill = False)
  {
    if (!strcmp(parent->getName(), "struct"))
      fprintf(fd, "super(%s);", var);
    else if (!strcmp(parent->getName(), "union"))
      fprintf(fd, "super(%s);", var);
    else
      fprintf(fd, "super(%s, 1);", var);
  
    fprintf(fd, "\n");
  }

  static void const_f01(FILE *fd, Class *parent, const char *var,
			Bool fill = False)
  {
    if (!strcmp(parent->getName(), "struct"))
      fprintf(fd, "super(%s, share);", var);
    else if (!strcmp(parent->getName(), "union"))
      fprintf(fd, "super(%s, share);", var);
    else
      fprintf(fd, "super(%s, share, 1);", var);
  
    fprintf(fd, "\n");
  }

  static void const_f1(FILE *fd, GenContext *ctx, const char *name, Bool share)
  {
    fprintf(fd, "%sheaderCode(org.eyedb.ObjectHeader._Struct_Type, "
	    "idr_psize, org.eyedb.ObjectHeader.IDB_XINFO_LOCAL_OBJ, true);\n",
	    ctx->get());
  
    if (share)
      fprintf(fd, "%sif (!share)\n", ctx->get());
  
    fprintf(fd, "%s%sgetClass(true).newObjRealize(getDatabase(), this);\n", ctx->get(), (share ? "  " : ""));
    fprintf(fd, "%ssetGRTObject(true);\n", ctx->get());
  }

  Status AgregatClass::generateConstructors_Java(GenContext *ctx)
  {
    FILE *fd = ctx->getFile();
  
    fprintf(fd, "%spublic %s(org.eyedb.Database db) throws org.eyedb.Exception {\n", ctx->get(), name);
  
    ctx->push();
    fprintf(fd, ctx->get());
    const_f0(fd, parent, "db", True);
  
    fprintf(fd, "%sinitialize(db);\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%spublic %s(org.eyedb.Database db, org.eyedb.Dataspace dataspace) throws org.eyedb.Exception {\n", ctx->get(), name);
  
    ctx->push();
    fprintf(fd, ctx->get());
    const_f0(fd, parent, "db, dataspace", True);
  
    fprintf(fd, "%sinitialize(db);\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%sprivate void initialize(org.eyedb.Database db) throws org.eyedb.Exception {\n", ctx->get(),
	    name, name);
    ctx->push();
    fprintf(fd, "%ssetClass(((db != null) ? db.getSchema().getClass(\"%s\") : %s.idbclass));\n\n", ctx->get(), getAliasName(), className(this)); // 10/12/05
    /*
      fprintf(fd, "%sidr_objsz = getClass(true).getObjectSize();\n", ctx->get());
      fprintf(fd, "%sidr_psize = getClass(true).getObjectPSize();\n\n", ctx->get());
    */
    fprintf(fd, "%ssetIDR(new byte[idr_objsz]);\n\n",
	    ctx->get());
    fprintf(fd, "%sorg.eyedb.Coder.memzero(getIDR(), org.eyedb.ObjectHeader.IDB_OBJ_HEAD_SIZE, "
	    "idr_objsz - org.eyedb.ObjectHeader.IDB_OBJ_HEAD_SIZE);\n",
	    ctx->get());
  
    const_f1(fd, ctx, name, False);
    fprintf(fd, "%suserInitialize();\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%spublic %s(org.eyedb.Struct x, boolean share) throws org.eyedb.Exception {\n", ctx->get(),
	    name, name);
  
    ctx->push();
    fprintf(fd, ctx->get());
    const_f01(fd, parent, "x", True);
    const_f1(fd, ctx, name, True);
    fprintf(fd, "%suserInitialize();\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%spublic %s(%s x, boolean share) throws org.eyedb.Exception {\n", ctx->get(), name, name);
  
    ctx->push();
    fprintf(fd, ctx->get());
    const_f01(fd, parent, "x", True);
    const_f1(fd, ctx, name, True);
    fprintf(fd, "%suserInitialize();\n", ctx->get());
    ctx->pop();
  
    fprintf(fd, "%s}\n\n", ctx->get());
  
    return Success;
  }

  static int
  true_items_count(const Class *cls, Attribute **items, int items_cnt)
  {
    for (int i = 0; i < items_cnt; i++)
      if (items[i]->getClassOwner() == cls && !items[i]->isNative())
	return items_cnt - i;

    return 0;
  }

  Status AgregatClass::generateClassDesc_Java(GenContext *ctx,
					      const char *prefix)
  {
    FILE *fd = ctx->getFile();
    Status status;
    const char *_type = (asStructClass() ? "org.eyedb.Struct" : "org.eyedb.Union");
  
    fprintf(fd, "%sstatic %sClass make(%sClass %s_class, org.eyedb.Schema m)\n", ctx->get(), _type, _type, name);
    fprintf(fd, "%s throws org.eyedb.Exception {\n", ctx->get());
    ctx->push();
  
    fprintf(fd, "%sif (%s_class == null)\n", ctx->get(), name);
    fprintf(fd,"%s  return new %sClass(\"%s\", ((m != null) ? m.getClass(\"%s\") : %s.idbclass));\n", ctx->get(), _type, getAliasName(),
	    className(parent, True, True), className(parent));
  
    if (items_cnt)
      {
	fprintf(fd, "%sorg.eyedb.Attribute[] attr = new org.eyedb.Attribute[%d];\n",
		ctx->get(), items_cnt);
	fprintf(fd, "%sint[] dims;\n", ctx->get());
      
	for (int i = 0; i < items_cnt; i++)
	  {
	    if (items[i]->class_owner == (void *)this)
	      {
		status = items[i]->generateClassDesc_Java(ctx);
		if (status)
		  return status;
	      }
	  }
      
	int count = true_items_count(this, items, items_cnt);
	fprintf(fd, "\n%s%s_class.setAttributes(attr, %d, %d);\n", ctx->get(),
		name, items_cnt - count, count);
      }

    /*
      if (isSystem())
      fprintf(fd, "%sClassPeer::setMType(%s_class, Class::System);\n",
      ctx->get(), name);
    */
  
    fprintf(fd, "\n%sreturn %s_class;\n", ctx->get(), name);
  
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());

    fprintf(fd, "%sstatic void init_p()\n", ctx->get());
    fprintf(fd, "%s throws org.eyedb.Exception {\n", ctx->get());
    ctx->push();
    fprintf(fd, "%sidbclass = make(null, null);\n", ctx->get());
    fprintf(fd, "%stry {\n", ctx->get());
    ctx->push();
    fprintf(fd, "%s%sDatabase.hash.put(\"%s\", %s.class.getConstructor(%sDatabase.clazz));\n",
	    ctx->get(), prefix, getAliasName(), name, prefix);
    ctx->pop();
    fprintf(fd, "%s} catch(java.lang.Exception e) {\n", ctx->get());
    ctx->push();
    fprintf(fd, "%se.printStackTrace();\n", ctx->get());
    //fprintf(fd, "%sSystem.err.println(e);\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%sstatic void init()\n", ctx->get());
    fprintf(fd, "%s throws org.eyedb.Exception {\n", ctx->get());
    ctx->push();
    fprintf(fd, "%smake((%sClass)idbclass, null);\n\n", ctx->get(),
	    _type);
    //  fprintf(fd, "%s%s_agritems = %s%s->getAttributes();\n", ctx->get(),
    //	  name, name, classSuffix);
    fprintf(fd, "%sidr_objsz = idbclass.getObjectSize();\n",
	    ctx->get());
    fprintf(fd, "%sidr_psize = idbclass.getObjectPSize();\n",
	    ctx->get());
  
    //  fprintf(fd, "%sObjectPeer::setUnrealizable(%s%s, True);\n",
    //ctx->get(), name, classSuffix);
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
    return Success;
  }

  static inline void
  argTypeDump(GenContext *ctx, ArgType *argtype)
  {
    FILE *fd = ctx->getFile();
  
    fprintf(fd, "%sargtype = new ArgType();\n", ctx->get());
    fprintf(fd, "%sargtype->setType(%d);\n", ctx->get(),
	    argtype->getType());
  
    if (argtype->getType() == OBJ_TYPE)
      fprintf(fd, "%sargtype->setClname(\"%s\");\n", ctx->get(),
	      argtype->getClname().c_str());
  }

  Status AgregatClass::generateClassComponent_Java(GenContext *ctx,
						   GenContext *ctxMth,
						   GenContext *ctxTrg)
  {
    FILE *fd = ctx->getFile();
    FILE *fdmth = ctxMth->getFile();
    FILE *fdtrg = ctxTrg->getFile();
  
    Status status;
    const char *_type = (asStructClass() ? "Struct" : "Union");
  
    fprintf(fd, "static Status %s_comp_realize(Database *db, Class *cls)\n{\n", name);
  
    ctx->push();
    fprintf(fd, "%sClassComponent *comp;\n", ctx->get());
    fprintf(fd, "%sStatus status;\n", ctx->get());
    fprintf(fd, "%sSignature *sign;\n", ctx->get());
    fprintf(fd, "%sArgType *argtype;\n\n", ctx->get());
  
    LinkedListCursor c(complist);
  
    ClassComponent *comp;
    while (complist->getNextObject(&c, (void *&)comp))
      {
	if (comp->asTrigger())
	  {
	    Trigger *trig = comp->asTrigger();
	    fprintf(fd, "%scomp = new Trigger(db, cls, %d, \"%s\", %s);\n",
		    ctx->get(), trig->getType(), trig->getSuffix().c_str(),
		    trig->getLight() ? "true" : "false");
	    fprintf(fdtrg, "extern \"C\"\n");
	    fprintf(fdtrg, "Status %s(ArgType type, Database *db, "
		    "const Oid &oid, Object *o)\n{\n",
		    trig->getCSym());
	    fprintf(fdtrg, "  return Success;\n}\n\n");
	  }
	else if (comp->asMethod())
	  {
	    Method *mth = comp->asMethod();
	    fprintf(fd, "%ssign = new Signature();\n",
		    ctx->get());
	    Executable *ex = mth->getEx();
	    Signature *sign = ex->getSign();
	    const char *prefix;
	    const char *extref = ex->getExtrefBody().c_str();
	  
	    if (mth->asFEMethod_C())
	      prefix = "FE";
	    else
	      prefix = "BE";
	  
	    argTypeDump(ctx, sign->getRettype());
	    fprintf(fd, "%ssign->setRettype(argtype);\n", ctx->get());
	    fprintf(fd, "%sdelete argtype;\n\n", ctx->get());
	  
	    int nargs = sign->getNargs();
	    fprintf(fd, "%ssign->setNargs(%d);\n", ctx->get(), nargs);
	    for (int i = 0; i < nargs; i++)
	      {
		argTypeDump(ctx, sign->getTypes(i));
		fprintf(fd, "%ssign->setTypes(%d, argtype);\n", ctx->get(),
			i);
		fprintf(fd, "%sdelete argtype;\n\n", ctx->get());
	      }
	  
	    fprintf(fd,
		    "%scomp = new %sMethod_Java(db, cls, \"%s\", sign, \"%s\");\n",
		    ctx->get(), prefix, ex->getExname().c_str(), extref);
	    fprintf(fdmth, "extern \"C\"\n");
	    fprintf(fdmth, "Status %s(Database *db, %sMethod_C *m, "
		    "Object *o, ArgArray &array, Argument &retarg)\n{\n",
		    Executable::makeInternalName(ex->getExname().c_str(),
						 sign, False,
						 getClass()->getName()),
		    prefix);
	  
	    fprintf(fdmth, "  return Success;\n}\n\n");
	  }
	else
	  abort();
      
	fprintf(fd, "%sif (status = comp->realize()) return status;\n\n", ctx->get());
	if (comp->asMethod())
	  fprintf(fd, "%sdelete sign;\n\n", ctx->get());
      }
  
    fprintf(fd, "%sreturn Success;\n", ctx->get());
  
    ctx->pop();
    fprintf(fd, "}\n\n");
    return Success;
  }

  Status AgregatClass::generateCode_Java(Schema *m,
					 const char *prefix,
					 const GenCodeHints &hints,
					 FILE *fd)
  {
    GenContext ctx(fd);
    int i;
    Status status;
  
    fprintf(fd, "public class %s extends %s {\n", name, className(parent));
  
    ctx.push();
  
    /*
      fprintf(fd, "\n%s// ----------------------------------------------------------------------\n", ctx.get());
      fprintf(fd, "%s// %s Interface\n", ctx.get(), name);
      fprintf(fd, "%s// ----------------------------------------------------------------------\n\n");
    */
    fprintf(fd, "\n");

    generateConstructors_Java(&ctx);

    for (i = 0; i < items_cnt; i++)
      {
	if (items[i]->class_owner == (void *)this)
	  {
	    status = items[i]->generateCode_Java(this, &ctx, hints, prefix);
	    if (status)
	      return status;
	  }
      }

    /*
      fprintf(fd, "\n%s// ----------------------------------------------------------------------\n", ctx.get());
      fprintf(fd, "%s// %s Protected Part\n", ctx.get(), name);
      fprintf(fd, "%s// ----------------------------------------------------------------------\n\n");
    */
    fprintf(fd, "\n");

    fprintf(fd, "%sprotected %s(org.eyedb.Database db, int dummy) {\n", ctx.get(),
	    name);
  
    ctx.push();
    fprintf(fd, ctx.get());
    const_f0(fd, parent, "db");
  
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());

    fprintf(fd, "%sprotected %s(org.eyedb.Database db, org.eyedb.Dataspace dataspace, int dummy) {\n", ctx.get(),
	    name);
  
    ctx.push();
    fprintf(fd, ctx.get());
    const_f0(fd, parent, "db, dataspace");
  
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());

    fprintf(fd, "%sprotected %s(org.eyedb.Struct x, boolean share, int dummy) {\n ",
	    ctx.get(), name);
    ctx.push();
    fprintf(fd, ctx.get());
    const_f01(fd, parent, "x");
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());
  
    fprintf(fd, "%sprotected %s(%s x, boolean share, int dummy) {\n ",
	    ctx.get(), name, name);

    ctx.push();
    fprintf(fd, ctx.get());
    const_f01(fd, parent, "x");
    ctx.pop();
  
    fprintf(fd, "%s}\n", ctx.get());
  
    /*
      fprintf(fd, "\n%s// ----------------------------------------------------------------------\n", ctx.get());
      fprintf(fd, "%s// %s Private Part\n", ctx.get(), name);
      fprintf(fd, "%s// ----------------------------------------------------------------------\n\n");
    */
    fprintf(fd, "\n");
  
    fprintf(fd, "%sstatic int idr_psize;\n", ctx.get());
    fprintf(fd, "%sstatic int idr_objsz;\n", ctx.get());
    fprintf(fd, "%spublic static org.eyedb.Class idbclass;\n", ctx.get());

    generateClassDesc_Java(&ctx, prefix);
  
    //  generateClassComponent_Java(&ctx, NULL);
  
    if (getTiedCode())
      {
	fprintf(fd, "\n%s// ----------------------------------------------------------------------\n", ctx.get());
	fprintf(fd, "%s// %s User Part\n", ctx.get(), name);
	fprintf(fd, "%s// ----------------------------------------------------------------------\n", ctx.get());
	fprintf(fd, "%s\n", getTiedCode());
      }
  
    fprintf(fd, "}\n\n");
  
    ctx.pop();

    return Success;
  }

  Status EnumClass::generateClassDesc_Java(GenContext *ctx)
  {
    FILE *fd = ctx->getFile();
    Status status;
  
    fprintf(fd, "%sstatic org.eyedb.EnumClass make(org.eyedb.EnumClass %s_class, org.eyedb.Schema m)\n", ctx->get(), name);
  
    fprintf(fd, "%s{\n", ctx->get());
    ctx->push();
    fprintf(fd, "%sif (%s_class == null)\n", ctx->get(), name);
    fprintf(fd, "%s  return new org.eyedb.EnumClass(\"%s\");\n\n", ctx->get(),
	    getAliasName());
  
    fprintf(fd, "%sorg.eyedb.EnumItem[] en = new org.eyedb.EnumItem[%d];\n", ctx->get(), items_cnt);

    for (int i = 0; i < items_cnt; i++)
      fprintf(fd, "%sen[%d] = new org.eyedb.EnumItem(\"%s\", %d, %d);\n", ctx->get(),
	      i, items[i]->getName(), items[i]->getValue(), i);
  
    fprintf(fd, "\n%s%s_class.setEnumItems(en);\n", ctx->get(), name);
  
    /*
      if (isSystem())
      fprintf(fd, "%sClassPeer::setMType(%s_class, Class::System);\n",
      ctx->get(), name);
    */
  
    fprintf(fd, "\n%sreturn %s_class;\n", ctx->get(), name);
  
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());

    fprintf(fd, "%sstatic void init_p()\n%s{\n", ctx->get(), ctx->get());
    ctx->push();
    fprintf(fd, "%sidbclass = make(null, null);\n", ctx->get());
    ctx->pop();
    fprintf(fd, "%s}\n\n", ctx->get());
  
    fprintf(fd, "%sstatic void init()\n%s{\n", ctx->get(), ctx->get());
    ctx->push();
    fprintf(fd, "%smake((org.eyedb.EnumClass)idbclass, null);\n", ctx->get());
  
    //  fprintf(fd, "\n%sObjectPeer::setUnrealizable(%s%s, True);\n",
    //	  ctx->get(), name, classSuffix);
    ctx->pop();

    fprintf(fd, "%s}\n", ctx->get());

    return Success;
  }

  Status EnumClass::generateCode_Java(Schema *,
				      const char *prefix,
				      const GenCodeHints &hints,
				      FILE *fd)
  {
    GenContext ctx(fd);
    int i;

    fprintf(fd, "public class %s extends org.eyedb.Enum {\n\n", name);

    ctx.push();

    fprintf(fd, "%s%s(org.eyedb.Database db)\n", ctx.get(), name);
    fprintf(fd, "%s{\n", ctx.get());
    ctx.push();
    fprintf(fd, "%ssuper(db);\n", ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());

    fprintf(fd, "%s%s()\n", ctx.get(), name);
    fprintf(fd, "%s{\n", ctx.get());
    ctx.push();
    fprintf(fd, "%ssuper();\n", ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());

    EnumItem **it;
    for (i = 0, it = items; i < items_cnt; i++, it++)
      fprintf(fd, "%spublic static final int %s = %d;\n", ctx.get(),
	      (*it)->getName(), (*it)->getValue());
  
    fprintf(fd, "\n");

    generateClassDesc_Java(&ctx);

    fprintf(fd, "%spublic static org.eyedb.Class idbclass;\n", ctx.get());
  
    ctx.pop();

    fprintf(fd, "}\n\n");
    return Success;
  }

  Status CollectionClass::generateCode_Java(Schema *,
					    const char *prefix,
					    const GenCodeHints &hints,
					    FILE *fd)
  {
    GenContext ctx(fd);
    int i;
    Status status;
    const char *c_name = getCName();
    //    const char *_type = getCSuffix();
    const char *_type = className(this); // 10/12/05
  
    fprintf(fd, "public class %s extends org.eyedb.CollSetClass {\n\n", c_name);

    ctx.push();
    fprintf(fd, "%sprivate %s(org.eyedb.Class coll_class, boolean isref) {\n",
	    ctx.get(), c_name);
    ctx.push();
    fprintf(fd, "%ssuper(coll_class, isref);\n", ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());
    fprintf(fd, "%spublic static org.eyedb.Class idbclass;\n\n", ctx.get());

    // FD modif 8/12/05: added org.eyedb.
    fprintf(fd, "%sstatic %sClass make(%sClass cls, "
	    "org.eyedb.Schema m)\n", ctx.get(), _type, _type);
    fprintf(fd, "%s{\n", ctx.get());
    ctx.push();
    fprintf(fd, "%sif (cls == null)\n%s  {\n", ctx.get(), ctx.get());
  
    ctx.push();
    fprintf(fd, "%scls = new %sClass(((m != null) ? m.getClass(\"%s\") : %s.idbclass), ", ctx.get(), _type, coll_class->getName(),
	    className(coll_class)); // 10/12/05 <- className(coll_class, False));
  
    if (dim > 1)
      fprintf(fd, "%d);\n", dim);
    else
      fprintf(fd, "%s);\n", (isref ? "true" : "false"));
  
    //  fprintf(fd, "%s    ClassPeer::setMType(cls, Class::System);\n",
    //	  ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n", ctx.get());
  
    fprintf(fd, "%sreturn cls;\n", ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n\n", ctx.get());
  
    fprintf(fd, "%sstatic void init_p()\n", ctx.get());
    fprintf(fd, "%s{\n", ctx.get());
    ctx.push();
    fprintf(fd, "%sidbclass = make(null, null);\n", ctx.get());
    ctx.pop();
    fprintf(fd, "%s}\n", ctx.get());
    ctx.pop();

    fprintf(fd, "}\n\n");

    return Success;
  }
}
