// INTERFACE_HASH:7940331014023579972:13939906432982306273
// This file is reflected from code.
#ifdef FAZE_VULKAN
#define VK_BINDING(index) [[vk::binding(index)]]
#else // FAZE_DX12
#define VK_BINDING(index) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  DescriptorTable(\
     SRV(t0, numDescriptors = 2),\
     UAV(u0, numDescriptors = 1)),\
  StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{int a; int b; int c; };
VK_BINDING(0) ConstantBuffer<Constants> constants : register( b0 );

// Read Only resources
VK_BINDING(1) ByteAddressBuffer test : register( t0 );
VK_BINDING(2) Buffer<float> test2 : register( t1 );

// ReadWrite resources
VK_BINDING(3) RWBuffer<float4> testOut : register( u0 );

// Usable Static Samplers
VK_BINDING(4) SamplerState bilinearSampler : register( s0 );
VK_BINDING(5) SamplerState pointSampler : register( s1 );
VK_BINDING(6) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(7) SamplerState pointSamplerWrap : register( s3 );
