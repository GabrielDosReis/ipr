#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <ipr/impl>
#include <ipr/io>
#include <sstream>

TEST_CASE("global constant variable can be printed") {
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
  CHECK(!ss.str().empty());
}

TEST_CASE("Can create and print line numbers")
{
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module m{lexicon};
  impl::Interface_unit unit{lexicon, m};

  impl::Scope* global_scope = unit.global_scope();

  const Name* name = lexicon.make_identifier("bufsz");
  auto& type = lexicon.get_qualified(Type_qualifier::Const, lexicon.int_type());
  impl::Var* var = global_scope->make_var(*name, type);
  var->init = lexicon.make_literal(lexicon.int_type(), "1024");

  auto file_index = lexicon.make_fileindex(lexicon.get_string("hello.cpp"));
  Source_location loc{ Line_number{1}, Column_number{2}, file_index};
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

