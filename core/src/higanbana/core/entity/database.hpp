#pragma once
#include "higanbana/core/entity/sparsetable.hpp"
#include "higanbana/core/entity/tagtable.hpp"
#include "higanbana/core/entity/query.hpp"
#include "higanbana/core/datastructures/vector.hpp"
#include "higanbana/core/platform/definitions.hpp"
#include <stdint.h>
#include <cstdint>
#include <array>
#include <type_traits>
#include <random>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <string>
namespace higanbana
{
  template <typename T>
  auto typehash()
  {
    //std::hash<std::string> hfn;
#if defined(HIGANBANA_PLATFORM_WINDOWS)
    return ::std::string(__FUNCSIG__);
#else
    return ::std::string(__PRETTY_FUNCTION__);
#endif
  }

  // this just encapsulates the id I guess
  typedef size_t Id;


  // COuNTER ACTION RISING!
  template<size_t INDEX_TABLE, size_t MAXELEMENTS = INDEX_TABLE*128>
  class Database
  {
    template <typename T>
    using SparseTable = _SparseTable<T, MAXELEMENTS, INDEX_TABLE>;
    template <typename T>
    using TagTable = _TagTable<T, INDEX_TABLE>;
    using Indexfield = Bitfield<INDEX_TABLE>;

    // data
    std::unordered_map<std::string, std::shared_ptr<void>> m_components;
    std::unordered_map<std::string, std::shared_ptr<void>> m_tags;
    std::vector<Indexfield*> m_indextables;
    Indexfield m_entities;

    // random
    //std::mt19937 m_eng;
    uint64_t m_nextId;

    inline Id nextUniqueIndex()
    {
      //Id id(m_eng() % MAXELEMENTS);
      Id id((m_nextId++) % MAXELEMENTS);

      while (id < MAXELEMENTS  && m_entities.checkIdxBit(id))
      {
        ++id;
        if (id >= MAXELEMENTS)
        {
          id = (m_nextId++) % MAXELEMENTS;
        }
      }
      return id;
    }


    bool seen(std::string hash)
    {
      return m_components.find(hash) != m_components.end();
    }
    bool seenTag(std::string hash)
    {
      return m_tags.find(hash) != m_tags.end();
    }

  public:
    Database(){}

    Id createEntity()
    {
      auto e = nextUniqueIndex();
      m_entities.setIdxBit(e); //m_entities only used for checking available id
      return e;
    }

    // must be called before adding new entities
    // Not intersect! union? all indexes orred.
    void refreshEntitytable()
    {
      if (m_indextables.size() == 0)
        return;

      Bitfield<INDEX_TABLE> transaction;
      //printf("m_indextables size %ld\n", m_indextables.size());
      for (int i=0;i<m_indextables.size();++i)
      {
        transaction = bitunion(transaction, *m_indextables[i]);
      }
      m_entities = transaction;
    }

    void deleteEntity(Id e)
    {
      for (auto it : m_indextables)
      {
        it->clearIdxBit(e);
      }
      m_entities.clearIdxBit(e);
    }

    size_t entityCount()
    {
      return m_entities.countElements();
    }

    template<typename T>
    [[nodiscard]] SparseTable<T>& get() {
      std::string hash = typehash<T>();
      typedef SparseTable<T> TYPETORETURN;
      if (seen(hash))
      {
        return *std::static_pointer_cast<TYPETORETURN>(m_components[hash]);
      }
      auto pdata = std::make_shared<TYPETORETURN>();
      m_indextables.emplace_back(&pdata->getBitfield());
      m_components[hash] = pdata; // cast to void* implicit
      return *pdata;
    }

    template<typename T>
    TagTable<T>& getTag()
    {
      std::string hash = typehash<T>();
      typedef TagTable<T> TYPETORETURN;
      if (seenTag(hash))
      {
        return *std::static_pointer_cast<TYPETORETURN>(m_tags[hash]);
      }
      auto pdata = std::make_shared<TYPETORETURN>();
      m_indextables.emplace_back(&pdata->getBitfield());
      m_tags[hash] = pdata; // cast to void* implicit
      return *pdata;
    }
  };
}
