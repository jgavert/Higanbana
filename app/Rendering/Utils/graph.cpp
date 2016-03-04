#include "graph.hpp"

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
    Graph::Graph(GpuDevice& device, float min, float max, faze::ivec2 graphSize)
      : m_topleft({-0.5f, 0.5f})
      , m_bottomright({ 0.5f, -0.5f })
      , currentUvX(0)
      , width(graphSize.x())
      , height(graphSize.y())
      , valueMin(min)
      , valueMax(max)
    {
      m_cmdPipeline = device.createComputePipeline(ComputePipelineDescriptor().shader("utils/graph_compute"));
      // shouldnt need to know that its doing srgb, device should know.
	  m_drawPipeline = device.createGraphicsPipeline(GraphicsPipelineDescriptor()
		  .PixelShader("utils/pixel_drawGraph")
		  .VertexShader("utils/vertex_drawGraph")
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

    }
    void Graph::updateGraphCompute(GraphicsCmdBuffer& gfx, float val)
    {
      GpuProfilingBracket(gfx, "Updating UtilGraph");
      {
        auto m = m_uploadConstants.Map<consts>();
        m[0].bottomright = m_bottomright;
        m[0].topleft = m_topleft;
        m[0].val = val;
        m[0].valueMin = valueMin;
        m[0].valueMax = valueMax;
        m[0].startUvX = currentUvX;
        m[0].width = width;
        m[0].height = height;
		m[0].texIndex = m_graphTextureUav.getCustomIndexInHeap();
      }
      currentUvX += 1;
      currentUvX %= width;
      gfx.CopyResource(m_graphConstants, m_uploadConstants);
    }

    void Graph::changeScreenPos(faze::vec2 topleft, faze::vec2 bottomright)
    {
      m_bottomright = bottomright;
      m_topleft = topleft;
    }

    void Graph::drawGraph(GraphicsCmdBuffer& gfx)
    {
      GpuProfilingBracket(gfx, "Drawing Utilgraph");
	  {
		  auto bind = gfx.bind(m_cmdPipeline);
		  bind.CBV(0, m_graphConstantsCbv);
		  bind.rootConstant(0, m_graphTextureUav.getCustomIndexInHeap());
		  unsigned int shaderGroup = 32;
		  unsigned int graphSize = height;
		  gfx.Dispatch(bind, graphSize / shaderGroup, 1, 1);
	  }
	  {
		  auto bind = gfx.bind(m_drawPipeline);
		  bind.CBV(0, m_graphConstantsCbv); // has the "vertex" data
		  bind.rootConstant(0, m_graphTextureUav.getCustomIndexInHeap());
		  gfx.drawInstanced(bind, 6); // box
	  }
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

	  m_graphTexture = device.createTexture(ResourceDescriptor()
		  .Width(width)
		  .Height(height)
		  .Format(FormatType::R8G8B8A8_UNORM)
		  .enableUnorderedAccess());

	  m_graphTextureUav = device.createTextureUAV(m_graphTexture);
    }
  };
};
