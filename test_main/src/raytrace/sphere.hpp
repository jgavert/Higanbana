#pragma once

#include "hittable.hpp"
#include <higanbana/core/math/math.hpp>

namespace rt
{
class Sphere : public Hittable {
public:
  Sphere() {}
  Sphere(double3 cen, double r) : center(cen), radius(r) {};

  virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override;

public:
  double3 center;
  double radius;
};
}