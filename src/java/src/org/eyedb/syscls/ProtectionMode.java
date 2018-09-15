
//
// class ProtectionMode
//
// package org.eyedb.syscls
//
// Generated by eyedbodl at Thu Sep 10 15:18:45 2009
//

package org.eyedb.syscls;

import org.eyedb.utils.*;
import org.eyedb.syscls.*;

public class ProtectionMode extends org.eyedb.Enum {

  ProtectionMode(org.eyedb.Database db)
  {
    super(db);
  }

  ProtectionMode()
  {
    super();
  }

  public static final int PROT_READ = 256;
  public static final int PROT_RW = 257;

  static org.eyedb.EnumClass make(org.eyedb.EnumClass ProtectionMode_class, org.eyedb.Schema m)
  {
    if (ProtectionMode_class == null)
      return new org.eyedb.EnumClass("protection_mode");

    org.eyedb.EnumItem[] en = new org.eyedb.EnumItem[2];
    en[0] = new org.eyedb.EnumItem("PROT_READ", 256, 0);
    en[1] = new org.eyedb.EnumItem("PROT_RW", 257, 1);

    ProtectionMode_class.setEnumItems(en);

    return ProtectionMode_class;
  }

  static void init_p()
  {
    idbclass = make(null, null);
  }

  static void init()
  {
    make((org.eyedb.EnumClass)idbclass, null);
  }
  public static org.eyedb.Class idbclass;
}
