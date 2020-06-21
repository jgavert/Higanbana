#pragma once
#include "higanbana/core/platform/ProgramParams.hpp"
#include <css/thread_pool.hpp>

class EntryPoint
{
private:
	ProgramParams m_params;
public:
	EntryPoint(ProgramParams params)
	: m_params(params)
	{
		css::createThreadPool();
	}

	// this is "main"
	int main();
};
