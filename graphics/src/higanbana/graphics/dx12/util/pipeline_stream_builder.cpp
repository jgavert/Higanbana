#include "higanbana/graphics/dx12/util/pipeline_stream_builder.hpp"

namespace higanbana
{
namespace backend
{
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::stateFlags(D3D12_PIPELINE_STATE_FLAGS flags) {
  m_flags = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS, flags);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::nodeMask(UINT value){
  m_nodeMask = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::rootSignature(ID3D12RootSignature* root){
  m_rootSig = root;
  insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, m_rootSig);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::indexStripValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE value){
  m_stripValue = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE value){
  m_topology = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::streamOutput(D3D12_STREAM_OUTPUT_DESC value){
  m_streamOutput = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::blend(D3D12_BLEND_DESC value){
  m_blend = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::depthStencil(D3D12_DEPTH_STENCIL_DESC value){
  m_depth = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::depthStencil1(D3D12_DEPTH_STENCIL_DESC1 value){
  m_depth1 = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::depthFormat(DXGI_FORMAT value){
  m_depthFormat = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::rasterizer(D3D12_RASTERIZER_DESC value){
  m_rasterizer = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::rtFormats(D3D12_RT_FORMAT_ARRAY value){
  m_rtArray = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::dxgiSample(DXGI_SAMPLE_DESC value){
  m_dxgiSample = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::sampleMask(UINT value){
  m_sampleMask = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::cachedPipeline(D3D12_CACHED_PIPELINE_STATE value){
  m_cachedPipeline = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::viewInstancing(D3D12_VIEW_INSTANCING_DESC value){
  m_viewInstancing = insertToStream(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, value);
  return *this;
}
DX12PipelineStateStreamBuilder& DX12PipelineStateStreamBuilder::shader(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, D3D12_SHADER_BYTECODE value){
  shaders.push_back(ShaderModule{type, value});
  return *this;
}

D3D12_PIPELINE_STATE_STREAM_DESC DX12PipelineStateStreamBuilder::finalize(){
  for (auto&& shader : shaders) {
    insertToStream(shader.type, shader.bytecode);
  }
  D3D12_PIPELINE_STATE_STREAM_DESC desc{};
  desc.SizeInBytes = m_stream.size();
  desc.pPipelineStateSubobjectStream = m_stream.data();
  return desc;
}
}
}