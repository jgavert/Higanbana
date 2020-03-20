#pragma once
#include <higanbana/core/platform/definitions.hpp>
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/dx12.hpp"
#include "higanbana/graphics/common/pipeline_descriptor.hpp"

namespace higanbana
{
namespace backend
{
class DX12PipelineStateStreamBuilder
{
  vector<uint8_t> m_stream;
  D3D12_PIPELINE_STATE_FLAGS m_flags = {};
  uint m_nodeMask = {};
  ID3D12RootSignature* m_rootSig = nullptr;
  // no input layout
  D3D12_INDEX_BUFFER_STRIP_CUT_VALUE m_stripValue = {};
  D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topology = {};
  D3D12_STREAM_OUTPUT_DESC m_streamOutput = {};
  D3D12_BLEND_DESC m_blend = {};
  D3D12_DEPTH_STENCIL_DESC m_depth = {};
  D3D12_DEPTH_STENCIL_DESC1 m_depth1 = {};
  DXGI_FORMAT m_depthFormat = {};
  D3D12_RASTERIZER_DESC m_rasterizer = {};
  D3D12_RT_FORMAT_ARRAY m_rtArray = {};
  DXGI_SAMPLE_DESC m_dxgiSample = {};
  UINT m_sampleMask = {};
  D3D12_CACHED_PIPELINE_STATE m_cachedPipeline = {};
  D3D12_VIEW_INSTANCING_DESC m_viewInstancing = {};
  struct ShaderModule {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type;
    D3D12_SHADER_BYTECODE bytecode;
  };
  vector<ShaderModule> shaders;

  template <typename InnerStructType>
  struct alignas(void*) STATE_STREAM_SUBOBJECT
  {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _Type;
    InnerStructType _Inner;
  };

  template <typename T> 
  T insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, T value) {
    STATE_STREAM_SUBOBJECT<T> obj{type, value};
    auto structSize = sizeof(obj);
    auto oldEnd = m_stream.size();
    auto newSize = m_stream.size() + structSize;
    m_stream.resize(newSize);
    memcpy(m_stream.data() + oldEnd, &obj, sizeof(obj));
    return value;
  }
  public:
  DX12PipelineStateStreamBuilder() {}

  DX12PipelineStateStreamBuilder& stateFlags(D3D12_PIPELINE_STATE_FLAGS flags);
  DX12PipelineStateStreamBuilder& nodeMask(UINT value);
  DX12PipelineStateStreamBuilder& rootSignature(ID3D12RootSignature* root);
  DX12PipelineStateStreamBuilder& indexStripValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE value);
  DX12PipelineStateStreamBuilder& topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE value);
  DX12PipelineStateStreamBuilder& streamOutput(D3D12_STREAM_OUTPUT_DESC value);
  DX12PipelineStateStreamBuilder& blend(D3D12_BLEND_DESC value);
  DX12PipelineStateStreamBuilder& depthStencil(D3D12_DEPTH_STENCIL_DESC value);
  DX12PipelineStateStreamBuilder& depthStencil1(D3D12_DEPTH_STENCIL_DESC1 value);
  DX12PipelineStateStreamBuilder& depthFormat(DXGI_FORMAT value);
  DX12PipelineStateStreamBuilder& rasterizer(D3D12_RASTERIZER_DESC value);
  DX12PipelineStateStreamBuilder& rtFormats(D3D12_RT_FORMAT_ARRAY value);
  DX12PipelineStateStreamBuilder& dxgiSample(DXGI_SAMPLE_DESC value);
  DX12PipelineStateStreamBuilder& sampleMask(UINT value);
  DX12PipelineStateStreamBuilder& cachedPipeline(D3D12_CACHED_PIPELINE_STATE value);
  DX12PipelineStateStreamBuilder& viewInstancing(D3D12_VIEW_INSTANCING_DESC value);
  DX12PipelineStateStreamBuilder& shader(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, D3D12_SHADER_BYTECODE value);
  
  D3D12_PIPELINE_STATE_STREAM_DESC finalize();
};
}
}
#endif