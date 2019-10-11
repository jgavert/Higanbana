#pragma once
#include "higanbana/core/platform/ProgramParams.hpp"

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
