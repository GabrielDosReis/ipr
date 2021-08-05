#include "doctest/doctest.h"

#include <ipr/traversal>
#include <ipr/impl>

TEST_CASE("words are unified")
{
    ipr::util::string_pool pool { };

    auto& int1 = pool.intern("int");
    auto& int2 = pool.intern("int");
    CHECK(ipr::physically_same(int1, int2));

    auto& foo1 = pool.intern("fhoo");
    auto& foo2 = pool.intern("fhoo");
    CHECK(ipr::physically_same(foo1, foo2));
}
