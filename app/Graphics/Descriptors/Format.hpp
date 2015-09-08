#pragma once
#include "Formats.hpp"

// structured buffer
template <typename T>
struct Format
{
  size_t stride;
  FormatType format;
  Format() :format(FormatType::Unknown), stride(sizeof(T)) {}
};

template <>
struct Format<FormatType>
{
  size_t stride;
  FormatType format;
  template<FormatType type>
  Format() :format(type), stride(0) {}
};
