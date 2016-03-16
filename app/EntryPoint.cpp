#include "core/src/Platform/EntryPoint.hpp"

// EntryPoint.cpp
#ifdef WIN64
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "core/src/neural/network.hpp"
#include "core/src/system/logger.hpp"

#include <cstdio>
#include <iostream>


using namespace faze;

int EntryPoint::main()
{
  Logger log;
  auto main = [=, &log](std::string name)
  {
	testNetwork();
    log.update();
  };

  main("w1");
  return 0;
}
