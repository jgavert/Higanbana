#include <catch2/catch_all.hpp>
#include <higanbana/core/math/math.hpp>
#include <higanbana/core/global_debug.hpp>

using namespace higanbana;
using namespace higanbana::math;

void printfloat3(float3 val) {
  HIGAN_LOGi("%.2f %.2f %.2f\n", val.x, val.y, val.z);
}
void printfloat4(float4 val) {
  HIGAN_LOGi("%.2f %.2f %.2f %.2f\n", val.x, val.y, val.z, val.w);
}

TEST_CASE( "simple camera movement" ) {
  quaternion direction = {1.f, 0.f, 0.f, 0.f};
  float3 pos{};
  float3 dir = normalize(rotateVector({ 0.f, 0.f, 1.f }, direction));
  float3 updir = normalize(rotateVector({ 0.f, 1.f, 0.f }, direction));
  float3 sideVec = normalize(rotateVector({ 1.f, 0.f, 0.f }, direction));

  HIGAN_LOGi("dir ");
  printfloat3(dir);
  pos = add(pos, mul(dir, 1.f));
  HIGAN_LOGi("pos ");
  printfloat3(pos);
  REQUIRE(pos.z > 0);

  pos = add(pos, mul(sideVec, 1.f));
  HIGAN_LOGi("pos ");
  printfloat3(pos);
  REQUIRE(pos.x > 0);

  pos = add(pos, mul(updir, 1.f));
  HIGAN_LOGi("pos ");
  printfloat3(pos);
  REQUIRE(pos.y > 0);

  float4x4 rotmat = math::rotationMatrixLH(direction);
  HIGAN_LOGi("\nmatrix\n");
  printfloat4(rotmat.row(0));
  printfloat4(rotmat.row(1));
  printfloat4(rotmat.row(2));
  printfloat4(rotmat.row(3));

  float4 pos2{};

  pos2 = mul(float4(0,0,1,1), rotmat).row(0);
  HIGAN_LOGi("pos2 ");
  printfloat4(pos2);

  {
    float4x4 pers = math::perspectiveLH(90, 1, 0.1, 100);
    HIGAN_LOGi("\nperspective LH\n");
    printfloat4(pers.row(0));
    printfloat4(pers.row(1));
    printfloat4(pers.row(2));
    printfloat4(pers.row(3));

    pos2 = {};
    pos2 = mul(float4(0,0,1,1), pers).row(0);
    HIGAN_LOGi("pos2 ");
    printfloat4(pos2);
  }

  {
    float4x4 pers = math::perspectiveLHInverseZ(90, 1, 0.1, 100);
    HIGAN_LOGi("\nperspective inversez\n");
    printfloat4(pers.row(0));
    printfloat4(pers.row(1));
    printfloat4(pers.row(2));
    printfloat4(pers.row(3));

    pos2 = {};
    pos2 = mul(float4(0,0,1,1), pers).row(0);
    HIGAN_LOGi("pos2 ");
    printfloat4(pos2);
  }

  {
    float4x4 pers = math::perspectiveLHInverseInfZ(90, 1, 0.1);
    HIGAN_LOGi("\nperspective inverse infinite Z\n");
    printfloat4(pers.row(0));
    printfloat4(pers.row(1));
    printfloat4(pers.row(2));
    printfloat4(pers.row(3));

    pos2 = {};
    pos2 = mul(float4(0,0,1,1), pers).row(0);
    HIGAN_LOGi("pos2 ");
    printfloat4(pos2);
  }
}