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
#include "ipr/input"

namespace ipr::input {
    static constexpr std::uint32_t index_watermark { 1u << 31 }; 


    LineIndex::LineIndex(LineSort s, std::uint32_t i)
        : srt{(assert(s == LineSort::Simple || s == LineSort::Composite), std::to_underlying(s))}, 
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
                if (cursor <= line_start)
                    continue;               // skip entirely blank lines.
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
                    depot.indices.emplace_back(LineSort::Composite, idx);
                    composite.lines.clear();
                }
                else
                {
                    auto idx = depot.simples.size();
                    depot.indices.emplace_back(LineSort::Simple, idx);
                    depot.simples.emplace_back(line);
                }
            }

            return depot;
        }
    }

    SourceListing::SourceListing(const SystemPath& path) 
        : SourceFile{path}, depot{read_lines(*this)}
    { }

    const SimpleLine& SourceListing::simple_line(LineIndex line) const
    {
        assert(idx.sort() == LineSort::Simple);
        auto n = line.index();
        assert(n < depot.simples.size());
        return depot.simples[n];
    }

    const CompositeLine& SourceListing::composite_line(LineIndex line) const
    {
        assert(idx.sort() == LineSort::Composite);
        auto n = line.index();
        assert(n < depot.composites.size());
        return depot.composites[n];
    }
}