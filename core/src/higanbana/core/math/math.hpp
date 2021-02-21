#pragma once

#include <cstring>
#include <cstdint>
#include <cmath>
#include <compare>
#include "higanbana/core/platform/definitions.hpp"

#if defined(HIGANBANA_PLATFORM_WINDOWS)
#pragma warning( push )
#pragma warning( disable : 4201 ) // unreferenced local function has been removed
#endif

namespace higanbana
{
  namespace math
  {
    /*  operations available
    *  Vector:
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
    constexpr double Epsilon = 0.00000001;

    constexpr float PI = 3.14159265f;

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

          HIGAN_INLINE vType& operator()(unsigned index)
          {
            return data[index];
          }

          constexpr inline const vType& operator()(unsigned index) const
          {
            return data[index];
          }
          template <typename rType = vType, std::enable_if_t<std::is_floating_point<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<2, rType>& rhs) const {
            for (int i = 0; i < 2; i++) {
              if (std::fabs(data[i] - rhs(i)) > Epsilon)
                return false;
            }
            return true;
          }
          template <typename rType = vType, std::enable_if_t<std::is_integral<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<2, rType>& rhs) const {
            for (int i = 0; i < 2; i++) {
              if (data[i] != rhs(i))
                return false;
            }
            return true;
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

          HIGAN_INLINE vType& operator()(unsigned index)
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

          template <typename rType = vType, std::enable_if_t<std::is_floating_point<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<3, rType>& rhs) const {
            for (int i = 0; i < 3; i++) {
              if (std::fabs(data[i] - rhs(i)) > Epsilon)
                return false;
            }
            return true;
          }
          template <typename rType = vType, std::enable_if_t<std::is_integral<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<3, rType>& rhs) const {
            for (int i = 0; i < 3; i++) {
              if (data[i] != rhs(i))
                return false;
            }
            return true;
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
          constexpr inline Vector(Vector<2, vType> o, vType z, vType w) : x(o.x), y(o.y), z(z), w(w) {}
          constexpr inline Vector(Vector<3, vType> o, vType v) : x(o.x), y(o.y), z(o.z), w(v) {}
          constexpr inline Vector(const Vector& o) : x(o.x), y(o.y), z(o.z), w(o.w) {}
          template<typename vTypeAnother>
          constexpr inline Vector(const Vector<4, vTypeAnother>& o) : x(static_cast<vType>(o.x)), y(static_cast<vType>(o.y)), z(static_cast<vType>(o.z)), w(static_cast<vType>(o.w)) {}

          HIGAN_INLINE vType& operator()(unsigned index)
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

          template <typename rType = vType, std::enable_if_t<std::is_floating_point<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<4, rType>& rhs) const {
            for (int i = 0; i < 4; i++) {
              if (std::fabs(data[i] - rhs(i)) > Epsilon)
                return false;
            }
            return true;
          }
          template <typename rType = vType, std::enable_if_t<std::is_integral<rType>::value, bool> = true>
          constexpr auto operator==(const Vector<4, rType>& rhs) const {
            for (int i = 0; i < 4; i++) {
              if (data[i] != rhs(i))
                return false;
            }
            return true;
          }
        };

        /*
        using float2 = Vector<2, float>;
        using float3 = Vector<3, float>;
        using float4 = Vector<4, float>;

        using double2 = Vector<2, double>;
        using double3 = Vector<3, double>;
        using double4 = Vector<4, double>;

        using int2 = Vector<2, int>;
        using int3 = Vector<3, int>;
        using int4 = Vector<4, int>;

        using uint2 = Vector<2, unsigned>;
        using uint3 = Vector<3, unsigned>;
        using uint4 = Vector<4, unsigned>;*/

        template<int ELEMENT_COUNT, typename vType>
        inline vType length_squared(Vector<ELEMENT_COUNT, vType> a)
        {
          vType val{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            val += a(i) * a(i);
          }
          return val;
        }

        template<int ELEMENT_COUNT, typename vType>
        inline vType length(Vector<ELEMENT_COUNT, vType> a)
        {
          return std::sqrt(length_squared(a));
        }

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC normalize(Vector<ELEMENT_COUNT, vType> a)
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
        inline vType dot(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
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
        inline VEC mul(Vector<ELEMENT_COUNT, vType> a, vType b)
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
        constexpr inline VEC div(vType a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n.data[i] = a / b.data[i];
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
        inline VEC sub(Vector<ELEMENT_COUNT, vType> a, vType b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) - b;
          }
          return n;
        }

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC sub(vType b, Vector<ELEMENT_COUNT, vType> a)
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
        inline VEC mul(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) * b(i);
          }
          return n;
        }

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC div(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) / b(i);
          }
          return n;
        }

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC add(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) + b(i);
          }
          return n;
        }

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC sub(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) - b(i);
          }
          return n;
        };

        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC min(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) < b(i) ? a(i) : b(i);
          }
          return n;
        };
        template<int ELEMENT_COUNT, typename vType, typename VEC = Vector<ELEMENT_COUNT, vType>>
        inline VEC max(Vector<ELEMENT_COUNT, vType> a, Vector<ELEMENT_COUNT, vType> b)
        {
          VEC n{};
          for (int i = 0; i < ELEMENT_COUNT; ++i)
          {
            n(i) = a(i) > b(i) ? a(i) : b(i);
          }
          return n;
        };

        template<typename vType>
        inline Vector<3, vType> crossProduct(Vector<3, vType> a, Vector<3, vType> b)
        {
          Vector<3, vType> r;
          r.x = a.y * b.z - a.z * b.y;
          r.y = a.z * b.x - a.x * b.z;
          r.z = a.x * b.y - a.y * b.x;
          return r;
        }

        template<typename vType>
        inline Vector<4, vType> crossProduct(Vector<4, vType> a, Vector<4, vType> b)
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

          HIGAN_INLINE vType& operator()(unsigned x, unsigned y)
          {
            return data[y * columnCount + x];
          }
          constexpr inline const vType& operator()(unsigned x, unsigned y) const
          {
            return data[y * columnCount + x];
          }

          HIGAN_INLINE vType& operator()(unsigned x)
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

          inline Vector<columnCount, vType> row(unsigned y) const
          {
            Vector<columnCount, vType> v1{};
            for (int i = 0; i < columnCount; ++i)
            {
              v1(i) = data[y*columnCount + i];
            }
            return v1;
          }

          inline Vector<rowCount, vType> column(unsigned x) const
          {
            Vector<rowCount, vType> v1{};
            for (int i = 0; i < rowCount; ++i)
            {
              v1(i) = data[i*columnCount + x];
            }
            return v1;
          }

          static Matrix identity()
          {
            Matrix m{};
            constexpr auto max = rowCount > columnCount ? rowCount : columnCount;
            for (int i = 0; i < max; ++i)
            {
              m(i, i) = static_cast<vType>(1);
            }
            return m;
          }

          template <typename rType = vType, std::enable_if_t<std::is_floating_point<rType>::value, bool> = true>
          constexpr auto operator==(const Matrix<rowCount, columnCount, rType>& rhs) const {
            for (int i = 0; i < rowCount*columnCount; i++) {
              if (std::fabs(operator()(i) - rhs(i)) > Epsilon)
                return false;
            }
            return true;
          }

          template <typename rType = vType, std::enable_if_t<std::is_integral<rType>::value, bool> = true>
          constexpr auto operator==(const Matrix<rowCount, columnCount, rType>& rhs) const {
            for (int i = 0; i < rowCount*columnCount; i++) {
              if (operator()(i) != rhs(i))
                return false;
            }
            return true;
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

          inline float& operator()(unsigned index)
          {
            return data[index];
          }

          constexpr inline const float& operator()(unsigned index) const
          {
            return data[index];
          }
          auto operator==(const Quaternion& b) const
          {
            bool same = true;
            for (int i = 0; i < 4; ++i) {
              if (fabs(data[i] - b.data[i]) > Epsilon)
                same = false;
            }
            return same; 
          }
        };

        //std::string toString(Quaternion a);

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
        inline Quaternion rotateAxis(Vector<3, float> directionUnitVector, float amount)
        {
          Quaternion l{};
          l.w = std::cos(amount / 2.f);
          l.x = std::sin(amount / 2.f)*directionUnitVector.x;
          l.y = std::sin(amount / 2.f)*directionUnitVector.y;
          l.z = std::sin(amount / 2.f)*directionUnitVector.z;
          return l;
        }

        inline Quaternion mul(Quaternion q1, Quaternion q2)
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

    inline Vector<3, float> rotateVector(Vector<3, float> v, Quaternion quat)
    {
      // Extract the vector part of the quaternion
      Vector<3, float> u{ quat.x, quat.y, quat.z };

      // Extract the scalar part of the quaternion
      float s = quat.w;

      return add(mul(2.0f * dot(u, v), u), add(mul((s*s - dot(u, u)), v), mul(2.0f, mul(s, crossProduct(u, v)))));
    }

    typedef Matrix<3, 3, float> float3x3;
    typedef Matrix<4, 4, float> float4x4;
    typedef Matrix<3, 4, float> float3x4;
    typedef Vector<3, float> float3;
    typedef Vector<4, float> float4;

    template <typename vType, int rows, int cols>
    inline Matrix<cols, rows, vType> transpose(Matrix<rows, cols, vType> value)
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
    inline Matrix<rows, cols2, vType> mul(Matrix<rows, cols, vType> a, Matrix<rows2, cols2, vType> b)
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

    template <typename vType, int rows, int cols, int rows2, int cols2>
    inline Matrix<rows, cols2, vType> mul2(Matrix<rows, cols, vType> a, Matrix<rows2, cols2, vType> b)
    {
      Matrix<rows, cols2, vType> outResult{};
      for (int i = 0; i < cols2; ++i)
      {
        for (int k = 0; k < rows; ++k)
        {
          for (int j = 0; j < rows2; ++j)
          {
            outResult(j, i) += a(k, i) * b(j, k);
          }
        }
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    inline Matrix<1, cols, vType> mul(Vector<cols, vType> v, Matrix<rows, cols, vType> a)
    {
      Matrix<1, cols, vType> v2{};
      for (int i = 0; i < cols; ++i)
      {
        v2(i) = v(i);
      }
      return mul(v2, a);
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, 1, vType> mul(Matrix<rows, cols, vType> a, Vector<rows, vType> v)
    {
      Matrix<rows, 1, vType> v2{};
      for (int i = 0; i < cols; ++i)
      {
        v2(i) = v(i);
      }
      return mul(a, v2);
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, cols, vType> mul(Matrix<rows, cols, vType> a, vType b)
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
    inline Matrix<rows, cols, vType> div(Matrix<rows, cols, vType> a, vType b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) / b;
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, cols, vType> add(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) + b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, cols, vType> add(Matrix<rows, cols, vType> a, vType b)
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
    inline Matrix<rows, cols, vType> sub(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) - b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, cols, vType> sub(Matrix<rows, cols, vType> a, vType b)
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
    inline Matrix<1, rows*cols + rows2 * cols2, vType> concatenateToSingleDimension(Matrix<rows, cols, vType> a, Matrix<rows2, cols2, vType> b)
    {
      Matrix<1, rows*cols + rows2 * cols2, vType> outResult{};
      memcpy(outResult.data, a.data, rows*cols * sizeof(vType));
      memcpy(outResult.data + (rows*cols), b.data, rows2*cols2 * sizeof(vType));
      return outResult;
    }

    template <typename vType, int rows, int cols, int rows2, int cols2>
    constexpr inline void extractMatrices(Matrix<1, rows*cols + rows2 * cols2, vType> input, Matrix<rows, cols, vType>& out1, Matrix<rows2, cols2, vType> out2)
    {
      memcpy(input.data, out1.data, rows*cols*sizeof(vType));
      memcpy(input.data + rows*cols, out2.data, rows2*cols2*sizeof(vType));
    }

    template <typename vType, int rows, int cols>
    inline Matrix<rows, cols, vType> mulElementWise(Matrix<rows, cols, vType> a, Matrix<rows, cols, vType> b)
    {
      Matrix<rows, cols, vType> outResult{};
      for (int i = 0; i < rows*cols; ++i)
      {
        outResult(i) = a(i) * b(i);
      }
      return outResult;
    }

    template <typename vType, int rows, int cols, typename Func>
    inline Matrix<rows, cols, vType> transform(Matrix<rows, cols, vType> value, Func&& f)
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

    inline float4x4 scale(float x, float y, float z)
    {
      float4x4 result = float4x4::identity();
      result(0, 0) = x; result(1, 1) = y; result(2, 2) = z;
      return result;
    }
    
    inline float4x4 scale(float whole)
    {
      return scale(whole, whole, whole);
    }

    inline float4x4 scale(Vector<3, float> whole)
    {
      return scale(whole.x, whole.y, whole.z);
    }

    inline float3x4 translation3x4(float x, float y, float z)
    {
      float3x4 result = float3x4::identity();
      result(3, 0) = -x; result(3, 1) = -y; result(3, 2) = -z;
      return result;
    }
    inline float3x4 translation3x4(Vector<3, float> vec)
    {
      float3x4 result = float3x4::identity();
      result(3, 0) = -vec.x; result(3, 1) = -vec.y; result(3, 2) = -vec.z;
      return result;
    }
    inline float3x4 translation3x4(Vector<4, float> vec)
    {
      float3x4 result = float3x4::identity();
      result(3, 0) = -vec.x; result(3, 1) = -vec.y; result(3, 2) = -vec.z; 
      return result;
    }

    inline float4x4 translation(float x, float y, float z)
    {
      float4x4 result = float4x4::identity();
      result(0, 3) = x; result(1, 3) = y; result(2, 3) = z;
      return result;
    }

    inline float4x4 translation(Vector<3, float> vec)
    {
      float4x4 result = float4x4::identity();
      result(0, 3) = -vec.x; result(1, 3) = -vec.y; result(2, 3) = -vec.z;
      return result;
    }
    inline float4x4 translation(Vector<4, float> vec)
    {
      float4x4 result = float4x4::identity();
      result(0, 3) = -vec.x; result(1, 3) = -vec.y; result(2, 3) = -vec.z; 
      return result;
    }

    inline Vector<2, float> first2termsOfPerspective(float fov, float aspect) {
      if (fov <= 0 || aspect == 0)
      {
        return Vector<2, float>{};
      }

      float b = 1.0f / std::tan(fov*(PI / 180.f)*0.5f);
      return Vector<2, float>(aspect*b, b);
    }

    // x&y -> [-1, 1], z -> [0, 1], OK! Don't know which, rh or lh. but works now.
    // seems like this is left handed, see result(2,3) and (3,2)
    inline float4x4 perspectiveLH(float fov, float aspect, float NearZ, float FarZ)
    {
      auto terms = first2termsOfPerspective(fov, aspect);

      float4x4 result{};
      result(0, 0) = terms(0);
      result(1, 1) = terms(1);
      result(2, 2) = FarZ / (FarZ - NearZ); 
      result(3, 2) = 1.f;
      result(2, 3) = NearZ * FarZ / (NearZ - FarZ);
      return result;
    }

    inline float4x4 perspectiveRH(float fov, float aspect, float NearZ, float FarZ)
    {
      auto kek = perspectiveLH(fov, aspect, NearZ, FarZ);
      kek(2, 2) = FarZ / (NearZ - FarZ);
      kek(3, 2) = -1.f;
      return kek;
    }

    inline float4x4 perspectiveLHInverseZ(float fov, float aspect, float NearZ, float FarZ)
    {
      auto terms = first2termsOfPerspective(fov, aspect);
      float4x4 result = {};
      result(0, 0) = terms(0);
      result(1, 1) = terms(1);
      result(2, 2) = -1.f * (NearZ / (FarZ - NearZ));
      result(3, 2) = 1.f;
      result(2, 3) = (FarZ * NearZ) / (FarZ - NearZ);
      return result;
    }

    inline float4x4 perspectiveRHInverseZ(float fov, float aspect, float NearZ, float FarZ)
    {
      auto kek = perspectiveLHInverseZ(fov, aspect, NearZ, FarZ);
      kek(2, 2) = NearZ / (FarZ - NearZ);
      kek(3, 2) = -1.f;
      return kek;
    }

    inline float4x4 perspectiveLHInverseInfZ(float fov, float aspect, float NearZ)
    {
      auto terms = first2termsOfPerspective(fov, aspect);
      float4x4 result = {};
      result(0, 0) = terms(0);
      result(1, 1) = terms(1);
      result(2, 2) = 0.f;
      result(3, 2) = 1.f;
      result(2, 3) = NearZ;
      return result;
    }

    inline float4x4 perspectiveRHInverseInfZ(float fov, float aspect, float NearZ) {
      auto kek = perspectiveLHInverseInfZ(fov, aspect, NearZ);
      kek(3, 2) = -1.f;
      return kek;
    }

    // http://www.cs.virginia.edu/~gfx/Courses/1999/intro.fall99.html/lookat.html
    // https://www.gamedev.net/articles/programming/graphics/perspective-projections-in-lh-and-rh-systems-r3598/
    // top link sort of dead. oh well :), great article below.
    inline float4x4 lookAt(Vector<4, float> cameraPos, Vector<4, float> cameraTarget, Vector<4, float> upVec = Vector<4, float>(0.f, 1.f, 0.f, 0.f))
    {
      Vector<4, float> dirVec = sub(cameraTarget, cameraPos);
      // crush the w component so that it doesn't invalidate our result
      dirVec.w = 0.f;
      dirVec = normalize(dirVec);
      Vector<4, float> cross = crossProduct(dirVec, upVec);
      Vector<4, float> side = normalize(cross);
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
      r(1, 2) = 2.f*q.y * q.z - 2.f*q.w * q.x;
      r(2, 1) = 2.f*q.y * q.z + 2.f*q.w * q.x;
      return r;
    }

    inline float4x4 rotationMatrixLH(Quaternion q)
    {
      q = normalize(q);
      auto r = float4x4::identity();
      float xx = std::pow(q.x, 2.f);
      float yy = std::pow(q.y, 2.f);
      float zz = std::pow(q.z, 2.f);
      r(0, 0) = 1.f - 2.f * yy - 2.f * zz;
      r(1, 1) = 1.f - 2.f * xx - 2.f * zz;
      r(2, 2) = 1.f - 2.f * xx - 2.f * yy;
      r(0, 1) = 2.f * q.x * q.y + 2.f * q.w * q.z;
      r(1, 0) = 2.f * q.x * q.y - 2.f * q.w * q.z;
      r(2, 0) = 2.f * q.x * q.z + 2.f * q.w * q.y;
      r(0, 2) = 2.f * q.x * q.z - 2.f * q.w * q.y;
      r(1, 2) = 2.f * q.y * q.z + 2.f * q.w * q.x;
      r(2, 1) = 2.f * q.y * q.z - 2.f * q.w * q.x;
      #if 1
      //r = math::transpose(r);
      #endif
      return r;
    }
    
    inline float3x4 rotationMatrixLH3x4(Quaternion q)
    {
      q = normalize(q);
      auto r = float3x4::identity();
      float xx = std::pow(q.x, 2.f);
      float yy = std::pow(q.y, 2.f);
      float zz = std::pow(q.z, 2.f);
      r(0, 0) = 1.f - 2.f * yy - 2.f * zz;
      r(1, 1) = 1.f - 2.f * xx - 2.f * zz;
      r(2, 2) = 1.f - 2.f * xx - 2.f * yy;
      r(0, 1) = 2.f * q.x * q.y + 2.f * q.w * q.z;
      r(1, 0) = 2.f * q.x * q.y - 2.f * q.w * q.z;
      r(2, 0) = 2.f * q.x * q.z + 2.f * q.w * q.y;
      r(0, 2) = 2.f * q.x * q.z - 2.f * q.w * q.y;
      r(1, 2) = 2.f * q.y * q.z + 2.f * q.w * q.x;
      r(2, 1) = 2.f * q.y * q.z - 2.f * q.w * q.x;
      #if 1
      //r = math::transpose(r);
      #endif
      return r;
    }

    inline float3x4 combine(Quaternion q, float3 pos) {
      q = normalize(q);
      auto r = float3x4::identity();
      float xx = std::pow(q.x, 2.f);
      float yy = std::pow(q.y, 2.f);
      float zz = std::pow(q.z, 2.f);
      r(0, 0) = 1.f - 2.f * yy - 2.f * zz;
      r(1, 1) = 1.f - 2.f * xx - 2.f * zz;
      r(2, 2) = 1.f - 2.f * xx - 2.f * yy;
      r(0, 1) = 2.f * q.x * q.y + 2.f * q.w * q.z;
      r(1, 0) = 2.f * q.x * q.y - 2.f * q.w * q.z;
      r(2, 0) = 2.f * q.x * q.z + 2.f * q.w * q.y;
      r(0, 2) = 2.f * q.x * q.z - 2.f * q.w * q.y;
      r(1, 2) = 2.f * q.y * q.z + 2.f * q.w * q.x;
      r(2, 1) = 2.f * q.y * q.z - 2.f * q.w * q.x;

      r(3, 0) = -pos.x;
      r(3, 1) = -pos.y;
      r(3, 2) = -pos.z;
      #if 1
      //r = math::transpose(r);
      #endif
      return r;

    }

    inline float4x4 inverse(float4x4 m) {
      float n11 = m(0,0), n12 = m(0,1), n13 = m(0,2), n14 = m(0,3);
      float n21 = m(1,0), n22 = m(1,1), n23 = m(1,2), n24 = m(1,3);
      float n31 = m(2,0), n32 = m(2,1), n33 = m(2,2), n34 = m(2,3);
      float n41 = m(3,0), n42 = m(3,1), n43 = m(3,2), n44 = m(3,3);

      float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
      float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
      float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
      float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

      float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
      float idet = 1.0f / det;

      float4x4 ret;

      ret(0,0) = t11 * idet;
      ret(1,0) = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
      ret(2,0) = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
      ret(3,0) = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

      ret(0,1) = t12 * idet;
      ret(1,1) = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
      ret(2,1) = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
      ret(3,1) = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

      ret(0,2) = t13 * idet;
      ret(1,2) = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
      ret(2,2) = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
      ret(3,2) = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

      ret(0,3) = t14 * idet;
      ret(1,3) = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
      ret(2,3) = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
      ret(3,3) = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;
      return ret;
    }
  }
}
using float2 = higanbana::math::Vector<2, float>;
using float3 = higanbana::math::Vector<3, float>;
using float4 = higanbana::math::Vector<4, float>;

using double2 = higanbana::math::Vector<2, double>;
using double3 = higanbana::math::Vector<3, double>;
using double4 = higanbana::math::Vector<4, double>;

using int2 = higanbana::math::Vector<2, int>;
using int3 = higanbana::math::Vector<3, int>;
using int4 = higanbana::math::Vector<4, int>;

using uint = unsigned;
using uint2 = higanbana::math::Vector<2, unsigned>;
using uint3 = higanbana::math::Vector<3, unsigned>;
using uint4 = higanbana::math::Vector<4, unsigned>;

using float3x3 = higanbana::math::Matrix<3, 3, float>;
using float4x4 = higanbana::math::Matrix<4, 4, float>;
using float3x4 = higanbana::math::Matrix<3, 4, float>;

using quaternion = higanbana::math::Quaternion;

namespace higanbana
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
    uint2 size() {
      return uint2(sub(rightBottom, leftTop));
    }

    constexpr auto operator==(const Rect& rhs) const {
      return leftTop == rhs.leftTop && rightBottom == rhs.rightBottom;
    }
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

    int3 size() {
      return int3(sub(rightBottomBack, leftTopFront));
    }
    constexpr auto operator==(const Box& rhs) const {
      return leftTopFront == rhs.leftTopFront && rightBottomBack == rhs.rightBottomBack;
    }
  };
#if defined(HIGANBANA_PLATFORM_WINDOWS)
}
#pragma warning( pop )
#endif