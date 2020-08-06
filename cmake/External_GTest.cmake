cmake_minimum_required(VERSION 3.9)
project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(
   googletest
   GIT_REPOSITORY https://github.com/google/googletest.git
   GIT_TAG release-1.10.0
   PREFIX "${CMAKE_BINARY_DIR}/gtest"
   CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/gtest"
   UPDATE_COMMAND ""
   LOG_DOWNLOAD ON
   LOG_CONFIGURE ON
   LOG_BUILD ON)

