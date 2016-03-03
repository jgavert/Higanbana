#pragma once
#include "core/src/global_debug.hpp"


class _ApiDebug
{
public:
  static void dbgMsg(const char* msg, int num);
};

#define F_VK_ERROR(msg, eeenum) \
 _ApiDebug::dbgMsg(msg, eeenum);

