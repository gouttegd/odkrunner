dnl ICP_CHECK_NOTCH_FUNCS
dnl Check for various functions that may not be present
dnl everywhere, and for which we provide a replacement.
dnl
AC_DEFUN([ICP_CHECK_NOTCH_FUNCS],[
AC_CHECK_HEADERS([err.h], [], [AC_CONFIG_LINKS([err.h:lib/err.compat.h])])
AC_REPLACE_FUNCS([err realpath])
AC_CHECK_DECLS([program_invocation_short_name], [],
               [AC_CHECK_FUNCS([getprogname setprogname], [],
                               [AC_LIBOBJ([progname])])],
               [#include <errno.h>
                ])
AC_CHECK_DECLS([P_tmpdir])
AH_BOTTOM([#include <compat.h>
           ])
])
