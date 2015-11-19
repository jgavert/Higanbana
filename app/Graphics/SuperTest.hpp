#pragma once

#include "Graphics/gfxApi.hpp"
#include <d3d12shader.h>
#include <D3Dcompiler.h>

class test
{
public:
  struct buf
  {
    int i;
    int k;
    int x;
    int y;
  };
  ID3D12Resource *UploadData;
  ID3D12Resource *GpuData;
  ID3D12Resource *GpuOutData;
  ID3D12Resource *readBackRes;
  ID3D12PipelineState* pipeline;
  ID3D12RootSignature* g_RootSig;
  ID3D12DescriptorHeap* descHeap;
  GpuFence fence;
  D3D12_GPU_DESCRIPTOR_HANDLE GpuView;
  D3D12_CPU_DESCRIPTOR_HANDLE GpuOutDataUAV;
  D3D12_CPU_DESCRIPTOR_HANDLE GpuDataSRV;
  buf* mappedArea;
  D3D12_RANGE range;
  D3D12_RANGE range2;
  test(GpuDevice& dev)
  {
    fence = dev.createFence();
    D3D12_RESOURCE_DESC datadesc = {};
    D3D12_HEAP_PROPERTIES heapprop = {};
    D3D12_RESOURCE_DESC readdesc = {};
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    buf* mappedArea;
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
      return;
    }

    // -------------------------------------------------------------------------
    // -----------------------------FILL UPLOAD HEAP----------------------------
    // -------------------------------------------------------------------------
    hr = UploadData->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
    if (FAILED(hr))
    {
      return;
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
      return;
    }

    datadesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = dev.mDevice->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &datadesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&GpuOutData));
    if (FAILED(hr))
    {
      return;
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
      return;
    }

    // -------------------------------------------------------------------------
    // --------------COMMAND PIPELINE CREATION START----------------------------
    // -------------------------------------------------------------------------
    ID3DBlob* blobCompute;
    ID3DBlob* errorMsg;
    hr = D3DCompileFromFile(L"compute.hlsl", nullptr, nullptr, "CSMain", "cs_5_0", 0, 0, &blobCompute, &errorMsg);
    // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
    if (FAILED(hr))
    {
      if (errorMsg)
      {
        OutputDebugStringA((char*)errorMsg->GetBufferPointer());
        errorMsg->Release();
      }
      return;
    }
    D3D12_SHADER_BYTECODE byte;
    byte.pShaderBytecode = blobCompute->GetBufferPointer();
    byte.BytecodeLength = blobCompute->GetBufferSize();
    // -------------------------------------------------------------------------
    // ------------------------ROOT SIGNATURE BEGINS----------------------------
    // -------------------------------------------------------------------------
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
      return;
    }

    hr = dev.mDevice->CreateRootSignature(
      0, blobSig->GetBufferPointer(), blobSig->GetBufferSize(),
      __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&g_RootSig));
    if (FAILED(hr))
    {
      return;
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

    
    hr = dev.mDevice->CreateComputePipelineState(&comPi, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
    if (FAILED(hr))
    {
      return;
    }
    // -------------------------------------------------------------------------
    // ------------------CREATE RESOURCE DESCRIPTORS-----------------------------
    // -------------------------------------------------------------------------
    // descriptor heap for shader view
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.NodeMask = 0;
    Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    Desc.NumDescriptors = 2;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = dev.mDevice->CreateDescriptorHeap(&Desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&descHeap));
    if (FAILED(hr))
    {
      return;
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

    GpuDataSRV = descHeap->GetCPUDescriptorHandleForHeapStart();
    uint32_t HandleIncrementSize = dev.mDevice->GetDescriptorHandleIncrementSize(Desc.Type);
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

    GpuOutDataUAV.ptr = GpuDataSRV.ptr + 1 * HandleIncrementSize;

    dev.mDevice->CreateUnorderedAccessView(GpuOutData, nullptr, &shaderUAV, GpuOutDataUAV);

    GpuView = descHeap->GetGPUDescriptorHandleForHeapStart();
  }
  void RunCompute(GpuDevice& dev, GpuCommandQueue& queue, GfxCommandList& list)
  {
    /*
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

    GpuDataSRV = descHeap->GetCPUDescriptorHandleForHeapStart();
    uint32_t HandleIncrementSize = dev.mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

    GpuOutDataUAV.ptr = GpuDataSRV.ptr + 1 * HandleIncrementSize;

    dev.mDevice->CreateUnorderedAccessView(GpuOutData, nullptr, &shaderUAV, GpuOutDataUAV);

    GpuView = descHeap->GetGPUDescriptorHandleForHeapStart();
    */
    // -------------------------------------------------------------------------
    // -------------------------Begin Commandlist-------------------------------
    // -------------------------------------------------------------------------
    // First begin with uploading initialdata to the gpu residing memory.
    list.m_CommandList->CopyResource(GpuData, UploadData);

    // Also change the DestData should be changed to COPY_SRC
    D3D12_RESOURCE_BARRIER barrierDesc = {};
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
    barrierDesc = {};
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
    dev.mCommandListAllocator->Reset();
    ////… if (FAILED(hr)) …
    list.m_CommandList->Reset(dev.mCommandListAllocator.get(), NULL);

    // -------------------------------------------------------------------------
    // Verify
    // -------------------------------------------------------------------------
    range2 = range;
    range2.End = 0;
    HRESULT hr = readBackRes->Map(0, &range, reinterpret_cast<void**>(&mappedArea));
    if (FAILED(hr))
    {
      return;
    }
    for (int i = 0;i < 1000 * 1000;++i)
    {
      if (mappedArea[i].i != static_cast<float>(i) + static_cast<float>(i))
      {
        readBackRes->Unmap(0, &range2);
        return;
      }
      if (mappedArea[i].k != 0)
      {
        readBackRes->Unmap(0, &range2);
        return;
      }
    }
    readBackRes->Unmap(0, &range2);
  }
};
