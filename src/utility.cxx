//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis.
// See LICENSE for copright and license notices.
//

#include <ipr/utility>

#include <algorithm>

char
ipr::util::string::operator[](int i) const
{
   if (i < 0 or i >= length)
      throw std::domain_error("invalid index for util::string::operator[]");
   return data[i];
}

ipr::util::string::arena::arena()
      : mem(static_cast<pool*>(operator new(poolsz))),
        next_header(mem->storage)
{
   mem->previous = nullptr;
}

ipr::util::string::arena::~arena()
{
   while (mem != nullptr) {
      pool* cur = mem;
      mem = mem->previous;
      operator delete (cur);
   }
}

// Allocate storage sufficient to hold an immutable string of length "n".

ipr::util::string*
ipr::util::string::arena::allocate(int n)
{
   const int m = (n - string::padding_count + headersz - 1) / headersz + 1;
   string* header{};

   // If we have enough space left, juts grab it.
   if (m <= remaining_header_count()) {
      header = next_header;
      next_header += m;
   }
   // If we need to allocate storage more than what can possibly fit
   // in a fresh pool object, just allocate that string on its own
   else if (n > bufsz) {
      pool* new_pool = static_cast<pool*>
         (operator new(poolsz + (n - bufsz)));
      header = new_pool->storage;

      new_pool->previous = mem->previous;
      mem->previous = new_pool;
   }
   // Not enough space left.  Take the bet that, there does not
   // remain sufficient room in the buffer.  This is likely so if
   // the buffer is allocated sufficiently large to start with.
   else {
      pool* new_pool = static_cast<pool*>(operator new(poolsz));
      new_pool->previous = mem;
      mem = new_pool;

      header = mem->storage;
      next_header = header + m;
   }

   return header;
}

const ipr::util::string*
ipr::util::string::arena::make_string(const char* s, int n)
{
   string* header = allocate(n);

   header->length = n;
   // >>>> Yuriy Solodkyy: 2007/05/29
   // Put cast to avoid gettinch assert with safe STL in MSVC
   std::copy(s, s + n, (char*)header->data);
   // <<<< Yuriy Solodkyy: 2007/05/29
   return header;
}
