cmake_minimum_required(VERSION 2.8.12)
project (ipr)
set (ipr_VERSION_MAJOR 0)
set (ipr_VERSION_MINOR 47)

# Hey, we are building C++ for C++.
enable_language(CXX)

# Microsoft's Visual C++ still does not support alternative keywords! :-/
# Also, set the appropriate exception handling flavor.
if (WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FIiso646.h")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

# On UNIX systems, we do want the ability to process large files.
if (UNIX)
   add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

# We really want to use C++14 (bug fix over C++11).
# When the booting compiler is g++ or clang, use -std= flag.
# Approximate that with UNIX platforms for now.
if (UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2 -Wall")
endif()

# Clang can sometimes be overly anal-rententive vis-a-vis good programming idioms :-(
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
endif()

# Set up search paths for source header files.
include_directories ("${PROJECT_SOURCE_DIR}/include")

# The final IPR library
add_library(ipr STATIC
		src/interface.cxx
		src/impl.cxx
		src/io.cxx
		src/traversal.cxx
		src/utility.cxx)


## Installation time, folks.
install(TARGETS ipr
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(DIRECTORY include/ipr DESTINATION include)

