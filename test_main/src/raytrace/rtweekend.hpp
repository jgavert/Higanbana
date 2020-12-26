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
