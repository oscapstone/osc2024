#pragma once

#include "util.hpp"

// ref:
// https://github.com/llvm/llvm-project/blob/llvmorg-18.1.6/compiler-rt/lib/orc/stl_extras.h
//===-------- stl_extras.h - Useful STL related functions-------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of the ORC runtime support library.
//
//===----------------------------------------------------------------------===//

/// Substitute for std::bit_ceil.
constexpr uint64_t bit_ceil(uint64_t Val) noexcept {
  Val |= (Val >> 1);
  Val |= (Val >> 2);
  Val |= (Val >> 4);
  Val |= (Val >> 8);
  Val |= (Val >> 16);
  Val |= (Val >> 32);
  return Val + 1;
}

// ref:
// https://github.com/llvm/llvm-project/blob/llvmorg-18.1.6/compiler-rt/lib/orc/bitmask_enum.h
//===---- bitmask_enum.h - Enable bitmask operations on enums ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <type_traits>

#define MARK_AS_BITMASK_ENUM(LargestValue) \
  BITMASK_LARGEST_ENUMERATOR = LargestValue

#define DECLARE_ENUM_AS_BITMASK(Enum, LargestValue)                     \
  template <>                                                           \
  struct is_bitmask_enum<Enum> : std::true_type {};                     \
  template <>                                                           \
  struct largest_bitmask_enum_bit<Enum> {                               \
    static constexpr std::underlying_type_t<Enum> value = LargestValue; \
  }

template <typename E, typename Enable = void>
struct is_bitmask_enum : std::false_type {};

template <typename E>
struct is_bitmask_enum<
    E, std::enable_if_t<sizeof(E::BITMASK_LARGEST_ENUMERATOR) >= 0>>
    : std::true_type {};

template <typename E>
inline constexpr bool is_bitmask_enum_v = is_bitmask_enum<E>::value;

template <typename E, typename Enable = void>
struct largest_bitmask_enum_bit;

template <typename E>
struct largest_bitmask_enum_bit<
    E, std::enable_if_t<sizeof(E::BITMASK_LARGEST_ENUMERATOR) >= 0>> {
  using UnderlyingTy = std::underlying_type_t<E>;
  static constexpr UnderlyingTy value =
      static_cast<UnderlyingTy>(E::BITMASK_LARGEST_ENUMERATOR);
};

template <typename E>
constexpr std::underlying_type_t<E> Mask() {
  return bit_ceil(largest_bitmask_enum_bit<E>::value) - 1;
}

template <typename E>
constexpr std::underlying_type_t<E> Underlying(E Val) {
  auto U = static_cast<std::underlying_type_t<E>>(Val);
  // assert(U >= 0 && "Negative enum values are not allowed");
  // assert(U <= Mask<E>() && "Enum value too large (or langest val too small");
  return U;
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
constexpr E operator~(E Val) {
  return static_cast<E>(~Underlying(Val) & Mask<E>());
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
constexpr E operator|(E LHS, E RHS) {
  return static_cast<E>(Underlying(LHS) | Underlying(RHS));
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
constexpr E operator&(E LHS, E RHS) {
  return static_cast<E>(Underlying(LHS) & Underlying(RHS));
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
constexpr E operator^(E LHS, E RHS) {
  return static_cast<E>(Underlying(LHS) ^ Underlying(RHS));
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
E& operator|=(E& LHS, E RHS) {
  LHS = LHS | RHS;
  return LHS;
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
E& operator&=(E& LHS, E RHS) {
  LHS = LHS & RHS;
  return LHS;
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
E& operator^=(E& LHS, E RHS) {
  LHS = LHS ^ RHS;
  return LHS;
}

template <typename E, typename = std::enable_if_t<is_bitmask_enum_v<E>>>
bool has(E LHS, E RHS) {
  return (LHS & RHS) == RHS;
}

template <typename E, typename T>
E cast_enum(T Val) {
  return static_cast<E>((uint64_t)(Val)&Mask<E>());
}
