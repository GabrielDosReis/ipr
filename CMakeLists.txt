cmake_minimum_required(
   VERSION 3.9
)

option(BUILD_DOC
   "Build LaTeX documentation"
   OFF
)

option(BUILD_TESTING
   "Build the tests. Requires GTest."
   OFF
)

option(DOWNLOAD_GTEST
  "Force downloading GTest instead of using a local copy."
   OFF
)

project(ipr
   VERSION 0.47
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

set_target_properties(${PROJECT_NAME}
   PROPERTIES
      CXX_STANDARD 17
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

if(BUILD_TESTING)
   enable_testing()

   find_package(GTest)

   # Download GTest if not found or forced.
   if(NOT GTest_FOUND OR DOWNLOAD_GTEST)
      message(STATUS "Downloading and building GTest.")
      configure_file(cmake/External_GTest.cmake gtest/CMakeLists.txt)
      execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/gtest"
         OUTPUT_QUIET
      )
      execute_process(COMMAND "${CMAKE_COMMAND}" --build .
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/gtest"
         OUTPUT_QUIET
      )
      set(GTEST_ROOT ${CMAKE_BINARY_DIR}/gtest)
      find_package(GTest REQUIRED)
   endif()

   if(NOT GTest_FOUND)
      message(FATAL_ERROR "GTest could not be found or built.")
   endif()

   include(GoogleTest)

   add_subdirectory(tests)
endif()
