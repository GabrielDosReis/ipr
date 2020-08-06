set(TEST_BINARY tests)

add_executable(${TEST_BINARY}
   main.cpp
   simple.cpp
)

target_link_libraries(${TEST_BINARY}
   ${PROJECT_NAME}
   GTest::GTest
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
