#include "doctest/doctest.h"

#include <ipr/impl>

TEST_CASE("C++ Standard Conversions") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  INFO("static_cast<long long>(4);");
  // Integral Promotion            (Promotion)
  lexicon.make_promotion(
    *lexicon.make_literal(lexicon.int_type(), "4"),
    lexicon.long_long_type()
  );

  INFO("(float)2.2;");
  // Floating-Point Conversion     (Demotion)
  lexicon.make_demotion(
    *lexicon.make_literal(lexicon.double_type(), "2.2"),
    lexicon.float_type()
  );

  INFO("int* const ptr = 0;");
  // Pointer Conversion            (Coercion)
  const auto& ptr_type = lexicon.get_qualified(
    Type_qualifier::Const, lexicon.get_pointer(lexicon.int_type()));
  auto& ptr = *unit.global_region()->declare_var(*lexicon.make_identifier("ptr"), ptr_type);
  ptr.init = lexicon.make_coercion(
    *lexicon.make_literal(lexicon.int_type(), "0"),
    ptr_type,
    ptr_type
  );

  INFO("if (ptr) double(6);");
  // Boolean Conversion            (Coercion)
  // Lvalue-to-Rvalue              (Read)
  // Integral-Floating Conversion  (Coercion)
  const auto& condition = *lexicon.make_coercion(
    *lexicon.make_read(
      *lexicon.make_id_expr(ptr),
      lexicon.get_pointer(lexicon.int_type()) // != ptr.type() (see 7.2.2/2)
    ),
    lexicon.bool_type(),
    lexicon.bool_type()
  );
  const auto& then_expr = *lexicon.make_coercion(
      *lexicon.make_literal(lexicon.int_type(), "6"),
      lexicon.double_type(),
      lexicon.double_type()
  );
  lexicon.make_if(condition, *lexicon.make_expr_stmt(then_expr));

  // Bitcast                      (Pretend)
  INFO("(bool*)ptr");
  lexicon.make_pretend(
    *lexicon.make_id_expr(ptr),
    lexicon.get_pointer(lexicon.bool_type()),
    lexicon.get_pointer(lexicon.bool_type())
  );
}

TEST_CASE("Class Conversions") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  INFO("struct Base {};");
  INFO("struct Derived : Base {};");
  auto& base = *lexicon.make_class(*unit.global_region());
  auto& derived = *lexicon.make_class(*unit.global_region());
  derived.declare_base(base);

  INFO("Base b;");
  INFO("Derived d;");
  auto& base_ptr = lexicon.get_pointer(base);
  auto& derived_ptr = lexicon.get_pointer(derived);
  auto& b = *unit.global_region()->declare_var(*lexicon.make_identifier("b"), base_ptr);
  auto& d = *unit.global_region()->declare_var(*lexicon.make_identifier("d"), derived_ptr);

  // Derived-to-base conversion
  INFO("(Base*)b;");
  lexicon.make_narrow(
    *lexicon.make_id_expr(d),
    base,
    base_ptr
  );

  // Checked base-to-derived conversion
  INFO("dynamic_cast<Derived*>(b);");
  lexicon.make_widen(
    *lexicon.make_id_expr(b),
    derived,
    derived_ptr
  );

  // Unchecked base-to-derived conversion
  INFO("(Derived*)b;");
  lexicon.make_pretend(
    *lexicon.make_id_expr(b),
    derived_ptr,
    derived_ptr
  );
}

TEST_CASE("CV Conversions") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  // Standard C++ CV qualification
  INFO("(volatile int)7;");
  lexicon.make_qualification(
    *lexicon.make_literal(lexicon.double_type(), "2.2"),
    Type_qualifier::Volatile,
    lexicon.int_type() // prvalue will be adjusted to remove qualifiers (see 7.2.2/2)
  );

  INFO("const int* ptr;");
  const auto& ptr_type = lexicon.get_qualified(
    Type_qualifier::Const, lexicon.get_pointer(lexicon.int_type()));
  auto& ptr = *unit.global_region()->declare_var(*lexicon.make_identifier("ptr"), ptr_type);

  // Removal of const is non-standard C++ conversion
  // Keep original explicit Const_cast
  INFO("const_cast<int* const>(ptr);");
  lexicon.make_const_cast(
    lexicon.get_qualified(Type_qualifier::Const, lexicon.get_pointer(lexicon.int_type())),
    *lexicon.make_id_expr(ptr)
    // TODO: The constraint to this expression should be different to the target type
    // but the impl groups the two together. Should I change how casts are implemented??
    // &lexicon.get_pointer(lexicon.int_type()) // prvalue will be adjusted to remove qualifiers (see 7.2.2/2)
  );

  INFO("int** ptr_ptr");
  const auto& ptr_ptr_type = lexicon.get_pointer(lexicon.get_pointer(lexicon.int_type()));
  auto& ptr_ptr = *unit.global_region()->declare_var(
    *lexicon.make_identifier("ptr_ptr"), ptr_ptr_type);

  INFO("(const int** const)ptr_ptr");
  const auto& constraint = lexicon.get_pointer(lexicon.get_pointer(
    lexicon.get_qualified(Type_qualifier::Const, lexicon.int_type())));
  lexicon.make_qualification(
    // TODO: how do you handle nesting as mentioned in #117. This feels wrong to
    // only support qualification of the top level type.
    *lexicon.make_coercion(
      *lexicon.make_id_expr(ptr),
      constraint,
      constraint
    ),
    Type_qualifier::Const,
    constraint // prvalue will be adjusted to remove qualifiers (see 7.2.2/2)
  );
}
