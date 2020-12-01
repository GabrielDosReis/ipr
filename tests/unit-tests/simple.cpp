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

TEST_CASE("nullptr constant can be printed") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module m{lexicon};
  impl::Interface_unit unit{lexicon, m};

  impl::Scope* global_scope = unit.global_scope();

  auto get_nullptr = [&lexicon]() {
    // I don't have the nullptr type yet. I need to use an intermediate type.
    auto* nullptr_lit = lexicon.make_literal(lexicon.void_type(), "nullptr");
    // At this point, the invariants are broken, since the type of nullptr was
    // set to an intermediate type. Here decltype(nullptr) would mean that 
    // intermediate type as per the standard's definition.
    auto& nullptr_type = lexicon.get_decltype(*nullptr_lit);
    // The commented code below would break the ordering of RB tree. 
    // Bot do not compile anyways.
    // nullptr_lit->rep.first = nullptr_type;

    // Make the right nullptr, with the right type. Except not really,
    // since the expression in the decltype and the created literal have
    // different types.
    auto* nullptr_lit_real = lexicon.make_literal(nullptr_type, "nullptr");
    // This check fails;
    CHECK(&nullptr_lit_real->type() == &nullptr_lit->type());

    return nullptr_lit_real;
  };

  auto* first_nullptr = get_nullptr();
  auto* second_nullptr = get_nullptr();
  // Should we except all nullptr literals the same?
  // The code above will not fulfill this check.
  CHECK(first_nullptr == second_nullptr);
  // Should we except all their types the same?
  // How de we ensure that? The code above will not fulfill this check.
  // Should we have a dedicated nullptr node in lexicon instead of
  // making the user to create it?
  CHECK(&first_nullptr->type() == &second_nullptr->type());

  const Name* name = lexicon.make_identifier("np");
  auto& type = lexicon.get_qualified(Type_qualifier::Const, lexicon.int_type());
  impl::Var* var = global_scope->make_var(*name, first_nullptr->type());

  std::stringstream ss;
  Printer pp{ss};
  // This will fail with infinite recursion.
  pp << unit;
  CHECK(!ss.str().empty());
}
