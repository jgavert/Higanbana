// INTERFACE_HASH:3914398196978621125:11754377212221884353
// This file is generated from code.
#ifdef HIGANBANA_VULKAN
#define VK_BINDING(index, set) [[vk::binding(index, set)]]
#else // HIGANBANA_DX12
#define VK_BINDING(index, set) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  DescriptorTable(\
     SRV(t0, numDescriptors = 1, space=0 )),\
  DescriptorTable(\
     UAV(u0, numDescriptors = 1, space=1 )),\
  StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{float value; };
VK_BINDING(0, 2) ConstantBuffer<Constants> constants : register( b0 );
// Shader Arguments 0

// Read Only resources
VK_BINDING(0, 0) Buffer<float> input : register( t0, space0 );

// Read Write resources
// Shader Arguments 1

// Read Only resources

// Read Write resources
VK_BINDING(0, 1) RWBuffer<float> output : register( u0, space1 );

// Usable Static Samplers
VK_BINDING(1, 2) SamplerState bilinearSampler : register( s0 );
VK_BINDING(2, 2) SamplerState pointSampler : register( s1 );
VK_BINDING(3, 2) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(4, 2) SamplerState pointSamplerWrap : register( s3 );