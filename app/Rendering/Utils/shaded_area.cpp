#include "shaded_area.hpp"

namespace rendering
{
  namespace utils
  {
    /*
    faze::vec2 m_topleft;
    faze::vec2 m_bottomright;
    int currentUvX;
    int width;
    int height;
    float valueMin;
    float valueMax;
    */
    ShadedArea::ShadedArea(GpuDevice& device, faze::ivec2 graphSize, std::string variation)
      : m_topleft({-0.5f, 0.5f})
      , m_bottomright({ 0.5f, -0.5f })
      , width(graphSize.x())
      , height(graphSize.y())
    {
      m_cmdPipeline = device.createComputePipeline(ComputePipelineDescriptor().shader("utils/shaded_compute" + variation));
      // shouldnt need to know that its doing srgb, device should know.
	  m_drawPipeline = device.createGraphicsPipeline(GraphicsPipelineDescriptor()
		  .PixelShader("utils/pixel_shaded")
		  .VertexShader("utils/vertex_shaded")
		  .setRenderTargetCount(1)
		  .RTVFormat(0, FormatType::R8G8B8A8_UNORM)
		  .DepthStencil(DepthStencilDescriptor().DepthEnable(false))
		  .Blend(GraphicsBlendDescriptor()
			  .setRenderTarget(0, RenderTargetBlendDescriptor()
				  .BlendEnable(true)
				  .setBlendOp(BlendOp::Add)
				  .BlendOpAlpha(BlendOp::Add)
				  .SrcBlend(Blend::SrcAlpha)
				  .DestBlend(Blend::InvSrcAlpha))));

	  m_uploadConstants = device.createBuffer(ResourceDescriptor()
		  .Format<consts>()
		  .Usage(ResourceUsage::UploadHeap));
	  m_graphConstants = device.createBuffer(ResourceDescriptor()
		  .Format<consts>());

	  m_graphConstantsCbv = device.createBufferCBV(m_graphConstants);

	  m_graphTexture = device.createTexture(ResourceDescriptor()
		  .Width(width)
		  .Height(height)
		  .Format(FormatType::R8G8B8A8_UNORM)
		  .enableUnorderedAccess()
		  .Dimension(FormatDimension::Texture2D));

	  m_graphTextureUav = device.createTextureUAV(m_graphTexture);
	  m_graphTextureSrv = device.createTextureSRV(m_graphTexture);

    }
    void ShadedArea::update(GfxCommandList& gfx)
    {
      GpuProfilingBracket(gfx, "Updating ShadedGraph");
      {
        auto m = m_uploadConstants.Map<consts>();
        m[0].bottomright = m_bottomright;
        m[0].topleft = m_topleft;
        m[0].width = width;
        m[0].height = height;
		m[0].texIndex = m_graphTextureUav.getCustomIndexInHeap();
      }
	  //F_LOG("updating graph with val %f\n", val);
      gfx.CopyResource(m_graphConstants, m_uploadConstants);
    }

    void ShadedArea::changeScreenPos(faze::vec2 topleft, faze::vec2 bottomright)
    {
      m_bottomright = bottomright;
      m_topleft = topleft;
    }

    void ShadedArea::render(GfxCommandList& gfx)
    {
      GpuProfilingBracket(gfx, "Drawing ShadedGraph");
	  {
		  auto bind = gfx.bind(m_cmdPipeline);
		  bind.CBV(0, m_graphConstantsCbv);
		  bind.rootConstant(0, m_graphTextureUav.getCustomIndexInHeap());
		  bind.barrier(m_graphTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_TYPE_UAV);
		  unsigned int shaderGroup = 8;
		  //unsigned int graphSize = height*width;
		  gfx.Dispatch(bind, width / shaderGroup, height / shaderGroup, 1);
	  }
	  {
		  auto bind = gfx.bind(m_drawPipeline);
		  bind.CBV(0, m_graphConstantsCbv); // has the "vertex" data
		  bind.rootConstant(0, m_graphTextureSrv.getCustomIndexInHeap());
		  bind.barrier(m_graphTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		  gfx.drawInstanced(bind, 6); // box
	  }
    }

    // call before drawing/compute, safeplace
    void ShadedArea::changeGraphResolution(GpuDevice& device, faze::ivec2 graphSize)
    {
      width = graphSize.x();
      height = graphSize.y();

	  m_graphTexture = device.createTexture(ResourceDescriptor()
		  .Width(width)
		  .Height(height)
		  .Format(FormatType::R8G8B8A8_UNORM)
		  .enableUnorderedAccess());

	  m_graphTextureUav = device.createTextureUAV(m_graphTexture);
    }
  };
};
