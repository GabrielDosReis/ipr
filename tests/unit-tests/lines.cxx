#include "doctest/doctest.h"
#ifdef _WIN32
#  include <windows.h>
#  define WIDEN_(S) L ## S
#  define WIDEN(S) WIDEN_(S)
#else
#  define WIDEN(S) S
#endif

#include <iostream>
#include "ipr/input"

TEST_CASE("echo input file") {
    ipr::input::SystemPath path = WIDEN(__FILE__);
    ipr::input::SourceFile file{path};
    auto n = 1;
    std::cout << "file.size: " << file.contents().size() << std::endl;
    for (auto line : file.lines())
    {
        std::cout << '[' << n << ']'
                << " -> {offset: " << line.offset
                << ", length: " << line.length << "}\n";
        ++n;
    }
    CHECK(n == 27); // Adjust this number based on the actual number of lines in the file
}
