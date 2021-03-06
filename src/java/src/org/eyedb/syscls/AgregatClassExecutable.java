
//
// class AgregatClassExecutable
//
// package org.eyedb.syscls
//
// Generated by eyedbodl at Thu Sep 10 15:18:45 2009
//

package org.eyedb.syscls;

import org.eyedb.utils.*;
import org.eyedb.syscls.*;

public class AgregatClassExecutable extends AgregatClassComponent {

  public AgregatClassExecutable(org.eyedb.Database db) throws org.eyedb.Exception {
    super(db, 1);
    initialize(db);
  }

  public AgregatClassExecutable(org.eyedb.Database db, org.eyedb.Dataspace dataspace) throws org.eyedb.Exception {
    super(db, dataspace, 1);
    initialize(db);
  }

  private void initialize(org.eyedb.Database db) throws org.eyedb.Exception {
    setClass(((db != null) ? db.getSchema().getClass("agregat_class_executable") : AgregatClassExecutable.idbclass));

    setIDR(new byte[idr_objsz]);

    org.eyedb.Coder.memzero(getIDR(), org.eyedb.ObjectHeader.IDB_OBJ_HEAD_SIZE, idr_objsz - org.eyedb.ObjectHeader.IDB_OBJ_HEAD_SIZE);
    headerCode(org.eyedb.ObjectHeader._Struct_Type, idr_psize, org.eyedb.ObjectHeader.IDB_XINFO_LOCAL_OBJ, true);
    getClass(true).newObjRealize(getDatabase(), this);
    setGRTObject(true);
    userInitialize();
  }

  public AgregatClassExecutable(org.eyedb.Struct x, boolean share) throws org.eyedb.Exception {
    super(x, share, 1);
    headerCode(org.eyedb.ObjectHeader._Struct_Type, idr_psize, org.eyedb.ObjectHeader.IDB_XINFO_LOCAL_OBJ, true);
    if (!share)
      getClass(true).newObjRealize(getDatabase(), this);
    setGRTObject(true);
    userInitialize();
  }

  public AgregatClassExecutable(AgregatClassExecutable x, boolean share) throws org.eyedb.Exception {
    super(x, share, 1);
    headerCode(org.eyedb.ObjectHeader._Struct_Type, idr_psize, org.eyedb.ObjectHeader.IDB_XINFO_LOCAL_OBJ, true);
    if (!share)
      getClass(true).newObjRealize(getDatabase(), this);
    setGRTObject(true);
    userInitialize();
  }

  public void setEx(Executable _ex)
  throws org.eyedb.Exception {
    getClass(true).getAttributes()[4].setValue(this, new org.eyedb.Value(_ex), 0);
  }

  public Executable getEx()
  throws org.eyedb.Exception {
    org.eyedb.Value __x;
    org.eyedb.Object __y;

    __x = getClass(true).getAttributes()[4].getValue(this, 0, true);
    __y = Database.makeObject(__x.sgetObject(), true);
    if (__y != __x.sgetObject())
      getClass(true).getAttributes()[4].setValue(this, new org.eyedb.Value(__y), 0);
    return (Executable)__y;
  }


  protected AgregatClassExecutable(org.eyedb.Database db, int dummy) {
    super(db, 1);
  }

  protected AgregatClassExecutable(org.eyedb.Database db, org.eyedb.Dataspace dataspace, int dummy) {
    super(db, dataspace, 1);
  }

  protected AgregatClassExecutable(org.eyedb.Struct x, boolean share, int dummy) {
     super(x, share, 1);
  }

  protected AgregatClassExecutable(AgregatClassExecutable x, boolean share, int dummy) {
     super(x, share, 1);
  }

  static int idr_psize;
  static int idr_objsz;
  public static org.eyedb.Class idbclass;
  static org.eyedb.StructClass make(org.eyedb.StructClass AgregatClassExecutable_class, org.eyedb.Schema m)
   throws org.eyedb.Exception {
    if (AgregatClassExecutable_class == null)
      return new org.eyedb.StructClass("agregat_class_executable", ((m != null) ? m.getClass("eyedb::AgregatClassComponent") : AgregatClassComponent.idbclass));
    org.eyedb.Attribute[] attr = new org.eyedb.Attribute[5];
    int[] dims;

    dims = null;
    attr[4] = new org.eyedb.Attribute(((m != null) ? m.getClass("executable") : Executable.idbclass), idbclass, "ex", 4, false, 0, dims);

    AgregatClassExecutable_class.setAttributes(attr, 4, 1);

    return AgregatClassExecutable_class;
  }

  static void init_p()
   throws org.eyedb.Exception {
    idbclass = make(null, null);
    try {
      Database.hash.put("agregat_class_executable", AgregatClassExecutable.class.getConstructor(Database.clazz));
    } catch(java.lang.Exception e) {
      e.printStackTrace();
    }
  }

  static void init()
   throws org.eyedb.Exception {
    make((org.eyedb.StructClass)idbclass, null);

    idr_objsz = idbclass.getObjectSize();
    idr_psize = idbclass.getObjectPSize();
  }

}

