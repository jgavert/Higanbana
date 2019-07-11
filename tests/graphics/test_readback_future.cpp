#include "tests/graphics/test_readback_future.hpp"
#include "tests/graphics/graphics_config.hpp"

TEST_CASE("returning own future") {
  vector<std::future<ReadbackTexture>> allFutures;
  GiveFutures provider;

  for (int i = 0; i < 10; ++i)
  {
    allFutures.push_back(provider.giveFuture());
  }

  int i = 0;
  provider.activateFuture();
  while(true)
  {
    if (allFutures[i]._Is_ready())
    {
      if (allFutures[i].get())
      {
        i++;
        break;
      }
      i++;
      provider.activateFuture();
    }
  }
  REQUIRE(i == 10);
}

TEST_CASE_METHOD(GraphicsFixture, "create readback in graph") {
  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setCount(8));

  auto graph = gpu().createGraph();

  auto node = graph.createPass("copy&readback");
  vector<float> arr = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
  MemView<float> view = makeMemView<float>(arr.data(), arr.size());
  auto dyn = gpu().dynamicBuffer<float>(view);
  node.copy(buffer, dyn);
  auto asyncReadback = node.readback(buffer);
  graph.addPass(std::move(node));

  REQUIRE(!asyncReadback->_Is_ready());
  gpu().submit(graph);
  gpu().waitGpuIdle();

  REQUIRE(asyncReadback->_Is_ready()); // result should be ready at some point after gpu idle
}

TEST_CASE_METHOD(GraphicsFixture, "create readback in graph&check data") {
  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setCount(8));

  auto graph = gpu().createGraph();

  auto node = graph.createPass("copy&readback");
  vector<float> arr = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
  MemView<float> view = makeMemView<float>(arr.data(), arr.size());
  auto dyn = gpu().dynamicBuffer<float>(view);
  node.copy(buffer, dyn);
  auto asyncReadback = node.readback(buffer);
  graph.addPass(std::move(node));

  gpu().submit(graph);
  gpu().waitGpuIdle();

  REQUIRE(asyncReadback->_Is_ready());

  // check actual data
  auto rb = asyncReadback->get();
  auto rbdata = rb.view<float>();
  for (int i = 0; i < 8; i++)
  {
    REQUIRE(arr[i] == rbdata[i]);
  }
}