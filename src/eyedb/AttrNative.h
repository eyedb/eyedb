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


#ifndef _EYEDB_ATTR_NATIVE_H
#define _EYEDB_ATTR_NATIVE_H

namespace eyedb {

  class AttrNative : public Attribute {

    Status (*_getvalue)(const Object *, Data *, int, int, Bool *, Bool, Size *);
    Status (*_setvalue)(Object *, Data, int, int);
    Status (*_getoid)(const Object *, Oid *oid, int nb, int from);
    Status (*_setoid)(Object *, const Oid *, int, int, Bool);
    Status cannot(const char *) const;

    int iniCompute(const Database *, int, Data &, Data&) const {return 0;}
    Status copy(Object *, Bool) const {return Success;}

  public:

    AttrNative(Class *, Class *, const char *, Bool isRef,
	       Status (*)(const Object *, Data *, int, int, Bool *, Bool, Size *),
	       Status (*)(Object *, Data, int, int),
	       Status (*)(const Object *, Oid *oid, int nb, int from),
	       Status (*)(Object *, const Oid *, int, int, Bool));

    AttrNative(Class *, Class *, const char *, int dim,
	       Status (*)(const Object *, Data *, int, int, Bool *, Bool, Size *),
	       Status (*)(Object *, Data, int, int),
	       Status (*)(const Object *, Oid *oid, int nb, int from),
	       Status (*)(Object *, const Oid *, int, int, Bool));
    AttrNative(const AttrNative *, const Class *, const Class *, const Class *, int);
    Status setOid(Object *, const Oid *, int, int, Bool) const;
    Status getOid(const Object *, Oid *oid, int nb, int from) const;
    Status setValue(Object *, Data, int, int, Bool) const;
    Status getValue(const Object *, Data *, int, int, Bool *) const;
    Status getTValue(Database *db, const Oid &objoid,
		     Data *data, int nb = 1, int from = 0,
		     Bool *isnull = 0, Size *rnb = 0, Offset = 0) const;
    Status trace(const Object *, FILE *, int *, unsigned int, const RecMode *) const;
    Bool isNative() const {return True;}

    void reportAttrCompSetOid(Offset *offset, Data idr) const;
    Status generateCollSetClassMethod_C(Class *, GenContext *,
					const GenCodeHints &hints,
					const char *, Bool)
    {return Success;}
    Status generateCode_C(Class*, const GenCodeHints &,
			  GenContext *, GenContext *)
    {return Success;}
    Status generateClassDesc_C(GenContext *)
    {return Success;}

    Status generateBody_C(Class *, GenContext *,
			  const GenCodeHints &hints)
    {return Success;}
    Status generateGetMethod_C(Class *, GenContext *,
			       Bool,
			       const GenCodeHints &hints,
			       const char *)
    {return Success;}
    Status generateSetMethod_C(Class *, GenContext *,
			       const GenCodeHints &hints)
    {return Success;}

    Status generateSetMethod_C(Class *, GenContext *,
			       Bool,
			       const GenCodeHints &hints)
    {return Success;}


    Status generateCollSetClassMethod_Java(Class *, GenContext *,
					   const GenCodeHints &,
					   Bool)
    {return Success;}
    Status generateCode_Java(Class*, GenContext *,
			     const GenCodeHints &, const char *)
    {return Success;}
    Status generateClassDesc_Java(GenContext *)
    {return Success;}

    Status generateBody_Java(Class *, GenContext *,
			     const GenCodeHints &,
			     const char *prefix)
    {return Success;}
    Status generateGetMethod_Java(Class *, GenContext *,
				  Bool,
				  const GenCodeHints &,
				  const char *, const char *)
    {return Success;}
    Status generateSetMethod_Java(Class *, GenContext *,
				  Bool,
				  const GenCodeHints &)
    {return Success;}

    Status generateSetMethod_Java(Class *, GenContext *,
				  const GenCodeHints &)
    {return Success;}

    static void copy(int, Attribute ** &, unsigned int &, Class *);
    static void init();
    static void _release();
  };

  enum {
    ObjectITEMS = 0,
    ClassITEMS,
    CollectionITEMS,
    CollectionClassITEMS,
    idbITEMS_COUNT
  };

}

#endif
