AC_INIT([ipr],[0.47],[gdr@acm.org])
AM_INIT_AUTOMAKE([foreign])

dnl * We require Autoconf 2.69 or higher.
AC_PREREQ([2.69])

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG(C++)
IPR_REQUIRE_MODERN_CXX
LT_INIT

AC_PROG_INSTALL
AC_PROG_LN_S



dnl * We need to compile LaTeX files.
AC_CHECK_PROG(LATEX, latex, latex)

AC_CONFIG_FILES(Makefile include/Makefile src/Makefile doc/Makefile)

AC_OUTPUT
