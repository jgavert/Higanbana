#include "core/src/math/math.hpp"

#include "gtest/gtest.h"

using namespace faze::math;

TEST(Vector, Constructor)
{
  int4 v{ 0, 1, 2, 3 };
  EXPECT_EQ(0, v.x);
  EXPECT_EQ(1, v.y);
  EXPECT_EQ(2, v.z);
  EXPECT_EQ(3, v.w);
  for (int i = 0; i < 4; i++)
  {
    EXPECT_EQ(i, v(i));
  }
}

TEST(Vector, Length)
{
  int2 v{ 6, 8 };
  EXPECT_EQ(10, length(v));
}

TEST(Vector, Normalize)
{
  double2 v{ 6.0, 8.0 };
  auto vn = normalize(v);
  EXPECT_DOUBLE_EQ(0.6, vn.x);
  EXPECT_DOUBLE_EQ(0.8, vn.y);
}

TEST(Vector, Dot)
{
  double3 v1(1.0, 3.0, -5.0);
  double3 v2(4.0, -2.0, -1.0);
  auto scalar = dot(v1, v2);
  EXPECT_DOUBLE_EQ(3.0, scalar);
}

TEST(Vector, MulVecVec)
{
  double3 v1(1.0, 3.0, -5.0);
  double3 v2(4.0, -2.0, -1.0);
  auto v3 = mul(v1, v2);
  EXPECT_DOUBLE_EQ(4.0, v3.x);
  EXPECT_DOUBLE_EQ(-6.0, v3.y);
  EXPECT_DOUBLE_EQ(5.0, v3.z);
}

TEST(Vector, MulVecScalar)
{
  double3 v1(1.0, 3.0, -5.0);
  auto v2 = mul(v1, -1.0);
  auto v3 = mul(-1.0, v1);
  EXPECT_DOUBLE_EQ(-1.0, v2.x);
  EXPECT_DOUBLE_EQ(-3.0, v2.y);
  EXPECT_DOUBLE_EQ(5.0, v2.z);
  for (int i = 0; i < 3; ++i)
  {
    EXPECT_DOUBLE_EQ(v2(i), v3(i));
  }
}
TEST(Vector, DivVecVec)
{
  double3 v1(1.0, 3.0, -5.0);
  double3 v2(4.0, -2.0, -1.0);
  auto v3 = div(v1, v2);
  EXPECT_DOUBLE_EQ(0.25, v3.x);
  EXPECT_DOUBLE_EQ(-1.5, v3.y);
  EXPECT_DOUBLE_EQ(5.0, v3.z);
}

TEST(Vector, DivVecScalar)
{
  double3 v1(1.0, 3.0, -5.0);
  auto v2 = div(v1, 4.0);
  EXPECT_DOUBLE_EQ(0.25, v2.x);
  EXPECT_DOUBLE_EQ(0.75, v2.y);
  EXPECT_DOUBLE_EQ(-1.25, v2.z);
}

TEST(Vector, AddVecVec)
{
  double3 v1(1.0, 3.0, -5.0);
  double3 v2(4.0, -2.0, -1.0);
  auto v3 = add(v1, v2);
  EXPECT_DOUBLE_EQ(5.0, v3.x);
  EXPECT_DOUBLE_EQ(1.0, v3.y);
  EXPECT_DOUBLE_EQ(-6.0, v3.z);
}

TEST(Vector, AddVecScalar)
{
  double3 v1(1.0, 3.0, -5.0);
  auto v2 = add(v1, 1.0);
  auto v3 = add(1.0, v1);
  EXPECT_DOUBLE_EQ(2.0, v2.x);
  EXPECT_DOUBLE_EQ(4.0, v2.y);
  EXPECT_DOUBLE_EQ(-4.0, v2.z);
  for (int i = 0; i < 3; ++i)
  {
    EXPECT_DOUBLE_EQ(v2(i), v3(i));
  }
}

TEST(Vector, SubVecVec)
{
  double3 v1(1.0, 3.0, -5.0);
  double3 v2(4.0, -2.0, -1.0);
  auto v3 = sub(v1, v2);
  EXPECT_DOUBLE_EQ(-3.0, v3.x);
  EXPECT_DOUBLE_EQ(5.0, v3.y);
  EXPECT_DOUBLE_EQ(-4.0, v3.z);
}

TEST(Vector, SubVecScalar)
{
  double3 v1(1.0, 3.0, -5.0);
  auto v2 = sub(v1, 1.0);
  EXPECT_DOUBLE_EQ(0.0, v2.x);
  EXPECT_DOUBLE_EQ(2.0, v2.y);
  EXPECT_DOUBLE_EQ(-6.0, v2.z);
  auto v3 = sub(1.0, v1);
  EXPECT_DOUBLE_EQ(0.0, v3.x);
  EXPECT_DOUBLE_EQ(-2.0, v3.y);
  EXPECT_DOUBLE_EQ(6.0, v3.z);
}

TEST(Vector, CrossProduct)
{
  double3 v1(3, -3, 1);
  double3 v2(4, 9, 2);

  auto v3 = crossProduct(v1, v2);

  EXPECT_DOUBLE_EQ(-15, v3.x);
  EXPECT_DOUBLE_EQ(-2, v3.y);
  EXPECT_DOUBLE_EQ(39, v3.z);
}

TEST(Quaternion, Construct)
{
  Quaternion q1{ 1.f, 2.f, 3.f, 4.f };

  EXPECT_FLOAT_EQ(1.f, q1.w);
  EXPECT_FLOAT_EQ(2.f, q1.x);
  EXPECT_FLOAT_EQ(3.f, q1.y);
  EXPECT_FLOAT_EQ(4.f, q1.z);
}

TEST(Quaternion, Multiply)
{
  Quaternion q1{ 1.f, 2.f, 3.f, 4.f };
  Quaternion q2{ 5.f, 6.f, 7.f, 8.f };

  auto q3 = mul(q1, q2);

  EXPECT_FLOAT_EQ(-60.f, q3.w);
  EXPECT_FLOAT_EQ(12.f, q3.x);
  EXPECT_FLOAT_EQ(30.f, q3.y);
  EXPECT_FLOAT_EQ(24.f, q3.z);
}

TEST(Quaternion, rotationQuaternion)
{
  Quaternion q1 = rotationQuaternionSlow(1.f, 0.f, 0.f);
  Quaternion q2 = rotationQuaternionEuler(1.f, 0.f, 0.f);

  EXPECT_FLOAT_EQ(q1.w, q2.w);
  EXPECT_FLOAT_EQ(q1.x, q2.x);
  EXPECT_FLOAT_EQ(q1.y, q2.y);
  EXPECT_FLOAT_EQ(q1.z, q2.z);
}

TEST(Quaternion, rotationQuaternion2)
{
  Quaternion q1 = rotationQuaternionSlow(0.f, 1.f, 0.f);
  Quaternion q2 = rotationQuaternionEuler(0.f, 1.f, 0.f);

  EXPECT_FLOAT_EQ(q1.w, q2.w);
  EXPECT_FLOAT_EQ(q1.x, q2.x);
  EXPECT_FLOAT_EQ(q1.y, q2.y);
  EXPECT_FLOAT_EQ(q1.z, q2.z);
}

TEST(Quaternion, rotationQuaternion3)
{
  Quaternion q1 = rotationQuaternionSlow(0.f, 0.f, 1.f);
  Quaternion q2 = rotationQuaternionEuler(0.f, 0.f, 1.f);

  EXPECT_FLOAT_EQ(q1.w, q2.w);
  EXPECT_FLOAT_EQ(q1.x, q2.x);
  EXPECT_FLOAT_EQ(q1.y, q2.y);
  EXPECT_FLOAT_EQ(q1.z, q2.z);
}

TEST(Quaternion, rotationQuaternion4)
{
  Quaternion q1 = rotationQuaternionSlow(0.5f, 0.5f, 1.f);
  Quaternion q2 = rotationQuaternionEuler(0.5f, 0.5f, 1.f);

  printf("%s\n", toString(q1).c_str());
  printf("%s\n", toString(q2).c_str());

  EXPECT_NEAR(q1.w, q2.w, 0.2f);
  EXPECT_NEAR(q1.x, q2.x, 0.2f);
  EXPECT_NEAR(q1.y, q2.y, 0.2f);
  EXPECT_NEAR(q1.z, q2.z, 0.2f);
  // does the function results really differ so much!?
  // Quaternion quality???
}

TEST(Matrix, construction)
{
  Matrix<2, 2, double> m1{ 0, 1, 2, 3 };

  EXPECT_DOUBLE_EQ(0, m1(0, 0));
  EXPECT_DOUBLE_EQ(1, m1(1, 0));
  EXPECT_DOUBLE_EQ(2, m1(0, 1));
  EXPECT_DOUBLE_EQ(3, m1(1, 1));
  EXPECT_DOUBLE_EQ(0, m1(0));
  EXPECT_DOUBLE_EQ(1, m1(1));
  EXPECT_DOUBLE_EQ(2, m1(2));
  EXPECT_DOUBLE_EQ(3, m1(3));
}

TEST(Matrix, construction2)
{
  Matrix<2, 3, double> m1{ 1, 2, 3, 4, 5, 6 };

  EXPECT_DOUBLE_EQ(1, m1(0, 0));
  EXPECT_DOUBLE_EQ(2, m1(1, 0));
  EXPECT_DOUBLE_EQ(3, m1(2, 0));
  EXPECT_DOUBLE_EQ(4, m1(0, 1));
  EXPECT_DOUBLE_EQ(5, m1(1, 1));
  EXPECT_DOUBLE_EQ(6, m1(2, 1));
}

TEST(Matrix, construction3)
{
  Matrix<3, 2, double> m1{ 1, 2, 3, 4, 5, 6 };

  EXPECT_DOUBLE_EQ(1, m1(0, 0));
  EXPECT_DOUBLE_EQ(2, m1(1, 0));
  EXPECT_DOUBLE_EQ(3, m1(0, 1));
  EXPECT_DOUBLE_EQ(4, m1(1, 1));
  EXPECT_DOUBLE_EQ(5, m1(0, 2));
  EXPECT_DOUBLE_EQ(6, m1(1, 2));
}

TEST(Matrix, identity)
{
  auto m1 = Matrix<2, 2, double>::identity();

  EXPECT_DOUBLE_EQ(1, m1(0, 0));
  EXPECT_DOUBLE_EQ(0, m1(1, 0));
  EXPECT_DOUBLE_EQ(0, m1(0, 1));
  EXPECT_DOUBLE_EQ(1, m1(1, 1));
  EXPECT_DOUBLE_EQ(1, m1(0));
  EXPECT_DOUBLE_EQ(0, m1(1));
  EXPECT_DOUBLE_EQ(0, m1(2));
  EXPECT_DOUBLE_EQ(1, m1(3));
}

TEST(Matrix, mulIdentity)
{
  auto m1 = Matrix<2, 2, double>::identity();
  auto m2 = Matrix<2, 2, double>::identity();
  m2(0, 1) = 5.0;

  auto m3 = mul(m1, m2);
  EXPECT_DOUBLE_EQ(1, m3(0, 0));
  EXPECT_DOUBLE_EQ(0, m3(1, 0));
  EXPECT_DOUBLE_EQ(5, m3(0, 1));
  EXPECT_DOUBLE_EQ(1, m3(1, 1));
  EXPECT_DOUBLE_EQ(1, m3(0));
  EXPECT_DOUBLE_EQ(0, m3(1));
  EXPECT_DOUBLE_EQ(5, m3(2));
  EXPECT_DOUBLE_EQ(1, m3(3));
}

TEST(Matrix, mul)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = mul(m1, m1);
  EXPECT_DOUBLE_EQ(30, m2(0, 0));
  EXPECT_DOUBLE_EQ(36, m2(1, 0));
  EXPECT_DOUBLE_EQ(42, m2(2, 0));
  EXPECT_DOUBLE_EQ(66, m2(0, 1));
  EXPECT_DOUBLE_EQ(81, m2(1, 1));
  EXPECT_DOUBLE_EQ(96, m2(2, 1));
  EXPECT_DOUBLE_EQ(102, m2(0, 2));
  EXPECT_DOUBLE_EQ(126, m2(1, 2));
  EXPECT_DOUBLE_EQ(150, m2(2, 2));
}

TEST(Matrix, mul2)
{
  auto m1 = Matrix<2, 3, double>{ 1,2,3,4,5,6 };
  auto m2 = Matrix<3, 2, double>{ 1,2,3,4,5,6 };
  Matrix<2, 2, double> m3 = mul(m1, m2);
  EXPECT_DOUBLE_EQ(22, m3(0, 0));
  EXPECT_DOUBLE_EQ(28, m3(1, 0));
  EXPECT_DOUBLE_EQ(49, m3(0, 1));
  EXPECT_DOUBLE_EQ(64, m3(1, 1));
}

TEST(Matrix, mulScalar)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = mul(m1, 5.0);
  auto m3 = mul(5.0, m1);
  EXPECT_DOUBLE_EQ(5, m2(0, 0));
  EXPECT_DOUBLE_EQ(10, m2(1, 0));
  EXPECT_DOUBLE_EQ(15, m2(2, 0));
  EXPECT_DOUBLE_EQ(20, m2(0, 1));
  EXPECT_DOUBLE_EQ(25, m2(1, 1));
  EXPECT_DOUBLE_EQ(30, m2(2, 1));
  EXPECT_DOUBLE_EQ(35, m2(0, 2));
  EXPECT_DOUBLE_EQ(40, m2(1, 2));
  EXPECT_DOUBLE_EQ(45, m2(2, 2));

  for (int i = 0; i < 9; ++i)
  {
    EXPECT_DOUBLE_EQ(m3(i), m2(i));
  }
}

TEST(Matrix, mulVec)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  //printf("%s\n", toString(m1).c_str());
  Vector<3, double> v1 = { 1,2,3 };
  //printf("%s\n", toString(v1).c_str());
  Matrix<1, 3, double> m2 = mul(v1, m1);
  //printf("%s\n", toString(m2).c_str());
  EXPECT_DOUBLE_EQ(30, m2(0, 0));
  EXPECT_DOUBLE_EQ(36, m2(1, 0));
  EXPECT_DOUBLE_EQ(42, m2(2, 0));

  auto v2 = m2.row(0);
  EXPECT_DOUBLE_EQ(30, v2(0));
  EXPECT_DOUBLE_EQ(36, v2(1));
  EXPECT_DOUBLE_EQ(42, v2(2));
}

TEST(Matrix, mulVec2)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  //printf("%s\n", toString(m1).c_str());
  Vector<3, double> v1 = { 1,2,3 };
  //printf("%s\n", toString(v1).c_str());
  Matrix<1, 3, double> m2 = transpose(mul(m1, v1));
  //printf("%s\n", toString(m2).c_str());
  EXPECT_DOUBLE_EQ(14, m2(0, 0));
  EXPECT_DOUBLE_EQ(32, m2(1, 0));
  EXPECT_DOUBLE_EQ(50, m2(2, 0));

  auto v2 = m2.row(0);
  EXPECT_DOUBLE_EQ(14, v2(0));
  EXPECT_DOUBLE_EQ(32, v2(1));
  EXPECT_DOUBLE_EQ(50, v2(2));
}

TEST(Matrix, divScalar)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = div(m1, 2.0);
  EXPECT_DOUBLE_EQ(0.5, m2(0, 0));
  EXPECT_DOUBLE_EQ(1, m2(1, 0));
  EXPECT_DOUBLE_EQ(1.5, m2(2, 0));
  EXPECT_DOUBLE_EQ(2, m2(0, 1));
  EXPECT_DOUBLE_EQ(2.5, m2(1, 1));
  EXPECT_DOUBLE_EQ(3, m2(2, 1));
  EXPECT_DOUBLE_EQ(3.5, m2(0, 2));
  EXPECT_DOUBLE_EQ(4, m2(1, 2));
  EXPECT_DOUBLE_EQ(4.5, m2(2, 2));
}

TEST(Matrix, add)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = add(m1, m1);
  EXPECT_DOUBLE_EQ(2, m2(0, 0));
  EXPECT_DOUBLE_EQ(4, m2(1, 0));
  EXPECT_DOUBLE_EQ(6, m2(2, 0));
  EXPECT_DOUBLE_EQ(8, m2(0, 1));
  EXPECT_DOUBLE_EQ(10, m2(1, 1));
  EXPECT_DOUBLE_EQ(12, m2(2, 1));
  EXPECT_DOUBLE_EQ(14, m2(0, 2));
  EXPECT_DOUBLE_EQ(16, m2(1, 2));
  EXPECT_DOUBLE_EQ(18, m2(2, 2));
}
TEST(Matrix, addScalar)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = add(m1, 5.0);
  auto m3 = add(5.0, m1);
  EXPECT_DOUBLE_EQ(6, m2(0, 0));
  EXPECT_DOUBLE_EQ(7, m2(1, 0));
  EXPECT_DOUBLE_EQ(8, m2(2, 0));
  EXPECT_DOUBLE_EQ(9, m2(0, 1));
  EXPECT_DOUBLE_EQ(10, m2(1, 1));
  EXPECT_DOUBLE_EQ(11, m2(2, 1));
  EXPECT_DOUBLE_EQ(12, m2(0, 2));
  EXPECT_DOUBLE_EQ(13, m2(1, 2));
  EXPECT_DOUBLE_EQ(14, m2(2, 2));

  for (int i = 0; i < 9; ++i)
  {
    EXPECT_DOUBLE_EQ(m3(i), m2(i));
  }
}

TEST(Matrix, sub)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = sub(m1, m1);
  EXPECT_DOUBLE_EQ(0, m2(0, 0));
  EXPECT_DOUBLE_EQ(0, m2(1, 0));
  EXPECT_DOUBLE_EQ(0, m2(2, 0));
  EXPECT_DOUBLE_EQ(0, m2(0, 1));
  EXPECT_DOUBLE_EQ(0, m2(1, 1));
  EXPECT_DOUBLE_EQ(0, m2(2, 1));
  EXPECT_DOUBLE_EQ(0, m2(0, 2));
  EXPECT_DOUBLE_EQ(0, m2(1, 2));
  EXPECT_DOUBLE_EQ(0, m2(2, 2));
}
TEST(Matrix, subScalar)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = sub(m1, 5.0);
  auto m3 = sub(5.0, m1);
  EXPECT_DOUBLE_EQ(-4, m2(0, 0));
  EXPECT_DOUBLE_EQ(-3, m2(1, 0));
  EXPECT_DOUBLE_EQ(-2, m2(2, 0));
  EXPECT_DOUBLE_EQ(-1, m2(0, 1));
  EXPECT_DOUBLE_EQ(0, m2(1, 1));
  EXPECT_DOUBLE_EQ(1, m2(2, 1));
  EXPECT_DOUBLE_EQ(2, m2(0, 2));
  EXPECT_DOUBLE_EQ(3, m2(1, 2));
  EXPECT_DOUBLE_EQ(4, m2(2, 2));

  for (int i = 0; i < 9; ++i)
  {
    EXPECT_DOUBLE_EQ(m3(i), m2(i));
  }
}
TEST(Matrix, concatenate)
{
  auto m1 = Matrix<2, 2, double>{ 1,2,3,4 };
  auto m2 = Matrix<1, 3, double>{ 1,2,3 };
  Matrix<1, 7, double> m3 = concatenateToSingleDimension(m1, m2);

  for (int i = 0; i < 4; ++i)
    EXPECT_DOUBLE_EQ(m3(i), m1(i));
  for (int i = 0; i < 3; ++i)
    EXPECT_DOUBLE_EQ(m3(i + 4), m2(i));
}

TEST(Matrix, ElementWiseMultiplication)
{
  auto m1 = Matrix<3, 3, double>{ 1,2,3,4,5,6,7,8,9 };
  auto m2 = Matrix<3, 3, double>{ 1, -1, 0, -1, 0, 1, 0, 1, -1 };
  auto m3 = mulElementWise(m1, m2);
  EXPECT_DOUBLE_EQ(1, m3(0, 0));
  EXPECT_DOUBLE_EQ(-2, m3(1, 0));
  EXPECT_DOUBLE_EQ(0, m3(2, 0));
  EXPECT_DOUBLE_EQ(-4, m3(0, 1));
  EXPECT_DOUBLE_EQ(0, m3(1, 1));
  EXPECT_DOUBLE_EQ(6, m3(2, 1));
  EXPECT_DOUBLE_EQ(0, m3(0, 2));
  EXPECT_DOUBLE_EQ(8, m3(1, 2));
  EXPECT_DOUBLE_EQ(-9, m3(2, 2));
}

TEST(Matrix, Transpose)
{
  auto m1 = Matrix<1, 3, double>{ 1,2,3 };
  Matrix<3, 1, double> m2 = transpose(m1);
  EXPECT_DOUBLE_EQ(m1(0, 0), m2(0, 0));
  EXPECT_DOUBLE_EQ(m1(1, 0), m2(0, 1));
  EXPECT_DOUBLE_EQ(m1(2, 0), m2(0, 2));
}

TEST(Matrix, Transform)
{
  auto m1 = Matrix<1, 3, double>{ 1,2,3 };
  auto m2 = transform(m1, [](double v) { return -1.0; });
  EXPECT_DOUBLE_EQ(-1, m2(0, 0));
  EXPECT_DOUBLE_EQ(-1, m2(1, 0));
  EXPECT_DOUBLE_EQ(-1, m2(2, 0));
}

TEST(Matrix, Sum)
{
  auto m1 = Matrix<1, 3, double>{ 1,2,3 };
  auto val = sum(m1);
  EXPECT_DOUBLE_EQ(6, val);
}

// float4x4 translation(x, y, z or float3 or float4)
TEST(SpecialMatrix, Translation)
{
  float4 position(0.f, 0.f, 0.f, 1.f);
  float4x4 m = translation(1.f, 2.f, 3.f);
  auto res = mul(position, m).row(0);
  EXPECT_FLOAT_EQ(1.f, res(0));
  EXPECT_FLOAT_EQ(2.f, res(1));
  EXPECT_FLOAT_EQ(3.f, res(2));
  EXPECT_FLOAT_EQ(1.f, res(3));
}

// float4x4 perspective(fov, aspect, NearZ, FarZ)
TEST(SpecialMatrix, Perspective)
{
  auto pm = perspectivelh(90.f, 1920.f / 1080.f, 0.1f, 100.f);

  EXPECT_FLOAT_EQ(1.777778f, pm(0, 0));
  EXPECT_FLOAT_EQ(0.f, pm(1, 0));
  EXPECT_FLOAT_EQ(0.f, pm(2, 0));
  EXPECT_FLOAT_EQ(0.f, pm(3, 0));
  EXPECT_FLOAT_EQ(0.f, pm(0, 1));
  EXPECT_FLOAT_EQ(0.f, pm(0, 2));
  EXPECT_FLOAT_EQ(0.f, pm(0, 3));

  EXPECT_FLOAT_EQ(1.f, pm(1, 1));
  EXPECT_FLOAT_EQ(0.f, pm(2, 1));
  EXPECT_FLOAT_EQ(0.f, pm(3, 1));
  EXPECT_FLOAT_EQ(0.f, pm(1, 2));
  EXPECT_FLOAT_EQ(0.f, pm(1, 3));

  EXPECT_FLOAT_EQ(1.001001f, pm(2, 2));
  EXPECT_FLOAT_EQ(1.f, pm(3, 2));
  EXPECT_FLOAT_EQ(-0.1001001f, pm(2, 3));
  EXPECT_FLOAT_EQ(0.f, pm(3, 3));

  auto pmr = perspectiverh(90.f, 1920.f / 1080.f, 0.1f, 100.f);
  EXPECT_FLOAT_EQ(1.777778f, pmr(0, 0));
  EXPECT_FLOAT_EQ(0.f, pmr(1, 0));
  EXPECT_FLOAT_EQ(0.f, pmr(2, 0));
  EXPECT_FLOAT_EQ(0.f, pmr(3, 0));
  EXPECT_FLOAT_EQ(0.f, pmr(0, 1));
  EXPECT_FLOAT_EQ(0.f, pmr(0, 2));
  EXPECT_FLOAT_EQ(0.f, pmr(0, 3));

  EXPECT_FLOAT_EQ(1.f, pmr(1, 1));
  EXPECT_FLOAT_EQ(0.f, pmr(2, 1));
  EXPECT_FLOAT_EQ(0.f, pmr(3, 1));
  EXPECT_FLOAT_EQ(0.f, pmr(1, 2));
  EXPECT_FLOAT_EQ(0.f, pmr(1, 3));

  EXPECT_FLOAT_EQ(1.001001f, pmr(2, 2));
  EXPECT_FLOAT_EQ(-1.f, pmr(3, 2));
  EXPECT_FLOAT_EQ(0.1001001f, pmr(2, 3));
  EXPECT_FLOAT_EQ(0.f, pmr(3, 3));
}

// float4x4 lookAt(cameraPos, cameraDir)
// kind of tested by RotationMatrix, which can be made using this also.
TEST(SpecialMatrix, LookAt)
{
  float4 pos(1.f, 2.f, 3.f, 1.f);
  float4 target(0.f, 0.f, 0.f, 1.f);
  auto m = lookAt(pos, target);
  auto objectPoint = float4(0.f, 0.1f, 0.1f, 1.f);

  auto r = mul(objectPoint, m);
  printf("%s\n", toString(r).c_str());
  // not sure how to test.
}

// float4x4 rotationMatrix(x, y, z or quaternion)
TEST(SpecialMatrix, RotationMatrix)
{
  auto rm = rotationMatrixRH(0, 90.f*PI / 180.f, 0.f);
  //printf("%s\n\n", toString(rm).c_str());
  auto rm2 = rotationMatrixRH(rotationQuaternionSlow(90.f*PI / 180.f, 0, 0));
  //printf("%s\n", toString(rm2).c_str());

  for (int i = 0; i < 4 * 4; ++i)
  {
    //printf("%d\n", i);
    EXPECT_NEAR(rm(i), rm2(i), 0.000001f);
  }

  // fake rotation matrix
  float4 pos(0.f, 0.f, 0.f, 1.f);
  float4 target(1.f, 0.f, 0.f, 1.f);

  auto rm3 = lookAt(pos, target);

  for (int i = 0; i < 4 * 4; ++i)
  {
    //printf("%d\n", i);
    EXPECT_NEAR(rm(i), rm3(i), 0.000001f);
  }
}