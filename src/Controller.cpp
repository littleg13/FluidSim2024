#include "Controller.h"

#include <stdint.h>
#include <string>

#include "Renderer.h"
#include "Scene.h"
#include "View.h"

Controller::Controller(Renderer* renderer)
    : D3D12Renderer(renderer)
{
    // Add default view
    D3D12Renderer->SetCurrentView(AddView());
}

Controller::~Controller()
{
    for (View* view : Views)
    {
        delete view;
    }
}

void Controller::SetCurrentScene(Scene* NewScene)
{
    CurrentScene = NewScene;
    D3D12Renderer->SetCurrentScene(NewScene);
}

View* Controller::AddView()
{
    Views.emplace_back(new View());
    return Views.back();
}

View* Controller::GetCurrentView()
{
    return Views[CurrentViewIndex];
}

void Controller::HandleMouseMove(uint64_t wParam, int X, int Y)
{
    if (Panning)
    {
        GetCurrentView()->Rotate(-Y * MOUSE_SENSITIVITY, -X * MOUSE_SENSITIVITY);
    }
    if (Grabbing)
    {
        Math::Vec2<int32_t> ClientDimensions = D3D12Renderer->GetClientDimensions();
        CurrentMouseX = std::clamp(CurrentMouseX + X, 0, ClientDimensions.x);
        CurrentMouseY = std::clamp(CurrentMouseY + Y, 0, ClientDimensions.y);
    }
}

const Math::Vec4& Controller::GetProjectedMousePosition()
{
    Math::Vec2<int32_t> ClientDimensions = D3D12Renderer->GetClientDimensions();
    Math::Vec4 ClipPos = Math::Vec4(CurrentMouseX * 2 / float(ClientDimensions.x) - 1.0f, -CurrentMouseY * 2.0f / float(ClientDimensions.y) + 1.0f, 0.1f, 1.0f);
    Math::Vec4 ViewSpacePos = D3D12Renderer->GetInversePerspective() * ClipPos;
    ViewSpacePos /= ViewSpacePos.w;
    OutputDebugString(TEXT(((std::string)ViewSpacePos + " - ViewSpacePos\n").c_str()));
    ProjectedMousePos = (GetCurrentView()->GetInverse() * ViewSpacePos) - GetCurrentView()->GetEyePos();
    OutputDebugString(TEXT(((std::string)(ProjectedMousePos.Normalize() * 2.0f + GetCurrentView()->GetEyePos()) + "\n").c_str()));
    ProjectedMousePos = ProjectedMousePos.Normalize() * 2.0f + GetCurrentView()->GetEyePos();
    return ProjectedMousePos;
}

bool Controller::IsRightMouseDown()
{
    return Grabbing;
}

void Controller::HandleMouseButton(uint64_t wParam, int X, int Y)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    if (lButton)
    {
        Panning = true;
    }
    if (rButton)
    {
        Grabbing = true;
    }
    CurrentMouseX = X;
    CurrentMouseY = Y;
}

void Controller::HandleMouseRelease(uint64_t wParam, int X, int Y)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    if (!lButton)
    {
        Panning = false;
    }
    if (!rButton)
    {
        Grabbing = false;
    }
    CurrentMouseX = X;
    CurrentMouseY = Y;
}

void Controller::HandleKeyPress(uint64_t wParam, bool isRepeat)
{
    if (!isRepeat)
    {
        switch (wParam)
        {
        case 0x57: // W
            ViewVel.z += VIEW_MOVE_SPEED;
            break;
        case 0x41: // A
            ViewVel.x -= VIEW_MOVE_SPEED;
            break;
        case 0x53: // S
            ViewVel.z -= VIEW_MOVE_SPEED;
            break;
        case 0x44: // D
            ViewVel.x += VIEW_MOVE_SPEED;
            break;
        }
    }
    if (CurrentScene)
    {
        CurrentScene->HandleKeyPress(wParam, isRepeat);
    }
}

void Controller::HandleKeyRelease(uint64_t wParam)
{
    switch (wParam)
    {
    case 0x57: // W
        ViewVel.z -= VIEW_MOVE_SPEED;
        break;
    case 0x41: // A
        ViewVel.x += VIEW_MOVE_SPEED;
        break;
    case 0x53: // S
        ViewVel.z += VIEW_MOVE_SPEED;
        break;
    case 0x44: // D
        ViewVel.x -= VIEW_MOVE_SPEED;
        break;
    }
}

void Controller::Update(double DeltaTime)
{
    GetCurrentView()->Translate(ViewVel.x * DeltaTime, ViewVel.y * DeltaTime, ViewVel.z * DeltaTime);
}