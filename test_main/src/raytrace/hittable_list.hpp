#pragma once

#include "hittable.hpp"

#include <higanbana/core/datastructures/vector.hpp>
#include <memory>

namespace rt {

class HittableList : public Hittable {
public:
  HittableList() {}
  HittableList(std::shared_ptr<Hittable> object) { add(object); }

  void clear() { objects.clear(); }
  void add(std::shared_ptr<Hittable> object) { objects.push_back(object); }

  virtual bool hit(
    const Ray& r, double t_min, double t_max, HitRecord& rec) const override;

public:
  std::vector<std::shared_ptr<Hittable>> objects;
};

}