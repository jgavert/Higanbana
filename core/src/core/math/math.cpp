#include "core/math/math.hpp"

namespace faze
{
  namespace math
  {
    std::string toString(Quaternion a)
    {
      std::string fnl = "(";
      for (int i = 0; i < 3; ++i)
      {
        fnl += std::to_string(a(i)) + std::string(", ");
      }
      return fnl + std::to_string(a(3)) + std::string(")");
    }
  }
}