// -*- C++ -*-
//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copyright and license notices.
//
// Module interface: cxx.ipr.traversal
// Utility functions and visitor adapters for traversing the IPR node hierarchy.

module;

#include <ipr/std-preamble>

export module cxx.ipr.traversal;

import cxx.ipr;

namespace ipr {
   // This collection of routines implements structural equality
   // of Nodes.  They are, for example, useful in determining
   // when two (type-) expressions are same, from structural
   // point of view in context like dependent types.

   export bool structurally_same(const Node&, const Node&);

   // -- builtin types
   // This predicate holds for representation of built types: they are
   // the fix points of the As_type functor.
   export inline bool denote_builtin_type(const As_type& t)
   {
      return physically_same(t, t.expr());
   }

   // This visitor class applies the same function to all major nodes.
   // A typical example of use is to throw an exception or do nothing.
   export template<class F>
   struct Constant_visitor : Visitor, F {
      void visit(const Node& n) override { (*this)(n); }
      void visit(const Name& n) override { (*this)(n); }
      void visit(const Expr& n) override { (*this)(n); }
      void visit(const Type& n) override { (*this)(n); }
      void visit(const Directive& n) override { (*this)(n); }
      void visit(const Stmt& n) override { (*this)(n); }
      void visit(const Decl& n) override { (*this)(n); }
   };

   // This function object class implement "no-op" semantics.  Useful
   // with the above Visitor.
   export struct No_op {
      void operator()(const Node&) const { }
   };

   // This function object type throw an exception indicating that a
   // Visitor::visit() is missing for a particular IPR node type.
   export struct Missing_overrider {
      void operator()(const Node&) const;
   };

   namespace util {
      // This helper function returns a pointer to its argument, if that
      // node is from the category indicated by the template parameter.
      // This is a cheap, specialized version of dynamic cast.
      export template<class T>
      inline const T*
      view(const Node& n)
      {
         struct visitor : Constant_visitor<No_op> {
            const T* result = nullptr;
            void visit(const T& n) final { result = &n; }
         };

         visitor vis { };
         n.accept(vis);
         return vis.result;
      }
   }
}
