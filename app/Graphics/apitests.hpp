#pragma once
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "Graphics/gfxApi.hpp"
#include "core/src/entity/database.hpp"
#include <cstdio>
#include <iostream>
#include "core/src/tests/TestWorks.hpp"

#include <d3d12shader.h>
#include <D3Dcompiler.h>

class ApiTests
{
private:
  void runTestsForDevice(std::string name, int id)
  {
    faze::TestWorks t(name);
    t.addTest("Device creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      D3D12_FEATURE_DATA_D3D12_OPTIONS opts = {};
      HRESULT hr = dev.mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opts, sizeof(opts));
      return !FAILED(hr);
    });

    t.addTest("Queue creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      return queue.m_CommandQueue.get() != nullptr;
    });

    t.addTest("CommandList creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GfxCommandList list = dev.createUniversalCommandList();
      return list.m_CommandList.get() != nullptr;
    });

    t.addTest("CreateCommittedResource Upload", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Resource *data;

      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8;
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      D3D12_HEAP_PROPERTIES heapprop = {};
      heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));

      return !FAILED(hr);
    });

    t.addTest("Map upload resource to program memory", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Resource *data;

      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8;
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      D3D12_HEAP_PROPERTIES heapprop = {};
      heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));
      if (FAILED(hr))
      {

        return false;
      }

      void* mappedArea;
      D3D12_RANGE range;
      range.Begin = 0;
      range.End = 1000 * 1000 * 8;
      hr = data->Map(0, &range, (void**)&mappedArea);

      return !FAILED(hr);
    });

    t.addTest("CreateCommittedResource", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Resource *data;

      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8;
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      D3D12_HEAP_PROPERTIES heapprop = {};
      heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));

      return !FAILED(hr);
    });


    t.addTest("Move data to upload heap and move to gpu memory[UNVERIFIED]", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      ID3D12Resource *data;

      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * sizeof(float);
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      D3D12_HEAP_PROPERTIES heapprop = {};
      heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));
      if (FAILED(hr))
      {

        return false;
      }

      float* mappedArea;
      D3D12_RANGE range;
      range.Begin = 0;
      range.End = 1000 * 1000 * sizeof(float);
      hr = data->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
      if (FAILED(hr))
      {

        return false;
      }
      try
      {
        for (int i = 0;i < 1000 * 1000;++i)
        {
          mappedArea[i] = static_cast<float>(i);
        }
      }
      catch (...)
      {
        return false;
      }
      data->Unmap(0, &range);

      ID3D12Resource *DestData;

      heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&DestData));
      if (FAILED(hr))
      {

        return false;
      }

      list.m_CommandList->CopyResource(DestData, data);
      GpuFence fence = dev.createFence();
      queue.submit(list);
      queue.insertFence(fence);
      fence.wait();

      return true;
    });

    t.addTest("GpuHeap creation", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Heap* m_heap;

      D3D12_HEAP_DESC desc = {};
      desc.SizeInBytes = 1000 * 1000 * 8;
      // Alignment isnt supported apparently. Leave empty;
      //desc.Alignment = 16; // struct size?
      desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
      desc.Properties.VisibleNodeMask = 0;
      desc.Properties.CreationNodeMask = 0;
      desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

      HRESULT hr = dev.mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));

      return !FAILED(hr);
    });

    t.addTest("Place resource in self created heap", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Heap* m_heap;
      ID3D12Resource *data;

      D3D12_HEAP_DESC desc = {};
      desc.SizeInBytes = 1000 * 1000 * 8;
      desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
      desc.Properties.VisibleNodeMask = 0;
      desc.Properties.CreationNodeMask = 0;
      desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

      HRESULT hr = dev.mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));
      D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8; // bytes!
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      hr = dev.mDevice->CreatePlacedResource(m_heap, 0, &datadesc, state, 0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));

      return !FAILED(hr);
    });

    t.addTest("Create Virtual Resource", [id]()
    {
      HRESULT hr;
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Resource *data;
      D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8;
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      hr = dev.mDevice->CreateReservedResource(&datadesc, state, 0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));

      return !FAILED(hr);
    });

    t.addTest("Allocate real memory from heap for virtual resource [UNVERIFIED]", [id]()
    {
      HRESULT hr;
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      ID3D12Resource *data;
      D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * 8;
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      hr = dev.mDevice->CreateReservedResource(&datadesc, state, 0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));

      if (FAILED(hr))
      {
        return false;
      }
      ID3D12Heap* m_heap;

      D3D12_HEAP_DESC desc = {};
      desc.SizeInBytes = 1000 * 1000 * 8;
      desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
      desc.Properties.VisibleNodeMask = 0;
      desc.Properties.CreationNodeMask = 0;
      desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

      hr = dev.mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));
      if (FAILED(hr))
      {
        return false;
      }
      D3D12_TILED_RESOURCE_COORDINATE coord = {0,0,0,0};
      D3D12_TILE_REGION_SIZE size = {};
      size.Width = 1000 * 1000 * 8;
      queue.m_CommandQueue->UpdateTileMappings(data, 0, &coord, &size, m_heap, 0, 0, 0, 0, D3D12_TILE_MAPPING_FLAG_NONE);
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();
      return true;
    });

    t.addTest("Upload and readback the same data", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      ID3D12Resource *data;
      ID3D12Resource *DestData;
      float* mappedArea;
      D3D12_RANGE range;
      D3D12_RESOURCE_DESC datadesc = {};
      D3D12_HEAP_PROPERTIES heapprop = {};
      ID3D12Resource *readBackRes;
      D3D12_RESOURCE_DESC readdesc = {};

      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * sizeof(float);
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

      heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;

      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));
      if (FAILED(hr))
      {
        return false;
      }

      range.Begin = 0;
      range.End = 1000 * 1000 * sizeof(float);
      hr = data->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
      for (int i = 0;i < 1000 * 1000;++i)
      {
        mappedArea[i] = static_cast<float>(i);
      }
      data->Unmap(0, &range);

      heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&DestData));
      if (FAILED(hr))
      {
        return false;
      }

      list.m_CommandList->CopyResource(DestData, data);

      // Also change the DestData should be changed to COPY_SRC
      D3D12_RESOURCE_BARRIER barrierDesc = {};
      barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrierDesc.Transition.pResource = DestData;
      barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      list.m_CommandList->ResourceBarrier(1, &barrierDesc);

      // Now DestData should hold the data
      // craft a readback buffer and check the data
      readdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      readdesc.Format = DXGI_FORMAT_UNKNOWN;
      readdesc.Width = 1000 * 1000 * sizeof(float);
      readdesc.Height = 1;
      readdesc.DepthOrArraySize = 1;
      readdesc.MipLevels = 1;
      readdesc.SampleDesc.Count = 1;
      readdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      heapprop.Type = D3D12_HEAP_TYPE_READBACK;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &readdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&readBackRes));
      if (FAILED(hr))
      {
        return false;
      }

      list.m_CommandList->CopyResource(readBackRes, DestData);
      queue.submit(list);
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();

      range.Begin = 0;
      range.End = 1000 * 1000 * sizeof(float);
      hr = readBackRes->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
      for (int i = 0;i < 1000 * 1000;++i)
      {
        if (mappedArea[i] != static_cast<float>(i))
        {
          return false;
        }
      }
      readBackRes->Unmap(0, &range);
      return true;
    });

    t.addTest("Create Resoure Descriptor", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Resource *data;

      struct buf
      {
        int i;
        int k;
      };

      D3D12_RESOURCE_DESC datadesc = {};
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * sizeof(buf);
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      D3D12_HEAP_PROPERTIES heapprop = {};
      heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&data));
      if (FAILED(hr))
      {
        return false;
      }
      // descriptor heap for shader view
      ID3D12DescriptorHeap* descHeap;
      D3D12_DESCRIPTOR_HEAP_DESC Desc;
      Desc.NodeMask = 0;
      Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      Desc.NumDescriptors = 1;
      Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      hr = dev.mDevice->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&descHeap));
      if (FAILED(hr))
      {
        return false;
      }
      D3D12_BUFFER_SRV bufferSRV;
      bufferSRV.FirstElement = 0;
      bufferSRV.NumElements = 1000 * 1000;
      bufferSRV.StructureByteStride = sizeof(buf);
      bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      D3D12_SHADER_RESOURCE_VIEW_DESC shaderSRV;
      shaderSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      shaderSRV.Buffer = bufferSRV;
      shaderSRV.Format = DXGI_FORMAT_UNKNOWN;
      shaderSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

      D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = descHeap->GetCPUDescriptorHandleForHeapStart();

      dev.mDevice->CreateShaderResourceView(data, &shaderSRV, cpuhandle);
      return true;
    });

    t.addTest("Compile a compute shader", [id]()
    {
      ID3DBlob* blobCompute;
      ID3DBlob* errorMsg;
      HRESULT hr = D3DCompileFromFile(L"compute.hlsl", nullptr, nullptr, "CSMain", "cs_5_0", 0, 0, &blobCompute, &errorMsg);
      // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
      if (FAILED(hr))
      {
        if (errorMsg)
        {
          OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          errorMsg->Release();
        }
      }
        
      return !FAILED(hr);
    });

    t.addTest("Create Root Descriptor", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);

      ID3D12RootSignature* g_RootSig;
      ID3DBlob* blobSig;
      ID3DBlob* errorSig;

      D3D12_DESCRIPTOR_RANGE r[2];
      r[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      r[0].BaseShaderRegister = 0;
      r[0].NumDescriptors = 1;
      r[0].RegisterSpace = 0;
      r[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      r[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      r[1].BaseShaderRegister = 0;
      r[1].NumDescriptors = 1;
      r[1].RegisterSpace = 0;
      r[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_DESCRIPTOR_TABLE rdt;
      rdt.NumDescriptorRanges = 2;
      rdt.pDescriptorRanges = r;

      D3D12_ROOT_PARAMETER rP;
      rP.DescriptorTable = rdt;
      rP.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rP.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      D3D12_ROOT_SIGNATURE_DESC descRootSignature;
      ZeroMemory(&descRootSignature, sizeof(descRootSignature));
      descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      descRootSignature.NumParameters = 1;
      descRootSignature.pParameters = &rP; // API SLOT 0

      HRESULT hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &blobSig, &errorSig);
      if (FAILED(hr))
      {
        return false;
      }

      hr = dev.mDevice->CreateRootSignature(
        1, blobSig->GetBufferPointer(), blobSig->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&g_RootSig));
      return !FAILED(hr);
    });

    t.addTest("Create compute Pipeline", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);

      ID3DBlob* blobCompute;
      ID3DBlob* errorMsg;
      HRESULT hr = D3DCompileFromFile(L"compute.hlsl", nullptr, nullptr, "CSMain", "cs_5_0", 0, 0, &blobCompute, &errorMsg);
      // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
      if (FAILED(hr))
      {
        if (errorMsg)
        {
          OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          errorMsg->Release();
        }
        return false;
      }
      D3D12_SHADER_BYTECODE byte;
      byte.pShaderBytecode = blobCompute->GetBufferPointer();
      byte.BytecodeLength = blobCompute->GetBufferSize();

      // -------------------ROOT SIGNATURE BEGINS-------------------
      ID3D12RootSignature* g_RootSig;
      ID3DBlob* blobSig;
      ID3DBlob* errorSig;
      D3D12_DESCRIPTOR_RANGE r[2];
      r[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      r[0].BaseShaderRegister = 0;
      r[0].NumDescriptors = 1;
      r[0].RegisterSpace = 0;
      r[0].OffsetInDescriptorsFromTableStart = 0;

      r[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      r[1].BaseShaderRegister = 0;
      r[1].NumDescriptors = 1;
      r[1].RegisterSpace = 0;
      r[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_DESCRIPTOR_TABLE rdt;
      rdt.NumDescriptorRanges = 2;
      rdt.pDescriptorRanges = r;

      D3D12_ROOT_PARAMETER rP;
      rP.DescriptorTable = rdt;
      rP.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rP.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      D3D12_ROOT_SIGNATURE_DESC descRootSignature;
      ZeroMemory(&descRootSignature, sizeof(descRootSignature));
      descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      descRootSignature.NumParameters = 1;
      descRootSignature.pParameters = &rP; // API SLOT 0
      hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1,
        &blobSig, &errorSig);
      if (FAILED(hr))
      {
        return false;
      }

      hr = dev.mDevice->CreateRootSignature(
        0, blobSig->GetBufferPointer(), blobSig->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&g_RootSig));
      if (FAILED(hr))
      {
        return false;
      }

      // ------------------CREATE PIPELINE------------------------------
      D3D12_COMPUTE_PIPELINE_STATE_DESC comPi;
      ZeroMemory(&comPi, sizeof(comPi));
      comPi.CS = byte;
      comPi.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
      comPi.NodeMask = 0;
      comPi.pRootSignature = g_RootSig;

      ID3D12PipelineState* pipeline;
      hr = dev.mDevice->CreateComputePipelineState(&comPi, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
      return !FAILED(hr);
    });

    // when this test passes, wrappers should be written for all the above.
    // preferable a same set of tests but written with the wrappers, see where we can do better and cleaner.
    // Also improves debugging to compare raw working api calls to the wrappers if there are faults.
    // hence this should be improved first before doing wrappers.
    
    t.addTest("Modify data with compute shader", [id]()
    {
      // holy fuck... err Initialize phase
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      // lots of shit
      ID3D12Resource *UploadData;
      ID3D12Resource *GpuData;
      ID3D12Resource *GpuOutData;
      D3D12_RESOURCE_DESC datadesc = {};
      D3D12_HEAP_PROPERTIES heapprop = {};
      ID3D12Resource *readBackRes;
      D3D12_RESOURCE_DESC readdesc = {};
      D3D12_RESOURCE_BARRIER barrierDesc = {};

      // DATA RANGE
      struct buf
      {
        int i;
        int k;
      };
      buf* mappedArea;
      D3D12_RANGE range;
      range.Begin = 0;
      range.End = 1000 * 1000 * sizeof(buf);
      // -------------------------------------------------------------------------
      // ----------------UPLOAD HEAP CREATION AND FILL----------------------------
      // -------------------------------------------------------------------------
      datadesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      datadesc.Format = DXGI_FORMAT_UNKNOWN;
      datadesc.Width = 1000 * 1000 * sizeof(buf);
      datadesc.Height = 1;
      datadesc.DepthOrArraySize = 1;
      datadesc.MipLevels = 1;
      datadesc.SampleDesc.Count = 1;
      datadesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

      heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;

      HRESULT hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&UploadData));
      if (FAILED(hr))
      {
        return false;
      }

      // -------------------------------------------------------------------------
      // -----------------------------FILL UPLOAD HEAP----------------------------
      // -------------------------------------------------------------------------
      hr = UploadData->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
      if (FAILED(hr))
      {
        return false;
      }
      for (int i = 0;i < 1000 * 1000;++i)
      {
        mappedArea[i].i = i;
        mappedArea[i].k = i;
      }
      UploadData->Unmap(0, &range);
      // -------------------------------------------------------------------------
      // -----------------------------GPU RESIDING BUFFERS------------------------
      // -------------------------------------------------------------------------
      heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&GpuData));
      if (FAILED(hr))
      {
        return false;
      }

      datadesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&GpuOutData));
      if (FAILED(hr))
      {
        return false;
      }

      // -------------------------------------------------------------------------
      // -------------------------READBACK RESOURCE CREATION----------------------
      // -------------------------------------------------------------------------
      // Now DestData should hold the data
      // craft a readback buffer and check the data
      readdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      readdesc.Format = DXGI_FORMAT_UNKNOWN;
      readdesc.Width = 1000 * 1000 * sizeof(buf);
      readdesc.Height = 1;
      readdesc.DepthOrArraySize = 1;
      readdesc.MipLevels = 1;
      readdesc.SampleDesc.Count = 1;
      readdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      heapprop.Type = D3D12_HEAP_TYPE_READBACK;
      hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &readdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&readBackRes));
      if (FAILED(hr))
      {
        return false;
      }

      // -------------------------------------------------------------------------
      // --------------COMMAND PIPELINE CREATION START----------------------------
      // -------------------------------------------------------------------------
      ID3DBlob* blobCompute;
      ID3DBlob* errorMsg;
      hr = D3DCompileFromFile(L"compute.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, &blobCompute, &errorMsg);
      // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
      if (FAILED(hr))
      {
        if (errorMsg)
        {
          OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          errorMsg->Release();
        }
        return false;
      }
      D3D12_SHADER_BYTECODE byte;
      byte.pShaderBytecode = blobCompute->GetBufferPointer();
      byte.BytecodeLength = blobCompute->GetBufferSize();

      // -------------------------------------------------------------------------
      // ------------------------ROOT SIGNATURE BEGINS----------------------------
      // -------------------------------------------------------------------------
      ID3D12RootSignature* g_RootSig;
      ID3DBlob* blobSig;
      ID3DBlob* errorSig;
      D3D12_DESCRIPTOR_RANGE r[2];
      r[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      r[0].BaseShaderRegister = 0;
      r[0].NumDescriptors = 1;
      r[0].RegisterSpace = 0;
      r[0].OffsetInDescriptorsFromTableStart = 0;

      r[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      r[1].BaseShaderRegister = 0;
      r[1].NumDescriptors = 1;
      r[1].RegisterSpace = 0;
      r[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_DESCRIPTOR_TABLE rdt;
      rdt.NumDescriptorRanges = 2;
      rdt.pDescriptorRanges = r;

      D3D12_ROOT_PARAMETER rP;
      rP.DescriptorTable = rdt;
      rP.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rP.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      D3D12_ROOT_SIGNATURE_DESC descRootSignature;
      ZeroMemory(&descRootSignature, sizeof(descRootSignature));
      descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      descRootSignature.NumParameters = 1;
      descRootSignature.pParameters = &rP; // API SLOT 0
      hr = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1,
        &blobSig, &errorSig);
      if (FAILED(hr))
      {
        return false;
      }

      hr = dev.mDevice->CreateRootSignature(
        0, blobSig->GetBufferPointer(), blobSig->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&g_RootSig));
      if (FAILED(hr))
      {
        return false;
      }
  
      // -------------------------------------------------------------------------
      // --------------------COMPUTE PIPELINE FINISH------------------------------
      // -------------------------------------------------------------------------
      D3D12_COMPUTE_PIPELINE_STATE_DESC comPi;
      ZeroMemory(&comPi, sizeof(comPi));
      comPi.CS = byte;
      comPi.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
      comPi.NodeMask = 0;
      comPi.pRootSignature = g_RootSig;

      ID3D12PipelineState* pipeline;
      hr = dev.mDevice->CreateComputePipelineState(&comPi, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
      if (FAILED(hr))
      {
        return false;
      }
      // -------------------------------------------------------------------------
      // ------------------CREATE RESOURCE DESCRIPTORS-----------------------------
      // -------------------------------------------------------------------------
      // descriptor heap for shader view
      ID3D12DescriptorHeap* descHeap;
      D3D12_DESCRIPTOR_HEAP_DESC Desc;
      Desc.NodeMask = 0;
      Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      Desc.NumDescriptors = 2;
      Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      hr = dev.mDevice->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&descHeap));
      if (FAILED(hr))
      {
        return false;
      }
      // Actual descriptors
      D3D12_BUFFER_SRV bufferSRV;
      bufferSRV.FirstElement = 0;
      bufferSRV.NumElements = 1000 * 1000;
      bufferSRV.StructureByteStride = sizeof(buf);
      bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      D3D12_SHADER_RESOURCE_VIEW_DESC shaderSRV;
      shaderSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      shaderSRV.Buffer = bufferSRV;
      shaderSRV.Format = DXGI_FORMAT_UNKNOWN;
      shaderSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

      D3D12_CPU_DESCRIPTOR_HANDLE GpuDataSRV = descHeap->GetCPUDescriptorHandleForHeapStart();
      size_t HandleIncrementSize = dev.mDevice->GetDescriptorHandleIncrementSize(Desc.Type);
      dev.mDevice->CreateShaderResourceView(GpuData, &shaderSRV, GpuDataSRV);

      D3D12_BUFFER_UAV bufferUAV;
      bufferUAV.FirstElement = 0;
      bufferUAV.NumElements = 1000 * 1000;
      bufferUAV.StructureByteStride = sizeof(buf);
      bufferUAV.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
      bufferUAV.CounterOffsetInBytes = 0;

      D3D12_UNORDERED_ACCESS_VIEW_DESC shaderUAV;
      shaderUAV.Buffer = bufferUAV;
      shaderUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      shaderUAV.Format = DXGI_FORMAT_UNKNOWN;


      D3D12_CPU_DESCRIPTOR_HANDLE GpuOutDataUAV;
      GpuOutDataUAV.ptr = GpuDataSRV.ptr + 1 * HandleIncrementSize;

      dev.mDevice->CreateUnorderedAccessView(GpuOutData,nullptr, &shaderUAV, GpuOutDataUAV);

      D3D12_GPU_DESCRIPTOR_HANDLE GpuView = descHeap->GetGPUDescriptorHandleForHeapStart();
      // -------------------------------------------------------------------------
      // -------------------------Begin Commandlist-------------------------------
      // -------------------------------------------------------------------------
      // First begin with uploading initialdata to the gpu residing memory.
      list.m_CommandList->CopyResource(GpuData, UploadData);

      // Also change the DestData should be changed to COPY_SRC
      barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrierDesc.Transition.pResource = GpuData;
      barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      list.m_CommandList->ResourceBarrier(1, &barrierDesc);
      // bind compute pipeline
      list.m_CommandList->SetComputeRootSignature(g_RootSig);
      list.m_CommandList->SetPipelineState(pipeline);
      // bind descriptors
      list.m_CommandList->SetDescriptorHeaps(1, &descHeap);
      list.m_CommandList->SetComputeRootDescriptorTable(0, GpuView);
      // launch compute
      list.m_CommandList->Dispatch(1000 * 1000 / 50, 1, 1);
      // resource barrier
      barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrierDesc.Transition.pResource = GpuOutData;
      barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      list.m_CommandList->ResourceBarrier(1, &barrierDesc);
      // copy back the resource
      list.m_CommandList->CopyResource(readBackRes, GpuOutData);

      // -------------------------------------------------------------------------
      // ---------------------------end commandlist-------------------------------
      // -------------------------------------------------------------------------
      queue.submit(list);
      
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();

      // -------------------------------------------------------------------------
      // Verify
      // -------------------------------------------------------------------------
      hr = readBackRes->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
      if (FAILED(hr))
      {
        return false;
      }
      for (int i = 0;i < 1000 * 1000;++i)
      {
        if (mappedArea[i].i != static_cast<float>(i) + static_cast<float>(i))
        {
          return false;
        }
        if (mappedArea[i].k != 0)
        {
          return false;
        }
      }
      readBackRes->Unmap(0, &range);
      return true;
    });
    
    // this is actually already a miracle...
    t.addTest("Modify data with compute shader(less than 150lines)", [id]()
    {
      return false;
    });

    t.addTest("Modify data with compute shader(less than 100lines)", [id]()
    {
      return false;
    });

    // this should be darn good as there is initializations also involved
    t.addTest("Modify data with compute shader(less than 50lines)", [id]()
    {
      return false;
    });
    
    t.addTest("UploadHeap creation", [id]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ID3D12Heap* m_heap;

      D3D12_HEAP_DESC desc = {};
      desc.SizeInBytes = 1000 * 1000 * 8;
      // Alignment isnt supported apparently. Leave empty;
      //desc.Alignment = 16; // struct size?
      desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
      desc.Properties.VisibleNodeMask = 1; // need to be visible as its upload buffer which is shared 
      desc.Properties.CreationNodeMask = 1;
      desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

      HRESULT hr = dev.mDevice->CreateHeap(&desc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&m_heap));
      return !FAILED(hr);
    });



    t.runTests();
  }

public:
  void run()
  {
    SystemDevices sys;
    for (int i = 0; i < sys.DeviceCount(); ++i)
    {
      runTestsForDevice(sys.getInfo(i).description, i);
    }
  }
};