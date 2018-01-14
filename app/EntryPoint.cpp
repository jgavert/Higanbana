#include "core/src/Platform/EntryPoint.hpp"

//#include "windowMain.hpp"

#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"

using namespace faze;
using namespace faze::math;

int EntryPoint::main()
{
  //mainWindow(m_params);
  FileSystem fs;
  GraphicsSubsystem graphics(GraphicsApi::Vulkan, "faze");
  auto gpuInfo = graphics.getVendorDevice(VendorID::Amd);
  auto gpu = graphics.createDevice(fs, gpuInfo);
  return 0;
}