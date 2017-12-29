#pragma once

#define DIRECTINPUT_VERSION 0x0800
#define _CRT_SECURE_NO_DEPRECATE
#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#include <windows.h>

#include <oleauto.h>
#include <string>

#include "core/src/global_debug.hpp"

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput);
