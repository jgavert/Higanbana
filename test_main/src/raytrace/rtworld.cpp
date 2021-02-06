#include "rtworld.hpp"

#include "camera.hpp"
#include "material.hpp"
#include "sphere.hpp"

namespace rt {
rt::HittableList random_scene() {
  using namespace rt;
  HittableList world;

  auto ground_material = std::make_shared<Lambertian>(double3(0.5, 0.5, 0.5));
  world.add(std::make_shared<Sphere>(double3(0,-1000,0), 1000, ground_material));

  for (int a = -11; a < 11; a++) {
    for (int b = -11; b < 11; b++) {
      auto choose_mat = random_double();
      double3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

      if (length(sub(center, double3(4, 0.2, 0))) > 0.9) {
        std::shared_ptr<Material> sphere_material;

        if (choose_mat < 0.8) {
          // diffuse
          auto albedo = mul(random_vec(), random_vec());
          sphere_material = std::make_shared<Lambertian>(albedo);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        } else if (choose_mat < 0.95) {
          // metal
          auto albedo = random_vec(0.5, 1);
          auto fuzz = random_double(0, 0.5);
          sphere_material = std::make_shared<Metal>(albedo, fuzz);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        } else {
          // glass
          sphere_material = std::make_shared<Dielectric>(1.5);
          world.add(std::make_shared<Sphere>(center, 0.2, sphere_material));
        }
      }
    }
  }

  auto material1 = std::make_shared<Dielectric>(1.5);
  world.add(std::make_shared<Sphere>(double3(0, 1, 0), 1.0, material1));

  auto material2 = std::make_shared<Lambertian>(double3(0.4, 0.2, 0.1));
  world.add(std::make_shared<Sphere>(double3(-4, 1, 0), 1.0, material2));

  auto material3 = std::make_shared<Metal>(double3(0.7, 0.6, 0.5), 0.0);
  world.add(std::make_shared<Sphere>(double3(4, 1, 0), 1.0, material3));

  return world;
}
  World::World() {
    world = random_scene();
  }
}