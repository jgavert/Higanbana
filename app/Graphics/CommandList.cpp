#include "CommandList.hpp"

GfxCommandList::GfxCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList)
  :m_CommandList(cmdList)
{
}
void GfxCommandList::setViewPort(ViewPort view)
{
  m_CommandList->RSSetViewports(1, &view.getDesc());
}
void GfxCommandList::setResourceBarrier()
{

}

void GfxCommandList::CopyResource(Buffer& dstdata, Buffer& srcdata)
{
  D3D12_RESOURCE_BARRIER bD[2];
  size_t count = 0;
  if (dstdata.state & D3D12_RESOURCE_STATE_COPY_DEST == 0)
  {
    bD[count] = {};
    bD[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bD[count].Transition.pResource = dstdata.m_resource.get();
    bD[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    bD[count].Transition.StateBefore = dstdata.state;
    bD[count].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    ++count;
  }
  if (srcdata.state & D3D12_RESOURCE_STATE_COPY_SOURCE == 0)
  {
    bD[count] = {};
    bD[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bD[count].Transition.pResource = srcdata.m_resource.get();
    bD[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    bD[count].Transition.StateBefore = srcdata.state;
    bD[count].Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    ++count;
  }
  if (count > 0)
  {
    m_CommandList->ResourceBarrier(count, bD);
  }
  m_CommandList->CopyResource(dstdata.m_resource.get(), srcdata.m_resource.get());
}

void GfxCommandList::Dispatch(Binding asd, unsigned int x, unsigned int y, unsigned int z)
{

}

void GfxCommandList::DispatchIndirect(Binding bind)
{

}

Binding GfxCommandList::bind(GfxPipeline pipeline)
{
  return Binding();
}

Binding GfxCommandList::bind(ComputePipeline pipeline)
{
  return Binding();
}

bool GfxCommandList::isValid()
{
  return m_CommandList.get() != nullptr;
}


