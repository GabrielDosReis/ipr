#include "doctest/doctest.h"

#include <ipr/impl>

using namespace ipr;

TEST_CASE("asm-declaration")
{
    impl::Lexicon lexicon { };

    auto& s = lexicon.get_string("yo!");
    auto insn = lexicon.make_asm(s);
    CHECK(physically_same(insn->type(), lexicon.void_type()));
}

TEST_CASE("static_assert-declaration")
{
    impl::Lexicon lexicon { };
    auto cond = lexicon.make_not(lexicon.false_value(), lexicon.bool_type());
    auto assert = lexicon.make_static_assert(*cond, lexicon.get_string("wait! what?"));
    CHECK(physically_same(assert->type(), lexicon.bool_type()));
}
