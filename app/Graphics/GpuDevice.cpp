#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(ComPtr<ID3D12Device> device) : m_device(device)
{
  size_t descriptorHeapMaxSize = 128;

  ComPtr<ID3D12DescriptorHeap> heap;
  D3D12_DESCRIPTOR_HEAP_DESC Desc;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  Desc.NumDescriptors = 128*4; // 128 of any type
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT hr = m_device->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap.addr()));
  if (FAILED(hr))
  {
    // 
  }
  ResourceViewManager descHeap = ResourceViewManager(heap, m_device->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors, 32, 32);


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

// Needs to be created from descriptor, if you say you have 2 buffers, it is expected that this will then handle it.
// gpudevice handles the memory so it's fine. Strange that I don't have any heap stuff yet. hmm
// Decided, hardcode this so that we can test better the resource managing and fix this later to be properly configurable.
// swapchain holds the textures for now with the trivial way.
// shit
SwapChain GpuDevice::createSwapChain(Window& wnd, GpuCommandQueue& queue)
{
  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
  swapChainDesc.BufferCount = 2;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = wnd.getNative();
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

  IDXGIFactory2 *dxgiFactory = nullptr;
  HRESULT hr;
  hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), (void**)&dxgiFactory);
  if (FAILED(hr))
  {
    //
  }
  ComPtr<IDXGISwapChain3> mSwapChain;
  hr = dxgiFactory->CreateSwapChain(queue.m_CommandQueue.get(), &swapChainDesc, (IDXGISwapChain**)mSwapChain.addr());
  dxgiFactory->Release();
  if (FAILED(hr))
  {
    dxgiFactory->Release();
    F_LOG("wtf error!", 2);
  }
  /*
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
  ZeroMemory(&heapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
  heapDesc.NumDescriptors = 2;
  heapDesc.Type = D3D12_RTV_DESCRIPTOR_HEAP; //must set the type
  ID3D12DescriptorHeap* mDescriptorHeap;
  D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView[2];
  hr = m_device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mDescriptorHeap);

  //create cpu descriptor handle for backbuffer 0
  mRenderTargetView[0] = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  //create cpu descriptor handle for backbuffer 1, offset by D3D12_RTV_DESCRIPTOR_HEAP from backbuffer 0's descriptor
  UINT HandleIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP);
  mRenderTargetView[1] = mRenderTargetView[0].MakeOffsetted(1, HandleIncrementSize);

  ID3D12Resource* mRenderTarget[2];
  //A buffer is required to render to.This example shows how to create that buffer by using the swap chain and device.
  //This example shows calling ID3D12Device::CreateRenderTargetView.
  hr = mSwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget[0]);
  mRenderTarget[0]->SetName(L"mRenderTarget0");  //set debug name 
  m_device->CreateRenderTargetView(mRenderTarget[0], nullptr, mRenderTargetView[0]);

  //repeat for buffer #2
  hr = mSwapChain->GetBuffer(1, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget[1]);
  mRenderTarget[1]->SetName(L"mRenderTarget1");
  m_device->CreateRenderTargetView(mRenderTarget[1], nullptr, mRenderTargetView[1]);
  */
  return std::move(SwapChain(mSwapChain));
}

// Needs to be created from descriptor
GpuFence GpuDevice::createFence()
{
  ID3D12Fence* mFence;
  m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&mFence);
  return std::move(GpuFence(mFence));
}

GpuCommandQueue GpuDevice::createQueue()
{
  ID3D12CommandQueue* m_CommandQueue;
  HRESULT hr;
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  hr = m_device->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&m_CommandQueue));
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
	int bindlessSRV, bindlessUAV;
	auto bindingInput = shaderUtils::getRootDescriptorReflection(woot2, cbv, srv, uav);
	int descTableSRV = -1, descTableUAV = -1;
	shaderUtils::getRootDescriptorReflection2(woot2, descTableSRV, descTableUAV);
	ComputeBinding sourceBinding(bindingInput, static_cast<unsigned int>(cbv), static_cast<unsigned int>(srv), static_cast<unsigned int>(uav), descTableSRV, descTableUAV);
	return ComputePipeline(pipeline, existing, sourceBinding);
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

	return GraphicsPipeline(pipeline, newIf, sourceBinding);
}
