#include "doctest/doctest.h"

#include <ipr/impl>

TEST_CASE("warehouse") {
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
        FragileWarehouse warehouse;
        warehouse.types.push_back(lexicon.int_type());
        warehouse.types.push_back(lexicon.char_type());
        product = &lexicon.get_product(warehouse.types);
        sum = &lexicon.get_sum(warehouse.types);

        CHECK(warehouse.types.size() == 2);
        CHECK(product->size() == 2);
        CHECK(sum->size() == 2);

        // verify that begin / end works
        size_t count = 0;
        for (auto& type: warehouse.types)
            ++count;
        CHECK(count == 2);
    }

    CHECK(product->size() == 2);  // BOOM if not copied into lexicon type_seq
    CHECK(sum->size() == 2); // BOOM if not copied into lexicon type_seq
}
