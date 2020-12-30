#pragma once

#include "higanbana/graphics/common/resource_descriptor.hpp"
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/ranges/rectangle.hpp>
#include <higanbana/core/ranges/number_spiral.hpp>
#include <higanbana/core/global_debug.hpp>

#include <cmath>

namespace higanbana
{
struct TileView{
  MemView<uint8_t> pixels;
  size_t pixelSize;
  uint2 offset;
  uint2 size;
  size_t* iterations;

  template <typename Pixel>
  Pixel load(uint2 pos) const {
    HIGAN_ASSERT(sizeof(Pixel) == pixelSize, "should be same size");
    auto idx = pos.y*size.x+pos.x;
    Pixel px;
    memcpy(&px, &pixels[idx*sizeof(Pixel)], sizeof(Pixel));
    return px;
  }

  template <typename Pixel>
  inline void save(uint2 pos, Pixel data) {
    HIGAN_ASSERT(sizeof(Pixel) == pixelSize, "should be same size");
    auto idx = pos.y*size.x+pos.x;
    memcpy(&pixels[idx*sizeof(Pixel)], &data, sizeof(Pixel));
  }
};

class TiledImage
{
  struct Tile
  {
    vector<uint8_t> m_pixels;
    uint2 offset;
    uint2 tileSize;
    size_t iterations;
  };
  uint2 m_size;
  uint2 m_tileSize;
  size_t m_pixelSize;
  vector<Tile> m_tiles;
  vector<size_t> m_tileRemap;

  public:
  TiledImage(): m_size(uint2(0,0)), m_pixelSize(0){}
  TiledImage(uint2 size, const uint2 tileSize, FormatType format)
    : m_size(size)
    , m_tileSize(tileSize)
    , m_pixelSize(formatSizeInfo(format).pixelSize)
  {
    for (auto rect : ranges::Range2D(size, tileSize)) {
      uint2 ctileSize = rect.size();
      m_tiles.push_back(Tile{vector<uint8_t>(m_pixelSize*ctileSize.x*ctileSize.y, 0), rect.leftTop, ctileSize, 0});
    }
    HIGAN_LOGi("made %zd tiles\n", m_tiles.size());

    int tiles = m_tiles.size();
    int2 ssize = int2(std::round(size.x / double(tileSize.x)), std::round(size.y / double(tileSize.y)));
    int i = 0;
    while (tiles > 0) {
      auto lol = ranges::numberSpiralReverse(ssize, i);
      i++;
      if (lol >= m_tiles.size() || lol < 0) {
        continue;
      }
      --tiles;
      m_tileRemap.push_back(lol);
    }
  }

  TileView tile(size_t idx) {
    auto&& tile = m_tiles[idx];
    return TileView{MemView<uint8_t>(tile.m_pixels.data(), tile.m_pixels.size()), m_pixelSize, tile.offset, tile.tileSize, &tile.iterations};
  }

  TileView tileRemap(size_t idx) {
    auto&& tile = m_tiles.at(m_tileRemap[idx]);
    return TileView{MemView<uint8_t>(tile.m_pixels.data(), tile.m_pixels.size()), m_pixelSize, tile.offset, tile.tileSize, &tile.iterations};
  }

  size_t size() const {
    return m_tiles.size();
  }

  uint2 tileSize() const {
    return m_tileSize;
  }
};
}