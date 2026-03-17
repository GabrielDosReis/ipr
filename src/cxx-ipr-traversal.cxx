//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copyright and license notices.
//
// Module implementation unit for cxx.ipr.traversal.
// Contains out-of-line definitions: structurally_same template overloads
// for Unary<>, Binary<>, Ternary<>, and Missing_overrider::operator().

module;

#include <ipr/std-preamble>
#include <typeinfo>

module cxx.ipr.traversal;

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
             and structurally_same(lhs.second(), rhs.second()));
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
