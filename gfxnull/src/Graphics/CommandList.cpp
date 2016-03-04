#include "CommandList.hpp"

ComputeCmdBuffer::ComputeCmdBuffer(void* cmdList)
  :m_CommandList(cmdList), closed(false)
{
}

void ComputeCmdBuffer::setResourceBarrier()
{
}

void ComputeCmdBuffer::bindComputeBinding(ComputeBinding& )
{
}


void ComputeCmdBuffer::Dispatch(ComputeBinding& , unsigned int , unsigned int , unsigned int )
{
}

void ComputeCmdBuffer::DispatchIndirect(ComputeBinding& )
{
}

ComputeBinding ComputeCmdBuffer::bind(ComputePipeline& )
{
  return ComputeBinding();
}

bool ComputeCmdBuffer::isValid()
{
  return true;
}

void ComputeCmdBuffer::setHeaps(DescriptorHeapManager& )
{
}

void ComputeCmdBuffer::setSRVBindless(DescriptorHeapManager& )
{
}
void ComputeCmdBuffer::setUAVBindless(DescriptorHeapManager& )
{
}
void ComputeCmdBuffer::closeList()
{
	closed = true;
}
bool ComputeCmdBuffer::isClosed()
{
	return closed;
}

void ComputeCmdBuffer::resetList()
{
}

void ComputeCmdBuffer::CopyResource(Buffer&, Buffer&)
{
}

GraphicsBinding GraphicsCmdBuffer::bind(GraphicsPipeline& )
{
  return GraphicsBinding();
}

ComputeBinding GraphicsCmdBuffer::bind(ComputePipeline& )
{
  return ComputeBinding();
}

void GraphicsCmdBuffer::setViewPort(ViewPort& )
{
}

void GraphicsCmdBuffer::ClearRenderTargetView(TextureRTV& , faze::vec4 )
{
}


void GraphicsCmdBuffer::ClearDepthView(TextureDSV& )
{
}

void GraphicsCmdBuffer::bindGraphicsBinding(GraphicsBinding& )
{
}

void GraphicsCmdBuffer::ClearStencilView(TextureDSV& )
{
}

void GraphicsCmdBuffer::ClearDepthStencilView(TextureDSV& )
{
}

void GraphicsCmdBuffer::drawInstanced(GraphicsBinding& , unsigned int , unsigned int , unsigned int , unsigned int )
{
}

void GraphicsCmdBuffer::drawInstancedRaw(unsigned int , unsigned int , unsigned int , unsigned int )
{
}

void GraphicsCmdBuffer::preparePresent(TextureRTV& )
{
}

void GraphicsCmdBuffer::setRenderTarget(TextureRTV& )
{
}

void GraphicsCmdBuffer::setRenderTarget(TextureRTV& , TextureDSV& )
{
}

void GraphicsCmdBuffer::setSRVBindless(DescriptorHeapManager& )
{
}
void GraphicsCmdBuffer::setUAVBindless(DescriptorHeapManager& )
{
}

void GraphicsCmdBuffer::resetList()
{
}
