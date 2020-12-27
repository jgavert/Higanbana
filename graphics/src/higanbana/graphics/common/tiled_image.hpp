#pragma once

#include "higanbana/graphics/common/resource_descriptor.hpp"
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
struct TileView{
  MemView<uint8_t> pixels;
  size_t pixelSize;
  uint2 offset;
  uint2 size;

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
  };
  uint2 m_size;
  uint2 m_tileSize;
  size_t m_pixelSize;
  vector<Tile> m_tiles;

  public:
  TiledImage(): m_size(uint2(0,0)), m_pixelSize(0){}
  TiledImage(uint2 size, const uint2 tileSize, FormatType format)
    : m_size(size)
    , m_tileSize(tileSize)
    , m_pixelSize(formatSizeInfo(format).pixelSize)
  {
    uint2 pos = uint2(0,0);
    while(pos.y < m_size.y && pos.x < m_size.x) {
      uint2 ctileSize = add(pos, m_tileSize);
      ctileSize.x = ctileSize.x > m_size.x ? m_size.x : ctileSize.x;
      ctileSize.y = ctileSize.y > m_size.y ? m_size.y : ctileSize.y;
      ctileSize.x -= pos.x;
      ctileSize.y -= pos.y;
      if (ctileSize.x != 0 && ctileSize.y != 0)
        m_tiles.push_back(Tile{vector<uint8_t>(m_pixelSize*ctileSize.x*ctileSize.y, 0), pos, ctileSize});

      if (pos.x+ctileSize.x < m_size.x) {
        pos.x += ctileSize.x;
      } else if (pos.y+ctileSize.y < m_size.y){
        pos.y += ctileSize.y;
        pos.x = 0;
      } else {
        break;
      }
    }
    HIGAN_LOGi("made %zd tiles\n", m_tiles.size());
  }

  TileView tile(size_t idx) {
    auto&& tile = m_tiles[idx];
    return TileView{MemView<uint8_t>(tile.m_pixels.data(), tile.m_pixels.size()), m_pixelSize, tile.offset, tile.tileSize};
  }

  size_t size() const {
    return m_tiles.size();
  }

  uint2 tileSize() const {
    return m_tileSize;
  }
};
}