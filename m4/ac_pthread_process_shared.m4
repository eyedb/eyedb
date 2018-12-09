dnl checking whether Posix threads implementation supports PTHREAD_PROCESS_SHARED
AC_DEFUN([AC_PTHREAD_PROCESS_SHARED],[
	AC_MSG_CHECKING(whether PTHREAD_PROCESS_SHARED is supported)
	AC_RUN_IFELSE(
		[AC_LANG_PROGRAM([[
#include <pthread.h>
#include <stdlib.h>
]],
				[[
pthread_mutex_t mut;
pthread_mutexattr_t mattr;
int r;
if ((r = pthread_mutexattr_init(&mattr)) != 0) exit(r);
if ((r = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) != 0) exit(r);
if ((r = pthread_mutex_init(&mut, &mattr)) != 0) exit(r);
if ((r = pthread_mutexattr_destroy(&mattr)) != 0) exit(r);
exit(0);
		]])],
		[ac_pthread_process_shared="yes"],
		[ac_pthread_process_shared="no"]
		)
	if test "$ac_pthread_process_shared" = "yes"; then
		AC_DEFINE(HAVE_PTHREAD_PROCESS_SHARED, 1, [Define if Posix threads implementation supports PTHREAD_PROCESS_SHARED])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
])