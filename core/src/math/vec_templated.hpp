#pragma once
#include "../tests/TestWorks.hpp"

#include <type_traits>
#include <array>
#include <cmath>


namespace faze
{

  const float PI = 3.14159265f;

  template <int NUM_ELEMENTS, typename VectorType, bool Enabled = true>
  struct Vector
  {
    /*
    Vector() : data({ 0 }){}
    Vector(const Vector& vec) : data(vec.data){}

    template<typename type = VectorType>
    Vector(const std::initializer_list<type> &init_list) : data(init_list) {}
    */
    std::array<VectorType, NUM_ELEMENTS> data;

    inline Vector normalize()
    {
      VectorType amount = 0;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        amount += data[i] * data[i];
      }
      VectorType length = std::sqrt(amount);
      Vector result;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        result.data[i] = data[i] / length;
      }
      return result;
    }

    inline VectorType dot(Vector vec)
    {
      VectorType result = 0;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        result += data[i] * vec.data[i];
      }
      return result;
    }

    inline Vector operator*(VectorType scalar)
    {
      Vector vec;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        vec.data[i] = data[i] * scalar;
      }
      return vec;
    }

    inline Vector operator/(VectorType scalar)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] / scalar;
      }
      return v2;
    }
    inline Vector operator+(VectorType scalar)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] + scalar;
      }
      return v2;
    }
    inline Vector operator-(VectorType scalar)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] - scalar;
      }
      return v2;
    }
    inline Vector operator*(Vector vec)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] * vec.data[i];
      }
      return v2;
    }
    inline Vector operator/(Vector vec)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] / vec.data[i];
      }
      return v2;
    }
    inline Vector operator+(Vector vec)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] + vec.data[i];
      }
      return v2;
    }
    inline Vector operator-(Vector vec)
    {
      Vector v2;
      for (int i = 0; i < NUM_ELEMENTS; i++)
      {
        v2.data[i] = data[i] - vec.data[i];
      }
      return v2;
    }

    VectorType& operator[](int i)
    {
      return data[i];
    }

    // SPECIAL FUNCTIONS
    template<int numElem = NUM_ELEMENTS>
    inline Vector CrossProduct(Vector v2, typename std::enable_if<(numElem == 3 || numElem == 4), int>::type* = nullptr)
    {
      Vector res;
      res.data[0] = data[1] * v2.data[2] - data[2] * v2.data[1];
      res.data[1] = data[2] * v2.data[0] - data[0] * v2.data[2];
      res.data[2] = data[0] * v2.data[1] - data[1] * v2.data[0];
      return res;
    }

    // SPECIAL ACCESSORS
    template<int numElem = NUM_ELEMENTS>
    VectorType x(typename std::enable_if<(numElem >= 1), int>::type* = nullptr) const {
      return data[0];
    }
    template<int numElem = NUM_ELEMENTS>
    VectorType r(typename std::enable_if<(numElem >= 1), int>::type* = nullptr) const {
      return data[0];
    }

    template<int numElem = NUM_ELEMENTS>
    VectorType y(typename std::enable_if<(numElem >= 2), int>::type* = nullptr) const {
      return data[1];
    }
    template<int numElem = NUM_ELEMENTS>
    VectorType g(typename std::enable_if<(numElem >= 2), int>::type* = nullptr) const {
      return data[1];
    }

    template<int numElem = NUM_ELEMENTS>
    Vector<2, VectorType> xy(typename std::enable_if<(numElem >= 2), int>::type* = nullptr) const {
      return Vector<2, VectorType>(data[0], data[1]);
    }

    template<int numElem = NUM_ELEMENTS>
    VectorType z(typename std::enable_if<(numElem >= 3), int>::type* = nullptr) const {
      return data[2];
    }
    template<int numElem = NUM_ELEMENTS>
    VectorType b(typename std::enable_if<(numElem >= 3), int>::type* = nullptr) const {
      return data[2];
    }

    template<int numElem = NUM_ELEMENTS>
    Vector<3, VectorType> xyz(typename std::enable_if<(numElem >= 3), int>::type* = nullptr) const {
      return Vector<3, VectorType>(data[0], data[1], data[2]);
    }
    template<int numElem = NUM_ELEMENTS>
    Vector<3, VectorType> rgb(typename std::enable_if<(numElem >= 3), int>::type* = nullptr) const {
      return Vector<3, VectorType>(data[0], data[1], data[2]);
    }

    template<int numElem = NUM_ELEMENTS>
    VectorType w(typename std::enable_if<(numElem >= 4), int>::type* = nullptr) const {
      return data[3];
    }
    template<int numElem = NUM_ELEMENTS>
    VectorType a(typename std::enable_if<(numElem >= 4), int>::type* = nullptr) const {
      return data[3];
    }

    template<int numElem = NUM_ELEMENTS>
    Vector<3, VectorType> xyzw(typename std::enable_if<(numElem >= 4), int>::type* = nullptr) const {
      return Vector<3, VectorType>(data[0], data[1], data[2], data[3]);
    }
    template<int numElem = NUM_ELEMENTS>
    Vector<3, VectorType> rgba(typename std::enable_if<(numElem >= 4), int>::type* = nullptr) const {
      return Vector<3, VectorType>(data[0], data[1], data[2], data[3]);
    }
  };



  typedef Vector<2, float> vec2;
  typedef Vector<3, float> vec3;
  typedef Vector<4, float> vec4;

  typedef Vector<2, int> ivec2;
  typedef Vector<3, int> ivec3;
  typedef Vector<4, int> ivec4;

  struct Quaternion
  {
    Quaternion() : data({ 1.f, 0.f, 0.f, 0.f }) { }

    Vector<4, float> data;

    // RotateAxis(1,0,0,amount) -> this would rotate x axis... someway :D?
    inline Quaternion RotateAxis(float x, float y, float z, float amount)
    {
      Quaternion local;
      local[0] = cos(amount / 2.f);
      local[1] = sin(amount / 2.f)*x;
      local[2] = sin(amount / 2.f)*y;
      local[3] = sin(amount / 2.f)*z;
      return local;
    }

    // Use this to do all rotations at once (just does many times the RotateAxis)
    inline Quaternion Rotation(float x, float y, float z)
    {
      Quaternion total;
      total = RotateAxis(1.f, 0.f, 0.f, x) * total;
      total = RotateAxis(0.f, 1.f, 0.f, y) * total;
      total = RotateAxis(0.f, 0.f, 1.f, z) * total;
      return total;
    }
    // operations
    inline Quaternion Normalize(Quaternion& quat2)
    {
      Quaternion ret;
      float magnitude = std::sqrt(std::pow(quat2[0], 2.0f) + std::pow(quat2[1], 2.0f) + std::pow(quat2[2], 2.0f) + std::pow(quat2[3], 2.0f));
      for (int i = 0; i < 4; i++)
      {
        ret[i] = quat2[i] / magnitude;
      }
      return ret;
    }
    inline Quaternion operator*(Quaternion& quat2)
    {
      Quaternion retur;
      retur[0] = data[0] * quat2[0];
      for (int i = 1; i < 4; i++)
      {
        retur[0] -= data[i] * quat2[i];
      }
      retur[1] = data[0] * quat2[1] + data[1] * quat2[0] + data[2] * quat2[3] - data[3] * quat2[2];
      retur[2] = data[0] * quat2[2] - data[1] * quat2[3] + data[2] * quat2[0] + data[3] * quat2[1];
      retur[3] = data[0] * quat2[3] + data[1] * quat2[2] - data[2] * quat2[1] + data[3] * quat2[0];
      return retur;
    }

    float& operator[](int i)
    {
      return data[i];
    }

  };


  class vecTemplateTests
  {
  public:
    void updateTests()
    {
      TestWorks tests("Templated vectors");
      tests.addTest("simple", []()
      {
        ivec2 woot = { 1, 2 };
        return ((woot.y() - woot.x()) == 1);
      });
      tests.addTest("simple 2", []()
      {
        Vector<5, int> woot = { 1, 2, 3, 4, 5 };
        return ((woot[1] - woot[0]) == 1);
      });
      tests.addTest("accessing", []()
      {
        Vector<3, float> woot = { 1.f, 2.f, 3.f };
        return ((woot[1] - woot[0]) == 1.f);
      });
      tests.addTest("instatiating with itself", []()
      {
        vec3 woot = { 1.f, 2.f, 3.f };
        vec3 woot2 = { woot.data };
        return ((woot2[0] - woot[0]) == 0.f);
      });
      tests.addTest("instatiating with itself 2", []()
      {
        vec3 woot = { 1.f, 2.f, 3.f };
        vec3 woot2 = woot;
        return ((woot2[0] - woot[0]) == 0.f);
      });
      tests.runTests();
    }
  };

}
