#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <memory>
#include <future>

#include <catch2/catch.hpp>

using namespace higanbana;
using namespace higanbana::backend;

class ReadbackTexture
{
  //std::shared_ptr<ViewResourceHandle> m_id;
  bool cool_value;
  public:
  ReadbackTexture(bool value = false): cool_value(value){}

  explicit operator bool() const
  {
    return cool_value;
  }
};

class GiveFutures
{
  deque<std::promise<ReadbackTexture>> promises;
  public:

  GiveFutures() {}

  std::future<ReadbackTexture> giveFuture()
  {
    std::promise<ReadbackTexture> promise;
    promises.emplace_back(std::move(promise));
    return promises.back().get_future();
  }
  void activateFuture()
  {
    std::promise<ReadbackTexture> promise = std::move(promises.front());
    promises.pop_front();
    promise.set_value(ReadbackTexture(promises.empty()));
  }
};