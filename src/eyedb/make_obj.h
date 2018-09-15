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


#ifndef _EYEDB_MAKE_OBJ_H
#define _EYEDB_MAKE_OBJ_H

namespace eyedb {

  extern Status
  (*getMakeFunction(int type))(Database *, const Oid *, Object **,
				   const RecMode *,
				   const ObjectHeader *, Data,
				   LockMode lockmode,
				   const Class *);

  extern Status
  classMake(Database *, const Oid *, Object **,
		const RecMode *, const ObjectHeader *, Data,
		LockMode lockmode, const Class *);

  extern Status
  agregatClassMake(Database *, const Oid *, Object **,
		       const RecMode *, const ObjectHeader *, Data,
		       LockMode lockmode, const Class *);

  extern Status
  enumClassMake(Database *, const Oid *, Object **,
		    const RecMode *, const ObjectHeader *, Data,
		    LockMode lockmode, const Class *);

  extern Status
  basicClassMake(Database *, const Oid *, Object **,
		     const RecMode *, const ObjectHeader *, Data,
		     LockMode lockmode, const Class *);

  extern Status
  basicMake(Database *, const Oid *, Object **,
		const RecMode *, const ObjectHeader *, Data,
		LockMode lockmode, const Class *);

  extern Status
  agregatMake(Database *, const Oid *, Object **,
		  const RecMode *, const ObjectHeader *, Data,
		  LockMode lockmode, const Class *);

  extern Status
  enumMake(Database *, const Oid *, Object **,
	       const RecMode *, const ObjectHeader *, Data,
	       LockMode lockmode, const Class *);

  extern Status
  schemaClassMake(Database *, const Oid *, Object **,
		      const RecMode *, const ObjectHeader *, Data,
		      LockMode lockmode, const Class *);

  extern Status
  collectionMake(Database *, const Oid *, Object **,
		     const RecMode *, const ObjectHeader *, Data,
		     LockMode lockmode, const Class *);

  extern Status
  collectionClassMake(Database *, const Oid *, Object **,
			  const RecMode *, const ObjectHeader *, Data,
			  LockMode lockmode, const Class *);

  extern Status
  classCollectionMake(Database *, const Oid &, Collection **);

}

#endif
