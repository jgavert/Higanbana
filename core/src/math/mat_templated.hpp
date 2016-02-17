#pragma once

#include "../tests/TestWorks.hpp"
#include "vec_templated.hpp"

namespace faze
{

  template <typename T, int rows, int cols>
  struct Matrix {
    std::array<Vector<cols, T>, rows> data;

    Matrix()
    {
      for (int i = 0; i < rows*cols; i++)
      {
        int row = i / 4;
        int column = i % 4;
        data[row][column] = 0.f;
      }
    }

    Matrix operator*(Matrix& scnd)
    {
      Matrix mat;

      for (int i = 0; i < rows*cols; i++)
      {
        int row = i / 4;
        int column = i % 4;
        mat.data[row][column] = data[row][0] * scnd.data[0][column] + data[row][1] * scnd.data[1][column] + data[row][2] * scnd.data[2][column] + data[row][3] * scnd.data[3][column];
      }
      return mat;
    }

    Matrix operator+(Matrix& scnd)
    {
      Matrix mat;
      for (int i = 0; i < rows; i++)
      {
        mat.data[i] = data[i] + scnd.data[i];
      }
      return mat;
    }
    Matrix operator-(Matrix& scnd)
    {
      Matrix mat;
      for (int i = 0; i < rows; i++)
      {
        mat.data[i] = data[i] - scnd.data[i];
      }
      return mat;
    }
    Matrix operator*(float scalar)
    {
      Matrix mat;
      for (int i = 0; i < rows; i++)
      {
        mat.data[i] = data[i] * scalar;
      }
      return mat;
    }
    Matrix operator/(float scalar)
    {
      return operator*(1.f / scalar);
    }
    Matrix operator+(float scalar)
    {
      Matrix mat;
      for (int i = 0; i < rows; i++)
      {
        mat.data[i] = data[i] + scalar;
      }
      return mat;
    }
    Matrix operator-(float scalar)
    {
      Matrix mat;
      for (int i = 0; i < rows; i++)
      {
        mat.data[i] = data[i] - scalar;
      }
      return mat;
    }
    Vector<cols, T> operator*(Vector<cols, T>& vec)
    {
      Vector<cols, T> nvec;
      for (int i = 0; i < rows; i++)
      {
        int row = i;
        nvec.data[i] = data[row][0] * vec.data[0] + data[row][1] * vec.data[1] + data[row][2] * vec.data[2] + data[row][3] * vec.data[3];
      }
      return nvec;
    }
    Matrix operator+(vec4& vec)
    {
      Matrix mat;
      for (int i = 0; i < 4; i++)
      {
        mat.data[i] = data[i] + vec.data[i];
      }
      return mat;
    }
    Matrix operator-(vec4& vec)
    {
      Matrix mat;
      for (int i = 0; i < 4; i++)
      {
        mat.data[i] = data[i] - vec.data[i];
      }
      return mat;
    }

    Vector<cols, T>& operator[](int i)
    {
      return data[i];
    }


    // TODO: oh fuck 
    static Matrix Identity()
    {
      Matrix mat;
      for (int i = 0; i < rows*cols; i++)
      {
        int row = i / 4;
        int column = i % 4;
        mat.data[row][column] = 0.f;
      }
      for (int i = 0; i < std::min(rows, cols); i++)
      {
        mat.data[i][i] = 1.f;
      }
      return mat;
    }

  };

  typedef Matrix<float, 4, 4> mat4;
  typedef Matrix<float, 3, 3> mat3;
#pragma warning( push )
#pragma warning( disable : 4505 ) // unreferenced local function has been removed
  namespace MatrixMath
  {
    const float PI = 3.14159265f;
    static mat4 Translation(float x, float y, float z)
    {
      mat4 result = mat4::Identity();
      result[0][3] = x; result[1][3] = y; result[2][3] = z;
      return result;
    }

    // x&y -> [-1, 1], z -> [0, 1], OK! Don't know which, rh or lh. but works now.
    static mat4 Perspective(float fov, float aspect, float NearZ, float FarZ)
    {
      if (fov <= 0 || aspect == 0)
      {
        return mat4();
      }
      
      float b = 1.0f / std::tanf(fov*(PI / 180.f)*0.5f);

      mat4 result;
      result[0][0] = aspect * b;
      result[1][1] = b;
      result[2][2] = FarZ / (FarZ - NearZ);
      result[3][2] = 1.f;
      result[2][3] = -1.f * NearZ * result[2][2];
      return result;
    }

    // perverse old z [-1, 1] for opengl, can just use the above and set correct clip in opengl.
    static mat4 PerspectiveOld(float fov, float aspect, float NearZ, float FarZ)
    {
      if (fov <= 0 || aspect == 0)
      {
        return mat4();
      }
      mat4 result;
      result[1][1] = 1.f / std::tan(fov * (PI / 180.f) / 2.f);
      result[0][0] = result.data[1][1] / aspect;
      result[2][2] = (FarZ + NearZ) / (NearZ - FarZ);
      result[3][2] = 2 * FarZ * NearZ / (NearZ - FarZ);
      result[2][3] = -1.f;
      return result;
    }

    // http://www.cs.virginia.edu/~gfx/Courses/1999/intro.fall99.html/lookat.html
    static mat4 lookAt(vec4 cameraPos, vec4 cameraDir)
    {
      vec4 asd1 = cameraDir - cameraPos;
      vec4 dirVec = asd1.normalize();
      vec4 upVec = { 0.f, 1.f, 0.f, 0.f };
      vec4 cross = dirVec.CrossProduct(upVec);
      vec4 side = cross.normalize();
      upVec = side.CrossProduct(dirVec);
      mat4 M = mat4::Identity();
      M[0][0] = side.data[0]; M[1][0] = side.data[1]; M[2][0] = side.data[2];
      M[0][1] = upVec.data[0]; M[1][1] = upVec.data[1]; M[2][1] = upVec.data[2];
      M[0][2] = -dirVec.data[0]; M[1][2] = -dirVec.data[1]; M[2][2] = -dirVec.data[2];
      mat4 temp = Translation(-cameraPos.data[0], -cameraPos.data[1], -cameraPos.data[2]);
      auto ret = temp*M;
      return ret;
    }

    static mat4 Rotation(float x, float y, float z)
    {
      mat4 result = mat4::Identity();
      float a = std::cos(x);
      float b = std::sin(x);
      float c = std::cos(y);
      float d = std::sin(y);
      float f = std::cos(z);
      float g = std::sin(z);
      // precalculated matrix multiplications
      result[0][0] = c*f;
      result[0][1] = c*(-g);
      result[0][2] = d;
      result[1][0] = (-b)*(-d)*f + a*g;
      result[2][1] = (-b)*(-d)*(-g) + a*f;
      result[3][2] = (-b)*c;
      result[2][0] = a*(-d)*f + b*g;
      result[2][1] = a*(-d)*(-g) + b*f;
      result[2][2] = a*c;
      return result;
    }

    static mat4 Rotation(Quaternion& quat)
    {
      mat4 retur = mat4::Identity();
      float xx = std::pow(quat[1], 2.f);
      float yy = std::pow(quat[2], 2.f);
      float zz = std::pow(quat[3], 2.f);
      retur[0][0] = 1.f - 2.f*yy - 2.f*zz;
      retur[1][1] = 1.f - 2.f*xx - 2.f*zz;
      retur[2][2] = 1.f - 2.f*xx - 2.f*yy;
      retur[0][1] = 2.f*quat[1] * quat[2] - 2.f*quat[0] * quat[3];
      retur[1][0] = 2.f*quat[1] * quat[2] + 2.f*quat[0] * quat[3];
      retur[2][0] = 2.f*quat[1] * quat[3] - 2.f*quat[0] * quat[2];
      retur[0][2] = 2.f*quat[1] * quat[3] + 2.f*quat[0] * quat[2];
      retur[2][2] = 2.f*quat[2] * quat[3] - 2.f*quat[0] * quat[1];
      retur[1][3] = 2.f*quat[2] * quat[3] + 2.f*quat[0] * quat[1];
      return retur;
    }
  };

#pragma warning( pop )
}
