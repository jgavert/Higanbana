#pragma once

#include "higanbana/core/math/math.hpp"

namespace higanbana {
namespace ranges {

inline int number_spiral_helper(const int2& center, int2 input) {
  auto pos = sub(input, center);
  int len = std::max(std::abs(pos.x), std::abs(pos.y));
  int sideLength = len * 2 + 1;
  int ringMaxValue = pow(sideLength,2);
  int res = ringMaxValue-1;
  {
    if (pos.y == -len) {
      // top
      auto offset = sideLength / 2 + pos.x;
      res -= offset;
    }
    else if (pos.y == len) { 
      // bottom
      auto sizeX = sideLength;
      auto sizeZ = sideLength - 2;
      auto offset = sideLength / 2 - pos.x;
      res -= sizeX + sizeZ + offset;
    }
    else if (pos.x == len) {
      // right
      auto sizeX = sideLength;
      auto offset = sideLength / 2 + pos.y -1;
      res -= sizeX + offset;
    }
    else if (pos.x == -len) {
      // left
      auto sizeX = sideLength;
      auto sizeZ = sideLength - 2;
      auto offset = sideLength / 2 - pos.y - 1;
      res -= sizeX * 2 + sizeZ + offset;
    }
  }
  //HIGAN_LOGi("(%2d, %2d)", pos.x, pos.y);
  return res;
}

inline int numberSpiral(const int2& totalSize, int val) {
  int vx = val%totalSize.x - totalSize.x / 2;
  int vy = val/totalSize.x - totalSize.y / 2;
  //HIGAN_LOGi("\t %2d -> ", val);
  //HIGAN_LOGi("(%2d, %2d)", val%totalSize.x, val/totalSize.x);
  auto res = number_spiral_helper(int2(0,0), {vx, vy});
  return res;
}

inline int2 number_spiral_reverse_helper(const int2& center, int input) {
  //HIGAN_LOGi(" => %2d", input);

  int res = sqrt(input);
  int ring = (res+1) / 2;
  int sideLength = ring * 2 + 1;
  int ringMaxValue = pow(sideLength,2);
  int circumference = sideLength*4-4;
  res = ringMaxValue;
  int2 pos;
  if (input < ringMaxValue - sideLength*3 + 2) {
    // left
    auto offset = (ringMaxValue - sideLength*3 + 2) - input;
    pos.x = -ring;
    pos.y = sideLength - offset - sideLength / 2 - 1;
  } else if (input < ringMaxValue - sideLength*2 + 2) {
    // bottom
    auto offset = (ringMaxValue - sideLength*2 + 2) - input;
    pos.x = sideLength - offset - sideLength/2;
    pos.y = ring;
  } else if (input < ringMaxValue - sideLength) {
    // right
    auto offset = ringMaxValue - sideLength - input;
    pos.x = ring;
    pos.y = offset - sideLength/2;
  } else if (input < ringMaxValue) {
    // top
    auto offset = ringMaxValue - input;
    pos.y = -ring;
    pos.x = offset - sideLength/2-1;
  }
  //HIGAN_LOGi(" == (%2d, %2d)", pos.x, pos.y);
  return pos;
}

inline int numberSpiralReverse(int2 totalSize, int val) {
  int2 size = div(totalSize, 2);
  int2 pos = number_spiral_reverse_helper({0,0}, val);
  int2 newPos = add(pos, size);
  int res = newPos.y * totalSize.x + newPos.x;
  //HIGAN_LOGi("(%2d, %2d)=>%2d\t||", newPos.x, newPos.y, res);
  return res;
}
  /*
  int2 totalSize = {5,2};
  int2 size = div(totalSize, 2);

  for (int y = -size.y; y < size.y; y++) {
    for (int x = -size.x; x < size.x; x++) {
      auto res = remap(int2(0,0), {x,y});
    }
    HIGAN_LOGi("\n");
  }
  HIGAN_LOGi("\n");
  for (int y = 0; y < totalSize.y; y++) {
    for (int x = 0; x < totalSize.x; x++) {
      int val = y*totalSize.x+x;
      HIGAN_LOGi("\t%2d=>",val);
      auto res = remap2(totalSize, val);
      HIGAN_LOGi("=>%2d", res);
      auto lol = remapReverse2(totalSize, res);
    }
    HIGAN_LOGi("\n");
  }
  HIGAN_LOGi("\n");

  int tiles = totalSize.y*totalSize.x;
  int i = 0;
  while (tiles > 0) {
    HIGAN_LOGi("\t%2d ->", i);
    auto lol = remapReverse2(totalSize, i);
    i++;
    if (lol >= totalSize.y*totalSize.x || lol < 0) {
      HIGAN_LOGi("fail \n");
      continue;
    }
    --tiles;
    HIGAN_LOGi("success \n");
  }
  */
}
}