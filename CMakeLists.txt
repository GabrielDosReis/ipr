cmake_minimum_required(
   VERSION 3.9
)

option(BUILD_DOC
   "Build LaTeX documentation"
   OFF
)

option(BUILD_TESTS
   "Build the tests. Requires GTest."
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

if(BUILD_TESTS)
   enable_testing()

   # Download GTest.
   include(cmake/External_GTest.cmake)

   include(GoogleTest)
   set(TEST_BINARY tests)
   add_executable(${TEST_BINARY}
      tests/main.cpp
      tests/simple.cpp
   )
   target_include_directories(${PROJECT_NAME}
     PRIVATE
         ${PROJECT_SOURCE_DIR}/include
   )
   target_link_libraries(${TEST_BINARY}
      ${PROJECT_NAME}
      ${GTEST_LIBRARY}
   )
   set_target_properties(${TEST_BINARY}
      PROPERTIES
         CXX_STANDARD 17
         CXX_STANDARD_REQUIRED ON
       CXX_EXTENSIONS OFF
   )
   gtest_add_tests(
      TARGET ${TEST_BINARY}
   )
endif()
