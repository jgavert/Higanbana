#include "higanbana/core/global_debug.hpp"

#ifdef _MSC_VER
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include <Windows.h>
#include <comdef.h>
#endif


void debugBreak()
{
  if (IsDebuggerPresent())
    __debugbreak();
}

bool checkHRError(const char *fn, int ln, long hr)
{
  if (FAILED(hr))
  {
    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();
    std::string _msg = ws2s(errMsg);
    log_immideateAssert(fn, ln, "[SYSTEM/fail]: HRESULT: \"%s\"", _msg.c_str());
    return true;
  }
  return false;
}

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
  int count = -1;

  if (size != 0)
    count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  if (count == -1)
    count = _vscprintf(format, ap);

  return count;
}
inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
  int count;
  va_list ap;

  va_start(ap, format);
  count = c99_vsnprintf(str, size, format, ap);
  va_end(ap);

  return count;
}

#endif // _MSC_VER

using namespace higanbana;

void log_adv(const char *fn, int ln, const char* format, ...)
{
  va_list args;
  char buf[1024];
  int n = snprintf(buf, sizeof(buf), "%s(%d): ", fn, ln);

  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#else
  vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#endif
  va_end(args);

  LogMessage log(buf);
  sendMessage(log);
}

void log_def(const char* format, ...)
{
  va_list args;
  char buf[1024];
  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[0], sizeof(buf), format, args);
#else
  vsnprintf(&buf[0], sizeof(buf), format, args);
#endif
  va_end(args);
  LogMessage log(buf);
  sendMessage(log);
}

void log_imSys(const char* prefix, const char* format, ...)
{
  va_list args;
  char buf[1024];
  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[0], sizeof(buf), format, args);
#else
  vsnprintf(&buf[0], sizeof(buf), format, args);
#endif
  va_end(args);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  OutputDebugStringA("[");
  OutputDebugStringA(prefix);
  OutputDebugStringA("] ");
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
#endif
  /*
  std::cerr << "[" << prefix << "] ";
  std::cerr.write(buf, strlen(buf));
  std::cerr << std::endl;
  */
  printf("[%s] %s\n", prefix, buf);
}

void log_im(const char* format, ...)
{
  va_list args;
  char buf[1024];
  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[0], sizeof(buf), format, args);
#else
  vsnprintf(&buf[0], sizeof(buf), format, args);
#endif
  va_end(args);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  OutputDebugStringA(buf);
#endif
  /*
  std::cerr.write(buf, strlen(buf));
  */
  printf("%s", buf);
}

void log_sys(const char* prefix, const char* format, ...)
{
  va_list args;
  char buf[1024];
  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[0], sizeof(buf), format, args);
#else
  vsnprintf(&buf[0], sizeof(buf), format, args);
#endif
  va_end(args);

  std::string asd = "[";
  asd += prefix;
  asd += "] ";
  asd += buf;
  LogMessage log(asd.c_str());
  sendMessage(log);
}

void log_immideateAssert(const char *fn, int ln, const char* format, ...)
{
  va_list args;
  char buf[1024];
  int n = snprintf(buf, sizeof(buf), "%s(%d): ASSERT!!!\n", fn, ln);

  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#else
  vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#endif
  va_end(args);

#if defined(HIGANBANA_PLATFORM_WINDOWS)
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
#endif
  /*
  std::cerr.write(buf, strlen(buf));
  std::cerr << std::endl;
  */
  printf("%s\n", buf);
}

void log_immideate(const char *fn, int ln, const char* format, ...)
{
  va_list args;
  char buf[1024];
  int n = snprintf(buf, sizeof(buf), "%s(%d): ", fn, ln);

  va_start(args, format);
#if defined(HIGANBANA_PLATFORM_WINDOWS)
  _vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#else
  vsnprintf(&buf[n], sizeof(buf) - n, format, args);
#endif
  va_end(args);

#if defined(HIGANBANA_PLATFORM_WINDOWS)
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
#endif
  /*
  std::cerr.write(buf, strlen(buf));
  std::cerr << std::endl;
  */
  printf("%s\n", buf);
}

std::string _log_getvalue(std::string type, float& value)
{
  assert(type == "f");
  return std::to_string(value);
}

std::string _log_getvalue(std::string type, int64_t& value)
{
  assert(type == "i64");
  return std::to_string(value);
}

std::string _log_str(const char* s)
{
  return std::string(s, strlen(s));
}

std::wstring s2ws(const std::string& str)
{
  using convert_typeX = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
  using convert_typeX = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.to_bytes(wstr);
}