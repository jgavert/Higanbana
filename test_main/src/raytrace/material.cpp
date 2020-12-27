#include "material.hpp"
#include "hittable.hpp"
#include "rtweekend.hpp"

namespace rt {

bool Lambertian::scatter(
  const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
) const {
  auto scatter_direction = add(rec.normal, random_unit_vector());
  if (near_zero(scatter_direction))
    scatter_direction = rec.normal;
  scattered = Ray(rec.p, scatter_direction);
  attenuation = albedo;
  return true;
}

bool Metal::scatter(
  const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
) const {
  double3 reflected = reflect(normalize(r_in.direction()), rec.normal);
  scattered = Ray(rec.p, add(reflected, mul(fuzz,random_in_unit_sphere())));
  attenuation = albedo;
  return (dot(scattered.direction(), rec.normal) > 0);
}

bool Dielectric::scatter(
  const Ray& r_in, const HitRecord& rec, double3& attenuation, Ray& scattered
) const {
  attenuation = double3(1.0, 1.0, 1.0);
  double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

  double3 unit_direction = normalize(r_in.direction());
  double3 refracted = refract(unit_direction, rec.normal, refraction_ratio);
  double cos_theta = fmin(dot(mul(-1.0,unit_direction), rec.normal), 1.0);
  double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

  bool cannot_refract = refraction_ratio * sin_theta > 1.0;
  double3 direction;

  if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
    direction = reflect(unit_direction, rec.normal);
  else
    direction = refract(unit_direction, rec.normal, refraction_ratio);

  scattered = Ray(rec.p, direction);
  return true;
}
}