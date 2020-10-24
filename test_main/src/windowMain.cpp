#include "windowMain.hpp"

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
    if (input.alive && !captureMouse)
    {
      auto floatizeAxises = [](int16_t input, float& output)
      {
        constexpr int16_t deadzone = 9000;
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
        constexpr int16_t deadzone = 5000;
        if (std::abs(input) > deadzone)
        {
          constexpr float max = static_cast<float>(std::numeric_limits<uint16_t>::max() - deadzone);
          output = static_cast<float>(std::abs(input) - deadzone) / max;
        }
      };
      float l{}, r{};

      floatizeTriggers(input.lTrigger.value, l);
      floatizeTriggers(input.rTrigger.value, r);

      rxyz.z = l - r;

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
      if (inputs.isPressedThisFrame('W', 2)) xy.y = -1.f * multiplier;
      if (inputs.isPressedThisFrame('S', 2)) xy.y = 1.f * multiplier;
      if (inputs.isPressedThisFrame('D', 2)) xy.x = -1.f * multiplier;
      if (inputs.isPressedThisFrame('A', 2)) xy.x = 1.f * multiplier;
      xy = math::mul(xy, std::max(delta, 0.001f));
    }

    // use our current up and forward vectors and calculate our new up and forward vectors
    // yaw, pitch, roll

    direction = math::normalize(direction);

    position = math::add(position, math::mul(sideVec, xy.x));
    position = math::add(position, math::mul(dir, xy.y));
  });
}
css::Task<int> RenderingApp::runVisualLoop(app::Renderer& rend, higanbana::GpuGroup& dev) {
  //HIGAN_CPU_BRACKET("Logic&Render");
  co_await css::async([&]{
    {
      HIGAN_CPU_BRACKET("Load meshes to gpu");
      auto& gpuBuffer = m_ecs.get<components::GpuBufferInstance>();
      query(pack(m_ecs.get<components::RawBufferData>()), // pack(ecs.getTag<components::MeshNode>()),
        [&](higanbana::Id id, components::RawBufferData index) {
        components::GpuBufferInstance instance{rend.loadBuffer(m_world.getBuffer(index.id))};
        gpuBuffer.insert(id, instance);
      });
    }
    {
      HIGAN_CPU_BRACKET("link meshes to gpu meshes");
      auto& gpuMesh = m_ecs.get<components::GpuMeshInstance>();
      auto& gpuBuffer = m_ecs.get<components::GpuBufferInstance>();
      query(pack(m_ecs.get<components::RawMeshData>()), // pack(ecs.getTag<components::MeshNode>()),
        [&](higanbana::Id id, components::RawMeshData index) {
        auto mesh = m_world.getMesh(index.id);
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
      auto& gpuTexture = m_ecs.get<components::GpuTextureInstance>();
      query(pack(m_ecs.get<components::RawTextureData>()), // pack(ecs.getTag<components::MeshNode>()),
        [&](higanbana::Id id, components::RawTextureData index) {
        components::GpuTextureInstance instance{rend.loadTexture(m_world.getTexture(index.id))};
        gpuTexture.insert(id, instance);
      });
    }
    // link initial textures to materials
    {
      HIGAN_CPU_BRACKET("link textures to existing materials");
      auto& textureInstance = m_ecs.get<components::GpuTextureInstance>();
      query(pack(m_ecs.get<MaterialData>(), m_ecs.get<components::MaterialLink>()), // pack(ecs.getTag<components::MeshNode>()),
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
      auto& gpuMaterial = m_ecs.get<components::GpuMaterialInstance>();
      query(pack(m_ecs.get<MaterialData>()), // pack(ecs.getTag<components::MeshNode>()),
      [&](higanbana::Id id, MaterialData material) {
        int gpuid = rend.loadMaterial(material);
        components::GpuMaterialInstance instance{gpuid};
        gpuMaterial.insert(id, instance);
      });
    }
  });
  float timeSinceLastInput = 0;
  vector<InstanceDraw> allMeshesToDraw;
  vector<app::RenderViewportInfo> rendererViewports;
  rendererViewports.push_back({});
  while(m_renderActive) {
    HIGAN_CPU_BRACKET("render thread iteration");
    ImGuiIO& io = ::ImGui::GetIO();
    auto windowSize = rend.windowSize();
    io.DisplaySize = {float(windowSize.x), float(windowSize.y)};
    if (m_advanceSimulation || m_stepOneFrameForward) {
      m_time.tick();
      m_stepOneFrameForward = false;
    }
    m_logicAndRenderTime.startFrame();
    io.DeltaTime = m_time.getFrameTimeDelta();
    if (io.DeltaTime < 0.f) io.DeltaTime = 0.00001f;
    auto                 lastRead = m_inputsRead;
    auto                 inputs = m_ainputs.read();
    gamepad::X360LikePad pad = m_agamepad.read();
    auto                 currentInput = inputs.frame();
    auto                 diffSinceLastInput = currentInput - lastRead;
    auto                 diffWithWriter = m_inputsUpdated.load() - currentInput;
    if (diffSinceLastInput > 0) {
      HIGAN_CPU_BRACKET("reacting to new input");
      m_inputsRead = currentInput;
      auto mouse = m_amouse.read();

      if (mouse.captured) {
        io.MousePos.x = static_cast<float>(mouse.m_pos.x);
        io.MousePos.y = static_cast<float>(mouse.m_pos.y);

        io.MouseWheel = static_cast<float>(mouse.mouseWheel) * 0.1f;
      }

      inputs.goThroughNFrames(diffSinceLastInput, [&](Input i) {
        if (i.key >= 0) {
          switch (i.key) {
            case VK_LBUTTON: {
              io.MouseDown[0] = (i.action > 0);
              break;
            }
            case VK_RBUTTON: {
              io.MouseDown[1] = (i.action > 0);
              break;
            }
            case VK_MBUTTON: {
              io.MouseDown[2] = (i.action > 0);
              break;
            }
            default:
              if (i.key < 512) io.KeysDown[i.key] = (i.action > 0);
              break;
          }
        }
      });

      for (auto ch : inputs.charInputs()) io.AddInputCharacter(static_cast<ImWchar>(ch));

      io.KeyCtrl = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_CONTROL, 2);
      io.KeyShift = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_SHIFT, 2);
      io.KeyAlt = inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2);
      io.KeySuper = false;

      if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_F1, 1)) {
        m_toggleCaptureMouse = true;
      }
      if (inputs.isPressedWithinNFrames(diffSinceLastInput, 220, 1))  // tilde... or something like that
      {
        m_renderOptions.renderImGui = !m_renderOptions.renderImGui;
      }

      if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '1', 1)) {
        m_toggleBorderlessFullscreen = true;
      }

      if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '2', 1)) {
        m_reInit = true;
        if (m_api == GraphicsApi::DX12)
          m_api = GraphicsApi::Vulkan;
        else
          m_api = GraphicsApi::DX12;
        m_chosenDeviceID = 0;
        m_renderActive = false;
      }

      if (inputs.isPressedWithinNFrames(diffSinceLastInput, VK_MENU, 2) && inputs.isPressedWithinNFrames(diffSinceLastInput, '3', 1)) {
        m_reInit = true;
        if (m_preferredVendor == VendorID::Amd)
          m_preferredVendor = VendorID::Nvidia;
        else
          m_preferredVendor = VendorID::Amd;
        m_renderActive = false;
      }

      if (m_frame > 10 && (m_closeAnyway || inputs.isPressedWithinNFrames(diffSinceLastInput, VK_ESCAPE, 1))) {
        m_renderActive = false;
      }
      auto timestep = timeSinceLastInput + m_time.getFrameTimeDelta();
      handleInputs(m_ecs, pad, mouse, inputs, timestep, m_captureMouse);
      timeSinceLastInput = 0.f;
    } else {
      timeSinceLastInput += m_time.getFrameTimeDelta();
    }

    {
      HIGAN_CPU_BRACKET("ImGui create menus");
      ::ImGui::NewFrame();

      if (m_renderOptions.renderImGui) {
        auto gid = ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Keys")) {
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
          ImVec2      windowOffset;
          ImVec2      windowSize;
          if (ImGui::Begin(viewportName.c_str())) {
            auto ws = ImGui::GetWindowSize();
            windowSize = ws;
            windowOffset = ImGui::GetWindowPos();
            ws.y -= 36;
            rendererViewports[i].options.visible = true;
            ImGui::Image(reinterpret_cast<void*>(enabledViewport + 1), ws);
            rendererViewports[i].viewportSize = int2(ws.x, ws.y);
            rendererViewports[i].viewportIndex = enabledViewport + 1;
            enabledViewport++;
          }
          ImGui::End();
          const float DISTANCE = 20.0f;
          static int  corner = 0;
          ImGuiIO&    io = ImGui::GetIO();
          if (rendererViewports[i].options.visible) {
            ImVec2 window_pos = ImVec2((corner & 1) ? (windowOffset.x + windowSize.x - DISTANCE) : (windowOffset.x + DISTANCE),
                                        (corner & 2) ? (windowOffset.y + windowSize.y - DISTANCE) : (windowOffset.y + DISTANCE));
            ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            ImGui::SetNextWindowBgAlpha(0.35f);  // Transparent background
          }
          viewportName += "##overlay";
          if (rendererViewports[i].options.visible &&
              ImGui::Begin(viewportName.c_str(),
                            nullptr,
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                ImGuiWindowFlags_NoNav)) {
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
          if (rendererViewports[i].options.visible) ImGui::End();
        }
        ImGui::SetNextWindowSize(ImVec2(360, 580), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("main")) {
          if (ImGui::ListBoxHeader("device selection", ImVec2(0, allGpus.size() * 18))) {
            int i = 0;
            for (auto&& device : allGpus) {
              if (ImGui::Selectable(device.name.c_str(), selectedDevice[i])) selectedDevice[i] = !selectedDevice[i];
              i++;
            }
            ImGui::ListBoxFooter();
          }
          ImGui::Checkbox("override gpu constants", &m_overrideGpuConstants);
          if (m_overrideGpuConstants)
            ImGui::Checkbox("gpu constants", &m_GpuConstants);
          ImGui::Checkbox("interopt", &m_interoptDevice);
          ImGui::SameLine();
          if (ImGui::Button("Reinit")) {
            for (int i = 0; i < selectedDevice.size(); i++) {
              if (selectedDevice[i]) {
                m_chosenDeviceID = allGpus[i].deviceId;
                m_reInit = true;
                m_renderActive = false;
                break;
              }
            }
          }
          ImGui::Text("%zd frames since last inputs. %zd diff to Writer, current: %zd read %d",
                      diffSinceLastInput,
                      diffWithWriter,
                      currentInput,
                      lastRead);

          ImGui::Text("average FPS %.2f (%.2fms)", 1000.f / m_time.getCurrentFps(), m_time.getCurrentFps());
          ImGui::Text("max FPS %.2f (%.2fms)", 1000.f / m_time.getMaxFps(), m_time.getMaxFps());
          ImGui::DragInt("FPS limit", &m_limitFPS, 1, -1, 144);
          ImGui::Checkbox("Readback shader prints", &higanbana::globalconfig::graphics::GraphicsEnableShaderDebug);
          bool rotateCam = m_autoRotateCamera;
          ImGui::Checkbox("rotate camera", &rotateCam);
          m_autoRotateCamera = rotateCam;
          ImGui::Checkbox("simulate", &m_advanceSimulation);
          if (!m_advanceSimulation) {
            ImGui::Checkbox("   Step Frame Forward", &m_stepOneFrameForward);
            ImGui::Checkbox("   Step Frame Backward", &m_stepOneFrameBackward);
            if (m_stepOneFrameForward) {
              m_time.startFrame();
            }
          }
          ImGui::Text("Validation Layer %s", m_cmdline.validationLayer ? "Enabled" : "Disabled");

          {
            ImGui::Checkbox("render selected glTF scene", &m_renderGltfScene);
            ImGui::Checkbox("render current scene", &m_renderECS);
            int activeViewports = 0;
            for (int i = 0; i < static_cast<int>(rendererViewports.size()); i++)
              if (rendererViewports[i].options.visible) activeViewports++;
            ImGui::Text("%d cubes * %d viewports", m_cubeCount, activeViewports);
            ImGui::Text("= %d draw calls / frame", m_cubeCount * activeViewports);
            ImGui::Text("Preallocated constant memory of 380mb or so");
            ImGui::Text("Exceeding >350k of drawcalls / frame crashes.");
            ImGui::Text("Thank you.");
            ImGui::DragInt("drawcalls/cubes", &m_cubeCount, 10, 0, 500000);
            ImGui::DragInt("drawcalls split into", &m_cubeCommandLists, 1, 1, 128);
          }
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Renderer options")) {
          ImGui::Text("Active devices:");
          for (int i = 0; i < activeDevices.size(); ++i) {
            const auto& info = activeDevices[i];
            std::string str = std::to_string(i) + std::string(": ") + info.name + " (" + std::string(toString(info.api)) + ")";
            ImGui::Text(str.c_str());
          }
          m_renderOptions.drawImGuiOptions();
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Renderer options for viewports")) {
          ImGui::SliderInt("viewport count", &m_viewportCount, 1, 16);
          rendererViewports.resize(m_viewportCount, app::RenderViewportInfo{1, int2(16, 16), {}, {}});
          ImGui::BeginTabBar("viewports");
          for (int i = 0; i < static_cast<int>(rendererViewports.size()); i++) {
            std::string index = std::string("viewport ") + std::to_string(i);
            auto&       vp = rendererViewports[i];
            if (ImGui::BeginTabItem(index.c_str())) {
              int2 currentRes = math::mul(vp.options.resolutionScale, float2(vp.viewportSize));
              ImGui::Text("resolution %dx%d", currentRes.x, currentRes.y);  // ImGui::SameLine();
              vp.options.drawImGuiOptions(activeDevices);
              ImGui::EndTabItem();
            }
          }
          ImGui::EndTabBar();
        }
        ImGui::End();

        auto si = rend.timings();

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Renderpass") && si) {
          ImGui::Text("RenderGraph statistics:");
          auto  rsi = si.value();
          float gpuTotal = 0.f;
          for (auto& list : rsi.lists) {
            gpuTotal += list.gpuTime.milliseconds();
          }

          uint64_t draws = 0, dispatches = 0;
          uint64_t bytesInCommandBuffersTotal = 0;
          for (auto&& cmdlist : rsi.lists) {
            for (auto&& node : cmdlist.nodes) {
              draws += node.draws;
              dispatches += node.dispatches;
              bytesInCommandBuffersTotal += node.cpuSizeBytes;
            }
          }

          float cpuTotal = rsi.timeBeforeSubmit.milliseconds() + rsi.submitCpuTime.milliseconds();
          if (gpuTotal > cpuTotal || rend.acquireTimeTakenMs() > 0.1f)
            ImGui::Text("Bottleneck: GPU");
          else
            ImGui::Text("Bottleneck: CPU");
          auto multiplier = 1000.f / m_time.getCurrentFps();
          auto allDraws = float(draws) * multiplier;
          int  dps = static_cast<int>(allDraws);
          ImGui::Text("Drawcalls per second %zu", dps);
          ImGui::Text("GPU execution %.3fms", gpuTotal);
          ImGui::Text("CPU execution %.3fms", cpuTotal);
          ImGui::Text("Acquire time taken %.3fms", rend.acquireTimeTakenMs());
          ImGui::Text("GraphNode size %.3fMB", bytesInCommandBuffersTotal / 1024.f / 1024.f);
          auto userFillTime = rsi.timeBeforeSubmit.milliseconds() - rend.acquireTimeTakenMs();  // we know that these can overlap.
          ImGui::Text("- user fill time %.2fms", userFillTime);
          auto bandwidth = float(bytesInCommandBuffersTotal / 1024.f / 1024.f) / userFillTime * 1000.f;
          ImGui::Text("- CPU Bandwidth in filling %.3fGB/s", bandwidth / 1024.f);
          ImGui::Text("- combine nodes %.3fms", rsi.addNodes.milliseconds());
          ImGui::Text("- Graph solving %.3fms", rsi.graphSolve.milliseconds());
          ImGui::Text("- Filling Lists %.3fms", rsi.fillCommandLists.milliseconds());
          ImGui::Text("- Submitting Lists %.3fms", rsi.submitSolve.milliseconds());
          ImGui::Text("- Submit total %.2fms", rsi.submitCpuTime.milliseconds());

          int listIndex = 0;
          for (auto& cmdlist : rsi.lists) {
            std::string listName = toString(cmdlist.type);
            listName += " list " + std::to_string(listIndex++);
            if (ImGui::TreeNode(listName.c_str())) {
              // ImGui::Text("\n%s Commandlist", toString(cmdlist.type));
              ImGui::Text("\tGPU time %.3fms", cmdlist.gpuTime.milliseconds());
              ImGui::Text("\t- totalTimeOnGPU %.3fms", cmdlist.fromSubmitToFence.milliseconds());
              ImGui::Text("\t- barrierPrepare %.3fms", cmdlist.barrierAdd.milliseconds());
              ImGui::Text("\t- barrierSolveLocal %.3fms", cmdlist.barrierSolveLocal.milliseconds());
              ImGui::Text("\t- barrierSolveGlobal %.3fms", cmdlist.barrierSolveGlobal.milliseconds());
              ImGui::Text("\t- fillNativeList %.3fms", cmdlist.fillNativeList.milliseconds());
              ImGui::Text("\t- cpuBackendTime(?) %.3fms", cmdlist.cpuBackendTime.milliseconds());
              ImGui::Text("\tGPU nodes:");
              for (auto& graphNode : cmdlist.nodes) {
                ImGui::Text("\t\t%s %.3fms (cpu %.3fms)",
                            graphNode.nodeName.c_str(),
                            graphNode.gpuTime.milliseconds(),
                            graphNode.cpuTime.milliseconds());
              }
              ImGui::TreePop();
            }
          }
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Gpu statistics")) {
          auto stats = dev.gpuStatistics();
          auto bytesToMb = [](uint64_t bytes) { return bytes / 1024.f / 1024.f; };
          auto unitsToK = [](uint64_t units) { return units / 1000.f; };
          for (auto& stat : stats) {
            ImGui::Text("Commandbuffers on gpu       %zu", stat.commandlistsOnGpu);
            ImGui::Text("GPU Memory allocated        %.2fmb / %.2fmb  %.2f%%",
                        bytesToMb(stat.gpuMemoryAllocated),
                        bytesToMb(stat.gpuTotalMemory),
                        bytesToMb(stat.gpuMemoryAllocated) / bytesToMb(stat.gpuTotalMemory) * 100.f);
            ImGui::Text("Memory used by constants    %.2fmb / %.2fmb  %.2f%%",
                        bytesToMb(stat.constantsUploadMemoryInUse),
                        bytesToMb(stat.maxConstantsUploadMemory),
                        bytesToMb(stat.constantsUploadMemoryInUse) / bytesToMb(stat.maxConstantsUploadMemory) * 100.f);
            ImGui::Text("Generic upload memory used  %.2fmb / %.2fmb  %.2f%%",
                        bytesToMb(stat.genericUploadMemoryInUse),
                        bytesToMb(stat.maxGenericUploadMemory),
                        bytesToMb(stat.genericUploadMemoryInUse) / bytesToMb(stat.maxGenericUploadMemory) * 100.f);
            if (stat.descriptorsInShaderArguments)
              ImGui::Text("ShaderArguments             %zu / %zu  %.2f%%",
                          stat.descriptorsAllocated,
                          stat.maxDescriptors,
                          float(stat.descriptorsAllocated) / float(stat.maxDescriptors) * 100.f);
            else
              ImGui::Text("Descriptors                 %.2fk / %.2fk %.2f%%",
                          unitsToK(stat.descriptorsAllocated),
                          unitsToK(stat.maxDescriptors),
                          unitsToK(stat.descriptorsAllocated) / unitsToK(stat.maxDescriptors) * 100.f);
          }
        }
        ImGui::End();
        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Camera")) {
          auto& t_pos = m_ecs.get<components::Position>();
          auto& t_rot = m_ecs.get<components::Rotation>();
          auto& t_cameraSet = m_ecs.get<components::CameraSettings>();
          query(pack(t_pos, t_rot, t_cameraSet),
                pack(m_ecs.getTag<components::ActiveCamera>()),
                [&](higanbana::Id id, components::Position& pos, components::Rotation& rot, components::CameraSettings& set) {
                  float3 dir = math::normalize(rotateVector({0.f, 0.f, -1.f}, rot.rot));
                  float3 updir = math::normalize(rotateVector({0.f, 1.f, 0.f}, rot.rot));
                  float3 sideVec = math::normalize(rotateVector({1.f, 0.f, 0.f}, rot.rot));
                  float3 xyz{};
                  if (m_autoRotateCamera) {
                    float circleMultiplier = -1.2f;
                    pos.pos = math::add(pos.pos, math::mul(circleMultiplier * m_time.getFrameTimeDelta(), sideVec));
                    xyz.x = -0.23 * m_time.getFrameTimeDelta();
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
        m_entityViewer.render(m_ecs);

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        m_sceneEditor.render(m_ecs);

        ImGui::SetNextWindowDockID(gid, ImGuiCond_FirstUseEver);
        m_entityEditor.render(m_ecs);
      }
      ImGui::Render();
    }

    // collect all model instances and their matrices for drawing...
    allMeshesToDraw.clear();
    if (m_renderGltfScene) {
      HIGAN_CPU_BRACKET("collecting gltf mesh instances to render");

      struct ActiveScene {
        components::SceneInstance target;
        float3                    wp;
      };
      higanbana::vector<ActiveScene> scenes;
      query(pack(m_ecs.get<components::SceneInstance>(), m_ecs.get<components::Position>()),
            [&](higanbana::Id id, components::SceneInstance scene, components::Position wp) {
              scenes.push_back(ActiveScene{scene, wp.pos});
            });

      auto& children = m_ecs.get<components::Childs>();

      auto findMeshes = [&](higanbana::Id id) -> std::optional<components::Childs> {
        // check if has meshes ->
        if (auto hasMesh = m_ecs.get<components::Mesh>().tryGet(id)) {
          for (auto&& childMesh : children.get(hasMesh.value().target).childs) {
            auto meshTarget = m_ecs.get<components::GpuMeshInstance>().tryGet(childMesh);

            InstanceDraw draw{};
            if (meshTarget) {
              auto material = m_ecs.get<components::MaterialInstance>().tryGet(childMesh);
              if (material) {
                auto matID = m_ecs.get<components::GpuMaterialInstance>().get(material->id);
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
      if (auto c0 = children.tryGet(scenes.back().target.target)) {
        stack.push_back(c0.value().childs);
      }

      if (auto base = scenes.back().target.target)
        while (!stack.empty()) {
          if (stack.back().empty()) {
            stack.pop_back();
            continue;
          }
          auto& workingSet = stack.back();
          auto  val = workingSet.back();
          workingSet.pop_back();
          if (auto new_chlds = findMeshes(val)) {
            stack.push_back(new_chlds.value().childs);
          }
        }
    } else if (m_renderECS) {
      HIGAN_CPU_BRACKET("collecting entity instances to render");

      auto& positions = m_ecs.get<components::Position>();
      auto& rotations = m_ecs.get<components::Rotation>();
      auto& scales = m_ecs.get<components::Scale>();
      auto& meshes = m_ecs.get<components::MeshInstance>();
      auto& materials = m_ecs.get<components::MaterialInstance>();

      query(pack(positions, rotations, scales, meshes, materials),
            [&](higanbana::Id                       id,
                const components::Position&         pos,
                const components::Rotation&         rot,
                const components::Scale&            scale,
                const components::MeshInstance&     mesh,
                const components::MaterialInstance& mat) {
              auto gpuMesh = m_ecs.get<components::GpuMeshInstance>().get(mesh.id);
              auto gpuMat = m_ecs.get<components::GpuMaterialInstance>().get(mat.id);

              float4x4     rotM = math::rotationMatrixLH(rot.rot);
              float4x4     posM = math::translation(pos.pos);
              float4x4     scaleM = math::scale(scale.value);
              float4x4     worldM = math::mul(math::mul(scaleM, rotM), posM);
              InstanceDraw draw;
              draw.mat = worldM;
              draw.materialId = gpuMat.id;
              draw.meshId = gpuMesh.id;
              allMeshesToDraw.push_back(draw);
            });
    }

    auto& t_pos = m_ecs.get<components::Position>();
    auto& t_rot = m_ecs.get<components::Rotation>();
    auto& t_cameraSet = m_ecs.get<components::CameraSettings>();

    ActiveCamera ac{};

    query(pack(t_pos, t_rot, t_cameraSet),
          pack(m_ecs.getTag<components::ActiveCamera>()),
          [&](higanbana::Id id, components::Position& pos, components::Rotation& rot, components::CameraSettings& set) {
            ac.position = pos.pos;
            ac.direction = rot.rot;
            ac.fov = set.fov;
            ac.minZ = set.minZ;
            ac.maxZ = set.maxZ;
          });
    HIGAN_CPU_BRACKET("Render");
    rend.ensureViewportCount(m_viewportCount);
    vector<app::RenderViewportInfo> activeViewports;
    for (auto&& vp : rendererViewports) {
      vp.camera = ac;
      if (vp.options.visible) {
        activeViewports.push_back(vp);
      }
    }

    auto viewports = MemView(activeViewports.data(), activeViewports.size());
    if (!m_renderOptions.renderImGui) {
      viewports = MemView(rendererViewports.data(), 1);
    }
    co_await rend.renderViewports(m_lbs, m_time, m_renderOptions, viewports, allMeshesToDraw, m_cubeCount, m_cubeCommandLists);

    m_logicAndRenderTime.tick();
    auto totalTime = m_logicAndRenderTime.getFrameTimeDelta();
    if (m_limitFPS > 6 && totalTime < (1.f / float(m_limitFPS))) {
      auto targetTime = (1000000.f / float(m_limitFPS));
      auto ourTime = m_time.getFrameTimeDelta() * 1000000.f;
      auto howFarAreWe = std::max(std::min(0.f, targetTime - ourTime), targetTime * -2.f);
      auto overTime = (1000000.f / float(m_limitFPS)) - totalTime * 1000000.f;
      auto waitTime = std::min(1000000ll, static_cast<int64_t>(overTime + howFarAreWe * 1.8f));
      std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
      // HIGAN_LOGi("delta to fps limit %.4f %.4f %.4f %.4f %.4f\n", howFarAreWe, totalTime*1000000.f, overTime, targetTime, ourTime);
    }
  }
  co_return 0;
}

RenderingApp::RenderingApp(AppInputs inputs)
  : m_cmdline(inputs)
  , m_api(inputs.cmdlineApiId)
  , m_preferredVendor(inputs.cmdlineVendorId)
  , m_fs("/../../data")
{
  m_renderOptions.submitSingleThread = false;

}

void RenderingApp::runCoreLoop(ProgramParams& params) {
  HIGAN_LOG("Trying to start with %s api and %s vendor\n", toString(m_api), toString(m_preferredVendor));

  WTime time;
  WTime logicAndRenderTime;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // IMGUI
  auto loadWorld = [&]()->css::Task<void> { 
    HIGAN_CPU_BRACKET("load world");
    m_fs.initialLoad();
    {
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      io.IniFilename = nullptr;
      if (m_fs.fileExists("/imgui.config")) {
        auto file = m_fs.viewToFile("/imgui.config");
        ImGui::LoadIniSettingsFromMemory(reinterpret_cast<const char*>(file.data()), file.size_bytes());
      }
      io.ConfigFlags = ImGuiConfigFlags_DockingEnable;
    }
    ImGui::StyleColorsDark();
    //m_world.loadGLTFScene(m_ecs, m_fs, "/scenes");
    co_await m_world.loadGLTFSceneCgltfTasked(m_ecs, m_fs, "/scenes");
    {
      auto& name = m_ecs.get<components::Name>();
      auto& childs = m_ecs.get<components::Childs>();
      auto& sceneRoot = m_ecs.getTag<components::SceneEntityRoot>();
      auto e = m_ecs.createEntity();
      name.insert(e, components::Name{"Scene Root"});
      childs.insert(e, components::Childs{});
      sceneRoot.insert(e);
    }
    {
      auto& t_pos = m_ecs.get<components::Position>();
      auto& t_rot = m_ecs.get<components::Rotation>();
      auto& t_cameraSet = m_ecs.get<components::CameraSettings>();

      auto cid = m_ecs.createEntity();
      t_pos.insert(cid, {float3(0, 0 , 4)});
      t_rot.insert(cid, {quaternion{ 1.f, 0.f, 0.f, 0.f }});
      t_cameraSet.insert(cid, {90.f, 0.01f, 100.f});
      m_ecs.getTag<components::ActiveCamera>().insert(cid);
    }
    co_return;
  }();

  while (true)
  {
    HIGAN_CPU_BRACKET("main program");
    Window window(params, "test main", m_windowSize.x, m_windowSize.y, m_windowPos.x, m_windowPos.y);
    window.open();

    GraphicsSubsystem graphics(m_cmdline.allowedApis, "higanbana", m_cmdline.validationLayer);
    if (m_cmdline.rgpCapture)
    {
      allGpus = graphics.availableGpus(m_api, VendorID::Amd);
      if (!allGpus.empty()) {
        physicalDevice = allGpus[0];
      }
      else
        HIGAN_LOGi("Failed to find AMD RGP compliant device in %s api", toString(m_api));
    }
    else
    {
      allGpus = graphics.availableGpus();
      if (m_chosenDeviceID == 0) {
        auto expApi = m_api;
        if (m_interoptDevice)
          expApi = GraphicsApi::All;
        physicalDevice = graphics.getVendorDevice(expApi, m_preferredVendor);
        m_chosenDeviceID = physicalDevice.deviceId;
        if (!m_interoptDevice)
          m_chosenApi = physicalDevice.api;
      } 
      else
      {
        for (auto&& gpu : allGpus) {
          if (gpu.deviceId == m_chosenDeviceID) {
            physicalDevice = gpu;
            if (!m_interoptDevice)
              physicalDevice.api = m_chosenApi;
            break;
          }
        }
      }
    }
    m_log.update();
    if (allGpus.empty()) {
      HIGAN_LOGi("No gpu's found, exiting...\n");
      return;
    }

    higanbana::GpuGroup dev;
    if (m_overrideGpuConstants)
      physicalDevice.gpuConstants = m_GpuConstants;
    if (m_interoptDevice && physicalDevice.api == GraphicsApi::All)
      dev = graphics.createInteroptDevice(m_fs, physicalDevice);
    else
      dev = graphics.createDevice(m_fs, physicalDevice);
    
    activeDevices = dev.activeDevicesInfo();


    selectedDevice.resize(allGpus.size());
    for (int i = 0; i < allGpus.size(); ++i) {
      if (allGpus[i].deviceId == m_chosenDeviceID)
        selectedDevice[i] = true;
    }
    loadWorld.wait();
    app::Renderer rend(graphics, dev);
    rend.initWindow(window, physicalDevice);
    rend.loadLogos(m_fs);

    // Load meshes to gpu
    //loadWorld.wait();

    {
      auto toggleHDR = false;

      HIGAN_LOGi("Created device \"%s\"\n", physicalDevice.name.c_str());


      m_ainputs.write(window.inputs());
      m_amouse.write(window.mouse());

      m_renderActive = true;
      css::Task<int> logicAndRenderAsync = runVisualLoop(rend, dev);
      css::waitOwnQueueStolen();
      //std::this_thread::sleep_for(std::chrono::milliseconds(200));
      while (!window.simpleReadMessages(m_frame++))
      {
        HIGAN_CPU_BRACKET("update Inputs");
        m_log.update();
        m_fs.updateWatchedFiles();
        if (window.hasResized())
        {
          m_renderResize = true;
          window.resizeHandled();
        }
        if (m_toggleCaptureMouse) {
          m_toggleCaptureMouse = false;
          m_captureMouse = !m_captureMouse;
          window.captureMouse(m_captureMouse);
        }
        if (m_toggleBorderlessFullscreen) {
          m_toggleBorderlessFullscreen = false;
          window.toggleBorderlessFullscreen();
        }
        m_inputs.pollDevices(gamepad::Controllers::PollOptions::OnlyPoll);
        m_agamepad.write(m_inputs.getFirstActiveGamepad());
        auto kinputs = window.inputs();
        m_ainputs.write(kinputs);
        m_amouse.write(window.mouse());
        m_inputsUpdated = kinputs.frame();
        auto sizeInfo = window.posAndSize();
        m_windowSize = int2(sizeInfo.z, sizeInfo.w);
        m_windowPos = sizeInfo.xy();
        if (logicAndRenderAsync.is_ready())
          break;
        css::executeFor(16000);
        //std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
      m_renderActive = false;
      logicAndRenderAsync.get();
      dev.waitGpuIdle();
    }
    if (!m_reInit)
      break;
    else
    {
      m_reInit = false;
    }
  }
  m_quit = true;
  size_t amountToWrite = 0;
  auto data = ImGui::SaveIniSettingsToMemory(&amountToWrite);
  vector<uint8_t> out;
  out.resize(amountToWrite);
  memcpy(out.data(), data, amountToWrite);
  m_fs.writeFile("/imgui.config", makeMemView(out));
  higanbana::profiling::writeProfilingData(m_fs);
  m_log.update();
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

  AppInputs inputs;
  try
  {
    auto results = options.parse(params.m_argc, params.m_argv);
    if (results.count("vulkan"))
    {
      inputs.cmdlineApiId = GraphicsApi::Vulkan;
    }
    if (results.count("dx12"))
    {
      inputs.cmdlineApiId = GraphicsApi::DX12;
    }
    if (results.count("amd"))
    {
      inputs.cmdlineVendorId = VendorID::Amd;
    }
    if (results.count("intel"))
    {
      inputs.cmdlineVendorId = VendorID::Intel;
    }
    if (results.count("nvidia"))
    {
      inputs.cmdlineVendorId = VendorID::Nvidia;
    }
    if (results.count("gfx_debug"))
    {
      inputs.validationLayer = true;
    }
    if (results.count("rgp"))
    {
      inputs.rgpCapture = true;
      inputs.cmdlineVendorId = VendorID::Amd;
      HIGAN_LOGi("Preparing for RGP capture...\n");
      inputs.allowedApis = inputs.cmdlineApiId;
    }
    if (results.count("help"))
    {
      HIGAN_LOGi("%s\n", options.help({""}).c_str());
      return;
    }
  }
  catch(const std::exception& e)
  {
    //std::cerr << e.what() << '\n';
    HIGAN_LOGi("Bad commandline! reason: %s\n%s", e.what(), options.help({""}).c_str());
    return;
  }
  
  RenderingApp app(inputs);
  app.runCoreLoop(params);
}
