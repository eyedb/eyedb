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


#ifndef _EYEDB_OQL_H
#define _EYEDB_OQL_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class OQL {

    // ----------------------------------------------------------------------
    // OQL Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param db
       @param fmt
    */
    OQL(Database *db, const char *fmt, ...);

    /**
       Not yet documented
       @param conn
       @param fmt
    */
    OQL(Connection *conn, const char *fmt, ...);

    /**
       Not yet documented
       @return
    */
    Status execute();

    /**
       Not yet documented
       @param value
       @return
    */
    Status execute(Value &value);

    // get flattened results

    /**
       Not yet documented
       @param obj_vect
       @param recmode
       @return
    */
    Status execute(ObjectPtrVector &obj_vect,
		   const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param obj_array
       @param recmode
       @return
    */
    Status execute(ObjectArray &obj_array,
		   const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @return
    */
    Status execute(OidArray &oid_array);

    /**
       Not yet documented
       @param val_array
       @return
    */
    Status execute(ValueArray &val_array);

    /**
       Not yet documented
       @param value
       @return
    */
    Status getResult(Value &value);

    // get flattened results

    /**
       Not yet documented
       @param obj_vect
       @param recmode
       @return
    */
    Status getResult(ObjectPtrVector &obj_vect,
		     const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param obj_array
       @param recmode
       @return
    */
    Status getResult(ObjectArray &obj_array,
		     const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param oid_array
       @return
    */
    Status getResult(OidArray &oid_array);

    /**
       Not yet documented
       @param val_array
       @return
    */
    Status getResult(ValueArray &val_array);

    /**
       Not yet documented
       @return
    */
    Database *getDatabase() {return db;}

    ~OQL();

    // ----------------------------------------------------------------------
    // OQL Private Part
    // ----------------------------------------------------------------------
  private:
    char *oql_string;
    int qid;
    Connection *conn;
    Database *db;
    Value result_value;
    Bool state;
    Status oql_status;
    void init(Database *);
    Bool executed;
    Bool value_read;
    SchemaInfo *schema_info;
    Status getResult();
    void init(Connection *, Database *, const char *);
    void log_result() const;

  public: /* restricted */
    static Status initDatabase(Database *db);
  };


  /**
     @}
  */

}

#endif
