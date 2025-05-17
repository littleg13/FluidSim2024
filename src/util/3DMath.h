#ifndef MATH_H
#define MATH_H
#include <string>
namespace Math
{
#define PI 3.14159265358979323846
    struct Vec4;
    struct Matrix4x4;
    typedef Matrix4x4 Matrix;
    typedef Vec4 Vector4;
    struct alignas(float) Matrix4x4
    {
        union
        {
            struct
            {
                float m11, m21, m31, m41; // Column 1
                float m12, m22, m32, m42; // Column 2
                float m13, m23, m33, m43; // Column 3
                float m14, m24, m34, m44; // Column 4
            };
            struct
            {
                float m[16];
            };
        };

    public:
        Matrix4x4();
        Matrix4x4(float m[]);
        Matrix4x4(float m11, float m21, float m31, float m41, float m12, float m22, float m32, float m42, float m13, float m23, float m33, float m43, float m14, float m24, float m34, float m44);
        Matrix4x4 Mult(const Matrix4x4& B) const;
        Matrix4x4 Inverse() const;
        Matrix4x4 Transpose() const;
        float Determinant() const;
        Matrix4x4 operator+(const Matrix4x4& B) const;
        Matrix4x4 operator-(const Matrix4x4& B) const;
        Matrix4x4& operator+=(const Matrix4x4& B);
        Matrix4x4 operator*(const Matrix4x4& B) const;
        Matrix4x4 operator*(const float Scalar) const;
        Vec4 operator*(const Vec4& B) const;
        Vec4 GetRow(int I);
        Vec4 GetColumn(int I);
    };
    const Matrix4x4 Identity = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

    Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B);
    Matrix4x4 ViewMatrix(const Vec4& Forward, const Vec4& Up, const Vec4& EyePosition);
    Matrix4x4 PerspectiveMatrix(float FOV, float AspectRatio, float NearZ, float FarZ);
    Matrix4x4 RotateAboutAxis(const Vec4& Axis, float Angle);
    Matrix4x4 Rotate(const float Roll, const float Pitch, const float Yaw);
    Matrix4x4 Translate(const Vec4& Translation);
    Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B);
    Matrix4x4 TransformationMatrix(const Matrix4x4& Rotation, const Vec4& Translation, const Vec4& Scale);
    Matrix4x4 TransformationMatrix(const Matrix4x4& Rotation, const Vec4& Translation);

    struct alignas(float) Vec4
    {
        float x, y, z;
        float w = 1.0f;

    public:
        Vec4();
        Vec4(float X, float Y, float Z, float W);
        Vec4(float X, float Y, float Z);
        Vec4& Normalize();
        float Dot(const Vec4& B) const;
        Vec4 Cross(const Vec4& B) const;
        Matrix4x4 OuterProduct(const Vec4& B) const;
        Vec4 Subtract(const Vec4& B) const;
        Vec4 operator-(const Vec4& B) const;
        Vec4 operator+(const Vec4& B) const;
        Vec4& operator+=(const Vec4& B);
        Vec4 operator*(const Matrix4x4& B) const;
        Vec4 operator*(float Scalar) const;
        Vec4& operator/=(float Scalar);
        operator std::string() const
        {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
        };
    };

    template <typename T = float>
    struct Vec2
    {
        T x, y;

    public:
        Vec2();
        Vec2(T X, T Y);
        Vec2<T>& Normalize();
        Vec2<T> Subtract(const Vec2<T>& B) const;
        T Dot(const Vec2<T>& B) const;
        Vec2<T>& Pow(const int Power);
        Vec2<T> operator-(const Vec2<T>& B) const;
        Vec2<T> operator+(const Vec2<T>& B) const;
        Vec2<T>& operator+=(const Vec2<T>& B);
        Vec2<T> operator*(T Scalar);
    };

};
#endif