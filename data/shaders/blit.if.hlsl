// INTERFACE_HASH:13579803029130971044:13912690230064188939
// This file is reflected from code.
#ifdef FAZE_VULKAN
#define VK_BINDING(index) [[vk::binding(index)]]
#else // FAZE_DX12
#define VK_BINDING(index) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  DescriptorTable(\
     SRV(t0, numDescriptors = 2)),\
  StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{float resx; float resy; float time; int unused; };
VK_BINDING(0) ConstantBuffer<Constants> constants : register( b0 );

// Read Only resources
VK_BINDING(1) ByteAddressBuffer vertexInput : register( t0 );
VK_BINDING(2) Texture2D<float4> texInput : register( t1 );

// ReadWrite resources

// Usable Static Samplers
VK_BINDING(3) SamplerState bilinearSampler : register( s0 );
VK_BINDING(4) SamplerState pointSampler : register( s1 );
VK_BINDING(5) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(6) SamplerState pointSamplerWrap : register( s3 );
