#pragma once

#include "system/fazmesg.hpp"

#ifdef WIN64
#define NOMINMAX
#include <Windows.h>
#endif
#include <string.h>
#include <cstdio>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <sstream>
#include <cassert>

#define F_DLOG(msg, ...) log_adv(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define F_LOG(msg, ...) log_def(msg, ##__VA_ARGS__)
#ifdef _MSC_VER
#define _snprintf c99_snprintf
#define _vsnprintf c99_vsnprintf
inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap);
inline int c99_snprintf(char* str, size_t size, const char* format, ...);

#endif // _MSC_VER

void log_adv(const char *fn, int ln, const char* format, ...);
void log_def(const char* format, ...);

std::string _log_getvalue(std::string type, float& value);
std::string _log_getvalue(std::string type, int64_t& value);
std::string _log_str(const char* s);



