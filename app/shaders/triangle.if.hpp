#ifndef FAZE_SHADER_DEFINITIONS
#define FAZE_SHADER_DEFINITIONS

#include "../graphics/definitions.hpp"

FAZE_BEGIN_LAYOUT(Triangle)

struct TriangleConstants
{
	float4 color;
};

FAZE_CBUFFER(color)

FAZE_END_LAYOUT


#endif /*FAZE_SHADER_DEFINITIONS*/