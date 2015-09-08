#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "core/src/tests/TestWorks.hpp"

GpuDevice::GpuDevice(ComPtr<ID3D12Device> device) : mDevice(device)
{
  HRESULT hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&mCommandListAllocator);
  if (FAILED(hr))
  {
    //
  }
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
  IDXGISwapChain3* mSwapChain = nullptr;
  hr = dxgiFactory->CreateSwapChain(queue.m_CommandQueue.get(), &swapChainDesc, (IDXGISwapChain**)&mSwapChain);
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
  ID3D12GraphicsCommandList* m_CommandList;
  HRESULT hr;
  hr = mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandListAllocator.get(), NULL, __uuidof(ID3D12CommandList), (void**)&m_CommandList);
  if (FAILED(hr))
  {
    //
  }
  return std::move(GfxCommandList(m_CommandList));
}

void GpuDevice::doExperiment()
{

}

EvolRes GpuDevice::CommittedResTest()
{
  float triangleVerts[] ={ 0.0f, 0.5f, 0.0f, 0.45f, -0.5, 0.0f,-0.45f, -0.5f, 0.0f };

  ID3D12Resource *data;

  D3D12_RESOURCE_DESC datadesc = {};
  datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  datadesc.Format = DXGI_FORMAT_UNKNOWN;
  datadesc.Width = 80000000;
  datadesc.Height = 1;
  datadesc.DepthOrArraySize = 1;
  datadesc.MipLevels = 1;
  datadesc.SampleDesc.Count = 1;
  datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  D3D12_HEAP_PROPERTIES heapprop = {};
  heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
  HRESULT hr = mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&data);
  if (FAILED(hr))
  {
    F_LOG("woot!\n");
  }
  else
  {
    D3D12_HEAP_PROPERTIES properties;
    D3D12_HEAP_FLAGS flag;
    hr = data->GetHeapProperties(&properties, &flag);
    if (!FAILED(hr))
    {
      F_LOG("yay!\n", 2);
    }
  }
  EvolRes asd;
  asd.m_res = data;
  return asd;

  //
  // actually create the vert buffer
  // Note: using upload heaps to transfer static data like vert buffers is not recommended.
  // Every time the GPU needs it, the upload heap will be marshalled over.  Please read up on Default Heap usage.
  // An upload heap is used here for code simplicity and because there are very few verts to actually transfer
  //
  /*
  mDevice->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(3 * sizeof(float)),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,    // Clear value
    IID_PPV_ARGS(mBufVerts.GetAddressOf()));

  //
  // copy the triangle data to the vertex buffer
  //

  UINT8* dataBegin;
  mBufVerts->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
  memcpy(dataBegin, triangleVerts, sizeof(triangleVerts));
  mBufVerts->Unmap(0, nullptr);

  //
  // create vertex buffer view
  //

  mDescViewBufVert.BufferLocation = mBufVerts->GetGPUVirtualAddress();
  mDescViewBufVert.StrideInBytes = sizeof(VERTEX);
  mDescViewBufVert.SizeInBytes = sizeof(triangleVerts);
  */
}

ID3D12Heap* GpuDevice::heapCreationTest()
{
  ID3D12Heap* m_heap;

  D3D12_HEAP_DESC desc = {};
  desc.SizeInBytes = 1000*1000*128;
  // Alignment isnt supported apparently. Leave empty;
  //desc.Alignment = 16; // struct size?
  desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  desc.Properties.VisibleNodeMask = 1; // need to be visible as its upload buffer which is shared 
  desc.Properties.CreationNodeMask = 1;
  desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

  HRESULT hr = mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));
  if (FAILED(hr))
  {
    F_LOG("woot!\n");
  }
  else
  {
    F_LOG("A real heap now!\n");
  }
  return m_heap;
}
using namespace faze;
void GpuDevice::RunApiTestCoverage(std::string gpuDescription)
{
  TestWorks t(gpuDescription);
  t.addTest("layouts", [&]()
  {
    return true;
  });
  t.addTest("UploadHeap creation test", [&]()
  {
    ID3D12Heap* m_heap;

    D3D12_HEAP_DESC desc = {};
    desc.SizeInBytes = 1000 * 1000 * 128;
    // Alignment isnt supported apparently. Leave empty;
    //desc.Alignment = 16; // struct size?
    desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    desc.Properties.VisibleNodeMask = 1; // need to be visible as its upload buffer which is shared 
    desc.Properties.CreationNodeMask = 1;
    desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    HRESULT hr = mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));
    if (FAILED(hr))
    {
      F_LOG("woot!\n");
      return false;
    }
    return true;
  });
}
