// EntryPoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/entity/database.hpp"
#include "core/src/system/time.hpp"
#include "core/src/tests/schedulertests.hpp"
#include "core/src/tests/bitfield_tests.hpp"
#include "core/src/math/mat_templated.hpp"
#include "app/Graphics/gfxApi.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/spirvcross/spirv_glsl.hpp"
#include <shaderc/shaderc.hpp> 
#include <cstdio>
#include <iostream>


using namespace faze;

int EntryPoint::main()
{
  Logger log;
  GraphicsInstance devices;
  if (!devices.createInstance("faze"))
  {
    F_ILOG("System", "Failed to create Vulkan instance, exiting");
    log.update();
    return 1;
  }
  {
	  SchedulerTests::Run();
  }

  auto main = [&](std::string name)
  {
    //LBS lbs;
    WTime t;
    ivec2 ires = { 800, 600 };
    vec2 res = { static_cast<float>(ires.x()), static_cast<float>(ires.y()) };
    //Window window(m_params, name, ires.x(), ires.y());
    //window.open();
    
    {
      GpuDevice gpu = devices.createGpuDevice();
      GraphicsQueue gfxQueue = gpu.createGraphicsQueue();
      DMAQueue dmaQueue = gpu.createDMAQueue();
      {
        GraphicsCmdBuffer gfx = gpu.createGraphicsCommandBuffer();
        DMACmdBuffer dma = gpu.createDMACommandBuffer();
        auto testHeap = gpu.createMemoryHeap(HeapDescriptor().setName("ebin").sizeInBytes(32000000).setHeapType(HeapType::Upload)); // 32megs, should be the common size...
        auto buffer = gpu.createBuffer(testHeap,
          ResourceDescriptor()
            .Name("testBuffer")
            .Width(1000)
            .Usage(ResourceUsage::UploadHeap)
            .Dimension(FormatDimension::Buffer)
            .Format<float>());

        if (buffer.isValid())
        {
          F_LOG("yay! a buffer\n");
          {
            auto map = buffer.Map<float>(0, 1000);
            if (map.isValid())
            {
              F_LOG("yay! mapped buffer!\n");
              map[0] = 1.f;
            }
          }
        }
      }
    }


	FileSystem fs(".");

	log.update();
	{
		auto blob = fs.readFile("/shaders/sampleShader.comp");
		std::string text(reinterpret_cast<char*>(blob.data()));
		{
			std::string asdError = "";
			int lastEnd = 0;
			int count = 1;
			for (int i = 0; i < text.size(); ++i)
			{
				if (text[i] == '\r')
				{
					text[i] = ' ';
				}
				if (text[i] == '\n')
				{
					asdError = text.substr(lastEnd, i - lastEnd);
					lastEnd = i + 1;
					++i;
					F_SLOG("SHADERC", "%d: %s\n", count++, asdError.c_str());
				}
			}
		}
		//F_LOG("%s\n", text.c_str());
		shaderc::CompileOptions opt;
		shaderc::Compiler compiler;
		auto something = compiler.CompileGlslToSpv(text, shaderc_glsl_compute_shader, "main", opt);
		if (something.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			auto asd = something.GetErrorMessage();
			F_SLOG("SHADERC", "Compilation failed. trying to print error...\n");
			std::string asdError = "";
			int lastEnd = 0;
			for (int i = 0; i < asd.size(); ++i)
			{
				if (asd[i] < 0)
				{
					//asd[i] = ' ';
				}
				if (asd[i] == '\n')
				{
					asdError = asd.substr(lastEnd, i - lastEnd);
					lastEnd = i + 1;
					++i;
					F_SLOG("SHADERC", "%s\n", asdError.c_str());
				}
			}
			//F_SLOG("SHADERC", "%s\n", asd.c_str());
		}
		else
		{
			F_SLOG("SHADERC", "Compilation success\n");
			size_t length_in_words = something.cend() - something.cbegin();
			F_SLOG("SHADERC", "size... :o %zubytes\n", length_in_words * 4);
			fs.writeFile("/shaders/sampleShader.comp.spv", something.cbegin(), length_in_words * 4);
			fs.flushFiles();
		}
		log.update();
	}
	{
		auto spirvBinary = fs.readFile("/shaders/sampleShader.comp.spv");
		std::vector<uint32_t> data(spirvBinary.size() / 4);
		memcpy(data.data(), spirvBinary.data(), spirvBinary.size());
		F_SLOG("SPIRV-CROSS", "original %zu vs new %zu, 4? %zu\n", spirvBinary.size(), data.size(), data.size() * 4);
		try
		{
			spirv_cross::CompilerGLSL glsl(std::move(data));
			spirv_cross::ShaderResources resources = glsl.get_shader_resources();

			auto lamda = [&](std::vector<spirv_cross::Resource>& res)
			{
				for (auto&& resource : res)
				{
					unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
					unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
					F_SLOG("SPIRV-CROSS", "Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);
				}
			};
			lamda(resources.uniform_buffers);
			lamda(resources.storage_buffers);
			lamda(resources.stage_inputs);
			lamda(resources.stage_outputs);
			lamda(resources.subpass_inputs);
			lamda(resources.storage_images);
			lamda(resources.sampled_images);
			lamda(resources.atomic_counters);
		}
		catch (spirv_cross::CompilerError e)
		{
			F_SLOG("SPIRV-CROSS", "%s\n", e.what());
		}
	}

  };

  main("w1");
  return 0;
}
