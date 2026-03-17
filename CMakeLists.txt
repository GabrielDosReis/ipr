cmake_minimum_required(
   VERSION 3.31
)
include(CheckIPOSupported)

option(BUILD_DOC
   "Build LaTeX documentation"
   OFF
)

project(ipr
   VERSION 0.50
   LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# Enable link-time optimization or close approximation
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_disabled_msg)
if (ipo_supported)
   set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
   message(WARNING "LTO disabled: ${ipo_disabled_msg}")
endif()

add_library(${PROJECT_NAME}
   src/impl.cxx
   src/cxx-ipr-io.cxx
   src/cxx-ipr-traversal.cxx
   src/input.cxx
   src/utility.cxx
   src/cxx-ipr.cxx
)

target_sources(${PROJECT_NAME}
   PUBLIC
      FILE_SET CXX_MODULES FILES
         src/cxx-ipr-vocabulary.ixx
         src/cxx-ipr-syntax.ixx
         src/cxx-ipr.ixx
         src/cxx-ipr-traversal.ixx
         src/cxx-ipr-io.ixx
)

target_include_directories(${PROJECT_NAME}
   PUBLIC
      ${PROJECT_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/ipr/impl
        ${PROJECT_SOURCE_DIR}/include/ipr/lexer
        ${PROJECT_SOURCE_DIR}/include/ipr/node-category
        ${PROJECT_SOURCE_DIR}/include/ipr/utility-impl
        ${PROJECT_SOURCE_DIR}/include/ipr/input
        ${PROJECT_SOURCE_DIR}/3rdparty/doctest/doctest.h
)

set_target_properties(${PROJECT_NAME}
   PROPERTIES
      CXX_STANDARD 23
      CXX_STANDARD_REQUIRED ON
	   CXX_EXTENSIONS OFF
)

target_compile_options(${PROJECT_NAME}
   PUBLIC
      $<$<CXX_COMPILER_ID:MSVC>:
         /permissive-  # Turn on strict language conformance
         /EHsc         # Turn on exception handling semantics
      >
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
         -Wno-overloaded-virtual  # Avoid bogus warnings against foundational techniques.
      >
   PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
         /W2           # Usual warnings
      >
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
         -Wall         # Turn on all useful warnings
         -pedantic     # Turn on strict language conformance
         >
      $<$<CXX_COMPILER_ID:Clang>:
		 -Wno-delete-non-abstract-non-virtual-dtor # System headers plagued
      >
)

target_compile_definitions(${PROJECT_NAME}
   PRIVATE
	   $<$<PLATFORM_ID:UNIX>:
		   _FILE_OFFSET_BITS=64 # We want the ability to process large files.
	   >
)

set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/input.cxx APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG)

install(
   TARGETS ipr
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   FILE_SET CXX_MODULES DESTINATION lib/cmake/ipr
)
install(
   DIRECTORY include/ipr
   DESTINATION include
)

if(BUILD_DOC)
   include(cmake/UseLATEX.cmake)
   add_subdirectory(doc)
endif()

# Testing
enable_testing()
set(DOCTEST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/3rdparty")
add_subdirectory(tests)

