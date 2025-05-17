#include "View.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <windows.h>

View::View()
    : Forward{0, 0, 1, 0}, Up{0, 1, 0, 0}, Right(Up.Cross(Forward)), EyePos{0.5f, 0.5f, -1, 1}, ViewMatrix(Math::ViewMatrix(Forward, Up, EyePos))
{
}

View::View(Math::Vec4& Forward, Math::Vec4& Up, Math::Vec4& EyePos)
    : Forward(Forward), Up(Up), EyePos(EyePos), ViewMatrix(Math::ViewMatrix(Forward, Up, EyePos))
{
}

const Math::Matrix& View::GetMatrix()
{
    return ViewMatrix;
}

const Math::Matrix& View::GetInverse()
{
    if (InverseDirty)
    {
        InverseViewMatrix = ViewMatrix.Inverse();
        InverseDirty = false;
    }
    return InverseViewMatrix;
}

const Math::Vec4& View::GetEyePos()
{
    return EyePos;
}

void View::Rotate(const float NewPitch, const float NewYaw)
{
    // Update Yaw (rotation around Up axis)
    Pitch = std::clamp(NewPitch + Pitch, -90.0f, 90.0f);
    Yaw += NewYaw;
    float CP = cos(Pitch * PI / 180);
    float CY = cos((Yaw + 90) * PI / 180);
    float SY = sin((Yaw + 90) * PI / 180);
    float SP = sin(Pitch * PI / 180);
    Forward = Math::Vec4(CY * CP, SP, SY * CP);
    Forward.Normalize();
    ViewMatrix = Math::ViewMatrix(Forward, Up, EyePos);
    InverseDirty = true;
}

void View::Translate(const float X, const float Y, const float Z)
{
    Math::Vec4 Translation = Math::Vec4(X, Y, Z);
    EyePos += Forward * Translation.z + Up.Cross(Forward).Normalize() * Translation.x;
    ViewMatrix = Math::ViewMatrix(Forward, Up, EyePos);
    InverseDirty = true;
}