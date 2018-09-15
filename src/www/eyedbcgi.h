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


#ifndef _eyedb_eyedbcgi_
#define _eyedb_eyedbcgi_

using namespace eyedb;
using namespace std;

/* #include <eyedb/log.h> */

// ------------------------------------------------------------------------
//
// W Types
//
// ------------------------------------------------------------------------

enum idbWTextMode {
  idbWTextSimple = 1,
  idbWTextHTML
};

enum idbWVerboseMode {
  idbWVerboseFull = 1,
  idbWVerboseRestricted
};

enum idbWHeaderMode {
  idbWHeaderNo = 1,
  idbWHeaderYes
};

class idbWDest {

public:
  idbWDest();
  idbWDest(int fd, int = 3);
  int flush(const char *fmt, ...);
  int cflush(const char *fmt, ...);
  int cflush(const char *s, Bool);

  void push();
  void pop();

  Bool isTopLevel() const {return IDBBOOL(indent == 0);}
  const char *getIndent() const;
  Bool isHtml() const {return is_html;}

  int getIndentLen() const {return indent;}

  Bool setMarks(Bool _is_marks) {
    Bool ois_marks = is_marks;
    is_marks = _is_marks;
    return ois_marks;
  }

  Bool getMarks() const {return is_marks;}
  
  ~idbWDest() {}

private:
  int indent;
  int fd;
  int indent_inc;
  Bool is_html;
  Bool is_marks;
};

// ------------------------------------------------------------------------
//
// Dump functions
//
// ------------------------------------------------------------------------

extern Status idbWDump(idbWDest *, Object *, idbWTextMode);

extern Status
idbWDumpObjectRealize(idbWDest *dest, Object *o, idbWTextMode textmod,
		      idbWVerboseMode verbmod, idbWHeaderMode headmod);

extern Status
idbWItemDump(idbWDest *dest, Object *o, const Attribute *item,
	     idbWTextMode textmod, idbWVerboseMode verbmod,
	     idbWHeaderMode headmod);

extern Status
idbWItemDumpCollection(idbWDest *dest, Object *o,
		       const Attribute *item,
		       idbWTextMode textmod, idbWVerboseMode verbmod,
		       idbWHeaderMode headmod);

extern Status
idbWItemDumpObject(idbWDest *dest, Object *o, const Attribute *item,
		   idbWTextMode textmod, idbWVerboseMode verbmod,
		   idbWHeaderMode headmod);

extern Status
idbWItemDumpBasic(idbWDest *dest, Object *o, const Attribute *item,
		  idbWTextMode textmod, idbWVerboseMode verbmod,
		  idbWHeaderMode headmod);

extern Status
idbWItemDumpEnum(idbWDest *dest, Object *o, const Attribute *item,
		 idbWTextMode textmod, idbWVerboseMode verbmod,
		 idbWHeaderMode headmod);

// ------------------------------------------------------------------------
//
// Tag functions
//
// ------------------------------------------------------------------------

extern unsigned int idbWGetSize(const Attribute *, Object *);

extern Status idbWGetTag(Object *, const char *&tag, idbWTextMode,
			    int &);

extern Status idbWGetTagEnum(const EnumClass *menum, Enum *o,
				const char *&tag, idbWTextMode, int &);

extern Status idbWGetTagClass(const Class *mclass, Object *o,
				 const char *&tag, idbWTextMode, int &);
extern Status idbWGetTagItem(const Attribute *item, Object *o,
				const char *&tag, idbWTextMode, int &);

extern const char *idbWGetTag(Object *);

extern Status idbWGetTagRealize(Object *, const char *&tag,
				   idbWTextMode, int &);

extern char *idbWMakeTag(const Oid &oid, const char *tag,
			 idbWTextMode txtmod, int &len,
			 Bool strong = False,
			 Bool reqdlggen = False);

extern const char *idbWGetEnumName(const Attribute *item, Object *o,
				   int from, int *val);

extern const char *idbWFieldName(const Class *mclass, const char *name,
				 idbWTextMode txtmod, Bool head = False);

extern Bool idbWIsString(const Attribute *);

extern char *idbWGetString(const Attribute *, Object *, Bool * = 0);

extern Bool idbWIsint16(const Attribute *);
extern Bool idbWIsint32(const Attribute *);
extern Bool idbWIsint64(const Attribute *);

extern eyedblib::int16 idbWGetInt16(const Attribute *, Object *, int, Bool * = 0);
extern eyedblib::int32 idbWGetInt32(const Attribute *, Object *, int, Bool * = 0);
extern eyedblib::int64 idbWGetInt64(const Attribute *, Object *, int, Bool * = 0);

extern Bool idbWIsChar(const Attribute *);

extern char idbWGetChar(const Attribute *, Object *, int,
			Bool * = 0);

extern Bool idbWIsFloat(const Attribute *);

extern double idbWGetFloat(const Attribute *item, Object *, int,
			   Bool * = 0);

extern Bool idbWIsEmpty(Object *, const char *&);

extern char *idbWFilterArg(const char *tag);

extern void idbWInit(int fd);
extern void idbWInit();

extern Connection *idbWConn;
extern int idbWOpenConnection();

extern const char *idbWMakeHTMLTag(const char *tag);

#define idbW_DEF_TAG_LEN 256
extern char *idbWNewTagBuf(int len = idbW_DEF_TAG_LEN);

// ------------------------------------------------------------------------
//
// External variables
//
// ------------------------------------------------------------------------

extern const char idbWUnknownTag[];

#define idbWSPACE "&"

#endif
