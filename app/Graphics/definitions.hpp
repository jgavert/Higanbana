#ifndef FAZE_SHADER_DEFINITIONS
#define FAZE_SHADER_DEFINITIONS

#ifdef __cplusplus

#define FAZE_BEGIN_LAYOUT(signatureName) \
      struct signatureName {
#define FAZE_END_LAYOUT \
      };

#define FAZE_CBUFFER(cbufferType) \
    static constexpr cbufferType constants;

#define FAZE_SRV(type, name, index) \
	static constexpr int name = index;

#define FAZE_UAV(type, name, index) \
	static constexpr int name = index;

#else /*__cplusplus*/

#define FAZE_BEGIN_LAYOUT(signatureName)
#define FAZE_END_LAYOUT

#define FAZE_CBUFFER(cbufferType) \
    ConstantBuffer<cbufferType> constants : register( b0 );

#define FAZE_SRV(type, name, index) \
	type name : register(s##index);

#define FAZE_UAV(type, name, index) \
	type name : register(u##index);

#define FAZE_STATIC_SAMPLER(name) SamplerState name : register( s0 );

#ifdef FAZE_DX12
#define ROOTSIG "RootFlags( ), " \
        "CBV(b0), " \
        "DescriptorTable( SRV(t0, numDescriptors = 32)), " \
        "DescriptorTable( UAV(u0, numDescriptors = 8)), " \
        "StaticSampler(s0, " \
             "addressU = TEXTURE_ADDRESS_WRAP, " \
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