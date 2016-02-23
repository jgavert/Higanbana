#include "binding.hpp"

void Binding_::checkResourceStateUAV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state)
{
  if (state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
  {
    D3D12_RESOURCE_BARRIER barrierDesc = {};
	  barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierDesc.Transition.pResource = resptr;
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.StateBefore = state;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    m_resbars.push_back(barrierDesc);
  }
}

void Binding_::checkResourceStateSRV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state)
{
  if (state != D3D12_RESOURCE_STATE_GENERIC_READ)
  {
    D3D12_RESOURCE_BARRIER barrierDesc = {};
	  barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = resptr;
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.StateBefore = state;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    state = D3D12_RESOURCE_STATE_GENERIC_READ;
    m_resbars.push_back(barrierDesc);
  }
}

void Binding_::checkResourceStateCBV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state)
{
  if (state != D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
  {
	  D3D12_RESOURCE_BARRIER barrierDesc = {};
	  barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = resptr;
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.StateBefore = state;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    m_resbars.push_back(barrierDesc);
  }
}


void Binding_::UAV(unsigned int index, BufferUAV& buf)
{
  checkResourceStateUAV(buf.buffer().m_resource.get(), buf.buffer().state);
  auto& ptr = m_uavs.at(index).first;
  //ptr = buf.buffer().view.getGpuHandle();
  ptr.ptr = buf.buffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::SRV(unsigned int index, BufferSRV& buf)
{
  checkResourceStateSRV(buf.buffer().m_resource.get(), buf.buffer().state);
  auto& ptr = m_srvs.at(index).first;
  //ptr = buf.buffer().view.getGpuHandle();
  ptr.ptr = buf.buffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::CBV(unsigned int index, BufferCBV& buf)
{
  checkResourceStateCBV(buf.buffer().m_resource.get(), buf.buffer().state);
  auto& ptr = m_cbvs.at(index).first;
  //ptr = buf.buffer().view.getGpuHandle();
  ptr.ptr = buf.buffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::UAV(unsigned int index, BufferNewUAV& buf)
{
  if (!buf.getBuffer().m_immutableState)
    checkResourceStateUAV(buf.getBuffer().m_resource.get(), buf.getBuffer().m_state);
  auto& ptr = m_uavs.at(index).first;
  ptr.ptr = buf.getBuffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::SRV(unsigned int index, BufferNewSRV& buf)
{
  if (!buf.getBuffer().m_immutableState)
    checkResourceStateSRV(buf.getBuffer().m_resource.get(), buf.getBuffer().m_state);
  auto& ptr = m_srvs.at(index).first;
  ptr.ptr = buf.getBuffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::CBV(unsigned int index, BufferNewCBV& buf)
{
  if (!buf.getBuffer().m_immutableState)
    checkResourceStateCBV(buf.getBuffer().m_resource.get(), buf.getBuffer().m_state);
  auto& ptr = m_cbvs.at(index).first;
  ptr.ptr = buf.getBuffer().m_resource->GetGPUVirtualAddress();
}

void Binding_::rootConstant(unsigned int index, unsigned int value)
{
  m_rootConstants.at(index).first = value;
}

