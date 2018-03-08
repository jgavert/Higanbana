#ifndef FAZE_SHADER_DEFINITIONS
#define FAZE_SHADER_DEFINITIONS

#ifdef __cplusplus

#include "core/src/math/math.hpp"

#define FAZE_BEGIN_LAYOUT(signatureName) \
namespace shader\
{ \
      struct signatureName {
#define FAZE_END_LAYOUT \
        static Constants constants; \
      }; \
};

#define FAZE_CBUFFER \
    struct Constants

#define FAZE_SRV(type, name, index) \
	static constexpr int name = index;

#define FAZE_UAV(type, name, index) \
	static constexpr int name = index;

#define FAZE_SRV_TABLE(size) \
  static constexpr int srv = size;

#define FAZE_UAV_TABLE(size) \
  static constexpr int uav = size;

#define FAZE_SAMPLER_BILINEAR(name)
#define FAZE_SAMPLER_POINT(name)
#define FAZE_SAMPLER_BILINEAR_WRAP(name)
#define FAZE_SAMPLER_POINT_WRAP(name)

#else /*__cplusplus*/

#define FAZE_BEGIN_LAYOUT(signatureName)
#define FAZE_END_LAYOUT

#define FAZE_CBUFFER \
    [[vk::binding(0, 0)]] cbuffer _constants : register( b0, space0 )

#define FAZE_SRV(type, name, index) \
	[[vk::binding(index, 1)]] type name : register(t##index, space0);

#define FAZE_UAV(type, name, index) \
	[[vk::binding(index, 2)]] type name : register(u##index, space0);

#define FAZE_SRV_TABLE(size)
#define FAZE_UAV_TABLE(size)

#define FAZE_SAMPLER_BILINEAR(name) [[vk::binding(1, 0)]] SamplerState name : register( s0, space0);
#define FAZE_SAMPLER_POINT(name) [[vk::binding(2, 0)]] SamplerState name : register( s1, space0 );
#define FAZE_SAMPLER_BILINEAR_WRAP(name) [[vk::binding(3, 0)]] SamplerState name : register( s2, space0 );
#define FAZE_SAMPLER_POINT_WRAP(name) [[vk::binding(4, 0)]] SamplerState name : register( s3, space0 );

#ifdef FAZE_DX12
#define ROOTSIG "RootFlags(0), " \
        "CBV(b0), " \
        "DescriptorTable( SRV(t0, numDescriptors = 32)), " \
        "DescriptorTable( UAV(u0, numDescriptors = 8)), " \
        "StaticSampler(s0, " \
			"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), " \
		"StaticSampler(s1, " \
			"filter = FILTER_MIN_MAG_MIP_POINT), " \
		"StaticSampler(s2, " \
			"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, " \
			"addressU = TEXTURE_ADDRESS_WRAP, " \
			"addressV = TEXTURE_ADDRESS_WRAP, " \
			"addressW = TEXTURE_ADDRESS_WRAP), " \
		"StaticSampler(s3, " \
			"filter = FILTER_MIN_MAG_MIP_POINT, " \
			"addressU = TEXTURE_ADDRESS_WRAP, " \
			"addressV = TEXTURE_ADDRESS_WRAP, " \
			"addressW = TEXTURE_ADDRESS_WRAP) "

#define GX_SIGNATURE [RootSignature(ROOTSIG)]
#define CS_SIGNATURE [RootSignature(ROOTSIG)] \
	[numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)]

#else /*FAZE_DX12*/

#define GX_SIGNATURE
#define CS_SIGNATURE [numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)]

#endif /*FAZE_DX12*/
#endif /*__cplusplus*/
#endif /*FAZE_SHADER_DEFINITIONS*/