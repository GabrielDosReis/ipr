#include "doctest/doctest.h"

#include <ipr/impl>

void test_wharehouse()
{
    ipr::impl::Lexicon lexicon{};

    union X
    {
        using Type_list = ipr::impl::Wharehouse<ipr::Type>;
        Type_list types;
        char mem[sizeof(types)];
        X() : types() {}
        ~X()
        {
            types.~Type_list();
            // trash the memory
            std::fill(std::begin(mem), std::end(mem), 255);
        }
    };

    const ipr::Product* product{ };
    const ipr::Sum* sum{ };

    {
        X x;
        x.types.push_back(lexicon.int_type());
        x.types.push_back(lexicon.char_type());
        product = &lexicon.get_product(x.types);
        sum = &lexicon.get_sum(x.types);

        (void)product->size();
        (void)sum->size();
        puts("first size ok");
    }

    product->size(); // BOOM if not copied into lexicon type_seq
    sum->size();  // BOOM if not copied into lexicon type_seq
    puts("second size ok");
}
