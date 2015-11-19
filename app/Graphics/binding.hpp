#pragma once
#include "Buffer.hpp"
#include "Texture.hpp"

#include <tuple>

class Binding
{
  friend class GpuDevice;
	friend class GfxCommandList;
	friend class ComputeCommandList;
  
  enum class RootType
  {
    CBV,
    SRV,
    UAV
  };

  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_cbvs;
  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_srvs;
  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_uavs;
  std::vector<D3D12_RESOURCE_BARRIER>      m_resbars;

  using ShaderIndex = unsigned int;

  Binding(std::vector<std::tuple<UINT, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount)
  {
    auto fn = [](size_t count, auto& vec)
    {
      for (size_t i = 0; i < count;++i)
      {
        D3D12_GPU_DESCRIPTOR_HANDLE h;
        h.ptr = 0;
        vec.push_back(std::make_pair(h, 0));
      }
    };
    fn(cbvCount, m_cbvs);
    fn(srvCount, m_srvs);
    fn(uavCount, m_uavs);
    for (auto it : input)
    {
      switch (std::get<1>(it))
      {
      case RootType::CBV:
        m_cbvs.at(std::get<2>(it)).second = std::get<0>(it);
        break;
      case RootType::SRV:
        m_srvs.at(std::get<2>(it)).second = std::get<0>(it);
        break;
      case RootType::UAV:
        m_uavs.at(std::get<2>(it)).second = std::get<0>(it);
        break;
      default:
        break;
      }
    }
  }

  void checkResourceStateUAV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state);
  void checkResourceStateSRV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state);
public:
	void UAV(unsigned int index, BufferUAV buf);
	void UAV(unsigned int index, TextureUAV tex);
	void SRV(unsigned int index, BufferSRV buf);
	void SRV(unsigned int index, TextureSRV tex);
  void CBV(unsigned int index, BufferCBV buf);

};
