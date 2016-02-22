#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(ComPtr<ID3D12Device> device, bool debugLayer) : m_device(device), m_debugLayer(debugLayer)
{
	// Check support of various features
	D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
	auto hri = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
	D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
	hri = device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE));
	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
	const D3D_FEATURE_LEVEL levels[9] = { D3D_FEATURE_LEVEL_9_1 , D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_12_1 };
	featureLevels.pFeatureLevelsRequested = levels;
	featureLevels.NumFeatureLevels = 9;
	hri = device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));
	D3D12_FEATURE_DATA_FORMAT_SUPPORT formatOptions = {};
	hri = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatOptions, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT));
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels = {};
	hri = device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_quality_levels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {};
	hri = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(D3D12_FEATURE_DATA_FORMAT_INFO));
	D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT virtual_address_support = {};
	hri = device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &virtual_address_support, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT));


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

GpuDevice::~GpuDevice()
{
	if (m_debugLayer)
	{
		ComPtr<ID3D12InfoQueue> infoQueue;
		HRESULT hr = m_device->QueryInterface(infoQueue.addr());
		if (!FAILED(hr))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		}
	}
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
	auto woot = stringutils::s2ws(desc.shaderSourcePath + ".hlsl");
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
      auto addHlsl = shaderPath + ".hlsl";
			auto woot = stringutils::s2ws(addHlsl);
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
  Buffer_new buf = {};
  buf.m_desc = resDesc;

  desc.Width = resDesc.m_width;
  desc.Height = resDesc.m_height;
  desc.DepthOrArraySize = static_cast<UINT16>(resDesc.m_arraySize);
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // TODO: need something that isn't platform specific
  desc.Format = FormatToDXGIFormat[resDesc.m_format];
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // ...?
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // for buffers, "Layout must be"
  desc.MipLevels = static_cast<UINT16>(resDesc.m_miplevels);
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // 64KB
  

  heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
  //heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
  buf.m_immutableState = false;
  if (resDesc.m_usage == ResourceUsage::UploadHeap)
  {
	  buf.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
	  buf.m_immutableState = true;
	  heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	  //heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }
  else if (resDesc.m_usage == ResourceUsage::ReadbackHeap)
  {
	  buf.m_state = D3D12_RESOURCE_STATE_COPY_DEST;
	  heapprop.Type = D3D12_HEAP_TYPE_READBACK;
	  buf.m_immutableState = true;
	  //heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }

  auto info = m_device->GetResourceAllocationInfo(0, 1, &desc);

  if (desc.Alignment != info.Alignment)
  {
	  F_LOG("Resource creation alignment differed from d3d12 devices defaults.\n");
	  desc.Alignment = info.Alignment;
  }
  F_LOG("Resource sizeInBytes: %.2f KB, %zu B\n", static_cast<float>(info.SizeInBytes) / 1024.f, info.SizeInBytes);
  F_LOG("Resource Alignment:   %.2f KB\n", static_cast<float>(info.Alignment) / 1024.f, info.SizeInBytes);

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

Texture_new GpuDevice::createTexture(ResourceDescriptor resDesc)
{
  Texture_new tex = {};
  D3D12_RESOURCE_DESC desc = {};
  D3D12_HEAP_PROPERTIES heapprop = {};
  ID3D12Resource* ptr;
  tex.m_desc = resDesc;

  desc.Width = resDesc.m_width;
  desc.Height = resDesc.m_height;
  desc.DepthOrArraySize = static_cast<UINT16>(resDesc.m_arraySize);
  desc.Dimension = FormatDimensionToD3D12[FormatDimension::DimTexture2D]; // TODO: need something that isn't hardcoded
  desc.Format = FormatToDXGIFormat[resDesc.m_format];
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // ...?
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // Basically, leave the choice to driver.
  // other msaa supported layout is D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE but requires reservedResource and updated with updateTileMappings
  desc.MipLevels = static_cast<UINT16>(resDesc.m_miplevels);
  desc.SampleDesc.Count = resDesc.m_msCount;
  desc.SampleDesc.Quality = resDesc.m_msQuality;
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

  tex.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
  // should probably be D3D12_RESOURCE_STATE_COMMON, for the sake of using specialized state
  // Every state used should be explicitly true and barriers used to change between those. Generic read is horrible!
  // TODO: sanitize this.

  heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
  //heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
  tex.m_immutableState = false;
  if (resDesc.m_usage == ResourceUsage::UploadHeap)
  {
    tex.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    tex.m_immutableState = true;
    heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }
  else if (resDesc.m_usage == ResourceUsage::ReadbackHeap)
  {
    tex.m_state = D3D12_RESOURCE_STATE_COPY_DEST;
    heapprop.Type = D3D12_HEAP_TYPE_READBACK;
    tex.m_immutableState = true;
	//heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }

  auto info = m_device->GetResourceAllocationInfo(0, 1, &desc);

  if (desc.Alignment != info.Alignment)
  {
	  F_LOG("Resource creation alignment differed from d3d12 devices defaults.\n");
	  desc.Alignment = info.Alignment;
  }
  F_LOG("Resource sizeInBytes: %.2f KB, %zu B\n", static_cast<float>(info.SizeInBytes) / 1024.f, info.SizeInBytes);
  F_LOG("Resource Alignment:   %.2f KB\n", static_cast<float>(info.Alignment) / 1024.f, info.SizeInBytes);

  HRESULT hr = m_device->CreateCommittedResource(
    &heapprop,
    D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
    &desc,
    tex.m_state,
    nullptr,
    __uuidof(ID3D12Resource),
    reinterpret_cast<void**>(&ptr));

  if (!FAILED(hr))
  {
	//F_LOG("Texture creation failed!");
    tex.m_resource = std::make_shared<RawResource>(ptr);
  } 
  else
  {
	tex.m_resource = std::make_shared<RawResource>(nullptr);
  }
  return tex;
}
