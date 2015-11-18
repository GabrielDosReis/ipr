# Copyright (C) 2015, Gabriel Dos Reis and Bjarne Stroustrup.
# All rights reserved.
# 
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# 3. Neither the names of the copyright holders nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dnl ---------------------------
dnl -- IPR_CXX_ACCEPT_SWITCH --
dnl ---------------------------
dnl Check whether the booting C++ compiler accepts a given switch.
dnl If yes, augment the list passed as second argument.
AC_DEFUN([IPR_CXX_ACCEPT_SWITCH],[
flag=$1
switch_list=$2
if test -z $switch_list; then
   switch_list=CXXFLAGS
fi
old_list=\$${switch_list}
ipr_saved_cxxflags="$CXXFLAGS"
AC_MSG_CHECKING([whether $CXX supports "$flag"])
CXXFLAGS="$CXXFLAGS $flag"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
	[AC_MSG_RESULT([yes])]
	[CXXFLAGS=$ipr_saved_cxxflags]
	[eval "$switch_list=\"$old_list $flag\""],
	[AC_MSG_RESULT([no])]
	[CXXFLAGS="$ipr_saved_cxxflags"])
])


dnl ----------------------------
dnl -- IPR_REQUIRE_MODERN_CXX --
dnl ----------------------------
dnl Require a modern C++ compiler, e.g. at least C++14.
AC_DEFUN([IPR_REQUIRE_MODERN_CXX],[
# Assume supported compilers understand -std=c++14
ipr_saved_cxx_flags="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++14"
AC_MSG_CHECKING([whether C++ compiler supports C++14])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])]
  [AC_MSG_ERROR([IPR requires a C++ that supports at least C++14])])
])
