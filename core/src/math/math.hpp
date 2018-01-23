#pragma once

#include <cstring>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cmath>

#if defined(FAZE_PLATFORM_WINDOWS)
#pragma warning( push )
#pragma warning( disable : 4201 ) // unreferenced local function has been removed
#endif

namespace faze
{
  namespace math
  {
    /*  operations available
    *  Vector:
    *    string  toString(vec)
    *    scalar  length(vec)
    *    vec     normalize(vec)
    *    scalar  dot(vec, vec)
    *    vec     mul(vec, vec), mul(vec, scalar)
    *    vec     div(vec, vec), div(vec, scalar)
    *    vec     add(vec, vec), add(vec, scalar)
    *    vec     sub(vec, vec), sub(vec, scalar)
    *    vec     crossProduct(vec, vec)
    *
    *  Quaternion:
    *    quat  mul(quat, quat)
    *    quat  normalize(quat)
    *    quat  rotationQuaternion(x, y, z)
    *
    *  Matrix:
    *    Matrix<n, m, type>::identity()
    *
    *    string    toString(mat)
    *    mat       mul(mat, mat), mul(mat, scalar)
    *    vec       mul(mat, vec)
    *    mat       div(mat, scalar)
    *    mat       add(mat, mat), add(mat, scalar)
    *    mat       sub(mat, mat), sub(mat, scalar)
    *
    *    mat<1, n> concatenateToSingleDimension(mat, mat)
    *    mat       mulElementWise(mat, mat)
    *    mat       transpose(mat)
    *    mat       transform(mat, func)
    *    scalar    sum(mat)
    *
    *  Special Matrix:
    *    float4x4 translation(x, y, z or float3 or float4)
    *    float4x4 perspective(fov, aspect, NearZ, FarZ)
    *    float4x4 lookAt(cameraPos, cameraDir)
    *    float4x4 rotationMatrix(x, y, z or quaternion)
    */

    const float PI = 3.14159265f;

    template<int ELEMENT_COUNT, typename vType>
    struct Vector;

    template<typename vType>
    struct Vector<2, vType>
    {
      union
      {
        struct {
          vType x, y;
        };
        vType data[2];
      };

      constexpr inline Vector() : x(static_cast<vType>(0)), y(static_cast<vType>(0)) {}
      constexpr inline Vector(vType v) : x(v), y(v) {}
      constexpr inline Vector(vType x, vType y) : x(x), y(y) {}
      constexpr inline Vector(const Vector& o) : x(o.x), y(o.y) {}
      template<typename vTypeAnother>
      constexpr inline Vector(const Vector<2, vTypeAnother>& o) : x(static_cast<vType>(o.x)), y(static_cast<vType>(o.y)) {}

      constexpr inline vType& operator()(unsigned index)
      {
        return data[index];
      }

      constexpr inline const vType& operator()(unsigned index) const
      {
        return data[index];
      }
    };

    template<typename vType>
    struct Vector<3, vType>
    {
      union
      {
        struct {
          vType x, y, z;
        };
        vType data[3];
      };

      constexpr inline Vector() : x(static_cast<vType>(0)), y(static_cast<vType>(0)), z(static_cast<vType>(0)) {}
      constexpr inline Vector(vType v) : x(v), y(v), z(v) {}
      constexpr inline Vector(vType x, vType y, vType z) : x(x), y(y), z(z) {}
      constexpr inline Vector(Vector<2, vType> o, vType v) : x(o.x), y(o.y), z(v) {}
      constexpr inline Vector(const Vector& o) : x(o.x), y(o.y), z(o.z) {}
      template<typename vTypeAnother>
      constexpr inline Vector(const Vector<3, vTypeAnother>& o) : x(static_cast<vType>(o.x)), y(static_cast<vType>(o.y)), z(static_cast<vType>(o.z)) {}

      constexpr inline vType& operator()(unsigned index)
      {
        return data[index];
      }

      constexpr inline const vType& operator()(unsigned index) const
      {
        return data[index];
      }

      constexpr inline Vector<2, vType> xy() const
      {
        return Vector<2, vType>(x, y);
      }
    };

    template<typename vType>
    struct Vector<4, vType>
    {
      union
      {
        struct {
          vType x, y, z, w;
        };
        vType data[4];
      };

      constexpr inline Vector() : x(static_cast<vType>(0)), y(static_cast<vType>(0)), z(static_cast<vType>(0)), w(static_cast<vType>(0)) {}
      constexpr inline Vector(vType v) : x(v), y(v), z(v), w(v) {}
      constexpr inline Vector(vType x, vType y, vType z, vType w) : x(x), y(y), z(z), w(w) {}
      constexpr inline Vector(Vector<3, vType> o, vType v) : x(o.x), y(o.y), z(o.z), w(v) {}
      constexpr inline Vector(const Vector& o) : x(o.x), y(o.y), z(o.z), w(o.w) {}
      template<typename vTypeAnother>
      constexpr inline Vector(const Vector<4, vTypeAnother>& o) : x(static_cast<vType>(o.x)), y(static_cast<vType>(o.y)), z(static_cast<vType>(o.z)), w(static_cast<vType>(o.w)) {}

      constexpr inline vType& operator()(unsigned index)
      {
        return data[index];
      }

      constexpr inline const vType& operator()(unsigned index) const
      {
        return data[index];
      }

      constexpr inline Vector<3, vType> xyz() const
      {
        return Vector<3, vType>(x, y, z);
      }
      constexpr inline Vector<2, vType> xy() const
      {
        return Vector<2, vType>(x, y);
      }
    };

    typedef Vector<2, float> float2;
    typedef Vector<3, float> float3;
    typedef Vector<4, float> float4;

    typedef Vector<2, double> double2;
    typedef Vector<3, double> double3;
    typedef Vector<4, double> double4;

    typedef Vector<2, int> int2;
    typedef Vector<3, int> int3;
    typedef Vector<4, int> int4;

    typedef Vector<2, unsigned> uint2;
    typedef Vector<3, unsigned> uint3;
    typedef Vector<4, unsigned> uint4;

    template<int ELEMENT_COUNT, typename vType>
    std::string toString(Vector<ELEMENT_COUNT, vType> a)
    {
      std::string fnl = "(";
      for (int i = 0; i < ELEMENT_COUNT - 1; ++i)
      {
        fnl += std::to_string(a(i)) + std::string(", ");
      }
      return fnl + std::to_string(a(ELEMENT_COUNT - 1)) + std::string(")");
    }

    template<int ELEMENT_COUNT, typename vType>
    constexpr inline vType length(Vector<ELEMENT_COUNT, vType> a)
    {
      vType val{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        val += a(i) * a(i);
      }
      return std::sqrt(val);
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC normalize(Vector<ELEMENT_COUNT, vType> a)
    {
      VEC n{};
      const vType len = length(a);
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) / len;
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline vType dot(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
    {
      vType v{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        v += a(i) * b(i);
      }
      return v;
    }

    // scalars
    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC mul(Vector<ELEMENT_COUNT, vType> a, vType b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) * b;
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC mul(vType b, Vector<ELEMENT_COUNT, vType> a)
    {
      return mul(a, b);
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC div(Vector<ELEMENT_COUNT, vType> a, vType b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n.data[i] = a.data[i] / b;
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC add(Vector<ELEMENT_COUNT, vType> a, vType b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) + b;
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC add(vType b, Vector<ELEMENT_COUNT, vType> a)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = b + a(i);
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC sub(Vector<ELEMENT_COUNT, vType> a, vType b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) - b;
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC sub(vType b, Vector<ELEMENT_COUNT, vType> a)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = b - a(i);
      }
      return n;
    }

    // vectors
    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC mul(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) * b(i);
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC div(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) / b(i);
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC add(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) + b(i);
      }
      return n;
    }

    template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
    constexpr inline VEC sub(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
    {
      VEC n{};
      for (int i = 0; i < ELEMENT_COUNT; ++i)
      {
        n(i) = a(i) - b(i);
      }
      return n;
    };

    template<typename vType>
    constexpr inline Vector<3, vType> crossProduct(Vector<3, vType> a, Vector<3, vType> b)
    {
      Vector<3, vType> r;
      r.x = a.y * b.z - a.z * b.y;
      r.y = a.z * b.x - a.x * b.z;
      r.z = a.x * b.y - a.y * b.x;
      return r;
    }

    template<typename vType>
    constexpr inline Vector<4, vType> crossProduct(Vector<4, vType> a, Vector<4, vType> b)
    {
      Vector<4, vType> r{};
      r.x = a.y * b.z - a.z * b.y;
      r.y = a.z * b.x - a.x * b.z;
      r.z = a.x * b.y - a.y * b.x;
      return r;
    }

    template <int rowCount, int columnCount, typename vType>
    struct Matrix
    {
      vType data[rowCount*columnCount];

      constexpr inline vType& operator()(unsigned x, unsigned y)
      {
        return data[y * columnCount + x];
      }
      constexpr inline const vType& operator()(unsigned x, unsigned y) const
      {
        return data[y * columnCount + x];
      }

      constexpr inline vType& operator()(unsigned x)
      {
        return data[x];
      }

      constexpr inline const vType& operator()(unsigned x) const
      {
        return data[x];
      }

      constexpr inline int rows() const
      {
        return rowCount;
      }
      constexpr inline int cols() const
      {
        return columnCount;
      }

      constexpr inline size_t size() const
      {
        return rowCount * columnCount;
      }

      constexpr inline Vector<columnCount, vType> row(unsigned y)
      {
        Vector<columnCount, vType> v1{};
        for (int i = 0; i < columnCount; ++i)
        {
          v1(i) = data[y*columnCount + i];
        }
        return v1;
      }

      static Matrix identity()
      {
        Matrix m{};
        const auto max = std::max(rowCount, columnCount);
        for (int i = 0; i < max; ++i)
        {
          m(i, i) = static_cast<vType>(1);
        }
        return m;
      }
    };

    struct Quaternion
    {
      union
      {
        struct {
          float w, x, y, z;
        };
        float data[4];
      };

      constexpr inline float& operator()(unsigned index)
      {
        return data[index];
      }

      constexpr inline const float& operator()(unsigned index) const
      {
        return data[index];
      }
    };

    std::string toString(Quaternion a);

    // RotateAxis(1,0,0,amount) -> this would rotate x axis... someway :D?
    // constexpr inline
    inline Quaternion rotateAxis(float x, float y, float z, float amount)
    {
      Quaternion l{};
      l.w = std::cos(amount / 2.f);
      l.x = std::sin(amount / 2.f)*x;
      l.y = std::sin(amount / 2.f)*y;
      l.z = std::sin(amount / 2.f)*z;
      return l;
    }

    //constexpr inline
    inline Quaternion rotateAxis(float3 directionUnitVector, float amount)
    {
      Quaternion l{};
      l.w = std::cos(amount / 2.f);
      l.x = std::sin(amount / 2.f)*directionUnitVector.x;
      l.y = std::sin(amount / 2.f)*directionUnitVector.y;
      l.z = std::sin(amount / 2.f)*directionUnitVector.z;
      return l;
    }

    constexpr inline Quaternion mul(Quaternion q1, Quaternion q2)
    {
      Quaternion r{};
      r.w = q1.w * q2.w;
      for (int i = 1; i < 4; i++)
      {
        r.w -= q1(i) * q2(i);
      }
      r.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
      r.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
      r.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
      return r;
    }

    // Use this to do all rotations at once (just does many times the RotateAxis)
    //constexpr inline
    inline Quaternion rotationQuaternionSlow(float yaw, float pitch, float roll)
    {
      Quaternion r{ 1.f, 0.f, 0.f, 0.f };
      r = mul(r, rotateAxis(0.f, 1.f, 0.f, yaw)); // y is up, so its second component
      r = mul(r, rotateAxis(0.f, 0.f, 1.f, pitch));
      r = mul(r, rotateAxis(1.f, 0.f, 0.f, roll));
      return r;
    }

    // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    //constexpr inline
    inline Quaternion rotationQuaternionEuler(float yaw, float pitch, float roll)
    {
      Quaternion q{};
      // Abbreviations for the various angular functions
      //
      // Function was written so that z is up, converted to y is up, switch yaw and pitch
      float cy = std::cos(pitch * 0.5f);
      float sy = std::sin(pitch * 0.5f);
      float cr = std::cos(roll * 0.5f);
      float sr = std::sin(roll * 0.5f);
      float cp = std::cos(yaw * 0.5f);
      float sp = std::sin(yaw * 0.5f);

      q.w = cy * cr * cp + sy * sr * sp;
      q.x = cy * sr * cp - sy * cr * sp;
      q.y = cy * cr * sp + sy * sr * cp;
      q.z = sy * cr * cp - cy * sr * sp;
      return q;
    }

    //constexpr inline
    inline Quaternion normalize(Quaternion q)
    {
      Quaternion r{};
      float magnitude = std::sqrt(std::pow(q.w, 2.0f) + std::pow(q.x, 2.0f) + std::pow(q.y, 2.0f) + std::pow(q.z, 2.0f));
      for (int i = 0; i < 4; i++)
      {
        r(i) = q(i) / magnitude;
      }
      return r;
    }

    constexpr inline Vector<3, float> rotateVector(Vector<3, float> v, Quaternion quat)
    {
      // Extract the vector part of the quaternion
      Vector<3, float> u{ quat.x, quat.y, quat.z };

      // Extract the scalar part of the quaternion
      float s = quat.w;

      return add(mul(2.0f * dot(u, v), u), add(mul((s*s - dot(u, u)), v), mul(2.0f, mul(s, crossProduct(u, v)))));
    }

    typedef Matrix<3, 3, float> float3x3;
    typedef Matrix<4, 4, float> float4x4;

    template <int rowCount, int columnCount, typename vType>
    std::string toString(Matrix<rowCount, columnCount, vType> m)
    {
      std::string r = "[";
      for (int y = 0; y < rowCount - 1; ++y)
      {
        for (int x = 0; x < columnCount - 1; ++x)
        {
          r += std::to_string(m(x, y)) + std::string(", ");
        }
        r += std::to_string(m(columnCount - 1, y)) + std::string("]\n[");
      }
      for (int x = 0; x < columnCount - 1; ++x)
      {
        r += std::to_string(m(x, rowCount - 1)) + std::string(", ");
      }
      r += std::to_string(m(columnCount - 1, rowCount - 1)) + std::string("]");
      return r;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<cols, rows, vType> transpose(Matrix<rows, cols, vType> value)
    {
      Matrix<cols, rows, vType> outResult{};
      for (int y = 0; y < rows; ++y)
      {
        for (int x = 0; x < cols; ++x)
        {
          outResult(y, x) = value(x, y);
        }
      }
      return outResult;
    }

    template <typename vType, int rows, int cols, int rows2, int cols2>
    constexpr inline Matrix<rows, cols2, vType> mul(Matrix<rows, cols, vType> a, Matrix<rows2, cols2, vType> b)
    {
      // a = <2, 3>, b = <3, 2>
      // rows = 2
      // cols2 = 2
      Matrix<rows, cols2, vType> outResult{};
      for (int y = 0; y < cols2; ++y)
      {
        for (int x = 0; x < rows; ++x)
        {
          for (int j = 0; j < rows2; ++j)
          {
            outResult(y, x) += a(j, x) * b(y, j);
            //printf("m: y:%d x:%d j:%d %f %f %f\n", y, x, j, outResult(x, y), a(j, x), b(j, y));
          }
        }
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<1, cols, vType> mul(Vector<cols, vType> v, Matrix<rows, cols, vType> a)
    {
      Matrix<1, cols, vType> v2{};
      for (int i = 0; i < cols; ++i)
      {
        v2(i) = v(i);
      }
      return mul(v2, a);
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, 1, vType> mul(Matrix<rows, cols, vType> a, Vector<rows, vType> v)
    {
      Matrix<rows, 1, vType> v2{};
      for (int i = 0; i < cols; ++i)
      {
        v2(i) = v(i);
      }
      return mul(a, v2);
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> mul(Matrix<rows, cols, vType> a, vType b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) * b;
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> mul(vType b, Matrix<rows, cols, vType> a)
    {
      return mul(a, b);
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> div(Matrix<rows, cols, vType> a, vType b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) / b;
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> add(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) + b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> add(Matrix<rows, cols, vType> a, vType b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) + b;
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> add(vType b, Matrix<rows, cols, vType> a)
    {
      return add(a, b);
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> sub(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) - b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> sub(Matrix<rows, cols, vType> a, vType b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) - b;
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> sub(vType b, Matrix<rows, cols, vType> a)
    {
      return sub(a, b);
    }

    template <typename vType, int rows, int cols, int rows2, int cols2>
    constexpr inline Matrix<1, rows*cols + rows2 * cols2, vType> concatenateToSingleDimension(Matrix<rows, cols, vType> a, Matrix<rows2, cols2, vType> b)
    {
      Matrix<1, rows*cols + rows2 * cols2, vType> outResult{};
      memcpy(outResult.data, a.data, rows*cols * sizeof(vType));
      memcpy(outResult.data + (rows*cols), b.data, rows2*cols2 * sizeof(vType));
      return outResult;
    }

    template <typename vType, int rows, int cols, int rows2, int cols2>
    constexpr inline void extractMatrices(Matrix<1, rows*cols + rows2 * cols2, vType> input, Matrix<rows, cols, vType>& out1, Matrix<rows2, cols2, vType> out2)
    {
      std::copy_n(std::begin(input.data), rows*cols, std::begin(out1.data));
      std::copy_n(std::begin(input.data) + rows * cols, rows2*cols2, std::begin(out2.data));
    }

    template <typename vType, int rows, int cols>
    constexpr inline Matrix<rows, cols, vType> mulElementWise(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) * b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols, typename Func>
    constexpr inline Matrix<rows, cols, vType> transform(Matrix<rows, cols, vType> value, Func&& f)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int y = 0; y < rows; ++y)
      {
        for (int x = 0; x < cols; ++x)
        {
          outResult(x, y) = f(value(x, y));
        }
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    inline vType sum(Matrix<rows, cols, vType> value)
    {
      vType outResult{};
      for (int y = 0; y < rows; ++y)
      {
        for (int x = 0; x < cols; ++x)
        {
          outResult += value(x, y);
        }
      }
      return outResult;
    }

    inline float4x4 translation(float x, float y, float z)
    {
      float4x4 result = float4x4::identity();
      result(0, 3) = x; result(1, 3) = y; result(2, 3) = z;
      return result;
    }

    inline float4x4 translation(float3 vec)
    {
      float4x4 result = float4x4::identity();
      result(0, 3) = vec.x; result(1, 3) = vec.y; result(2, 3) = vec.z;
      return result;
    }
    inline float4x4 translation(float4 vec)
    {
      float4x4 result = float4x4::identity();
      result(0, 1) = vec.x; result(1, 3) = vec.y; result(2, 3) = vec.z;
      return result;
    }

    // x&y -> [-1, 1], z -> [0, 1], OK! Don't know which, rh or lh. but works now.
    // seems like this is left handed, see result(2,3) and (3,2)
    inline float4x4 perspectivelh(float fov, float aspect, float NearZ, float FarZ)
    {
      if (fov <= 0 || aspect == 0)
      {
        return float4x4{};
      }

      float b = 1.0f / std::tan(fov*(PI / 180.f)*0.5f);

      float4x4 result{};
      result(0, 0) = aspect * b;
      result(1, 1) = b;
      result(2, 2) = FarZ / (FarZ - NearZ);
      result(3, 2) = 1.f;
      result(2, 3) = -1.f * NearZ * result(2, 2);
      return result;
    }

    inline float4x4 perspectiverh(float fov, float aspect, float NearZ, float FarZ)
    {
      auto kek = perspectivelh(fov, aspect, NearZ, FarZ);
      kek(3, 2) = -1.f;
      kek(2, 3) = 1.f * NearZ * kek(2, 2);
      return kek;
    }

    // http://www.cs.virginia.edu/~gfx/Courses/1999/intro.fall99.html/lookat.html
    // https://www.gamedev.net/articles/programming/graphics/perspective-projections-in-lh-and-rh-systems-r3598/
    // top link sort of dead. oh well :), great article below.
    inline float4x4 lookAt(float4 cameraPos, float4 cameraTarget, float4 upVec = float4(0.f, 1.f, 0.f, 0.f))
    {
      float4 dirVec = sub(cameraTarget, cameraPos);
      // crush the w component so that it doesn't invalidate our result
      dirVec.w = 0.f;
      dirVec = normalize(dirVec);
      float4 cross = crossProduct(dirVec, upVec);
      float4 side = normalize(cross);
      upVec = crossProduct(side, dirVec);
      float4x4 m = float4x4::identity();
      m(0, 0) = side.x;    m(0, 1) = side.y;    m(0, 2) = side.z;
      m(1, 0) = upVec.x;   m(1, 1) = upVec.y;   m(1, 2) = upVec.z;
      m(2, 0) = -dirVec.x; m(2, 1) = -dirVec.y; m(2, 2) = -dirVec.z;
      auto temp = translation(mul(cameraPos, -1.f));
      auto ret = mul(temp, m);
      return ret;
    }

    /*
    * directx style
    * X right
    * Y up
    * Z into screen
    *
    * this function was for
    * X right
    * Y up
    * Z toward viewer
    * basically my yaw/pitch/roll are not in line with anything.
    */
    inline float4x4 rotationMatrixRH(float yaw, float pitch, float roll)
    {
      auto r = float4x4::identity();
      float a = std::cos(yaw);
      float b = std::sin(yaw);
      float c = std::cos(pitch);
      float d = std::sin(pitch);
      float f = std::cos(roll);
      float g = std::sin(roll);
      // precalculated matrix multiplications
      r(0, 0) = c * f;
      r(0, 1) = c * (-g);
      r(0, 2) = d;
      r(1, 0) = (-b)*(-d)*f + a * g;
      r(2, 1) = (-b)*(-d)*(-g) + a * f;
      r(3, 2) = (-b)*c;
      r(2, 0) = a * (-d)*f + b * g;
      r(2, 1) = a * (-d)*(-g) + b * f;
      r(2, 2) = a * c;
      return r;
    }

    inline float4x4 rotationMatrixRH(Quaternion q)
    {
      auto r = float4x4::identity();
      float xx = std::pow(q.x, 2.f);
      float yy = std::pow(q.y, 2.f);
      float zz = std::pow(q.z, 2.f);
      r(0, 0) = 1.f - 2.f*yy - 2.f*zz;
      r(1, 1) = 1.f - 2.f*xx - 2.f*zz;
      r(2, 2) = 1.f - 2.f*xx - 2.f*yy;
      r(0, 1) = 2.f*q.x * q.y - 2.f*q.w * q.z;
      r(1, 0) = 2.f*q.x * q.y + 2.f*q.w * q.z;
      r(2, 0) = 2.f*q.x * q.z - 2.f*q.w * q.y;
      r(0, 2) = 2.f*q.x * q.z + 2.f*q.w * q.y;
      r(2, 1) = 2.f*q.y * q.z - 2.f*q.w * q.x;
      r(1, 2) = 2.f*q.y * q.z + 2.f*q.w * q.x;
      return r;
    }

    inline float4x4 rotationMatrixLH(Quaternion q)
    {
      auto r = float4x4::identity();
      float xx = std::pow(q.x, 2.f);
      float yy = std::pow(q.y, 2.f);
      float zz = std::pow(q.z, 2.f);
      r(0, 0) = 1.f - 2.f*yy - 2.f*xx;
      r(1, 1) = 1.f - 2.f*xx - 2.f*zz;
      r(2, 2) = 1.f - 2.f*xx - 2.f*yy;
      r(0, 1) = 2.f*q.x * q.y + 2.f*q.w * q.z;
      r(1, 0) = 2.f*q.x * q.y - 2.f*q.w * q.z;
      r(2, 0) = 2.f*q.x * q.z + 2.f*q.w * q.y;
      r(0, 2) = 2.f*q.x * q.z - 2.f*q.w * q.y;
      r(2, 1) = 2.f*q.y * q.z + 2.f*q.w * q.x;
      r(1, 2) = 2.f*q.y * q.z - 2.f*q.w * q.x;
      return r;
    }
  }
}
typedef faze::math::Vector<2, float> float2;
typedef faze::math::Vector<3, float> float3;
typedef faze::math::Vector<4, float> float4;

typedef faze::math::Vector<2, double> double2;
typedef faze::math::Vector<3, double> double3;
typedef faze::math::Vector<4, double> double4;

typedef faze::math::Vector<2, int> int2;
typedef faze::math::Vector<3, int> int3;
typedef faze::math::Vector<4, int> int4;

typedef unsigned uint;
typedef faze::math::Vector<2, unsigned> uint2;
typedef faze::math::Vector<3, unsigned> uint3;
typedef faze::math::Vector<4, unsigned> uint4;

typedef faze::math::Matrix<3, 3, float> float3x3;
typedef faze::math::Matrix<4, 4, float> float4x4;

typedef faze::math::Quaternion quaternion;

namespace faze
{
  struct Rect
  {
    uint2 leftTop;
    uint2 rightBottom;

    Rect()
      : leftTop(0)
      , rightBottom(0)
    {}
    Rect(uint2 start, uint2 size)
      : leftTop(start)
      , rightBottom(add(start, size))
    {}
  };

  struct Box
  {
    uint3 leftTopFront;
    uint3 rightBottomBack;

    Box()
      : leftTopFront(0)
      , rightBottomBack(0)
    {}
    Box(uint3 start, uint3 size)
      : leftTopFront(start)
      , rightBottomBack(add(start, size))
    {}
  };
}
#if defined(FAZE_PLATFORM_WINDOWS)
#pragma warning( pop )
#endif