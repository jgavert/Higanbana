#pragma once
#include "higanbana/core/platform/ProgramParams.hpp"
#include "higanbana/core/coroutine/task.hpp"

class EntryPoint
{
private:
	ProgramParams m_params;
public:
	EntryPoint(ProgramParams params)
	: m_params(params)
	{
		higanbana::experimental::globals::createGlobalLBSPool();
	}

	// this is "main"
	higanbana::coro::task<int> main();
};
