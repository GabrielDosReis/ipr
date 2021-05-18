//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copright and license notices.
//

#include <assert.h>
#include <ipr/io>
#include <ipr/traversal>
#include <ostream>
#include <cctype>
#include <typeinfo>
#include <stdexcept>
#include <iostream>

namespace ipr
{
   Printer& operator<<(Printer& p, Mapping_level x)
   {
      auto n = static_cast<std::size_t>(x);
      p.channel() << n;
      return p;
   }

   Printer& operator<<(Printer& p, Decl_position x)
   {
      auto n = static_cast<std::size_t>(x);
      p.channel() << n;
      return p;
   }

   struct pp_base : Constant_visitor<Missing_overrider> {
      explicit pp_base(Printer& p) : pp(p) { }
      using Visitor::visit;
      void visit(const Type& t) override { visit(as<Expr>(t)); }
      void visit(const Decl& d) override { visit(as<Expr>(d)); }

   protected:
      Printer& pp;
   };

   Printer::Printer(std::ostream& os)
         : stream(os), pad(None), emit_newline(false),
           pending_indentation(0) { }

   Printer&
   Printer::operator<<(const char* s)
   {
      this->stream << s;
      return *this;
   }

   void
   Printer::write(const char* begin, const char* last)
   {
      std::copy(begin, last, std::ostream_iterator<char>(this->stream));
   }

   template<typename T>
   struct Token_helper {
      T const value;
      Token_helper(T t) : value(t) { }
   };

   template<typename T>
   inline Token_helper<T>
   token(T t)
   {
      return Token_helper<T>(t);
   }

   template<typename T>
   inline Printer&
   operator<<(Printer& printer, Token_helper<T> t)
   {
      return printer << t.value << Printer::None;
   }

   inline Printer&
   insert_xtoken(Printer& printer, const char* s)
   {
      return printer << s << Printer::None;
   }

   struct needs_newline { };

   inline Printer&
   operator<<(Printer& printer, needs_newline)
   {
      printer.needs_newline(true);
      return printer;
   }

   struct newline { };

   Printer&
   operator<<(Printer& printer, newline)
   {
      printer << token('\n');
      const int n = printer.indent();
      for (int i = 0; i < n; ++i)
         printer << ' ';
      printer.needs_newline(false);
      return printer;
   }

   // -- An Expr_list is mostly a expression-seq.  See print_sequence().
   static inline Printer&
   operator<<(Printer& pp, const Expr_list& l)
   {
      const int n = l.size();
      for (int i = 0; i < n; ++i)
         {
            if (i != 0)
               pp << token(", ");
            pp << xpr_expr(l[i]);
         }
      return pp;
   }


   // -- Print out a sequence of type.
   static inline Printer&
   operator<<(Printer& pp, const Sequence<Type>& s)
   {
      const int n = s.size();
      for (int i = 0; i < n; ++i)
         {
            if (i != 0)
               pp << token(", ");
            pp << xpr_type(s[i]);
         }
      return pp;
   }


   // -- A Parameter_list is mostly a Parameter-seq.  See print_sequence().
   static inline Printer&
   operator<<(Printer& pp, const Parameter_list& l)
   {
      const int n = l.size();
      for (int i = 0; i < n; ++i)
         {
            if (i != 0)
               pp << token(", ");
            pp << xpr_decl(l.members()[i]);
         }
      return pp;
   }

   struct xpr_initializer {
      const ipr::Expr& expr;
      explicit xpr_initializer(const ipr::Expr& e) : expr(e) { }
   };

   static Printer& operator<<(Printer&, xpr_initializer);

   struct indentation {
      const int amount;
      indentation(int n) : amount(n) { }
   };

   inline Printer&
   operator<<(Printer& printer, indentation i)
   {
      printer.indent(i.amount);
      return printer;
   }

   struct newline_and_indent {
      const int indentation;
      newline_and_indent(int n = 0) : indentation(n) { }
   };

   inline Printer&
   operator<<(Printer& printer, newline_and_indent m)
   {
      printer << indentation(m.indentation) << newline();
      return printer;
   }

   struct xpr_primary_expr {
      const Expr& expr;
      explicit xpr_primary_expr(const Expr& e) : expr(e) { }
   };
   static Printer& operator<<(Printer&, xpr_primary_expr);

   struct xpr_cast_expr {
   const Expr& expr;
      xpr_cast_expr(const Expr& e) : expr(e) { }
   };
   static Printer& operator<<(Printer&, xpr_cast_expr);

   struct xpr_assignment_expression {
      const Expr& expr;
      xpr_assignment_expression(const Expr& e) : expr(e) { }
   };
   static Printer& operator<<(Printer&, xpr_assignment_expression);


   struct xpr_exception_spec {
      const Type& type;
      explicit xpr_exception_spec(const Type& t) : type(t) { }
   };
   static Printer& operator<<(Printer&, xpr_exception_spec);

   // Pretty-print identifiers.
   struct xpr_identifier {
      const char* const begin;
      const char* const last;

      explicit xpr_identifier(const ipr::String& s)
            : begin(s.begin()), last(s.end()) { }

      template<int N>
      explicit xpr_identifier(const char (&s)[N])
            : begin(s), last(s + N - 1) { }
   };

   static inline Printer&
   operator<<(Printer& printer, xpr_identifier id)
   {
      if (printer.padding() == Printer::Before)
         printer << ' ';
      printer.write(id.begin, id.last);

      return printer <<  Printer::Before;
   }

   Printer&
   operator<<(Printer& pp, const Identifier& id)
   {
      return pp << xpr_identifier(id.string());
   }

   // ------------------------------
   // -- Pretty printing of names --
   // ------------------------------

   // -------------------------------------------
   // -- Pretty-printing of classic expression --
   // -------------------------------------------

   // -- name:
   //       identifier
   //       operator-function-id
   //       conversion-function-id
   //       type-id
   //       scope-ref
   //       template-id
   //       ctor-name
   //       dtor-name
   //       template-parameter-canonical-name

   namespace xpr {
      struct Name : pp_base {
         explicit Name(Printer& p) : pp_base(p) { }

         void visit(const Identifier& id) final
         {
            pp << id;
         }

         // -- operator-function-id:
         //         operator operator-name
         //
         //    operator-name: one of
         //          +  ++  -=  -  --  -=  =  ==  !  !=  %   %=
         //          *  *=  /  /=  ^  ^=  &  &&  &=  |  ||  |=
         //          ~   ,  ()  []  <   <<  <<=   <=  >   >>
         //          >>=  >=  new  new[]   delete   delete[]
         void visit(const Operator& o) final
         {
            pp << xpr_identifier("operator");

            const ipr::String& s = o.opname();
            if (!std::isalpha(*s.begin()))
               {
                  pp.write(s.begin(), s.end());
                  pp << Printer::None;
               }
            else
               pp << xpr_identifier(s);
         }

         // -- conversion-function-id:
         //        operator  type-id
         // -- NOTE: This production is very different from ISO Standard
         // --        production for conversion-function-id.
         void visit(const Conversion& c) final
         {
            // For now only regular cast, later we'll add support for overloading
            // dynamic_cast, reinterpret_cast, const_cast and static_cast
            pp << xpr_identifier("operator")
               << xpr_identifier("cast")
               << token("<|") << xpr_type(c.target()) << token("|>");
         }

         // Suffix is a user defined literal, as in operator""_km.
         // We display it as operator "_km".
         void visit(const Suffix& s) final
         {
             pp << xpr_identifier("operator")
                << token('"')
                << s.name()
                << token('"');
         }

         // A type-id is just the spelling of the type expression.
         void visit(const Type_id& n) final
         {
            pp << xpr_type(n.type_expr());
         }

         // -- A Scope_ref corresponds to Standard C++ notion of
         // -- qualified-id.  Here, it takes the production of
         //    scope-ref:
         //       @ name ( identifier )
         void visit(const Scope_ref& n) final
         {
            pp << xpr_expr(n.scope()) << token("::") << xpr_expr(n.member());
         }

         // -- template-id:
         //       primary-expression < expression-seq >
         void visit(const Template_id& n) final
         {
            n.template_name().accept(*this);
            pp << token("<|") << n.args() << token("|>");
         }

         // -- ctor-name:
         //        # ctor
         void visit(const Ctor_name&) final
         {
            pp << xpr_identifier("#ctor");
         }

         // -- dtor-name
         //    # dtor
         void visit(const Dtor_name&) final
         {
            pp << xpr_identifier("#dtor");
         }

         // -- parameter-canonical-name
         //    #(level, position)
         // The (template) parameter is indicated in a generalized
         // de Bruijn nottation, where "level" is the level of
         // template-parameter list where the parameter was bound
         // (starting with 0 for the outermost level, and increasing
         // by one as the levels nest), and "position" is the position
         // of the parameter in the parameter list where it was bound
         // (starting 0 for the first parameter).
         void visit(const Rname& rn) final
         {
            pp << xpr_identifier("#(")
               << rn.level()
               << token(", ")
               << rn.position()
               << token(')');
         }
      };
   }

   struct xpr_name {
      const Name& name;
      explicit xpr_name(const Name& n) : name(n) { }
      explicit xpr_name(const Decl& d) : name(d.name()) { }
   };

   static inline Printer&
   operator<<(Printer& printer, xpr_name x)
   {
      xpr::Name pp { printer };
      x.name.accept(pp);
      return printer;
   }

   // -- primary-expression:
   //      name
   //      label
   //      type
   //      ( expression )
   //      { expression-seq }

   namespace xpr {
      struct Primary_expr : xpr::Name {
         Primary_expr(Printer& pp) : xpr::Name(pp) { }

         void visit(const Label& l) final { xpr::Name::visit(l.name()); }
         void visit(const Id_expr& id) final { pp << xpr_name{ id.name() }; }
         void visit(const Literal&) final;
         void visit(const Symbol& s) final { pp << xpr_name{ s.name() }; }
         void visit(const As_type& t) final
         {
            if (denote_builtin_type(t))
               pp << xpr_name(t.name());
            else
               pp << xpr_primary_expr(t.expr());
         }
         void visit(const Phantom&) final { } // nothing to print
         void visit(const Enclosure& e) final
         {
            static constexpr const char* syntax[] = { "\0\0", "()", "{}", "[]", "<>" };
            const auto delimiters = syntax[static_cast<int>(e.delimiters())];
            pp << token(delimiters[0]) << xpr_expr(e.expr()) << token(delimiters[1]);
         }
         void visit(const Expr& e) override
         {
            pp << token('(') << xpr_expr(e) << token(')');
         }
         void visit(const Decl& d) override { d.name().accept(*this); }
      };

      void
      Primary_expr::visit(const ipr::Literal& l)
      {
         const String& s = l.string();
         String::iterator cur = s.begin();
         String::iterator end = s.end();
         for (; cur != end; ++cur)
            switch (*cur)
               {
               default:
                  pp << *cur;
                  break;

               case '\n':
                  pp << "\\n";
                  break;

               case '\r':
                  pp << "\\r";
                  break;

               case '\f':
                  pp << "\\f";
                  break;

               case '\t':
                  pp << "\\t";
                  break;

               case '\v':
                  pp << "\\v";
                  break;

               case '\b':
                  pp << "\\b";
                  break;

               case '\a':
                  pp << "\\a";
                  break;

               case '\\':
                  pp << "\\\\";
                  break;

//               case '\'':
//                  pp << "\\'";
//                  break;

               case '\0':
                  pp << "\\0";
                  break;

               case '\1':
               case '\2':
               case '\3':
                  pp << "\\0" << std::oct << int(*cur);
                  break;
               }
      }
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_primary_expr x)
   {

      xpr::Primary_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   struct xpr_postfix_expr {
      const Expr& expr;
      explicit xpr_postfix_expr(const Expr& e) : expr(e) { }
   };

   static Printer& operator<<(Printer&, xpr_postfix_expr);

   template<Category_code code, int N>
   void
   new_style_cast(Printer& pp, const Cast_expr<code>& e, const char (&op)[N])
   {
      pp << xpr_identifier(op)
         << token("<|") << xpr_type(e.type()) << token("|>")
         << token('(') << xpr_expr(e.expr()) << token(')');
   }

   namespace xpr {
      // -- postfix-expression:
      //        primary-expression
      struct Postfix_expr : xpr::Primary_expr {
         Postfix_expr(Printer& pp) : xpr::Primary_expr(pp) { }

         //        postfix-expression [ expression ]
         void visit(const Array_ref& e) final
         {
            pp << xpr_postfix_expr(e.base())
               << token('[') << xpr_expr(e.member()) << token(']');
         }

         //        postfix-expression . primary-expression
         void visit(const Dot& e) final
         {
            pp << xpr_postfix_expr(e.base()) << token('.')
               << xpr_primary_expr(e.member());
         }

         //        postfix-expression -> primary-expression
         void visit(const Arrow& e) final
         {
            pp << xpr_postfix_expr(e.base()) << token("->")
               << xpr_primary_expr(e.member());
         }

         //        postfix-expression ( expression-list )
         void visit(const Call& e) final
         {
            pp << xpr_postfix_expr(e.function())
               << token('(') << e.args() << token(')');
         }

         void visit(const Construction& e) final
         {
            pp << xpr_type(e.type())
               << xpr_primary_expr(e.arguments());
         }

         //        postfix-expression --
         void visit(const Post_decrement& e) final
         {
            pp << xpr_postfix_expr(e.operand()) << token("--");
         }

         //        postfix-expression ++
         void visit(const Post_increment& e) final
         {
            pp << xpr_postfix_expr(e.operand()) << token("++");
         }

         //        dynamic_cast < type > ( expression )
         void visit(const Dynamic_cast& e) final
         {
            new_style_cast(pp, e, "dynamic_cast");
         }

         //        static_cast < type > ( expression )
         void visit(const Static_cast& e) final
         {
            new_style_cast(pp, e, "static_cast");
         }

         //        const_cast < type > ( expression )
         void visit(const Const_cast& e) final
         {
            new_style_cast(pp, e, "const_cast");
         }

         //        reinterpret_cast < type > ( expression )
         void visit(const Reinterpret_cast& e) final
         {
            new_style_cast(pp, e, "reinterpret_cast");
         }

         //        typeid ( expression )
         void visit(const Typeid& e) final
         {
            pp << xpr_identifier("typeid")
               << token('(') << xpr_expr(e.operand()) << token(')');
         }

         //       noexcept '(' expression ')'
         void visit(const Noexcept& e) final
         {
            pp << xpr_identifier("noexcept")
               << token('(') << xpr_expr(e.operand()) << token(')');
         }
      };
   }


   static inline Printer&
   operator<<(Printer& printer, xpr_postfix_expr x)
   {
      xpr::Postfix_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- unary-expression:
   //         -- cast-expression
   //         ++ cast-expression
   //         & cast-expression
   //         ~ cast-expression

   template<class T>
   inline void
   unary_operation(Printer& pp, const Unary<T>& e, const char* op)
   {
      pp << token(op) << xpr_cast_expr(e.operand());
   }

   namespace xpr {
      struct Unary_expr : xpr::Postfix_expr {
         Unary_expr(Printer& pp) : xpr::Postfix_expr(pp) { }

         void visit(const Pre_decrement& e) final { unary_operation(pp, e, "--"); }

         void visit(const Pre_increment& e) final { unary_operation(pp, e, "++"); }

         void visit(const Address& e) final { unary_operation(pp, e, "&"); }

         void visit(const Complement& e) final { unary_operation(pp, e, "~"); }

         void visit(const Deref& e) final { unary_operation(pp, e, "*"); }

         void visit(const Unary_minus& e) final { unary_operation(pp, e, "-"); }

         void visit(const Not& e) final { unary_operation(pp, e, "!"); }

         void visit(const Sizeof& e) final
         {
            pp << xpr_identifier("sizeof")
               << token(' ') << xpr_expr(e.operand());
         }

         void visit(const Args_cardinality& e) final
         {
            pp << xpr_identifier("sizeof") << token("...")
               << token('(') << xpr_expr(e.operand()) << token(')');
         }

         void visit(const Unary_plus& e) final
         {
            pp << token('+') << xpr_expr(e.operand());
         }

         void visit(const New& e) final
         {
            pp << xpr_identifier("new") << token(' ');
            if (auto p = e.placement())
               pp << token('(') << p.get() << token(") ");
            // Note: The following does not exactly conform to the ISO C++ grammar (because of ambiguity).
            pp << xpr_expr(e.initializer());
         }

         void visit(const Delete& e) final
         {
            pp << xpr_identifier("delete")
               << token(' ')
               << xpr_cast_expr(e.storage());
         }

         void visit(const Array_delete& e) final
         {
            pp << xpr_identifier("delete[]")
               << token(' ')
               << xpr_cast_expr(e.storage());
         }
      };
   }

   static Printer& operator<<(Printer&, xpr_cast_expr);

   namespace xpr {
      struct Cast_expr : xpr::Unary_expr {
         Cast_expr(Printer& p) : xpr::Unary_expr(p) { }

         // -- cast-expression
         //       unary-expression
         //       "(" type ")" cast-expression
         void visit(const Cast& e) final
         {
            new_style_cast(pp, e, "cast");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_cast_expr x)
   {
      xpr::Cast_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- pm-expression:
   //      cast-expression
   //      pm-expression ".*" cast-expression
   //      pm-expression "->*" cast-expression

   struct xpr_pm_expr {
      const Expr& expr;
      explicit xpr_pm_expr(const Expr& e) : expr(e) { }
   };

   static Printer& operator<<(Printer&, xpr_pm_expr);

   template<Category_code code>
   static void
   offset_with_pm(Printer& pp, const Member_selection<code>& e, const char* op)
   {
      pp << xpr_pm_expr(e.base())
         << op
         << xpr_cast_expr(e.member());
   }

   namespace xpr {
      struct Pm_expr : xpr::Cast_expr {
         Pm_expr(Printer& p) : xpr::Cast_expr(p) { }

         void visit(const Dot_star& e) final { offset_with_pm(pp, e, ".*"); }
         void visit(const Arrow_star& e) final { offset_with_pm(pp, e, "->*"); }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_pm_expr x)
   {
      xpr::Pm_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- Print a binary expression.  Instantiated by the relevant
   // -- grammar production, with precedence built-in.
   template<class Left, class Right, class E, class Op>
   inline void
   binary_expression(Printer& pp, const E& e, Op op)
   {
      pp << Left(e.first())
         << token(' ') << op << token(' ')
         << Right(e.second());
   }

   // -- multiplicative-expression:
   //          pm-expression
   //          multiplicative-expression * pm-expression
   //          multiplicative-expression / pm-expression
   //          multiplicative-expression % pm-expression

   struct xpr_mul_expr {
      const Expr& expr;
      explicit xpr_mul_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Mul_expr : xpr::Pm_expr {
         Mul_expr(Printer& p) : xpr::Pm_expr(p) { }

         void visit(const Mul& e) final
         {
            binary_expression<xpr_mul_expr, xpr_pm_expr>(pp, e, '*');
         }
         void visit(const Div& e) final
         {
            binary_expression<xpr_mul_expr, xpr_pm_expr>(pp, e, '/');
         }
         void visit(const Modulo& e) final
         {
            binary_expression<xpr_mul_expr, xpr_pm_expr>(pp, e, '%');
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_mul_expr x)
   {
      xpr::Mul_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- additive-expression:
   //         multiplicative-expression
   //         additive-expression + multiplicative-expression
   //         additive-expression - multiplicative-expression

   struct xpr_add_expr {
      const Expr& expr;
      explicit xpr_add_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Add_expr : xpr::Mul_expr {
         Add_expr(Printer& p) : xpr::Mul_expr(p) { }

         void visit(const Plus& e) final
         {
            binary_expression<xpr_add_expr, xpr_mul_expr>(pp, e, '+');
         }

         void visit(const Minus& e) final
         {
            binary_expression<xpr_add_expr, xpr_mul_expr>(pp, e, '-');
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_add_expr x)
   {
      xpr::Add_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- shift-expression:
   //         additive-expression
   //         shift-expression << additive-expression
   //         shift-expression >> additive-expression

   struct xpr_shift_expr {
      const Expr& expr;
      explicit xpr_shift_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Shift_expr : xpr::Add_expr {
         Shift_expr(Printer& p) : xpr::Add_expr(p) { }

         void visit(const Lshift& e) final
         {
            binary_expression<xpr_shift_expr, xpr_add_expr>(pp, e, "<<");
         }

         void visit(const Rshift& e) final
         {
            binary_expression<xpr_shift_expr, xpr_add_expr>(pp, e, ">>");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_shift_expr x)
   {
      xpr::Shift_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- relational-expression
   //        shift-expression
   //        relational-expression < shift-expression
   //        relational-expression > shift-expression
   //        relational-expression <= shift-expression
   //        relational-expression >= shift-expression

   struct xpr_rel_expr {
      const Expr& expr;
      explicit xpr_rel_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Rel_expr : xpr::Shift_expr {
         Rel_expr(Printer& p) : xpr::Shift_expr(p) { }

         void visit(const Less& e) final
         {
            binary_expression<xpr_rel_expr, xpr_shift_expr>(pp, e, '<');
         }

         void visit(const Less_equal& e) final
         {
            binary_expression<xpr_rel_expr, xpr_shift_expr>(pp, e, "<=");
         }

         void visit(const Greater& e) final
         {
            binary_expression<xpr_rel_expr, xpr_shift_expr>(pp, e, '>');
         }

         void visit(const Greater_equal& e) final
         {
            binary_expression<xpr_rel_expr, xpr_shift_expr>(pp, e, ">=");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_rel_expr x)
   {
      xpr::Rel_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- equality-expression
   //          relational-expression
   //          equality-expression == relational-expression
   //          equality-expression != relational-expression

   struct xpr_eq_expr {
      const Expr& expr;
      explicit xpr_eq_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Eq_expr : xpr::Rel_expr {
         Eq_expr(Printer& p) : xpr::Rel_expr(p) { }

         void visit(const Equal& e) final
         {
            binary_expression<xpr_eq_expr, xpr_rel_expr>(pp, e, "==");
         }

         void visit(const Not_equal& e) final
         {
            binary_expression<xpr_eq_expr, xpr_rel_expr>(pp, e, "!=");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_eq_expr x)
   {
      xpr::Eq_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- and-expression
   //          equality-expression
   //          and-expression & equality-expression

   struct xpr_and_expr {
      const Expr& expr;
      explicit xpr_and_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct And_expr : xpr::Eq_expr {
         And_expr(Printer& p) : xpr::Eq_expr(p) { }

         void visit(const Bitand& e) final
         {
            binary_expression<xpr_and_expr, xpr_eq_expr>(pp, e, '&');
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_and_expr x)
   {
      xpr::And_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // -- exclusive-or-expression
   //          and-expression
   //          exclusive-or-expression ^ and-expression

   struct xpr_xor_expr {
      const Expr& expr;
      explicit xpr_xor_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Xor_expr : xpr::And_expr {
         Xor_expr(Printer& p) : xpr::And_expr(p) { }

         void visit(const Bitxor& e) final
         {
            binary_expression<xpr_xor_expr, xpr_and_expr>(pp, e, '^');
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_xor_expr x)
   {
      xpr::Xor_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- inclusive-or-expression
   //         exclusive-or-exclusive
   //         inclusive-or-expression | exclusive-or-expression

   struct xpr_ior_expr {
      const Expr& expr;
      explicit xpr_ior_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Ior_expr : xpr::Xor_expr {
         Ior_expr(Printer& p) : xpr::Xor_expr(p) { }

         void visit(const Bitor& e) final
         {
            binary_expression<xpr_ior_expr, xpr_xor_expr>(pp, e, '|');
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_ior_expr x)
   {
      xpr::Ior_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- logical-and-expression
   //           inclusive-or-expression
   //           logical-and-expression && inclusive-or-expression

   struct xpr_land_expr {
      const Expr& expr;
      explicit xpr_land_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Land_expr : xpr::Ior_expr {
         Land_expr(Printer& p) : xpr::Ior_expr(p) { }

         void visit(const And& e) final
         {
            binary_expression<xpr_land_expr, xpr_ior_expr>(pp, e, "&&");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_land_expr x)
   {
      xpr::Land_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- logical-or-expression
   //         logical-and-expression
   //         logical-or-expression || logical-and-expression

   struct xpr_lor_expr {
      const Expr& expr;
      explicit xpr_lor_expr(const Expr& e) : expr(e) { }
   };

   namespace xpr {
      struct Lor_expr : xpr::Land_expr {
         Lor_expr(Printer& p) : xpr::Land_expr(p) { }

         void visit(const Or& e) final
         {
            binary_expression<xpr_lor_expr, xpr_land_expr>(pp, e, "||");
         }
      };
   }

   static inline Printer&
   operator<<(Printer& printer, xpr_lor_expr x)
   {
      xpr::Lor_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }


   // -- conditional-expression
   //          logical-or-expression
   //          logical-or-expression ? expression : assignment-expression

   namespace xpr {
      struct Cond_expr : xpr::Lor_expr {
         Cond_expr(Printer& p) : xpr::Lor_expr(p) { }

         void visit(const Conditional& e) final
         {
            pp << xpr_lor_expr(e.condition())
               << token(" ? ")
               << xpr_expr(e.then_expr())
               << token(" : ")
               << xpr_assignment_expression(e.else_expr());
         }
      };
   }

   struct xpr_mapping_expression {
      const Mapping& mapping;
      explicit xpr_mapping_expression(const Mapping& m) : mapping(m) { }
   };

   // >>>> Yuriy Solodkyy: 2006/05/31
   // MSVC 7.1 has problem with calling same function from its local class.
   // Therefore this class was moved from being a local class of subsequent
   // function to being just a regular class, which that function uses.
   struct xpr_mapping_expression_visitor : pp_base {
      const ipr::Mapping& map;
      xpr_mapping_expression_visitor(Printer& p, const ipr::Mapping& m) : pp_base(p), map(m) { }

      void visit(const Function& t) final
      {
         pp << token('(');
         pp << map.params();
         pp << token(')');
         pp << xpr_exception_spec(t.throws());

         pp << xpr_initializer(map.result());
      }

      void visit(const Forall&) final
      {
         pp << token('<');
         pp << map.params();
         pp << token('>');
         pp << xpr_initializer(map.result());
      }
   };
   // <<<< Yuriy Solodkyy: 2006/05/31

   Printer&
   operator<<(Printer& pp, xpr_mapping_expression e)
   {
      xpr_mapping_expression_visitor impl(pp, e.mapping);
      e.mapping.type().accept(impl);
      return pp;
   }

   // -- assignment-expression:
   //         conditional-expression
   //         logical-or-expression assignment-operator assignment-expression
   //         throw expression
   //
   //    assignment-operator: one of
   //         =   *=  /=  %=  +=  -=  >>=  <<=  &=  ^=  |=

   namespace xpr {
      struct Assignment_expr : xpr::Cond_expr {
         Assignment_expr(Printer& p) : xpr::Cond_expr(p) { }

         void visit(const Assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, '=');
         }
         void visit(const Plus_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "+=");
         }
         void visit(const Bitand_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "&=");
         }
         void visit(const Bitor_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "|=");
         }
         void visit(const Bitxor_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "^=");
         }
         void visit(const Div_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "/=");
         }
         void visit(const Modulo_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "%=");
         }
         void visit(const Mul_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "*=");
         }
         void visit(const Lshift_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "<<=");
         }
         void visit(const Rshift_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, ">>=");
         }
         void visit(const Minus_assign& e) final
         {
            binary_expression<xpr_lor_expr, xpr_assignment_expression>
               (pp, e, "-=");
         }
         void visit(const Throw& e) final
         {
            pp << xpr_identifier("throw") << token(' ')
               << xpr_assignment_expression(e.operand());
         }

         void visit(const Mapping& m) final
         {
            pp << xpr_mapping_expression (m);
         }
      };
   }

   Printer&
   operator<<(Printer& printer, xpr_assignment_expression x)
   {
      xpr::Assignment_expr pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   // >>>> Yuriy Solodkyy: 2006/05/31
   // MSVC 7.1 has problem with calling same function from its local class.
   // Therefore this class was moved from being a local class of subsequent
   // function to being just a regular class, which that function uses.
   struct xpr_expr_visitor : pp_base {
      xpr_expr_visitor(Printer& p) : pp_base(p) { }

      void visit(const Comma& e) final
      {
         pp << xpr_expr(e.first()) << token("@, ")
            << xpr_assignment_expression(e.second());
      }

      void visit(const Scope& s) final
      {
         const Sequence<Decl>& decls = s.members();
         const int n = decls.size();
         for (int i = 0; i < n; ++i)
            {
               pp << xpr_decl(decls[i], true)
                  << newline();
            }
      }

      void visit(const Expr_list& e) final { pp << e; }

      void visit(const Member_init& e) final
      {
         pp << xpr_expr(e.member())
            << token('(') << xpr_expr(e.initializer()) << token(')');
      }

      void visit(const Type& t) final { pp << xpr_type(t); }
      void visit(const Expr& e) final { pp << xpr_assignment_expression(e); }
      void visit(const Stmt& s) final { pp << xpr_stmt(s); }
      void visit(const Decl& d) final
      {
         // A declaration used as an expression must have appeared
         // as a primary-expression.
         pp << xpr_primary_expr(d);
      }
   };
   // <<<< Yuriy Solodkyy: 2006/05/31

   Printer&
   operator<<(Printer& printer, xpr_expr x)
   {
      xpr_expr_visitor impl(printer);
      x.expr.accept(impl);
      return printer;
   }

   //  -- Types --

   static Printer&
   operator<<(Printer& printer, xpr_exception_spec x)
   {
      return  printer << token(' ') << xpr_identifier("throw")
                      << token('(') << xpr_type(x.type) << token(')');
   }

   struct xpr_base_classes {
      const ipr::Sequence<ipr::Base_type>& bases;
      explicit xpr_base_classes(const ipr::Sequence<ipr::Base_type>& b)
            : bases(b) { }
   };

   static Printer&
   operator<<(Printer& pp, xpr_base_classes x)
   {
      const Sequence<Base_type>& bases = x.bases;
      const int n = bases.size();
      if (n > 0)
         {
            pp << token('(');
            for (int i = 0; i < n; ++i)
               {
                  if (i != 0)
                     pp << token(", ");
                  pp << xpr_decl(bases[i]);
               }
            pp << token(')');
         }

      return pp;
   }

   Printer&
   operator<<(Printer& printer, Type_qualifier cv)
   {
      if (implies(cv, Type_qualifier::Const))
         printer << xpr_identifier("const");
      if (implies(cv, Type_qualifier::Volatile))
         printer << xpr_identifier("volatile");
      if (implies(cv, Type_qualifier::Restrict))
         printer << xpr_identifier("restrict");

      return printer;
   }

   struct xpr_type_expr {
      const Expr& type;
      explicit xpr_type_expr(const Expr& t) : type(t) { }
   };

   template<typename T>
   static Printer& operator<<(Printer& pp, const ipr::Udt<T>& t)
   {
      return pp << token(' ')
            << token('{')
            << newline_and_indent(3)
            << xpr_expr(t.scope())
            << newline_and_indent(-3)
            << token('}')
            << needs_newline();
   }

   // >>>> Yuriy Solodkyy: 2006/05/31
   // MSVC 7.1 has problem with calling same function from its local class.
   // Therefore this class was moved from being a local class of subsequent
   // function to being just a regular class, which that function uses.
   static Printer&
   operator<<(Printer& printer, xpr_type_expr x);

   struct xpr_type_expr_visitor : pp_base {
      xpr_type_expr_visitor(Printer& p) : pp_base(p) { }

      void visit(const Array& a) final
      {
         pp << token('[') << xpr_expr(a.bound()) << token(']')
            << xpr_type(a.element_type());
      }

      void visit(const As_type& t) final
      {
         pp << xpr_expr(t.expr());
      }

      void visit(const Class& c) final
      {
         pp << xpr_base_classes(c.bases())
            << token(' ')
            << token('{')
            << newline_and_indent(3)
            << xpr_expr(c.scope())
            << newline_and_indent(-3)
            << token('}')
            << needs_newline();
      }

      void visit(const Decltype& t) final
      {
         pp << xpr_identifier("decltype") << token(' ')
            << token('(') << xpr_expr(t.expr()) << token(')');
      }

      void visit(const Function& f) final
      {
         pp << token('(') << f.source().operand() << token(')')
            << xpr_exception_spec(f.throws())
            << xpr_type(f.target());
      }

      void visit(const Pointer& t) final
      {
         pp << token('*') << xpr_type(t.points_to());
      }

      void visit(const Ptr_to_member& t) final
      {
         pp << token('*')
            << token('[')
            << xpr_type(t.containing_type())
            << token(']') << token(',')
            << xpr_type(t.member_type());
      }

      void visit(const Qualified& t) final
      {
         pp << t.qualifiers()
            << xpr_type(t.main_variant());
      }

      void visit(const Reference& t) final
      {
         pp << token('&') << xpr_type(t.refers_to());
      }

      void visit(const Rvalue_reference& t) final
      {
         pp << token('&') << token('&') << xpr_type(t.refers_to());
      }

      void visit(const Forall& t) final
      {
         pp << token('<') << t.source().operand() << token('>')
            << token(' ')
            << xpr_type_expr(t.target());
      }

      void visit(const Union& t) final { pp << t; }
      void visit(const Enum& t) final { pp << t; }
      void visit(const Namespace& t) final { pp << t; }
   };
   // <<<< Yuriy Solodkyy: 2006/05/31

   static Printer&
   operator<<(Printer& printer, xpr_type_expr x)
   {
      xpr_type_expr_visitor impl(printer);
      x.type.accept(impl);
      return printer;
   }

   // >>>> Yuriy Solodkyy: 2006/05/31
   // MSVC 7.1 has problem with calling same function from its local class.
   // Therefore this class was moved from being a local class of subsequent
   // function to being just a regular class, which that function uses.
   struct xpr_type_visitor : pp_base {
      xpr_type_visitor(Printer& p) : pp_base(p) { }

      void visit(const As_type& t) final
      {
         if (denote_builtin_type(t))
            pp << xpr_name(t.name());
         else
            pp << xpr_expr(t.expr());
      }

      void visit(const Array& a) final
      { pp << xpr_type_expr(a); }

      void visit(const Function& f) final
      { pp << xpr_type_expr(f); }

      void visit(const Pointer& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Ptr_to_member& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Qualified& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Reference& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Rvalue_reference& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Forall& t) final
      { pp << xpr_type_expr(t); }

      void visit(const Type& t) final
      {
         // FIXME: Check.
         pp << xpr_name(t.name());
      }

      void visit(const Product& t) final
      {
         pp << t.operand();
      }

      void visit(const Sum& t) final
      {
         pp << t.operand();
      }
   };
   // <<<< Yuriy Solodkyy: 2006/05/31

   Printer&
   operator<<(Printer& printer, xpr_type x)
   {
      xpr_type_visitor impl(printer);
      x.type.accept(impl);
      return printer;
   }

   // Initializer expressions.

   static Printer&
   operator<<(Printer& printer, xpr_initializer x)
   {
      struct V : xpr::Assignment_expr {
         V(Printer& p) : xpr::Assignment_expr(p) { }

         void visit(const ipr::Type& t) final
         {
            pp << xpr_type_expr(t);
         }

         void visit(const ipr::Expr& e) final
         {
            pp << xpr_expr(e);
         }

         void visit(const ipr::Stmt& e) final
         {
            pp << xpr_stmt(e);
         }

         void visit(const ipr::Decl& e) final
         {
            pp << xpr_decl(e);
         }
      };
      V pp(printer);
      x.expr.accept(pp);
      return printer;
   }

   //  -- Statements --
   namespace xpr {
      struct Stmt : xpr::Assignment_expr {
         Stmt(Printer& p) : xpr::Assignment_expr(p) { }

         void visit(const Expr_stmt& e) final
         {
            pp << xpr_expr(e.expr())
               << token(';')
               << needs_newline();
         }

         void visit(const Labeled_stmt& s) final
         {
            if (pp.needs_newline())
               pp << newline_and_indent(-3);
            else
               pp << indentation(-3);

            pp << xpr_identifier("label")
               << token(' ')
               << xpr_expr(s.label())
               << token(':')
               << indentation(3) << needs_newline()
               << xpr_stmt(s.stmt())
               << needs_newline();
         }

         void visit(const Block& s) final
         {
            pp << token('{')
               << needs_newline() << indentation(3);
            const Sequence<ipr::Stmt>& body = s.body();
            int n = body.size();
            for (int i = 0; i < n; ++i)
               pp << xpr_stmt(body[i])
                  << needs_newline();
            pp << newline_and_indent(-3)
               << token('}')
               << needs_newline();

            const Sequence<Handler>& handlers = s.handlers();
            n = handlers.size();
            for (int i = 0; i < n; ++i)
               pp << xpr_stmt(handlers[i],false);
         }

         void visit(const Ctor_body& b) final
         {
            const Expr_list& inits = b.inits();
            if (inits.size() > 0)
               pp << token(" : ")
                  << inits << needs_newline();

            pp << needs_newline() << xpr_stmt(b.block());
         }

         void visit(const If& s) final
         {
            pp << xpr_identifier("if")
               << token(' ')
               << token('(') << xpr_expr(s.condition()) << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.consequence());
            if (auto alt = s.alternative()) {
               pp << newline_and_indent(-3)
                  << xpr_identifier("else")
                  << newline_and_indent(3)
                  << xpr_stmt(alt.get());
            }
            pp << indentation(-3) << needs_newline();
         }

         void visit(const Return& s) final
         {
            pp << xpr_identifier("return")
               << token(' ')
               << xpr_expr(s.value())
               << token(';')
               << needs_newline();
         }

         void visit(const Switch& s) final
         {
            pp << xpr_identifier("switch")
               << token(' ')
               << token('(') << xpr_expr(s.condition()) << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << newline_and_indent(-3);
         }

         void visit(const While& s) final
         {
            pp << xpr_identifier("while")
               << token(' ')
               << token('(') << xpr_expr(s.condition()) << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << needs_newline() << indentation(-3);
         }

         void visit(const Do& s) final
         {
            pp << xpr_identifier("do")
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << newline_and_indent(-3)
               << xpr_identifier("while")
               << token(' ')
               << token('(') << xpr_expr(s.condition()) << token(')')
               << token(';') << needs_newline();
         }

         void visit(const For& s) final
         {
            pp << xpr_identifier("for")
               << token(" (")
               << xpr_expr(s.initializer())
               << token("; ")
               << xpr_expr(s.condition())
               << token("; ")
               << xpr_expr(s.increment())
               << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << indentation(-3) << needs_newline();
         }

         void visit(const For_in& s) final
         {
            pp << xpr_identifier("for")
               << token(" (")
               << xpr_decl(s.variable())
               << token(" <- ")
               << xpr_expr(s.sequence())
               << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << indentation(-3) << needs_newline();
         }

         void visit(const Break&) final
         {
            pp << xpr_identifier("break")
               << token(';')
               << needs_newline();
         }

         void visit(const Continue&) final
         {
            pp << xpr_identifier("continue")
               << token(';')
               << needs_newline();
         }

         void visit(const Goto& s) final
         {
            pp << xpr_identifier("goto")
               << token(' ')
               << xpr_expr(s.target())
               << token(';')
               << needs_newline();
         }

         void visit(const Handler& s) final
         {
            pp << xpr_identifier("catch")
               << token(' ')
               << token('(')
               << xpr_decl(s.exception())
               << token(')')
               << newline_and_indent(3)
               << xpr_stmt(s.body())
               << newline_and_indent(-3);
         }

         void visit(const Decl& d) override
         {
            // These are declaration statements, so they end up
            // with a semicolon.
            pp << xpr_decl(d, true);
         }
      };

      struct Location_printer
      {
         Printer* pp;

         // Location is only present on nodes derived from ipr::Stmt.
         // Nothing to print for other types of nodes at the moment.
         void operator()(const ipr::Node&) {}

         void operator()(const ipr::Stmt& stmt)
         {
            auto& locus = stmt.source_location();
            if (locus.file != File_index{})
            {
               *pp << token("F") << (int)locus.file << token(':') << (int)locus.line;
               if (locus.column != Column_number{})
                  *pp << token(':') << (int)locus.column;
               *pp << token(' ');
            }
         }
         static void print(Printer& printer, const ipr::Node& node)
         {
            if (printer.print_locations)
            {
               Constant_visitor<xpr::Location_printer> location_printer;
               location_printer.pp = &printer;
               node.accept(location_printer);
            }
         }
      };
   }

   Printer&
   operator<<(Printer& printer, xpr_stmt x)
   {
      if(printer.needs_newline())
         printer << newline_and_indent();
      
      xpr::Location_printer::print(printer, x.stmt);
      xpr::Stmt impl(printer);
      x.stmt.accept(impl);
      return printer;
   }


   Printer&
   operator<<(Printer& printer, DeclSpecifiers spec)
   {
      if (implies(spec, DeclSpecifiers::Export))
         printer << xpr_identifier("export");
      if (implies(spec, DeclSpecifiers::Register))
         printer << xpr_identifier("register");
      if (implies(spec, DeclSpecifiers::Static))
         printer << xpr_identifier("static");
      if (implies(spec, DeclSpecifiers::Extern))
         printer << xpr_identifier("extern");
      if (implies(spec, DeclSpecifiers::Mutable))
         printer << xpr_identifier("mutable");
      if (implies(spec, DeclSpecifiers::Inline))
         printer << xpr_identifier("inline");
      if (implies(spec, DeclSpecifiers::Virtual))
         printer << xpr_identifier("virtual");
      if (implies(spec, DeclSpecifiers::Explicit))
         printer << xpr_identifier("explicit");
      if (implies(spec, DeclSpecifiers::Friend))
         printer << xpr_identifier("friend");
      if (implies(spec, DeclSpecifiers::Public))
         printer << xpr_identifier("public");
      if (implies(spec, DeclSpecifiers::Protected))
         printer << xpr_identifier("protected");
      if (implies(spec, DeclSpecifiers::Private))
         printer << xpr_identifier("private");

      return printer;
   }


   // -- Declarations --

   namespace xpr {
      struct Decl : xpr::Stmt {
         Decl(Printer& p) : xpr::Stmt(p) { }

            void visit(const ipr::Alias& d) final
            {
               pp << ipr::xpr_name(d)
                  << token(" : ")
                  << d.specifiers()
                  << token(" typedef ")
                  << xpr_expr(d.initializer().get());
            }

         void visit(const ipr::Decl& d) override
         {
            pp << ipr::xpr_name(d)
               << token(" : ")
               << d.specifiers()
               << xpr_type(d.type());

            if (auto init = d.initializer())
               {
                  pp << token('(')
                     << xpr_expr(init.get())
                     << token(')');
               }
         }

         void visit(const Typedecl& d) final
         {
            pp << ipr::xpr_name(d)
               << token(" : ")
               << xpr_type(d.type());

            if (auto init = d.initializer())
               pp << xpr_type_expr(init.get());
         }

         void visit(const Enumerator& e) final
         {
            e.name().accept(*this);
            if (auto init = e.initializer())
               pp << token('(') << xpr_expr(init.get()) << token(')');
         }

         void visit(const Bitfield& b) final
         {
            b.name().accept(*this);
            pp << token(" : #")
               << xpr_identifier("bitfield")
               << token('(') << xpr_expr(b.precision()) << token(')')
               << xpr_type(b.type());
         }

         void visit(const Base_type& b) final
         {
            pp << b.specifiers() << xpr_type(b.type());
         }

         void visit(const Fundecl& f) final
         {
            pp << ipr::xpr_name(f)
               << token(" : ")
               << f.specifiers() << ' '
               << token('(') << f.parameters() << token(')');

            const Function* pfn = util::view<Function>(f.type());
            if (pfn)
                pp << xpr_type(pfn->target()); // dump result type

            if (auto init = f.initializer())
               pp << needs_newline()
                  << xpr_stmt(init.get());
         }

         void visit(const Template& m) final
         {
            m.name().accept(*this);
            pp << token(" : ")
               << xpr_mapping_expression(m.mapping());
         }
      };
   }

   Printer&
   operator<<(Printer& printer, xpr_decl x)
   {
      if (printer.needs_newline())
         printer << newline_and_indent();

      xpr::Location_printer::print(printer, x.decl);
      xpr::Decl impl(printer);
      x.decl.accept(impl);
      if (x.needs_semicolon)
        printer << token(';');
      return printer;
   }

   Printer&
   operator<<(Printer& pp, const Translation_unit& unit)
   {
      return pp << xpr_expr(unit.global_namespace().scope());
   }

} // of namespace ipr
