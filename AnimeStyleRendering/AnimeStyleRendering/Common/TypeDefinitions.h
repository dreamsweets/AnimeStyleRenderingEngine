#pragma once
#include <stdint.h>
#include <optional>

using uchar = unsigned char;
// unsigned int
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

// signed int
using s64 = int64_t;
using s32 = int32_t;
using s16 = int16_t;
using s8 = int8_t;

constexpr u64 u64_invalid_id(0xffff'ffff'ffff'ffffui64);
constexpr u32 u32_invalid_id(0xffff'ffffui32);
constexpr u16 u16_invalid_id(0xffffui16);
constexpr u8 u8_invalid_id(0xffui8);

using f32 = float;

template <typename T>
using u_ptr = std::unique_ptr<T>;

template <typename T>
using s_ptr = std::shared_ptr<T>;

template <typename T>
using func = std::function<T>;

namespace anime {
template <typename T>
using Vector = std::vector<T>;

template <typename T>
using List = std::list<T>;

template <typename T>
using Deque = std::deque<T>;

using String = std::string;

// Hash = Unordered_Map
template <typename KeyType, typename Data>
using Hash = std::unordered_map<KeyType, Data>;

// Optional
template <typename T>
using Option = std::optional<T>;
constexpr auto nullopt = std::nullopt;

// Pair
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;
}  // namespace anime