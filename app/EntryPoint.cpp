#include "core/src/Platform/EntryPoint.hpp"

// EntryPoint.cpp
#ifdef FAZE_PLATFORM_WINDOWS
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include "core/src/neural/network.hpp"
#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"
#include "core/src/filesystem/filesystem.hpp"
#include "core/src/Platform/Window.hpp"
#include "core/src/system/logger.hpp"
#include "core/src/Platform/EntryPoint.hpp"
#include "core/src/global_debug.hpp"

#include "core/src/math/math.hpp"

#include "shaders/textureTest.if.hpp"
#include "shaders/posteffect.if.hpp"
#include "shaders/triangle.if.hpp"

#include "renderer/blitter.hpp"
#include "renderer/imgui_renderer.hpp"

#include "renderer/texture_pass.hpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "core/src/external/imgiu/imgui.h"

#include "faze/src/new_gfx/common/cpuimage.hpp"
#include "faze/src/new_gfx/common/image_loaders.hpp"

#include "core/src/input/gamepad.hpp"

using namespace faze;
using namespace faze::math;

template <typename T>
class DoubleBuffered
{
private:
  std::atomic<int> m_writer = 2;
  std::atomic<int> m_reader = 0;
  T objects[3] = {};

public:
  T readValue()
  {
    int readerIndex = m_reader;
    auto val = objects[readerIndex];
    readerIndex = (readerIndex + 1) % 3;
    int writerIndex = m_writer;
    if (readerIndex != writerIndex)
    {
      m_reader = readerIndex;
    }
    return val;
  }

  void writeValue(T value)
  {
    int writeIndex = m_writer;
    objects[writeIndex] = value;

    writeIndex = (writeIndex + 1) % 3;
    int readerIndex = m_reader;
    if (readerIndex != writeIndex)
    {
      m_writer = writeIndex;
    }
  }
};

class DynamicScale
{
private:
  int m_scale = 10;
  float m_frametimeTarget = 16.6667f; // 60fps
  float m_targetThreshold = 1.f; // 1 millisecond

public:
  DynamicScale(int scale = 10, float frametimeTarget = 16.6667f, float threshold = 1.f)
    : m_scale(scale)
    , m_frametimeTarget(frametimeTarget)
    , m_targetThreshold(threshold)
  {}

  int tick(float frametimeInMilli)
  {
    auto howFar = std::fabs((m_frametimeTarget - frametimeInMilli)*10.f);
    if (frametimeInMilli > m_frametimeTarget)
    {
      //F_LOG("Decreasing scale %d %f %f\n", m_scale, frametimeInMilli, m_frametimeTarget);
      m_scale -= int(howFar * 2);
    }
    else if (frametimeInMilli < m_frametimeTarget - m_targetThreshold)
    {
      m_scale += int(howFar);
    }
    if (m_scale <= 0)
      m_scale = 1;
    return m_scale;
  }
};

int EntryPoint::main()
{
  Logger log;
  //testNetwrk(log);

  LBS lbs;

  auto main = [&](GraphicsApi api, VendorID preferredVendor, bool updateLog)
  {
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs;
    WTime time;
    WTime graphicsCpuTime;
    WTime CpuTime;

    WTime logicTime;

    gamepad::Fic directInputs;
    DoubleBuffered<gamepad::X360LikePad> pad;
    DoubleBuffered<float> logicFps;
    bool quit = false;
    logicTime.firstTick();
    lbs.addTask("gamepad", [&](size_t)
    {
      logicTime.tick();
      logicFps.writeValue(logicTime.getCurrentFps());
      if (quit)
      {
        lbs.addTask("gamepadDone", [&](size_t)
        {
        });
        return;
      }
      directInputs.pollDevices(gamepad::Fic::PollOptions::AllowDeviceEnumeration);
      pad.writeValue(directInputs.getFirstActiveGamepad());
      rescheduleTask();
    });

    auto image = textureUtils::loadImageFromFilesystem(fs, "/simple.jpg");
    log.update();

    bool explicitID = false;
    int chosenGpu = 0;

    while (true)
    {
      GraphicsSubsystem graphics(api, "faze");
      F_LOG("Using api %s\n", graphics.gfxApi().c_str());
      F_LOG("Have gpu's\n");
      auto gpus = graphics.availableGpus();
      if (!explicitID)
      {
        for (auto&& it : gpus)
        {
          if (it.vendor == preferredVendor)
          {
            chosenGpu = it.id;
          }
          F_LOG("\t%d. %s (memory: %zd, api: %s)\n", it.id, it.name.c_str(), it.memory, it.apiVersionStr.c_str());
        }
      }
      if (updateLog) log.update();
      if (gpus.empty())
        return;

      int2 ires = { 200, 100 };

      int raymarch_Res = 10;
      int raymarch_x = 16;
      int raymarch_y = 9;

      float raymarchfpsTarget = 16.666667f;
      float raymarchFpsThr = 0.2f;
      bool raymarchDynamicEnabled = false;

      // position and dir
      float3 position{ 0.f, 0.f, 0.f };
      float3 dir{ 1.f, 0.f, 0.f };
      float3 updir{ 0.f, 1.f, 0.f };
      float3 sideVec{ 0.f, 0.f, 1.f };
      quaternion direction{ 1.f, 0.f, 0.f, 0.f };

      Window window(m_params, gpus[chosenGpu].name, 1280, 720, 300, 200);
      window.open();

      auto surface = graphics.createSurface(window);
      auto dev = graphics.createDevice(fs, gpus[chosenGpu]);
      time.firstTick();
      {
        auto toggleHDR = false;
        auto scdesc = SwapchainDescriptor().formatType(FormatType::Unorm8RGBA).colorspace(Colorspace::BT709).bufferCount(3);
        auto swapchain = dev.createSwapchain(surface, scdesc);

        F_LOG("Created device \"%s\"\n", gpus[chosenGpu].name.c_str());

        auto bufferdesc = ResourceDescriptor()
          .setName("testBufferTarget")
          .setFormat(FormatType::Float32)
          .setWidth(100)
          .setDimension(FormatDimension::Buffer);

        auto buffer = dev.createBuffer(bufferdesc);

        auto testImage = dev.createTexture(image);
        auto testSrv = dev.createTextureSRV(testImage, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

        auto testDesc = ResourceDescriptor()
          .setName("testTexture")
          .setFormat(FormatType::Unorm16RGBA)
          .setWidth(ires.x)
          .setHeight(ires.y)
          .setMiplevels(4)
          .setDimension(FormatDimension::Texture2D)
          .setUsage(ResourceUsage::RenderTargetRW);
        auto texture = dev.createTexture(testDesc);
        auto texRtv = dev.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
        auto texSrv = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
        auto texUav = dev.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

        auto texture2 = dev.createTexture(testDesc
          .setName("postEffect")
          .setMiplevels(1));
        auto texSrv2 = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
        auto texUav2 = dev.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

        Renderpass triangleRenderpass = dev.createRenderpass();

        auto pipelineDescriptor = GraphicsPipelineDescriptor()
          .setVertexShader("triangle")
          .setPixelShader("triangle")
          .setPrimitiveTopology(PrimitiveTopology::Triangle)
          .setDepthStencil(DepthStencilDescriptor()
            .setDepthEnable(false));

        GraphicsPipeline trianglePipe = dev.createGraphicsPipeline(pipelineDescriptor);
        F_LOG("%d\n", trianglePipe.descriptor.sampleCount);

        renderer::TexturePass<::shader::PostEffect> postPass(dev, "posteffect", uint3(8, 8, 1));
        renderer::Blitter blit(dev);
        renderer::ImGui imgRenderer(dev);

        ComputePipeline testCompute = dev.createComputePipeline(ComputePipelineDescriptor()
          .setShader("textureTest")
          .setThreadGroups(uint3(8, 8, 1)));

        bool closeAnyway = false;

        bool captureMouse = false;

        graphicsCpuTime.firstTick();
        CpuTime.firstTick();
        while (!window.simpleReadMessages(frame++))
        {
          CpuTime.startFrame();

          // update inputs and our position
          //directInputs.pollDevices(gamepad::Fic::PollOptions::AllowDeviceEnumeration);
          {
            //auto input = directInputs.getFirstActiveGamepad();

            float3 rxyz{ 0 };
            float2 xy{ 0 };

            auto input = pad.readValue();

            if (input.alive)
            {
              auto floatizeAxises = [](int16_t input, float& output)
              {
                constexpr int16_t deadzone = 4500;
                if (std::abs(input) > deadzone)
                {
                  constexpr float max = static_cast<float>(std::numeric_limits<int16_t>::max() - deadzone);
                  output = static_cast<float>(std::abs(input) - deadzone) / max;
                  output *= (input < 0) ? -1 : 1;
                }
              };

              floatizeAxises(input.lstick[0].value, xy.x);
              floatizeAxises(input.lstick[1].value, xy.y);

              floatizeAxises(input.rstick[0].value, rxyz.x);
              floatizeAxises(input.rstick[1].value, rxyz.y);

              rxyz = mul(rxyz, float3(-1.f, 1.f, 0));

              auto floatizeTriggers = [](uint16_t input, float& output)
              {
                constexpr int16_t deadzone = 4000;
                if (std::abs(input) > deadzone)
                {
                  constexpr float max = static_cast<float>(std::numeric_limits<uint16_t>::max() - deadzone);
                  output = static_cast<float>(std::abs(input) - deadzone) / max;
                }
              };
              float l{}, r{};

              floatizeTriggers(input.lTrigger.value, l);
              floatizeTriggers(input.rTrigger.value, r);

              rxyz.z = r - l;

              rxyz = math::mul(rxyz, std::max(time.getFrameTimeDelta(), 0.001f));
              xy = math::mul(xy, std::max(time.getFrameTimeDelta(), 0.001f));

              quaternion yaw = math::rotateAxis(updir, rxyz.x);
              quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
              quaternion roll = math::rotateAxis(dir, rxyz.z);
              direction = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);
            }
            else if (captureMouse)
            {
              auto m = window.mouse();
              auto p = m.m_pos;
              auto floatizeMouse = [](int input, float& output)
              {
                constexpr float max = static_cast<float>(100);
                output = std::min(static_cast<float>(std::abs(input)) / max, max);
                output *= (input < 0) ? -1 : 1;
              };

              floatizeMouse(p.x, rxyz.x);
              floatizeMouse(p.y, rxyz.y);

              rxyz = math::mul(rxyz, std::max(time.getFrameTimeDelta(), 0.001f));
              //xy = math::mul(xy, std::max(time.getFrameTimeDelta(), 0.001f));

              quaternion yaw = math::rotateAxis(updir, rxyz.x);
              quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
              //quaternion roll = math::rotateAxis(dir, rxyz.z);
              direction = math::mul(math::mul(yaw, direction), pitch);
            }

            // use our current up and forward vectors and calculate our new up and forward vectors
            // yaw, pitch, roll

            dir = normalize(rotateVector({ 1.f, 0.f, 0.f }, direction));
            updir = normalize(rotateVector({ 0.f, 1.f, 0.f }, direction));
            sideVec = normalize(rotateVector({ 0.f, 0.f, 1.f }, direction));

            position = math::add(position, math::mul(sideVec, xy.x));
            position = math::add(position, math::mul(dir, xy.y));
          }

          // update fs
          fs.updateWatchedFiles();
          if (window.hasResized() || toggleHDR)
          {
            dev.adjustSwapchain(swapchain, scdesc);
            window.resizeHandled();
            toggleHDR = false;
          }
          auto& inputs = window.inputs();

          if (inputs.isPressedThisFrame(VK_F1, 1))
          {
            window.captureMouse(true);
            captureMouse = true;
          }
          if (inputs.isPressedThisFrame(VK_F2, 1))
          {
            window.captureMouse(false);
            captureMouse = false;
          }

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('1', 1))
          {
            window.toggleBorderlessFullscreen();
          }

          if (frame > 10 && (closeAnyway || inputs.isPressedThisFrame(VK_ESCAPE, 1)))
          {
            break;
          }
          if (updateLog) log.update();

          // If you acquire, you must submit it. Next, try to first present empty image.
          // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
          CpuTime.tick();
          auto backbuffer = dev.acquirePresentableImage(swapchain);
          graphicsCpuTime.startFrame();

          CommandGraph tasks = dev.createGraph();

          {
            auto node = tasks.createPass("clear");
            node.clearRT(backbuffer, float4{ std::sin(time.getFTime())*.5f + .5f, 0.f, 0.f, 0.f });
            //node.clearRT(texRtv, vec4{ std::sin(float(frame)*0.01f)*.5f + .5f, std::sin(float(frame)*0.01f)*.5f + .5f, 0.f, 1.f });
            tasks.addPass(std::move(node));
          }
          uint2 iRes = uint2{ texUav.desc().desc.width, texUav.desc().desc.height };
          {
            auto node = tasks.createPass("compute!");

            auto binding = node.bind<::shader::TextureTest>(testCompute);
            binding.constants = {};
            binding.constants.iResolution = iRes;
            binding.constants.iFrame = static_cast<int>(frame);
            binding.constants.iTime = time.getFTime();
            binding.constants.iPos = position;
            binding.constants.iDir = dir;
            binding.constants.iUpDir = updir;
            binding.constants.iSideDir = sideVec;
            binding.uav(::shader::TextureTest::output, texUav);

            unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.x), 8));
            unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(iRes.y), 8));
            node.dispatch(binding, uint3{ x, y, 1 });

            tasks.addPass(std::move(node));
          }
          //if (inputs.isPressedThisFrame('3', 2))

          postPass.compute(tasks, texUav2, texSrv, iRes);

          {
            // we have pulsing red color background, draw a triangle on top of it !
            auto node = tasks.createPass("Triangle!");
            node.renderpass(triangleRenderpass);
            backbuffer.setOp(LoadOp::Clear);
            node.subpass(backbuffer);
            backbuffer.setOp(LoadOp::DontCare);

            vector<float4> vertices;
            vertices.push_back(float4{ -1.f, -1.f, 1.f, 1.f });
            vertices.push_back(float4{ -1.0f, 3.f, 1.f, 1.f });
            vertices.push_back(float4{ 3.f, -1.f, 1.f, 1.f });

            auto verts = dev.dynamicBuffer(makeMemView(vertices), FormatType::Float32RGBA);

            auto binding = node.bind<::shader::Triangle>(trianglePipe);
            //binding.constants.color = float4{ 0.f, 0.f, std::sin(float(frame)*0.01f + 1.0f)*.5f + .5f, 1.f };
            binding.constants.color = float4{ 0.f, 0.f, 0.f, 1.f };
            binding.constants.colorspace = static_cast<int>(swapchain.impl()->displayCurve());
            binding.srv(::shader::Triangle::vertices, verts);
            binding.srv(::shader::Triangle::yellow, texSrv2);
            node.draw(binding, 3, 1);
            node.endRenderpass();
            tasks.addPass(std::move(node));
          }
          //float heightMulti = 1.f; // float(testSrv.desc().desc.height) / float(testSrv.desc().desc.width);
          //blit.blitImage(dev, tasks, backbuffer, testSrv, renderer::Blitter::FitMode::Fit);

          /*
              uint2 sdim = { testSrv.desc().desc.width, testSrv.desc().desc.height };
              float scale = float(sdim.x()) / float(sdim.y());

              float nwidth = float(backbuffer.desc().desc.width)*0.2f;
              float nheight = nwidth / scale;

              blit.blit(dev, tasks, backbuffer, testSrv, { 4, 800 }, int2{ int(nwidth), int(nheight) });
          */

          {
            ImGuiIO &io = ImGui::GetIO();
            time.tick();
            io.DeltaTime = time.getFrameTimeDelta();
            if (io.DeltaTime < 0.f)
              io.DeltaTime = 0.00001f;

            auto& mouse = window.mouse();

            if (mouse.captured)
            {
              io.MousePos.x = static_cast<float>(mouse.m_pos.x);
              io.MousePos.y = static_cast<float>(mouse.m_pos.y);

              io.MouseWheel = static_cast<float>(mouse.mouseWheel)*0.1f;
            }

            auto& input = window.inputs();

            input.goThroughThisFrame([&](Input i)
            {
              if (i.key >= 0)
              {
                switch (i.key)
                {
                case VK_LBUTTON:
                {
                  io.MouseDown[0] = (i.action > 0);
                  break;
                }
                case VK_RBUTTON:
                {
                  io.MouseDown[1] = (i.action > 0);
                  break;
                }
                case VK_MBUTTON:
                {
                  io.MouseDown[2] = (i.action > 0);
                  break;
                }
                default:
                  if (i.key < 512)
                    io.KeysDown[i.key] = (i.action > 0);
                  break;
                }
              }
            });

            for (auto ch : window.charInputs())
              io.AddInputCharacter(static_cast<ImWchar>(ch));

            io.KeyCtrl = inputs.isPressedThisFrame(VK_CONTROL, 2);
            io.KeyShift = inputs.isPressedThisFrame(VK_SHIFT, 2);
            io.KeyAlt = inputs.isPressedThisFrame(VK_MENU, 2);
            io.KeySuper = false;
            bool kek = true;

            imgRenderer.beginFrame(backbuffer);

            ::ImGui::ShowTestWindow(&kek);

            ImGui::SetNextWindowPos({ 10.f, 10.f });
            if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
              if (ImGui::Button("Api"))
                ImGui::OpenPopup("selectApi");
              ImGui::SameLine();
              ImGui::Text(api == GraphicsApi::DX12 ? "DX12" : "Vulkan");
              if (ImGui::BeginPopup("selectApi"))
              {
                if (ImGui::Selectable("DX12"))
                {
                  api = GraphicsApi::DX12;
                  reInit = true;
                  closeAnyway = true;
                }
                if (ImGui::Selectable("Vulkan"))
                {
                  api = GraphicsApi::Vulkan;
                  reInit = true;
                  closeAnyway = true;
                }
                ImGui::EndPopup();
              }

              if (ImGui::Button("Device"))
                ImGui::OpenPopup("selectGpu");
              ImGui::SameLine();
              ImGui::Text(gpus[chosenGpu].name.c_str());
              if (ImGui::BeginPopup("selectGpu"))
              {
                for (auto&& it : gpus)
                {
                  if (ImGui::Selectable(it.name.c_str()))
                  {
                    chosenGpu = it.id;
                    reInit = true;
                    closeAnyway = true;
                    explicitID = true;
                  }
                }
                ImGui::EndPopup();
              }

              ImGui::Text("FPS %.2f", 1000.f / time.getCurrentFps());
              auto gfxTime = graphicsCpuTime.getCurrentFps();
              auto cpuTime = CpuTime.getCurrentFps();
              auto logic = logicFps.readValue();
              ImGui::Text("Graphics Cpu FPS %.2f (%.2fms)", 1000.f / gfxTime, gfxTime);
              ImGui::Text("Cpu before FPS %.2f (%.2fms)", 1000.f / cpuTime, cpuTime);
              ImGui::Text("total CpuTime FPS %.2f (%.2fms)", 1000.f / (cpuTime + gfxTime), cpuTime + gfxTime);
              ImGui::Text("Logic FPS %.2f (%.2fms)", 1000.f / logic, logic);

              ImGui::Text("Position:   %s length:%f", toString(position).c_str(), length(position));
              ImGui::Text("Forward:    %s length:%f", toString(dir).c_str(), length(dir));
              ImGui::Text("Side:       %s length:%f", toString(sideVec).c_str(), length(sideVec));
              ImGui::Text("Up:         %s length:%f", toString(updir).c_str(), length(updir));
              ImGui::Text("rotation Q: %s", toString(direction).c_str());

              ImGui::Text("reset "); ImGui::SameLine();
              if (ImGui::Button("position"))
              {
                position = float3{ 0.f, 0.f, 0.f };
              } ImGui::SameLine();
              if (ImGui::Button("direction"))
              {
                direction = quaternion{ 1.f, 0.f, 0.f, 0.f };
              } ImGui::SameLine();
              if (ImGui::Button("all"))
              {
                position = float3{ 0.f, 0.f, 0.f };
                dir = float3{ 1.f, 0.f, 0.f };
                updir = float3{ 0.f, 1.f, 0.f };
                direction = quaternion{ 1.f, 0.f, 0.f, 0.f };
              }

              ImGui::Text("Swapchain");
              ImGui::Text("format %s", formatToString(scdesc.desc.format));
              if (ImGui::Button(formatToString(FormatType::Unorm8RGBA)))
              {
                scdesc.desc.format = FormatType::Unorm8RGBA;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(formatToString(FormatType::Unorm10RGB2A)))
              {
                scdesc.desc.format = FormatType::Unorm10RGB2A;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(formatToString(FormatType::Float16RGBA)))
              {
                scdesc.desc.format = FormatType::Float16RGBA;
                toggleHDR = true;
              }
              ImGui::Text("Resolution: %dx%d", swapchain.impl()->desc().desc.width, swapchain.impl()->desc().desc.height);
              ImGui::Text("Displaycurve: %s", displayCurveToStr(swapchain.impl()->displayCurve()));
              ImGui::Text("%s", swapchain.impl()->HDRSupport() ? "HDR Available" : "HDR not supported");
              ImGui::Text("PresentMode: %s", presentModeToStr(scdesc.desc.mode));
              if (ImGui::Button(presentModeToStr(PresentMode::FifoRelaxed)))
              {
                scdesc.desc.mode = PresentMode::FifoRelaxed;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(presentModeToStr(PresentMode::Fifo)))
              {
                scdesc.desc.mode = PresentMode::Fifo;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(presentModeToStr(PresentMode::Immediate)))
              {
                scdesc.desc.mode = PresentMode::Immediate;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(presentModeToStr(PresentMode::Mailbox)))
              {
                scdesc.desc.mode = PresentMode::Mailbox;
                toggleHDR = true;
              }
              ImGui::Text("Colorspace: %s", colorspaceToStr(scdesc.desc.colorSpace));
              if (ImGui::Button(colorspaceToStr(Colorspace::BT709)))
              {
                scdesc.desc.colorSpace = Colorspace::BT709;
                toggleHDR = true;
              } ImGui::SameLine();
              if (ImGui::Button(colorspaceToStr(Colorspace::BT2020)))
              {
                scdesc.desc.colorSpace = Colorspace::BT2020;
                toggleHDR = true;
              }
              ImGui::SliderInt("Buffer count", &scdesc.desc.bufferCount, 2, 8);

              ImGui::SliderInt("latency", &scdesc.desc.frameLatency, 1, 7);

              ImGui::Text("raymarch texture size %s", math::toString(ires).c_str());

              int oldRes = raymarch_Res;
              int oldraymarch_x = raymarch_x;
              int oldraymarch_y = raymarch_y;
              ImGui::SliderInt("scale X", &raymarch_x, 1, 30); ImGui::SameLine();
              ImGui::SliderInt("Y", &raymarch_y, 1, 30);	ImGui::SameLine();
              ImGui::SliderInt("max", &raymarch_Res, 1, 4096);

              ImGui::Checkbox("dynamic raymarch scale", &raymarchDynamicEnabled); ImGui::SameLine();
              raymarchfpsTarget = 1000.f / raymarchfpsTarget;
              ImGui::SliderFloat("target framerate", &raymarchfpsTarget, 10.f, 999.f);	ImGui::SameLine();
              raymarchfpsTarget = 1000.f / raymarchfpsTarget;
              ImGui::SliderFloat("threshold", &raymarchFpsThr, 0.001f, 1.f);

              if (ImGui::Button("reset to 30fps"))
              {
                raymarchfpsTarget = 16.666667f * 2.f;
              }
              ImGui::SameLine();
              if (ImGui::Button("reset to 60fps"))
              {
                raymarchfpsTarget = 16.666667f;
              }
              ImGui::SameLine();
              if (ImGui::Button("reset to 120fps"))
              {
                raymarchfpsTarget = 16.666667f / 2.f;
              }

              if (raymarchDynamicEnabled)
              {
                if (frame % 30 == 0)
                {
                  DynamicScale raymarchDynamicScale(raymarch_Res, raymarchfpsTarget, raymarchFpsThr);
                  raymarch_Res = raymarchDynamicScale.tick(time.getCurrentFps());
                }
              }

              if (raymarch_x > raymarch_y)
              {
                ires.data[0] = raymarch_Res;
                ires.data[1] = static_cast<int>(static_cast<float>(raymarch_y) / static_cast<float>(raymarch_x) * static_cast<float>(raymarch_Res));
              }
              else
              {
                ires.data[1] = raymarch_Res;
                ires.data[0] = static_cast<int>(static_cast<float>(raymarch_x) / static_cast<float>(raymarch_y) * static_cast<float>(raymarch_Res));
              }

              if (ImGui::Button("Reinit raymarch texture") || raymarch_Res != oldRes || oldraymarch_x != raymarch_x || oldraymarch_y != raymarch_y)
              {
                ires.data[0] = std::max(ires.data[0], 1);
                ires.data[1] = std::max(ires.data[1], 1);

                testDesc = ResourceDescriptor()
                  .setName("testTexture")
                  .setFormat(FormatType::Unorm16RGBA)
                  .setWidth(ires.data[0])
                  .setHeight(ires.data[1])
                  .setMiplevels(4)
                  .setDimension(FormatDimension::Texture2D)
                  .setUsage(ResourceUsage::RenderTargetRW);
                texture = dev.createTexture(testDesc);

                texRtv = dev.createTextureRTV(texture, ShaderViewDescriptor().setMostDetailedMip(0));
                texSrv = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
                texUav = dev.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));

                texture2 = dev.createTexture(testDesc
                  .setName("postEffect")
                  .setMiplevels(1));
                texSrv2 = dev.createTextureSRV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
                texUav2 = dev.createTextureUAV(texture, ShaderViewDescriptor().setMostDetailedMip(0).setMipLevels(1));
              }

              ImGui::PlotLines("graph test", [](void*, int idx) {return 1.f / float(idx); }, nullptr, 100, 0, "Zomg", 0.f, 1.f, ImVec2(500, 400));
            }
            ImGui::End();

            imgRenderer.endFrame(dev, tasks, backbuffer);
          }

          {
            auto& node = tasks.createPass2("present");
            node.prepareForPresent(backbuffer);
          }

          dev.submit(swapchain, tasks);

          graphicsCpuTime.tick();
          dev.present(swapchain);
        }
      }
      if (!reInit)
        break;
      else
      {
        reInit = false;
      }
    }
    quit = true;
    lbs.sleepTillKeywords({ "gamepadDone" });
  };
#if 0
  main(GraphicsApi::DX12, VendorID::Amd, true);
#else

  lbs.addTask("test1", [&](size_t) {main(GraphicsApi::DX12, VendorID::Amd, true); });
  lbs.sleepTillKeywords({ "test1" });

#endif
  log.update();
  return 1;
}