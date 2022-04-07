#include <doctest/doctest.h>

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
    Type_qualifiers::Const, lexicon.get_pointer(lexicon.int_type()));
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

  INFO("Base* b;");
  INFO("Derived* d;");
  auto& base_ptr = lexicon.get_pointer(base);
  auto& derived_ptr = lexicon.get_pointer(derived);
  auto& b = *unit.global_region()->declare_var(*lexicon.make_identifier("b"), base_ptr);
  auto& d = *unit.global_region()->declare_var(*lexicon.make_identifier("d"), derived_ptr);

  // Derived-to-base conversion
  INFO("b = d;");
  lexicon.make_assign(
    *lexicon.make_id_expr(b),
    *lexicon.make_widen(
      *lexicon.make_id_expr(d),
      base,
      base_ptr
  ));

  // Checked base-to-derived conversion
  INFO("dynamic_cast<Derived*>(b);");
  lexicon.make_narrow(
    *lexicon.make_id_expr(b),
    derived,
    derived_ptr
  );

  // Unchecked base-to-derived conversion has no abstract
  // representation in IPR so keep explicit cast.
  INFO("(Derived*)b;");
  lexicon.make_cast(
    derived_ptr,
    *lexicon.make_id_expr(b)
  );
}

TEST_CASE("CV Conversions") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  // Standard C++ CV qualification
  INFO("(int) -> (volatile int)");
  lexicon.make_qualification(
    *lexicon.make_literal(lexicon.int_type(), "7"),
    Type_qualifiers::Volatile,
    lexicon.int_type() // prvalue can be adjusted to remove qualifiers (see 7.2.2/2)
  );

  INFO("const int* ptr;");
  const auto& ptr_type = lexicon.get_qualified(
    Type_qualifiers::Const, lexicon.get_pointer(lexicon.int_type()));
  auto& ptr = *unit.global_region()->declare_var(*lexicon.make_identifier("ptr"), ptr_type);

  // Removal of const is a non-implicit conversion
  INFO("const_cast<int* const>(ptr);");
  lexicon.make_const_cast(
    lexicon.get_qualified(Type_qualifiers::Const, lexicon.get_pointer(lexicon.int_type())),
    *lexicon.make_id_expr(ptr)
  );

  INFO("int** ptr_ptr");
  const auto& ptr_ptr_type = lexicon.get_pointer(lexicon.get_pointer(lexicon.int_type()));
  auto& ptr_ptr = *unit.global_region()->declare_var(
    *lexicon.make_identifier("ptr_ptr"), ptr_ptr_type);

  INFO("(int**) -> (int* const* const)");
  lexicon.make_qualification(
    *lexicon.make_qualification(
      *lexicon.make_id_expr(ptr_ptr),
      Type_qualifiers::Const,
      lexicon.get_qualified(Type_qualifiers::Const, ptr_ptr.type())
    ),
    Type_qualifiers::Const,
    lexicon.get_pointer(lexicon.get_qualified(Type_qualifiers::Const,
      lexicon.get_pointer(lexicon.int_type())))
      // prvalue can be adjusted to remove qualifier on top-level (see 7.2.2/2)
  );

  INFO("const int var = 0;");
  auto& var = *unit.global_region()->declare_var(*lexicon.make_identifier("var"), 
    lexicon.get_qualified(Type_qualifiers::Const, lexicon.int_type()));

  // Pretend can be used to explicitly reperesent automatic type adjustment as detailed
  // in (7.2.2/2). Compilers are likely to just apply this adjustment on constraints without
  // explicitly providing this node.
  INFO("&var;");
  // Automatic-adjustment         (Pretend)
  lexicon.make_pretend(
    *lexicon.make_address(
      *lexicon.make_id_expr(var),
      &lexicon.get_qualified(Type_qualifiers::Const, lexicon.int_type())
    ),
    lexicon.int_type(),
    lexicon.int_type()
  );
}
