#pragma once

#include <higanbana/core/math/math.hpp>

namespace rt {

class Ray {
public:

Ray() {}
Ray(const double3& origin, const double3& direction)
  : orig(origin), dir(direction)
{}

double3 origin() const  { return orig; }
double3 direction() const { return dir; }
double3 at(double t) const {
  return add(orig, mul(t, dir));
}

double3 orig;
double3 dir;
};
}