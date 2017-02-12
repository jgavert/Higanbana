#include "formats.hpp"

namespace faze
{
  namespace backend
  {
    FormatDX12Conversion dxformatToFazeFormat(DXGI_FORMAT format)
    {
      for (int i = 0; i < DXFormatTransformTableSize; ++i)
      {
        if (DXFormatTransformTable[i].view == format
         || DXFormatTransformTable[i].storage == format
         || DXFormatTransformTable[i].raw == format)
        {
          return DXFormatTransformTable[i];
        }
      }
      return DXFormatTransformTable[0];
    }

    FormatDX12Conversion formatTodxFormat(FormatType format)
    {
      for (int i = 0; i < DXFormatTransformTableSize; ++i)
      {
        if (DXFormatTransformTable[i].enm == format)
        {
          return DXFormatTransformTable[i];
        }
      }
      return DXFormatTransformTable[0];
    }
  }
}