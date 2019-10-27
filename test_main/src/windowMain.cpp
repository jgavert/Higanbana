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
#include "world/visual_data_structures.hpp"

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

  query(pack(ecs.get<components::WorldPosition>(), ecs.get<components::Rotation>()), pack(ecs.getTag<components::ActiveCamera>()),
  [&](higanbana::Id, components::WorldPosition& pos, components::Rotation& rot)
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
        multiplier = 100.f;
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
    Database<2048> ecs;

    app::World world;
    world.loadGLTFScene(ecs, fs, "/scenes");
    app::EntityView entityViewer;
    bool renderECS = false;
    int cubeCount = 27;

    {
      auto& t_pos = ecs.get<components::WorldPosition>();
      auto& t_rot = ecs.get<components::Rotation>();
      auto& t_cameraSet = ecs.get<components::CameraSettings>();

      auto cid = ecs.createEntity();
      t_pos.insert(cid, {float3(3, 0 , 8)});
      t_rot.insert(cid, {quaternion{ 1.f, 0.f, 0.f, 0.f }});
      t_cameraSet.insert(cid, {90.f, 0.01f, 100.f});
      ecs.getTag<components::ActiveCamera>().insert(cid);
    }

    bool quit = false;
    LBS lbs;

    log.update();

    bool explicitID = false;
    //GpuInfo gpuinfo{};

    WTime time;

    while (true)
    {
      vector<GpuInfo> allGpus;
      GraphicsSubsystem graphics(allowedApis, "higanbana", validationLayer);
      vector<GpuInfo> gpus;
      if (rgpCapture)
      {
        gpus = graphics.availableGpus(api, VendorID::Amd);
        if (!gpus.empty())
          allGpus.push_back(gpus[0]);
        else
          HIGAN_LOGi("Failed to find AMD RGP compliant device in %s api", toString(api));
      }
      else
      {
        gpus = graphics.availableGpus();
        
        //HIGAN_LOG("\t%s: %d. %s (memory: %zdMB, api: %s)\n", toString(it.api), it.id, it.name.c_str(), it.memory/1024/1024, it.apiVersionStr.c_str());
        auto gpuInfo = graphics.getVendorDevice(api, preferredVendor);
        auto api2 = GraphicsApi::Vulkan;
        if (api == GraphicsApi::Vulkan)
        {
          api2 = GraphicsApi::DX12;
        }
        auto gpuInfo2 = graphics.getVendorDevice(api2, preferredVendor);
        allGpus.emplace_back(gpuInfo);
      }
      //allGpus.emplace_back(gpuInfo2);
      if (updateLog) log.update();
      if (gpus.empty())
        return;

	    std::string windowTitle = "";
      for (auto& gpu : allGpus)
      {
        windowTitle += std::string(toString(gpu.api)) + ": " + gpu.name + " ";
      }
      Window window(params, windowTitle, 1280, 720, 300, 200);
      window.open();

      // IMGUI
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr;
      }
      ImGui::StyleColorsDark();

      auto dev = graphics.createGroup(fs, allGpus);
      app::Renderer rend(graphics, dev);

      // Load meshes to gpu
      {
        auto& gpuMesh = ecs.get<components::MeshInstance>();
        query(pack(ecs.get<components::RawMeshData>()), // pack(ecs.getTag<components::MeshNode>()),
          [&](higanbana::Id id, components::RawMeshData index) {
          components::MeshInstance instance{rend.loadMesh(world.getMesh(index.id))};
          gpuMesh.insert(id, instance);
        });
      }
      {
        auto toggleHDR = false;
        rend.initWindow(window, allGpus[0]);

        for (auto& gpu : allGpus)
        {
          HIGAN_LOG("Created device \"%s\"\n", gpu.name.c_str());
        }

        bool closeAnyway = false;
        bool renderActive = true;
        bool renderResize = false;
        bool captureMouse = false;
        bool controllerConnected = false;

        AtomicBuffered<InputBuffer> ainputs;
        ainputs.write(window.inputs());
        AtomicBuffered<MouseState> amouse;
        amouse.write(window.mouse());
        std::atomic<int> inputsUpdated = 0;
        int inputsRead = 0;

        lbs.addTask("logic&render loop", [&](size_t) {
          while (renderActive)
          {
            ImGuiIO &io = ::ImGui::GetIO();
            auto windowSize = rend.windowSize();
            io.DisplaySize = { float(windowSize.x), float(windowSize.y) };
            time.tick();
            io.DeltaTime = time.getFrameTimeDelta();
            if (io.DeltaTime < 0.f)
              io.DeltaTime = 0.00001f;
            
            auto lastRead = inputsRead;
            auto inputs = ainputs.read();
            auto currentInput = inputs.frame();
            auto diffSinceLastInput = currentInput - lastRead;
            auto diffWithWriter = inputsUpdated.load() - currentInput;
            if (diffSinceLastInput > 0)
            {
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

              gamepad::X360LikePad pad;
              pad.alive = false;
              handleInputs(ecs, pad, mouse, inputs, time.getFrameTimeDelta(), captureMouse);
            }
            ::ImGui::NewFrame();
            ImGui::SetNextWindowSize(ImVec2(360, 580), ImGuiCond_Once);
            ImGui::Begin("main");
            ImGui::Text("%zd frames since last inputs. %zd diff to Writer, current: %zd read %d", diffSinceLastInput, diffWithWriter, currentInput, lastRead);

            ImGui::Text("average FPS %.2f (%.2fms)", 1000.f / time.getCurrentFps(), time.getCurrentFps());
            ImGui::Text("max FPS %.2f (%.2fms)", 1000.f / time.getMaxFps(), time.getMaxFps());

            ImGui::Text("Validation Layer %s", validationLayer ? "Enabled" : "Disabled");
            if (ImGui::CollapsingHeader("Keys"))
            {
              ImGui::Text(" Exit:                         ESC");
              ImGui::Text(" Capture Mouse:                F1");
              ImGui::Text(" Release Mouse:                F2");
              ImGui::Text(" Toggle Borderless Fullscreen: Alt + 1");
              ImGui::Text(" Toggle GFX API Vulkan/DX12:   Alt + 2");
              ImGui::Text(" Toggle GPU Vendor Nvidia/AMD: Alt + 3");
            }

            auto si = rend.timings();
            if (si && ImGui::CollapsingHeader("Renderpass"))
            {
              ImGui::Text("RenderGraph statistics:");
              auto rsi = si.value();
              float gpuTotal = 0.f;
              for (auto& list : rsi.lists)
              {
                gpuTotal += list.gpuTime.milliseconds();
              }
              float cpuTotal = rsi.timeBeforeSubmit.milliseconds() + rsi.submitCpuTime.milliseconds();
              if (gpuTotal > cpuTotal)
                ImGui::Text("Bottleneck: GPU");
              else
                ImGui::Text("Bottleneck: CPU");
              ImGui::Text("GPU execution %.3fms", gpuTotal);
              ImGui::Text("CPU execution %.3fms", cpuTotal);
              ImGui::Text("- user fill time %.2fms", rsi.timeBeforeSubmit.milliseconds());
              ImGui::Text("- combine nodes %.3fms", rsi.addNodes.milliseconds());
              ImGui::Text("- Graph solving %.3fms", rsi.graphSolve.milliseconds());
              ImGui::Text("- Filling Lists %.3fms", rsi.fillCommandLists.milliseconds());
              ImGui::Text("- Submitting Lists %.3fms", rsi.submitSolve.milliseconds());
              ImGui::Text("- Submit total %.2fms", rsi.submitCpuTime.milliseconds());

              for (auto& cmdlist : rsi.lists)
              {
                ImGui::Text("\n%s Commandlist", toString(cmdlist.type));
                ImGui::Text("\tGPU time %.3fms", cmdlist.gpuTime.milliseconds());
                ImGui::Text("\t- totalTimeOnGPU %.3fms", cmdlist.fromSubmitToFence.milliseconds());
                ImGui::Text("\t- barrierSolve %.3fms", cmdlist.barrierSolve.milliseconds());
                ImGui::Text("\t- fillNativeList %.3fms", cmdlist.fillNativeList.milliseconds());
                ImGui::Text("\t- cpuBackendTime(?) %.3fms", cmdlist.cpuBackendTime.milliseconds());
                ImGui::Text("\tGPU nodes:");
                for (auto& graphNode : cmdlist.nodes)
                {
                  ImGui::Text("\t\t%s %.3fms (cpu %.3fms)", graphNode.nodeName.c_str(), graphNode.gpuTime.milliseconds(), graphNode.cpuTime.milliseconds());
                }
              }
            }
            if (ImGui::CollapsingHeader("Gpu statistics"))
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
            {
              auto& t_pos = ecs.get<components::WorldPosition>();
              auto& t_rot = ecs.get<components::Rotation>();
              auto& t_cameraSet = ecs.get<components::CameraSettings>();
              query(pack(t_pos, t_rot, t_cameraSet), pack(ecs.getTag<components::ActiveCamera>()),
              [&](higanbana::Id id, components::WorldPosition& pos, components::Rotation& rot, components::CameraSettings& set)
              {
                ImGui::Text("Camera");
                ImGui::DragFloat3("position", pos.pos.data);
                float3 dir = math::normalize(rotateVector({ 0.f, 0.f, -1.f }, rot.rot));
                float3 updir = math::normalize(rotateVector({ 0.f, 1.f, 0.f }, rot.rot));
                float3 sideVec = math::normalize(rotateVector({ 1.f, 0.f, 0.f }, rot.rot));
                float3 xyz{};
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
            {
              ImGui::Checkbox("render ECS", &renderECS);
              ImGui::Text("%d cubes/draw calls", cubeCount*cubeCount*cubeCount);
              ImGui::SameLine();
              ImGui::DragInt("cube multiple", &cubeCount, 1, 0, 64);
            }
            ImGui::End();
            // render entities
            entityViewer.render(ecs);

            ImGui::Render();

            // collect all model instances and their matrices for drawing...
            vector<InstanceDraw> allMeshesToDraw;
            if (renderECS)
            {

              struct ActiveScene
              {
                components::SceneInstance target;
                float3 wp;
              };
              higanbana::vector<ActiveScene> scenes;
              query(pack(ecs.get<components::SceneInstance>(), ecs.get<components::WorldPosition>()), [&](higanbana::Id id, components::SceneInstance scene, components::WorldPosition wp)
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
                    auto meshTarget = ecs.get<components::MeshInstance>().tryGet(childMesh);

                    if (meshTarget)
                      allMeshesToDraw.push_back({{}, {}, meshTarget.value().id});
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

            auto& t_pos = ecs.get<components::WorldPosition>();
            auto& t_rot = ecs.get<components::Rotation>();
            auto& t_cameraSet = ecs.get<components::CameraSettings>();

            ActiveCamera ac{};

            query(pack(t_pos, t_rot, t_cameraSet), pack(ecs.getTag<components::ActiveCamera>()),
            [&](higanbana::Id id, components::WorldPosition& pos, components::Rotation& rot, components::CameraSettings& set)
            {
              ac.position = pos.pos;
              ac.direction = rot.rot;
              ac.fov = set.fov;
              ac.minZ = set.minZ;
              ac.maxZ = set.maxZ;
            });
            rend.render(ac, allMeshesToDraw, cubeCount);
          }
        });
        while (!window.simpleReadMessages(frame++))
        {
          log.update();
          fs.updateWatchedFiles();
          if (window.hasResized())
          {
            renderResize = true;
            window.resizeHandled();
          }

          auto inputs = window.inputs();
          ainputs.write(inputs);
          amouse.write(window.mouse());
          inputsUpdated = inputs.frame();
          if (!renderActive)
            break;
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
