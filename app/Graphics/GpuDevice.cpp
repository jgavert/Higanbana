#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(ComPtr<ID3D12Device> device, bool debugLayer) : m_device(device), m_debugLayer(debugLayer)
{
  m_nullSrv = createTextureSrvObj(Dimension(1,1));
  m_nullUav = createTextureUavObj(Dimension(1,1));

  constexpr unsigned HeapSRVCount = 60;
  constexpr unsigned HeapUAVCount = HeapSRVCount;
  constexpr unsigned HeapDescriptorCount = HeapSRVCount + HeapUAVCount + 120;

  ComPtr<ID3D12DescriptorHeap> heap;
  D3D12_DESCRIPTOR_HEAP_DESC Desc;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  Desc.NumDescriptors = HeapDescriptorCount; // 128 of any type
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT hr = m_device->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap.addr()));
  if (FAILED(hr))
  {
    // 
  }
  ResourceViewManager descHeap = ResourceViewManager(heap, m_device->GetDescriptorHandleIncrementSize(Desc.Type), HeapDescriptorCount, HeapSRVCount, HeapUAVCount);
  // srv's
  

  auto lol = heap->GetCPUDescriptorHandleForHeapStart();
  UINT HandleIncrementSize = m_device->GetDescriptorHandleIncrementSize(Desc.Type);

  // srv desc
  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc;
  ZeroMemory(&srvdesc, sizeof(srvdesc));
  srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvdesc.Texture2D.MipLevels = 1;

  for (int i = 0; i < HeapSRVCount; i++)
  {
	  auto lol2 = lol;
	  lol2.ptr = lol.ptr + i * HandleIncrementSize;
	  m_device->CreateShaderResourceView(m_nullSrv.texture().m_resource.get(), &srvdesc, lol2);
  }

  // uav's
  D3D12_UNORDERED_ACCESS_VIEW_DESC uavdesc = {};
  ZeroMemory(&uavdesc, sizeof(uavdesc));
  uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  uavdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  //uavdesc.Texture2D
  for (int i = HeapSRVCount; i < HeapUAVCount; i++)
  {
	  auto lol2 = lol;
	  lol2.ptr = lol.ptr + i * HandleIncrementSize;
	  m_device->CreateUnorderedAccessView(m_nullUav.texture().m_resource.get(), nullptr, &uavdesc, lol2);
  }
  


  ComPtr<ID3D12DescriptorHeap> heap2;
  D3D12_DESCRIPTOR_HEAP_DESC Desc2;
  Desc2.NodeMask = 0;
  Desc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  Desc2.NumDescriptors = 8;
  Desc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = m_device->CreateDescriptorHeap(&Desc2, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap2.addr()));
  if (FAILED(hr))
  {
    // 
    abort();
  }
  ResourceViewManager descRTVHeap = ResourceViewManager(heap2, m_device->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

  ComPtr<ID3D12DescriptorHeap> heap3;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  Desc.NumDescriptors = 8;
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = m_device->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap3.addr()));
  if (FAILED(hr))
  {
    // 
    abort();
  }
  ResourceViewManager descDSVHeap = ResourceViewManager(heap3, m_device->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

  // special heaps for bindless textures/buffers
  // actually no special heaps, need to use one special
  // generic needs to be splitted into ranges.

  m_descHeaps.m_heaps[DescriptorHeapManager::Generic] = descHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::Generic] = descHeap.m_descHeap.get();

  m_descHeaps.m_heaps[DescriptorHeapManager::RTV] = descRTVHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::RTV] = descRTVHeap.m_descHeap.get();

  m_descHeaps.m_heaps[DescriptorHeapManager::DSV] = descDSVHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::DSV] = descDSVHeap.m_descHeap.get();

}

// Needs to be created from descriptor
GpuFence GpuDevice::createFence()
{
  ComPtr<ID3D12Fence> mFence;
  m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void**>(mFence.addr()));
  return std::move(GpuFence(mFence));
}

GpuCommandQueue GpuDevice::createQueue()
{
  ComPtr<ID3D12CommandQueue> m_CommandQueue;
  HRESULT hr;
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  hr = m_device->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(m_CommandQueue.addr()));
  if (FAILED(hr))
  {
    //
  }
  return std::move(GpuCommandQueue(m_CommandQueue));
}

// Needs to be created from descriptor
GfxCommandList GpuDevice::createUniversalCommandList()
{
  ComPtr<ID3D12GraphicsCommandList> commandList;
  ComPtr<ID3D12CommandAllocator> commandListAllocator;
  HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)commandListAllocator.addr());
  if (FAILED(hr))
  {
    abort();
  }
  hr = m_device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, commandListAllocator.get(), NULL, __uuidof(ID3D12CommandList), (void**)commandList.addr());
  if (FAILED(hr))
  {
    abort();
  }
  return std::move(GfxCommandList(commandList, commandListAllocator));
}

ComputePipeline GpuDevice::createComputePipeline(ComputePipelineDescriptor desc)
{
	ComPtr<ID3D12RootSignature>    m_gRootSig;
	ComPtr<ID3DBlob> blobCompute;
	auto woot = stringutils::s2ws(desc.shaderSourcePath);
	ShaderInterface existing;
	shaderUtils::getShaderInfo(m_device, ShaderType::Compute, woot, existing, blobCompute);
	D3D12_SHADER_BYTECODE byte;
	byte.pShaderBytecode = blobCompute->GetBufferPointer();
	byte.BytecodeLength = blobCompute->GetBufferSize();

	// figure out if we already have that rootDescriptor
	bool hadOne = false;
	for (auto&& it : m_shaderInterfaceCache)
	{
		if (it.isCopyOf(existing.m_rootDesc))
		{
			existing = it;
			hadOne = true;
			break;
		}
	}
	if (!hadOne)
	{
		// new shaderinterface
		m_shaderInterfaceCache.push_back(existing);
	}
	ComPtr<ID3D12RootSignatureDeserializer> asd;
	HRESULT hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
	if (FAILED(hr))
	{
		abort();
	}
	const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();

	// pipeline is allowed to be done after we have verified the rootDescriptor
	D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc;
	ZeroMemory(&computeDesc, sizeof(computeDesc));
	computeDesc.CS = byte;
	computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computeDesc.NodeMask = 0;
	computeDesc.pRootSignature = m_gRootSig.get();

	ComPtr<ID3D12PipelineState> pipeline;
	hr = m_device->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.addr()));
	if (FAILED(hr))
	{
		abort();
	}

	size_t cbv = 0, srv = 0, uav = 0;
	auto bindingInput = shaderUtils::getRootDescriptorReflection(woot2, cbv, srv, uav);
	int descTableSRV = -1, descTableUAV = -1;
	shaderUtils::getRootDescriptorReflection2(woot2, descTableSRV, descTableUAV);
	ComputeBinding sourceBinding(bindingInput, static_cast<unsigned int>(cbv), static_cast<unsigned int>(srv), static_cast<unsigned int>(uav), descTableSRV, descTableUAV);
	sourceBinding.m_descTableSRV.first = m_descHeaps.getGeneric().getSRVStartAddress();
	sourceBinding.m_descTableUAV.first = m_descHeaps.getGeneric().getUAVStartAddress();
	auto pipe = ComputePipeline(pipeline, existing, sourceBinding, m_descHeaps.getGeneric());
	return pipe;
}

GraphicsPipeline GpuDevice::createGraphicsPipeline(GraphicsPipelineDescriptor desc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc = desc.getDesc();
	ComPtr<ID3DBlob> blobVertex;
	ComPtr<ID3DBlob> blobPixel;
	ComPtr<ID3DBlob> blobGeometry;
	ComPtr<ID3DBlob> blobHull;
	ComPtr<ID3DBlob> blobDomain;
	ShaderInterface newIf;
	auto lam = [&](ComPtr<ID3DBlob>& blob, std::string shaderPath, ShaderType type)
	{
		if (!shaderPath.empty())
		{
			auto woot = stringutils::s2ws(shaderPath);
			shaderUtils::getShaderInfo(m_device, type, woot, newIf, blob);
		}
	};
	lam(blobVertex, desc.vertexShaderPath, ShaderType::Vertex);
	lam(blobPixel, desc.pixelShaderPath, ShaderType::Pixel);
	lam(blobGeometry, desc.geometryShaderPath, ShaderType::Geometry);
	lam(blobHull, desc.hullShaderPath, ShaderType::Hull);
	lam(blobDomain, desc.domainShaderPath, ShaderType::Domain);

	// need to check, if we had new rootdescriptor or not.
	bool hadOne = false;
	for (auto&& it : m_shaderInterfaceCache)
	{
		if (it.isCopyOf(newIf.m_rootDesc))
		{
			newIf = it;
			hadOne = true;
			break;
		}
	}
	if (!hadOne)
	{
		// new shaderinterface
		m_shaderInterfaceCache.push_back(newIf);
	}

	auto modi = [](D3D12_SHADER_BYTECODE& byte, ComPtr<ID3DBlob> blob)
	{
		if (blob.get() != nullptr)
		{
			byte.BytecodeLength = blob->GetBufferSize();
			byte.pShaderBytecode = blob->GetBufferPointer();
		}
	};

	modi(graphicsDesc.VS, blobVertex);
	modi(graphicsDesc.PS, blobPixel);
	modi(graphicsDesc.GS, blobGeometry);
	modi(graphicsDesc.HS, blobHull);
	modi(graphicsDesc.DS, blobDomain);
	graphicsDesc.pRootSignature = newIf.m_rootSig.get();
	ComPtr<ID3D12PipelineState> pipeline;
	HRESULT hr = m_device->CreateGraphicsPipelineState(&graphicsDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.addr()));
	if (FAILED(hr))
	{
		abort();
	}

	// reflect the root signature
	ComPtr<ID3D12RootSignatureDeserializer> asd;
	hr = D3D12CreateRootSignatureDeserializer(blobVertex->GetBufferPointer(), blobVertex->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
	if (FAILED(hr))
	{
		abort();
	}
	const D3D12_ROOT_SIGNATURE_DESC* woot2 = asd->GetRootSignatureDesc();

	size_t cbv = 0, srv = 0, uav = 0;
	auto bindingInput = shaderUtils::getRootDescriptorReflection(woot2, cbv, srv, uav);
	int descTableSRV = -1, descTableUAV = -1;
	shaderUtils::getRootDescriptorReflection2(woot2, descTableSRV, descTableUAV);
	GraphicsBinding sourceBinding(bindingInput, static_cast<unsigned int>(cbv), static_cast<unsigned int>(srv), static_cast<unsigned int>(uav), descTableSRV, descTableUAV);
	sourceBinding.m_descTableSRV.first = m_descHeaps.getGeneric().getSRVStartAddress();
	sourceBinding.m_descTableUAV.first = m_descHeaps.getGeneric().getUAVStartAddress();
	return GraphicsPipeline(pipeline, newIf, sourceBinding, m_descHeaps.getGeneric());
}

Buffer_new GpuDevice::createBuffer(ResourceDescriptor resDesc)
{
  D3D12_RESOURCE_DESC desc = {};
  D3D12_HEAP_PROPERTIES heapprop = {};
  ComPtr<ID3D12Resource> ptr;
  Buffer_new buf;
  buf.m_desc = resDesc;

  desc.Width = resDesc.m_width;
  desc.Height = resDesc.m_height;
  desc.DepthOrArraySize = resDesc.m_arraySize;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // TODO: need something that isn't platform specific
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // ...?
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.MipLevels = resDesc.m_miplevels;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;

  buf.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
  heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
  buf.m_immutableState = false;
  if (resDesc.m_usage == ResourceUsage::UploadHeap)
  {
    buf.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    buf.m_immutableState = true;
    heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
  }
  else if (resDesc.m_usage == ResourceUsage::ReadbackHeap)
  {
    buf.m_state = D3D12_RESOURCE_STATE_COPY_DEST;
    heapprop.Type = D3D12_HEAP_TYPE_READBACK;
    buf.m_immutableState = true;
  }

  HRESULT hr = m_device->CreateCommittedResource(
                  &heapprop,
                  D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                  &desc,
                  buf.m_state,
                  nullptr,
                  __uuidof(ID3D12Resource),
                  reinterpret_cast<void**>(ptr.addr()));

  if (!FAILED(hr))
  {
    buf.m_resource = std::move(ptr);
  }
  return buf;
}

Texture_new GpuDevice::createTexture(ResourceDescriptor desc)
{
  Texture_new tex;
  return tex;
}
