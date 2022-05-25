#include "doctest/doctest.h"

#include <ipr/traversal>
#include <ipr/impl>
#include <ipr/utility>

const ipr::Region& nearest_namespace_or_block_region(const ipr::Region& current_region)
{
    using namespace ipr;
    auto* region = &current_region;
    for (;;)
    {
        auto& owner = region->owner();

        if (auto* block = ipr::util::view<Block>(owner))
            return block->region();
        if (auto* ns = ipr::util::view<Namespace>(owner))
            return ns->region();

        // keep going up
        region = &region->enclosing();
    }
}

TEST_CASE("Region-owner user") {
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  // the helper above does not consider namespace to be udt type
  auto& r1 = nearest_namespace_or_block_region(*unit.global_region());
  CHECK(&r1 == unit.global_region());

  auto& clazz = *lexicon.make_class(*unit.global_region());
  auto& r2 = nearest_namespace_or_block_region(clazz.region());
  CHECK(&r2 == unit.global_region());
}

TEST_CASE("Callable species")
{
  using namespace ipr;
  impl::Lexicon lexicon{};
  impl::Module module{lexicon};
  impl::Interface_unit unit{lexicon, module};

  auto& region = *unit.global_region();
  auto nesting = Mapping_level{0};

  auto& spread = *lexicon.make_specifiers_spread();
  auto& callable = *region.make_callable_species(region, nesting);
  callable.inputs.parms.owned_by = &spread;
  spread.proc_seq.push_back(&callable);

  auto& r = nearest_namespace_or_block_region(callable.inputs.region());
  CHECK(&r == unit.global_region());
}
