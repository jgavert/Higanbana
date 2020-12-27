#pragma once

#include <higanbana/core/math/math.hpp>
#include "ray.hpp"

namespace rt {

struct HitRecord;

class Material {
public:
  virtual bool scatter(
    const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
  ) const = 0;
};

class Lambertian : public Material {
public:
  Lambertian(const double3& a) : albedo(a) {}

  virtual bool scatter(
    const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
  ) const override;

public:
  double3 albedo;
};

class Metal : public Material {
public:
  Metal(const double3& a, double f) : albedo(a), fuzz(f<1?f:1) {}

  virtual bool scatter(
    const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
  ) const override; 
public:
  double3 albedo;
  double fuzz;
};

class Dielectric : public Material {
public:
  Dielectric(double index_of_refraction) : ir(index_of_refraction) {}

virtual bool scatter(
  const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
) const override;

public:
double ir; // Index of Refraction
static double reflectance(double cosine, double ref_idx) {
  // Use Schlick's approximation for reflectance.
  auto r0 = (1-ref_idx) / (1+ref_idx);
  r0 = r0*r0;
  return r0 + (1-r0)*pow((1 - cosine),5);
}
};
}