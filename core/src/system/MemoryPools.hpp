#pragma once

#include "core/src/datastructures/proxy.hpp"

#include <memory>
#include <functional>

namespace faze
{
  template <typename T>
  class Rabbitpool // they will multiply if needed, infinite supply but never too much
  {
  private:
    std::shared_ptr<vector<T>> m_pool;
    std::function<T()> m_makeItem;

  public:
    RabbitPool(std::function<T()> makeItem)
      : m_pool(std::make_shared<vector<T>>())
      , m_makeItem(makeItem)
    {
    }

    struct Item
    {
      std::weak_ptr<vector<T>> m_pool;
      T object;

      Item(T object, std::shared_ptr<vector<T>> pool)
        : m_pool(pool)
        , object(std::forward<decltype(object)>(object))
      {}

      ~Item()
      {
        if (auto pool = m_pool.lock())
        {
          pool.emplace_back(object);
        }
      }
    };

    struct SharedItem
    {
      std::shared_ptr<Item> p;

      T* operator->()
      {
        return &p->item;
      }
    };

    SharedItem allocate()
    {
      if (m_pool.empty())
      {
        m_pool->emplace_back(std::move(m_makeItem()));
      }
      auto obj = std::make_shared<Item>(m_pool->back(), m_pool);
      m_pool->pop_back();

      return obj;
    }
  };
}