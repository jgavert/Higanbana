#pragma once

#include "ray.hpp"

namespace rt
{
class Material;

struct HitRecord {
  double3 p;
  double3 normal;
  Material* mat_ptr;
  double t;
  bool front_face;

  inline void set_face_normal(const Ray& r, const double3& outward_normal) {
    front_face = dot(r.direction(), outward_normal) < 0;
    normal = front_face ? outward_normal : mul(outward_normal, -1.0);
  }
};

class Hittable {
public:
  virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const = 0;
};
}