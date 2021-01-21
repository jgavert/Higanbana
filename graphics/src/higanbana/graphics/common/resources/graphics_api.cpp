#include "higanbana/graphics/common/resources/graphics_api.hpp"

namespace higanbana
{
  const char* toString(GraphicsApi api)
  {
    if (api == GraphicsApi::DX12)
      return "DX12";
    return "Vulkan";
  }
  const char* toString(VendorID id)
  {
    if (id == VendorID::Amd)
      return "AMD";
    if (id == VendorID::Nvidia)
      return "Nvidia";
    if (id == VendorID::Intel)
      return "Intel";
    return "Any";
  }

  const char* toString(QueueType id)
  {
    if (id == QueueType::Graphics)
      return "Graphics";
    if (id == QueueType::Compute)
      return "Compute";
    if (id == QueueType::Dma)
      return "Dma";
    if (id == QueueType::External)
      return "External";
    return "Unknown";
  }
  
	const char* presentModeToStr(PresentMode mode)
	{
		switch (mode)
		{
		case PresentMode::Immediate:
			return "Immediate";
		case PresentMode::Mailbox:
			return "Mailbox";
		case PresentMode::Fifo:
			return "Fifo";
		case PresentMode::FifoRelaxed:
			return "FifoRelaxed";
		case PresentMode::Unknown:
		default:
			break;
	    }
		return "Unknown PresentMode";
	}
	const char* colorspaceToStr(Colorspace mode)
	{
		switch (mode)
		{
		case Colorspace::BT709:
			return "BT709";
		case Colorspace::BT2020:
			return "BT2020";
		default:
			break;
		}
		return "Unknown Colorspace";
	}
	const char* displayCurveToStr(DisplayCurve mode)
	{
		switch (mode)
		{
		case DisplayCurve::None:
			return 	"Linear";
		case DisplayCurve::sRGB:
			return 	"sRGB";
		case DisplayCurve::ST2084:
			return 	"ST2084";
		default:
			break;
		}
		return "Unknown DisplayCurve";
	}
}