#include <higanbana/core/system/LBS.hpp>
#include <higanbana/core/system/time.hpp>
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/entity/database.hpp>

#include <catch2/catch_all.hpp>

struct Position
{
  float4 val;
};

struct Velocity
{
  float4 val;
};

struct Mass
{
  float mass;
};

TEST_CASE("entity operations")
{
  using namespace higanbana;
  Database<2048> db;
  auto& pos = db.get<Position>();
  auto& vel = db.get<Velocity>();
  auto& mass = db.get<Mass>();

  int entityID = -1;
  {
    std::default_random_engine gen;
    std::uniform_real_distribution<float> dist(-10.0,10.0);
    std::uniform_real_distribution<float> dist2(-0.1,0.1);
    std::uniform_real_distribution<float> dist3(0.1,0.4);
    auto e = db.createEntity();
    pos.insert(e, Position{float4(dist(gen), dist(gen), dist(gen),1.f)});
    vel.insert(e, Velocity{float4(dist2(gen), dist2(gen), dist2(gen),0.f)});
    mass.insert(e, Mass{dist3(gen)});
    entityID = e;
  }
  int count = 0;
  query(pack(pos, vel, mass), [&](higanbana::Id id, Position& pos, Velocity& vel, Mass& mass)
  {
    pos.val = math::mul(math::mul(pos.val, vel.val), mass.mass);
    mass.mass -= 0.00001f;
    vel.val = math::add(float4(0.00001f, 0.00001f, 0.0001f, 0.0001f), vel.val);
    REQUIRE(id == entityID);
    count++;
  });
  REQUIRE(count == 1);
}
