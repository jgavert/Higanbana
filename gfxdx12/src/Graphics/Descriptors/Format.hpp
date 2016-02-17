#pragma once
#include "Formats.hpp"

// structured buffer
template <typename T>
struct Format
{
  size_t stride;
  FormatType format;
  Format() :format(FormatType::Unknown), stride(sizeof(T)) {}
  Format(FormatType type) : format(type), stride(0) {}
};


