#ifndef RDB_TYPES_HPP
#define RDB_TYPES_HPP

#include <array>
#include <cstring>

namespace rdb {
using byte64 = std::array<std::byte, 8>; // 64 bits
using byte128 = std::array<std::byte, 16>; // 128 bits

template <class From> byte64 to_byte64(const From &src) {
  byte64 ret{};
  memcpy(&ret, &src, sizeof(From));
  return ret;
}

template <class From> byte128 to_byte128(const From &src) {
  byte128 ret{};
  memcpy(&ret, &src, sizeof(From));
  return ret;
}
} // namespace rdb

#endif
