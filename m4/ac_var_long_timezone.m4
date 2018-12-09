dnl checking for variable timezone of type long
AC_DEFUN([AC_VAR_LONG_TIMEZONE],[
AC_CHECK_FUNC(tzset,ac_func_tzset="yes",ac_func_tzset="no",[#include <time.h>])
if test "$ac_func_tzset" = "yes"
then
	AC_MSG_CHECKING(for long timezone)
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[#include <time.h>]],[[long tmp; tzset();tmp = -(timezone / 60);]])],
		[ac_var_long_timezone="yes"],
		[ac_var_long_timezone="no"]
		)
	if test "$ac_var_long_timezone" = "yes"; then
		AC_DEFINE(HAVE_VAR_LONG_TIMEZONE, 1, [Define if you have long timezone])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi
])
