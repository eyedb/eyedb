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


#ifndef _EYEDB_EYEDB_H
#define _EYEDB_EYEDB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include <eyedblib/thread.h>
#include <eyedb/linklist.h>

#include <eyedb/gbx.h>
#include <eyedb/base.h>
#include <eyedb/Error.h>
#include <eyedb/init.h>

#include <eyedb/Oid.h>
#include <eyedb/Exception.h>
#include <eyedb/RecMode.h>
#include <eyedb/AbstractIterator.h>
#include <eyedb/ObjectLocation.h>

#include <eyedb/IndexImpl.h>

#include <eyedb/Object.h>

#include <eyedb/TransactionParams.h>

#include <eyedb/Class.h>
#include <eyedb/Value.h>
#include <eyedb/Attribute.h>

#include <eyedb/BasicClass.h>
#include <eyedb/CharClass.h>
#include <eyedb/ByteClass.h>
#include <eyedb/OidClass.h>
#include <eyedb/Int16Class.h>
#include <eyedb/Int32Class.h>
#include <eyedb/Int64Class.h>
#include <eyedb/FloatClass.h>

#include <eyedb/AgregatClass.h>
#include <eyedb/StructClass.h>
#include <eyedb/UnionClass.h>
#include <eyedb/EnumClass.h>

#include <eyedb/CollectionClass.h>
#include <eyedb/CollSetClass.h>
#include <eyedb/CollBagClass.h>
#include <eyedb/CollListClass.h>
#include <eyedb/CollArrayClass.h>

#include <eyedb/Instance.h>

#include <eyedb/Basic.h>
#include <eyedb/Char.h>
#include <eyedb/Byte.h>
#include <eyedb/OidP.h>
#include <eyedb/Int16.h>
#include <eyedb/Int32.h>
#include <eyedb/Int64.h>
#include <eyedb/Float.h>

#include <eyedb/Agregat.h>
#include <eyedb/Struct.h>
#include <eyedb/Union.h>
#include <eyedb/Enum.h>

#include <eyedb/Collection.h>
#include <eyedb/CollSet.h>
#include <eyedb/CollBag.h>
#include <eyedb/CollList.h>
#include <eyedb/CollArray.h>

#include <eyedb/Connection.h>

#include <eyedb/Datafile.h>
#include <eyedb/Dataspace.h>
#include <eyedb/Database.h>
#include <eyedb/Schema.h>
#include <eyedb/Transaction.h>

#include <eyedb/OQL.h>

#include <eyedb/ClassIterator.h>
#include <eyedb/CollectionIterator.h>
#include <eyedb/OQLIterator.h>

#include <eyedb/Architecture.h>
#include <eyedb/Config.h>
#include <eyedb/ClientConfig.h>
#include <eyedb/ServerConfig.h>
#include <eyedb/GenHashTable.h>

#include <eyedb/CollImpl.h>
#include <eyedb/DBM.h>
#include <eyedb/syscls.h>
#include <eyedb/utils.h>
#include <eyedb/opts.h>
#include <eyedb/version.h>
#include <eyedb/Argument.h>

#endif
