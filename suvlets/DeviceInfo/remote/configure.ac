#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([suvlet-device-info], [1.0], [http://www.clearfoundation.com/docs/developer/bug_tracker/])
AM_INIT_AUTOMAKE([suvlet-device-info], [1.0])
AC_CONFIG_SRCDIR([suva_receiver.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Check for Suva SCL API header
AC_ARG_WITH([sclapi],
    [AS_HELP_STRING([--with-sclapi],
        [specifiy Suva SCL API include location @<:@default=../../..@:>@])],
    [],
    [with_sclapi=../../..])

if test "x$with_sclapi" != xno; then
    CFLAGS="$CFLAGS -I$with_sclapi"
    CXXFLAGS="$CXXFLAGS -I$with_sclapi"
fi

# Check for Suva SCL API library 
AC_ARG_WITH([scllib],
    [AS_HELP_STRING([--with-scllib],
        [specifiy Suva SCL API library location @<:@default=../../../.libs@:>@])],
    [],
    [with_scllib=../../../.libs])

LIBS="$LIBS -lcompat-suva"
if test "x$with_scllib" != xno; then
    LDFLAGS="$LDFLAGS -L$with_scllib"
fi

# Checks for libraries.
#AC_CHECK_LIB([compat-suva], [suva_api_init], [], [
#	AC_MSG_ERROR([suva_api_init not found but is required.])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
