dnl @synopsis AC_PYTHON_MODULE(module)
dnl
dnl Checks if Python module can be imported
dnl
dnl Copied from: https://core.fluendo.com/flumotion/trac/browser/flumotion/trunk/common/as-python.m4
dnl Original author: Andrew Dalke
dnl

AC_DEFUN([AC_PYTHON_MODULE],[
	AC_MSG_CHECKING([for python module $1])
	prog="
import sys
try:
  import $1
  sys.exit(0)
except ImportError:
  sys.exit(1)
except SystemExit:
  raise
except Exception, e:
  print '  Error while trying to import $1:'
  print '    %r: %s' % (e, e)
  sys.exit(1)"
 	
if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
then
  AC_MSG_RESULT(found)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(not found)
  ifelse([$3], , :, [$3])
fi
])
