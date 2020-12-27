#pragma once

#include "hittable.hpp"
#include <higanbana/core/math/math.hpp>
#include <memory>

namespace rt
{
class Sphere : public Hittable {
public:
  Sphere() {}
  Sphere(double3 cen, double r, std::shared_ptr<Material> m) : center(cen), radius(r), mat_ptr(m) {};

  virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override;

public:
  double3 center;
  double radius;
  std::shared_ptr<Material> mat_ptr;
};
}