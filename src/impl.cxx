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
   namespace impl {

      Token::Token(const ipr::String& s, const Source_location& l,
                   TokenValue v, TokenCategory c)
            : text{ s }, location{ l }, token_value{ v }, token_category{ c }
      { }

      struct scope_datum::comp {
         int operator()(const scope_datum& lhs, const scope_datum& rhs) const
         {
            return compare(lhs.scope_pos, rhs.scope_pos);
         }

         int operator()(int pos, const scope_datum& s) const
         {
            return compare(pos, s.scope_pos);
         }

         int operator()(const scope_datum& s, int pos) const
         {
            return compare(s.scope_pos, pos);
         }
      };

      // --------------------------------------
      // -- master_decl_data<ipr::Named_map> --
      // --------------------------------------

      master_decl_data<ipr::Named_map>::
      master_decl_data(impl::Overload* ovl, const ipr::Type& t)
            : Base(this), overload_entry(t),
              def(0), langlinkage(0), primary(0),home(0),
              overload(ovl)
      { }

      // -------------------------
      // -- impl::decl_sequence --
      // -------------------------

      int
      decl_sequence::size() const {
         return decls.size();
      }

      const ipr::Decl&
      decl_sequence::get(int i) const {
         scope_datum* result = decls.find(i, scope_datum::comp());
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

      int
      Overload::size() const {
         return entries.size();
      }

      const ipr::Decl&
      Overload::get(int i) const {
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

      int
      singleton_overload::size() const {
         return 1;
      }

      const ipr::Decl&
      singleton_overload::get(int i) const {
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

      int
      empty_overload::size() const {
         return 0;
      }

      const ipr::Decl&
      empty_overload::get(int) const {
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


      // -----------------
      // -- impl::Alias --
      // -----------------

      Alias::Alias() : aliasee(0), lexreg(0)
      { }

      const ipr::Expr&
      Alias::initializer() const {
         return *util::check(aliasee);
      }

      bool
      Alias::has_initializer() const {
         return aliasee != 0;;
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

      const ipr::Udt&
      Bitfield::membership() const {
         return *util::check(member_of);
      }

      const ipr::Expr&
      Bitfield::initializer() const {
         return *util::check(init);
      }

      bool
      Bitfield::has_initializer() const {
         return init != 0;;
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

      Base_type::Base_type(const ipr::Type& t, const ipr::Region& r, int p)
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

      int
      Base_type::position() const {
         return scope_pos;
      }

      const ipr::Expr&
      Base_type::initializer() const {
         throw std::domain_error("impl::Base_type::initializer");
      }

      bool
      Base_type::has_initializer() const {
         return false;
      }

      const ipr::Sequence<ipr::Decl>&
      Base_type::decl_set() const {
         return overload.seq;
      }

      // ----------------------
      // -- impl::Enumerator --
      // ----------------------

      Enumerator::Enumerator(const ipr::Name& n, const ipr::Enum& t, int p)
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

      const ipr::Sequence<ipr::Decl>&
      Enumerator::decl_set() const {
         return overload.seq;
      }

      int
      Enumerator::position() const {
         return scope_pos;
      }

      const ipr::Expr&
      Enumerator::initializer() const {
         return *util::check(init);
      }

      bool
      Enumerator::has_initializer() const {
         return init != 0;
      }

      // -----------------
      // -- impl::Field --
      // -----------------

      Field::Field()
            : member_of(0), init(0)
      { }

      const ipr::Expr&
      Field::initializer() const {
         return *util::check(init);
      }

      bool
      Field::has_initializer() const {
         return init != 0;
      }

      const ipr::Udt&
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
            : member_of(0), init(0), lexreg(0)
      { }

      const ipr::Mapping&
      Fundecl::mapping() const {
         return *util::check(init);
      }

      const ipr::Udt&
      Fundecl::membership() const {
         return *util::check(member_of);
      }

      const ipr::Expr&
      Fundecl::initializer() const {
         return *util::check(util::check(init)->body);
      }

      bool
      Fundecl::has_initializer() const {
         return init != 0 && init->body != 0;
      }

      const ipr::Region&
      Fundecl::lexical_region() const {
         return *util::check(lexreg);
      }

      // ---------------------
      // -- impl::Named_map --
      // ---------------------

      Named_map::Named_map() : member_of(0), init(0), lexreg(0) { }

      const ipr::Named_map&
      Named_map::primary_named_map() const {
         return *util::check(util::check(decl_data.master_data)->primary);
      }

      const ipr::Sequence<ipr::Decl>&
      Named_map::specializations() const {
         return util::check(decl_data.master_data)->specs;
      }

      const ipr::Mapping&
      Named_map::mapping() const {
         return *util::check(init);
      }

      const ipr::Expr&
      Named_map::initializer() const {
         return *util::check(util::check(init)->body);
      }

      bool
      Named_map::has_initializer() const {
         return init != 0 && init->body != 0;
      }

      const ipr::Region&
      Named_map::lexical_region() const {
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
         return *util::check(where);
      }

      const ipr::Region&
      Parameter::lexical_region() const {
         return *util::check(where);
      }

      const ipr::Parameter_list&
      Parameter::membership() const {
         return *util::check(where);
      }

      const ipr::Sequence<ipr::Decl>&
      Parameter::decl_set() const {
         return overload.seq;
      }

      int
      Parameter::position() const {
         return abstract_name.rep.second;
      }

      const ipr::Expr&
      Parameter::initializer() const {
         return *util::check(init);
      }

      bool
      Parameter::has_initializer() const {
         return init != 0;
      }

      // --------------------
      // -- impl::Typedecl --
      // --------------------

      Typedecl::Typedecl() : init(0), member_of(0), lexreg(0)
      { }

      const ipr::Expr&
      Typedecl::initializer() const {
         return *util::check(init);
      }

      bool
      Typedecl::has_initializer() const {
         return init != 0;
      }

      const ipr::Udt&
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

      const ipr::Expr&
      Var::initializer() const {
         return *util::check(init);
      }

      bool
      Var::has_initializer() const {
         return init != 0;
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

      const ipr::Type&
      Break::type() const {
         return util::check(stmt)->type();
      }

      const ipr::Stmt&
      Break::from() const {
         return *util::check(stmt);
      }

      // --------------------
      // -- impl::Continue --
      // --------------------

      Continue::Continue() : stmt(0) { }

      const ipr::Type&
      Continue::type() const {
         return util::check(stmt)->type();
      }

      const ipr::Stmt&
      Continue::iteration() const {
         return *util::check(stmt);
      }

      // ------------------------
      // -- impl::stmt_factory --
      // ------------------------

      impl::Break*
      stmt_factory::make_break() {
         return breaks.make();
      }

      impl::Continue*
      stmt_factory::make_continue() {
         return continues.make();
      }

      impl::Empty_stmt*
      stmt_factory::make_empty_stmt() {
         return empty_stmts.make(*make_phantom());
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

      impl::Do*
      stmt_factory::make_do(const ipr::Stmt& s, const ipr::Expr& c) {
         return dos.make(c, s);
      }

      impl::If_then*
      stmt_factory::make_if_then(const ipr::Expr& c, const ipr::Stmt& s) {
         return ifs.make(c, s);
      }

      impl::Switch*
      stmt_factory::make_switch(const ipr::Expr& c, const ipr::Stmt& s) {
         return switches.make(c, s);
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

      impl::While*
      stmt_factory::make_while(const ipr::Expr& c, const ipr::Stmt& s) {
         return whiles.make(c, s);
      }

      impl::If_then_else*
      stmt_factory::make_if_then_else(const ipr::Expr& c, const ipr::Stmt& t,
                                      const ipr::Stmt& f) {
         return ifelses.make(c, t, f);
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

      Enum::Enum(const ipr::Region& r, const ipr::Type& t, Kind k)
            : body(r, t), enum_kind(k)
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
         impl::Enumerator* e = body.scope.push_back(n, *this, body.size());
         e->where = &body;
         return e;
      }

      // -----------------
      // -- impl::Class --
      // -----------------

      Class::Class(const ipr::Region& pr, const ipr::Type& t)
            : impl::Udt<ipr::Class>(&pr, t),
              base_subobjects(pr, t) {
         base_subobjects.owned_by = this;
      }

      const ipr::Sequence<ipr::Base_type>&
      Class::bases() const {
         return base_subobjects.scope.decls.seq;
      }

      impl::Base_type*
      Class::declare_base(const ipr::Type& t) {
         return base_subobjects.scope.push_back(t, base_subobjects,
                                                base_subobjects.size());
      }

      // ------------------
      // -- impl::String --
      // ------------------
      String::String(const util::string& s) : text(s)
      { }

      int
      String::size() const {
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

      Parameter_list::Parameter_list(const ipr::Region& p, const ipr::Type& t)
            : Base(p, t)
      { }

      impl::Parameter*
      Parameter_list::add_member(const ipr::Name& n, const impl::Rname& rn)
      {
         impl::Parameter* param = scope.decls.seq.push_back(n, rn);
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
         return lhs.node_id == rhs.node_id;
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

      impl::Template*
      type_factory::make_template(const ipr::Product& s, const ipr::Type& t) {
         using rep = impl::Template::Rep;
         return templates.insert(rep{ s, t }, binary_compare());
      }

      impl::Enum*
      type_factory::make_enum(const ipr::Region& pr, const ipr::Type& t, Enum::Kind k) {
         return enums.make(pr, t, k);
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
            : impl::Unary<impl::Expr<ipr::Id_expr>>(n), decl(0)
      { }

      const ipr::Type&
      Id_expr::type() const {
         return util::check(decl)->type();
      }

      const ipr::Decl&
      Id_expr::resolution() const {
         return *util::check(decl);
      }


      // -------------------
      // -- impl::Mapping --
      // -------------------

      Mapping::Mapping(const ipr::Region& pr, const ipr::Type& t, int d)
            : parameters(pr, t), value_type(0),
              body(0), nesting_level(d)
      {
        // 31Oct08 added by PIR to avoid exceptions,
        //   when querying the region (parameters) for its owner
        parameters.owned_by = this;
      }

      const ipr::Parameter_list&
      Mapping::params() const {
         return parameters;
      }

      const ipr::Type&
      Mapping::result_type() const {
         return *util::check(value_type);
      }

      const ipr::Expr&
      Mapping::result() const {
         return *util::check(body);
      }

      int
      Mapping::depth() const {
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
         decl->decl_data.scope_pos = decls.seq.size();
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

      impl::Named_map*
      Scope::make_primary_map(const ipr::Name& n, const ipr::Template& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == 0) {
            impl::Named_map* decl = primary_maps.declare(ovl, t);
            decl->decl_data.master_data->primary = decl;
            add_member(decl);
            return decl;
         }
         else {
            impl::Named_map* decl = primary_maps.redeclare(master);
            // FIXME: set the primary field.
            add_member(decl);
            return decl;
         }
      }

      impl::Named_map*
      Scope::make_secondary_map(const ipr::Name& n, const ipr::Template& t)
      {
         impl::Overload* ovl = overloads.insert(n, node_compare());
         overload_entry* master = ovl->lookup(t);

         if (master == 0) {
            impl::Named_map* decl = secondary_maps.declare(ovl, t);
            // FXIME: record this a secondary map and set its primary.
            add_member(decl);
            return decl;
         }
         else {
            impl::Named_map* decl = secondary_maps.redeclare(master);
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

      impl::Phantom*
      expr_factory::make_phantom() {
         return phantoms.make();
      }

      const ipr::Phantom*
      expr_factory::make_phantom(const ipr::Type& t) {
         return phantoms.make(&t);
      }

      impl::Address*
      expr_factory::make_address(const ipr::Expr& e) {
         return addresses.make(e);
      }

      impl::Array_delete*
      expr_factory::make_array_delete(const ipr::Expr& e) {
         return array_deletes.make(e);
      }

      impl::Complement*
      expr_factory::make_complement(const ipr::Expr& e) {
         return complements.make(e);
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

      impl::Deref*
      expr_factory::make_deref(const ipr::Expr& e) {
         return derefs.make(e);
      }

      impl::Dtor_name*
      expr_factory::make_dtor_name(const ipr::Type& t) {
         return dtors.insert(t, unary_compare());
      }

      impl::Expr_list*
      expr_factory::make_expr_list() {
         return xlists.make();
      }

      impl::Expr_sizeof*
      expr_factory::make_expr_sizeof(const ipr::Expr& e) {
         return xsizeofs.make(e);
      }

      impl::Expr_typeid*
      expr_factory::make_expr_typeid(const ipr::Expr& e) {
         return xtypeids.make(e);
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


      impl::Id_expr*
      expr_factory::make_id_expr(const ipr::Name& n) {
         return id_exprs.make(n);
      }

      impl::Id_expr*
      expr_factory::make_id_expr(const ipr::Decl& d) {
         impl::Id_expr* x = id_exprs.make(d.name());
         x->constraint = &d.type();
         x->decl = &d;
         return x;
      }

      impl::Initializer_list*
      expr_factory::make_initializer_list(const ipr::Expr_list& e) {
         return init_lists.make(e);
      }

      impl::Not*
      expr_factory::make_not(const ipr::Expr& e) {
         return nots.make(e);
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

      impl::Paren_expr*
      expr_factory::make_paren_expr(const ipr::Expr& e) {
         return parens.make(e);
      }

      impl::Post_increment*
      expr_factory::make_post_increment(const ipr::Expr& e) {
         return post_increments.make(e);
      }

      impl::Post_decrement*
      expr_factory::make_post_decrement(const ipr::Expr& e) {
         return post_decrements.make(e);
      }

      impl::Pre_increment*
      expr_factory::make_pre_increment(const ipr::Expr& e) {
         return pre_increments.make(e);
      }

      impl::Pre_decrement*
      expr_factory::make_pre_decrement(const ipr::Expr& e) {
         return pre_decrements.make(e);
      }

      impl::Throw*
      expr_factory::make_throw(const ipr::Expr& e) {
         return throws.make(e);
      }

      impl::Type_id*
      expr_factory::make_type_id(const ipr::Type& t) {
         return typeids.insert(t, unary_compare());
      }

      impl::Type_sizeof*
      expr_factory::make_type_sizeof(const ipr::Type& t) {
         return tsizeofs.insert(t, unified_type_compare());
      }

      impl::Type_typeid*
      expr_factory::make_type_typeid(const ipr::Type& t) {
         return ttypeids.insert(t, unified_type_compare());
      }

      impl::Unary_minus*
      expr_factory::make_unary_minus(const ipr::Expr& e) {
         return unary_minuses.make(e);
      }

      impl::Unary_plus*
      expr_factory::make_unary_plus(const ipr::Expr& e) {
         return unary_pluses.make(e);
      }

      impl::Expansion*
      expr_factory::make_expansion(const ipr::Expr& e) {
         return expansions.make(e);
      }

      impl::And*
      expr_factory::make_and(const ipr::Expr& l, const ipr::Expr& r) {
         return ands.make(l, r);
      }

      impl::Array_ref*
      expr_factory::make_array_ref(const ipr::Expr& l, const ipr::Expr& r) {
         return array_refs.make(l, r);
      }

      impl::Arrow*
      expr_factory::make_arrow(const ipr::Expr& l, const ipr::Expr& r) {
         return arrows.make(l, r);
      }

      impl::Arrow_star*
      expr_factory::make_arrow_star(const ipr::Expr& l, const ipr::Expr& r) {
         return arrow_stars.make(l, r);
      }

      impl::Assign*
      expr_factory::make_assign(const ipr::Expr& l, const ipr::Expr& r) {
         return assigns.make(l, r);
      }

      impl::Bitand*
      expr_factory::make_bitand(const ipr::Expr& l, const ipr::Expr& r) {
         return bitands.make(l, r);
      }

      impl::Bitand_assign*
      expr_factory::make_bitand_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return bitand_assigns.make(l, r);
      }

      impl::Bitor*
      expr_factory::make_bitor(const ipr::Expr& l, const ipr::Expr& r) {
         return bitors.make(l, r);
      }

      impl::Bitor_assign*
      expr_factory::make_bitor_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return bitor_assigns.make(l, r);
      }

      impl::Bitxor*
      expr_factory::make_bitxor(const ipr::Expr& l, const ipr::Expr& r) {
         return bitxors.make(l, r);
      }

      impl::Bitxor_assign*
      expr_factory::make_bitxor_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return bitxor_assigns.make(l, r);
      }

      impl::Cast*
      expr_factory::make_cast(const ipr::Type& t, const ipr::Expr& e) {
         return casts.make(t, e);
      }

      impl::Call*
      expr_factory::make_call(const ipr::Expr& l, const ipr::Expr_list& r) {
         return calls.make(l, r);
      }

      impl::Comma*
      expr_factory::make_comma(const ipr::Expr& l, const ipr::Expr& r) {
         return commas.make(l, r);
      }

      impl::Const_cast*
      expr_factory::make_const_cast(const ipr::Type& t, const ipr::Expr& e) {
         return ccasts.make(t, e);
      }

      impl::Datum*
      expr_factory::make_datum(const ipr::Type& t, const ipr::Expr_list& e) {
         return data.make(t, e);
      }

      impl::Div*
      expr_factory::make_div(const ipr::Expr& l, const ipr::Expr& r) {
         return divs.make(l, r);
      }

      impl::Div_assign*
      expr_factory::make_div_assign(const ipr::Expr& l, const ipr::Expr& r) {
         return div_assigns.make(l, r);
      }

      impl::Dot*
      expr_factory::make_dot(const ipr::Expr& l, const ipr::Expr& r) {
         return dots.make(l, r);
      }

      impl::Dot_star*
      expr_factory::make_dot_star(const ipr::Expr& l, const ipr::Expr& r) {
         return dot_stars.make(l, r);
      }

      impl::Dynamic_cast*
      expr_factory::make_dynamic_cast(const ipr::Type& t, const ipr::Expr& e)
      {
         return dcasts.make(t, e);
      }

      impl::Equal*
      expr_factory::make_equal(const ipr::Expr& l, const ipr::Expr& r) {
         return equals.make(l, r);
      }

      impl::Greater*
      expr_factory::make_greater(const ipr::Expr& l, const ipr::Expr& r) {
         return greaters.make(l, r);
      }

      impl::Greater_equal*
      expr_factory::make_greater_equal(const ipr::Expr& l, const ipr::Expr& r)
      {
         return greater_equals.make(l, r);
      }

      impl::Less*
      expr_factory::make_less(const ipr::Expr& l, const ipr::Expr& r) {
         return lesses.make(l, r);
      }

      impl::Less_equal*
      expr_factory::make_less_equal(const ipr::Expr& l, const ipr::Expr& r) {
         return less_equals.make(l, r);
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
      expr_factory::make_lshift(const ipr::Expr& l, const ipr::Expr& r) {
         return lshifts.make(l, r);
      }

      impl::Lshift_assign*
      expr_factory::make_lshift_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return lshift_assigns.make(l, r);
      }

      impl::Member_init*
      expr_factory::make_member_init(const ipr::Expr& l, const ipr::Expr& r) {
         return member_inits.make(l, r);
      }

      impl::Minus*
      expr_factory::make_minus(const ipr::Expr& l, const ipr::Expr& r) {
         return minuses.make(l, r);
      }

      impl::Minus_assign*
      expr_factory::make_minus_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return minus_assigns.make(l, r);
      }

      impl::Modulo*
      expr_factory::make_modulo(const ipr::Expr& l, const ipr::Expr& r) {
         return modulos.make(l, r);
      }

      impl::Modulo_assign*
      expr_factory::make_modulo_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return modulo_assigns.make(l, r);
      }

      impl::Mul*
      expr_factory::make_mul(const ipr::Expr& l, const ipr::Expr& r) {
         return muls.make(l, r);
      }

      impl::Mul_assign*
      expr_factory::make_mul_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return mul_assigns.make(l, r);
      }

      impl::Not_equal*
      expr_factory::make_not_equal(const ipr::Expr& l, const ipr::Expr& r) {
         return not_equals.make(l, r);
      }

      impl::Or*
      expr_factory::make_or(const ipr::Expr& l, const ipr::Expr& r) {
         return ors.make(l, r);
      }

      impl::Plus*
      expr_factory::make_plus(const ipr::Expr& l, const ipr::Expr& r) {
         return pluses.make(l, r);
      }

      impl::Plus_assign*
      expr_factory::make_plus_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return plus_assigns.make(l, r);
      }

      impl::Reinterpret_cast*
      expr_factory::make_reinterpret_cast(const ipr::Type& t,
                                          const ipr::Expr& e) {
         return rcasts.make(t, e);
      }

      impl::Scope_ref*
      expr_factory::make_scope_ref(const ipr::Expr& l, const ipr::Expr& r)
      {
         using Rep = impl::Scope_ref::Rep;
         return scope_refs.insert(Rep{ l, r }, binary_compare());
      }

      impl::Rshift*
      expr_factory::make_rshift(const ipr::Expr& l, const ipr::Expr& r) {
         return rshifts.make(l, r);
      }

      impl::Rshift_assign*
      expr_factory::make_rshift_assign(const ipr::Expr& l, const ipr::Expr& r)
      {
         return rshift_assigns.make(l, r);
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

      impl::New*
      expr_factory::make_new(const ipr::Expr_list& p, const ipr::Type& t,
                             const ipr::Expr_list& i) {
         return news.make(p, t, i);
      }

      impl::Conditional*
      expr_factory::make_conditional(const ipr::Expr& c, const ipr::Expr& t,
                                     const ipr::Expr& f) {
         return conds.make(c, t, f);
      }

      impl::Rname*
      expr_factory::rname_for_next_param(const impl::Mapping& map,
                                         const ipr::Type& t) {
         using Rep = impl::Rname::Rep;
         return rnames.insert(Rep{ t, map.nesting_level, map.parameters.size() },
                              ternary_compare());
      }

      impl::Mapping*
      expr_factory::make_mapping(const ipr::Region& r, const ipr::Type& t,
                                 int l) {
         return mappings.make(r, t, l);
      }


      // ----------------
      // -- impl::Unit --
      // ----------------

      const ipr::Linkage&
      Unit::get_cxx_linkage() const {
         return const_cast<Unit*>(this)->get_linkage("C++");
      }

      const ipr::Linkage&
      Unit::get_c_linkage() const {
         return const_cast<Unit*>(this)->get_linkage("C");
      }

      // -------------------------------------
      // -- impl::Unit::record_builtin_type --
      // -------------------------------------

      void
      Unit::record_builtin_type(const ipr::As_type& t) {
         builtin_map.insert(t, unary_compare());
      }


      // ----------------------
      // -- impl::Unit::Unit --
      // ----------------------

      Unit::Unit()
            : anytype(get_identifier("typename"), get_cxx_linkage(), anytype),
              classtype(get_identifier("class"), get_cxx_linkage(), anytype),
              uniontype(get_identifier("union"), get_cxx_linkage(), anytype),
              enumtype(get_identifier("enum"), get_cxx_linkage(), anytype),
              namespacetype(get_identifier("namespace"), get_cxx_linkage(), anytype),

              voidtype(get_identifier("void"), get_cxx_linkage(), anytype),
              booltype(get_identifier("bool"), get_cxx_linkage(), anytype),
              chartype(get_identifier("char"), get_cxx_linkage(), anytype),
              schartype(get_identifier("signed char"), get_cxx_linkage(), anytype),
              uchartype(get_identifier("unsigned char"), get_cxx_linkage(), anytype),
              wchar_ttype(get_identifier("wchar_t"), get_cxx_linkage(), anytype),
              shorttype(get_identifier("short"), get_cxx_linkage(), anytype),
              ushorttype(get_identifier("unsigned short"),
                         get_cxx_linkage(), anytype),
              inttype(get_identifier("int"), get_cxx_linkage(), anytype),
              uinttype(get_identifier("unsigned int"), get_cxx_linkage(), anytype),
              longtype(get_identifier("long"), get_cxx_linkage(), anytype),
              ulongtype(get_identifier("unsigned long"),
                        get_cxx_linkage(), anytype),
              longlongtype(get_identifier("long long"), get_cxx_linkage(), anytype),
              ulonglongtype(get_identifier("unsigned long long"),
                            get_cxx_linkage(), anytype),
              floattype(get_identifier("float"), get_cxx_linkage(), anytype),
              doubletype(get_identifier("double"), get_cxx_linkage(), anytype),
              longdoubletype(get_identifier("long double"),
                             get_cxx_linkage(), anytype),
              ellipsistype(get_identifier("..."), get_cxx_linkage(), anytype),
              global_ns(0, namespacetype)
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
         global_ns.id = &get_identifier("");
      }

      // -----------------------
      // -- impl::Unit::~Unit --
      // -----------------------

      Unit::~Unit() { }

      // -----------------------------
      // -- impl::Unit::get_literal --
      // -----------------------------

      const ipr::Literal&
      Unit::get_literal(const ipr::Type& t, const char* s) {
         return get_literal(t, get_string(s));
      }

      const ipr::Literal&
      Unit::get_literal(const ipr::Type& t, const std::string& s) {
         return get_literal(t, get_string(s));
      }

      const ipr::Literal&
      Unit::get_literal(const ipr::Type& t, const ipr::String& s) {
         return *make_literal(t, s);
      }

      // --------------------------------
      // -- impl::Unit::get_identifier --
      // --------------------------------

      const ipr::Identifier&
      Unit::get_identifier(const char* s) {
         return get_identifier(get_string(s));
      }

      const ipr::Identifier&
      Unit::get_identifier(const std::string& s) {
         return get_identifier(get_string(s));
      }

      const ipr::Identifier&
      Unit::get_identifier(const ipr::String& s) {
         impl::Identifier* id = expr_factory::make_identifier(s);
         if (id->constraint == 0)
            id->constraint = &get_decltype(*id);
         return *id;
      }

      // ----------------------------------
      // -- impl::Unit::get_global_scope --
      // ----------------------------------

      const ipr::Global_scope&
      Unit::get_global_scope() const {
         return global_ns;
      }

      // --------------------------
      // -- impl::Unit::get_void --
      // --------------------------

      const ipr::Void&
      Unit::get_void() const {
         return voidtype;
      }

      const ipr::Bool&
      Unit::get_bool() const {
         return booltype;
      }

      const ipr::Char&
      Unit::get_char() const {
         return chartype;
      }

      const ipr::sChar&
      Unit::get_schar() const {
         return schartype;
      }

      const ipr::uChar&
      Unit::get_uchar() const {
         return uchartype;
      }

      const ipr::Wchar_t&
      Unit::get_wchar_t() const {
         return wchar_ttype;
      }

      const ipr::Short&
      Unit::get_short() const {
         return shorttype;
      }

      const ipr::uShort&
      Unit::get_ushort() const {
         return ushorttype;
      }

      const ipr::Int&
      Unit::get_int() const {
         return inttype;
      }

      const ipr::uInt&
      Unit::get_uint() const {
         return uinttype;
      }

      const ipr::Long&
      Unit::get_long() const {
         return longtype;
      }

      const ipr::uLong&
      Unit::get_ulong() const {
         return ulongtype;
      }

      const ipr::Long_long&
      Unit::get_long_long() const {
         return longlongtype;
      }

      const ipr::uLong_long&
      Unit::get_ulong_long() const {
         return ulonglongtype;
      }

      const ipr::Float&
      Unit::get_float() const {
         return floattype;
      }

      const ipr::Double&
      Unit::get_double() const {
         return doubletype;
      }

      const ipr::Long_double&
      Unit::get_long_double() const {
         return longdoubletype;
      }

      const ipr::Ellipsis&
      Unit::get_ellipsis() const {
         return ellipsistype;
      }


      const ipr::Type&
      Unit::get_typename() const {
         return anytype;
      }

      const ipr::Type&
      Unit::get_class() const {
         return classtype;
      }

      const ipr::Type&
      Unit::get_union() const {
         return uniontype;
      }

      const ipr::Type&
      Unit::get_enum() const {
         return enumtype;
      }

      const ipr::Type&
      Unit::get_namespace() const {
         return namespacetype;
      }

      // -----------------------------
      // -- impl::Unit::finish_type --
      // -----------------------------

      template<class T>
      T*
      Unit::finish_type(T* t) {
         if (t->constraint == 0)
            t->constraint = &anytype;

         if (t->id == 0)
            t->id = make_type_id(*t);

         return t;
      }

      // -------------------------------
      // -- impl::Unit::get_ctor_name --
      // -------------------------------

      const ipr::Ctor_name&
      Unit::get_ctor_name(const ipr::Type& t) {
         impl::Ctor_name* id = expr_factory::make_ctor_name(t);
         if (id->constraint == 0)
            id->constraint = &get_decltype(*id);
         return *id;
      }

      // -------------------------------
      // -- impl::Unit::get_dtor_name --
      // -------------------------------

      const ipr::Dtor_name&
      Unit::get_dtor_name(const ipr::Type& t) {
         impl::Dtor_name* id = expr_factory::make_dtor_name(t);
         if (id->constraint == 0)
            id->constraint = &get_decltype(*id);
         return *id;
      }

      // ------------------------------
      // -- impl::Unit::get_operator --
      // ------------------------------

      const ipr::Operator&
      Unit::get_operator(const char* s) {
         return get_operator(get_string(s));
      }

      const ipr::Operator&
      Unit::get_operator(const std::string& s) {
         return get_operator(get_string(s));
      }

      const ipr::Operator&
      Unit::get_operator(const ipr::String& s) {
         impl::Operator* op = expr_factory::make_operator(s);
         if (op->constraint == 0)
            op->constraint = &get_decltype(*op);
         return *op;
      }

      // --------------------------------
      // -- impl::Unit::get_conversion --
      // --------------------------------

      const ipr::Conversion&
      Unit::get_conversion(const ipr::Type& t) {
         impl::Conversion* conv = expr_factory::make_conversion(t);
         if (conv->constraint == 0)
            conv->constraint = &get_decltype(*conv);
         return *conv;
      }

      // -------------------------------
      // -- impl::Unit::get_scope_ref --
      // -------------------------------

      const ipr::Scope_ref&
      Unit::get_scope_ref(const ipr::Expr& s, const ipr::Expr& m) {
         impl::Scope_ref* sr = expr_factory::make_scope_ref(s, m);
         if (sr->constraint == 0)
            sr->constraint = &get_decltype(*sr);
         return *sr;
      }

      // ---------------------------------
      // -- impl::Unit::get_template_id --
      // ---------------------------------

      const ipr::Template_id&
      Unit::get_template_id(const ipr::Name& t, const ipr::Expr_list& a) {
         impl::Template_id* tid = expr_factory::make_template_id(t, a);
         if (tid->constraint == 0)
            tid->constraint = &get_decltype(*tid);
         return *tid;
      }

      // ---------------------------
      // -- impl::Unit::get_array --
      // ---------------------------

      const ipr::Array&
      Unit::get_array(const ipr::Type& t, const ipr::Expr& b) {
         return *finish_type(types.make_array(t, b));
      }

      // -----------------------------
      // -- impl::Unit::get_as_type --
      // -----------------------------

      const ipr::As_type&
      Unit::get_as_type(const ipr::Expr& e) {
         return get_as_type(e, get_cxx_linkage());
      }

      const ipr::As_type&
      Unit::get_as_type(const ipr::Expr& e, const ipr::Linkage& l) {
         return *finish_type(types.make_as_type(e, l));
      }

      // ------------------------------
      // -- impl::Unit::get_decltype --
      // ------------------------------

      const ipr::Decltype&
      Unit::get_decltype(const ipr::Expr& e) {
         return *finish_type(types.make_decltype(e));
      }

      // ------------------------------
      // -- impl::Unit::get_function --
      // ------------------------------

      const ipr::Function&
      Unit::get_function(const ipr::Product& p, const ipr::Type& t,
                         const ipr::Sum& s, const ipr::Linkage& l) {
         return *finish_type(types.make_function(p, t, s, l));
      }

      const ipr::Function&
      Unit::get_function(const ipr::Product& p, const ipr::Type& t,
                         const ipr::Sum& s) {
         return get_function(p, t, s, get_cxx_linkage());
      }

      const ipr::Function&
      Unit::get_function(const ipr::Product& p, const ipr::Type& t,
                         const ipr::Linkage& l) {
         ref_sequence<ipr::Type> ex;
         ex.push_back(&ellipsistype);
         return get_function(p, t, get_sum(ex), l);
      }

      const ipr::Function&
      Unit::get_function(const ipr::Product& p, const ipr::Type& t) {
         return get_function(p, t, get_cxx_linkage());
      }
      // -----------------------------
      // -- impl::Unit::get_pointer --
      // -----------------------------

      const ipr::Pointer&
      Unit::get_pointer(const ipr::Type& t) {
         return *finish_type(types.make_pointer(t));
      }

      // -----------------------------
      // -- impl::Unit::get_product --
      // -----------------------------

      const ipr::Product&
      Unit::get_product(const ref_sequence<ipr::Type>& s) {
         return *finish_type
            (types.make_product(*type_seqs.insert(s, unary_compare())));
      }

      // -----------------------------------
      // -- impl::Unit::get_ptr_to_member --
      // -----------------------------------

      const ipr::Ptr_to_member&
      Unit::get_ptr_to_member(const ipr::Type& s, const ipr::Type& t) {
         return *finish_type(types.make_ptr_to_member(s, t));
      }

      // -------------------------------
      // -- impl::Unit::get_qualified --
      // -------------------------------

      const ipr::Qualified&
      Unit::get_qualified(ipr::Type_qualifier cv, const ipr::Type& t) {
         assert (cv != ipr::Type_qualifier::None);
         return *finish_type(types.make_qualified(cv, t));
      }

      // -------------------------------
      // -- impl::Unit::get_reference --
      // -------------------------------

      const ipr::Reference&
      Unit::get_reference(const ipr::Type& t) {
         return *finish_type(types.make_reference(t));
      }

      // -------------------------------------
      // -- impl::Unit::get_value_reference --
      // -------------------------------------

      const ipr::Rvalue_reference&
      Unit::get_rvalue_reference(const ipr::Type& t) {
         return *finish_type(types.make_rvalue_reference(t));
      }

      // -------------------------
      // -- impl::Unit::get_sum --
      // -------------------------

      const ipr::Sum&
      Unit::get_sum(const ref_sequence<ipr::Type>& s) {
         return *finish_type
            (types.make_sum(*type_seqs.insert(s, unary_compare())));
      }

      // ------------------------------
      // -- impl::Unit::get_template --
      // ------------------------------

      const ipr::Template&
      Unit::get_template(const ipr::Product& p, const ipr::Type& t) {
         return *finish_type(types.make_template(p, t));
      }

      impl::Class*
      Unit::make_class(const ipr::Region& pr) {
         impl::Class* c = types.make_class(pr, anytype);
         c->constraint = &classtype;
         return c;
      }

      impl::Enum*
      Unit::make_enum(const ipr::Region& pr, Enum::Kind k) {
         impl::Enum* e = types.make_enum(pr, anytype, k);
         e->constraint = &enumtype;
         return e;
      }

      impl::Namespace*
      Unit::make_namespace(const ipr::Region& pr) {
         impl::Namespace* ns = types.make_namespace(&pr, anytype);
         ns->constraint = &namespacetype;
         return ns;
      }

      impl::Union*
      Unit::make_union(const ipr::Region& pr) {
         impl::Union* u = types.make_union(pr, anytype);
         u->constraint = &anytype;
         return u;
      }

      impl::Mapping*
      Unit::make_mapping(const ipr::Region& r) {
         return expr_factory::make_mapping(r, anytype);
      }

      impl::Parameter*
      Unit::make_parameter(const ipr::Name& n, const ipr::Type& t,
                           impl::Mapping& m) {
         return m.param(n, *rname_for_next_param(m, t));
      }

      const ipr::Auto& Unit::get_auto()
      {
         auto t = autos.make();
         t->id = &get_identifier("auto");
         return *t;
      }

//       int
//       Unit::make_fileindex(const ipr::String& s)
//       {
//          Filemap::iterator where = std::find(filemap.begin(),
//                                              filemap.end(), s);
//          if (where == filemap.end())
//             filemap.push_back(s);
//          return std::distance(filemap.begin(), where);
//       }

//       const ipr::String&
//       Unit::to_filename(int index) const
//       {
//          if (index < 0 || index >= 0)
//             throw std::domain_error("invalid file index");

//          Filemap::const_iterator what = filemap.begin();
//          std::advance(what, index);
//          return *what;
//       }
   }
}


/*
#include <ipr/impl>
#include <ipr/io>
#include <iostream>

int main()
{
   using namespace ipr;
   impl::Unit unit;              // current translation unit

   impl::Scope* global_scope = unit.global_scope();

   // Build the variable's name,
   const Name* name = unit.make_identifier("bufsz");
   // then its type,
   const Type* type = unit.make_cv_qualified(unit.Int(), Type::Const);
   // and the actual impl::Var node,
   impl::Var* var = global_scope->make_var(*name, *type);
   // set its initializer,
   var->init = unit.make_literal(unit.Int(), "1024");
   // and inject it into its scope.

   // Print out the whole translation unit
   Printer pp(std::cout);
   pp << xpr_declaration(unit.global_members());
   std::cout << std::endl;

}

*/
