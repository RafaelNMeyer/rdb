#ifndef RDB_BIT_HPP
#define RDB_BIT_HPP

#include <cstddef>
#include <cstring>
namespace rdb {
template <class To> To from_bytes(const std::byte *bytes) {
  To ret;
  memcpy(&ret, bytes, sizeof(To));
  return ret;
}

template <class From> const std::byte *as_bytes(const From &from) {
  return reinterpret_cast<const std::byte *>(&from);
}

template <class From> std::byte *as_bytes(From &from) {
  return reinterpret_cast<std::byte *>(&from);
}
} // namespace rdb

#endif
