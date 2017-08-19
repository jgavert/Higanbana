#ifndef FAZE_SHADER_DEFINITIONS
#define FAZE_SHADER_DEFINITIONS

#ifdef __cplusplus

#include "core/src/math/vec_templated.hpp"

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

#define FAZE_STATIC_SAMPLER(name)

#else /*__cplusplus*/

#define FAZE_BEGIN_LAYOUT(signatureName)
#define FAZE_END_LAYOUT

#define FAZE_CBUFFER \
    cbuffer _constants : register( b0 )

#define FAZE_SRV(type, name, index) \
	type name : register(t##index);

#define FAZE_UAV(type, name, index) \
	type name : register(u##index);

#define FAZE_SRV_TABLE(size)
#define FAZE_UAV_TABLE(size)

#define FAZE_STATIC_SAMPLER(name) SamplerState name : register( s0 );

#ifdef FAZE_DX12
#define ROOTSIG "RootFlags(0), " \
        "CBV(b0), " \
        "DescriptorTable( SRV(t0, numDescriptors = 32, flags = DESCRIPTORS_VOLATILE)), " \
        "DescriptorTable( UAV(u0, numDescriptors = 8, flags = DESCRIPTORS_VOLATILE)), " \
        "StaticSampler(s0, " \
             "filter = FILTER_MIN_MAG_MIP_LINEAR )"

#define GX_SIGNATURE [RootSignature(ROOTSIG)]
#define CS_SIGNATURE [RootSignature(ROOTSIG)] \
	[numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)]

#else /*FAZE_DX12*/

#define GX_SIGNATURE
#define CS_SIGNATURE [numthreads(FAZE_THREADGROUP_X, FAZE_THREADGROUP_Y, FAZE_THREADGROUP_Z)]

#endif /*FAZE_DX12*/
#endif /*__cplusplus*/
#endif /*FAZE_SHADER_DEFINITIONS*/