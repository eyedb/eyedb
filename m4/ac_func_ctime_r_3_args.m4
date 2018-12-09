dnl checking whether ctime_r has 2 or 3 arguments
AC_DEFUN([AC_FUNC_CTIME_R_3_ARGS],[
AC_CHECK_FUNC(ctime_r,ac_func_ctime_r="yes",ac_func_ctime_r="no",[#include <sys/time.h>])
if test "$ac_func_ctime_r" = "no"
then
   AC_MSG_ERROR([cannot find function ctime_r()])
else
   AC_MSG_CHECKING(for ctime_r arguments count)
   AC_LANG_ASSERT(C++)
   ctime_r3="yes"
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[#include <sys/time.h>]],[[char buf[26]; time_t t; ctime_r(&t,buf,26);]])],
     [AC_DEFINE(HAVE_CTIME_R_3, 1, [Define if you have ctime_r(time_t*,char *buf,size_t s)])],
     [ctime_r3="no"]
     )
   if test "$ctime_r3" = "yes"
   then
	AC_MSG_RESULT(3)
   else
	AC_MSG_RESULT(2)
   fi
fi
])