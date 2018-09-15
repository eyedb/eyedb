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


#ifndef _eyedb_eyedbcgip_
#define _eyedb_eyedbcgip_

#include "eyedb/eyedb.h"
#include "../eyedb/Attribute_p.h"
#include "eyedbcgi.h"
/* #include "eyedb/idbdbm.h" */
#include "eyedblib/rpc_lib.h"
/* #include "eyedb/conf.h" */
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>
#include "eyedbcgiprot.h"

using namespace eyedb;

class idbWProcess;

#define idbWCOOKIE_SIZE 24
#define idbWSTR_LEN     32

#define idbW_SET(X) \
memset(X, 0, sizeof(X)); \
strcpy(X, _##X)

enum idbWTBool {
  idbWTFalse,
  idbWTTrue,
  idbWTUndef
};

enum idbWAction {
  idbW_NOACTION = 0,
  idbW_GETCOOKIE,
  idbW_DBDLGGEN,
  idbW_SCHGEN,
  idbW_SCHSTATS,
  idbW_IDLGEN,
  idbW_ODLGEN,
  idbW_REQDLGGEN,
  idbW_DUMPCONF,
  idbW_LOADCONF,
  idbW_REQCPDLGGEN,
  idbW_DUMPGEN,
  idbW_DUMPNEXTGEN,
  idbW_DUMPPREVGEN
};

struct idbWLookConfig {
  char *head_str;
  char *tail_str;
  char *bgcolor;
  char *textcolor;
  char *linkcolor;
  char *vlinkcolor;
  char *tablebgcolor;
  char *fieldcolor;
  char *errorbgcolor;
  int border;

  idbWLookConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    free(head_str);
    free(tail_str);
    free(bgcolor);
    free(fieldcolor);
    free(textcolor);
    free(linkcolor);
    free(vlinkcolor);
    free(tablebgcolor);
    free(errorbgcolor);
  }
};

struct idbWMainPageConfig {
  char *url;
  char *head;
  Bool classes;
  Bool schema;
  Bool idl;
  Bool odl;
  Bool stats;
  Bool oqlform;
  Bool webconf;
  int cls_cnt;
  struct Config {
    const Class *cls;
    Bool link;
    Config() {
      memset(this, 0, sizeof(*this));
    }      
    ~Config() {
    }
  } *cls_config;

  idbWMainPageConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    free(url);
    free(head);
    delete [] cls_config;
  }
};

struct idbWDisplayConfig {
  int border;
  int collelemcount;
  int indent;
  char indent_char;
  char indent_expand_char;
  char *open;
  char *close;
  Bool type_link;

  int cls_cnt;

  struct Config {
    const Class *cls;

    int expand_cnt;
    struct Expand {    
      int attrnum;
      Bool expand;

      Expand() {
	attrnum = -1;
      };

      void set(int _attrnum, Bool b) {
	attrnum = _attrnum;
	expand = b;
      }
    } *expands;

    int attr_cnt;
    int attr_alloc_cnt;
    struct Attr {
      Bool isExtra;
      union {
	const Attribute *attr;
	struct {
	  char *name;
	  char *oql;
	} extra;
      };

      Bool isIn;
      int num;

      Attr() {isExtra = False; attr = 0; extra.name = 0; extra.oql = 0; isIn = True;}
      void set(const Attribute *_attr) {
	attr = _attr;
	isExtra = False;
	num = attr->getNum();
      }

      void set(const Class *cls, const char *_name, const char *_oql) {
	extra.name = strdup(_name);
	extra.oql = strdup(_oql);
	isExtra = True;
	num = cls->getAttributesCount();
      }

      const char *getName() const {
	return isExtra ? extra.name : attr->getName();
      }

      int getNum() const {
	return num;
      }

      void setNum(int _num) {
	num = _num;
      }

      ~Attr() {if (isExtra) {free(extra.name); free(extra.oql);}}
    } *attrs;

    Config() {
      memset(this, 0, sizeof(*this));
    }      

    ~Config() {
      delete[] expands;
      delete[] attrs;
    }
  } *cls_config;

  idbWDisplayConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    free(open);
    free(close);
    delete [] cls_config;
  }
};

struct idbWItemList {
  int item_cnt;
  struct BItem {
    char *item;
    Bool isregex;
    idbWTBool on;
    idbWTBool compare(const char *) const;
    void release();
  } *bitems;
  char *buf;
  char *orig_str;
  idbWItemList(const char *);
  ~idbWItemList();
};

struct idbWClassQueryConfig {
  Bool description;
  Bool instances_link;
  char *attribute_indexed_string;
  Bool attribute_type;
  int form_count;
  int form_depth;
  Bool form_polymorph;
  int cls_cnt;
  struct Config {
    const Class *cls;
    char *head;
    Bool description;
    Bool instances_link;
    int form_count;
    int form_depth;
    Bool form_polymorph;
    idbWItemList *items;

    Config() {
      memset(this, 0, sizeof(*this));
    }      

    ~Config() {
      free(head);
      delete items;
    }
  } *cls_config;

  idbWClassQueryConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    free(attribute_indexed_string);
    delete [] cls_config;
  }
};

struct idbWStatsConfig {
  int cls_cnt;
  struct Config {
    const Class *cls;
    char *description;
    Config() {
      memset(this, 0, sizeof(*this));
    }      
    ~Config() {
      free(description);
    }
  } *cls_config;

  idbWStatsConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    delete [] cls_config;
  }
};

struct idbWOQLFormConfig {
  int rows, cols;

  idbWOQLFormConfig() {
    memset(this, 0, sizeof(*this));
  }
  void release() {
  }
};

struct idbWClassConfig {
  int cls_cnt;
  struct Config {
    Class *cls;
    char *gettag;
    Config() {
      memset(this, 0, sizeof(*this));
    }      
    ~Config() {
      free(gettag);
    }
  } *cls_config;

  idbWClassConfig() {
    memset(this, 0, sizeof(*this));
  }

  void release() {
    delete [] cls_config;
  }
};

struct idbWGenConfig {
  idbWLookConfig look_config;
  idbWMainPageConfig mainpage_config;
  idbWDisplayConfig display_config;
  idbWClassQueryConfig clsquery_config;
  idbWStatsConfig stats_config;
  idbWOQLFormConfig oqlform_config;
  idbWClassConfig cls_config;

  idbWGenConfig() { }

  void release() {
    look_config.release();
    mainpage_config.release();
    display_config.release();
    clsquery_config.release();
    stats_config.release();
    oqlform_config.release();
    cls_config.release();
  }

 private:
  idbWGenConfig(const idbWGenConfig &);
  idbWGenConfig &operator=(const idbWGenConfig&);
};

struct idbWDBContext {
  char cookie[idbWCOOKIE_SIZE];
  char dbname[idbWSTR_LEN];
  char userauth[idbWSTR_LEN], passwdauth[idbWSTR_LEN];
  char confname[idbWSTR_LEN];
  Database *db;
  LinkedList methodList;
  LinkedList reopen_list;
  int tstamp;
  static LinkedList db_ctx_list;
  static int db_tstamp;
  idbWGenConfig config;
  WConfig *w_config;
  Database::OpenFlag mode;

  static int makeNewTStamp() {
    return ++db_tstamp;
  }

 private:
  idbWDBContext(const char *_cookie, const char *_dbname,
		const char *_userauth, const char *_passwdauth,
		Database::OpenFlag _mode,
		const char *_confname,
		Database *_db) {
    idbW_SET(cookie);
    idbW_SET(dbname);
    idbW_SET(userauth);
    idbW_SET(passwdauth);
    strcpy(confname, _confname ? _confname : "");
    db = _db;
    mode = _mode;
    w_config = 0;
    tstamp = makeNewTStamp();
  }

 public:

  static idbWDBContext *genDBContext(idbWProcess *p,
				     const char *dbname,
				     const char *userauth,
				     const char *passwdauth,
				     Database::OpenFlag mode,
				     const char *confname,
				     Database *db);

  static int checkCookie(idbWProcess *p, const char *cookie,
			 const char *dbname, const char *user,
			 const char *passwd, const char *confname,
			 unsigned int mode);

  static idbWDBContext *lookup(const char *cookie, const char *dbname,
			       const char *userauth, const char *passwdauth,
			       const char *confname,
			       unsigned int mode,
			       Bool check_db = False)
  {
    LinkedListCursor c(&db_ctx_list);

    idbWDBContext *db_ctx;

    while (db_ctx_list.getNextObject(&c, (void *&)db_ctx))
      {
	if (check_db && !db_ctx->db)
	  continue;

	if (cookie)
	  {
	    if (!strcmp(db_ctx->cookie, cookie) &&
		(!dbname || !*dbname || !strcmp(db_ctx->dbname, dbname)))
	      {
		db_ctx->tstamp = makeNewTStamp();
		return db_ctx;
	      }
	  }
	else if (dbname &&
		 !strcmp(db_ctx->dbname, dbname) &&
		 !strcmp(db_ctx->userauth, userauth) &&
		 !strcmp(db_ctx->passwdauth, passwdauth) &&
		 db_ctx->mode == mode &&
		 ((!confname && !*db_ctx->confname) ||
		  confname && !strcmp(confname, db_ctx->confname)))
	  {
	    db_ctx->tstamp = makeNewTStamp();
	    return db_ctx;
	  }
      }

    return 0;
  }

  static idbWDBContext *setup(const char *cookie,
			      const char *dbname,
			      const char *userauth,
			      const char *passwdauth,
			      Database::OpenFlag mode,
			      const char *confname,
			      Database *db);

  void setReOpen(idbWProcess *p) {
    reopen_list.insertObjectLast(p);
  }

  Bool mustReOpen(idbWProcess *p) {
    return (reopen_list.getPos(p) >= 0 ? True : False);
  }

  void unsetReOpen(idbWProcess *p) {
    reopen_list.deleteObject(p);
  }

  ~idbWDBContext() {
  }

 private:
   idbWDBContext(const idbWDBContext&);
   idbWDBContext &operator=(const idbWDBContext&);
};

#define EXPAND

struct idbWPathDesc {
  Bool expand;
  int depth;
  struct Item {
    int clsnum;
    int itemnum;
    int ind;
    Item();
    void set(int, int, int);
    ~Item();
  } *items;

  idbWPathDesc();
  void init();
  void push(int, int, int);
  void pop();
  void trace() const;
  std::string build() const;
  int operator==(const idbWPathDesc &);
  int operator!=(const idbWPathDesc &path_desc) {
    return !(*this == path_desc);
  }
  Bool isExpanded() const;
  ~idbWPathDesc();
  void release();
};

struct idbWDumpContext {

  idbWDumpContext();
  void init(Object *);
  void push(Object *o, int itemnum, int ind) {
    path.push((o ? o->getClass()->getNum() : 0), itemnum, ind);
  }
  void pop() {
    path.pop();
  }
  void trace() const;
  ~idbWDumpContext();

  void release();
  Object *rootobj;
  idbWPathDesc path;
};

struct idbWExpandContext {

  idbWExpandContext();
  int init(int &i, int argc, char *argv[]);
  std::string build(const idbWDumpContext &, Bool &is_expanded);
  Bool isExpanded(const idbWDumpContext &dump_ctx, int * = 0) const;

  void trace() const;
  int path_cnt;
  idbWPathDesc *paths;
  Bool isBuilt;
  std::string getBuiltPath();
  void release();

 private:
  char *built_path;
};

struct idbWContext {
  idbWAction action;
  idbWTextMode glb_textmod;
  Bool nohtml;
  int etc_cnt;
  int etc_alloc;
  char **etc;
  Bool cp_query;
  Database *db;
  unsigned int anchor;
  char *user;
  char *passwd;
  char *cookie;
  char *raddr;
  char *rhost;
  char *cgidir;
  char *dir;
  char *referer;
  char *als;
  char *_class;
  char *dbname;
  char *confname;
  char *cgibasedir;
  char *rootdir;
  char *imgdir;
  idbWVerboseMode verbmod;
  idbWDBContext *db_ctx;
  Bool reopen;
  Database::OpenFlag mode;
  int local;
  char *set_oid_str, *mcl_oid_str, *item_name, *start_str;
  char *totaldumpedcount_str;
  idbWExpandContext exp_ctx;
  idbWDumpContext dump_ctx;
  struct {
    char *dbname;
    char *cookie;
  } orig;

  idbWContext();

  void push(const char *_dbname) {
    if (!orig.dbname)
      {
	orig.dbname = strdup(dbname);
	orig.cookie = strdup(cookie);
      }

    free(dbname);
    dbname = strdup(_dbname);
    free(cookie);
    cookie = 0;
  }

  void make(idbWDBContext *_db_ctx) {
    free(user);
    user = strdup(_db_ctx->userauth);
    free(passwd);
    passwd = strdup(_db_ctx->passwdauth);
  }

  void restore() {
    if (!orig.dbname)
      return;

    free(dbname);
    dbname = strdup(orig.dbname);
    free(cookie);
    cookie = strdup(orig.cookie);
  }

  void release() {
    dump_ctx.release();
    exp_ctx.release();
  }
};

enum idbWProcessState {
  FREE = 1,
  BUSY,
  NOTLAUNCHED
};

struct idbWProcess {
  idbWProcessState state;
  int pipe_c2p[2];
  int pipe_p2c[2];
  int pid;
  int port;
  int fd;
  idbWProcess() {
  }
};

extern int idbW_tstamp;

struct idbWObjectContext {
  Bool unique_compute;
  Bool unique;
  const Attribute *item;
  unsigned int tstamp;

  idbWObjectContext() {
    unique_compute = False;
    unique = False;
    item = 0;
    tstamp = idbW_tstamp;
  }

  ~idbWObjectContext() {}
};

extern int idbWDumpGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWDumpContextRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWDBDialogGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWSchemaGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWSchemaStatsRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWODLGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWIDLGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWLoadConfRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWDumpConfRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWRequestDialogGenRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWGetCookieRealize(idbWProcess *p = 0, int fd = 1);
extern int idbWHead(Bool = False);
extern int idbWTail();
extern int idbWOpenDatabase(idbWProcess *p, int input_fd);
extern void idbWPrintError(const char *s);
extern void idbWFlushText(idbWDest *dest, const char *s, Bool fixed,
			   Bool isnull, Bool put_sep = True);

extern char *idbWGetDBName(int dbid);
extern int idbWCheckMultiDB(idbWProcess *p, const Oid &, Bool);
extern int idbWOpenCrossDB(const char *dbname);
extern void idbWEmptyMethodCache();
extern void idbWMethodInit();
extern int idbWHrefDB(const char *dbname);
extern void idbWBackTo();
extern void idbWHrefTo(const char *opt, const char *label);
extern void idbWHrefRedisplay(const Oid *oid_array, int oid_cnt);
extern void idbWAddTo(Oid * &oid_array, int &oid_cnt, int &oid_alloc, const Oid &oid);
extern Status
idbWDumpAgregatClass(idbWDest *dest, Class *cls, 
		     idbWTextMode textmod,
		     idbWHeaderMode headmod, Bool inTable = False);

extern int idbWGetOpts(int argc, char *argv[]);

extern void idbW_get_conf_color(char *&color, const char *resname, const char *def, idbWDest *dest = 0, const char *xdef = 0);
extern void idbW_get_conf_string(char *&s, const char *resname, const char *def, idbWDest *dest = 0, const char *xdef = 0);
extern void idbW_get_conf_uint(int &ui, const char *resname, unsigned int def, idbWDest *dest = 0, const char *xdef = 0);
extern void idbW_get_conf_bool(Bool &b, const char *resname, Bool def, idbWDest *dest = 0, const char *xdef = 0);

extern int
idbW_get_instance_number(const Class *cls, Bool reload);

extern const char *
idbW_get_clsquery_header(const Class *cls);
extern int idbW_get_clsquery_form_count(const Class *cls);
extern int idbW_get_clsquery_form_depth(const Class *cls);
extern Bool idbW_is_clsquery_form_polymorph(const Class *cls);
extern Bool idbW_should_link_class(const Class *cls);
extern const char *idbW_getGetTagOQL(Object *o);
extern int idbW_should_instances_link(const Class *cls);
extern int idbW_should_dump_description(const Class *cls);
extern const idbWItemList *idbW_get_item_list(const Class *cls);
extern idbWDisplayConfig::Config::Attr *
idbW_getAttrList(const Class *cls, int &attr_cnt);

extern Bool
idbW_is_expanded(int clsnum, int attrnum);
extern Bool
idbW_is_expanded(const char *clsname, const char *attrname);

extern idbWTBool
idbW_is_in_item_list(const idbWItemList *items, const char *item_name);

extern idbWTBool
idbW_is_in_item_list(const idbWItemList *items, AttrIdxContext &);

extern const char *
idbW_get_description(const Class *cls);

extern void idbWGarbage();

extern void idbWDisplayConf(Database *, idbWDest *);
extern void idbWTemplateConf(Database *, idbWDest *);
extern void idbWExpandConf(Database *, idbWDest *, const char *);
extern Status idbWInitConf(Database * = 0, const char * = 0,
			      const char * = 0);

extern void idbWResetOptions();

extern int
idbWMakeContinueContext(idbWDest *dest, const char action[],
			const Class *cls, const char *item_name,
			const Oid& colloid,	unsigned int start, 
			unsigned int, idbWTextMode textmod,
			Bool submit = True, Bool useTable = True);

extern Method *
getWGetTagMethod(Object *o);
extern const char *
idbW_get_tmp_file(const char *extension, int s);

extern Status
idbWOverLoadDump(Object *o, Bool &found);

#define idbWREAD_C2P(P, X, SZ) \
if (rpc_socketRead((P)->pipe_c2p[0], (void *)X, SZ) != SZ) \
{ perror("read"); abort();}

#define idbWWRITE_C2P(P, X, SZ) \
if (rpc_socketWrite((P)->pipe_c2p[1], (void *)X, SZ) != SZ) \
{ perror("write"); abort();}

#define idbWREAD_P2C(P, X, SZ) \
if (rpc_socketRead((P)->pipe_p2c[0], (void *)X, SZ) != SZ) \
{ perror("read"); abort();}

#define idbWWRITE_P2C(P, X, SZ) \
if (rpc_socketWrite((P)->pipe_p2c[1], (void *)X, SZ) != SZ) \
{ perror("write"); abort();}

extern char *idbW_head_str, *idbW_tail_str;
extern idbWContext *idbW_ctx;
extern idbWDBContext *idbW_dbctx;
extern idbWDest *idbW_dest;
extern idbWContext idbW_context;
extern std::string idbW_cmd;
extern idbWGenConfig *idbW_db_config;;
extern Connection *idbW_conn;
extern char *idbW_progname;
extern Status idbW_status;
extern int idbW_nullCount;
extern const char idbWExtent[];
extern int idbW_start_tmp, idbW_ostart_tmp;
extern const char *idbW_accessfile;
extern const char *idbW_cgidir, *idbW_alias, *idbW_wdir;
extern const int idbW_member;
extern idbWProcess *idbW_root_proc;

#define IDB_LOG_WWW IDB_LOG_USER(0)
#define IDB_LOG_WWW_OUTPUT IDB_LOG_USER(1)

#define idbW_OPERAND   "__op__"
#define idbW_FIELD     "__field__"
#define idbW_FIELD_STR "__string__"
#define idbW_FIELD_STR_LEN 10
#define idbW_LOGICAL   "__logical__"
#define idbW_PAR_OPEN  "__par_open__"
#define idbW_PAR_CLOSE "__par_close__"
#define idbW_SELECT    "__select__"
#define idbW_SELECT_LEN 10
#define idbW_ETC       "$"
#define idbW_QUOTE     "%22"
#define idbW_QUOTE_LEN 3
#define idbW_TABULAR() idbW_dest->cflush("     ")
#define idbW_NL()      idbW_dest->cflush("\n")
#define idbW_LIST_DATABASES "All"

#define idbW_STRING_SEP "\'"
#define idbW_null "null"
#define idbW_FL_LEN 35

#define idbW_HELP_PREFIX " href=\"../help/eyedb-"
#define idbW_HELP_SUFFIX "-help.html\""

#define idbW_STPR(S) do {\
  IDB_LOG(IDB_LOG_WWW, ("error: '%s'\n", ((S)->getDesc())));\
  idbWPrintError((S)->getDesc());\
} while(0)

#define idbW_CHECK_FLUSH(X) do {if (X) return Success;} while(0)

#define idbW_CHECK(X) do {\
  if (idbW_status = (X)) {\
    idbW_STPR(idbW_status); return 1; \
  } \
} while (0)

#define idbW_CHECK_S(X) do {\
   if (idbW_status = (X)) {\
     idbW_STPR(idbW_status); return idbW_status;\
  } \
} while(0)

#define idbW_OPTION(X) idbW_dest->flush("<option value=\"" X "\">" X "</option>")
#define idbW_OPTION_X(X, Y) idbW_dest->flush("<option value=\"" X "\">" Y "</option>")
#define idbW_OPTION_SELECTED(X, Y) idbW_dest->flush("<option selected value=\"" X "\">" Y "</option>")

#define idbW_PLURAL(X) ((X) > 1 ? "s" : "")

#endif
