#include <higanbana/graphics/common/command_buffer.hpp>
#include <higanbana/graphics/common/command_packets.hpp>
#include <string>
#include <catch2/catch.hpp>

using namespace higanbana;
using namespace higanbana::backend;

TEST_CASE("basic test") {
  CommandBuffer buffer(1024);
  std::string text = "testBlock";
  buffer.insert<gfxpacket::RenderBlock>(makeMemView(text));
  auto itr = buffer.begin();
  auto* header = (*itr);
  REQUIRE(header->type == PacketType::RenderBlock);
  /*
  for (auto iter = buffer.begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
  {

  }
  */
}