#include "doctest/doctest.h"

#include <ipr/impl>

TEST_CASE("asm-declaration")
{
    using namespace ipr;
    impl::Lexicon lexicon { };

    auto& s = lexicon.get_string(u8"yo!");
    auto insn = lexicon.make_asm(s);
    CHECK(physically_same(insn->type(), lexicon.void_type()));
}

TEST_CASE("static_assert-declaration")
{
    using namespace ipr;
    impl::Lexicon lexicon { };
    auto cond = lexicon.make_not(lexicon.false_value(), lexicon.bool_type());
    auto assert = lexicon.make_static_assert(*cond, lexicon.get_string(u8"wait! what?"));
    CHECK(physically_same(assert->type(), lexicon.bool_type()));
}
