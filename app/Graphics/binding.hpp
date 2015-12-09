#pragma once
#include "Buffer.hpp"
#include "Texture.hpp"

#include <tuple>

class Binding_
{
  friend class GpuDevice;
	friend class GfxCommandList;
	friend class CptCommandList;

  friend class ComputeBinding;
  friend class GraphicsBinding;

  std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, int> m_descTableSRV;
  std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, int> m_descTableUAV;
  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_cbvs;
  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_srvs;
  std::vector<std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, UINT>> m_uavs;
  std::vector<std::pair<unsigned int, UINT>> m_rootConstants;
  std::vector<D3D12_RESOURCE_BARRIER>      m_resbars;

  using ShaderIndex = unsigned;

public:
  Binding_() {}
  enum class RootType
  {
    CBV,
    SRV,
    UAV,
	Num32
  };
  Binding_(std::vector<std::tuple<UINT, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount, int descriptorTableSRVSlot = -1, int descriptorTableUAVslot = -1)
  {
	D3D12_GPU_DESCRIPTOR_HANDLE h2;
	h2.ptr = 0;
	m_descTableSRV = std::make_pair(h2, descriptorTableSRVSlot);
	m_descTableUAV = std::make_pair(h2, descriptorTableUAVslot);
    auto fn = [](size_t count, auto& vec)
    {
      for (size_t i = 0; i < count;++i)
      {
        D3D12_GPU_DESCRIPTOR_HANDLE h;
        h.ptr = 0;
        vec.push_back(std::make_pair(h, 0));
      }
    };

	auto fn2 = [](size_t count, auto& vec)
	{
		for (size_t i = 0; i < count; ++i)
		{
			vec.push_back(std::make_pair(0, 0));
		}
	};

    fn(cbvCount, m_cbvs);
    fn(srvCount, m_srvs);
    fn(uavCount, m_uavs);

	int rootconstantCount = 0;
	for (auto&& it : input)
	{
		switch (std::get<1>(it))
		{
		case RootType::Num32:
			rootconstantCount++;
			break;
		default:
			break;
		}
	}
	fn2(rootconstantCount, m_rootConstants);

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
	  case RootType::Num32:
		m_rootConstants.at(std::get<2>(it)).second = std::get<0>(it);
		break;
      default:
        break;
      }
    }
  }

  void checkResourceStateUAV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state);
  void checkResourceStateSRV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state);
  void checkResourceStateCBV(ID3D12Resource* resptr, D3D12_RESOURCE_STATES& state);
  void UAV(unsigned int index, BufferUAV buf);
  //void UAV(unsigned int index, TextureUAV tex);
  void SRV(unsigned int index, BufferSRV buf);
  //void SRV(unsigned int index, TextureSRV tex);
  void CBV(unsigned int index, BufferCBV buf);

  void rootConstant(unsigned int index, unsigned int value);
};

class ComputeBinding : public Binding_
{
private:
  friend class GpuDevice;
  friend class GfxCommandList;
  friend class CptCommandList;

  ComputeBinding(std::vector<std::tuple<UINT, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount, int descriptorTableSRVSlot = -1, int descriptorTableUAVslot = -1) :
    Binding_(input, cbvCount, srvCount, uavCount, descriptorTableSRVSlot, descriptorTableUAVslot)
  {}
public:
  ComputeBinding():Binding_()
  {}
};

class GraphicsBinding : public Binding_
{
  friend class GpuDevice;
  friend class GfxCommandList;
  friend class CptCommandList;

  GraphicsBinding(std::vector<std::tuple<UINT, RootType, ShaderIndex>> input, unsigned int cbvCount, unsigned int srvCount, unsigned int uavCount, int descriptorTableSRVSlot = -1, int descriptorTableUAVslot = -1) :
    Binding_(input, cbvCount, srvCount, uavCount, descriptorTableSRVSlot, descriptorTableUAVslot)
  {}
public:
  GraphicsBinding() :Binding_()
  {}
};
