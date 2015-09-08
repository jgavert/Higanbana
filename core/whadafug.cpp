#pragma warning( disable : 4005 )
#include <windows.h>
#include <d3d12.h>
#include <DXGI1_4.h>
#include <cstdio>
#include <iostream>

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
  {
    PostQuitMessage(0);
    return 0;
  } break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain2(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{

  ID3D12Device* mDevice;
  /*HRESULT hr = D3D12CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, D3D12_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_1,
    D3D12_SDK_VERSION, __uuidof(ID3D12Device), reinterpret_cast<void**>(&mDevice));
    */
  HRESULT hr;
  if (FAILED(hr))
  {
    printf("Create device d3d12 failed\n");
    return 1;
  }
  printf("We have d3d12 device\n");
  ID3D12CommandAllocator* mCommandListAllocator;
  hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&mCommandListAllocator);
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ID3D12CommandQueue* m_CommandQueue;
  hr = mDevice->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&m_CommandQueue));
  if (FAILED(hr))
  {
    printf("Command queue creation failed\n");
    return 1;
  }
  printf("We have d3d12 commandqueue\n");

  ShowCursor(0);
  HWND hWnd;
  WNDCLASSEX wc;

  ZeroMemory(&wc, sizeof(WNDCLASSEX));

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.lpszClassName = "WindowClass";

  RegisterClassEx(&wc);

  RECT wr = { 0, 0, 800, 600 };
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
  hWnd = CreateWindowEx(NULL, "WindowClass", 0, WS_OVERLAPPEDWINDOW, 100, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);
  if (hWnd == NULL)
  {
    printf("wtf window null\n");
  }
  ShowWindow(hWnd, nCmdShow);

  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
  swapChainDesc.BufferCount = 2;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = hWnd;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; 

  IDXGIFactory2 *dxgiFactory = nullptr;
  hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), (void**)&dxgiFactory);
  if (FAILED(hr))
  {
    printf("CreateDXGIFactory2 failed\n");
    return 1;
  }
  IDXGISwapChain3* mSwapChain = nullptr;
  hr = dxgiFactory->CreateSwapChain(m_CommandQueue, &swapChainDesc, (IDXGISwapChain**)&mSwapChain);
  dxgiFactory->Release();
  if (FAILED(hr))
  {
    printf("CreateSwapChain failed\n");
    return 1;
  }
  printf("We got swapchain\n");
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
  /*
  devParams.hDeviceWindow = CreateWindow("static", 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0);
  if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, devParams.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &devParams, &d3dDevice)<0)
    return;
    
  ShowCursor(0);
  */
  /*
  //With the command list allocator and WITHOUT a PSO, you can create the actual command list, which will be executed at a later time.
  //This example shows calling ID3D12Device::CreateCommandList.
  ID3D12GraphicsCommandList* m_CommandList;
  hr = mDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandListAllocator, NULL, __uuidof(ID3D12CommandList), (void**)&m_CommandList);

  //create a GPU fence that will fire an event once the command list has been executed by the command queue.
  ID3D12Fence* mFence;
  mDevice->CreateFence(0, D3D12_FENCE_MISC_NONE, __uuidof(ID3D12Fence), (void**)&mFence);
  //And the CPU event that the fence will fire off
  HANDLE mHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);


  //Define the viewport that you will be rendering to.This example shows how to set the viewport to the same size as the Win32 window client.Also note that mViewPort is a member variable.
  //Whenever a command list is reset, you must attach the view port state to the command list before the command list is executed.
  //mViewPort will let you do so without needing to redefine it every frame.

  //get the client window area size.
  RECT clientSize;
  UINT width, height;
  GetClientRect(hWnd, &clientSize);
  width = clientSize.right;
  height = clientSize.bottom;

  //fill out a viewport struct
  D3D12_VIEWPORT mViewPort;
  ZeroMemory(&mViewPort, sizeof(D3D12_VIEWPORT));
  mViewPort.TopLeftX = 0;
  mViewPort.TopLeftY = 0;
  mViewPort.Width = (float)width;
  mViewPort.Height = (float)height;

  //This example shows calling ID3D12GraphicsCommandList::RSSetViewports.
  m_CommandList->RSSetViewports(1, &mViewPort);

  //***********************************************************************************
  //NOTE: You can omit the above call to RSSetViewPorts and the rest of this function, 
  //it is only here to illustrate the usage and resetting of a command list/queue/allocator.
  //**************************************************************************************

  //Close the command list and put it into the execution queue so that the framework can actually execute the run - one - time commands.
  //This example shows calling ID3D12GraphicsCommandList::Close and ID3D12CommandQueue::ExecuteCommandList.
  hr = m_CommandList->Close();
  m_CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&m_CommandList);

  //wait for GPU to signal it has finished processing the queued command list(s).
  //reset the fence signal
  mFence->Signal(0);
  //set the event to be fired once the signal value is 1
  mFence->SetEventOnCompletion(1, mHandle);

  //after the command list has executed, tell the GPU to signal the fence
  m_CommandQueue->Signal(mFence, 1);

  //wait for the event to be fired by the fence
  WaitForSingleObject(mHandle, INFINITE);

  //During set up, the member variable mCommandList was used to record and execute all of the set up commands.You can now reuse that member in the main render loop.
  //You must reset the command list allocator and the command list itself before you can reuse it.
  //In more advanced scenarios, one can reset the allocator every several frames.Memory is associated with the allocator that can't be released immediately after executing a command list. 
  //The best practice involves cycling through multiple allocators, but for simplicity we won't show that here.This example shows how to reset the allocator after every frame.
  //This example shows calling ID3D12CommandAllocator::Reset and ID3D12GraphicsCommandList::Reset.

  // Command list allocators can be only be reset when the associated command lists have finished execution on the GPU; 
  // apps should use fences to determine GPU execution progress.
  hr = mCommandListAllocator->Reset();
  //… if (FAILED(hr)) …
  hr = m_CommandList->Reset(mCommandListAllocator, NULL);
  //… if (FAILED(hr)) …

  MSG msg;

  while (TRUE)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT)
        break;
    }
    else
    {    HRESULT hr;

    //raise the red channel 0.01f per frame, reset at 1.0f
    static float red = 0.0f;
    red += 0.01f;
    if (red > 1.0f) red = 0.0f;

    //Get the index of the active back buffer from the swapchain
    UINT backBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    //Get active backbuffer's RTV from the manually built RTV descriptor array
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRenderTargetView[backBufferIndex];


    //Now, reuse the command list for the current frame.
    //Reattach the viewport to the command list, indicate that the resource will be in use as a render target, record commands, and then 
    //indicate that the render target will be used to present when the command list is done executing.

    //This example shows calling ID3D12GraphicsCommandList::ResourceBarrier to indicate to the system that you are about to use a resource.
    //Resource barriers are used to handle multiple accesses to a resource(refer to the Remarks for ResourceBarrier).
    //You have to explicitly state that mRenderTarget is about to be changed from being "used to present" to being "used as a render target".
    m_CommandList->RSSetViewports(1, &mViewPort);
    // Indicate that this resource will be in use as a render target.
    	D3D12_RESOURCE_BARRIER_DESC barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = mRenderTarget[backBufferIndex];
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_USAGE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_USAGE_RENDER_TARGET;

  m_CommandList->ResourceBarrier(1, &barrierDesc);

    // Record commands.
    float clearColor[] = { red, 0.2f, 0.4f, 1.0f };
    m_CommandList->ClearRenderTargetView(rtv, clearColor, NULL, 0);

    // Indicate that the render target will now be used to present when the command list is done executing.
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = mRenderTarget[backBufferIndex];
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_USAGE_RENDER_TARGET;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_USAGE_PRESENT;

    m_CommandList->ResourceBarrier(1, &barrierDesc);
    // Execute the command list.
    hr = m_CommandList->Close();
    m_CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&m_CommandList);

    //wait for GPU to signal it has finished processing the queued command list(s).
    mFence->Signal(0);
    //set the event to be fired once the signal value is 1
    mFence->SetEventOnCompletion(1, mHandle);

    //after the command list has executed, tell the GPU to signal the fence
    m_CommandQueue->Signal(mFence, 1);

    //wait for the event to be fired by the fence
    WaitForSingleObject(mHandle, INFINITE);

    // Command list allocators can be only be reset when the associated command lists have finished execution on the GPU; 
    // apps should use fences to determine GPU execution progress.
    hr = mCommandListAllocator->Reset();
    hr = m_CommandList->Reset(mCommandListAllocator, NULL);

    // Swap the back and front buffers.
    hr = mSwapChain->Present(1, 0);
    // Run game code here
    // ...
    // ...
    }
  }

  std::cin.get();
  */
  return 1;
}