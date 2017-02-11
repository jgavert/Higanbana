#pragma once

#include <sparsepp.h>
#include "core/src/spookyhash/SpookyV2.h"
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