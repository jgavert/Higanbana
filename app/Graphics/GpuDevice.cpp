#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(ComPtr<ID3D12Device> device) : mDevice(device)
{
  size_t descriptorHeapMaxSize = 128;

  ComPtr<ID3D12DescriptorHeap> heap;
  D3D12_DESCRIPTOR_HEAP_DESC Desc;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  Desc.NumDescriptors = 128*4; // 128 of any type
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  HRESULT hr = mDevice->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap.addr()));
  if (FAILED(hr))
  {
    // 
  }
  ResourceViewManager descHeap = ResourceViewManager(heap, mDevice->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);


  ComPtr<ID3D12DescriptorHeap> heap2;
  D3D12_DESCRIPTOR_HEAP_DESC Desc2;
  Desc2.NodeMask = 0;
  Desc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  Desc2.NumDescriptors = 8;
  Desc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = mDevice->CreateDescriptorHeap(&Desc2, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap2.addr()));
  if (FAILED(hr))
  {
    // 
    abort();
  }
  ResourceViewManager descRTVHeap = ResourceViewManager(heap2, mDevice->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

  ComPtr<ID3D12DescriptorHeap> heap3;
  Desc.NodeMask = 0;
  Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  Desc.NumDescriptors = 8;
  Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = mDevice->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(heap3.addr()));
  if (FAILED(hr))
  {
    // 
    abort();
  }
  ResourceViewManager descDSVHeap = ResourceViewManager(heap3, mDevice->GetDescriptorHandleIncrementSize(Desc.Type), Desc.NumDescriptors);

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
  hr = mDevice->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&mDescriptorHeap);

  //create cpu descriptor handle for backbuffer 0
  mRenderTargetView[0] = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  //create cpu descriptor handle for backbuffer 1, offset by D3D12_RTV_DESCRIPTOR_HEAP from backbuffer 0's descriptor
  UINT HandleIncrementSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP);
  mRenderTargetView[1] = mRenderTargetView[0].MakeOffsetted(1, HandleIncrementSize);

  ID3D12Resource* mRenderTarget[2];
  //A buffer is required to render to.This example shows how to create that buffer by using the swap chain and device.
  //This example shows calling ID3D12Device::CreateRenderTargetView.
  hr = mSwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget[0]);
  mRenderTarget[0]->SetName(L"mRenderTarget0");  //set debug name 
  mDevice->CreateRenderTargetView(mRenderTarget[0], nullptr, mRenderTargetView[0]);

  //repeat for buffer #2
  hr = mSwapChain->GetBuffer(1, __uuidof(ID3D12Resource), (LPVOID*)&mRenderTarget[1]);
  mRenderTarget[1]->SetName(L"mRenderTarget1");
  mDevice->CreateRenderTargetView(mRenderTarget[1], nullptr, mRenderTargetView[1]);
  */
  return std::move(SwapChain(mSwapChain));
}

// Needs to be created from descriptor
GpuFence GpuDevice::createFence()
{
  ID3D12Fence* mFence;
  mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&mFence);
  return std::move(GpuFence(mFence));
}

GpuCommandQueue GpuDevice::createQueue()
{
  ID3D12CommandQueue* m_CommandQueue;
  HRESULT hr;
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  hr = mDevice->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&m_CommandQueue));
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
  HRESULT hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)commandListAllocator.addr());
  if (FAILED(hr))
  {
    abort();
  }
  hr = mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, commandListAllocator.get(), NULL, __uuidof(ID3D12CommandList), (void**)commandList.addr());
  if (FAILED(hr))
  {
    abort();
  }
  return std::move(GfxCommandList(commandList, commandListAllocator));
}
