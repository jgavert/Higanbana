#pragma once
#include "app/Graphics/gfxApi.hpp"
#include "core/src/math/vec_templated.hpp"

namespace rendering
{
  namespace utils
  {
    class ShadedArea
    {
    private:

      struct consts // reflects hlsl
      {
        faze::vec2 topleft;
        faze::vec2 bottomright;
        int width;
        int height;
		unsigned texIndex;
        float empty;
      };

      ComputePipeline m_cmdPipeline;
      GraphicsPipeline m_drawPipeline;
      Buffer m_uploadConstants;
      Buffer m_graphConstants;
      BufferCBV m_graphConstantsCbv;
	  Texture m_graphTexture;
      TextureUAV m_graphTextureUav;
      TextureSRV m_graphTextureSrv;

      faze::vec2 m_topleft;
      faze::vec2 m_bottomright;
      int width;
      int height;
    public:
	  ShadedArea(GpuDevice& device, faze::ivec2 graphSize, std::string variation = "");
      void update(GfxCommandList& gfx);
      void render(GfxCommandList& gfx);
      void changeScreenPos(faze::vec2 topleft, faze::vec2 bottomright);
      void changeGraphResolution(GpuDevice& device, faze::ivec2 graphSize);
    };
  };
};
