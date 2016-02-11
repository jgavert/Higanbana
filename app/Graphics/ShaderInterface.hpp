#pragma once
#include "ComPtr.hpp"
#include <d3d12.h>
#include <vector>

// my structs for the descriptor, pointers OUT vectors IN
// for the sake of my own sanity and NOW RAW POINTERS OR DELETES.
// Ill just get them wrong also they are const pointers in the structs.

struct descriptorTable
{
  UINT NumDescriptorRanges;
  std::vector<D3D12_DESCRIPTOR_RANGE> pDescriptorRanges;

  descriptorTable()
	  : NumDescriptorRanges(0)
  {
  }
};

struct rootParam
{
  D3D12_ROOT_PARAMETER_TYPE ParameterType;
  descriptorTable DescriptorTable;
  D3D12_ROOT_CONSTANTS Constants;
  D3D12_ROOT_DESCRIPTOR Descriptor;
  D3D12_SHADER_VISIBILITY ShaderVisibility;

  rootParam()
	  : ParameterType(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
	  , Constants({})
	  , Descriptor({})
	  , ShaderVisibility(D3D12_SHADER_VISIBILITY_ALL)
  {
  }
};

struct RootSignatureReflection
{
  UINT NumParameters;
  std::vector<rootParam> pParameters;
  UINT NumStaticSamplers;
  std::vector<D3D12_STATIC_SAMPLER_DESC> pStaticSamplers;
  D3D12_ROOT_SIGNATURE_FLAGS Flags;

  RootSignatureReflection()
	  : NumParameters(0)
	  , NumStaticSamplers(0)
	  , Flags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
  {
  }
};

class ShaderInterface 
{
private:
  friend class GpuDevice;
  friend class GpuCommandQueue;
  friend class GfxCommandList;
  friend class CptCommandList;
  friend class shaderUtils;

  ComPtr<ID3D12RootSignature> m_rootSig;
  RootSignatureReflection m_rootDesc;

  static RootSignatureReflection copyDesc(const D3D12_ROOT_SIGNATURE_DESC& desc)
  {
    RootSignatureReflection rootDesc;
    ZeroMemory(&rootDesc, sizeof(rootDesc));
    rootDesc.Flags = desc.Flags;
    rootDesc.NumParameters = desc.NumParameters;
    for (unsigned i = 0;i < desc.NumParameters;++i)
    {
      rootDesc.pParameters.push_back(rootParam());
    }
    for (unsigned int i = 0; i < desc.NumParameters;++i)
    {
      auto it = desc.pParameters[i];
      rootDesc.pParameters[i].ParameterType = it.ParameterType;
      rootDesc.pParameters[i].ShaderVisibility = it.ShaderVisibility;
      switch (it.ParameterType)
      {
      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        rootDesc.pParameters[i].DescriptorTable.NumDescriptorRanges = it.DescriptorTable.NumDescriptorRanges;
        for (unsigned k = 0; k < it.DescriptorTable.NumDescriptorRanges;++k)
        {
          rootDesc.pParameters[i].DescriptorTable.pDescriptorRanges.push_back(D3D12_DESCRIPTOR_RANGE());
        }
        for (unsigned k = 0; k < it.DescriptorTable.NumDescriptorRanges;++k)
        {
          rootDesc.pParameters[i].DescriptorTable.pDescriptorRanges[k] = it.DescriptorTable.pDescriptorRanges[k];
        }
        break;
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        rootDesc.pParameters[i].Constants = it.Constants;
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        rootDesc.pParameters[i].Descriptor = it.Descriptor;
      default:
        break;
      }
    }
    rootDesc.NumStaticSamplers = desc.NumStaticSamplers;
    for (unsigned i = 0;i < desc.NumStaticSamplers;++i)
    {
      rootDesc.pStaticSamplers.push_back(D3D12_STATIC_SAMPLER_DESC());
    }
    for (unsigned i = 0;i < desc.NumStaticSamplers;++i)
    {
      rootDesc.pStaticSamplers[i] = desc.pStaticSamplers[i];
    }
    return rootDesc;
  }

  ShaderInterface(ComPtr<ID3D12RootSignature> rootSig,const D3D12_ROOT_SIGNATURE_DESC& desc) : m_rootSig(rootSig), m_rootDesc(copyDesc(desc)){}

  bool isCopyOf(const D3D12_ROOT_SIGNATURE_DESC& compared)
  {
    if ( m_rootDesc.Flags != compared.Flags
      || m_rootDesc.NumParameters != compared.NumParameters
      || m_rootDesc.NumStaticSamplers != compared.NumStaticSamplers)
    {
      return false;
    }
    for (unsigned int i = 0; i < m_rootDesc.NumParameters;++i)
    {
      auto it = m_rootDesc.pParameters[i];
      auto co = compared.pParameters[i];
      if ( it.ParameterType != co.ParameterType
        || it.ShaderVisibility != co.ShaderVisibility)
      {
        return false;
      }

      switch (it.ParameterType)
      {
      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        if (it.DescriptorTable.NumDescriptorRanges != co.DescriptorTable.NumDescriptorRanges)
        {
          return false;
        }
        for (unsigned k = 0; k < it.DescriptorTable.NumDescriptorRanges; ++k)
        {
          auto& it2 = it.DescriptorTable.pDescriptorRanges[k];
          auto& co2 = co.DescriptorTable.pDescriptorRanges[k];
          if ( it2.BaseShaderRegister != co2.BaseShaderRegister
            || it2.NumDescriptors != co2.NumDescriptors
            || it2.OffsetInDescriptorsFromTableStart != co2.OffsetInDescriptorsFromTableStart
            || it2.RangeType != co2.RangeType
            || it2.RegisterSpace != co2.RegisterSpace)
          {
            return false;
          }
        }
        break;
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        if ( it.Constants.Num32BitValues != co.Constants.Num32BitValues
          || it.Constants.RegisterSpace != co.Constants.RegisterSpace
          || it.Constants.ShaderRegister != co.Constants.ShaderRegister)
        {
          return false;
        }
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        if ( it.Descriptor.RegisterSpace != co.Descriptor.RegisterSpace
          || it.Descriptor.ShaderRegister != co.Descriptor.ShaderRegister)
        {
          return false;
        }
        break;
      default:
        break;
      }
    }
    return true;
  }

  bool isCopyOf(const RootSignatureReflection& compared)
  {
    if (m_rootDesc.Flags != compared.Flags
      || m_rootDesc.NumParameters != compared.NumParameters
      || m_rootDesc.NumStaticSamplers != compared.NumStaticSamplers)
    {
      return false;
    }
    for (unsigned int i = 0; i < m_rootDesc.NumParameters;++i)
    {
      auto it = m_rootDesc.pParameters[i];
      auto co = compared.pParameters[i];
      if (it.ParameterType != co.ParameterType
        || it.ShaderVisibility != co.ShaderVisibility)
      {
        return false;
      }

      switch (it.ParameterType)
      {
      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        if (it.DescriptorTable.NumDescriptorRanges != co.DescriptorTable.NumDescriptorRanges)
        {
          return false;
        }
        for (unsigned k = 0; k < it.DescriptorTable.NumDescriptorRanges; ++k)
        {
          auto& it2 = it.DescriptorTable.pDescriptorRanges[k];
          auto& co2 = co.DescriptorTable.pDescriptorRanges[k];
          if (it2.BaseShaderRegister != co2.BaseShaderRegister
            || it2.NumDescriptors != co2.NumDescriptors
            || it2.OffsetInDescriptorsFromTableStart != co2.OffsetInDescriptorsFromTableStart
            || it2.RangeType != co2.RangeType
            || it2.RegisterSpace != co2.RegisterSpace)
          {
            return false;
          }
        }
        break;
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        if (it.Constants.Num32BitValues != co.Constants.Num32BitValues
          || it.Constants.RegisterSpace != co.Constants.RegisterSpace
          || it.Constants.ShaderRegister != co.Constants.ShaderRegister)
        {
          return false;
        }
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        if (it.Descriptor.RegisterSpace != co.Descriptor.RegisterSpace
          || it.Descriptor.ShaderRegister != co.Descriptor.ShaderRegister)
        {
          return false;
        }
        break;
      default:
        break;
      }
    }
    return true;
  }

public:
  ShaderInterface():m_rootSig(nullptr)
  {
  }

  bool operator!=(ShaderInterface& compared)
  {
    return m_rootSig.get() != compared.m_rootSig.get();
  }
  bool operator==(ShaderInterface& compared)
  {
    return m_rootSig.get() == compared.m_rootSig.get();
  }
  bool valid()
  {
    return m_rootSig.get() != nullptr;
  }
};

