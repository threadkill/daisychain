AC_PREREQ([2.71])
AC_INIT([daisychain],[1.0],[BUG-REPORT-ADDRESS])

# Automake macros
AM_INIT_AUTOMAKE([silent-rules] [subdir-objects] [foreign])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

# Libtool macros
LT_PREREQ([2.2])
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SUBDIRS([libdaisychain daisy])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

echo "
_____________________________________________________________________________
${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Targets:
    Type 'make' to build.
    Type 'make install' to install to "${prefix}"
    Type 'make dist' to create tarball.

"
