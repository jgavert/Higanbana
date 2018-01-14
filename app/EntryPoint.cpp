#include "core/src/Platform/EntryPoint.hpp"

//#include "windowMain.hpp"

#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "core/src/system/logger.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"

using namespace faze;
using namespace faze::math;

int EntryPoint::main()
{
  //mainWindow(m_params);
  Logger log;
  FileSystem fs;
  GraphicsSubsystem graphics(GraphicsApi::Vulkan, "faze");
  auto gpuInfo = graphics.getVendorDevice(VendorID::Amd);
  auto gpu = graphics.createDevice(fs, gpuInfo);
  F_LOG("gpu \"%s\" %s apiVersion: %s %zdMB\n", gpuInfo.name.c_str(), gpuInfo.canPresent ? "can present" : "cannot present", gpuInfo.apiVersionStr.c_str(), gpuInfo.memory / 1024 / 1024);
  log.update();
  return 0;
}