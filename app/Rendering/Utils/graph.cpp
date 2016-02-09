#include "graph.hpp"

namespace rendering
{
  namespace utils
  {
    Graph::Graph(GpuDevice& device, float min, float max, faze::ivec2 graphSize)
      : currentUvX(0)
      , width(graphSize.x())
      , height(graphSize.y())
      , valueMin(min)
      , valueMax(max)
      , m_bottomright({ 0.5f, -0.5f })
      , m_topleft({-0.5f, 0.5f})
    {
      m_cmdPipeline = device.createComputePipeline(ComputePipelineDescriptor().shader("utils/graph_compute.hlsl"));
      // shouldnt need to know that its doing srgb, device should know.
	  m_drawPipeline = device.createGraphicsPipeline(GraphicsPipelineDescriptor()
		  .PixelShader("utils/pixel_drawGraph.hlsl")
		  .VertexShader("utils/vertex_drawGraph.hlsl")
		  .setRenderTargetCount(1)
		  .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB)
		  .DepthStencil(DepthStencilDescriptor().DepthEnable(false))
		  .Blend(GraphicsBlendDescriptor()
			  .setRenderTarget(0, RenderTargetBlendDescriptor()
				  .BlendEnable(true)
				  .setBlendOp(BlendOp::Add)
				  .BlendOpAlpha(BlendOp::Add)
				  .SrcBlend(Blend::SrcAlpha)
				  .DestBlend(Blend::InvSrcAlpha))));

      m_uploadConstants = device.createConstantBuffer(Dimension(1), Format<consts>(), ResUsage::Upload);
      m_graphConstants = device.createConstantBuffer(Dimension(1), Format<consts>(), ResUsage::Gpu);

      m_graphTexture = device.createTextureUAV(
        Dimension(width, height)
        , Format<int>(FormatType::R8G8B8A8_UNORM)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
    }
    void Graph::updateGraphCompute(GfxCommandList& gfx, float val)
    {
      GpuProfilingBracket(gfx, "Updating UtilGraph");
      {
        auto m = m_uploadConstants.buffer().Map<consts>();
        m[0].bottomright = m_bottomright;
        m[0].topleft = m_topleft;
        m[0].val = val;
        m[0].valueMin = valueMin;
        m[0].valueMax = valueMax;
        m[0].startUvX = currentUvX;
        m[0].width = width;
        m[0].height = height;
      }
      currentUvX += 1;
      currentUvX %= width;
      gfx.CopyResource(m_graphConstants.buffer(), m_uploadConstants.buffer());

      auto bind = gfx.bind(m_cmdPipeline);
      bind.CBV(0, m_graphConstants);
      bind.rootConstant(0, m_graphTexture.texture().shader_heap_index);
      unsigned int shaderGroup = 32;
      unsigned int graphSize = height;
      gfx.Dispatch(bind, graphSize / shaderGroup, 1, 1);
    }

    void Graph::changeScreenPos(faze::vec2 topleft, faze::vec2 bottomright)
    {
      m_bottomright = bottomright;
      m_topleft = topleft;
    }

    void Graph::drawGraph(GfxCommandList& gfx)
    {
      GpuProfilingBracket(gfx, "Drawing Utilgraph");
      auto bind = gfx.bind(m_drawPipeline);
      bind.CBV(0, m_graphConstants); // has the "vertex" data
      bind.rootConstant(0, m_graphTexture.texture().shader_heap_index);
      gfx.drawInstanced(bind, 6); // box
    }

    void Graph::changeMin(float min)
    {
      valueMin = min;
    }

    void Graph::changeMax(float max)
    {
      valueMax = max;
    }

    // call before drawing/compute, safeplace
    void Graph::changeGraphResolution(GpuDevice& device, faze::ivec2 graphSize)
    {
      width = graphSize.x();
      height = graphSize.y();
      m_graphTexture = device.createTextureUAV(
        Dimension(width, height)
        , Format<int>(FormatType::R8G8B8A8_UNORM_SRGB)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
    }
  };
};
