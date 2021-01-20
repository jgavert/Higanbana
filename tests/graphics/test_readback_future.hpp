#include <higanbana/graphics/GraphicsCore.hpp>
#include "graphics_config.hpp"
#include <string>
#include <memory>
#include <future>

#include <catch2/catch_all.hpp>

using namespace higanbana;
using namespace higanbana::backend;

class ReadbackTextureTest
{
  //std::shared_ptr<ViewResourceHandle> m_id;
  bool cool_value;
  public:
  ReadbackTextureTest(bool value = false): cool_value(value){}

  explicit operator bool() const
  {
    return cool_value;
  }
};

class GiveFutures
{
  deque<std::promise<ReadbackTextureTest>> promises;
  public:

  GiveFutures() {}

  std::future<ReadbackTextureTest> giveFuture()
  {
    std::promise<ReadbackTextureTest> promise;
    promises.emplace_back(std::move(promise));
    return promises.back().get_future();
  }
  void activateFuture()
  {
    std::promise<ReadbackTextureTest> promise = std::move(promises.front());
    promises.pop_front();
    promise.set_value(ReadbackTextureTest(promises.empty()));
  }
};