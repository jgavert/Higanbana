#pragma once

#include "higanbana/core/system/fazmesg.hpp"
#include "higanbana/core/platform/definitions.hpp"

#include <string.h>
#include <cstdio>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <sstream>
#include <cassert>

#include <locale>
#include <codecvt>

#define HIGAN_DEBUG_LOG(msg, ...) log_adv(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define HIGAN_LOG(msg, ...) log_sys("Output", msg, ##__VA_ARGS__)
#define HIGAN_LOGi(msg, ...) log_im(msg, ##__VA_ARGS__)
#define HIGAN_ILOG(prefix, msg, ...) log_imSys(prefix, msg, ##__VA_ARGS__)
#define HIGAN_SLOG(prefix, msg, ...) log_sys(prefix, msg, ##__VA_ARGS__)
#define HIGAN_LOG_UNFORMATTED(msg, ...) log_def(msg, ##__VA_ARGS__);

#if 1 //defined(DEBUG)
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#define HIGAN_ERROR(msg, ...) \
  log_immideateAssert(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
  debugBreak(); \
  abort();

#define HIGAN_ASSERT(cond, msg, ...)\
        do \
        { \
          if (!(cond)) \
          { \
            log_immideateAssert(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
            debugBreak(); \
            abort(); \
          } \
      } while (0)

#define HIGANBANA_CHECK_HR(hr) \
        do \
        { \
        if (checkHRError(hr)) \
          { \
            debugBreak(); \
            abort();\
          } \
      } while (0)
#else
#define HIGAN_ERROR(msg, ...) \
  log_immideate(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
  abort();

#define HIGAN_ASSERT(cond, msg, ...)\
        do \
        { \
        if (!(cond)) \
          { \
            log_immideate(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
            abort();\
          } \
      } while (0)
#endif
#elif NDEBUG
#ifdef _MSC_VER
#define HIGAN_ERROR(msg, ...) __assume(false);
#define HIGAN_ASSERT(cond, msg, ...) __assume(cond);
#define HIGANBANA_CHECK_HR(hr) do { hr; } while (0)
#else
#define HIGAN_ERROR(msg, ...) __builtin_unreachable();
#define HIGAN_ASSERT(cond, msg, ...) do { if (!(cond)) { __builtin_unreachable(); } } while(0)
#define HIGANBANA_CHECK_HR(hr) do { hr; } while (0)
#endif
#endif

void debugBreak();

#ifdef _MSC_VER

bool checkHRError(long hr);

#define _snprintf c99_snprintf
#define _vsnprintf c99_vsnprintf
inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap);
inline int c99_snprintf(char* str, size_t size, const char* format, ...);

#endif // _MSC_VER

void log_adv(const char *fn, int ln, const char* format, ...);
void log_def(const char* format, ...);
void log_im(const char* format, ...);
void log_imSys(const char* prefix, const char* format, ...);
void log_sys(const char* prefix, const char* format, ...);

void log_immideate(const char *fn, int ln, const char* format, ...);
void log_immideateAssert(const char *fn, int ln, const char* format, ...);

std::string _log_getvalue(std::string type, float& value);
std::string _log_getvalue(std::string type, int64_t& value);
std::string _log_str(const char* s);

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);