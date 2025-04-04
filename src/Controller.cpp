#include "Controller.h"

#include <stdint.h>
#include <string>

#include "Renderer.h"
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
}

void Controller::HandleMouseButton(uint64_t wParam, int X, int Y)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    if (lButton)
    {
        Panning = true;
    }
}
void Controller::HandleMouseRelease(uint64_t wParam, int X, int Y)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    if (!lButton)
    {
        Panning = false;
    }
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