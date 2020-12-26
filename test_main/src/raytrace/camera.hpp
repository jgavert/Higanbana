#pragma once

#include <higanbana/core/math/math.hpp>
#include "ray.hpp"

namespace rt {
class Camera {
public:
Camera(){}
Camera(double aspect_ratio)
  : viewport_height(2.0)
  , viewport_width(aspect_ratio * viewport_height)
  , focal_length(1.0)
  , origin(double3(0,0,0))
  , horizontal(double3(viewport_width, 0, 0))
  , vertical(double3(0, viewport_height, 0))
  , lower_left_corner(origin)
{
  using namespace higanbana::math;
  auto t0 = div(horizontal, 2.0);
  auto t1 = div(vertical, 2.0);
  lower_left_corner = sub(sub(sub(origin, t0), t1), double3(0, 0, focal_length));
}
double viewport_height;
double viewport_width;
double focal_length;

double3 origin;
double3 horizontal;
double3 vertical;
double3 lower_left_corner;

inline double hit_sphere(const double3& center, double radius, const Ray& r) {
  double3 oc = sub(r.origin(), center);
  auto a = dot(r.direction(), r.direction());
  auto b = 2.0 * dot(oc, r.direction());
  auto c = dot(oc, oc) - radius*radius;
  auto discriminant = b*b - 4*a*c;
  if (discriminant < 0) {
    return -1.0;
  } else {
    return (-b - sqrt(discriminant) ) / (2.0*a);
  }
}

inline double3 ray_color(const Ray& r) {
  auto t = hit_sphere(double3(0,0,-1), 0.5, r);
  if (t > 0.0) {
    double3 N = normalize(sub(r.at(t), double3(0,0,-1)));
    return mul(0.5, double3(N.x+1, N.y+1, N.z+1));
  }
  double3 unit_direction = higanbana::math::normalize(r.direction());
  t = 0.5*(unit_direction.y + 1.0);
  return add(mul((1.0-t),double3(1.0, 1.0, 1.0)), mul(t,double3(0.5, 0.7, 1.0)));
}
};
}