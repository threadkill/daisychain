AC_PREREQ([2.69])
AC_INIT([daisy], [1.0], [BUG-REPORT-ADDRESS])

# Automake macros
AM_INIT_AUTOMAKE([silent-rules] [subdir-objects] [foreign])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

# Libtool macros
LT_PREREQ([2.2])
LT_INIT([shared disable-static pic-only])

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AX_CXX_COMPILE_STDCXX([20], [noext])

AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
