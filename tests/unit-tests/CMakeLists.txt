set(TEST_BINARY unittests)

add_executable(${TEST_BINARY}
   main.cpp
   conversions.cpp
   simple.cpp
)

target_link_libraries(${TEST_BINARY}
   ${PROJECT_NAME}
)

target_include_directories(${TEST_BINARY}
  PRIVATE ${DOCTEST_INCLUDE_DIR}
)

set_target_properties(${TEST_BINARY}
   PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
)

target_compile_options(${TEST_BINARY}
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

add_test(simple ${TEST_BINARY})

