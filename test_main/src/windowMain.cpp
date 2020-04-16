#include "windowMain.hpp"
#include <higanbana/core/system/LBS.hpp>
#include <higanbana/core/filesystem/filesystem.hpp>
#include <higanbana/core/entity/database.hpp>
#include <higanbana/core/platform/Window.hpp>
#include <higanbana/core/system/logger.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/entity/bitfield.hpp>
#include <higanbana/core/system/misc.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/core/input/gamepad.hpp>
#include <higanbana/core/system/AtomicBuffered.hpp>
#include <higanbana/graphics/GraphicsCore.hpp>
#include <random>
#include <tuple>
#include <imgui.h>
#include <cxxopts.hpp>

#include "entity_test.hpp"
#include "rendering.hpp"
#include "world/world.hpp"
#include "world/components.hpp"
#include "world/entity_viewer.hpp"
#include "world/scene_editor.hpp"
#include "world/entity_editor.hpp"
#include "world/visual_data_structures.hpp"
#include "world/map_data_extractor.hpp"

#include <higanbana/core/profiling/profiling.hpp>

using namespace higanbana;
using namespace higanbana::math;

vector<std::string> splitByDelimiter(std::string data, const char* delimiter)
{
  vector<std::string> splits;
  
  size_t pos = 0;
  std::string token;
  while ((pos = data.find(delimiter)) != std::string::npos) {
    token = data.substr(0, pos);
    splits.emplace_back(token);
    data.erase(0, pos + strlen(delimiter));
  }
  return splits;
}

vector<int> convertToInts(vector<std::string> data)
{
  vector<int> ints;
  for (auto&& s : data)
  {
    try
    {
      auto val = std::stoi(s.c_str());
      ints.push_back(val);
    } catch (...)
    {
    }
  }
  return ints;
}

void handleInputs(Database<2048>& ecs, gamepad::X360LikePad& input, MouseState& mouse, InputBuffer& inputs, float delta, bool captureMouse)
{
  float3 rxyz{ 0 };
  float2 xy{ 0 };

  query(pack(ecs.get<components::Position>(), ecs.get<components::Rotation>()), pack(ecs.getTag<components::ActiveCamera>()),
  [&](higanbana::Id, components::Position& pos, components::Rotation& rot)
  {
    quaternion& direction = rot.rot;
    float3& position = pos.pos;

    float3 dir = math::normalize(rotateVector({ 0.f, 0.f, 1.f }, direction));
    float3 updir = math::normalize(rotateVector({ 0.f, 1.f, 0.f }, direction));
    float3 sideVec = math::normalize(rotateVector({ 1.f, 0.f, 0.f }, direction));

    //controllerConnected = false;
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

      rxyz = mul(rxyz, float3(1.f, -1.f, 0));
      xy = mul(xy, float2(-1, -1));

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

      rxyz = math::mul(rxyz, std::max(delta, 0.001f));
      xy = math::mul(xy, std::max(delta, 0.001f));

      quaternion yaw = math::rotateAxis(updir, rxyz.x);
      quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
      quaternion roll = math::rotateAxis(dir, rxyz.z);
      direction = math::mul(math::mul(math::mul(yaw, pitch), roll), direction);
    }
    else if (captureMouse)
    {
      auto m = mouse;
      //F_LOG("print mouse %d %d\n", m.m_pos.x, m.m_pos.y);
      auto p = m.m_pos;
      auto floatizeMouse = [](int input, float& output)
      {
        constexpr float max = static_cast<float>(100);
        output = std::min(static_cast<float>(std::abs(input)) / max, max);
        output *= (input < 0) ? 1 : -1;
      };

      floatizeMouse(p.x, rxyz.x);
      floatizeMouse(p.y, rxyz.y);

      if (inputs.isPressedThisFrame('Q', 2))
      {
        rxyz.z = 1.f;
      }
      if (inputs.isPressedThisFrame('E', 2))
      {
        rxyz.z = -1.f;
      }

      rxyz = math::mul(rxyz, std::max(delta, 0.001f));
      rxyz.x *= 50.f;
      rxyz.y *= 50.f;

      quaternion yaw = math::rotateAxis(updir, rxyz.x);
      quaternion pitch = math::rotateAxis(sideVec, rxyz.y);
      quaternion roll = math::rotateAxis(dir, rxyz.z);
      direction = math::normalize(math::mul(math::mul(math::mul(yaw, pitch), roll), direction));

      float multiplier = 1.f;
      if (inputs.isPressedThisFrame(VK_SHIFT, 2))
      {
        multiplier = 1000.f;
      }
      if (inputs.isPressedThisFrame('W', 2))
      {
        xy.y = -1.f * multiplier;
      }
      if (inputs.isPressedThisFrame('S', 2))
      {
        xy.y = 1.f * multiplier;
      }
      if (inputs.isPressedThisFrame('D', 2))
      {
        xy.x = -1.f * multiplier;
      }
      if (inputs.isPressedThisFrame('A', 2))
      {
        xy.x = 1.f * multiplier;
      }
      xy = math::mul(xy, std::max(delta, 0.001f));
    }

    // use our current up and forward vectors and calculate our new up and forward vectors
    // yaw, pitch, roll

    direction = math::normalize(direction);

    position = math::add(position, math::mul(sideVec, xy.x));
    position = math::add(position, math::mul(dir, xy.y));
  });
}

void mainWindow(ProgramParams& params)
{
  cxxopts::Options options("TestMain", "TestMain tests some practical functionality of Higanbana.");
  options.add_options()
    ("rgp", "forces Radeon GPU Profiler, allows only AMD gpu's.")
    ("vulkan", "priorizes Vulkan API")
    ("dx12", "priorizes DirectX 12 API")
    ("intel", "priorizes Intel GPU's")
    ("nvidia", "priorizes NVidia GPU's")
    ("amd", "priorizes AMD GPU's")
    ("gfx_debug", "enables vulkan/dx12 api validation layer")
    ("h,help", "Print help and exit.")
    ;

  GraphicsApi cmdlineApiId = GraphicsApi::DX12;
  VendorID cmdlineVendorId = VendorID::Amd;
  GraphicsApi allowedApis = GraphicsApi::All;
  bool rgpCapture = false;
  bool validationLayer = false;
  try
  {
    auto results = options.parse(params.m_argc, params.m_argv);
    if (results.count("vulkan"))
    {
      cmdlineApiId = GraphicsApi::Vulkan;
    }
    if (results.count("dx12"))
    {
      cmdlineApiId = GraphicsApi::DX12;
    }
    if (results.count("amd"))
    {
      cmdlineVendorId = VendorID::Amd;
    }
    if (results.count("intel"))
    {
      cmdlineVendorId = VendorID::Intel;
    }
    if (results.count("nvidia"))
    {
      cmdlineVendorId = VendorID::Nvidia;
    }
    if (results.count("gfx_debug"))
    {
      validationLayer = true;
    }
    if (results.count("rgp"))
    {
      rgpCapture = true;
      cmdlineVendorId = VendorID::Amd;
      HIGAN_LOGi("Preparing for RGP capture...\n");
      allowedApis = cmdlineApiId;
    }
    if (results.count("help"))
    {
      HIGAN_LOGi("%s\n", options.help({""}).c_str());
      Sleep(10000);
      return;
    }
  }
  catch(const std::exception& e)
  {
    //std::cerr << e.what() << '\n';
    HIGAN_LOGi("Bad commandline! reason: %s\n%s", e.what(), options.help({""}).c_str());
    Sleep(10000);
    return;
  }
  
  Logger log;
  // test_entity();
  // test_bitfield();
  auto main = [&](GraphicsApi api, VendorID preferredVendor, bool updateLog)
  {
    HIGAN_LOG("Trying to start with %s api and %s vendor\n", toString(api), toString(preferredVendor));
    bool reInit = false;
    int64_t frame = 1;
    FileSystem fs("/../../data");
    auto coolHeightmap = app::readInfoFromOpenMapDataASC(fs);
    Database<2048> ecs;

    app::World world;
    higanbana::gamepad::Controllers inputs;
    world.loadGLTFScene(ecs, fs, "/scenes");
    app::EntityView entityViewer;
    app::SceneEditor sceneEditor;
    app::EntityEditor entityEditor;
    bool renderGltfScene = false;
    bool renderECS = false;
    int cubeCount = 30;
    int cubeCommandLists = 4;
    int limitFPS = -1;
    int viewportCount = 1;
    bool advanceSimulation = true;
    bool stepOneFrameForward = false;
    bool stepOneFrameBackward = false;
    app::RendererOptions renderOptions;
    renderOptions.submitSingleThread = false;
    vector<app::RenderViewportInfo> rendererViewports;
    rendererViewports.push_back({});

    {
      auto& name = ecs.get<components::Name>();
      auto& childs = ecs.get<components::Childs>();
      auto& sceneRoot = ecs.getTag<components::SceneEntityRoot>();
      auto e = ecs.createEntity();
      name.insert(e, components::Name{"Scene Root"});
      childs.insert(e, components::Childs{});
      sceneRoot.insert(e);
    }
    {
      auto& t_pos = ecs.get<components::Position>();
      auto& t_rot = ecs.get<components::Rotation>();
      auto& t_cameraSet = ecs.get<components::CameraSettings>();

      auto cid = ecs.createEntity();
      t_pos.insert(cid, {float3(0, 0 , 4)});
      t_rot.insert(cid, {quaternion{ 1.f, 0.f, 0.f, 0.f }});
      t_cameraSet.insert(cid, {90.f, 0.01f, 100.f});
      ecs.getTag<components::ActiveCamera>().insert(cid);
    }

    bool quit = false;
    LBS lbs;

    log.update();

    WTime time;
    WTime logicAndRenderTime;

    uint chosenDeviceID = 0;
    GraphicsApi chosenApi = api;
    bool interoptDevice = false;
    int2 windowSize = div(int2(1280, 720), 1);
    int2 windowPos = int2(300,400);

    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    {
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      io.IniFilename = nullptr;
      if (fs.fileExists("/imgui.config")) {
        auto file = fs.viewToFile("/imgui.config");
        ImGui::LoadIniSettingsFromMemory(reinterpret_cast<const char*>(file.data()), file.size_bytes());
      }
      io.ConfigFlags = ImGuiConfigFlags_DockingEnable;
    }
    ImGui::StyleColorsDark();

    while (true)
    {
      HIGAN_CPU_BRACKET("main program");
      vector<GpuInfo> allGpus;
      vector<GpuInfo> activeDevices;
      vector<bool> selectedDevice;
      GpuInfo physicalDevice;
      GraphicsSubsystem graphics(allowedApis, "higanbana", validationLayer);
      if (rgpCapture)
      {
        allGpus = graphics.availableGpus(api, VendorID::Amd);
        if (!allGpus.empty()) {
          physicalDevice = allGpus[0];
        }
        else
          HIGAN_LOGi("Failed to find AMD RGP compliant device in %s api", toString(api));
      }
      else
      {
        allGpus = graphics.availableGpus();
        if (chosenDeviceID == 0) {
          auto expApi = api;
          if (interoptDevice)
            expApi = GraphicsApi::All;
          physicalDevice = graphics.getVendorDevice(expApi, preferredVendor);
          chosenDeviceID = physicalDevice.deviceId;
          if (!interoptDevice)
            chosenApi = physicalDevice.api;
        } 
        else
        {
          for (auto&& gpu : allGpus) {
            if (gpu.deviceId == chosenDeviceID) {
              physicalDevice = gpu;
              if (!interoptDevice)
                physicalDevice.api = chosenApi;
              break;
            }
          }
        }
      }
      if (updateLog) log.update();
      if (allGpus.empty()) {
        HIGAN_LOGi("No gpu's found, exiting...\n");
        return;
      }



      higanbana::GpuGroup dev;
      if (interoptDevice && physicalDevice.api == GraphicsApi::All)
        dev = graphics.createInteroptDevice(fs, physicalDevice);
      else
        dev = graphics.createDevice(fs, physicalDevice);
      
      activeDevices = dev.activeDevicesInfo();

	    std::string windowTitle = "";
      {
        int devId = 0;
        for(auto&& info : activeDevices)
          windowTitle += std::to_string(devId++) + std::string(": ") + std::string(toString(info.api)) + ": " + info.name + " ";
      }
      Window window(params, windowTitle, windowSize.x, windowSize.y, windowPos.x, windowPos.y);
      window.open();

      selectedDevice.resize(allGpus.size());
      for (int i = 0; i < allGpus.size(); ++i) {
        if (allGpus[i].deviceId == chosenDeviceID)
          selectedDevice[i] = true;
      }
      app::Renderer rend(graphics, dev);

      // Load meshes to gpu
      {
        HIGAN_CPU_BRACKET("Load meshes to gpu");
        auto& gpuBuffer = ecs.get<components::GpuBufferInstance>();
        query(pack(ecs.get<components::RawBufferData>()), // pack(ecs.getTag<components::MeshNode>()),
          [&](higanbana::Id id, components::RawBufferData index) {
          components::GpuBufferInstance instance{rend.loadBuffer(world.getBuffer(index.id))};
          gpuBuffer.insert(id, instance);
        });
      }
      {
        HIGAN_CPU_BRACKET("link meshes to gpu meshes");
        auto& gpuMesh = ecs.get<components::GpuMeshInstance>();
        auto& gpuBuffer = ecs.get<components::GpuBufferInstance>();
        query(pack(ecs.get<components::RawMeshData>()), // pack(ecs.getTag<components::MeshNode>()),
          [&](higanbana::Id id, components::RawMeshData index) {
          auto mesh = world.getMesh(index.id);
          int buffers[5];
          if (auto buf = gpuBuffer.tryGet(mesh.indices.buffer)) {
            buffers[0] = buf.value().id;
          }
          if (auto buf = gpuBuffer.tryGet(mesh.vertices.buffer)) {
            buffers[1] = buf.value().id;
          }
          if (auto buf = gpuBuffer.tryGet(mesh.normals.buffer)) {
            buffers[2] = buf.value().id;
          }
          if (auto buf = gpuBuffer.tryGet(mesh.texCoords.buffer)) {
            buffers[3] = buf.value().id;
          }
          if (auto buf = gpuBuffer.tryGet(mesh.tangents.buffer)) {
            buffers[4] = buf.value().id;
          }
          components::GpuMeshInstance instance{rend.loadMesh(mesh, buffers)};
          gpuMesh.insert(id, instance);
        });
      }
      // load textures to gpu
      {
        HIGAN_CPU_BRACKET("load textures to gpu");
        auto& gpuTexture = ecs.get<components::GpuTextureInstance>();
        query(pack(ecs.get<components::RawTextureData>()), // pack(ecs.getTag<components::MeshNode>()),
          [&](higanbana::Id id, components::RawTextureData index) {
          components::GpuTextureInstance instance{rend.loadTexture(world.getTexture(index.id))};
          gpuTexture.insert(id, instance);
        });
      }
      // link initial textures to materials
      {
        HIGAN_CPU_BRACKET("link textures to existing materials");
        auto& textureInstance = ecs.get<components::GpuTextureInstance>();
        query(pack(ecs.get<MaterialData>(), ecs.get<components::MaterialLink>()), // pack(ecs.getTag<components::MeshNode>()),
        [&](higanbana::Id id, MaterialData& material, components::MaterialLink& link) {
          auto getInstance = [&](higanbana::Id linkid){
            if (linkid != higanbana::Id(-1))
            {
              auto instance = textureInstance.tryGet(linkid);
              if (instance){
                return instance->id+1;
              }
            }
            return 0;
          };
          material.albedoIndex = getInstance(link.albedo); // baseColorTexIndex
          material.normalIndex = getInstance(link.normal); // normalTexIndex
          material.metallicRoughnessIndex = getInstance(link.metallicRoughness);
          material.occlusionIndex = getInstance(link.occlusion);
          material.emissiveIndex = getInstance(link.emissive);
        });
      }

      // make initial materials
      {
        HIGAN_CPU_BRACKET("upload material structs to gpu");
        auto& gpuMaterial = ecs.get<components::GpuMaterialInstance>();
        query(pack(ecs.get<MaterialData>()), // pack(ecs.getTag<components::MeshNode>()),
        [&](higanbana::Id id, MaterialData material) {
          int gpuid = rend.loadMaterial(material);
          components::GpuMaterialInstance instance{gpuid};
          gpuMaterial.insert(id, instance);
        });
      }
      {
        auto toggleHDR = false;
        rend.initWindow(window, physicalDevice);

        HIGAN_LOGi("Created device \"%s\"\n", physicalDevice.name.c_str());

        bool closeAnyway = false;
        bool renderActive = true;
        bool renderResize = false;
        bool captureMouse = false;
        bool controllerConnected = false;
        bool autoRotateCamera = false;

        AtomicBuffered<InputBuffer> ainputs;
        AtomicBuffered<gamepad::X360LikePad> agamepad;
        ainputs.write(window.inputs());
        AtomicBuffered<MouseState> amouse;
        amouse.write(window.mouse());
        std::atomic<int> inputsUpdated = 0;
        int inputsRead = 0;

        lbs.addTask("logic&render loop", [&](size_t) {
          float timeSinceLastInput = 0;
          while (renderActive)
          {
            HIGAN_CPU_BRACKET("Logic&Render loop begin!");

            ImGuiIO &io = ::ImGui::GetIO();
            auto windowSize = rend.windowSize();
            io.DisplaySize = { float(windowSize.x), float(windowSize.y) };
            if (advanceSimulation || stepOneFrameForward)
            {
              time.tick();
              stepOneFrameForward = false;
            }
            logicAndRenderTime.startFrame();
            io.DeltaTime = time.getFrameTimeDelta();
            if (io.DeltaTime < 0.f)
              io.DeltaTime = 0.00001f;
            auto lastRead = inputsRead;
            auto inputs = ainputs.read();
            gamepad::X360LikePad pad = agamepad.read();
            auto currentInput = inputs.frame();
            auto diffSinceLastInput = currentInput - lastRead;
            auto diffWithWriter = inputsUpdated.load() - currentInput;
            if (diffSinceLastInput > 0)
            {
              HIGAN_CPU_BRACKET("reacting to new input");
              inputsRead = currentInput;
              auto mouse = amouse.read();


              if (mouse.captured)
              {
                io.MousePos.x = static_cast<float>(mouse.m_pos.x);
                io.MousePos.y = static_cast<float>(mouse.m_pos.y);

                io.MouseWheel = static_cast<float>(mouse.mouseWheel)*0.1f;
              }

              inputs.goThroughNFrames(diffSinceLastInput, [&](Input i)
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

              io.KeyCtrl = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_CONTROL, 2);
              io.KeyShift = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_SHIFT, 2);
              io.KeyAlt = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2);
              io.KeySuper = false;

              if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_F1, 1))
              {
                window.captureMouse(true);
                captureMouse = true;
              }
              if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_F2, 1))
              {
                window.captureMouse(false);
                captureMouse = false;
              }
              if (inputs.isPressedWithinNFrames(diffSinceLastInput, 220, 1)) // tilde... or something like that
              {
                renderOptions.renderImGui = !renderOptions.renderImGui;
              }

              if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '1', 1))
              {
                window.toggleBorderlessFullscreen();
              }

              if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '2', 1))
              {
                reInit = true;
                if (api == GraphicsApi::DX12)
                  api = GraphicsApi::Vulkan;
                else
                  api = GraphicsApi::DX12;
                chosenDeviceID = 0;
                renderActive = false;
              }

              if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '3', 1))
              {
                reInit = true;
                if (preferredVendor == VendorID::Amd)
                  preferredVendor = VendorID::Nvidia;
                else
                  preferredVendor = VendorID::Amd;
                renderActive = false;
              }

              if (frame > 10 && (closeAnyway || inputs.isPressedWithinNFrames(diffSinceLastInput, VK_ESCAPE, 1)))
              {
                renderActive = false;
              }
              auto timestep = timeSinceLastInput + time.getFrameTimeDelta();
              handleInputs(ecs, pad, mouse, inputs, timestep, captureMouse);
              timeSinceLastInput = 0.f;
            }
            else {
              timeSinceLastInput += time.getFrameTimeDelta();
            }
            ::ImGui::NewFrame();

            if (renderOptions.renderImGui)
            {
              auto gid = ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Keys"))
              {
                ImGui::Text(" Exit:                         ESC");
                ImGui::Text(" Toggle ImGui:                 ~ or \"console key\"");
                ImGui::Text(" Capture Mouse(fps controls):  F1");
                ImGui::Text(" Release Mouse:                F2");
                ImGui::Text(" Toggle Borderless Fullscreen: Alt + 1");
                ImGui::Text("   - Just dont switch api or vendor while borderless fullscreen.");
                ImGui::Text(" Toggle GFX API Vulkan/DX12:   Alt + 2");
                ImGui::Text(" Toggle GPU Vendor Nvidia/AMD: Alt + 3");
              }
              ImGui::End();
              ImGui::SetNextWindowSize(ImVec2(660, 580), ImGuiCond_FirstUseEver);
              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              int enabledViewport = 0;
              for (int i = 0; i < static_cast<int>(rendererViewports.size()); ++i) {
                const auto& gpInfo = activeDevices[rendererViewports[i].options.gpuToUse];
                rendererViewports[i].options.visible = false;
                std::string viewportName = std::string("Viewport ") + std::to_string(i);
                ImVec2 windowOffset;
                ImVec2 windowSize;
                if (ImGui::Begin(viewportName.c_str())) {
                  auto ws = ImGui::GetWindowSize();
                  windowSize = ws;
                  windowOffset = ImGui::GetWindowPos();
                  ws.y -= 36;
                  rendererViewports[i].options.visible = true;
                  ImGui::Image(reinterpret_cast<void*>(enabledViewport+1), ws);
                  rendererViewports[i].viewportSize = int2(ws.x, ws.y);
                  rendererViewports[i].viewportIndex = enabledViewport+1;
                  enabledViewport++;
                }
                ImGui::End();
                const float DISTANCE = 20.0f;
                static int corner = 0;
                ImGuiIO& io = ImGui::GetIO();
                if (rendererViewports[i].options.visible)
                {
                  ImVec2 window_pos = ImVec2((corner & 1) ? (windowOffset.x + windowSize.x - DISTANCE) : (windowOffset.x + DISTANCE), (corner & 2) ? (windowOffset.y + windowSize.y - DISTANCE) : (windowOffset.y + DISTANCE));
                  ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
                  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
                  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
                }
                viewportName += "##overlay";
                if (rendererViewports[i].options.visible && ImGui::Begin(viewportName.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
                {
                    std::string gpu = std::string(toString(gpInfo.api)) + ": " + gpInfo.name;
                    ImGui::Text(gpu.c_str());
                    /*
                    ImGui::Separator();
                    if (ImGui::IsMousePosValid())
                        ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x - windowOffset.x, io.MousePos.y-windowOffset.y);
                    else
                        ImGui::Text("Mouse Position: <invalid>");
                        */
                }
                if (rendererViewports[i].options.visible)
                  ImGui::End();
              }
              ImGui::SetNextWindowSize(ImVec2(360, 580), ImGuiCond_FirstUseEver);
              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("main")) {
                if (ImGui::ListBoxHeader("device selection", ImVec2(0, allGpus.size()*18))) {
                  int i = 0;
                  for (auto&& device : allGpus) {
                    if (ImGui::Selectable(device.name.c_str(), selectedDevice[i]))
                      selectedDevice[i] = !selectedDevice[i];
                    i++;
                  }
                  ImGui::ListBoxFooter();
                }
                ImGui::Checkbox("interopt", &interoptDevice); ImGui::SameLine();
                if (ImGui::Button("Reinit")) {
                  for (int i = 0; i < selectedDevice.size(); i++) {
                    if (selectedDevice[i]) {
                      chosenDeviceID = allGpus[i].deviceId;
                      reInit = true;
                      renderActive = false;
                      break;
                    }
                  }
                }
                ImGui::Text("%zd frames since last inputs. %zd diff to Writer, current: %zd read %d", diffSinceLastInput, diffWithWriter, currentInput, lastRead);

                ImGui::Text("average FPS %.2f (%.2fms)", 1000.f / time.getCurrentFps(), time.getCurrentFps());
                ImGui::Text("max FPS %.2f (%.2fms)", 1000.f / time.getMaxFps(), time.getMaxFps());
                ImGui::DragInt("FPS limit", &limitFPS, 1, -1, 144);
                ImGui::Checkbox("Readback shader prints", &higanbana::globalconfig::graphics::GraphicsEnableShaderDebug);
                ImGui::Checkbox("rotate camera", &autoRotateCamera);
                ImGui::Checkbox("simulate", &advanceSimulation);
                if (!advanceSimulation)
                {
                  ImGui::Checkbox("   Step Frame Forward", &stepOneFrameForward);
                  ImGui::Checkbox("   Step Frame Backward", &stepOneFrameBackward);
                  if (stepOneFrameForward)
                  {
                    time.startFrame();
                  }
                }
                ImGui::Text("Validation Layer %s", validationLayer ? "Enabled" : "Disabled");

                {
                  ImGui::Checkbox("render selected glTF scene", &renderGltfScene);
                  ImGui::Checkbox("render current scene", &renderECS);
                  int activeViewports = 0;
                  for (int i = 0; i < static_cast<int>(rendererViewports.size()); i++)
                    if (rendererViewports[i].options.visible)
                      activeViewports++;
                  ImGui::Text("%d cubes * %d viewports", cubeCount, activeViewports);
                  ImGui::Text("= %d draw calls / frame", cubeCount*activeViewports);
                  ImGui::Text("Preallocated constant memory of 380mb or so");
                  ImGui::Text("Exceeding >350k of drawcalls / frame crashes.");
                  ImGui::Text("Thank you.");
                  ImGui::DragInt("drawcalls/cubes", &cubeCount, 10, 0, 500000);
                  ImGui::DragInt("drawcalls split into", &cubeCommandLists, 1, 1, 128);
                }
              }
              ImGui::End();

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Renderer options")) {
                ImGui::Text("Active devices:");
                for (int i = 0; i < activeDevices.size(); ++i) {
                  const auto& info = activeDevices[i];
                  std::string str = std::to_string(i) + std::string(": ")  + info.name + " (" + std::string(toString(info.api))+ ")";
                  ImGui::Text(str.c_str());
                }
                renderOptions.drawImGuiOptions();
              }
              ImGui::End();

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Renderer options for viewports")) {
                ImGui::SliderInt("viewport count", &viewportCount, 1, 16);
                rendererViewports.resize(viewportCount, app::RenderViewportInfo{1, int2(16, 16),{},{}});
                ImGui::BeginTabBar("viewports");
                for (int i = 0; i < static_cast<int>(rendererViewports.size()); i++) {
                  std::string index = std::string("viewport ") + std::to_string(i);
                  auto& vp = rendererViewports[i];
                  if (ImGui::BeginTabItem(index.c_str())) {
                    int2 currentRes = math::mul(vp.options.resolutionScale, float2(vp.viewportSize));
                    ImGui::Text("resolution %dx%d", currentRes.x, currentRes.y); //ImGui::SameLine();
                    vp.options.drawImGuiOptions(activeDevices);
                    ImGui::EndTabItem();
                  }
                }
                ImGui::EndTabBar();
              }
              ImGui::End();

              auto si = rend.timings();
              
              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Renderpass") && si)
              {
                ImGui::Text("RenderGraph statistics:");
                auto rsi = si.value();
                float gpuTotal = 0.f;
                for (auto& list : rsi.lists)
                {
                  gpuTotal += list.gpuTime.milliseconds();
                }

                uint64_t draws = 0, dispatches = 0;
                uint64_t bytesInCommandBuffersTotal = 0;
                for (auto&& cmdlist : rsi.lists)
                {
                  for (auto&& node : cmdlist.nodes)
                  {
                    draws += node.draws;
                    dispatches += node.dispatches;
                    bytesInCommandBuffersTotal += node.cpuSizeBytes;
                  }
                }

                float cpuTotal = rsi.timeBeforeSubmit.milliseconds() + rsi.submitCpuTime.milliseconds();
                if (gpuTotal > cpuTotal)
                  ImGui::Text("Bottleneck: GPU");
                else
                  ImGui::Text("Bottleneck: CPU");
                auto multiplier = 1000.f / time.getCurrentFps();
                auto allDraws = float(draws) * multiplier;
                int dps = static_cast<int>(allDraws);
                ImGui::Text("Drawcalls per second %zu", dps);
                ImGui::Text("GPU execution %.3fms", gpuTotal);
                ImGui::Text("CPU execution %.3fms", cpuTotal);
                ImGui::Text("Acquire time taken %.3fms", rend.acquireTimeTakenMs());
                ImGui::Text("GraphNode size %.3fMB", bytesInCommandBuffersTotal/1024.f/1024.f);
                ImGui::Text("- user fill time %.2fms", rsi.timeBeforeSubmit.milliseconds());
                ImGui::Text("- combine nodes %.3fms", rsi.addNodes.milliseconds());
                ImGui::Text("- Graph solving %.3fms", rsi.graphSolve.milliseconds());
                ImGui::Text("- Filling Lists %.3fms", rsi.fillCommandLists.milliseconds());
                ImGui::Text("- Submitting Lists %.3fms", rsi.submitSolve.milliseconds());
                ImGui::Text("- Submit total %.2fms", rsi.submitCpuTime.milliseconds());

                int listIndex = 0;
                for (auto& cmdlist : rsi.lists)
                {
                  std::string listName = toString(cmdlist.type);
                  listName += " list " + std::to_string(listIndex++);
                  if (ImGui::TreeNode(listName.c_str()))
                  {
                    //ImGui::Text("\n%s Commandlist", toString(cmdlist.type));
                    ImGui::Text("\tGPU time %.3fms", cmdlist.gpuTime.milliseconds());
                    ImGui::Text("\t- totalTimeOnGPU %.3fms", cmdlist.fromSubmitToFence.milliseconds());
                    ImGui::Text("\t- barrierPrepare %.3fms", cmdlist.barrierAdd.milliseconds());
                    ImGui::Text("\t- barrierSolveLocal %.3fms", cmdlist.barrierSolveLocal.milliseconds());
                    ImGui::Text("\t- barrierSolveGlobal %.3fms", cmdlist.barrierSolveGlobal.milliseconds());
                    ImGui::Text("\t- fillNativeList %.3fms", cmdlist.fillNativeList.milliseconds());
                    ImGui::Text("\t- cpuBackendTime(?) %.3fms", cmdlist.cpuBackendTime.milliseconds());
                    ImGui::Text("\tGPU nodes:");
                    for (auto& graphNode : cmdlist.nodes)
                    {
                      ImGui::Text("\t\t%s %.3fms (cpu %.3fms)", graphNode.nodeName.c_str(), graphNode.gpuTime.milliseconds(), graphNode.cpuTime.milliseconds());
                    }
                    ImGui::TreePop();
                  }
                }
              }
              ImGui::End();

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Gpu statistics"))
              {
                auto stats = dev.gpuStatistics();
                auto bytesToMb = [](uint64_t bytes){ return bytes / 1024.f / 1024.f; };
                auto unitsToK = [](uint64_t units){ return units / 1000.f; };
                for (auto& stat : stats)
                {
                  ImGui::Text("Commandbuffers on gpu       %zu", stat.commandlistsOnGpu);
                  ImGui::Text("GPU Memory allocated        %.2fmb / %.2fmb  %.2f%%", bytesToMb(stat.gpuMemoryAllocated), bytesToMb(stat.gpuTotalMemory), bytesToMb(stat.gpuMemoryAllocated) / bytesToMb(stat.gpuTotalMemory) * 100.f);
                  ImGui::Text("Memory used by constants    %.2fmb / %.2fmb  %.2f%%", bytesToMb(stat.constantsUploadMemoryInUse), bytesToMb(stat.maxConstantsUploadMemory), bytesToMb(stat.constantsUploadMemoryInUse) / bytesToMb(stat.maxConstantsUploadMemory) * 100.f);
                  ImGui::Text("Generic upload memory used  %.2fmb / %.2fmb  %.2f%%", bytesToMb(stat.genericUploadMemoryInUse), bytesToMb(stat.maxGenericUploadMemory), bytesToMb(stat.genericUploadMemoryInUse) / bytesToMb(stat.maxGenericUploadMemory) * 100.f);
                  if (stat.descriptorsInShaderArguments)
                    ImGui::Text("ShaderArguments             %zu / %zu  %.2f%%", stat.descriptorsAllocated, stat.maxDescriptors, float(stat.descriptorsAllocated) / float(stat.maxDescriptors) * 100.f);
                  else
                    ImGui::Text("Descriptors                 %.2fk / %.2fk %.2f%%", unitsToK(stat.descriptorsAllocated), unitsToK(stat.maxDescriptors), unitsToK(stat.descriptorsAllocated) / unitsToK(stat.maxDescriptors) * 100.f);
                }
              }
              ImGui::End();
              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              if (ImGui::Begin("Camera"))
              {
                auto& t_pos = ecs.get<components::Position>();
                auto& t_rot = ecs.get<components::Rotation>();
                auto& t_cameraSet = ecs.get<components::CameraSettings>();
                query(pack(t_pos, t_rot, t_cameraSet), pack(ecs.getTag<components::ActiveCamera>()),
                [&](higanbana::Id id, components::Position& pos, components::Rotation& rot, components::CameraSettings& set)
                {
                  float3 dir = math::normalize(rotateVector({ 0.f, 0.f, -1.f }, rot.rot));
                  float3 updir = math::normalize(rotateVector({ 0.f, 1.f, 0.f }, rot.rot));
                  float3 sideVec = math::normalize(rotateVector({ 1.f, 0.f, 0.f }, rot.rot));
                  float3 xyz{};
                  if (autoRotateCamera)
                  {
                    float circleMultiplier = -1.2f;
                    pos.pos = math::add(pos.pos, math::mul(circleMultiplier * time.getFrameTimeDelta(), sideVec));
                    xyz.x = -0.23* time.getFrameTimeDelta();
                  }
                  ImGui::DragFloat3("position", pos.pos.data);
                  ImGui::DragFloat3("xyz", xyz.data, 0.01f);
                  ImGui::DragFloat4("quaternion", rot.rot.data, 0.01f);
                  ImGui::DragFloat3("forward", dir.data);
                  ImGui::DragFloat3("side", sideVec.data);
                  ImGui::DragFloat3("up", updir.data);
                  ImGui::DragFloat("fov", &set.fov, 0.1f);
                  ImGui::DragFloat("minZ", &set.minZ, 0.1f);
                  ImGui::DragFloat("maxZ", &set.maxZ, 1.f);
                  quaternion yaw = math::rotateAxis(updir, xyz.x);
                  quaternion pitch = math::rotateAxis(sideVec, xyz.y);
                  quaternion roll = math::rotateAxis(dir, xyz.z);
                  rot.rot = math::mul(math::mul(math::mul(yaw, pitch), roll), rot.rot);
                });
              }
              ImGui::End();

              // render entities
              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              entityViewer.render(ecs);

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              sceneEditor.render(ecs);

              ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
              entityEditor.render(ecs);
            }
            ImGui::Render();

            // collect all model instances and their matrices for drawing...
            vector<InstanceDraw> allMeshesToDraw;
            if (renderGltfScene)
            {
              HIGAN_CPU_BRACKET("collecting gltf mesh instances to render");

              struct ActiveScene
              {
                components::SceneInstance target;
                float3 wp;
              };
              higanbana::vector<ActiveScene> scenes;
              query(pack(ecs.get<components::SceneInstance>(), ecs.get<components::Position>()), [&](higanbana::Id id, components::SceneInstance scene, components::Position wp)
              {
                scenes.push_back(ActiveScene{scene, wp.pos});
              });

              auto& children = ecs.get<components::Childs>();

              auto findMeshes = [&](higanbana::Id id) -> std::optional<components::Childs>
              {
                // check if has meshes -> 
                if (auto hasMesh = ecs.get<components::Mesh>().tryGet(id))
                {
                  for (auto&& childMesh : children.get(hasMesh.value().target).childs)
                  {
                    auto meshTarget = ecs.get<components::GpuMeshInstance>().tryGet(childMesh);

                    InstanceDraw draw{};
                    if (meshTarget) {
                      auto material = ecs.get<components::MaterialInstance>().tryGet(childMesh);
                      if (material) {
                        auto matID = ecs.get<components::GpuMaterialInstance>().get(material->id);
                        draw.materialId = matID.id;
                      }
                      draw.meshId = meshTarget.value().id;
                      draw.mat = float4x4::identity();
                      allMeshesToDraw.push_back(draw);
                    }
                  }
                }
                return children.tryGet(id);
              };
              vector<vector<higanbana::Id>> stack;
              if (auto c0 = children.tryGet(scenes.back().target.target))
              {
                stack.push_back(c0.value().childs);
              }
              
              if (auto base = scenes.back().target.target)
              while(!stack.empty())
              {
                
                if (stack.back().empty())
                {
                  stack.pop_back();
                  continue;
                }
                auto& workingSet = stack.back();
                auto val = workingSet.back();
                workingSet.pop_back();
                if (auto new_chlds = findMeshes(val))
                {
                  stack.push_back(new_chlds.value().childs);
                }
              }
            }
            else if (renderECS) {
              HIGAN_CPU_BRACKET("collecting entity instances to render");

              auto& positions = ecs.get<components::Position>();
              auto& rotations = ecs.get<components::Rotation>();
              auto& scales = ecs.get<components::Scale>();
              auto& meshes = ecs.get<components::MeshInstance>();
              auto& materials = ecs.get<components::MaterialInstance>();

              query(pack(positions, rotations, scales, meshes, materials),
                [&](higanbana::Id id,
                    const components::Position& pos,
                    const components::Rotation& rot,
                    const components::Scale& scale,
                    const components::MeshInstance& mesh,
                    const components::MaterialInstance& mat){
                auto gpuMesh = ecs.get<components::GpuMeshInstance>().get(mesh.id);
                auto gpuMat = ecs.get<components::GpuMaterialInstance>().get(mat.id);
                
                float4x4 rotM = math::rotationMatrixLH(rot.rot);
                float4x4 posM = math::translation(pos.pos);
                float4x4 scaleM = math::scale(scale.value);
                float4x4 worldM = math::mul(math::mul(scaleM, rotM), posM);
                InstanceDraw draw;
                draw.mat = worldM;
                draw.materialId = gpuMat.id;
                draw.meshId = gpuMesh.id;
                allMeshesToDraw.push_back(draw);
              });
            }

            auto& t_pos = ecs.get<components::Position>();
            auto& t_rot = ecs.get<components::Rotation>();
            auto& t_cameraSet = ecs.get<components::CameraSettings>();

            ActiveCamera ac{};

            query(pack(t_pos, t_rot, t_cameraSet), pack(ecs.getTag<components::ActiveCamera>()),
            [&](higanbana::Id id, components::Position& pos, components::Rotation& rot, components::CameraSettings& set)
            {
              ac.position = pos.pos;
              ac.direction = rot.rot;
              ac.fov = set.fov;
              ac.minZ = set.minZ;
              ac.maxZ = set.maxZ;
            });
            HIGAN_CPU_BRACKET("Render");
            rend.ensureViewportCount(viewportCount);
            vector<app::RenderViewportInfo> activeViewports;
            for (auto&& vp : rendererViewports) {
              vp.camera = ac;
              if (vp.options.visible) {
                activeViewports.push_back(vp);
              }
            }

            auto viewports = MemView(activeViewports.data(), activeViewports.size());
            if (!renderOptions.renderImGui) {
              viewports = MemView(rendererViewports.data(), 1);
            }
            rend.renderViewports(lbs, time, renderOptions, viewports, allMeshesToDraw, cubeCount, cubeCommandLists);

            logicAndRenderTime.tick();
            auto totalTime = logicAndRenderTime.getFrameTimeDelta();
            if (limitFPS > 6 && totalTime < (1.f / float(limitFPS)))
            {
              auto targetTime = (1000000.f / float(limitFPS));
              auto ourTime = time.getFrameTimeDelta()*1000000.f;
              auto howFarAreWe = std::max(std::min(0.f, targetTime - ourTime), targetTime * -2.f);
              auto overTime = (1000000.f / float(limitFPS)) - totalTime*1000000.f;
              auto waitTime = std::min(1000000ll, static_cast<int64_t>(overTime+howFarAreWe*1.8f));
              std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
              //HIGAN_LOGi("delta to fps limit %.4f %.4f %.4f %.4f %.4f\n", howFarAreWe, totalTime*1000000.f, overTime, targetTime, ourTime);
            }
          }
        });
        while (!window.simpleReadMessages(frame++))
        {
          HIGAN_CPU_BRACKET("update Inputs");
          log.update();
          fs.updateWatchedFiles();
          if (window.hasResized())
          {
            renderResize = true;
            window.resizeHandled();
          }
          inputs.pollDevices(gamepad::Controllers::PollOptions::OnlyPoll);
          agamepad.write(inputs.getFirstActiveGamepad());
          auto kinputs = window.inputs();
          ainputs.write(kinputs);
          amouse.write(window.mouse());
          inputsUpdated = kinputs.frame();
          auto sizeInfo = window.posAndSize();
          windowSize = int2(sizeInfo.z, sizeInfo.w);
          windowPos = sizeInfo.xy();
          if (!renderActive)
            break;
          std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        renderActive = false;
        lbs.sleepTillKeywords({"logic&render loop"});
        dev.waitGpuIdle();
      }
      if (!reInit)
        break;
      else
      {
        reInit = false;
      }
    }
    quit = true;
    size_t amountToWrite = 0;
    auto data = ImGui::SaveIniSettingsToMemory(&amountToWrite);
    vector<uint8_t> out;
    out.resize(amountToWrite);
    memcpy(out.data(), data, amountToWrite);
    fs.writeFile("/imgui.config", makeMemView(out));
    higanbana::profiling::writeProfilingData(fs);
  };
#if 0
  main(GraphicsApi::DX12, VendorID::Amd, true);
#else
  main(cmdlineApiId, cmdlineVendorId, true);
  //lbs.addTask("test1", [&](size_t) {main(GraphicsApi::Vulkan, VendorID::Nvidia, true); });
  //lbs.sleepTillKeywords({ "test1" });

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
  //std::this_thread::sleep_for(std::chrono::seconds(5));
}
