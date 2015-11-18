//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis <gdr@cs.tamu.edu>
// 

#ifndef IPR_UTILITY_INCLUDED
#define IPR_UTILITY_INCLUDED

#include <utility>
#include <memory>
#include <new>
#include <stdexcept>
#include <algorithm>
#include <iosfwd>

namespace ipr {
   namespace util {

      // -- Check for nonnull pointer.
      template<typename T>
      inline T* check(T* ptr)
      {
         if (ptr == 0)
            throw std::logic_error("attempt to dereference a null pointer");
         return ptr;
      }

      // >>>> Yuriy Solodkyy: 2008/12/12 
      // This is a generic counter-measure to the above check function for those
      // cases when there is no appropriate has_... member to check for existence
      // of a member. This function is extremely slow and inefficient and should
      // only be used for debated cases of whether has_... member function should
      // be provided. The function is provided for the convenience of grepping
      // for all such debated cases.
      // This function is currently used in place of Mapping::has_result until 
      // the issue whether it should be present or not inside Mapping is resolved.
      template <class T, class S, class R>
      inline bool node_has_member(const T& t, R (S::*method)() const)
      {
          bool has = true;
          try { (t.*method)(); } catch (...) { has = false; }
          return has;
      }
      // <<<< Yuriy Solodkyy: 2008/12/12 

      // --------------------
      // -- Red-back trees --
      // --------------------

      // The implementation found here is based on ideas in
      // T. H. Cormen, C. E. Leiserson, R. L. Rivest and C. Strein:
      //     "Introduction to Algorithms", 2nd edition.

      // One reason why we (re-)implement our own "set" data structures
      // instead of using the standard ones is because standard sets
      // do not allow for in-place modification.  That puts an
      // unreasonable burden on how we can write the codes for IPR.
      // Another reason is that, while the standard set is really a
      // a red-lack tree in disguise, there is no way one can have
      // access to that structure.  Furthermore, we use red-black
      // trees in of both "intrusive" and "non-intrusive" forms.

      namespace rb_tree {
         // The type of the links used to chain together data in
         // a red-black tree.
         template<class Node>
         struct link {
            enum Color { Black, Red };
            enum Dir { Left, Right, Parent };

            link() : arm(), color(Red)
            {
                // >>>> Yuriy Solodkyy: 2006/06/04
                // the above arm() does not initialize arm with 0 in MSVC
                arm[Left] = 0;
                arm[Right] = 0;
                arm[Parent] = 0;
                // <<<< Yuriy Solodkyy: 2006/06/04
            }

            Node*& parent() { return arm[Parent]; }
            Node*& left() { return arm[Left]; }
            Node*& right() { return arm[Right]; }

            Node* arm[3];
            Color color;
         };

         template<class Node>
         struct core {
            core() : root(0), count(0) { }

            int size() const { return count; }

         protected:
            Node* root;
            int count;

            // Do a left rotation about X.  X->left() is assumed nonnull,
            // which after the manoeuvre becomes X's parent.
            void rotate_left(Node*);

            // Same as rotate_left, except that the rotation does right.
            void rotate_right(Node*);

            // After raw insertion, the tree is unbalanced again; this
            // function re-balance the tree, fixing up properties destroyed.
            void fixup_insert(Node*);
         };

         template<class Node>
         void
         core<Node>::rotate_left(Node* x)
         {
            Node* y = x->right();
            // Make y's left a subtree of x's right subtree.
            x->right() = y->left();
            if (y->left() != 0)
               y->left()->parent() = x;

            // Update y's parent and its left or right arms.
            y->parent() = x->parent();
            if (x->parent() == 0)
               // x was the root of the tree; make y the new root.
               this->root = y;
            else if (x->parent()->left() == x)
               x->parent()->left() = y;
            else
               x->parent()->right() = y;

            // Now, x must go on y's left.
            y->left() = x;
            x->parent() = y;
         }

         template<class Node>
         void
         core<Node>::rotate_right(Node* x)
         {
            Node* y = x->left();

            x->left() = y->right();
            if (y->right() != 0)
               y->right()->parent() = x;

            y->parent() = x->parent();
            if (x->parent() == 0)
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
            while (z != root && z->parent()->color == Node::Red) {
               if (z->parent() == z->parent()->parent()->left()) {
                  Node* y = z->parent()->parent()->right();
                  if (y != 0 && y->color == Node::Red) {
                     z->parent()->color = Node::Black;
                     y->color = Node::Black;
                     z->parent()->parent()->color = Node::Red;
                     z = z->parent()->parent();
                  } else {
                     if (z->parent()->right() == z) {
                        z = z->parent();
                        rotate_left(z);
                     }
                     z->parent()->color = Node::Black;
                     z->parent()->parent()->color = Node::Red;
                     rotate_right(z->parent()->parent());
                  }
               } else {
                  Node* y = z->parent()->parent()->left();
                  if (y != 0 && y->color == Node::Red) {
                     z->parent()->color = Node::Black;
                     y->color = Node::Black;
                     z->parent()->parent()->color = Node::Red;
                     z = z->parent()->parent();
                  } else {
                     if (z->parent()->left() == z) {
                        z = z->parent();
                        rotate_right(z);
                     }
                     z->parent()->color = Node::Black;
                     z->parent()->parent()->color = Node::Red;
                     rotate_left(z->parent()->parent());
                  }
               }

            }

            root->color = Node::Black;
         }


         template<class _Node>
         struct chain : core<_Node> {
            template<class Comp>
            _Node* insert(_Node*, Comp);

            template<typename Key, class Comp>
            _Node* find(const Key&, Comp) const;
         };

         template<class _Node>
         template<typename Key, class Comp>
         _Node*
         chain<_Node>::find(const Key& key, Comp comp) const
         {
            bool found = false;
            _Node* result = this->root;
            while (result != 0 && !found) {
               int ordering = comp(key, *result) ;
               if (ordering < 0)
                  result = result->left();
               else if (ordering > 0)
                  result = result->right();
               else
                  found = true;
            }

            return result;
         }

         template<class _Node>
         template<class Comp>
         _Node*
         chain<_Node>::insert(_Node* z, Comp comp)
         {
            _Node** slot = &this->root;
            _Node* up = 0;

            bool found = false;
            while (!found && *slot != 0) {
               int ordering = comp(*z, **slot);
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

            if (this->root == 0) {
               // This is the first time we're inserting into the tree.
               this->root = z;
               z->color = _Node::Black;
            }
            else if (*slot == 0) {
               // key is not present, do what we're asked to do.
               *slot = z;
               z->parent() = up;
               z->color = _Node::Red;
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
         struct container : core<node<T>> {
            template<typename Key, class Comp>
            T* find(const Key&, Comp) const;

            // We want to insert a node constructed out of a Key, using
            // an admissible comparator LESS.  Returns a pointer to the
            // newly created node, if successfully inserted, or the old
            // one if the Key is already present.
            template<class Key, class Comp>
            T* insert(const Key&, Comp);

         private:
            node<T>* allocate() {
               node<T> * n = static_cast<node<T>*>
                  (operator new (sizeof(node<T>)));

               n->arm[node<T>::Left] = 0;
               n->arm[node<T>::Right] = 0;
               n->arm[node<T>::Parent] = 0;

               return n;
            }

            void deallocate(node<T>* n) {
               operator delete(n);
            }

            template<class U>
            node<T>* make_node(const U& u) {
               node<T>* n = allocate();
               new (&n->data) T(u);
               return n;
            }

            void destroy_node(node<T>* n) {
               if (n != 0) {
                  n->~node<T>();
                  this->deallocate(n);
               }
            }
         };

         template<typename T>
         template<typename Key, class Comp>
         T*
         container<T>::find(const Key& key, Comp comp) const
         {
            for (node<T>* x = this->root; x != 0; ) {
               int ordering = comp(key, x->data);
               if (ordering < 0)
                  x = x->left();
               else if (ordering > 0)
                  x = x->right();
               else
                  return &x->data;
            }

            return 0;
         }

         template<typename T>
         template<typename Key, class Comp>
         T*
         container<T>::insert(const Key& key, Comp comp)
         {
            if (this->root == 0) {
               // This is the first time we're inserting into the tree.
               this->root = make_node(key);
               this->root->color = node<T>::Black;
               ++this->count;
               return &this->root->data;
            }

            node<T>** slot = &this->root;
            node<T>* parent = 0;
            node<T>* where = 0;
            bool found = false;

            for (where = this->root; where != 0 && !found; where = *slot) {
               int ordering = comp(key, where->data);
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

            if (where == 0) {
               // key is not present, do what we're asked to do.
               where = *slot = make_node(key);
               where->parent() = parent;
               where->color = node<T>::Red;
               ++this->count;
               this->fixup_insert(where);
            }

            return &where->data;
         }
      }


      template<class T>
      struct slist_node {
         slist_node* next;
         T data;
      };

      template<typename T>
      struct slist_iterator : std::iterator<std::forward_iterator_tag, T> {
         explicit slist_iterator(slist_node<T>* n = 0) : node(n) { }

         slist_iterator& operator++()
         {
            node = node->next;
            return *this;
         }

         slist_iterator operator++(int)
         {
            slist_node<T>* tmp = node;
            node = node->next;
            return slist_iterator(tmp);
         }

         T& operator*() const
         {
            return node->data;
         }

         T* operator->() const
         {
            return &node->data;
         }

         bool operator==(slist_iterator that) const
         {
            return node == that.node;
         }

         bool operator!=(slist_iterator that) const
         {
            return node != that.node;
         }

      private:
         slist_node<T>* node;
      };
      template<typename T>
      struct const_slist_iterator : std::iterator<std::forward_iterator_tag,
                                                  const T> {
         explicit const_slist_iterator(slist_node<T>* n = 0) : node(n) { }

         const_slist_iterator& operator++()
         {
            node = node->next;
            return *this;
         }

         const_slist_iterator operator++(int)
         {
            slist_node<T>* tmp = node;
            node = node->next;
            return const_slist_iterator(tmp);
         }

         const T& operator*() const
         {
            return node->data;
         }

         const T* operator->() const
         {
            return &node->data;
         }

         bool operator==(const_slist_iterator that) const
         {
            return node == that.node;
         }

         bool operator!=(const_slist_iterator that) const
         {
            return node != that.node;
         }

      private:
         slist_node<T>* node;
      };

      template<typename T>
      struct slist {
         slist() : first(0), last(0), count(0) { }
         ~slist();

         using iterator = slist_iterator<T>;
         using const_iterator = const_slist_iterator<T>;

         iterator begin()
         {
            return iterator(first);
         }

         const_iterator begin() const
         {
            return const_iterator(first);
         }

         iterator end()
         {
            return iterator(last);
         }

         const_iterator end() const
         {
            return const_iterator(last);
         }


         T* push_back();

         template<typename U>
         T* push_back(const U&);

         template<typename U, typename V>
         T* push_back(const U&, const V&);

         template<typename U, typename V, typename W>
         T* push_back(const U&, const V&, const W&);

         int size() const { return count; }

      private:
         using node = slist_node<T>;
         node* first;
         node* last;
         int count;

         slist_node<T>* allocate() {
            slist_node<T>* n = static_cast<slist_node<T>*>
               (operator new (sizeof (slist_node<T>)));
            n->next = 0;
            return n;
         }

         void deallocate(slist_node<T>* n) {
            operator delete(n);
         }
      };

      template<typename T>
      T*
      slist<T>::push_back()
      {
         node* n = allocate();

         if (first == 0)
            first = n;
         else
            last->next = n;
         last = n;
         new (&n->data) T();

         ++count;
         return &n->data;
      }

      template<typename T>
      template<typename U>
      T*
      slist<T>::push_back(const U& u)
      {
         node* n = allocate();

         if (first == 0)
            first = n;
         else
            last->next = n;
         last = n;
         new (&n->data) T(u);

         ++count;
         return &n->data;
      }

      template<typename T>
      template<typename U, typename V>
      T*
      slist<T>::push_back(const U& u, const V& v)
      {
         node* n = allocate();
         if (first == 0)
            first = n;
         else
            last->next = n;
         last = n;
         new (&n->data) T(u, v);

         ++count;
         return &n->data;
      }

      template<typename T>
      template<typename U, typename V, typename W>
      T*
      slist<T>::push_back(const U& u, const V& v, const W& w)
      {
         node* n = allocate();
         if (first == 0)
            first = n;
         else
            last->next = n;
         last = n;
         new (&n->data) T(u, v, w);

         ++count;
         return &n->data;
      }

      template<typename T>
      slist<T>::~slist()
      {
         while (node* n = first) {
            first = first->next;
            n->~slist_node<T>();
            this->deallocate(n);
         }
      }


      // -- helper for implementing permanent string objects.  They uniquely
      // -- represent their contents throughout their lifetime.  Ideally,
      // -- they are allocated from a pool.
      struct string {
         struct arena;

         using size_type = int; // integer type that string length

         // number of characters directly contained in this header
         // of the string.  Takent to be the number of bytes in size_type.
         enum {
            padding_count = sizeof (size_type)
         };

         int size() const { return length; }
         char operator[](int) const;

         const char* begin() const { return data; }
         const char* end() const { return begin() + length; }

         int length;
         char data[padding_count];
      };

      struct string::arena {
         arena();
         ~arena();

         const string* make_string(const char*, int);

      private:
         util::string* allocate(int);
         int remaining_header_count() const
         {
            return (int)(next_header - &mem->storage[0]);
         }

         struct pool;

         enum { headersz = sizeof (util::string) };
         enum { bufsz = headersz << (20 - sizeof (pool*)) };

         struct pool {
            pool* previous;
            util::string storage[bufsz];
         };

         enum { poolsz = sizeof (pool) };

         pool* mem;
         string* next_header;
      };


      struct lexicographical_compare {
         template<typename In1, typename In2, class Compare>
         int operator()(In1 first1, In1 last1, In2 first2, In2 last2,
                        Compare compare) const
         {
            for (; first1 != last1 && first2 != last2; ++first1, ++first2)
               if (int cmp = compare(*first1, *first2))
                  return cmp;

            return first1 == last1 ? (first2 == last2 ? 0 : -1) : 1;
         }
      };


   }
}

#endif // IPR_UTILITY_INCLUDED
