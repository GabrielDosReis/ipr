//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copright and license notices.
//

#include <algorithm>
#include <string>
#include <typeinfo>
#include <ipr/traversal>

namespace ipr {

   template<class Cat, class Op>
   inline bool
   structurally_same(const Unary<Cat, Op>& lhs, const Unary<Cat, Op>& rhs)
   {
      return physically_same(lhs, rhs)
         or structurally_same(lhs.operand(), rhs.operand());
   }

   template<class Cat, class Op1, class Op2>
   inline bool
   structurally_same(const Binary<Cat, Op1, Op2>& lhs,
                     const Binary<Cat, Op1, Op2>& rhs)
   {
      return physically_same(lhs, rhs)
         or (structurally_same(lhs.first(), rhs.first())
             and structurally_same(lhs.first(), rhs.first()));
   }

   template<class Cat, class Op1, class Op2, class Op3>
   inline bool
   structurally_same(const Ternary<Cat, Op1, Op2, Op3>& lhs,
                     const Ternary<Cat, Op1, Op2, Op3>& rhs)
   {
      return physically_same(lhs, rhs)
         or (structurally_same(lhs.first(), rhs.first())
             and structurally_same(lhs.second(), rhs.second())
             and structurally_same(lhs.third(), rhs.third()));
   }
}



void
ipr::Missing_overrider::operator()(const ipr::Node& n) const
{
   throw std::logic_error(std::string("missing overrider for ")
                          + typeid(n).name());
}

// -- ipr::Visitor --
void
ipr::Visitor::visit(const Annotation&) { }

void
ipr::Visitor::visit(const Region& r)
{
    visit(as<Node>(r));
}

void
ipr::Visitor::visit(const Comment& c)
{
   visit(as<Node>(c));
}

void
ipr::Visitor::visit(const String& s)
{
   visit(as<Node>(s));
}

void
ipr::Visitor::visit(const Linkage& l)
{
   visit(as<Node>(l));
}

// Because Name is a very high-level interface to
// Identifier, Operator, Conversion, Instantiation and
// Qualified and these share common very high-level
// semantics, it is convenient to have the implementation
// of the corresponding Visitor::visit() functions forward to
// Visitor::visit(const Name&).  That way, code duplication
// can be substantially reduced.  The same goes for other
// sub-hierarchies.

void
ipr::Visitor::visit(const Classic& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Identifier& id)
{
   visit(as<Name>(id));
}

void
ipr::Visitor::visit(const Suffix& s)
{
   visit(as<Name>(s));
}

void
ipr::Visitor::visit(const Operator& op)
{
   visit(as<Name>(op));
}

void
ipr::Visitor::visit(const Conversion& conv)
{
   visit(as<Name>(conv));
}

void
ipr::Visitor::visit(const Template_id& e)
{
   visit(as<Name>(e));
}

void
ipr::Visitor::visit(const Type_id& n)
{
   visit(as<Name>(n));
}

void
ipr::Visitor::visit(const Ctor_name& n)
{
   visit(as<Name>(n));
}

void
ipr::Visitor::visit(const Dtor_name& n)
{
   visit(as<Name>(n));
}

void
ipr::Visitor::visit(const Guide_name& n)
{
   visit(as<Name>(n));
}

// -- Types visiting hooks --

void
ipr::Visitor::visit(const Array& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Class& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Closure& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Decltype& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Enum& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const As_type& t)
{
   visit(as<Type>(t));
}

void ipr::Visitor::visit(const Tor& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Function& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Namespace& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Pointer& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Product& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Ptr_to_member& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Qualified& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Reference& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Rvalue_reference& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Sum& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Forall& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Auto& t)
{
   visit(as<Type>(t));
}

void
ipr::Visitor::visit(const Union& t)
{
   visit(as<Type>(t));
}

// -- Expressions visiting hooks --

void
ipr::Visitor::visit(const Expr_list& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Overload& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Scope& e)
{
   visit(as<Expr>(e));
}

// empty-expressions as in array of unknown-bound.
void
ipr::Visitor::visit(const Phantom& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Eclipsis& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Lambda& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Symbol& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Address& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Array_delete& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Complement& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Delete& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Demotion& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Deref& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Enclosure& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Alignof& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Sizeof& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Args_cardinality& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Typeid& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Id_expr& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Label& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Unary_minus& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Materialization& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Not& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Post_decrement& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Post_increment& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Pre_decrement& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Pre_increment& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Promotion& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Read& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Throw& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Unary_plus& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Expansion& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Noexcept& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Rewrite& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Scope_ref& n)
{
   visit(as<Classic>(n));
}

void
ipr::Visitor::visit(const Plus& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Plus_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const And& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Array_ref& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Arrow& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Arrow_star& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitand& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitand_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitor& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitor_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitxor& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Bitxor_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Cast& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Call& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Coercion& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Comma& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Const_cast& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Div& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Div_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Dot& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Dot_star& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Dynamic_cast& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Equal& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Greater& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Greater_equal& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Less& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Less_equal& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Literal& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Member_init& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Modulo& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Modulo_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Mul& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Mul_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Narrow& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Not_equal& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Construction& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Or& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Pretend& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Qualification& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Reinterpret_cast& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Lshift& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Lshift_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Rshift& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Rshift_assign& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Static_cast& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Widen& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Minus& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Minus_assign& e)
{
   visit(as<Classic>(e));
}

void ipr::Visitor::visit(const Binary_fold& e)
{
   visit(as<Classic>(e));
}

void ipr::Visitor::visit(const Where& e)
{
   visit(as<Expr>(e));
}

void ipr::Visitor::visit(const Instantiation& e)
{
   visit(as<Expr>(e));
}

void
ipr::Visitor::visit(const Conditional& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const New& e)
{
   visit(as<Classic>(e));
}

void
ipr::Visitor::visit(const Mapping& s)
{
   visit(as<Expr>(s));
}

// -- Directives visiting hooks --

void
ipr::Visitor::visit(const Asm& d)
{
   visit(as<Directive>(d));
}

void ipr::Visitor::visit(const Specifiers_spread& d)
{
   visit(as<Directive>(d));
}

void
ipr::Visitor::visit(const Static_assert& d)
{
   visit(as<Directive>(d));
}

void ipr::Visitor::visit(const Structured_binding& d)
{
   visit(as<Directive>(d));
}

void ipr::Visitor::visit(const Using_declaration& d)
{
   visit(as<Directive>(d));
}

void
ipr::Visitor::visit(const Using_directive& d)
{
   visit(as<Directive>(d));
}

void ipr::Visitor::visit(const Pragma& d)
{
   visit(as<Directive>(d));
}

// -- Statements visiting hooks --

void
ipr::Visitor::visit(const Labeled_stmt& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Block& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Ctor_body& e)
{
   visit(as<Stmt>(e));
}

void
ipr::Visitor::visit(const Expr_stmt& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const If& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Switch& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const While& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Do& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const For& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const For_in& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Break& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Continue& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Goto& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Return& s)
{
   visit(as<Stmt>(s));
}

void
ipr::Visitor::visit(const Handler& s)
{
   visit(as<Stmt>(s));
}

// Forward Visitor::visit() that operates on nodes
// derived from Decl to whatever derived visitors do
// for general declarations.

void
ipr::Visitor::visit(const Alias& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Base_type& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Bitfield& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Enumerator& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Field& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Fundecl& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Parameter& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Parameter_list& l)
{
   visit(as<Node>(l));
}

void
ipr::Visitor::visit(const Typedecl& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Template& d)
{
   visit(as<Decl>(d));
}

void
ipr::Visitor::visit(const Var& d)
{
   visit(as<Decl>(d));
}

void ipr::Visitor::visit(const EH_parameter& d)
{
   visit(as<Decl>(d));
}

void
ipr::Translation_unit::Visitor::visit(const Module_unit& u)
{
   visit(as<Translation_unit>(u));
}

void
ipr::Translation_unit::Visitor::visit(const Interface_unit& u)
{
   visit(as<Translation_unit>(u));
}
