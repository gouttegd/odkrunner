AC_INIT([ODK Runner], [0.1.0],
        [dgouttegattat@incenp.org], [odkrun])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([lib])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign subdir-objects silent-rules])

dnl Check for development tools
AC_PROG_CC

dnl Check for some non-ubiquitous functions
ICP_CHECK_NOTCH_FUNCS

# We use the absence of sys/wait as an indicator
# that we are likely compiling for Windows.
AC_CHECK_HEADERS([sys/wait.h])
AC_CHECK_HEADERS([windows.h])

dnl Support for macOS universal binaries
AC_ARG_ENABLE([fat-binaries],
              [AS_HELP_STRING([--disable-fat-binaries],
                              [On macOS, only build for the current architecture.])],
              [build_fat_binaries=$enableval],
              [build_fat_binaries=yes])
AS_IF([test "x$build_fat_binaries" = xyes],
      [MACOS_EXTRA_CFLAGS="$MACOS_EXTRA_CFLAGS -arch x86_64 -arch arm64"],
      [MACOS_EXTRA_CFLAGS="$MACOS_EXTRA_CFLAGS"])

dnl Set minimal version of macOS targeted
AC_ARG_WITH([min-osx-version],
            [AS_HELP_STRING([--with-min-osx-version],
                            [Specify the minimum macOS version the program will run on.])],
            [MACOS_EXTRA_CFLAGS="$MACOS_EXTRA_CFLAGS -mmacosx-version-min=$withval"],
            [MACOS_EXTRA_CFLAGS="$MACOS_EXTRA_CFLAGS"])

dnl Host-based tweaks
AC_CANONICAL_HOST
AS_CASE([$host],
        [*-*-darwin*], [CFLAGS="$CFLAGS $MACOS_EXTRA_CFLAGS"])

dnl Output files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl Summary
echo "

${PACKAGE_NAME} version ${PACKAGE_VERSION}
Configuration complete

Compiler:           '${CC} ${CFLAGS} ${CPPFLAGS}'

"
