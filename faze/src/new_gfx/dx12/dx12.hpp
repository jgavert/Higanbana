#pragma once
#include <DXGI.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <wrl.h>
#include <Objbase.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using D3D12Swapchain = IDXGISwapChain4;