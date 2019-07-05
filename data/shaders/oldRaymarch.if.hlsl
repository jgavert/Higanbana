// INTERFACE_HASH:8942587802796542211:1290693801927549001
// This file is reflected from code.
#ifdef HIGANBANA_VULKAN
#define VK_BINDING(index) [[vk::binding(index)]]
#else // HIGANBANA_DX12
#define VK_BINDING(index) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  DescriptorTable(\
     UAV(u0, numDescriptors = 1)),\
  StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{float resx; float resy; float time; int unused; };
VK_BINDING(0) ConstantBuffer<Constants> constants : register( b0 );

// Read Only resources

// ReadWrite resources
VK_BINDING(1) RWTexture2D<float4> output : register( u0 );

// Usable Static Samplers
VK_BINDING(2) SamplerState bilinearSampler : register( s0 );
VK_BINDING(3) SamplerState pointSampler : register( s1 );
VK_BINDING(4) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(5) SamplerState pointSamplerWrap : register( s3 );
