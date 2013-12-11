AC_INIT([descripten],[0.1])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_PROG_AR
LT_INIT
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# These compilation flags must be set before AC_PROG_CC/CXX,
# or we'll get "-g -O2" by default.
#CFLAGS="${CFLAGS=} -g3 -Wall"
#CXXFLAGS="${CXXFLAGS=} -g3 -Wall"

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_CONFIG_MACRO_DIR([m4])

# Version information (current:revision:age).
COMMON_VERSION=1:0:0
AC_SUBST(COMMON_VERSION)
PARSER_VERSION=1:0:0
AC_SUBST(PARSER_VERSION)
IR_VERSION=1:0:0
AC_SUBST(IR_VERSION)
PEREGRINE_VERSION=1:0:0
AC_SUBST(PEREGRINE_VERSION)
RICHMOND_VERSION=1:0:0
AC_SUBST(RICHMOND_VERSION)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

CXXFLAGS="$CXXFLAGS -std=c++11"

CXXFLAGS="$CXXFLAGS `pkg-config --cflags bdw-gc libpcre`"
LIBS="$LIBS `pkg-config --libs bdw-gc libpcre`"

# Check if debug or release build.
AC_MSG_CHECKING(whether to generate a debug build)
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug=[[yes/no]]],
                              [generates a debug build [default=no]])],
              [case "${enableval}" in
               yes) debug_build=true ;;
               no)  debug_build=false ;;
               *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
               esac],
              debug_build=false)

arch=`uname -m`
platform=`uname -s`

AM_CONDITIONAL([DEBUG], [test x$debug_build = xtrue])
AM_CONDITIONAL([ARCH_X86_64], [test x$arch = xx86_64])
AM_CONDITIONAL([ARCH_I686], [test x$arch = xi686])
AM_CONDITIONAL([PLATFORM_LINUX], [test x$platform = xLinux])
AM_CONDITIONAL([PLATFORM_DARWIN], [test x$platform = xDarwin])

if [ test x$platform = xDarwin ]
then
    AM_CONDITIONAL([ARCH_X86_64], [true])
fi

if [ test x$debug_build = xtrue ]
then
    AC_MSG_RESULT(yes)
    CXXFLAGS="$CXXFLAGS -Wall -Werror -Wfatal-errors -Wno-switch -O0 -DDEBUG"
else
    AC_MSG_RESULT(no)
    # If we let the 'omit frame pointer' optimisation enabled,
    # we'll have difficulty later on while trying
    # to analyse crash dump files sent by the users.
    CXXFLAGS="$CXXFLAGS -Wall -Werror -Wfatal-errors -Wno-switch -O3 -DNDEBUG -fno-omit-frame-pointer"
fi

AC_DEFINE(_UNIX)
AC_DEFINE(_FILE_OFFSET_BIT,64,[Enable support for large files.])

AC_CONFIG_FILES([Makefile common/Makefile parser/Makefile ir/Makefile runtime/Makefile compiler/Makefile tools/Makefile])
AC_OUTPUT

