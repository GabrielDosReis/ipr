cmake_minimum_required(
   VERSION 3.5 # Ubuntu 16.04 LTS
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
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED ON
	  CXX_EXTENSIONS OFF
)
				
target_compile_options(${PROJECT_NAME}
   PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
         /W4           # Highest sensible warning level
         /permissive-  # Turn on strict language conformance
         /EHsc         # Turn on exception handling semantics
         /FIiso646.h   # MSVC still does not support alternative keywords! :-/
      >
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
         -Wall         # Turn on all warnings
         -Wextra       # Turn on even more warnings
         -pedantic     # Turn on strict language conformance
      >
      $<$<CXX_COMPILER_ID:Clang>:
		 -Wno-overloaded-virtual  # Too many false positives
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