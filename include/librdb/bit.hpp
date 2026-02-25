#ifndef RDB_BIT_HPP
#define RDB_BIT_HPP

#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>
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

inline std::string_view to_string_view(const std::byte *data,
                                       std::size_t size) {
  return {reinterpret_cast<const char *>(data), size};
}

inline std::string_view to_string_view(const std::vector<std::byte> &data) {
  return {reinterpret_cast<const char *>(data.data()), data.size()};
}

} // namespace rdb

#endif
