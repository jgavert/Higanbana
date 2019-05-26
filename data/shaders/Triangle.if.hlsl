// INTERFACE_HASH:7941720630108861851:6035904498340629919
// This file is reflected from code.
#ifdef FAZE_VULKAN
#define VK_BINDING(index) [[vk::binding(index)]]
#else // FAZE_DX12
#define VK_BINDING(index) 
#endif

#define ROOTSIG "RootFlags(0), \
  CBV(b0), \
  StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), \
  StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), \
  StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), \
  StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"


// Read Only resources

// ReadWrite resources

// Usable Static Samplers
VK_BINDING(1) SamplerState bilinearSampler : register( s0 );
VK_BINDING(2) SamplerState pointSampler : register( s1 );
VK_BINDING(3) SamplerState bilinearSamplerWarp : register( s2 );
VK_BINDING(4) SamplerState pointSamplerWrap : register( s3 );
