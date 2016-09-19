#ifdef FAZE_VULKAN

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define FAZE_PushConstants(USELESS) \
  layout(push_constant) uniform USELESS

#define FAZE_CBUFFER(cbufferType) \
	layout(set = 0, binding = 0, std430) buffer ConstantBlock \
  { \
    cbufferType fazeConstants[];\
  }
#define FAZE_BufferSRV(srvType, bufferType, srvName, srvSlot, USELESS) \
	layout(set = 0, binding = srvSlot, std430) srvType USELESS \
  { \
    bufferType srvName[]; \
  }
#define FAZE_BufferUAV(uavType, bufferType, uavName, uavSlot, USELESS) \
  layout(set = 0, binding = uavSlot, std430) uavType USELESS \
  { \
    bufferType uavName[]; \
  }
#define FAZE_TextureSRV(srvType, srvName, srvSlot) \
    layout(set = 0, binding = srvSlot) uniform srvType srvName; 
#define FAZE_TextureUAV(uavType, uavName, uavSlot) \
    layout(set = 0, binding = uavSlot) uniform uavType uavName;

#define FAZE_ExtractConstants(cbufferType, variable) \
    cbufferType variable = fazeConstants[0];

#define FAZE_DescriptorSetLayout(srvBufferStartIndex, uavBufferStartIndex, srvTextureStartIndex, uavTextureStartIndex)
#define FAZE_BEGIN_LAYOUT(layoutName)
#define FAZE_END_LAYOUT

#else
#ifdef __cplusplus

namespace faze
{
	namespace shader 
	{
		template <typename T> struct ShaderPushConstants {};
    template <unsigned Slot> struct ShaderCBuffer {};
    template <typename T, unsigned Slot> using ShaderSRVBuffer = unsigned;
    template <typename T, unsigned Slot> using ShaderUAVBuffer = unsigned;
		template <unsigned Slot> using ShaderSRVTexture = unsigned;
		template <unsigned Slot> using ShaderUAVTexture = unsigned;
    template <unsigned srvBufferStartIndex, unsigned uavBufferStartIndex, unsigned srvTextureStartIndex, unsigned uavTextureStartIndex> struct ShaderDescriptorSet {};
	}
}

#define FAZE_BEGIN_LAYOUT(signatureName) \
      struct signatureName {
#define FAZE_END_LAYOUT };

#define FAZE_PushConstants(USELESS) \
    struct pushConstants : ::faze::shader::ShaderPushConstants<pushConstants>

#define FAZE_CBUFFER(cbufferType) \
    struct ShaderConstants : ::faze::shader::ShaderCBuffer<0> \
    { \
      cbufferType constants;\
    };

#define FAZE_BufferSRV(srvType, bufferType, srvName, srvSlot, useless) \
    static constexpr ::faze::shader::ShaderSRVBuffer<bufferType, srvSlot> srvName = srvSlot;
#define FAZE_BufferUAV(uavType, bufferType, uavName, uavSlot, useless) \
    static constexpr ::faze::shader::ShaderUAVBuffer<bufferType, uavSlot> uavName = uavSlot;
#define FAZE_TextureSRV(srvType, srvName, srvSlot) \
    static constexpr ::faze::shader::ShaderSRVTexture<srvSlot> srvName = srvSlot;
#define FAZE_TextureUAV(uavType, uavName, uavSlot) \
    static constexpr ::faze::shader::ShaderUAVTexture<uavSlot> uavName = uavSlot;

#define FAZE_DescriptorSetLayout(srvBufferStartIndex, uavBufferStartIndex, srvTextureStartIndex, uavTextureStartIndex) \
    static constexpr ::faze::shader::ShaderDescriptorSet<srvBufferStartIndex, uavBufferStartIndex, srvTextureStartIndex, uavTextureStartIndex> shaderLayout{};

#endif
#endif