#include "doctest/doctest.h"

#include <ipr/traversal>
#include <ipr/impl>
#include <ipr/utility>

const ipr::Type* get_enclosing_udt_if_any(const ipr::Region& current_region)
{
    auto* region = &current_region;
    for (;;)
    {
        switch (region->owner().category)
        {
        case ipr::Category_code::Class:
        case ipr::Category_code::Enum:
        case ipr::Category_code::Union:
            return ipr::util::view<ipr::Type>(region->owner());

        case ipr::Category_code::Block:
        case ipr::Category_code::Namespace:
            return nullptr;

        default:
            region = &region->enclosing();
        }
    }
}

TEST_CASE("Region-owner user") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  // the helper above does not consider namespace to be udt type
  auto* not_found = get_enclosing_udt_if_any(*unit.global_region());
  CHECK(not_found == nullptr);

  auto& clazz = *lexicon.make_class(*unit.global_region());
  auto* found = get_enclosing_udt_if_any(clazz.region());
  CHECK(found == &clazz);
}
