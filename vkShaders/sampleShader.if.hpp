#include "shader_defines.hpp"

FAZE_BEGIN_LAYOUT(BasicLayout)

FAZE_BufferSRV(buffer, DataIn, 0)
{
  float a[];
} dataIn;

FAZE_BufferUAV(buffer, DataOut, 1)
{
  float a[];
} dataOut;

FAZE_END_LAYOUT