#pragma once

#include <higanbana/core/math/math.hpp>
#include "hittable.hpp"
#include "material.hpp"
#include "rtweekend.hpp"

namespace rt {
class Camera {
public:
Camera(){}
Camera(double3 pos, double3 dir, double3 up, double3 side, double vfov, double aspect_ratio, double aperture, double focus_dist)
  : origin(pos)
  , lower_left_corner(origin)
  , u(side)
  , v(up)
  , w(dir)
{
  using namespace higanbana::math;
  auto theta = degrees_to_radians(vfov);
  auto h = tan(theta/2.0);
  auto viewport_height = 2.0 * h;
  auto viewport_width = aspect_ratio * viewport_height;

  auto focal_length = 1.0;

  horizontal = mul(viewport_width*focus_dist, u);
  vertical = mul(viewport_height*focus_dist, v);
  auto th = div(horizontal, 2.0);
  auto tv = div(vertical, 2.0);
  lower_left_corner = sub(sub(sub(origin, th), tv), mul(w, focus_dist));

  lens_radius = aperture / 2.0;
}
//double viewport_height;
//double viewport_width;
//double focal_length;

double3 origin;
double3 horizontal;
double3 vertical;
double3 lower_left_corner;
double3 u,v,w;
double lens_radius;


constexpr auto operator<=>(const Camera&) const = default;

inline Ray get_ray(const double2 uv) {
  double3 rd = mul(lens_radius, random_in_unit_disk());
  double3 offset = add(mul(u, rd.x), mul(v, rd.y));

  double3 t2 = mul(uv.x, horizontal);
  double3 t3 = mul(uv.y, vertical);
  auto dir = sub(sub(add(add(lower_left_corner, t2), t3), origin), offset);
  return rt::Ray(add(origin, offset), dir);
}

inline double hit_sphere(const double3& center, double radius, const Ray& r) {
  double3 oc = sub(r.origin(), center);
  auto a = length_squared(r.direction());
  auto half_b = dot(oc, r.direction());
  auto c = length_squared(oc) - radius * radius;
  auto discriminant = half_b*half_b - a*c;
  if (discriminant < 0) {
    return -1.0;
  } else {
    return (-half_b - sqrt(discriminant) ) / a;
  }
}

inline double3 ray_color(const Ray& r, const Hittable& world, int depth) {
  HitRecord rec;
  if (depth <= 0)
    return double3(0,0,0);
  if (world.hit(r, 0.001, infinity, rec)) {
    Ray scattered;
    double3 attenuation;
    if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
      return mul(attenuation, ray_color(scattered, world, depth-1));
    return double3(0,0,0);
  }
  double3 unit_direction = normalize(r.direction());
  auto t = 0.5*(unit_direction.y + 1.0);
  return add(mul((1.0-t),double3(1.0, 1.0, 1.0)), mul(t, double3(0.5, 0.7, 1.0)));
}
};
}