#ifdef FAZE_VULKAN

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define FAZE_CBUFFER(cbufferName, cbufferId) \
	layout(set = 0, binding = cbufferId, std140) buffer cbufferName
#define FAZE_BufferSRV(srvType, srvName, srvSlot) \
	layout(set = 0, binding = srvSlot, std430) srvType srvName 
#define FAZE_BufferUAV(uavType, uavName, uavSlot) \
    layout(set = 0, binding = uavSlot, std430) uavType uavName 
#define FAZE_TextureSRV(srvType, srvName, srvSlot) \
    layout(set = 0, binding = srvSlot) uniform srvType srvName; 
#define FAZE_TextureUAV(uavType, uavName, uavSlot) \
    layout(set = 0, binding = uavSlot) uniform uavType uavName;

#define FAZE_BEGIN_LAYOUT(layoutName)
#define FAZE_END_LAYOUT

#else
#ifdef __cplusplus

namespace faze
{
	namespace shader 
	{
		template <typename T, unsigned Slot> struct ShaderCBuffer {};
		template <typename T, unsigned Slot> struct ShaderSRVBuffer {};
		template <typename T, unsigned Slot> struct ShaderUAVBuffer {};
		template <unsigned Slot> using ShaderSRVTexture = unsigned;
		template <unsigned Slot> using ShaderUAVTexture = unsigned;
	}
}

#define FAZE_BEGIN_LAYOUT(signatureName) struct signatureName {
#define FAZE_END_LAYOUT };

#define FAZE_CBUFFER(cbufferName, cbufferId) \
    struct cbufferName : ::faze::shader::ShaderCBuffer<cbufferName, cbufferId>

#define FAZE_BufferSRV(srvType, srvName, srvSlot) \
    struct srvName : ::faze::shader::ShaderSRVBuffer<srvName, srvSlot>
#define FAZE_BufferUAV(uavType, uavName, uavSlot) \
    struct uavName : ::faze::shader::ShaderUAVBuffer<uavName, uavSlot>
#define FAZE_TextureSRV(srvType, srvName, srvSlot) \
    static const ::faze::shader::ShaderSRVTexture<srvSlot> srvName = srvSlot;
#define FAZE_TextureUAV(uavType, uavName, uavSlot) \
    static const ::faze::shader::ShaderUAVTexture<uavSlot> uavName = uavSlot;
/*
layout(std140, binding = 0) buffer DataIn
{
	float a[];
} dataIn;

layout(std430, binding = 1) buffer DataOut
{
	float a[];
} dataOut;

struct Super
{

} super;
*/
#endif
#endif