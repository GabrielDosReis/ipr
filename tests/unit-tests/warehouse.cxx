#include "doctest/doctest.h"

#include <ipr/impl>

void test_wharehouse()
{
    ipr::impl::Lexicon lexicon{};

    union FragileWarehouse
    {
        using Type_list = ipr::impl::Warehouse<ipr::Type>;
        Type_list types;
        char mem[sizeof(types)];
        FragileWarehouse() : types() {}
        ~FragileWarehouse()
        {
            // destruct the Warehouse
            types.~Type_list();
            // wipe the memory
            std::fill(std::begin(mem), std::end(mem), 255);
        }
    };

    const ipr::Product* product{ };
    const ipr::Sum* sum{ };

    {
        FragileWarehouse x;
        x.types.push_back(lexicon.int_type());
        x.types.push_back(lexicon.char_type());
        product = &lexicon.get_product(x.types);
        sum = &lexicon.get_sum(x.types);

        (void)product->size();
        (void)sum->size();
        puts("first size ok");
    }

    (void)product->size(); // BOOM if not copied into lexicon type_seq
    (void)sum->size();  // BOOM if not copied into lexicon type_seq
    puts("second size ok");
}
