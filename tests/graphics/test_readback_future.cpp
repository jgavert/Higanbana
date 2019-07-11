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

TEST_CASE_METHOD(GraphicsFixture, "test copying&readback: small copies") {
  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setCount(8));

  auto graph = gpu().createGraph();

  auto node = graph.createPass("copy&readback");
  vector<float> arr = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
  MemView<float> view = makeMemView<float>(arr.data(), 2);
  auto dyn = gpu().dynamicBuffer<float>(view);
  MemView<float> view2 = makeMemView<float>(arr.data()+2, 2);
  auto dyn2 = gpu().dynamicBuffer<float>(view2);
  MemView<float> view3 = makeMemView<float>(arr.data()+4, 2);
  auto dyn3 = gpu().dynamicBuffer<float>(view3);
  MemView<float> view4 = makeMemView<float>(arr.data()+6, 2);
  auto dyn4 = gpu().dynamicBuffer<float>(view4);
  node.copy(buffer, dyn);
  node.copy(buffer, 2, dyn2);
  node.copy(buffer, 4, dyn3);
  node.copy(buffer, 6, dyn4);
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

TEST_CASE_METHOD(GraphicsFixture, "test copying&readback: small readbacks") {
  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setCount(8));

  auto graph = gpu().createGraph();

  auto node = graph.createPass("copy&readback");
  vector<float> arr = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
  MemView<float> view = makeMemView<float>(arr.data(), arr.size());
  auto dyn = gpu().dynamicBuffer<float>(view);
  node.copy(buffer, dyn);
  auto ar1 = node.readback(buffer, 0, 2);
  auto ar2 = node.readback(buffer, 2, 2);
  auto ar3 = node.readback(buffer, 4, 2);
  auto ar4 = node.readback(buffer, 6, 2);
  graph.addPass(std::move(node));

  gpu().submit(graph);
  gpu().waitGpuIdle();

  REQUIRE(ar1->_Is_ready());
  REQUIRE(ar2->_Is_ready());
  REQUIRE(ar3->_Is_ready());
  REQUIRE(ar4->_Is_ready());

  // check actual data
  int sarr = 0;
  auto checkReadback = [&](MemView<float> rb)
  {
    for (int i = 0; i < 2; i++)
    {
      REQUIRE(arr[sarr] == rb[i]);
      sarr++;
    }
  };
  auto rb = ar1->get();
  auto rbdata = rb.view<float>();

  auto rb2 = ar2->get();
  auto rbdata2 = rb2.view<float>();
  auto rb3 = ar3->get();
  auto rbdata3 = rb3.view<float>();
  auto rb4 = ar4->get();
  auto rbdata4 = rb4.view<float>();

  checkReadback(rbdata);
  checkReadback(rbdata2);
  checkReadback(rbdata3);
  checkReadback(rbdata4);
}