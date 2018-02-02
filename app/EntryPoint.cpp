#include "core/src/Platform/EntryPoint.hpp"

//#include "windowMain.hpp"
#include "fighterMain.hpp"

#include "core/src/system/LBS.hpp"
#include "core/src/system/time.hpp"
#include "core/src/system/logger.hpp"
#include "faze/src/new_gfx/GraphicsCore.hpp"

using namespace faze;
using namespace faze::math;

int EntryPoint::main()
{
  fighterWindow(m_params);

  return 0;
}