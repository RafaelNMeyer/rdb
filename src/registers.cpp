#include <algorithm>
#include <cstdint>
#include <iostream>
#include <librdb/bit.hpp>
#include <librdb/error.hpp>
#include <librdb/process.hpp>
#include <librdb/register_info.hpp>
#include <librdb/registers.hpp>
#include <librdb/types.hpp>
#include <type_traits>
#include <variant>

namespace {
template <class T> rdb::byte128 widen(const rdb::register_info &info, T t) {
  using namespace rdb;
  if constexpr (std::is_floating_point_v<T>) {
    if (info.format == register_format::double_float)
      return to_byte128(static_cast<double>(t));
    if (info.format == register_format::long_double)
      return to_byte128(static_cast<long double>(t));
  } else if constexpr (std::is_signed_v<T>) {
    if (info.format == register_format::uint) {
      switch (info.size) {
      case 2:
        return to_byte128(static_cast<std::int16_t>(t));
      case 4:
        return to_byte128(static_cast<std::int32_t>(t));
      case 8:
        return to_byte128(static_cast<std::int64_t>(t));
      }
    }
  }
  return to_byte128(t);
}
} // namespace

rdb::registers::value rdb::registers::read(const register_info &info) const {
  auto bytes = as_bytes(data_);

  if (info.format == register_format::uint) {
    switch (info.size) {
    case 1:
      return from_bytes<std::uint8_t>(bytes + info.offset);
    case 2:
      return from_bytes<std::uint16_t>(bytes + info.offset);
    case 4:
      return from_bytes<std::uint32_t>(bytes + info.offset);
    case 8:
      return from_bytes<std::uint64_t>(bytes + info.offset);
    default:
      error::send("Unexpected register size");
    }
  } else if (info.format == register_format::double_float) {
    return from_bytes<double>(bytes + info.offset);
  } else if (info.format == register_format::long_double) {
    return from_bytes<long double>(bytes + info.offset);
  } else if (info.format == register_format::vector && info.size == 8) {
    return from_bytes<byte64>(bytes + info.offset);
  } else {
    return from_bytes<byte128>(bytes + info.offset);
  }
}

void rdb::registers::write(const register_info &info, value val) {
  auto bytes = as_bytes(data_);

  // this only writes to our registers from user struct
  std::visit(
      [&](auto &v) -> void {
        if (sizeof(v) <= info.size) {
          auto wide = widen(info, v);
          auto val_bytes = as_bytes(wide);
          std::copy(val_bytes, val_bytes + info.size, bytes + info.offset);
        } else {
          std::cerr << "rdb::registers::write called with mismatched register "
                       "and value sizes";
          std::terminate();
        }
      },
      val);

  // needed to align 8 bytes for registers ah,bh,ch,dh
  auto aligned_offset = info.offset & ~0b111;
  // this write to user area!
  // and can throw an error since we cannot write to i837 registers
  // they are smaller than 64 bits and we can write more than once
  // so we do a condition here
  if (info.type == register_type::fpr) {
    proc_->write_fprs(data_.i387);
  } else {
    proc_->write_user_area(aligned_offset,
                           from_bytes<std::uint64_t>(bytes + aligned_offset));
  }
}
