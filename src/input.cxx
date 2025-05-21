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

#include <ipr/input>

namespace ipr::input {
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
        if (s.QuadPart)
            return;
        SystemHandle mapping = CreateFileMapping(file.get_handle(), nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (mapping.get_handle() == nullptr)
            throw FileMappingError{ path, GetLastError() };
        auto start = MapViewOfFile(mapping.get_handle(), FILE_MAP_READ, 0, 0, 0);
        view = { reinterpret_cast<const std::byte*>(start), static_cast<View::size_type>(s.QuadPart) };
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
        view = { reinterpret_cast<std::byte*>(start), static_cast<View::size_type>(s.st_size) };
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
            munmap(const_cast<std::byte*>(view.data()), view.size());
#endif
        }
    }
}