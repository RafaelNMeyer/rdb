#ifndef RDB_TYPES_HPP
#define RDB_TYPES_HPP

#include <array>
#include <cstdint>
#include <cstring>

namespace rdb {
using byte64 = std::array<std::byte, 8>;   // 64 bits
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

class virt_addr {
public:
  virt_addr() = default;
  explicit virt_addr(std::uint64_t addr) : addr_(addr) {}

  std::uint64_t addr() const { return addr_; }

  virt_addr operator+(std::int64_t offset) const {
    return virt_addr(addr_ + offset);
  }

  virt_addr operator-(std::int64_t offset) const {
    return virt_addr(addr_ - offset);
  }

  virt_addr &operator+=(std::int64_t offset) {
    addr_ += offset;
    return *this;
  }

  virt_addr &operator-=(std::int64_t offset) {
    addr_ -= offset;
    return *this;
  }

  bool operator==(virt_addr &other) const { return addr_ == other.addr_; }
  bool operator!=(virt_addr &other) const { return addr_ != other.addr_; }
  bool operator<(virt_addr &other) const { return addr_ < other.addr_; }
  bool operator<=(virt_addr &other) const { return addr_ <= other.addr_; }
  bool operator>(virt_addr &other) const { return addr_ > other.addr_; }
  bool operator>=(virt_addr &other) const { return addr_ >= other.addr_; }

private:
  std::uint64_t addr_ = 0;
};

} // namespace rdb

#endif
