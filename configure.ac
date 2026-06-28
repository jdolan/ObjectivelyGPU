AC_PREREQ(2.69)

AC_INIT([ObjectivelyGPU], [1.0.0], [jay@jaydolan.com])

AC_CONFIG_HEADERS([Sources/ObjectivelyGPU/Config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Sources/ObjectivelyGPU.h])

AC_CANONICAL_HOST

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC

AC_CHECK_PROG([GLSLC], [glslc], [glslc], [])
AS_IF([test -z "$GLSLC"],
  [AC_MSG_WARN([glslc not found; 'make shaders' will not work])])
AC_CHECK_PROG([SHADERCROSS], [shadercross], [shadercross], [])
AS_IF([test -z "$SHADERCROSS"],
  [AC_MSG_WARN([shadercross not found; 'make shaders' will not work])])
LT_INIT

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

AH_TOP([#pragma once])

HOST_NAME=""
HOST_CFLAGS=""
HOST_LDFLAGS=""
HOST_LIBS=""

AC_MSG_CHECKING([host type])
case "$host" in
	*-apple-*)
		HOST_NAME="APPLE"
		HOST_LDFLAGS="-headerpad_max_install_names"
		AC_MSG_RESULT(apple)
		;;
	*-mingw*)
		HOST_NAME="MINGW"
		HOST_LDFLAGS="-no-undefined"
		AC_MSG_RESULT(mingw)
		;;
	*)
		HOST_NAME="LINUX"
		AC_MSG_RESULT(linux)
		;;
esac

AC_SUBST(HOST_NAME)
AC_SUBST(HOST_CFLAGS)
AC_SUBST(HOST_LDFLAGS)
AC_SUBST(HOST_LIBS)

AM_CONDITIONAL([APPLE], [test "x$HOST_NAME" = "xAPPLE"])
AM_CONDITIONAL([MINGW], [test "x$HOST_NAME" = "xMINGW"])
AM_CONDITIONAL([LINUX], [test "x$HOST_NAME" = "xLINUX"])

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
PKG_CHECK_MODULES([OBJECTIVELY], [Objectively >= 2.0.0])
PKG_CHECK_MODULES([SDL3], [sdl3 >= 3.2.0])

AC_CONFIG_FILES([
	Makefile
	Examples/Makefile
	Examples/Shaders/Makefile
	Sources/Makefile
	Sources/ObjectivelyGPU.pc
	Sources/ObjectivelyGPU/Makefile
])

AC_OUTPUT
