# 
# configure.in for eyedb
# 

AC_INIT(eyedb, 2.9.2, eyedb-dev@lists.sourceforge.net, eyedb)

#. $srcdir/VERSION
#EYEDB_VERSION=${EYEDB_MAJOR_VERSION}.${EYEDB_MINOR_VERSION}.${EYEDB_PATCH_LEVEL}${EYEDB_VERSION_STATUS}
#PACKAGE_VERSION=$EYEDB_VERSION
#EYEDB_NUMVERSION=`echo | awk '{printf("%d%02d%03d", '${EYEDB_MAJOR_VERSION}','${EYEDB_MINOR_VERSION}','${EYEDB_PATCH_LEVEL}')}' `

# FD CHECK hardcoded for now
EYEDB_VERSION=2.9.2
EYEDB_MAJOR_VERSION=2
EYEDB_MINOR_VERSION=9
EYEDB_PATCH_LEVEL=2
EYEDB_VERSION_STATUS=
EYEDB_NUMVERSION=209002

AC_SUBST(EYEDB_VERSION)
AC_SUBST(EYEDB_MAJOR_VERSION)
AC_SUBST(EYEDB_MINOR_VERSION)
AC_SUBST(EYEDB_PATCH_LEVEL)
AC_SUBST(EYEDB_VERSION_STATUS)
AC_SUBST(EYEDB_NUMVERSION)

AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER(eyedbconfig.h)
AC_CONFIG_MACRO_DIRS([m4/java m4/swig m4/python m4/extras])
AC_CONFIG_SRCDIR([include/eyedb/eyedb.h])

AM_INIT_AUTOMAKE(foreign)


#
# Checks for command line flags
#

# if CXXFLAGS are not already set, set them to an empty value
if test x"$CXXFLAGS" = "x"
then
	CXXFLAGS=''
fi

#
# Collect arguments
#
# debug
AC_ARG_ENABLE(debug,
		AS_HELP_STRING([--enable-debug],
				[compile with debug [default=no]]),,
		enable_debug="no")
# optimize
AC_ARG_ENABLE(optimize,
		AS_HELP_STRING([--enable-optimize=flag],
				[compile with given optimization flag [default=no]]),,
		enable_optimize="")
# profile
AC_ARG_ENABLE(profile,
		AS_HELP_STRING([--enable-profile],
				[compile with profile [default=no]]),,
		enable_profile="no")
# Java
AC_ARG_ENABLE(java,
		AS_HELP_STRING([--enable-java],
				[enable Java code compilation [default=yes]]),,
		enable_java="yes")
# Documentation
AC_ARG_ENABLE(doc,
		AS_HELP_STRING([--enable-doc],
				[enable documentation generation [default=yes]]),,
		enable_doc="yes")
# Swig
AC_ARG_ENABLE(swig,
		AS_HELP_STRING([--enable-swig],
				[enable SWIG bindings generation [default=no]]),,
		enable_swig="no")
# Location of eyedbsm installation
AC_ARG_WITH(eyedbsm-prefix, 
	     AS_HELP_STRING([--with-eyedbsm-prefix=PREFIX],[give Eyedb-SM installation prefix (if none given, guessed by configure]),
	     ac_eyedbsm_prefix="$withval",
	     ac_eyedbsm_prefix="guess")
# Semaphore policy
AC_ARG_WITH( semaphore-policy, 
	     AS_HELP_STRING([--with-semaphore-policy=(posix|sysv_ipc)],[force semaphore implementation (if none given, is found by configure.in)]),
	     ac_semaphore_policy="$withval",
	     ac_semaphore_policy="guess")
# Installation directory: data bases directory
AC_ARG_WITH( databasedir, 
	     AS_HELP_STRING([--with-databasedir=DIRECTORY],[give databases directory (default is LOCALSTATEDIR/lib/eyedb/db)]),
	     ac_eyedb_databasedir="$withval",
	     ac_eyedb_databasedir='${localstatedir}/lib/eyedb/db')
# Installation directory: pipes directory
AC_ARG_WITH( pipedir, 
	     AS_HELP_STRING([--with-pipedir=DIRECTORY],[give pipes directory (default is LOCALSTATEDIR/lib/eyedb/pipes)]),
	     ac_eyedb_pipedir="$withval",
	     ac_eyedb_pipedir='${localstatedir}/lib/eyedb/pipes')
# Installation directory: temporary directory
AC_ARG_WITH( tmpdir, 
	     AS_HELP_STRING([--with-tmpdir=DIRECTORY],[give temporary directory (default is LOCALSTATEDIR/lib/eyedb/tmp)]),
	     ac_eyedb_tmpdir="$withval",
	     ac_eyedb_tmpdir='${localstatedir}/lib/eyedb/tmp')
# User under which EyeDB server should run
AC_ARG_WITH(eyedbd-user,
	AS_HELP_STRING([--with-eyedbd-user=USERNAME],[give user the EyeDB server shall be run as (default is eyedb)]),
	EYEDBD_USER="$withval",
	EYEDBD_USER=eyedb)
AC_SUBST(EYEDBD_USER)
# readline
AC_ARG_WITH(readline, 
	     AS_HELP_STRING([--with-readline=(readline|editline)],[use GNU readline or BSD editline for line editing (default is readline if available)]),
	     ac_eyedb_readline="$withval",
	     ac_eyedb_readline='none')
#
# End of arguments collect


# by default we install in /usr/local/eyedb
AC_PREFIX_DEFAULT(/usr/local/eyedb)

# search eyedb-sm installation prefix
AC_MSG_CHECKING(for Eyedb-SM pkg-config)
case "$ac_eyedbsm_prefix" in
guess)
	# if a prefix is given to this configure.in, look into it, otherwise do nothing
	if test -f "$prefix/lib/pkgconfig/eyedbsm.pc" ; then
		PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
		export PKG_CONFIG_PATH
		AC_MSG_RESULT($PKG_CONFIG_PATH)
	else
		AC_MSG_RESULT(none given, will use standard loccation)
	fi
	;;
*)
	# set the PKG_CONFIG_PATH environment variable
	PKG_CONFIG_PATH="$ac_eyedbsm_prefix/lib/pkgconfig"
	export PKG_CONFIG_PATH
	AC_MSG_RESULT($PKG_CONFIG_PATH)
	;;
esac


# check for GNU make
CHECK_GNU_MAKE
if test x"$_cv_gnu_make_command" = x ; then
    AC_MSG_ERROR(you must use GNU make to compile EyeDB)
fi

# check for compilers
AC_PROG_CXX

AC_LANG(C++)

# debug, optimize, profile flags
if test "$enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -g"
fi
if test "x$enable_optimize" != "x"; then
  CXXFLAGS="$CXXFLAGS $enable_optimize"
fi
if test "$enable_profile" = "yes"; then
   if test x"$ac_cv_cxx_compiler_gnu" = x"yes"; then
      CXXFLAGS="$CXXFLAGS -pg"
      LDFLAGS="$LDFLAGS -pg"
   fi
fi

# check for architecture
AC_CANONICAL_HOST
AC_SUBST(host)
AC_SUBST(host_cpu)
AC_SUBST(host_os)

# special settings on Solaris 64 bits
case "$host" in
sparc-sun-solaris2.9)
	if test "$ac_cv_cxx_compiler_gnu" != "yes"
	then
		CFLAGS="-xtarget=ultra -xarch=v9"
		CXXFLAGS="$CXXFLAGS -xtarget=ultra -xarch=v9"
		LDFLAGS="-XCClinker -xtarget=ultra -XCClinker -xarch=v9"
	fi
	;;
esac
# special settings on Mac OS X
case "$host" in
powerpc-apple-darwin*)
	LDFLAGS="-Wl,-m -Wl,-multiply_defined -Wl,suppress"
	;;
esac

# check for endianism
AC_C_BIGENDIAN
if test $ac_cv_c_bigendian = yes; then
EYEDB_CONFIG_BYTE_ORDER=EYEDBLIB_BIG_ENDIAN
else
EYEDB_CONFIG_BYTE_ORDER=EYEDBLIB_LITTLE_ENDIAN
fi
AC_SUBST(EYEDB_CONFIG_BYTE_ORDER)

# check for unsigned long long type
AC_CHECK_TYPES( unsigned long long)
if test x"$ac_cv_type_unsigned_long_long" != xyes ; then
   AC_MSG_ERROR( "this platform does not support type unsigned long long")
fi

# check for type sizes
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned long long)
if test "$ac_cv_sizeof_unsigned_long_long" -lt 8 ; then
   AC_MSG_ERROR( "this platform has an unsigned long long type with sizeof(unsigned long long) < 8")
fi

AC_MSG_CHECKING(for integer type of same size as void *)
case $ac_cv_sizeof_void_p in
$ac_cv_sizeof_unsigned_int)
		ac_pointer_int_type='unsigned int'
	;;
$ac_cv_sizeof_unsigned_long)
		ac_pointer_int_type='unsigned long'
	;;
$ac_cv_sizeof_unsigned_long_long)
		ac_pointer_int_type='unsigned long long'
	;;
*)
		AC_MSG_ERROR('cannot find an integer type of same size as void *')
	;;
esac
AC_MSG_RESULT($ac_pointer_int_type)
AC_DEFINE_UNQUOTED([POINTER_INT_TYPE], $ac_pointer_int_type, [Define to the integer type of same size as void *])

# check for libtool
AC_PROG_LIBTOOL

# Check for lex/flex
AM_PROG_LEX
AC_PATH_PROG(_XLEX,$LEX)
if test x"$_XLEX" = x ; then
	AC_MSG_ERROR(you must have GNU flex or lex installed to compile EyeDB)
fi
# Check for yacc/bison
AC_PROG_YACC
AC_PATH_PROG(_XYACC,$YACC)
if test x"$_XYACC" = x ; then
	AC_MSG_ERROR(you must have GNU bison or yacc installed to compile EyeDB)
fi

# check for eyedbsm
PKG_CHECK_MODULES(EYEDBSM, eyedbsm,, [], [AC_MSG_ERROR([cannot find eyedbsm])])
CXXFLAGS="$CXXFLAGS $EYEDBSM_CFLAGS"
CFLAGS="$CFLAGS $EYEDBSM_CFLAGS"

# check for libraries
# this library is necessary on Solaris
AC_CHECK_LIB( nsl, gethostbyname)
AC_CHECK_LIB( dl, dlopen)
AC_CHECK_LIB( crypt, crypt)
AC_SEARCH_LIBS( socket, socket)

# check for pthread
ACX_PTHREAD
LIBS="$LIBS $PTHREAD_LIBS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
AC_CHECK_FUNC(pthread_mutexattr_setpshared,AC_DEFINE([HAVE_PTHREAD_MUTEXATTR_SETPSHARED],1,[Defined if function pthread_mutexattr_setpshared exists]),,[#include <pthread.h>])

# echo CFLAGS=$CFLAGS
# echo CXXFLAGS=$CXXFLAGS

# check for headers
AC_CHECK_HEADERS( time.h sys/time.h unistd.h sys/types.h regex.h libgen.h sys/stat.h pwd.h stropts.h crypt.h)
AC_HEADER_TIME

# check for flock structure
AC_CHECK_TYPES(flock_t,,,[#include <fcntl.h>])

# check for union semun
AC_CHECK_MEMBERS(union semun.val,,,[#include <sys/sem.h>])

# check for struct in_addr
AC_CHECK_MEMBERS(struct in_addr.s_addr,,,[#include <netinet/in.h>])
AC_CHECK_MEMBERS(struct in_addr._S_un,,,[#include <netinet/in.h>])

# check for functions
AC_CHECK_FUNCS(strsignal)
AC_CHECK_FUNCS(fattach)
# check if function ctime_r exists and has 2 or 3 arguments
AC_CHECK_FUNCS(ctime_r)
AC_FUNC_CTIME_R_3_ARGS
# check for regular expressions functions
AC_CHECK_FUNC(regcomp,AC_DEFINE([HAVE_REGCOMP],1,[Defined if function regcomp exists]),,[#include <regex.h>])
AC_CHECK_FUNC(regexec,AC_DEFINE([HAVE_REGEXEC],1,[Defined if function regexec exists]),,[#include <regex.h>])
AC_CHECK_FUNC(regcmp,AC_DEFINE([HAVE_REGCMP],1,[Defined if function regcmp exists]),,[#include <libgen.h>])
AC_CHECK_FUNC(regex,AC_DEFINE([HAVE_REGEX],1,[Defined if function regex exists]),,[#include <libgen.h>])

# check for tzset and long timezone
AC_CHECK_FUNC(tzset,AC_DEFINE([HAVE_TZSET],1,[Defined if function tzset exists]),,[#include <time.h>])
AC_VAR_LONG_TIMEZONE

# check for X11
AC_PATH_XTRA

# check for large file
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

# check for 'more' executable
AC_PATH_PROG(PATH_TO_MORE,more)
AC_DEFINE_UNQUOTED([PATH_TO_MORE], "$PATH_TO_MORE", [Define to the absolute path of the more command])

# check for /proc
AC_MSG_CHECKING(for /proc)
if test -d "/proc/1" ; then
   AC_DEFINE([HAVE_SLASH_PROC],1,[Defined if pseudo-filesystem /proc exists])
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

# check for readline
case "$ac_eyedb_readline" in
readline)
	ac_eyedb_readline_tries=readline
;;
editline)
	ac_eyedb_readline_tries=editline
;;
none)
	ac_eyedb_readline_tries="readline editline"
;;
esac
AC_CHECK_READLINE($ac_eyedb_readline_tries)

# determine the semaphore implementation for this platform
AC_PTHREAD_PROCESS_SHARED

AC_MSG_CHECKING(for semaphore policy)
case "$ac_semaphore_policy" in
posix)
	if test "$ac_pthread_process_shared" = "no"; then
	   AC_MSG_ERROR( this platform does not support PTHREAD_PROCESS_SHARED and cannot use Posix semaphore policy)
	fi
	AC_MSG_RESULT(Posix)
	AC_DEFINE(HAVE_SEMAPHORE_POLICY_POSIX, 1, [Define if semaphore policy is Posix (must support PTHREAD_PROCESS_SHARED)])
	;;
sysv_ipc)
	AC_MSG_RESULT(Sys V IPC)
	AC_DEFINE(HAVE_SEMAPHORE_POLICY_SYSV_IPC, 1, [Define if semaphore policy is Sys V IPC])
	;;
guess)
	if test "$ac_pthread_process_shared" = "yes"; then
	   AC_MSG_RESULT(Posix)
	   ac_semaphore_policy="posix"
	   AC_DEFINE(HAVE_SEMAPHORE_POLICY_POSIX, 1, [Define if semaphore policy is Posix (must support PTHREAD_PROCESS_SHARED)])
	else
	   ac_semaphore_policy="sysv_ipc"
	   AC_MSG_RESULT(Sys V IPC)
	   AC_DEFINE(HAVE_SEMAPHORE_POLICY_SYSV_IPC, 1, [Define if semaphore policy is Sys V IPC])
	fi
	;;
*)
	AC_MSG_ERROR( "semaphore policy must be one of posix or sysv_ipc")
	;;
esac

# determine if we must use eyedbsmd, depending on the semaphore policy
if test "$ac_semaphore_policy" = "sysv_ipc" ; then
   AC_DEFINE(HAVE_EYEDBSMD, 1, [Define to 1 if we must use eyedbsmd])
fi
AM_CONDITIONAL(COND_EYEDBSMD, test "$ac_semaphore_policy" = "sysv_ipc")

# installation directories
eyedb_databasedir="$ac_eyedb_databasedir"
AC_SUBST(eyedb_databasedir)
eyedb_pipedir="$ac_eyedb_pipedir"
AC_SUBST(eyedb_pipedir)
eyedb_tmpdir="$ac_eyedb_tmpdir"
AC_SUBST(eyedb_tmpdir)

# must disappear
case "$host" in
sparc-sun-solaris2.9)
	ARCH_DEFINES="-DSOLARIS -DSPARCV9"
esac

# -D are taken verbatim for now. Later, may use autodetection and/or command line flags
DEFINES=""
CXXFLAGS="$CXXFLAGS $DEFINES $ARCH_DEFINES"

# ========================================
# JAVA compilation
# ========================================
if test "$enable_java" = "yes"
then
	# On Mac OS X, if JAVAC is not set, we force it to be javac, because
	# AC_PROG_JAVAC tests first jikes, which is currently broken on Mac OS X
	case "$host" in
	powerpc-apple-darwin*)
		if test x"$JAVAC" = x ; then export JAVAC=javac ; fi
		;;
	esac

	AC_PROG_JAVAC
	export CLASSPATH=.
	AC_PROG_JAVA
	AC_PATH_PROG(JAR, jar)

	if test "$enable_debug" = "yes"
	then
		JAVACFLAGS="$JAVACFLAGS -g"
	fi
fi

if test "$enable_java" = "yes" -a x"$JAVAC" != x -a x"$JAR" != x  ; then
   COND_JAVA="yes"
fi
AM_CONDITIONAL(COND_JAVA, test x"$COND_JAVA" = xyes)
AC_SUBST(JAVACFLAGS)

# ========================================
# Python/pexpect tests
# ========================================
AM_PATH_PYTHON(,ac_python_found="yes",ac_python_found="no")
if test "$ac_python_found" = "yes" ; then
   AC_PYTHON_MODULE(pexpect,ac_python_pexpect="yes",ac_python_pexpect="no")
fi
if test "$ac_python_found" = "yes" -a "$ac_python_pexpect" = "yes" ; then
   COND_PYTHON_PEXPECT="yes"
fi
AM_CONDITIONAL(COND_PYTHON_PEXPECT, test x"$COND_PYTHON_PEXPECT" = xyes)

# ========================================
# SWIG
# ========================================
if test "$enable_swig" = "yes" ; then
	AC_PROG_SWIG
	SWIG_ENABLE_CXX
	SWIG_PYTHON
fi

if test "$enable_swig" = "yes" ; then
   COND_SWIG="yes"
fi
AM_CONDITIONAL(COND_SWIG, test x"$COND_SWIG" = xyes)

# ========================================
# Documentation building
# ========================================
if test "$enable_doc" = "yes" ; then
   COND_DOC="yes"
fi
AM_CONDITIONAL(COND_DOC, test x"$COND_DOC" = xyes)

# check for needed programs

# Doxygen for C++ API documentation
AC_PATH_PROG(DOXYGEN, doxygen)
if test x"$DOXYGEN" != x ; then
   AC_MSG_CHECKING(for doxygen version)
   DOXYGEN_VERSION=`$DOXYGEN --version`
   AC_MSG_RESULT($DOXYGEN_VERSION)
   DOXYGEN_MAJOR_VERSION=[`echo "$DOXYGEN_VERSION" | sed -e 's/.[1-9]*.[1-9]*$//'`]
   DOXYGEN_MINOR_VERSION=[`echo "$DOXYGEN_VERSION" | sed -e 's/^[1-9]*.//' -e 's/.[1-9]*$//'`]
   DOXYGEN_REQUIRED_MAJOR_VERSION=1
   DOXYGEN_REQUIRED_MINOR_VERSION=4
   if test "$DOXYGEN_MAJOR_VERSION" -le "$DOXYGEN_REQUIRED_MAJOR_VERSION" -a "$DOXYGEN_MINOR_VERSION" -lt "$DOXYGEN_REQUIRED_MINOR_VERSION" ; then
      AC_MSG_WARN(doxygen version should be at least $DOXYGEN_REQUIRED_MAJOR_VERSION.$DOXYGEN_REQUIRED_MINOR_VERSION.x)
   else
      COND_DOXYGEN="yes"
   fi
fi
AM_CONDITIONAL(COND_DOXYGEN, test x"$COND_DOXYGEN" = xyes)

# Java API documentation
AC_PATH_PROG(JAVADOC, javadoc)
if test x"$JAVADOC" != x ; then
COND_JAVADOC="yes"
fi
AM_CONDITIONAL(COND_JAVADOC, test x"$COND_JAVADOC" = xyes)

# Docbook documentation
# Look for xsltproc
AC_PATH_PROG(XSLTPROC, xsltproc)

# Docbook manpages
# Look for XSL stylesheet
AC_MSG_CHECKING(for Docbook manpages XSL stylesheet)
MANPAGES_XSL_STYLESHEET_LOCATIONS="/usr/share/sgml/docbook/xsl-stylesheets/manpages /usr/share/xml/docbook/stylesheet/nwalsh/manpages"
for d in $MANPAGES_XSL_STYLESHEET_LOCATIONS ; do
    if test -f $d/docbook.xsl ; then
       MANPAGES_XSL_STYLESHEET_DIR=$d
       break
    fi
done
AC_MSG_RESULT($MANPAGES_XSL_STYLESHEET_DIR/docbook.xsl)
AC_SUBST(MANPAGES_XSL_STYLESHEET_DIR)

if test x"$XSLTPROC" != x -a x"$MANPAGES_XSL_STYLESHEET_DIR" != x ; then
COND_DOCBOOK_MANPAGES="yes"
fi
AM_CONDITIONAL(COND_DOCBOOK_MANPAGES, test x"$COND_DOCBOOK_MANPAGES" = xyes)

# Docbook manuals: HTML output
AC_MSG_CHECKING(for Docbook HTML XSL stylesheet)
HTML_XSL_STYLESHEET_LOCATIONS="/usr/share/sgml/docbook/xsl-stylesheets/html /usr/share/xml/docbook/stylesheet/nwalsh/html"
for d in $HTML_XSL_STYLESHEET_LOCATIONS ; do
    if test -f $d/docbook.xsl ; then
       HTML_XSL_STYLESHEET_DIR=$d
       break
    fi
done
AC_MSG_RESULT($HTML_XSL_STYLESHEET_DIR/docbook.xsl)
AC_SUBST(HTML_XSL_STYLESHEET_DIR)

if test x"$XSLTPROC" != x -a x"$HTML_XSL_STYLESHEET_DIR" != x ; then
COND_DOCBOOK_HTML="yes"
fi
AM_CONDITIONAL(COND_DOCBOOK_HTML, test x"$COND_DOCBOOK_HTML" = xyes)

# Docbook manuals: PDF output
AC_PATH_PROG(DBLATEX, dblatex)
if test x"$DBLATEX" != x ; then
   COND_DBLATEX="yes"
fi
AM_CONDITIONAL(COND_DBLATEX, test x"$COND_DBLATEX" = xyes)

# Latex manuals
AC_PATH_PROG(LATEX, latex)
AC_PATH_PROG(DVIPS, dvips)
AC_PATH_PROG(PS2PDF, ps2pdf)
if test x"$LATEX" != x -a x"$DVIPS" != x -a x"$PS2PDF" != x ; then
   COND_LATEX="yes"
fi
AM_CONDITIONAL(COND_LATEX, test x"$COND_LATEX" = xyes)

AC_PATH_PROG(LATEX2HTML, latex2html)
if test x"$LATEX2HTML" != x ; then
   COND_LATEX2HTML="yes"
fi
AM_CONDITIONAL(COND_LATEX2HTML, test x"$COND_LATEX2HTML" = xyes)

# ========================================
# Examples building
#  is not architecture independant because of CXXFLAGS setting by hand
#  must be replaced in order to have a configure.in that does not do 'case $host'
# ========================================
case "$host" in
i*86-*-linux*)
	EYEDB_ARCH_CPPFLAGS="-D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
	EYEDB_ARCH_CXXFLAGS="-fPIC $PTHREAD_CFLAGS"
	EYEDB_ARCH_SOFLAGS=-shared
	;;

x86_64-*-linux*)
	EYEDB_ARCH_CPPFLAGS="-D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
	EYEDB_ARCH_CXXFLAGS="-fPIC $PTHREAD_CFLAGS"
	EYEDB_ARCH_SOFLAGS=-shared
	;;

sparc-sun-solaris2.9)
	if test "$ac_cv_cxx_compiler_gnu" = "yes"
	then
		EYEDB_ARCH_CPPFLAGS="-D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
		EYEDB_ARCH_CXXFLAGS="-fPIC $PTHREAD_CFLAGS"
		EYEDB_ARCH_SOFLAGS=-shared
	else
		EYEDB_ARCH_CPPFLAGS="-mt -xtarget=ultra -xarch=v9"
		EYEDB_ARCH_CXXFLAGS="-KPIC $PTHREAD_CFLAGS"
		EYEDB_ARCH_SOFLAGS=-G
	fi
	;;

*)
	# should output some error?
	;;

esac

EYEDB_ARCH_LIBS=$LIBS
AC_CHECK_LIB( pthread, pthread_join, EYEDB_ARCH_LIBS="-lpthread $EYEDB_ARCH_LIBS")

AC_SUBST(EYEDB_ARCH_CPPFLAGS)
AC_SUBST(EYEDB_ARCH_CXXFLAGS)
AC_SUBST(EYEDB_ARCH_SOFLAGS)
AC_SUBST(EYEDB_ARCH_LIBS)

# ========================================
# End of Examples building
# ========================================



AC_CONFIG_FILES( dnl
Makefile dnl
doc/Makefile dnl
doc/images/Makefile dnl
doc/doxygen/Makefile dnl
doc/doxygen/eyedb-doxygen.cfg dnl
doc/javadoc/Makefile dnl
doc/manpages/Makefile dnl
doc/manual/Makefile dnl
doc/manual/administration/Makefile dnl
doc/manual/administration.old/Makefile dnl
doc/manual/cpp-programming/Makefile dnl
doc/manual/docbook-common/Makefile dnl
doc/manual/getting-started/Makefile dnl
doc/manual/getting-started.old/Makefile dnl
doc/manual/installation/Makefile dnl
doc/manual/java-programming/Makefile dnl
doc/manual/latex-common/Makefile dnl
doc/manual/latex-common/version.tex dnl
doc/manual/latex-common/latex2html-init dnl
doc/manual/odl/Makefile dnl
doc/manual/oql/Makefile dnl
doc/manual/overview/Makefile dnl
doc/manual/overview/figures/Makefile dnl
doc/release-notes/Makefile dnl
examples/C++Binding/Makefile dnl
examples/C++Binding/Makefile.eyedb-examples dnl
examples/C++Binding/generic/Makefile dnl
examples/C++Binding/generic/Makefile.eyedb-examples dnl
examples/C++Binding/generic/basic/Makefile dnl
examples/C++Binding/generic/basic/Makefile.eyedb-examples dnl
examples/C++Binding/generic/query/Makefile dnl
examples/C++Binding/generic/query/Makefile.eyedb-examples dnl
examples/C++Binding/generic/schema/Makefile dnl
examples/C++Binding/generic/schema/Makefile.eyedb-examples dnl
examples/C++Binding/generic/store/Makefile dnl
examples/C++Binding/generic/store/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/Makefile dnl
examples/C++Binding/schema-oriented/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/admin/Makefile dnl
examples/C++Binding/schema-oriented/admin/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/methods/Makefile dnl
examples/C++Binding/schema-oriented/methods/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/query/Makefile dnl
examples/C++Binding/schema-oriented/query/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/share/Makefile dnl
examples/C++Binding/schema-oriented/share/Makefile.eyedb-examples dnl
examples/C++Binding/schema-oriented/store/Makefile dnl
examples/C++Binding/schema-oriented/store/Makefile.eyedb-examples dnl
examples/GettingStarted/Makefile dnl
examples/GettingStarted/Makefile.eyedb-examples dnl
examples/JavaBinding/Makefile dnl
examples/JavaBinding/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/Makefile dnl
examples/JavaBinding/generic/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/applet/Makefile dnl
examples/JavaBinding/generic/applet/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/Makefile dnl
examples/JavaBinding/generic/appli/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/basic/Makefile dnl
examples/JavaBinding/generic/appli/basic/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/collections/Makefile dnl
examples/JavaBinding/generic/appli/collections/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/query/Makefile dnl
examples/JavaBinding/generic/appli/query/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/schema/Makefile dnl
examples/JavaBinding/generic/appli/schema/Makefile.eyedb-examples dnl
examples/JavaBinding/generic/appli/store/Makefile dnl
examples/JavaBinding/generic/appli/store/Makefile.eyedb-examples dnl
examples/JavaBinding/schema-oriented/Makefile dnl
examples/JavaBinding/schema-oriented/Makefile.eyedb-examples dnl
examples/JavaBinding/schema-oriented/share/Makefile dnl
examples/JavaBinding/schema-oriented/share/Makefile.eyedb-examples dnl
examples/JavaBinding/schema-oriented/share/person/Makefile dnl
examples/JavaBinding/schema-oriented/store/Makefile dnl
examples/JavaBinding/schema-oriented/store/Makefile.eyedb-examples dnl
examples/Makefile dnl
examples/Makefile.eyedb-examples dnl
examples/oql/Makefile dnl
include/Makefile dnl
include/eyedb/Makefile dnl
include/eyedb/version.h dnl
include/eyedb/internals/Makefile dnl
install/Makefile dnl
install/etc/Makefile dnl
pkg-config/Makefile dnl
scripts/Makefile dnl
scripts/eyedbjrun dnl
src/Makefile dnl
src/bindings/Makefile dnl
src/bindings/python/Makefile dnl
src/eyedb/Makefile dnl
src/eyedb/commands/Makefile dnl
src/eyedb/commands/admin/Makefile dnl
src/eyedb/commands/admin/old/Makefile dnl
src/eyedb/commands/oql/Makefile dnl
src/eyedb/commands/server/Makefile dnl
src/java/Makefile dnl
src/java/src/Makefile dnl
src/java/src/org/Makefile dnl
src/java/src/org/eyedb/Makefile dnl
src/java/src/org/eyedb/Version.java dnl
src/java/src/org/eyedb/dbm/Makefile dnl
src/java/src/org/eyedb/syscls/Makefile dnl
src/java/src/org/eyedb/utils/Makefile dnl
src/oql/Makefile dnl
src/www/Makefile dnl
src/www/eyedb.conf dnl
src/www/html/Makefile dnl
src/www/icons/Makefile dnl
)
AC_OUTPUT

# make symbolic links for ODL files
for f in src/java/src/org/eyedb/dbm/DBM.odl src/java/src/org/eyedb/utils/utils.odl src/java/src/org/eyedb/syscls/syscls.odl 
do
	if test ! -f $srcdir/$f
	then 
		test -L $srcdir/$f && rm $srcdir/$f
		(cd $srcdir/`dirname $f`; ln -s $srcdir/src/eyedb/`basename $f`)
	fi
done

echo
echo
echo "  EyeDB ${PACKAGE_VERSION} configured for '${host}'."
echo
echo "  EyeDB will be compiled with the following options:"
echo 
echo "  *** EyeDB Core ***" 
echo "  EyeDB will be installed in: ${prefix:-none}"
V=${eyedb_databasedir}; while echo $V | grep '\$' > /dev/null 2>&1; do V=$(eval echo $V) ; done
echo "  EyeDB databases will be in: $V"
V=${eyedb_pipedir}; while echo $V | grep '\$' > /dev/null 2>&1; do V=$(eval echo $V) ; done
echo "  EyeDB pipes will be in: $V"
V=${eyedb_tmpdir}; while echo $V | grep '\$' > /dev/null 2>&1; do V=$(eval echo $V) ; done
echo "  EyeDB temporary files will be in: $V"
echo "  C++ compiler: ${CXX:-none}"
echo "  C++ compiler flags: ${CXXFLAGS:-none}"
echo "  Compile with debug: ${enable_debug:-no}"
echo "  Compile with profile: ${enable_profile:-no}"
echo "  Compile with optimization: ${enable_optimize:-no}"
echo "  Semaphore policy: ${ac_semaphore_policy}"
V=$(if test x"$ac_cv_check_readline" = x"yes"; then echo "readline"; elif test x"$ac_cv_check_editline" = x"yes"; then echo "editline"; else echo "none" ; fi)
echo "  Command line editing in OQL: $V"
echo "  Libraries: ${LIBS}"
echo 
echo "  *** EyeDB Java binding ***" 
echo "  Compile Java binding: ${COND_JAVA:-no}"
echo "  Java compiler: ${JAVAC:-none}"
echo "  Java compiler flags: ${JAVACFLAGS:-none}"
echo "  Java virtual machine: ${JAVA:-none}"
echo "  Jar utility: ${JAR:-none}"
echo 
echo "  *** EyeDB Python pexpect tests ***" 
echo "  Run Python pexpect tests: ${COND_PYTHON_PEXPECT:-no}"
echo 
echo "  *** EyeDB Swig multi-languages binding ***" 
echo "  Compile Swig bindings: ${COND_SWIG:-no}"
echo 
echo "  *** EyeDB documentation ***" 
echo "  Generate documentation: ${COND_DOC:-no}"
echo "  Generate doxygen C++ api documentation: ${COND_DOXYGEN:-no}"
echo "  Generate javadoc Java api documentation: ${COND_JAVADOC:-no}"
echo "  Generate Docbook man pages: ${COND_DOCBOOK_MANPAGES:-no}"
echo "  Generate LaTEX PDF manual: ${COND_LATEX:-no}"
echo "  Generate LaTEX HTML manual: ${COND_LATEX2HTML:-no}"
echo "  Generate Docbook PDF manual: ${COND_DBLATEX:-no}"
echo "  Generate Docbook HTML manual: ${COND_DOCBOOK_HTML:-no}"
echo

