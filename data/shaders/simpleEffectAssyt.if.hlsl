// INTERFACE_HASH:6562379285513792293:17196241796759788332
// This file is generated from code.
#ifdef HIGANBANA_VULKAN
#define VK_BINDING(index, set) [[vk::binding(index, set)]]
#else // HIGANBANA_DX12
#define VK_BINDING(index, set) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  DescriptorTable(\
     UAV(u0, numDescriptors = 1, space=0 )),\
    DescriptorTable(\
     UAV(u99, numDescriptors = 1, space=99 )),\
StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{float resx; float resy; float time; int unused; };
VK_BINDING(0, 1) ConstantBuffer<Constants> constants : register( b0 );
VK_BINDING(1, 1) RWByteAddressBuffer debugPrint : register( u99 );
// Shader Arguments 0

// Read Only resources

// Read Write resources
VK_BINDING(0, 0) RWTexture2D<float4> output : register( u0, space0 );

// Usable Static Samplers
VK_BINDING(2, 1) SamplerState bilinearSampler : register( s0 );
VK_BINDING(3, 1) SamplerState pointSampler : register( s1 );
VK_BINDING(4, 1) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(5, 1) SamplerState pointSamplerWrap : register( s3 );
