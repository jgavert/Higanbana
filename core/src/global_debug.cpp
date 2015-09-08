#include "global_debug.hpp"
#ifdef _MSC_VER
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

using namespace faze;

void log_adv(const char *fn, int ln, const char* format, ...)
{
  va_list args;
  char buf[1024];
  int n = snprintf(buf, sizeof(buf), "%s:%d> ", fn, ln);

  va_start(args, format);
  _vsnprintf(&buf[n], sizeof(buf)-n, format, args);
  va_end(args);

  LogMessage log(buf);
  sendMessage(log);
}

void log_def(const char* format, ...)
{
  va_list args;
  char buf[1024];
  va_start(args, format);
  _vsnprintf(&buf[0], sizeof(buf), format, args);
  va_end(args);
  LogMessage log(buf);
  sendMessage(log);
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
