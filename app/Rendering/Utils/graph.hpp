#pragma once
#include "Graphics/gfxApi.hpp"
#include "core/src/math/vec_templated.hpp"

namespace rendering
{
  namespace utils
  {
    class Graph
    {
    private:

      struct consts // reflects hlsl
      {
        faze::vec2 topleft;
        faze::vec2 bottomright;
        float val;
        float valueMin;
        float valueMax;
        int startUvX;
        int width;
        int height;
        faze::vec2 empty;
      };

      ComputePipeline m_cmdPipeline;
      GraphicsPipeline m_drawPipeline;
      BufferCBV m_graphConstants;
      BufferCBV m_uploadConstants;
      TextureUAV m_graphTexture;

      int currentUvX;
      int width;
      int height;
      float valueMin;
      float valueMax;
    public:
      Graph(GpuDevice& device, float min, float max, faze::ivec2 graphSize);
      void updateGraphCompute(GfxCommandList& gfx, float val, faze::vec2 topleft, faze::vec2 bottomright);
      void drawGraph(GfxCommandList& gfx);
      void changeMin(float min);
      void changeMax(float max);
      void changeGraphResolution(GpuDevice& device, faze::ivec2 graphSize);
    };
  };
};
