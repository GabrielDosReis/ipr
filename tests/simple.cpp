#include <gtest/gtest.h>
#include <ipr/impl>
#include <ipr/io>
#include <sstream>

TEST(TestIO, GlobalConstIntWithInit) {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module m{lexicon};
  impl::Interface_unit unit{lexicon, m};

  impl::Scope* global_scope = unit.global_scope();

  const Name* name = lexicon.make_identifier("bufsz");
  auto& type = lexicon.get_qualified(Type_qualifier::Const, lexicon.int_type());
  impl::Var* var = global_scope->make_var(*name, type);
  var->init = lexicon.make_literal(lexicon.int_type(), "1024");

  std::stringstream ss;
  Printer pp{ss};
  pp << unit;
  EXPECT_GT(ss.str().size(), 0);
}



