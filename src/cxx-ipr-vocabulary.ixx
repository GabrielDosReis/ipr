// -*- C++ -*-
//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copyright and license notices.
//
// Module partition: cxx.ipr:vocabulary
// Foundational types for the IPR interface: structural templates,
// strong enums, source locations, sequences, and optionals.

module;

#include <ipr/std-preamble>

export module cxx.ipr:vocabulary;

// -- Utility subset: only the symbols needed by the interface --

namespace ipr::util {
    // Predicate to detect enumeration types.
    export template<typename T>
    concept EnumType = std::is_enum_v<T>;

    // The underlying type of an enumeration
    export template<EnumType T>
    using raw = std::underlying_type_t<T>;

    // Return the value representation of an enumeration value.
    export template<typename T> requires EnumType<T>
    constexpr auto rep(T t)
    {
        return static_cast<raw<T>>(t);
    }

    // Type of view over words as stored internally.
    export using word_view = std::u8string_view;

    // -- Check for nonnull pointer.
    export template<typename T>
    inline T* check(T* ptr)
    {
        if (ptr == nullptr)
            throw std::logic_error("attempt to dereference a null pointer");
        return ptr;
    }
}

// -- Forward declarations for the entire node hierarchy --
// These are needed by ancillary types (Sequence<T>, Optional<T>, etc.)
// and by the :syntax partition.

namespace ipr {
    export struct Node;                  // universal base class for all IPR nodes
    export struct Visitor;               // base class for all IPR visitor classes
    export struct Annotation;            // node annotations
    export struct Region;                // declarative region
    export struct Comment;               // C-style and BCPL-style comments
    export struct String;                // literal string
    export struct Language_linkage;       // general language linkage
    export struct Expr;                  // general expressions
    export struct Name;                  // general names
    export struct Type;                  // general types
    export struct Directive;             // general directives
    export struct Stmt;                  // general statements
    export struct Decl;                  // general declarations
    export struct Scope;                 // declarations in a region
    export struct Overload;              // overload set
    export struct Parameter_list;        // function/template parameter list

    // -- results of type constructor constants --
    export struct Array;                 // array type
    export struct As_type;               // use-expression as type
    export struct Class;                 // user-defined type - declared as "class" or "struct"
    export struct Decltype;              // strict type of a declaration/expression
    export struct Enum;                  // user-defined type - declared as "enum" or "class enum"
    export struct Tor;                   // types of constructors and destructors
    export struct Function;              // function type
    export struct Namespace;             // user-defined type - declared as "namespace"
    export struct Pointer;               // pointer type
    export struct Ptr_to_member;         // pointer-to-member type
    export struct Product;               // product type - not ISO C++ type
    export struct Qualified;             // cv-qualified types
    export struct Reference;             // reference type
    export struct Rvalue_reference;      // rvalue-reference type
    export struct Sum;                   // sum type - not ISO C++ type
    export struct Forall;                // universally quantified type
    export struct Union;                 // user-defined type - declared as "union"
    export struct Auto;                  // "auto" -- each occurrence is generative
    export struct Closure;               // closure type -- type of lambda expression

    // -- results of name constructor constants --
    export struct Identifier;            // identifier                          foo
    export struct Suffix;                // Literal operator suffix       "Plato"sv
    export struct Operator;              // C++ operator name             operator+
    export struct Conversion;            // conversion function name   operator int
    export struct Template_id;           // C++ template-id                  S<int>
    export struct Type_id;               // C++ type-id                  const int*
    export struct Ctor_name;             // constructor name                   T::T
    export struct Dtor_name;             // destructor name                   T::~T
    export struct Guide_name;            // deduction guide name 

    // -- results of nullary expression constructor constants --
    export struct Phantom;               // placeholder for arrays of unknown bounds, rethrow, etc.
    export struct Eclipsis;              // the `...' in a unary fold
    export struct Lambda;                // Lambda expression
    export struct Requires;              // requires-expression

    // -- results of unary expression constructor constants --
    export struct Symbol;                // self-evaluating symbolic values
    export struct Address;               // address-of                          &a
    export struct Array_delete;          // array delete-expression     delete[] p
    export struct Asm;                   // asm-declaration
    export struct Complement;            // bitwise complement                  ~m
    export struct Delete;                // delete-expression             delete p
    export struct Demotion;              // inverse of an integral/floating-point promotion
    export struct Deref;                 // dereference expression              *p
    export struct Expr_list;             // comma-separated expression list
    export struct Alignof;               // alignment query                  alignof(T)
    export struct Sizeof;                // sizeof expression
    export struct Typeid;                // typeid expression
    export struct Id_expr;               // use of a name as an expression
    export struct Label;                 // a label - target of a goto-statement
    export struct Materialization;       // temporary materialization
    export struct Not;                   // logical negation                 !cond
    export struct Enclosure;             // expression in paired delimiters
    export struct Post_decrement;        // post-decrement                     p--
    export struct Post_increment;        // post-increment                     p++
    export struct Pre_decrement;         // pre-decrement                      --p
    export struct Pre_increment;         // pre-increment                      ++p
    export struct Promotion;             // integral or floating-point promotion
    export struct Read;                  // lvalue to rvalue conversion
    export struct Throw;                 // throw expression               throw a
    export struct Unary_minus;           // unary minus                         -a
    export struct Unary_plus;            // unary plus                          +a
    export struct Expansion;             // pack expansion                    t...
    export struct Noexcept;              // noexcept expression        noexcept(e)
    export struct Args_cardinality;      // sizeof...(args)
    export struct Restriction;           // requires-clause

    // -- results of binary expression constructor constants --
    export struct Plus;                  // addition                       a + b
    export struct Plus_assign;           // in-place addition              a += b
    export struct And;                   // logical and                    a && b
    export struct Array_ref;             // array member selection         a[i]
    export struct Arrow;                 // indirect member selection      p->m
    export struct Arrow_star;            // indirect member indirection    p->*m
    export struct Assign;                // assignment                     a = b
    export struct Bitand;                // bitwise and                    a & b
    export struct Bitand_assign;         // in-place bitwise and           a &= b
    export struct Bitor;                 // bitwise or                     a | b
    export struct Bitor_assign;          // in-place bitwise or            a |= b
    export struct Bitxor;                // bitwise exclusive or           a ^ b
    export struct Bitxor_assign;         // in-place bitwise exclusive or  a ^= b
    export struct Call;                  // function call                  f(u, v)
    export struct Cast;                  // C-style cast                   (T) e
    export struct Coercion;              // generalized type conversion
    export struct Comma;                 // comma-operator                 a, b
    export struct Const_cast;            // const_cast-expression
    export struct Construction;          // object construction            T(v)
    export struct Div;                   // division                       a / b
    export struct Div_assign;            // in-place division              a /= b
    export struct Dot;                   // direct member selection        x.m
    export struct Dot_star;              // direct member indirection      x.*pm
    export struct Dynamic_cast;          // dynamic_cast-expression
    export struct Equal;                 // equality comparison            a == b
    export struct Greater;               // greater comparison             a > b
    export struct Greater_equal;         // greater-or-equal comparison    a >= b
    export struct Less;                  // less comparison                a < b
    export struct Less_equal;            // less-equal comparison          a <= b
    export struct Literal;               // literal expressions            3.14
    export struct Lshift;                // left shift                     a << b
    export struct Lshift_assign;         // in-place left shift            a <<= b
    export struct Member_init;           // member initialization          : m(v)
    export struct Minus;                 // subtraction                    a - b
    export struct Minus_assign;          // in-place subtraction           a -= b
    export struct Modulo;                // modulo arithmetic              a % b
    export struct Modulo_assign;         // in-place modulo arithmetic     a %= b
    export struct Mul;                   // multiplication                 a * b
    export struct Mul_assign;            // in-place multiplication        a *= b
    export struct Narrow;                // checked base to derived conversion
    export struct Not_equal;             // not-equality comparison        a != b
    export struct Or;                    // logical or                     a || b
    export struct Pretend;               // generalization of bitcast/reinterpret cast
    export struct Qualification;         // cv-qualification conversion
    export struct Reinterpret_cast;      // reinterpret_cast-expression
    export struct Rshift;                // right shift                    a >> b
    export struct Rshift_assign;         // in-place right shift           a >>= b
    export struct Scope_ref;             // qualified name                N::f
    export struct Static_cast;           // static_cast-expression
    export struct Widen;                 // derived to base class conversion
    export struct Binary_fold;           // primary expression (a op ... op b)
    export struct Mapping;               // function
    export struct Rewrite;               // semantics by translation
    export struct Where;                 // expression with local bindings
    export struct Static_assert;         // static-assert declaration
    export struct Instantiation;         // substitution into parameterized expression

    // -- result of trinary expression constructor constants --
    export struct New;                   // new-expression              new (p) T(v)
    export struct Conditional;           // conditional                   p ? a : b

    // -- result of statement constructor constants --
    export struct Block;                 // brace-enclosed statement sequence
    export struct Break;                 // break-statement
    export struct Continue;              // continue-statement
    export struct Ctor_body;             // constructor-body
    export struct Do;                    // do-statement
    export struct Expr_stmt;             // expression-statement
    export struct For;                   // for-statement
    export struct For_in;                // structured for-statement
    export struct Goto;                  // goto-statement
    export struct Handler;               // exception handler statement
    export struct If;                    // if-statement
    export struct Labeled_stmt;          // labeled-statement
    export struct Return;                // return-statement
    export struct Switch;                // switch-statement
    export struct While;                 // while-statement

    // -- result of declaration constructor constants --
    export struct Template;              // parameterized declaration
    export struct Enumerator;            // classic enumerator
    export struct Alias;                 // alias declaration (typedef, namespace-alias, etc.)
    export struct Base_type;             // base-specifier
    export struct Parameter;             // parameter declaration
    export struct EH_parameter;          // exception handler parameter
    export struct Fundecl;               // function declaration
    export struct Concept;               // concept declaration
    export struct Var;                   // variable declaration
    export struct Field;                 // nonstatic data member
    export struct Bitfield;              // bit-field data member
    export struct Typedecl;              // type declaration

    export struct Translation_unit;
    export struct Module_unit;
    export struct Interface_unit;
    export struct Module;
    export struct Lexicon;
}

                                // -- Various Location Types --
// C++ constructs span locations.  There are at least four flavours of
// locations:
//       (a) physical source location;
//       (b) logical source location;
//       (c) physical unit location; and
//       (d) logical unit location.
// Physical and logical source locations are locations as witnessed
// at translation phase 2 (see ISO C++, §2.1/1).
// Physical and logical unit locations are locations as manifest when
// looking at a translation unit, at translation phase 4.
//
// Many IPR nodes will have a source and unit locations.  Instead of
// storing a source file and unit name as a string in every
// location data type, we save space by mapping file-names and
// unit-names to IDs (integers).  That mapping is managed by the Unit
// instance.

namespace ipr {
    export enum class Line_number : std::uint32_t { };
    export enum class Column_number : std::uint32_t { };

    export struct Basic_location {
        Line_number line = { };
        Column_number column = { };
    };

    export enum class File_index : std::uint32_t { };

    export struct Source_location : Basic_location {
        File_index file = { };    // ID of the file
    };

    export enum class Unit_index : std::uint32_t { };

    export struct Unit_location : Basic_location {
        Unit_index unit = { };    // ID of the unit
    };
}

// -- Ancillary types --

namespace ipr {
    // ---------------------------
    // -- Phases of translation --
    // ---------------------------
    // A bitmask type for the various phases of C++ program translation.
    export enum class Phases {
        Unknown              = 0x0000,   // Unknown translation phase
        Reading              = 0x0001,   // Opening and reading an input source
        Lexing               = 0x0002,   // Lexical decomposition of input source
        Preprocessing        = 0x0004,   // Macro expansion and friends
        Parsing              = 0x0008,   // Grammatical decomposition of the input source
        Name_resolution      = 0x0010,   // Name lookup
        Typing               = 0x0020,   // Type assignment of expressions
        Evaluation           = 0x0040,   // Compile-time evaluation
        Instantiation        = 0x0080,   // Template instantiation phase
        Code_generation      = 0x0100,   // Code generation phase
        Linking              = 0x0200,   // Linking phase
        Loading              = 0x0400,   // Program loading phase
        Execution            = 0x0800,   // Runtime execution

        Elaboration          = Name_resolution | Typing | Evaluation | Instantiation,
        All                  = ~0x0,
    };

    // -- Qualifiers --
    // Abstract data type capturing compositions of standard and extended type qualifiers.
    // Composition of type qualifiers is commutative.
    export enum class Qualifiers : std::uintptr_t { };

                                // -- Binding_mode --
    // Mode of binding of a object value to a name (parameter, variable, alias, etc).
    export enum class Binding_mode : std::uint8_t {
        Copy,                         // by copy operation; default binding mode of C and C++
        Reference,                    // by ref; the parameter or varable has an lvalue reference type
        Move,                         // by move operation; transfer of ownership
        Default = Copy,
    };

                                 // -- Delimiter --
    // Enclosure delimiters of expressions
    export enum class Delimiter {
        Nothing,                   // no delimiter
        Paren,                     // "()"
        Brace,                     // "{}"
        Bracket,                   // "[]"
        Angle,                     // "<>"
    };
}

// -- Structural base templates --

namespace ipr {
                                // -- Basic_unary --
    // A structure entirely determined by its sole component, the `operand()`.
    export template<typename Operand>
    struct Basic_unary {
        using Arg_type = Operand;
        virtual Operand operand() const = 0;
    };

                                // -- Unary<> --
    // A unary-expression is a specification of an operation that takes
    // only one operand, an expression.  By extension, a unary-node is a
    // node category that is essentially determined only by one node,
    // its "operand".  Usually, such an operand node is a classic expression.
    // Occasionally, it can be a type (e.g. sizeof (T)), we don't want to
    // loose that information, therefore we add a template-parameter
    // (the second) to indicate the precise type of the operand.  The first
    // template-parameter designates the actual node subcategory this class
    // provides an interface for.
    export template<class Cat, class Operand = const Expr&>
    struct Unary : Cat, Basic_unary<Operand> { };

                                // -- Basic_binary --
    // A structure entirely determined by its two components, the `first()`
    // and `second()`.
    export template<typename First, typename Second>
    struct Basic_binary {
        using Arg1_type = First;
        using Arg2_type = Second;
        virtual First first() const = 0;
        virtual Second second() const = 0;
    };

                                // -- Binary<> --
    // In full generality, a binary-expression is an expression that
    // consists in (a) an operator, and (b) two operands.  In Standard
    // C++, the two operands often are of the same type (and if they are not,
    // they are implicitly converted).  In IPR, they need not be
    // of the same type.  This generality allows representations of
    // cast-expressions which are conceptually binary-expressions -- they
    // take a type and an expression.  Also, a function call is
    // conceptually a binary-expression that applies a function to
    // a list of arguments.  By extension, a binary node is any node that
    // is essentially dermined by two nodes, its "operands".
    // As for Unary<> nodes, we indicate the operands' type information
    // through the template-parameters First and Second.
    export template<class Cat, class First = const Expr&, class Second = const Expr&>
    struct Binary : Cat, Basic_binary<First, Second> { };

                                // -- Ternary<> --
    // Similar to Unary<> and Binary<> categories.  This is for
    // ternary-expressions, or more generally for ternary nodes.
    // An example of a ternary node is a Conditional node.
    export template<class Cat, class First = const Expr&,
             class Second = const Expr&, class Third = const Expr&>
    struct Ternary : Cat {
        using Arg1_type = First;
        using Arg2_type = Second;
        using Arg3_type = Third;
        virtual First first() const = 0;
        virtual Second second() const = 0;
        virtual Third third() const = 0;
    };
}

// -- Sequence and Optional --

namespace ipr {
                                // -- Sequence<> --
    // Often, we use a notion of sequence to represent intermediate
    // abstractions like base-classes, enumerators, catch-clauses,
    // parameter-type-list, etc.  A "Sequence" is a collection abstractly
    // described by its "begin()" and "end()" iterators.  It is made
    // abstract because it may admit different implementations depending
    // on concrete constraints.  For example, a scope (a sequence of
    // declarations, that additionally supports look-up by name) may
    // implement this interface either as a associative-array that maps
    // Names to Declarations (with no particular order) or as
    // vector<Declaration*> (in order of their appearance in programs), or
    // as a much more elaborated data structure.
    export template<class T>
    struct Sequence {
        struct Iterator;          // An iterator is a pair of (sequence,
                                  // position).  The position indicates
                                  // the particular value referenced in
                                  // the sequence.

        // Provide STL-style interface, for use with some STL algorithm
        // helper classes.  Sequence<> is an immutable sequence.
        using value_type = T;
        using reference = const T&;
        using pointer = const T*;
        using Index = std::size_t;
        using iterator = Iterator;

        virtual Index size() const = 0;
        bool empty() const { return not (size() > 0); }
        Iterator begin() const;
        Iterator end() const;
        Iterator position(Index) const;

    protected:
        virtual const T& get(Index) const = 0;
    };

                                // -- Sequence<>::Iterator --
    // This iterator class is as abstract as it could be, for useful
    // purposes.  It forwards most operations to the "Sequence" class
    // it provides a view for.
    template<class T>
    struct Sequence<T>::Iterator {
        using self_type = Sequence<T>::Iterator;
        using value_type = const T;
        using reference = const T&;
        using pointer = const T*;
        using difference_type = ptrdiff_t;
        using Index = typename Sequence<T>::Index;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() {}
        Iterator(const Sequence* s, Index i) : seq{ s }, index{ i } { }

        const T& operator*() const
        { return seq->get(index); }

        const T* operator->() const
        { return &seq->get(index); }

        Iterator& operator++()
        {
            ++index;
            return *this;
        }

        Iterator& operator--()
        {
            --index;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++index;
            return tmp;
        }

        Iterator operator--(int)
        {
            Iterator tmp = *this;
            --index;
            return tmp;
        }

        bool operator==(Iterator other) const
        { return seq == other.seq and index == other.index; }

        bool operator!=(Iterator other) const
        { return not(*this == other); }

    private:
        const Sequence* seq { };
        Index index { };
    };

    template<class T>
    inline typename Sequence<T>::Iterator
    Sequence<T>::position(Index i) const
    { return { this, i }; }

    template<class T>
    inline typename Sequence<T>::Iterator
    Sequence<T>::begin() const
    { return { this, 0 }; }

    template<class T>
    inline typename Sequence<T>::Iterator
    Sequence<T>::end() const
    { return { this, size() }; }

                                // -- Optional<> --
    // Occasionally, a node has an optional property (e.g. a variable
    // has an optional initializer).  This class template captures
    // that commonality and provides a checked access.
    export template<typename T>
    struct Optional {
        constexpr Optional(const T* p = nullptr) : ptr{p} { }
        constexpr Optional(const T& t) requires std::is_abstract_v<T> : ptr{&t} { }
        const T& get() const { return *util::check(ptr); }
        bool is_valid() const { return ptr != nullptr; }
        explicit operator bool() const { return is_valid(); }
        template<typename U, bool = std::is_base_of_v<T, U>>
        operator Optional<U>() const { return { ptr }; }
    private:
        const T* ptr;
    };
}
