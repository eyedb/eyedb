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


#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>
#include <unistd.h>

#include "eyedb/eyedb.h"

#include "eyedb/base_p.h"
#include "eyedb/internals/ObjectHeader.h"
#include "misc.h"

namespace eyedb {

  const char NullString[] = "NULL";
  const char NilString[]  = "nil";

  Bool isOidValid(const eyedbsm::Oid *oid)
  {
    return (oid && oid->getNX() && oid->getUnique()) ? True : False;
  }

  eyedbsm::Oid *oidInvalidate(eyedbsm::Oid *oid)
  {
    memset(oid, 0, sizeof(eyedbsm::Oid));
    return oid;
  }

  const eyedbsm::Oid *getInvalidOid()
  {
    static eyedbsm::Oid oid;
    memset(&oid, 0, sizeof(eyedbsm::Oid)); /* be secure */
    return &oid;
  }

  Bool OidCompare(const eyedbsm::Oid *o1, const eyedbsm::Oid *o2)
  {
    return (o1->getNX() == o2->getNX() && o1->getUnique() == o2->getUnique()) ? True: False;
  }

  Bool ObjectHeadCompare(const ObjectHeader *h1,
			 const ObjectHeader *h2)
  {
    /*
      return (h1->magic == h2->magic &&
      h1->type == h2->type &&
      h1->size == h2->size &&
      OidCompare(&h1->oid_cl, &h2->oid_cl)) ? True : False;
    */
    Bool f = (h1->magic == h2->magic &&
	      h1->type == h2->type &&
	      h1->size == h2->size &&
	      (OidCompare(&h1->oid_cl, &h2->oid_cl) ||
	       !isOidValid(&h1->oid_cl))) ? True : False;
    if (!f)
      printf("OBJH CMP: %x %x, %d %d, %s %s\n",
	     h1->type, h2->type,
	     h1->size, h2->size,
	     OidGetString(&h1->oid_cl), OidGetString(&h2->oid_cl));

    return f;
  }

  eyedbsm::Oid stringGetOid(const char *buf)
  {
    eyedbsm::Oid oid;
    eyedbsm::Oid::NX nx;
    eyedbsm::Oid::Unique unique;
    eyedbsm::Oid::DbID dbid;

    static eyedbsm::Oid null_oid;
    int len = strlen(buf);
    char c = *buf;

    if (c < '0' || c > '9' || len < 9)
      return null_oid;

    if (strcmp(&buf[len-4], ":oid"))
      return null_oid;
  
    if (sscanf(buf, "%u.%u.%u:oid", &nx, &dbid, &unique) == 3 ||
	sscanf(buf, "%u:%u:%u:oid", &nx, &dbid, &unique) == 3) {
      oid.setNX(nx);
      eyedbsm::dbidSet(&oid, dbid);
      oid.setUnique(unique);
      return oid;
    }
    return null_oid;
  }

  const char *OidGetString(const eyedbsm::Oid *oid)
  {
#define NBUFS 8
    static char bufs[NBUFS][32];
    static int which;

    if (which == NBUFS)
      which = 0;

    if (isOidValid(oid))
      sprintf(bufs[which], eyedbsm::getOidString(oid));
    else
      strcpy(bufs[which], NullString);

    return bufs[which++];
  }

#define INDENT_INC 8

  char *make_indent(int indent)
  {
    char *indent_str = (char *)malloc(indent < 0 ? 1 : indent+1);
    char *p;
    int i;

    if (indent < 0)
      indent = 0;

    for (p = indent_str, i = 0; i < indent; i++, p++)
      *p = ' ';

    *p = 0;
    return indent_str;
  }

  void delete_indent(char *indent_str)
  {
    free(indent_str);
  }

  void dump_data(Data data, Size size)
  {
    unsigned char *p = (unsigned char *)data;
    static const char sep[] =
      "------------------------------------------------------------";

    printf("%s\n", sep);
    printf("Dumping data 0x%x [%d]\n", data, size);

    while (size--)
      printf("0x%x ", *p++);

    printf("\n%s\n", sep);
  }

  const char *makeName(const char *name, const char *prefix)
  {
    if (prefix)
      {
	static char bufname[256];
#if 0
	const char *p = strrchr(prefix, ':');
	p = (!p ? prefix : p+1);
#else
	const char *p = prefix;
#endif
	sprintf(bufname, "%s%s", p, name);
	return bufname;
      }

    return name;
  }

  static int trace = 0;

  void
  set_trace(int tr)
  {
    trace = tr;
  }

  void tr(const char *fname, const char *fmt, ...)
  {
    if (trace)
      {
	va_list ap;
	char s[512];

	va_start(ap, fmt);

	vsprintf(s, fmt, ap);

	printf("%s: %s\n", fname, s);
	va_end(ap);
      }
  }

  /*#define IDB_PROBE*/

  void idbPROBE(const char *fmt, ...)
  {
#ifdef IDB_PROBE
    va_list ap;
    static struct timeval tp_s;
    struct timeval tp;
    int ms;

    gettimeofday(&tp, NULL);

    if (!tp_s.tv_sec)
      tp_s = tp;
  
    ms = ((tp.tv_sec - tp_s.tv_sec) * 1000) + ((tp.tv_usec - tp_s.tv_usec)/1000);

    va_start(ap, fmt);

    vfprintf(stdout, fmt, ap);
    fprintf(stdout, ": %d ms\n", ms);
    va_end(ap);
#endif
  }

  static void
  clean(const char *tmpfile_1, const char *tmpfile_2)
  {
    unlink(tmpfile_1);
    if (*tmpfile_2)
      unlink(tmpfile_2);
  }

  FILE *
  run_cpp(FILE *fd, const char *cpp_cmd, const char *cpp_flags,
	  const char *file)
  {
    if (!cpp_cmd)
      cpp_cmd = eyedb::ClientConfig::getCValue("cpp_cmd");

    if (!cpp_flags || !*cpp_flags)
      cpp_flags = eyedb::ClientConfig::getCValue("cpp_flags");

    if (!cpp_flags)
      cpp_flags = "";

    if (cpp_cmd) {
      fclose(fd);

      char *tmpfile_1, *tmpfile_2;

      char templ1[] =  "/tmp/eyedb-cpp.XXXXXX";
      tmpfile_1 = mktemp(templ1);

      char cmd[512];

      // 12/01/99 all those commands are rather fragile!!!
      
      // first pass: because the C preprocessor does not
      // deal well with C++ the comments '//'
      sprintf(cmd, "sed -e 's|//.*||' %s | %s %s - > %s", file, cpp_cmd,
	      cpp_flags, tmpfile_1);

      if (system(cmd)) {
	fprintf(stderr, "command '%s' failed. Perharps the C preprocessor command '%s%s%s' is not correct\n", cmd, cpp_cmd,
		(cpp_flags && *cpp_flags ? " " : ""), cpp_flags); 
	clean(tmpfile_1, "");
	return 0;
      }

      // second pass: to substitute the "<stdin>" file directive
      // to the true file directive
      char templ2[] = "/tmp/eyedb-cpp.out.XXXXXX";
      tmpfile_2 = mktemp(templ2);

      sprintf(cmd, "sed -e 's|<stdin>|%s|g' %s > %s",
	      file, tmpfile_1, tmpfile_2);

      if (system(cmd)) {
	clean(tmpfile_1, tmpfile_2);
	return 0;
      }

      // third pass: to deal well with the ## preprocessor directive
      // and to transform ': :' to '::'
      sprintf(cmd, "sed -e 's/ ## //g' -e 's/## //g' -e 's/ ##//g' "
	      "-e 's/# \\([a-zA-Z_][a-zA-Z_0-9]*\\)/\"\\1\"/g' "
	      "-e 's/^\\\\#/#/' -e 's/##//g' -e 's/: :/::/g' %s | "
	      "grep -v \"^#ident\" | grep -v \"^#pragma\" > %s",
	      tmpfile_2, tmpfile_1);
	      
      if (system(cmd)) {
	clean(tmpfile_1, tmpfile_2);
	return 0;
      }
	      
      FILE *odlin = fopen(tmpfile_1, "r");

      clean(tmpfile_1, tmpfile_2);
      if (!odlin) {
	fprintf(stderr,
		"eyedbodl: cannot open file '%s' for reading\n",
		tmpfile);
	return 0;
      }

      return odlin;
    }

    return fd;
  }
}
