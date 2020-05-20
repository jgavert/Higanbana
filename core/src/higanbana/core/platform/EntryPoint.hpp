#pragma once
#include "higanbana/core/platform/ProgramParams.hpp"
#include "higanbana/core/coroutine/stolen_task.hpp"

class EntryPoint
{
private:
	ProgramParams m_params;
public:
	EntryPoint(ProgramParams params)
	: m_params(params)
	{
		higanbana::taskstealer::globals::createTaskStealingPool();
	}

	// this is "main"
	int main();
};
