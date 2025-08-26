//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copright and license notices.
//

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <fcntl.h>
#  include <unistd.h>
#endif

#include <assert.h>
#include <iostream>
#include <utility>
#include <string_view>
#include <algorithm>
#include "ipr/input"

namespace ipr::input {
    namespace {
        // A cap on valid index values.
        constexpr std::uint64_t index_watermark { std::uint64_t{1} << 58 }; 

        // A valid input line is either simple or composite.
        bool valid_category(LineSort k)
        {
            return k == LineSort::Simple or k == LineSort::Composite; 
        }
    }

    bool valid_species(LineSpecies s)
    {
        switch (s) {
        case LineSpecies::Text:
        case LineSpecies::SolitaryHash:
        case LineSpecies::If:
        case LineSpecies::Ifdef:
        case LineSpecies::Ifndef:
        case LineSpecies::Elif:
        case LineSpecies::Elifdef:
        case LineSpecies::Elifndef:
        case LineSpecies::Else:
        case LineSpecies::Endif:
        case LineSpecies::Include:
        case LineSpecies::Export:
        case LineSpecies::Import:
        case LineSpecies::Embed:
        case LineSpecies::Define:
        case LineSpecies::Undef:
        case LineSpecies::Line:
        case LineSpecies::Error:
        case LineSpecies::Warning:
        case LineSpecies::Pragma:
        case LineSpecies::ExtendedDirective:
            return true;
        default:
            return false;
        }
    }

    LineDescriptor::LineDescriptor(LineSort k, LineSpecies s, std::uint64_t i)
        : srt{(assert(valid_category(k)), std::to_underlying(k))},
          spc{(assert(valid_species(s)), std::to_underlying(s))},
          idx{(assert(i < index_watermark), i)}
    {
    }



#ifdef _WIN32
    // Helper type for automatically closing a handle on scope exit.
    struct SystemHandle {
        SystemHandle(HANDLE h) : handle{h} { }
        bool valid() const { return handle != INVALID_HANDLE_VALUE; }
        auto get_handle() const { return handle; }
        ~SystemHandle()
        {
            if (valid())
                CloseHandle(handle);
        }
    private:
        HANDLE handle;
    };
#endif

    SourceFile::SourceFile(const SystemPath& path)
    {
#ifdef _WIN32
        // FIXME: Handle the situation of large files in a 32-bit program.
        static_assert(sizeof(LARGE_INTEGER) == sizeof(std::size_t));

        SystemHandle file = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, nullptr);
        if (not file.valid())
            throw AccessError{ path, GetLastError() };
        LARGE_INTEGER s { };
        if (not GetFileSizeEx(file.get_handle(), &s))
            throw AccessError{ path, GetLastError() };
        if (s.QuadPart == 0)
            return;
        SystemHandle mapping = CreateFileMapping(file.get_handle(), nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (mapping.get_handle() == nullptr)
            throw FileMappingError{ path, GetLastError() };
        auto start = MapViewOfFile(mapping.get_handle(), FILE_MAP_READ, 0, 0, 0);
        view = { reinterpret_cast<const char8_t*>(start), static_cast<View::size_type>(s.QuadPart) };
#else
        struct stat s { };
        errno = 0;
        if (stat(path.c_str(), &s) < 0)
            throw AccessError{ path, errno };
        else if (not S_ISREG(s.st_mode))
            throw RegularFileError{ path };

        // Don't labor too hard with empty files.
        if (s.st_size == 0)
            return;
        
        auto fd = open(path.c_str(), O_RDONLY);
        if (fd < 0)
            throw AccessError{ path, errno };
        auto start = mmap(nullptr, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (start == MAP_FAILED)
            throw FileMappingError{ path };
        view = { reinterpret_cast<const char8_t*>(start), static_cast<View::size_type>(s.st_size) };
#endif
    }

    SourceFile::SourceFile(SourceFile&& src) noexcept : view{src.view}
    {
        src.view = { };
    }

    SourceFile::~SourceFile()
    {
        if (not view.empty())
        {
#ifdef _WIN32
            UnmapViewOfFile(view.data());
#else
            munmap(const_cast<char8_t*>(view.data()), view.size());
#endif
        }
    }

    SourceFile::View SourceFile::contents(Morsel m) const noexcept
    {
        assert(m.length < view.size());
        return { view.data() + m.offset, m.length };
    }

    // All code fragments directly indexable must have offsets and extents less than these limits.
    constexpr auto max_offset = std::uint64_t{1} << 48;
    constexpr auto max_extent = std::uint64_t{1} << 16;

    // Characters from a raw input source file marking new lines: either CR+LR or just LF.
    constexpr char8_t carriage_return = 0x0D;    // '\r';
    constexpr char8_t line_feed = 0x0A;          // '\n';

    static inline bool white_space(char8_t c)
    {
        switch (c)
        {
        case u' ': case u8'\t': case u8'\v': case u8'\f':
            return true;
        default:
            return false;
        }
    }

    void SourceFile::LineRange::next_line() noexcept
    {
        const auto offset = static_cast<std::uint64_t>(ptr - src->view.data());
        assert(offset < max_offset);
        const auto limit = src->view.size();
        std::uint64_t idx = 0;
        while (idx < limit and ptr[idx] != carriage_return and ptr[idx] != line_feed)
            ++idx;
        assert(idx < max_extent);
        cache.morsel.offset = offset;
        cache.morsel.length = idx;
        ++cache.number;

        // Skip the new line marker.
        if (idx < limit)
        {
            if (ptr[idx] == carriage_return and idx+1 < limit and ptr[idx+1] == line_feed)
                ++idx;
            ++idx;
        }
        ptr += idx;
    }

    SourceFile::LineRange::LineRange(const SourceFile& src) : src{&src}, ptr{src.view.data()}
    {
        // Skip a possible misguided UTF-8 BOM.
        if (src.view.size() >= 3 and ptr[0] == 0xEF and ptr[1] == 0xBB and ptr[2] == 0xBF)
            ptr += 3;
        next_line();
    }

    PhysicalLine SourceFile::LineRange::iterator::operator*() const noexcept
    {
        assert(range != nullptr);
        return range->cache;
    }

    SourceFile::LineRange::iterator& SourceFile::LineRange::iterator::operator++() noexcept
    {
        assert(range != nullptr);
        if (range->ptr >= range->src->view.data() + range->src->view.size())
            range = nullptr;
        else
            range->next_line();

        return *this;
    }

    namespace {
        // A mapping of a preprocessing directive to an algebraic value.
        struct StandardDirective {
            const char8_t* name;
            LineSpecies species;
        };

        constexpr auto cmp_dir_by_name = [](auto& x, auto& y) constexpr {
            return std::u8string_view{x.name} < std::u8string_view{y.name};
        };

        // A mapping of standard directive spelling to line species.
        // Note: This table is stored in alphabetic order of the spelling.
        constexpr StandardDirective standard_directives[] {
            {u8"define", LineSpecies::Define},
            {u8"elif", LineSpecies::Elif},
            {u8"elifdef", LineSpecies::Elifdef},
            {u8"elifndef", LineSpecies::Elifndef},
            {u8"else", LineSpecies::Else},
            {u8"embed", LineSpecies::Embed},
            {u8"endif", LineSpecies::Endif},
            {u8"error", LineSpecies::Error},
            {u8"export", LineSpecies::Export},
            {u8"if", LineSpecies::If},
            {u8"ifdef", LineSpecies::Ifdef},
            {u8"ifndef", LineSpecies::Ifndef},
            {u8"import", LineSpecies::Import},
            {u8"include", LineSpecies::Include},
            {u8"line", LineSpecies::Line},
            {u8"pragma", LineSpecies::Pragma},
            {u8"undef", LineSpecies::Undef},
            {u8"warning", LineSpecies::Warning},
        };

        static_assert(std::ranges::is_sorted(standard_directives, cmp_dir_by_name));

        // If the argument for `s` is the spelling of a standard preprocessing directive,
        // return a pointer to the corresponding precomputed map entry.  Otherwise, return null.
        const StandardDirective* get_standard_directive(std::u8string_view s)
        {
            auto where = std::ranges::lower_bound(standard_directives, s, { }, &StandardDirective::name);
            if (where == std::end(standard_directives) or where->name > s)
                return nullptr;
            return where;
        }

        // This predicate holds if the argument for `c` denotes the first character of a standard
        // preprocessing directive.
        inline bool may_begin_standard_directive(char8_t c)
        {
            switch (c) {
            case u8'd': case u8'e': case u8'i': case u8'l':
            case u8'p': case u8'u': case u8'w':
                return true;
            default:
                return false;
            }
        }

        // Quick and simple predicate for constitutents of a narrow identifier.
        inline bool narrow_letter_or_digit(char8_t c)
        {
            return (c >= u8'A' and c <= u8'Z')
                or (c >= u8'z' and c <= u8'z')
                or (c >= u8'0' and c <= u8'9')
                or c == u8'_';
        }

        // Advance the argument bound to `cursor` past the next consecutive whitespace characters.
        inline void skip_blank(const char8_t*& cursor, const char8_t* end)
        {
            while (cursor < end and white_space(*cursor))
                ++cursor;
        }

        // Return the species of a logical line.
        LineSpecies species(SourceFile::View line)
        {
            auto cursor = line.data();
            const auto line_end = cursor + line.size();
            skip_blank(cursor, line_end);
            if (cursor == line_end)
                return LineSpecies::Unknown;
            else if (*cursor != u8'#')
                return LineSpecies::Text;

            skip_blank(++cursor, line_end);
            if (cursor == line_end)
                return LineSpecies::SolitaryHash;
            if (not may_begin_standard_directive(*cursor))
                return LineSpecies::ExtendedDirective;
            
            const auto directive_start = cursor;
            while (cursor < line_end and narrow_letter_or_digit(*cursor))
                ++cursor;
            if (auto directive = get_standard_directive({directive_start, cursor}))
                return directive->species;
            return LineSpecies::ExtendedDirective;
        }

        // Return the species of a composite logical line.
        LineSpecies species(const SourceFile& src, const CompositeLine& composite)
        {
            // FIXME: Building a buffer is not strictly needed in most practical cases.
            std::u8string buffer;
            for (auto& line : composite.lines)
            {
                auto chars = src.contents(line.morsel);
                buffer.append(chars.data(), chars.size());
            }
            return species(buffer);
        }

        LineDepot read_lines(const SourceFile& src)
        {
            LineDepot depot { };
            const auto file_start = src.contents().data();

            CompositeLine composite { };
            for (auto line: src.lines())
            {
                if (line.empty())
                    continue;
                // Trim any trailing whitespace character when determining logical line continuation.
                const auto line_start = file_start + line.morsel.offset;
                auto cursor = line_start + line.morsel.length;
                while (--cursor > line_start and white_space(*cursor))
                    ;
                if (*cursor == u8'\\')
                {
                    line.morsel.length = cursor - line_start;
                    composite.lines.push_back(line);
                    continue;
                }
                else if (not composite.lines.empty())
                {
                    composite.lines.push_back(line);
                    auto idx = depot.composites.size();
                    depot.composites.push_back(composite);
                    auto spc = species(src, composite);
                    depot.indices.emplace_back(LineSort::Composite, spc, idx);
                    composite.lines.clear();
                }
                else if (cursor == line_start)
                    continue;               // skip entirely blank logical lines.
                else
                {
                    auto idx = depot.simples.size();
                    auto spc = species({line_start, cursor});
                    depot.indices.emplace_back(LineSort::Simple, spc, idx);
                    depot.simples.emplace_back(line);
                }
            }

            return depot;
        }
    }

    SourceListing::SourceListing(const SystemPath& path) 
        : SourceFile{path}, depot{read_lines(*this)}
    { }

    const SimpleLine& SourceListing::simple_line(LineDescriptor line) const
    {
        assert(idx.sort() == LineSort::Simple);
        auto n = line.index();
        assert(n < depot.simples.size());
        return depot.simples[n];
    }

    const CompositeLine& SourceListing::composite_line(LineDescriptor line) const
    {
        assert(idx.sort() == LineSort::Composite);
        auto n = line.index();
        assert(n < depot.composites.size());
        return depot.composites[n];
    }
}