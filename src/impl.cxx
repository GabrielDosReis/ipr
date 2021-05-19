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
#include <string>
namespace ipr {
   const String& String::empty_string()
   {
      struct Empty_string final : impl::Node<String> {
         Index size() const { return 0; }
         iterator begin() const { return ""; }
         iterator end() const { return begin(); }
      };

      static constexpr Empty_string empty { };
      return empty;
   }


   namespace impl {

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

      const ipr::Sequence<ipr::Identifier>& Module_name::stems() const {
         return components;
      }

      struct scope_datum::comp {
         int operator()(const scope_datum& lhs, const scope_datum& rhs) const
         {
            return compare(lhs.scope_pos, rhs.scope_pos);
         }

         int operator()(Decl_position pos, const scope_datum& s) const
         {
            return compare(pos, s.scope_pos);
         }

         int operator()(const scope_datum& s, Decl_position pos) const
         {
            return compare(s.scope_pos, pos);
         }
      };

      // -- impl::Symbol --
      Symbol::Symbol(const ipr::Name& n)
         : Unary_expr<ipr::Symbol>{ n }
      { }

      Symbol::Symbol(const ipr::Name& n, const ipr::Type& t)
         : Unary_expr<ipr::Symbol>{ n }
      {
         constraint = &t;
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
            : Base(this), overload_entry(t),
              primary(0),home(0),
              overload(ovl)
      { }

      // -------------------------
      // -- impl::decl_sequence --
      // -------------------------

      decl_sequence::Index decl_sequence::size() const {
         return decls.size();
      }

      const ipr::Decl&
      decl_sequence::get(Index i) const {
         scope_datum* result = decls.find(Decl_position{ i }, scope_datum::comp());
         return *util::check(result)->decl;
      }

      void
      decl_sequence::insert(scope_datum* s) {
         decls.insert(s, scope_datum::comp());
      }

      // --------------------
      // -- impl::Overload --
      // --------------------

      Overload::Overload(const ipr::Name& n)
            : name(n), where(0)
      { }

      Overload::Index Overload::size() const {
         return entries.size();
      }

      const ipr::Decl&
      Overload::get(Index i) const {
         return *masters.at(i)->decl;
      }

      const ipr::Sequence<ipr::Decl>&
      Overload::operator[](const ipr::Type& t) const {
         overload_entry* master = lookup(t);
         return util::check(master)->declset;
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

      // ------------------------
      // -- singleton_overload --
      // ------------------------

      singleton_overload::singleton_overload(const ipr::Decl& d)
            : seq(d)
      { }

      const ipr::Type&
      singleton_overload::type() const {
         return seq.datum.type();
      }

      singleton_overload::Index
      singleton_overload::size() const {
         return 1;
      }

      const ipr::Decl&
      singleton_overload::get(Index i) const {
         if (i != 1)
            throw std::domain_error("singleton_overload::get: out-of-range ");
         return seq.datum;
      }

      const ipr::Sequence<ipr::Decl>&
      singleton_overload::operator[](const ipr::Type& t) const {
         if (&t != &seq.datum.type())
            throw std::domain_error("invalid type subscription");
         return seq;
      }

      // --------------------------
      // -- impl::empty_overload --
      // --------------------------

      const ipr::Type&
      empty_overload::type() const {
         throw std::domain_error("empty_overload::type");
      }

      empty_overload::Index empty_overload::size() const {
         return 0;
      }

      const ipr::Decl&
      empty_overload::get(Index) const {
         throw std::domain_error("impl::empty_overload::get");
      }

      const ipr::Sequence<ipr::Decl>&
      empty_overload::operator[](const ipr::Type&) const {
         throw std::domain_error("impl::empty_overload::operator[]");
      }

      // -----------------
      // -- impl::Rname --
      // -----------------
      Rname::Rname(Rep r)
            : impl::Ternary<impl::Node<ipr::Rname>>(r) { }

      const ipr::Type&
      Rname::type() const {
         return rep.first;
      }

      namespace {
         // Helper function for building expression nodes with type assignment.
         template<typename T, typename... Args>
         auto make(stable_farm<T>& factory, const Args&... args)
         {
            struct Holder {
               explicit Holder(T* x) : impl{ x } { }

               T* with_type(const ipr::Type& t) 
               {
                  impl->constraint = &t;
                  return impl;
               }

               T* with_type(Optional<ipr::Type> t) 
               {
                  impl->constraint = t;
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

      Alias::Alias() : aliasee(0), lexreg(0)
      { }

      Optional<ipr::Expr> Alias::initializer() const {
         return { aliasee };
      }

      const ipr::Region&
      Alias::lexical_region() const {
         return *util::check(lexreg);
      }

      // --------------------
      // -- impl::Bitfield --
      // --------------------

      Bitfield::Bitfield() : length(0), member_of(0), init(0)
      { }

      const ipr::Expr&
      Bitfield::precision() const {
         return *util::check(length);
      }

      const ipr::Udt<ipr::Decl>&
      Bitfield::membership() const {
         return *util::check(member_of);
      }

      Optional<ipr::Expr> Bitfield::initializer() const {
         return init;
      }

      const ipr::Region&
      Bitfield::lexical_region() const {
         return util::check(member_of)->region();
      }

      const ipr::Region&
      Bitfield::home_region() const {
         return util::check(member_of)->region();
      }

      // ---------------------
      // -- impl::Base_type --
      // ---------------------

      Base_type::Base_type(const ipr::Type& t, const ipr::Region& r, Decl_position p)
            : base(t), where(r), scope_pos(p)
      { }

      const ipr::Type&
      Base_type::type() const {
         return base;
      }

      const ipr::Region&
      Base_type::lexical_region() const {
         return where;
      }

      const ipr::Region&
      Base_type::home_region() const {
         return where;
      }

      Decl_position Base_type::position() const {
         return scope_pos;
      }

      Optional<ipr::Expr>
      Base_type::initializer() const {
         throw std::domain_error("impl::Base_type::initializer");
      }

      // ----------------------
      // -- impl::Enumerator --
      // ----------------------

      Enumerator::Enumerator(const ipr::Name& n, const ipr::Enum& t, Decl_position p)
            : id(n), constraint(t), scope_pos(p), where(0), init(0)
      { }

      const ipr::Name&
      Enumerator::name() const {
         return id;
      }

      const ipr::Region&
      Enumerator::lexical_region() const {
         return *util::check(where);
      }

      const ipr::Region&
      Enumerator::home_region() const {
         return *util::check(where);
      }

      const ipr::Enum&
      Enumerator::membership() const {
         return constraint;
      }

      Decl_position Enumerator::position() const {
         return scope_pos;
      }

      Optional<ipr::Expr> Enumerator::initializer() const {
         return init;
      }

      // -----------------
      // -- impl::Field --
      // -----------------

      Field::Field()
            : member_of(0), init(0)
      { }

      Optional<ipr::Expr> Field::initializer() const {
         return init;
      }

      const ipr::Udt<ipr::Decl>&
      Field::membership() const {
         return *util::check(member_of);
      }

      const ipr::Region&
      Field::lexical_region() const {
         return util::check(member_of)->region();
      }

      const ipr::Region&
      Field::home_region() const {
         return util::check(member_of)->region();
      }

      // -------------------
      // -- impl::Fundecl --
      // -------------------

      Fundecl::Fundecl()
            : member_of(nullptr), data{ }, lexreg(nullptr)
      { }

      const ipr::Parameter_list& Fundecl::parameters() const {
         if (data.index() == 0)
            return *util::check(data.parameters());
         return util::check(data.mapping())->parameters;
      }

      Optional<ipr::Mapping> Fundecl::mapping() const {
         if (data.index() == 0)
            return { };
         return { data.mapping() };
      }

      const ipr::Udt<ipr::Decl>&
      Fundecl::membership() const {
         return *util::check(member_of);
      }

      Optional<ipr::Expr> Fundecl::initializer() const {
         if (data.index() == 0)
            return { };
         return { data.mapping() };
      }

      const ipr::Region&
      Fundecl::lexical_region() const {
         return *util::check(lexreg);
      }

      // --------------------
      // -- impl::Template --
      // --------------------

      Template::Template() : member_of(0), init(0), lexreg(0) { }

      const ipr::Template&
      Template::primary_template() const {
         return *util::check(util::check(decl_data.master_data)->primary);
      }

      const ipr::Sequence<ipr::Decl>&
      Template::specializations() const {
         return util::check(decl_data.master_data)->specs;
      }

      const ipr::Mapping&
      Template::mapping() const {
         return *util::check(init);
      }

      Optional<ipr::Expr> Template::initializer() const {
         return { util::check(init)->body };
      }

      const ipr::Region&
      Template::lexical_region() const {
         return *util::check(lexreg);
      }

      // ---------------------
      // -- impl::Parameter --
      // ---------------------

      Parameter::Parameter(const ipr::Name& n, const impl::Rname& rn)
            :id(n), abstract_name(rn),
             where(0), init(0)
      { }

      const ipr::Name&
      Parameter::name() const {
         return id;
      }

      const ipr::Type&
      Parameter::type() const {
         return abstract_name.rep.first;
      }

      const ipr::Region&
      Parameter::home_region() const {
         return util::check(where)->region();
      }

      const ipr::Region&
      Parameter::lexical_region() const {
         return util::check(where)->region();
      }

      const ipr::Parameter_list&
      Parameter::membership() const {
         return *util::check(where);
      }

      Decl_position Parameter::position() const {
         return abstract_name.rep.third;
      }

      Optional<ipr::Expr> Parameter::initializer() const {
         return init;
      }

      // --------------------
      // -- impl::Typedecl --
      // --------------------

      Typedecl::Typedecl() : init{ }, member_of(0), lexreg(0)
      { }

      Optional<ipr::Expr> Typedecl::initializer() const { return init; }

      const ipr::Expr&
      Typedecl::membership() const {
         return *util::check(member_of);
      }

      const ipr::Region&
      Typedecl::lexical_region() const {
         // return util::check(member_of)->region();
         // 31OCT08 - PIR: since a Typedecl has a lexreg field
         //    I assume that it should be returned by lexical_region()
         return *util::check(lexreg);
      }

      // ---------------
      // -- impl::Var --
      // ---------------

      Var::Var() : init(0), lexreg(0)
      { }

      Optional<ipr::Expr> Var::initializer() const {
         return init;
      }

      const ipr::Region&
      Var::lexical_region() const {
         return *util::check(lexreg);
      }

      // -----------------
      // -- impl::Block --
      // -----------------

      Block::Block(const ipr::Region& pr, const ipr::Type& t)
            : region(&pr, t)
      {
         // >>>> pmp 16jun08
         // regions' owners are set typically by IPR
         region.owned_by = this;
         // <<<< pmp 16jun08
      }

      const ipr::Type&
      Block::type() const {
         return region.scope.type();
      }

      const ipr::Scope&
      Block::members() const {
         return region.scope;
      }

      const ipr::Sequence<ipr::Stmt>&
      Block::body() const {
         return stmt_seq;
      }

      const ipr::Sequence<ipr::Handler>&
      Block::handlers() const {
         return handler_seq;
      }

      // ---------------
      // -- impl::For --
      // ---------------

      For::For() : init(0), cond(0), inc(0), stmt(0) { }

      const ipr::Type&
      For::type() const {
         return util::check(stmt)->type();
      }

      const ipr::Expr&
      For::initializer() const {
         return *util::check(init);
      }

      const ipr::Expr&
      For::condition() const {
         return *util::check(cond);
      }

      const ipr::Expr&
      For::increment() const {
         return *util::check(inc);
      }

      const ipr::Stmt&
      For::body() const {
         return *util::check(stmt);
      }

      // ------------------
      // -- impl::For_in --
      // ------------------
      For_in::For_in() : var(), seq(), stmt() { }

      const ipr::Var&
      For_in::variable() const {
         return *util::check(var);
      }

      const ipr::Expr&
      For_in::sequence() const {
         return *util::check(seq);
      }

      const ipr::Type&
      For_in::type() const {
         return util::check(stmt)->type();
      }

      const ipr::Stmt&
      For_in::body() const {
         return *util::check(stmt);
      }

      // -----------------
      // -- impl::Break --
      // -----------------

      Break::Break() : stmt(0) { }

      const ipr::Stmt&
      Break::from() const {
         return *util::check(stmt);
      }

      // --------------------
      // -- impl::Continue --
      // --------------------

      Continue::Continue() : stmt(0) { }

      const ipr::Stmt&
      Continue::iteration() const {
         return *util::check(stmt);
      }

      // ------------------------
      // -- impl::stmt_factory --
      // ------------------------

      impl::Break* stmt_factory::make_break(const ipr::Type& t)
      {
         return make(breaks).with_type(t);
      }

      impl::Continue* stmt_factory::make_continue(const ipr::Type& t)
      {
         return make(continues).with_type(t);
      }

      impl::Empty_stmt*
      stmt_factory::make_empty_stmt(const ipr::Type& t) {
         return empty_stmts.make(*make_phantom(t));
      }

      impl::Block*
      stmt_factory::make_block(const ipr::Region& pr, const ipr::Type& t) {
         return blocks.make(pr, t);
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

      impl::If*
      stmt_factory::make_if(const ipr::Expr& c, const ipr::Stmt& s) {
         return ifs.make(c, s, nullptr);
      }

      impl::If*
      stmt_factory::make_if(const ipr::Expr& c, const ipr::Stmt& t, const ipr::Stmt& f) {
         return ifs.make(c, t, &f);
      }

      impl::Switch* stmt_factory::make_switch()
      {
         return switches.make();
      }

      impl::Handler*
      stmt_factory::make_handler(const ipr::Decl& d, const ipr::Block& b) {
         return handlers.make(d, b);
      }

      impl::Labeled_stmt*
      stmt_factory::make_labeled_stmt(const ipr::Expr& l, const ipr::Stmt& s)
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

      // -----------------
      // -- impl::Class --
      // -----------------

      Class::Class(const ipr::Region& pr, const ipr::Type& t)
            : impl::Udt<ipr::Class>(&pr, t),
              base_subobjects(pr) {
         base_subobjects.owned_by = this;
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

      // ------------------
      // -- impl::String --
      // ------------------
      String::String(const util::string& s) : text(s)
      { }

      String::Index String::size() const {
         return text.size();
      }

      const char*
      String::begin() const {
         return text.begin();
      }

      const char*
      String::end() const {
         return text.end();
      }

      // --------------------------
      // -- impl::Parameter_list --
      // --------------------------

      Parameter_list::Parameter_list(const ipr::Region& p)
            : parms(p)
      { }

      const ipr::Product& Parameter_list::type() const { return parms.scope.type(); }

      const ipr::Region& Parameter_list::region() const { return parms; }

      const ipr::Sequence<ipr::Parameter>&
      Parameter_list::members() const {
         return parms.scope.decls.seq;
      }

      impl::Parameter*
      Parameter_list::add_member(const ipr::Name& n, const impl::Rname& rn)
      {
         impl::Parameter* param = parms.scope.push_back(n, rn);
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
    	  int operator()(const ipr::Type& lhs, const ipr::Unary<Cat,Operand>& rhs) const
		  {
              return compare(lhs, rhs.operand());
		  }
	  };
      // <<<< Yuriy Solodkyy: 2008/07/10

      impl::Array*
      type_factory::make_array(const ipr::Type& t, const ipr::Expr& b)
      {
         using rep = impl::Array::Rep;
         return arrays.insert(rep{ t, b }, binary_compare());
      }

      impl::Qualified*
      type_factory::make_qualified(ipr::Type_qualifier cv, const ipr::Type& t)
      {
         // It is an error to call this function if there is no real
         // qualified.
         if (cv == ipr::Type_qualifier::None)
            throw std::domain_error
               ("type_factoy::make_qualified: no qualifier");

         using rep = impl::Qualified::Rep;
         return qualifieds.insert(rep{ cv, t }, binary_compare());
      }


      impl::Decltype*
      type_factory::make_decltype(const ipr::Expr& e)
      {
         return decltypes.insert(e, unary_compare());
      }

      impl::As_type*
      type_factory::make_as_type(const ipr::Expr& e, const ipr::Linkage& l)
      {
         using Rep = impl::As_type::Rep;
         return type_refs.insert(Rep{ e, l }, binary_compare());
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

      struct quaternary_compare {
         template<class T>
         int operator()(const Quaternary<T>& lhs,
                        const typename Quaternary<T>::Rep& rhs) const
         {
            if (int cmp = compare(lhs.rep.first, rhs.first)) return cmp;
            if (int cmp = compare(lhs.rep.second, rhs.second)) return cmp;
            if (int cmp = compare(lhs.rep.third, rhs.third)) return cmp;

            return compare(lhs.rep.fourth, rhs.fourth);
         }

         template<class T>
         int operator()(const typename Quaternary<T>::Rep& lhs,
                        const Quaternary<T>& rhs) const
         {
            if (int cmp = compare(lhs.first, rhs.rep.first)) return cmp;
            if (int cmp = compare(lhs.second, rhs.rep.second)) return cmp;
            if (int cmp = compare(lhs.third, rhs.rep.third)) return cmp;

            return compare(lhs.fourth, rhs.rep.fourth);
         }
      };

      impl::Function*
      type_factory::make_function(const ipr::Product& s, const ipr::Type& t,
                                  const ipr::Sum& e, const ipr::Linkage& l)
      {
         using rep = impl::Function::Rep;
         return functions.insert(rep{ s, t, e, l }, quaternary_compare());
      }

      impl::Pointer*
      type_factory::make_pointer(const ipr::Type& t)
      {
         // >>>> Yuriy Solodkyy: 2008/07/10
         // Fixed pointer comparison for unification
         return pointers.insert(t, unified_type_compare());
         // <<<< Yuriy Solodkyy: 2008/07/10
      }

      impl::Product*
      type_factory::make_product(const ipr::Sequence<ipr::Type>& seq) {
         return products.insert(seq, unary_compare());
      }

      impl::Ptr_to_member*
      type_factory::make_ptr_to_member(const ipr::Type& c, const ipr::Type& t)
      {
         using rep = impl::Ptr_to_member::Rep;
         return member_ptrs.insert(rep{ c, t }, binary_compare());
      }

      impl::Reference*
      type_factory::make_reference(const ipr::Type& t) {
         return references.insert(t, unified_type_compare());
      }

      impl::Rvalue_reference*
      type_factory::make_rvalue_reference(const ipr::Type& t) {
         return refrefs.insert(t, unified_type_compare());
      }

      impl::Sum*
      type_factory::make_sum(const ipr::Sequence<ipr::Type>& seq) {
         return sums.insert(seq, unary_compare());
      }

      impl::Forall*
      type_factory::make_forall(const ipr::Product& s, const ipr::Type& t) {
         using rep = impl::Forall::Rep;
         return foralls.insert(rep{ s, t }, binary_compare());
      }

      impl::Enum*
      type_factory::make_enum(const ipr::Region& pr, Enum::Kind k) {
         return enums.make(pr, k);
      }

      impl::Class*
      type_factory::make_class(const ipr::Region& pr, const ipr::Type& t) {
         return classes.make(pr, t);
      }

      impl::Union*
      type_factory::make_union(const ipr::Region& pr, const ipr::Type& t) {
         return unions.make(&pr, t);
      }

      impl::Namespace*
      type_factory::make_namespace(const ipr::Region* pr, const ipr::Type& t)
      {
         return namespaces.make(pr, t);
      }

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

      const ipr::Type&
      Id_expr::type() const {
         return constraint.get();
      }

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

      // -------------------
      // -- impl::Mapping --
      // -------------------

      Mapping::Mapping(const ipr::Region& pr, Mapping_level d)
            : parameters(pr), value_type(0),
              body(0), nesting_level(d)
      {
        // 31Oct08 added by PIR to avoid exceptions,
        //   when querying the region (parameters) for its owner
        parameters.parms.owned_by = this;
      }

      const ipr::Parameter_list&
      Mapping::params() const {
         return parameters;
      }

      const ipr::Type&
      Mapping::result_type() const {
         return value_type.get();
      }

      const ipr::Expr&
      Mapping::result() const {
         return body.get();
      }

      Mapping_level Mapping::depth() const {
         return nesting_level;
      }

      impl::Parameter*
      Mapping::param(const ipr::Name& n, const impl::Rname& rn) {
         return parameters.add_member(n, rn);
      }

      // -------------------------------
      // -- impl::Scope --
      // -------------------------------
      Scope::Scope(const ipr::Region& r, const ipr::Type& t) : region(r)
      {
         decls.constraint = &t;
      }

      const ipr::Type&
      Scope::type() const {
         return decls;
      }

      const ipr::Sequence<ipr::Decl>&
      Scope::members() const {
         return decls.seq;
      }

      const ipr::Overload&
      Scope::operator[](const ipr::Name& n) const {
         if (impl::Overload* ovl = overloads.find(n, node_compare()))
            return *ovl;
         return missing;
      }

      template<class T>
      inline void
      Scope::add_member(T* decl) {
         decl->decl_data.scope_pos = Decl_position{ decls.seq.size() };
         decls.seq.insert(&decl->decl_data);
      }

      impl::Alias*
      Scope::make_alias(const ipr::Name& n, const ipr::Expr& i) {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(i.type());

         if (master == 0) {
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

         if (master == 0) {
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

         if (master == 0) {
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

         if (master == 0) {
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
         impl::Typedecl* decl;
         if (master == 0)       // no, this is the first declaration
             decl = typedecls.declare(ovl, t);
         else                   // just re-declare.
            decl = typedecls.redeclare(master);
         add_member(decl);      // remember we saw a declaration.
         return decl;
      }

      impl::Fundecl*
      Scope::make_fundecl(const ipr::Name& n, const ipr::Function& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == 0) {
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

         if (master == 0) {
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

         if (master == 0) {
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

      Region::Region(const ipr::Region* pr, const ipr::Type& t)
            : parent(pr), owned_by(0), scope(*this, t)
      { }

      const ipr::Region&
      Region::enclosing() const {
         return *util::check(parent);
      }

      const ipr::Scope&
      Region::bindings() const {
         return scope;
      }

      const Region::location_span&
      Region::span() const {
         return extent;
      }

      const ipr::Expr&
      Region::owner() const {
         return *util::check(owned_by);
      }

      Region*
      Region::make_subregion() {
         return subregions.make(this, scope.type().type());
      }


      // ------------------------
      // -- impl::expr_factory --
      // ------------------------

      const ipr::String&
      expr_factory::get_string(const char* s) {
         return get_string(s,std::strlen(s));
      }

      const ipr::String&
      expr_factory::get_string(const std::string& s) {
         return get_string(s.data(), s.size());
      }

      struct string_comp {
         using proxy = std::pair<const char*, int>;

         struct char_compare {
            int operator()(unsigned char lhs, unsigned char rhs) const
            {
               return compare(lhs, rhs);
            }
         };

         int
         operator()(const proxy& lhs, const impl::String& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.first, lhs.first + lhs.second,
                rhs.begin(), rhs.end(), char_compare());
         }

         int
         operator()(const impl::String& lhs, const proxy& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.begin(), lhs.end(),
                rhs.first, rhs.first + rhs.second, char_compare());
         }

         int
         operator()(const util::string& lhs, const impl::String& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.data, lhs.data + lhs.length,
                rhs.begin(), rhs.end(), char_compare());
         }

         int
         operator()(const impl::String& lhs, const util::string& rhs) const
         {
            return util::lexicographical_compare()
               (lhs.begin(), lhs.end(),
                rhs.data, rhs.data + rhs.length, char_compare());
         }
      };

      const ipr::String&
      expr_factory::get_string(const char* s, int n) {
         const impl::String* item = strings.find(std::make_pair(s, n),
                                                 string_comp());
         if (item == 0)
            item = strings.insert(*string_pool.make_string(s, n),
                                  string_comp());

         return *item;
      }


      // -- Language linkage
      const ipr::Linkage&
      expr_factory::get_linkage(const char* s) {
         return get_linkage(get_string(s));
      }

      const ipr::Linkage&
      expr_factory::get_linkage(const std::string& s) {
         return get_linkage(get_string(s));
      }

      const ipr::Linkage&
      expr_factory::get_linkage(const ipr::String& lang) {
         return *linkages.insert(lang, unary_compare());
      }

      const ipr::Symbol&
      expr_factory::get_symbol(const ipr::Name& n, const ipr::Type& t)
      {
         const auto comparator = [](auto& x, auto& y) {
            if (auto cmp = compare(x.name(), x.name()))
               return cmp;
            return compare(x.type(), y.type());
         };

         return *symbols.insert(impl::Symbol{ n, t }, comparator);
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

      impl::Complement*
      expr_factory::make_complement(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(complements, e).with_type(t);
      }

      impl::Conversion*
      expr_factory::make_conversion(const ipr::Type& t) {
         return convs.insert(t, unary_compare());
      }

      impl::Ctor_name*
      expr_factory::make_ctor_name(const ipr::Type& t) {
         return ctors.insert(t, unary_compare());
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

      impl::Dtor_name*
      expr_factory::make_dtor_name(const ipr::Type& t) {
         return dtors.insert(t, unary_compare());
      }

      impl::Expr_list*
      expr_factory::make_expr_list() {
         return xlists.make();
      }

      impl::Identifier*
      expr_factory::make_identifier(const ipr::String& s) {
         return ids.insert(s, id_compare());
      }

      impl::Identifier*
      expr_factory::make_identifier(const char* s) {
         return make_identifier(get_string(s));
      }

      impl::Identifier*
      expr_factory::make_identifier(const std::string& s) {
         return make_identifier(get_string(s));
      }

      impl::Suffix*
      expr_factory::make_suffix(const ipr::Identifier& s) {
         return suffixes.insert(s, unary_compare());
      }

      impl::Guide_name*
      expr_factory::make_guide_name(const ipr::Template& m) {
         return guide_ids.insert(m, unary_compare());
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

      impl::Operator*
      expr_factory::make_operator(const ipr::String& s) {
         return ops.insert(s, unary_compare());
      }

      impl::Operator*
      expr_factory::make_operator(const char* s) {
         return make_operator(get_string(s));
      }

      impl::Operator*
      expr_factory::make_operator(const std::string& s) {
         return make_operator(get_string(s));
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

      impl::Type_id*
      expr_factory::make_type_id(const ipr::Type& t) {
         return type_ids.insert(t, unary_compare());
      }

      impl::Sizeof*
      expr_factory::make_sizeof(const ipr::Expr& t) {
         return sizeofs.insert(t, unary_compare());
      }

      impl::Args_cardinality*
      expr_factory::make_args_cardinality(const ipr::Expr& e, Optional<ipr::Type> t)
      {
         return make(cardinalities, e).with_type(t);
      }

      impl::Typeid*
      expr_factory::make_typeid(const ipr::Expr& t) {
         return xtypeids.insert(t, unary_compare());
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
      expr_factory::make_literal(const ipr::Type& t, const char* s) {
         return make_literal(t, get_string(s));
      }

      impl::Literal*
      expr_factory::make_literal(const ipr::Type& t, const std::string& s) {
         return make_literal(t, get_string(s));
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
      expr_factory::make_template_id(const ipr::Name& n,
                                     const ipr::Expr_list& args) {
         using Rep = impl::Template_id::Rep;
         return template_ids.insert(Rep{ n, args }, binary_compare());
      }

      impl::Static_cast*
      expr_factory::make_static_cast(const ipr::Type& t, const ipr::Expr& e) {
         return scasts.make(t, e);
      }

      impl::Qualification*
      expr_factory::make_qualification(const ipr::Expr& e, ipr::Type_qualifier q, const ipr::Type& t)
      {
         return make(qualifications, e, q).with_type(t);
      }

      impl::Binary_fold*
      expr_factory::make_binary_fold(Category_code op, const ipr::Expr& l, const ipr::Expr& r, Optional<ipr::Type> t)
      {
         return make(folds, op, l, r).with_type(t);
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

      impl::Rname*
      expr_factory::rname_for_next_param(const impl::Mapping& map,
                                         const ipr::Type& t) {
         using Rep = impl::Rname::Rep;
         Decl_position pos { map.parameters.size() };
         return rnames.insert(Rep{ t, map.nesting_level, pos }, ternary_compare());
      }

      impl::Mapping*
      expr_factory::make_mapping(const ipr::Region& r, Mapping_level l) {
         return mappings.make(r, l);
      }


      // -- impl::Lexicon --

      const ipr::Linkage& Lexicon::cxx_linkage() const {
         return const_cast<Lexicon*>(this)->get_linkage("C++");
      }

      const ipr::Linkage& Lexicon::c_linkage() const {
         return const_cast<Lexicon*>(this)->get_linkage("C");
      }

      void Lexicon::record_builtin_type(const ipr::As_type& t) {
         builtin_map.insert(t, unary_compare());
      }

      Lexicon::Lexicon()
            : anytype(get_identifier("typename"), cxx_linkage(), anytype),
              classtype(get_identifier("class"), cxx_linkage(), anytype),
              uniontype(get_identifier("union"), cxx_linkage(), anytype),
              enumtype(get_identifier("enum"), cxx_linkage(), anytype),
              namespacetype(get_identifier("namespace"), cxx_linkage(), anytype),

              voidtype(get_identifier("void"), cxx_linkage(), anytype),
              booltype(get_identifier("bool"), cxx_linkage(), anytype),
              chartype(get_identifier("char"), cxx_linkage(), anytype),
              schartype(get_identifier("signed char"), cxx_linkage(), anytype),
              uchartype(get_identifier("unsigned char"), cxx_linkage(), anytype),
              wchar_ttype(get_identifier("wchar_t"), cxx_linkage(), anytype),
              char8_ttype(get_identifier("char8_t"), cxx_linkage(), anytype),
              char16_ttype(get_identifier("char16_t"), cxx_linkage(), anytype),
              char32_ttype(get_identifier("char32_t"), cxx_linkage(), anytype),
              shorttype(get_identifier("short"), cxx_linkage(), anytype),
              ushorttype(get_identifier("unsigned short"),
                         cxx_linkage(), anytype),
              inttype(get_identifier("int"), cxx_linkage(), anytype),
              uinttype(get_identifier("unsigned int"), cxx_linkage(), anytype),
              longtype(get_identifier("long"), cxx_linkage(), anytype),
              ulongtype(get_identifier("unsigned long"),
                        cxx_linkage(), anytype),
              longlongtype(get_identifier("long long"), cxx_linkage(), anytype),
              ulonglongtype(get_identifier("unsigned long long"),
                            cxx_linkage(), anytype),
              floattype(get_identifier("float"), cxx_linkage(), anytype),
              doubletype(get_identifier("double"), cxx_linkage(), anytype),
              longdoubletype(get_identifier("long double"),
                             cxx_linkage(), anytype),
              ellipsistype(get_identifier("..."), cxx_linkage(), anytype),
              null(get_identifier("nullptr"), get_decltype(null))
      {
         record_builtin_type(anytype);
         record_builtin_type(classtype);
         record_builtin_type(uniontype);
         record_builtin_type(enumtype);
         record_builtin_type(namespacetype);

         record_builtin_type(voidtype);

         record_builtin_type(booltype);

         record_builtin_type(chartype);
         record_builtin_type(schartype);
         record_builtin_type(uchartype);
         record_builtin_type(wchar_ttype);
         record_builtin_type(char8_ttype);
         record_builtin_type(char16_ttype);
         record_builtin_type(char32_ttype);

         record_builtin_type(shorttype);
         record_builtin_type(ushorttype);

         record_builtin_type(inttype);
         record_builtin_type(uinttype);

         record_builtin_type(longtype);
         record_builtin_type(ulongtype);

         record_builtin_type(longlongtype);
         record_builtin_type(ulonglongtype);

         record_builtin_type(floattype);
         record_builtin_type(doubletype);
         record_builtin_type(longdoubletype);

         record_builtin_type(ellipsistype);
      }

      Lexicon::~Lexicon() { }

      const ipr::Literal&
      Lexicon::get_literal(const ipr::Type& t, const char* s) {
         return get_literal(t, get_string(s));
      }

      const ipr::Literal&
      Lexicon::get_literal(const ipr::Type& t, const std::string& s) {
         return get_literal(t, get_string(s));
      }

      const ipr::Literal&
      Lexicon::get_literal(const ipr::Type& t, const ipr::String& s) {
         return *make_literal(t, s);
      }

      const ipr::Identifier&
      Lexicon::get_identifier(const char* s) {
         return get_identifier(get_string(s));
      }

      const ipr::Identifier&
      Lexicon::get_identifier(const std::string& s) {
         return get_identifier(get_string(s));
      }

      const ipr::Identifier&
      Lexicon::get_identifier(const ipr::String& s) {
         return *expr_factory::make_identifier(s);
      }

      const ipr::Suffix&
      Lexicon::get_suffix(const ipr::Identifier& id) {
         return *expr_factory::make_suffix(id);
      }

      const ipr::Type& Lexicon::void_type() const {  return voidtype;  }

      const ipr::Type& Lexicon::bool_type() const { return booltype; }

      const ipr::Type& Lexicon::char_type() const { return chartype; }

      const ipr::Type& Lexicon::schar_type() const { return schartype; }

      const ipr::Type& Lexicon::uchar_type() const { return uchartype; }

      const ipr::Type& Lexicon::wchar_t_type() const { return wchar_ttype; }

      const ipr::Type& Lexicon::char8_t_type() const { return char8_ttype; }

      const ipr::Type& Lexicon::char16_t_type() const { return char16_ttype; }

      const ipr::Type& Lexicon::char32_t_type() const { return char32_ttype; }

      const ipr::Type& Lexicon::short_type() const { return shorttype; }

      const ipr::Type& Lexicon::ushort_type() const { return ushorttype; }

      const ipr::Type& Lexicon::int_type() const { return inttype; }

      const ipr::Type& Lexicon::uint_type() const { return uinttype; }

      const ipr::Type& Lexicon::long_type() const { return longtype; }

      const ipr::Type& Lexicon::ulong_type() const { return ulongtype; }

      const ipr::Type& Lexicon::long_long_type() const { return longlongtype; }

      const ipr::Type& Lexicon::ulong_long_type() const { return ulonglongtype; }

      const ipr::Type& Lexicon::float_type() const { return floattype; }

      const ipr::Type& Lexicon::double_type() const { return doubletype; }

      const ipr::Type& Lexicon::long_double_type() const {
         return longdoubletype;
      }

      const ipr::Type& Lexicon::ellipsis_type() const { return ellipsistype; }

      const ipr::Type& Lexicon::typename_type() const { return anytype; }

      const ipr::Type& Lexicon::class_type() const { return classtype; }

      const ipr::Type& Lexicon::union_type() const { return uniontype; }

      const ipr::Type& Lexicon::enum_type() const { return enumtype; }

      const ipr::Type& Lexicon::namespace_type() const { return namespacetype; }

      const ipr::Expr& Lexicon::nullptr_value() const { return null; }

      template<class T>
      T* Lexicon::finish_type(T* t) {
         if (!t->constraint.is_valid())
            t->constraint = &anytype;

         if (!t->id.is_valid())
            t->id = make_type_id(*t);

         return t;
      }

      const ipr::Ctor_name&
      Lexicon::get_ctor_name(const ipr::Type& t) {
         return *expr_factory::make_ctor_name(t);
      }

      const ipr::Dtor_name&
      Lexicon::get_dtor_name(const ipr::Type& t) {
         return *expr_factory::make_dtor_name(t);
      }

      const ipr::Operator&
      Lexicon::get_operator(const char* s) {
         return get_operator(get_string(s));
      }

      const ipr::Operator&
      Lexicon::get_operator(const std::string& s) {
         return get_operator(get_string(s));
      }

      const ipr::Operator&
      Lexicon::get_operator(const ipr::String& s) {
         return *expr_factory::make_operator(s);
      }

      const ipr::Conversion&
      Lexicon::get_conversion(const ipr::Type& t) {
         return *expr_factory::make_conversion(t);
      }

      const ipr::Template_id&
      Lexicon::get_template_id(const ipr::Name& t, const ipr::Expr_list& a) {
         return *expr_factory::make_template_id(t, a);
      }

      const ipr::Array&
      Lexicon::get_array(const ipr::Type& t, const ipr::Expr& b) {
         return *finish_type(types.make_array(t, b));
      }

      const ipr::As_type&
      Lexicon::get_as_type(const ipr::Expr& e) {
         return get_as_type(e, cxx_linkage());
      }

      const ipr::As_type&
      Lexicon::get_as_type(const ipr::Expr& e, const ipr::Linkage& l) {
         return *finish_type(types.make_as_type(e, l));
      }

      const ipr::Decltype&
      Lexicon::get_decltype(const ipr::Expr& e) {
         return *finish_type(types.make_decltype(e));
      }

      const ipr::Function&
      Lexicon::get_function(const ipr::Product& p, const ipr::Type& t,
                            const ipr::Sum& s, const ipr::Linkage& l) {
         return *finish_type(types.make_function(p, t, s, l));
      }

      const ipr::Function&
      Lexicon::get_function(const ipr::Product& p, const ipr::Type& t,
                            const ipr::Sum& s) {
         return get_function(p, t, s, cxx_linkage());
      }

      const ipr::Function&
      Lexicon::get_function(const ipr::Product& p, const ipr::Type& t,
                            const ipr::Linkage& l) {
         ref_sequence<ipr::Type> ex;
         ex.push_back(&ellipsistype);
         return get_function(p, t, get_sum(ex), l);
      }

      const ipr::Function&
      Lexicon::get_function(const ipr::Product& p, const ipr::Type& t) {
         return get_function(p, t, cxx_linkage());
      }

      const ipr::Pointer&
      Lexicon::get_pointer(const ipr::Type& t) {
         return *finish_type(types.make_pointer(t));
      }

      const ipr::Product&
      Lexicon::get_product(const ref_sequence<ipr::Type>& s) {
         return *finish_type
            (types.make_product(*type_seqs.insert(s, unary_compare())));
      }

      const ipr::Ptr_to_member&
      Lexicon::get_ptr_to_member(const ipr::Type& s, const ipr::Type& t) {
         return *finish_type(types.make_ptr_to_member(s, t));
      }

      const ipr::Qualified&
      Lexicon::get_qualified(ipr::Type_qualifier cv, const ipr::Type& t) {
         assert (cv != ipr::Type_qualifier::None);
         return *finish_type(types.make_qualified(cv, t));
      }

      const ipr::Reference&
      Lexicon::get_reference(const ipr::Type& t) {
         return *finish_type(types.make_reference(t));
      }

      const ipr::Rvalue_reference&
      Lexicon::get_rvalue_reference(const ipr::Type& t) {
         return *finish_type(types.make_rvalue_reference(t));
      }

      const ipr::Sum&
      Lexicon::get_sum(const ref_sequence<ipr::Type>& s) {
         return *finish_type
            (types.make_sum(*type_seqs.insert(s, unary_compare())));
      }

      const ipr::Forall&
      Lexicon::get_forall(const ipr::Product& p, const ipr::Type& t) {
         return *finish_type(types.make_forall(p, t));
      }

      impl::Class*
      Lexicon::make_class(const ipr::Region& pr) {
         return types.make_class(pr, classtype);
      }

      impl::Enum*
      Lexicon::make_enum(const ipr::Region& pr, Enum::Kind k) {
         auto t = types.make_enum(pr, k);
         t->constraint = &enumtype;
         return t;
      }

      impl::Namespace*
      Lexicon::make_namespace(const ipr::Region& pr) {
         return types.make_namespace(&pr, namespacetype);
      }

      impl::Union*
      Lexicon::make_union(const ipr::Region& pr) {
         return types.make_union(pr, uniontype);
      }

      impl::Mapping*
      Lexicon::make_mapping(const ipr::Region& r, Mapping_level l) {
         auto x = expr_factory::make_mapping(r, l);
         // Note: the parameters form a Product type needing its type set
         x->parameters.parms.scope.decls.constraint = &anytype;
         return x;
      }

      impl::Parameter*
      Lexicon::make_parameter(const ipr::Name& n, const ipr::Type& t,
                              impl::Mapping& m) {
         return m.param(n, *rname_for_next_param(m, t));
      }

      const ipr::Auto& Lexicon::get_auto() {
         auto t = autos.make();
         t->id = &get_identifier("auto");
         return *t;
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
   const Name* name = lexicon.make_identifier("bufsz");
   // then its type,
   auto& type = lexicon.get_qualified(Type_qualifier::Const, lexicon.int_type());
   // and the actual impl::Var node,
   impl::Var* var = global_scope->make_var(*name, type);
   // set its initializer,
   var->init = lexicon.make_literal(lexicon.int_type(), "1024");
   // and inject it into its scope.

   // Print out the whole translation unit
   Printer pp { std::cout };
   pp << unit;
   std::cout << std::endl;

}

*/
