#include "definitions.hpp"

namespace faze
{
  namespace globalconfig
  {
    namespace graphics
    {
      // barrier booleans, toggle to control. Nothing thread safe here.
      // cannot toggle per window... I guess.
      bool GraphicsEnableReadStateCombining = false;
      bool GraphicsEnableHandleCommonState = false;
      bool GraphicsEnableSplitBarriers = true;
      bool GraphicsSplitBarriersPlaceBeginsOnExistingPoints = true;
    }
  }
}