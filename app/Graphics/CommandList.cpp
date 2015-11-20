#include "CommandList.hpp"

CptCommandList::CptCommandList(ComPtr<ID3D12GraphicsCommandList> cmdList)
  :m_CommandList(cmdList)
{
}
void CptCommandList::setViewPort(ViewPort view)
{
  m_CommandList->RSSetViewports(1, &view.getDesc());
}

void CptCommandList::ClearRenderTargetView(TextureRTV rtv, faze::vec4 color)
{
  m_CommandList->ClearRenderTargetView(rtv.texture().view.getCpuHandle(), color.data.data(), NULL, 0);
}

void CptCommandList::setResourceBarrier()
{

}

void CptCommandList::CopyResource(Buffer& dstdata, Buffer& srcdata)
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
    bD[count].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    ++count;
  }
  if (count > 0)
  {
    m_CommandList->ResourceBarrier(count, bD);
  }
  m_CommandList->CopyResource(dstdata.m_resource.get(), srcdata.m_resource.get());
}

void CptCommandList::Dispatch(ComputeBinding asd, unsigned int x, unsigned int y, unsigned int z)
{
  if (asd.m_resbars.size() > 0)
    m_CommandList->ResourceBarrier(asd.m_resbars.size(), asd.m_resbars.data());
  for (size_t i = 0; i < asd.m_cbvs.size(); ++i)
  {
    if (asd.m_cbvs[i].first.ptr != 0)
      m_CommandList->SetComputeRootConstantBufferView(asd.m_cbvs[i].second, asd.m_cbvs[i].first.ptr);
  }
  for (size_t i = 0; i < asd.m_srvs.size(); ++i)
  {
    if (asd.m_srvs[i].first.ptr != 0)
      m_CommandList->SetComputeRootShaderResourceView(asd.m_srvs[i].second, asd.m_srvs[i].first.ptr);
  }
  for (size_t i = 0; i < asd.m_uavs.size(); ++i)
  {
    if (asd.m_uavs[i].first.ptr != 0)
      m_CommandList->SetComputeRootUnorderedAccessView(asd.m_uavs[i].second, asd.m_uavs[i].first.ptr);
  }
  m_CommandList->Dispatch(x, y, z);
}

void CptCommandList::DispatchIndirect(ComputeBinding bind)
{

}


ComputeBinding CptCommandList::bind(ComputePipeline& pipeline)
{
  m_CommandList->SetComputeRootSignature(pipeline.getRootSig());
  m_CommandList->SetPipelineState(pipeline.getState());
  m_CommandList->SetDescriptorHeaps(1, pipeline.getDescHeap());
  return pipeline.getBinding();
}

bool CptCommandList::isValid()
{
  return m_CommandList.get() != nullptr;
}


GraphicsBinding GfxCommandList::bind(GraphicsPipeline& pipeline)
{
  m_CommandList->SetGraphicsRootSignature(pipeline.getRootSig());
  m_CommandList->SetPipelineState(pipeline.getState());
  auto* heap = pipeline.getDescHeap();
  m_CommandList->SetDescriptorHeaps(1, &heap);
  return pipeline.getBinding();
}
