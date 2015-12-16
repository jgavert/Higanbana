#pragma once
#include "Platform/EntryPoint.hpp"
#include "Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/tests/TestWorks.hpp"

#include "Graphics/gfxApi.hpp"
#include <cstdio>
#include <iostream>
#include <d3d12shader.h>
#include <D3Dcompiler.h>

class ApiTests2
{
private:
  void runTestsForDevice(std::string name, int id, ProgramParams params)
  {
    faze::TestWorks t(name);
    
    t.addTest("Device creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      return dev.isValid();
    });

    t.addTest("Queue creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      return queue.isValid();
    });

    t.addTest("CommandList creation", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GfxCommandList list = dev.createUniversalCommandList();
      return list.isValid();
    });

    t.addTest("Create Upload buffer", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBufferUAV(
        Dimension(4096)
        , Format<float>()
        , ResUsage::Upload);
      return buf.isValid();
    });

    t.addTest("Map upload resource to program memory", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBufferUAV(
        Dimension(4096), Format<float>(), ResUsage::Upload);

      auto a = buf.buffer().Map<float>();
      a[0] = 1.f;
      return a[0] != 0.f;
    });

    t.addTest("Create Gpu resident buffer", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      auto buf = dev.createBufferSRV(
        Dimension(4096), Format<float>());
      return buf.isValid();
    });

    t.addTest("Move data to upload heap and move to gpu memory", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();

      auto srcdata = dev.createBufferSRV(
        Dimension(4096), Format<float>(), ResUsage::Upload);
      auto dstdata = dev.createBufferSRV(
        Dimension(4096), Format<float>(), ResUsage::Gpu);

      {
        auto tmp = srcdata.buffer().Map<float>();
        for (int i = 0;i < srcdata.buffer().size; ++i)
        {
          tmp.get()[i] = static_cast<float>(i);
        }
      }

      list.CopyResource(dstdata.buffer(), srcdata.buffer());
      list.close();
      queue.submit(list);
      GpuFence fence = dev.createFence();
      queue.insertFence(fence);
      fence.wait();

      return true;
    });
    t.addTest("Upload and readback the same data", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      GpuCommandQueue queue = dev.createQueue();
      GfxCommandList list = dev.createUniversalCommandList();
      GpuFence fence = dev.createFence();

      auto srcdata = dev.createBufferSRV(Dimension(4096), Format<float>(), ResUsage::Upload);
      auto dstdata = dev.createBufferSRV(Dimension(4096), Format<float>(), ResUsage::Gpu);
      auto rbdata = dev.createBufferSRV(Dimension(4096), Format<float>(), ResUsage::Readback);

      {
        auto tmp = srcdata.buffer().Map<float>();
        for (int i = 0;i < srcdata.buffer().size; ++i)
        {
          tmp[i] = static_cast<float>(i);
        }
      }

      list.CopyResource(dstdata.buffer(), srcdata.buffer());
      list.CopyResource(rbdata.buffer(), dstdata.buffer());
      list.close();
      queue.submit(list);
      queue.insertFence(fence);
      fence.wait();
      {
        auto woot = rbdata.buffer().Map<float>();
        for (int i = 0;i < rbdata.buffer().size; ++i)
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
    t.addTest("shader root signature", [&]()
    {
      SystemDevices sys;
      GpuDevice dev = sys.CreateGpuDevice(id);
      ComPtr<ID3DBlob> blobCompute;
      ComPtr<ID3DBlob> errorMsg;
      HRESULT hr = D3DCompileFromFile(L"compute_1.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, blobCompute.addr(), errorMsg.addr());
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
      ComPtr<ID3D12RootSignatureDeserializer> asd;
      hr = D3D12CreateRootSignatureDeserializer(blobCompute->GetBufferPointer(), blobCompute->GetBufferSize(), __uuidof(ID3D12RootSignatureDeserializer), reinterpret_cast<void**>(asd.addr()));
      if (FAILED(hr))
      {
        return false;
      }
      ComPtr<ID3D12RootSignature> g_RootSig;
      ComPtr<ID3DBlob> blobSig;
      ComPtr<ID3DBlob> errorSig;
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

	t.addTest("shader root signature mirror structure", [&]()
	{
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);
		ComPtr<ID3DBlob> blobCompute;
		ComPtr<ID3DBlob> errorMsg;
		HRESULT hr = D3DCompileFromFile(L"compute_1.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", 0, 0, blobCompute.addr(), errorMsg.addr());
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
		ComPtr<ID3D12RootSignatureDeserializer> asd;
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

	t.addTest("Create compute Pipeline", [id]()
	{
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);
		ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("compute_1.hlsl"));
		return true;
	});



	t.addTest("Pipeline binding and modify data in compute (sub 50 lines!)", [id]()
	{
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);
		GpuCommandQueue queue = dev.createQueue();
		GfxCommandList list = dev.createUniversalCommandList();
		GpuFence fence = dev.createFence();

    ComputePipeline pipeline = dev.createComputePipeline(ComputePipelineDescriptor().shader("compute_1.hlsl"));

    struct buf
    {
      int i;
      int k;
      int x;
      int y;
    };
		auto srcdata = dev.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Upload);
		auto dstdata = dev.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Gpu);
		auto completedata = dev.createBufferUAV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Gpu);
		auto rbdata = dev.createBufferSRV(Dimension(1000 * 1000), Format<buf>(), ResUsage::Readback);

		{
			auto tmp = srcdata.buffer().Map<buf>();
			for (int i = 0;i < srcdata.buffer().size; ++i)
			{
        tmp[i].i = i;
        tmp[i].k = i;
			}
		}

		list.CopyResource(dstdata.buffer(), srcdata.buffer());
		auto bind = list.bind(pipeline);
		bind.SRV(0, dstdata);
		bind.UAV(0, completedata);
    unsigned int shaderGroup = 50;
    unsigned int inputSize = 1000 * 1000;
		list.Dispatch(bind, inputSize / shaderGroup, 1, 1);

		list.CopyResource(rbdata.buffer(), completedata.buffer());
    list.close();
    queue.submit(list);
		queue.insertFence(fence);
		fence.wait();

    auto mapd = rbdata.buffer().Map<buf>();
    for (int i = 0;i < rbdata.buffer().size;++i)
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
    SystemDevices sys;
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
    SystemDevices sys;
    GpuDevice dev = sys.CreateGpuDevice(id);
    auto buf = dev.createTextureUAV(
      Dimension(800, 600)
      , Format<int>(FormatType::R8G8B8A8_UNORM)
      , ResUsage::Gpu
      , MipLevel()
      , Multisampling());;
    return buf.isValid();
  });

  t.addTest("Create TextureRTV", [id]()
  {
    SystemDevices sys;
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
    SystemDevices sys;
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
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);

    auto woot = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
      .PixelShader("pixel.hlsl")
      .VertexShader("vertex.hlsl")
      .DSVFormat(FormatType::D32_FLOAT)
      .setRenderTargetCount(1)
      .RTVFormat(0, FormatType::R8G8B8A8_UNORM_SRGB));

		return woot.valid();
	});

	t.addTest("Create SwapChain", [&]()
	{
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);
    GpuCommandQueue queue = dev.createQueue();

    Window window(params, "kek", 800, 600);
    window.open();
    SwapChain sc = std::move(dev.createSwapChain(queue, window));
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
		SystemDevices sys;
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
      UINT backBufferIndex = sc->GetCurrentBackBufferIndex();
      vec[0] += 0.02f;
      if (vec[0] > 1.0f)
        vec[0] = 0.f;
      list.ClearRenderTargetView(sc[backBufferIndex], vec);
      list.close();
      queue.submit(list);

      sc->Present(1, 0);
      queue.insertFence(fence);
      fence.wait();
      list.resetList();
    }
    window.close();

		return true;
	});
  
  t.addTest("Create window and render a triangle for full 1 second in loop", [&]()
  {
    SystemDevices sys;
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
    auto srcdata = dev.createBufferSRV(Dimension(6), Format<buf>(), ResUsage::Upload);
    auto dstdata = dev.createBufferSRV(Dimension(6), Format<buf>(), ResUsage::Gpu);

    {
      auto tmp = srcdata.buffer().Map<buf>();
      float size = 0.5f;
      tmp[0] = { size, size, 0.f, 1.f };
      tmp[1] = { size, -size, 0.f, 1.f };
      tmp[2] = { -size, size, 0.f, 1.f };
      tmp[3] = { -size, size, 0.f, 1.f };
      tmp[4] = { size, -size, 0.f, 1.f };
      tmp[5] = { -size, -size, 0.f, 1.f };
    }

    gfx.CopyResource(dstdata.buffer(), srcdata.buffer());
    GpuFence fence = dev.createFence();
    gfx.close();
    queue.submit(gfx);
    queue.insertFence(fence);

    auto pipeline = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
      .PixelShader("pixel.hlsl")
      .VertexShader("vertex.hlsl")
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
      auto backBufferIndex = sc->GetCurrentBackBufferIndex();
      gfx.ClearRenderTargetView(sc[backBufferIndex], vec);
      gfx.setRenderTarget(sc[backBufferIndex]);
      // graphics begin
      auto bind = gfx.bind(pipeline);
      bind.SRV(0, dstdata);
      gfx.drawInstanced(bind, 6, 1, 0, 0);


      // submit all
      gfx.close();
      queue.submit(gfx);

      // present
      sc->Present(1, 0);
      queue.insertFence(fence);
    }
    fence.wait();
    window.close();

		return true;
	});

	t.addTest("Rotating triangle with shaders", [id]()
	{
		SystemDevices sys;
		GpuDevice dev = sys.CreateGpuDevice(id);

		return false;
	});

    t.runTests();
  }

public:
  void run(ProgramParams params)
  {
    SystemDevices sys;
    for (int i = 0; i < 1; ++i)
    {
      runTestsForDevice(sys.getInfo(i).description, i, params);
    }
  }
};