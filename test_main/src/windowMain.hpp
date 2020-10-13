#pragma once

#include <higanbana/core/platform/EntryPoint.hpp>
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
#include <css/task.hpp>
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

struct AppInputs {
  higanbana::GraphicsApi cmdlineApiId = higanbana::GraphicsApi::DX12;
  higanbana::VendorID cmdlineVendorId = higanbana::VendorID::Amd;
  higanbana::GraphicsApi allowedApis = higanbana::GraphicsApi::All;
  bool rgpCapture = false;
  bool validationLayer = false;
};
class RenderingApp {
  private:
  AppInputs m_cmdline;
  higanbana::GraphicsApi m_api;
  higanbana::VendorID m_preferredVendor;
  higanbana::FileSystem m_fs;
  app::RendererOptions m_renderOptions;
  higanbana::Logger m_log;
  higanbana::Database<2048> m_ecs;
  app::World m_world;
  higanbana::gamepad::Controllers m_inputs;
  app::EntityView m_entityViewer;
  app::SceneEditor m_sceneEditor;
  app::EntityEditor m_entityEditor;
  bool m_quit = false;
  std::atomic_bool m_reInit = false;
  std::atomic_int64_t m_frame = 1;
  bool m_renderGltfScene = false;
  bool m_renderECS = false;
  int m_cubeCount = 1000;
  int m_cubeCommandLists = 1;
  int m_limitFPS = -1;
  int m_viewportCount = 1;
  bool m_captureMouse = false;
  bool m_advanceSimulation = true;
  bool m_stepOneFrameForward = false;
  bool m_stepOneFrameBackward = false;
  higanbana::LBS m_lbs; // todo: get rid of this.
  uint m_chosenDeviceID = 0;
  higanbana::GraphicsApi m_chosenApi;
  bool m_interoptDevice = false;
  int2 m_windowSize = div(int2(1280, 720), 1);
  int2 m_windowPos = int2(300,400);

  higanbana::WTime m_logicAndRenderTime;
  higanbana::WTime m_time;

  higanbana::vector<higanbana::GpuInfo> allGpus;
  higanbana::vector<higanbana::GpuInfo> activeDevices;
  higanbana::vector<bool> selectedDevice;
  higanbana::GpuInfo physicalDevice;

  bool m_closeAnyway = false;
  std::atomic_bool m_renderActive = true;
  bool m_renderResize = false;
  bool m_controllerConnected = false;
  bool m_autoRotateCamera = false;

  std::atomic_bool m_toggleCaptureMouse = false;
  std::atomic_bool m_toggleBorderlessFullscreen = false;

  higanbana::AtomicBuffered<higanbana::InputBuffer> m_ainputs;
  higanbana::AtomicBuffered<higanbana::gamepad::X360LikePad> m_agamepad;
  higanbana::AtomicBuffered<higanbana::MouseState> m_amouse;
  std::atomic<int> m_inputsUpdated = 0;
  int m_inputsRead = 0;

  css::Task<int> runVisualLoop(app::Renderer& rend, higanbana::GpuGroup& dev); // while task not done, renders. Returns success, if quit, otherwise should be restarted.

  public:
  RenderingApp(AppInputs inputs);

  void runCoreLoop(ProgramParams& params); // this is our "core loop" or update created window. Since there are stupid requirements who can run it.

};

void mainWindow(ProgramParams& params);