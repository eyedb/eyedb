
/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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

#include <eyedb/eyedb.h>

int
main(int argc, char *argv[])
{
  eyedb::init(argc, argv);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <dbname>\n", argv[0]);
    return 1;
    }

  eyedb::Exception::setMode(eyedb::Exception::ExceptionMode);

  try {
    eyedb::Connection conn;
    // connecting to the EyeDB server
    conn.open();

    eyedb::Database db(argv[1]);

    // opening database argv[1]
    eyedb::Status s = db.open(&conn, eyedb::Database::DBRead);
    if (s) std::cerr << s << std::endl;
  }

  catch(eyedb::Exception &e) {
    std::cerr << e;
    eyedb::release();
    return 1;
  }

  eyedb::release();

  return 0;
}
