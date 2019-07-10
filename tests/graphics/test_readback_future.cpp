#include "tests/graphics/test_readback_future.hpp"

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
      i++;
      if (allFutures[i].get())
      {
        break;
      }
      provider.activateFuture();
    }
  }
  REQUIRE(i == 10);
}