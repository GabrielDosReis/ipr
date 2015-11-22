lib_LTLIBRARIES	= libipr.la

libipr_la_SOURCES = utility.cxx \
		    interface.cxx \
		    impl.cxx \
		    traversal.cxx \
		    io.cxx
#		    lexer.C

AM_CPPFLAGS	= -I@top_builddir@/include -I@top_srcdir@/include
WARN_FLAGS	= -Wall -Wextra -Wno-non-virtual-dtor
AM_CXXFLAGS	= $(WARN_FLAGS) -O2 -g
