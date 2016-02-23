#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "FazCPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(FazCPtr<ID3D12Device> device, bool debugLayer) : m_device(device), m_debugLayer(debugLayer)
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

  auto nullTexture = createTexture(ResourceDescriptor()
	  .Width(100)
	  .Height(100)
	  .enableUnorderedAccess()
	  .Layout(TextureLayout::StandardSwizzle64kb)
	  .Dimension(FormatDimension::Texture2D)
	  .Format(FormatType::R8G8B8A8_UNORM));

  constexpr unsigned HeapSRVCount = 60;
  constexpr unsigned HeapUAVCount = HeapSRVCount;
  constexpr unsigned HeapDescriptorCount = HeapSRVCount + HeapUAVCount + 120;

  FazCPtr<ID3D12DescriptorHeap> heap;
  D3D12_DESCRIPTOR_HEAP_DESC Desc;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  Desc.NumDescriptors = HeapDescriptorCount; // 128 of any type
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT hr = m_device->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap.releaseAndAddr()));
  if (FAILED(hr))
  {
    // 
  }
  ResourceViewManager descHeap = ResourceViewManager(heap, m_device->GetDescriptorHandleIncrementSize(Desc.Type), HeapDescriptorCount, HeapSRVCount, HeapUAVCount);

  FazCPtr<ID3D12DescriptorHeap> heap2;
  D3D12_DESCRIPTOR_HEAP_DESC Desc2;
  Desc2.NodeMask = 0;
  Desc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  Desc2.NumDescriptors = 8;
  Desc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = m_device->CreateDescriptorHeap(&Desc2, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap2.releaseAndAddr()));
  if (FAILED(hr))
  {
	  // 
	  abort();
  }
  ResourceViewManager descRTVHeap = ResourceViewManager(heap2, m_device->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

  FazCPtr<ID3D12DescriptorHeap> heap3;
  D3D12_DESCRIPTOR_HEAP_DESC Desc3;
  Desc3.NodeMask = 0;
  Desc3.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  Desc3.NumDescriptors = 8;
  Desc3.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = m_device->CreateDescriptorHeap(&Desc3, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap3.releaseAndAddr()));
  if (FAILED(hr))
  {
	  // 
	  abort();
  }
  ResourceViewManager descDSVHeap = ResourceViewManager(heap3, m_device->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

  m_descHeaps.m_heaps[DescriptorHeapManager::Generic] = descHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::Generic] = descHeap.m_descHeap.get();

  m_descHeaps.m_heaps[DescriptorHeapManager::RTV] = descRTVHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::RTV] = descRTVHeap.m_descHeap.get();

  m_descHeaps.m_heaps[DescriptorHeapManager::DSV] = descDSVHeap;
  m_descHeaps.m_rawHeaps[DescriptorHeapManager::DSV] = descDSVHeap.m_descHeap.get();
  // srv's
  

  auto lol = m_descHeaps.getSRV().m_descHeap->GetCPUDescriptorHandleForHeapStart();
  UINT HandleIncrementSize = m_device->GetDescriptorHandleIncrementSize(Desc.Type);

  // srv desc
  ShaderViewDescriptor Viewdesc;
  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = createSrvDesc(nullTexture.getTexture().m_desc, Viewdesc);

  for (int i = 0; i < HeapSRVCount; i++)
  {
	  auto lol2 = lol;
	  lol2.ptr = lol.ptr + i * HandleIncrementSize;
	  m_device->CreateShaderResourceView(nullTexture.getTexture().m_resource->get(), &srvdesc, lol2);
  }

  // uav's
  D3D12_UNORDERED_ACCESS_VIEW_DESC uavdesc  = createUavDesc(nullTexture.getTexture().m_desc, Viewdesc);

  //uavdesc.Texture2D
  for (int i = HeapSRVCount; i < HeapUAVCount; i++)
  {
	  auto lol2 = lol;
	  lol2.ptr = lol.ptr + i * HandleIncrementSize;
	  m_device->CreateUnorderedAccessView(nullTexture.getTexture().m_resource->get(), nullptr, &uavdesc, lol2);
  }

  // special heaps for bindless textures/buffers
  // actually no special heaps, need to use one special
  // generic needs to be splitted into ranges.


}

GpuDevice::~GpuDevice()
{
	if (m_debugLayer)
	{
		FazCPtr<ID3D12InfoQueue> infoQueue;
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
  FazCPtr<ID3D12Fence> mFence;
  m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void**>(mFence.addr()));
  return std::move(GpuFence(mFence));
}

GpuCommandQueue GpuDevice::createQueue()
{
  FazCPtr<ID3D12CommandQueue> m_CommandQueue;
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
  FazCPtr<ID3D12GraphicsCommandList> commandList;
  FazCPtr<ID3D12CommandAllocator> commandListAllocator;
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
	FazCPtr<ID3D12RootSignature>    m_gRootSig;
	FazCPtr<ID3DBlob> blobCompute;
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
	FazCPtr<ID3D12RootSignatureDeserializer> asd;
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

	FazCPtr<ID3D12PipelineState> pipeline;
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
	FazCPtr<ID3DBlob> blobVertex;
	FazCPtr<ID3DBlob> blobPixel;
	FazCPtr<ID3DBlob> blobGeometry;
	FazCPtr<ID3DBlob> blobHull;
	FazCPtr<ID3DBlob> blobDomain;
	ShaderInterface newIf;
	auto lam = [&](FazCPtr<ID3DBlob>& blob, std::string shaderPath, ShaderType type)
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

	auto modi = [](D3D12_SHADER_BYTECODE& byte, FazCPtr<ID3DBlob> blob)
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
	FazCPtr<ID3D12PipelineState> pipeline;
	HRESULT hr = m_device->CreateGraphicsPipelineState(&graphicsDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(pipeline.addr()));
	if (FAILED(hr))
	{
		abort();
	}

	// reflect the root signature
	FazCPtr<ID3D12RootSignatureDeserializer> asd;
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

BufferNew GpuDevice::createBuffer(ResourceDescriptor resDesc)
{
  D3D12_RESOURCE_DESC desc = {};
  D3D12_HEAP_PROPERTIES heapprop = {};
  FazCPtr<ID3D12Resource> ptr;
  Buffer_new buf = {};
  buf.m_desc = resDesc;

  desc.Width = resDesc.m_width * (std::max)(resDesc.m_stride, 1u);
  desc.Height = resDesc.m_height;
  desc.DepthOrArraySize = static_cast<UINT16>(resDesc.m_arraySize);
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // TODO: need something that isn't platform specific
  desc.Format = FormatToDXGIFormat[resDesc.m_format];
  desc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(resDesc.m_layout); // for buffers, "Layout must be"
  desc.MipLevels = static_cast<UINT16>(resDesc.m_miplevels);
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // 64KB
  buf.m_state = D3D12_RESOURCE_STATE_COMMON;
  desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  if (resDesc.m_rendertarget)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }
  if (resDesc.m_depthstencil)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  }
  if (resDesc.m_unorderedaccess)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  if (resDesc.m_denysrv)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }
  if (resDesc.m_allowCrossAdapter)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
  if (resDesc.m_allowSimultaneousAccess)
  {
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
  }

  heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
  buf.m_immutableState = false;
  if (resDesc.m_usage == ResourceUsage::UploadHeap)
  {
	  buf.m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
	  buf.m_immutableState = true;
	  heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  }
  else if (resDesc.m_usage == ResourceUsage::ReadbackHeap)
  {
	  buf.m_state = D3D12_RESOURCE_STATE_COPY_DEST;
	  heapprop.Type = D3D12_HEAP_TYPE_READBACK;
	  buf.m_immutableState = true;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  }

  auto info = m_device->GetResourceAllocationInfo(0, 1, &desc);

  if (desc.Alignment != info.Alignment)
  {
	  F_LOG("Resource creation alignment differed from d3d12 devices defaults.\n");
	  desc.Alignment = info.Alignment;
  }
  /*
  F_LOG("Resource sizeInBytes: %.2f KB, %zu B\n", static_cast<float>(info.SizeInBytes) / 1024.f, info.SizeInBytes);
  F_LOG("Resource Alignment:   %.2f KB\n", static_cast<float>(info.Alignment) / 1024.f, info.SizeInBytes);
  */

  buf.m_sizeInBytes = info.SizeInBytes;
  buf.m_alignment = info.Alignment;

  HRESULT hr = m_device->CreateCommittedResource(
                  &heapprop,
                  D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                  &desc,
                  buf.m_state,
                  nullptr,
                  __uuidof(ID3D12Resource),
                  reinterpret_cast<void**>(ptr.releaseAndAddr()));
  BufferNew buf_ret;
  if (!FAILED(hr))
  {
    buf.m_resource = std::move(ptr);
    buf_ret.buffer = std::make_shared<Buffer_new>(std::move(buf));
  }
  return buf_ret;
}

TextureNew GpuDevice::createTexture(ResourceDescriptor resDesc)
{
  Texture_new tex = {};
  D3D12_RESOURCE_DESC desc = {};
  D3D12_HEAP_PROPERTIES heapprop = {};
  ID3D12Resource* ptr;
  tex.m_desc = resDesc;

  desc.Width = resDesc.m_width;
  desc.Height = resDesc.m_height;
  desc.DepthOrArraySize = static_cast<UINT16>(resDesc.m_arraySize);
  desc.Dimension = FormatDimensionToD3D12[FormatDimensionBase::DimTexture2D]; // TODO: need something that isn't hardcoded
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

  TextureNew tex_ret;
  if (!FAILED(hr))
  {
	//F_LOG("Texture creation failed!");
    tex.m_resource = std::make_shared<RawResource>(ptr);
    tex_ret.texture = std::make_shared<Texture_new>(tex);
  } 
  else
  {
	  tex.m_resource = std::make_shared<RawResource>(nullptr);
  }
  return tex_ret;
}

D3D12_SHADER_RESOURCE_VIEW_DESC GpuDevice::createSrvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc)
{
  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
  srvdesc.Format = FormatToDXGIFormat[desc.m_format];
  
  if (desc.m_dimension == FormatDimension::Unknown)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER; // structured buffer
    srvdesc.Buffer.FirstElement = viewDesc.m_firstElement;
    srvdesc.Buffer.NumElements = desc.m_width;
    srvdesc.Buffer.StructureByteStride = desc.m_stride;
    srvdesc.Buffer.Flags = (srvdesc.Format == DXGI_FORMAT_R32_TYPELESS) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
  }
  else 
  if (desc.m_dimension == FormatDimension::Buffer)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvdesc.Buffer.FirstElement = viewDesc.m_firstElement;
    srvdesc.Buffer.NumElements = desc.m_width;
    srvdesc.Buffer.StructureByteStride = desc.m_stride; // must be 0 if not structured buffer
    srvdesc.Buffer.Flags = (srvdesc.Format == DXGI_FORMAT_R32_TYPELESS) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
  }
  else if (desc.m_dimension == FormatDimension::Texture1D)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
    srvdesc.Texture1D.MipLevels = desc.m_miplevels;
    srvdesc.Texture1D.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.Texture1D.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  else if (desc.m_dimension == FormatDimension::Texture1DArray)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    srvdesc.Texture1DArray.ArraySize = desc.m_arraySize;
    srvdesc.Texture1DArray.FirstArraySlice = viewDesc.m_arraySlice;
    srvdesc.Texture1DArray.MipLevels = desc.m_miplevels;
    srvdesc.Texture1DArray.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.Texture1DArray.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp; 
  }
  else if (desc.m_dimension == FormatDimension::Texture2D)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvdesc.Texture2D.PlaneSlice = viewDesc.m_planeSlice;
    srvdesc.Texture2D.MipLevels = desc.m_miplevels;
    srvdesc.Texture2D.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.Texture2D.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMS)
  { 
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DArray)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvdesc.Texture2DArray.PlaneSlice = viewDesc.m_planeSlice;
    srvdesc.Texture2DArray.ArraySize = desc.m_arraySize;
    srvdesc.Texture2DArray.FirstArraySlice = viewDesc.m_arraySlice;
    srvdesc.Texture2DArray.MipLevels = desc.m_miplevels;
    srvdesc.Texture2DArray.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.Texture2DArray.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMSArray)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
    srvdesc.Texture2DMSArray.ArraySize = desc.m_arraySize;
    srvdesc.Texture2DMSArray.FirstArraySlice = viewDesc.m_arraySlice;
  }
  else if (desc.m_dimension == FormatDimension::Texture3D)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvdesc.Texture3D.MipLevels = desc.m_miplevels;
    srvdesc.Texture3D.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.Texture3D.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  else if (desc.m_dimension == FormatDimension::TextureCube)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvdesc.TextureCube.MipLevels = desc.m_miplevels;
    srvdesc.TextureCube.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.TextureCube.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  else if (desc.m_dimension == FormatDimension::TextureCubeArray)
  {
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
    srvdesc.TextureCubeArray.First2DArrayFace = viewDesc.m_first2DArrayFace;
    srvdesc.TextureCubeArray.MipLevels = desc.m_miplevels;
    srvdesc.TextureCubeArray.MostDetailedMip = viewDesc.m_mostDetailedMip;
    srvdesc.TextureCubeArray.ResourceMinLODClamp = viewDesc.m_resourceMinLODClamp;
  }
  srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // when should this differ?
  return srvdesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC GpuDevice::createUavDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc)
{
  D3D12_UNORDERED_ACCESS_VIEW_DESC uavdesc = {};
  uavdesc.Format = FormatToDXGIFormat[desc.m_format];

  if (desc.m_dimension == FormatDimension::Unknown)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER; // structured buffer
    uavdesc.Buffer.FirstElement = viewDesc.m_firstElement;
    uavdesc.Buffer.NumElements = desc.m_width;
    uavdesc.Buffer.StructureByteStride = desc.m_stride;
    uavdesc.Buffer.CounterOffsetInBytes = 0;
    uavdesc.Buffer.Flags = (uavdesc.Format == DXGI_FORMAT_R32_TYPELESS) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
  }
  else if (desc.m_dimension == FormatDimension::Buffer)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavdesc.Buffer.FirstElement = viewDesc.m_firstElement;
    uavdesc.Buffer.NumElements = desc.m_width;
    uavdesc.Buffer.StructureByteStride = desc.m_stride; // must be 0 if not structured buffer
    uavdesc.Buffer.CounterOffsetInBytes = 0; // must be 0 if not structured buffer
    uavdesc.Buffer.Flags = (uavdesc.Format == DXGI_FORMAT_R32_TYPELESS) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
  }
  else if (desc.m_dimension == FormatDimension::Texture1D)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
    uavdesc.Texture1D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture1DArray)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
    uavdesc.Texture1DArray.ArraySize = desc.m_arraySize;
    uavdesc.Texture1DArray.FirstArraySlice = viewDesc.m_arraySlice;
    uavdesc.Texture1DArray.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2D)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavdesc.Texture2D.PlaneSlice = viewDesc.m_planeSlice;
    uavdesc.Texture2D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DArray)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    uavdesc.Texture2DArray.PlaneSlice = viewDesc.m_planeSlice;
    uavdesc.Texture2DArray.ArraySize = desc.m_arraySize;
    uavdesc.Texture2DArray.FirstArraySlice = viewDesc.m_arraySlice;
    uavdesc.Texture2DArray.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture3D)
  {
    uavdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavdesc.Texture3D.FirstWSlice = 0; // The zero-based index of the first depth slice to be accessed.
    uavdesc.Texture3D.MipSlice = viewDesc.m_mostDetailedMip; // The zero-based index of the first depth slice to be accessed.
    uavdesc.Texture3D.WSize = 0; //The number of depth slices.
  }
  else
  {
    abort(); // replace with assert and exit.
  }

  return uavdesc;
}

D3D12_RENDER_TARGET_VIEW_DESC GpuDevice::createRtvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc)
{
  D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
  rtv.Format = FormatToDXGIFormat[desc.m_format];
  if (desc.m_dimension == FormatDimension::Unknown)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
    rtv.Buffer.FirstElement;
    rtv.Buffer.NumElements;
  }
  else if (desc.m_dimension == FormatDimension::Buffer)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
    rtv.Buffer.FirstElement;
    rtv.Buffer.NumElements;
  }
  else if (desc.m_dimension == FormatDimension::Texture1D)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
    rtv.Texture1D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture1DArray)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
    rtv.Texture1DArray.ArraySize = desc.m_arraySize;
    rtv.Texture1DArray.FirstArraySlice = viewDesc.m_arraySlice;
    rtv.Texture1DArray.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2D)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv.Texture2D.PlaneSlice = viewDesc.m_planeSlice;
    rtv.Texture2D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMS)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DArray)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    rtv.Texture2DArray.ArraySize = desc.m_arraySize;
    rtv.Texture2DArray.FirstArraySlice = viewDesc.m_arraySlice;
    rtv.Texture2DArray.MipSlice = viewDesc.m_mostDetailedMip;
    rtv.Texture2DArray.PlaneSlice = viewDesc.m_planeSlice;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMSArray)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
    rtv.Texture2DMSArray.ArraySize = desc.m_arraySize;
    rtv.Texture2DMSArray.FirstArraySlice = viewDesc.m_arraySlice;
  }
  else if (desc.m_dimension == FormatDimension::Texture3D)
  {
    rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
    rtv.Texture3D.FirstWSlice = 0; // The zero-based index of the first depth slice to be accessed.
    rtv.Texture3D.MipSlice = viewDesc.m_mostDetailedMip; // The zero-based index of the first depth slice to be accessed.
    rtv.Texture3D.WSize = 0; //The number of depth slices.
  }
  else
  {
    abort(); // TODO: replace with somethign better
  }
  return rtv;
}

D3D12_DEPTH_STENCIL_VIEW_DESC GpuDevice::createDsvDesc(ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc)
{
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
  dsv.Format = FormatToDXGIFormat[desc.m_format];
  if (desc.m_dimension == FormatDimension::Texture1D)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
    dsv.Texture1D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture1DArray)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
    dsv.Texture1DArray.ArraySize = desc.m_arraySize;
    dsv.Texture1DArray.FirstArraySlice = viewDesc.m_arraySlice;
    dsv.Texture1DArray.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2D)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMS)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DArray)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
    dsv.Texture2DArray.ArraySize = desc.m_arraySize;
    dsv.Texture2DArray.FirstArraySlice = viewDesc.m_arraySlice;
    dsv.Texture2DArray.MipSlice = viewDesc.m_mostDetailedMip;
  }
  else if (desc.m_dimension == FormatDimension::Texture2DMSArray)
  {
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
    dsv.Texture2DMSArray.ArraySize = desc.m_arraySize;
    dsv.Texture2DMSArray.FirstArraySlice = viewDesc.m_arraySlice;
  }
  else
  {
    abort(); // TODO: replace with somethign better
  }
  return dsv;
}

TextureNewSRV GpuDevice::createTextureSRV(TextureNew targetTexture, ShaderViewDescriptor viewDesc)
{
  // get index
  auto& m_descSRVHeap = m_descHeaps.getSRV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descSRVHeap.m_handleIncrementSize);
  auto lol = m_descSRVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descSRVHeap.getSRVIndex();

  // save information
  TextureNewSRV srv;
  srv.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  srv.indexInHeap = FazPtr<size_t>(index, [&m_descSRVHeap](size_t index)
  {
    m_descSRVHeap.freeView(index);
  });
  srv.customIndex = index - m_descSRVHeap.getSRVStartIndex();
  srv.m_texture = targetTexture;

  // create descriptor for shaderview
  auto srvdesc = createSrvDesc(srv.texture().m_desc, viewDesc);
  srv.viewDesc = viewDesc;
  // create srv
  m_device->CreateShaderResourceView(srv.texture().m_resource->get(), &srvdesc, srv.cpuHandle);
  return srv;
}

TextureNewUAV GpuDevice::createTextureUAV(TextureNew targetTexture, ShaderViewDescriptor viewDesc)
{
  // get index
  auto& m_descUAVHeap = m_descHeaps.getUAV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descUAVHeap.m_handleIncrementSize);
  auto lol = m_descUAVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descUAVHeap.getUAVIndex();

  // save information
  TextureNewUAV uav;
  uav.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  uav.indexInHeap = FazPtr<size_t>(index, [&m_descUAVHeap](size_t index)
  {
    m_descUAVHeap.freeView(index);
  });
  uav.customIndex = index - m_descUAVHeap.getUAVStartIndex();
  uav.m_texture = targetTexture;

  // create descriptor for shaderview
  auto uavdesc = createUavDesc(uav.texture().m_desc, viewDesc);
  uav.viewDesc = viewDesc;
  // create uav
  // TODO add counterResourceSupport?
  m_device->CreateUnorderedAccessView(uav.texture().m_resource->get(), nullptr, &uavdesc, uav.cpuHandle);
  return uav;
}

TextureNewRTV GpuDevice::createTextureRTV(TextureNew targetTexture, ShaderViewDescriptor viewDesc)
{
  // get index
  auto& m_descRTVHeap = m_descHeaps.getRTV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descRTVHeap.m_handleIncrementSize);
  auto lol = m_descRTVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descRTVHeap.getNextIndex(); 

  // save information
  TextureNewRTV rtv;
  rtv.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  rtv.indexInHeap = FazPtr<size_t>(index, [&m_descRTVHeap](size_t index)
  {
    m_descRTVHeap.freeView(index);
  });
  rtv.customIndex = index;
  rtv.m_texture = targetTexture;

  // create descriptor for shaderview
  auto rtvdesc = createRtvDesc(rtv.texture().m_desc, viewDesc);
  rtv.viewDesc = viewDesc;
  // create srv
  m_device->CreateRenderTargetView(rtv.texture().m_resource->get(), &rtvdesc, rtv.cpuHandle);
  return rtv;
}

TextureNewDSV GpuDevice::createTextureDSV(TextureNew targetTexture, ShaderViewDescriptor viewDesc)
{
  // get index
  auto& m_descDSVHeap = m_descHeaps.getDSV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descDSVHeap.m_handleIncrementSize);
  auto lol = m_descDSVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descDSVHeap.getNextIndex();

  // save information
  TextureNewDSV dsv;
  dsv.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  dsv.indexInHeap = FazPtr<size_t>(index, [&m_descDSVHeap](size_t index)
  {
    m_descDSVHeap.freeView(index);
  });
  dsv.customIndex = index;
  dsv.m_texture = targetTexture;

  // create descriptor for shaderview
  auto dsvdesc = createDsvDesc(dsv.texture().m_desc, viewDesc);
  dsv.viewDesc = viewDesc;
  // create srv
  m_device->CreateDepthStencilView(dsv.texture().m_resource->get(), &dsvdesc, dsv.cpuHandle);
  return dsv;
}

BufferNewSRV GpuDevice::createBufferSRV(BufferNew buffer, ShaderViewDescriptor viewDesc)
{
  // get index
  Buffer_new& targetBuffer = buffer.getBuffer();
  auto& m_descSRVHeap = m_descHeaps.getSRV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descSRVHeap.m_handleIncrementSize);
  auto lol = m_descSRVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descSRVHeap.getSRVIndex();

  // save information
  BufferNewSRV srv;
  srv.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  srv.indexInHeap = FazPtr<size_t>(index, [&m_descSRVHeap](size_t index)
  {
    m_descSRVHeap.freeView(index);
  });
  srv.customIndex = index - m_descSRVHeap.getSRVStartIndex();
  srv.m_buffer = buffer;

  // create descriptor
  auto srvdesc = createSrvDesc(targetBuffer.m_desc, viewDesc);
  srv.viewDesc = viewDesc;

  // create srv
  m_device->CreateShaderResourceView(targetBuffer.m_resource.get(), &srvdesc, srv.cpuHandle);
  return srv;
}

BufferNewUAV GpuDevice::createBufferUAV(BufferNew buffer, ShaderViewDescriptor viewDesc)
{
  // get index
  Buffer_new& targetBuffer = buffer.getBuffer();
  auto& m_descUAVHeap = m_descHeaps.getUAV();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descUAVHeap.m_handleIncrementSize);
  auto lol = m_descUAVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descUAVHeap.getUAVIndex();

  // save information
  BufferNewUAV uav;
  uav.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  uav.indexInHeap = FazPtr<size_t>(index, [&m_descUAVHeap](size_t index)
  {
    m_descUAVHeap.freeView(index);
  });
  uav.customIndex = index - m_descUAVHeap.getUAVStartIndex();
  uav.m_buffer = buffer;

  // create descriptor for uav
  auto uavdesc = createUavDesc(targetBuffer.m_desc, viewDesc);
  uav.viewDesc = viewDesc;

  // create uav
  // TODO add counterResourceSupport?
  m_device->CreateUnorderedAccessView(targetBuffer.m_resource.get(), nullptr, &uavdesc, uav.cpuHandle);
  return uav;
}

BufferNewCBV GpuDevice::createBufferCBV(BufferNew buffer, ShaderViewDescriptor)
{
  // get index
  Buffer_new& targetBuffer = buffer.getBuffer();
  auto& m_descCBVHeap = m_descHeaps.getGeneric();
  UINT HandleIncrementSize = static_cast<unsigned int>(m_descCBVHeap.m_handleIncrementSize);
  auto lol = m_descCBVHeap.m_descHeap->GetCPUDescriptorHandleForHeapStart();
  auto index = m_descCBVHeap.getCBVIndex();

  // save information
  BufferNewCBV cbv;
  cbv.cpuHandle.ptr = lol.ptr + index * HandleIncrementSize;
  cbv.indexInHeap = FazPtr<size_t>(index, [&m_descCBVHeap](size_t index)
  {
    m_descCBVHeap.freeView(index);
  });
  cbv.customIndex = index - m_descCBVHeap.getCBVStartIndex();
  cbv.m_buffer = buffer;

  // create descriptor for cbv
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
  cbvdesc.BufferLocation = 0;
  cbvdesc.SizeInBytes = static_cast<unsigned>(targetBuffer.m_sizeInBytes);

  // create cbv
  m_device->CreateConstantBufferView(&cbvdesc, cbv.cpuHandle);
  return cbv;
}

// !? TODO
BufferNewIBV GpuDevice::createBufferIBV(BufferNew buffer, ShaderViewDescriptor )
{
  BufferNewIBV ibv;
  ibv.m_buffer = buffer;

  
  return ibv;
}
