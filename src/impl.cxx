//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copright and license notices.
//

#include <ipr/impl>
#include <ipr/traversal>
#include <new>
#include <stdexcept>
#include <algorithm>
#include <typeinfo>
#include <cassert>
#include <iterator>
#include <utility>
#include <cstring>
#include <array>

namespace ipr {
   const String& String::empty_string()
   {
      struct Empty_string final : impl::Node<String> {
         constexpr util::word_view characters() const final { return u8""; }
      };

      static constexpr Empty_string empty { };
      return empty;
   }
}

namespace ipr::impl {
    namespace {
       // Representation of standard names (mostly identifiers) used in
       // in the internals of the IPR, with standard semantics.
       struct std_identifier : impl::Node<ipr::Identifier> {
          constexpr std_identifier(const char8_t* p) : str{p} { }
          constexpr const impl::String& operand() const final { return str; }
          constexpr auto text() const { return str.characters(); }
       private:
          impl::String str;
       };

        // A table of statically known words used in the internal representation.
        constexpr std_identifier known_words[] {
           u8"...",
           u8"C",
           u8"C++",
           u8"auto",
           u8"bool",
           u8"char",
           u8"char16_t",
           u8"char32_t",
           u8"char8_t",
           u8"class",
           u8"default",
           u8"delete",
           u8"double",
           u8"enum",
           u8"false",
           u8"float",
           u8"int",
           u8"long",
           u8"long double",
           u8"long long",
           u8"namespace",
           u8"nullptr",
           u8"short",
           u8"signed char",
           u8"this",
           u8"true",
           u8"typename",
           u8"union",
           u8"unsigned char",
           u8"unsigned int",
           u8"unsigned long",
           u8"unsigned long long",
           u8"unsigned short",
           u8"void",
           u8"wchar_t",
        };

        // Lexicographical less-than comparison between two known words.
        constexpr bool word_less(const std_identifier& x, const std_identifier& y)
        {
           return x.text() < y.text();
        }

        // Ensure the table of statically known words is lexicographically sorted.
        static_assert(std::is_sorted(std::begin(known_words), std::end(known_words), word_less));

        // less-than comparator for known words (which are all constexpr data)
        constexpr auto word_lt = [](auto& x, auto& y) { return x.text() < y; };

        // Return the String representation for a known word.
        // Raise an exception otherwise.
        constexpr const std_identifier& known_word(const char8_t* p)
        {
            auto place = std::lower_bound(std::begin(known_words), std::end(known_words), p, word_lt);
            if (place >= std::end(known_words) or place->text() != p)
               throw std::domain_error("unknown word");
            return *place;
        }

        constexpr auto& internal_string(const char8_t* p) { return known_word(p).operand(); }

        // Known language linkages to all C++ implementations.
        constexpr Linkage c_link { internal_string(u8"C") };
        constexpr Linkage cxx_link { internal_string(u8"C++") };
    }

    const ipr::Linkage& c_linkage() { return impl::c_link; }
    const ipr::Linkage& cxx_linkage() { return impl::cxx_link; }
}

namespace ipr::impl {
   namespace {
      // Representation of generalized built-in types.  All built-in types have type
      // "typename" and, of course, have C++ linkage.  Because they are known to all
      // implementations as elementary, they can be directly represented as constants,
      // therefore reducing initialization or startup time.
      using Builtin = symbolic_type<std_identifier>;

      enum class Fundamental {
#define BUILTIN_TYPE(S,N)  S,
#include "builtin.def"
#undef  BUILTIN_TYPE
      };

      constexpr Builtin builtins[] {
#define BUILTIN_TYPE(S,N) Builtin{ known_word(N) },
#include "builtin.def"
#undef BUILTIN_TYPE
      };

      constexpr const Builtin& builtin(Fundamental t)
      {
         return builtins[static_cast<int>(t)];
      }

      // Truth value symbolic constants.
      constexpr Symbol false_cst { known_word(u8"false"), builtin(Fundamental::Bool) };
      constexpr Symbol true_cst { known_word(u8"true"), builtin(Fundamental::Bool) };

      // Universal defaulter constant.
      constexpr Symbol default_cst { known_word(u8"default"), builtin(Fundamental::Auto) };

      // Universal deleted constant.  Nothing ever comes out.  Void.
      constexpr Symbol delete_cst { known_word(u8"delete"), builtin(Fundamental::Void) };
   }

   const ipr::Type& typename_type() { return builtin(Fundamental::Typename); }
}

namespace ipr::impl {
   namespace {
      // Representation of the `nullptr` symbolic constant.  It defines its own
      // type as it is a singleton. The type of `nullptr` is defined as the irreducible
      // type expression `decltype(nullptr)`.  Note that std::nullptr_t is just an
      // alias for that type, i.e. `std::nullptr_t` is a Decl, not a type.
      struct Nullptr : impl::Node<ipr::Symbol> {
         constexpr Nullptr() : typing{*this} { }
         const ipr::Name& operand() const final { return known_word(u8"nullptr"); }
         const ipr::Decltype& type() const final { return typing; }
      private:
         impl::Decltype typing;
      };

      constexpr Nullptr nullptr_cst { };
   }
}

namespace ipr::util {
    const ipr::String& string_pool::intern(word_view w)
    {
       if (w.empty())
         return ipr::String::empty_string();

        // For statically known words, just return the statically allocated address.
        if (const auto p = std::lower_bound(std::begin(impl::known_words), std::end(impl::known_words), w, impl::word_lt);
            p < std::end(impl::known_words) and p->text() == w)
            return p->string();

        // Dynamically allocated words are slotted by their hash codes into singly-linked lists.
        const hash_code h { std::hash<word_view>{ }(w) };
        auto& bucket = (*this)[h];
        const auto eq = [&w](auto& x) { return x.characters() == w; };
        if (const auto p = std::find_if(bucket.begin(), bucket.end(), eq); p != bucket.end())
            return *p;
        const auto fresh = strings.make_string(w.data(), w.length());
        bucket.emplace_front(util::word_view(fresh->data, fresh->length));
        return bucket.front();
    }
}

namespace ipr::cxx_form::impl {
   Function_morphism::Function_morphism(const ipr::Region& parent, Mapping_level level)
      : inputs{ parent, level }
   { }

   Pointer_indirector* form_factory::make_pointer_indirector()
   {
      return pointer_indirectors.make();
   }

   Reference_indirector* form_factory::make_reference_indirector()
   {
      return reference_indirectors.make();
   }

   Member_indirector* form_factory::make_member_indirector()
   {
      return member_indirectors.make();
   }

   Id_species* form_factory::make_id_species()
   {
      return id_species.make();
   }

   Function_morphism* form_factory::make_function_morphism(const ipr::Region& parent, Mapping_level level)
   {
      return function_morphisms.make(parent, level);
   }

   Array_morphism* form_factory::make_array_morphism()
   {
      return array_morphisms.make();
   }

   Parenthesized_species* form_factory::make_parenthesized_species()
   {
      return paren_species.make();
   }
}

namespace ipr::impl {
      Token::Token(const ipr::String& s, const Source_location& l,
                   TokenValue v, TokenCategory c)
            : text{ s }, location{ l }, token_value{ v }, token_category{ c }
      { }

      const ipr::BasicAttribute&
      attr_factory::make_basic_attribute(const ipr::Token& t)
      {
         return *basics.make(t);
      }

      const ipr::ScopedAttribute&
      attr_factory::make_scoped_attribute(const ipr::Token& s, const ipr::Token& m)
      {
         return *scopeds.make(s, m);
      }

      const ipr::LabeledAttribute&
      attr_factory::make_labeled_attribute(const ipr::Token& l, const ipr::Attribute& a)
      {
         return *labeleds.make(l, a);
      }

      const ipr::CalledAttribute&
      attr_factory::make_called_attribute(const ipr::Attribute& f, const ipr::Sequence<ipr::Attribute>& s)
      {
         return *calleds.make(f, s);
      }

      const ipr::ExpandedAttribute&
      attr_factory::make_expanded_attribute(const ipr::Token& t, const ipr::Attribute& a)
      {
         return *expandeds.make(t, a);
      }

      const ipr::FactoredAttribute&
      attr_factory::make_factored_attribute(const ipr::Token& t, const ipr::Sequence<ipr::Attribute>& s)
      {
         return *factoreds.make(t, s);
      }

      const ipr::ElaboratedAttribute&
      attr_factory::make_elaborated_attribute(const ipr::Expr& x)
      {
         return *elaborateds.make(x);
      }

      const ipr::Identifier&
      Enclosing_local_capture_specification::name() const
      {
         return *util::check(util::view<ipr::Identifier>(decl.name()));
      }

      // -- impl::capture_spec_factory --
      const ipr::Capture_specification::Default&
      capture_spec_factory::default_capture(Binding_mode m)
      {
         return *defaults.make(m);
      }

      const ipr::Capture_specification::Implicit_object&
      capture_spec_factory::implicit_object_capture(Binding_mode m)
      {
         return *implicits.make(m);
      }

      const ipr::Capture_specification::Enclosing_local&
      capture_spec_factory::enclosing_local_capture(const ipr::Decl& d, Binding_mode m)
      {
         return *enclosings.make(d, m);
      }

      const ipr::Capture_specification::Binding&
      capture_spec_factory::binding_capture(const ipr::Identifier& n, const ipr::Expr& x, Binding_mode m)
      {
         return *bindings.make(n, x, m);
      }

      const ipr::Capture_specification::Expansion&
      capture_spec_factory::expansion_capture(const ipr::Capture_specification::Named& c)
      {
         return *expansions.make(c);
      }

      const ipr::Sequence<ipr::Identifier>& Module_name::stems() const {
         return components;
      }

      // -- impl::New --
      New::New(Optional<ipr::Expr_list> where, const ipr::Construction& expr)
            : Classic_binary_expr<ipr::New>{ where, expr }
      { }

      bool New::global_requested() const { return global; }

      // -------------------------------------
      // -- master_decl_data<ipr::Template> --
      // -------------------------------------

      master_decl_data<ipr::Template>::
      master_decl_data(impl::Overload* ovl, const ipr::Type& t)
            : Base{this}, overload_entry{t},
              primary{}, home{}, overload{ovl}
      { }

      // --------------------
      // -- impl::Overload --
      // --------------------

      Overload::Overload(const ipr::Name& n)
            : name{n}
      { }

      Optional<ipr::Decl>
      Overload::operator[](const ipr::Type& t) const
      {
         if (auto entry = lookup(t))
            return { entry->declset.get(0) };                     // Note: first decl is canonical
         return { };
      }

      impl::overload_entry*
      Overload::lookup(const ipr::Type& t) const {
         return  entries.find(t, node_compare());
      }

      template<class T>
      void
      Overload::push_back(master_decl_data<T>* data) {
         entries.insert(data, node_compare());
         masters.push_back(data);
      }

      // -- Directives --
      single_using_declaration::single_using_declaration(const ipr::Scope_ref& s, Designator::Mode m)
         : what{s, m}
      { }

      Using_directive::Using_directive(const ipr::Scope& s) : scope{s} { }

      namespace {
         // Helper function for building expression nodes with type assignment.
         template<typename T, typename... Args>
         auto make(stable_farm<T>& factory, const Args&... args)
         {
            struct Holder {
               explicit Holder(T* x) : impl{ x } { }

               T* with_type(const ipr::Type& t)
               {
                  impl->typing = &t;
                  return impl;
               }

               T* with_type(Optional<ipr::Type> t)
               {
                  impl->typing = t;
                  return impl;
               }

            private:
               T* impl;
            };

            return Holder{ factory.make(args...) };
         }
      }

      // -----------------
      // -- impl::Alias --
      // -----------------

      Alias::Alias() : aliasee{nullptr}
      { }

      // --------------------
      // -- impl::Bitfield --
      // --------------------

      Bitfield::Bitfield() : length{}, init{}
      { }

      // ---------------------
      // -- impl::Base_type --
      // ---------------------

      Base_type::Base_type(const ipr::Type& t, const ipr::Region& r, Decl_position p)
            : base(t), where(r), scope_pos(p), spec{ }
      { }

      Optional<ipr::Expr>
      Base_type::initializer() const {
         throw std::domain_error("impl::Base_type::initializer");
      }

      // ----------------------
      // -- impl::Enumerator --
      // ----------------------

      Enumerator::Enumerator(const ipr::Name& n, const ipr::Enum& t, Decl_position p)
            : id{n}, typing{t}, scope_pos{p}, where{}, init{}
      { }

      // -----------------
      // -- impl::Field --
      // -----------------

      Field::Field() : init{}
      { }

      // -------------------
      // -- impl::Fundecl --
      // -------------------

      Fundecl::Fundecl()
            : data{}, lexreg{}
      { }

      const ipr::Parameter_list& Fundecl::parameters() const {
         if (data.index() == 0)
            return *util::check(data.parameters());
         return util::check(data.mapping())->parameters();
      }

      Optional<ipr::Mapping> Fundecl::mapping() const {
         if (data.index() == 0)
            return { };
         return { data.mapping() };
      }

      Optional<ipr::Expr> Fundecl::initializer() const {
         if (data.index() == 0)
            return { };
         return { data.mapping() };
      }

      // --------------------
      // -- impl::Template --
      // --------------------

      Template::Template() : init{}, lexreg{} { }

      const ipr::Template&
      Template::primary_template() const {
         return *util::check(util::check(decl_data.master_data)->primary);
      }

      const ipr::Sequence<ipr::Decl>&
      Template::specializations() const {
         return util::check(decl_data.master_data)->specs;
      }

      // ---------------------
      // -- impl::Parameter --
      // ---------------------

      Parameter::Parameter(const ipr::Name& n, const ipr::Type& t, Decl_position p)
            :id{n}, typing{t}, pos{p}
      { }

      // -- impl::EH_parameter
      EH_parameter::EH_parameter(const ipr::Region& r, const ipr::Name& n, const ipr::Type& t)
         : id{n}, typing{t}, home{r}
      { }

      // -- impl::handler_block
      handler_block::handler_block(const ipr::Region& r) : lexical_region{&r} { }

      // -- impl::Handler
      Handler::Handler(const ipr::Region& r, const ipr::Name& n, const ipr::Type& t)
         : eh{r, n, t}, block{eh}
      { }

      // --------------------
      // -- impl::Typedecl --
      // --------------------

      Typedecl::Typedecl() : init{}, lexreg{}
      { }

      // ---------------
      // -- impl::Var --
      // ---------------

      Var::Var() : init{}, lexreg{}
      { }

      // -----------------
      // -- impl::Block --
      // -----------------

      Block::Block(const ipr::Region& pr)
            : lexical_region(&pr)
      {
         lexical_region.owned_by = this;
      }

      impl::Handler* Block::new_handler(const ipr::Name& n, const ipr::Type& t)
      {
         return handler_seq.push_back(lexical_region.enclosing(), n, t);
      }

      // ---------------
      // -- impl::For --
      // ---------------

      For::For() : init{}, cond{}, inc{}, stmt{}
      { }

      // ------------------
      // -- impl::For_in --
      // ------------------
      For_in::For_in() : var{}, seq{}, stmt{}
      { }

      // -- impl::Break
      Break::Break() : stmt{} { }

      const ipr::Type& Break::type() const
      {
         return impl::builtin(Fundamental::Void);
      }

      // -- impl::Continue
      Continue::Continue() : stmt{} { }

      const ipr::Type& Continue::type() const
      {
         return impl::builtin(Fundamental::Void);
      }

      // -- impl::dir_factory --
      impl::Specifiers_spread* dir_factory::make_specifiers_spread()
      {
         return spreads.make();
      }

      impl::Structured_binding*
      dir_factory::make_structured_binding()
      {
         return bindings.make();
      }

      impl::single_using_declaration*
      dir_factory::make_using_declaration(const ipr::Scope_ref& s, ipr::Using_declaration::Designator::Mode m)
      {
         return singles.make(s, m);
      }

      impl::Using_declaration* dir_factory::make_using_declaration()
      {
         return usings.make();
      }

      impl::Using_directive* dir_factory::make_using_directive(const ipr::Scope& s, const ipr::Type& t)
      {
         return make(dirs, s).with_type(t);
      }

      impl::Phased_evaluation* dir_factory::make_phased_evaluation(const ipr::Expr& e, Phases f)
      {
         return phaseds.make(e, f);
      }

      impl::Pragma* dir_factory::make_pragma()
      {
         return pragmas.make();
      }

      // ------------------------
      // -- impl::stmt_factory --
      // ------------------------

      impl::Break* stmt_factory::make_break()
      {
         return breaks.make();
      }

      impl::Continue* stmt_factory::make_continue()
      {
         return continues.make();
      }

      impl::Block*
      stmt_factory::make_block(const ipr::Region& pr, Optional<ipr::Type> t) {
         return make(blocks, pr).with_type(t);
      }

      impl::Ctor_body*
      stmt_factory::make_ctor_body(const ipr::Expr_list& m,
                                   const ipr::Block& b) {
         return ctor_bodies.make(m, b);
      }

      impl::Expr_stmt*
      stmt_factory::make_expr_stmt(const ipr::Expr& e) {
         return expr_stmts.make(e);
      }

      impl::Goto*
      stmt_factory::make_goto(const ipr::Expr& e) {
         return gotos.make(e);
      }

      impl::Return*
      stmt_factory::make_return(const ipr::Expr& e) {
         return returns.make(e);
      }

      impl::Do* stmt_factory::make_do()
      {
         return dos.make();
      }

      impl::If* stmt_factory::make_if(const ipr::Expr& c, const ipr::Expr& s)
      {
         return ifs.make(c, s, nullptr);
      }

      impl::If*
      stmt_factory::make_if(const ipr::Expr& c, const ipr::Expr& t, const ipr::Expr& f)
      {
         return ifs.make(c, t, &f);
      }

      impl::Switch* stmt_factory::make_switch()
      {
         return switches.make();
      }

      impl::Labeled_stmt* stmt_factory::make_labeled_stmt(const ipr::Expr& l, const ipr::Expr& s)
      {
         return labeled_stmts.make(l, s);
      }

      impl::While* stmt_factory::make_while()
      {
         return whiles.make();
      }

      impl::For*
      stmt_factory::make_for() {
         return fors.make();
      }

      impl::For_in*
      stmt_factory::make_for_in() {
         return for_ins.make();
      }

      // ----------------
      // -- impl::Enum --
      // ----------------

      Enum::Enum(const ipr::Region& r, Kind k)
            : body(r), enum_kind(k)
      {
         body.owned_by = this;
      }

      const ipr::Type& Enum::type() const { return impl::builtin(Fundamental::Enum); }

      const ipr::Region&
      Enum::region() const {
         return body;
      }

      const ipr::Sequence<ipr::Enumerator>&
      Enum::members() const {
         return body.scope.decls.seq;
      }

      Enum::Kind Enum::kind() const { return enum_kind; }

      impl::Enumerator*
      Enum::add_member(const ipr::Name& n) {
         Decl_position pos { members().size() };
         impl::Enumerator* e = body.scope.push_back(n, *this, pos);
         e->where = &body;
         return e;
      }

      // -- impl::Union
      Union::Union(const ipr::Region& r) : Udt<ipr::Union>{&r} { }

      const ipr::Type& Union::type() const
      {
         return impl::builtin(Fundamental::Union);
      }

      // -- impl::Namespace
      Namespace::Namespace(const ipr::Region* r) : Udt<ipr::Namespace>{r} { }

      const ipr::Type& Namespace::type() const
      {
         return impl::builtin(Fundamental::Namespace);
      }

      // -- impl::Class --
      Class::Class(const ipr::Region& pr)
            : impl::Udt<ipr::Class>(&pr),
              base_subobjects(pr)
      {
         base_subobjects.owned_by = this;
      }

      const ipr::Type& Class::type() const
      {
         return impl::builtin(Fundamental::Class);
      }

      const ipr::Sequence<ipr::Base_type>&
      Class::bases() const {
         return base_subobjects.scope.decls.seq;
      }

      impl::Base_type*
      Class::declare_base(const ipr::Type& t) {
         Decl_position pos { bases().size() };
         return base_subobjects.scope.push_back(t, base_subobjects, pos);
      }

      // -- impl::Closure --
      Closure::Closure(const ipr::Region& r)
         : impl::Udt<ipr::Closure>(&r)
      { }

      const ipr::Type& Closure::type() const
      {
         return impl::builtin(Fundamental::Class);
      }

      // --------------------------
      // -- impl::Parameter_list --
      // --------------------------

      Parameter_list::Parameter_list(const ipr::Region& p, Mapping_level l)
            : parms(p), nesting{ l }
      { }

      const ipr::Product& Parameter_list::type() const { return parms.scope.type(); }

      const ipr::Region& Parameter_list::region() const { return parms; }

      const ipr::Sequence<ipr::Parameter>&
      Parameter_list::elements() const {
         return parms.scope.decls.seq;
      }

      impl::Parameter*
      Parameter_list::add_member(const ipr::Name& n, const ipr::Type& t)
      {
         Decl_position pos { parms.scope.size() } ;
         impl::Parameter* param = parms.scope.push_back(n, t, pos);
         param->where = this;
         return param;
      }

      // ------------------------
      // -- impl::type_factory --
      // ------------------------

      struct unary_compare {
         int operator()(const ipr::Node& lhs, const ipr::Node& rhs) const
         {
            return compare(lhs, rhs);
         }

         template<class T>
         int operator()(const node_ref<T>& lhs, const ipr::Node& rhs) const
         {
            return compare(lhs.node, rhs);
         }

         template<class T>
         int operator()(const ipr::Node& lhs, const node_ref<T>& rhs) const
         {
            return compare(lhs, rhs.node);
         }

         template<class T>
         int operator()(const Unary<T>& lhs,
                        const typename Unary<T>::Arg_type& rhs) const
         {
            return compare(lhs.rep, rhs);
         }

         template<class T>
         int operator()(const typename Unary<T>::Arg_type& lhs,
                        const Unary<T>& rhs) const
         {
            return compare(lhs, rhs.rep);
         }
      };

      struct unary_lexicographic_compare {
         template<class T>
         int operator()(const Unary<T>& lhs,
                        const ipr::Sequence<ipr::Type>& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.rep.begin(), lhs.rep.end(),
                rhs.begin(), rhs.end(), unary_compare());
         }

         template<class T>
         int operator()(const ipr::Sequence<ipr::Type>& lhs,
                        const Unary<T>& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.begin(), lhs.end(),
                rhs.rep.begin(), rhs.rep.end(), unary_compare());
         }

         template<class T>
         int operator()(const ref_sequence<T>& lhs,
                        const ref_sequence<T>& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.begin(), lhs.end(),
                rhs.begin(), rhs.end(), unary_compare());
         }
      };


      inline bool
      operator==(const ipr::Node& lhs, const ipr::Node& rhs)
      {
         return &lhs == &rhs;
      }

      struct binary_compare {
         template<class T>
         int operator()(const Binary<T>& lhs,
                        const typename Binary<T>::Rep& rhs) const
         {
            if (int cmp = compare(lhs.rep.first, rhs.first)) return cmp;

            return compare(lhs.rep.second, rhs.second);
         }

         template<class T>
         int operator()(const typename Binary<T>::Rep lhs,
                         const Binary<T>& rhs) const
         {
            if (int cmp = compare(lhs.first, rhs.rep.first)) return cmp;

            return compare(lhs.second, rhs.rep.second);
         }
      };

      struct id_compare
      {
          int operator()(const ipr::Identifier& lhs, const ipr::String& rhs) const
          {
              return compare(lhs.string(), rhs);
          }

          int operator()(const ipr::String& lhs, const ipr::Identifier& rhs) const
          {
              return compare(lhs, rhs.string());
          }
      };


      // >>>> Yuriy Solodkyy: 2008/07/10
      // This comparison would be used to unify Unary nodes: on LHS we would be
      // called with a type for which Pointer (Reference/sizeof etc.) is created,
      // on RHS we would be called with already allocated Pointer types. Thus we
      // have to check whether any of the existing Pointer types does not already
      // have a points_to (its operand()) equal to the type in LHS.
      struct unified_type_compare
      {
          template<class Cat, class Operand>
          int operator()(const ipr::Unary<Cat,Operand>& lhs, const ipr::Type& rhs) const
          {
              return compare(lhs.operand(), rhs);
          }
      };
      // <<<< Yuriy Solodkyy: 2008/07/10

      const ipr::Array& type_factory::get_array(const ipr::Type& t, const ipr::Expr& b)
      {
         using rep = impl::Array::Rep;
         return *arrays.insert(rep{ t, b }, binary_compare());
      }

      const ipr::Qualified&
      type_factory::get_qualified(ipr::Type_qualifiers cv, const ipr::Type& t)
      {
         // It is an error to call this function if there is no real
         // qualified.
         if (cv == ipr::Type_qualifiers::None)
            throw std::domain_error
               ("type_factoy::get_qualified: no qualifier");

         using rep = impl::Qualified::Rep;
         return *qualifieds.insert(rep{ cv, t }, binary_compare());
      }

      const ipr::Decltype& type_factory::get_decltype(const ipr::Expr& e)
      {
         if (physically_same(e, impl::nullptr_cst))
            return impl::nullptr_cst.type();
         return *decltypes.make(e);
      }

      const ipr::As_type& type_factory::get_as_type(const ipr::Identifier& id)
      {
         for (auto& t : impl::builtins) {
            if (physically_same(t.name(), id))
               return t;
         }
         return *extendeds.insert(id, unary_compare());
      }

      const ipr::As_type& type_factory::get_as_type(const ipr::Expr& e)
      {
         return *type_refs.insert(e, unary_compare());
      }

      const ipr::As_type&
      type_factory::get_as_type(const ipr::Expr& e, const ipr::Linkage& l)
      {
         if (physically_same(l, impl::cxx_linkage()))
            return get_as_type(e);

         using T = impl::As_type_with_linkage;
         struct Comparator {
            int operator()(const T& x, const T::Rep& y) const
            {
               if (auto cmp = compare(x.expr(), y.expr))
                  return cmp;
               return compare(x.linkage(), y.link);
            }

            int operator()(const T::Rep& x, const T& y) const
            {
               return -(*this)(y, x);
            }
         };
         return *type_links.insert(T::Rep{e, l}, Comparator{ });
      }

      struct ternary_compare {
         template<class T>
         int operator()(const Ternary<T>& lhs,
                        const typename Ternary<T>::Rep& rhs) const
         {
            if (int cmp = compare(lhs.rep.first, rhs.first)) return cmp;
            if (int cmp = compare(lhs.rep.second, rhs.second)) return cmp;

            return compare(lhs.rep.third, rhs.third);
         }

         template<class T>
         int operator()(const typename Ternary<T>::Rep& lhs,
                        const Ternary<T>& rhs) const
         {
            if (int cmp = compare(lhs.first, rhs.rep.first)) return cmp;
            if (int cmp = compare(lhs.second, rhs.rep.second)) return cmp;

            return compare(lhs.third, rhs.rep.third);
         }
      };

      const ipr::Tor& type_factory::get_tor(const ipr::Product& s, const ipr::Sum& e)
      {
         using rep = impl::Tor::Rep;
         return *tors.insert(rep{ s, e }, binary_compare());
      }

      const ipr::Function& type_factory::get_function(const ipr::Product& s, const ipr::Type& t)
      {
         return get_function(s, t, impl::false_cst);
      }

      const ipr::Function&
      type_factory::get_function(const ipr::Product& s, const ipr::Type& t, const ipr::Linkage& l)
      {
         return get_function(s, t, impl::false_cst, l);
      }

      const ipr::Function&
      type_factory::get_function(const ipr::Product& s, const ipr::Type& t,
                                 const ipr::Expr& e)
      {
         using rep = impl::Function::Rep;
         return *functions.insert(rep{ s, t, e }, ternary_compare());
      }

      const ipr::Function&
      type_factory::get_function(const ipr::Product& s, const ipr::Type& t,
                                 const ipr::Expr& e, const ipr::Linkage& l)
      {
         if (physically_same(l, impl::cxx_linkage()))
            return get_function(s, t, e);

         using T = impl::Function_with_linkage;
         struct Comparator {
            int operator()(const T& x, const T::Rep& y) const
            {
               if (auto cmp = compare(x.source(), y.source))
                  return cmp;
               if (auto cmp = compare(x.target(), y.target))
                  return cmp;
               if (auto cmp = compare(x.throws(), y.throws))
                  return cmp;
               return compare(x.linkage(), y.link);
            }

            int operator()(const T::Rep& x, const T& y) const
            {
               return -(*this)(y, x);
            }
         };

         return *fun_links.insert(T::Rep{ s, t, e, l }, Comparator{ });
      }

      const ipr::Pointer& type_factory::get_pointer(const ipr::Type& t)
      {
         // >>>> Yuriy Solodkyy: 2008/07/10
         // Fixed pointer comparison for unification
         return *pointers.insert(t, unified_type_compare());
         // <<<< Yuriy Solodkyy: 2008/07/10
      }

      const ipr::Product& type_factory::get_product(const ipr::Sequence<ipr::Type>& seq)
      {
         return *products.insert(seq, unary_lexicographic_compare());
      }

      const ipr::Product& type_factory::get_product(const Warehouse<ipr::Type>& seq)
      {
         return get_product(*type_seqs.insert(seq.rep(), unary_lexicographic_compare()));
      }

      const ipr::Ptr_to_member&
      type_factory::get_ptr_to_member(const ipr::Type& c, const ipr::Type& t)
      {
         using rep = impl::Ptr_to_member::Rep;
         return *member_ptrs.insert(rep{ c, t }, binary_compare());
      }

      const ipr::Reference& type_factory::get_reference(const ipr::Type& t)
      {
         return *references.insert(t, unified_type_compare());
      }

      const ipr::Rvalue_reference& type_factory::get_rvalue_reference(const ipr::Type& t)
      {
         return *refrefs.insert(t, unified_type_compare());
      }

      const ipr::Sum& type_factory::get_sum(const ipr::Sequence<ipr::Type>& seq)
      {
         return *sums.insert(seq, unary_lexicographic_compare());
      }

      const ipr::Sum& type_factory::get_sum(const Warehouse<ipr::Type>& seq)
      {
         return get_sum(*type_seqs.insert(seq.rep(), unary_lexicographic_compare()));
      }

      const ipr::Forall& type_factory::get_forall(const ipr::Product& s, const ipr::Type& t)
      {
         using rep = impl::Forall::Rep;
         return *foralls.insert(rep{ s, t }, binary_compare());
      }

      const ipr::Auto& type_factory::get_auto()
      {
         return *autos.make();
      }

      impl::Enum* type_factory::make_enum(const ipr::Region& pr, Enum::Kind k)
      {
         return enums.make(pr, k);
      }

      impl::Class* type_factory::make_class(const ipr::Region& pr)
      {
         return classes.make(pr);
      }

      impl::Union* type_factory::make_union(const ipr::Region& pr)
      {
         return unions.make(pr);
      }

      impl::Namespace* type_factory::make_namespace(const ipr::Region& pr)
      {
         return namespaces.make(&pr);
      }

      impl::Closure* type_factory::make_closure(const ipr::Region& r)
      {
         return closures.make(r);
      }

      // -- impl::Asm
      Asm::Asm(const ipr::String& s) : impl::Unary_node<ipr::Asm>{s} { }

      const ipr::Type& Asm::type() const { return impl::builtin(Fundamental::Void); }

      // ---------------------
      // -- impl::Expr_list --
      // ---------------------

      Expr_list::Expr_list()
      { }

      Expr_list::Expr_list(const ref_sequence<ipr::Expr>& s) : seq(s)
      { }

      const ipr::Product&
      Expr_list::type() const {
         return seq;
      }

      const ipr::Sequence<ipr::Expr>&
      Expr_list::operand() const {
         return seq.seq;
      }

      // -------------
      // -- Id_expr --
      // -------------

      Id_expr::Id_expr(const ipr::Name& n)
            : impl::Unary<impl::Expr<ipr::Id_expr>>(n)
      { }

      Optional<ipr::Expr>
      Id_expr::resolution() const {
         return decls;
      }

      // ---------------
      // -- Enclosure --
      // ---------------
      Enclosure::Enclosure(ipr::Delimiter d, const ipr::Expr& e)
         : impl::Unary_expr<ipr::Enclosure>{ e }, delim{ d}
      { }

      // -----------------
      // -- Binary_fold --
      // -----------------
      Binary_fold::Binary_fold(Category_code op, const ipr::Expr& x, const ipr::Expr& y)
         : Classic{x, y}, fold_op{ op }
      { }

      Category_code Binary_fold::operation() const { return fold_op; }

      // -- impl::General_substitution
      const ipr::Expr& General_substitution::operator[](const ipr::Parameter& p) const
      {
         if (auto where = mapping.find(&p); where != mapping.end())
            return *where->second;
         return p;
      }

      General_substitution& General_substitution::subst(const ipr::Parameter& p, const ipr::Expr& v)
      {
         mapping.insert_or_assign(&p, &v);
         return *this;
      }

      // -- impl::Mapping
      Mapping::Mapping(const ipr::Region& pr, Mapping_level d)
            : impl::Parameterization<ipr::Expr, impl::Expr<ipr::Mapping>>{pr, d}
      {
         inputs.parms.owned_by = this;
      }

      // -- impl::Lambda
      Lambda::Lambda(const ipr::Region& r, Mapping_level l)
         : impl::Parameterization<ipr::Expr, impl::Node<ipr::Lambda>>{r, l}, lam_spec{}
      {
         inputs.parms.owned_by = this;
      }

      // -------------------------------
      // -- impl::Scope --
      // -------------------------------
      Scope::Scope() { }

      Optional<ipr::Overload> Scope::operator[](const ipr::Name& n) const
      {
         if (impl::Overload* ovl = overloads.find(n, node_compare()))
            return { ovl };
         return { };
      }

      template<class T>
      inline void
      Scope::add_member(T* decl)
      {
         decls.seq.push_back(decl);
      }

      impl::Alias*
      Scope::make_alias(const ipr::Name& n, const ipr::Expr& i) {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(i.type());

         if (master == nullptr) {
            impl::Alias* decl = aliases.declare(ovl, i.type());
            decl->aliasee = &i;
            add_member(decl);
            return decl;
         }
         else {
            impl::Alias* decl = aliases.redeclare(master);
            decl->aliasee = &i;
            add_member(decl);
            return decl;
         }
      }

      impl::Var*
      Scope::make_var(const ipr::Name& n, const ipr::Type& t) {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Var* var = vars.declare(ovl, t);
            add_member(var);
            return var;
         }
         else {
            impl::Var* var = vars.redeclare(master);
            add_member(var);
            return var;
         }
      }

      impl::Field*
      Scope::make_field(const ipr::Name& n, const ipr::Type& t) {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Field* field = fields.declare(ovl, t);
            add_member(field);
            return field;
         }
         else {
            impl::Field* field = fields.redeclare(master);
            add_member(field);
            return field;
         }
      }

      impl::Bitfield*
      Scope::make_bitfield(const ipr::Name& n, const ipr::Type& t) {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Bitfield* field = bitfields.declare(ovl, t);
            add_member(field);
            return field;
         }
         else {
            impl::Bitfield* field = bitfields.redeclare(master);
            add_member(field);
            return field;
         }
      }

      // Make a node for a type-declaration with name N and type T.
      impl::Typedecl*
      Scope::make_typedecl(const ipr::Name& n, const ipr::Type& t)
      {
         // Get the overload-set for this name.
         impl::Overload* ovl = overloads.insert(n, node_compare());

         // Does the overload-set already contain a decl with that type?
         overload_entry* master = ovl->lookup(t);
         impl::Typedecl* decl = master == nullptr ?
            typedecls.declare(ovl, t) : // no, this is the first declaration
            typedecls.redeclare(master); // just re-declare.
         add_member(decl);      // remember we saw a declaration.
         return decl;
      }

      impl::Fundecl*
      Scope::make_fundecl(const ipr::Name& n, const ipr::Function& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Fundecl* decl = fundecls.declare(ovl, t);
            add_member(decl);
            return decl;
         }
         else {
            impl::Fundecl* decl = fundecls.redeclare(master);
            add_member(decl);
            return decl;
         }
      }

      impl::Template*
      Scope::make_primary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Template* decl = primary_maps.declare(ovl, t);
            decl->decl_data.master_data->primary = decl;
            add_member(decl);
            return decl;
         }
         else {
            impl::Template* decl = primary_maps.redeclare(master);
            // FIXME: set the primary field.
            add_member(decl);
            return decl;
         }
      }

      impl::Template*
      Scope::make_secondary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == nullptr) {
            impl::Template* decl = secondary_maps.declare(ovl, t);
            // FXIME: record this a secondary map and set its primary.
            add_member(decl);
            return decl;
         }
         else {
            impl::Template* decl = secondary_maps.redeclare(master);
            // FIXME: set primary info.
            add_member(decl);
            return decl;
         }
      }

      // --------------------------------
      // -- impl::Region --
      // --------------------------------

      Region::Region(Optional<ipr::Region> pr)
            : parent{pr}
      { }


      Region*
      Region::make_subregion() {
         return subregions.make(this);
      }

      Where::Where(const ipr::Region& parent) : region{&parent} { }

      // -- impl::Static_assert
      Static_assert::Static_assert(const ipr::Expr& e, Optional<ipr::String> s)
         : Binary_node<ipr::Static_assert>{e, s}
      { }

      const ipr::Type& Static_assert::type() const
      {
         return impl::builtin(Fundamental::Bool);
      }

      // -- impl::name_factory

      const ipr::String& name_factory::get_string(util::word_view w)
      {
         return strings.intern(w);
      }

      const ipr::Identifier& name_factory::get_identifier(const ipr::String& s)
      {
         return *ids.insert(s, id_compare());
      }

      const ipr::Identifier& name_factory::get_identifier(util::word_view w)
      {
         return get_identifier(get_string(w));
      }

      const ipr::Suffix& name_factory::get_suffix(const ipr::Identifier& s)
      {
         return *suffixes.insert(s, unary_compare());
      }

      const ipr::Operator& name_factory::get_operator(const ipr::String& s)
      {
         return *ops.insert(s, unary_compare());
      }

      const ipr::Operator& name_factory::get_operator(util::word_view w)
      {
         return get_operator(get_string(w));
      }

      const ipr::Ctor_name& name_factory::get_ctor_name(const ipr::Type& t)
      {
         return *ctors.insert(t, unary_compare());
      }

      const ipr::Dtor_name& name_factory::get_dtor_name(const ipr::Type& t)
      {
         return *dtors.insert(t, unary_compare());
      }

      const ipr::Conversion& name_factory::get_conversion(const ipr::Type& t)
      {
         return *convs.insert(t, unary_compare());
      }

      const ipr::Guide_name& name_factory::get_guide_name(const ipr::Template& m)
      {
         return *guide_ids.insert(m, unary_compare());
      }

      // ------------------------
      // -- impl::expr_factory --
      // ------------------------

      // -- Language linkage
      const ipr::Linkage& expr_factory::get_linkage(util::word_view w)
      {
         if (w == u8"C")
            return impl::c_link;
         else if (w == u8"C++")
            return impl::cxx_link;
         return get_linkage(get_string(w));
      }

      const ipr::Linkage& expr_factory::get_linkage(const ipr::String& lang)
      {
         if (physically_same(lang, internal_string(u8"C")))
            return impl::c_link;
         else if (physically_same(lang, internal_string(u8"C++")))
            return impl::cxx_link;
         return *linkages.insert(lang, unary_compare());
      }

      const ipr::Symbol&
      expr_factory::get_symbol(const ipr::Name& n, const ipr::Type& t)
      {
         const auto comparator = [&t](auto& x, auto& y) {
            if (auto cmp = compare(x.name(), y))
               return cmp;
            return compare(x.type(), t);
         };

         auto sym = symbols.insert(n, comparator);
         sym->typing = &t;
         return *sym;
      }

      const ipr::Symbol& expr_factory::get_label(const ipr::Identifier& n)
      {
         if (physically_same(n, known_word(u8"default")))
            return impl::default_cst;
         return get_symbol(n, impl::builtin(Fundamental::Void));
      }

      const ipr::Symbol& expr_factory::get_this(const ipr::Type& t)
      {
         return get_symbol(known_word(u8"this"), t);
      }

      impl::Phantom*
      expr_factory::make_phantom() {
         return phantoms.make();
      }

      const ipr::Phantom*
      expr_factory::make_phantom(const ipr::Type& t) {
         return phantoms.make(&t);
      }

      impl::Eclipsis* expr_factory::make_eclipsis(const ipr::Type& t)
      {
         return eclipses.make(&t);
      }

      impl::Address*
      expr_factory::make_address(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(addresses, e).with_type(t);
      }

      impl::Array_delete*
      expr_factory::make_array_delete(const ipr::Expr& e) {
         return array_deletes.make(e);
      }
      impl::Asm* expr_factory::make_asm_expr(const ipr::String& s)
      {
         return asms.make(s);
      }

      impl::Complement*
      expr_factory::make_complement(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(complements, e).with_type(t);
      }

      impl::Delete*
      expr_factory::make_delete(const ipr::Expr& e) {
         return deletes.make(e);
      }

      impl::Demotion*
      expr_factory::make_demotion(const ipr::Expr& e, const ipr::Type& t)
      {
         return make(demotions, e).with_type(t);
      }

      impl::Deref*
      expr_factory::make_deref(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(derefs, e).with_type(t);
      }

      impl::Expr_list*
      expr_factory::make_expr_list() {
         return xlists.make();
      }

      impl::Id_expr*
      expr_factory::make_id_expr(const ipr::Name& n, Optional<ipr::Type> t)
      {
         return make(id_exprs, n).with_type(t);
      }

      impl::Id_expr*
      expr_factory::make_id_expr(const ipr::Decl& d)
      {
         auto x = make(id_exprs, d.name()).with_type(d.type());
         x->decls = &d;
         return x;
      }

      impl::Label*
      expr_factory::make_label(const ipr::Identifier& n, Optional<ipr::Type> t)
      {
         return make(labels, n).with_type(t);
      }

      Materialization*
      expr_factory::make_materialization(const ipr::Expr& e, const ipr::Type& t)
      {
         return make(materializations, e).with_type(t);
      }

      impl::Not*
      expr_factory::make_not(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(nots, e).with_type(t);
      }

      impl::Enclosure*
      expr_factory::make_enclosure(ipr::Delimiter d, const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(enclosures, d, e).with_type(t);
      }

      impl::Post_increment*
      expr_factory::make_post_increment(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(post_increments, e).with_type(t);
      }

      impl::Post_decrement*
      expr_factory::make_post_decrement(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(post_decrements, e).with_type(t);
      }

      impl::Pre_increment*
      expr_factory::make_pre_increment(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(pre_increments, e).with_type(t);
      }

      impl::Pre_decrement*
      expr_factory::make_pre_decrement(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(pre_decrements, e).with_type(t);
      }

      impl::Promotion*
      expr_factory::make_promotion(const ipr::Expr& e, const ipr::Type& t)
      {
         return make(promotions, e).with_type(t);
      }

      impl::Read*
      expr_factory::make_read(const ipr::Expr& e, const ipr::Type& t)
      {
         return make(reads, e).with_type(t);
      }

      impl::Throw*
      expr_factory::make_throw(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(throws, e).with_type(t);
      }

      impl::Alignof* expr_factory::make_alignof(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(alignofs, e).with_type(t);
      }

      impl::Sizeof*
      expr_factory::make_sizeof(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(sizeofs, e).with_type(t);
      }

      impl::Args_cardinality*
      expr_factory::make_args_cardinality(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(cardinalities, e).with_type(t);
      }

      impl::Typeid*
      expr_factory::make_typeid(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(xtypeids, e).with_type(t);
      }

      impl::Unary_minus*
      expr_factory::make_unary_minus(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(unary_minuses, e).with_type(t);
      }

      impl::Unary_plus*
      expr_factory::make_unary_plus(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(unary_pluses, e).with_type(t);
      }

      impl::Expansion*
      expr_factory::make_expansion(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(expansions, e).with_type(t);
      }

      impl::Construction*
      expr_factory::make_construction(const ipr::Type& t, const ipr::Enclosure& e)
      {
         return make(constructions, e).with_type(t);
      }

      impl::Noexcept*
      expr_factory::make_noexcept(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(noexcepts, e).with_type(t);
      }

      impl::Rewrite* expr_factory::make_rewrite(const ipr::Expr& s, const ipr::Expr& t)
      {
         return rewrites.make(s, t);
      }

      impl::And*
      expr_factory::make_and(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(ands, l, r).with_type(t);
      }

      impl::Array_ref*
      expr_factory::make_array_ref(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(array_refs, l, r).with_type(t);
      }

      impl::Arrow*
      expr_factory::make_arrow(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(arrows, l, r).with_type(t);
      }

      impl::Arrow_star*
      expr_factory::make_arrow_star(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(arrow_stars, l, r).with_type(t);
      }

      impl::Assign*
      expr_factory::make_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(assigns, l, r).with_type(t);
      }

      impl::Bitand*
      expr_factory::make_bitand(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitands, l, r).with_type(t);
      }

      impl::Bitand_assign*
      expr_factory::make_bitand_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitand_assigns, l, r).with_type(t);
      }

      impl::Bitor*
      expr_factory::make_bitor(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitors, l, r).with_type(t);
      }

      impl::Bitor_assign*
      expr_factory::make_bitor_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitor_assigns, l, r).with_type(t);
      }

      impl::Bitxor*
      expr_factory::make_bitxor(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitxors, l, r).with_type(t);
      }

      impl::Bitxor_assign*
      expr_factory::make_bitxor_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(bitxor_assigns, l, r).with_type(t);
      }

      impl::Cast*
      expr_factory::make_cast(const ipr::Type& t, const ipr::Expr& e) {
         return casts.make(t, e);
      }

      impl::Call*
      expr_factory::make_call(const ipr::Expr& l, const ipr::Expr_list& r, Optional<ipr::Type> t)
      {
         return make(calls, l, r).with_type(t);
      }

      impl::Coercion*
      expr_factory::make_coercion(const ipr::Expr& l, const ipr::Type& r, const ipr::Type& t)
      {
         return make(coercions, l, r).with_type(t);
      }

      impl::Comma*
      expr_factory::make_comma(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(commas, l, r).with_type(t);
      }

      impl::Const_cast*
      expr_factory::make_const_cast(const ipr::Type& t, const ipr::Expr& e) {
         return ccasts.make(t, e);
      }

      impl::Div*
      expr_factory::make_div(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(divs, l, r).with_type(t);
      }

      impl::Div_assign*
      expr_factory::make_div_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(div_assigns, l, r).with_type(t);
      }

      impl::Dot*
      expr_factory::make_dot(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(dots, l, r).with_type(t);
      }

      impl::Dot_star*
      expr_factory::make_dot_star(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(dot_stars, l, r).with_type(t);
      }

      impl::Dynamic_cast*
      expr_factory::make_dynamic_cast(const ipr::Type& t, const ipr::Expr& e)
      {
         return dcasts.make(t, e);
      }

      impl::Equal*
      expr_factory::make_equal(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(equals, l, r).with_type(t);
      }

      impl::Greater*
      expr_factory::make_greater(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(greaters, l, r).with_type(t);
      }

      impl::Greater_equal*
      expr_factory::make_greater_equal(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(greater_equals, l, r).with_type(t);
      }

      impl::Less*
      expr_factory::make_less(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(lesses, l, r).with_type(t);
      }

      impl::Less_equal*
      expr_factory::make_less_equal(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(less_equals, l, r).with_type(t);
      }

      impl::Literal*
      expr_factory::make_literal(const ipr::Type& t, const ipr::String& s) {
         using rep = impl::Literal::Rep;
         return lits.insert(rep{ t, s }, binary_compare());
      }

      impl::Literal*
      expr_factory::make_literal(const ipr::Type& t, util::word_view w) {
         return make_literal(t, get_string(w));
      }

      impl::Lshift*
      expr_factory::make_lshift(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(lshifts, l, r).with_type(t);
      }

      impl::Lshift_assign*
      expr_factory::make_lshift_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(lshift_assigns, l, r).with_type(t);
      }

      impl::Member_init*
      expr_factory::make_member_init(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(member_inits, l, r).with_type(t);
      }

      impl::Minus*
      expr_factory::make_minus(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(minuses, l, r).with_type(t);
      }

      impl::Minus_assign*
      expr_factory::make_minus_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(minus_assigns, l, r).with_type(t);
      }

      impl::Modulo*
      expr_factory::make_modulo(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(modulos, l, r).with_type(t);
      }

      impl::Modulo_assign*
      expr_factory::make_modulo_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(modulo_assigns, l, r).with_type(t);
      }

      impl::Mul*
      expr_factory::make_mul(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(muls, l, r).with_type(t);
      }

      impl::Mul_assign*
      expr_factory::make_mul_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(mul_assigns, l, r).with_type(t);
      }

      impl::Narrow*
      expr_factory::make_narrow(const ipr::Expr& e, const ipr::Type& t, const ipr::Type& result) {
         return make(narrows, e, t).with_type(result);
      }

      impl::Not_equal*
      expr_factory::make_not_equal(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(not_equals, l, r).with_type(t);
      }

      impl::Or*
      expr_factory::make_or(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(ors, l, r).with_type(t);
      }

      impl::Plus*
      expr_factory::make_plus(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(pluses, l, r).with_type(t);
      }

      impl::Plus_assign*
      expr_factory::make_plus_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(plus_assigns, l, r).with_type(t);
      }

      impl::Pretend*
      expr_factory::make_pretend(const ipr::Expr& e, const ipr::Type& t, const ipr::Type& result) {
         return make(pretends, e, t).with_type(result);
      }

      impl::Qualification*
      expr_factory::make_qualification(const ipr::Expr& e, ipr::Type_qualifiers q, const ipr::Type& t)
      {
         return make(qualifications, e, q).with_type(t);
      }

      impl::Reinterpret_cast*
      expr_factory::make_reinterpret_cast(const ipr::Type& t,
                                          const ipr::Expr& e) {
         return rcasts.make(t, e);
      }

      impl::Scope_ref*
      expr_factory::make_scope_ref(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(scope_refs, l, r).with_type(t);
      }

      impl::Rshift*
      expr_factory::make_rshift(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(rshifts, l, r).with_type(t);
      }

      impl::Rshift_assign*
      expr_factory::make_rshift_assign(const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(rshift_assigns, l, r).with_type(t);
      }

      impl::Template_id*
      expr_factory::make_template_id(const ipr::Expr& n, const ipr::Expr_list& args) {
         using Rep = impl::Template_id::Rep;
         return template_ids.insert(Rep{ n, args }, binary_compare());
      }

      impl::Static_cast*
      expr_factory::make_static_cast(const ipr::Type& t, const ipr::Expr& e) {
         return scasts.make(t, e);
      }

      impl::Widen*
      expr_factory::make_widen(const ipr::Expr& e, const ipr::Type& t, const ipr::Type& result) {
         return make(widens, e, t).with_type(result);
      }

      impl::Binary_fold*
      expr_factory::make_binary_fold(Category_code op, const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(folds, op, l, r).with_type(t);
      }

      impl::Where*
      expr_factory::make_where(const ipr::Region& parent)
      {
         return wheres.make(parent);
      }

      impl::Where_no_decl*
      expr_factory::make_where(const ipr::Expr& main, const ipr::Expr& attendant)
      {
         return where_nodecls.make(main, attendant);
      }

      impl::Static_assert* expr_factory::make_static_assert_expr(const ipr::Expr& e, Optional<ipr::String> s)
      {
         return asserts.make(e, s);
      }

      impl::Instantiation*
      expr_factory::make_instantiation(const ipr::Expr& e, const ipr::Substitution& s)
      {
         return insts.make(e, s);
      }

      impl::New*
      expr_factory::make_new(Optional<ipr::Expr_list> where, const ipr::Construction& expr, Optional<ipr::Type> t)
      {
         return make(news, where, expr).with_type(t);
      }

      impl::Conditional*
      expr_factory::make_conditional(const ipr::Expr& expr, const ipr::Expr& then,
                                     const ipr::Expr& alt, Optional<ipr::Type> t)
      {
         return make(conds, expr, then, alt).with_type(t);
      }

      impl::Mapping*
      expr_factory::make_mapping(const ipr::Region& r, Mapping_level l) {
         return mappings.make(r, l);
      }

      impl::Lambda* expr_factory::make_lambda(const ipr::Region& r, Mapping_level l)
      {
         return lambdas.make(r, l);
      }

      impl::Elementary_substitution*
      expr_factory::make_elementary_substitution(const ipr::Parameter& p, const ipr::Expr& v)
      {
         return elem_substs.make(p, v);
      }

      impl::General_substitution* expr_factory::make_general_substitution()
      {
         return gen_substs.make();
      }

      // -- impl::Lexicon --

      const ipr::Linkage& Lexicon::c_linkage() const { return impl::c_link; }
      const ipr::Linkage& Lexicon::cxx_linkage() const { return impl::cxx_link; }

      Lexicon::Lexicon() { }
      Lexicon::~Lexicon() { }

      const ipr::Literal&
      Lexicon::get_literal(const ipr::Type& t, util::word_view w) {
         return get_literal(t, get_string(w));
      }

      const ipr::Literal&
      Lexicon::get_literal(const ipr::Type& t, const ipr::String& s) {
         return *make_literal(t, s);
      }

      const ipr::Type& Lexicon::void_type() const {  return impl::builtin(Fundamental::Void);  }
      const ipr::Type& Lexicon::bool_type() const { return impl::builtin(Fundamental::Bool); }
      const ipr::Type& Lexicon::char_type() const { return impl::builtin(Fundamental::Char); }
      const ipr::Type& Lexicon::schar_type() const { return impl::builtin(Fundamental::Schar); }
      const ipr::Type& Lexicon::uchar_type() const { return impl::builtin(Fundamental::Uchar); }
      const ipr::Type& Lexicon::wchar_t_type() const { return impl::builtin(Fundamental::Wchar_t); }
      const ipr::Type& Lexicon::char8_t_type() const { return impl::builtin(Fundamental::Char8_t); }
      const ipr::Type& Lexicon::char16_t_type() const { return impl::builtin(Fundamental::Char16_t); }
      const ipr::Type& Lexicon::char32_t_type() const { return impl::builtin(Fundamental::Char32_t); }
      const ipr::Type& Lexicon::short_type() const { return impl::builtin(Fundamental::Short); }
      const ipr::Type& Lexicon::ushort_type() const { return impl::builtin(Fundamental::Ushort); }
      const ipr::Type& Lexicon::int_type() const { return impl::builtin(Fundamental::Int); }
      const ipr::Type& Lexicon::uint_type() const { return impl::builtin(Fundamental::Uint); }
      const ipr::Type& Lexicon::long_type() const { return impl::builtin(Fundamental::Long); }
      const ipr::Type& Lexicon::ulong_type() const { return impl::builtin(Fundamental::Ulong); }
      const ipr::Type& Lexicon::long_long_type() const { return impl::builtin(Fundamental::Long_long); }
      const ipr::Type& Lexicon::ulong_long_type() const { return impl::builtin(Fundamental::Ulong_long); }
      const ipr::Type& Lexicon::float_type() const { return impl::builtin(Fundamental::Float); }
      const ipr::Type& Lexicon::double_type() const { return impl::builtin(Fundamental::Double); }
      const ipr::Type& Lexicon::long_double_type() const { return impl::builtin(Fundamental::Long_double); }
      const ipr::Type& Lexicon::ellipsis_type() const { return impl::builtin(Fundamental::Ellipsis); }
      const ipr::Type& Lexicon::typename_type() const { return impl::builtin(Fundamental::Typename); }
      const ipr::Type& Lexicon::class_type() const { return impl::builtin(Fundamental::Class); }
      const ipr::Type& Lexicon::union_type() const { return impl::builtin(Fundamental::Union); }
      const ipr::Type& Lexicon::enum_type() const { return impl::builtin(Fundamental::Enum); }
      const ipr::Type& Lexicon::namespace_type() const { return impl::builtin(Fundamental::Namespace); }

      const ipr::Symbol& Lexicon::false_value() const { return impl::false_cst; }
      const ipr::Symbol& Lexicon::true_value() const { return impl::true_cst; }
      const ipr::Symbol& Lexicon::nullptr_value() const { return impl::nullptr_cst; }
      const ipr::Symbol& Lexicon::default_value() const { return impl::default_cst; }
      const ipr::Symbol& Lexicon::delete_value() const { return impl::delete_cst; }

      const ipr::Template_id&
      Lexicon::get_template_id(const ipr::Expr& t, const ipr::Expr_list& a) {
         return *expr_factory::make_template_id(t, a);
      }

      impl::Phased_evaluation* Lexicon::make_asm(const ipr::String& s)
      {
         return make_phased_evaluation(*make_asm_expr(s), Phases::Code_generation);
      }

      impl::Phased_evaluation* Lexicon::make_static_assert(const ipr::Expr& e, Optional<ipr::String> s)
      {
         return make_phased_evaluation(*make_static_assert_expr(e, s), Phases::Elaboration);
      }

      impl::Mapping*
      Lexicon::make_mapping(const ipr::Region& r, Mapping_level l) {
         return expr_factory::make_mapping(r, l);
      }

      // -- impl::Interface_unit --
      Interface_unit::Interface_unit(impl::Lexicon& l, const ipr::Module& m)
            : basic_unit<ipr::Interface_unit>{ l, m }
      { }

      const ipr::Sequence<ipr::Module>&
      Interface_unit::exported_modules() const {
         return modules_exported;
      }

      const ipr::Sequence<ipr::Decl>&
      Interface_unit::exported_declarations() const {
         return decls_exported;
      }

                                // -- impl::Module --
      Module::Module(impl::Lexicon& l) : lexicon{ l }, iface{l, *this }
      { }

      const ipr::Module_name& Module::name() const { return stems; }

      const ipr::Interface_unit& Module::interface_unit() const {
         return iface;
      }

      const ipr::Sequence<ipr::Module_unit>&
      Module::implementation_units() const { return units; }

      impl::Module_unit* Module::make_unit() {
         return units.push_back(lexicon, *this);
      }
}


/*
#include <ipr/impl>
#include <ipr/io>
#include <iostream>

int main()
{
   using namespace ipr;
   impl::Lexicon lexicon { };
   impl::Translation_unit unit { lexicon };   // current translation unit

   impl::Scope* global_scope = unit.global_scope();

   // Build the variable's name,
   auto& name = lexicon.get_identifier(u8"bufsz");
   // then its type,
   auto& type = lexicon.get_qualified(Type_qualifiers::Const, lexicon.int_type());
   // and the actual impl::Var node,
   impl::Var* var = global_scope->make_var(name, type);
   // set its initializer,
   var->init = lexicon.make_literal(lexicon.int_type(), u8"1024");
   // and inject it into its scope.

   // Print out the whole translation unit
   Printer pp { std::cout };
   pp << unit;
   std::cout << std::endl;

}

*/
