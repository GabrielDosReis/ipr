![Build & Test](https://github.com/GabrielDosReis/ipr/actions/workflows/main.yml/badge.svg)

The IPR, short for *Internal Program Representation*, is an open source
project originally developed as the core data structures of a framework
for semantics-based analysis and transformation of C++ programs.  The
foundations of the IPR were laid down and implemented between 2004 and
2005, as part of The Pivot research project.  An overview, the
general philosophy, and the design principles behind the IPR are
presented in the paper ["A Principled, Complete, and Efficient
Representation of C++"](http://www.stroustrup.com/macis09.pdf) 
<!-- Restore when axiomatic is up ["A Principled, Complete, and Efficient
Representation of C++"](http://www.axiomatics.org/~gdr/ipr/mcs.pdf)-->.
That paper is a useful source of general information about the IPR
and non-obvious challenges in representing C++ programs in their most
general forms.

The IPR library purposefully separates the interface (a collection of abstract 
classes found in `<ipr/interface>`) from the implementation (found in `<ipr/impl`>)
for various reasons.  An interface class (say `ipr::Fundecl`) can admit several 
implementations: a class for non-defining function declarations, and another class for  
function definitions.  Furthermore, compilers and tools (in general) can provide their own
specific optimized implementations of the interface without impacting users of the
IPR as long as those users restrict themselves to the public interface.  Such a
separation of concerns shields users of the library from implementation vagaries.
The implementation in `<ipr/impl>` is provided for exposition and reference.

For more information, bug reports, and suggestions, please visit

   https://github.com/GabrielDosReis/ipr

Gabriel Dos Reis, Bjarne Stroustrup.


