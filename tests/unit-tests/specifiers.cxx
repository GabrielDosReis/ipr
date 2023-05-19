#include <doctest/doctest.h>

#include <ctime>
#include <string_view>
#include <sstream>
#include <vector>
#include <algorithm>
#include <ipr/impl>
#include <ipr/io>
#include <ipr/utility>

const std::vector<std::u8string_view> specs {
    u8"export",
    
    u8"public",
    u8"protected",
    u8"private",

    u8"static",
    u8"extern",
    u8"mutable",
    u8"thread_local",
    u8"register",

    u8"virtual",
    u8"explicit",

    u8"friend",
    u8"inline",
    u8"consteval",
    u8"constexpr",
    u8"constinit",

    u8"typedef",
};

TEST_CASE("individual basic specifier") {
    using namespace ipr;
    impl::Lexicon lexicon { };
    impl::Module m {lexicon};
    impl::Interface_unit unit {lexicon, m};

    for (auto w : specs) {
        auto& logo = lexicon.get_logogram(lexicon.get_string(w));
        CHECK(logo.what().characters() == w);
        auto spec = lexicon.specifiers(ipr::Basic_specifier{logo});
        CHECK(spec != ipr::Specifiers{});

        auto vec = lexicon.decompose(spec);
        CHECK(vec.size() == 1);
        CHECK(vec[0].logogram().what().characters() == w);
  }
}

TEST_CASE("random combination of basic specifiers") {
    using namespace ipr;
    impl::Lexicon lexicon { };

    std::srand(std::time(nullptr));
    const auto spec_count = specs.size();
    const auto sample_size = 1 + std::rand() % spec_count;
    std::vector<std::u8string_view> test;
    ipr::Specifiers specifiers { };
    for (int i = 0; i < sample_size; ++i) {
        auto w = specs[std::rand() % sample_size];
        // Only add new specifier.
        if (std::find(test.begin(), test.end(), w) < test.end())
            continue;
        test.push_back(w);
        auto& logo = lexicon.get_logogram(lexicon.get_string(w));
        specifiers |= lexicon.specifiers(ipr::Basic_specifier{logo});
    }
    CHECK(test.size() == std::popcount(util::rep(specifiers)));

    auto elements = lexicon.decompose(specifiers);
    CHECK(elements.size() == test.size());

    std::ranges::sort(test);
    constexpr auto cmp = [](auto& x, auto& y) { 
        return x.logogram().what().characters() < y.logogram().what().characters(); 
    };
    std::ranges::sort(elements, cmp);
    for (auto i = 0u; i < test.size(); ++i) {
        CHECK(test[i] == elements[i].logogram().what().characters());
    }
}