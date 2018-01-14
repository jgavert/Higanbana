#pragma once

#include "core/src/external/SpookyV2.hpp"
#include "core/src/external/robinhashmap.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <vector>
#include <deque>

//#define USING_SPARSEPP

inline size_t HashMemory(const void * p, size_t sizeBytes)
{
  return size_t(SpookyHash::Hash64(p, sizeBytes, 0));
}

// boost hash combine
inline size_t hash_combine(size_t lhs, size_t rhs) {
  lhs ^= rhs + 0x9e3779b97f4a7c16 + (lhs << 6ull) + (lhs >> 2ull);
  return lhs;
}

template <typename K>
inline size_t HashKey(const K* key, size_t size)
{
  return HashMemory(key, size);
}

template<typename TKey>
struct Hasher
{
  std::size_t operator()(const TKey& key) const
  {
    return HashKey(&key, sizeof(TKey));
  }
};

template <>
struct Hasher<std::string>
{
  std::size_t operator()(const std::string& key) const
  {
    return HashKey(key.data(), key.size());
  }
};

namespace faze
{
#if defined(USING_SPARSEPP) && defined(_DEBUG)
  template <typename key, typename val>
  using unordered_map = spp::sparse_hash_map<key, val, Hasher<key>>;

  template <typename key>
  using unordered_set = spp::sparse_hash_set<key, Hasher<key>>;
#elif 1
  // cannot use robinhood hashmap yet, missing possibility to loop all elements.
  template <typename key, typename val, typename hasher = Hasher<key>>
  using unordered_map = RobinHoodInfobytePairNoOverflow::Map<key, val, hasher>;

  template <typename key, typename hasher = Hasher<key>>
  using unordered_set = std::unordered_set<key, Hasher<key>>;
#else
  template <typename key, typename val>
  using unordered_map = std::unordered_map<key, val, Hasher<key>>;

  template <typename key>
  using unordered_set = std::unordered_set<key, Hasher<key>>;
#endif

  template <typename type>
  using vector = std::vector<type>;

  template <typename type>
  using deque = std::deque<type>;
}
