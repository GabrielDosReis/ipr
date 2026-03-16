// -*- C++ -*-
//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copyright and license notices.
//
// Module partition: cxx.ipr:syntax
// C++ syntactic forms: tokens, attributes, declarators, constraints,
// initialization provisions, and designators.
//
// -- The IPR focuses primarily on capturing the semantics of C++, not mimic - with
// -- high fidelity - the ISO C++ source-level syntax minutiae, grammars, and other obscurities.
// -- The IPR model of declaration reflects that semantics-oriented view.  Occasionally, it
// -- is necessary to bridge the gap between the generalized semantics model and the
// -- irregularities and other grammatical hacks via IPR directives.  Some of those
// -- directives need to refer to some form of input source level constructs, especially in
// -- cases of templated declarations involving non-ground types.
// -- This partition defines interfaces for some syntactic forms, i.e. abstractions of syntactic
// -- input, to aid complete embedding of the ISO C++ specification.
// -- Note: There is no effort here to reify the parse trees of the ISO C++ grammar.

module;

#include <ipr/std-preamble>

export module cxx.ipr:syntax;
import :vocabulary;

// -----------------------------------
// -- Basic syntactic units: Tokens --
// -----------------------------------
// The IPR focuses primarily on semantic aspects of C++ constructs.
// Certain constructs, such as uninstantiated template definitions,
// are primarily syntactic with minimal semantic processing.  IPR nodes
// can fully represent those structurally well-defined syntactic entities.
// However, C++11 added attributes which are essentially token soups.  These
// attributes make the IPR interface less abstract than wanted.

namespace ipr {
    // General classification of tokens, e.g. identifier, number, space, comment, etc.
    export enum class TokenCategory : std::uint8_t { };

    // A numerical value associated with each token.
    export enum class TokenValue : std::uint16_t { };

    export struct Lexeme {
        virtual const String& spelling() const = 0;
        virtual const Source_location& locus() const = 0;
    };

    export struct Token {
        virtual const Lexeme& lexeme() const = 0;
        virtual TokenValue value() const = 0;
        virtual TokenCategory category() const = 0;
    };

    export struct Attribute {
        struct Visitor;
        virtual void accept(Visitor&) const = 0;
    };

    // A simple token used as attribute.
    export struct BasicAttribute : Unary<Attribute, const Token&> {
        const Token& token() const { return operand(); }
    };

    // An attribute of the form `token1 :: token2'
    export struct ScopedAttribute : Binary<Attribute, const Token&, const Token&> {
        const Token& scope() const { return first(); }
        const Token& member() const { return second(); }
    };

    // An attribute of the form `token : attribute'.
    export struct LabeledAttribute : Binary<Attribute, const Token&, const Attribute&> {
        const Token& label() const { return first(); }
        const Attribute& attribute() const { return second(); }
    };

    // An attribute of the form `f(args)'.
    export struct CalledAttribute : Binary<Attribute, const Attribute&,
                                          const Sequence<Attribute>&> {
        const Attribute& function() const { return first(); }
        const Sequence<Attribute>& arguments() const { return second(); }
    };

    // An attribute of the form `attribute...'
    export struct ExpandedAttribute : Binary<Attribute, const Token&, const Attribute&> {
        const Token& expander() const { return first(); }
        const Attribute& operand() const { return second(); }
    };

    // An attribute of the form `[[using check: memory(3), type(2)]]'
    export struct FactoredAttribute : Binary<Attribute, const Token&,
                                             const Sequence<Attribute>&> {
        const Token& factor() const { return first(); }
        const Sequence<Attribute>& terms() const { return second(); }
    };

    // An attribute of the form `[[ expr ]]', where `expr' is the elaboration result
    // of parsing and semantics analysis of the enclosed token sequence.  This is a 
    // common non-standard form of attribute.
    export struct ElaboratedAttribute : Unary<Attribute, const Expr&> {
        const Expr& elaboration() const { return operand(); }
    };

    struct Attribute::Visitor {
        virtual void visit(const BasicAttribute&) = 0;
        virtual void visit(const ScopedAttribute&) = 0;
        virtual void visit(const LabeledAttribute&) = 0;
        virtual void visit(const CalledAttribute&) = 0;
        virtual void visit(const ExpandedAttribute&) = 0;
        virtual void visit(const FactoredAttribute&) = 0;
        virtual void visit(const ElaboratedAttribute&) = 0;
    };
}

// -- C++ syntactic forms --

namespace ipr::cxx_form {
    // -- Syntactic constraints.
    // -- C++ Concepts, as supported since C++20, focus primarily on the syntactic requirements
    // -- on combinations of template arguments. The semantics counterpart is not yet supported by
    // -- the base language.
    // -- A constraint generally applies a concept to a compile-time argument list.  Hence the syntax
    // -- uses the "template specialization" syntax for the application.  The result is a Boolean
    // -- expression indicating whether the syntactic constraints contained in the body if the 
    // -- concept are satisfied or not.  Given a monadic concept (a concept with exactly one parameter),
    // -- the concept name, as a predicate, can be viewed as a type for a template-type parameter.  
    // -- That is known as the "short-hand notation" . Similar notational convenience for non-type
    // -- template parameter isn't as straightforward.
    // -- Given a polyadic concept (a concept with more N parameters, N > 1), a constraint can be formed
    // -- by applying the concept name to N-1 compile-time arguments (the "trailing arguments"). 

    export struct Constraint_visitor;

    export struct Constraint {
        struct Monadic;                                         // -> C;
        struct Polyadic;                                        // -> C<e1, e2>;
        virtual void accept(Constraint_visitor&) const = 0;
    };

    // An object of this type corresponds to an instance of the first alternative of the ISO C++
    // grammar production for type-constraint
    //          nested-name-specifier_opt concept-name
    struct Constraint::Monadic : Constraint {
        virtual Optional<Expr> scope() const = 0;
        virtual const Identifier& concept_name() const = 0;
    };

    // An object of this type corresponds to an instance of the second alternative of the ISO C++
    // grammar production for type-constraint
    //          nested-name-specifier_opt concept-name `< template-argument--list_opt `>`
    struct Constraint::Polyadic : Constraint {
        virtual Optional<Expr> scope() const = 0;
        virtual const Identifier& concept_name() const = 0;
        virtual const Sequence<Expr>& trailing_arguments() const = 0;
    };

    export struct Constraint_visitor {
        virtual void visit(const Constraint::Monadic&) = 0;
        virtual void visit(const Constraint::Polyadic&) = 0;
    };

    // Base class for navigation of requirements.
    export struct Requirement_visitor;

    // -- Typically, the definition of a concept is expressed in terms of syntactic requirements.
    // -- Those requirements are formulated as proof obliggations for the validity of stylized
    // -- expressions (usage patterns).
    export struct Requirement {
        struct Simple;                                  // *p = t;
        struct Type;                                    // typename C::iterator;
        struct Compound;                                // { &*p } -> same_as<T*>;
        struct Nested;                                  // requires Trivial<C::iterator>;
        virtual void accept(Requirement_visitor&) const = 0;
    };

    // An object of this type corresponds to an instance of the ISO C++ grammar production
    // for simple-requirement:
    //          expression `;`
    struct Requirement::Simple : Requirement {
        virtual const Expr& expr() const = 0;
    };

    // An object of this type corresponds to an instance of the ISO C++ grammar production
    // for type-requirement:
    //      `typename` nested-name-specifier_opt type-namexpression `;`
    struct Requirement::Type : Requirement {
        virtual Optional<Expr> scope() const = 0;
        virtual const Name& type_name() const = 0;
    };

    // An object of this type corresponds to an instance of the ISO C++ grammar production
    // for compound-requirement:
    //      `{` expression `}` `noexcept`_opt return-type-requirement_opt `;`
    // where the grammar production for return-type-requirement is
    //     `->` type-constraint
    struct Requirement::Compound : Requirement {
        virtual const Expr& expr() const = 0;
        virtual Optional<Constraint> constraint() const = 0;
        virtual bool nothrow() const = 0;
    };

    // An object of this type corresponds to an instance of the ISO C++ grammar production
    // for nested-requirement:
    //      `requires` constant-expression `;`
    struct Requirement::Nested : Requirement {
        virtual const Expr& condition() const = 0;
    };

    export struct Requirement_visitor {
        virtual void visit(const Requirement::Simple&) = 0;
        virtual void visit(const Requirement::Type&) = 0;
        virtual void visit(const Requirement::Compound&) = 0;
        virtual void visit(const Requirement::Nested&) = 0;
    };

    // -- Abstraction of ISO C++ Declarators:
    // -- The C and C++ languages have tortuous grammars for declarations, whereby
    // -- declarations are structured to mimic use (operator precedence).  A declaration
    // -- is made of two essential pieces (decl-specifiers-seq and declarator), with an
    // -- an optional component representing the initializer.  The decl-specifier-seq
    // -- is a soup made of decl-specifier such as `static`, `extern`, etc. and simple-type
    // -- names such as `int`, and type qualifiers.  Compound types are made with
    // -- type-constructors such as `*` (pointer) or `&` (reference), `[N]` (array), etc.  
    // -- The type-constructors (which really imply some form of data indirection) are specified
    // -- along with the name being introduced, making up the declarator.
    // -- Pointer-style type-constructors are modeled by `Indirector`.
    // -- Mapping-style type constructors (functions, arrays) are modeled by classes in
    // -- `Morphism` hierarchy.
    // -- Complete declarators that have mapping-style type contructors are modelled in the
    // -- `Species_declarator` class hierarchy.

    export struct Indirector_visitor;      // Base class of visitors for traversing `Indirector`s.

    // Base class for objects representing ptr-operator (C++ grammar).
    // While the source-level grammar of ISO C++ does not allow attributes on
    // reference-style ptr-operator, the representation adopted here makes room for that possibility.
    // Hence all `Indirector` objects have an `attributes()` operation.
    // At the input source level, `Indirector`s precede the name being declared in the declarator.
    export struct Indirector {
        struct Pointer;             // pointer indirector
        struct Reference;           // reference indirector
        struct Member;              // non-static member indirector
        virtual const Sequence<Attribute>& attributes() const = 0;
        virtual void accept(Indirector_visitor&) const = 0;
    };

    // A `Pointer` indirector object is a simple possibly cv-qualified pointer indirection.
    // And object of this type corresponds to the instance of the ptr-operator C++ grammar:
    //      `*` attribute-specifier-seq_opt cv-qualifier-seq_opt
    struct Indirector::Pointer : Indirector {
        virtual Qualifiers qualifiers() const = 0;
    };

    // Syntactic indication of reference binding flavor.
    export enum class Reference_flavor {
        Lvalue,                     // "&" ptr-operator in an indirector
        Rvalue                      // "&&" ptr-operator in an indirector
    };

    // A `Reference` indirector object is a reference indirection.
    // An object of this type corresponds to an instance of any of the following alternatives
    // of the ptr-operator C++ grammar production:
    //     `&` attribute-specifier-seq_opt
    //     `&&` attribute-specifier-seq_opt
    struct Indirector::Reference : Indirector {
        virtual Reference_flavor flavor() const = 0;
    };

    // A member `Indirector` object represents a ptr-operator that specifies a pointer to member.
    // An object of this type corresponds to an instance of the following alternative of the
    // ptr-operator C++ grammar production:
    //     nested-named-specifier `*` attribute-specifier-seq_opt cv-qualifier-seq_opt
    struct Indirector::Member : Indirector {
        virtual const Expr& scope() const = 0;
        virtual Qualifiers qualifiers() const = 0;
    };

    // Traversal of Indirector objects is facilitated by visitor classes deriving
    // from this interface.
    export struct Indirector_visitor {
        virtual void visit(const Indirector::Pointer&) = 0;
        virtual void visit(const Indirector::Reference&) = 0;
        virtual void visit(const Indirector::Member&) = 0;
    };

    // -- Species and Morphisms
    // A species declarator introduces a name along with typical usage of a named being declared.
    // This usage pattern is captured by a suffix of `Morphism`s.  A `Morphism` is a form that
    // specifies the type constuctor from the declared entity to the type of typical uses of that 
    // entity in expressions.
    //   - Function: a callable expression, the entity is typically called
    //   - Array: a table expression, the entity is typically indexed

    // -- Morphism and navigation
    export struct Morphism_visitor;

    export struct Morphism {
        struct Function;                            // -- (T, int) & noexcept
        struct Array;                               // -- [34][] [[S::A]]
        virtual const Sequence<Attribute>& attributes() const = 0;
        virtual void accept(Morphism_visitor&) const = 0;
    };

    export struct Morphism_visitor {
        virtual void visit(const Morphism::Function&) = 0;
        virtual void visit(const Morphism::Array&) = 0;
    };

    // -- Species and navigation
    export struct Species_visitor;

    // A species declarator is either a name (possibly a pack), or a parenthesized term declarator, followed
    // by a possibly empty sequence of morphisms.  For instance, in the declaration
    //        bool (*pfs[8])(int)
    // that declares pfs as an array of 8 pointers to functions taking an `int` returning a `bool`, the complete
    // declarator `(*pfs[8])(int)` is a parenthesized species (`(*pfs[8])`) with the suffix consisting exactly of
    // the singleton sequence of function morphism `(int)`.
    // The term declarator `*pfs[8]` in turn has the indirectors is comprised of the single pointer indirector `*`,
    // and of the id species `pfs` with array suffix `[8]`.
    export struct Species_declarator {
        struct Unqualified_id;                      // -- int p;
        struct Pack;                                // -- Ts... xs
        struct Qualified_id;                        // -- int X<T>::count;
        struct Parenthesized;                       // parenthesized term declarator -- (*p)
        virtual const Sequence<Morphism>& suffix() const = 0;
        virtual void accept(Species_visitor&) const = 0;
    };

    // Base class for representing instances of the C++ production id-expression in the C++
    // grammar for declarator-id.
    export struct Declarator_id : Species_declarator {
        virtual const Sequence<Attribute>& attributes() const = 0;
    };

    // Representation of an unqualified-id in an instance of the C++ grammar production for declarator-id.
    // The operation `name()` returns an optional value for use in representing instances of the C++
    // grammar production for noptr-abstract-declarator.
    struct Species_declarator::Unqualified_id : Declarator_id {
        virtual Optional<ipr::Name> name() const = 0;
    };

    // Representation of a pack parameter in an instance of the C++ grammar production for declarator-id
    // The operation `name()` returns an optional value for use in representing instances of the C++
    // grammar production for noptr-abstract-abstract-declarator.
    // Example:
    //     ... x
    struct Species_declarator::Pack : Declarator_id {
        virtual Optional<ipr::Identifier> name() const = 0;
    };

    // Representation of a qualified-id in an instance of the C++ grammar production for declarator-id
    // Such qualified-id are used in the out-of-class definition of an entity.
    struct Species_declarator::Qualified_id : Declarator_id {
        virtual const ipr::Expr& scope() const = 0;
        virtual const ipr::Name& member() const = 0;
    };

    // -- Declarator.
    // -- A declarator is either a term, or a species with a target type.
    // -- A term is a sequence of indirectors followed by a species.
    // -- A species indicates the typical usage syntactic structure of the named being declared.

    export struct Declarator_visitor;

    export struct Declarator {
        struct Term;                                // *p or (&a)[42]
        struct Targeted;                            // f(T& p) -> int
        virtual const Species_declarator& species() const = 0;
        virtual void accept(Declarator_visitor&) const = 0;
    };

    // A term declarator is a sequence of `Indirector`s followed by a species declarator.
    // An object of this type represents an instance of the first alternative of the C++ 
    // grammar production for declarator:
    //     ptr-declarator
    // Examples:
    //     (&a)[42]
    //     *f(T) noexcept
    struct Declarator::Term : Declarator {
        virtual const Sequence<Indirector>& indirectors() const = 0;
    };

    // A targeted declarator is a species declarator with a trailing return type,
    // given by the `target()` operation.
    // An object of this type represents an instance of the second alternative of the C++
    // grammar production for declarator:
    //     noptr-declarator paramters-and-qualifiers trailing-return-type
    // In particular, the `species()` of a targeted declarator has a function Morphism as 
    // last element in its `suffix()`.  Furthermore, a targeted declarator lacks indirectors.
    // Examples:
    //    f(T a, U b) -> decltype(a + b)
    struct Declarator::Targeted : Declarator {
        virtual const Type& target() const = 0;
    };

    // Traversal of declarator objects is facilitated by visitor classes deriving
    // from this interface.
    export struct Declarator_visitor {
        virtual void visit(const Declarator::Term&) = 0;
        virtual void visit(const Declarator::Targeted&) = 0;
    };

    // A term declarator requiring parentheses to obey operator precedence rules, or just
    // a redundant parentheses. An object of this type represents an instance of the fourth
    // alternative of the C++ grammar for noptr-declarator:
    //      `(` ptr-declarator `)`
    // Example:
    //      (a)
    //      (*p)
    struct Species_declarator::Parenthesized : Species_declarator {
        virtual const Declarator::Term& term() const = 0;
    };

    // -- Function Morphism
    // A morphism for a declarator indicating something that can be called.
    // An object of this type captures the components introduced by an instance the second 
    // alternative of the C++ grammar of noptr-delarator:
    //    noptr-declarator parameters-and-qualifiers
    struct Morphism::Function : Morphism {
        virtual const Parameter_list& parameters() const = 0;
        virtual Qualifiers qualifiers() const = 0;
        virtual Binding_mode binding_mode() const = 0;
        virtual Optional<Expr> throws() const = 0;
    };

    // -- Array Morphism
    // A morphism for a declarator indicating something that can be indexed.
    // An object of this type captures the components introduced by an instance the third
    // alternative of the C++ grammar of noptr-delarator:
    //    noptr-declarator `[` constant-expression_opt `]` attribute-specifier-seq_opt
    struct Morphism::Array : Morphism {
        virtual Optional<Expr> bound() const = 0;
    };

    // Traversal of species objects is facilitated by visitor classes deriving
    // from this interface.
    export struct Species_visitor {
        virtual void visit(const Species_declarator::Unqualified_id&) = 0;
        virtual void visit(const Species_declarator::Pack&) = 0;
        virtual void visit(const Species_declarator::Qualified_id&) = 0;
        virtual void visit(const Species_declarator::Parenthesized&) = 0;
    };

    // -- Proclamator.
    // A proclamator is a complete declarator along with an initializer, or with a constraint.
    // A proclamator represents an instance of the C++ grammar for init-declarator.  Furtermore,
    // a proclamator holds the result of elaborating its declarator with a decl-specifier-seq that
    // preceds it in a declaration.

    // -- Initialization provision.
    // -- An initialization provision is an initializer form that stipulates how an entity
    // -- (the name of which is introduced by a declarator) shall be initialized. Choices range from
    // -- classic provision introduced by the `=` sign, to recent innovations such as parenthesized
    // -- expression list or brace enclosed initializer list for uniform initialization.

    export struct Provision_visitor;

    // Base class of initialization provision classes.  They don't share much in common other than
    // appearing in top-level init-declarators.
    export struct Initialization_provision {
        virtual void accept(Provision_visitor&) const = 0;
    };

    // Base for navigating elemental initializers.
    export struct Initializer_visitor;

    // Base class of initializer form (other than earmarked initializer) that can appear in a
    // brace-enclosed initilizer form. 
    export struct Elemental_initializer {
        virtual void accept(Initializer_visitor&) const = 0;
    };

    // Initialization provision of the form
    //     `=` initializer-clause
    export struct Classic_provision : Initialization_provision {
        virtual const Elemental_initializer& initializer() const = 0;
    };

    // Initialization provision of the form
    //    `(` expression-list `)`;
    export struct Parenthesized_provision : Initialization_provision {
        virtual const Expr& initializer() const = 0;
    };

    // Embedding of an expression in an elemental initializer hierarchy.
    // Note that the expression here can be either
    //      expression
    // in general, or an
    //     expression-list_opt
    export struct Expr_initializer : Elemental_initializer {
        virtual const Expr& expression() const = 0;
    };

    // Initialization provision of the form
    //    `{` initializer-list_opt `}`
    export struct Braced_provision : Initialization_provision, Elemental_initializer {
        virtual const Sequence<Elemental_initializer>& elements() const = 0;
    };

    // An earmarked initializer is a form that specifies an initializer for a specific subobject
    // of a structure or an array, in a designated-initializer-list.  Strictly speaking, ISO C++
    // (unlike C99) currently supports only designated-initializer-list for structures, but not
    // arrays.  But since the C99 designated-initialization for arrays is in practice accepted
    // by major compilers as extensions, and that extension fits the general model here, the form
    // data structures below make room for them.
    export struct Earmarked_initializer;

    // An initializer provision of the form
    //    `{` designated-initializer-list_opt `}`
    export struct Designated_list_provision : Initialization_provision, Elemental_initializer {
        virtual const Sequence<Earmarked_initializer>& elements() const = 0;
    };

    // Base class for navigation of initialization provisions.
    export struct Provision_visitor {
        virtual void visit(const Classic_provision&) = 0;
        virtual void visit(const Parenthesized_provision&) = 0;
        virtual void visit(const Braced_provision&) = 0;
        virtual void visit(const Designated_list_provision&) = 0;
    };

    // -- Designator.
    // A designator is a form that identifies an immediate subobject of a structure (by name),
    // or an array (by slot index).  For example, in designated-initializer-list, the element
    //     .galaxy { "Milky Way"}
    // provides a braced-initializer for the field named `galaxy`.  Similarly the element
    //     [4] = "Tau'ri"
    // provides a classic-initializer for the fifth slot of the array being initialized.
    export struct Designator_visitor;

    export struct Subobject_designator {
        virtual void accept(Designator_visitor&) const = 0;
    };

    // Designator of a field.
    // Example:
    //     . location
    export struct Field_designator : Subobject_designator {
        virtual const Identifier& name() const = 0;
    };

    // Designator of an array slot.
    // Example:
    //    [ 13 ]
    export struct Slot_designator : Subobject_designator {
        virtual const Expr& index() const = 0;
    };

    export struct Designator_visitor {
        virtual void visit(const Field_designator&) = 0;
        virtual void visit(const Slot_designator&) = 0;
    };

    // An earmarked initializer is an element of a designated-initializer-list that provides
    // an initialier for a direct subobject of a structure or an array.
    // Example:
    //    .galaxy { "Milky Way" }
    //    [4] = "Tau'ri"
    export struct Earmarked_initializer {
        virtual const Subobject_designator& subobject() const = 0;
        virtual const Initialization_provision& initializer() const = 0;
    };

    // Base class for navigation of elemental initializers.
    export struct Initializer_visitor {
        virtual void visit(const Expr_initializer&) = 0;
        virtual void visit(const Braced_provision&) = 0;
        virtual void visit(const Designated_list_provision&) = 0;
    };

    export struct Proclamator_visitor;

    export struct Proclamator {
        struct Initialized;                         // int ary[] { 1, 2, 3 };
        struct Constrained;
        virtual const Declarator& declarator() const = 0;
        virtual Decl& result() const = 0;
        virtual void accept(Proclamator_visitor&) const = 0;
    };

    struct Proclamator::Initialized : Proclamator {
        virtual Optional<Initialization_provision> initializer() const = 0;
    };

    struct Proclamator::Constrained : Proclamator {
        virtual const Expr& constraint() const = 0;
    };

    export struct Proclamator_visitor {
        virtual void visit(const Proclamator::Initialized&) = 0;
        virtual void visit(const Proclamator::Constrained&) = 0;
    };
}
