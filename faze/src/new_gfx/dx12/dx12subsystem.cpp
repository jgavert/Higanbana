#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"
#include "faze/src/new_gfx/common/gpudevice.hpp"
#include "faze/src/new_gfx/definitions.hpp"

#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Subsystem::DX12Subsystem(const char* , unsigned , const char* , unsigned )
    {
      CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
    }

    std::string DX12Subsystem::gfxApi()
    {
      return "DX12"; 
    }

    vector<GpuInfo> DX12Subsystem::availableGpus()
    {
      UINT i = 0;
      IDXGIAdapter1* pAdapter;
      while (pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
      {
        IDXGIAdapter3* wanted;
        HRESULT hr = pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void **)&wanted);
        if (SUCCEEDED(hr))
        {
          vAdapters.push_back(wanted);
        }

        DXGI_ADAPTER_DESC desc;
        wanted->GetDesc(&desc);
        char ch[128];
        char DefChar = ' ';
        WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, ch, 128, &DefChar, NULL);

        GpuInfo info{};
        info.name = std::string(ch);
        info.vendor = VendorID::Unknown;
        if (desc.VendorId == 4098)
        {
            info.vendor = VendorID::Amd;
        }
        else if (desc.VendorId == 4318)
        {
            info.vendor = VendorID::Nvidia;
        }
        else if (desc.VendorId == 32902)
        {
            info.vendor = VendorID::Intel;
        }
        info.id = i;
        info.memory = static_cast<int64_t>(desc.DedicatedVideoMemory);
        info.type = DeviceType::Unknown;
        info.canPresent = true;
        infos.push_back(info);

        ++i;
      }

      return infos;
    }

    GpuDevice DX12Subsystem::createGpuDevice(FileSystem& , GpuInfo gpu)
    {
#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
      {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
          debugController->EnableDebugLayer();
#if defined(FAZE_GRAPHICS_GPUBASED_VALIDATION)
          ComPtr<ID3D12Debug1> debugController1;
          debugController.As(&debugController1);
          debugController1->SetEnableGPUBasedValidation(true);
          debugController1->SetEnableSynchronizedCommandQueueValidation(true);
#endif
        }
      }
#endif

      ID3D12Device* device;
      D3D_FEATURE_LEVEL createdLevel;
      constexpr D3D_FEATURE_LEVEL tryToEnable[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
      };

      int i = 0;
      while (i < 4)
      {
#if defined(FAZE_GRAPHICS_WARP_DRIVER)
        ComPtr<IDXGIAdapter1> pAdapter;
        pFactory->EnumWarpAdapter(IID_PPV_ARGS(pAdapter.GetAddressOf()));
        HRESULT hr = D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
#else
        HRESULT hr = D3D12CreateDevice(vAdapters[gpu.id], tryToEnable[i], IID_PPV_ARGS(&device));
#endif
        if (SUCCEEDED(hr))
        {
          createdLevel = tryToEnable[i];
          device->SetName(L"HwDevice");
#if defined(FAZE_GRAPHICS_WARP_DRIVER)
          device->SetName(L"Warp");
#endif
          break;
        }
        ++i;
      }


#if defined(FAZE_GRAPHICS_VALIDATION_LAYER)
      {
        {
          ComPtr<ID3D12InfoQueue> infoQueue;
          device->QueryInterface(IID_PPV_ARGS(infoQueue.GetAddressOf()));

          infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
          infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
          infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

          D3D12_MESSAGE_SEVERITY disabledSeverities[] = {
            D3D12_MESSAGE_SEVERITY_INFO,
          };
          D3D12_MESSAGE_ID disabledMessages[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED,
          };


          D3D12_INFO_QUEUE_FILTER filter = {};
          filter.DenyList.NumSeverities = 1;
          filter.DenyList.NumIDs = 3;
          filter.DenyList.pSeverityList = disabledSeverities;
          filter.DenyList.pIDList = disabledMessages;
          infoQueue->PushStorageFilter(&filter);
        }
      }
#endif

      std::shared_ptr<DX12Device> impl = std::shared_ptr<DX12Device>(new DX12Device(gpu, device),
        [device](DX12Device* ptr)
      {
        device->Release();
        delete ptr;
      });
      
      return GpuDevice(DeviceData(impl));
    }
  }
}
#endif