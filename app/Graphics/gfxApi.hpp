#pragma once

#include "GpuDevice.hpp"
#include "core/src/global_debug.hpp"
#include "ComPtr.hpp"
#include "gfxDebug.hpp"
#include <utility>
#include <string>
#include <DXGI.h>


struct GpuInfo
{
  std::string description;
  unsigned vendorId;
  unsigned deviceId;
  unsigned subSysId;
  unsigned revision;
  size_t dedVidMem;
  size_t dedSysMem;
  size_t shaSysMem;
};

class SystemDevices
{
private:
  ComPtr<IDXGIFactory4> pFactory;
  std::vector<IDXGIAdapter1*> vAdapters;
  std::vector<GpuInfo> infos;
  // 2 device support only
  int betterDevice;
  int lesserDevice;

public:
  SystemDevices() : betterDevice(-1), lesserDevice(-1)
  {
    UINT i = 0;
    CreateDXGIFactory1(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&pFactory));
    IDXGIAdapter1* pAdapter;
    while (pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
      vAdapters.push_back(pAdapter);
      ++i;
    }
    size_t betterDeviceMemory = 0;
    i = 0;
    for (auto&& it : vAdapters)
    {
      DXGI_ADAPTER_DESC desc;
      it->GetDesc(&desc);
      char ch[128];
      char DefChar = ' ';
      WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, ch, 128, &DefChar, NULL);
      /*
      F_LOG("%u. Device: %s\n", i, ch);
      F_LOG("Vendor id: %u\n", desc.VendorId);
      F_LOG("Device id: %u\n", desc.DeviceId);
      F_LOG("SubSys id: %u\n", desc.SubSysId);
      F_LOG("Revision : %u\n", desc.Revision);
      F_LOG("Memory\n", 2);
      F_LOG("DedicatedVideoMemory:  %zu\n", desc.DedicatedVideoMemory);
      F_LOG("DedicatedSystemMemory: %zu\n", desc.DedicatedSystemMemory);
      F_LOG("SharedSystemMemory:    %zu\n", desc.SharedSystemMemory);
      */
      if (desc.DedicatedVideoMemory > betterDeviceMemory)
      {
        betterDevice = i;
      }
      else
      {
        lesserDevice = i;
      }
      GpuInfo info;
      info.description = std::string(ch);
      info.vendorId = desc.VendorId;
      info.deviceId = desc.DeviceId;
      info.subSysId = desc.SubSysId;
      info.revision = desc.Revision;
      info.dedVidMem = desc.DedicatedVideoMemory;
      info.dedSysMem = desc.DedicatedSystemMemory;
      info.shaSysMem = desc.SharedSystemMemory;
      infos.push_back(info);
      i++;

    }
    if (betterDevice == -1)
    {
      betterDevice = lesserDevice;
    }
  }

  GpuDevice CreateGpuDevice(bool debug = false, bool warpDriver = false)
  {
#ifdef DEBUG
    debug = true;
#endif
    return CreateGpuDevice(betterDevice, debug, warpDriver);
  }

  GpuDevice CreateGpuDevice(int num, bool debug = false, bool warpDriver = false)
  {
    if (debug)
    {
      ComPtr<ID3D12Debug> debugController;
      if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), reinterpret_cast<void**>(debugController.addr()))))
      {
        debugController->EnableDebugLayer();
      }
    }
    ComPtr<ID3D12Device> device;
    HRESULT hr = D3D12CreateDevice(vAdapters[num], D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(device.addr()));
    if (FAILED(hr))
    {
      F_LOG("Device creation failed\n", 2);
    }
    else if (debug)
    {
      ComPtr<ID3D12DebugDevice> debugInterface;
      if (SUCCEEDED(device->QueryInterface(debugInterface.addr())))
      {
        debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
      }

      // configure infoqueue
      ComPtr<ID3D12InfoQueue> infoQueue;
      if (SUCCEEDED(device->QueryInterface(infoQueue.addr())))
      {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_SET_ROOT_CONSTANT_BUFFER_VIEW_INVALID, false);
        infoQueue->SetBreakOnID(D3D12_MESSAGE_ID_CREATE_CONSTANT_BUFFER_VIEW_INVALID_RESOURCE, false);
      }
    }
    return std::move(GpuDevice(device, debug));
  }

  GpuInfo getInfo(int num)
  {
    if (num < 0 || num >= infos.size())
      return{};
    return infos[num];
  }

  int DeviceCount()
  {
    return static_cast<int>(infos.size());
  }

};
