#include <doctest/doctest.h>

#include <ipr/impl>
#include <ipr/io>
#include <ipr/traversal>
#include <sstream>

TEST_CASE("global constant variable can be printed") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module m{lexicon};
  impl::Interface_unit unit{lexicon, m};

  impl::Scope* global_scope = unit.global_scope();

  auto& name = lexicon.get_identifier("bufsz");
  auto& type = lexicon.get_qualified(Type_qualifiers::Const, lexicon.int_type());
  impl::Var* var = global_scope->make_var(name, type);
  var->init = lexicon.make_literal(lexicon.int_type(), "1024");

  std::stringstream ss;
  Printer pp{ss};
  pp << unit;
  CHECK(!ss.str().empty());
}

TEST_CASE("Can create and print line numbers")
{
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module m{lexicon};
  impl::Interface_unit unit{lexicon, m};

  impl::Scope* global_scope = unit.global_scope();

  auto& name = lexicon.get_identifier("bufsz");
  auto& type = lexicon.get_qualified(Type_qualifiers::Const, lexicon.int_type());
  impl::Var* var = global_scope->make_var(name, type);
  var->init = lexicon.make_literal(lexicon.int_type(), "1024");

  Source_location loc{ Line_number{1}, Column_number{2}, File_index{1} };
  var->src_locus = loc;

  std::stringstream ss;
  Printer pp{ss};
  pp << unit;
  // By default location printing is off
  CHECK(ss.str().find("F1:1:2") == std::string::npos);
  pp.print_locations = true;
  pp << unit;
  // Now we should see a location printed.
  // File name is printed as a file index for brevity
  CHECK(ss.str().find("F1:1:2") != std::string::npos);
}

TEST_CASE("linkages are deduplicated") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  auto& l1 = lexicon.cxx_linkage();
  auto& l2 = lexicon.cxx_linkage();
  CHECK(&l1 == &l2);
}

TEST_CASE("nullptr defines its own type") {
  using namespace ipr;
  impl::Lexicon lexicon { };
  auto& null = lexicon.nullptr_value();
  auto& type = lexicon.get_decltype(null);
  CHECK(physically_same(type, null.type()));
}

TEST_CASE("Truth values have type bool") {
  using namespace ipr;
  impl::Lexicon lexicon { };
  auto& vrai = lexicon.true_value();
  auto& faux = lexicon.false_value();
  CHECK(physically_same(vrai.type(), lexicon.bool_type()));
  CHECK(physically_same(faux.type(), lexicon.bool_type()));
}

