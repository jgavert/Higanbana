#include <catch2/catch_all.hpp>
#include <higanbana/core/entity/bitfield.hpp>

using namespace higanbana;

TEST_CASE( "simple bitfield test" ) {
  DynamicBitfield bits;

  bits.setBit(0);
  bits.setBit(10);
  bits.setBit(64);
  bits.setBit(74);
  bits.setBit(75);

  int index = bits.findFirstSetBit(0);
  REQUIRE( index == 0);
  index = bits.findFirstSetBit(index+1);
  REQUIRE( index == 10);
  index = bits.findFirstSetBit(index+1);
  REQUIRE( index == 64);
  index = bits.findFirstSetBit(index+1);
  REQUIRE( index == 74);
  index = bits.findFirstSetBit(index+1);
  REQUIRE( index == 75);
}