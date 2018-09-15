/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-1999,2004-2006 SYSRA
   
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

#include <string>

static const std::string ALL_OPT = "all";
static const std::string DATAFILES_OPT = "datafiles";
static const std::string DBFILE_OPT = "dbfile";
static const std::string DBID_OPT = "dbid";
static const std::string DBNAME_OPT = "dbname";
static const std::string DEFACCESS_OPT = "defaccess";
static const std::string FILEDIR_OPT = "filedir";
static const std::string FILENAME_OPT = "filename";
static const std::string LIST_OPT = "list";
static const std::string MTHDIR_OPT = "mthdir";
static const std::string MAXOBJCNT_OPT = "max-object-count";
static const std::string NAME_OPT = "name";
static const std::string PHYSICAL_OPT = "physical";
static const std::string SIZE_OPT = "size";
static const std::string SLOTSIZE_OPT = "slotsize";
static const std::string STATS_OPT = "stats";
static const std::string USERACCESS_OPT = "useraccess";

static const std::string DBS_EXT = ".dbs";
static const std::string DTF_EXT = ".dat";

static const unsigned int ONE_K = 1024;
static const unsigned int DEFAULT_DTFSIZE = 2048;
static const unsigned int DEFAULT_DTFSZSLOT = 16;
static const char DEFAULT_DTFNAME[] = "DEFAULT";
static const unsigned int DEFAULT_MAXOBJCNT = 10000000;

