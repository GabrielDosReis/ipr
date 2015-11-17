//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis <gdr@cs.tamu.edu>
// 

#ifndef IPR_TRAVERSAL_INCLUDED
#define IPR_TRAVERSAL_INCLUDED

#include <ipr/interface>

namespace ipr {
   // Returns true if both operands share the same physical
   // storage.
   inline bool
   physically_same(const Node& lhs, const Node& rhs)
   {
      return &lhs == &rhs;
   }

   // This collection of routines implements structural equality
   // of Nodes.  They are, for example, useful in determining
   // when two (type-) expressions are same, from structural
   // point of view in context like dependent types.

   bool structurally_same(const Node&, const Node&);
   
   // -- utility functions --

   // Helper function, for implicit conversion Derived -> Base.
   // It lets view a node, from a more concrete node category (Derived),
   // as a member of more abstract node category (Base).
   template<class T, class U>
   inline const T& as(const U& u) { return u; }
   
   // This visitor class applies the same function to all major nodes.
   // A typical example of use is to throw an exception or do nothing.
   template<class F>
   struct Constant_visitor : Visitor, F {
      void visit(const Node& n) { (*this)(n); }
      void visit(const Expr& n) { (*this)(n); }
      void visit(const Type& n) { (*this)(n); }
      void visit(const Stmt& n) { (*this)(n); }
      void visit(const Decl& n) { (*this)(n); }
   };

   // This function object class implement "no-op" semantics.  Useful
   // with the above Visitor.
   struct No_op {
      void operator()(const Node&) const { }
   };

   // This function object type throw an exception indicating that a
   // Visitor::visit() is missing for a particular IPR node type.
   struct Missing_overrider {
      void operator()(const Node&) const;
   };

   namespace util {
      // >>>> Yuriy Solodkyy: 2006/07/21 
      // MSVC 7.1 has problem with taking address of n when this is a local class.
      // Therefore this class was moved from being a local class of subsequent
      // function to being just a regular class, which that function uses.
      template <class T>
      struct view_visitor : Constant_visitor<No_op> {
        const T* result;
        view_visitor() : result(0) { }
        
        void visit(const T& n) { result = &n; }
      };
      // <<<< Yuriy Solodkyy: 2006/07/21 

      // This helper function returns a pointer to its argument, if that
      // node is from the category indicated by the template parameter.
      // This is a cheap, specialized version of dynamic cast.
      template<class T>
      const T*
      view(const Node& n)
      {
         view_visitor<T> vis;
         n.accept(vis);
         return vis.result;
      }
   }
}

#endif // IPR_TRAVERSAL_INCLUDED
