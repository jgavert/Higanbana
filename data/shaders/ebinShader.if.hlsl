// This file is reflected from code.
#ifdef FAZE_DX12
#define VK_BINDING(index) 
#else // vulkan
#define VK_BINDING(index) [[vk::binding(index)]]
#endif

#define ROOTSIG "RootFlags(0), CBV(b0), DescriptorTable(SRV(t0, numDescriptors = 1),UAV(u0, numDescriptors = 1),SRV(t1, numDescriptors = 1)), StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT), StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT), StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)"

struct Constants
{int a; int b; int c; };
VK_BINDING(0) ConstantBuffer<Constants> constants : register( b0 );

// Read Only resources
VK_BINDING(1) Buffer<float> test2 : register( t0 );
VK_BINDING(3) ByteAddressBuffer test : register( t1 );

// ReadWrite resources
VK_BINDING(2) RWBuffer<float4> testOut : register( u0 );
