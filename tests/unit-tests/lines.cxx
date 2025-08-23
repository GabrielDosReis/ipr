#include "doctest/doctest.h"
#ifdef _WIN32
#  include <windows.h>
#  define WIDEN_(S) L ## S
#  define WIDEN(S) WIDEN_(S)
#else
#  define WIDEN(S) S
#endif

#define  DUP(S) \
    S ## S

#include <iostream>
#include "ipr/input"

TEST_CASE("echo input file") {
    ipr::input::SystemPath path = WIDEN(__FILE__);
    ipr::input::SourceListing file{path};
    std::cout << "file.size: " << file.contents().size() << std::endl;
    std::uint32_t last_line_number = 0;
    for (auto line : file.lines())
    {
        std::cout << '[' << line.number << ']'
                << " -> {offset: " << line.morsel.offset
                << ", length: " << line.morsel.length << "}\n";
        last_line_number = line.number;
    }
    CHECK(last_line_number == 29); // Adjust this number based on the actual number of lines in the file
}
