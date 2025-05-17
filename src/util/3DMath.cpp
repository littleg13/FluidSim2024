#include "3DMath.h"

#include <Windows.h>
#include <cmath>
#include <stdint.h>

namespace Math
{
    Matrix4x4::Matrix4x4()
        : m11(0), m12(0), m13(0), m14(0), m21(0), m22(0), m23(0), m24(0), m31(0), m32(0), m33(0), m34(0), m41(0), m42(0), m43(0), m44(0)
    {
    }

    Matrix4x4::Matrix4x4(float m[])
        : m11(m[0]), m21(m[1]), m31(m[2]), m41(m[3]), m12(m[4]), m22(m[5]), m32(m[6]), m42(m[7]), m13(m[8]), m23(m[9]), m33(m[10]), m43(m[11]), m14(m[12]), m24(m[13]), m34(m[14]), m44(m[15])
    {
    }

    Matrix4x4::Matrix4x4(float m11, float m21, float m31, float m41, float m12, float m22, float m32, float m42, float m13, float m23, float m33, float m43, float m14, float m24, float m34, float m44)
        : m11(m11), m21(m21), m31(m31), m41(m41), m12(m12), m22(m22), m32(m32), m42(m42), m13(m13), m23(m23), m33(m33), m43(m43), m14(m14), m24(m24), m34(m34), m44(m44)
    {
    }

    Matrix4x4 Matrix4x4::Mult(const Matrix4x4& B) const
    {
        return {
            m11 * B.m11 + m12 * B.m21 + m13 * B.m31 + m14 * B.m41,
            m21 * B.m11 + m22 * B.m21 + m23 * B.m31 + m24 * B.m41,
            m31 * B.m11 + m32 * B.m21 + m33 * B.m31 + m34 * B.m41,
            m41 * B.m11 + m42 * B.m21 + m43 * B.m31 + m44 * B.m41, // End column 1
            m11 * B.m12 + m12 * B.m22 + m13 * B.m32 + m14 * B.m42,
            m21 * B.m12 + m22 * B.m22 + m23 * B.m32 + m24 * B.m42,
            m31 * B.m12 + m32 * B.m22 + m33 * B.m32 + m34 * B.m42,
            m41 * B.m12 + m42 * B.m22 + m43 * B.m32 + m44 * B.m42, // End column 2
            m11 * B.m13 + m12 * B.m23 + m13 * B.m33 + m14 * B.m43,
            m21 * B.m13 + m22 * B.m23 + m23 * B.m33 + m24 * B.m43,
            m31 * B.m13 + m32 * B.m23 + m33 * B.m33 + m34 * B.m43,
            m41 * B.m13 + m42 * B.m23 + m43 * B.m33 + m44 * B.m43, // End column 3
            m11 * B.m14 + m12 * B.m24 + m13 * B.m34 + m14 * B.m44,
            m21 * B.m14 + m22 * B.m24 + m23 * B.m34 + m24 * B.m44,
            m31 * B.m14 + m32 * B.m24 + m33 * B.m34 + m34 * B.m44,
            m41 * B.m14 + m42 * B.m24 + m43 * B.m34 + m44 * B.m44 // End column 4
        };
    }

    Matrix4x4 Matrix4x4::Inverse() const
    {
        float A2323 = m33 * m44 - m34 * m43;
        float A1323 = m32 * m44 - m34 * m42;
        float A1223 = m32 * m43 - m33 * m42;
        float A0323 = m31 * m44 - m34 * m41;
        float A0223 = m31 * m43 - m33 * m41;
        float A0123 = m31 * m42 - m32 * m41;
        float A2313 = m23 * m44 - m24 * m43;
        float A1313 = m22 * m44 - m24 * m42;
        float A1213 = m22 * m43 - m23 * m42;
        float A2312 = m23 * m34 - m24 * m33;
        float A1312 = m22 * m34 - m24 * m32;
        float A1212 = m22 * m33 - m23 * m32;
        float A0313 = m21 * m44 - m24 * m41;
        float A0213 = m21 * m43 - m23 * m41;
        float A0312 = m21 * m34 - m24 * m31;
        float A0212 = m21 * m33 - m23 * m31;
        float A0113 = m21 * m42 - m22 * m41;
        float A0112 = m21 * m32 - m22 * m31;

        float det = m11 * (m22 * A2323 - m23 * A1323 + m24 * A1223) - m12 * (m21 * A2323 - m23 * A0323 + m24 * A0223) + m13 * (m21 * A1323 - m22 * A0323 + m24 * A0123) - m14 * (m21 * A1223 - m22 * A0223 + m23 * A0123);
        if (std::abs(det) <= 0.1)
        {
            OutputDebugString(TEXT("Failed to find the inverse matrix"));
        }
        det = 1 / det;

        return {
            det * (m22 * A2323 - m23 * A1323 + m24 * A1223),  // m11
            det * -(m21 * A2323 - m23 * A0323 + m24 * A0223), // m21
            det * (m21 * A1323 - m22 * A0323 + m24 * A0123),  // m31
            det * -(m21 * A1223 - m22 * A0223 + m23 * A0123), // m41
            det * -(m12 * A2323 - m13 * A1323 + m14 * A1223), // m12
            det * (m11 * A2323 - m13 * A0323 + m14 * A0223),  // m22
            det * -(m11 * A1323 - m12 * A0323 + m14 * A0123), // m32
            det * (m11 * A1223 - m12 * A0223 + m13 * A0123),  // m42
            det * (m12 * A2313 - m13 * A1313 + m14 * A1213),  // m13
            det * -(m11 * A2313 - m13 * A0313 + m14 * A0213), // m23
            det * (m11 * A1313 - m12 * A0313 + m14 * A0113),  // m33
            det * -(m11 * A1213 - m12 * A0213 + m13 * A0113), // m43
            det * -(m12 * A2312 - m13 * A1312 + m14 * A1212), // m14
            det * (m11 * A2312 - m13 * A0312 + m14 * A0212),  // m24
            det * -(m11 * A1312 - m12 * A0312 + m14 * A0112), // m34
            det * (m11 * A1212 - m12 * A0212 + m13 * A0112)}; // m44
    }

    Matrix4x4 Matrix4x4::Transpose() const
    {
        return {
            m11, m12, m13, m14,
            m21, m22, m23, m24,
            m31, m32, m33, m34,
            m41, m42, m43, m44};
    }

    float Matrix4x4::Determinant() const
    {
        float A2323 = m33 * m44 - m34 * m43;
        float A1323 = m32 * m44 - m34 * m42;
        float A1223 = m32 * m43 - m33 * m42;
        float A0323 = m31 * m44 - m34 * m41;
        float A0223 = m31 * m43 - m33 * m41;
        float A0123 = m31 * m42 - m32 * m41;
        return m11 * (m22 * A2323 - m23 * A1323 + m24 * A1223) - m12 * (m21 * A2323 - m23 * A0323 + m24 * A0223) + m13 * (m21 * A1323 - m22 * A0323 + m24 * A0123) - m14 * (m21 * A1223 - m22 * A0223 + m23 * A0123);
    }

    Matrix4x4 Matrix4x4::operator+(const Matrix4x4& B) const
    {
        return {
            m[0] + B.m[0],
            m[1] + B.m[1],
            m[2] + B.m[2],
            m[3] + B.m[3],
            m[4] + B.m[4],
            m[5] + B.m[5],
            m[6] + B.m[6],
            m[7] + B.m[7],
            m[8] + B.m[8],
            m[9] + B.m[9],
            m[10] + B.m[10],
            m[11] + B.m[11],
            m[12] + B.m[12],
            m[13] + B.m[13],
            m[14] + B.m[14],
            m[15] + B.m[15]};
    }

    Matrix4x4 Matrix4x4::operator-(const Matrix4x4& B) const
    {
        return {
            m[0] - B.m[0],
            m[1] - B.m[1],
            m[2] - B.m[2],
            m[3] - B.m[3],
            m[4] - B.m[4],
            m[5] - B.m[5],
            m[6] - B.m[6],
            m[7] - B.m[7],
            m[8] - B.m[8],
            m[9] - B.m[9],
            m[10] - B.m[10],
            m[11] - B.m[11],
            m[12] - B.m[12],
            m[13] - B.m[13],
            m[14] - B.m[14],
            m[15] - B.m[15]};
    }

    Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& B)
    {
        for (int i = 0; i < 16; i++)
        {
            m[i] += B.m[i];
        }
        return *this;
    }

    Matrix4x4 Matrix4x4::operator*(const Matrix4x4& B) const
    {
        return Mult(B);
    }

    Matrix4x4 Matrix4x4::operator*(const float Scalar) const
    {
        return {
            m[0] * Scalar,
            m[1] * Scalar,
            m[2] * Scalar,
            m[3] * Scalar,
            m[4] * Scalar,
            m[5] * Scalar,
            m[6] * Scalar,
            m[7] * Scalar,
            m[8] * Scalar,
            m[9] * Scalar,
            m[10] * Scalar,
            m[11] * Scalar,
            m[12] * Scalar,
            m[13] * Scalar,
            m[14] * Scalar,
            m[15] * Scalar};
    }

    Vec4 Matrix4x4::GetRow(int I)
    {
        return {m[I * 4], m[(I + 1) * 4], m[(I + 2) * 4], m[(I + 3) * 4]};
    }
    Vec4 Matrix4x4::GetColumn(int I)
    {
        return {m[I * 4], m[I * 4 + 1], m[I * 4 + 2], m[I * 4 + 3]};
    }

    float Vec4::Dot(const Vec4& B) const
    {
        return x * B.x + y * B.y + z * B.z;
    }

    Vec4::Vec4()
        : x(0), y(0), z(0), w(1) {}

    Vec4::Vec4(float X, float Y, float Z, float W)
        : x(X), y(Y), z(Z), w(W) {}

    Vec4::Vec4(float X, float Y, float Z)
        : x(X), y(Y), z(Z), w(1) {}

    Vec4 Vec4::Cross(const Vec4& B) const
    {
        return {
            y * B.z - z * B.y,
            -1 * x * B.z + z * B.x,
            x * B.y - y * B.x,
            1.0f};
    }

    Matrix4x4 Vec4::OuterProduct(const Vec4& B) const
    {
        return {
            x * B.x,
            x * B.y,
            x * B.z,
            x * B.w,
            y * B.x,
            y * B.y,
            y * B.z,
            y * B.w,
            z * B.x,
            z * B.y,
            z * B.z,
            z * B.w,
            w * B.x,
            w * B.y,
            w * B.z,
            w * B.w,
        };
    }

    Vec4& Vec4::Normalize()
    {
        float Magnitude = std::sqrt(x * x + y * y + z * z);
        x = x / Magnitude;
        y = y / Magnitude;
        z = z / Magnitude;
        return *this;
    }

    Vec4 Vec4::Subtract(const Vec4& B) const
    {
        return {x - B.x, y - B.y, z - B.z, w};
    }

    Vec4 Vec4::operator-(const Vec4& B) const
    {
        return Subtract(B);
    }

    Vec4 Vec4::operator+(const Vec4& B) const
    {
        return {x + B.x, y + B.y, z + B.z, w};
    }

    Vec4& Vec4::operator+=(const Vec4& B)
    {
        x += B.x;
        y += B.y;
        z += B.z;
        return *this;
    }

    Vec4 Matrix4x4::operator*(const Vec4& B) const
    {
        return {
            m11 * B.x + m12 * B.y + m13 * B.z + m14 * B.w,
            m21 * B.x + m22 * B.y + m23 * B.z + m24 * B.w,
            m31 * B.x + m32 * B.y + m33 * B.z + m34 * B.w,
            m41 * B.x + m42 * B.y + m43 * B.z + m44 * B.w,
        };
    }

    Vec4 Vec4::operator*(const Matrix4x4& B) const
    {
        return {
            x * B.m11 + y * B.m21 + z * B.m31 + w * B.m41,
            x * B.m12 + y * B.m22 + z * B.m32 + w * B.m42,
            x * B.m13 + y * B.m23 + z * B.m33 + w * B.m43,
            x * B.m14 + y * B.m24 + z * B.m34 + w * B.m44,
        };
    }

    Vec4 Vec4::operator*(float Scalar) const
    {
        return {x * Scalar, y * Scalar, z * Scalar, w};
    }

    Vec4& Vec4::operator/=(float Scalar)
    {
        x /= Scalar;
        y /= Scalar;
        z /= Scalar;
        w /= Scalar;
        return *this;
    }

    Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B)
    {
        return A.Mult(B);
    }

    Matrix4x4 ViewMatrix(const Vec4& Forward, const Vec4& Up, const Vec4& EyePosition)
    {
        Vec4 Right = Up.Cross(Forward).Normalize();
        Vec4 ForcedUp = Forward.Cross(Right).Normalize();
        return {
            Right.x, ForcedUp.x, Forward.x, 0,
            Right.y, ForcedUp.y, Forward.y, 0,
            Right.z, ForcedUp.z, Forward.z, 0,
            -1 * Right.Dot(EyePosition), -1 * ForcedUp.Dot(EyePosition), -1 * Forward.Dot(EyePosition), 1};
    }

    Matrix4x4 PerspectiveMatrix(float FOV, float AspectRatio, float NearZ, float FarZ)
    {
        float Theta = FOV * PI / 360;
        float CotTheta = cos(Theta) / sin(Theta);
        float ScaledFarDepth = FarZ / (FarZ - NearZ);
        return {
            CotTheta / AspectRatio, 0, 0, 0,
            0, CotTheta, 0, 0,
            0, 0, ScaledFarDepth, 1,
            0, 0, -1 * NearZ * ScaledFarDepth, 0};
    }

    Matrix4x4 RotateAboutAxis(const Vec4& A, float Angle)
    {
        float Theta = Angle * PI / 180;
        float C = cos(Theta);
        float OMC = 1 - C;
        float S = sin(Theta);
        return {
            C + A.x * A.x * OMC, A.y * A.x * OMC + A.z * S, A.z * A.x * OMC - A.y * S, 0,
            A.x * A.y * OMC - A.z * S, C + A.y * A.y * OMC, A.y * A.z * OMC - A.x * S, 0,
            A.x * A.z * OMC + A.y * S, A.y * A.z * OMC - A.x * S, C + A.z * A.z * OMC, 0,
            0, 0, 0, 1};
    }

    Matrix4x4 Rotate(const float Roll, const float Pitch, const float Yaw)
    {
        float CR = cos(Roll * PI / 180);
        float SR = sin(Roll * PI / 180);
        float CP = cos(Pitch * PI / 180);
        float SP = sin(Pitch * PI / 180);
        float CY = cos(Yaw * PI / 180);
        float SY = sin(Yaw * PI / 180);
        return {
            CP * CY, CP * SY, -SP, 0,
            SR * SP * CY - CR * SY, SR * SP * SY + CR * CY, SR * CP, 0,
            CR * SP * CY + SR * SY, CR * SP * SY - SR * CY, CR * CP, 0,
            0, 0, 0, 1};
    }

    Matrix4x4 Translate(const Vec4& Translation)
    {
        return {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            Translation.x, Translation.y, Translation.z, 1};
    }

    Matrix4x4 TransformationMatrix(const Matrix4x4& Rotation, const Vec4& Translation, const Vec4& Scale)
    {
        return {
            Rotation.m11 * Scale.x, Rotation.m21, Rotation.m31, Rotation.m41,
            Rotation.m12, Rotation.m22 * Scale.y, Rotation.m32, Rotation.m42,
            Rotation.m13, Rotation.m23, Rotation.m33 * Scale.z, Rotation.m43,
            Translation.x, Translation.y, Translation.z, 1};
    }

    Matrix4x4 TransformationMatrix(const Matrix4x4& Rotation, const Vec4& Translation)
    {
        return {
            Rotation.m11, Rotation.m21, Rotation.m31, Rotation.m41,
            Rotation.m12, Rotation.m22, Rotation.m32, Rotation.m42,
            Rotation.m13, Rotation.m23, Rotation.m33, Rotation.m43,
            Translation.x, Translation.y, Translation.z, 1};
    }

    template <typename T>
    Vec2<T>::Vec2()
        : x(0), y(0)
    {
    }

    template <typename T>
    Vec2<T>::Vec2(T X, T Y)
        : x(X), y(Y)
    {
    }

    template <typename T>
    Vec2<T>& Vec2<T>::Normalize()
    {
        float Magnitude = std::sqrt(x * x + y * y);
        x = x / Magnitude;
        y = y / Magnitude;
        return *this;
    }

    template <typename T>
    Vec2<T> Vec2<T>::Subtract(const Vec2<T>& B) const
    {
        return {x - B.x, y - B.y};
    }

    template <typename T>
    T Vec2<T>::Dot(const Vec2<T>& B) const
    {
        return x * B.x + y * B.y;
    }

    template <typename T>
    Vec2<T>& Vec2<T>::Pow(const int Power)
    {
        x = pow(x, Power);
        y = pow(y, Power);
        return *this;
    }

    template <>
    Vec2<int32_t>& Vec2<int32_t>::Pow(const int Power)
    {
        for (int i = 0; i < Power - 1; i++)
        {
            x *= x;
            y *= y;
        }
        return *this;
    }

    template <typename T>
    Vec2<T> Vec2<T>::operator-(const Vec2<T>& B) const
    {
        return Subtract(B);
    }

    template <typename T>
    Vec2<T> Vec2<T>::operator+(const Vec2<T>& B) const
    {
        return {x + B.x, y + B.y};
    }

    template <typename T>
    Vec2<T>& Vec2<T>::operator+=(const Vec2<T>& B)
    {
        x += B.x;
        y += B.y;
        return *this;
    }

    template <typename T>
    Vec2<T> Vec2<T>::operator*(T Scalar)
    {
        return {x * Scalar, y * Scalar};
    }

    template class Vec2<float>;
    template class Vec2<int32_t>;
};