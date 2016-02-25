#pragma once
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/tests/TestWorks.hpp"

#include "Graphics/gfxApi.hpp"
#include <cstdio>
#include <iostream>

#if defined(GFX_D3D12)
#include <d3d12shader.h>
#include <D3Dcompiler.h>
#endif


#pragma warning( push )
#pragma warning( disable : 4189 ) // don't really want warnings from "local variable is initialized but not referenced"
#pragma warning( disable : 4702 )

class ApiTests2
{
private:
  void runTestsForDevice(std::string name, int id, ProgramParams params)
  {
    faze::TestWorks t(name);
    
    t.addTest("Device creation", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      return dev.isValid();
    });

    t.addTest("Queue creation", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      return queue.isValid();
    });

    t.addTest("CommandList creation", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GfxCommandList list = dev.createUniversalCommandList();
      return list.isValid();
    });

    t.addTest("Map upload resource to program memory", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBuffer(ResourceDescriptor()
        .Width(1)
        .Format<float>()
        .Usage(ResourceUsage::UploadHeap));

      auto a = buf.Map<float>();
      a[0] = 1.f;
      return a[0] != 0.f;
    });

    t.addTest("Create Gpu resident buffer", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBuffer(ResourceDescriptor()
        .Width(4096)
        .Format<float>());
      return buf.isValid();
    });

    t.addTest("Create Gpu resident buffer (new)", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBuffer(ResourceDescriptor()
                    .Width(10));
      return buf.isValid();
    });

    t.addTest("Create Gpu resident texture (new)", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto tex = dev.createTexture(ResourceDescriptor()
                .Width(1024)
                .Height(1024)
                .Format(R8G8B8A8_UNORM));
      return tex.isValid();
    });

    t.addTest("Move data to upload heap and move to gpu memory (new resource)", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();

      auto srcdata = dev.createBuffer(ResourceDescriptor()
          .Width(4096)
          .Format<float>()
          .Usage(ResourceUsage::UploadHeap));
      auto dstdata = dev.createBuffer(ResourceDescriptor()
        .Width(4096)
        .Format<float>()
        .Usage(ResourceUsage::GpuOnly)); 

      {
        auto tmp = srcdata.Map<float>();
        for (size_t i = tmp.rangeBegin(); i < tmp.rangeEnd(); ++i)
        {
          tmp.get()[i] = static_cast<float>(i);
        }
      }

      list.CopyResource(dstdata, srcdata);
      list.closeList();
      queue.submit(list);
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();

      return true;
    });

    t.addTest("Upload and readback the same data", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      GpuFence fence = dev.createFence();

      auto srcdata = dev.createBuffer(ResourceDescriptor()
        .Width(4096)
        .Format<float>()
        .Usage(ResourceUsage::UploadHeap));
      auto dstdata = dev.createBuffer(ResourceDescriptor()
        .Width(4096)
        .Format<float>()
        .Usage(ResourceUsage::GpuOnly));
      auto rbdata = dev.createBuffer(ResourceDescriptor()
        .Width(4096)
        .Format<float>()
        .Usage(ResourceUsage::ReadbackHeap));

      {
        auto tmp = srcdata.Map<float>();
        for (size_t i = tmp.rangeEnd();i < tmp.rangeEnd(); ++i)
        {
          tmp[i] = static_cast<float>(i);
        }
      }

      list.CopyResource(dstdata, srcdata);
      list.CopyResource(rbdata, dstdata);
      list.closeList();
      queue.submit(list);
      queue.insertFence(fence);
      fence.wait();
      {
        auto woot = rbdata.Map<float>();
        for (size_t i = woot.rangeBegin();i < woot.rangeEnd(); ++i)
        {
          if (woot[i] != static_cast<float>(i))
          {
            return false;
          }
        }
        return true;
      }
    });



    // TODO: shit, using some interfaces straigh. Needs to be prettyfied and seems like
    // doing dynamic reflection might give the best results. Atleast easiest.
    // Maybe compare root signature between shaders or just define some "extract_rootsignature.hlsl" thats included to every file.
    // Honestly sounds fine, just the problem of creating "root signature" object that knows what to put and where needed to be done.
#if defined(GFX_D3D12)
    t.addTest("shader root signature (DISABLED)", [&]()
    {
		return false;
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      FazCPtr<ID3DBlob> blobCompute;
      FazCPtr<ID3DBlob> errorMsg;
      HRESULT hr = D3DCompileFromFile(L"compute_1", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, blobCompute.addr(), errorMsg.addr());
      // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
      if (FAILED(hr))
      {
        if (errorMsg.get())
        {
          OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          errorMsg->Release();
        }
        return false;
      }
      FazCPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        return false;
      }
      FazCPtr<ID3D12RootSignature> g_RootSig;
      FazCPtr<ID3DBlob> blobSig;
      FazCPtr<ID3DBlob> errorSig;
      const D3D12_ROOT_SIGNATURE_DESC* woot = asd->GetRootSignatureDesc();

      hr = D3D12SerializeRootSignature(woot, D3D_ROOT_SIGNATURE_VERSION_1, blobSig.addr(), errorSig.addr());
      if (FAILED(hr))
      {
        return false;
      }

      hr = dev.m_device->CreateRootSignature(
        1, blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(),
        __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(g_RootSig.addr()));
      return !FAILED(hr);
    });
    t.addTest("shader root signature mirror structure (DISABLED)", [&]()
    {
      return false;
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      FazCPtr<ID3DBlob> blobCompute;
      FazCPtr<ID3DBlob> errorMsg;
      HRESULT hr = D3DCompileFromFile(L"compute_1", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, blobCompute.addr(), errorMsg.addr());
      // https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
      if (FAILED(hr))
      {
        if (errorMsg.get())
        {
          OutputDebugStringA((char*)errorMsg->GetBufferPointer());
          errorMsg->Release();
        }
        return false;
      }
      FazCPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        return false;
      }
      const D3D12_ROOT_SIGNATURE_DESC* woot = asd->GetRootSignatureDesc();
      //F_LOG("\n");
      for (unsigned int i = 0;i < woot->NumParameters;++i)
      {
        auto it = woot->pParameters[i];
        switch (it.ParameterType)
        {
          
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
          //F_LOG("DescTable:\n", it.DescriptorTable);
          for (unsigned int k = 0; k < it.DescriptorTable.NumDescriptorRanges; ++k)
          {
            auto it2 = it.DescriptorTable.pDescriptorRanges[k];
            //F_LOG("\tRangeType: ");
            switch (it2.RangeType)
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
              //F_LOG("SRV ");
              break; 
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
              //F_LOG("UAV ");
              break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
              //F_LOG("CBV ");
              break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
              //F_LOG("Sampler ");
              break;
            default:
              break;
            }
            //F_LOG("NumDescriptors: %u, RegisterSpace: %u, BaseShaderRegister:%u", it2.NumDescriptors, it2.RegisterSpace, it2.BaseShaderRegister);
            //F_LOG("\n");
          }
          break;
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
          //F_LOG("32Bit constant Num32BitValues:%u RegisterSpace:%u ShaderRegister:%u\n", it.Constants.Num32BitValues, it.Constants.RegisterSpace, it.Constants.ShaderRegister);
          break;
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
          //F_LOG("CBV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
          break;
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
          //F_LOG("SRV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
          break;
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
          //F_LOG("UAV RegisterSpace:%u ShaderRegister:%u\n", it.Descriptor.RegisterSpace, it.Descriptor.ShaderRegister);
          break;
        default:
          break;
        }
      }
      //F_LOG("\n");
      return true;
    });
#endif

    t.addTest("Create compute Pipeline", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("compute_1"));
      return true;
    });


    t.addTest("Pipeline binding and modify data in compute (sub 50 lines!) (new resources)", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      GpuFence fence = dev.createFence();

      ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("compute_1"));

      struct buf
      {
        int i;
        int k;
        int x;
        int y;
      };
      auto srcdata = dev.createBuffer(ResourceDescriptor()
        .Width(1000 * 1000)
        .Format<buf>()
        .Usage(ResourceUsage::UploadHeap));
      auto dstdata = dev.createBuffer(ResourceDescriptor()
        .Width(1000 * 1000)
        .Format<buf>());
      auto completedata = dev.createBuffer(ResourceDescriptor()
        .Width(1000 * 1000)
        .Format<buf>()
        .enableUnorderedAccess());
      auto rbdata = dev.createBuffer(ResourceDescriptor()
        .Width(1000 * 1000)
        .Format<buf>()
        .Usage(ResourceUsage::ReadbackHeap));

      auto dstdataSRV = dev.createBufferSRV(dstdata);
      auto completedataUAV = dev.createBufferUAV(completedata);

      {
        auto tmp = srcdata.Map<buf>();
        for (size_t i = tmp.rangeBegin(); i < tmp.rangeEnd(); ++i)
        {
          tmp[i].i = static_cast<int>(i);
          tmp[i].k = static_cast<int>(i);
        }
      }

      list.CopyResource(dstdata, srcdata);
      auto bind = list.bind(pipeline);
      bind.SRV(0, dstdataSRV);
      bind.UAV(0, completedataUAV);
      unsigned int shaderGroup = 64;
      unsigned int inputSize = 1000 * 1000;
      list.Dispatch(bind, inputSize / shaderGroup, 1, 1);

      list.CopyResource(rbdata, completedata);
      list.closeList();
      queue.submit(list);
      queue.insertFence(fence);
      fence.wait();

      auto mapd = rbdata.Map<buf>();
      for (size_t i = mapd.rangeBegin(); i < mapd.rangeEnd(); ++i)
      {
        auto& obj = mapd[i];
        if (obj.i != i + i)
        {
          return false;
        }
        if (obj.k != i)
        {
          return false;
        }
      }
      return true;
    });

    t.addTest("Create TextureSRV", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createTextureSRV(
        Dimension(800,600)
        , Format<int>(FormatType::R8G8B8A8_UNORM)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
      return buf.isValid();
    });

    t.addTest("Create TextureUAV", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createTextureUAV(
        Dimension(800, 600)
        , Format<int>(FormatType::R8G8B8A8_UNORM)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
      return buf.isValid();
    });

    t.addTest("Create TextureRTV", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createTextureRTV(
        Dimension(800, 600)
        , Format<int>(FormatType::R8G8B8A8_UNORM)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
      return buf.isValid();
    });

    t.addTest("Create TextureDSV", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createTextureDSV(
        Dimension(800, 600)
        , Format<int>(FormatType::D32_FLOAT)
        , ResUsage::Gpu
        , MipLevel()
        , Multisampling());
      return buf.isValid();
    });

    t.addTest("Create Graphics Pipeline", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);

      auto woot = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("pixel")
        .VertexShader("vertex")
        .DSVFormat(FormatType::D32_FLOAT)
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB));

      return woot.valid();
    });

    t.addTest("Create SwapChain", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();

      Window window(params, "kek", 800, 600);
      window.open();
      SwapChain sc = dev.createSwapChain(queue, window);
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        if (msg.message == WM_QUIT)
          continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      return sc.valid();
    });

    t.addTest("Create window and render(?) for full 1 second in loop", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();

      Window window(params, "ebin", 800, 600);
      window.open();
      SwapChain sc = dev.createSwapChain(queue, window);

      ViewPort port(800, 600);
      auto timepoint2 = std::chrono::high_resolution_clock::now();
      float i = 0;
      auto vec = faze::vec4({ i, 0.2f, 0.2f, 1.0f });
      auto limit = std::chrono::seconds(1).count();

      auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
      while (timeSince < limit)
      {
        timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        window.simpleReadMessages();
        GpuFence fence = dev.createFence();
        list.setViewPort(port);
        UINT backBufferIndex = sc.GetCurrentBackBufferIndex();
        vec[0] += 0.02f;
        if (vec[0] > 1.0f)
          vec[0] = 0.f;
        list.ClearRenderTargetView(sc[backBufferIndex], vec);
        list.closeList();
        queue.submit(list);

        sc.present(1, 0);
        queue.insertFence(fence);
        fence.wait();
        list.resetList();
      }
      window.close();

      return true;
    });
    
    t.addTest("Create window and render a triangle for full 1 second in loop", [&]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList gfx = dev.createUniversalCommandList();

      Window window(params, "ebin", 800, 600);
      window.open();
      SwapChain sc = dev.createSwapChain(queue, window);
      ViewPort port(800, 600);

      struct buf
      {
        float pos[4];
      };
	  auto srcdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<buf>()
		  .Usage(ResourceUsage::UploadHeap));
	  auto dstdata = dev.createBuffer(ResourceDescriptor()
		  .Width(6)
		  .Format<buf>()
		  .Usage(ResourceUsage::GpuOnly));

	  auto dstdataSrv = dev.createBufferSRV(dstdata);

      {
        auto tmp = srcdata.Map<buf>();
        float size = 0.5f;
        tmp[0] = { size, size, 0.f, 1.f };
        tmp[1] = { size, -size, 0.f, 1.f };
        tmp[2] = { -size, size, 0.f, 1.f };
        tmp[3] = { -size, size, 0.f, 1.f };
        tmp[4] = { size, -size, 0.f, 1.f };
        tmp[5] = { -size, -size, 0.f, 1.f };
      }

      gfx.CopyResource(dstdata, srcdata);
      GpuFence fence = dev.createFence();
      gfx.closeList();
      queue.submit(gfx);
      queue.insertFence(fence);

      auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
        .PixelShader("pixel")
        .VertexShader("vertex")
        .setRenderTargetCount(1)
        .RTVFormat(0, FormatType::R8G8B8A8_UNORM)
        .DepthStencil(DepthStencilDescriptor().DepthEnable(false)));

      auto vec = faze::vec4({ 0.2f, 0.2f, 0.2f, 1.0f });

      auto limit = std::chrono::seconds(1).count();
      auto timepoint2 = std::chrono::high_resolution_clock::now();
      auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
      while (timeSince < limit)
      {
        timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - timepoint2).count();
        if (window.simpleReadMessages())
          break;
        fence.wait();
        gfx.resetList();


        // Rendertarget
        gfx.setViewPort(port);
        vec[2] += 0.02f;
        if (vec[2] > 1.0f)
          vec[2] = 0.f;
        auto backBufferIndex = sc.GetCurrentBackBufferIndex();
        gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
        gfx.setRenderTarget(sc[backBufferIndex]);
        // graphics begin
        auto bind = gfx.bind(pipeline);
        bind.SRV(0, dstdataSrv);
        gfx.drawInstanced(bind, 6, 1, 0, 0);


        // submit all
		    gfx.preparePresent(sc[backBufferIndex]);
        gfx.closeList();
        queue.submit(gfx);

        // present
        sc.present(1, 0);
        queue.insertFence(fence);
      }
      fence.wait();
      window.close();

      return true;
    });
  
	  t.addTest("Pipeline binding and modify data in compute with root constant", [id]()
	  {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
		  GpuDevice dev = sys.CreateGpuDevice(id);
		  GpuCommandQueue queue = dev.createQueue();
		  GfxCommandList list = dev.createUniversalCommandList();
		  GpuFence fence = dev.createFence();

		  ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("tests/compute_rootconstant"));

		  struct buf
		  {
			  int i;
			  int k;
			  int x;
			  int y;
		  };

		  auto completedata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::GpuOnly)
			  .enableUnorderedAccess());
		  auto rbdata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::ReadbackHeap));

		  auto completedataUav = dev.createBufferUAV(completedata);

		  auto bind = list.bind(pipeline);
		  bind.rootConstant(0, 1337);
		  bind.UAV(0, completedataUav);
		  list.Dispatch(bind, 1, 1, 1);

		  list.CopyResource(rbdata, completedata);
		  list.closeList();
		  queue.submit(list);
		  queue.insertFence(fence);
		  fence.wait();

		  auto mapd = rbdata.Map<buf>();
		  auto& obj = mapd[0];
		  return (obj.k == 1337);
	  });
    
    /*
	  t.addTest("Pipeline binding and modify data in compute with variable UAV (doesnt work)", [id]()
	  {
		  //return false;
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
		  GpuDevice dev = sys.CreateGpuDevice(id);
		  GpuCommandQueue queue = dev.createQueue();
		  GfxCommandList list = dev.createUniversalCommandList();
		  GpuFence fence = dev.createFence();

		  ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("tests/compute_rootconstant_ver2"));

		  struct buf
		  {
			  int i;
			  int k;
			  int x;
			  int y;
		  };

		  auto completedata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .enableUnorderedAccess());
		  auto completedata2 = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .enableUnorderedAccess());
		  auto completedata3 = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .enableUnorderedAccess());
		  auto completedataUav = dev.createBufferUAV(completedata);
		  auto completedataUav2 = dev.createBufferUAV(completedata2);
		  auto completedataUav3 = dev.createBufferUAV(completedata3);

		  auto rbdata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::ReadbackHeap));
		  auto rbdata2 = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::ReadbackHeap));
		  auto rbdata3 = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::ReadbackHeap));


		  {
			  auto bind = list.bind(pipeline);
			  bind.rootConstant(0, completedataUav.getCustomIndexInHeap());
			  list.Dispatch(bind, 1, 1, 1);
		  }
		  {
			  auto bind = list.bind(pipeline);
			  bind.rootConstant(0, completedataUav2.getCustomIndexInHeap());
			  list.Dispatch(bind, 1, 1, 1);
		  }
		  {
			  auto bind = list.bind(pipeline);
			  bind.rootConstant(0, completedataUav3.getCustomIndexInHeap());
			  list.Dispatch(bind, 1, 1, 1);
		  }

		  list.CopyResource(rbdata, completedata);
		  list.CopyResource(rbdata2, completedata2);
		  list.CopyResource(rbdata3, completedata3);
		  list.closeList();
		  queue.submit(list);
		  queue.insertFence(fence);
		  fence.wait();

		  auto mapd = rbdata.Map<buf>();
		  auto& obj = mapd[0];
		  auto mapd2 = rbdata2.Map<buf>();
		  auto& obj2 = mapd2[0];
		  auto mapd3 = rbdata3.Map<buf>();
		  auto& obj3 = mapd3[0];
		  return (obj3.k == static_cast<int>(completedataUav3.getCustomIndexInHeap()));
	  });*/

	  t.addTest("Pipeline binding and modify data in compute with variable SRV", [id]()
	  {
		  GraphicsInstance sys;
		  sys.createInstance("test", 1, "faze_test", 1);
		  GpuDevice dev = sys.CreateGpuDevice(id);
		  GpuCommandQueue queue = dev.createQueue();
		  GfxCommandList list = dev.createUniversalCommandList();
		  GpuFence fence = dev.createFence();

		  ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("tests/compute_rootconstant_ver3"));

		  struct buf
		  {
			  int i;
			  int k;
			  int x;
			  int y;
		  };

      auto buffer2 = dev.createBuffer(ResourceDescriptor()
        .Width(1)
        .Format<buf>());
      auto bufferSRV2 = dev.createBufferSRV(buffer2);
      auto upload2 = dev.createBuffer(ResourceDescriptor()
        .Width(1)
        .Format<buf>()
        .Usage(ResourceUsage::UploadHeap));
		  std::vector<BufferNew> uploadBuffers;
		  std::vector<BufferNewSRV> bufferSRVs;
		  for (int i = 0; i < 8; ++i)
		  {
			  auto buffer = dev.createBuffer(ResourceDescriptor()
				  .Width(1)
				  .Format<buf>());
			  auto bufferSRV = dev.createBufferSRV(buffer);
			  auto upload = dev.createBuffer(ResourceDescriptor()
				  .Width(1)
				  .Format<buf>()
				  .Usage(ResourceUsage::UploadHeap));

			  {
				  auto map = upload.Map<buf>();
				  map[0].i = bufferSRV.getCustomIndexInHeap();
				  map[0].k = i;
				  map[0].x = 0;
				  map[0].y = bufferSRV.getCustomIndexInHeap();
			  }
        list.CopyResource(buffer, upload);
			  uploadBuffers.push_back(upload);
			  bufferSRVs.push_back(bufferSRV);
		  }

		  auto completedata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .enableUnorderedAccess());
		  auto completedataUav = dev.createBufferUAV(completedata);

		  auto rbdata = dev.createBuffer(ResourceDescriptor()
			  .Width(1)
			  .Format<buf>()
			  .Usage(ResourceUsage::ReadbackHeap));

		  {
			  auto bind = list.bind(pipeline);
			  bind.UAV(0, completedataUav);
			  bind.rootConstant(0, bufferSRVs[bufferSRVs.size()-3].getCustomIndexInHeap());
			  list.Dispatch(bind, 1, 1, 1);
		  }

		  list.CopyResource(rbdata, completedata);
		  list.closeList();
		  queue.submit(list);
		  queue.insertFence(fence);
		  fence.wait();

		  auto mapd = rbdata.Map<buf>();
		  auto& obj = mapd[0];
		  return (obj.i == static_cast<int>(bufferSRVs[bufferSRVs.size()-3].getCustomIndexInHeap()));
	  });
    /*
    t.addTest("Pipeline binding and modify data to bunch of uav's in compute", [id]()
    {
      GraphicsInstance sys;
      sys.createInstance("test", 1, "faze_test", 1);
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      GpuFence fence = dev.createFence();

      ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("tests/compute_rootconstant_ver4"));

      struct buf
      {
        int i;
        int k;
        int x;
        int y;
      };
      std::vector<BufferNew> readBuffers;
      std::vector<BufferNewUAV> bufferUavs;
      for (int i = 0; i < 60; ++i) // 60 should be max currently
      {
        auto buffer = dev.createBuffer(ResourceDescriptor()
          .Width(1)
          .Format<buf>()
          .enableUnorderedAccess());
        auto bufferUAV = dev.createBufferUAV(buffer);
        auto upload = dev.createBuffer(ResourceDescriptor()
          .Width(1)
          .Format<buf>()
          .Usage(ResourceUsage::ReadbackHeap));

        readBuffers.push_back(upload);
        bufferUavs.push_back(bufferUAV);
      }

      {
        auto bind = list.bind(pipeline);
        list.Dispatch(bind, 1, 1, 1);
      }
      for (int i = 0; i < 60; ++i)
      {
        list.CopyResource(readBuffers[i], bufferUavs[i].buffer());
      }
      list.closeList();
      queue.submit(list);
      queue.insertFence(fence);
      fence.wait();

      for (int i = 0; i < 60; ++i)
      {
        auto mapd = readBuffers[i].Map<buf>();
        F_LOG("%d %d %d %d \n", mapd[0].i, mapd[0].k, mapd[0].x, mapd[0].y);
      }
      return true;
    });
    */

    t.runTests();
  }

public:
  void run(ProgramParams params)
  {
    runTestsForDevice("todo", 0, params);
  }
};

#pragma warning( pop ) 