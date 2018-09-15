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

#include <eyedbconfig.h>

#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_REGEX_H
#include <regex.h>
#elif defined(HAVE_LIBGEN_H)
#include <libgen.h>
#else
#error No regular expression implementation available on this platform
#endif

#include <eyedb/syscls.h>
#include <eyedblib/strutils.h>

using namespace std;

//
// Constants
//

namespace eyedb {

  //
  // OString user methods
  //

  OString *
  OString::ostring(Database * db)
  {
    return new OString(db);
  }

  OString *
  OString::ostring(Database * db, const char * s)
  {
    OString * os = new OString(db);

    os->assign(s);

    return os;
  }

  OString *
  OString::ostring(Database * db, const char * s, int len)
  {
    OString * os = new OString(db);

    os->assign(s, len);

    return os;
  }

  OString *
  OString::ostring(Database * db, const char * s, int offset, int len)
  {

    OString * os = new OString(db);

    os->assign(s, offset, len);

    return os;

  }

  OString *
  OString::ostring(Database * db, const OString & s)
  {
    OString * os = new OString(db);

    os->setS(s.getS());

    return os;
  }

  OString *
  OString::ostring(Database * db, char s)
  {
    std::string is = str_convert(s);

    OString * os = new OString(db);
    os->setS(is.c_str());
  
    return os;
  }

  OString *
  OString::ostring(Database * db, int s)
  {
    std::string is = str_convert(s);

    OString * os = new OString(db);
    os->setS(is.c_str());
  
    return os;
  }

  OString *
  OString::ostring(Database * db, double s)
  {
    std::string is = str_convert(s);

    OString * os = new OString(db);
    os->setS(is.c_str());
  
    return os;
  }

  char *
  OString::substr(const char * s, int offset, int len)
  {
    char * s_copy = 0;

    int s_len = strlen(s);
    if( offset >= s_len 
	|| offset < 0 
	|| len < 0 )
      {
	return 0;
      }

    if( offset + len > s_len )
      {
	len = s_len - offset;
      }

    s_copy = new char[len + 1];

    strncpy( s_copy, s + offset, len);
    s_copy[len] = '\0';
   
    return s_copy;
  }

  char *
  OString::toLower(const char * s)
  {
    char * s2 = new char[strlen(s) + 1];
    char * p = s2;

    while (*s++)
      *p++ = (char)tolower( *s);

    *p = '\0';

    return s2;
  }

  char *
  OString::toUpper(const char * s)
  {
    char * s2 = new char[strlen(s) + 1];
    char * p = s2;

    while (*s++)
      *p++ = (char)toupper( *s);

    *p = '\0';

    return s2;
  }

  char *
  OString::rtrim(const char * s)
  {
    const char * s_rtrimmed = s + strlen(s) - 1;

    while (s_rtrimmed >= s 
	   && ((*s_rtrimmed=='\n') || (*s_rtrimmed=='\r') || (*s_rtrimmed=='\t') || (*s_rtrimmed=='\v')))
      s_rtrimmed--;

    int return_value_size = s_rtrimmed - s + 1;
    char * return_value = new char[return_value_size + 1];
    strncpy(return_value, s, return_value_size);
    return_value[return_value_size] = '\0';

    return return_value;
  }

  char *
  OString::ltrim(const char * s)
  {
    const char * s_ltrimmed = s + strspn(s, "\n\r\t\v ");

    char * return_value = new char[strlen(s_ltrimmed + 1)];
    strcpy(return_value, s_ltrimmed);

    return return_value;
  }

  Status
  OString::setChar(char c, int offset)
  {
    return setS(offset, c); 
  }

  char
  OString::getChar(int offset) const
  {
    return getS(offset);
  }

  OString &
  OString::append(const char * s)
  {
    std::string is(getS());

    is += s;

    setS(is.c_str());

    return *this;
  }

  OString &
  OString::append(const char * s, int len)
  { 
    return append(s, 0 , len);
  }

  OString &
  OString::append(const char * s, int offset, int len)
  {
    char * sub = OString::substr(s, offset, len);

    if( sub != 0 )
      {
	append(sub);
	delete sub;
      }

    return *this;
  }

  OString &
  OString::prepend(const char * s)
  {
    std::string is(s);

    is += getS();

    setS(is.c_str());

    return *this;
  }

  OString &
  OString::prepend(const char * s, int len)
  {
    return prepend(s, 0, len);
  }

  OString &
  OString::prepend(const char * s, int offset, int len)
  {
    char * sub = OString::substr(s, offset, len);

    if( sub != 0 )
      {
	prepend(sub);
	delete sub;
      }

    return *this;
  }

  OString &
  OString::insert(int offset, const char * s)
  {
    return insert(offset, s, 0, strlen(s));
  }

  OString &
  OString::insert(int offset, const char * s, int len)
  {
    return insert(offset, s, 0, len);
  }

  OString &
  OString::insert(int offset, const char * s, int offset2, int len)
  {
    const char * this_s = getS().c_str();

    // check arguments
    if( offset < 0 
	|| offset2 < 0
	|| len <= 0 )
      {
	return *this;
      }

    // insert
    char * inserted_s = new char[strlen(this_s) + strlen(s) + 1];
    inserted_s[0] = '\0';
  
    strncat(inserted_s, this_s, offset);
    strncat(inserted_s, s + offset2, len);
    strcat(inserted_s, this_s + offset);

    setS(inserted_s);
  
    delete inserted_s;

    return *this;
  }

  OString *
  OString::concat(Database * db, const char * s1, const char * s2)
  {
    OString * os = OString::ostring(db, s1);

    os->append(s2);

    return os;
  }

  int
  OString::first(const char * s) const
  {
    return find(s, 0);
  }

  int
  OString::last(const char * s) const
  {
    /* not implemented */
    cerr << "Not implemented" << endl;
    return 0;
  }

  int
  OString::find(const char * s, int offset) const
  {
    const char * this_s = getS().c_str();
  
    if (offset > strlen(this_s) || offset < 0 || !*s)
      {
	return -1;
      }

    int find_offset = -1;

    char * match = (char *)strstr(this_s + offset, s);
    if( match != 0)
      {
	find_offset = match - this_s;
      }

    return find_offset;
  }

  /// @return A not persistent OString
  OString *
  OString::substr(int offset, int len) const
  {
    OString * os = OString::ostring(0, getS().c_str(), offset, len);

    return os;
  }

  /// @return A not persistent OString
  OString *
  OString::substr(const char * regexp, int offset) const
  {
    // check argument
    const char * regexp_subject = getS().c_str();

    if( offset > strlen(regexp_subject) 
	|| offset < 0)
      {
	return OString::ostring(0);
      }
  
      
    // Apply the regular expresion pattern
#ifdef HAVE_REGCOMP
    regex_t * compiled_regexp = (regex_t *)malloc(sizeof(regex_t));

    int reg_status = regcomp(compiled_regexp, regexp, REG_EXTENDED);
    if ( reg_status != 0)
      {
	free(compiled_regexp);
	return OString::ostring(0);      
      }

    regmatch_t match[1];
    reg_status = regexec((regex_t *)compiled_regexp, regexp_subject + offset, 1, match, 0);
    if ( reg_status != 0)
      {
	free(compiled_regexp);
	return OString::ostring(0);      
      }
  
    const char * start_match = regexp_subject + offset + match[0].rm_so;
    const char * end_match = regexp_subject + offset + match[0].rm_eo;
#elif defined(HAVE_REGCMP)
    char * compiled_regexp = regcmp(regexp, (char *)0);

    if(compiled_regexp == 0) 
      {
	return OString::ostring(0);
      }

    const char * end_match = regex(compiled_regexp, regexp_subject + offset);
    const char * start_match = __loc1;
#endif

    free(compiled_regexp);

    // return the matched string
    OString * os = 0;
    if( end_match )
      {
	os = OString::ostring(0, start_match, 0, end_match - start_match); 
      }
    else
      {
	os = OString::ostring(0);
      }

    return os;

  }

  OString &
  OString::erase(int offset, int len)
  {
    // check arguments
    const char * this_s = getS().c_str();

    int this_len = strlen(this_s);
    if( offset > this_len
	|| offset < 0
	|| len <= 0 )
      {
	return *this;
      }

    if( offset + len > this_len )
      {
	len = this_len - offset;
      }

    // erase = 1/substr

    char * erased_string = new char[this_len + 1];
  
    strncpy(erased_string, this_s, offset);
    strcpy(erased_string + offset, this_s + offset + len);
  
    setS(erased_string);

    delete erased_string;

    return *this;
  }

  OString &
  OString::replace(int offset, int len, const char * s)
  {
    return replace(offset, len, s, 0, strlen(s));
  }

  OString &
  OString::replace(int offset, int len, const char * s, int len2)
  {
    return replace(offset, len, s, 0, len2);
  }

  OString &
  OString::replace(int offset, int len, const char * s, int offset2, int len2)
  {
    // check arguments
    const char * this_s = getS().c_str();

    int this_len = strlen(this_s);
    int s_len = strlen(s);

    if( offset > this_len
	|| offset < 0
	|| len <= 0 
	|| offset2 > s_len
	|| offset2 < 0
	|| len2 <= 0 )
      {
	return *this;
      }

    if( offset + len > this_len )
      {
	len = this_len - offset;
      }


    // replace

    char * replaced_string = new char[this_len + s_len + 1];
    replaced_string[0] = '\0';
  
    strncat(replaced_string, this_s, offset);
    strncat(replaced_string, s + offset2, len2);
    strcat(replaced_string, this_s + offset + len);
  
    setS(replaced_string);

    delete [] replaced_string;

    return *this;
  }


  OString &
  OString::replace(const char * s1, const char * s2)
  {
    const char * this_s = getS().c_str();
    int s1_len = strlen(s1);
  

    // EV 27/08/02 : added ``+ 1''
    char * replaced_string = new char[strlen(this_s) * ( strlen(s2) + 1 ) + 1];
    replaced_string[0] = '\0';

    int current_offset = 0;
    int previous_offset = 0;
    while( (current_offset = find(s1, previous_offset)) >= 0 )
      {
	strncat(replaced_string, this_s + previous_offset, current_offset - previous_offset);
	strcat(replaced_string, s2);

	previous_offset = current_offset + s1_len;
      }

  
    if( previous_offset < strlen(this_s) )
      {
	strcat(replaced_string, this_s + previous_offset);
      }

    setS(replaced_string);

    // EV 27/08/02 : added ``[]''
    delete [] replaced_string;

    return *this;
  }

  OString &
  OString::assign(const char * s)
  {
    setS(s);

    return *this;
  }

  OString &
  OString::assign(const char * s, int len)
  {
    char * sub = OString::substr(s, 0, len);
  
    if( sub != 0 )
      {
	setS(sub);
	delete sub;
      }

    return *this;
  }

  OString &
  OString::assign(const char * s, int offset, int len)
  {
    char * sub = OString::substr(s, offset, len);
  
    if( sub != 0 )
      {
	setS(sub);
	delete sub;
      }

    return *this;
  }

  Status
  OString::reset()
  {
    return setS("");
  }

  OString &
  OString::toLower()
  {
    char * s2 = toLower(getS().c_str());
    setS(s2);
    delete s2;

    return *this;
  }

  OString &
  OString::toUpper()
  {
    char * s2 = toUpper(getS().c_str());
    setS(s2);
    delete s2;

    return *this;
  }

  OString &
  OString::rtrim()
  {  
    char * s2 = rtrim(getS().c_str());
    setS(s2);
    delete s2;
 
    return *this;
  }

  OString & 
  OString::ltrim()
  {
    char * s2 = ltrim(getS().c_str());
    setS(s2);
    delete s2;

    return *this;
  }

  int
  OString::compare(const char * s) const
  {
    return strcmp(getS().c_str(), s);
  }

  int
  OString::compare(const char * s, int to) const
  {
    return strncmp(getS().c_str(), s, to);
  }

  int
  OString::compare(const char * s, int from, int to) const
  {  
    const char * this_s = getS().c_str();

    if(from >= strlen(s)
       || from >= strlen(this_s) )
      {
	return 0;
      }

    return strncmp(this_s + from, s + from, to);
  }

  Bool
  OString::is_null() const
  {
    return ( (length() == 0) ? True : False);
  }


  Bool
  OString::match(const char * regexp) const
  {
    const char * regexp_subject = getS().c_str();

#ifdef HAVE_REGCOMP
    regex_t * compiled_regexp = (regex_t *)malloc(sizeof(regex_t));

    int reg_status = regcomp(compiled_regexp, regexp, REG_EXTENDED);
    if ( reg_status != 0)
      {
	free(compiled_regexp);
	return False;      
      }

    regmatch_t match[1];
    reg_status = regexec((regex_t *)compiled_regexp, regexp_subject, 1, match, 0);
    if ( reg_status != 0)
      {
	free(compiled_regexp);
	return False;      
      }
  
    const char * start_match = regexp_subject + match[0].rm_so;
    const char * end_match = regexp_subject + match[0].rm_eo;
#elif defined(HAVE_REGCMP)
    char * compiled_regexp = regcmp(regexp, (char *)0);

    if(compiled_regexp == 0)
      {
	return False;
      }

    char * end_match = regex(compiled_regexp, regexp_subject);
    char * start_match = __loc1;
#endif

    free(compiled_regexp);

    if( (regexp_subject + strlen(regexp_subject)) == end_match 
	&& regexp_subject ==  start_match)
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  int
  OString::length() const
  {  
    return strlen(getS().c_str());
  }

  char **
  OString::split(const char * separator, int & nb_pieces) const
  {
    //get a copy of the string
    char * s_copy = strdup(getS().c_str());

    // break the copy into tokens
    nb_pieces = 1;

    int separator_len = strlen(separator);

    int previous_offset = 0;
    int current_offset = 0;
    while( (current_offset = find(separator, previous_offset)) >= 0 )
      {
	s_copy[current_offset] = '\0';

	++nb_pieces;
	previous_offset = current_offset + separator_len;
      }

    // create and fill an array with the tokens
    typedef char * token;
    token * return_value = new token[nb_pieces];

    char * cursor = s_copy;
    for(int i=0; i < nb_pieces; ++i)
      {
	int token_len = strlen(cursor);
      
	char * current_token = new char[token_len + 1];
	strcpy(current_token, cursor);
	return_value[i] = current_token;	  

	// advance cursor
	cursor += token_len + separator_len;
      }

    // return
    free(s_copy);

    return return_value;
  }

  char **
  OString::regexp_split(const char * regexp_separator, int & nb_pieces) const
  {
    //get a copy of the string
    char * s_copy = strdup(getS().c_str());

    // prepare the regular expression
#ifdef HAVE_REGCOMP
    regex_t * compiled_separator = (regex_t *)malloc(sizeof(regex_t));

    int reg_status = regcomp(compiled_separator, regexp_separator, REG_EXTENDED);
    if ( reg_status != 0)
      {
	free(compiled_separator);
	return 0;      
      }
#elif defined(HAVE_REGCMP)
    char * compiled_separator = regcmp(regexp_separator, (char*)0);
    if( compiled_separator == 0 )
      {
	return 0;
      }
#endif

    // break the copy into tokens
    // and put them into an array
    nb_pieces = 0;

    typedef char * token;
    token * return_value = new token[strlen(s_copy)]; // strlen(s_copy) > nb_pieces

    char * cursor1 = s_copy;
    char * cursor2 = s_copy;
 
#ifdef HAVE_REGEXEC
    regmatch_t match[1];
    while( regexec( compiled_separator, cursor1, 1, match, 0) == 0)
#elif defined(HAVE_REGEX)
    while( (cursor2 = regex(compiled_separator, cursor1)) != 0 )
#endif
      // NB: the #ifdef and the loop body are breaked out this way 
      // to save automatic indentation under emacs
      {
#ifdef HAVE_REGEXEC
	char * separator = cursor1 + match[0].rm_so;
	char * cursor2 = cursor1 + match[0].rm_eo;
#elif defined(HAVE_REGEX)
	char * separator = __loc1;
#endif

	int separator_len = cursor2 - separator;
      
	int token_len = separator - cursor1;
	char * current_token = new char[ token_len + 1];
	current_token[0] = '\0';
	strncat(current_token, cursor1, token_len);
	return_value[nb_pieces] = current_token;
     
      
	++nb_pieces;
	cursor1 = cursor2;
      }

    // add the last token
    char * current_token = new char[ strlen(cursor1) + 1];
    strcpy(current_token, cursor1);
    return_value[nb_pieces] = current_token;
    ++nb_pieces;
 

    // return
    free(s_copy);
    free(compiled_separator);

    return return_value;

  }
}
    
