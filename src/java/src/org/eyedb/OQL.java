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

package org.eyedb;

public class OQL {

  private Database db;
  private String query;
  private boolean state = true;
  private Connection conn;
  private int qid = 0;
  private boolean executed = false;
  private boolean value_read = false;
  private Value result_value = null;
  
  public OQL(Database db, String query) {
    this.db = db;
    this.query = query.trim();
    if (this.query.length() > 0 &&
	this.query.charAt(this.query.length()-1) != ';')
      this.query += ";";
    conn = db.getConnection();
  }

  public Database getDatabase() {return db;}

  public void execute() 
    throws Exception {
    if (executed) return;

    if (!db.isInTransaction())
      throw new TransactionException
	(new Status(Status.SE_TRANSACTION_NEEDED,
		       "oql: " + query));

    Value vqid = new Value();
    Status status = RPC.rpc_OQL_CREATE(conn, db, query, vqid);
    if (!status.isSuccess())
      throw new InvalidQueryException(status);
    qid = vqid.sgetInt();
  }

  public void execute(Value v)
    throws Exception {
      execute();
      getResult();
      v.set(result_value);
  }

  // get flattened results
  public void execute(ObjectArray obj_arr)
    throws Exception {
      execute();
      getResult();
      result_value.toArray(db, obj_arr);
  }

  public void execute(ObjectArray obj_arr, RecMode rcm)
    throws Exception {
      execute();
      getResult();
      result_value.toArray(db, obj_arr, rcm);
  }

  public void execute(OidArray oid_arr)
    throws Exception {
      execute();
      getResult();
      result_value.toArray(oid_arr);
  }

  public void execute(ValueArray val_arr)
    throws Exception {
      execute();
      getResult();
      result_value.toArray(val_arr);
  }

  // get flattened results
  public void getResult(ObjectArray obj_arr)
    throws Exception {
      execute(obj_arr);
  }

  public void getResult(ObjectArray obj_arr, RecMode rcm)
    throws Exception {
      execute(obj_arr, rcm);
  }

  public void getResult(OidArray oid_arr)
    throws Exception {
      execute(oid_arr);
  }

  public void getResult(ValueArray value_arr)
    throws Exception {
      execute(value_arr);
  }

  public void dispose()
    throws Exception {

    if (!executed || qid == 0) return;
    Status status = RPC.rpc_OQL_DELETE(conn, db, qid);
    if (!status.isSuccess())
      throw new InvalidQueryException(status);
    qid = 0;
  }

  protected void finalize() {
    if (!executed || qid == 0) return;
    RPC.rpc_OQL_DELETE(conn, db, qid);
  }

  private void getResult()
    throws Exception {
      if (value_read) return;
      if (!executed) execute();

      result_value = new Value();
      Status status = RPC.rpc_OQL_GETRESULT(conn, db, qid, result_value);
      if (!status.isSuccess())
	throw new InvalidQueryException(status);

      value_read = true;
  }
}
