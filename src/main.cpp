#define NOMINMAX 1
#define _DEBUG

#include <Windows.h>
#include <chrono>
#include <cstdlib>
#include <hidusage.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <wrl.h>

#include "Controller.h"
#include "PSOBuilder.h"
#include "Renderer.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "View.h"
#include "fluids/FluidObject.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"
#include "primitives/PrimitiveObject.h"
#include "primitives/Sphere.h"
#include "util/FileWatcher.h"
#include "util/RenderUtils.h"

#ifdef _DEBUG
const std::vector<LPTSTR> LiveCompileShaders = {
    TEXT("D:\\Dev\\Projects\\FluidSim2024\\shaders\\")};
#endif

ShaderDesc VertexShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\VertexShader.hlsl",
    L"vs_6_0",
    L"main"};
ShaderDesc FragmentShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\FragmentShader.hlsl",
    L"ps_6_0",
    L"main"};

// Window callback function.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

struct WindowUserData
{
    Renderer* D3D12Renderer;
    Controller* ViewController;
    RECT* OldCursorClip;
};

void RegisterWindowClass(HINSTANCE HInst, const wchar_t* WindowClassName)
{
    // Register a window class for creating our render window with.
    WNDCLASSEXW WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = HInst;
    WindowClass.hIcon = LoadIcon(HInst, NULL);
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = WindowClassName;
    WindowClass.hIconSm = LoadIcon(HInst, NULL);
    static ATOM Atom = RegisterClassExW(&WindowClass);
}

HWND CreateLocalWindow(const wchar_t* windowClassName, HINSTANCE HInst, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
    int ScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int ScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT WindowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    ::AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int WindowWidth = WindowRect.right - WindowRect.left;
    int WindowHeight = WindowRect.bottom - WindowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int WindowX = (ScreenWidth - WindowWidth) / 2;
    int WindowY = (ScreenHeight - WindowHeight) / 2;

    HWND HWnd = ::CreateWindowExW(
        NULL,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        WindowX,
        WindowY,
        WindowWidth,
        WindowHeight,
        NULL,
        NULL,
        HInst,
        nullptr);
    if (RenderUtils::CreateDialogOnLastError())
    {
        return NULL;
    }
    return HWnd;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WindowUserData* UserData = reinterpret_cast<WindowUserData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (UserData)
    {
        Controller* ViewController = UserData->ViewController;
        Renderer* D3D12Renderer = UserData->D3D12Renderer;
        RECT WindowRect;
        if (D3D12Renderer->IsInitialized())
        {
            switch (uMsg)
            {
            case WM_PAINT:
                break;
            case WM_INPUT:
            {
                UINT dwSize = sizeof(RAWINPUT);
                static BYTE lpb[sizeof(RAWINPUT)];

                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    ViewController->HandleMouseMove(wParam, raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                }
                break;
            }
            case WM_LBUTTONDOWN:
                SetCapture(hwnd);
                GetWindowRect(hwnd, &WindowRect);
                ClipCursor(&WindowRect);
                ShowCursor(false);
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                ViewController->HandleMouseButton(wParam, LOWORD(lParam), HIWORD(lParam));
                break;
            case WM_LBUTTONUP:
                ReleaseCapture();
                ClipCursor(UserData->OldCursorClip);
                GetWindowRect(hwnd, &WindowRect);
                SetCursorPos((WindowRect.right - WindowRect.left) / 2 + WindowRect.left, (WindowRect.bottom - WindowRect.top) / 2 + WindowRect.top);
                ShowCursor(true);
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
                ViewController->HandleMouseRelease(wParam, LOWORD(lParam), HIWORD(lParam));
                break;
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                WORD KeyFlags = HIWORD(lParam);
                ViewController->HandleKeyPress(wParam, KeyFlags & KF_REPEAT);
                switch (wParam)
                {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_F5:
                    D3D12Renderer->Flush();
                    D3D12Renderer->GetCurrentScene()->ReloadShaders();
                    break;
                }
                break;
            }
            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                ViewController->HandleKeyRelease(wParam);
                break;
            }
            case WM_SIZE:
            {
                RECT ClientRect = {};
                ::GetClientRect(hwnd, &ClientRect);

                int width = ClientRect.right - ClientRect.left;
                int height = ClientRect.bottom - ClientRect.top;

                D3D12Renderer->Resize(width, height);
            }
            break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
            }
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE HInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    int ClientWidth = 1920;
    int ClientHeight = 1080;

    HANDLE KillThreadsEvent = CreateEventW(NULL, false, false, L"KillThreads");

    RegisterWindowClass(HInstance, L"FluidSim");
    HWND HWnd = CreateLocalWindow(L"FluidSim", HInstance, L"FluidSim", ClientWidth, ClientHeight);

    // Register device for getting Raw input. Enables more precise mouse tracking from WM_INPUT
    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    Rid[0].dwFlags = RIDEV_INPUTSINK;
    Rid[0].hwndTarget = HWnd;
    RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

    srand(static_cast<unsigned>(time(0)));

    // Attach debugger
    // MessageBox(NULL, TEXT("Pausing to attach debugger"), TEXT("Attach debugger"), MB_OK);
    ShaderCompiler Compiler;

    Renderer* D3D12Renderer = new Renderer(Compiler);
    D3D12Renderer->Init(HWnd, ClientWidth, ClientHeight);

    PSOBuilder GraphicsRootSignatureBuilder;
    GraphicsRootSignatureBuilder.AddConstantRootParameter(32);
    D3D12_DESCRIPTOR_RANGE1 Ranges[1];
    Ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    GraphicsRootSignatureBuilder.AddDescriptorTableRootParameter(1, Ranges);
    D3D12Renderer->SetGraphicsRootSignature(GraphicsRootSignatureBuilder.BuildGraphicsRootSignature(D3D12Renderer->GetDevice()));

    PSOBuilder ComputeRootSignatureBuilder;
    D3D12_DESCRIPTOR_RANGE1 ComputeRanges[2];
    ComputeRanges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ComputeRanges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ComputeRootSignatureBuilder.AddDescriptorTableRootParameter(2, ComputeRanges);
    D3D12Renderer->SetComputeRootSignature(ComputeRootSignatureBuilder.BuildComputeRootSignature(D3D12Renderer->GetDevice()));

    Scene* MainScene = new Scene(D3D12Renderer->GetDevice(), Compiler);
    Controller* ViewController = new Controller(D3D12Renderer);
    View* MainView = ViewController->GetCurrentView();

    D3D12Renderer->SetCurrentScene(MainScene);
    D3D12Renderer->SetCurrentView(MainView);
    Sphere::GenerateSphereData();

    FluidObject* Fluid = new FluidObject();
    Fluid->CreateBuffers(D3D12Renderer);
    MainScene->AddObject(Fluid);

    PrimitiveObject<Plane>* Object = new PrimitiveObject<Plane>();
    Object->ApplyTranslation(Math::Vec4(0.5f, 0.5f, 0.1));
    Object->ApplyRotation(Math::RotateAboutAxis(Math::Vec4(1, 0, 0), 90));
    Object->CreateBuffers(D3D12Renderer);
    MainScene->AddObject(Object);

    RECT OldCursorClip;
    GetClipCursor(&OldCursorClip);

    WindowUserData* UserData = new WindowUserData();
    UserData->D3D12Renderer = D3D12Renderer;
    UserData->ViewController = ViewController;
    UserData->OldCursorClip = &OldCursorClip;
    SetWindowLongPtr(HWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(UserData));

    ShowWindow(HWnd, SW_SHOW);

    bool ShadersUpdated = false;
#ifdef _DEBUG
    auto LiveCompileLambda = [HWnd, KillThreadsEvent, &Compiler, &ShadersUpdated]()
    {
        FileWatcher::WatchAnyFiles(
            LiveCompileShaders,
            [HWnd, &Compiler, &ShadersUpdated]()
            {
                auto Callback = [&ShadersUpdated](bool DidCompile)
                { ShadersUpdated = DidCompile; };
                Compiler.CompileShader(VertexShader, Callback, false);
                Compiler.CompileShader(FragmentShader, Callback, false);
            },
            KillThreadsEvent);
    };
    std::thread LiveCompileThread(LiveCompileLambda);
#endif

    // 500 Updates per second
    std::chrono::milliseconds UpdateFrequency{2};
    double DeltaTime = UpdateFrequency.count() * 1e-3;
    std::chrono::time_point LastUpdate = std::chrono::high_resolution_clock::now();
    MSG Msg = {};
    while (Msg.message != WM_QUIT)
    {
        if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
        std::chrono::time_point Now = std::chrono::high_resolution_clock::now();
        if (LastUpdate + UpdateFrequency <= Now)
        {
#ifdef _DEBUG
            if (Now - LastUpdate > 1.1 * UpdateFrequency)
            {
                // OutputDebugString(TEXT(("Warning: Frame took longer than 110% of update frequency. Update time: " + std::to_string((Now - LastUpdate).count() * 1e-3) + "\n").c_str()));
            }
#endif
            LastUpdate = Now;

            MainScene->Update(DeltaTime);
            ViewController->Update(DeltaTime);
        }
        D3D12Renderer->Render();
        if (ShadersUpdated)
        {
            D3D12Renderer->Flush();
            MainScene->ReloadShaders();
            ShadersUpdated = false;
        }
    }

    delete D3D12Renderer;
    delete ViewController;
    delete MainScene;
    delete UserData;
    SetEvent(KillThreadsEvent);

#ifdef _DEBUG
    LiveCompileThread.join();
#endif
    //_CrtDumpMemoryLeaks();
    return 0;
}