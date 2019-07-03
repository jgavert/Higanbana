#include "graphics/common/resource_descriptor.hpp"

namespace faze
{
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