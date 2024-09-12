AC_PREREQ([2.69])
AC_INIT([libdaisychain], [1.0], [BUG-REPORT-ADDRESS])

# Automake macros
AM_INIT_AUTOMAKE([silent-rules] [subdir-objects] [foreign])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

# Libtool macros
LT_PREREQ([2.2])
LT_INIT([static pic-only])

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AX_CXX_COMPILE_STDCXX([20], [noext])
CXXFLAGS="-fPIC -O3 -fvisibility=hidden -flto"

AC_CHECK_HEADERS([fcntl.h stdint.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset mkdir mkfifo])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/graph.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
