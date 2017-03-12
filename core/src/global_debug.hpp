#pragma once

#include "system/fazmesg.hpp"

#if defined(FAZE_PLATFORM_WINDOWS)
#include <Windows.h>
#include <comdef.h>
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
#define F_LOG(msg, ...) log_sys("Output", msg, ##__VA_ARGS__)
#define F_ILOG(prefix, msg, ...) log_imSys(prefix, msg, ##__VA_ARGS__)
#define F_SLOG(prefix, msg, ...) log_sys(prefix, msg, ##__VA_ARGS__)
#define F_LOG_UNFORMATTED(msg, ...) log_def(msg, ##__VA_ARGS__);

#if 1 //defined(DEBUG)
#if defined(FAZE_PLATFORM_WINDOWS)
#define F_ERROR(msg, ...) \
  log_immideateAssert(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
  if (IsDebuggerPresent()) \
    __debugbreak(); \
  abort();

#define F_ASSERT(cond, msg, ...)\
        do \
        { \
        if (!(cond)) \
          { \
            log_immideateAssert(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
            if (IsDebuggerPresent()) \
              __debugbreak(); \
            abort();\
          } \
      } while (0)

#define FAZE_CHECK_HR(hr) \
        do \
        { \
        if (FAILED(hr)) \
          { \
            _com_error err(hr); \
            LPCTSTR errMsg = err.ErrorMessage(); \
            log_immideateAssert(__FILE__, __LINE__, "[SYSTEM/fail]: HRESULT: \"%s\"", errMsg); \
            if (IsDebuggerPresent()) \
              __debugbreak(); \
            abort();\
          } \
      } while (0)
#else
#define F_ERROR(msg, ...) \
  log_immideate(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
  abort();

#define F_ASSERT(cond, msg, ...)\
        do \
        { \
        if (!(cond)) \
          { \
            log_immideate(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
            abort();\
          } \
      } while (0)
#endif
#else
#define F_ERROR(msg, ...) do {} while(0)
#define F_ASSERT(cond, msg, ...) do {} while(0)
#endif


#ifdef _MSC_VER
#define _snprintf c99_snprintf
#define _vsnprintf c99_vsnprintf
inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap);
inline int c99_snprintf(char* str, size_t size, const char* format, ...);

#endif // _MSC_VER

void log_adv(const char *fn, int ln, const char* format, ...);
void log_def(const char* format, ...);
void log_imSys(const char* prefix, const char* format, ...);
void log_sys(const char* prefix, const char* format, ...);

void log_immideate(const char *fn, int ln, const char* format, ...);
void log_immideateAssert(const char *fn, int ln, const char* format, ...);

std::string _log_getvalue(std::string type, float& value);
std::string _log_getvalue(std::string type, int64_t& value);
std::string _log_str(const char* s);



#define GFX_ILOG(msg, ...) F_ILOG("Graphics", msg, ##__VA_ARGS__)
#define GFX_LOG(msg, ...) F_SLOG("Graphics", msg, ##__VA_ARGS__)

