#include "windowMain.hpp"

#include "core/neural/network.hpp"
#include "core/system/LBS.hpp"
#include "core/system/time.hpp"
#include "graphics/GraphicsCore.hpp"
#include "core/filesystem/filesystem.hpp"
#include "core/Platform/Window.hpp"
#include "core/system/logger.hpp"
#include "core/global_debug.hpp"
#include "core/math/math.hpp"
#include "graphics/common/cpuimage.hpp"
#include "graphics/common/image_loaders.hpp"
#include "core/input/gamepad.hpp"
#include "graphics/definitions.hpp"
#include "graphics/helpers/pingpongTexture.hpp"

#include "graphics/common/commandbuffer.hpp"

#include <tuple>

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

void printRange(const SubresourceRange& range)
{
  F_LOG("(%d, %d)(%d, %d)\n", range.mipOffset, range.mipLevels, range.sliceOffset, range.arraySize);
}

#include "graphics/desc/shader_input_descriptor.hpp"


STRUCT_DECL(kewl,
    int a;
    int b;
    int c;
);

STRUCT_DECL(PixelConstants,
  float resx;
  float resy;
  float time;
  int unused;
);

void mainWindow(ProgramParams& params)
{

  Logger log;
  //testNetwrk(log);

  LBS lbs;

  {
    using namespace faze::backend;
    CommandBuffer buffer(1024);
    buffer.insert<sample_packet>(5, 4, false);
    std::vector<int> woot;
    for (int i = 0; i < 5; i++)
    {
      woot.push_back(i*2);
    }
    std::vector<int> woot2;
    for (int i = 0; i < 5; i++)
    {
      woot2.push_back(i*4);
    }
    buffer.insert<sample_vectorPacket>(woot, woot2);
    buffer.foreach([](PacketType type)
    {
      printf("packet found %d\n", type);
    });

    for (auto&& it : buffer)
    {
      F_ILOG("Lol", "packet %d", it->type);
    }
  }


  auto main = [&](GraphicsApi api, VendorID preferredVendor, bool updateLog)
  {
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs("/../../data");
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

    //auto image = textureUtils::loadImageFromFilesystem(fs, "/simple.jpg");
    log.update();

    bool explicitID = false;
    GpuInfo gpuinfo{};

    while (true)
    {
      GraphicsSubsystem graphics("faze");
      F_LOG("Have gpu's\n");
      auto gpus = graphics.availableGpus();
      if (!explicitID)
      {
        gpuinfo = graphics.getVendorDevice(api);
        for (auto&& it : gpus)
        {
          if (it.api == api && it.vendor == preferredVendor)
          {
            gpuinfo = it;
            break;
          }
          F_LOG("\t%s: %d. %s (memory: %zdMB, api: %s)\n", toString(it.api), it.id, it.name.c_str(), it.memory/1024/1024, it.apiVersionStr.c_str());
        }
      }
      if (updateLog) log.update();
      if (gpus.empty())
        return;

      // position and dir
      float3 position{ 0.f, 0.f, 0.f };
      float3 dir{ 1.f, 0.f, 0.f };
      float3 updir{ 0.f, 1.f, 0.f };
      float3 sideVec{ 0.f, 0.f, 1.f };
      quaternion direction{ 1.f, 0.f, 0.f, 0.f };
	  std::string windowTitle = std::string(toString(gpuinfo.api)) + ": " + gpuinfo.name;
      Window window(params, windowTitle, 1280, 720, 300, 200);
      window.open();

      auto surface = graphics.createSurface(window, gpuinfo);
      auto dev = graphics.createDevice(fs, gpuinfo);
      time.firstTick();
      {
        auto toggleHDR = false;
        auto scdesc = SwapchainDescriptor()
          .formatType(FormatType::Unorm8RGBA)
          .colorspace(Colorspace::BT709)
          .bufferCount(3).presentMode(PresentMode::Fifo);
        auto swapchain = dev.createSwapchain(surface, scdesc);

        F_LOG("Created device \"%s\"\n", gpuinfo.name.c_str());

        // test compute pipeline
        auto shaderInputDecl = ShaderInputDescriptor()
            .constants<kewl>()
            .readOnly(ShaderResourceType::ByteAddressBuffer, "test")
            .readOnly(ShaderResourceType::Buffer, "float", "test2")
            .readWrite(ShaderResourceType::Buffer, "float4", "testOut");

        auto compute = dev.createComputePipeline(ComputePipelineDescriptor()
          .setShader("ebinShader")
          .setLayout(shaderInputDecl)
          .setThreadGroups(uint3(32, 1, 1)));

        auto bufferdesc = ResourceDescriptor()
          .setName("testBuffer1")
          .setFormat(FormatType::Raw32)
          .setWidth(32)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuReadOnly);

        auto bufferdesc2 = ResourceDescriptor()
          .setName("testBuffer2")
          .setFormat(FormatType::Float32)
          .setWidth(32)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuReadOnly);

        auto bufferdesc3 = ResourceDescriptor()
          .setName("testOutBuffer")
          .setFormat(FormatType::Float32RGBA)
          .setWidth(8)
          .setDimension(FormatDimension::Buffer)
          .setUsage(ResourceUsage::GpuRW);

        auto buffer = dev.createBuffer(bufferdesc);
        auto testSRV = dev.createBufferSRV(buffer);
        auto buffer2 = dev.createBuffer(bufferdesc2);
        auto test2SRV = dev.createBufferSRV(buffer2);
        auto buffer3 = dev.createBuffer(bufferdesc3);
        auto testOut = dev.createBufferUAV(buffer3);

        
        auto babyInf = ShaderInputDescriptor();
        //    .constants<PixelConstants>();

        auto triangle = dev.createGraphicsPipeline(GraphicsPipelineDescriptor()
          .setVertexShader("Triangle")
          .setPixelShader("Triangle")
          .setLayout(babyInf)
          .setPrimitiveTopology(PrimitiveTopology::Triangle)
          .setRTVFormat(0, swapchain.buffers()[0].format())
          .setRenderTargetCount(1)
          .setDepthStencil(DepthStencilDescriptor()
            .setDepthEnable(false)));

        auto triangleRP = dev.createRenderpass();

        // end test decl

        bool closeAnyway = false;
        bool captureMouse = false;
        bool controllerConnected = false;
        vector<std::pair<std::string, double>> asyncCompute;
        vector<std::pair<std::string, double>> counters;
        float gpuFrameTime = -1.f;

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
            controllerConnected = false;
            if (input.alive)
            {
              controllerConnected = true;
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
              //F_LOG("print mouse %d %d\n", m.m_pos.x, m.m_pos.y);
              auto p = m.m_pos;
              auto floatizeMouse = [](int input, float& output)
              {
                constexpr float max = static_cast<float>(100);
                output = std::min(static_cast<float>(std::abs(input)) / max, max);
                output *= (input < 0) ? -1 : 1;
              };

              floatizeMouse(p.x, rxyz.x);
              floatizeMouse(p.y, rxyz.y);

              auto& inputs = window.inputs();

              if (inputs.isPressedThisFrame('Q', 2))
              {
                rxyz.z = -1.f;
              }
              if (inputs.isPressedThisFrame('E', 2))
              {
                rxyz.z = 1.f;
              }

              rxyz = math::mul(rxyz, std::max(time.getFrameTimeDelta(), 0.001f));
              //xy = math::mul(xy, std::max(time.getFrameTimeDelta(), 0.001f));

              /*
               basic fps control .... idea
              quaternion yaw = math::rotateAxis(updir, rxyz.x);
              quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
              //quaternion roll = math::rotateAxis(dir, rxyz.z);
              direction = math::mul(math::mul(yaw, direction), pitch);
              */

              quaternion yaw = math::rotateAxis(updir, rxyz.x);
              quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
              quaternion roll = math::rotateAxis(dir, rxyz.z);
              direction = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);

              if (inputs.isPressedThisFrame('W', 2))
              {
                xy.y = 1.f;
              }
              if (inputs.isPressedThisFrame('S', 2))
              {
                xy.y = -1.f;
              }
              if (inputs.isPressedThisFrame('D', 2))
              {
                xy.x = 1.f;
              }
              if (inputs.isPressedThisFrame('A', 2))
              {
                xy.x = -1.f;
              }
              xy = math::mul(xy, std::max(time.getFrameTimeDelta(), 0.001f));
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

          if (inputs.isPressedThisFrame(VK_MENU, 2) && inputs.isPressedThisFrame('2', 1))
          {
            reInit = true;
            if (api == GraphicsApi::DX12)
              api = GraphicsApi::Vulkan;
            else
              api = GraphicsApi::DX12;
            break;
          }

          if (frame > 10 && (closeAnyway || inputs.isPressedThisFrame(VK_ESCAPE, 1)))
          {
            break;
          }
          if (updateLog) log.update();

          // If you acquire, you must submit it. Next, try to first present empty image.
          // On vulkan, need to at least clear the image or we will just get error about it. (... well at least when the contents are invalid in the beginning.)
          CpuTime.tick();
          TextureRTV backbuffer = dev.acquirePresentableImage(swapchain);
          graphicsCpuTime.startFrame();

          CommandGraph tasks = dev.createGraph();

          {
            auto node = tasks.createPass("compute test");
            auto binding = node.bind(compute);
            binding.bind("test", testSRV);
            binding.bind("test2", test2SRV);
            binding.bind("testOut", testOut);

            uint3 groups = uint3(1, 1, 1);
            node.dispatch(binding, groups);

			      tasks.addPass(std::move(node));
          }
          {
            auto node = tasks.createPass("Triangle");
            node.acquirePresentableImage(swapchain);
            float redcolor = std::sin(time.getFTime())*.5f + .5f;

            backbuffer.clearOp(float4{ 0.f, 0.f, redcolor, 0.f });
            node.renderpass(triangleRP, backbuffer);
            {
              auto binding = node.bind(triangle);

              PixelConstants consts{};
              consts.time = time.getFTime();
              consts.resx = backbuffer.desc().desc.width;
              consts.resy = backbuffer.desc().desc.height;
              binding.constants(consts);

              node.draw(binding, 3);
            }
            node.endRenderpass();

			      tasks.addPass(std::move(node));
          }
          {
            auto node = tasks.createPass("preparePresent");

            node.prepareForPresent(backbuffer);
            node.queryCounters([&](MemView<std::pair<std::string, double>> view)
            {
              counters.clear();
              for (auto&& it : view)
              {
                counters.push_back(std::make_pair(it.first, it.second));
              }
            });
            tasks.addPass(std::move(node));
          }

          dev.submit(swapchain, tasks);

          graphicsCpuTime.tick();
          dev.present(swapchain);
          time.tick();
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

  lbs.addTask("test1", [&](size_t) {main(GraphicsApi::DX12, VendorID::Nvidia, true); });
  lbs.sleepTillKeywords({ "test1" });

#endif
  /*
  RangeMath::difference({ 0, 5, 0, 4 }, { 2, 1, 1, 2 }, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({0, 5, 1, 2}, {2, 1, 0, 5}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({1, 2, 0, 4}, {0, 5, 1, 2}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({2, 1, 1, 2}, {0, 5, 0, 4}, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({ 0, 2, 0, 2 }, { 1, 1, 1, 1 }, [](SubresourceRange r) {printRange(r);});
  RangeMath::difference({ 0, 1, 0, 1 }, { 1, 1, 1, 1 }, [](SubresourceRange r) {printRange(r);});
  */

  log.update();
}
