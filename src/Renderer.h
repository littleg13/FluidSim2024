#include <chrono>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ShaderCompiler.h"
#include "util/3DMath.h"
#include <d3d12.h>
#include <queue>
#include <vector>
#include <wrl.h>

class Allocation;
class DescriptorHeapAllocator;
class IDXGIAdapter4;
class IDXGISwapChain4;
class IDXGIFactory4;
class ObjectRenderer;
class Scene;
class View;

typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

typedef Microsoft::WRL::ComPtr<IDXGIAdapter4> IDXGIAdapterPtr;
typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;
typedef Microsoft::WRL::ComPtr<IDXGISwapChain4> IDXGISwapChainPtr;
typedef Microsoft::WRL::ComPtr<IDXGIFactory4> IDXGIFactoryPtr;

#define NUM_SWAP_BUFFERS 2

class Renderer
{
public:
    Renderer(ShaderCompiler& Compiler);
    ~Renderer();

    void Init(HWND HWnd, uint32_t Width, uint32_t Height);
    bool IsInitialized();
    void Render();
    void Resize(uint32_t Width, uint32_t Height);
    Math::Vec2<int32_t> GetClientDimensions();
    void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, ID3D12Resource* Resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, ID3D12Resource* Resource);
    ShaderCompiler& GetCompiler();

    std::unique_ptr<Allocation> GetAllocation();

    void UploadDefaultBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource>& BufferResource, Microsoft::WRL::ComPtr<ID3D12Resource>& UploadResource, size_t NumElements, size_t ElementSize, const void* BufferData, D3D12_RESOURCE_FLAGS Flags, size_t MinSize = 0);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(D3D12_HEAP_TYPE HeapType, UINT64 NumBytes, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE);

    void SetGraphicsRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature);
    void SetComputeRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> GetComputeRootSignature();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();
    uint32_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList);

    ID3D12DevicePtr GetDevice();
    void WaitForFenceValue(uint64_t FenceValue, std::chrono::milliseconds Duration = std::chrono::milliseconds::max());

    void Flush();

    Scene* GetCurrentScene();
    const Math::Matrix& GetInversePerspective();
    void SetCurrentScene(Scene* NewScene);
    void SetCurrentView(View* NewView);

private:
    void EnableDebugLayer();
    IDXGIAdapterPtr FindAdapter();
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12DevicePtr D3D12Device, D3D12_COMMAND_LIST_TYPE Type);
    IDXGISwapChainPtr CreateSwapChain(HWND HWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue, uint32_t Width, uint32_t Height, uint32_t BufferCount);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12DevicePtr Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    void UpdateRenderTargetViews(ID3D12DevicePtr Device, IDXGISwapChainPtr SwapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap, Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[]);
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12DevicePtr Device, D3D12_COMMAND_LIST_TYPE Type);
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12DevicePtr Device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator, D3D12_COMMAND_LIST_TYPE Type);
    ID3D12DevicePtr CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter);
    void WaitForFenceValue(Microsoft::WRL::ComPtr<ID3D12Fence> D3D12Fence, uint64_t FenceValue, HANDLE FenceEvent, std::chrono::milliseconds Duration = (std::chrono::milliseconds::max()));

    void CreateDepthBuffer();

    Scene* CurrentScene;

    ShaderCompiler& Compiler;

    IDXGIAdapterPtr DxgiAdapter;
    ID3D12DevicePtr D3D12Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12CommandQueue;
    IDXGISwapChainPtr DxgiSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12DescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthDescriptorHeap;
    DescriptorHeapAllocator* CBVSRVUAVHeapAllocator;
    Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[NUM_SWAP_BUFFERS];
    Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
    uint64_t FrameFenceValues[NUM_SWAP_BUFFERS];

    Microsoft::WRL::ComPtr<ID3D12RootSignature> GraphicsRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputeRootSignature;

    Microsoft::WRL::ComPtr<ID3D12Fence> D3D12Fence;
    HANDLE FenceEvent;
    uint64_t FenceValue = 0;

    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;

    struct CommandAllocatorTracker
    {
        uint64_t FenceValue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
    };
    std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> AvailableCommandLists;
    std::queue<CommandAllocatorTracker> TrackedCommandAllocators;

    Math::Matrix PerspectiveMatrix;
    Math::Matrix InvPerspectiveMatrix;
    View* CurrentView;

    uint32_t ClientWidth;
    uint32_t ClientHeight;
    bool Initialized = false;
};
