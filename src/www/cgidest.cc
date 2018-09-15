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


#include "eyedbcgiP.h"
#include "eyedblib/butils.h"

//
// idbWDest utilities
//

idbWDest::idbWDest()
{
  fd = 1; // stdout
  indent = 0;
  is_html = False;
  is_marks = True;
  //  is_marks = False;
}

idbWDest::idbWDest(int _fd, int _indent_inc) : fd(_fd), indent_inc(_indent_inc)
{
  indent = 0;
  is_html = True;
  is_marks = True;
  //is_marks = False;
}

void
idbWDest::push()
{
  indent += indent_inc;
}

void
idbWDest::pop()
{
  indent -= indent_inc;
}

const char *
idbWDest::getIndent() const
{
  static char indent_str[128];
  char *p = indent_str;

  char indent_char = idbW_db_config->display_config.indent_char;
  if (is_marks)
    {
      char indent_expand_char = idbW_db_config->display_config.indent_expand_char;
      int inc2 = indent_inc << 1;
      for (int n = 0; n < indent; n++)
	{
	  if (n && n != indent -1 && !((n-2) % inc2))
	    *p++ = indent_expand_char;
	  else
	    *p++ = indent_char;
	}
    }
  else
    {
      int n = indent;
      while (n--)
	*p++ = indent_char;
    }

  *p = 0;

  return indent_str;
}

//static char out[8192];

static int is_empty_buf(const char *s)
{
  char c;
  while (c = *s++)
    if (c != ' ' && c != '\t' && c != '\n')
      return 0;

  return 1;
}

int
idbWDest::cflush(const char *s, Bool)
{
  int len = strlen(s);

  if (rpc_socketWrite(fd, (void *)s, len) != len)
    return 1;

  if ((IDB_LOG_WWW_OUTPUT & eyedblib::log_mask) && !is_empty_buf(s))
    IDB_LOG(IDB_LOG_WWW_OUTPUT, ("'%s'\n", s));

  return 0;
}

int
idbWDest::cflush(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *out = eyedblib::getFBuffer(fmt, ap);
  vsprintf(out, fmt, ap);
  va_end(ap);

  int len = strlen(out);

  if (rpc_socketWrite(fd, (void *)out, len) != len)
    return 1;

  if ((IDB_LOG_WWW_OUTPUT & eyedblib::log_mask) && !is_empty_buf(out))
    IDB_LOG(IDB_LOG_WWW_OUTPUT, ("'%s'\n", out));
  return 0;
}

int
idbWDest::flush(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *out = eyedblib::getFBuffer(fmt, ap);
  vsprintf(out, fmt, ap);
  va_end(ap);

  const char *s = getIndent();

  int len = strlen(s);

  if  (rpc_socketWrite(fd, (void *)s, len) != len)
    return 1;

  len = strlen(out);

  if (rpc_socketWrite(fd, (void *)out, len) != len)
    return 1;

  if ((IDB_LOG_WWW_OUTPUT & eyedblib::log_mask) && !is_empty_buf(out))
    IDB_LOG(IDB_LOG_WWW_OUTPUT, ("'%s'\n", out));

  return 0;
}

