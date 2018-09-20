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


#include "odl.h"
//#include "imdl.h"
#include <eyedb/odlgen_utils.h>

using namespace eyedb;
using namespace std;

namespace eyedb {
  extern Bool odl_smartptr;
}

#include "GetOpt.h"

extern void
rootclass_manage(odlAgregSpec agreg_spec);

static char *package = NULL;
static char *module = NULL;
static char *prefix = (char*)"";/*@@@@warning cast*/
static char *db_prefix = (char*)"";/*@@@@warning cast*/
static char *c_namespace = NULL;
static char *schname = 0;
static Bool _export = False;
static const char *prog;
static char *odlfile = 0;

static Bool update = False;
static Bool diff = False;
static Bool checkfile = False;
static unsigned int open_flags;
static const char *dbname = NULL;
static Bool orbix_gen = False;
static Bool orbacus_gen = False;
#ifdef SUPPORT_CORBA
static Bool idl_gen = False;
#endif
static Bool odl_gen = False;
static Bool cplus_gen = False;
static Bool java_gen = False;
static GenCodeHints *hints;
static char *imdlfile = NULL;
#ifdef SUPPORT_CORBA
static char *idltarget = NULL;
#endif
static Schema *m;
static Database *db = NULL;
static char *ofile = NULL;
#ifdef SUPPORT_CORBA
static char *generic_idl = NULL;
static Bool no_generic_idl = False;
static Bool no_factory = False;
static Bool no_factory_set = False;
#endif
static Bool comments = True;
static const char *odl_tmpfile;

static Bool no_rootclass = False;

static Bool with_cpp = False;
static const char *default_cpp_cmd = "gcc";
static const char *cpp_cmd = NULL;
static const char *default_cpp_flags = "-E -P";
static std::string cpp_flags;

static int
usage(const char *msg = 0, const char *etc = 0)
{
  static const char sep[] = "";
  static const char us[] = "eyedbodl ";
  //static const char nl[] = "\n";
  //  static const char sp[] = "         ";
  static const char nl[] = "";
  static const char sp[] = " ";

  if (msg) {
    fprintf(stderr, "%seyedbodl: ", sep);
    fprintf(stderr, msg, etc);
    fprintf(stderr, "\n");
    return 1;
  }

  fprintf(stderr, "\nUsage:\n");
  fprintf(stderr, "%s--gencode=C++ [--package=PACKAGE]%s", us, nl);

  fprintf(stderr, "%s[--output-dir=DIRNAME] [--output-file-prefix=PREFIX]%s",
	  sp, nl);
  fprintf(stderr, "%s[--schema-name=SCHNAME] [--namespace=NAMESPACE] [--use-smart-pointers=yes|no]%s", sp, nl);
  fprintf(stderr, "%s[--class-prefix=PREFIX] [--db-class-prefix=DBPREFIX]%s", sp, nl);
  fprintf(stderr, "%s[--attr-style=implicit|explicit]%s", sp, nl);
  fprintf(stderr, "%s[--dynamic-attr]%s", sp, nl);
  fprintf(stderr, "%s[--gen-class-stubs] [--class-enums=yes|no]%s", sp, nl);
  fprintf(stderr, "%s[--c-suffix=SUFFIX] [--h-suffix=SUFFIX] [--export]%s",
	  sp, nl);

  fprintf(stderr, "%s[--down-casting=yes|no] [--gencode-error-policy=status|exception]%s",
	  sp, nl);
  fprintf(stderr, "%s[--attr-cache=yes|no] [--rootclass=ROOTCLASS] [--no-rootclass]%s", sp, nl);
  fprintf(stderr, "%s[--cpp] [--cpp-cmd=CPP] [--cpp-flags=FLAGS]%s%sODLFILE|-|-d DBNAME|--database=DBNAME [OPENFLAGS]\n", sp, nl, sp);

 
  fprintf(stderr, "\n%s--gencode=Java --package=PACKAGE%s", us, nl);

  fprintf(stderr, "%s[--output-dir=DIRNAME] [--output-file-prefix=PREFIX]%s",
	  sp, nl);
  fprintf(stderr, "%s[--schema-name=SCHNAME]%s", sp, nl);
  fprintf(stderr, "%s[--class-prefix=PREFIX] [--db-class-prefix=DBPREFIX]%s", sp, nl);
  fprintf(stderr, "%s[--attr-style=implicit|explicit]%s", sp, nl);
  fprintf(stderr, "%s[--dynamic-attr]%s", sp, nl);
  fprintf(stderr, "%s[--down-casting=yes|no] [--gencode-error-policy=status|exception]%s",
	  sp, nl);
  fprintf(stderr, "%s[--cpp] [--cpp-cmd=CPP] [--cpp-flags=FLAGS]%s%sODLFILE|-|-d DBNAME|--database=DBNAME [OPENFLAGS]\n", sp, nl, sp);

#ifdef SUPPORT_CORBA
  fprintf(stderr, "\n%s--gencode Orbix|Orbacus|IDL --package PACKAGE -idl-module MODULE\n", us);

  fprintf(stderr, "%s[--imdl IMDLFILE] [--idl-target IDLFILE] [--output-dir DIRNAME] [--no-generic-idl]\n",
	  sp);
  fprintf(stderr, "%s[--generic-idl IDLFILE] [--class-prefix PREFIX]\n", sp);
  fprintf(stderr, "%s[--factory yes|no] [--sync yes|no] [--comments yes|no]\n", sp);
  fprintf(stderr, "%s[--cpp CPP [--cpp-flags FLAGS [--no-cpp] ODLFILE|-|-d DBNAME|--database=DBNAME [OPENFLAGS]\n", sp);
#endif

  fprintf(stderr, "\n%s--gencode=ODL -d DBNAME|--database=DBNAME [--system-class] [-o ODLFILE] [OPENFLAGS]%s", us, nl);
  fprintf(stderr, "\n%s--diff -d DBNAME|--database=DBNAME [--system-class] [OPENFLAGS]%s", us, nl);
  fprintf(stderr, "%s[--cpp] [--cpp-cmd=CPP] [--cpp-flags=FLAGS] ODLFILE|-\n",
	  sp);

  fprintf(stderr, "\n%s-u|-update -d DBNAME|--database=DBNAME%s", us, nl);
  fprintf(stderr, "%s[--db-class-prefix=DBPREFIX] [OPENFLAGS]%s", sp, nl);
  fprintf(stderr, "%s[--schema-name=SCHNAME] [--rmv-undef-attrcomp=yes|no]%s%s[--update-index=yes|no]%s", sp, nl, sp, nl);
  fprintf(stderr, "%s[--cpp] [--cpp-cmd=CPP] [--cpp-flags=FLAGS]%s%s[--rmcls=CLASS...] [--rmsch] [ODLFILE|-]\n",
	  sp, nl, sp);
  fprintf(stderr, "\n%s--checkfile ODLFILE|-\n", us);

  fprintf(stderr, "\n%s--help\n", us);

  return 1;
}

static void
help()
{
  fprintf(stderr, "One must specify one and only one of the following major options:\n");
  fprintf(stderr, "--gencode=C++                 Generates C++ code\n");
  fprintf(stderr, "--gencode=Java                Generates Java code\n");

#ifdef SUPPORT_CORBA
  fprintf(stderr, "--gencode Orbix               Generates Orbix code (IDL and C++ stubs)\n");
  fprintf(stderr, "--gencode Orbacus             Generates Orbacus code (IDL and C++ stubs)\n");
  fprintf(stderr, "--gencode IDL                 Generates IDL\n");
#endif

  fprintf(stderr, "--gencode=ODL                 Generates ODL\n");
  fprintf(stderr, "--update|-u                   Updates schema in database DBNAME\n");
  fprintf(stderr, "--diff                        Displays the differences between a database schema and an odl file\n");
  fprintf(stderr, "--checkfile                   Check input ODL file\n");
  fprintf(stderr, "--help                        Displays the current information\n");

  fprintf(stderr, "\nThe following options must be added to the --gencode=C++ or Java option:\n");
  fprintf(stderr, "ODLFILE|-|-d DBNAME|--database=DBNAME Input ODL file (or - for standard input) or the database name\n");

  fprintf(stderr, "\nThe following options can be added to the --gencode=C++ or Java option:\n");
  fprintf(stderr, "--package=PACKAGE             Package name\n");
  fprintf(stderr, "--output-dir=DIRNAME          Output directory for generated files\n");
  fprintf(stderr, "--output-file-prefix=PREFIX   Ouput file prefix (default is the package name)\n");
  fprintf(stderr, "--class-prefix=PREFIX         Prefix to be put at the begining of each runtime class\n");
  fprintf(stderr, "--db-class-prefix=PREFIX      Prefix to be put at the begining of each database class\n");
  fprintf(stderr, "--attr-style=implicit         Attribute methods have the attribute name\n");
  fprintf(stderr, "--attr-style=explicit         Attribute methods have the attribute name prefixed by get/set (default)\n");
  fprintf(stderr, "--schema-name=SCHNAME         Schema name (default is PACKAGE)\n");
  fprintf(stderr, "--export                      Export class instances in the .h file\n");
  fprintf(stderr, "--dynamic-attr                Uses a dynamic fetch for attributes in the get and set methods\n");
  fprintf(stderr, "--down-casting=yes            Generates the down casting methods (the default)\n");
  fprintf(stderr, "--down-casting=no             Does not generate the down casting methods\n");
  fprintf(stderr, "--attr-cache=yes              Use a second level cache for attribute value\n");
  fprintf(stderr, "--attr-cache=no               Does not use a second level cache for attribute value (the default)\n");
  fprintf(stderr, "\nFor the --gencode=C++ option only\n");
  fprintf(stderr, "--namespace=NAMESPACE         Define classes with the namespace NAMESPACE\n");
  fprintf(stderr, "--use-smart-pointers=yes|no   The generated C++ code will use (=yes) smart pointers or not (=no)\n");
  fprintf(stderr, "--c-suffix=SUFFIX             Use SUFFIX as the C file suffix\n");
  fprintf(stderr, "--h-suffix=SUFFIX             Use SUFFIX as the H file suffix\n");
  fprintf(stderr, "--gen-class-stubs             Generates a file class_stubs.h for each class\n");
  fprintf(stderr, "--class-enums=yes             Generates enums within a class\n");
  fprintf(stderr, "--class-enums=no              Do not generate enums within a class (default)\n");
  fprintf(stderr, "--gencode-error-policy=status Status oriented error policy (the default)\n");
  fprintf(stderr, "--gencode-error-policy=exception Exception oriented error policy\n");
  fprintf(stderr, "--rootclass=ROOTCLASS         Use ROOTCLASS name for the root class instead of the package name\n");
  fprintf(stderr, "--no-rootclass                Does not use any root class\n");

  fprintf(stderr, "\nThe following options can be added to the --gencode=ODL option:\n");
  fprintf(stderr, "--system-class                Generates system class ODL\n");
#ifdef SUPPORT_CORBA
  fprintf(stderr, "\nThe following options must be added to the --gencode=Orbix, Orbacus or IDL option:\n");
  fprintf(stderr, "--package=PACKAGE             Package name\n");
  fprintf(stderr, "--idl-module MODULE           The IDL module name\n");
  fprintf(stderr, "\nOne of the following options must be added to the --gencode Orbix or Orbacus option:\n");
  fprintf(stderr, "ODLFILE|-|-d DBNAME|--database=DBNAME Input ODL file (- is the standard input) or\n");
  fprintf(stderr, "                                the database which contains the schema\n");

  fprintf(stderr, "\nThe following options can be added to the --gencode Orbix, Orbacus or IDL option:\n");
  fprintf(stderr, "--output-dir DIRNAME          Output directory for generated files\n");
  fprintf(stderr, "--imdl IMDLFILE|-             IMDL file (- is the standard input)\n");
  fprintf(stderr, "--idl-target IDLFILE          The target IDL file\n");
  fprintf(stderr, "--no-generic-idl              The EyeDB generic IDL file 'eyedb.idl' will not\n");
  fprintf(stderr, "                                be automatically included in the IDML file\n");
  fprintf(stderr, "--generic-idl IDLFILE         Give the location of the EyeDB generic IDL file\n");
  fprintf(stderr, "                                'eyedb.idl'\n");
  fprintf(stderr, "--factory yes                 The factory interface will be generated\n");
  fprintf(stderr, "                                (the default when -idl-target is not used)\n");
  fprintf(stderr, "--factory no                  The factory interface will not be generated\n");
  fprintf(stderr, "                                (the default when -idl-target is used)\n");
  fprintf(stderr, "--sync yes|no                 The backend runtime objects will be synchronized\n");
  fprintf(stderr, "                                (yes the default), or not synchronized (no) with the database\n");
  fprintf(stderr, "--comments yes|no             Does (yes) or does not (no) generate mapping\n");
  fprintf(stderr, "                                comments in the IDL\n");
#endif

  fprintf(stderr, "\nThe following option must be added to the --update|-u option:\n");
  fprintf(stderr, "-d DBNAME|--database=DBNAME   Database for which operation is performed\n");

  fprintf(stderr, "\nThe following options can be added to the --update|-u option:\n");
  fprintf(stderr, "ODLFILE|-                     Input ODL file or '-' (standard input)\n");
  fprintf(stderr, "--schema-name=SCHNAME         Schema name (default is package)\n");
  fprintf(stderr, "--db-class-prefix=PREFIX      Prefix to be put at the begining of each database class\n");
  fprintf(stderr, "--rmv-undef-attrcomp=yes|no   Removes (yes) or not (no) the undefined attribute components (constraint, index and implementation). Default is no\n");
  fprintf(stderr, "--update-index=yes|no         Updates (yes) or not (no) the index with a different implementation in the DB. Default is no\n");
  fprintf(stderr, "--rmcls=CLASS...              Removes the given class list\n");
  fprintf(stderr, "--rmsch                       Removes the entire schema\n");

  fprintf(stderr, "\nThe following options must be added to the --diff option:\n");
  fprintf(stderr, "-d DBNAME|--database=DBNAME Database for which the schema difference is performed\n");
  fprintf(stderr, "ODLFILE                       The input ODL file for which the schema difference is performed\n");
  fprintf(stderr, "\nThe following options can be added to the --diff option:\n");
  fprintf(stderr, "--system-class                Performs difference on system classes also\n");
  fprintf(stderr, "\nThe following option must be added to the --checkfile option:\n");
  fprintf(stderr, "ODLFILE|-                     Input ODL file or '-' (standard input)\n");

#ifdef SUPPORT_CORBA
  fprintf(stderr, "\nThe following options can be added when an ODLFILE, an IDLFILE\n");
  fprintf(stderr, "or an IMDLFILE is set:\n");
#else
  fprintf(stderr, "\nThe following options can be added when an ODLFILE is set:\n");
#endif
  fprintf(stderr, "--cpp                         Preprocess the ODL file with the C preprocessor\n");
  fprintf(stderr, "--cpp-cmd=CPP                 Uses CPP preprocessor instead of the default one (%s)\n", default_cpp_cmd);
  fprintf(stderr, "--cpp-flags=CPP_FLAGS         Adds CPP_FLAGS to the preprocessing command\n");
  //fprintf(stderr, "\nThe following options can be added when the -d DBNAME|--database=DBNAME is used:,\n");

  print_use_help(cerr);

  fflush(stderr);
}

#define MISSING "missing argument after "
#define OPTION  " option"

#define GETOPT(X, OPT) \
do { \
  if (i+1 >= argc) \
    return usage(MISSING #OPT OPTION); \
  X = argv[i+1]; \
  i += 2; \
} while(0)

static int
getOpts(int argc, char *argv[], Bool &dirname_set)
{
  prog = argv[0];
  dirname_set = False;

  for (int n = 1; n < argc; n++) {
    char *s = argv[n];

    string value;

    if (GetOpt::parseLongOpt(s, "output-dir", &value)) {
      hints->setDirName(value.c_str());
      dirname_set = True;
    } else if (GetOpt::parseLongOpt(s, "output-file-prefix", &value)) {
      hints->setFilePrefix(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "package", &value)) {
      package = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "gendate", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	hints->gen_date = True;
      else if (!strcasecmp(value.c_str(), "no"))
	hints->gen_date = False;
      else
	return usage("needs `yes' or `no' after --gendate option");
    } else if (GetOpt::parseLongOpt(s, "schema-name", &value)) {
      schname = strdup(value.c_str());
    } else if (!strcmp(s, "--export")) {
      _export = True;
    } else if (GetOpt::parseLongOpt(s, "comments", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	comments = True;
      else if (!strcasecmp(value.c_str(), "no"))
	comments = False;
      else
	return usage("needs `yes' or `no' after --comments option");
    } else if (!strcmp(s, "--admin")) {
      open_flags |= _DBAdmin;
    } else if (!strcmp(s, "--local")) {
      open_flags |= _DBOpenLocal;
    } else if (!strcmp(s, "--dynamic-attr")) {
      odl_dynamic_attr = True;
    } else if (GetOpt::parseLongOpt(s, "attr-style", &value)) {
      if (!strcmp(value.c_str(), "implicit")) {
	hints->attr_style = GenCodeHints::ImplicitAttrStyle;
	// backward compatibility
	hints->setStyle("implicit");
      } else if (!strcmp(value.c_str(), "explicit")) {
	hints->attr_style = GenCodeHints::ExplicitAttrStyle;
	// backward compatibility
	hints->setStyle("explicit");
      }
      else {
	Status s = hints->setStyle(value.c_str());
	if (s) {
	  s->print();
	  return 1;
	}
      }
    } else if (GetOpt::parseLongOpt(s, "class-prefix", &value)) {
      prefix = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "namespace", &value)) {
      c_namespace = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "use-smart-pointers", &value)) {
      const char *s = value.c_str();
      if (!strcasecmp(s, "yes"))
	odl_smartptr = True;
      else if (!strcasecmp(s, "no"))
	odl_smartptr = False;
      else
	return usage("must set yes or no behind --use-smart-pointers");
    } else if (GetOpt::parseLongOpt(s, "db-class-prefix", &value)) {
      db_prefix = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "rmv-undef-attrcomp", &value)) {
      cerr << "warning: --rmv-undef-attrcomp option is not yet implemented\n";
      return 1;
    } else if (GetOpt::parseLongOpt(s, "update-index", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	odl_update_index = True;
      else if (!strcasecmp(value.c_str(), "no"))
	odl_update_index = False;
      else
	return usage("yes or no is expected after --update-index option");
    } else if (GetOpt::parseLongOpt(s, "rmcls", &value)) {
      // must split classname with ','
      char *p = strdup(value.c_str());
      for (;;) {
	char *q = strchr(p, ',');
	if (q)
	  *q = 0;

	odl_cls_rm.insertObject(p);

	if (!q)
	  break;
	p = q+1;
      }
    } else if (!strcmp(s, "--rmsch")) {
      odl_sch_rm = True;
    } else if (!strcmp(s, "--gen-class-stubs")) {
      hints->gen_class_stubs = True;
    } else if (GetOpt::parseLongOpt(s, "class-enums", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	hints->class_enums = True;
      else if (!strcasecmp(value.c_str(), "no"))
	hints->class_enums = False;
      else
	return usage("yes or no is expected after --class-enums option");

    } else if (GetOpt::parseLongOpt(s, "c-suffix", &value)) {
      hints->setCSuffix(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "h-suffix", &value)) {
      hints->setHSuffix(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "down-casting", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	hints->gen_down_casting = True;
      else if (!strcasecmp(value.c_str(), "no"))
	hints->gen_down_casting = False;
      else
	return usage("yes or no is expected after --down-casting option");
    } else if (GetOpt::parseLongOpt(s, "attr-cache", &value)) {
      if (!strcasecmp(value.c_str(), "yes"))
	hints->attr_cache = True;
      else if (!strcasecmp(value.c_str(), "no"))
	hints->attr_cache = False;
      else
	return usage("yes or no is expected after --attr-cache option");
    } else if (GetOpt::parseLongOpt(s, "gencode-error-policy", &value)) {
      if (!strcasecmp(value.c_str(), "status"))
	hints->error_policy = GenCodeHints::StatusErrorPolicy;
      else if (!strcasecmp(value.c_str(), "exception"))
	hints->error_policy = GenCodeHints::ExceptionErrorPolicy;
      else
	return usage("status or exception is expected after --error-policy option");
    } else if (GetOpt::parseLongOpt(s, "gencode", &value)) {
      if (!strcasecmp(value.c_str(), "c++")) {
	cplus_gen = True;
	odl_lang = ProgLang_C;
      } else if (!strcasecmp(value.c_str(), "java")) {
	java_gen = True;
	odl_lang = ProgLang_Java;
      } else if (!strcasecmp(value.c_str(), "odl"))
	odl_gen = True;
      else
	return usage("C++, Java or ODL is expected after --gencode");
    } else if (GetOpt::parseLongOpt(s, "database", &value)) {
      if (!dbname)
	dbname = strdup(value.c_str());
      else
	return usage("cannot specified two databases");
    } else if (!strcmp(s, "-d")) {
      if (n+1 >= argc)
	return usage("missing argument after -d");
      if (!dbname) {
	dbname = strdup(argv[n+1]);
	n++;
      } else
	return usage("cannot specified two databases");
    } else if (GetOpt::parseLongOpt(s, "cpp_cmd", &value)) {
      cpp_cmd = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "cpp", NULL)) {
      with_cpp = True;
    } else if (GetOpt::parseLongOpt(s, "cpp-flags", &value)) {
      cpp_flags = strdup(value.c_str());
    } else if (GetOpt::parseLongOpt(s, "no-rootclass")) {
      no_rootclass = True;
    } else if (GetOpt::parseLongOpt(s, "rootclass", &value)) {
      odl_rootclass = strdup(value.c_str());
    } else if (!strcmp(s, "-o")) {
      if (n+1 >= argc)
	return usage("missing argument after -o");
      ofile = strdup(argv[n+1]);
      n++;
    } else if (!strcmp(s, "--update") || !strcmp(s, "-u")) {
      update = True;
    } else if (!strcmp(s, "--diff")) {
      diff = True;
      no_rootclass = True;
    } else if (!strcmp(s, "--checkfile")) {
      checkfile = True;
    } else if (!strcmp(s, "--help") || !strcmp(s, "-h")) {
      usage();
      fprintf(stderr, "\n");
      help();
      exit(0);
    } else if (!strcmp(s, "--system-class")) { // internal use only!
      odl_system_class = True;
    } else if (!strcmp(s, "-") || (!odlfile && s[0] != '-')) {
      if (!odlfile)
	odlfile = argv[n];
    } else
      return usage();
  }


#ifdef SUPPORT_CORBA
  if (orbix_gen || orbacus_gen || idl_gen)
    no_rootclass = True;
#endif

  if (no_rootclass)
    odl_rootclass = 0;
  else if (!odl_rootclass)
    odl_rootclass = "Root";

#ifdef SUPPORT_CORBA
  if (idltarget && !no_factory_set)
    no_factory = True;

  if (orbix_gen)
    cpp_flags += " -DEYEDBORBIX";
  else if (orbacus_gen)
    cpp_flags += " -DEYEDBORBACUS";
#endif

  return 0;
}

static const char *
get_gen_opt()
{
  if (odl_lang == ProgLang_Java)
    return "Java";
  if (odl_lang == ProgLang_C)
    return "C++";
#ifdef SUPPORT_CORBA
  if (idl_gen)
    return "IDL";
  if (orbix_gen)
    return "Orbix";
  if (orbacus_gen)
    return "Orbacus";
#endif
  return "";
}

static int
check_package()
{
  const char *p = package;
  char c = *p;

  if (!c)
    return 0;

  if (!((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c == '_') || (java_gen && c == '.')))
    return 1;

  while (c=*++p)
    if (!((c >= 'a' && c <= 'z') ||
	  (c >= 'A' && c <= 'Z') ||
	  (c >= '0' && c <= '9') ||
	  (c == '_') || (java_gen && c == '.')))
      return 1;

  return 0;
}

static void
make_package(char *&package, const char *odlfile)
{
  if (package || !odlfile)
    return;

  char *s = strdup(odlfile);
  char *x = strrchr(s, '.');
  if (x) *x = 0;
  x = strrchr(s, '/');

  if (x)
    package = strdup(x+1);
  else
    package = strdup(s);
  free(s);
}

static int
checkOpts(Bool dirname_set)
{
  int nopts = 0;

  if (cplus_gen) nopts++;
  if (java_gen)  nopts++;
#ifdef SUPPORT_CORBA
  if (orbix_gen) nopts++;
  if (orbacus_gen) nopts++;
  if (idl_gen)   nopts++;
#endif
  if (odl_gen)   nopts++;
  if (update)    nopts++;
  if (diff)      nopts++;
  if (checkfile) nopts++;

  // one and only one major option
  if (nopts == 0 || nopts > 1)
    {
      usage("one and only one major option must be specified:\n"
	    "\t    --gencode=C++\n"
	    "\t    --gencode=Java\n"
#ifdef SUPPORT_CORBA
	    "\t    --gencode=Orbix\n"
	    "\t    --gencode=Orbacus\n"
	    "\t    --gencode=IDL\n"
#endif
	    "\t    --gencode=ODL\n"
	    "\t    --update|-u\n"
	    "\t    --diff\n"
	    "\t    --checkfile\n"
	    "\t    --help");
      fprintf(stderr, "\n");
      return usage();
    }

  const char *opackage = package;
  make_package(package, odlfile);

  if (update && !dbname)
    return usage("Must specified -d DBNAME|--database=DBNAME option when -u is used");

#ifdef SUPPORT_CORBA
  if ((orbix_gen || orbacus_gen || idl_gen) && !package)
    return usage("--package is a mandatory option when -gencode Orbix|Orbacus|IDL is used");

  if ((orbix_gen || orbacus_gen || idl_gen) && !module)
    return usage("--idl-module is a mandatory option when -gencode Orbix|Orbacus|IDL is used");
#endif

  if (odl_lang && !package)
    return usage("--package is a mandatory option when -gencode Java|C++ or -u is used");

  if ((odl_lang || odl_gen) && dbname && odlfile)
    return usage("cannot specify both a database and an ODL file");

  if (!package)
    package = (char*)"";/*@@@@warning cast*/

  if (check_package()) {
    std::string s = std::string("invalid package name '") + package + "'.\n"
      "A package name must be under the form [a-zA_Z_][a-zA-Z0-9_]*";
    if (!opackage) s += "\nUse the -package option";
    return usage(s.c_str());
  }

#ifdef SUPPORT_CORBA
  if (ofile && !odl_gen && !idl_gen)
    return usage("-o option may be used only with -gencode ODL option");
#endif

  if (java_gen && !dirname_set)
    hints->setDirName(package);

#ifdef SUPPORT_CORBA
  if (orbix_gen || orbacus_gen || idl_gen || odl_lang)
    {
      if (!odlfile && !dbname)
	return usage("using --gencode=%s : must specify an ODLFILE or a database (-d DBNAME|--database=DBNAME)", get_gen_opt());

      /*
	if (odlfile && dbname)
	return usage("using -gencode %s : must specify an ODLFILE or a database (-d DBNAME)", get_gen_opt());
      */
    }
  //else
#endif

  if (odl_cls_rm.getCount() > 0 && odl_sch_rm)
    return usage("cannot use both --rmcls and --rmsch options");
  else if (update && (odl_sch_rm || odl_cls_rm.getCount() > 0)) {
    // nop
  }
  else if (!cplus_gen && !java_gen && !odl_gen && !odlfile)
    return usage("an ODLFILE must be specified; use '-' for standard input");

  if (diff) {
    if (!odlfile || !dbname)
      return usage();
  }

  if (update && *prefix)
    return usage();
    
  return 0;
}

static int
checkDb()
{
  if (!schname) {
    schname = package;
    if (!schname || !*schname)
      schname = (char*)"UNTITLED";/*@@@@warning cast*/
  }

  if (dbname) {
    Connection *conn = new Connection();
    Status s;

    if (s = conn->open()) {
      fprintf(stderr, "eyedbodl: ");
      s->print(stderr);
      return 1;
    }
      
    db = new Database(dbname);
    
    if (s = db->open(conn, (Database::OpenFlag)
		     ((update ? Database::DBRWAdmin :
		       Database::DBSRead) | open_flags))) {
      fprintf(stderr, "eyedbodl: ");
      s->print(stderr);
      return 1;
    }
  }

  return 0;
}

int
realize(int argc, char *argv[])
{
  TransactionParams::setGlobalDefaultMagOrder(100000);

  eyedb::init(argc, argv);

  const char *x;

  x = eyedb::ClientConfig::getCValue("cpp_cmd");
  if (x)
    default_cpp_cmd = x;

  x = eyedb::ClientConfig::getCValue("cpp_flags");
  if (x)
    default_cpp_flags = x;

  hints = new GenCodeHints();

  hints->setDirName(".");

  Bool dirname_set;

  if (getOpts(argc, argv, dirname_set))
    return 1;

  if (checkOpts(dirname_set))
    return 1;

  if (!hints->fileprefix)
    hints->setFilePrefix(package);

  if (checkDb()) // on doit faire seulement une partie de ca
    return 1;

  if (with_cpp) {
    if (cpp_cmd == NULL)
      cpp_cmd = default_cpp_cmd;
    if (cpp_flags.empty())
      cpp_flags = default_cpp_flags;
  }

  Status s = Success;

  Exception::setMode(Exception::ExceptionMode);

  try {
    if (update)
      s = db->updateSchema(odlfile, package, schname, db_prefix,
			   stdout, cpp_cmd, cpp_flags.c_str());

    else if (diff)
      s = Schema::displaySchemaDiff(db, odlfile, package, db_prefix, stdout,
				    cpp_cmd, cpp_flags.c_str());

#ifdef SUPPORT_CORBA
    else if (idl_gen)
      s = Schema::genIDL(db, odlfile, package, module, schname, prefix,
			 db_prefix, idltarget, imdlfile, no_generic_idl,
			 generic_idl, no_factory, imdl_synchro,
			 hints, comments, cpp_cmd, cpp_flags.c_str(), ofile);
#endif
    else if (odl_gen)
      s = Schema::genODL(db, odlfile, package, schname,
			 prefix, db_prefix,  ofile, cpp_cmd,
			 cpp_flags.c_str());

#ifdef SUPPORT_CORBA
    else if (orbix_gen || orbacus_gen)
      s = Schema::genORB((orbix_gen ? "orbix" : "orbacus"),
			 db, odlfile, package, module, schname, prefix,
			 db_prefix, idltarget, imdlfile, no_generic_idl,
			 generic_idl, no_factory, imdl_synchro,
			 hints, comments, cpp_cmd, cpp_flags.c_str());

#endif
    else if (odl_lang == ProgLang_C)
      s = Schema::genC_API(db, odlfile, package, schname,
			   c_namespace, prefix,
			   db_prefix, _export, hints, cpp_cmd,
			   cpp_flags.c_str());

    else if (odl_lang == ProgLang_Java)
      s = Schema::genJava_API(db, odlfile, package, schname, prefix,
			      db_prefix, _export, hints, cpp_cmd,
			      cpp_flags.c_str());

    else if (checkfile)
      s = Schema::checkODL(odlfile, package, cpp_cmd, cpp_flags.c_str());

  } catch(Exception &e) {
    cerr << "\neyedbodl: ";
    cerr << e << endl;
    return 1;
  }

  if (s) {
    fprintf(stderr, "\neyedbodl: ");
    s->print(stderr);
    return 1;
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  int r = realize(argc, argv);
  if (odl_tmpfile) unlink(odl_tmpfile);
  return r;
}

