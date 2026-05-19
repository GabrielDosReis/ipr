// -*- C++ -*-
//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copyright and license notices.
//
// Module interface: cxx.ipr.impl
// Implementation layer for the IPR: utility types, template infrastructure,
// node types, factory classes, Lexicon, and translation units.

module;

#include <ipr/std-preamble>

export module cxx.ipr.impl;

export import cxx.ipr;

// -------------------
// -- Utility types --
// -------------------

namespace ipr::util {
   template<typename T>
   struct ref {
      ref(T* p = { }) : ptr{p} { }
      T& get() const { return *util::check(ptr); }
   private:
      T* ptr;
   };

   namespace rb_tree {
      enum class Color { Black, Red };

      template<class Node>
      struct link {
         enum Dir { Left, Right, Parent };

         Node*& parent() { return arm[Parent]; }
         Node*& left() { return arm[Left]; }
         Node*& right() { return arm[Right]; }

         Node* arm[3] { };
         Color color = Color::Red;
      };

      template<class Node>
      struct core {
         std::ptrdiff_t size() const { return count; }

      protected:
         Node* root { };
         std::ptrdiff_t count { };

         void rotate_left(Node*);
         void rotate_right(Node*);
         void fixup_insert(Node*);
      };

      template<class Node>
      void
      core<Node>::rotate_left(Node* x)
      {
         Node* y = x->right();
         x->right() = y->left();
         if (y->left() != nullptr)
            y->left()->parent() = x;

         y->parent() = x->parent();
         if (x->parent() == nullptr)
            this->root = y;
         else if (x->parent()->left() == x)
            x->parent()->left() = y;
         else
            x->parent()->right() = y;

         y->left() = x;
         x->parent() = y;
      }

      template<class Node>
      void
      core<Node>::rotate_right(Node* x)
      {
         Node* y = x->left();

         x->left() = y->right();
         if (y->right() != nullptr)
            y->right()->parent() = x;

         y->parent() = x->parent();
         if (x->parent() == nullptr)
            this->root = y;
         else if (x->parent()->right() == x)
            x->parent()->right() = y;
         else
            x->parent()->left() = y;

         y->right() = x;
         x->parent() = y;
      }

      template<class Node>
      void
      core<Node>::fixup_insert(Node* z)
      {
         while (z != root and z->parent()->color == Color::Red) {
            if (z->parent() == z->parent()->parent()->left()) {
               Node* y = z->parent()->parent()->right();
               if (y != nullptr and y->color == Color::Red) {
                  z->parent()->color = Color::Black;
                  y->color = Color::Black;
                  z->parent()->parent()->color = Color::Red;
                  z = z->parent()->parent();
               } else {
                  if (z->parent()->right() == z) {
                     z = z->parent();
                     rotate_left(z);
                  }
                  z->parent()->color = Color::Black;
                  z->parent()->parent()->color = Color::Red;
                  rotate_right(z->parent()->parent());
               }
            } else {
               Node* y = z->parent()->parent()->left();
               if (y != nullptr and y->color == Color::Red) {
                  z->parent()->color = Color::Black;
                  y->color = Color::Black;
                  z->parent()->parent()->color = Color::Red;
                  z = z->parent()->parent();
               } else {
                  if (z->parent()->left() == z) {
                     z = z->parent();
                     rotate_right(z);
                  }
                  z->parent()->color = Color::Black;
                  z->parent()->parent()->color = Color::Red;
                  rotate_left(z->parent()->parent());
               }
            }
         }

         root->color = Color::Black;
      }

      template<class Node>
      struct chain : core<Node> {
         template<class Comp>
         Node* insert(Node*, Comp);

         template<typename Key, class Comp>
         Node* find(const Key&, Comp) const;
      };

      template<class Node>
      template<typename Key, class Comp>
      Node*
      chain<Node>::find(const Key& key, Comp comp) const
      {
         bool found = false;
         Node* result = this->root;
         while (result != nullptr and not found) {
            auto ordering = comp(*result, key) ;
            if (ordering < 0)
               result = result->left();
            else if (ordering > 0)
               result = result->right();
            else
               found = true;
         }

         return result;
      }

      template<class Node>
      template<class Comp>
      Node*
      chain<Node>::insert(Node* z, Comp comp)
      {
         Node** slot = &this->root;
         Node* up = nullptr;

         bool found = false;
         while (not found and *slot != nullptr) {
            auto ordering = comp(**slot, *z);
            if (ordering < 0) {
               up = *slot;
               slot = &up->left();
            }
            else if (ordering > 0) {
               up = *slot;
               slot = &up->right();
            }
            else
               found = true;
         }

         if (this->root == nullptr) {
            this->root = z;
            z->color = Color::Black;
         }
         else if (*slot == nullptr) {
            *slot = z;
            z->parent() = up;
            z->color = Color::Red;
            this->fixup_insert(z);
         }

         ++this->count;
         return z;
      }

      template<typename T>
      struct node : link<node<T>> {
         T data;
      };

      template<typename T>
      struct container : core<node<T>>, private std::allocator<node<T>> {
         template<typename Key, class Comp>
         T* find(const Key&, Comp) const;

         template<class Key, class Comp>
         T* insert(const Key&, Comp);

      private:
         template<class U>
         node<T>* make_node(const U& u) {
            node<T>* n = this->allocate(1);
            new (&n->data) T(u);
            n->left() = nullptr;
            n->right() = nullptr;
            n->parent() = nullptr;
            return n;
         }

         void destroy_node(node<T>* n) {
            if (n != nullptr) {
               n->data.~T();
               this->deallocate(n, 1);
            }
         }
      };

      template<typename T>
      template<typename Key, class Comp>
      T*
      container<T>::find(const Key& key, Comp comp) const
      {
         for (node<T>* x = this->root; x != nullptr; ) {
            auto ordering = comp(x->data, key);
            if (ordering < 0)
               x = x->left();
            else if (ordering > 0)
               x = x->right();
            else
               return &x->data;
         }

         return nullptr;
      }

      template<typename T>
      template<typename Key, class Comp>
      T*
      container<T>::insert(const Key& key, Comp comp)
      {
         if (this->root == nullptr) {
            this->root = make_node(key);
            this->root->color = Color::Black;
            ++this->count;
            return &this->root->data;
         }

         node<T>** slot = &this->root;
         node<T>* parent = nullptr;
         node<T>* where = nullptr;
         bool found = false;

         for (where = this->root; where != nullptr and not found; where = *slot) {
            auto ordering = comp(where->data, key);
            if (ordering < 0) {
               parent = where;
               slot = &where->left();
            }
            else if (ordering > 0) {
               parent = where;
               slot = &where->right();
            }
            else
               found = true;
         }

         if (where == nullptr) {
            where = *slot = make_node(key);
            where->parent() = parent;
            where->color = Color::Red;
            ++this->count;
            this->fixup_insert(where);
         }

         return &where->data;
      }
   }

   struct string {
      struct arena;

      using size_type = std::ptrdiff_t;

      static constexpr size_type padding_count = sizeof (size_type);

      size_type size() const { return length; }
      char operator[](size_type) const;

      const char8_t* begin() const { return data; }
      const char8_t* end() const { return begin() + length; }

      size_type length;
      char8_t data[padding_count];
   };

   struct string::arena {
      arena();
      ~arena();

      const string* make_string(const char8_t*, size_type);

   private:
      util::string* allocate(size_type);
      auto remaining_header_count() const
      {
         return mem->storage + bufsz - next_header;
      }

      struct pool;

      static constexpr size_type headersz = sizeof (util::string);
      static constexpr size_type bufsz = headersz << (20 - sizeof (pool*));

      struct pool {
         pool* previous;
         util::string storage[bufsz];
      };

      static constexpr size_type poolsz = sizeof (pool);

      pool* mem;
      string* next_header;
   };

   struct lexicographical_compare {
      template<typename In1, typename In2, class Compare>
      int operator()(In1 first1, In1 last1, In2 first2, In2 last2,
                     Compare compare) const
      {
         for (; first1 != last1 and first2 != last2; ++first1, ++first2)
            if (auto cmp = compare(*first1, *first2))
               return cmp;

         return first1 == last1 ? (first2 == last2 ? 0 : -1) : 1;
      }
   };
}

// -----------------------------------
// -- Base implementation templates --
// -----------------------------------

namespace ipr::util {
   struct immotile {
      constexpr immotile() = default;
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~immotile() = default;
      immotile(immotile&&) = delete;
      immotile(const immotile&) = delete;
      immotile& operator=(immotile&&) = delete;
      immotile& operator=(const immotile&) = delete;
   };

   enum class hash_code : std::size_t { };
}

namespace ipr::impl {
   template<class T>
   struct Node : T {
      using Interface = T;
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Node() = default;
      void accept(ipr::Visitor& v) const final { v.visit(*this); }
   };

   template<typename T>
   struct immotile_node : impl::Node<T>, util::immotile { };

   template<typename T>
   using immotile_base = immotile_node<T>;

   template<class Interface>
   struct Basic_unary : Interface {
      using typename Interface::Arg_type;
      Arg_type rep;

      explicit constexpr Basic_unary(Arg_type a) : rep{a} { }
      constexpr Arg_type operand() const final { return rep; }
   };

   template<typename T>
   using Unary_node = impl::Basic_unary<immotile_node<T>>;

   template<class Interface>
   struct Basic_binary : Interface {
      using typename Interface::Arg1_type;
      using typename Interface::Arg2_type;
      struct Rep {
         Arg1_type first;
         Arg2_type second;
      };
      Rep rep;

      constexpr Basic_binary(const Rep& r) : rep{r} { }
      constexpr Basic_binary(Arg1_type f, Arg2_type s) : rep{f, s} { }
      constexpr Arg1_type first() const final { return rep.first; }
      constexpr Arg2_type second() const final { return rep.second; }
   };

   template<typename T>
   using Binary_node = impl::Basic_binary<immotile_node<T>>;

   template<class Interface>
   struct Ternary : Interface {
      using typename Interface::Arg1_type;
      using typename Interface::Arg2_type;
      using typename Interface::Arg3_type;
      struct Rep {
         Arg1_type first;
         Arg2_type second;
         Arg3_type third;
      };
      Rep rep;

      Ternary(const Rep& r) : rep(r) { }
      Ternary(Arg1_type f, Arg2_type s, Arg3_type t) : rep{ f, s, t } { }
      Arg1_type first() const final { return rep.first; }
      Arg2_type second() const final { return rep.second; }
      Arg3_type third() const final { return rep.third; }
   };

   template<class Interface>
   struct Quaternary : Interface {
      using typename Interface::Arg1_type;
      using typename Interface::Arg2_type;
      using typename Interface::Arg3_type;
      using typename Interface::Arg4_type;
      struct Rep {
         Arg1_type first;
         Arg2_type second;
         Arg3_type third;
         Arg4_type fourth;
      };
      Rep rep;

      Quaternary(const Rep& r) : rep(r) { }
      Quaternary(Arg1_type f, Arg2_type s, Arg3_type t, Arg4_type u) : rep{ f, s, t, u } { }
      Arg1_type first() const final { return rep.first; }
      Arg2_type second() const final { return rep.second; }
      Arg3_type third() const final { return rep.third; }
      Arg4_type fourth() const final { return rep.fourth; }
   };

   using Type_id = Unary_node<ipr::Type_id>;
}

// ------------------------------
// -- Sequence implementations --
// ------------------------------

namespace ipr::impl {
   template<typename T>
   concept Has_interface = requires { typename T::Interface; };

   template<typename T>
   struct abstraction {
      using type = T;
   };

   template<Has_interface T>
   struct abstraction<T> {
      using type = typename T::Interface;
   };

   template<typename T, template<typename...> class C>
   struct abstraction<C<T>> : abstraction<T> { };

   template<typename T>
   using projection = typename abstraction<T>::type;

   template<typename T>
   struct ref_sequence : ipr::Sequence<T>, private std::vector<const void*> {
      using Seq = Sequence<T>;
      using Rep = std::vector<const void*>;
      using pointer = const T*;
      using Iterator = typename Seq::Iterator;
      using Index = typename Seq::Index;
      explicit ref_sequence(std::size_t n = 0) : Rep(n) { }
      Index size() const final { return Rep::size(); }

      using Seq::begin;
      using Seq::end;
      using Rep::resize;
      using Rep::push_back;
      const T& get(Index p) const final { return *pointer(this->at(p)); }
   };

   export template<typename T>
   struct Warehouse : private ref_sequence<T> {
      using Rep = ref_sequence<T>;
      explicit Warehouse(std::size_t n = 0) : Rep(n) { }

      Rep& rep() { return *this; }
      const Rep& rep() const { return *this; }
      using Rep::Seq::begin;
      using Rep::Seq::end;
      using Rep::size;
      void push_back(const T& item) { Rep::push_back(&item); }
   };

   template<typename T>
   struct stable_farm : std::forward_list<T> {
      using std::forward_list<T>::forward_list;
      template<typename... Args>
      T* make(Args&&... args)
      {
         this->emplace_front(std::forward<Args>(args)...);
         return &this->front();
      }
   };

   template<typename T>
   struct obj_sequence : ipr::Sequence<projection<T>>, private std::deque<T> {
      using Seq = ipr::Sequence<projection<T>>;
      using Impl = std::deque<T>;
      using Iterator = typename Seq::Iterator;
      using Index = typename Seq::Index;
      using Seq::begin;
      using Seq::end;

      Index size() const final { return Impl::size(); }
      const projection<T>& get(Index p) const final
      {
         if (p < 0 or p >= size())
            throw std::domain_error("obj_sequence::get");
         return backing_store()[p];
      }

      Impl& backing_store() { return *this; }
      const Impl& backing_store() const { return *this; }

      template<typename... Args>
      T* push_back(Args&&... args)
      {
         return &Impl::emplace_back(std::forward<Args>(args)...);
      }
   };

   template<typename T>
   struct obj_list : ipr::Sequence<projection<T>>, private stable_farm<T> {
      using Seq = ipr::Sequence<projection<T>>;
      using Impl = stable_farm<T>;
      using Iterator = typename Seq::Iterator;
      using Index = typename Seq::Index;
      using Seq::begin;
      using Seq::end;

      obj_list() : mark{this->before_begin()} { }

      Index size() const final
      {
         return std::distance(Impl::begin(), Impl::end());
      }

      Impl& backing_store() { return *this; }
      const Impl& backing_store() const { return *this; }
      const projection<T>& get(Index p) const final
      {
         if (p < 0 or p >= size())
            throw std::domain_error("obj_list::get");

         auto b = Impl::begin();
         std::advance(b, p);
         return *b;
      }

      template<typename... Args>
      T* push_back(Args&&... args)
      {
         mark = Impl::emplace_after(mark, std::forward<Args>(args)...);
         return &*mark;
      }
   private:
      typename Impl::iterator mark;
   };

   template<class T>
   struct empty_sequence : ipr::Sequence<T> {
      using Index = typename Sequence<T>::Index;
      Index size() const final { return 0; }

      const T& get(Index) const final
      {
         throw std::domain_error("empty_sequence::get");
      }
   };

   template<typename T>
   struct singleton {
      using iterator = T*;
      using const_iterator = const T*;
      template<typename... Args>
      explicit singleton(Args&&... args) : item{std::forward<Args>(args)...} { }
      iterator begin() { return &item; }
      const_iterator begin() const { return &item; }
      iterator end() { return begin() + 1; }
      const_iterator end() const { return begin() + 1; }
      T& front() { return item; }
      const T& front() const { return item; }
   private:
      T item;
   };

   template<typename T>
   struct singleton_obj : ipr::Sequence<projection<T>> {
      using Index = typename Sequence<projection<T>>::Index;

      template<typename... Args>
      singleton_obj(Args&&... args) : seq{std::forward<Args>(args)...} { }
      Index size() const final { return 1; }
      const projection<T>& get(Index i) const final
      {
         if (i == 0)
            return seq.front();
         throw std::domain_error("singleton::get");
      }
      const T& element() const { return seq.front(); }
      singleton<T>& backing_store() { return seq; }
      const singleton<T>& backing_store() const { return seq; }
   private:
      impl::singleton<T> seq;
   };
}

// ------------------------------------------------
// -- Expression, type, and scope base templates --
// ------------------------------------------------

namespace ipr::impl {
   const ipr::Transfer& cxx_transfer();
   const ipr::Language_linkage& c_linkage();
   const ipr::Language_linkage& cxx_linkage() { return cxx_transfer().language_linkage(); }
   const ipr::Type& typename_type();

   template<class Operation>
   struct Classic : Operation {
      Optional<ipr::Expr> op_impl { };
      using Operation::Operation;

      Optional<ipr::Expr> implementation() const final { return op_impl; }
   };

   template<class Interface>
   struct Expr : immotile_node<Interface> {
      Optional<ipr::Type> typing;
      constexpr Expr(Optional<ipr::Type> t = { }) : typing{ t } { }
      const ipr::Type& type() const final { return typing.get(); }
   };

   template<typename T>
   using Unary_expr = Basic_unary<Expr<T>>;
   template<typename T>
   using Classic_unary_expr = Classic<Unary_expr<T>>;
   template<typename T>
   using Binary_expr = impl::Basic_binary<Expr<T>>;
   template<typename T>
   using Classic_binary_expr = Classic<Binary_expr<T>>;

   template<class T>
   struct Type : impl::Node<T> {
      const ipr::Transfer& transfer() const final { return impl::cxx_transfer(); }
   };

   template<typename T>
   struct Composite : impl::Type<T> {
      constexpr Composite() : id{*this} { }
      const ipr::Type& type() const final { return impl::typename_type(); }
      const ipr::Name& name() const final { return id; }
   private:
      impl::Type_id id;
   };

   template<typename T>
   using Unary_type = impl::Basic_unary<Composite<T>>;
   template<typename T>
   using Binary_type = impl::Basic_binary<Composite<T>>;
   template<typename T>
   using Ternary_type = Ternary<Composite<T>>;
}

// --------------------------
// -- Scope infrastructure --
// --------------------------

namespace ipr::impl {
   struct scope_datum : util::rb_tree::link<scope_datum> {
      ipr::Specifiers spec = { };
      const ipr::Decl* decl = { };
   };

   struct decl_sequence : ref_sequence<ipr::Decl> { };

   template<typename T>
   struct singleton_ref : ipr::Sequence<T> {
      using Index = typename Sequence<T>::Index;
      const T& datum;
      explicit singleton_ref(const T& t) : datum(t) { }
      Index size() const final { return 1; }
      const T& get(Index i) const final
      {
         if (i == 0)
            return datum;
         throw std::domain_error("singleton_ref::get");
      }
   };

   template<typename T>
   struct singleton_overload : immotile_node<ipr::Overload> {
      T decl;
      template<typename... Args>
      explicit singleton_overload(const Args&... args) : decl{args...} { }
      const ipr::Name& name() const { return decl.name(); }
      const ipr::Type& type() const final { return decl.type(); }
      Optional<ipr::Decl> operator[](const ipr::Type& t) const final
      {
         if (physically_same(t, decl.type()))
            return decl;
         return { };
      }
      operator const T&() const { return decl; }
   };

   template<class Seq>
   struct typed_sequence : impl::Composite<ipr::Product>,
                           ipr::Sequence<ipr::Type> {
      using Index = typename Sequence<ipr::Type>::Index;
      Seq seq;

      typed_sequence() { }
      explicit typed_sequence(const Seq& s) : seq(s) { }
      template<typename... Args>
      explicit typed_sequence(Args&&... args) : seq(std::forward<Args>(args)...) { }
      const ipr::Sequence<ipr::Type>& operand() const final { return *this; }
      Index size() const final { return seq.size(); }
      const ipr::Type& get(Index i) const final
      {
         return seq.get(i).type();
      }
   };

   template<typename Member, template<typename> class Seq = obj_list>
   struct homogeneous_scope : impl::Node<ipr::Scope>,
                              ipr::Sequence<ipr::Decl>,
                              ipr::Sequence<ipr::Expr> {
      using Index = typename ipr::Sequence<ipr::Decl>::Index;
      using Members = Seq<singleton_overload<Member>>;
      typed_sequence<Members> decls;
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~homogeneous_scope() = default;
      template<typename... Args>
      homogeneous_scope(Args&&... args) : decls{std::forward<Args>(args)...} { }
      Index size() const final { return decls.size(); }
      const projection<Member>& get(Index i) const final { return decls.seq.get(i); }
      const ipr::Product& type() const final { return decls; }
      const ipr::Sequence<ipr::Decl>& elements() const final { return *this; }
      Optional<ipr::Overload> operator[](const Name&) const final;
      template<typename... Args>
      Member* push_back(const Args&... args)
      {
         return &decls.seq.push_back(args...)->decl;
      }
   };

   template<typename Member, template<typename> class Seq>
   Optional<ipr::Overload>
   homogeneous_scope<Member, Seq>::operator[](const ipr::Name& n) const
   {
      for (auto& decl : decls.seq.backing_store()) {
         if (physically_same(decl.name(), n))
            return { decl };
      }
      return { };
   }

   template<typename Member, template<typename> class Seq = obj_list>
   struct homogeneous_region : impl::Node<ipr::Region> {
      using location_span = ipr::Region::Location_span;
      const ipr::Region& parent;
      location_span extent;
      Optional<ipr::Expr> owned_by { };
      homogeneous_scope<Member, Seq> scope;
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~homogeneous_region() = default;

      explicit homogeneous_region(const ipr::Region& p) : parent(p) { }
      template<typename... Args>
      explicit homogeneous_region(const ipr::Region& r, Args&&... args)
         : parent{r}, scope{r, std::forward<Args>(args)...}
      { }
      const ipr::Region& enclosing() const final { return parent; }
      const ipr::Sequence<ipr::Expr>& body() const final { return scope; }
      const ipr::Scope& bindings() const final { return scope; }
      const location_span& span() const final { return extent; }
      Optional<ipr::Expr> owner() const final { return owned_by; }
      bool global() const final { return false; }
   };
}

// ----------------------------------------------------------
// -- Directive, statement, and declaration base templates --
// ----------------------------------------------------------

namespace ipr::impl {
   template<typename T, Phases f>
   struct Directive : impl::Expr<T> {
      Phases phases() const final { return f; }
   };

   template<class S>
   struct Stmt : S {
      ipr::Unit_location unit_locus;
      ipr::Source_location src_locus;
      ref_sequence<ipr::Annotation> notes;
      ref_sequence<ipr::Attribute> attrs;
      const ipr::Unit_location& unit_location() const final { return unit_locus; }
      const ipr::Source_location& source_location() const final { return src_locus; }
      const ipr::Sequence<ipr::Annotation>& annotation() const final { return notes; }
      const ipr::Sequence<ipr::Attribute>& attributes() const final { return attrs; }
   };

   template<typename S>
   using immotile_stmt = Stmt<immotile_node<S>>;

   template<class Interface>
   struct unique_decl : immotile_stmt<Interface> {
      unique_decl() : seq{*this} { }
      ipr::Specifiers specifiers() const override { return { }; }
      const ipr::Decl& master() const final { return *this; }
      const ipr::Language_linkage& language_linkage() const final { return impl::cxx_linkage(); }
      const ipr::Sequence<ipr::Decl>& decl_set() const final { return seq; }
   private:
      singleton_ref<ipr::Decl> seq;
   };
}

// ----------------------------------------------------------
// -- Comparison, overload, and declaration infrastructure --
// ----------------------------------------------------------

namespace ipr::impl {
   template<typename T,
         typename std::enable_if_t<std::is_scalar_v<T>, int> = 0>
   constexpr int compare(T lhs, T rhs)
   {
      constexpr std::less<> lt { };
      return lt(lhs, rhs) ? -1 : (lt(rhs, lhs) ? 1 : 0);
   }

   constexpr int compare(const ipr::Node& lhs, const ipr::Node& rhs)
   {
      return compare(&lhs, &rhs);
   }

   constexpr int compare(const ipr::String& lhs, const ipr::String& rhs)
   {
      return lhs.characters().compare(rhs.characters());
   }

   inline int compare(const ipr::Logogram& lhs, const ipr::Logogram& rhs)
   {
      return compare(lhs.what(), rhs.what());
   }

   inline int compare(const ipr::Language_linkage& lhs, const ipr::Language_linkage& rhs)
   {
      return compare(lhs.language(), rhs.language());
   }

   template<class Interface>
   struct Conversion_expr : impl::Classic<Binary_node<Interface>> {
      using Base = impl::Classic<Binary_node<Interface>>;
      using Base::Base;
      const ipr::Type& type() const final { return this->rep.first; }
   };

   template<class T>
   struct node_ref {
      const T& node;
      explicit node_ref(const T& t) : node(t) { }
   };

   struct overload_entry : util::rb_tree::link<overload_entry> {
      const ipr::Type& type;
      ref_sequence<ipr::Decl> declset;
      explicit overload_entry(const ipr::Type& t) : type(t) { }
   };

   template<class> struct master_decl_data;
   struct Overload;

   template<class Interface>
   struct basic_decl_data : scope_datum {
      master_decl_data<Interface>* master_data;

      explicit basic_decl_data(master_decl_data<Interface>* mdd)
            : master_data(mdd)
      { }
   };

   template<class Interface>
   struct master_decl_data : basic_decl_data<Interface>, overload_entry {
      Optional<Interface> def { };
      util::ref<const ipr::Language_linkage> lang_linkage { };

      impl::Overload* overload;
      const ipr::Region* home;
      master_decl_data(impl::Overload* ovl, const ipr::Type& t)
            : basic_decl_data<Interface>{this},
               overload_entry{t},
               overload{ovl},
               home{nullptr}
      { }
   };

   template<>
   struct master_decl_data<ipr::Template>
      : basic_decl_data<ipr::Template>, overload_entry {
      using Base = basic_decl_data<ipr::Template>;
      Optional<ipr::Template> def { };
      util::ref<const ipr::Language_linkage> lang_linkage { };
      const ipr::Template* primary;
      const ipr::Region* home;

      impl::Overload* overload;

      decl_sequence specs;

      master_decl_data(impl::Overload*, const ipr::Type&);
   };

   struct Overload : impl::Expr<ipr::Overload> {
      const ipr::Name& name;

      util::rb_tree::chain<overload_entry> entries;
      std::vector<scope_datum*> masters;

      explicit Overload(const ipr::Name&);
      Optional<ipr::Decl> operator[](const ipr::Type&) const final;
      overload_entry* lookup(const ipr::Type&) const;

      template<class T>
      void push_back(master_decl_data<T>*);
   };

   struct node_compare {
      int operator()(const ipr::Node& lhs, const ipr::Node& rhs) const
      {
         return compare(lhs, rhs);
      }

      int
      operator()(const overload_entry& e, const ipr::Type& t) const
      {
         return (*this)(e.type, t);
      }

      int
      operator()(const overload_entry& l, const overload_entry& r) const
      {
         return (*this)(l.type, r.type);
      }
   };

   template<class T>
   void
   Overload::push_back(master_decl_data<T>* data) {
      entries.insert(data, node_compare());
      masters.push_back(data);
   }

   template<typename Base>
   struct Controlled_stmt : immotile_stmt<Base> {
      using typename Base::Arg1_type;
      using typename Base::Arg2_type;
      util::ref<std::remove_reference_t<Arg1_type>> control;
      util::ref<std::remove_reference_t<Arg2_type>> stmt;
      Arg1_type first() const final { return control.get(); }
      Arg2_type second() const final { return stmt.get(); }
      const ipr::Type& type() const final { return second().type(); }
   };

   template<class D>
   struct Decl : immotile_stmt<D> {
      basic_decl_data<D> decl_data;

      Decl() : decl_data{ nullptr } { }

      const ipr::Language_linkage& language_linkage() const final
      {
         return util::check(decl_data.master_data)->lang_linkage.get();
      }

      const ipr::Region& home_region() const final {
         return *util::check(util::check(decl_data.master_data)->home);
      }

      using D::specifiers;
      void specifiers(ipr::Specifiers s) {
         decl_data.spec = s;
      }
   };

   template<typename T>
   struct decl_rep : T {
      using Interface = typename T::Interface;
      decl_rep(master_decl_data<Interface>* mdd)
      {
         this->decl_data.master_data = mdd;
         this->decl_data.decl = this;
         mdd->declset.push_back(this);
      }

      const ipr::Name& name() const final
      {
         return this->decl_data.master_data->overload->name;
      }

      ipr::Specifiers specifiers() const final
      {
         return this->decl_data.spec;
      }

      const ipr::Type& type() const final
      {
         return this->decl_data.master_data->type;
      }

      const ipr::Scope& scope() const
      {
         return this->decl_data.master_data->overload->where;
      }

      Decl_position position() const
      {
         return this->decl_data.scope_pos;
      }

      const ipr::Decl& master() const final
      {
         return *util::check(this->decl_data.master_data->decl);
      }

      const ipr::Sequence<ipr::Decl>& decl_set() const final
      {
         return this->decl_data.master_data->declset;
      }

      Optional<Interface> definition() const
      {
         return this->decl_data.master_data->def;
      }
   };

   template<typename T>
   struct decl_factory {
      using Interface = typename T::Interface;
      stable_farm<decl_rep<T>> decls;
      stable_farm<master_decl_data<Interface>> master_info;

      decl_rep<T>* declare(Overload* ovl, const ipr::Type& t)
      {
         master_decl_data<Interface>* data = master_info.make(ovl, t);
         decl_rep<T>* master = decls.make(data);
         ovl->push_back(data);

         return master;
      }

      decl_rep<T>* redeclare(overload_entry* decl)
      {
         return decls.make
            (static_cast<master_decl_data<Interface>*>(decl));
      }
   };
}

// ----------------
// -- Name nodes --
// ----------------

namespace ipr::impl {
   export using Logogram = Basic_unary<ipr::Logogram>;

   export using Comment = Unary_node<ipr::Comment>;

   export using Identifier = Unary_node<ipr::Identifier>;
   export using Suffix = Unary_node<ipr::Suffix>;
   export using Operator = Unary_node<ipr::Operator>;
   export using Conversion = Unary_node<ipr::Conversion>;
   export using Ctor_name = Unary_node<ipr::Ctor_name>;
   export using Dtor_name = Unary_node<ipr::Dtor_name>;
   export using Guide_name = Unary_node<ipr::Guide_name>;
   // Note: Type_id is defined earlier in this file, needed by Composite<T>.

   // -- Generic implementarion of ipr::Transfer
   export using Transfer = impl::Basic_binary<ipr::Transfer>;

   // -- Specialized implementation of ipr::Transfer
   // An object of this class pairs a specified language linkage with the natural
   // C++ calling convention.
   export struct Transfer_from_linkage : ipr::Transfer {
      constexpr explicit Transfer_from_linkage(const ipr::Language_linkage& l) : link{l} { }
      const ipr::Language_linkage& first() const final { return link; }
      const ipr::Calling_convention& second() const final;
   private:
      const ipr::Language_linkage& link;
   };

   // -- Specialized implementation of ipr::Transfer
   // An object of this class pairs the C++ language linkage with a specified calling convention.
   export struct Transfer_from_cc : ipr::Transfer {
      constexpr explicit Transfer_from_cc(const ipr::Calling_convention& cc) : conv{cc} { }
      const ipr::Language_linkage& first() const final;
      const ipr::Calling_convention& second() const final { return conv; }
   private:
      const ipr::Calling_convention& conv;
   };
}

// ----------------------------
// -- String and string pool --
// ----------------------------

namespace ipr::impl {
                             // -- impl::String --
   export struct String : immotile_node<ipr::String> {
      constexpr String(const char8_t* s) : txt{ s } { }
      constexpr String(util::word_view w) : txt{ w } { }
      constexpr util::word_view characters() const final { return txt; }
   private:
      const util::word_view txt;
   };
}

namespace ipr::util {
   // String pool.  Used to intern words used for external designation of entities.
   export struct string_pool : private std::map<util::hash_code, std::forward_list<impl::String>> {
      const ipr::String& intern(word_view);
   private:
      util::string::arena strings;
   };
}

// -----------------------------------------------
// -- Parameters (needed before cxx_form::impl) --
// -----------------------------------------------

namespace ipr::impl {
   export struct Parameter_list;

                              // -- impl::Parameter --
   export struct Parameter final : unique_decl<ipr::Parameter> {
      Optional<ipr::Expr> init;
      Parameter(const ipr::Name&, const ipr::Type&, Decl_position);
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return typing; }
      const ipr::Region& home_region() const final { return where.get().region(); }
      Mapping_level level() const final { return where.get().level(); }
      Decl_position position() const final { return pos; }
      Optional<ipr::Expr> initializer() const final { return init; }
   private:
      const ipr::Name& id;
      const ipr::Type& typing;
      util::ref<const ipr::Parameter_list> where;
      const Decl_position pos;
      friend struct Parameter_list;
   };

                              // -- impl::Parameter_list --
   export struct Parameter_list : impl::Node<ipr::Parameter_list> {
      Parameter_list(const ipr::Region&, Mapping_level);
      const ipr::Product& type() const final;
      const ipr::Region& region() const final;
      Mapping_level level() const final { return nesting; }
      const ipr::Sequence<ipr::Parameter>& elements() const final;
      impl::Parameter* add_member(const ipr::Name&, const ipr::Type&);

      // Need access by Mapping, Lambda and whomever creates Callable_species
      homogeneous_region<impl::Parameter> parms;
   private:
      const Mapping_level nesting;
   };

   // -- impl::Parameterization
   template<typename T, typename Base>
   struct Parameterization : Base {
      impl::Parameter_list inputs;
      util::ref<const T> body;
      Parameterization(const ipr::Region& r, Mapping_level l) : inputs{r, l} { }
      const ipr::Parameter_list& parameters() const final { return inputs; }
      const T& result() const final { return body.get(); }
   };
}

// ---------------------------------------------------------
// -- C++ syntactic form implementations (cxx_form::impl) --
// ---------------------------------------------------------

namespace ipr::cxx_form::impl {
   // -- implementation of ipr::cxx_form::Constraint
   template<typename T>
   struct Constraint : T, ipr::util::immotile {
      using Interface = T;
      void accept(Constraint_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Constraint::Monadic
   export struct Monadic_constraint : impl::Constraint<cxx_form::Constraint::Monadic> {
      explicit Monadic_constraint(const ipr::Identifier& n) : id{n} { }
      Monadic_constraint(const ipr::Expr& s, const ipr::Identifier& n) : outer{s}, id{n} { }
      Optional<ipr::Expr> scope() const final { return outer; }
      const ipr::Identifier& concept_name() const final { return id; }
   private:
      Optional<ipr::Expr> outer { };
      const ipr::Identifier& id;
   };

   // -- implementation of ipr::cxx_form::Constraint::Polyadic
   export struct Polyadic_constraint : impl::Constraint<cxx_form::Constraint::Polyadic> {
      ipr::impl::ref_sequence<ipr::Expr> args;
      explicit Polyadic_constraint(const ipr::Identifier& n) : id{n} { }
      Polyadic_constraint(const ipr::Expr& s, const ipr::Identifier& n) : outer{s}, id{n} { }
      Optional<ipr::Expr> scope() const final { return outer; }
      const ipr::Identifier& concept_name() const final { return id; }
      const ipr::Sequence<ipr::Expr>& trailing_arguments() const final { return args; }
   private:
      Optional<ipr::Expr> outer { };
      const ipr::Identifier& id;
   };

   // -- implementation of ipr::cxx_form::Requirement
   template<typename T>
   struct Requirement : T, ipr::util::immotile {
      using Interface = T;
      void accept(Requirement_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Requirement::Simple
   export struct Simple_requirement : impl::Requirement<cxx_form::Requirement::Simple> {
      explicit Simple_requirement(const ipr::Expr& x) : pattern{x} { }
      const ipr::Expr& expr() const final { return pattern; }
   private:
      const ipr::Expr& pattern;
   };

   // -- implementation of ipr::cxx_form::Requirement::Type
   export struct Type_requirement : impl::Requirement<cxx_form::Requirement::Type> {
      explicit Type_requirement(const ipr::Name& n) : name{n} { }
      Type_requirement(const ipr::Expr& s, const ipr::Name& n) : outer{s}, name{n} { }
      Optional<ipr::Expr> scope() const final { return outer; }
      const ipr::Name& type_name() const final { return name; }
   private:
      Optional<ipr::Expr> outer { };
      const ipr::Name& name;
   };

   // -- implementation of ipr::cxx_form::Requirement::Compound
   export struct Compound_requirement : impl::Requirement<cxx_form::Requirement::Compound> {
      Optional<cxx_form::Constraint> type;
      bool has_noexcept = false;
      explicit Compound_requirement(const ipr::Expr& x) : pattern{x} { }
      const ipr::Expr& expr() const final { return pattern; }
      Optional<cxx_form::Constraint> constraint() const final { return type; }
      bool nothrow() const final { return has_noexcept; }
   private:
      const ipr::Expr& pattern;
   };

   // -- implementation of ipr::cxx_form::Requirement::Compound
   export struct Nested_requirement : impl::Requirement<cxx_form::Requirement::Nested> {
      explicit Nested_requirement(const ipr::Expr& x) : cond{x} { }
      const ipr::Expr& condition() const final { return cond; }
   private:
      const ipr::Expr& cond;
   };

   // -- implementation of ipr::cxx_form::Indirector
   template<typename T>
   struct Indirector : T {
      using Interface = T;
      ipr::impl::ref_sequence<ipr::Attribute> attr_seq;
      const ipr::Sequence<ipr::Attribute>& attributes() const final { return attr_seq; }
      void accept(Indirector_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Indirector::Pointer
   export struct Pointer_indirector : impl::Indirector<cxx_form::Indirector::Pointer> {
      explicit Pointer_indirector(ipr::Qualifiers q) : quals{q} { }
      ipr::Qualifiers qualifiers() const final { return quals; }
   private:
      ipr::Qualifiers quals;
   };

   // -- implementation of ipr::cxx_form::Indirector::Reference
   export struct Reference_indirector : impl::Indirector<cxx_form::Indirector::Reference> {
      explicit Reference_indirector(Reference_flavor f) : how{f} { }
      Reference_flavor flavor() const final { return how; }
   private:
      Reference_flavor how { };
   };

   // -- implementation of ipr::cxx_form::Indirector::Member
   export struct Member_indirector : impl::Indirector<cxx_form::Indirector::Member> {
      Member_indirector(const ipr::Expr& s, ipr::Qualifiers q) : whole{s}, quals{q} { }
      const ipr::Expr& scope() const final { return whole; }
      ipr::Qualifiers qualifiers() const final { return quals; }
   private:
      const ipr::Expr& whole;
      ipr::Qualifiers quals;
   };

   // -- implementation of ipr::cxx_form::Morphism
   template<typename T>
   struct Morphism : T {
      using Interface = T;
      ipr::impl::ref_sequence<ipr::Attribute> attr_seq;
      const ipr::Sequence<ipr::Attribute>& attributes() const final { return attr_seq; }
      void accept(Morphism_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Morphism::Function
   export struct Function_morphism : impl::Morphism<cxx_form::Morphism::Function> {
      ipr::impl::Parameter_list inputs;
      Optional<ipr::Expr> eh_spec;
      ipr::Qualifiers quals { };
      Binding_mode ref_qual { };
      Function_morphism(const ipr::Region&, Mapping_level);
      const ipr::Parameter_list& parameters() const final { return inputs; }
      Optional<ipr::Expr> throws() const final { return eh_spec; }
      ipr::Qualifiers qualifiers() const final { return quals; }
      Binding_mode binding_mode() const final { return ref_qual; }
   };

   // -- implementation of ipr::cxx_form::Morphism::Array
   export struct Array_morphism : impl::Morphism<cxx_form::Morphism::Array> {
      Optional<ipr::Expr> array_bound;
      Optional<ipr::Expr> bound() const final { return array_bound; }
   };

   // -- implementation of ipr::cxx_form::Species_declarator
   template<typename T>
   struct Species : T {
      using Interface = T;
      ipr::impl::ref_sequence<cxx_form::Morphism> morphisms;
      const ipr::Sequence<cxx_form::Morphism>& suffix() const final { return morphisms; }
      void accept(Species_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // Provide implementation for common operations for the various classes implementing
   // classes for instances of declarator-id.
   template<typename T>
   struct Id_species : impl::Species<T> {
      ipr::impl::ref_sequence<ipr::Attribute> attr_seq;
      const ipr::Sequence<ipr::Attribute>& attributes() const final { return attr_seq; }
   };

   // Provide implementation for common operations for the various classes implementing
   // unqualified-id instances of declarator-id.
   template<typename T, typename S>
   struct Name_species : impl::Id_species<T> {
      Name_species() = default;
      explicit Name_species(const S& s) : id{s} { }
      Optional<S> name() const final { return id; }
   private:
      Optional<S> id{ };
   };

   // -- implementation of ipr::cxx_form::Species_declarator::Unqualified_id
   export using Unqualified_id_species = Name_species<cxx_form::Species_declarator::Unqualified_id, ipr::Name>;

   // -- implementation of ipr::cxx_form::Species_declarator::Pack
   export using Pack_species = Name_species<cxx_form::Species_declarator::Pack, ipr::Identifier>;

   // -- implementation of ipr::cxx_form::Species_declarator::Qualified_id
   export struct Qualified_id_species : impl::Id_species<cxx_form::Species_declarator::Qualified_id> {
      Qualified_id_species(const ipr::Expr& s, const ipr::Name& n) : outer{s}, id{n} { }
      const ipr::Expr& scope() const final { return outer; }
      const ipr::Name& member() const final { return id; }
   private:
      const ipr::Expr& outer;
      const ipr::Name& id;
   };

   // -- implementation of ipr::cxx_form::Species_declarator::Parenthesized
   export struct Parenthesized_species : impl::Species<cxx_form::Species_declarator::Parenthesized> {
      ipr::util::ref<const Declarator::Term> declarator;
      const Declarator::Term& term() const final { return declarator.get(); }
   };

   // -- implementation of ipr::cxx_form::Declarator.
   // Only the `accept()` operation is implemented here.  The other common operation, `species()`,
   // is individually implemented by each of the derived classes.
   template<typename T>
   struct Declarator : T {
      using Interface = T;
      void accept(cxx_form::Declarator_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Declarator::Term
   export struct Term_declarator : impl::Declarator<cxx_form::Declarator::Term> {
      ipr::impl::ref_sequence<cxx_form::Indirector> prefix;
      ipr::util::ref<cxx_form::Species_declarator> tail;
      const ipr::Sequence<cxx_form::Indirector>& indirectors() const final { return prefix; }
      const Species_declarator& species() const final { return tail.get(); }
   };

   // -- implementation of ipr::cxx_form::Declarator::Targeted
   export struct Targeted_declarator : impl::Declarator<cxx_form::Declarator::Targeted> {
      Targeted_declarator(const cxx_form::Species_declarator& s, const ipr::Type& t)
         : domain{s}, range{t} { }
      const cxx_form::Species_declarator& species() const final { return domain; }
      const ipr::Type& target() const final { return range; }
   private:
      const cxx_form::Species_declarator& domain;
      const ipr::Type& range;
   };

   // -- implementation of ipr::cxx_form::Initialization_provision
   template<typename T>
   struct Initialization_provision : T {
      void accept(cxx_form::Provision_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Elemental_initializer
   template<typename T>
   struct Elemental_initializer : T {
      using Interface = T;
      void accept(cxx_form::Initializer_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Subobject_designator
   template<typename T>
   struct Subobject_designator : T {
      using Interface = T;
      void accept(cxx_form::Designator_visitor& v) const final { v.visit(static_cast<const T&>(*this)); }
   };

   // -- implementation of ipr::cxx_form::Field_designator
   export struct Field_designator : impl::Subobject_designator<cxx_form::Field_designator> {
      explicit Field_designator(const ipr::Identifier& x) : id{x} { }
      const ipr::Identifier& name() const final { return id; }
   private:
      const ipr::Identifier& id;
   };

   // -- implementation of ipr::cxx_form::Slot_designator
   export struct Slot_designator : impl::Subobject_designator<cxx_form::Slot_designator> {
      explicit Slot_designator(const ipr::Expr& x) : idx{x} { }
      const ipr::Expr& index() const final { return idx; }
   private:
      const ipr::Expr& idx;
   };

   // -- implementation of ipr::cxx_form::Earmarked_initializer
   export struct Earmarked_initializer : cxx_form::Earmarked_initializer {
      using Interface = cxx_form::Earmarked_initializer;
      Earmarked_initializer(const cxx_form::Subobject_designator& f, const cxx_form::Initialization_provision& x)
         : sub{f}, init{x} { }
      const cxx_form::Subobject_designator& subobject() const final { return sub; }
      const cxx_form::Initialization_provision& initializer() const final { return init; }
   private:
      const cxx_form::Subobject_designator& sub;
      const cxx_form::Initialization_provision& init;
   };

   // -- implementation of ipr::cxx_form::Classic_provision
   export struct Classic_provision : impl::Initialization_provision<ipr::cxx_form::Classic_provision> {
      explicit Classic_provision(const ipr::cxx_form::Elemental_initializer& i) : init{i} { }
      const ipr::cxx_form::Elemental_initializer& initializer() const final { return init; }
   private:
      const ipr::cxx_form::Elemental_initializer& init;
   };

   // -- implementation of ipr::cxx_form::Parenthesized_provision
   export struct Parenthesized_provision : impl::Initialization_provision<ipr::cxx_form::Parenthesized_provision> {
      explicit Parenthesized_provision(const ipr::Expr& x) : expr{x} { }
      const ipr::Expr& initializer() const final { return expr; }
   private:
      const ipr::Expr& expr;
   };

   // -- implementation of ipr::cxx_form::Braced_provision
   export struct Braced_provision : impl::Initialization_provision<impl::Elemental_initializer<ipr::cxx_form::Braced_provision>> {
      ipr::impl::ref_sequence<cxx_form::Elemental_initializer> seq;
      const ipr::Sequence<cxx_form::Elemental_initializer>& elements() const final { return seq; }
   };

   // -- implementation of ipr::cxx_form::Designated_list_provision
   export struct Designated_list_provision : impl::Initialization_provision<impl::Elemental_initializer<cxx_form::Designated_list_provision>> {
      ipr::impl::obj_sequence<cxx_form::impl::Earmarked_initializer> seq;
      const ipr::Sequence<cxx_form::Earmarked_initializer>& elements() const final { return seq; }
   };   

   // -- Factory of C++ declarator forms.
   export struct form_factory {
      Monadic_constraint* make_monadic_constraint(const ipr::Identifier&);
      Monadic_constraint* make_monadic_constraint(const ipr::Expr&, const ipr::Identifier&);
      Polyadic_constraint* make_polyadic_constraint(const ipr::Identifier&);
      Polyadic_constraint* make_polyadic_constraint(const ipr::Expr&, const ipr::Identifier&);
      Simple_requirement* make_simple_requirement(const ipr::Expr&);
      Type_requirement* make_type_requirement(const ipr::Name&);
      Type_requirement* make_type_requirement(const ipr::Expr&, const ipr::Name&);
      Compound_requirement* make_compound_requirement(const ipr::Expr&);
      Nested_requirement* make_nested_requirement(const ipr::Expr&);

      Pointer_indirector* make_pointer_indirector(ipr::Qualifiers);
      Reference_indirector* make_reference_indirector(Reference_flavor);
      Member_indirector* make_member_indirector(const ipr::Expr&, ipr::Qualifiers);
      Unqualified_id_species* make_unqualified_id_species();
      Unqualified_id_species* make_unqualified_id_species(const ipr::Name&);
      Pack_species* make_pack_species();
      Pack_species* make_pack_species(const ipr::Identifier&);
      Qualified_id_species* make_qualified_id_species(const ipr::Expr&, const ipr::Name&);
      Parenthesized_species* make_parenthesized_species();
      Function_morphism* make_function_morphism(const ipr::Region& parent, Mapping_level level);
      Array_morphism* make_array_morphism();
      Term_declarator* make_term_declarator();
      Targeted_declarator* make_targeted_declarator(const cxx_form::Species_declarator&, const ipr::Type&);
      Classic_provision* make_classic_provision(const cxx_form::Elemental_initializer&);
      Parenthesized_provision* make_parenthesized_provision(const ipr::Expr&);
      Braced_provision* make_braced_provision();
      Designated_list_provision* make_designated_provision();
      Field_designator* make_field_designator(const ipr::Identifier&);
      Slot_designator* make_slot_designator(const ipr::Expr&);

   private:
      ipr::impl::stable_farm<Monadic_constraint> monadic_constraints;
      ipr::impl::stable_farm<Polyadic_constraint> polyadic_constraints;
      ipr::impl::stable_farm<Simple_requirement> simple_reqs;
      ipr::impl::stable_farm<Type_requirement> type_reqs;
      ipr::impl::stable_farm<Compound_requirement> compound_reqs;
      ipr::impl::stable_farm<Nested_requirement> nested_reqs;
      ipr::impl::stable_farm<Pointer_indirector> pointer_indirectors;
      ipr::impl::stable_farm<Reference_indirector> reference_indirectors;
      ipr::impl::stable_farm<Member_indirector> member_indirectors;
      ipr::impl::stable_farm<Unqualified_id_species> unqualified_id_species;
      ipr::impl::stable_farm<Pack_species> pack_species;
      ipr::impl::stable_farm<Qualified_id_species> qualified_id_species;
      ipr::impl::stable_farm<Parenthesized_species> paren_species;
      ipr::impl::stable_farm<Function_morphism> function_morphisms;
      ipr::impl::stable_farm<Array_morphism> array_morphisms;
      ipr::impl::stable_farm<Term_declarator> term_declarators;
      ipr::impl::stable_farm<Targeted_declarator> targeted_declarators;
      ipr::impl::stable_farm<Classic_provision> classic_provisions;
      ipr::impl::stable_farm<Parenthesized_provision> paren_provisions;
      ipr::impl::stable_farm<Braced_provision> braced_provisions;
      ipr::impl::stable_farm<Designated_list_provision> designated_provisions;
      ipr::impl::stable_farm<Field_designator> field_designators;
      ipr::impl::stable_farm<Slot_designator> slot_designators;
   };
}

// ---------------------------------
// -- Token, attributes, captures --
// ---------------------------------

namespace ipr::impl {
                              // -- impl::Token --
   export struct Token : ipr::Lexeme, ipr::Token {
      using Interface = ipr::Token;
      Token(const ipr::String&, const Source_location&, TokenValue, TokenCategory);
      const ipr::String& spelling() const final { return text; }
      const Source_location& locus() const final { return location; }
      const ipr::Lexeme& lexeme() const final { return *this; }
      TokenValue value() const final { return token_value; }
      TokenCategory category() const final { return token_category; }

   private:
      const ipr::String& text;
      Source_location location;
      TokenValue token_value;
      TokenCategory token_category;
   };

   // -- Parameterized implementation of ipr::Attribute.
   template<typename T>
   struct Attribute : T {
      void accept(typename T::Visitor& v) const final { v.visit(*this); }
   };

   template<typename T>
   using Unary_attribute = impl::Basic_unary<impl::Attribute<T>>;
   template<typename T>
   using Binary_attribute = impl::Basic_binary<impl::Attribute<T>>;

   export using BasicAttribute = Unary_attribute<ipr::BasicAttribute>;
   export using ScopedAttribute = Binary_attribute<ipr::ScopedAttribute>;
   export using LabeledAttribute = Binary_attribute<ipr::LabeledAttribute>;
   export using CalledAttribute = Binary_attribute<ipr::CalledAttribute>;
   export using ExpandedAttribute = Binary_attribute<ipr::ExpandedAttribute>;
   export using FactoredAttribute = Binary_attribute<ipr::FactoredAttribute>;
   export using ElaboratedAttribute = Unary_attribute<ipr::ElaboratedAttribute>;

   // -- Attributes factory --
   export struct attr_factory {
      const ipr::BasicAttribute& make_basic_attribute(const ipr::Token&);
      const ipr::ScopedAttribute& make_scoped_attribute(const ipr::Token&, const ipr::Token&);
      const ipr::LabeledAttribute& make_labeled_attribute(const ipr::Token&, const ipr::Attribute&);
      const ipr::CalledAttribute& make_called_attribute(const ipr::Attribute&, const ipr::Sequence<ipr::Attribute>&);
      const ipr::ExpandedAttribute& make_expanded_attribute(const ipr::Token&, const ipr::Attribute&);
      const ipr::FactoredAttribute& make_factored_attribute(const ipr::Token&, const ipr::Sequence<ipr::Attribute>&);
      const ipr::ElaboratedAttribute& make_elaborated_attribute(const ipr::Expr&);
   private:
      stable_farm<impl::BasicAttribute> basics;
      stable_farm<impl::ScopedAttribute> scopeds;
      stable_farm<impl::LabeledAttribute> labeleds;
      stable_farm<impl::CalledAttribute> calleds;
      stable_farm<impl::ExpandedAttribute> expandeds;
      stable_farm<impl::FactoredAttribute> factoreds;
      stable_farm<impl::ElaboratedAttribute> elaborateds;
   };

   // -- implementation for ipr::Capture
   export struct Capture : ipr::Capture {
      using Interface = ipr::Capture;
      Capture(const ipr::Decl& d, ipr::Binding_mode m) : decl{d}, md{m} { }
      ipr::Binding_mode mode() const final { return md; }
      const ipr::Decl& entity() const final { return decl; }
   private:
      const ipr::Decl& decl;
      const ipr::Binding_mode md;
   };

   // -- Generic implementation of ipr::Capture_specification
   template<typename T>
   struct Capture_specification : T {
      void accept(typename T::Visitor& v) const final { v.visit(*this); }
   };

   export struct Default_capture_specification : Capture_specification<ipr::Capture_specification::Default> {
      explicit Default_capture_specification(Binding_mode m) : md{m} { }
      Binding_mode mode() const final { return md; }
   private:
      const Binding_mode md;
   };

   export struct Implicit_object_capture_specification : Capture_specification<ipr::Capture_specification::Implicit_object> {
      explicit Implicit_object_capture_specification(Binding_mode m) : md{m} { }
      Binding_mode how() const final { return md; }
   private:
      const Binding_mode md;
   };

   export struct Enclosing_local_capture_specification : Capture_specification<ipr::Capture_specification::Enclosing_local> {
      Enclosing_local_capture_specification(const ipr::Decl& x, Binding_mode m) : decl{x}, md{m} { }
      Binding_mode mode() const final { return md; }
      const ipr::Identifier& name() const final;
      const ipr::Decl& declaration() const final { return decl; }
   private:
      const ipr::Decl& decl;
      const Binding_mode md;
   };

   export struct Binding_capture_specification : Capture_specification<ipr::Capture_specification::Binding> {
      Binding_capture_specification(const ipr::Identifier& n, const ipr::Expr& x, Binding_mode m) : id{n}, init{x}, md{m} { }
      Binding_mode mode() const final { return md; }
      const ipr::Identifier& name() const final { return id; }
      const ipr::Expr& initializer() const final { return init; }
   private:
      const ipr::Identifier& id;
      const ipr::Expr& init;
      const Binding_mode md;
   };

   export struct Expansion_capture_specification : Capture_specification<ipr::Capture_specification::Expansion> {
      explicit Expansion_capture_specification(const Named& x) : cap{x} { }
      const Named& what() const final { return cap; }
   private:
      const Named& cap;
   };

   // -- capture specification factory
   export struct capture_spec_factory {
      const ipr::Capture_specification::Default& default_capture(Binding_mode);
      const ipr::Capture_specification::Implicit_object& implicit_object_capture(Binding_mode);
      const ipr::Capture_specification::Enclosing_local& enclosing_local_capture(const ipr::Decl&, Binding_mode);
      const ipr::Capture_specification::Binding& binding_capture(const ipr::Identifier&, const ipr::Expr&, Binding_mode);
      const ipr::Capture_specification::Expansion& expansion_capture(const ipr::Capture_specification::Named&);
   private:
      stable_farm<impl::Default_capture_specification> defaults;
      stable_farm<impl::Implicit_object_capture_specification> implicits;
      stable_farm<impl::Enclosing_local_capture_specification> enclosings;
      stable_farm<impl::Binding_capture_specification> bindings;
      stable_farm<impl::Expansion_capture_specification> expansions;
   };

                              // -- impl::Module_name --
   export struct Module_name : ipr::Module_name {
      ref_sequence<ipr::Identifier> components;

      const ipr::Sequence<ipr::Identifier>& stems() const final;
   };
}

// ------------------
// -- Declarations --
// ------------------

namespace ipr::impl {
   // -- impl::symbolic_type
   // Representation of generalized built-in type.
   // All built-in types have type "typename" and, of course, have C++ linkage.
   // The type-parameter to this template is the type of identifier naming the type.
   template<std::derived_from<ipr::Identifier> T>
   struct symbolic_type : impl::Type<ipr::As_type> {
      constexpr explicit symbolic_type(const T& t) : id{t} { }
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return impl::typename_type(); }
      const ipr::Expr& operand() const final { return *this; }
   private:
      const T& id;
   };

   using extended_type = symbolic_type<ipr::Identifier>;

   export struct Base_type final : unique_decl<ipr::Base_type> {
      const ipr::Type& base;
      const ipr::Region& where;
      const Decl_position scope_pos;
      ipr::Specifiers spec;

      Base_type(const ipr::Type&, const ipr::Region&, Decl_position);
      const ipr::Type& type() const final { return base; }
      const ipr::Region& lexical_region() const final { return where; }
      const ipr::Region& home_region() const final { return where; }
      Decl_position position() const final { return scope_pos; }
      Optional<ipr::Expr> initializer() const final;
      Specifiers specifiers() const final { return spec; }
   };

   export struct Enumerator final : unique_decl<ipr::Enumerator> {
      const ipr::Name& id;
      const ipr::Enum& typing;
      const Decl_position scope_pos;
      util::ref<const ipr::Region> where;
      Optional<ipr::Expr> init;

      Enumerator(const ipr::Name&, const ipr::Enum&, Decl_position);
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return typing; }
      const ipr::Region& lexical_region() const final { return where.get(); }
      const ipr::Region& home_region() const final { return where.get(); }
      Decl_position position() const final { return scope_pos; }
      Optional<ipr::Expr> initializer() const final { return init; }
   };
}

// ----------------
// -- Type nodes --
// ----------------

namespace ipr::impl {
   export using Array = Binary_type<ipr::Array>;
   export using Decltype = Unary_type<ipr::Decltype>;
   export using As_type = Unary_type<ipr::As_type>;
   export using Tor = Binary_type<ipr::Tor>;
   export using Function = Ternary_type<ipr::Function>;
   export using Pointer = Unary_type<ipr::Pointer>;
   export using Product = Unary_type<ipr::Product>;
   export using Ptr_to_member = Binary_type<ipr::Ptr_to_member>;
   export using Qualified = Binary_type<ipr::Qualified>;
   export using Reference = Unary_type<ipr::Reference>;
   export using Rvalue_reference = Unary_type<ipr::Rvalue_reference>;
   export using Sum = Unary_type<ipr::Sum>;
   export using Forall = Binary_type<ipr::Forall>;

   // -- Specialized implementation of ipr::As_type.
   export struct As_type_with_transfer : immotile_node<ipr::As_type> {
      struct Rep {
         const ipr::Expr& expr;
         const ipr::Transfer& xfer;
      };

      explicit As_type_with_transfer(const Rep& r) : id{*this}, rep{r} { }
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return impl::typename_type(); }
      const ipr::Expr& operand() const final { return rep.expr; }
      const ipr::Transfer& transfer() const final { return rep.xfer; }
   private:
      Type_id id;
      Rep rep;
   };

   // -- Specialized implementation of ipr::Function.
   export struct Function_with_transfer : immotile_node<ipr::Function> {
      struct Rep {
         const ipr::Product& source;
         const ipr::Type& target;
         const ipr::Expr& throws;
         const ipr::Transfer& xfer;
      };

      explicit Function_with_transfer(const Rep& r) : id{*this}, rep{r} { }
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return impl::typename_type(); }
      const ipr::Transfer& transfer() const final { return rep.xfer; }
      const ipr::Product& first() const final { return rep.source; }
      const ipr::Type& second() const final { return rep.target; }
      const ipr::Expr& third() const final { return rep.throws; }
   private:
      Type_id id;
      Rep rep;
   };
}

// ----------------------
// -- Expression nodes --
// ----------------------

namespace ipr::impl {
   export using Phantom = Expr<ipr::Phantom>;
   export using Eclipsis = Expr<ipr::Eclipsis>;

   export struct Symbol final : Unary_expr<ipr::Symbol> {
      explicit constexpr Symbol(const ipr::Name& n) : Unary_expr<ipr::Symbol>{ n } { }
      constexpr Symbol(const ipr::Name& n, const ipr::Type& t)
          : Unary_expr<ipr::Symbol>{ n }
      { typing = t; }
   };

   export struct Lambda : impl::Parameterization<ipr::Expr, impl::Node<ipr::Lambda>> {
      util::ref<const ipr::Closure> typing;
      Optional<ipr::Type> value_type;
      Optional<ipr::Expr> decl_constraint;
      impl::ref_sequence<ipr::Attribute> attrs;
      impl::ref_sequence<ipr::Capture_specification> env_spec;
      Optional<ipr::Expr> eh;
      Lambda_specifiers lam_spec;

      Lambda(const ipr::Region&, Mapping_level);
      const ipr::Closure& type() const final { return typing.get(); }
      Optional<ipr::Type> target() const final { return value_type; }
      Optional<ipr::Expr> requirement() const final { return decl_constraint; }
      const ipr::Sequence<ipr::Attribute>& attributes() const final { return attrs; }
      Optional<ipr::Expr> eh_specification() const final { return eh; }
      Lambda_specifiers specifiers() const final { return lam_spec; }
      const ipr::Sequence<ipr::Capture_specification>& captures() const final { return env_spec; }
   };

   export struct Requires : immotile_node<ipr::Requires> {
      impl::Parameter_list formals;
      impl::ref_sequence<cxx_form::Requirement> requirements;
      Requires(const ipr::Region& r, Mapping_level l) : formals{r, l} { }
      const ipr::Type& type() const final;
      const ipr::Parameter_list& parameters() const final { return formals; }
      const ipr::Sequence<cxx_form::Requirement>& body() const final { return requirements; }
   };

   export using Address = Classic_unary_expr<ipr::Address>;
   export using Array_delete = Classic_unary_expr<ipr::Array_delete>;
   export using Complement = Classic_unary_expr<ipr::Complement>;
   export using Delete = Classic_unary_expr<ipr::Delete>;
   export using Demotion = Unary_expr<ipr::Demotion>;
   export using Deref = Classic_unary_expr<ipr::Deref>;

   export struct Asm : impl::Unary_node<ipr::Asm> {
      explicit Asm(const ipr::String&);
      const ipr::Type& type() const final;
   };

   export struct Expr_list : immotile_node<ipr::Expr_list> {
      typed_sequence<ref_sequence<ipr::Expr>> seq;

      Expr_list();
      explicit Expr_list(const ref_sequence<ipr::Expr>&);

      const ipr::Product& type() const final;

      const ipr::Sequence<ipr::Expr>& operand() const final;

      void push_back(const ipr::Expr* e) { seq.seq.push_back(e); }
   };

   export using Alignof = Unary_expr<ipr::Alignof>;
   export using Sizeof = Unary_expr<ipr::Sizeof>;
   export using Args_cardinality = Unary_expr<ipr::Args_cardinality>;
   export using Typeid = Unary_expr<ipr::Typeid>;
   export using Label = Unary_expr<ipr::Label>;

   export struct Restriction : Unary_node<ipr::Restriction> {
      explicit Restriction(const ipr::Expr& x) : Unary_node<ipr::Restriction>{x} { }
      const ipr::Type& type() const final;
   };

   export struct Id_expr : Unary_expr<ipr::Id_expr> {
      Optional<ipr::Expr> decls { };

      explicit Id_expr(const ipr::Name&);
      Optional<ipr::Expr> resolution() const final;
   };

   export using Materialization = Unary_expr<ipr::Materialization>;
   export using Not = Classic_unary_expr<ipr::Not>;

   export struct Enclosure : impl::Unary_expr<ipr::Enclosure> {
      Enclosure(ipr::Delimiter, const ipr::Expr&);
      Delimiter delimiters() const final { return delim; }
   private:
      Delimiter delim;
   };

   export using Pre_decrement = Classic_unary_expr<ipr::Pre_decrement>;
   export using Pre_increment = Classic_unary_expr<ipr::Pre_increment>;
   export using Post_decrement = Classic_unary_expr<ipr::Post_decrement>;
   export using Post_increment = Classic_unary_expr<ipr::Post_increment>;
   export using Promotion = Unary_expr<ipr::Promotion>;
   export using Read = Unary_expr<ipr::Read>;
   export using Throw = Classic_unary_expr<ipr::Throw>;

   export using Unary_minus = Classic_unary_expr<ipr::Unary_minus>;
   export using Unary_plus = Classic_unary_expr<ipr::Unary_plus>;
   export using Expansion = Classic_unary_expr<ipr::Expansion>;
   export using Construction = Classic_unary_expr<ipr::Construction>;
   export using Noexcept = Unary_expr<ipr::Noexcept>;

   export using Rewrite = Binary_node<ipr::Rewrite>;
   export using And = Classic_binary_expr<ipr::And>;
   export using Annotation = Binary_node<ipr::Annotation>;
   export using Array_ref = Classic_binary_expr<ipr::Array_ref>;
   export using Arrow = Classic_binary_expr<ipr::Arrow>;
   export using Arrow_star = Classic_binary_expr<ipr::Arrow_star>;
   export using Assign = Classic_binary_expr<ipr::Assign>;
   export using Bitand = Classic_binary_expr<ipr::Bitand>;
   export using Bitand_assign = Classic_binary_expr<ipr::Bitand_assign>;
   export using Bitor = Classic_binary_expr<ipr::Bitor>;
   export using Bitor_assign = Classic_binary_expr<ipr::Bitor_assign>;
   export using Bitxor = Classic_binary_expr<ipr::Bitxor>;
   export using Bitxor_assign = Classic_binary_expr<ipr::Bitxor_assign>;
   export using Call = Classic_binary_expr<ipr::Call>;
   export using Cast = Conversion_expr<ipr::Cast>;
   export using Coercion = Classic_binary_expr<ipr::Coercion>;
   export using Comma = Classic_binary_expr<ipr::Comma>;
   export using Const_cast = Conversion_expr<ipr::Const_cast>;
   export using Div = Classic_binary_expr<ipr::Div>;
   export using Div_assign = Classic_binary_expr<ipr::Div_assign>;
   export using Dot = Classic_binary_expr<ipr::Dot>;
   export using Dot_star = Classic_binary_expr<ipr::Dot_star>;
   export using Dynamic_cast = Conversion_expr<ipr::Dynamic_cast>;
   export using Equal = Classic_binary_expr<ipr::Equal>;
   export using Greater = Classic_binary_expr<ipr::Greater>;
   export using Greater_equal = Classic_binary_expr<ipr::Greater_equal>;
   export using Less = Classic_binary_expr<ipr::Less>;
   export using Less_equal = Classic_binary_expr<ipr::Less_equal>;
   export using Literal = Conversion_expr<ipr::Literal>;
   export using Lshift = Classic_binary_expr<ipr::Lshift>;
   export using Lshift_assign = Classic_binary_expr<ipr::Lshift_assign>;

   export struct Mapping : impl::Parameterization<ipr::Expr, impl::Expr<ipr::Mapping>> {
      Mapping(const ipr::Region&, Mapping_level);
      impl::Parameter* param(const ipr::Name& n, const ipr::Type& t)
      {
          return inputs.add_member(n, t);
      }
   };

   export using Member_init = Binary_expr<ipr::Member_init>;
   export using Minus = Classic_binary_expr<ipr::Minus>;
   export using Minus_assign = Classic_binary_expr<ipr::Minus_assign>;
   export using Modulo = Classic_binary_expr<ipr::Modulo>;
   export using Modulo_assign = Classic_binary_expr<ipr::Modulo_assign>;
   export using Mul = Classic_binary_expr<ipr::Mul>;
   export using Mul_assign = Classic_binary_expr<ipr::Mul_assign>;
   export using Narrow = Binary_expr<ipr::Narrow>;
   export using Not_equal = Classic_binary_expr<ipr::Not_equal>;
   export using Or = Classic_binary_expr<ipr::Or>;
   export using Plus = Classic_binary_expr<ipr::Plus>;
   export using Plus_assign = Classic_binary_expr<ipr::Plus_assign>;
   export using Pretend = Binary_expr<ipr::Pretend>;
   export using Qualification = Binary_expr<ipr::Qualification>;
   export using Reinterpret_cast = Conversion_expr<ipr::Reinterpret_cast>;
   export using Rshift = Classic_binary_expr<ipr::Rshift>;
   export using Rshift_assign = Classic_binary_expr<ipr::Rshift_assign>;
   export using Scope_ref = Classic_binary_expr<ipr::Scope_ref>;
   export using Static_cast = Conversion_expr<ipr::Static_cast>;
   export using Template_id = Binary_node<ipr::Template_id>;
   export using Widen = Binary_expr<ipr::Widen>;
   // A Where node where the attendant() is not a scope.
   export using Where_no_decl = Binary_node<ipr::Where>;

   export struct Instantiation : immotile_node<ipr::Instantiation> {
      Optional<ipr::Expr> result;
      Instantiation(const ipr::Expr& e, const ipr::Substitution& s) : expr{e}, subst{s} { }
      const ipr::Expr& pattern() const final { return expr; }
      const ipr::Substitution& substitution() const final { return subst; }
      Optional<ipr::Expr> instance() const final { return result; }
   private:
      const ipr::Expr& expr;
      const ipr::Substitution& subst;
   };

   export struct Binary_fold : Classic_binary_expr<ipr::Binary_fold> {
      Category_code fold_op;

      Binary_fold(Category_code, const ipr::Expr&, const ipr::Expr&);
      Category_code operation() const final;
   };

   export struct New : impl::Classic_binary_expr<ipr::New> {
      bool global = false;

      New(Optional<ipr::Expr_list>, const ipr::Construction&);
      bool global_requested() const final;
   };

   export using Conditional = Ternary<Classic<Expr<ipr::Conditional>>>;

   // An elementary substitution is a subsitution the domain of which is a singleton.
   export struct Elementary_substitution : ipr::Substitution {
      Elementary_substitution(const ipr::Parameter& p, const ipr::Expr& v) : parm{p}, value{v} { }
      const ipr::Expr& operator[](const ipr::Parameter& p) const final
      {
         return physically_same(p, parm) ? parm : value;
      }
   private:
      const ipr::Parameter& parm;
      const ipr::Expr& value;
   };

   // A general substitution is a substitution the domain of which has cardinality greater than one.
   export struct General_substitution : ipr::Substitution {
      const ipr::Expr& operator[](const ipr::Parameter&) const final;
      General_substitution& subst(const ipr::Parameter&, const ipr::Expr&);
   private:
      std::map<const ipr::Parameter*, const ipr::Expr*> mapping;
   };
}

// -----------------------
// -- Declaration nodes --
// -----------------------

namespace ipr::impl {
   export struct Template : impl::Decl<ipr::Template> {
      util::ref<impl::Mapping> init;
      util::ref<const ipr::Region> lexreg;

      Template();
      const ipr::Template& primary_template() const final;
      const ipr::Sequence<ipr::Decl>& specializations() const final;
      const ipr::Mapping& mapping() const final { return init.get(); }
      Optional<ipr::Expr> initializer() const final { return { mapping().result() }; }
      const ipr::Region& lexical_region() const final { return lexreg.get(); }
   };

   struct fundecl_data : std::variant<impl::Parameter_list*, impl::Mapping*> {
      ipr::FunctionTraits traits { };
      impl::Parameter_list* parameters() const { return std::get<0>(*this); }
      impl::Mapping* mapping() const { return std::get<1>(*this); }
   };

   export struct Alias : impl::Decl<ipr::Alias> {
      util::ref<const ipr::Expr> aliasee;

      Alias();
      Optional<ipr::Expr> initializer() const final { return aliasee.get(); }
   };

   export struct Var : impl::Decl<ipr::Var> {
      Optional<ipr::Expr> init;
      util::ref<const ipr::Region> lexreg;

      Var();
      Optional<ipr::Expr> initializer() const final { return init; }
      const ipr::Region& lexical_region() const final { return lexreg.get(); }
   };

   export struct Field : impl::Decl<ipr::Field> {
      Optional<ipr::Expr> init;

      Field();
      Optional<ipr::Expr> initializer() const final { return init; }
   };

   export struct Bitfield : impl::Decl<ipr::Bitfield> {
      util::ref<const ipr::Expr> length;
      Optional<ipr::Expr> init;

      Bitfield();
      const ipr::Expr& precision() const final { return length.get(); }
      Optional<ipr::Expr> initializer() const final { return init; }
   };

   export struct Typedecl : impl::Decl<ipr::Typedecl> {
      Optional<ipr::Type> init;
      util::ref<const ipr::Region> lexreg;

      Typedecl();
      Optional<ipr::Expr> initializer() const final { return init; }
      const ipr::Region& lexical_region() const final { return lexreg.get(); }
   };

   export struct Concept : impl::Decl<ipr::Concept> {
      impl::Parameter_list inputs;
      util::ref<const ipr::Expr> init;
      explicit Concept(const ipr::Region& r) : inputs{r, { }} { }
      const ipr::Parameter_list& parameters() const final { return inputs; }
      const ipr::Expr& result() const final { return init.get(); }
      Optional<ipr::Expr> initializer() const final { return { *this }; }
   };

   export struct Fundecl : impl::Decl<ipr::Fundecl> {
      fundecl_data data;
      util::ref<const ipr::Region> lexreg;

      Fundecl();

      ipr::FunctionTraits traits() const final { return data.traits; }
      const ipr::Parameter_list& parameters() const final;
      Optional<ipr::Mapping> mapping() const final;
      Optional<ipr::Expr> initializer() const final;
      const ipr::Region& lexical_region() const final { return lexreg.get(); }
   };
}

// ---------------------------------------
// -- Scope, Region, user-defined types --
// ---------------------------------------

namespace ipr::impl {
   export struct Scope : immotile_node<ipr::Scope> {
      Scope();
      const ipr::Type& type() const final { return decls; }
      const ipr::Sequence<ipr::Decl>& elements() const final { return decls.seq; }
      Optional<ipr::Overload> operator[](const ipr::Name&) const final;

      impl::Alias* make_alias(const ipr::Name&, const ipr::Expr&);
      impl::Var* make_var(const ipr::Name&, const ipr::Type&);
      impl::Field* make_field(const ipr::Name&, const ipr::Type&);
      impl::Bitfield* make_bitfield(const ipr::Name&, const ipr::Type&);
      impl::Typedecl* make_typedecl(const ipr::Name&, const ipr::Type&);
      impl::Fundecl* make_fundecl(const ipr::Name&, const ipr::Function&);
      impl::Template* make_primary_template(const ipr::Name&, const ipr::Forall&);
      impl::Template* make_secondary_template(const ipr::Name&, const ipr::Forall&);

   private:
      util::rb_tree::container<impl::Overload> overloads;
      typed_sequence<decl_sequence> decls;
      decl_factory<impl::Alias> aliases;
      decl_factory<impl::Var> vars;
      decl_factory<impl::Field> fields;
      decl_factory<impl::Bitfield> bitfields;
      decl_factory<impl::Fundecl> fundecls;
      decl_factory<impl::Typedecl> typedecls;
      decl_factory<impl::Template> primary_maps;
      decl_factory<impl::Template> secondary_maps;

      template<class T> void add_member(T*);
   };

   // GCC BUG: internal compiler error in is_really_empty_class from emit_mem_initializers
   // when this class has an explicit destructor.
   export struct Region : immotile_node<ipr::Region>, cxx_form::impl::form_factory {
      using location_span = ipr::Region::Location_span;
      Optional<ipr::Region> parent;
      location_span extent;
      Optional<ipr::Expr> owned_by { };
      impl::Scope scope;
      impl::ref_sequence<ipr::Expr> expr_seq;

      const ipr::Region& enclosing() const final { return parent.get(); }
      const ipr::Sequence<ipr::Expr>& body() const final { return expr_seq; }
      const ipr::Scope& bindings() const final { return scope; }
      const location_span& span() const final { return extent; }
      Optional<ipr::Expr> owner() const final { return owned_by; }
      bool global() const final { return not parent.is_valid(); }

      impl::Region* make_subregion();

      impl::Alias* declare_alias(const ipr::Name& n, const ipr::Type& t)
      {
         return scope.make_alias(n, t);
      }

      impl::Var* declare_var(const ipr::Name& n, const ipr::Type& t)
      {
         return scope.make_var(n, t);
      }

      impl::Field* declare_field(const ipr::Name& n, const ipr::Type& t)
      {
         return scope.make_field(n, t);
      }

      impl::Bitfield* declare_bitfield(const ipr::Name& n,
                                       const ipr::Type& t)
      {
         return scope.make_bitfield(n, t);
      }

      Typedecl* declare_type(const ipr::Name& n, const ipr::Type& t)
      {
         return scope.make_typedecl(n, t);
      }

      Fundecl* declare_fun(const ipr::Name& n, const ipr::Function& t)
      {
         return scope.make_fundecl(n, t);
      }

      Template* declare_primary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         return scope.make_primary_template(n, t);
      }

      Template* declare_secondary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         return scope.make_secondary_template(n, t);
      }

      explicit Region(Optional<ipr::Region>);

   private:
      stable_farm<Region> subregions;
   };

   // Implement common operations for user-defined types.
   template<class Interface>
   struct Udt : impl::Type<Interface> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Udt() = default;
      Region body;
      Optional<ipr::Name> id;
      explicit Udt(const ipr::Region* pr) : body(pr)
      { 
         body.owned_by = this;
      }
      const ipr::Name& name() const final { return id.get(); }
      const ipr::Region& region() const final { return body; }

      impl::Alias*
      declare_alias(const ipr::Name& n, const ipr::Type& t)
      {
         impl::Alias* alias = body.declare_alias(n, t);
         return alias;
      }

      impl::Field*
      declare_field(const ipr::Name& n, const ipr::Type& t)
      {
         impl::Field* field = body.declare_field(n, t);
         return field;
      }

      impl::Bitfield*
      declare_bitfield(const ipr::Name& n, const ipr::Type& t)
      {
         impl::Bitfield* field = body.declare_bitfield(n, t);
         return field;
      }

      impl::Var*
      declare_var(const ipr::Name& n, const ipr::Type& t)
      {
         impl::Var* var = body.declare_var(n, t);
         return var;
      }

      impl::Typedecl*
      declare_type(const ipr::Name& n, const ipr::Type& t)
      {
         impl::Typedecl* typedecl = body.declare_type(n, t);
         return typedecl;
      }

      impl::Fundecl*
      declare_fun(const ipr::Name& n, const ipr::Function& t)
      {
         impl::Fundecl* fundecl = body.declare_fun(n, t);
         return fundecl;
      }

      impl::Template*
      declare_primary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         impl::Template* map = body.declare_primary_template(n, t);
         return map;
      }

      impl::Template*
      declare_secondary_template(const ipr::Name& n, const ipr::Forall& t)
      {
         impl::Template* map = body.declare_secondary_template(n, t);
         return map;
      }
   };

   export struct Enum : impl::Type<ipr::Enum> {
      Optional<ipr::Type> underlying;
      homogeneous_region<impl::Enumerator, obj_sequence> body;
      Optional<ipr::Name> id;
      const Kind enum_kind;

      Enum(const ipr::Region&, Kind);
      const ipr::Name& name() const final { return id.get(); }
      const ipr::Type& type() const final;
      const ipr::Region& region() const final;
      const Sequence<ipr::Enumerator>& members() const final;
      Kind kind() const final;
      Optional<ipr::Type> base() const final { return underlying; }
      impl::Enumerator* add_member(const ipr::Name&);
   };

   export struct Union : Udt<ipr::Union> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Union() = default;
      explicit Union(const ipr::Region&);
      const ipr::Type& type() const final;
   };

   export struct Namespace : Udt<ipr::Namespace> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Namespace() = default;
      explicit Namespace(const ipr::Region*);
      const ipr::Type& type() const final;
   };

   export struct Class : impl::Udt<ipr::Class> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Class() = default;
      homogeneous_region<impl::Base_type> base_subobjects;
      explicit Class(const ipr::Region&);
      const ipr::Type& type() const final;
      const ipr::Sequence<ipr::Base_type>& bases() const final;
      impl::Base_type* declare_base(const ipr::Type&);
   };

   export struct Auto : impl::Composite<ipr::Auto> {
   };

   export struct Closure : impl::Udt<ipr::Closure> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Closure() = default;
      impl::obj_list<impl::Capture> captures;
      explicit Closure(const ipr::Region&);
      const ipr::Type& type() const final;
      const ipr::Sequence<ipr::Capture>& members() const final { return captures; }
   };
}

// -------------------------------
// -- Directives and statements --
// -------------------------------

namespace ipr::impl {
   export struct Specifiers_spread : impl::Directive<ipr::Specifiers_spread, Phases::Elaboration> {
      impl::ref_sequence<cxx_form::Proclamator> proc_seq;
      ipr::Specifiers specs { };

      const ipr::Sequence<cxx_form::Proclamator>& targets() const final { return proc_seq; }
      ipr::Specifiers specifiers() const final { return specs; }
   };

   export struct Structured_binding : impl::Directive<ipr::Structured_binding, Phases::Elaboration> {
      impl::ref_sequence<ipr::Identifier> ids;
      util::ref<const ipr::Expr> init;
      impl::ref_sequence<ipr::Decl> decl_seq;
      ipr::Specifiers specs { };
      ipr::Binding_mode binding_mode { };

      ipr::Specifiers specifiers() const final { return specs; }
      ipr::Binding_mode mode() const final { return binding_mode; }
      const ipr::Sequence<ipr::Identifier>& names() const final { return ids; }
      const ipr::Expr& initializer() const final { return init.get(); }
      const ipr::Sequence<ipr::Decl>& bindings() const final { return decl_seq; }
   };

   export struct single_using_declaration : impl::Directive<ipr::Using_declaration, Phases::Elaboration> {
      single_using_declaration(const ipr::Scope_ref&, Designator::Mode);
      const ipr::Sequence<Designator>& designators() const final { return what; }
   private:
      singleton_obj<Designator> what;
   };

   export struct Using_declaration : impl::Directive<ipr::Using_declaration, Phases::Elaboration> {
      impl::obj_list<Designator> seq;

      const ipr::Sequence<Designator>& designators() const final { return seq; }
   };

   export struct Using_directive : impl::Directive<ipr::Using_directive, Phases::Elaboration> {
      explicit Using_directive(const ipr::Scope&);
      const ipr::Scope& nominated_scope() const final { return scope; }
   private:
      const ipr::Scope& scope;
   };

   export struct Phased_evaluation : immotile_node<ipr::Phased_evaluation> {
      Phased_evaluation(const ipr::Expr& x, Phases f) : expr{x}, ph{f} { }
      Phases phases() const final { return ph; }
      const ipr::Expr& expression() const final { return expr; }
   private:
      const ipr::Expr& expr;
      Phases ph;
   };

   export struct Pragma : impl::Directive<ipr::Pragma, Phases::All> {
      obj_list<impl::Token> tokens;
      const ipr::Sequence<ipr::Token>& operand() const final { return tokens; }
   };

   export using Ctor_body = impl::Basic_binary<impl::Stmt<impl::Expr<ipr::Ctor_body>>>;
   export using Do = Controlled_stmt<ipr::Do>;
   export using Expr_stmt = impl::Unary_node<impl::Stmt<ipr::Expr_stmt>>;
   export using Goto = impl::Unary_node<impl::Stmt<ipr::Goto>>;
   export using If = Ternary<Stmt<Expr<ipr::If>>>;
   export using Labeled_stmt = impl::Binary_node<impl::Stmt<ipr::Labeled_stmt>>;
   export using Return = impl::Basic_unary<impl::Stmt<impl::Expr<ipr::Return>>>;
   export using Switch = Controlled_stmt<ipr::Switch>;
   export using While = Controlled_stmt<ipr::While>;

   export struct EH_parameter : unique_decl<ipr::EH_parameter> {
      EH_parameter(const ipr::Region&, const ipr::Name&, const ipr::Type&);
      const ipr::Name& name() const final { return id; }
      const ipr::Type& type() const final { return typing; }
      const ipr::Region& home_region() const final { return home; }
      const ipr::Region& lexical_region() const final { return home; }
   private:
      const ipr::Name& id;
      const ipr::Type& typing;
      const ipr::Region& home;
   };

   export struct handler_block : impl::Stmt<impl::Expr<ipr::Block>> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~handler_block() = default;
      impl::Region lexical_region;
      handler_block(const ipr::Region&);
      const ipr::Region& region() const final { return lexical_region; }
      const ipr::Sequence<ipr::Handler>& handlers() const final { return none; }

      impl::Scope* scope() { return &lexical_region.scope; }
      void add_stmt(const ipr::Expr& s)
      {
         lexical_region.expr_seq.push_back(&s);
      }
   private:
      impl::empty_sequence<ipr::Handler> none;
   };

   export struct eh_region : homogeneous_region<impl::EH_parameter, singleton_obj> {
      using base = homogeneous_region<impl::EH_parameter, singleton_obj>;
      using base::base;
      const ipr::EH_parameter& parameter() const { return scope.decls.seq.element(); }
    };

   export struct Handler : immotile_stmt<ipr::Handler> {
      Handler(const ipr::Region&, const ipr::Name&, const ipr::Type&);
      const ipr::Type& type() const final { return block.type(); }
      const ipr::EH_parameter& exception() const final { return eh.parameter(); }
      const ipr::Block& body() const final { return block; }
      impl::handler_block& body() { return block; }
   private:
      eh_region eh;
      impl::handler_block block;
   };

   export struct Block : impl::Stmt<Expr<ipr::Block>> {
      impl::Region lexical_region;
      explicit Block(const ipr::Region&);
      const ipr::Region& region() const final { return lexical_region; }
      const ipr::Sequence<ipr::Handler>& handlers() const final { return handler_seq; }

      impl::Scope* scope() { return &lexical_region.scope; }
      impl::Handler* new_handler(const ipr::Name&, const ipr::Type&);
      void add_stmt(const ipr::Expr& s)
      {
         lexical_region.expr_seq.push_back(&s);
      }
   private:
      impl::obj_list<impl::Handler> handler_seq;
   };

   export struct For : immotile_stmt<ipr::For> {
      util::ref<const ipr::Expr> init;
      util::ref<const ipr::Expr> cond;
      util::ref<const ipr::Expr> inc;
      util::ref<const ipr::Stmt> stmt;

      For();
      const ipr::Type& type() const final { return body().type(); }
      const ipr::Expr& initializer() const final { return init.get(); }
      const ipr::Expr& condition() const final { return cond.get(); }
      const ipr::Expr& increment() const final { return inc.get(); }
      const ipr::Stmt& body() const final { return stmt.get(); }
   };

   export struct For_in : immotile_stmt<ipr::For_in> {
      util::ref<const ipr::Var> var;
      util::ref<const ipr::Expr> seq;
      util::ref<const ipr::Stmt> stmt;

      For_in();
      const ipr::Type& type() const final { return body().type(); }
      const ipr::Var& variable() const final { return var.get(); }
      const ipr::Expr& sequence() const final { return seq.get(); }
      const ipr::Stmt& body() const final { return stmt.get(); }
   };

   export struct Break : immotile_stmt<ipr::Break> {
      util::ref<const ipr::Stmt> stmt;
      Break();
      const ipr::Type& type() const final;
      const ipr::Stmt& from() const final { return stmt.get(); }
   };

   export struct Continue : immotile_stmt<ipr::Continue> {
      util::ref<const ipr::Stmt> stmt;
      Continue();
      const ipr::Type& type() const final;
      const ipr::Stmt& iteration() const final { return stmt.get(); }
   };

   export struct Where : immotile_node<ipr::Where> {
      impl::Region region;
      util::ref<const ipr::Expr> result;

      explicit Where(const ipr::Region&);
      const ipr::Expr& first() const final { return result.get(); }
      const ipr::Scope& second() const final { return region.bindings(); }
   };

   export struct Static_assert : impl::Binary_node<ipr::Static_assert> {
      Static_assert(const ipr::Expr&, Optional<ipr::String>);
      const ipr::Type& type() const final;
   };
}

// ---------------------------
// -- Factories and Lexicon --
// ---------------------------

namespace ipr::impl {
   export struct type_factory {
      const ipr::Transfer& get_transfer_from_linkage(const ipr::Language_linkage&);
      const ipr::Transfer& get_transfer_from_convention(const ipr::Calling_convention&);
      const ipr::Transfer& get_transfer(const ipr::Language_linkage&, const ipr::Calling_convention&);

      const ipr::As_type& get_as_type(const ipr::Identifier&);
      const ipr::As_type& get_as_type(const ipr::Expr&);
      const ipr::As_type& get_as_type(const ipr::Expr&, const ipr::Transfer&);

      const ipr::Array& get_array(const ipr::Type&, const ipr::Expr&);
      const ipr::Qualified& get_qualified(ipr::Qualifiers, const ipr::Type&);
      const ipr::Decltype& get_decltype(const ipr::Expr&);
      const ipr::Tor& get_tor(const ipr::Product&, const ipr::Sum&);
      const ipr::Function& get_function(const ipr::Product&, const ipr::Type&);
      const ipr::Function& get_function(const ipr::Product&, const ipr::Type&, const ipr::Transfer&);
      const ipr::Function& get_function(const ipr::Product&, const ipr::Type&, const ipr::Expr&);
      const ipr::Function& get_function(const ipr::Product&, const ipr::Type&,
                                        const ipr::Expr&, const ipr::Transfer&);
      const ipr::Pointer& get_pointer(const ipr::Type&);
      const ipr::Product& get_product(const ipr::Sequence<ipr::Type>&);
      const ipr::Product& get_product(const Warehouse<ipr::Type>&);
      const ipr::Ptr_to_member& get_ptr_to_member(const ipr::Type&, const ipr::Type&);
      const ipr::Reference& get_reference(const ipr::Type&);
      const ipr::Rvalue_reference& get_rvalue_reference(const ipr::Type&);
      const ipr::Sum& get_sum(const ipr::Sequence<ipr::Type>&);
      const ipr::Sum& get_sum(const Warehouse<ipr::Type>&);
      const ipr::Forall& get_forall(const ipr::Product&, const ipr::Type&);
      const ipr::Auto& get_auto();

      impl::Enum* make_enum(const ipr::Region&, Enum::Kind);
      impl::Class* make_class(const ipr::Region&);
      impl::Union* make_union(const ipr::Region&);
      impl::Namespace* make_namespace(const ipr::Region&);
      impl::Closure* make_closure(const ipr::Region&);
   private:
      util::rb_tree::container<impl::Transfer_from_linkage> xfer_links;
      util::rb_tree::container<impl::Transfer_from_cc> xfer_ccs;
      util::rb_tree::container<impl::Transfer> xfers;

      util::rb_tree::container<impl::extended_type> extendeds;
      util::rb_tree::container<impl::Array> arrays;
      util::rb_tree::container<impl::As_type> type_refs;
      util::rb_tree::container<impl::As_type_with_transfer> type_xfers;
      util::rb_tree::container<impl::Tor> tors;
      util::rb_tree::container<impl::Function> functions;
      util::rb_tree::container<impl::Function_with_transfer> fun_xfers;
      util::rb_tree::container<impl::Pointer> pointers;
      util::rb_tree::container<impl::Product> products;
      util::rb_tree::container<impl::Ptr_to_member> member_ptrs;
      util::rb_tree::container<impl::Qualified> qualifieds;
      util::rb_tree::container<impl::Reference> references;
      util::rb_tree::container<impl::Rvalue_reference> refrefs;
      util::rb_tree::container<impl::Sum> sums;
      util::rb_tree::container<impl::Forall> foralls;
      util::rb_tree::container<ref_sequence<ipr::Type>> type_seqs;
      stable_farm<impl::Decltype> decltypes;
      stable_farm<impl::Enum> enums;
      stable_farm<impl::Class> classes;
      stable_farm<impl::Union> unions;
      stable_farm<impl::Namespace> namespaces;
      stable_farm<impl::Closure> closures;
      stable_farm<impl::Auto> autos;
   };

   export struct name_factory {
      const ipr::String& get_string(util::word_view);
      const ipr::Identifier& get_identifier(const ipr::String&);
      const ipr::Identifier& get_identifier(util::word_view);
      const ipr::Suffix& get_suffix(const ipr::Identifier&);
      const ipr::Operator& get_operator(const ipr::String&);
      const ipr::Operator& get_operator(util::word_view);
      const ipr::Conversion& get_conversion(const ipr::Type&);
      const ipr::Ctor_name& get_ctor_name(const ipr::Type&);
      const ipr::Dtor_name& get_dtor_name(const ipr::Type&);
      const ipr::Guide_name& get_guide_name(const ipr::Template&);
      const ipr::Logogram& get_logogram(const ipr::String&);
   private:
      util::string_pool strings;
      util::rb_tree::container<impl::Logogram> logos;
      util::rb_tree::container<impl::Identifier> ids;
      util::rb_tree::container<impl::Suffix> suffixes;
      util::rb_tree::container<impl::Conversion> convs;
      util::rb_tree::container<impl::Ctor_name> ctors;
      util::rb_tree::container<impl::Dtor_name> dtors;
      util::rb_tree::container<impl::Operator> ops;
      util::rb_tree::container<impl::Guide_name> guide_ids;
   };

   export struct expr_factory : name_factory {
      const ipr::Language_linkage& get_linkage(util::word_view);
      const ipr::Language_linkage& get_linkage(const ipr::String&);
      const ipr::Calling_convention& get_calling_convention(util::word_view);
      const ipr::Symbol& get_symbol(const ipr::Name&, const ipr::Type&);
      const ipr::Symbol& get_label(const ipr::Identifier&);
      const ipr::Symbol& get_this(const ipr::Type&);

      Annotation* make_annotation(const ipr::String&, const ipr::Literal&);
      Phantom* make_phantom();
      const ipr::Phantom* make_phantom(const ipr::Type&);
      Eclipsis* make_eclipsis(const ipr::Type&);
      Literal* make_literal(const ipr::Type&, const ipr::String&);
      Literal* make_literal(const ipr::Type&, util::word_view);

      Address* make_address(const ipr::Expr&, Optional<ipr::Type> = {});
      Array_delete* make_array_delete(const ipr::Expr&);
      Complement* make_complement(const ipr::Expr&, Optional<ipr::Type> = {});
      Delete* make_delete(const ipr::Expr&);
      Demotion* make_demotion(const ipr::Expr&, const ipr::Type&);
      Deref* make_deref(const ipr::Expr&, Optional<ipr::Type> = {});
      Expr_list* make_expr_list();
      Alignof* make_alignof(const ipr::Expr&, Optional<ipr::Type> = { });
      Sizeof* make_sizeof(const ipr::Expr&, Optional<ipr::Type> = { });
      Args_cardinality* make_args_cardinality(const ipr::Expr&, Optional<ipr::Type> = { });
      Typeid* make_typeid(const ipr::Expr&, Optional<ipr::Type> = { });
      Restriction* make_restriction(const ipr::Expr&);
      impl::Id_expr* make_id_expr(const ipr::Name&, Optional<ipr::Type> = {});
      Id_expr* make_id_expr(const ipr::Decl&);
      Label* make_label(const ipr::Identifier&, Optional<ipr::Type> = {});
      Materialization* make_materialization(const ipr::Expr&, const ipr::Type&);
      Not* make_not(const ipr::Expr&, Optional<ipr::Type> = {});
      Enclosure* make_enclosure(ipr::Delimiter, const ipr::Expr&, Optional<ipr::Type> = { });
      Post_increment* make_post_increment(const ipr::Expr&, Optional<ipr::Type> = {});
      Post_decrement* make_post_decrement(const ipr::Expr&, Optional<ipr::Type> = {});
      Pre_increment* make_pre_increment(const ipr::Expr&, Optional<ipr::Type> = {});
      Pre_decrement* make_pre_decrement(const ipr::Expr&, Optional<ipr::Type> = {});
      Promotion* make_promotion(const ipr::Expr&, const ipr::Type&);
      Read* make_read(const ipr::Expr&, const ipr::Type&);
      Throw* make_throw(const ipr::Expr&, Optional<ipr::Type> = {});
      Unary_minus* make_unary_minus(const ipr::Expr&, Optional<ipr::Type> = {});
      Unary_plus* make_unary_plus(const ipr::Expr&, Optional<ipr::Type> = {});
      Expansion* make_expansion(const ipr::Expr&, Optional<ipr::Type> = {});
      Construction* make_construction(const ipr::Type&, const ipr::Enclosure&);
      Noexcept* make_noexcept(const ipr::Expr&, Optional<ipr::Type> = { });

      Rewrite* make_rewrite(const ipr::Expr&, const ipr::Expr&);
      And* make_and(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Array_ref* make_array_ref(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Arrow* make_arrow(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Arrow_star* make_arrow_star(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Assign* make_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitand* make_bitand(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitand_assign* make_bitand_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitor* make_bitor(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitor_assign* make_bitor_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitxor* make_bitxor(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Bitxor_assign* make_bitxor_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Cast* make_cast(const ipr::Type&, const ipr::Expr&);
      Call* make_call(const ipr::Expr&, const ipr::Expr_list&, Optional<ipr::Type> = {});
      Coercion* make_coercion(const ipr::Expr&, const ipr::Type&, const ipr::Type&);
      Comma* make_comma(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Const_cast* make_const_cast(const ipr::Type&, const ipr::Expr&);
      Div* make_div(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Div_assign* make_div_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Dot* make_dot(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Dot_star* make_dot_star(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Dynamic_cast* make_dynamic_cast(const ipr::Type&, const ipr::Expr&);
      Equal* make_equal(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Greater* make_greater(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Greater_equal* make_greater_equal(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Less* make_less(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Less_equal* make_less_equal(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Lshift* make_lshift(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Lshift_assign* make_lshift_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Member_init* make_member_init(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Minus* make_minus(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Minus_assign* make_minus_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Modulo* make_modulo(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Modulo_assign* make_modulo_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Mul* make_mul(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Mul_assign* make_mul_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Narrow* make_narrow(const ipr::Expr&, const ipr::Type&, const ipr::Type&);
      Not_equal* make_not_equal(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Or* make_or(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Plus* make_plus(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Plus_assign* make_plus_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Pretend* make_pretend(const ipr::Expr&, const ipr::Type&, const ipr::Type&);
      Qualification* make_qualification(const ipr::Expr&, ipr::Qualifiers, const ipr::Type&);
      Reinterpret_cast* make_reinterpret_cast(const ipr::Type&, const ipr::Expr&);
      Scope_ref* make_scope_ref(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Rshift* make_rshift(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Rshift_assign* make_rshift_assign(const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Template_id* make_template_id(const ipr::Expr&, const ipr::Expr_list&);
      Static_cast* make_static_cast(const ipr::Type&, const ipr::Expr&);
      Widen* make_widen(const ipr::Expr&, const ipr::Type&, const ipr::Type&);
      Binary_fold* make_binary_fold(Category_code, const ipr::Expr&, const ipr::Expr&, Optional<ipr::Type> = {});
      Where* make_where(const ipr::Region&);
      Where_no_decl* make_where(const ipr::Expr&, const ipr::Expr&);
      impl::Instantiation* make_instantiation(const ipr::Expr&, const ipr::Substitution&);
      New* make_new(Optional<ipr::Expr_list>, const ipr::Construction&, Optional<ipr::Type> = {});
      Conditional* make_conditional(const ipr::Expr&, const ipr::Expr&,
                                    const ipr::Expr&, Optional<ipr::Type> = {});
      Mapping* make_mapping(const ipr::Region&, Mapping_level);
      Lambda* make_lambda(const ipr::Region&, Mapping_level);
      Requires* make_requires(const ipr::Region&, Mapping_level);

      Elementary_substitution* make_elementary_substitution(const ipr::Parameter&, const ipr::Expr&);
      General_substitution* make_general_substitution();

   protected:
      impl::Asm* make_asm_expr(const ipr::String&);
      impl::Static_assert* make_static_assert_expr(const ipr::Expr&, Optional<ipr::String> = { });

   private:
      util::rb_tree::container<ipr::Language_linkage> linkages;
      util::rb_tree::container<ipr::Calling_convention> conventions;

      util::rb_tree::container<impl::Literal> lits;
      util::rb_tree::container<impl::Template_id> template_ids;

      stable_farm<impl::Phantom> phantoms;
      stable_farm<impl::Eclipsis> eclipses;

      util::rb_tree::container<impl::Symbol> symbols;
      stable_farm<impl::Alignof> alignofs;
      stable_farm<impl::Sizeof> sizeofs;
      stable_farm<impl::Typeid> xtypeids;
      stable_farm<impl::Address> addresses;
      stable_farm<impl::Annotation> annotations;
      stable_farm<impl::Array_delete> array_deletes;
      stable_farm<impl::Asm> asms;
      stable_farm<impl::Complement> complements;
      stable_farm<impl::Delete> deletes;
      stable_farm<impl::Demotion> demotions;
      stable_farm<impl::Deref> derefs;
      stable_farm<impl::Expr_list> xlists;
      stable_farm<impl::Id_expr> id_exprs;
      stable_farm<impl::Label> labels;
      stable_farm<impl::Materialization> materializations;
      stable_farm<impl::Not> nots;
      stable_farm<impl::Enclosure> enclosures;
      stable_farm<impl::Pre_increment> pre_increments;
      stable_farm<impl::Pre_decrement> pre_decrements;
      stable_farm<impl::Post_increment> post_increments;
      stable_farm<impl::Post_decrement> post_decrements;
      stable_farm<impl::Promotion> promotions;
      stable_farm<impl::Read> reads;
      stable_farm<impl::Throw> throws;
      stable_farm<impl::Unary_minus> unary_minuses;
      stable_farm<impl::Unary_plus> unary_pluses;
      stable_farm<impl::Expansion> expansions;
      stable_farm<impl::Construction> constructions;
      stable_farm<impl::Noexcept> noexcepts;
      stable_farm<impl::Args_cardinality> cardinalities;
      stable_farm<impl::Restriction> restrictions;

      stable_farm<impl::Rewrite> rewrites;
      stable_farm<impl::Scope_ref> scope_refs;
      stable_farm<impl::And> ands;
      stable_farm<impl::Array_ref> array_refs;
      stable_farm<impl::Arrow> arrows;
      stable_farm<impl::Arrow_star> arrow_stars;
      stable_farm<impl::Assign> assigns;
      stable_farm<impl::Bitand> bitands;
      stable_farm<impl::Bitand_assign> bitand_assigns;
      stable_farm<impl::Bitor> bitors;
      stable_farm<impl::Bitor_assign> bitor_assigns;
      stable_farm<impl::Bitxor> bitxors;
      stable_farm<impl::Bitxor_assign> bitxor_assigns;
      stable_farm<impl::Cast> casts;
      stable_farm<impl::Call> calls;
      stable_farm<impl::Comma> commas;
      stable_farm<impl::Const_cast> ccasts;
      stable_farm<impl::Div> divs;
      stable_farm<impl::Div_assign> div_assigns;
      stable_farm<impl::Dot> dots;
      stable_farm<impl::Dot_star> dot_stars;
      stable_farm<impl::Dynamic_cast> dcasts;
      stable_farm<impl::Equal> equals;
      stable_farm<impl::Greater> greaters;
      stable_farm<impl::Greater_equal> greater_equals;
      stable_farm<impl::Less> lesses;
      stable_farm<impl::Less_equal> less_equals;
      stable_farm<impl::Lshift> lshifts;
      stable_farm<impl::Lshift_assign> lshift_assigns;
      stable_farm<impl::Member_init> member_inits;
      stable_farm<impl::Minus> minuses;
      stable_farm<impl::Minus_assign> minus_assigns;
      stable_farm<impl::Modulo> modulos;
      stable_farm<impl::Modulo_assign> modulo_assigns;
      stable_farm<impl::Mul> muls;
      stable_farm<impl::Mul_assign> mul_assigns;
      stable_farm<impl::Narrow> narrows;
      stable_farm<impl::Not_equal> not_equals;
      stable_farm<impl::Or> ors;
      stable_farm<impl::Plus> pluses;
      stable_farm<impl::Plus_assign> plus_assigns;
      stable_farm<impl::Pretend> pretends;
      stable_farm<impl::Qualification> qualifications;
      stable_farm<impl::Reinterpret_cast> rcasts;
      stable_farm<impl::Rshift> rshifts;
      stable_farm<impl::Rshift_assign> rshift_assigns;
      stable_farm<impl::Static_cast> scasts;
      stable_farm<impl::Widen> widens;
      stable_farm<impl::Binary_fold> folds;
      stable_farm<impl::Where_no_decl> where_nodecls;
      stable_farm<impl::Where> wheres;
      stable_farm<impl::Static_assert> asserts;
      stable_farm<impl::Instantiation> insts;

      stable_farm<impl::New> news;
      stable_farm<impl::Coercion> coercions;
      stable_farm<impl::Conditional> conds;
      stable_farm<impl::Mapping> mappings;
      stable_farm<impl::Lambda> lambdas;
      stable_farm<impl::Requires> reqs;

      stable_farm<impl::Elementary_substitution> elem_substs;
      stable_farm<impl::General_substitution> gen_substs;
   };

   export struct dir_factory {
      impl::Specifiers_spread* make_specifiers_spread();
      impl::Structured_binding* make_structured_binding();
      impl::single_using_declaration* make_using_declaration(const ipr::Scope_ref&,
                                                               ipr::Using_declaration::Designator::Mode);
      impl::Using_declaration* make_using_declaration();
      impl::Using_directive* make_using_directive(const ipr::Scope&, const ipr::Type&);
      impl::Phased_evaluation* make_phased_evaluation(const ipr::Expr&, Phases);
      impl::Pragma* make_pragma();
   private:
      stable_farm<impl::Specifiers_spread> spreads;
      stable_farm<impl::Structured_binding> bindings;
      stable_farm<impl::single_using_declaration> singles;
      stable_farm<impl::Using_declaration> usings;
      stable_farm<impl::Using_directive> dirs;
      stable_farm<impl::Phased_evaluation> phaseds;
      stable_farm<impl::Pragma> pragmas;
   };

   export struct stmt_factory : expr_factory, dir_factory {
      impl::Break* make_break();
      impl::Continue* make_continue();
      impl::Block* make_block(const ipr::Region&, Optional<ipr::Type> = { });
      impl::Ctor_body* make_ctor_body(const ipr::Expr_list&, const ipr::Block&);
      impl::Expr_stmt* make_expr_stmt(const ipr::Expr&);
      impl::Goto* make_goto(const ipr::Expr&);
      impl::Return* make_return(const ipr::Expr&);
      impl::Do* make_do();
      impl::If* make_if(const ipr::Expr&, const ipr::Expr&);
      impl::If* make_if(const ipr::Expr&, const ipr::Expr&, const ipr::Expr&);
      impl::Switch* make_switch();
      impl::Labeled_stmt* make_labeled_stmt(const ipr::Expr&, const ipr::Expr&);
      impl::While* make_while();
      impl::For* make_for();
      impl::For_in* make_for_in();

   protected:
      stable_farm<impl::Break> breaks;
      stable_farm<impl::Continue> continues;
      stable_farm<impl::Block> blocks;
      stable_farm<impl::Expr_stmt> expr_stmts;
      stable_farm<impl::Goto> gotos;
      stable_farm<impl::Return> returns;
      stable_farm<impl::Ctor_body> ctor_bodies;
      stable_farm<impl::Do> dos;
      stable_farm<impl::If> ifs;
      stable_farm<impl::Handler> handlers;
      stable_farm<impl::Labeled_stmt> labeled_stmts;
      stable_farm<impl::Switch> switches;
      stable_farm<impl::While> whiles;
      stable_farm<impl::For> fors;
      stable_farm<impl::For_in> for_ins;
   };

                              // -- impl::Lexicon --
   export struct Lexicon : ipr::Lexicon, type_factory, stmt_factory {
      Lexicon();
      ~Lexicon();

      const ipr::Language_linkage& cxx_linkage() const final;
      const ipr::Language_linkage& c_linkage() const final;

      ipr::Specifiers export_specifier() const final;
      ipr::Specifiers static_specifier() const final;
      ipr::Specifiers extern_specifier() const final;
      ipr::Specifiers mutable_specifier() const final;
      ipr::Specifiers constinit_specifier() const final;
      ipr::Specifiers thread_local_specifier() const final;
      ipr::Specifiers register_specifier() const final;
      ipr::Specifiers inline_specifier() const final; 
      ipr::Specifiers constexpr_specifier() const final;
      ipr::Specifiers consteval_specifier() const final;
      ipr::Specifiers virtual_specifier() const final;
      ipr::Specifiers abstract_specifier() const final;
      ipr::Specifiers explicit_specifier() const final;
      ipr::Specifiers friend_specifier() const final;
      ipr::Specifiers typedef_specifier() const final;
      ipr::Specifiers public_specifier() const final;
      ipr::Specifiers protected_specifier() const final;
      ipr::Specifiers private_specifier() const final;
      ipr::Specifiers specifiers(ipr::Basic_specifier) const final;
      std::vector<ipr::Basic_specifier> decompose(ipr::Specifiers) const final;

      ipr::Qualifiers const_qualifier() const final;
      ipr::Qualifiers volatile_qualifier() const final;
      ipr::Qualifiers restrict_qualifier() const final;
      ipr::Qualifiers qualifiers(ipr::Basic_qualifier) const final;
      std::vector<ipr::Basic_qualifier> decompose(ipr::Qualifiers) const final;

      const ipr::Template_id& get_template_id(const ipr::Expr&,
                                                const ipr::Expr_list&);

      const ipr::Literal& get_literal(const ipr::Type&, util::word_view);
      const ipr::Literal& get_literal(const ipr::Type&, const ipr::String&);

      const ipr::Type& void_type() const final;
      const ipr::Type& bool_type() const final;
      const ipr::Type& char_type() const final;
      const ipr::Type& schar_type() const final;
      const ipr::Type& uchar_type() const final;
      const ipr::Type& wchar_t_type() const final;
      const ipr::Type& char8_t_type() const final;
      const ipr::Type& char16_t_type() const final;
      const ipr::Type& char32_t_type() const final;
      const ipr::Type& short_type() const final;
      const ipr::Type& ushort_type() const final;
      const ipr::Type& int_type() const final;
      const ipr::Type& uint_type() const final;
      const ipr::Type& long_type() const final;
      const ipr::Type& ulong_type() const final;
      const ipr::Type& long_long_type() const final;
      const ipr::Type& ulong_long_type() const final;
      const ipr::Type& float_type() const final;
      const ipr::Type& double_type() const final;
      const ipr::Type& long_double_type() const final;
      const ipr::Type& ellipsis_type() const final;
      const ipr::Type& typename_type() const final;
      const ipr::Type& class_type() const final;
      const ipr::Type& union_type() const final;
      const ipr::Type& enum_type() const final;
      const ipr::Type& namespace_type() const final;

      const ipr::Symbol& false_value() const final;
      const ipr::Symbol& true_value() const final;
      const ipr::Symbol& nullptr_value() const final;
      const ipr::Symbol& default_value() const final;
      const ipr::Symbol& delete_value() const final;

      impl::Phased_evaluation* make_asm(const ipr::String&);
      impl::Phased_evaluation* make_static_assert(const ipr::Expr&, Optional<ipr::String>);

      impl::Mapping* make_mapping(const ipr::Region&, Mapping_level = { });

      const impl::Token* make_token(const ipr::String&,
                                    const Source_location&,
                                    TokenValue, TokenCategory);
   private:
      stable_farm<impl::Token> tokens;
      util::rb_tree::container<ref_sequence<ipr::Expr>> expr_seqs;
   };
}

// -----------------------------------
// -- Translation units and modules --
// -----------------------------------

namespace ipr::impl {
   export template<typename T>
   struct unit_base : T {
      using Interface = T;
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~unit_base() = default;
      unit_base(impl::Lexicon& l)
            : context{ l },
               global_ns{nullptr}
      {
         global_ns.id = &context.get_identifier(u8"");
      }

      void accept(Translation_unit::Visitor& v) const override {
         v.visit(*this);
      }

      const ipr::Namespace& global_namespace() const final {
         return global_ns;
      }

      const ipr::Sequence<ipr::Module>& imported_modules() const final {
         return modules_imported;
      }

      Region* global_region() { return &global_ns.body; }
      Scope* global_scope() { return &global_ns.body.scope; }
      ref_sequence<ipr::Module>* imports() { return &modules_imported; }

   private:
      impl::Lexicon& context;
      impl::Namespace global_ns;
      ref_sequence<ipr::Module> modules_imported;
   };

   export template<typename T>
   struct basic_unit : unit_base<T> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~basic_unit() = default;
      const ipr::Module& parent;
      impl::ref_sequence<ipr::Decl> owned_decls;

      basic_unit(impl::Lexicon& l, const ipr::Module& m)
            : unit_base<T>{ l }, parent{ m }
      { }
      const ipr::Module& parent_module() const final { return parent; }
      const ipr::Sequence<ipr::Decl>& purview() const final {
         return owned_decls;
      }
   };

   export using Translation_unit = unit_base<ipr::Translation_unit>;
   export using Module_unit = basic_unit<ipr::Module_unit>;

   export struct Interface_unit : basic_unit<ipr::Interface_unit> {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Interface_unit() = default;
      impl::ref_sequence<ipr::Module> modules_exported;
      impl::ref_sequence<ipr::Decl> decls_exported;

      Interface_unit(impl::Lexicon&, const ipr::Module&);
      const ipr::Sequence<ipr::Module>& exported_modules() const final;
      const ipr::Sequence<ipr::Decl>& exported_declarations() const final;
   };

   export struct Module : ipr::Module {
      // GCC BUG workaround: cross-module protected destructor not seen as accessible.
      ~Module() = default;
      using ImplUnits = impl::obj_list<impl::Module_unit>;
      impl::Lexicon& lexicon;
      impl::Module_name stems;
      impl::Interface_unit iface;
      ImplUnits units;

      Module(impl::Lexicon&);
      const ipr::Module_name& name() const final;
      const ipr::Interface_unit& interface_unit() const final;
      const ipr::Sequence<ipr::Module_unit>& implementation_units() const final;
      impl::Module_unit* make_unit();
   };
}
