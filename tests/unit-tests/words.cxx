#include "doctest/doctest.h"

#include <ipr/traversal>
#include <ipr/impl>

TEST_CASE("words are unified")
{
    ipr::util::string_pool pool { };

    auto& int1 = pool.intern(u8"int");
    auto& int2 = pool.intern(u8"int");
    CHECK(ipr::physically_same(int1, int2));

    auto& foo1 = pool.intern(u8"fhoo");
    auto& foo2 = pool.intern(u8"fhoo");
    CHECK(ipr::physically_same(foo1, foo2));
}
