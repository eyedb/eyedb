/* 
   EyeDB Object Database Management System
   OQL interpreter command-line tool
   Copyright (C) 1994-2008 SYSRA
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   Author: Eric Viara <viara@sysra.com>
*/


#include <eyedb/eyedb.h>
#include <eyedb/IteratorAtom.h>

namespace eyedb {

class OQLParser;

class OQLCommand {
 public:
  OQLCommand(const char *, int (*)(OQLParser *, int, char **),
		void (*)(FILE *, OQLCommand *, bool),
		void (*)(FILE *, OQLCommand *));
  OQLCommand(const char **, int, int (*)(OQLParser *, int, char **),
		void (*)(FILE *, OQLCommand *, bool),
		void (*)(FILE *, OQLCommand *));
  int (*exec)(OQLParser *, int, char **);
  void (*usage)(FILE *, OQLCommand *, bool);
  void (*help)(FILE *, OQLCommand *);
  const char **getNames(int &_name_cnt) {_name_cnt = name_cnt; return (const char **)names;}

 private:
  friend class OQLParser;
  char **names;
  int name_cnt;
  OQLCommand *prev, *next;
};


class OQLParser {
  Bool exit_mode;
  char *prompt_str, *second_prompt_str;
  char term_char;
  char escape_char;
  OQLCommand *head, *last;
  int buffer_parse(char *, char **, int, int &);
  void set_standard_command();
  int oql_buffer_len, oql_buffer_alloc;
  char *oql_buffer;
  Status oql_send();
  Status oql_make(const char *);
  Connection *conn;
  Database *db;
  TransactionMode tr_mode;
  int setOqlCtbDatabaserealize(const char *, Database::OpenFlag, Bool);
  int last_alloc, last_cnt;
  Value query_value;
  ValueArray value_arr;
  void last_reset();
  void setLastValueArray(OQL &q);
  char *dbmdb;
  int prmode;
  Bool shmode;
  int cmmode;
  int mutemode;
  int echofound;
  Bool echo_mode;
  Bool int_mode;
  int splitLen;
  char *sep;
  char *bindOpen, *bindClose, *nl;
  int align;
  int align_left;
  int columns;
  Database *openDBM();
  int onerror_abort, onerror_quit, onerror_echo;

 public:
  enum {
    fullRecurs = 1,
    noRecurs = 2,
    none = 3
  };

  OQLParser(Connection *, int&, char *[]);
  Bool getExitMode() const;
  void setExitMode(Bool);
  int parse(FILE *, Bool = False);
  int parse(const char *, Bool);
  int parse_file(const char *);
  void prompt(FILE * = stdout);
  void setPrompt(const char *);
  void setOnErrorAbort(int on) {onerror_abort = on;}
  void setOnErrorQuit(int on) {onerror_quit = on;}
  void setOnErrorEcho(int on) {onerror_echo = on;}
  int getOnErrorAbort() const {return onerror_abort;}
  int getOnErrorQuit() const {return onerror_quit;}
  int getOnErrorEcho() const {return onerror_echo;}
  const char *getPrompt() const;
  void setSecondPrompt(const char *);
  const char *getSecondPrompt() const;

  const char *getEffectivePrompt() const;

  void setEscapeChar(char);
  char getEscapeChar() const;
  void setTermChar(char);
  void setEchoMode(Bool mode) {echo_mode = mode;}
  char getTermChar() const;
  void clear_oql_buffer();

  void quit();

  void closeDB();
  void setPrintMode(int mode) {prmode = mode;}
  int getPrintMode() const {return prmode;} 
  void setShowMode(Bool mode) {shmode = mode;}
  Bool getShowMode() const {return shmode;} 
  void setCommitOnCloseMode(int mode) {
    cmmode = mode;
    Database::setDefaultCommitOnClose(mode ? True : False);
  }
  int getCommitOnCloseMode() {return cmmode;}
  void setMuteMode(int mode) {mutemode = mode;}
  void setEchoFound(int mode) {echofound = mode;}
  int getMuteMode() const {return mutemode;} 
  int getEchoFound() const {return echofound;} 
  int getEchoMode() const {return echo_mode;} 
  OQLCommand *find_command(const char *) const;
  void add_command(OQLCommand *);
  void remove_command(OQLCommand *);
  OQLCommand *get_first_command() const;
  OQLCommand *get_next_command(OQLCommand *) const;
  int setDatabase(const char *, Database::OpenFlag, const OpenHints *);
  Database *getDatabase();
  ValueArray &getLastValueArray();
  Connection *getConnection();
  void setDbmdb(const char *);
  const char *getDbmdb() const;
  void setInterrupt(Bool b) {int_mode = b;}
  Bool getInterrupt() {return int_mode;}
  int getSplitLen() {return splitLen;}
  const char *getSep() {return sep;}
  const char *getBindOpen() {return bindOpen;}
  const char *getBindClose() {return bindClose;}
  const char *getBindNL() {return nl;}
  int getColumns() {return columns;}
  int getAlign() {return align;}
  int getAlignLeft() {return align_left;}
  void resetFormat();

  void setSplitLen(int x) {splitLen = x;}
  void setSep(const char *x) {free(sep); sep = strdup(x);}
  void setBindOpen(const char *x) {free(bindOpen); bindOpen = strdup(x);}
  void setBindClose(const char *x) {free(bindClose); bindClose = strdup(x);}
  void setBindNL(const char *x) {free(nl); nl = strdup(x);}
  void setAlign(int x, int y) {align = x; align_left = y;}
  void setColumns(int x) {columns = x;}
};

enum idbOqlOperation {
  idbOqlLocal,
  idbOqlCritical,
  idbOqlQuerying,
  idbOqlScanning
};
}
