#pragma once

#include <cmath>
#include <limits>
#include <random>

// Constants
namespace rt {
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees) {
  return degrees * pi / 180.0;
}

inline double random_double() {
  thread_local static std::uniform_real_distribution<double> distribution(0.0, 1.0);
  thread_local static std::mt19937 generator;
  return distribution(generator);
}

inline double random_double(double min, double max) {
  // Returns a random real in [min,max).
  return min + (max-min)*random_double();
}

inline double3 random_vec() {
  return double3(random_double(), random_double(), random_double());
}

inline double3 random_vec(double min, double max) {
  return double3(random_double(min,max), random_double(min,max), random_double(min,max));
}

inline double3 random_in_unit_sphere() {
  while (true) {
    auto p = random_vec(-1,1);
    if (length_squared(p) >= 1) continue;
    return p;
  }
}

inline double3 random_unit_vector() {
    return normalize(random_in_unit_sphere());
}

inline double3 random_in_hemisphere(const double3& normal) {
  double3 in_unit_sphere = random_in_unit_sphere();
  if (dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
    return in_unit_sphere;
  else
    return mul(in_unit_sphere, -1.0);
}

inline double clamp(double x, double min, double max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

inline double3 color_samples(double3 pixel_color, int samples_per_pixel) {
  auto scale = 1.0 / samples_per_pixel;
  // Divide the color by the number of samples.
  return mul(pixel_color, scale);
}
// Common Headers

//#include "ray.h"
//#include "vec3.h"
}
