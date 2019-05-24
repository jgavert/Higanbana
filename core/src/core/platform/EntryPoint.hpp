#pragma once
#include "core/Platform/ProgramParams.hpp"

class EntryPoint
{
private:
	ProgramParams m_params;
public:
	EntryPoint(ProgramParams params)
	: m_params(params)
	{}

	// this is "main"
	int main();
};
