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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int
realize(const char *input)
{
  char *x = strdup(input);
  char *str = x;
  char c;
  while (c = *input) {
    if (c == '=' || c == '&')
      *x++ = ' ';
    else if (c == '%') {
      char s[3];
      s[0] = *++input;
      s[1] = *++input;
      s[2] = 0;
      int cn;
      sscanf(s, "%X", &cn);
      if (cn == '\r')
	cn = '\n';
      *x++ = cn;
    }
    else if (c == '_') {
      if (!strncmp(input, "_x_", 3)) {
	*x++ = ' ';
	input++;
	input++;
      }
      else
	*x++ = c;
    }
    else
      *x++ = c;
    
    input++;
  }
  
  *x = 0;
  write(1, str, strlen(str));
  free(str);
  return 0;
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <string>\n", argv[0]);
    return 1;
  }

  return realize(argv[1]);
}
