set(TEST_BINARY unittests)

add_executable(${TEST_BINARY}
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

add_test(simple ${TEST_BINARY})

