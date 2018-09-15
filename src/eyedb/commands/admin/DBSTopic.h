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

#ifndef _EYEDB_ADMIN_DBTOPIC_H
#define _EYEDB_ADMIN_DBTOPIC_H

#include "Topic.h"
#include "eyedb/GetOpt.h"

class DBSTopic : public Topic {

public:
  DBSTopic();
};

CMDCLASS_GETOPT(DBSCreateCmd, "create");
CMDCLASS_GETOPT(DBSDeleteCmd, "delete");
CMDCLASS_GETOPT(DBSListCmd, "list");
CMDCLASS_GETOPT(DBSRenameCmd, "rename");
CMDCLASS_GETOPT(DBSMoveCmd, "move");
CMDCLASS_GETOPT(DBSCopyCmd, "copy");
CMDCLASS_GETOPT(DBSDefAccessCmd, "defaccess");
CMDCLASS_GETOPT(DBSSetDefDspCmd, "setdefdsp");
CMDCLASS_GETOPT(DBSGetDefDspCmd, "getdefdsp");
CMDCLASS_GETOPT(DBSExportCmd, "export");
CMDCLASS_GETOPT(DBSImportCmd, "import");
CMDCLASS_GETOPT(DBSSetMaxObjCountCmd, "setmaxobjcount");
CMDCLASS_GETOPT(DBSGetMaxObjCountCmd, "getmaxobjcount");
CMDCLASS_GETOPT(DBSSetLogSizeCmd, "setlogsize");
CMDCLASS_GETOPT(DBSGetLogSizeCmd, "getlogsize");

#endif
