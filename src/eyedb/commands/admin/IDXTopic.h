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

#ifndef _EYEDB_ADMIN_IDXTOPIC_H
#define _EYEDB_ADMIN_IDXTOPIC_H

#include "Topic.h"
#include "eyedb/GetOpt.h"

class IDXTopic : public Topic {

public:
  IDXTopic();
};

CMDCLASS_GETOPT(IDXCreateCmd, "create");
CMDCLASS_GETOPT(IDXDeleteCmd, "delete");
CMDCLASS_GETOPT(IDXUpdateCmd, "update");
CMDCLASS_GETOPT(IDXListCmd, "list");
CMDCLASS_GETOPT(IDXStatsCmd, "stats");
CMDCLASS_GETOPT(IDXSimulateCmd, "simulate");
CMDCLASS_GETOPT(IDXMoveCmd, "move");
CMDCLASS_GETOPT(IDXSetdefdspCmd, "setdefdsp");
CMDCLASS_GETOPT(IDXGetdefdspCmd, "getdefdsp");
CMDCLASS_GETOPT(IDXGetlocaCmd, "getloca");

#endif
