#pragma once

#include "util/3DMath.h"
#include "util/Singleton.h"
#include <stdint.h>
#include <vector>

class View;
class Renderer;
class Scene;

#define MOUSE_SENSITIVITY 0.1
#define VIEW_MOVE_SPEED 8

class Controller : public Singleton<Controller>
{
public:
    Controller(Renderer* renderer);
    ~Controller();

    View* AddView();
    View* GetCurrentView();

    const Math::Vec4& GetProjectedMousePosition();
    bool IsRightMouseDown();

    void SetCurrentScene(Scene* NewScene);

    void Update(double DeltaTime);

    void HandleMouseMove(uint64_t wParam, int X, int Y);
    void HandleMouseButton(uint64_t wParam, int X, int Y);
    void HandleMouseRelease(uint64_t wParam, int X, int Y);
    void HandleKeyPress(uint64_t wParam, bool isRepeat);
    void HandleKeyRelease(uint64_t wParam);

private:
    Renderer* D3D12Renderer;
    Scene* CurrentScene;
    std::vector<View*> Views;
    int CurrentViewIndex = 0;

    int CurrentMouseX, CurrentMouseY = 0;
    Math::Vec4 ProjectedMousePos;
    bool Panning = false;
    bool Grabbing = false;
    Math::Vec4 ViewVel = Math::Vec4(0, 0, 0);
};