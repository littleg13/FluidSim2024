#pragma once

#include "util/3DMath.h"

class View
{
public:
    View();
    View(Math::Vec4& Forward, Math::Vec4& Up, Math::Vec4& EyePos);

    const Math::Matrix& GetMatrix();
    const Math::Matrix& GetInverse();

    const Math::Vec4& GetEyePos();

    void Rotate(const float Pitch, const float Yaw);
    void Translate(const float X, const float Y, const float Z);

private:
    Math::Vec4 Forward;
    Math::Vec4 Up;
    Math::Vec4 Right;
    Math::Vec4 EyePos;

    float Roll = 0.0f;
    float Pitch = 0.0f;
    float Yaw = 0.0f;
    Math::Matrix ViewMatrix;

    bool InverseDirty = true;
    Math::Matrix InverseViewMatrix;
};