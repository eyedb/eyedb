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
   Author: Francois Dechelle <francois@dechelle.net>
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <eyedb/eyedb.h>
#include "eyedb/DBM_Database.h"
#include <eyedblib/butils.h>
#include <eyedblib/xdr.h>
#include <eyedblib/rpc_lib.h>
#include <eyedblib/strutils.h>
#include "DatabaseExportImport.h"

using namespace eyedb;
using namespace std;

extern const std::string PROGNAME;
static unsigned int magic = 0xde728eab;

// 
// Export/import common macros
// 
#define LSEEK(FD, OFF, WH) \
   do {if (lseek(FD, OFF, WH) < 0) {perror("lseek"); return 1;}} while(0)

#define WRITE(FD, PTR, SZ) \
   do {if (write(FD, PTR, SZ) != (SZ)) {perror("write"); return 1;}} while(0)

#define READ(FD, PTR, SZ) \
   do {if (read(FD, PTR, SZ) != (SZ)) {perror("read"); return 1;}} while(0)

#define READ_XDR_16(FD, PTR, SZ) \
   do {assert(SZ == 2); \
       eyedblib::int16 i16; \
       READ(FD, &i16, SZ); \
       x2h_16_cpy(PTR, &i16);} while(0)

#define READ_XDR_16_ARR(FD, PTR2, NB) \
   do { for (int j = 0; j < NB; j++) { \
          READ_XDR_16(FD, (((short *)PTR2)+j), sizeof(*PTR2)); \
        } \
   } while(0)

#define READ_XDR_32(FD, PTR, SZ) \
   do {assert(SZ == 4); \
       eyedblib::int32 i32; \
       READ(FD, &i32, SZ); \
       x2h_32_cpy(PTR, &i32);} while(0)

#define READ_XDR_64(FD, PTR, SZ) \
   do {assert(SZ == 8); \
       eyedblib::int64 i64; \
       READ(FD, &i64, SZ); \
       x2h_64_cpy(PTR, &i64);} while(0)

#define WRITE_XDR_16(FD, PTR, SZ) \
   do {assert(SZ == 2); \
       eyedblib::int16 i16; \
       h2x_16_cpy(&i16, PTR); \
       WRITE(FD, &i16, SZ); } while(0)

#define WRITE_XDR_16_ARR(FD, PTR2, NB) \
   do { for (int j = 0; j < NB; j++) { \
          WRITE_XDR_16(FD, (((short *)PTR2)+j), sizeof(*PTR2)); \
       } \
   } while(0)

#define WRITE_XDR_32(FD, PTR, SZ) \
   do {assert(SZ == 4); \
       eyedblib::int32 i32; \
       h2x_32_cpy(&i32, PTR); \
       WRITE(FD, &i32, SZ); } while(0)

#define WRITE_XDR_64(FD, PTR, SZ) \
   do {assert(SZ == 8); \
       eyedblib::int64 i64; \
       h2x_64_cpy(&i64, PTR); \
       WRITE(FD, &i64, SZ); } while(0)


//
// Export/import common structures
//
struct DBMethod {
  char extref[64];
  char file[256];
  int fd;
  unsigned long long cdsize, rsize;
};

struct DBDatafile {
  int datfd, dmpfd;
  char file[eyedbsm::L_FILENAME];
  char name[eyedbsm::L_NAME+1];
  unsigned long long maxsize;
  unsigned int slotsize;
  unsigned int dtype;
  short dspid;
  unsigned long long dat_cdsize, dat_rsize;
  unsigned long long dmp_cdsize, dmp_rsize;
};

struct DBDataspace {
  char name[eyedbsm::L_NAME+1];
  unsigned int ndat;
  short datid[eyedbsm::MAX_DAT_PER_DSP];
};

struct DBInfo {
  char dbname[256];
  off_t offset;
  unsigned long long size;
  eyedbsm::Oid::NX nbobjs;
  unsigned int dbid;
  unsigned int ndat;
  unsigned int ndsp;
  unsigned int nmths;
  unsigned long long db_cdsize, db_rsize;
  unsigned long long omp_cdsize, omp_rsize;
  DBDatafile *datafiles;
  DBDataspace *dataspaces;
  DBMethod *mths;
};

//
// Export functions
//
static const char *
get_ompfile(const char *dbfile)
{
  static char ompfile[256];
  strcpy(ompfile, dbfile);
  char *p = strrchr(ompfile, '.');
  if (p)
    *p = 0;

  strcat(ompfile, ".omp");
  return ompfile;
}

static int
code_string(int fd, const char *s)
{
  int len = strlen(s);
  WRITE_XDR_32(fd, &len, sizeof(len));
  WRITE(fd, s, len+1);
  return 0;
}

#define min(x,y) ((x)<(y)?(x):(y))

static int
copy(int fdsrc, int fddest, unsigned long long &size, int import = 0,
     unsigned long long rsize = 0,
     int info = 0, Bool lseek_ok = True)
{
  char buf[512];
  static char const zero[sizeof buf] = { 0 };
  int n;
  off_t zeros = 0;
  int cnt;
  unsigned long long cdsize;

  if (import)
    {
      cdsize = size;
      cnt = min(sizeof(buf), cdsize);
    }
  else
    cnt = sizeof(buf);

  size = 0;

  while ((n = read(fdsrc, buf, cnt)) > 0)
    {
      size += n;

      if (memcmp(buf, zero, n) == 0)
	zeros += n;
      else if (!info)
	{
	  if (zeros)
	    {
	      if (lseek_ok)
		LSEEK(fddest, zeros, SEEK_CUR);
	      else
		while (zeros--)
		  WRITE(fddest, zero, 1);

	      zeros = 0;
	    }

	  WRITE(fddest, buf, n);
	}
      else
	zeros = 0;

      if (import)
	{
	  cnt = min(sizeof(buf), cdsize-size);
	  if (size >= cdsize)
	    break;
	}
    }

  if (n < 0)
    return 1;

  size -= zeros;

  if (import && rsize > 0)
    {
      char zero = 0;
      LSEEK(fddest, rsize-1, SEEK_SET);
      WRITE(fddest, &zero, 1);
    }

  return 0;
}

static int
code_file(int fd, int file_fd, unsigned long long &totsize, Bool info, Bool lseek_ok)
{
  struct stat st = {0};
  if (file_fd != -1) {
    if (fstat(file_fd, &st) < 0)
      {
	perror("stat");
	return 1;
      }
  }

  unsigned long long size;
  if (info)
    {
      WRITE_XDR_64(fd, &st.st_size, sizeof(st.st_size)); // real size
      if (file_fd != -1) {
	if (copy(file_fd, fd, size, 0, 0, 1))
	  return 1;
      }
      else
	size = 0;
      WRITE_XDR_64(fd, &size, sizeof(size)); // sparsify size
    }
  else
    {
      WRITE_XDR_32(fd, &magic, sizeof(magic));
      if (file_fd != -1)
	if (copy(file_fd, fd, size, 0, 0, 0, lseek_ok)) return 1;
    }

  totsize += size;
  return 0;
}

static const char *
dmpfileGet(const char *datafile)
{
  static std::string str;
  char *s = strdup(datafile);
  char *p;
  if (p = strrchr(s, '.'))
    *p = 0;
  strcat(s, ".dmp");
  str = s;
  free(s);
  return str.c_str();
}

static int
export_realize(int fd, const DbInfoDescription &dbdesc,
	       const char *dbname, unsigned int dbid,
	       eyedbsm::Oid::NX nbobjs, int dbfd, int ompfd,
	       DBDatafile datafiles[], unsigned int ndat, 
	       DBDataspace dataspaces[], unsigned int ndsp,
	       DBMethod *mths, unsigned int nmths)
{
  Bool lseek_ok = (lseek(fd, 0, SEEK_CUR) < 0 ? False : True);

  WRITE_XDR_32(fd, &magic, sizeof(magic));
  unsigned int version = getVersionNumber();
  WRITE_XDR_32(fd, &version, sizeof(version));

  if (code_string(fd, dbname))
    return 1;

  WRITE_XDR_32(fd, &nbobjs, sizeof(nbobjs));
  WRITE_XDR_32(fd, &dbid, sizeof(dbid));
  WRITE_XDR_32(fd, &ndat, sizeof(ndat));
  WRITE_XDR_32(fd, &ndsp, sizeof(ndsp));
  WRITE_XDR_32(fd, &nmths, sizeof(nmths));

  unsigned long long size = 0;

  fprintf(stderr, "Coding header...\n");

  if (code_file(fd, dbfd, size, True, lseek_ok))
    return 1;
  if (code_file(fd, ompfd, size, True, lseek_ok))
    return 1;

  int i, j;

  for (i = 0; i < ndat; i++) {
    if (code_string(fd, datafiles[i].file))
      return 1;
    if (code_string(fd, datafiles[i].name))
      return 1;
    WRITE_XDR_64(fd, &datafiles[i].maxsize, sizeof(datafiles[i].maxsize));
    WRITE_XDR_32(fd, &datafiles[i].slotsize, sizeof(datafiles[i].slotsize));
    WRITE_XDR_32(fd, &datafiles[i].dtype, sizeof(datafiles[i].dtype));
    WRITE_XDR_16(fd, &datafiles[i].dspid, sizeof(datafiles[i].dspid));
    if (code_file(fd, datafiles[i].datfd, size, True, lseek_ok))
      return 1;
    if (code_file(fd, datafiles[i].dmpfd, size, True, lseek_ok))
      return 1;
  }

  for (i = 0; i < ndsp; i++) {
    if (code_string(fd, dataspaces[i].name))
      return 1;
    WRITE_XDR_32(fd, &dataspaces[i].ndat, sizeof(dataspaces[i].ndat));
    //    WRITE_XDR_16_ARR(fd, dataspaces[i].datid, dataspaces[i].ndat * sizeof(dataspaces[i].datid[0]));
    WRITE_XDR_16_ARR(fd, dataspaces[i].datid, dataspaces[i].ndat);
  }

  for (j = 0; j < nmths; j++) {
    if (code_string(fd, mths[j].extref))
      return 1;
    if (code_string(fd, mths[j].file))
      return 1;
    if (code_file(fd, mths[j].fd, size, True, lseek_ok))
      return 1;
  }

  WRITE_XDR_64(fd, &size, sizeof(size));

  //fprintf(stderr, "TOTAL SIZE = %d [%d]\n", size, lseek(fd, 0, SEEK_CUR));

  LSEEK(dbfd, 0, SEEK_SET);
  fprintf(stderr, "Coding file %s...\n", dbdesc.dbfile);
  if (code_file(fd, dbfd, size, False, lseek_ok)) return 1;
  LSEEK(ompfd, 0, SEEK_SET);
  fprintf(stderr, "Coding file %s...\n", get_ompfile(dbdesc.dbfile));
  if (code_file(fd, ompfd, size, False, lseek_ok)) return 1;

  for (i = 0; i < ndat; i++) {
    if (datafiles[i].datfd != -1)
      LSEEK(datafiles[i].datfd, 0, SEEK_SET);
    fprintf(stderr, "Coding file %s...\n", datafiles[i].file);
    if (code_file(fd, datafiles[i].datfd, size, False, lseek_ok))
      return 1;
    fprintf(stderr, "Coding file %s...\n", dmpfileGet(datafiles[i].file));
    if (datafiles[i].dmpfd != -1)
      LSEEK(datafiles[i].dmpfd, 0, SEEK_SET);
    if (code_file(fd, datafiles[i].dmpfd, size, False, lseek_ok))
      return 1;
  }

  for (j = 0; j < nmths; j++) {
    LSEEK(mths[j].fd, 0, SEEK_SET);
    fprintf(stderr, "Coding file %s...\n", mths[j].file);
    if (code_file(fd, mths[j].fd, size, False, lseek_ok))
      return 1;
  }

  return 0;
}

static int
is_system_method(const char *name)
{
  return !strncmp(name, "etcmthbe", strlen("etcmthbe")) ||
    !strncmp(name, "etcmthfe", strlen("etcmthfe")) ||
    !strncmp(name, "oqlctbmthbe", strlen("oqlctbmthbe")) || 
    !strncmp(name, "oqlctbmthfe", strlen("oqlctbmthfe")) ||
    !strncmp(name, "utilsmthfe", strlen("utilsmthfe")) ||
    !strncmp(name, "utilsmthfe", strlen("utilsmthfe"));
}

static int
methods_manage(Database *db, DBMethod*& mths, unsigned int& nmths)
{
  db->transactionBegin();

  OQL q( db, "select method");

  ObjectArray obj_arr;
  q.execute(obj_arr);

  int err = 0;
  mths = 0;
  nmths = 0;
  int n = obj_arr.getCount();
  if (n) {
    mths = new DBMethod[n];
    memset(mths, 0, sizeof(mths[0])*n);
    for (int i = 0; i < n; i++) {
      Method *m = (Method *)obj_arr[i];
      if (m->asBEMethod_OQL())
	continue;
      const char *extref = m->getEx()->getExtrefBody().c_str();
      if (is_system_method(extref))
	continue;

      int j;
      for (j = 0; j < nmths; j++)
	if (!strcmp(mths[j].extref, extref))
	  break;

      if (j == nmths) {
	int r = nmths;
	const char *s = Executable::getSOFile(extref);

	if (!s) {
	  std::cerr << PROGNAME << ": error: cannot find file for extref '" << extref << "'\n";
	  err = 1;
	  continue;
	}

	if ((mths[nmths].fd = open(s, O_RDONLY)) < 0) {
	  std::cerr << PROGNAME << ": error: cannot open method file '" << (const char *)s << "' for reading\n";
	  err = 1;
	  continue;
	}
	      
	strcpy(mths[nmths].extref, extref);
	strcpy(mths[nmths].file, Executable::makeExtRef(extref));
	nmths++;

	if (nmths == r) {
	  std::cerr << PROGNAME << ": error: no '" << Executable::makeExtRef(extref) << "' method file found.\n";
	  err = 1;
	  strcpy(mths[nmths++].extref, extref);
	}
      }
    }
  }

  db->transactionAbort();
  return err;
}

int eyedb::databaseExport( Connection &conn, const char *dbname, const char *file)
{
  if (!eyedb::ServerConfig::getSValue("sopath")) {
    std::cerr << PROGNAME << ": error: variable 'sopath' must be set in your configuration file.\n";
    return 1;
  }

  Database *db = new Database(dbname);

  db->open( &conn, Database::DBSRead);

  db->transactionBeginExclusive();

  unsigned int dbid = db->getDbid();
  DbCreateDescription dbdesc;
  db->getInfo( &conn, 0, 0, &dbdesc);

  int dbfd = open(dbdesc.dbfile, O_RDONLY);
  if (dbfd < 0) {
    std::cerr << PROGNAME << ": error: cannot open dbfile '" << dbdesc.dbfile << "' for reading\n";
    return 1;
  }

  const char *ompfile = get_ompfile(dbdesc.dbfile);
  int ompfd = open(ompfile, O_RDONLY);
  if (ompfd < 0) {
    std::cerr << PROGNAME << ": error: cannot open ompfile '" << ompfile << "' for reading\n";
    return 1;
  }

  const eyedbsm::DbCreateDescription *s = &dbdesc.sedbdesc;

  DBDatafile *datafiles = new DBDatafile[s->ndat];
  memset(datafiles, 0, sizeof(datafiles[0])*s->ndat);
  char *pathdir = strdup(dbdesc.dbfile);
  char *x = strrchr(pathdir, '/');
  if (x) *x = 0;
  
  for (int i = 0; i < s->ndat; i++) {
    const char *file = s->dat[i].file;
    const char *p = strrchr(file, '/');

    if (!*s->dat[i].file)
      *datafiles[i].file = 0;
    else if (p)
      strcpy(datafiles[i].file, p+1);
    else {
      strcpy(datafiles[i].file, file);
      file = strdup((std::string(pathdir) + "/" + std::string(s->dat[i].file)).c_str());
    }

    if (*datafiles[i].file) {
      if ((datafiles[i].datfd = open(file, O_RDONLY)) < 0) {
	std::cerr << PROGNAME << ": error: cannot open datafile '" << file << "' for reading.\n";
	return 1;
      }

      if ((datafiles[i].dmpfd = open(dmpfileGet(file), O_RDONLY)) < 0) {
	std::cerr << PROGNAME << ": error: cannot open dmpfile '" << dmpfileGet( file) << "' for reading.\n";
	return 1;
      }
    }
    else {
      datafiles[i].datfd = -1;
      datafiles[i].dmpfd = -1;
    }

    strcpy(datafiles[i].name, s->dat[i].name);
    datafiles[i].maxsize = s->dat[i].maxsize;
    datafiles[i].slotsize = s->dat[i].sizeslot;
    datafiles[i].dtype = s->dat[i].dtype;

    datafiles[i].dspid = s->dat[i].dspid;
  }

  DBDataspace *dataspaces = new DBDataspace[s->ndsp];

  for (int i = 0; i < s->ndsp; i++) {
    strcpy(dataspaces[i].name, s->dsp[i].name);
    dataspaces[i].ndat = s->dsp[i].ndat;
    memcpy(dataspaces[i].datid, s->dsp[i].datid,
	   s->dsp[i].ndat*sizeof(s->dsp[i].datid[0]));
  }

  int fd;

  if (!strcmp(file, "-"))
    fd = 1;
  else {
    fd = creat(file, 0666);

    if (fd < 0) {
      std::cerr << PROGNAME << ": error: cannot create file '" << file << "'\n";
      return 1;
    }
  }

  unsigned int nmths;
  DBMethod *mths;

  if (methods_manage(db, mths, nmths))
    return 1;

  if (export_realize(fd, dbdesc, dbname, dbid, s->nbobjs, dbfd, ompfd,
		     datafiles, s->ndat,
		     dataspaces, s->ndsp,
		     mths, nmths)) {
    close(fd);
    return 1;
  }

  close(fd);

  db->transactionCommit();

  return 0;
}

//
// Import functions
//

// Global variables...
static const char *dbmdb;
static char userauth[32];
static char passwdauth[10];

static void
getansw(const char *msg, char *&rdir, const char *defdir, bool isdir = True)
{
  char dir[256];
  *dir = 0;

  // (FD) Why is this commented? Looks reasonnible?
  //while (!*dir || *dir != '/')
  {
    printf(msg);
    if (defdir)
      printf(" [%s]", defdir);
    printf(" ? ");

    if (!fgets(dir, sizeof(dir)-1, stdin))
      exit(1);

    if (strrchr(dir, '\n'))
      *strrchr(dir, '\n') = 0;

    if (!*dir && defdir)
      strcpy(dir, defdir);

    if (isdir && *dir && *dir != '/')
      printf("Please, needs an absolute path.\n");
  }

  rdir = strdup(dir);
}

static int
decode_info_file(int fd, unsigned long long &rsize, unsigned long long &cdsize)
{
  READ_XDR_64(fd, &rsize, sizeof(rsize));
  READ_XDR_64(fd, &cdsize, sizeof(cdsize));

  return 0;
}

static int
decode_string(int fd, char s[])
{
  int len;
  READ_XDR_32(fd, &len, sizeof(len));
  READ(fd, s, len+1);
  return 0;
}

static int
decode_file(int fd, const char *f, const char *, const char *dir,
	    unsigned long long cdsize, unsigned long long rsize,
	    int check_if_exist = 0)
{
  unsigned int m;
  READ_XDR_32(fd, &m, sizeof(m));

  if (m != magic) {
    std::cerr << PROGNAME << ": error: invalid EyeDB export file while decoding file '" << f << "'\n";
    return 1;
  }

  char file[256];
  int fddest;
  Bool lseek_ok = True;

  if (*dir)
    sprintf(file, "%s/%s", dir, f);
  else
    sprintf(file, f);

  std::cerr << "Decoding file " << file << "...\n";

  if (is_system_method(f)) {
    fddest = open(file, O_RDONLY);
    strcpy(file, "/dev/null");
    lseek_ok = False;
  }
  else {
    if (check_if_exist) {
      fddest = open(file, O_RDONLY);
      if (fddest >= 0) {
	std::cerr << PROGNAME << ": error: file '" << file << "' already exists: cannot overwrite it.\n";
	close(fddest);
	return 1;
      }
    }

    fddest = creat(file, 0600);
  }

  if (fddest < 0) {
    std::cerr << PROGNAME <<  ": error: cannot create file '" << file << "'\n";
    return 1;
  }
  
  if (copy(fd, fddest, cdsize, 1, rsize, 0, lseek_ok))
    return 1;

  return 0;
}

static int
dbimport_getinfo(int fd, const char *file, DBInfo &info, unsigned int &version)
{
  unsigned int m;
  READ_XDR_32(fd, &m, sizeof(m));

  if (m != magic) {
    std::cerr << PROGNAME <<  ": error: invalid EyeDB export file\n";
    return 1;
  }

  READ_XDR_32(fd, &version, sizeof(version));

  if (decode_string(fd, info.dbname))
    return 1;

  READ_XDR_32(fd, &info.nbobjs, sizeof(info.nbobjs));
  READ_XDR_32(fd, &info.dbid, sizeof(info.dbid));
  READ_XDR_32(fd, &info.ndat, sizeof(info.ndat));
  READ_XDR_32(fd, &info.ndsp, sizeof(info.ndsp));
  READ_XDR_32(fd, &info.nmths, sizeof(info.nmths));

  if (decode_info_file(fd, info.db_rsize, info.db_cdsize))
    return 1;

  if (decode_info_file(fd, info.omp_rsize, info.omp_cdsize))
    return 1;

  info.datafiles = new DBDatafile[info.ndat];
  memset(info.datafiles, 0, sizeof(info.datafiles[0])*info.ndat);

  for (int i = 0; i < info.ndat; i++) {
    if (decode_string(fd, info.datafiles[i].file))
      return 1;
    if (decode_string(fd, info.datafiles[i].name))
      return 1;
    READ_XDR_64(fd, &info.datafiles[i].maxsize, sizeof(info.datafiles[i].maxsize));
    READ_XDR_32(fd, &info.datafiles[i].slotsize, sizeof(info.datafiles[i].slotsize));
    READ_XDR_32(fd, &info.datafiles[i].dtype, sizeof(info.datafiles[i].dtype));
    READ_XDR_16(fd, &info.datafiles[i].dspid, sizeof(info.datafiles[i].dspid));
    if (decode_info_file(fd, info.datafiles[i].dat_rsize, info.datafiles[i].dat_cdsize))
      return 1;
    if (decode_info_file(fd, info.datafiles[i].dmp_rsize, info.datafiles[i].dmp_cdsize))
      return 1;
  }

  info.dataspaces = new DBDataspace[info.ndsp];
  for (int i = 0; i < info.ndsp; i++) {
    if (decode_string(fd, info.dataspaces[i].name))
      return 1;
    READ_XDR_32(fd, &info.dataspaces[i].ndat, sizeof(info.dataspaces[i].ndat));
    READ_XDR_16_ARR(fd, info.dataspaces[i].datid, info.dataspaces[i].ndat);
  }

  if (info.nmths) {
    info.mths = new DBMethod[info.nmths];
    memset(info.mths, 0, sizeof(info.mths[0]));
    for (int j = 0; j < info.nmths; j++) {
      if (decode_string(fd, info.mths[j].extref))
	return 1;
      if (decode_string(fd, info.mths[j].file))
	return 1;
      if (decode_info_file(fd, info.mths[j].rsize, info.mths[j].cdsize))
	return 1;
    }
  }

  READ_XDR_64(fd, &info.size, sizeof(info.size));

  return 0;
}

#define MIN_DATAFILE_1 5000
#define MIN_DATAFILE   1000

static int
check_datafile_size(int ind, const eyedbsm::Datafile *datf)
{
  if (!ind && datf->maxsize < MIN_DATAFILE_1)
    {
      std::cerr << PROGNAME << ": error: datafile " << datf->file << ": the KB size of the first datafile must be greater than " << MIN_DATAFILE_1 << "[" << datf->maxsize << "Kb]\n";
      return 1;
    }
      
  if (datf->maxsize < MIN_DATAFILE)
    {
      std::cerr << PROGNAME << "error: datafile " << datf->file << ": the KB size of a datafile must be greater than " << MIN_DATAFILE << "[" << datf->maxsize << "Kb]\n";
      return 1;
    }

  return 0;
}

static const char *
get_dir(const char *s)
{
  char *p = strdup(s);
  char *q = strrchr(p, '/');
  if (!q)
    {
      free(p);
      return "";
    }

  *q = 0;
  static std::string str;
  str = p;
  return str.c_str();
}

static const char datext[] = ".dat";

static std::string
getDatafile(const char *dbname, const char *dbfile)
{
  if (strcmp(dbname, DBM_Database::getDbName()))
    return std::string(dbname) + datext;

  const char *start = strrchr(dbfile, '/');
  const char *end   = strrchr(dbfile, '.'); 
 
  if (!start) 
    start = dbfile;
  else 
    start++; 
  
  if (!end) 
    end = &dbfile[strlen(dbfile)-1]; 
  else 
    end--; 
 
  int len = end-start+1;
  char *s = new char[len+strlen(datext)+1];
  strncpy(s, &dbfile[start - dbfile], len);
  s[len] = 0; 
  strcat(s, datext);
  std::string datafile = s;
  delete [] s;
  return datafile;
}

// DEF_NBOBJS is equal to 10 M objects
const int DEF_NBOBJS = 10000000;
#define DEF_DATSIZE_STR "2000"
#define DEF_SIZESLOT_STR "16"
#define DEF_DATTYPE_STR "log"

static int
dbcreate_prologue(Database *db, const char *dbname,
		  char *&dbfile, char *datafiles[], int &datafiles_cnt,
		  const char *deffiledir, DbCreateDescription *dbdesc,
		  eyedbsm::Oid::NX nbobjs = DEF_NBOBJS, int dbid = 0)
{
  std::string dirname = (deffiledir ? deffiledir : "");
  if (dirname.length())
    dirname += "/";

  if (!dbfile) {
    if (strcmp(dbname, DBM_Database::getDbName()))
      dbfile = strdup((dirname + dbname + ".dbs").c_str());
    else {
      dbfile = strdup(Database::getDefaultServerDBMDB());
      //dbfile = strdup(Database::getDefaultDBMDB());
    }
  }
  else if (dbfile[0] != '/')
    dbfile = strdup((dirname + dbfile).c_str());

  strcpy(dbdesc->dbfile, dbfile);
  eyedbsm::DbCreateDescription *d = &dbdesc->sedbdesc;
  d->dbid     = dbid;
  d->nbobjs   = nbobjs;

  if (!datafiles_cnt) {
    datafiles_cnt = 5;
    datafiles[0] = strdup(getDatafile(dbname, dbfile).c_str());
    datafiles[1] = "";
    datafiles[2] = DEF_DATSIZE_STR;
    datafiles[3] = (char *)DEF_SIZESLOT_STR;
    datafiles[4] = (char *)DEF_DATTYPE_STR;
  }

  assert(!(datafiles_cnt%5));
  d->ndat = datafiles_cnt/5;
  for (int i = 0, j = 0; i < datafiles_cnt; i += 5, j++) {
    strcpy(d->dat[j].file, datafiles[i]);
    strcpy(d->dat[j].name, datafiles[i+1]);
    d->dat[j].maxsize = atoi(datafiles[i+2]) * 1024;
    if (*d->dat[j].file && check_datafile_size(j, &d->dat[j]))
      return 1;
    
    d->dat[j].mtype = eyedbsm::BitmapType;
    d->dat[j].sizeslot = atoi(datafiles[i+3]);

    // added 20/10/05
    d->dat[j].dspid = 0;
    // changed 2/12/05
    d->dat[j].dspid = j;

    if (!strcasecmp(datafiles[i+4], "phy"))
      d->dat[j].dtype = eyedbsm::PhysicalOidType;
    else if (!strcasecmp(datafiles[i+4], "log"))
      d->dat[j].dtype = eyedbsm::LogicalOidType;
    else
      return 1;
  }

  return 0;
}

static int
dbcreate_realize( Connection &conn, char *dbname,  char *dbfile,
		  char *datafiles[], int datafiles_cnt, const char *filedir,
		  eyedbsm::Oid::NX nbobjs, int dbid = 0)
{
  DbCreateDescription dbdesc;

  Database *db = new Database(dbname, dbmdb);

  if (dbcreate_prologue(db, dbname, dbfile, datafiles, datafiles_cnt,
			filedir, &dbdesc, nbobjs, dbid))
    return 1;

  db->create( &conn, userauth, passwdauth, &dbdesc);

  return 0;
}

static int
dbdelete_realize( Connection &conn, char *dbname)
{
  Database *db = new Database(dbname, dbmdb);

  db->remove( &conn, userauth, passwdauth);

  return 0;
}

static int
dbimport_realize( Connection &conn, int fd, const char *file, const char *dbname, const char *_filedir, const char *_mthdir, bool lseek_ok, DBInfo &info)
{
  if (lseek_ok && !dbname) {
    char s[128];
    printf("database [%s]? ", info.dbname);

    if (!fgets(s, sizeof(s)-1, stdin))
      return 1;

    if (strrchr(s, '\n'))
      *strrchr(s, '\n') = 0;

    if (*s)
      strcpy(info.dbname, s);
  }
  else {
    if (!dbname) {
      std::cerr << PROGNAME << ": error: use -db option when importing from a pipe.\n";
      return 1;
    }

    strcpy(info.dbname, dbname);
  }

  char *dbdir = 0;
  char **filedir = new char*[info.ndat];
  memset(filedir, 0, sizeof(*filedir)*info.ndat);

  int i;
  if (lseek_ok && !_filedir) {
    getansw("Directory for Database File",
	    dbdir, eyedb::ServerConfig::getSValue("databasedir"));

    for (i = 0; i < info.ndat; i++) {
      if (!*info.datafiles[i].file)
	continue;
      printf("Datafile #%d%s:\n", i,
	     (*info.datafiles[i].name ? std::string(" (") + info.datafiles[i].name + ")" : std::string("")).c_str());
      char *name;
      getansw("  File name", name, info.datafiles[i].file, False);
      strcpy(info.datafiles[i].file, name);
      getansw("  Directory", filedir[i], (!i ? "" : filedir[i-1]));
    }
  }
  else {
    if (!_filedir)
      _filedir = eyedb::ServerConfig::getSValue("databasedir");

    /*
      for (i = 0; i < info.ndat; i++)
      filedir[i] = (char *)_filedir;
    */

    dbdir = (char *)_filedir;
  }

  char **mthdir = 0;
  if (info.nmths) {
    std::string def_mthdir = std::string(eyedb::ServerConfig::getSValue("sopath"));
    mthdir = new char*[info.nmths];
    if (lseek_ok && !_mthdir) {
      for (i = 0; i < info.nmths; i++)
	getansw((std::string("Directory for Method File '") + info.mths[i].file +
		 "'").c_str(),
		mthdir[i], (!i ? def_mthdir.c_str() : mthdir[i-1]));
    }
    else {
      if (!_mthdir)
	_mthdir = strdup(def_mthdir.c_str());

      for (i = 0; i < info.nmths; i++)
	mthdir[i] = (char *)_mthdir;
    }
  }

  std::string dbfile = std::string(dbdir) + "/" + std::string(info.dbname) + ".dbs";
  std::string ompfile = std::string(dbdir) + "/" + std::string(info.dbname) + ".omp";
  char **datafiles = new char *[info.ndat*5];

  for (int j = 0; j < info.ndat; j++) {
    std::string s;
    if (!filedir[j] || !*filedir[j])
      datafiles[5*j] = strdup(info.datafiles[j].file);
    else
      datafiles[5*j] = strdup((std::string(filedir[j]) + "/" + info.datafiles[j].file).c_str());
    datafiles[5*j+1] = strdup(info.datafiles[j].name);
    datafiles[5*j+2] = strdup(str_convert((long)info.datafiles[j].maxsize/1024).c_str());
    datafiles[5*j+3] = strdup(str_convert((long)info.datafiles[j].slotsize).c_str());
    datafiles[5*j+4] = strdup(info.datafiles[j].dtype == eyedbsm::PhysicalOidType ? "phy" : "log");
  }
      
  printf("\nCreating database %s...\n", info.dbname);
  if (dbcreate_realize( conn, info.dbname, (char *)dbfile.c_str(), 
			datafiles, info.ndat*5, (const char *)0,
			info.nbobjs, info.dbid))
    return 1;

  if (decode_file(fd, (std::string(info.dbname) + ".dbs").c_str(), info.dbname,
		  dbdir, info.db_cdsize, info.db_rsize)) {
    dbdelete_realize( conn, info.dbname);
    return 1;
  }

  if (decode_file(fd, (std::string(info.dbname) + ".omp").c_str(), info.dbname,
		  dbdir, info.omp_cdsize, info.omp_rsize)) {
    dbdelete_realize( conn,info.dbname);
    return 1;
  }

  for (i = 0; i < info.ndat; i++) {
    if (!*info.datafiles[i].file)
      continue;
    if (decode_file(fd, info.datafiles[i].file,
		    info.dbname,
		    (filedir[i] && *filedir[i] == '/' ? filedir[i] : dbdir),
		    info.datafiles[i].dat_cdsize, info.datafiles[i].dat_rsize)) {
      dbdelete_realize( conn, info.dbname);
      return 1;
    }

    if (decode_file(fd, dmpfileGet(info.datafiles[i].file),
		    info.dbname,
		    (filedir[i] && *filedir[i] == '/' ? filedir[i] : dbdir),
		    info.datafiles[i].dmp_cdsize, info.datafiles[i].dmp_rsize)) {
      dbdelete_realize( conn, info.dbname);
      return 1;
    }
  }

  eyedbsm::DbRelocateDescription rel;
  rel.ndat = info.ndat;
  for (i = 0; i < info.ndat; i++)
    strcpy(rel.file[i], datafiles[5*i]);

  eyedbsm::Status se = eyedbsm::dbRelocate(dbfile.c_str(), &rel);
  if (se) {
    eyedbsm::statusPrint(se, "relocation failed");
    dbdelete_realize( conn, info.dbname);
    return 1;
  }

  for (i = 0; i < info.nmths; i++) {
    if (decode_file(fd, info.mths[i].file, "", mthdir[i], 
		    info.mths[i].cdsize, info.mths[i].rsize, 1)) {
      dbdelete_realize( conn, info.dbname);
      return 1;
    }
  }

  return 0;
}

static void
dbinfo_display(const char *msg, unsigned long long cdsize,
	       unsigned long long rsize)
{
  printf("  %-15s %lldb [real size %lldb]\n", msg, cdsize, rsize);
}

static int
dbimport_list(const char *file, DBInfo &info, unsigned int version)
{
  if (!strcmp(file, "-") || !file)
    printf("\nImport File <stdin>\n\n");
  else
    printf("\nImport File \"%s\"\n\n", file);

  printf("  Version         %d\n", version);
  printf("  Database        %s\n", info.dbname);
  printf("  Dbid            %d\n", info.dbid);
  printf("  Total Size      %llub\n", info.size);

  dbinfo_display("Database File", info.db_cdsize, info.db_rsize);
  dbinfo_display("Object Map File", info.omp_cdsize, info.omp_rsize);

  printf("  Datafile Count  %d\n", info.ndat);

  for (int i = 0; i < info.ndat; i++) {
    dbinfo_display((std::string("Datafile #") + str_convert((long)i)).c_str(),
		   info.datafiles[i].dat_cdsize, info.datafiles[i].dat_rsize);
    dbinfo_display("  Dmpfile",
		   info.datafiles[i].dmp_cdsize, info.datafiles[i].dmp_rsize);
    printf("    File          %s\n", info.datafiles[i].file);
    if (*info.datafiles[i].name)
      printf("    Name          %s\n", info.datafiles[i].name);
    printf("    Maxsize       %lldKb\n", info.datafiles[i].maxsize);
    printf("    Slotsize      %db\n", info.datafiles[i].slotsize);
    printf("    Type          %s\n",
	   info.datafiles[i].dtype == eyedbsm::PhysicalOidType ? "Physical" : "Logical");
    printf("    Dspid         #%d\n", info.datafiles[i].dspid);
  }

  printf("  Dataspace Count  %d\n", info.ndsp);
  for (int i = 0; i < info.ndsp; i++) {
    printf("  Dataspace    #%d\n", i);
    printf("    Name    %s\n", info.dataspaces[i].name);
    printf("    Datafile Count    %d\n", info.dataspaces[i].ndat);
    for (int j = 0; j < info.dataspaces[i].ndat; j++)
      printf("      Datafile #%d\n", info.dataspaces[i].datid[j]);
  }

  if (info.nmths)
    printf("\n");

  for (int i = 0; i < info.nmths; i++) {
    std::string s = (std::string)"Method File (" + info.mths[i].extref + ")";
    printf("  %-15s '%s' [real size %db]\n", s.c_str(),
	   info.mths[i].file,
	   info.mths[i].rsize);
  }

  return 0;
}

static void
read_pipe(int fd)
{
  if (fd == 0) {
    char buf[8192];
    while (read(fd, buf, sizeof(buf)) > 0)
      ;
  }
}

static const char *getUserName(int uid)
{
  struct passwd *p = getpwuid(uid);
  if (!p)
    return "<unknown>";
  return p->pw_name;
}

int eyedb::databaseImport( Connection &conn, const char *dbname, const char *file, const char *filedir, const char *mthdir, bool listOnly)
{
  int fd;
  
  if (!strcmp(file, "-"))
    fd = 0;
  else {
    fd = open(file, O_RDONLY);

    if (fd < 0) {
      std::cerr << PROGNAME << ": error: cannot open file '" << file << "' for reading.\n";
      return 1;
    }
  }

  bool lseek_ok = (fd == 0 ? False : True);

  DBInfo info;
  unsigned int version;
  if (dbimport_getinfo(fd, file, info, version))
    return 1;

  if (listOnly) {
    int r = dbimport_list(file, info, version);
    read_pipe(fd);
    return r;
  }

  if (conn.getServerUid() != getuid()) {
    std::cerr << PROGNAME 
	      << "EyeDB server is running under user '" 
	      << getUserName(conn.getServerUid()) 
	      << "': database importation must be done under the same uid than the EyeDB server.\n";
    return 1;
  }

  //  auth_realize();

  return dbimport_realize( conn, fd, file, dbname, filedir, mthdir, lseek_ok, info);
}

