#pragma once
#include <core/platform/definitions.hpp>
#include <graphics/vk/vulkan.hpp>
#include <graphics/common/pipeline_descriptor.hpp>

namespace faze
{
  namespace backend
  {
    // blend things
    vk::BlendFactor convertBlend(Blend b);
    vk::BlendOp convertBlendOp(BlendOp op);
    vk::LogicOp convertLogicOp(LogicOp op);
    vk::ColorComponentFlags convertColorWriteEnable(unsigned colorWrite);

    // raster things
    vk::PolygonMode convertFillMode(FillMode mode);
    vk::CullModeFlags convertCullMode(CullMode mode);
    vk::ConservativeRasterizationModeEXT convertConservativeRasterization(ConservativeRasterization mode);

    // depthstencil
    //D3D12_DEPTH_WRITE_MASK convertDepthWriteMask(DepthWriteMask mask);
    vk::CompareOp convertComparisonFunc(ComparisonFunc func);
    vk::StencilOp convertStencilOp(StencilOp op);

    // misc
    vk::PrimitiveTopology convertPrimitiveTopology(PrimitiveTopology topology);
    vk::SampleCountFlags convertSamplecount(MultisampleCount count);

    // pipeline parts
    vector<vk::PipelineColorBlendAttachmentState> getBlendAttachments(const BlendDescriptor& desc);
    vk::PipelineColorBlendStateCreateInfo getBlendStateDesc(const BlendDescriptor& desc, const vector<vk::PipelineColorBlendAttachmentState>& attachments);

    vk::PipelineRasterizationStateCreateInfo getRasterStateDesc(const RasterizerDescriptor& desc);
    vk::PipelineDepthStencilStateCreateInfo getDepthStencilDesc(const DepthStencilDescriptor& desc);
    vk::PipelineInputAssemblyStateCreateInfo getInputAssemblyDesc(const GraphicsPipelineDescriptor& desc);
    vk::PipelineMultisampleStateCreateInfo getMultisampleDesc(const GraphicsPipelineDescriptor& desc);
  }
}