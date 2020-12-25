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
  , lower_left_corner(higanbana::math::sub(origin, higanbana::math::sub(higanbana::math::div(horizontal,2.0), higanbana::math::sub(higanbana::math::div(vertical,2.0), double3(0, 0, focal_length)))))
{
}
double viewport_height;
double viewport_width;
double focal_length;

double3 origin;
double3 horizontal;
double3 vertical;
double3 lower_left_corner;

double3 ray_color(const Ray& r) {
  double3 unit_direction = higanbana::math::normalize(r.direction());
  auto t = 0.5*(unit_direction.y + 1.0);
  return add(mul((1.0-t),double3(1.0, 1.0, 1.0)), mul(t,double3(0.5, 0.7, 1.0)));
}
};
}