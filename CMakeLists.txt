cmake_minimum_required(
   VERSION 3.15
)

option(BUILD_DOC
   "Build LaTeX documentation"
   OFF
)

project(ipr
   VERSION 0.50
   LANGUAGES CXX
)

add_library(${PROJECT_NAME}
   src/interface.cxx
   src/impl.cxx
   src/io.cxx
   src/traversal.cxx
   src/utility.cxx
)

target_include_directories(${PROJECT_NAME}
   PUBLIC
      ${PROJECT_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/ipr/ancillary
        ${PROJECT_SOURCE_DIR}/include/ipr/attribute
        ${PROJECT_SOURCE_DIR}/include/ipr/cxx-form
        ${PROJECT_SOURCE_DIR}/include/ipr/impl
        ${PROJECT_SOURCE_DIR}/include/ipr/interface
        ${PROJECT_SOURCE_DIR}/include/ipr/io
        ${PROJECT_SOURCE_DIR}/include/ipr/lexer
        ${PROJECT_SOURCE_DIR}/include/ipr/location
        ${PROJECT_SOURCE_DIR}/include/ipr/node-category
        ${PROJECT_SOURCE_DIR}/include/ipr/synopsis
        ${PROJECT_SOURCE_DIR}/include/ipr/traversal
        ${PROJECT_SOURCE_DIR}/include/ipr/utility
        ${PROJECT_SOURCE_DIR}/3rdparty/doctest/doctest.h
)

set_target_properties(${PROJECT_NAME}
   PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
	   CXX_EXTENSIONS OFF
)

target_compile_options(${PROJECT_NAME}
   PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
         /W2           # Usual warnings
         /permissive-  # Turn on strict language conformance
         /EHsc         # Turn on exception handling semantics
      >
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
         -Wall         # Turn on all useful warnings
         -pedantic     # Turn on strict language conformance
      >
      $<$<CXX_COMPILER_ID:Clang>:
		 -Wno-overloaded-virtual                   # Too many false positives
		 -Wno-delete-non-abstract-non-virtual-dtor # System headers plagued
      >
)

target_compile_definitions(${PROJECT_NAME}
   PRIVATE
	  $<$<PLATFORM_ID:UNIX>:
		 _FILE_OFFSET_BITS=64 # We want the ability to process large files.
	  >
)

install(
   TARGETS ipr
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
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

