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

TEST_CASE( "right hand perspective matrix" ) {
  float4x4 rh = math::perspectiveRH(60, 1, 0.1, 100);
  float4x4 rhInvZ = math::perspectiveRHInverseZ(60, 1, 0.1, 100);
  float4x4 rhInvZInf = math::perspectiveRHInverseInfZ(60, 1, 0.1);
  float4x4 lh = math::perspectiveLH(60, 1, 0.1, 100);
  float4x4 lhInvZ = math::perspectiveLHInverseZ(60, 1, 0.1, 100);
  float4x4 lhInvZInf = math::perspectiveLHInverseInfZ(60, 1, 0.1);

  float4x4 hardcoded = { 1, 0, 0, 0,
                         0, 1, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1};

  float4 pos = float4(1, 1, 1, 1);
  float4 outputRH = mul(pos, rh).row(0);
  float4 outputRHInverseZ = mul(pos, rhInvZ).row(0);
  float4 outputRHInverseInfZ = mul(pos, rhInvZInf).row(0);
  float4 outputLH = mul(pos, lh).row(0);
  float4 outputLHInverseZ = mul(pos, lhInvZ).row(0);
  float4 outputLHInverseInfZ = mul(pos, lhInvZInf).row(0);
  float4 outputcustom = mul(pos, hardcoded).row(0);
  HIGAN_LOGi("\nPerspective matrix behavior inspection\n");
  HIGAN_LOGi("RH output             ");
  printfloat4(outputRH);
  HIGAN_LOGi("RH InverseZ output    ");
  printfloat4(outputRHInverseZ);
  HIGAN_LOGi("RH InverseInfZ output ");
  printfloat4(outputRHInverseInfZ);
  HIGAN_LOGi("LH output             ");
  printfloat4(outputLH);
  HIGAN_LOGi("LH InverseZ output    ");
  printfloat4(outputLHInverseZ);
  HIGAN_LOGi("LH InverseInfZ output ");
  printfloat4(outputLHInverseInfZ);

  HIGAN_LOGi("custom output         ");
  printfloat4(outputcustom);
}