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

inline double3 random_in_unit_disk() {
  while (true) {
    auto p = double3(random_double(-1,1), random_double(-1,1), 0);
    if (length_squared(p) >= 1.0) continue;
    return p;
  }
}

inline double clamp(double x, double min, double max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

inline bool near_zero(const double3 e) {
  // Return true if the vector is close to zero in all dimensions.
  const auto s = 1e-8;
  return (fabs(e(0)) < s) && (fabs(e(1)) < s) && (fabs(e(2)) < s);
}

inline double3 reflect(const double3& v, const double3& n) {
    return sub(v, mul(2.0,mul(dot(v,n),n)));
}

inline double3 refract(const double3& uv, const double3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(mul(-1.0, uv), n), 1.0);
    double3 r_out_perp = mul(etai_over_etat, add(uv, mul(cos_theta,n)));
    double3 r_out_parallel = mul(-sqrt(fabs(1.0 - length_squared(r_out_perp))), n);
    return add(r_out_perp, r_out_parallel);
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
