#include "CommandList.hpp"

CptCommandList::CptCommandList(void* cmdList)
  :m_CommandList(cmdList), closed(false)
{
}

void CptCommandList::setResourceBarrier()
{
}

void CptCommandList::bindComputeBinding(ComputeBinding& )
{
}


void CptCommandList::Dispatch(ComputeBinding& , unsigned int , unsigned int , unsigned int )
{
}

void CptCommandList::DispatchIndirect(ComputeBinding& )
{
}

ComputeBinding CptCommandList::bind(ComputePipeline& )
{
  return ComputeBinding();
}

bool CptCommandList::isValid()
{
  return true;
}

void CptCommandList::setHeaps(DescriptorHeapManager& )
{
}

void CptCommandList::setSRVBindless(DescriptorHeapManager& )
{
}
void CptCommandList::setUAVBindless(DescriptorHeapManager& )
{
}
void CptCommandList::closeList()
{
	closed = true;
}
bool CptCommandList::isClosed()
{
	return closed;
}

void CptCommandList::resetList()
{
}

void CptCommandList::CopyResource(Buffer&, Buffer&)
{
}

GraphicsBinding GfxCommandList::bind(GraphicsPipeline& )
{
  return GraphicsBinding();
}

ComputeBinding GfxCommandList::bind(ComputePipeline& )
{
  return ComputeBinding();
}

void GfxCommandList::setViewPort(ViewPort& )
{
}

void GfxCommandList::ClearRenderTargetView(TextureRTV& , faze::vec4 )
{
}


void GfxCommandList::ClearDepthView(TextureDSV& )
{
}

void GfxCommandList::bindGraphicsBinding(GraphicsBinding& )
{
}

void GfxCommandList::ClearStencilView(TextureDSV& )
{
}

void GfxCommandList::ClearDepthStencilView(TextureDSV& )
{
}

void GfxCommandList::drawInstanced(GraphicsBinding& , unsigned int , unsigned int , unsigned int , unsigned int )
{
}

void GfxCommandList::drawInstancedRaw(unsigned int , unsigned int , unsigned int , unsigned int )
{
}

void GfxCommandList::preparePresent(TextureRTV& )
{
}

void GfxCommandList::setRenderTarget(TextureRTV& )
{
}

void GfxCommandList::setRenderTarget(TextureRTV& , TextureDSV& )
{
}

void GfxCommandList::setSRVBindless(DescriptorHeapManager& )
{
}
void GfxCommandList::setUAVBindless(DescriptorHeapManager& )
{
}

void GfxCommandList::resetList()
{
}
