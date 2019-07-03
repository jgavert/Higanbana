#include "entity_test.hpp"
#include <core/system/LBS.hpp>
#include <core/system/time.hpp>
#include <core/math/math.hpp>
#include <core/entity/database.hpp>

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


void test_entity()
{
  using namespace faze;
  Database<2048> db;
  auto& pos = db.get<Position>();
  auto& vel = db.get<Velocity>();
  auto& mass = db.get<Mass>();

  constexpr const int max_entitys = 200000;
  {
    std::default_random_engine gen;
    std::uniform_real_distribution<float> dist(-10.0,10.0);
    std::uniform_real_distribution<float> dist2(-0.1,0.1);
    std::uniform_real_distribution<float> dist3(0.1,0.4);
    for (int i = 0; i < max_entitys; i++)
    {
      auto e = db.createEntity();
      //F_ILOG("wtf", "id %zu", e);
      pos.insert(e, Position{float4(dist(gen), dist(gen), dist(gen),1.f)});
      vel.insert(e, Velocity{float4(dist2(gen), dist2(gen), dist2(gen),0.f)});
      mass.insert(e, Mass{dist3(gen)});
      //db.createEntity();
    }
  }

  {
    LBS lbs;
    auto task2 = desc::Task("asd", {}, {});
    lbs.addParallelFor<1>(task2, 0, 100, [](size_t){});
    WTime timer;
    timer.startFrame();
    auto task = desc::Task("Kekbur", {"startTimer"}, {});
    queryParallel(lbs, task, pack(pos, vel, mass), [](faze::Id id, Position& pos, Velocity& vel, Mass& mass)
    {
      pos.val = math::mul(math::mul(pos.val, vel.val), mass.mass);
      mass.mass -= 0.00001f;
      vel.val = math::add(float4(0.00001f, 0.00001f, 0.0001f, 0.0001f), vel.val);
    });
    lbs.addTask("endTimer", {"Kekbur"}, {}, [&](size_t)
    {
      timer.tick(); 
    });
    lbs.addTask("startTimer", [&](size_t)
    {
      timer.startFrame(); 
    });

    lbs.sleepTillKeywords({"endTimer"});
    auto val = timer.getCurrentNano();
    timer.startFrame();
    int ids = 0;
    query(pack(pos, vel, mass),[&ids](faze::Id id, Position& pos, Velocity& vel, Mass& mass)
    {
      pos.val = math::mul(math::mul(pos.val, vel.val), mass.mass);
      mass.mass -= 0.00001f;
      vel.val = math::add(float4(0.00001f, 0.00001f, 0.0001f, 0.0001f), vel.val);
      ids++;
    });
    timer.tick();
    auto val2 = timer.getCurrentNano();
    F_ILOG("queryParallel", "time took for %d id's simple operation: mt %zuns vs single thread %zuns", ids, val, val2);
  }
}
using namespace faze;
void test_bitfield()
{
  DynamicBitfield bits;

  bits.setBit(0);
  bits.setBit(10);
  bits.setBit(64);
  bits.setBit(74);
  bits.setBit(75);

  int index = bits.findFirstSetBit(0);
  while (index >= 0 )
  {
    F_ILOG("setbit", "found index: %d expecting {0, 10, 64, 74, 75}", index);
    index = bits.findFirstSetBit(index+1);
  }
}