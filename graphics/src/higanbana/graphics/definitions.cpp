#include "higanbana/graphics/definitions.hpp"

namespace higanbana
{
  namespace globalconfig
  {
    namespace graphics
    {
      // barrier booleans, toggle to control. Nothing thread safe here.
      // cannot toggle per window... I guess.
      bool GraphicsEnableReadStateCombining = false;
      bool GraphicsEnableHandleCommonState = true;
      bool GraphicsEnableSplitBarriers = false;
      bool GraphicsSplitBarriersPlaceBeginsOnExistingPoints = false;
      int GraphicsHowManyBytesBeforeNewCommandBuffer = 200 * 1024;
    }
  }
}