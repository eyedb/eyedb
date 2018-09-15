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


#ifndef _EYEDB_ODLGEN_UTILS_H
#define _EYEDB_ODLGEN_UTILS_H

namespace eyedb {

  class GenCodeHints {

  public:
    enum OpType {
      tGet = 0,
      tSet,
      tGetOid,
      tSetOid,
      // variable dimension attribute
      tGetCount,
      tSetCount,
      // collections
      tGetColl,
      tSetColl,
      tAddItemToColl,
      tRmvItemFromColl,
      tSetItemInColl,
      tUnsetItemInColl,
      tGetItemAt,
      tGetOidItemAt,
      tRetrieveItemAt,
      tRetrieveOidItemAt,
      // casting methods
      tCast,
      tSafeCast,
      tLastOp
    };

    class Style {

    public:
      Style(const char *);

      Status getStatus() const {return status;}
      const char *getString(OpType op, const char *name,
			    const char *prefix = "");

      ~Style();

    private:
      Status status;
      struct Desc {
	OpType op;
	char *fmt;
	struct CompDesc {
	  char *cfmt;
	  int cnt;
	  struct Item {
	    enum {No, Name, Prefix} which;
	    const char *(*fun)(const char *);
	  } items[4];

	  ~CompDesc() {free(cfmt);}
	} comp;
	~Desc() {free(fmt);}
      } desc[tLastOp];
      Status compile();
      void parse_file(const char *);
      Status compile(Desc *);
      static const char *opTypeStr(OpType op);
    } *style;

    // keep now for now
    enum AttrStyle {
      ImplicitAttrStyle = 10,
      ExplicitAttrStyle
    };

    enum ErrorPolicy {
      StatusErrorPolicy = 20,
      ExceptionErrorPolicy
    };

    AttrStyle attr_style; // keep it for now
    Bool gen_class_stubs;
    Bool gen_down_casting;
    Bool class_enums;
    Bool attr_cache;
    Bool gen_date;
    ErrorPolicy error_policy;
    char *dirname, *fileprefix;
    const char *stubs;
    char *c_suffix, *h_suffix;

    GenCodeHints();

    void setDirName(const char *_dirname) {
      free(dirname);
      dirname = strdup(_dirname);
    }

    void setFilePrefix(const char *_fileprefix) {
      free(fileprefix);
      fileprefix = strdup(_fileprefix);
    }

    void setCSuffix(const char *_c_suffix) {
      free(c_suffix);
      c_suffix = strdup(_c_suffix);
    }

    void setHSuffix(const char *_h_suffix) {
      free(h_suffix);
      h_suffix = strdup(_h_suffix);
    }

    Status setStyle(const char *);

    ~GenCodeHints() {
      free(dirname);
      free(fileprefix);
      free(c_suffix);
      free(h_suffix);
      delete style;
    }

  private:
    GenCodeHints(const GenCodeHints &);
    GenCodeHints& operator=(const GenCodeHints &);
  };

}

#endif
