#include <higanbana/graphics/common/command_buffer.hpp>
#include <higanbana/graphics/common/command_packets.hpp>
#include <string>
#include <catch2/catch.hpp>

using namespace higanbana;
using namespace higanbana::backend;

TEST_CASE("insert 1") {
  CommandBuffer buffer(1024);
  std::string text = "testBlock";
  buffer.insert<gfxpacket::RenderBlock>(makeMemView(text));
  auto itr = buffer.begin();
  auto* header = (*itr);
  REQUIRE(header->type == PacketType::RenderBlock);
  itr++;
  header = (*itr);
  REQUIRE(header->type == PacketType::EndOfPackets);
  /*
  for (auto iter = buffer.begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
  {

  }
  */
}

TEST_CASE("insert 1 and copy") {
  CommandBuffer buffer(1024);
  std::string text = "testBlock";
  buffer.insert<gfxpacket::RenderBlock>(makeMemView(text));

  CommandBuffer buffer2 = buffer;

  auto itr = buffer2.begin();
  auto* header = (*itr);
  REQUIRE(header->type == PacketType::RenderBlock);
  itr++;
  header = (*itr);
  REQUIRE(header->type == PacketType::EndOfPackets);
}

TEST_CASE("insert 1 and resize") {
  CommandBuffer buffer(1);
  std::string text = "testBlock";
  buffer.insert<gfxpacket::RenderBlock>(makeMemView(text));

  auto itr = buffer.begin();
  auto* header = (*itr);
  REQUIRE(header->type == PacketType::RenderBlock);
  itr++;
  header = (*itr);
  REQUIRE(header->type == PacketType::EndOfPackets);
}

TEST_CASE("insert 10 resizes") {
  CommandBuffer buffer(6);
  std::string text = "testBlock";
  constexpr int pcount = 5;
  for (int i = 0; i < pcount; ++i)
  {
    buffer.insert<gfxpacket::RenderBlock>(makeMemView(text));

    auto itr = buffer.begin();
    auto* header = (*itr);
    for (int k = 0; k < i+1; ++k)
    {
      REQUIRE(header->type == PacketType::RenderBlock);
      itr++;
      header = (*itr);
    }
    REQUIRE(header->type == PacketType::EndOfPackets);
  }
}